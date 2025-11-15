#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hf.h"

#define MAX_LINE 4096

int main() {
    PF_Init();  // PF must be initialised
    PF_SetBufferSize(20);        // or smaller if you want to stress paging
    PF_SetReplacementPolicy(PF_REPL_LRU);

    const char *dataFile = "../../data/student.txt";
    const char *heapFile = "student.hf";

    // (Re)create HF file
    PF_DestroyFile((char*)heapFile);  // ignore error if not exists
    if (HF_CreateFile((char*)heapFile) != HFE_OK) {
        PF_PrintError("HF_CreateFile");
        return 1;
    }

    int fd = HF_OpenFile((char*)heapFile);
    if (fd < 0) {
        PF_PrintError("HF_OpenFile");
        return 1;
    }

    FILE *fp = fopen(dataFile, "r");
    if (!fp) {
        perror("fopen student.txt");
        return 1;
    }

    char line[MAX_LINE];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        // strip trailing newline
        size_t len = strlen(line);
        if (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';

        RID rid;
        int err = HF_InsertRec(fd, line, (int)len, &rid);
        if (err != HFE_OK) {
            printf("HF_InsertRec error %d at record %d\n", err, count);
            return 1;
        }
        count++;
    }
    fclose(fp);

    printf("Inserted %d student records into heap file %s\n", count, heapFile);

    // sanity check: sequential scan
    HF_Scan scan;
    HF_OpenFileScan(fd, &scan);

    int scanned = 0;
    RID rid;
    char *rec;
    int recLen;

    while (1) {
        int err = HF_GetNextRec(fd, &scan, &rid, &rec, &recLen);
        if (err == HFE_EOF) break;
        if (err != HFE_OK) {
            printf("HF_GetNextRec error %d\n", err);
            return 1;
        }

        // print first 3 records only for sanity
        if (scanned < 3) {
            printf("RID(page=%d, slot=%d): %.*s\n",
                   rid.pageNum, rid.slotNum, recLen, rec);
        }
        scanned++;
    }
    HF_CloseFileScan(&scan);

    printf("Scanned %d records from heap file\n", scanned);

    HF_CloseFile(fd);
    return 0;
}
