#include <time.h>

#include "test.h"
#include "error.h"

#define NSEC_PER_SEC	1000000000L

/*
static long long benchmark_iter(struct hash *h, bool tagged)
{
	volatile unsigned long sink = 0;
	struct hash_iter iter;
	struct timespec start, finish;
	long long nsec;
	int l, loops = 1;
	void **slot;

#ifdef BENCHMARK
again:
#endif
	clock_gettime(CLOCK_MONOTONIC, &start);
	for (l = 0; l < loops; l++) {
		hash_for_each_slot(slot, h, &iter, 0)
			sink ^= (unsigned long)slot;
	}
	clock_gettime(CLOCK_MONOTONIC, &finish);

	nsec = (finish.tv_sec - start.tv_sec) * NSEC_PER_SEC +
	       (finish.tv_nsec - start.tv_nsec);

#ifdef BENCHMARK
	if (loops == 1 && nsec * 5 < NSEC_PER_SEC) {
		loops = NSEC_PER_SEC / nsec / 4 + 1;
		goto again;
	}
#endif

	nsec /= loops;
	return nsec;
}
*/

static void benchmark_insert(struct hash *h,
			     unsigned long size, unsigned long step)
{
	struct timespec start, finish;
	unsigned long index;
	long long nsec;

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (index = 0 ; index < size ; index += step)
		item_insert(h, index);

	clock_gettime(CLOCK_MONOTONIC, &finish);

	nsec = (finish.tv_sec - start.tv_sec) * NSEC_PER_SEC +
	       (finish.tv_nsec - start.tv_nsec);

	printv(1, "Size: %8ld, step: %8ld, insertion: %15lld ns\n",
		size, step, nsec);
}

static void benchmark_search(struct hash *h,
			     unsigned long size, unsigned long step)
{
	struct timespec start, finish;
	unsigned long index;
	long long nsec;

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (index = 0 ; index < size ; index += step)
		item_check_present(h, index);

	clock_gettime(CLOCK_MONOTONIC, &finish);

	nsec = (finish.tv_sec - start.tv_sec) * NSEC_PER_SEC +
	       (finish.tv_nsec - start.tv_nsec);

	printv(1, "Size: %8ld, step: %8ld, search: %15lld ns\n",
		size, step, nsec);
}

#if 0
static void benchmark_delete(struct hash *h,
			     unsigned long size, unsigned long step)
{
	struct timespec start, finish;
	unsigned long index;
	long long nsec;

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (index = 0 ; index < size ; index += step)
		item_delete(h, index);

	clock_gettime(CLOCK_MONOTONIC, &finish);

	nsec = (finish.tv_sec - start.tv_sec) * NSEC_PER_SEC +
	       (finish.tv_nsec - start.tv_nsec);

	printv(1, "Size: %8ld, step: %8ld, deletion: %16lld ns\n",
		size, step, nsec);
}
#endif 

static void benchmark_size(unsigned long size, unsigned long step)
{
	struct hash *h = hash_alloc(size);
	long long normal, tagged;

	benchmark_insert(h, size, step);
	benchmark_search(h, size, step);

//	benchmark_delete(h, size, step);

//	item_kill_tree(h);
//	rcu_barrier();
}


void my_benchmark() {
    unsigned long indices[5] = {2, 6, 22, 37, 65};
	unsigned long item[5] = {2, 6, 22, 37, 65};

	struct hash *h = hash_alloc(20);	

	/* Construct Radix Tree */
    int i;
    for (i = 0; i < ARRAY_SIZE(indices); i++)
        hash_insert(h, indices[i], (void *)item[i]);


	struct hash_node *node = NULL;

	int j;
	for (i = 0; i < ARRAY_SIZE(indices); i++)
		for (j = 0; j <= i; j++)
			hash_get(h, indices[i], &node); 

	display(h);

   return;
}


void benchmark() {
	unsigned long size[] = {1 << 10, 1 << 20, 0};
	unsigned long step[] = {1, 2, 7, 15, 63, 64, 65,
				128, 256, 512, 12345, 0};
	int c, s;

	printv(1, "starting benchmarks\n");

	/*
	for (c = 0; size[c]; c++)
		for (s = 0; step[s]; s++)
			benchmark_size(size[c], step[s]);
	*/
	my_benchmark();

	printv(1, "benchmarks finished.\n");
}
