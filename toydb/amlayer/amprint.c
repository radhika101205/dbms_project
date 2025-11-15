# include <stdio.h>
#include <string.h>
# include "am.h"
# include "pf.h"

void AM_PrintIntNode(char *pageBuf, char attrType)
{
int tempPageint;
int i;
int recSize;
AM_INTHEADER *header;


header = (AM_INTHEADER *) calloc(1,AM_sint);
bcopy(pageBuf,header,AM_sint);
recSize = header->attrLength + AM_si;
printf("PAGETYPE %c\n",header->pageType);
printf("NUMKEYS %d\n",header->numKeys);
printf("MAXKEYS %d\n",header->maxKeys);
printf("ATTRLENGTH %d\n",header->attrLength);
bcopy(pageBuf + AM_sint,&tempPageint,AM_si);
printf("FIRSTPAGE is %d\n",tempPageint);
for(i = 1 ; i <= (header->numKeys);i++)
  {
   AM_PrintAttr(pageBuf + (i-1)*recSize + AM_sint + AM_si,attrType,
                 header->attrLength);
   bcopy(pageBuf + i*recSize + AM_sint,&tempPageint,AM_si);
   printf("NEXTPAGE is %d\n",tempPageint);
  }
}


void AM_PrintLeafNode(char *pageBuf, char attrType)
{
short nextRec;
int i;
int recSize;
int recId;
int offset1;
AM_LEAFHEADER *header;

header = (AM_LEAFHEADER *) calloc(1,AM_sl);
bcopy(pageBuf,header,AM_sl);
recSize = header->attrLength + AM_ss;
printf("PAGETYPE %c\n",header->pageType);
printf("NEXTLEAFPAGE %d\n",header->nextLeafPage);

/*printf("RECIDPTR %d\n",header->recIdPtr);
printf("KEYPTR %d\n",header->keyPtr);
printf("FREELISTPTR %d\n",header->freeListPtr);
printf("NUMINFREELIST %d\n",header->numinfreeList);
printf("ATTRLENGTH %d\n",header->attrLength);
printf("MAXKEYS %d\n",header->maxKeys);*/

printf("NUMKEYS %d\n",header->numKeys);
for (i = 1; i <= header->numKeys; i++)
  {
  offset1 = (i - 1) * recSize + AM_sl;
  AM_PrintAttr(pageBuf + AM_sl + (i-1)*recSize,attrType,header->attrLength);
  bcopy(pageBuf + offset1 + header->attrLength,(char *)&nextRec,AM_ss);
  while (nextRec != 0)
    {
    bcopy(pageBuf + nextRec,(char *)&recId,AM_si);
    printf("RECID is %d\n",recId);
    bcopy(pageBuf + nextRec + AM_si,(char *)&nextRec,AM_ss);
    }
  printf("\n");
  printf("\n");
  }
}

void AM_DumpLeafPages(int fileDesc, int min,
                      char attrType, int attrLength)
{
    int errVal;
    int pageNum;
    char *pageBuf;

    pageNum = min;

    while (pageNum != AM_NULL_PAGE) {
        errVal = PF_GetThisPage(fileDesc, pageNum, &pageBuf);
        if (errVal != PFE_OK) {
            PF_PrintError("PF_GetThisPage in AM_DumpLeafPages");
            return;
        }

        AM_PrintLeafKeys(pageBuf, attrType);

        /* assume leaf header at start */
        AM_LEAFHEADER head, *header;
        header = &head;
        bcopy(pageBuf, header, AM_sl);
        pageNum = header->nextLeafPage;

        PF_UnfixPage(fileDesc, pageNum, FALSE);
    }
}



void AM_PrintLeafKeys(char *pageBuf, char attrType)
{
    AM_LEAFHEADER head, *header;
    int recSize;
    int i;

    header = &head;
    bcopy(pageBuf, header, AM_sl);
    recSize = header->attrLength + AM_si;

    for (i = 1; i <= header->numKeys; i++) {
        AM_PrintAttr(pageBuf + AM_sl + (i-1)*recSize,
                     attrType, header->attrLength);
        printf(" ");
    }
    printf("\n");
}


void AM_PrintAttr(char *bufPtr, char attrType, int attrLength)
{
    int i;
    int ival;
    float fval;
    char cval;

    switch (attrType) {
    case 'i':
        bcopy(bufPtr, (char *)&ival, sizeof(int));
        printf("%d", ival);
        break;
    case 'f':
        bcopy(bufPtr, (char *)&fval, sizeof(float));
        printf("%f", fval);
        break;
    case 'c':
        for (i = 0; i < attrLength; i++) {
            bcopy(bufPtr + i, &cval, 1);
            putchar(cval);
        }
        break;
    default:
        printf("<?>");
    }
}



void AM_PrintTree(int fileDesc, int pageNum, char attrType)
{
    int errVal;
    char *pageBuf;
    AM_INTHEADER ihead, *iheader;
    AM_LEAFHEADER lhead, *lheader;

    errVal = PF_GetThisPage(fileDesc, pageNum, &pageBuf);
    if (errVal != PFE_OK) {
        PF_PrintError("PF_GetThisPage in AM_PrintTree");
        return;
    }

    /* Determine node type from pageType */
    if (pageBuf[0] == 'i') {  /* internal node */
        iheader = &ihead;
        bcopy(pageBuf, iheader, AM_sint);
        AM_PrintIntNode(pageBuf, attrType);

        /* recurse over children */
        int recSize = iheader->attrLength + AM_si;
        int i;
        int childPage;

        /* leftmost child pointer */
        bcopy(pageBuf + AM_sint, (char *)&childPage, AM_si);
        AM_PrintTree(fileDesc, childPage, attrType);

        for (i = 1; i <= iheader->numKeys; i++) {
            bcopy(pageBuf + AM_sint + i*recSize, (char *)&childPage, AM_si);
            AM_PrintTree(fileDesc, childPage, attrType);
        }
    } else { /* leaf node */
        AM_PrintLeafNode(pageBuf, attrType);
    }

    PF_UnfixPage(fileDesc, pageNum, FALSE);
}
