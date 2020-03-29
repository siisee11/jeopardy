#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <urcu.h>

#include "error.h"
#include "radix-tree.h"

#define container_of(ptr, type, member) ({	\
		const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
		(type *)( (char *)__mptr - offsetof(type,member) );})

static inline struct radix_tree_node *entry_to_node(void *ptr)
{
	return (void *)((unsigned long)ptr & ~RADIX_TREE_INTERNAL_NODE);
}

static inline void *node_to_entry(void *ptr)
{
	return (void *)((unsigned long)ptr | RADIX_TREE_INTERNAL_NODE);
}

static inline unsigned long
get_slot_offset(const struct radix_tree_node *parent, void __rcu **slot)
{
	return parent ? slot - parent->slots : 0;
}

static unsigned int radix_tree_descend(const struct radix_tree_node *parent,
			struct radix_tree_node **nodep, unsigned long index)
{
	unsigned int offset = (index >> parent->shift) & RADIX_TREE_MAP_MASK;
	void __rcu **entry = rcu_dereference_raw(parent->slots[offset]);

	*nodep = (void *)entry;
	return offset;
}

#if 0
/* Tag operations */
static inline void tag_set(struct radix_tree_node *node, unsigned int tag,
		int offset)
{
	__set_bit(offset, node->tags[tag]);
}

static inline void tag_clear(struct radix_tree_node *node, unsigned int tag,
		int offset)
{
	__clear_bit(offset, node->tags[tag]);
}

static inline int tag_get(const struct radix_tree_node *node, unsigned int tag,
		int offset)
{
	return test_bit(offset, node->tags[tag]);
}

static inline void root_tag_set(struct radix_tree_root *root, unsigned tag)
{
	root->xa_flags |= (__force gfp_t)(1 << (tag + ROOT_TAG_SHIFT));
}

static inline void root_tag_clear(struct radix_tree_root *root, unsigned tag)
{
	root->xa_flags &= (__force gfp_t)~(1 << (tag + ROOT_TAG_SHIFT));
}

static inline void root_tag_clear_all(struct radix_tree_root *root)
{
	root->xa_flags &= (__force gfp_t)((1 << ROOT_TAG_SHIFT) - 1);
}

static inline int root_tag_get(const struct radix_tree_root *root, unsigned tag)
{
	return (__force int)root->xa_flags & (1 << (tag + ROOT_TAG_SHIFT));
}

static inline unsigned root_tags_get(const struct radix_tree_root *root)
{
	return (__force unsigned)root->xa_flags >> ROOT_TAG_SHIFT;
}
#endif

/*
 * The maximum index which can be stored in a radix tree
 */
static inline unsigned long shift_maxindex(unsigned int shift)
{
	return (RADIX_TREE_MAP_SIZE << shift) - 1;
}

static inline unsigned long node_maxindex(const struct radix_tree_node *node)
{
	return shift_maxindex(node->shift);
}

static unsigned long next_index(unsigned long index,
				const struct radix_tree_node *node,
				unsigned long offset)
{
	return (index & ~node_maxindex(node)) + (offset << node->shift);
}



static struct radix_tree_node *
radix_tree_node_alloc(struct radix_tree_node *parent,
			struct radix_tree_root *root,
			unsigned int shift, unsigned int offset,
			unsigned int count, unsigned int nr_values)
{
	struct radix_tree_node *ret = NULL;

	ret = (struct radix_tree_node *)calloc(1, sizeof(struct radix_tree_node)); 

	printv(3, "%s()::shift= %d, offset= %d, count= %d, nr_values= %d\n",
			__func__, shift, offset, count, nr_values);

	if (ret) {
		ret->shift = shift;
		ret->offset = offset;
		ret->count = count;
		ret->nr_values = nr_values;
		ret->parent = parent;
	}
	return ret;
}

struct radix_tree_root *
radix_tree_root_alloc()
{
	struct radix_tree_root *ret = NULL;
	struct radix_tree_node *node = NULL;

	ret = (struct radix_tree_root *)calloc(1, sizeof(struct radix_tree_root)); 
	node = radix_tree_node_alloc(NULL, ret, 0, 0, 0, 0);

	if (ret) {
		ret->height = 1;
		ret->rnode = node_to_entry(node);
	}

	return ret;
}

