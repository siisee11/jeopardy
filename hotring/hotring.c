#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

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


/*
 * Get head node (first node in ring)
 */
static int __hash_get_first(struct hash *h, unsigned long key, struct hash_node **nodep) 
{
	struct head *head_pointer;
	struct hash_node *tmp, *head_node;
	struct list_head *p;

	volatile uintptr_t iptr;
	unsigned long *ptr;
	unsigned long hashTag;
	unsigned long hashIndex = hashCode(h, key, &hashTag);  

	if (h->slots[hashIndex] != NULL) {
		head_pointer = rcu_dereference_raw(h->slots[hashIndex]);

		iptr = head_pointer->addr;
		ptr = (unsigned long *)iptr;

		head_node = (struct hash_node *)rcu_dereference_raw(ptr);
		*nodep = head_node;
		return 1;
	}
	return 0;
}

/*
 * Get last node (prev node of first node)
 */
static int __hash_get_last(struct hash *h, unsigned long key, struct hash_node **nodep) 
{
	struct head *head_pointer;
	struct hash_node *tmp, *head_node;
	struct list_head *p;

	volatile uintptr_t iptr;
	unsigned long *ptr;
	unsigned long hashTag;
	unsigned long hashIndex = hashCode(h, key, &hashTag);  

	if (h->slots[hashIndex] != NULL) {
		head_pointer = rcu_dereference_raw(h->slots[hashIndex]);

		iptr = head_pointer->addr;
		ptr = (unsigned long *)iptr;

		head_node = (struct hash_node *)rcu_dereference_raw(ptr);

		list_for_each(p, &head_node->list) {
			tmp = list_entry(p, struct hash_node, list);
		}
		
		*nodep = tmp;
		
		return 1;
	}
	return 0;
}




static int __hash_get(struct hash *h, unsigned long key, struct hash_node **nodep, struct hash_node **prevp) 
{
	struct head *head_pointer;
	struct hash_node *tmp, *head_node, *prev;
	struct list_head *p;

	volatile uintptr_t iptr;
	unsigned long *ptr;
	unsigned long hashTag;
	unsigned long hashIndex = hashCode(h, key, &hashTag);  

	if (h->slots[hashIndex] != NULL) {
		head_pointer = rcu_dereference_raw(h->slots[hashIndex]);
		head_pointer->counter++;

		iptr = head_pointer->addr;
		ptr = (unsigned long *)iptr;

		head_node = (struct hash_node *)rcu_dereference_raw(ptr);
		*prevp = NULL;
		if (head_node->key == key) {
			head_node->list.counter++;
			*nodep = head_node;
			goto keep_hot;
		} else {
			prev = head_node;
			list_for_each(p, &head_node->list) {
				tmp = list_entry(p, struct hash_node, list);
				if (tmp->key == key) {
					tmp->list.counter++;
					*nodep = tmp;
					*prevp = prev;
					goto access_cold;
				}
				if (prev->key < key && tmp->key > key)
					goto not_found;
				if (key < tmp->key && tmp->key < prev->key)
					goto not_found;
				if (tmp->key < prev->key && prev->key < key)
					goto not_found;
				prev = tmp;
			}
		}
	}
not_found:
	*prevp = NULL;
	return 0;

keep_hot:
	if (nr_request >= 5)
		nr_request = 0;

access_cold:
	if (nr_request >= 5) {
		hotspot_shift(h->slots[hashIndex], head_node);
		display(h);
		nr_request = 0;
	}
	return 1;

}

int hash_get(struct hash *h, unsigned long key, struct hash_node **nodep, struct hash_node **prevp) 
{
	nr_request++;
	return __hash_get(h, key, nodep, prevp);
}

void *hash_lookup(struct hash *h, unsigned long key) 
{
	struct hash_node *node;
	unsigned long hashTag;
	unsigned long hashIndex = hashCode(h, key, &hashTag);  

	if (h->slots[hashIndex] != NULL) {
		node = h->slots[hashIndex];
		return &node->value;
	}

	return NULL;
}

/* index자리에 노드가 있으면 반환, 없으면 생성 */
static int __hash_create(struct hash *h,
		unsigned long index, unsigned long tag,
		struct hash_node __rcu **nodep)
{
	struct head *head_pointer = rcu_dereference_raw(h->slots[index]);
	struct hash_node *head_node, *prev, *next;
	struct hash_node *node = NULL;
	struct list_head *p;

