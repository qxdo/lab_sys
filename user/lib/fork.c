#include <env.h>
#include <lib.h>
#include <mmu.h>

/* Overview:
 *   Map the faulting page to a private writable copy.
 *
 * Pre-Condition:
 * 	'va' is the address which led to the TLB Mod exception.
 *
 * Post-Condition:
 *  - Launch a 'user_panic' if 'va' is not a copy-on-write page.
 *  - Otherwise, this handler should map a private writable copy of
 *    the faulting page at the same address.
 */
static void __attribute__((noreturn)) cow_entry(struct Trapframe *tf) {
	u_int va = tf->cp0_badvaddr;
	u_int perm;
	//debugf("cow: %x\n", va);
	/* Step 1: Find the 'perm' in which the faulting address 'va' is mapped. */
	/* Hint: Use 'vpt' and 'VPN' to find the page table entry. If the 'perm' doesn't have
	 * 'PTE_COW', launch a 'user_panic'. */
	/* Exercise 4.13: Your code here. (1/6) */
	perm=vpt[VPN(va)] & 0xfff;
	if(!(perm&PTE_COW))
		user_panic("panic at:");
	/* Step 2: Remove 'PTE_COW' from the 'perm', and add 'PTE_D' to it. */
	/* Exercise 4.13: Your code here. (2/6) */
	perm=(perm&(~PTE_COW))|PTE_D;
	/* Step 3: Allocate a new page at 'UCOW'. */
	/* Exercise 4.13: Your code here. (3/6) */
	panic_on(syscall_mem_alloc(0,(void *)UCOW,perm));
	/* Step 4: Copy the content of the faulting page at 'va' to 'UCOW'. */
	/* Hint: 'va' may not be aligned to a page! */
	/* Exercise 4.13: Your code here. (4/6) */
	void *src=(void *)ROUNDDOWN(va,BY2PG);
	void *dst=(void *)ROUNDDOWN(UCOW,BY2PG);
	memcpy(dst,src,BY2PG);
	// Step 5: Map the page at 'UCOW' to 'va' with the new 'perm'.
	/* Exercise 4.13: Your code here. (5/6) */
	panic_on(syscall_mem_map(0,(void *)UCOW,0,(void *)va,perm));

	// Step 6: Unmap the page at 'UCOW'.
	/* Exercise 4.13: Your code here. (6/6) */
	panic_on(syscall_mem_unmap(0,(void *)UCOW));
	// Step 7: Return to the faulting routine.
	int r = syscall_set_trapframe(0, tf);
	user_panic("syscall_set_trapframe returned %d", r);
}


/* Overview:
 *   Grant our child 'envid' access to the virtual page 'vpn' (with address 'vpn' * 'BY2PG') in our
 *   (current env's) address space.子进程是envid，虚拟页面是vpn*BY2PG
 *   'PTE_COW' should be used to isolate the modifications on unshared memory from a parent and its
 *   children.用PTE_COW分离开父子进程（具体怎么实现呢）
 * Post-Condition:
 *   If the virtual page 'vpn' has 'PTE_D' and doesn't has 'PTE_LIBRARY', both our original virtual
 *   page and 'envid''s newly-mapped virtual page should be marked 'PTE_COW' and without 'PTE_D',
 *   while the other permission bits are kept.
 *	满足上述条件，父子进程都要被映射到虚拟页面，权限位是PTE_COW|(~PTE_D)
 *   If not, the newly-mapped virtual page in 'envid' should have the exact same permission as our
 *   original virtual page.
 *	如果不满足，子进程的新映射的虚拟页面的权限和父进程相同
 * Hint:
 *   - 'PTE_LIBRARY' indicates that the page should be shared among a parent and its children.
		PTE_LIBRARY表示父子进程共享该页面。（？具体不知道怎么用
 *   - A page with 'PTE_LIBRARY' may have 'PTE_D' at the same time, you should handle it correctly.
		PTE_LIBRARY可能和PTE_D同时存在，所以要正确处理？不懂
 *   - You can pass '0' as an 'envid' in arguments of 'syscall_*' to indicate current env because
 *     kernel 'envid2env' converts '0' to 'curenv').
		0表示当前进程
 *   - You should use 'syscall_mem_map', the user space wrapper around 'msyscall' to invoke
 *     'sys_mem_map' in kernel.
 >>:如果页面已经被标记为 PTE_D，并且没有标记为 PTE_LIBRARY，
 则我们需要在父进程和子进程之间使用写时复制技术来共享页面。
 如果页面没有被标记为 PTE_D 或者被标记为 PTE_LIBRARY，
 则在父进程和子进程之间共享页面，而不需要使用写时复制技术
 
 这里我好像理解，就是父进程的页面，根据权限映射给子进程
 */