void radix_tree_node_rcu_free(struct rcu_head *head)
{
	struct radix_tree_node *node =
			container_of(head, struct radix_tree_node, rcu_head);

	/*
	 * Must only free zeroed nodes into the slab.  We can be left with
	 * non-NULL entries by radix_tree_free_nodes, so clear the entries
	 * and tags here.
	 */
	memset(node->slots, 0, sizeof(node->slots));
	memset(node->tags, 0, sizeof(node->tags));
//	INIT_LIST_HEAD(&node->private_list);

	free(node);
}

static inline void
radix_tree_node_free(struct radix_tree_node *node)
{
	call_rcu(&node->rcu_head, radix_tree_node_rcu_free);
	printv(3, "%s()::shift= %d, offset= %d, count= %d, nr_values= %d\n",
			__func__, node->shift, node->offset, node->count, node->nr_values);
}

static unsigned radix_tree_load_root(const struct radix_tree_root *root,
		struct radix_tree_node **nodep, unsigned long *maxindex)
{
	struct radix_tree_node *node = rcu_dereference_raw(root->rnode);

	*nodep = node;

	if (radix_tree_is_internal_node(node)) {
		node = entry_to_node(node);
		*maxindex = node_maxindex(node);
		return node->shift + RADIX_TREE_MAP_SHIFT;
	}

	*maxindex = 0;
	return 0;
}


0x000001/000000/000000/

--> node(12,0)
000001 --> 1 --> 
--> node(6, 1)
0x000001/000000/
& 0x111111

000000

/*
 *	Extend a radix tree so it can store key @index.
 */
static int radix_tree_extend(struct radix_tree_root *root,
				unsigned long index, unsigned int shift)
{
	void *entry;
	unsigned int maxshift;
	int tag;

	/* Figure out what the shift should be.  */
	maxshift = shift;
	while (index > shift_maxindex(maxshift))
		maxshift += RADIX_TREE_MAP_SHIFT;

	printv(3, "%s()::shift= %d, maxshift= %d\n", __func__, shift, maxshift);

	entry = rcu_dereference_raw(root->rnode);
	if (!entry)
		goto out;

	do {
		struct radix_tree_node *node = radix_tree_node_alloc(NULL,
							root, shift, 0, 1, 0);
		if (!node)
			return -ENOMEM;

		entry_to_node(entry)->parent = node;
		/*
		 * entry was already in the radix tree, so we do not need
		 * rcu_assign_pointer here
		 */
		node->slots[0] = (void __rcu *)entry;
		entry = node_to_entry(node);
		rcu_assign_pointer(root->rnode, entry);
		shift += RADIX_TREE_MAP_SHIFT;
	} while (shift <= maxshift);
out:
	return maxshift + RADIX_TREE_MAP_SHIFT;
}

/**
 *	radix_tree_shrink    -    shrink radix tree to minimum height
 *	@root		radix tree root
 */
static inline bool radix_tree_shrink(struct radix_tree_root *root)
{
	bool shrunk = false;

	for (;;) {
		struct radix_tree_node *node = rcu_dereference_raw(root->rnode);
		struct radix_tree_node *child;

		if (!radix_tree_is_internal_node(node))
			break;
		node = entry_to_node(node);

		printv(3, "%s()::node<shift= %d, offset= %d, count= %d>\n", __func__,
				node->shift, node->offset, node->count);

		/*
		 * The candidate node has more than one child, or its child
		 * is not at the leftmost slot, we cannot shrink.
		 */
		if (node->count != 1)
			break;
		child = rcu_dereference_raw(node->slots[0]);
		if (!child)
			break;

		/*
		 * For the last node(shift == 0 and root->rnode)
		 */
		if (!node->shift)
			break;

		if (radix_tree_is_internal_node(child))
			entry_to_node(child)->parent = NULL;

		root->rnode = (void __rcu *)child;

		node->count = 0;
		radix_tree_node_free(node);
		shrunk = true;
	}
	printv(3, "%s()::shrink %s \n", __func__, shrunk? "done":"not done");

	return shrunk;
}

