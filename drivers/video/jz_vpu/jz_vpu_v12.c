
#include <linux/vmalloc.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/wakelock.h>
#include <linux/clk.h>
#include <linux/syscalls.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/mman.h>
#include <linux/pfn.h>
#include <jz_notifier.h>

#include <linux/fs.h>
#include <soc/base.h>
#include <soc/cpm.h>

#include <mach/jzcpm_pwc.h>
#ifdef CONFIG_SOC_M200
#include <mach/libdmmu.h>
#endif

#include "jz_vpu_v12.h"

//#define VPU_DBG_MEM

#ifdef	VPU_DBG_MEM
#define	pr_dbg_mem(msg...)	do {printk(msg);} while(0)
#else
#define	pr_dbg_mem(msg...)	do{;}while(0)
#endif

struct jz_vpu {
	void __iomem		*iomem;

	struct device		*dev;
	struct miscdevice	mdev;

	struct mutex		lock;		//lock vpu device node
	struct mutex		mutex;		//lock VPU for user using
	struct mutex		mem_lock;	//lock memory operation

	struct clk		*clk;
	struct clk		*clk_gate;

	struct wake_lock	wake_lock;

	struct completion	done;

	int			irq;
	int			use_count;
	pid_t			owner_pid;

	unsigned int		status;

	struct list_head	mem_list;

};





static int vpu_lock(struct jz_vpu *vpu);
static int vpu_unlock(struct jz_vpu *vpu);
static int vpu_dmmu_unmap_all(struct jz_vpu *vpu);


static int vpu_reset(struct jz_vpu *vpu)
{
	int timeout = 0xffffff;
	unsigned int srbc = cpm_inl(CPM_SRBC);

	cpm_set_bit(30, CPM_SRBC);
	while (!(cpm_inl(CPM_SRBC) & (1 << 29)) && --timeout);
	if (timeout == 0) {
		dev_err(vpu->dev,
				"[%d:%d] wait stop ack timeout when stop VPU\n",
				current->tgid, current->pid);
		cpm_outl(srbc, CPM_SRBC);
		return -EIO;
	} else {
		cpm_outl(srbc | (1 << 31), CPM_SRBC);
		cpm_outl(srbc, CPM_SRBC);
	}

	return 0;
}

static int vpu_on(struct jz_vpu *vpu)
{
	int ret;

	if (cpm_inl(CPM_OPCR) & OPCR_IDLE)
		return -EBUSY;

	ret = clk_enable(vpu->clk);
	if (ret) {
		dev_err(vpu->dev, "[%d:%d]VPU clock enable error,errno:%d\n",
				current->tgid, current->pid, ret);
		return -EIO;
	}
	ret = clk_enable(vpu->clk_gate);
	if (ret) {
		dev_err(vpu->dev, "[%d:%d]VPU clock gate enable error,errno:%d\n",
				current->tgid, current->pid, ret);
		return -EIO;
	}

	__asm__ __volatile__ (
		"mfc0  $2, $16,  7   \n\t"
		"ori   $2, $2, 0x340 \n\t"
		"andi  $2, $2, 0x3ff \n\t"
		"mtc0  $2, $16,  7  \n\t"
		"nop                  \n\t");

	ret = vpu_reset(vpu);
	if (ret) {
		dev_err(vpu->dev, "[%d:%d]VPU reset error\n",
				current->tgid, current->pid);
		return ret;
	}

	enable_irq(vpu->irq);

	wake_lock(&vpu->wake_lock);

	dev_dbg(vpu->dev, "[%d:%d] on\n", current->tgid, current->pid);

	return ret;
}

static void vpu_off(struct jz_vpu *vpu)
{
	//disable_irq_nosync(vpu->irq);
	disable_irq(vpu->irq);

	__asm__ __volatile__ (
		"mfc0  $2, $16,  7   \n\t"
		"andi  $2, $2, 0xbf \n\t"
		"mtc0  $2, $16,  7  \n\t"
		"nop                  \n\t");

	cpm_clear_bit(31,CPM_OPCR);
	clk_disable(vpu->clk_gate);
	clk_disable(vpu->clk);
	/* Clear completion use_count here to avoid
	 * a unhandled irq after vpu off */
	vpu->done.done = 0;
	wake_unlock(&vpu->wake_lock);
	dev_dbg(vpu->dev, "[%d:%d] off\n", current->tgid, current->pid);
}


