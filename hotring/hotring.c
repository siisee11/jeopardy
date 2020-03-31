#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "hotring.h"
#include "error.h"

__thread int nr_request = 0;

struct head *
head_alloc(struct hash *h)
{
	struct head *ret = NULL;

	ret = (struct head *)calloc(1, sizeof(struct head));

	printv(3, "%s()::head pointer alloced\n",__func__);

	return ret;
}

struct hash_node *
hash_node_alloc(struct hash *h)
{
	struct hash_node *ret = NULL;

	ret = (struct hash_node *)calloc(1, sizeof(struct hash_node));

	if (ret) {
		ret->tag = 0;
		ret->key = 0;
		ret->value = NULL;
	}

	printv(3, "%s()::hash_node alloced\n",__func__);

	return ret;
}

struct hash*
hash_alloc(unsigned long size)
{
	struct hash *ret = NULL;
	struct hash_node *node = NULL;

	ret = (struct hash *)calloc(1, sizeof(struct hash)); 

	if (ret) {
		ret->size = size;
		ret->slots = (void *)calloc(size, sizeof(void *));
		for(int i=0; i<size; i++)
		{
			node = NULL;
		}
	}

	return ret;
}

struct hash* hash_extend(struct hash *h)
{
	return NULL;
}

static int hash_get_first(struct hash *h, unsigned long key, struct hash_node **nodep) 
{
	struct head *head_pointer;
	struct hash_node *tmp, *head_node;
	struct list_head *p;
	int hashIndex = hashCode(h, key);  

	if (h->slots[hashIndex] != NULL) {
		head_pointer = rcu_dereference_raw(h->slots[hashIndex]);
		head_node = rcu_dereference_raw(head_pointer->node);
		*nodep = head_node;
		return 1;
	}
	return 0;
}

int hash_get(struct hash *h, unsigned long key, struct hash_node **nodep) 
{
	struct head *head_pointer;
	struct hash_node *tmp, *head_node;
	struct list_head *p;
	int hashIndex = hashCode(h, key);  

	nr_request++;

	if (h->slots[hashIndex] != NULL) {
		head_pointer = rcu_dereference_raw(h->slots[hashIndex]);
		head_pointer.counter++;
		head_node = rcu_dereference_raw(head_pointer->node);
		if (head_node->key == key) {
			head_node.counter++;
			*nodep = head_node;
			goto keep_hot;
		} else {
			list_for_each(p, &head_node->list) {
				tmp = list_entry(p, struct hash_node, list);
				if (tmp->key == key) {
					tmp.counter++;
					*nodep = tmp;
					goto access_cold;
				}
			}
		}
	}
	return 0;

keep_hot:
	if (nr_request >= 5)
		nr_request = 0;

access_cold:
	if (nr_request >= 5) {
		hotspot_shift(h->slots[hashIndex], *nodep);
		nr_request = 0;
	}
	return 1;
}

void *hash_lookup(struct hash *h, unsigned long key) 
{
	struct hash_node *node;
	int hashIndex = hashCode(h, key);  

	if (h->slots[hashIndex] != NULL) {
		node = h->slots[hashIndex];
		return &node->value;
	}

	return NULL;
}

/* index자리에 노드가 있으면 반환, 없으면 생성 */
static int __hash_create(struct hash *h,
		unsigned long index, struct hash_node __rcu **nodep)
{
	struct head *head = rcu_dereference_raw(h->slots[index]);
	struct hash_node *head_node;
	struct hash_node *node = NULL;

	printv(3, "%s()::head%p\n", __func__, head);

	if (head)
	{
		head_node = rcu_dereference_raw(head->node);	
		node = hash_node_alloc(h);
		list_add_tail( &(node->list), &(head_node->list) );
	}
	else
	{
		head = head_alloc(h);
		rcu_assign_pointer(h->slots[index], head);
		node = hash_node_alloc(h);
		INIT_LIST_HEAD(&node->list);
		rcu_assign_pointer(head->node, node);
	}

	if (nodep)
		*nodep = node;

	return 0;
}

static inline int insert_entries(struct hash_node __rcu *node,
		unsigned long key, void *value, bool replace)
{
	if (node->value)
		return -EEXIST;
	node->key = key;
	rcu_assign_pointer(node->value, value);
	return 1;
}

int hash_insert(struct hash *h, unsigned long key, void *value) 
{
	struct hash_node __rcu *node;
	int error;

	unsigned long hashIndex = hashCode(h, key);
	printv(3, "%s()::key= %ld, hashIndex= %ld, value= %p\n", __func__, key, hashIndex, value);

	error = __hash_create(h, hashIndex, &node);
	if (error)
		return error;

	error = insert_entries(node, key, value, false);
	if (error < 0)
		return error;

	return 0;
}

void display(struct hash *h) {
	int i = 0;
	struct hash_node *node = NULL;
	struct hash_node *tmp;
	struct list_head *p;

	for(i = 0; i< h->size; i++) {
		if (hash_get_first(h, i, &node)) {
			printv(3, "(%d, %p) --> ", i, node->value);
			list_for_each(p, &node->list) {
				tmp = list_entry(p, struct hash_node, list);
				printv(3, "(%d, %p) --> ", i, tmp->value);
			}
			printv(3, "\n");
		}
		else
			printv(3, " ~~ \n");
	}

	printf("\n");
}

void hotspot_shift(struct head *head, struct hash_node *node) {
	/* Set Active bit */
	head.active = 1;
	printv(3, "%s()::head= %p, node= %p\n", __func__, head, node);
	return;
}

void head_pointer_movement()
{

	return;
}