/* Delete node if all slots in node are NULL */
static bool delete_node(struct radix_tree_root *root,
			struct radix_tree_node *node)
{
	bool deleted = false;

	do {
		struct radix_tree_node *parent;

		printv(4, "%s()::node<shift= %d, offset= %d, count= %d>\n", __func__,
				node->shift, node->offset, node->count);

		if (node->count) {
			if (node_to_entry(node) ==
					rcu_dereference_raw(root->rnode))
				deleted |= radix_tree_shrink(root);
			return deleted;
		}

		parent = node->parent;
		if (parent) {
			parent->slots[node->offset] = NULL;
			parent->count--;
		} else {
			root->rnode= NULL;
		}

		radix_tree_node_free(node);
		deleted = true;

		node = parent;
	} while (node);

	return deleted;
}

/**
 *	__radix_tree_create	-	create a slot in a radix tree
 *	@root:		radix tree root
 *	@index:		index key
 *	@nodep:		returns node
 *	@slotp:		returns slot
 *
 *	Create, if necessary, and return the node and slot for an item
 *	at position @index in the radix tree @root.
 *
 *	Until there is more than one item in the tree, no nodes are
 *	allocated and @root->xa_head is used as a direct slot instead of
 *	pointing to a node, in which case *@nodep will be NULL.
 *
 *	Returns -ENOMEM, or 0 for success.
 */
static int __radix_tree_create(struct radix_tree_root *root,
		unsigned long index, struct radix_tree_node **nodep,
		void __rcu ***slotp)
{
	struct radix_tree_node *node = NULL, *child;
	void __rcu **slot = (void __rcu **)&root->rnode;
	unsigned long maxindex;
	unsigned int shift, offset = 0;
	unsigned long max = index;

	shift = radix_tree_load_root(root, &child, &maxindex);

	printv(3, "%s()::maxindex= %ld, shift= %d\n", __func__, maxindex, shift);

	/* Make sure the tree is high enough.  */
	if (max > maxindex) {
		/* return value of radix_tree_extend is new shift */
		int error = radix_tree_extend(root, max, shift);
		if (error < 0)
			return error;
		shift = error;
		child = rcu_dereference_raw(root->rnode);
		printv(3, "%s()::radix_tree_extend()--> shift= %d\n", __func__, shift);
	}

	while (shift > 0) {
		shift -= RADIX_TREE_MAP_SHIFT;
		if (child == NULL) {
			/* Have to add a child node.  */
			child = radix_tree_node_alloc(node, root, shift,
							offset, 0, 0);
			if (!child)
				return -ENOMEM;
			rcu_assign_pointer(*slot, node_to_entry(child));
			if (node)
				node->count++;
		}

		/* Go a level down */
		node = entry_to_node(child);
		offset = radix_tree_descend(node, &child, index);
		slot = &node->slots[offset];
	}

	if (nodep)
		*nodep = node;
	if (slotp)
		*slotp = slot;
	return 0;
}


static inline int insert_entries(struct radix_tree_node *node,
		void __rcu **slot, void *item, bool replace)
{
	if (*slot)
		return -EEXIST;
	rcu_assign_pointer(*slot, item);
	if (node) {
		node->count++;
		if (xa_is_value(item))
			node->nr_values++;
	}
	printv(3, "%s()::*slot= %p\n", __func__, *slot);
	return 1;
}

/**
 *	__radix_tree_insert    -    insert into a radix tree
 *	@root:		radix tree root
 *	@index:		index key
 *	@item:		item to insert
 *
 *	Insert an item into the radix tree at position @index.
 */
