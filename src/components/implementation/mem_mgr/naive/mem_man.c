/**
 * Copyright 2008 by Gabriel Parmer, gabep1@cs.bu.edu.  All rights
 * reserved.
 *
 * Completely rewritten to use a sane data-structure based on the L4
 * mapping data-base -- Gabriel Parmer, gparmer@gwu.edu, 2011.
 *
 * Redistribution of this file is permitted under the GNU General
 * Public License v2.
 *
 * I do _not_ use "embedded mapping nodes" here.  That is, I don't
 * embed the mapping nodes into the per-component "page tables" that
 * are used to look up individual mappings in each component.
 * Additionally, instead of the conventional implementation that has
 * these page table structures point to the frame structure that is
 * the base of the mapping tree, we point directly to the mapping to
 * avoid the O(N) cost when mapping where N is the number of nodes in
 * a mapping tree.  The combination of these design decisions means
 * that we might use more memory and have a few more data cache line
 * accesses.  We use a slab allocator to avoid excessive memory usage
 * for allocating memory mapping structures.  However, we use a very
 * fast (and predictable) lookup structure to perform the (component,
 * address)->mapping lookup.  Unfortunately the memory overhead of
 * that is significant (2 pages per component in the common case).
 * See cvectc.h for an alternative that trades (some) speed for memory
 * usage.
 */

/* 
 * FIXME: locking!
 */

#define COS_FMT_PRINT
#include <cos_component.h>
#include <cos_debug.h>
#include <cos_alloc.h>
#include <print.h>

#include <cos_list.h>
#include "../../sched/cos_sched_sync.h"

/* We use the the sched_data_area here only for the mem_mgr lock below. */
struct cos_sched_data_area cos_sched_notifications[NUM_CPU];

#define LOCK()   if (cos_sched_lock_take())    assert(0);
#define UNLOCK() if (cos_sched_lock_release()) assert(0);

/* will need to determine this systematically later */
/* 
 * The memory manager is going to need to know how many pages there
 *  are and what the start address is 
*/



#include <mem_mgr.h>

/***************************************************/
/*** Data-structure for tracking physical memory ***/
/***************************************************/

struct mapping;
/* A tagged union, where the tag holds the number of maps: */
struct frame {
	int nmaps;
	short int is_kern;
	int color;
	union {
		struct mapping *m;  /* nmaps > 0 : root of all mappings */
		vaddr_t addr;	    /* nmaps = -1: local mapping */
		struct frame *free; /* nmaps = 0 : no mapping */
	} c;
};

#define N_COLORS 256

struct frame frames[COS_MAX_MEMORY];
//struct frame *freelist;
struct frame * freelists[N_COLORS];
int last_color;

/* JWW */
struct frame kern_frames[COS_KERNEL_MEMORY];
struct frame *kern_freelist;

static inline int  frame_index(struct frame *f) { 
	if (!(f->is_kern)) {
		return f-frames;
	} else { 
		return f-kern_frames; 
	}
}
static inline int  frame_nrefs(struct frame *f) { return f->nmaps;}
static inline void frame_ref(struct frame *f)   { f->nmaps++; }

/* JWW */
/* static inline int  kern_frame_index(struct frame *f) { return f-kern_frames; } */
/* static inline int  kern_frame_nrefs(struct frame *f) { return f->nmaps; } */
/* static inline void kern_frame_ref(struct frame *f)   { f->nmaps++; } */
/* JWW */

static inline struct frame *
__frame_alloc(int use_kern_mem, int color) {
	assert(color < N_COLORS);
	struct frame *f;

	if (!use_kern_mem) {
		printc("PGCOLOR: allocating frame of color: %d\n", color);
		f = freelists[color];
	} else {
		f = kern_freelist;
	}

	//	if (!f) return NULL;
	assert(f);
	
	if (!use_kern_mem) {
		freelists[color] = f->c.free;
	} else {
		kern_freelist = f->c.free;
	}

	f->nmaps = 0;
	f->c.m   = NULL;

	return f;
}

// so the system will use one half of the colors
#define SYSTEM_COLOR_USAGE 2

static inline struct frame *
frame_alloc(int use_kern_mem)
{
	return __frame_alloc(use_kern_mem, (last_color++) % (N_COLORS / SYSTEM_COLOR_USAGE));
}