static int vpu_open(struct inode *inode, struct file *filp)
{
	struct miscdevice *dev = filp->private_data;
	struct jz_vpu *vpu = container_of(dev, struct jz_vpu, mdev);
	int ret = 0;

	dev_dbg(vpu->dev, "#########%s %d,open vpu, pid:%d,tgid:%d,comm:%s, current->mm: %p\n",
			__func__, __LINE__,current->pid,current->tgid,current->comm, current->mm);

	mutex_lock(&vpu->lock);

	if (vpu->use_count == 0) {
		ret = vpu_on(vpu);
		if (ret) {
			dev_err(vpu->dev, "[%d:%d]VPU on error\n",
					current->tgid, current->pid);
			goto ERR;
		}
		vpu->use_count++;
		INIT_LIST_HEAD(&vpu->mem_list);
	} else {
		dev_err(vpu->dev, "[%d:%d] VPU already on,"
				" please check your code!!!\n",
			current->tgid, current->pid);
		ret = -EBUSY;
		goto ERR;
	}

	mutex_unlock(&vpu->lock);


	return 0;
ERR:
	mutex_unlock(&vpu->lock);
	return ret;
}


static int vpu_dmmu_unmap_all(struct jz_vpu *vpu)
{
	int ret;
	mutex_lock(&vpu->mem_lock);
	ret = dmmu_unmap_all(vpu->mdev.this_device);
	mutex_unlock(&vpu->mem_lock);

	return ret;
}

static int vpu_release_resource(struct jz_vpu *vpu)
{
	int ret = 0;

	pr_dbg_mem("#########%s %d,close vpu, pid:%d,tgid:%d,comm:%s\n",
			__func__, __LINE__,current->pid,current->tgid,current->comm);

	mutex_lock(&vpu->lock);

	if (vpu->use_count > 0)
		vpu->use_count--;
	else {
		dev_warn(vpu->dev, "[%d:%d] VPU close already\n",
				current->tgid, current->pid);
		mutex_unlock(&vpu->lock);
		return 0;
	}

	if (vpu->use_count == 0) {
		vpu_off(vpu);

		ret = vpu_dmmu_unmap_all(vpu);
		if (ret) {
			dev_err(vpu->dev, "[%d:%d] VPU dmmu unmap all error\n",
					current->tgid, current->pid);
			mutex_unlock(&vpu->lock);
			WARN_ON(1);
			return ret;
		}

		if (mutex_is_locked(&vpu->mutex)) {
			dev_warn(vpu->dev, "[%d:%d]Exception: VPU has been locked,"
					" and USER have not release, so it is need"
					" to release after close VPU\n",
				 current->tgid, current->pid);
			mutex_unlock(&vpu->mutex);
		}
	} else {
		dev_err(vpu->dev, "[%d:%d] VPU device is not permited close"
				" or open more times. Please check you code!\n",
				current->tgid, current->pid);
		mutex_unlock(&vpu->lock);

		return -EPERM;
	}

	dev_info(vpu->dev, "[%d:%d] close\n", current->tgid, current->pid);

	mutex_unlock(&vpu->lock);

	return 0;
}

static int vpu_close(struct inode *inode, struct file *filp)
{
	struct miscdevice *dev = filp->private_data;
	struct jz_vpu *vpu = container_of(dev, struct jz_vpu, mdev);


	dev_dbg(vpu->dev, "###########%s %d,,pid:%d,tgid:%d,comm:%s, , mm: %p close vpu\n",
			__func__, __LINE__,current->pid,current->tgid,current->comm, current->mm);
	return vpu_release_resource(vpu);
//	return 0;
}

#if 1
static int vpu_lock(struct jz_vpu *vpu)
{
	int ret = 0;


	if (vpu->owner_pid == current->pid) {
		dev_err(vpu->dev, "[%d:%d] dead lock(vpu_driver)\n",
				current->tgid, current->pid);
		ret = -EINVAL;
	}

	if (mutex_lock_interruptible(&vpu->mutex) != 0) {
		dev_err(vpu->dev, "[%d:%d] lock vpu_driver error!"
				"the task is been interruptible\n",
				current->tgid, current->pid);
		ret = -EIO;
	}

	vpu->owner_pid = current->pid;

	return ret;
}

