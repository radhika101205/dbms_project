/* student_index.c
 * Build a B+-tree index on student roll numbers using the ToyDB AM layer.
 * mode 1: insert in file order (unsorted)
 * mode 2: sort by roll-no first (bulk-style)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "am.h"
#include "pf.h"

/* simple wall-clock timer in milliseconds */
static double now_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

/* parse roll number (1st field) from a semicolon-separated student line */
static int parse_rollno(const char *line, int *roll_out) {
    /* skip empty / header lines */
    if (line[0] == '\0' || line[0] == '\n')
        return -1;

    const char *p = strchr(line, ';');
    if (!p) {
        fprintf(stderr, "parse_rollno: malformed line: %s", line);
        return -1;
    }

    char buf[32];
    size_t len = p - line;
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    memcpy(buf, line, len);
    buf[len] = '\0';

    *roll_out = atoi(buf);
    return 0;
}

typedef struct {
    int roll;
    int recId;   /* logical record id = line number (0-based) */
} StuKey;

static int cmp_StuKey(const void *a, const void *b) {
    const StuKey *x = (const StuKey *)a;
    const StuKey *y = (const StuKey *)b;
    if (x->roll < y->roll) return -1;
    if (x->roll > y->roll) return 1;
    return 0;
}

static void build_index_mode1(const char *data_path) {
    FILE *fp = fopen(data_path, "r");
    if (!fp) {
        perror("fopen student_txt");
        exit(1);
    }

    StuKey *arr = NULL;
    int cap = 0, n = 0;
    char line[4096];
    int line_no = 0;

    while (fgets(line, sizeof(line), fp)) {
        int roll;
        if (parse_rollno(line, &roll) != 0) {
            /* malformed line, skip but keep line_no increasing */
            line_no++;
            continue;
        }
        if (n == cap) {
            cap = cap ? cap * 2 : 4096;
            arr = (StuKey *)realloc(arr, cap * sizeof(StuKey));
        }
        arr[n].roll = roll;
        arr[n].recId = line_no;   /* recId = line number */
        n++;
        line_no++;
    }
    fclose(fp);

    printf("Loaded %d student records from %s\n", n, data_path);
    printf("Using file order (unsorted) for index build...\n");

    /* create + open index */
    AM_CreateIndex("student.idx", 1, INT_TYPE, sizeof(int));
    int fd = AM_OpenIndex("student.idx", 1);
    if (fd < 0) {
        AM_PrintError("AM_OpenIndex");
        exit(1);
    }

    double t0 = now_ms();
    for (int i = 0; i < n; i++) {
        int roll = arr[i].roll;
        int recId = arr[i].recId;
        int err = AM_InsertEntry(fd, INT_TYPE, sizeof(int),
                                 (char *)&roll, recId);
        if (err != AME_OK) {
            AM_PrintError("AM_InsertEntry");
            exit(1);
        }
    }
    double t1 = now_ms();
    printf("Index build time = %.3f ms (mode 1)\n\n", t1 - t0);

    free(arr);

    /* -------- Equality query timing -------- */
    printf("Running sample equality and range queries on roll-no...\n");

    int query_roll = 95302001;   /* pick a known roll number */
    double qeq_start = now_ms();
    int scanDesc = AM_OpenIndexScan(fd, INT_TYPE, sizeof(int),
                                    EQ_OP, (char *)&query_roll);
    if (scanDesc < 0) {
        AM_PrintError("AM_OpenIndexScan (EQ)");
        exit(1);
    }

    int matches = 0;
    int recId;
    while ((recId = AM_FindNextEntry(scanDesc)) >= 0) {
        if (matches < 20)    /* don’t spam too much */
            printf("  -> recId = %d\n", recId);
        matches++;
    }
    double qeq_end = now_ms();
    AM_CloseIndexScan(scanDesc);

    if (recId != AME_EOF && recId < 0) {
        AM_PrintError("AM_FindNextEntry (EQ)");
    }

    printf("Total matches (EQ) = %d\n", matches);
    printf("Equality query time (mode 1) = %.3f ms\n\n", qeq_end - qeq_start);

    /* -------- Range-like query timing -------- */
    int lower = 1026006;
    printf("Range-like query: roll >= %d "
           "(intended window [%d, %d])\n", lower, lower, lower + 50);

    double qrg_start = now_ms();
    scanDesc = AM_OpenIndexScan(fd, INT_TYPE, sizeof(int),
                                GE_OP, (char *)&lower);
    if (scanDesc < 0) {
        AM_PrintError("AM_OpenIndexScan (GE)");
        exit(1);
    }

    matches = 0;
    while ((recId = AM_FindNextEntry(scanDesc)) >= 0) {
        matches++;
    }
    double qrg_end = now_ms();
    AM_CloseIndexScan(scanDesc);

    if (recId != AME_EOF && recId < 0) {
        AM_PrintError("AM_FindNextEntry (GE)");
    }

    printf("Matches with roll >= %d : %d (not filtered by upper bound here)\n",
           lower, matches);
    printf("Range-like query time (mode 1) = %.3f ms\n\n", qrg_end - qrg_start);

    AM_CloseIndex(fd);
    printf("student_index experiment done (mode 1).\n");
}

