# include <stdio.h>
# include "am.h"
# include "pf.h"

# define AM_MAXSTACK 50

struct
    {
     int pageNumber;
     int offset;
    } AM_Stack[AM_MAXSTACK];

int AM_topofStackPtr = -1;

<<<<<<< HEAD
AM_PushStack(pageNum,offset)
int pageNum;
int offset;

=======
void AM_PushStack(int pageNum, int offset)
>>>>>>> upstream/main
{
AM_topofStackPtr++;
AM_Stack[AM_topofStackPtr].pageNumber  = pageNum;
AM_Stack[AM_topofStackPtr].offset  = offset;
}

<<<<<<< HEAD
AM_PopStack()

=======
void AM_PopStack(void)
>>>>>>> upstream/main
{
AM_topofStackPtr--;
}

<<<<<<< HEAD
AM_topofStack(pageNum,offset)
int *pageNum;
int *offset;
=======
void AM_topofStack(int *pageNum, int *offset)
>>>>>>> upstream/main
{
*pageNum = AM_Stack[AM_topofStackPtr].pageNumber ;
*offset = AM_Stack[AM_topofStackPtr].offset ;
}

<<<<<<< HEAD
AM_EmptyStack()

=======
void AM_EmptyStack(void)
>>>>>>> upstream/main
{
AM_topofStackPtr = -1;
}

