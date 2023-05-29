#include <asm/asm.h>
#include <mmu.h>

.text
EXPORT(_start)
.set at
.set reorder
	/* disable interrupts */
	mtc0    zero, CP0_STATUS
/*
我们需要首先将 sp 寄存器设置到内核栈空
间的位置上，随后跳转到 mips_init 函数。内核栈空间的地址可以从 include/mmu.h 中看到。
这里做一个提醒，请注意栈的增长方向。*/

	/* hint: you can reference the memory layout in include/mmu.h */
	/* set up the kernel stack */
	/* Exercise 1.3: Your code here. (1/2) */
	lui sp,  0x8040
	/* jump to mips_init */
	/* Exercise 1.3: Your code here. (2/2) */
	jal mips_init