static inline void 
frame_deref(struct frame *f)
{ 
	assert(f->nmaps > 0);
	f->nmaps--; 
	if (f->nmaps == 0) {
		if (!(f->is_kern)) {
			f->c.free = freelists[f->color];
			freelists[f->color] = f;
		} else {
			f->c.free = kern_freelist;
			kern_freelist  = f;
		}
	}
}

static void
frame_init(void)
{
	int i;

	for (i = 0 ; i < COS_MAX_MEMORY ; i++) {

		if (i + N_COLORS < COS_MAX_MEMORY) {
			frames[i].c.free = &frames[i + N_COLORS];
		} else {
			printc("frame_init: Ended freelist at index %d, size: %d\n", i, (&frames[i] - frames) / (N_COLORS));
			frames[i].c.free = NULL;
		}
		//		frames[i].c.free = &frames[i+1];
		frames[i].nmaps  = 0;
		frames[i].is_kern = 0;
		frames[i].color = i % N_COLORS;
	}
	//	frames[COS_MAX_MEMORY-1].c.free = NULL;
	
	//	freelist = &frames[0];
	
	assert(N_COLORS < COS_MAX_MEMORY);

	for (i = 0; i < N_COLORS; i++) {
		freelists[i] = &frames[i];
	}
	last_color = 0;
}

/* JWW */
static void
kern_frame_init(void)
{
	int i;

	for (i = 0 ; i < COS_KERNEL_MEMORY-1 ; i++) {
		kern_frames[i].c.free = &kern_frames[i+1];
		kern_frames[i].nmaps  = 0;
		kern_frames[i].is_kern = 1;
	}
	kern_frames[COS_KERNEL_MEMORY-1].c.free = NULL;
	kern_freelist = &kern_frames[0];
}
/* /JWW */

static inline void
mm_init(void)
{
	printc("mm init as thread %d\n", cos_get_thd_id());

	frame_init();
	kern_frame_init();
}

/*************************************/
/*** Memory allocation shenanigans ***/
/*************************************/

static inline struct frame *frame_alloc(int use_kern_mem);
static inline int frame_index(struct frame *f);
static inline void *
__page_get(void)
{
	void *hp = cos_get_vas_page();
	struct frame *f = frame_alloc(0);

	assert(hp && f);
	frame_ref(f);
	f->nmaps  = -1; 	 /* belongs to us... */
	f->c.addr = (vaddr_t)hp; /* ...at this address */
	if (cos_mmap_cntl(COS_MMAP_GRANT, 0, cos_spd_id(), (vaddr_t)hp, frame_index(f))) {
		//		printc("grant @ %p for frame %d\n", hp, frame_index(f));
		BUG();
	}

	//	printc("page_get memset addr: %x\n", (unsigned int) hp);
	memset(hp, 0, PAGE_SIZE);

	return hp;
}
#define CPAGE_ALLOC() __page_get()
#include <cpage_alloc.h>

#define CSLAB_ALLOC(sz)   cpage_alloc()
#define CSLAB_FREE(x, sz) cpage_free(x)
#include <cslab.h>

#define CVECT_ALLOC() cpage_alloc()
#define CVECT_FREE(x) cpage_free(x)
#include <cvect.h>

/**********************************************/
/*** Virtual address tracking per component ***/
/**********************************************/

CVECT_CREATE_STATIC(comps);
struct comp_vas {
	int nmaps, spdid;
	cvect_t *pages;
};
CSLAB_CREATE(cvas, sizeof(struct comp_vas));

static struct comp_vas *
cvas_lookup(spdid_t spdid)
{ return cvect_lookup(&comps, spdid); }

static struct comp_vas *
cvas_alloc(spdid_t spdid)
{
	struct comp_vas *cv;

	assert(!cvas_lookup(spdid));
	cv = cslab_alloc_cvas();
	if (!cv) goto done;
	cv->pages = cvect_alloc();
	if (!cv->pages) goto free;
	cvect_init(cv->pages);
	cvect_add(&comps, cv, spdid);
	cv->nmaps = 0;
	cv->spdid = spdid;
done:
	return cv;
free:
	cslab_free_cvas(cv);
	cv = NULL;
	goto done;
}

static void
cvas_ref(struct comp_vas *cv)
{
	assert(cv);
	cv->nmaps++;
}

static void 
cvas_deref(struct comp_vas *cv)
{
	assert(cv && cv->nmaps > 0);
	cv->nmaps--;
	if (cv->nmaps == 0) {
		cvect_free(cv->pages);
		cvect_del(&comps, cv->spdid);
		cslab_free_cvas(cv);
	}
}

