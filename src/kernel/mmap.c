/**
 * Copyright 2007 by Gabriel Parmer, gabep1@cs.bu.edu
 *
 * Redistribution of this file is permitted under the GNU General
 * Public License v2.
 */

#include "include/mmap.h"

static struct cos_page cos_kernel_pages[COS_KERNEL_MEMORY];

/* JWW */
static paddr_t pages_start = 0x30000000; // 768 MB
static paddr_t pages_extent = (PAGE_SIZE) * COS_MAX_MEMORY; // 4K pages * (# of pages)
/* END JWW */

extern void *cos_alloc_page(void);
extern void *cos_free_page(void *page);
extern void *va_to_pa(void *va);
extern void *pa_to_va(void *pa);

void cos_init_memory(void) 
{
	/* JWW */
	
	int i;

	for (i = 0 ; i < COS_KERNEL_MEMORY ; i++) {
		void *r = cos_alloc_page();
		if (NULL == r) {
			printk("cos: ERROR -- could not allocate page for cos memory\n");
		}
		cos_kernel_pages[i].addr = (paddr_t)va_to_pa(r);
	}

	return;
}

void cos_shutdown_memory(void)
{
	/* I'm not sure there'll be anything here? */

	/* 
	 * Mostly because the kernel won't have a mapping into user memory at all anymore. 
	 * The mem_mgr will just request it all, and manage it later.
	 */
	int i;

	for (i = 0 ; i < COS_KERNEL_MEMORY ; i++) {
		paddr_t addr = cos_kernel_pages[i].addr;
		cos_free_page(pa_to_va((void*)addr));
		cos_kernel_pages[i].addr = 0;
	}
}

/*
 * This would be O(1) in the real implementation as there is a 1-1
 * correspondence between phys pages and memory capabilities, but in
 * our Linux implementation, this is not so.  The least we could do is
 * keep the page sorted by physaddr and do a binary search here.
 */
int cos_paddr_to_cap(paddr_t pa)
{
	return ( (pa - pages_start) / (PAGE_SIZE));
}

int cos_paddr_to_kernel_cap(paddr_t pa) 
{
	/* Will we ever need this? */

	int i;
	for (i = 0 ; i < COS_KERNEL_MEMORY ; i++) {
		if (cos_kernel_pages[i].addr == pa) {
			return i;
		}
	}

	return 0;
}

paddr_t cos_access_page(unsigned long cap_no)
{
	/* And this is just the reverse. Multiplication by 4k, and then addition */

	paddr_t addr;

	if (cap_no > COS_MAX_MEMORY) return 0;
	addr = ((cap_no * (PAGE_SIZE)) + pages_start);
	assert(addr >= pages_start);
	return addr;
}

paddr_t cos_access_kernel_page (unsigned long cap_no)
{
	paddr_t addr;

	//	if (cap_no > COS_KERNEL_MEMORY) return 0;
	assert(cap_no < COS_KERNEL_MEMORY);
	addr = cos_kernel_pages[cap_no].addr;
	printk("Returning kernel page: %x\n", addr);
	assert(addr);
	return addr;

}
