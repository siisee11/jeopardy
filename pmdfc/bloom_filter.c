/*
 * Copyright (C) 2013 Daniel Mack <daniel@zonque.org>
 *
 * Bloom filter implementation.
 * See https://en.wikipedia.org/wiki/Bloom_filter
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/device.h>
#include <linux/idr.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/init.h>
#include <linux/hash.h>
#include <crypto/algapi.h>

#include "bloom_filter.h"


struct bloom_filter *bloom_filter_new(int bit_size)
{
	struct bloom_filter *filter;
	unsigned long bitmap_size = BITS_TO_LONGS(bit_size)
				  * sizeof(unsigned long);

	filter = kzalloc(sizeof(*filter) + bitmap_size, GFP_KERNEL);
	if (!filter)
		return ERR_PTR(-ENOMEM);

	kref_init(&filter->kref);
	mutex_init(&filter->lock);
	filter->bitmap_size = bit_size;
	INIT_LIST_HEAD(&filter->alg_list);

	return filter;
}

struct bloom_filter *bloom_filter_ref(struct bloom_filter *filter)
{
	kref_get(&filter->kref);
	return filter;
}

static void bloom_crypto_alg_free(struct bloom_crypto_alg *alg)
{
	if (alg->hash_tfm_allocated)
		crypto_free_shash(alg->hash_tfm);
	list_del(&alg->entry);
	kfree(alg->data);
	kfree(alg);
}

static void __bloom_filter_free(struct kref *kref)
{
	struct bloom_crypto_alg *alg, *tmp;
	struct bloom_filter *filter =
		container_of(kref, struct bloom_filter, kref);

	mutex_lock(&filter->lock);
	list_for_each_entry_safe(alg, tmp, &filter->alg_list, entry)
		bloom_crypto_alg_free(alg);
	mutex_unlock(&filter->lock);

	kfree(filter);
}

void bloom_filter_unref(struct bloom_filter *filter)
{
	kref_put(&filter->kref, __bloom_filter_free);
}

int bloom_filter_add_crypto_shash(struct bloom_filter *filter,
				 struct crypto_shash *hash_tfm)
{
	struct bloom_crypto_alg *alg;
	int ret = 0;

	alg = kzalloc(sizeof(*alg), GFP_KERNEL);
	if (!alg) {
		ret = -ENOMEM;
		goto exit;
	}

	alg->len = crypto_shash_digestsize(hash_tfm);
	pr_info("alg->len=%d \n", alg->len);
	alg->data = kzalloc(alg->len, GFP_KERNEL);
	if (!alg->data) {
		ret = -ENOMEM;
		goto exit_free_alg;
	}

	mutex_lock(&filter->lock);
	list_add_tail(&alg->entry, &filter->alg_list);
	mutex_unlock(&filter->lock);

	return 0;

exit_free_alg:
	kfree(alg);

exit:
	return ret;
}

int bloom_filter_add_hash_alg(struct bloom_filter *filter,
			      const char *name)
{
	struct bloom_crypto_alg *alg;
	int ret = 0;

	alg = kzalloc(sizeof(*alg), GFP_KERNEL);
	if (!alg) {
		ret = -ENOMEM;
		goto exit;
	}

	alg->hash_tfm = crypto_alloc_shash(name, 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(alg->hash_tfm)) {
		ret = PTR_ERR(alg->hash_tfm);
		goto exit_free_alg;
	}

	alg->hash_tfm_allocated = true;
	alg->len = crypto_shash_digestsize(alg->hash_tfm);
	pr_info("alg(%s)->len=%d\n", name, alg->len);
	alg->data = kzalloc(alg->len, GFP_KERNEL);
	if (!alg->data) {
		ret = -ENOMEM;
		goto exit_free_hash;
	}

	mutex_lock(&filter->lock);
	list_add_tail(&alg->entry, &filter->alg_list);
	mutex_unlock(&filter->lock);

	return 0;

exit_free_hash:
	crypto_free_shash(alg->hash_tfm);

exit_free_alg:
	kfree(alg);

exit:
	return ret;
}

int __bit_for_crypto_alg(struct bloom_crypto_alg *alg,
			 const u8 *data,
			 unsigned int len,
			 unsigned int *bit)
{
	struct shash_desc desc;
	unsigned int i;
	int ret;

	desc.tfm = alg->hash_tfm;

	ret = crypto_shash_init(&desc);
	if (ret < 0)
		return ret;

	ret = crypto_shash_digest(&desc, data, len, alg->data);
	if (ret < 0)
		return ret;

	*bit = 0;

	for (i = 0; i < alg->len; i++) {
		*bit += alg->data[i];
		*bit %= len;
	}

	return 0;
}

int bloom_filter_add(struct bloom_filter *filter,
		     const u8 *data)
{
	struct bloom_crypto_alg *alg;
	int ret = 0;

	pr_info("data: %s\n", data);

	mutex_lock(&filter->lock);
	if (list_empty(&filter->alg_list)) {
		ret = -EINVAL;
		goto exit_unlock;
	}

	list_for_each_entry(alg, &filter->alg_list, entry) {
		unsigned int bit;

		ret = __bit_for_crypto_alg(alg, data, filter->bitmap_size, &bit);
		if (ret < 0)
			goto exit_unlock;
		pr_info("bit: %u\n", bit);

		set_bit(bit, filter->bitmap);
	}

exit_unlock:
	mutex_unlock(&filter->lock);

	return ret;
}

int bloom_filter_check(struct bloom_filter *filter,
		       const u8 *data, unsigned int size,
		       bool *result)
{
	struct bloom_crypto_alg *alg;
	int ret = 0;

	mutex_lock(&filter->lock);
	if (list_empty(&filter->alg_list)) {
		ret = -EINVAL;
		goto exit_unlock;
	}

	*result = true;

	list_for_each_entry(alg, &filter->alg_list, entry) {
		unsigned int bit;

		ret = __bit_for_crypto_alg(alg, data, filter->bitmap_size, &bit);
		if (ret < 0)
			goto exit_unlock;
		pr_info("bit: %u\n", bit);

		if (!test_bit(bit, filter->bitmap)) {
			*result = false;
			break;
		}
	}

exit_unlock:
	mutex_unlock(&filter->lock);

	return ret;
}

void bloom_filter_set(struct bloom_filter *filter,
		      const u8 *bit_data)
{
	memcpy(filter->bitmap, bit_data,
		BITS_TO_LONGS(filter->bitmap_size) * sizeof(unsigned long));
}

void bloom_filter_reset(struct bloom_filter *filter)
{
	mutex_lock(&filter->lock);
	bitmap_zero(filter->bitmap, filter->bitmap_size);
	mutex_unlock(&filter->lock);
}


/* ------------------------------------------------------- */
static struct sdesc *init_sdesc(struct crypto_shash *alg)
{
    struct sdesc *sdesc;
    int size;

