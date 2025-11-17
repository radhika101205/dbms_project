typedef struct am_leafheader
	{
		char pageType;
		int nextLeafPage;
		short recIdPtr;
		short keyPtr;
		short freeListPtr;
		short numinfreeList;
		short attrLength;
		short numKeys;
		short maxKeys;
	}  AM_LEAFHEADER; /* Header for a leaf page */

typedef struct am_intheader 
	{
		char pageType;
		short numKeys;
		short maxKeys;
		short attrLength;
	}	AM_INTHEADER ; /* Header for an internal node */

/* -------- Internal AM function prototypes (shared across modules) ------- */
/* ---- AM function prototypes used across modules ---- */

/* -------- Internal AM function prototypes (shared across modules) ------- */
/* ---- AM function prototypes used across modules ---- */

/* index-management API */
int  AM_CreateIndex(char *fileName, int indexNo, char attrType, int attrLength);
int  AM_DestroyIndex(char *fileName, int indexNo);
int  AM_InsertEntry(int fileDesc, char attrType, int attrLength,
                    char *value, int recId);
int  AM_DeleteEntry(int fileDesc, char attrType, int attrLength,
                    char *value, int recId);
void AM_PrintError(char *s);
/* -------- Scan API (implemented in amscan.c) ------- */
int AM_OpenIndexScan(int fileDesc, char attrType, int attrLength,
                     int op, char *value);
					 
/* Debug / printing helpers (amprint.c) */
void AM_PrintIntNode(char *pageBuf, char attrType);
void AM_PrintLeafNode(char *pageBuf, char attrType);
void AM_DumpLeafPages(int fileDesc, int min, char attrType, int attrLength);
void AM_PrintLeafKeys(char *pageBuf, char attrType);
void AM_PrintAttr(char *bufPtr, char attrType, int attrLength);
void AM_PrintTree(int fileDesc, int pageNum, char attrType);
int AM_OpenIndex(char *fileName, int indexNo);
int AM_CloseIndex(int fileDesc);

int AM_FindNextEntry(int scanDesc);

int AM_CloseIndexScan(int scanDesc);


/* Search (root → leaf) */
int  AM_Search(int fileDesc, char attrType, int attrLength,
               char *value, int *pageNum, char **pageBuf, int *indexPtr);

/* Stack helpers used during descent/splits */
void AM_PushStack(int pageNumber, int offset);
void AM_topofStack(int *pageNumber, int *offset);
void AM_PopStack(void);
void AM_EmptyStack(void);

/* Leaf / B+-tree manipulation helpers */

/* Insert a record into a leaf */
int AM_InsertintoLeaf(char *pageBuf, int attrLength, char *value,
                      int recId, int index, int status);

/* Internal helpers used by AM_InsertintoLeaf */
void AM_InsertToLeafFound(char *pageBuf, int recId, int index,
                          AM_LEAFHEADER *header);
void AM_InsertToLeafNotFound(char *pageBuf, char *value, int recId,
                             int index, AM_LEAFHEADER *header);

/* Compact keys/records from [low..high] from pageBuf -> tempPage */
void AM_Compact(int low, int high, char *pageBuf, char *tempPage,
                AM_LEAFHEADER *header);


/* Split + parent update helpers (defined in am.c) */
int  AM_SplitLeaf(int fileDesc, char *pageBuf, int *pageNum,
                  int attrLength, int recId, char *value,
                  int status, int index, char *key);

int  AM_AddtoParent(int fileDesc, int pageNum,
                    char *value, int attrLength);

/* Internal node manipulation */
void AM_AddtoIntPage(char *pageBuf, char *value, int pageNum,
                     AM_INTHEADER *header, int offset);

void AM_FillRootPage(char *pageBuf, int pageNum1, int pageNum2,
                     char *value, short attrLength, short maxKeys);

void AM_SplitIntNode(char *pageBuf, char *pbuf1, char *pbuf2,
                     AM_INTHEADER *header, char *value,
                     int pageNum, int offset);

/* Helpers implemented in amsearch.c */
int  AM_BinSearch(char *pageBuf, char attrType, int attrLength,
                  char *value, int *indexPtr, AM_INTHEADER *header);

int  AM_SearchLeaf(char *pageBuf, char attrType, int attrLength,
                   char *value, int *indexPtr, AM_LEAFHEADER *header);

/* Comparison helper (likely in misc.c) */
int  AM_Compare(char *keyPtr, char attrType, int attrLength, char *value);



extern int AM_RootPageNum; /* The page number of the root */
extern int AM_LeftPageNum; /* The page Number of the leftmost leaf */
extern int AM_Errno; /* last error in AM layer */
#include <stdlib.h>
// extern char *calloc();
// extern char *malloc();

# define AM_Check if (errVal != PFE_OK) {AM_Errno = AME_PF; return(AME_PF) ;}
# define AM_si sizeof(int)
# define AM_ss sizeof(short)
# define AM_sl sizeof(AM_LEAFHEADER)
# define AM_sint sizeof(AM_INTHEADER)
# define AM_sc sizeof(char)
# define AM_sf sizeof(float)
# define AM_NOT_FOUND 0 /* Key is not in tree */
# define AM_FOUND 1 /* Key is in tree */
# define AM_NULL 0 /* Null pointer for lists in a page */
# define AM_MAX_FNAME_LENGTH 80
# define AM_NULL_PAGE -1 
# define FREE 0 /* Free is chosen to be zero because C initialises all 
	   variablesto zero and we require that our scan table be initialised */
# define FIRST 1 
# define BUSY 2
# define LAST 3
# define OVER 4
# define ALL 0
# define EQUAL 1
# define LESS_THAN 2
# define GREATER_THAN 3
# define LESS_THAN_EQUAL 4
# define GREATER_THAN_EQUAL 5
# define NOT_EQUAL 6
# define MAXSCANS 20
# define AM_MAXATTRLENGTH 256


# define AME_OK 0
# define AME_INVALIDATTRLENGTH -1
# define AME_NOTFOUND -2
# define AME_PF -3
# define AME_INTERROR -4
# define AME_INVALID_SCANDESC -5
# define AME_INVALID_OP_TO_SCAN -6
# define AME_EOF -7
# define AME_SCAN_TAB_FULL -8
# define AME_INVALIDATTRTYPE -9
# define AME_FD -10
# define AME_INVALIDVALUE -11
