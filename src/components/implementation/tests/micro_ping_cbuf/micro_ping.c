#include <cos_component.h>
#include <print.h>
#include <stdlib.h>
#include <sched.h>
#include <micro_pong.h>
#include <cbuf.h>
#include <timed_blk.h>
#include <vas_mgr.h>


#define ITER 2000
#define MAX_SZ 4096
#define NCBUF 100

// using threadpool_max

#define ITERATIONS 10000

void cos_init(void)
{
	/* u64_t start, end, start_tmp, end_tmp; */
	/* int i, k, prev_sz = 1; */

	/* cbuf_t cbt[NCBUF]; */
	/* memset(cbt, 0 , NCBUF*sizeof(cbuf_t)); */
	/* void *mt[NCBUF]; */
	/* unsigned int sz[NCBUF]; */

	/* for (i = 0; i < NCBUF ; i++){ */
	/* 	cbt[i] = cbuf_null(); */
	/* 	sz[i] = 0; */
	/* } */

	/* printc("\nMICRO BENCHMARK TEST (PINGPONG WITH CBUF)\n"); */

        /* /\* RDTSCLL *\/ */
	/* printc("\n<<< RDTSCLL MICRO-BENCHMARK TEST >>>\n"); */
	/* rdtscll(start_tmp); */
	/* for (i = 0 ; i < ITER ; i++) { */
	/* 	rdtscll(start); */
	/* } */
	/* rdtscll(end_tmp); */
	/* printc("%d rdtscll avg %lld cycs\n", ITER, (end_tmp-start_tmp)/ITER); */
	
        /* /\* PINGPONG *\/ */
	/* printc("\n<<< PINGPONG MICRO-BENCHMARK TEST >>>\n"); */
	/* call(); */
	/* for (k = 0; k <10 ;k++){ */
		
	/* 	rdtscll(start); */
	/* 	for (i = 0 ; i < ITER ; i++) { */
	/* 		call(); */
	/* 	} */
	/* 	rdtscll(end); */
	/* 	printc("%d invs avg %lld cycs\n", ITER, (end-start)/ITER); */
	/* } */
	/* printc("<<< PINGPONG BENCHMARK TEST DONE >>>\n"); */

        /* /\* CACHING *\/ */
	/* printc("\n<<< WARM UP CBUF CACHE......."); */
	/* for (i = 0; i < NCBUF ; i++){ */
	/* 	prev_sz += 4; */
	/* 	prev_sz &= PAGE_SIZE-1; */
	/* 	sz[i] = prev_sz;		 */
	/* 	mt[i] = cbuf_alloc(sz[i], &cbt[i]); */
	/* } */

	/* for (i = 0; i < NCBUF ; i++){ */
	/* 	simple_call_buf2buf(cbt[i], sz[i]); */
	/* } */

	/* for (i = 0; i < NCBUF ; i++){ */
	/* 	cbuf_free(mt[i]); */
	/* } */
	/* printc(" Done! >>>\n"); */

        /* /\* CBUF_ALLOC  *\/ */
	/* printc("\n<<< CBUF_ALLOC MICRO-BENCHMARK TEST >>>\n"); */
	/* rdtscll(start); */
	/* for (i = 0; i < NCBUF ; i++){ */
	/* 	prev_sz += 4; */
	/* 	prev_sz &= PAGE_SIZE-1; */
	/* 	sz[i] = prev_sz; */
	/* 	mt[i] = cbuf_alloc(sz[i], &cbt[i]);  */
	/* } */
	/* rdtscll(end); */
	/* printc("%d alloc_cbuf %llu cycs\n", NCBUF, (end-start)/NCBUF); */
	/* printc("<<< CBUF_ALLOC MICRO-BENCHMARK TEST DONE >>>\n"); */

        /* /\* CBUF2BUF  *\/ */
	/* printc("\n<<< CBUF2BUF MICRO-BENCHMARK TEST >>>\n"); */
	/* for (i = 0; i < NCBUF ; i++){ */
	/* 	call_buf2buf(cbt[i], sz[i]); */
	/* } */
	/* printc("<<< CBUF2BUF MICRO-BENCHMARK TEST DONE >>>\n"); */

        /* /\* CBUF_FREE  *\/ */
	/* printc("\n<<< CBUF_FREE MICRO-BENCHMARK TEST >>>\n"); */
	/* rdtscll(start); */
	/* for (i = 0; i < NCBUF ; i++){ */
	/* 	cbuf_free(mt[i]);                 */
	/* } */
	/* rdtscll(end); */
	/* printc("%d free_cbuf %llu cycs avg\n", NCBUF, (end-start)/NCBUF); */
	/* printc("<<< CBUF_FREE MICRO-BENCHMARK TEST DONE >>>\n"); */

        /* /\* CBUF_ALLOC-CBUF2BUF-CBUF_FREE *\/ */
	/* printc("\n<<< CBUF_ALLOC-CBUF2BUF-CBUF_FREE MICRO-BENCHMARK TEST >>>\n"); */
	/* prev_sz += 4; */
	/* prev_sz &= PAGE_SIZE-1; */
	/* sz[0] = prev_sz; */
	/* rdtscll(start); */
	/* for (i = 0; i < ITER ; i++){ */
	/* 	mt[0] = cbuf_alloc(sz[0], &cbt[0]); */
	/* 	simple_call_buf2buf(cbt[0], sz[0]); */
	/* 	cbuf_free(mt[0]); */
	/* } */
	/* rdtscll(end); */
	/* printc("%d alloc-cbuf2buf-free %llu cycles avg\n", ITER, (end-start)/ITER); */

	/* printc("<<< CBUF_ALLOC-CBUF2BUF-CBUF_FREE MICRO-BENCHMARK TEST DONE >>>\n"); */

	/* printc("\nMICRO BENCHMARK TEST (PINGPONG WITH CBUF) DONE!\n\n"); */


	int i, color_range, color_start;
	unsigned int j;
	vaddr_t working_set_addr, ret;
	u64_t start, end;
	int working_set_size = 500;
	
	if (strcmp(cos_init_args(), "small_run") == 0) {
		color_start = 160;
		working_set_size = 200;
		color_range = 20;
	} else if (strcmp(cos_init_args(), "large_run") == 0) {
		color_range = 40;
		color_start = 140;
		working_set_size = 950;
	} else if (strcmp(cos_init_args(), "small_run_separate") == 0) {
		printc("Letting the other task do its thing.\n");
		color_start = 181;
		color_range = 20;
		working_set_size = 200;
	} else if (strcmp(cos_init_args(), "large_run_separate") == 0) {
		color_range = 40;
		color_start = 181;
		working_set_size = 950;
	} else if (strcmp(cos_init_args(), "single_multiple_colors") == 0) {
		color_range = 64;
		color_start = 181;
		working_set_size = 2048;
	} else if (strcmp(cos_init_args(), "single_one_color") == 0) {
		color_range = 18;
		color_start = 220;
		working_set_size = 2048;
	} else {
		printc("Invalid color_start argument, use 220 or 230\n");
		return;
	}

	//	vaddr_t working_set[working_set_size];

	printc("---------- PAGE COLORING TEST STARTING ------\n");


	/* 
	 * I don't currently know why there is always a collision on
	 * whatever address cos_get_vas_page() returns the first time
	 * around. I'm probably not incrementing something
	 * correctly. For now, this works. It also sucks.
	*/
	cos_get_vas_page(); 

	timed_event_block(cos_spd_id(), 1);

	long size =  (long) working_set_size * (long) PAGE_SIZE; 
	working_set_addr = vas_mgr_expand(cos_spd_id(), size * 2);
	printc("pcolor bench: got %ld bytes of VAS space at %x\n", size, (unsigned int) working_set_addr);
	if (working_set_addr == 0) {
		printc("pcolor benchmark: could not get enough vaddr space to run benchmark. Exiting.\n");
		return;
	}
	
	vaddr_t offset = 0;
	
	for (i = 0; i < working_set_size; i++) {
		//		working_set[i] = cos_get_vas_page();
		//		working_set[i] = array_offset;
		//		printc("Getting page at %x\n", (unsigned int) working_set[i]);
		ret = mman_get_page_color(cos_spd_id(), working_set_addr + offset, 0, color_start + (i % color_range));
		/* printc("Dereferencing page: %x\n", (unsigned int) working_set[i]); */
		/* int x = *((int *) working_set[i]); */
		//		ret = mman_get_page_color(cos_spd_id(), working_set[i], 0, -1);
		offset += PAGE_SIZE;
		if (!ret) {
			printc("Out of memory of this color, got %d pages\n", i);
			return;
		}
		printc("Got page number %d at %x\n", i, (unsigned int) ret);
	}

	//	cos_mmap_cntl(COS_MMAP_TLBFLUSH, 0, cos_spd_id(), cos_get_heap_ptr(), 0);

	vaddr_t working_set_start = working_set_addr;

	timed_event_block(cos_spd_id(), 1);

	/* volatile int rend = 0; //global */

	/* //in the startup-function before measurements */
	/* rend++; */
	/* while (rend < 2); */
	
	printc("Starting iterations\n");
	rdtscll(start);

	for (i = 0; i < ITERATIONS; i++) {
		//		printc("SPD %d, Iteration: %u \n", cos_spd_id(),i);
		
		for (j = 0; j < (working_set_size * PAGE_SIZE) / 2; j += sizeof(int)) {
			/* int *addr = (int *)((unsigned int) working_set_start + (unsigned int) j); */
			/* *addr = *addr + 1; */
			int *addr = (int *)((unsigned int) working_set_start + (unsigned int) j); 
			/* if ((unsigned int)addr % PAGE_SIZE == 0) */
			/* 	printc("pcolor bench: accessing address %x\n", (unsigned int) addr); */
			*addr = *addr + 1;
			/* addr = (int *)((unsigned int) working_set_start + (((unsigned int) working_set_size * PAGE_SIZE) - (unsigned int) j)) - 1; */
			/* printc("pcolor bench: accessing address %x\n", (unsigned int) addr); */
			/* *addr = *addr + 1; */
			//			printc("pcolor bench: finished loop\n");
					
		}
	}
	rdtscll(end);
	printc("End of iterations, color start: %d | SPD %d, %d Iterations, %llu cycles avg\n", color_start, cos_spd_id(), ITERATIONS, (end-start)/ITERATIONS);
	return;
}


