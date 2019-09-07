#include <stdio.h>
#include <string.h>
#include <libpmemobj.h>
#include "layout.h"


int main(int argc, char *argv[])
{
	PMEMobjpool *pop = pmemobj_open(argv[1], POBJ_LAYOUT_NAME(string_store));
	if (pop == NULL){
		perror("pmemobj_open");
		return 1;
	}

	TOID(struct my_root) root = POBJ_ROOT(pop, struct my_root);

	printf("%s\n", D_RO(root)->buf);

	pmemobj_close(pop);

	return 0;
}
