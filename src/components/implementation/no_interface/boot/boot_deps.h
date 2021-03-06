#include <print.h>
#ifndef assert
#define assert(node) do { if (unlikely(!(node))) { debug_print("assert error in @ "); *((int *)0) = 0;} } while(0)
#endif

#include <mem_mgr.h>
#include <sched.h>
#include <cos_alloc.h>
#include <cobj_format.h>

/* 
 * Abstraction layer around 1) synchronization, 2) scheduling and
 * thread creation, and 3) memory operations.  
 */

/* synchronization... */
#define LOCK()   if (sched_component_take(cos_spd_id())) BUG();
#define UNLOCK() if (sched_component_release(cos_spd_id())) BUG();

/* scheduling/thread operations... */
#define __sched_create_thread_default sched_create_thread_default

/* memory operations... */
#define __mman_get_page   mman_get_page
#define __mman_alias_page mman_alias_page
#define __mman_revoke_page mman_revoke_page

#include <cinfo.h>
#include <cos_vect.h>

COS_VECT_CREATE_STATIC(spd_info_addresses);

int
cinfo_map(spdid_t spdid, vaddr_t map_addr, spdid_t target)
{
	vaddr_t cinfo_addr;

	cinfo_addr = (vaddr_t)cos_vect_lookup(&spd_info_addresses, target);
	if (0 == cinfo_addr) return -1;
	if (map_addr != 
	    (__mman_alias_page(cos_spd_id(), cinfo_addr, spdid, map_addr, 0))) {
		return -1;
	}

	return 0;
}

spdid_t
cinfo_get_spdid(int iter)
{
	if (iter > MAX_NUM_SPDS) return 0;
	if (hs[iter] == NULL) return 0;

	return hs[iter]->id;
}

static int boot_spd_set_symbs(struct cobj_header *h, spdid_t spdid, struct cos_component_information *ci);
static void
comp_info_record(struct cobj_header *h, spdid_t spdid, struct cos_component_information *ci)
{
	if (!cos_vect_lookup(&spd_info_addresses, spdid)) {
		boot_spd_set_symbs(h, spdid, ci);
		cos_vect_add_id(&spd_info_addresses, (void*)round_to_page(ci), spdid);
	}
}

static void
boot_deps_init(void)
{
	cos_vect_init_static(&spd_info_addresses);
}

static void
boot_deps_run(void) { return; }

static void *
boot_get_map_dsrc (vaddr_t ucap_tbl, vaddr_t sched_info, vaddr_t dest_daddr, int *mman_flags) {
	if (dest_daddr == ucap_tbl || dest_daddr == sched_info) {
		*mman_flags |= MMAP_KERN;
	}
	return cos_get_vas_page();
}

static vaddr_t
boot_get_populate_dsrc (vaddr_t ucap_tbl, vaddr_t sched_info, vaddr_t lsrc, int *use_kern_mem) {
	if (lsrc == ucap_tbl || lsrc == sched_info) {
		printc("populating dsrc for kernel in regular booter\n");
		*use_kern_mem = 1;
	}
	return NULL;
}

static int
boot_get_dsrc_increment (int use_kern_mem) { return PAGE_SIZE; }


