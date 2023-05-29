#include <asm/asm.h>
#include <stackframe.h>

.section .text.tlb_miss_entry
tlb_miss_entry:
	j       exc_gen_entry

.section .text.exc_gen_entry
exc_gen_entry:
	SAVE_ALL #SAVE_ALL 宏将当前上下文保存到内核的异常栈
/* Exercise 3.9: Your code here. */
	mfc0 t0, CP0_CAUSE # Cause 寄存器的内容拷贝到 t0 
	andi t0, 0x7c#取得 Cause 寄存器中的 2~6 位，也就是对应的异常码
	lw t0, exception_handlers(t0)#以得到的异常码作为索引在 exception_handlers 数组中找到对应的中断处理函数
	jr t0 #跳转到对应的中断处理函数中