static int vpu_unlock(struct jz_vpu *vpu)
{
	int ret = 0;

#if 1
	mutex_unlock(&vpu->mutex);
	vpu->owner_pid = 0;
#else
	if (vpu->owner_pid == current->pid) {
		mutex_unlock(&vpu->mutex);
		vpu->owner_pid = 0;
	} else {
		dev_warn(vpu->dev, "[%d:%d] vpu unlock by else thread:%d\n",
				current->tgid, current->pid, current->pid);
		ret = -EINVAL;
	}

#endif
	return ret;
}
#endif

static int vpu_dmmu_map(struct jz_vpu *vpu, unsigned long arg)
{
	struct miscdevice *dev = &vpu->mdev;
	struct vpu_dmmu_map_info di;
	int ret = 0;
	if (copy_from_user(&di, (void *)arg, sizeof(di))) {
		printk("%s %d, cmd_vpu_dmmu_map, pid:%d,comm:%s\n", __func__, __LINE__,
				current->pid,current->comm);
		return 0;
	}

	mutex_lock(&vpu->mem_lock);

	ret = dmmu_map(dev->this_device, di.addr, di.len);
	if(ret == 0) {
		printk("vpu map user ptr error!!\n");
		mutex_unlock(&vpu->mem_lock);
		return -EFAULT;
	}

	mutex_unlock(&vpu->mem_lock);

	return ret;
}

static int vpu_dmmu_unmap(struct jz_vpu *vpu, unsigned long arg)
{
	struct miscdevice *dev = &vpu->mdev;
	struct vpu_dmmu_map_info di;
	int ret = 0;

	if (copy_from_user(&di, (void *)arg, sizeof(di))) {
		ret = -EFAULT;
		printk("%s %d, cmd_vpu_dmmu_unmap, pid:%d,comm:%s\n", __func__, __LINE__,
				current->pid,current->comm);
		return ret;
	}

	pr_dbg_mem("%s %d, cmd_vpu_dmmu_unmap, addr:0x%x, len:%d,pid:%d,comm:%s\n",
			__func__, __LINE__,di.addr, di.len,current->pid,current->comm);

	mutex_lock(&vpu->mem_lock);

	ret = dmmu_unmap(dev->this_device, di.addr, di.len);
	if(ret) {
		printk("vpu unmap user ptr error!!\n");
		mutex_unlock(&vpu->mem_lock);
		return -EFAULT;
	}

	mutex_unlock(&vpu->mem_lock);

	return ret;
}

