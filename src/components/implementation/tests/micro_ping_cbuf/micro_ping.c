#include <cos_component.h>
#include <print.h>
#include <stdlib.h>
#include <sched.h>
#include <micro_pong.h>
#include <cbuf.h>
#include <timed_blk.h>
#include <vas_mgr.h>

#define ITERATIONS 10000

static int is_thread = 0;

static vaddr_t working_set_addr;

void cos_init(void)
{


	int i, color_range, color_start;
	unsigned int j;
	vaddr_t ret;
	u64_t start, end;
	int working_set_size = 500;
	
	if (strcmp(cos_init_args(), "small_run") == 0) {
		color_start = 160;
		working_set_size = 200;
		color_range = 20;
	} else if (strcmp(cos_init_args(), "large_run") == 0) {
		color_range = 40;
		color_start = 140;
		working_set_size = 2048;
	} else if (strcmp(cos_init_args(), "small_run_separate") == 0) {
		printc("Letting the other task do its thing.\n");
		color_start = 181;
		color_range = 20;
		working_set_size = 200;
	} else if (strcmp(cos_init_args(), "large_run_separate") == 0) {
		color_range = 40;
		color_start = 181;
		working_set_size = 2048;
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


	printc("---------- PAGE COLORING TEST STARTING CORE: %d ------\n", cos_cpuid());
	
	if (!is_thread) {
		/* 
		 * I don't currently know why there is always a collision on
		 * whatever address cos_get_vas_page() returns the first time
		 * around. I'm probably not incrementing something
		 * correctly. For now, this works. It also sucks.
		 */
		cos_get_vas_page(); 

		//	printc("pcolor bench core %d: calling timed_event_block\n", cos_cpuid());

		//	timed_event_block(cos_spd_id(), 1);

		printc("pcolor bench core %d: calling vas_mgr_expand\n", cos_cpuid());

		long size =  (long) working_set_size * (long) PAGE_SIZE; 
		working_set_addr = vas_mgr_expand(cos_spd_id(), PAGE_SIZE * 2048);
		printc("pcolor bench: got %ld bytes of VAS space at %x\n", size, (unsigned int) working_set_addr);
		if (working_set_addr == 0) {
			printc("pcolor benchmark: could not get enough vaddr space to run benchmark. Exiting.\n");
			return;
		}
	
		printc("pcolor benchmark: Starting to get %d colored pages...\n", working_set_size);

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
			is_thread = 1;
			printc("pcolor bench: Creating thread from spd %d on core %d\n", cos_spd_id(), cos_cpuid());
			sched_create_thd(cos_spd_id(), 0, 0, 0);
			return;

	}

	
	printc("I'm the other thread from spd %d on core %d\n", cos_spd_id(), cos_cpuid());
	vaddr_t working_set_start = working_set_addr;
	printc("Starting iterations after block.\n");

	//	timed_event_block(cos_spd_id(), 1);
	
	while (!rendezvous(cos_spd_id()));
	printc("Successful rendezvous on spd %d\n", cos_spd_id());
	
	rdtscll(start);

	for (i = 0; i < ITERATIONS; i++) {
		if (i % 1000 == 0) {
			printc("SPD %d, Iteration: %u \n", cos_spd_id(),i);
		}
		for (j = 0; j < (working_set_size * PAGE_SIZE) / 2; j += sizeof(int)) {
			/* int *addr = (int *)((unsigned int) working_set_start + (unsigned int) j); */
			/* *addr = *addr + 1; */
			int *addr = (int *)((unsigned int) working_set_start + (unsigned int) j); 
			/* if ((unsigned int)addr % PAGE_SIZE == 0) */
			/* 	printc("pcolor bench: accessing address %x\n", (unsigned int) addr); */
			*addr = *addr + 1;
			addr = (int *)((unsigned int) working_set_start + (((unsigned int) working_set_size * PAGE_SIZE) - (unsigned int) j)) - 1;
			//			printc("pcolor bench: accessing address %x\n", (unsigned int) addr);
			*addr = *addr + 1;
			//			printc("pcolor bench: finished loop\n");
					
		}
	}
	rdtscll(end);
	printc("End of iterations, color start: %d | SPD %d, %d Iterations, %llu cycles avg\n", color_start, cos_spd_id(), ITERATIONS, (end-start)/ITERATIONS);
	return;
}