int radix_tree_insert(struct radix_tree_root *root, unsigned long index,
			void *item)
{
	struct radix_tree_node *node;
	void __rcu **slot;
	int error;

	printv(3, "%s()::index= %ld item= %p\n", __func__, index, item);
	error = __radix_tree_create(root, index, &node, &slot);
	if (error)
		return error;

	error = insert_entries(node, slot, item, false);
	if (error < 0)
		return error;

	if (node) {
		unsigned offset = get_slot_offset(node,slot);
	}

	return 0;
}


/**
 *	__radix_tree_lookup	-	lookup an item in a radix tree
 *	@root:		radix tree root
 *	@index:		index key
 *	@nodep:		returns node
 *	@slotp:		returns slot
 *
 *	Lookup and return the item at position @index in the radix
 *	tree @root.
 *
 *	Until there is more than one item in the tree, no nodes are
 *	allocated and @root->xa_head is used as a direct slot instead of
 *	pointing to a node, in which case *@nodep will be NULL.
 */
void *__radix_tree_lookup(const struct radix_tree_root *root,
			  unsigned long index, struct radix_tree_node **nodep,
			  void __rcu ***slotp)
{
	struct radix_tree_node *node, *parent;
	unsigned long maxindex;
	void __rcu **slot;

// restart:
	parent = NULL;
	slot = (void __rcu **)&root->rnode;
	radix_tree_load_root(root, &node, &maxindex);
	if (index > maxindex)
		return NULL;

	while (radix_tree_is_internal_node(node)) {
		unsigned offset;

		parent = entry_to_node(node);
		offset = radix_tree_descend(parent, &node, index);
		slot = parent->slots + offset;
//		if (node == RADIX_TREE_RETRY)
//			goto restart;
		if (parent->shift == 0)
			break;
	}

	if (nodep)
		*nodep = parent;
	if (slotp)
		*slotp = slot;
	return node;
}

/**
 *	radix_tree_lookup_slot    -    lookup a slot in a radix tree
 *	@root:		radix tree root
 *	@index:		index key
 *
 *	Returns:  the slot corresponding to the position @index in the
 *	radix tree @root. This is useful for update-if-exists operations.
 *
 *	This function can be called under rcu_read_lock iff the slot is not
 *	modified by radix_tree_replace_slot, otherwise it must be called
 *	exclusive from other writers. Any dereference of the slot must be done
 *	using radix_tree_deref_slot.
 */
void __rcu **radix_tree_lookup_slot(const struct radix_tree_root *root,
				unsigned long index)
{
	void __rcu **slot;

	if (!__radix_tree_lookup(root, index, NULL, &slot))
		return NULL;
	return slot;
}

void *radix_tree_lookup(const struct radix_tree_root *root, unsigned long index)
{
	return __radix_tree_lookup(root, index, NULL, NULL);
}

static void replace_slot(void **slot, void *item,
		struct radix_tree_node *node, int count, int values)
{
	printv(3, "%s()::old= %p, new= %p \n", __func__, *slot, item);
	if (node && (count || values)) {
		node->count += count;
		node->nr_values += values;
	}

	rcu_assign_pointer(*slot, item);
}

/*
 * IDR users want to be able to store NULL in the tree, so if the slot isn't
 * free, don't adjust the count, even if it's transitioning between NULL and
 * non-NULL.  For the IDA, we mark slots as being IDR_FREE while they still
 * have empty bits, but it only stores NULL in slots when they're being
 * deleted.
 */
static int calculate_count(struct radix_tree_root *root,
				struct radix_tree_node *node, void __rcu **slot,
				void *item, void *old)
{
	// TODO: tag
#if 0
	if (is_idr(root)) {
		unsigned offset = get_slot_offset(node, slot);
		bool free = node_tag_get(root, node, IDR_FREE, offset);
		if (!free)
			return 0;
		if (!old)
			return 1;
	}
#endif
	return !!item - !!old;
}

/**
 * __radix_tree_replace		- replace item in a slot
 * @root:		radix tree root
 * @node:		pointer to tree node
 * @slot:		pointer to slot in @node
 * @item:		new item to store in the slot.
 *
 * For use with __radix_tree_lookup().  Caller must hold tree write locked
 * across slot lookup and replacement.
 */
