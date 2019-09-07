POBJ_LAYOUT_BEGIN(lists);
	POBJ_LAYOUT_ROOT(lists, struct my_root);
	POBJ_LAYOUT_TOID(lists, struct note);
POBJ_LAYOUT_END(lists);

struct note {
	time_t date_created;
	POBJ_LIST_ENTRY(struct note) notes;
	char msg[];
};

struct my_root {
	POBJ_LIST_HEAD(plist, struct note) head;
};