static long vpu_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct miscdevice *dev = filp->private_data;
	struct jz_vpu *vpu = container_of(dev, struct jz_vpu, mdev);
	struct flush_cache_info info;
	unsigned int status = 0;
	volatile unsigned long flags;
	unsigned int addr, size;
	unsigned int * arg_r;
	int i, num;
	int ret = 0;

	switch (cmd) {
	case CMD_VPU_OPEN:
		pr_dbg_mem("###########%s %d,[%d:%d],comm:%s, open vpu\n",
				__func__, __LINE__,current->tgid,current->pid,current->comm);
		break;
	case CMD_VPU_CLOSE:
		pr_dbg_mem("###########%s %d,[%d:%d],comm:%s, close vpu\n",
				__func__, __LINE__,current->tgid,current->pid,current->comm);
		break;

	case WAIT_COMPLETE:
		pr_dbg_mem("###########%s %d,[%d:%d],comm:%s, wait complete\n",
				__func__, __LINE__,current->tgid,current->pid,current->comm);

		ret = wait_for_completion_timeout(
			&vpu->done, msecs_to_jiffies(200));
		if (ret > 0) {
			status = vpu->status;
		} else {
			dev_warn(vpu->dev, "[%d:%d] wait_for_completion for vpu_driver timeout\n",
				 current->tgid, current->pid);
			status = vpu_readl(vpu,REG_VPU_STAT);
			if (vpu_reset(vpu) < 0) {
				printk("Reset vpu error\n");
				status = 0;
			}

			vpu->done.done = 0;
		}
		if (copy_to_user((void *)arg, &status, sizeof(status)))
			ret = -EFAULT;
		break;
	case LOCK:
		pr_dbg_mem("###########%s %d,[%d:%d],comm:%s, lock vpu\n",
				__func__, __LINE__,current->tgid,current->pid,current->comm);
		ret = vpu_lock(vpu);
		if (ret) {
			dev_err(vpu->dev, "[%d:%d] lock vpu error!\n",
				current->tgid, current->pid);
		} else
			dev_dbg(vpu->dev, "[%d:%d] lock vpu_driver\n",
					current->tgid, current->pid);
		break;
	case UNLOCK:
		pr_dbg_mem("###########%s %d,[%d:%d],comm:%s, unlock vpu\n",
				__func__, __LINE__,current->tgid,current->pid,current->comm);
		ret= vpu_unlock(vpu);
		if (ret) {
			dev_err(vpu->dev, "[%d:%d] lock vpu error!\n",
				current->tgid, current->pid);
		} else
			dev_dbg(vpu->dev, "[%d:%d] unlock vpu_driver\n",
					current->tgid, current->pid);
		break;
	case FLUSH_CACHE:
		pr_dbg_mem("###########%s %d,[%d:%d],comm:%s, cache flush\n",
				__func__, __LINE__,current->tgid,current->pid,current->comm);
		if (copy_from_user(&info, (void *)arg, sizeof(info))) {
			ret = -EFAULT;
			break;
		}
		dma_cache_sync(NULL, (void *)info.addr, info.len, info.dir);
		dev_dbg(vpu->dev, "[%d:%d] vpu_driver flush cache\n",
				current->tgid, current->pid);
		break;
	case CMD_VPU_PHY:
		pr_dbg_mem("###########%s %d,[%d:%d],comm:%s, vpu phy\n",
				__func__, __LINE__,current->tgid,current->pid,current->comm);
		break;
	case CMD_VPU_CACHE:
		pr_dbg_mem("###########%s %d,[%d:%d],comm:%s, cache crash\n",
				__func__, __LINE__,current->tgid,current->pid,current->comm);
		arg_r = (unsigned int *)arg;
		addr = (unsigned int)arg_r[0];
		size = arg_r[1];
		if(size == 0) {
			printk("ERROR:%s:size = %d, addr = 0x%x\n", __func__, size, addr);
		//	force_sig(SIGSEGV, current);
		}else {
			dma_cache_wback_inv(addr, size);
		}
		break;
	case CMD_VPU_DMA_NOTLB:
		pr_dbg_mem("###########%s %d,[%d:%d],comm:%s, dma no tlb\n",
				__func__, __LINE__,current->tgid,current->pid,current->comm);
		local_irq_save(flags);
		arg_r = (unsigned int *)arg;
		REG_VPU_LOCK |= VPU_NEED_WAIT_END_FLAG;
		for(i = 0;i < 4; i += 2){
			*(unsigned int *)(arg_r[i]) = arg_r[i+1];
		}
		local_irq_restore(flags);
		break;
	case CMD_VPU_DMA_TLB:
		pr_dbg_mem("###########%s %d,[%d:%d],comm:%s, dma tlb\n",
				__func__, __LINE__,current->tgid,current->pid,current->comm);
		local_irq_save(flags);
		arg_r = (unsigned int *)arg;
		REG_VPU_LOCK |= VPU_NEED_WAIT_END_FLAG;
		for(i = 0;i < 10;i += 2)
			*(unsigned int *)(arg_r[i]) = arg_r[i+1];
		local_irq_restore(flags);
		break;
	case CMD_VPU_CLEAN_WAIT_FLAG:
		pr_dbg_mem("###########%s %d,[%d:%d],comm:%s, clean wait vpu\n",
				__func__, __LINE__,current->tgid,current->pid,current->comm);
		local_irq_save(flags);
		while(!(( REG_VPU_LOCK &(VPU_WAIT_OK)) || ( REG_VPU_STATUS&(VPU_END))))
			;
		REG_VPU_LOCK &= ~(VPU_NEED_WAIT_END_FLAG);
		if(REG_VPU_LOCK & VPU_WAIT_OK)
			REG_VPU_LOCK &= ~(VPU_WAIT_OK);
		local_irq_restore(flags);
		break;
	case CMD_VPU_RESET:
		pr_dbg_mem("###########%s %d,[%d:%d],comm:%s, reset vpu\n",
				__func__, __LINE__,current->tgid,current->pid,current->comm);
		local_irq_save(flags);
		REG_CPM_VPU_SWRST |= CPM_VPU_STP;
		while(!(REG_CPM_VPU_SWRST & CPM_VPU_ACK))
			;
		REG_CPM_VPU_SWRST = ((REG_CPM_VPU_SWRST | CPM_VPU_SR) & ~CPM_VPU_STP);
		REG_CPM_VPU_SWRST = (REG_CPM_VPU_SWRST & ~CPM_VPU_SR & ~CPM_VPU_STP);
		REG_VPU_LOCK = 0;
		local_irq_restore(flags);
		break;
	case CMD_VPU_SET_REG:
		pr_dbg_mem("###########%s %d,[%d:%d],comm:%s, set vpu register\n",
				__func__, __LINE__,current->tgid,current->pid,current->comm);
		local_irq_save(flags);
		num = *(unsigned int*)arg;
		arg += 4;
		arg_r = (unsigned int *)arg;
		REG_VPU_LOCK |= VPU_NEED_WAIT_END_FLAG;
		for(i = 0;i < num; i += 2)
			*(unsigned int *)(arg_r[i]) = arg_r[i+1];
		local_irq_restore(flags);
		break;
#ifdef CONFIG_SOC_M200
	case CMD_VPU_REQUEST_MEM:
		printk("###########%s %d,[%d:%d],comm:%s, request mem for vpu\n",
				__func__, __LINE__,current->tgid,current->pid,current->comm);
		return -EFAULT;
	case CMD_VPU_RELEASE_MEM:
		printk("###########%s %d,[%d:%d],comm:%s, release mem for vpu\n",
				__func__, __LINE__,current->tgid,current->pid,current->comm);
		return -EFAULT;
	case CMD_VPU_RELEASE_ALL_MEM:
		pr_dbg_mem("###########%s %d,[%d:%d],comm:%s, release all mem for vpu\n",
				__func__, __LINE__,current->tgid,current->pid,current->comm);
		return -EFAULT;

	case CMD_VPU_DMMU_MAP:
		pr_dbg_mem("###########%s %d,[%d:%d],comm:%s, dmmu map for vpu\n",
				__func__, __LINE__,current->tgid,current->pid,current->comm);
		return vpu_dmmu_map(vpu, arg);
	case CMD_VPU_DMMU_UNMAP:
		pr_dbg_mem("###########%s %d,[%d:%d],comm:%s, dmmu unmap for vpu\n",
				__func__, __LINE__,current->tgid,current->pid,current->comm);

		local_irq_save(flags);
		REG_CPM_VPU_SWRST |= CPM_VPU_STP;
		while(!(REG_CPM_VPU_SWRST & CPM_VPU_ACK))
			;
		REG_CPM_VPU_SWRST = ((REG_CPM_VPU_SWRST | CPM_VPU_SR) & ~CPM_VPU_STP);
		REG_CPM_VPU_SWRST = (REG_CPM_VPU_SWRST & ~CPM_VPU_SR & ~CPM_VPU_STP);
		REG_VPU_LOCK = 0;
		local_irq_restore(flags);

		return vpu_dmmu_unmap(vpu, arg);
	case CMD_VPU_DMMU_UNMAP_ALL:
		pr_dbg_mem("###########%s %d,[%d:%d],comm:%s, dmmu unmap all for vpu\n",
				__func__, __LINE__,current->tgid,current->pid,current->comm);

		local_irq_save(flags);
		REG_CPM_VPU_SWRST |= CPM_VPU_STP;
		while(!(REG_CPM_VPU_SWRST & CPM_VPU_ACK))
			;
		REG_CPM_VPU_SWRST = ((REG_CPM_VPU_SWRST | CPM_VPU_SR) & ~CPM_VPU_STP);
		REG_CPM_VPU_SWRST = (REG_CPM_VPU_SWRST & ~CPM_VPU_SR & ~CPM_VPU_STP);
		REG_VPU_LOCK = 0;
		local_irq_restore(flags);

		vpu_dmmu_unmap_all(vpu);
		break;
#endif
	default:
		pr_dbg_mem("###########%s %d,[%d:%d],comm:%s, default\n",
				__func__, __LINE__,current->tgid,current->pid,current->comm);
		break;
	}

	return ret;
}

