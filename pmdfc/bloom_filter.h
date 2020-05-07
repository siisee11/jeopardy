#ifndef _BLOOM_H_
#define _BLOOM_H_

#include <crypto/algapi.h>

struct sdesc {
    struct shash_desc shash;
    char ctx[];
};

struct bloom_filter {
	struct kref		kref;
	struct mutex		lock;
	struct list_head	alg_list;
	unsigned int		bitmap_size;
	unsigned long		bitmap[0];
};

struct bloom_crypto_alg {
//	u8					*data;
	u8					data[32];
	unsigned int		len;
	int					hash_tfm_allocated:1;
	struct crypto_shash	*hash_tfm;
	struct list_head	entry;
};

struct bloom_filter *bloom_filter_new(int bit_size);
struct bloom_filter *bloom_filter_ref(struct bloom_filter *filter);
void bloom_filter_unref(struct bloom_filter *filter);

int bloom_filter_add_crypto_shash(struct bloom_filter *filter,
				 struct crypto_shash *hash_tfm);
int bloom_filter_add_hash_alg(struct bloom_filter *filter,
			      const char *name);

int bloom_filter_add(struct bloom_filter *filter,
		     const u8 *data, unsigned int);
int bloom_filter_check(struct bloom_filter *filter,
		       const u8 *data, unsigned int size,
		       bool *result);
void bloom_filter_set(struct bloom_filter *filter,
		      const u8 *bit_data);
void bloom_filter_reset(struct bloom_filter *filter);
int test_main(void);

#endif /* _BLOOM_H_ */