/**************************/
/*** Mapping operations ***/
/**************************/

struct mapping {
	u16_t   flags;
	spdid_t spdid;
	vaddr_t addr;

	struct frame *f;
	/* child and sibling mappings */
	struct mapping *p, *c, *_s, *s_;
} __attribute__((packed));
CSLAB_CREATE(mapping, sizeof(struct mapping));

static void
mapping_init(struct mapping *m, spdid_t spdid, vaddr_t a, struct mapping *p, struct frame *f)
{
	assert(m && f);
	INIT_LIST(m, _s, s_);
	m->f     = f;
	m->flags = 0;
	m->spdid = spdid;
	m->addr  = a;
	m->p     = p;
	if (p) {
		m->flags = p->flags;
		if (!p->c) p->c = m;
		else       ADD_LIST(p->c, m, _s, s_);
	}
}

static struct mapping *
mapping_lookup(spdid_t spdid, vaddr_t addr)
{
	//	printc("doing a mapping lookup\n");
	struct comp_vas *cv = cvas_lookup(spdid);

	if (!cv) return NULL;
	return cvect_lookup(cv->pages, addr >> PAGE_SHIFT);
}

/* Make a child mapping */
static struct mapping *
mapping_crt(struct mapping *p, struct frame *f, spdid_t dest, vaddr_t to)
{
	//	printc("JWW: calling mapping_crt in mem_man.c\n");
	struct comp_vas *cv = cvas_lookup(dest);
	struct mapping *m = NULL;
	long idx = to >> PAGE_SHIFT;
	long flags = 0;

	assert(!p || p->f == f);
	assert(dest && to);

	/* no vas structure for this spd yet... */
	if (!cv) {
		cv = cvas_alloc(dest);
		if (!cv) goto done;
		assert(cv == cvas_lookup(dest));
	}
	assert(cv->pages);
	if (cvect_lookup(cv->pages, idx)) {
		printc("Collision in mapping_crt: cvect_lookup failed\n");
		goto collision;
	}

	cvas_ref(cv);
	m = cslab_alloc_mapping();
	if (!m) {
		printc("Collision in mapping_crt: cslab_alloc failed\n");
		goto collision;
	}

	if (f->is_kern) {
		flags |= MMAP_KERN;
	}

	
	printc("Granting memory to %d at %x with mem_id: %d, flags: %d\n", (int) dest, (unsigned int) to, (int) frame_index(f), (int) flags);
	if (cos_mmap_cntl(COS_MMAP_GRANT, flags, dest, to, frame_index(f))) {
		printc("mem_man: could not grant at %x:%d\n", dest, (int)to);
		goto no_mapping;
	}
	
	/* if (dest == cos_spdid()) { */
	/* 	int x = *((int *) to); */
	/* } */
	//memset(to, 0, PAGE_SIZE);

	
	mapping_init(m, dest, to, p, f);
	assert(!p || frame_nrefs(f) > 0);
	frame_ref(f);
	assert(frame_nrefs(f) > 0);
	if (cvect_add(cv->pages, m, idx)) BUG();
done:
	return m;
no_mapping:
	cslab_free_mapping(m);
collision:
	cvas_deref(cv);
	m = NULL;
	goto done;
}

/* Take all decedents, return them in a list. */
static struct mapping *
__mapping_linearize_decendents(struct mapping *m)
{
	struct mapping *first, *last, *c, *gc;
	
	first = c = m->c;
	m->c = NULL;
	if (!c) return NULL;
	do {
		last = LAST_LIST(first, _s, s_);
		c->p = NULL;
		gc = c->c;
		c->c = NULL;
		/* add the grand-children onto the end of our list of decedents */
		if (gc) ADD_LIST(last, gc, _s, s_);
		c = FIRST_LIST(c, _s, s_);
	} while (first != c);
	
	return first;
}

static void
__mapping_destroy(struct mapping *m)
{
	struct comp_vas *cv;
	int idx;

	assert(m);
	assert(EMPTY_LIST(m, _s, s_));
	assert(m->p == NULL);
	assert(m->c == NULL);
	cv = cvas_lookup(m->spdid);

	assert(cv && cv->pages);
	assert(m == cvect_lookup(cv->pages, m->addr >> PAGE_SHIFT));
	cvect_del(cv->pages, m->addr >> PAGE_SHIFT);
	cvas_deref(cv);
	idx = cos_mmap_cntl(COS_MMAP_REVOKE, 0, m->spdid, m->addr, 0);
	assert(idx == frame_index(m->f));
	frame_deref(m->f);
	cslab_free_mapping(m);
}

