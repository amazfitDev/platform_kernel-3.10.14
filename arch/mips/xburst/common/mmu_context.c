#include <jz_notifier.h>
#include <asm/mmu_context.h>




void arch_dup_mmap(struct mm_struct *oldmm,
				 struct mm_struct *mm)
{

	return;
}

void arch_exit_mmap(struct mm_struct *mm)
{
	jz_notifier_call(NOTEFY_PROI_HIGH, MMU_CONTEXT_EXIT_MMAP, mm);
	return;
}
