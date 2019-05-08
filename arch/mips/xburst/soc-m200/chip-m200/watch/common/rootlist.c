/*
 * Copyright (C) 2014 Ingenic Semiconductor Co., Ltd.
 * Authors: Kage Shen <kgat96@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the License, or (at your
 * option) any later version.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/slab.h>
#include "board_base.h"

#include <linux/rootlist.h>

/*
 * setup lcd hwlist
 */
extern void setup_lcd_hwlist(void);

static const char* const TAG = "rootlist:";

struct root_desc {
    root_key key;
    const char* name;
};

struct attr_desc {
    attr_key key;
    const char* name;
};

struct root_entry {
    struct list_head sublist;
    struct list_head rootnode;
    struct root_desc *desc;
};

struct sub_entry {
    struct list_head subnode;
    struct attr_desc *desc;
    const char *value;
};

struct root_desc root_tables[] = {
    {nSYSTEM, "system"},
    {nBLUETOOTH, "bluetooth"},
    {nLCD, "lcd"},
    {nSENSOR, "sensor"},
    {nMMI, "mmi"},
};

struct attr_desc attr_tables[] = {
    {tTYPE, "type"},
    {tCHIP, "chip"},
    {tNAME, "name"},
    {tVERSION, "version"},
    {tDPI, "dpi"},
    {tPPI, "ppi"},
    {tEXTERIOR, "exterior"},
    {tPATH, "path"},
    {tSIZE, "size"},
    {tPORT, "port"},
    {tITEM, "item"},
};

static LIST_HEAD(rootlist);

static int hwlist_create_root(void)
{
    int i;
    struct root_entry *entry;
    int n = sizeof(root_tables) / sizeof(struct root_desc);

    for (i = 0; i < n; i++) {
        entry = (struct root_entry *)kzalloc(sizeof(struct root_entry), GFP_KERNEL);
        if (entry == NULL) {
            return -ENOMEM;
        }

        entry->desc = &root_tables[i];
        INIT_LIST_HEAD(&entry->sublist);
        list_add(&entry->rootnode, &rootlist);
    }

    return 0;
}

inline struct root_entry *find_root_entry(root_key key)
{
    struct root_entry *entry = NULL;

    list_for_each_entry(entry, &rootlist, rootnode) {
        if (entry->desc->key == key) {
            return entry;
        }
    }

    return NULL;
}

int hwlist_create_attr(root_key root, attr_key attr,  add_key add, const char *value)
{
    int n,i;

    struct attr_desc *desc = NULL;
    struct sub_entry *sentry = NULL;
    struct root_entry *rentry = find_root_entry(root);
    char* tmp_value;

    if ((value == NULL) || (value[0] == '\0'))
        return -EINVAL;

    if (!rentry) {
        printk("%s cannot find root node entry\n", TAG);
        return -ENOENT;
    }

    n = sizeof(attr_tables) / sizeof(struct attr_desc);

    for (i = 0; i < n; i++) {
        if (attr_tables[i].key == attr)
            desc = &attr_tables[i];
    }

    if (!desc) {
        printk("%s cannot find attr name\n", TAG);
        return -ENOENT;
    }

    list_for_each_entry(sentry, &rentry->sublist, subnode) {
        if ((desc == sentry->desc) && (!strcmp(sentry->value, value))) {
            printk("%s have the same value\n", TAG);
            return -EEXIST;
        }

        if ((desc == sentry->desc) && (sentry->value != NULL)) {
            switch(add) {
            case tMULTIPLE:
                tmp_value = (char *)krealloc(sentry->value, strlen(sentry->value) + strlen(value) + 2, GFP_KERNEL);
                if (tmp_value != NULL) {
                    sentry->value = tmp_value;
                    sprintf(tmp_value,"%s,%s",sentry->value,value);
                } else
                    printk("%s krealloc failed\n", TAG);

                break;
            case tSINGLE :
            default:
                printk(KERN_DEBUG "%s has registered [%s.%s]\n", TAG, rentry->desc->name, desc->name);
                break;
            }
            return -EEXIST;
        }

    }

    sentry = kzalloc(sizeof(struct sub_entry), GFP_KERNEL);
    if (!sentry) {
        printk("%s malloc sub entry error\n", TAG);
        return -ENOMEM;
    }

    sentry->desc = desc;
    sentry->value = kstrdup(value,GFP_KERNEL);
    if (!sentry->value) {
        printk("%s strdup error\n", TAG);
        goto err_strdup;
    }

    list_add(&sentry->subnode, &rentry->sublist);

    printk(KERN_DEBUG "%s %s add sub:%s value:%s\n", TAG, rentry->desc->name,
            sentry->desc->name, sentry->value);

    return 0;
err_strdup:
    kfree(sentry);

    return -ENOMEM;
}