static void
mapping_del_children(struct mapping *m)
{
	struct mapping *d, *n; 	/* decedents, next */

	assert(m);
	d = __mapping_linearize_decendents(m);
	while (d) {
		n = FIRST_LIST(d, _s, s_);
		REM_LIST(d, _s, s_);
		__mapping_destroy(d);
		d = (n == d) ? NULL : n;
	}
	assert(!m->c);
}

static void
mapping_del(struct mapping *m)
{
	assert(m);
	mapping_del_children(m);
	assert(!m->c);
	if (m->p && m->p->c == m) {
		if (EMPTY_LIST(m, _s, s_)) m->p->c = NULL;
		else                       m->p->c = FIRST_LIST(m, _s, s_);
	}
	m->p = NULL;
	REM_LIST(m, _s, s_);
	__mapping_destroy(m);
}

/**********************************/
/*** Public interface functions ***/
/**********************************/

vaddr_t mman_get_page_color(spdid_t spd, vaddr_t addr, int flags, int color)
{

	/* JWW */
	int use_kern_mem = 0;
	if (flags & 1) {
		use_kern_mem = 1;
	}
	/* / JWW */
	struct frame *f;
	struct mapping *m = NULL;
	vaddr_t ret = -1;

	LOCK();
	if (color >= 0) {
		printc("using color: %d\n", color);
		f = __frame_alloc(use_kern_mem, color);
	} else {
		f = frame_alloc(use_kern_mem);
	}

	if (!f){
		printc("mem_man: out of memory or out of memory of this color: %d\n", color);
		goto done; 	/* -ENOMEM */	
	}

	//	printc("mapping page into mm at addr: %x\n", cos_get_heap_ptr());
	printc("frame_index: %d\n", frame_index(f));

	assert(!cos_mmap_cntl(COS_MMAP_GRANT, flags, cos_spd_id(), cos_get_heap_ptr(), frame_index(f)));
	memset(cos_get_heap_ptr(), 0, PAGE_SIZE);
	/* printc("mem_man dereferencing...\n"); */
	/* *((char *) cos_get_heap_ptr()) = 0; */
	//	printc("revoking page into mm at addr: %x\n", cos_get_heap_ptr());
	cos_mmap_cntl(COS_MMAP_REVOKE, flags, cos_spd_id(), cos_get_heap_ptr(), 0);
	//	printc("flushing tlb\n");
	cos_mmap_cntl(COS_MMAP_TLBFLUSH, 0, cos_spd_id(), cos_get_heap_ptr(), 0);


	assert(frame_nrefs(f) == 0);
	frame_ref(f);
	m = mapping_crt(NULL, f, spd, addr);
	if (!m) goto dealloc;
	f->c.m = m;
	assert(m->addr == addr);
	assert(m->spdid == spd);
	assert(m == mapping_lookup(spd, addr));
	ret = m->addr;
done:
	UNLOCK();
	return ret;
dealloc:
	frame_deref(f);
	ret = NULL;
	goto done;		/* -EINVAL */
}

vaddr_t mman_get_page(spdid_t spd, vaddr_t addr, int flags) {
	return mman_get_page_color(spd, addr, flags, -1);
}

vaddr_t mman_alias_page(spdid_t s_spd, vaddr_t s_addr, spdid_t d_spd, vaddr_t d_addr, int flags)
{
	printc("JWW: Calling mman_alias_page in mem_man.c s_spd: %d, d_spd: %d, s_addr: %x, d_addr: %x, flags: %d\n", s_spd, d_spd, s_addr, d_addr, flags);
	struct mapping *m, *n;
	vaddr_t ret = 0;

	LOCK();
	m = mapping_lookup(s_spd, s_addr);
	//printc("finished mapping lookup\n");
	if (!m){
		//		printc("mapping lookup failed!\n");
		goto done; 	/* -EINVAL */
	}


	n = mapping_crt(m, m->f, d_spd, d_addr);
	//	printc("finished mapping \n");
	if (!n) { 
		//		printc("mapping crt failed!\n");
		goto done;
	}

	assert(n->addr  == d_addr);
	assert(n->spdid == d_spd);
	assert(n->p     == m);
	ret = d_addr;
done:
	UNLOCK();
	return ret;
}