    size = sizeof(struct shash_desc) + crypto_shash_descsize(alg);
    sdesc = kmalloc(size, GFP_KERNEL);
    if (!sdesc)
        return ERR_PTR(-ENOMEM);
    sdesc->shash.tfm = alg;
    return sdesc;
}

static int calc_hash(struct crypto_shash *alg,
             const unsigned char *data, unsigned int datalen,
             unsigned char *digest)
{
    struct sdesc *sdesc;
    int ret;

    sdesc = init_sdesc(alg);
    if (IS_ERR(sdesc)) {
        pr_info("can't alloc sdesc\n");
        return PTR_ERR(sdesc);
    }

	printk("data:%s\n", data);
    ret = crypto_shash_digest(&sdesc->shash, data, datalen, digest);
    kfree(sdesc);
    return ret;
}

static int test_hash(const unsigned char *data, unsigned int datalen,
             unsigned char *digest)
{
    struct crypto_shash *alg;
    const char *hash_alg_name = "sha1";
    int ret;

    alg = crypto_alloc_shash(hash_alg_name, 0, 0);
    if (IS_ERR(alg)) {
            pr_info("can't alloc alg %s\n", hash_alg_name);
            return PTR_ERR(alg);
    }
    ret = calc_hash(alg, data, datalen, digest);
	printk("digest:%s\n", digest);
    crypto_free_shash(alg);
    return ret;
}

int test_main( void )
{
	u8 digest[20];
	test_hash("hihi", 4, digest);
	int i = 0;

	printk("digest: ");
	for (i = 0; i < 20; ++i) {
		printk("%02x", digest[i]);
	}
	printk("\n");

	return 0;
}
