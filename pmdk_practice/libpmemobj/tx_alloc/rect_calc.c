#include<stdio.h>
#include<libpmemobj.h>
#include<string.h>
#include "layout.h"


int area_calc(const TOID(struct rectangle) rect) {
	return D_RO(rect)->a * D_RO(rect)->b;
}


int main(int argc, char *argv[]){

	PMEMobjpool *pop = pmemobj_create(argv[1], POBJ_LAYOUT_NAME(rect_calc)
				, PMEMOBJ_MIN_POOL, 0666);
	TOID(struct my_root) root = POBJ_ROOT(pop, struct my_root);

	TX_BEGIN(pop){
		TX_ADD(root);
		TOID(struct rectangle) rect = TX_NEW(struct rectangle);
		D_RW(rect)->a = 5;
		D_RW(rect)->b = 10;
		D_RW(root)->rect = rect;
	} TX_END

	int p = area_calc(D_RO(root)->rect);
	printf("area: %d\n", p);

	TX_BEGIN(pop) {
		TX_ADD(root);
		TX_FREE(D_RW(root)->rect);
		D_RW(root)->rect = TOID_NULL(struct rectangle);
	} TX_END
}
