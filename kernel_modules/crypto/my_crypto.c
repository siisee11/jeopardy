#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include <linux/idr.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/init.h>
#include <linux/hash.h>
#include <crypto/algapi.h>

MODULE_LICENSE( "GPL" );

struct sdesc {
    struct shash_desc shash;
    char ctx[];
};

struct filter {
	unsigned int 	bitmap_size;
	unsigned long 	bitmap[0];
};

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
	int len;

    alg = crypto_alloc_shash(hash_alg_name, 0, 0);
    if (IS_ERR(alg)) {
            pr_info("can't alloc alg %s\n", hash_alg_name);
            return PTR_ERR(alg);
    }
	len = crypto_shash_digestsize(alg);
	printk("alg(%s)->len=%d\n", hash_alg_name, len);

    ret = calc_hash(alg, data, datalen, digest);
    crypto_free_shash(alg);
    return ret;
}

static int __init init_crypto( void )
{
	u8 digest[25];
	int i;

	struct filter *filter;
	unsigned long bitmap_size = BITS_TO_LONGS(65)
		* sizeof(unsigned long);
	printk("bitmap_size=%ld", bitmap_size);
	
	filter = kzalloc(sizeof(*filter) + bitmap_size, GFP_KERNEL);

	printk("bits: ");
	printk("%lx", filter->bitmap[0]);
	printk("%lx", filter->bitmap[1]);
	printk("\n");

	test_hash("hihi", 4, digest);

	printk("digest: ");
	for (i = 0; i < 5; ++i) {
		printk("%02x %02x %02x %02x %02x ", 
				digest[i], digest[i+1], digest[i+2], digest[i+3], digest[i+4]);
	}

#if 0
	for (i = 0; i < 20; i++) {
		*bits += digest[i];
		*bits %= 4;
	}
#endif

	filter->bitmap[0] += digest[0];
	filter->bitmap[0] |= digest[1] << 8;
	filter->bitmap[0] |= digest[2] << 16;
	filter->bitmap[0] |= digest[3] << 24;


	printk("bits: ");
	printk("%02lx", filter->bitmap[0]);
	printk("%02lx", filter->bitmap[1]);
	printk("\n");

	return 0;
}

static void __exit exit_crypto( void )
{
	return;
}


module_init(init_crypto);
module_exit(exit_crypto);