static int dump(void)
{
    struct root_entry *rentry = NULL;
    struct sub_entry *sentry = NULL;

    printk("%s ============rootlist============\n", TAG);

    list_for_each_entry(rentry, &rootlist, rootnode) {
        printk("%s root: %s\n", TAG, rentry->desc->name);
        list_for_each_entry(sentry, &rentry->sublist, subnode) {
            printk("%s sub:%s value:%s\n", TAG, sentry->desc->name,
                    sentry->value);
        }
    }

    return 0;
}

static int hardware_proc_show(struct seq_file *seq, void *v)
{
    struct root_entry *rentry = NULL;
    struct sub_entry *sentry = NULL;

    list_for_each_entry(rentry, &rootlist, rootnode) {
        list_for_each_entry(sentry, &rentry->sublist, subnode) {
            seq_printf(seq, "%s.%s:%s\n", rentry->desc->name,
                    sentry->desc->name, sentry->value);
        }
    }
    return 0;
}

static int hardware_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, hardware_proc_show, PDE_DATA(inode));
}

static const struct file_operations hardware_proc_fops = {
    .owner   = THIS_MODULE,
    .open    = hardware_proc_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = seq_release,
};

static int __init init_rootlist(void)
{
    struct proc_dir_entry *res;

    hwlist_create_root();
    hwlist_create_attr(nSYSTEM, tCHIP, tSINGLE, "m200");
    hwlist_create_attr(nSYSTEM, tTYPE, tSINGLE, "iwds");
    hwlist_create_attr(nSYSTEM, tNAME, tSINGLE, "ingenic");

#ifdef CONFIG_BLUETOOTH_NAME
    hwlist_bluetooth_chip(CONFIG_BLUETOOTH_NAME);
#endif

#ifdef BLUETOOTH_UPORT_NAME
    hwlist_bluetooth_port(BLUETOOTH_UPORT_NAME);
#endif

    hwlist_mmi_item("software_version");
    hwlist_mmi_item("touchkey");
    hwlist_mmi_item("lcddisplay");
    hwlist_mmi_item("backlight");
    hwlist_mmi_item("charge");
    hwlist_mmi_item("usb");
    hwlist_mmi_item("audioplay");
    hwlist_mmi_item("mic");
    hwlist_mmi_item("vibrator");
    hwlist_mmi_item("gsensor");
    hwlist_mmi_item("gyrsensor");
    //hwlist_mmi_item("magsensor");
    hwlist_mmi_item("stepcount");
    hwlist_mmi_item("pressuresensor");
    hwlist_mmi_item("heartrate_adc");
    hwlist_mmi_item("bluetooth");
#if defined(CONFIG_BCMDHD_1_141_66)
    hwlist_mmi_item("wifi");
#endif
    hwlist_mmi_item("factory_data_reset");  //must be on the bottom


    setup_lcd_hwlist();
    dump();

    res = proc_mkdir("hardware", NULL);
    if (res == NULL) {
        printk("%s  mkdir hardware dir error\n", TAG);
    return -EINVAL;
    }

    proc_create("list", 0, res, &hardware_proc_fops);

    return 0;
}

arch_initcall(init_rootlist);