static int vpu_mmap(struct file *file, struct vm_area_struct *vma)
{
	if( PFN_PHYS(vma->vm_pgoff) < 0x13200000 || PFN_PHYS(vma->vm_pgoff) >= 0x13300000){
		printk("phy addr err ,range is 0x13200000 - 13300000");
		return -EAGAIN;
	}
	vma->vm_flags |= VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (io_remap_pfn_range(vma,vma->vm_start,
			       vma->vm_pgoff,
			       vma->vm_end - vma->vm_start,
			       vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

static irqreturn_t vpu_interrupt(int irq, void *dev)
{
	struct jz_vpu *vpu = dev;
	unsigned int vpu_stat;

	vpu_stat = vpu_readl(vpu,REG_VPU_STAT);

#define CLEAR_VPU_BIT(vpu,offset,bm)				\
	do {							\
		unsigned int stat;				\
		stat = vpu_readl(vpu,offset);			\
		vpu_writel(vpu,offset,stat & ~(bm));		\
	} while(0)

#define check_vpu_status(STAT, fmt, args...) do {	\
		if(vpu_stat & STAT)			\
			dev_err(vpu->dev, fmt, ##args);	\
	}while(0)

	if(vpu_stat) {
		if(vpu_stat & VPU_STAT_ENDF) {
			if(vpu_stat & VPU_STAT_JPGEND) {
				dev_dbg(vpu->dev, "JPG successfully done!\n");
				vpu_stat = vpu_readl(vpu, REG_VPU_JPGC_STAT);
				CLEAR_VPU_BIT(vpu, REG_VPU_JPGC_STAT,
						JPGC_STAT_ENDF);
			} else {
				dev_dbg(vpu->dev, "SCH successfully done!\n");
				CLEAR_VPU_BIT(vpu, REG_VPU_SDE_STAT,
						SDE_STAT_BSEND);
				CLEAR_VPU_BIT(vpu, REG_VPU_DBLK_STAT,
						DBLK_STAT_DOEND);
			}
		} else {
			check_vpu_status(VPU_STAT_SLDERR, "SHLD error!\n");
			check_vpu_status(VPU_STAT_TLBERR, "TLB error! Addr is 0x%08x\n",
					 vpu_readl(vpu, REG_VPU_STAT));
			check_vpu_status(VPU_STAT_BSERR, "BS error!\n");
			check_vpu_status(VPU_STAT_ACFGERR, "ACFG error!\n");
			check_vpu_status(VPU_STAT_TIMEOUT, "TIMEOUT error!\n");
			CLEAR_VPU_BIT(vpu,REG_VPU_GLBC, (VPU_INTE_ACFGERR |
				       VPU_INTE_TLBERR | VPU_INTE_BSERR |
				       VPU_INTE_ENDF));
		}
	} else {
		if(vpu_readl(vpu,REG_VPU_AUX_STAT) & AUX_STAT_MIRQP) {
			dev_dbg(vpu->dev, "AUX successfully done!\n");
			CLEAR_VPU_BIT(vpu, REG_VPU_AUX_STAT, AUX_STAT_MIRQP);
		} else {
			dev_dbg(vpu->dev, "illegal interrupt happened!\n");
			return IRQ_HANDLED;
		}
	}

	vpu->status = vpu_stat;
	complete(&vpu->done);

	return IRQ_HANDLED;
}

static struct file_operations vpu_misc_fops = {
	.open		= vpu_open,
	.unlocked_ioctl	= vpu_ioctl,
	.mmap		= vpu_mmap,
	.release	= vpu_close,
};

static int vpu_probe(struct platform_device *pdev)
{
	int ret;
	struct resource	*regs;
	struct jz_vpu *vpu;

	vpu = kzalloc(sizeof(struct jz_vpu), GFP_KERNEL);
	if (!vpu)
		ret = -ENOMEM;

	vpu->irq = platform_get_irq(pdev, 0);
	if(vpu->irq < 0) {
		dev_err(&pdev->dev, "get irq failed\n");
		ret = vpu->irq;
		goto err_get_mem;
	}

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs) {
		dev_err(&pdev->dev, "No iomem resource\n");
		ret = -ENXIO;
		goto err_get_mem;
	}

	vpu->iomem = ioremap(regs->start, resource_size(regs));
	if (!vpu->iomem) {
		dev_err(&pdev->dev, "ioremap failed\n");
		ret = -ENXIO;
		goto err_get_mem;
	}

	vpu->clk_gate = clk_get(&pdev->dev, "vpu");
	if (IS_ERR(vpu->clk_gate)) {
		ret = PTR_ERR(vpu->clk_gate);
		goto err_get_clk_gate;
	}

	vpu->clk = clk_get(vpu->dev,"cgu_vpu");
	if (IS_ERR(vpu->clk)) {
		ret = PTR_ERR(vpu->clk);
		goto err_get_clk_cgu;
	}
	/*
	 * for jz4775, when vpu freq is set over 300M, the decode process
	 * of vpu may be error some times which can led to graphic abnomal
	 */

#if defined(CONFIG_SOC_4775)
	clk_set_rate(vpu->clk,250000000);
#else
	clk_set_rate(vpu->clk,300000000);
#endif

	vpu->dev = &pdev->dev;
	vpu->mdev.minor = MISC_DYNAMIC_MINOR;
	vpu->mdev.name =  "jz-vpu";
	vpu->mdev.fops = &vpu_misc_fops;

	mutex_init(&vpu->lock);
	mutex_init(&vpu->mem_lock);

	ret = misc_register(&vpu->mdev);
	if (ret < 0) {
		dev_err(&pdev->dev, "misc_register failed\n");
		goto err_registe_misc;
	}
	platform_set_drvdata(pdev, vpu);

	wake_lock_init(&vpu->wake_lock, WAKE_LOCK_SUSPEND, "vpu");

	mutex_init(&vpu->mutex);

	init_completion(&vpu->done);
	ret = request_irq(vpu->irq, vpu_interrupt, IRQF_DISABLED,
			  "vpu",vpu);
	if (ret < 0) {
		dev_err(&pdev->dev, "request_irq failed\n");
		goto err_request_irq;
	}

	disable_irq_nosync(vpu->irq);

	return 0;

err_request_irq:
	misc_deregister(&vpu->mdev);
err_registe_misc:
	clk_put(vpu->clk);
err_get_clk_cgu:
	clk_put(vpu->clk_gate);
err_get_clk_gate:
	iounmap(vpu->iomem);
err_get_mem:
	kfree(vpu);
	return ret;
}

static int vpu_remove(struct platform_device *dev)
{
	struct jz_vpu *vpu = platform_get_drvdata(dev);

	misc_deregister(&vpu->mdev);
	wake_lock_destroy(&vpu->wake_lock);
	clk_put(vpu->clk);
	clk_put(vpu->clk_gate);
	free_irq(vpu->irq, vpu);
	iounmap(vpu->iomem);
	kfree(vpu);

	return 0;
}
static int vpu_suspend(struct platform_device * pdev, pm_message_t state)
{
	struct jz_vpu *vpu = platform_get_drvdata(pdev);
	int timeout = 0xffff;

	if( vpu->use_count != 0 ) {
		if( REG_VPU_LOCK & VPU_NEED_WAIT_END_FLAG) {
			while(!( REG_VPU_STATUS & (0x1) ) && timeout--)
				;
			if(!timeout){
				printk("vpu -------- timeout\n");
				return 0;
			}
			REG_VPU_LOCK |= VPU_WAIT_OK;
			REG_VPU_LOCK &= ~VPU_NEED_WAIT_END_FLAG;
		}
		clk_disable(vpu->clk);
		clk_disable(vpu->clk_gate);
	}
	return 0;
}
static int vpu_resume(struct platform_device * pdev)
{
	struct jz_vpu *vpu = platform_get_drvdata(pdev);

	if(vpu->use_count != 0) {
		clk_set_rate(vpu->clk,300000000);
		clk_enable(vpu->clk);
		clk_enable(vpu->clk_gate);
	}
	return 0;
}
static struct platform_driver jz_vpu_driver = {
	.probe		= vpu_probe,
	.remove		= vpu_remove,
	.driver		= {
		.name	= "jz-vpu",
	},
	.suspend    = vpu_suspend,
	.resume     = vpu_resume,
};

static int __init vpu_init(void)
{
	return platform_driver_register(&jz_vpu_driver);
}

static void __exit vpu_exit(void)
{
	platform_driver_unregister(&jz_vpu_driver);
}

module_init(vpu_init);
module_exit(vpu_exit);

MODULE_DESCRIPTION("JZ M200 V12 VPU driver");
MODULE_LICENSE("GPL v2");
