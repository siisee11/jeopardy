POBJ_LAYOUT_BEGIN(string_store);
POBJ_LAYOUT_ROOT(string_store, struct my_root);
POBJ_LAYOUT_END(string_store);

#define MAX_BUF_LEN 10 /* maximum length of our buffer */

struct my_root {
	size_t len; /* = strlen(buf) */
	char buf[MAX_BUF_LEN];
};