void __radix_tree_replace(struct radix_tree_root *root,
			  struct radix_tree_node *node,
			  void __rcu **slot, void *item)
{
	void *old = rcu_dereference_raw(*slot);
	int values = !!xa_is_value(item) - !!xa_is_value(old);
	int count = calculate_count(root, node, slot, item, old);

	/*
	 * This function supports replacing value entries and
	 * deleting entries, but that needs accounting against the
	 * node unless the slot is root->xa_head.
	 */
	// TODO: WARN_ON_ONCE
//	WARN_ON_ONCE(!node && (slot != (void __rcu **)&root->rnode) &&
//			(count || values));
	replace_slot(slot, item, node, count, values);

	if (!node)
		return;

	delete_node(root, node);
}

void radix_tree_replace_slot(struct radix_tree_root *root,
			     void __rcu **slot, void *item)
{
	__radix_tree_replace(root, NULL, slot, item);
}

#if 0

/**
 *	radix_tree_gang_lookup - perform multiple lookup on a radix tree
 *	@root:		radix tree root
 *	@results:	where the results of the lookup are placed
 *	@first_index:	start the lookup from this key
 *	@max_items:	place up to this many items at *results
 *
 *	Performs an index-ascending scan of the tree for present items.  Places
 *	them at *@results and returns the number of items which were placed at
 *	*@results.
 *
 *	The implementation is naive.
 *
 *	Like radix_tree_lookup, radix_tree_gang_lookup may be called under
 *	rcu_read_lock. In this case, rather than the returned results being
 *	an atomic snapshot of the tree at a single point in time, the
 *	semantics of an RCU protected gang lookup are as though multiple
 *	radix_tree_lookups have been issued in individual locks, and results
 *	stored in 'results'.
 */
unsigned int
radix_tree_gang_lookup(const struct radix_tree_root *root, void **results,
			unsigned long first_index, unsigned int max_items)
{
	struct radix_tree_iter iter;
	void **slot;
	unsigned int ret = 0;

	if (unlikely(!max_items))
		return 0;

	radix_tree_for_each_slot(slot, root, &iter, first_index) {
		results[ret] = *slot;
		if (!results[ret])
			continue;
		if (radix_tree_is_internal_node(results[ret])) {
			slot = radix_tree_iter_retry(&iter);
			continue;
		}
		if (++ret == max_items)
			break;
	}

	return ret;
}
#endif


void __rcu **radix_tree_iter_resume(void __rcu **slot,
					struct radix_tree_iter *iter)
{
	slot++;
	iter->index = __radix_tree_iter_add(iter, 1);
	iter->next_index = iter->index;
	iter->tags = 0;
	return NULL;
}

/**
 * radix_tree_next_chunk - find next chunk of slots for iteration
 *
 * @root:	radix tree root
 * @iter:	iterator state
 * @flags:	RADIX_TREE_ITER_* flags and tag index
 * Returns:	pointer to chunk first slot, or NULL if iteration is over
 */
void __rcu **radix_tree_next_chunk(const struct radix_tree_root *root,
			     struct radix_tree_iter *iter, unsigned flags)
{
	unsigned tag = flags & RADIX_TREE_ITER_TAG_MASK;
	struct radix_tree_node *node, *child;
	unsigned long index, offset, maxindex;

	// TODO tag
	if ((flags & RADIX_TREE_ITER_TAGGED))
		return NULL;

	/*
	 * Catch next_index overflow after ~0UL. iter->index never overflows
	 * during iterating; it can be zero only at the beginning.
	 * And we cannot overflow iter->next_index in a single step,
	 * because RADIX_TREE_MAP_SHIFT < BITS_PER_LONG.
	 *
	 * This condition also used by radix_tree_next_slot() to stop
	 * contiguous iterating, and forbid switching to the next chunk.
	 */
	index = iter->next_index;
	if (!index && iter->index)
		return NULL;

 restart:
	radix_tree_load_root(root, &child, &maxindex);
	if (index > maxindex)
		return NULL;
	if (!child)
		return NULL;

