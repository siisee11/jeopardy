#ifndef _LIST_H 
#define _LIST_H 

struct list_head { 
    struct list_head *next, *prev; 
}; 

#define LIST_HEAD_INIT(name) { &(name), &(name) } 

#define LIST_HEAD(name) \
struct list_head name = LIST_HEAD_INIT(name) 

#define INIT_LIST_HEAD(ptr) do { \
    (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0) 

void __list_add(struct list_head *new, 
                              struct list_head *prev, 
                              struct list_head *next);

void list_add(struct list_head *new, struct list_head *head);
void list_add_tail(struct list_head *new, struct list_head *head);
void __list_del(struct list_head *prev, struct list_head *next);
void list_del(struct list_head *entry);
void list_del_init(struct list_head *entry);
void list_move(struct list_head *list, struct list_head *head);
void list_move_tail(struct list_head *list, 
                                  struct list_head *head);
int list_empty(struct list_head *head);
void __list_splice(struct list_head *list, 
                                 struct list_head *head);
void list_splice(struct list_head *list, struct list_head *head);
void list_splice_init(struct list_head *list, 
                                    struct list_head *head);

#define list_entry(ptr, type, member) \
((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member))) 

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); \
pos = pos->next) 

#define list_for_each_prev(pos, head) \
    for (pos = (head)->prev; pos != (head); \
pos = pos->prev) 

#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
pos = n, n = pos->next) 

#define list_for_each_entry(pos, head, member) \
    for (pos = list_entry((head)->next, typeof(*pos), member); \
    &pos->member != (head); \
    pos = list_entry(pos->member.next, typeof(*pos), member) \
) 

#endif 
