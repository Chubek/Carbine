/*--------------------------------------------------------------------*/
/* stack.c                                                            */
/* Author: Bob Dondero                                                */
/* A generic Stack ADT implementation                                 */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stdlib.h>
#include "stack.h"

/*--------------------------------------------------------------------*/

/* Each item is stored in a StackNode.  StackNodes are linked to
   form a list.  */

struct StackNode
{
   const void *pvItem;
   /* The item. */

   struct StackNode *psNextNode;
   /* The address of the next StackNode. */
};

/*--------------------------------------------------------------------*/

/* A Stack is a structure that points to the first StackNode. */

struct Stack
{
   struct StackNode *psFirstNode;
   /* The address of the first StackNode. */
};

/*--------------------------------------------------------------------*/

Stack_T Stack_new(void)

/* Return a new Stack_T object, or NULL is insufficient memory is
   available. */

{
   Stack_T oStack;

   oStack = (Stack_T)malloc(sizeof(struct Stack));
   if (oStack == NULL)
      return NULL;

  oStack->psFirstNode = NULL;
  return oStack;
}

/*--------------------------------------------------------------------*/

void Stack_free(Stack_T oStack)

/* Free oStack. */

{
   struct StackNode *psCurrentNode;
   struct StackNode *psNextNode;

   assert(oStack != NULL);

   for (psCurrentNode = oStack->psFirstNode;
        psCurrentNode != NULL;
        psCurrentNode = psNextNode)
   {
      psNextNode = psCurrentNode->psNextNode;
      free(psCurrentNode);
   }

   free(oStack);
}

/*--------------------------------------------------------------------*/

int Stack_push(Stack_T oStack, const void *pvItem)

/* Push pvItem onto *psStack.  Return 1 (TRUE) if successful, or 0
   (FALSE) if insufficient memory is available. */

{
   struct StackNode *psNewNode;

   assert(oStack != NULL);

   psNewNode = (struct StackNode*)malloc(sizeof(struct StackNode));
   if (psNewNode == NULL)
      return 0;

   psNewNode->pvItem = pvItem;
   psNewNode->psNextNode = oStack->psFirstNode;
   oStack->psFirstNode = psNewNode;
   return 1;
}

/*--------------------------------------------------------------------*/

void *Stack_top(Stack_T oStack)

/* Return the top item of oStack. */

{
   assert(oStack != NULL);
   assert(oStack->psFirstNode != NULL);

   return (void*)oStack->psFirstNode->pvItem;
}

/*--------------------------------------------------------------------*/

void Stack_pop(Stack_T oStack)

/* Pop and discard the top item of oStack. */

{
   struct StackNode *psNextNode;

   assert(oStack != NULL);
   assert(oStack->psFirstNode != NULL);

   psNextNode = oStack->psFirstNode->psNextNode;
   free(oStack->psFirstNode);
   oStack->psFirstNode = psNextNode;
}

/*--------------------------------------------------------------------*/

int Stack_isEmpty(Stack_T oStack)

/* Return 1 (TRUE) if oStack is empty, or 0 (FALSE) otherwise. */

{
   assert(oStack != NULL);

   return oStack->psFirstNode == NULL;
}
