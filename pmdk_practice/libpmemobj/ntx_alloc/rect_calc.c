#include<stdio.h>
#include<libpmemobj.h>
#include<string.h>
#include "layout.h"


int area_calc(const TOID(struct rectangle) rect) {
	return D_RO(rect)->a * D_RO(rect)->b;
}

int rect_construct(PMEMobjpool *pop, void *ptr, void *arg) {
	struct rectangle *rect = ptr;
	rect->a = 5;
	rect->b = 10;
	pmemobj_persist(pop, rect, sizeof *rect);

	return 0;
}

int main(int argc, char *argv[]){

	PMEMobjpool *pop = pmemobj_create(argv[1], POBJ_LAYOUT_NAME(rect_calc)
				, PMEMOBJ_MIN_POOL, 0666);
	TOID(struct my_root) root = POBJ_ROOT(pop, struct my_root);

	POBJ_NEW(pop, &D_RW(root)->rect, struct rectangle, rect_construct, NULL);


//	int p = area_calc(D_RO(root)->rect);
	int p = area_calc(POBJ_FIRST(pop, struct rectangle));
	printf("area: %d\n", p);

	POBJ_FREE(&D_RW(root)->rect);
}