int mman_revoke_page(spdid_t spd, vaddr_t addr, int flags)
{
	struct mapping *m;
	int ret = 0;

	LOCK();
	m = mapping_lookup(spd, addr);
	if (!m) {
		ret = -1;	/* -EINVAL */
		goto done;
	}
	mapping_del_children(m);
done:
	UNLOCK();
	return ret;
}

int mman_release_page(spdid_t spd, vaddr_t addr, int flags)
{
	struct mapping *m;
	int ret = 0;

	LOCK();
	m = mapping_lookup(spd, addr);
	if (!m) {
		ret = -1;	/* -EINVAL */
		goto done;
	}
	mapping_del(m);
done:
	UNLOCK();
	return ret;
}

void mman_print_stats(void) {}

void mman_release_all(void)
{
	int i;

	LOCK();
	/* kill all mappings in other components */
	for (i = 0 ; i < COS_MAX_MEMORY ; i++) {
		struct frame *f = &frames[i];
		struct mapping *m;

		if (frame_nrefs(f) <= 0) continue;
		m = f->c.m;
		assert(m);
		mapping_del(m);
	}
	/* kill local mappings */
	/* for (i = 0 ; i < COS_MAX_MEMORY ; i++) { */
	/* 	struct frame *f = &frames[i]; */
	/*      int idx; */

	/* 	if (frame_nrefs(f) >= 0) continue; */
	/* 	idx = cos_mmap_cntl(COS_MMAP_REVOKE, 0, cos_spd_id(), f->c.addr, 0); */
	/* 	assert(idx == frame_index(f)); */
	/* } */
	UNLOCK();
}

/*******************************/
/*** The base-case scheduler ***/
/*******************************/

#include <sched_hier.h>

int  sched_init(void)   { return 0; }

extern void parent_sched_exit(void);

static volatile int initialized_core[NUM_CPU] = { 0 }; /* record the cores that still depend on us */

void 
sched_exit(void)   
{
	int i;
	initialized_core[cos_cpuid()] = 0;
	if (cos_cpuid() == INIT_CORE) {
		/* The init core waiting for all cores to exit. */
		for (i = 0; i < NUM_CPU ; i++)
			if (initialized_core[i]) i = 0;
		mman_release_all(); 
	}
	parent_sched_exit();
}

int sched_isroot(void) { return 1; }

int 
sched_child_get_evt(spdid_t spdid, struct sched_child_evt *e, int idle, unsigned long wake_diff) { BUG(); return 0; }

extern int parent_sched_child_cntl_thd(spdid_t spdid);

int 
sched_child_cntl_thd(spdid_t spdid) 
{ 
	if (parent_sched_child_cntl_thd(cos_spd_id())) BUG();
	if (cos_sched_cntl(COS_SCHED_PROMOTE_CHLD, 0, spdid)) BUG();
	if (cos_sched_cntl(COS_SCHED_GRANT_SCHED, cos_get_thd_id(), spdid)) BUG();

	return 0;
}

int 
sched_child_thd_crt(spdid_t spdid, spdid_t dest_spd) { BUG(); return 0; }

void cos_upcall_fn(upcall_type_t t, void *arg1, void *arg2, void *arg3)
{
	//  cos_syscall_mmap_cntl(int spdid, long op_flags_dspd, vaddr_t daddr, unsigned long mem_id)
	printc("JWW: Initializing NAIVE MEM_MAN\n");
	/* void *hp = cos_get_vas_page(); */
	/* cos_mmap_cntl(COS_MMAP_GRANT, 0, cos_spd_id(), (vaddr_t)hp, 0x11 << 28); // JWW */
	/* int *test = (int *) hp; */
	/* *test = 0xDEADBEEF; */
	/* printc("JWW: Dereferenced large address %x\n", *test); */

	/* void *hp2 = cos_get_vas_page(); */
	/* cos_mmap_cntl(COS_MMAP_GRANT, 0, cos_spd_id(), (vaddr_t)hp2, (0x11 << 28) + (COS_MAX_MEMORY * (1 << 13))); // JWW */
	/* int *test2 = (int *) hp2; */
	/* *test2 = 0xDEADBEEF; */
	/* printc("JWW: Dereferenced second large address %x\n", *test2); */

	switch (t) {
	case COS_UPCALL_BOOTSTRAP:
		if (cos_cpuid() == INIT_CORE) {
			mm_init(); 
		} else {
			while (initialized_core[INIT_CORE] == 0) ;
		}
		initialized_core[cos_cpuid()] = 1;
		break;			
	default:
		BUG(); return;
	}

	return;
}