static void duppage(u_int envid, u_int vpn) {
	int r;
	u_int addr;
	u_int perm;
	/* Step 1: Get the permission of the page. */
	/* Hint: Use 'vpt' to find the page table entry. */
	/* Exercise 4.10: Your code here. (1/2) */
	Pte pte = vpt[vpn];
	perm = pte & 0xFFF;
	addr =vpn*BY2PG;//虚拟地址
	/* Step 2: If the page is writable, and not shared with children, and not marked as COW yet,
	 * then map it as copy-on-write, both in the parent (0) and the child (envid). */
	/* Hint: The page should be first mapped to the child before remapped in the parent. (Why?)
	 *
	 只读页面 对于不具有 PTE_D 权限位的页面，按照相同权限（只读）映射给子进程即可。
	写时复制页面 即具有 PTE_COW 权限位的页面。这类页面是之前的 fork 时 duppage 的结果，且在本次 fork 前必然未被写入过。
	共享页面 即具有 PTE_LIBRARY 权限位的页面。这类页面需要保持共享可写的状态，即在父子进程中映射到相同的物理页，使对其进行修改的结果相互可见。在文件系统部分的实验中，我们会使用到这样的页面。
	可写页面 即具有 PTE_D 权限位，且不符合以上特殊情况的页面。这类页面需要在父进程和子进程的页表项中都使用 PTE_COW 权限位进行保护。
	/* Exercise 4.10: Your code here. (2/2) */
	if((perm&PTE_D) && !(perm&PTE_LIBRARY) &&!(perm&PTE_COW) )
	{
		panic_on(syscall_mem_map(0,(void *)addr,envid,(void *)addr,(PTE_COW|perm)&(~PTE_D)));
		panic_on(syscall_mem_map(0,(void *)addr,0,(void *)addr,(PTE_COW|perm)&(~PTE_D)));
	}
	else
	{
		panic_on(syscall_mem_map(0,(void *)addr,envid,(void *)addr,perm));
	}
}

/* Overview:
 *   User-level 'fork'. Create a child and then copy our address space.
 *   Set up ours and its TLB Mod user exception entry to 'cow_entry'.
 *
 * Post-Conditon:
 *   Child's 'env' is properly set.
 *
 * Hint:
 *   Use global symbols 'env', 'vpt' and 'vpd'.
 *   Use 'syscall_set_tlb_mod_entry', 'syscall_getenvid', 'syscall_exofork',  and 'duppage'.
 */
int fork(void) {//实现在用户态创建进程
	u_int child;
	u_int i;
	extern volatile struct Env *env;

	/* Step 1: Set our TLB Mod user exception entry to 'cow_entry' if not done yet. */
	//首先判断当前进程的 env_user_tlb_mod_entry 是否已经设置为 cow_entry，
	//如果没有则调用系统调用 syscall_set_tlb_mod_entry 来设置为 cow_entry
	if (env->env_user_tlb_mod_entry != (u_int)cow_entry) {
		try(syscall_set_tlb_mod_entry(0, cow_entry));
	}

	/* Step 2: Create a child env that's not ready to be scheduled. */
	// Hint: 'env' should always point to the current env itself, so we should fix it to the
	// correct value.
	//然后调用系统调用 syscall_exofork 来创建一个子进程，
	//子进程的状态为 ENV_NOT_RUNNABLE，并且其页目录表和页表都和父进程相同。
	child = syscall_exofork();
	if (child == 0) {
		env = envs + ENVX(syscall_getenvid());
		return 0;
	}

	/* Step 3: Map all mapped pages below 'USTACKTOP' into the child's address space. */
	// Hint: You should use 'duppage'.
	/* Exercise 4.15: Your code here. (1/2) */
	//父进程需要将所有映射到 USTACKTOP 以下的虚拟地址都复制到子进程中，
	//这里使用了函数 duppage 来完成。
	//记得判断vpn在页目录和页表是否有效
	for(int i=0;i<USTACKTOP;i+=BY2PG)
	{//vpd vpt分别指向当前环境的页目录表和页表
		if((vpd[PDX(i)]&PTE_V)&&(vpt[VPN(i)]&PTE_V))
		{
			duppage(child,i/BY2PG);
		}
	}
	/* Step 4: Set up the child's tlb mod handler and set child's 'env_status' to
	 * 'ENV_RUNNABLE'. */
	/* Hint:
	 *   You may use 'syscall_set_tlb_mod_entry' and 'syscall_set_env_status'
	 *   Child's TLB Mod user exception entry should handle COW, so set it to 'cow_entry'
	 */
	 //然后父进程再次调用系统调用 syscall_set_tlb_mod_entry 和 syscall_set_env_status 
	 //来设置子进程的页写入异常处理函数为 cow_entry 并将其状态设置为 ENV_RUNNABLE
	/* Exercise 4.15: Your code here. (2/2) */
	//debugf("child0= %d\n",child);
	try(syscall_set_tlb_mod_entry(child, cow_entry));
	//debugf("child1= %d\n",child);
	try(syscall_set_env_status(child, ENV_RUNNABLE));
	//debugf("child= %d\n",child);
	return child;
}
