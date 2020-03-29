#ifndef TEST_H
#define TEST_H

#include "radix-tree.h"

struct item {
	struct rcu_head	rcu_head;
	unsigned long index;
	unsigned int order;
};

struct item *item_create(unsigned long index, unsigned int order);
int item_insert(struct radix_tree_root *root, unsigned long index);
void item_sanity(struct item *item, unsigned long index);
void item_free(struct item *item, unsigned long index);
int item_delete(struct radix_tree_root *root, unsigned long index);
struct item *item_lookup(struct radix_tree_root *root, unsigned long index);

void item_check_present(struct radix_tree_root *root, unsigned long index);
void item_check_absent(struct radix_tree_root *root, unsigned long index);

void item_kill_tree(struct radix_tree_root *root);

void benchmark(void);

#endif
