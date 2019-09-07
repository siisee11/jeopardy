#include<stdio.h>
#include<stdlib.h>
#include<libpmemobj.h>
#include<unistd.h>
#include<string.h>
#include "layout.h"


static PMEMobjpool *pop;
static TOID(struct note) current_note;

int note_construct(PMEMobjpool *pop, void *ptr, void *arg) {
	struct note *n = (struct note *)ptr;
	char *msg = arg;

	pmemobj_memcpy_persist(pop, n->msg, msg, strlen(msg));

	time(&n->date_created);
	pmemobj_persist(pop, &n->date_created, sizeof(time_t));

	return 0;
}

void create_note(PMEMobjpool *pop, char *msg) {
	TOID(struct my_root) root = POBJ_ROOT(pop, struct my_root);

	printf("create notes.....\n");
	size_t nlen = strlen(msg);
	POBJ_LIST_INSERT_NEW_HEAD(pop, &D_RW(root)->head, notes,
			sizeof(struct note) + nlen, note_construct, msg);
}

void note_read_init(PMEMobjpool *pop) {
	TOID(struct my_root) root = POBJ_ROOT(pop, struct my_root);
	current_note = POBJ_LIST_FIRST(&D_RO(root)->head);
	printf("current note initialization.....\n");
}

void note_read_next() {
	current_note = POBJ_LIST_NEXT(current_note, notes);
}

void note_read_prev() {
	current_note = POBJ_LIST_PREV(current_note, notes);
}

void note_print(const TOID(struct note) note) {
	printf("Created at %s: \n %s\n",
			ctime(&D_RO(note)->date_created),
			D_RO(note)->msg);
}

void note_print_all(PMEMobjpool *pop) {
	TOID(struct my_root) root = POBJ_ROOT(pop, struct my_root);

	TOID(struct note) iter;
	POBJ_LIST_FOREACH(iter, &D_RO(root)->head, notes) {
		note_print(iter);
	}
}

void note_remove(PMEMobjpool *pop) {
	TOID(struct my_root) root = POBJ_ROOT(pop, struct my_root);

	POBJ_LIST_REMOVE_FREE(pop, &D_RO(root)->head,
			current_note, notes);
}

int main(int argc, char *argv[]){
	const char *path = argv[1];
	pop = NULL;

	if (access(path, F_OK) != 0) {
		if((pop = pmemobj_create(path, POBJ_LAYOUT_NAME(lists),
						PMEMOBJ_MIN_POOL, 0666)) == NULL) {
			printf("failed to create pool\n");
			return 1;
		}
	} else {
		if ((pop = pmemobj_open(path, POBJ_LAYOUT_NAME(pi))) == NULL) {
			printf("failed to open pool\n");
			return 1;
		}
	}

	create_note(pop, "note1");
	create_note(pop, "note2");
	note_print_all(pop);


	pmemobj_close(pop);
}
