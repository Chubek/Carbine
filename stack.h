/*--------------------------------------------------------------------*/
/* stack.h                                                            */
/* Author: Bob Dondero                                                */
/* A generic Stack ADT interface                                      */
/*--------------------------------------------------------------------*/

#ifndef STACK_INCLUDED
#define STACK_INCLUDED

typedef struct Stack *Stack_T;
/* A Stack_T object is a last-in-first-out collection of items. */

Stack_T Stack_new(void);
/* Return a new Stack_T object, or NULL is insufficient memory is
   available. */

void Stack_free(Stack_T oStack);
/* Free oStack. */

int Stack_push(Stack_T oStack, const void *pvItem);
/* Push pvItem onto oStack.  Return 1 (TRUE) if successful, or 0
   (FALSE) if insufficient memory is available. */

void *Stack_top(Stack_T oStack);
/* Return the top item of oStack. */

void Stack_pop(Stack_T oStack);
/* Pop and discard the top item of oStack. */

int Stack_isEmpty(Stack_T oStack);
/* Return 1 (TRUE) if oStack is empty, or 0 (FALSE) otherwise. */

#endif
