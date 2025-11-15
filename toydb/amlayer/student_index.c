#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "../pflayer/pf.h"   /* PF_Init, PF_OpenFile, PF_CloseFile, PF_PrintError */
#include "am.h"              /* AM_CreateIndex, AM_DestroyIndex, AM_InsertEntry, scans, etc. */

#define MAX_LINE_LEN  2048
#define MAX_STUDENTS  20000   /* > 17815 so we are safe */

/* Simple struct: key = roll-no, recId = logical record id */
typedef struct {
    int roll;
    int recId;
} RecKey;

/* For method "sorted build": qsort comparator on roll-no */
static int cmpRecKey(const void *a, const void *b) {
    const RecKey *ra = (const RecKey *)a;
    const RecKey *rb = (const RecKey *)b;
    if (ra->roll < rb->roll) return -1;
    if (ra->roll > rb->roll) return 1;
    return 0;
}

/* Parse roll-no from the *second* field (semicolon-separated) of a line */
static int parse_rollno(const char *line) {
    char buf[MAX_LINE_LEN];
    strncpy(buf, line, MAX_LINE_LEN - 1);
    buf[MAX_LINE_LEN - 1] = '\0';

    char *token = strtok(buf, ";");   /* field 1 */
    token = strtok(NULL, ";");        /* field 2 = roll-no */

    if (!token) {
        fprintf(stderr, "parse_rollno: malformed line: %s\n", line);
        return -1;
    }
    return atoi(token);
}