	volatile uintptr_t iptr;
	unsigned long *ptr;
	bool added = false;

	printv(3, "%s()::head%p\n", __func__, head_pointer);


	if (head_pointer)
	{
		iptr = head_pointer->addr;
		ptr = (unsigned long *)iptr;
		head_node = (struct hash_node *)rcu_dereference_raw(ptr);	
		node = hash_node_alloc(h);
		/* tag and key comparison */
		if (tag < head_node->tag)
		{
			prev = head_node;
			/* add before head */	
			list_for_each(p, &head_node->list) {
				prev = list_entry(p, struct hash_node, list);
			}
			list_add( &(node->list), &(prev->list) );
		} 
		else 
		{
			prev = head_node;
			/* if next node's tag is bigger, create node before it. */
			list_for_each(p, &head_node->list) {
				next = list_entry(p, struct hash_node, list);
				if (tag <= next->tag) {
					list_add( &(node->list), &(prev->list) );
					added = true;
					break;
				}
				prev = next;
			}

			/* if largest tag, create node after it */
			if (!added)
				list_add( &(node->list), &(prev->list) ) ;
		}
	}
	else
	{
		head_pointer = head_alloc(h);
		rcu_assign_pointer(h->slots[index], head_pointer);
		node = hash_node_alloc(h);
		INIT_LIST_HEAD(&node->list);
		rcu_assign_pointer(head_pointer->node, node);
	}

	if (nodep)
		*nodep = node;

	return 0;
}

static inline int insert_entries(struct hash_node __rcu *node,
		unsigned long key, unsigned long tag, void *value, bool replace)
{
	if (node->value)
		return -EEXIST;
	node->key = key;
	node->tag = tag;
	rcu_assign_pointer(node->value, value);
	return 1;
}


int hash_insert(struct hash *h, unsigned long key, void *value) 
{
	struct hash_node __rcu *node;
	int error;
	unsigned long hashTag;

	unsigned long hashIndex = hashCode(h, key, &hashTag);
	printv(3, "%s()::key= %ld, hashIndex= %ld, hashTag= %ld, value= %p\n", __func__, key, hashIndex, hashTag, value);

	error = __hash_create(h, hashIndex, hashTag, &node);
	if (error)
		return error;

	error = insert_entries(node, key, hashTag, value, false);
	if (error < 0)
		return error;

	return 0;
}

void hotring_node_rcu_free(struct rcu_head *head)
{
	struct hash_node *node =
			container_of(head, struct hash_node, rcu_head);

	memset(&node->tag, 0, sizeof(node->tag));
	memset(&node->key, 0, sizeof(node->tag));
	memset(node->value, 0, sizeof(node->value));

	INIT_LIST_HEAD(&node->list);
	free(node);
}

static inline void
hotring_head_free(struct hash *h, unsigned long key)
{
	unsigned long tag = 0;
	unsigned long hashIndex = hashCode(h, key, &tag);  

	struct head *head_pointer = 
		rcu_dereference_raw(h->slots[hashIndex]);

	h->slots[hashIndex] = NULL;

	free(head_pointer);
}

static inline void
hotring_node_free(struct hash_node *node)
{
	free(node);
//	call_rcu(&node->rcu_head, hotring_node_rcu_free);
	printv(3, "%s()::key= %ld\n",__func__, node->key);
}

bool hotring_delete(struct hash *h, unsigned long key)
{
	bool deleted = false;

	struct hash_node *target, *prev;

	if (__hash_get(h, key, &target, &prev)) {
		if (prev != NULL)
			list_del( &(prev->list), &(target->list) );
		
		hotring_node_free(target);	

		if (prev == NULL)
		{
			hotring_head_free(h, key);
		}
		deleted = true;
	} 

	return deleted;
}

static struct hash *hotring_rehash_init(struct hash *h)
{
	struct hash *new;
	struct hash_node *node;
	int i, error;

	int hash_tag_bits = 64 - h->size;		// (48)
	unsigned long maxTag = ( (ULONG_MAX - 1) >> h->size );

	new = hash_alloc(2 * h->size);

