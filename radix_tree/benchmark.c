#include <time.h>
#include "test.h"
#include "error.h"

#define NSEC_PER_SEC	1000000000L

static long long benchmark_iter(struct radix_tree_root *root, bool tagged)
{
	volatile unsigned long sink = 0;
	struct radix_tree_iter iter;
	struct timespec start, finish;
	long long nsec;
	int l, loops = 1;
	void **slot;

#ifdef BENCHMARK
again:
#endif
	clock_gettime(CLOCK_MONOTONIC, &start);
	for (l = 0; l < loops; l++) {
		radix_tree_for_each_slot(slot, root, &iter, 0)
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


static void benchmark_insert(struct radix_tree_root *root,
			     unsigned long size, unsigned long step)
{
	struct timespec start, finish;
	unsigned long index;
	long long nsec;

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (index = 0 ; index < size ; index += step)
		item_insert(root, index);

	clock_gettime(CLOCK_MONOTONIC, &finish);

	nsec = (finish.tv_sec - start.tv_sec) * NSEC_PER_SEC +
	       (finish.tv_nsec - start.tv_nsec);

	printv(1, "Size: %8ld, step: %8ld, insertion: %15lld ns\n",
		size, step, nsec);
}


static void benchmark_delete(struct radix_tree_root *root,
			     unsigned long size, unsigned long step)
{
	struct timespec start, finish;
	unsigned long index;
	long long nsec;

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (index = 0 ; index < size ; index += step)
		item_delete(root, index);

	clock_gettime(CLOCK_MONOTONIC, &finish);

	nsec = (finish.tv_sec - start.tv_sec) * NSEC_PER_SEC +
	       (finish.tv_nsec - start.tv_nsec);

	printv(1, "Size: %8ld, step: %8ld, deletion: %16lld ns\n",
		size, step, nsec);
}


static void benchmark_size(unsigned long size, unsigned long step)
{
	struct radix_tree_root *tree = radix_tree_root_alloc();
	long long normal, tagged;

	benchmark_insert(tree, size, step);
//	benchmark_tagging(&tree, size, step);

//	tagged = benchmark_iter(&tree, true);
	normal = benchmark_iter(tree, false);

//	printv(1, "Size: %8ld, step: %8ld, tagged iteration: %8lld ns\n",
//		size, step, tagged);
	printv(1, "Size: %8ld, step: %8ld, normal iteration: %8lld ns\n",
		size, step, normal);

	benchmark_delete(tree, size, step);

	item_kill_tree(tree);
	rcu_barrier();
}

static void my_benchmark()
{
    unsigned long indices[3] = {2, 6, 65};
	unsigned long item[3] = {2, 6, 65};

    struct radix_tree_root *root = radix_tree_root_alloc();

    /* Construct Radix Tree */
    int i;
    for (i = 0; i < ARRAY_SIZE(indices); i++)
        radix_tree_insert(root, indices[i], (void *)item[i]);

	/* Search item and return slot address*/
	void **slot;
	slot = radix_tree_lookup_slot(root, 2);
	printv(1, "[idx: 1] => %p\n", *slot);
	slot = radix_tree_lookup_slot(root, 65);
	printv(1, "[idx: 65] => %p\n", *slot);

	/* Delete item by index */
	if (radix_tree_delete(root,6)) {
		printv(1, "DELETE index= 6 succeed\n");
		slot = radix_tree_lookup_slot(root, 6);
		if (slot) {
			printv(1, "[idx: 6] => %p\n", *slot);
		} else {
			printv(1, "[idx: 6] => NULL\n");
		}
	} else {
		printv(1, "DELETE index= 6 fail\n");
	}

	/* Delete item by index */
	if (radix_tree_delete(root,65)) {
		printv(1, "DELETE index= 65 succeed\n");
		slot = radix_tree_lookup_slot(root, 65);
		if (slot) {
			printv(1, "[idx: 65] => %p\n", *slot);
		} else {
			printv(1, "[idx: 65] => NULL\n");
		}
	} else {
		printv(1, "DELETE index= 65 fail\n");
	}
}


void benchmark(void){

//	unsigned long size[] = {2048, 0};
	unsigned long size[] = {1 << 10, 1 << 20, 0};
//	unsigned long step[] = {64, 0};
	unsigned long step[] = {1, 2, 7, 15, 63, 64, 65,
				128, 256, 512, 12345, 0};
	int c, s;

	printv(1, "starting benchmarks\n");
	printv(1, "RADIX_TREE_MAP_SHIFT = %d\n", RADIX_TREE_MAP_SHIFT);

	for (c = 0; size[c]; c++)
		for (s = 0; step[s]; s++)
			benchmark_size(size[c], step[s]);
//	my_benchmark();

	return;
}
