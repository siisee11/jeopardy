#include<stdio.h>
#include<stdlib.h>

#define HASH_SIZE 100
typedef struct lru_node{
	int key;
	struct lru_node* prev;
	struct lru_node* next;
}LRU_node;
typedef struct lru_list{
	struct lru_node* head;
	struct lru_node* tail;
}LRU_list;
typedef struct node{
	int key;
	int value;
	struct lru_node* LRUNode;
	struct node* next;
}hash_node;
typedef struct{
	hash_node* node;
	struct node* tail;
}hash_entry;

int get_hash_key(int input_key){
	return input_key%HASH_SIZE;
}
int main(){
	int input_key, hash_key, input_data,mode;
	hash_entry hash_table[HASH_SIZE];
	LRU_list LRUList;

	LRUList.head = NULL;
	LRUList.tail = NULL;
	for(int i=0;i<HASH_SIZE;i++){
		hash_table[i].node=NULL;
		hash_table[i].tail=NULL;
	}
	while(1){
		scanf("%d %d %d",&mode, &input_key,&input_data);
		hash_key = get_hash_key(input_key);
		if(mode==0){//insert
			//LRU
			LRU_node* tmpLRU = (LRU_node*)malloc(sizeof(LRU_node));
			tmpLRU->key = input_key;
			tmpLRU->prev = NULL;
			if(LRUList.head){
				LRUList.head->prev = tmpLRU;
				tmpLRU->next = LRUList.head;
			}else{
				LRUList.tail = tmpLRU;
			}
			LRUList.head = tmpLRU;
			
			LRU_node* b = LRUList.head;
			while(b){
				printf("<%d>\n",b->key);
				b=b->next;
			}

			//data
			hash_node* tmp = (hash_node*)malloc(sizeof(hash_node));
			tmp->key = input_key;
			tmp->value = input_data;
			tmp->next = NULL;
			tmp->LRUNode = tmpLRU;
			if(hash_table[hash_key].tail == NULL){
				hash_table[hash_key].node = tmp;
				hash_table[hash_key].tail = tmp;
			}else{
				hash_table[hash_key].tail->next = tmp;
				hash_table[hash_key].tail = tmp;
			}
			hash_node* a = hash_table[hash_key].node;
			while(a){
				printf("[%d %d]\n",a->key,a->value);
				a=a->next;
			}
		}else if(mode==1){//search
			//data
			hash_node* a = hash_table[hash_key].node;
			while(a){
				if(a->key == input_key) break;
				a=a->next;
			}
			if(a==NULL) printf("NULL\n");
			else printf("search : [%d %d]\n",a->key,a->value);

			//LRU_update
			if(a!=NULL){
				LRU_node* tmpLRU = a->LRUNode;
				if(tmpLRU != LRUList.head){
				if(tmpLRU != LRUList.tail){
					tmpLRU->prev->next = tmpLRU->next;
					tmpLRU->next->prev = tmpLRU->prev;
				}else{
					tmpLRU->prev->next = NULL;
					LRUList.tail = tmpLRU->prev;
				}
				tmpLRU->prev = NULL;
				tmpLRU->next = LRUList.head;
				tmpLRU->next->prev = tmpLRU;
				LRUList.head = tmpLRU;
				}
			}
			LRU_node* b = LRUList.head;
			while(b){
				if(b->prev) printf("<p%d>",b->prev->key);
				printf("<%d>",b->key);
				if(b->next) printf("<n%d>",b->next->key);
				printf("\n");
				b=b->next;
			}
		}else if(mode==2){//delete
			hash_node* a = hash_table[hash_key].node, *prev;
			while(a){
				if(a->key == input_key) break;
				prev=a;
				a=a->next;
			}
			LRU_node* tmpLRU = a->LRUNode;

			if(a==NULL) printf("NULL\n");
			else{
				//data
				prev->next = a->next;
				if(a->next ==NULL){
					hash_table[hash_key].tail = prev;
				}
				free(a);
			       	printf("search : [%d %d]\n",a->key,a->value);

				//LRUdata
			}
		}
	}
}
