/*
 * X-TIER_list.h
 *
 *  Created on: 25.11.2011
 *      Author: Sebastian Vogl <vogls@sec.in.tum.de>
 */

#ifndef XTIER_LIST_H_
#define XTIER_LIST_H_

/*
 * For each element in a list do something.
 *
 * @param list A pointer to a struct of type tracing_list.
 * @param iterator A element of type struct tracing_list_element *
 *                 that will be set to each of the elements in the list
 *                 'list'.
 */
#define XTIER_list_for_each(list, iterator) \
	for(iterator = list->first; iterator != NULL; iterator = iterator->next)

/*
 * Get the first element in a list whose member 'member' has
 * the value 'value'.
 *
 * @param list_ptr A pointer to a struct tracing_list structure
 *                 that contains all elements.
 * @param type The type of the elements that contain a struct
 * 	           tracing_list_element structure inside them.
 * @param member_list The name of the member in the structure
 *                    that is of type tracing_list_element.
 * @param member The name of the member that is supposed to
 *               have the value 'value'.
 * @param value The value that we are looking for. Member must
 *              have this value.
 * @param campare_func A function that is able to compare member
 *                     to the value and determine if member has
 *                     the value. For example, in the case of a
 *                     char *, a valid function would be 'strcmp'.
 *                     Notice that this function is only needed if
 *                     the value has a type that cannot be compared
 *                     with the '==' operator. In any other case this
 *                     param has to be set to NULL.
 *
 * @returns The first element that fulfills the condition
 *          member == value or NULL.
 */
#define XTIER_list_get_element(list_ptr, type, member_list,\
			 member, value, compare_func)                  \
	({							                           \
		type *result = NULL;					           \
		struct XTIER_list_element *e = NULL;               \
		int (*func)(const typeof(value),                   \
			    const typeof(value)) = compare_func;       \
		if(!list_ptr || !list_ptr->first)		           \
			result = NULL;				                   \
		else						                       \
		{						                           \
			XTIER_list_for_each(list_ptr, e)               \
			{			                                   \
				result = container_of(e, type,             \
							member_list);                  \
				if(func)         	                       \
				{                                          \
					if(func(result->member,                \
						(value)) == 0)                     \
						break;                             \
				}				                           \
				else if(result->member == (value))         \
					break;		                           \
								                           \
				result = NULL;			                   \
			}					                           \
		}						                           \
		result;						                       \
	})

#define mod(n, m)                                          \
	({                                                     \
		int result = (n) % (m);                            \
		if(result < 0)                                     \
			result += (m);                                 \
		                                                   \
		result;                                            \
	})

struct XTIER_list_element
{
	struct XTIER_list_element *next;
	struct XTIER_list_element *prev;
};

struct XTIER_list
{
	struct XTIER_list_element *first;
	struct XTIER_list_element *last;
	int length;
};

void XTIER_list_add(struct XTIER_list *list, struct XTIER_list_element *element);
void XTIER_list_remove(struct XTIER_list *list, struct XTIER_list_element *element);
struct XTIER_list * XTIER_list_create(void);
void XTIER_list_delete(struct XTIER_list *list);

#endif /* XTIER_LIST_H_ */