	if (!radix_tree_is_internal_node(child)) {
		/* Single-slot tree */
		iter->index = index;
		iter->next_index = maxindex + 1;
		iter->tags = 1;
		iter->node = NULL;
		return (void __rcu **)&root->rnode;
	}

	do {
		node = entry_to_node(child);
		offset = radix_tree_descend(node, &child, index);

		// TODO: tag
		if (!child) {
			/* Hole detected */
			if (flags & RADIX_TREE_ITER_CONTIG)
				return NULL;

			while (++offset	< RADIX_TREE_MAP_SIZE) {
				void *slot = rcu_dereference_raw(
						node->slots[offset]);
				if (slot)
					break;
			}
			index &= ~node_maxindex(node);
			index += offset << node->shift;
			/* Overflow after ~0UL */
			if (!index)
				return NULL;
			if (offset == RADIX_TREE_MAP_SIZE)
				goto restart;
			child = rcu_dereference_raw(node->slots[offset]);
		}

		if (!child)
			goto restart;
		// TODO: RADIX_TREE_RETRY
	} while (node->shift && radix_tree_is_internal_node(child));

	/* Update the iterator state */
	iter->index = (index &~ node_maxindex(node)) | offset;
	iter->next_index = (index | node_maxindex(node)) + 1;
	iter->node = node;

	/* TODO: tag 
	if (flags & RADIX_TREE_ITER_TAGGED)
		set_iter_tags(iter, node, offset, tag);
	*/
	return node->slots + offset;
}

static bool __radix_tree_delete(struct radix_tree_root *root,
				struct radix_tree_node *node, void **slot)
{
	void *old = rcu_dereference_raw(*slot);
	printv(3, "%s()::old= %p \n", __func__, old);
	int values = xa_is_value(old) ? -1 : 0;
#if 0
	unsigned offset = get_slot_offset(node, slot);
	int tag;

	for (tag = 0; tag < RADIX_TREE_MAX_TAGS; tag++)
		node_tag_clear(root, node, tag, offset);
#endif

	replace_slot(slot, NULL, node, -1, values);
	return node && delete_node(root, node);
}


/**
 * radix_tree_iter_delete - delete the entry at this iterator position
 * @root: radix tree root
 * @iter: iterator state
 * @slot: pointer to slot
 *
 * Delete the entry at the position currently pointed to by the iterator.
 * This may result in the current node being freed; if it is, the iterator
 * is advanced so that it will not reference the freed memory.  This
 * function may be called without any locking if there are no other threads
 * which can access this tree.
 */
void radix_tree_iter_delete(struct radix_tree_root *root,
				struct radix_tree_iter *iter, void __rcu **slot)
{
	if (__radix_tree_delete(root, iter->node, slot))
		iter->index = iter->next_index;
}

/**
 * radix_tree_delete_item - delete an item from a radix tree
 * @root: radix tree root
 * @index: index key
 * @item: expected item
 *
 * Remove @item at @index from the radix tree rooted at @root.
 *
 * Return: the deleted entry, or %NULL if it was not present
 * or the entry at the given @index was not @item.
 */
void *radix_tree_delete_item(struct radix_tree_root *root,
			     unsigned long index, void *item)
{
	struct radix_tree_node *node = NULL;
	void __rcu **slot = NULL;
	void *entry;

	entry = __radix_tree_lookup(root, index, &node, &slot);
	if (!slot)
		return NULL;
#if 0
	if (!entry && (!is_idr(root) || node_tag_get(root, node, IDR_FREE,
						get_slot_offset(node, slot))))
		return NULL;
#endif

	if (item && entry != item)
		return NULL;

	__radix_tree_delete(root, node, slot);

	return entry;
}

/**
 * radix_tree_delete - delete an entry from a radix tree
 * @root: radix tree root
 * @index: index key
 *
 * Remove the entry at @index from the radix tree rooted at @root.
 *
 * Return: The deleted entry, or %NULL if it was not present.
 */
void *radix_tree_delete(struct radix_tree_root *root, unsigned long index)
{
	return radix_tree_delete_item(root, index, NULL);
}


