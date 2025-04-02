#include "genlist.h"

genlist_t insertList(genlist_t head, void *elem)
{
	gennode_t *newnode;

	if ((newnode = malloc(sizeof(gennode_t))) == NULL)
	{
		return NULL;
	}
	newnode->next = head;
	newnode->elem = elem;
	head = newnode;
	return head;
}

void freeList(genlist_t head)
{
	genlist_t aux;
	while (head != NULL)
	{
		free(head->elem);
		aux = head;
		head = head->next;
		free(aux);
	}
}

void printList(genlist_t head, void (*funprint)(void *elem))
{
	genlist_t headaux;
	headaux = head;

	while (headaux != NULL)
	{
		funprint(headaux->elem);
		headaux = headaux->next;
	}
}
