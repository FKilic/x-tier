/*
 * X-TIER_list.c
 *
 *  Created on: 25.11.2011
 *      Author: Sebastian Vogl <vogls@sec.in.tum.de>
 */

#include "X-TIER_list.h"

struct XTIER_list * XTIER_list_create(void)
{
	struct XTIER_list *result = kzalloc(sizeof(struct XTIER_list), GFP_KERNEL);

	if(!result)
	{
		// Error
		return NULL;
	}

	result->first = NULL;
	result->last = NULL;
	result->length = 0;

	return result;
}

void XTIER_list_delete(struct XTIER_list *list)
{
	struct XTIER_list_element *e = list->first;
	struct XTIER_list_element *next = NULL;

	while(e)
	{
		next = e->next;
		kfree(e);
		e = next;
	}

	kfree(list);
}

void XTIER_list_add(struct XTIER_list *list, struct XTIER_list_element *element)
{
	if(!list->first)
	{
		list->first = element;
		element->prev = NULL;
	}
	else
	{
		element->prev = list->last;
		list->last->next = element;
	}

	element->next = NULL;
	list->last = element;

	list->length++;
}

void XTIER_list_remove(struct XTIER_list *list, struct XTIER_list_element *element)
{
	if(list->first == element)
		list->first = element->next;

	if(list->last == element)
		list->last = element->prev;

	if(element->next)
		element->next->prev = element->prev;

	if(element->prev)
		element->prev->next = element->next;

	list->length--;
}



