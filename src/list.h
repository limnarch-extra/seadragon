#ifndef LIST_H_
#define LIST_H_

typedef struct {
    unsigned int capacity;
    unsigned int length;
    void **items;
} list_t;

list_t *list_create();
void list_free(list_t *list);
void list_add(list_t *list, void *item);
void list_del(list_t *list, unsigned int index);
void list_cat(list_t *list, list_t *source);
void *list_pop(list_t *list);

#endif