/* Small helper to compute milliseconds from timeval */
static double elapsed_ms(struct timeval t1, struct timeval t2) {
    long sec  = (long)(t2.tv_sec  - t1.tv_sec);
    long usec = (long)(t2.tv_usec - t1.tv_usec);
    return (double)sec * 1000.0 + (double)usec / 1000.0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr,
            "Usage: %s <student_txt_path> <mode>\n"
            "  mode = 1  -> build index inserting in FILE ORDER (unsorted)\n"
            "  mode = 2  -> build index inserting in SORTED ORDER (bulk-load style)\n"
            "\nExample:\n"
            "  %s ../data/student.txt 1\n"
            "  %s ../data/student.txt 2\n",
            argv[0], argv[0], argv[0]);
        return 1;
    }

    const char *student_txt = argv[1];
    int mode = atoi(argv[2]);
    if (mode != 1 && mode != 2) {
        fprintf(stderr, "Invalid mode %d (use 1 or 2)\n", mode);
        return 1;
    }

    /* -------- 1. Read student.txt and collect (roll, recId) -------- */
    FILE *fp = fopen(student_txt, "r");
    if (!fp) {
        perror("fopen student_txt");
        return 1;
    }

    RecKey *arr = (RecKey *)malloc(MAX_STUDENTS * sizeof(RecKey));
    if (!arr) {
        fprintf(stderr, "Out of memory\n");
        fclose(fp);
        return 1;
    }

    char *line = NULL;
    size_t bufsize = 0;
    ssize_t len;
    int n = 0;

    while ((len = getline(&line, &bufsize, fp)) != -1) {
        if (n >= MAX_STUDENTS) {
            fprintf(stderr, "Too many records (increase MAX_STUDENTS)\n");
            free(arr);
            free(line);
            fclose(fp);
            return 1;
        }

        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }

        int roll = parse_rollno(line);
        if (roll < 0) {
            fprintf(stderr, "Skipping malformed line %d\n", n);
            continue;
        }

        arr[n].roll  = roll;
        arr[n].recId = n;    /* logical record id = line number */
        n++;
    }

    free(line);
    fclose(fp);

    printf("Loaded %d student records from %s\n", n, student_txt);

    /* -------- 2. Sort if mode == 2 (bulk-load style) ---------------- */
    if (mode == 2) {
        printf("Sorting by roll-no for bulk-load style build...\n");
        qsort(arr, n, sizeof(RecKey), cmpRecKey);
    } else {
        printf("Using file order (unsorted) for index build...\n");
    }

    /* -------- 3. Initialize PF + AM and create index ---------------- */
    PF_Init();
    /* If you have AM_Init() in your codebase, call it here:
       AM_Init(); */

    /* Remove any old index; ignore error code */
    AM_DestroyIndex("student.idx", 1);

    int err = AM_CreateIndex("student.idx", 1, 'i', sizeof(int));
    if (err != AME_OK) {
        AM_PrintError("AM_CreateIndex");
        free(arr);
        return 1;
    }

    /* IMPORTANT: This must match AM_CreateIndex’s file naming.
       Typical convention inside AM_CreateIndex is:
         sprintf(actualName, "%s.%d", fileName, indexNo);
       so we open "student.idx.1" here.
       If your amfns.c uses a different pattern, change this accordingly. */
    char indexFileName[256];
    sprintf(indexFileName, "%s.%d", "student.idx", 1);

    int fd = PF_OpenFile(indexFileName);
    if (fd < 0) {
        PF_PrintError("PF_OpenFile (index)");
        free(arr);
        return 1;
    }

    /* -------- 4. Insert all keys into the index --------------------- */
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

    for (int i = 0; i < n; i++) {
        int roll  = arr[i].roll;
        int recId = arr[i].recId;

        err = AM_InsertEntry(fd, 'i', sizeof(int), (char *)&roll, recId);
        if (err != AME_OK) {
            fprintf(stderr, "Insert failed for roll=%d recId=%d\n", roll, recId);
            AM_PrintError("AM_InsertEntry");
            PF_CloseFile(fd);
            free(arr);
            return 1;
        }
    }

    gettimeofday(&t2, NULL);
    double build_ms = elapsed_ms(t1, t2);
    printf("Index build time = %.3f ms (mode %d)\n", build_ms, mode);

    /* -------- 5. Sample queries on the index ------------------------ */
    printf("\nRunning sample equality and range queries on roll-no...\n");

    /* Equality query: use the first record’s roll-no */
    int targetRoll = arr[0].roll;
    int scanDesc;

    scanDesc = AM_OpenIndexScan(fd, 'i', sizeof(int), EQUAL, (char *)&targetRoll);
    if (scanDesc < 0) {
        AM_PrintError("AM_OpenIndexScan (EQUAL)");
    } else {
        printf("Equality query: roll = %d\n", targetRoll);
        int rid;
        int count = 0;
        while ((rid = AM_FindNextEntry(scanDesc)) >= 0) {
            printf("  -> recId = %d\n", rid);
            count++;
        }
        if (rid != AME_EOF && rid < 0) {
            AM_PrintError("AM_FindNextEntry (EQUAL)");
        }
        printf("Total matches = %d\n\n", count);
        AM_CloseIndexScan(scanDesc);
    }

    /* Range-ish query: roll >= lowRoll (we'll just show count) */
    int lowRoll  = arr[n/2].roll;
    int highRoll = lowRoll + 50;   /* just for printing */

    printf("Range-like query: roll >= %d (intended window [%d, %d])\n",
           lowRoll, lowRoll, highRoll);

    scanDesc = AM_OpenIndexScan(fd, 'i', sizeof(int),
                                GREATER_THAN_EQUAL, (char *)&lowRoll);
    if (scanDesc < 0) {
        AM_PrintError("AM_OpenIndexScan (GE)");
    } else {
        int rid;
        int count = 0;
        while ((rid = AM_FindNextEntry(scanDesc)) >= 0) {
            /* In a full system you’d fetch record by rid and re-check highRoll.
               Here we just count for demonstration. */
            count++;
        }
        if (rid != AME_EOF && rid < 0) {
            AM_PrintError("AM_FindNextEntry (GE)");
        }
        printf("Matches with roll >= %d : %d (not filtered by upper bound here)\n\n",
               lowRoll, count);
        AM_CloseIndexScan(scanDesc);
    }

    /* -------- 6. Cleanup ------------------------------------------- */
    PF_CloseFile(fd);
    free(arr);

    printf("student_index experiment done (mode %d).\n", mode);
    return 0;
}
