/**
 * Copyright 2007 by Gabriel Parmer, gabep1@cs.bu.edu
 *
 * Redistribution of this file is permitted under the GNU General
 * Public License v2.
 */

#include "include/mmap.h"

static struct cos_page cos_pages[COS_MAX_MEMORY];

/* JWW */
//static paddr_t pages_start = 0x11 << 28; // 768 MB
static paddr_t pages_start = 0x30000000 - ((PAGE_SIZE) * COS_MAX_MEMORY); // 768 MB
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

	/* for (i = 0 ; i < COS_MAX_MEMORY ; i++) { */
	/* 	void *r = cos_alloc_page(); */
	/* 	if (NULL == r) { */
	/* 		printk("cos: ERROR -- could not allocate page for cos memory\n"); */
	/* 	} */
	/* 	cos_pages[i].addr = (paddr_t)va_to_pa(r); */
	/* } */

	/* return; */

	
}

void cos_shutdown_memory(void)
{
	/* I'm not sure there'll be anything here? */

	/* 
	 * Mostly because the kernel won't have a mapping into user memory at all anymore. 
	 * The mem_mgr will just request it all, and manage it later.
	 */
	/* int i; */

	/* for (i = 0 ; i < COS_MAX_MEMORY ; i++) { */
	/* 	paddr_t addr = cos_pages[i].addr; */
	/* 	cos_free_page(pa_to_va((void*)addr)); */
	/* 	cos_pages[i].addr = 0; */
	/* } */
}

/*
 * This would be O(1) in the real implementation as there is a 1-1
 * correspondence between phys pages and memory capabilities, but in
 * our Linux implementation, this is not so.  The least we could do is
 * keep the page sorted by physaddr and do a binary search here.
 */
int cos_paddr_to_cap(paddr_t pa)
{
	/* 
	 * In the implementation I'm intending, there'll be just some math here.
	 * All we'll need to do is take the pa, subtract the base pa, and divide by 4k.
	 */

	/* int i; */

	/* for (i = 0 ; i < COS_MAX_MEMORY ; i++) { */
	/* 	if (cos_pages[i].addr == pa) { */
	/* 		return i; */
	/* 	} */
	/* } */

	/* return 0; */

	return ( (pa - pages_start) / (1 << 13) );
}   

paddr_t cos_access_page(unsigned long cap_no)
{
	/* And this is just the reverse. Multiplication by 4k, and then addition */

	paddr_t addr;

	if (cap_no > COS_MAX_MEMORY) return 0;
	/* addr = cos_pages[cap_no].addr; */
	/* assert(addr); */

	/* return addr; */
	
	addr = ((cap_no * (PAGE_SIZE)) + pages_start);
	assert(addr);
	return addr;
}