/* Mode 2: sort by roll before inserting – bulk-style */
static void build_index_mode2(const char *data_path) {
    FILE *fp = fopen(data_path, "r");
    if (!fp) {
        perror("fopen student_txt");
        exit(1);
    }

    StuKey *arr = NULL;
    int cap = 0, n = 0;
    char line[4096];
    int line_no = 0;

    while (fgets(line, sizeof(line), fp)) {
        int roll;
        if (parse_rollno(line, &roll) != 0) {
            line_no++;
            continue;
        }
        if (n == cap) {
            cap = cap ? cap * 2 : 4096;
            arr = (StuKey *)realloc(arr, cap * sizeof(StuKey));
        }
        arr[n].roll = roll;
        arr[n].recId = line_no;
        n++;
        line_no++;
    }
    fclose(fp);

    printf("Loaded %d student records from %s\n", n, data_path);
    printf("Sorting by roll-no for bulk-load style build...\n");
    qsort(arr, n, sizeof(StuKey), cmp_StuKey);

    AM_CreateIndex("student.idx", 1, INT_TYPE, sizeof(int));
    int fd = AM_OpenIndex("student.idx", 1);
    if (fd < 0) {
        AM_PrintError("AM_OpenIndex");
        exit(1);
    }

    double t0 = now_ms();
    for (int i = 0; i < n; i++) {
        int roll = arr[i].roll;
        int recId = arr[i].recId;
        int err = AM_InsertEntry(fd, INT_TYPE, sizeof(int),
                                 (char *)&roll, recId);
        if (err != AME_OK) {
            AM_PrintError("AM_InsertEntry");
            exit(1);
        }
    }
    double t1 = now_ms();
    printf("Index build time = %.3f ms (mode 2)\n\n", t1 - t0);

    free(arr);

    /* -------- Equality query timing -------- */
    printf("Running sample equality and range queries on roll-no...\n");

    int query_roll = 0;   /* just some value that appears many times */
    double qeq_start = now_ms();
    int scanDesc = AM_OpenIndexScan(fd, INT_TYPE, sizeof(int),
                                    EQ_OP, (char *)&query_roll);
    if (scanDesc < 0) {
        AM_PrintError("AM_OpenIndexScan (EQ)");
        exit(1);
    }

    int matches = 0;
    int recId;
    while ((recId = AM_FindNextEntry(scanDesc)) >= 0) {
        if (matches < 20)
            printf("  -> recId = %d\n", recId);
        matches++;
    }
    double qeq_end = now_ms();
    AM_CloseIndexScan(scanDesc);

    if (recId != AME_EOF && recId < 0) {
        AM_PrintError("AM_FindNextEntry (EQ)");
    }

    printf("Total matches (EQ) = %d\n", matches);
    printf("Equality query time (mode 2) = %.3f ms\n\n", qeq_end - qeq_start);

    /* -------- Range-like query timing -------- */
    int lower = 92304016;
    printf("Range-like query: roll >= %d "
           "(intended window [%d, %d])\n", lower, lower, lower + 50);

    double qrg_start = now_ms();
    scanDesc = AM_OpenIndexScan(fd, INT_TYPE, sizeof(int),
                                GE_OP, (char *)&lower);
    if (scanDesc < 0) {
        AM_PrintError("AM_OpenIndexScan (GE)");
        exit(1);
    }

    matches = 0;
    while ((recId = AM_FindNextEntry(scanDesc)) >= 0) {
        matches++;
    }
    double qrg_end = now_ms();
    AM_CloseIndexScan(scanDesc);

    if (recId != AME_EOF && recId < 0) {
        AM_PrintError("AM_FindNextEntry (GE)");
    }

    printf("Matches with roll >= %d : %d (not filtered by upper bound here)\n",
           lower, matches);
    printf("Range-like query time (mode 2) = %.3f ms\n\n", qrg_end - qrg_start);

    AM_CloseIndex(fd);
    printf("student_index experiment done (mode 2).\n");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr,
                "Usage: %s <student.txt path> <mode: 1=unsorted, 2=sorted>\n",
                argv[0]);
        exit(1);
    }

    const char *data_path = argv[1];
    int mode = atoi(argv[2]);

    PF_Init();   /* init PF layer */
    AM_Init();   /* if your AM code has an init; else remove */

    printf("student_index experiment: mode = %d\n", mode);

    if (mode == 1)
        build_index_mode1(data_path);
    else if (mode == 2)
        build_index_mode2(data_path);
    else {
        fprintf(stderr, "Invalid mode %d (use 1 or 2)\n", mode);
        return 1;
    }

    return 0;
}
