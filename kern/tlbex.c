#include <env.h>
#include <pmap.h>

static void passive_alloc(u_int va, Pde *pgdir, u_int asid) {
	struct Page *p = NULL;

	if (va < UTEMP) {
		panic("address too low");
	}

	if (va >= USTACKTOP && va < USTACKTOP + BY2PG) {
		panic("invalid memory");
	}

	if (va >= UENVS && va < UPAGES) {
		panic("envs zone");
	}

	if (va >= UPAGES && va < UVPT) {
		panic("pages zone");
	}

	if (va >= ULIM) {
		panic("kernel address");
	}

	panic_on(page_alloc(&p));
	panic_on(page_insert(pgdir, asid, p, PTE_ADDR(va), PTE_D));
}

/* Overview:
 *  Refill TLB.
 */
Pte _do_tlb_refill(u_long va, u_int asid) {
	Pte *pte;
	
	/* Hints:
	 *  Invoke 'page_lookup' repeatedly in a loop to find the page table entry 'pte' associated
	 *  with the virtual address 'va' in the current address space 'cur_pgdir'.
	 *  **While** 'page_lookup' returns 'NULL', indicating that the 'pte' could not be found,
	 *  allocate a new page using 'passive_alloc' until 'page_lookup' succeeds.
	 */ 
	/* Exercise 2.9: Your code here. */
	// 我现在要找到 pgdir，还有这个**ppte
	while(page_lookup(cur_pgdir,va,&pte)==NULL){ //page_lookup(Pde *pgdir, u_long va, Pte **ppte) 
		passive_alloc(va,cur_pgdir,asid);//passive_alloc(u_int va, Pde *pgdir, u_int asid)
	}
	return *pte;
}

#if !defined(LAB) || LAB >= 4
/* Overview:
 *   This is the TLB Mod exception handler in kernel.
 *   Our kernel allows user programs to handle TLB Mod exception in user mode, so we copy its
 *   context 'tf' into UXSTACK and modify the EPC to the registered user exception entry.
 *
 * Hints:
 *   'env_user_tlb_mod_entry' is the user space entry registered using
 *   'sys_set_user_tlb_mod_entry'.
 *	，处理页写入异常
	 UXSTACKTOP:异常处理栈的栈顶
	USTACKTOP:用户栈顶，即和异常处理栈的交界
 *   The user entry should handle this TLB Mod exception and restore the context.
 */
void do_tlb_mod(struct Trapframe *tf) {
	struct Trapframe tmp_tf = *tf;

	if (tf->regs[29] < USTACKTOP || tf->regs[29] >= UXSTACKTOP) {
		tf->regs[29] = UXSTACKTOP;
	}
	tf->regs[29] -= sizeof(struct Trapframe);//向下移动，为新tf分配空间
	*(struct Trapframe *)tf->regs[29] = tmp_tf;

	if (curenv->env_user_tlb_mod_entry) {
	//当前进程（即 curenv）的用户级异常处理程序的入口点，该程序将在 TLB 修改异常发生时被执行
		tf->regs[4] = tf->regs[29];//当前栈指针（$sp）的值存入 tf->regs[4] 中
		tf->regs[29] -= sizeof(tf->regs[4]);//并将 $sp 指向一个新的位置，以便在异常处理程序执行完成后能够正确返回
		// Hint: Set 'cp0_epc' in the context 'tf' to 'curenv->env_user_tlb_mod_entry'.
		/* Exercise 4.11: Your code here. */
		tf->cp0_epc=curenv->env_user_tlb_mod_entry;
		//当前进程的用户级异常处理程序的入口地址存入 cp0_epc 寄存器中，
		//以便异常处理程序执行完成后能够正确返回到正确的地址
	} else {
		panic("TLB Mod but no user handler registered");
		//如果当前进程没有注册用户级异常处理程序，则报错并触发内核恐慌
	}
}
#endif