	printv(3, "%s()::maxTag= %ld\n", __func__, maxTag);

	for ( i = 0 ; i < h->size ; i++){
		/* insert tag 0 node */
		node = hash_node_alloc(h);
		error = __hash_create(h, i, 0, &node);
		error = insert_entries(node, 0, 0, NULL, false);
		rcu_assign_pointer(new->slots[(i << 1) + 0], node);

		/* insert tag T/2 node */
		node = hash_node_alloc(h);
		error = __hash_create(h, i, maxTag/2, &node);
		error = insert_entries(node, 0, maxTag/2, NULL, false);
		rcu_assign_pointer(new->slots[(i << 1) + 1], node);
	}

	return new;
}


/* TODO: Make it correct */
struct hash *hotring_rehash(struct hash *h)
{
	struct hash *new;
	struct hash_node *ring1_head, *ring1_tail, *ring2_head, *ring2_tail;

	/* Initialization */
	new = hotring_rehash_init(h);

	/* Split */
	int i;
	int hash_tag_bits = 64 - h->size;
	unsigned long maxTag = (1 << hash_tag_bits) - 1;

	for (i = 0 ; i < h->size; i++){
		__hash_get_first(h, i, &ring1_head);
		__hash_get_last(h, i, &ring2_tail);
		__hash_get(h, i + (maxTag << h->size), &ring1_tail, &ring2_head);
	}

	/*
	rcu_assign_pointer(ring1_tail->list.next, ring1_head);
	rcu_assign_pointer(ring2_tail->list.next, ring2_head);
	*/
	ring1_tail->list.next = &ring1_head->list;
	ring2_tail->list.next = &ring2_head->list;

	/* Deletion */
	for (i = 0 ; i < new->size; i++){
		hotring_delete(new, i);
	}

	return new;
}

void display(struct hash *h) {
	int i = 0;
	struct hash_node *node = NULL;
	struct hash_node *tmp;
	struct list_head *p;

	for(i = 0; i< h->size; i++) {
		if (__hash_get_first(h, i, &node)) {
			printv(3, "(%d, %p) \t--> ", i, node->value);
			list_for_each(p, &node->list) {
				tmp = list_entry(p, struct hash_node, list);
				printv(3, "(%d, %p) \t--> ", i, tmp->value);
			}
			printv(3, "\n");
		}
		else
			printv(3, " ~~ \n");
	}
	printf("\n");
}
	

void hotspot_shift(struct head *head, struct hash_node *head_node) {
	/* Set Active bit */
	head->active = 1;

	double total_counter = head->counter;
	double counters[10] = {0,};

	double min_income = 1000000000;
	double income = 0;
	int nr_hottest = 0;
	struct hash_node *hottest = NULL;
	struct hash_node *tmp = NULL;
	struct list_head *p;

	int nr_node = 0;
	counters[nr_node] = head_node->list.counter;	
	head_node->list.counter = 0;
	list_for_each(p, &head_node->list) {
		/* get counters */
		nr_node++;
		tmp = list_entry(p, struct hash_node, list);
		counters[nr_node] = tmp->list.counter;	

		/* reset counters */
		tmp->list.counter = 0;
	}
	nr_node++;


	double head_income = 0;
	/* Find hottest item index*/
	int t, i;
	for (t = 0; t < nr_node ; t++){
		for (i = 0; i < nr_node ; i++) {
			income += (counters[i] / total_counter) * abs((i - t) % nr_node);
		}
		if (t == 0) {
			head_income = income;
		}
		if (income < min_income) {
			min_income = income;
			nr_hottest = t;
		}
		income = 0;
	}

	if (min_income == head_income)
		goto stay_hot;

	/* Find hottest hash_node */
	i = 1;
	list_for_each(p, &head_node->list) {
		if (i == nr_hottest) {
			hottest = list_entry(p, struct hash_node, list);
			break;
		}
		i++;
	}

	/* assign hottest node to head pointer */
	rcu_assign_pointer(head->node, hottest);

stay_hot:
	/* reset all counters and flags */
	head->counter = 0;
	head->active = 0;

	printv(3, "%s()::head= %p, node= %p\n", __func__, head, head_node);
	return;
}

void head_pointer_movement()
{

	return;
}
