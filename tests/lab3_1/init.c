void mips_init() {
	printk("init.c:\tmips_init() is called\n");
	mips_detect_memory();
	mips_vm_init();
	page_init();
	// printk("666\n");
	env_init();//printk("777 j\n");
	env_check();
	halt();
}
