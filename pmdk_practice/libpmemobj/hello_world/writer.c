#include <stdio.h>
#include <string.h>
#include <libpmemobj.h>
#include "layout.h"


int main(int argc, char *argv[])
{
	char my_buf[MAX_BUF_LEN];

	PMEMobjpool *pop = pmemobj_create(argv[1], POBJ_LAYOUT_NAME(string_store)
			, PMEMOBJ_MIN_POOL, 0666);
	if (pop == NULL) {
		perror("pmemobj_create");
		return 1;
	}

	TOID(struct my_root) root = POBJ_ROOT(pop, struct my_root);

	scanf("%9s", my_buf);

	TX_BEGIN(pop) {
		TX_MEMCPY(D_RW(root)->buf, my_buf, strlen(my_buf));
	} TX_END

	pmemobj_close(pop);
	return 0;
}
