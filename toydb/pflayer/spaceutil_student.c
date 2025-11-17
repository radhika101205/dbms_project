#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>

#define PF_PAGE_SIZE 1020      // same as in pf.h

// Remove trailing newline / carriage return
static void rstrip(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) {
        s[n-1] = '\0';
        n--;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr,
            "Usage: %s <student_txt_path> <student_hf_path> <recSize1> [recSize2 ...]\n"
            "Example: %s ../../data/student.txt student.hf 150 200 250\n",
            argv[0], argv[0]);
        return 1;
    }

    const char *student_txt = argv[1];
    const char *student_hf  = argv[2];

    // ------------ 1. Read student.txt and compute logical data size -------------
    FILE *fp = fopen(student_txt, "r");
    if (!fp) {
        perror("fopen student.txt");
        return 1;
    }

    char *line = NULL;
    size_t bufsize = 0;
    ssize_t len;

    long long numRecords = 0;
    long long totalBytes = 0;      // sum of lengths WITHOUT newline
    int maxLen = 0;

    while ((len = getline(&line, &bufsize, fp)) != -1) {
        rstrip(line);
        int L = (int)strlen(line);
        numRecords++;
        totalBytes += L;
        if (L > maxLen) maxLen = L;
    }
    free(line);
    fclose(fp);

    if (numRecords == 0) {
        fprintf(stderr, "student.txt seems to be empty.\n");
        return 1;
    }

    double avgLen = (double)totalBytes / (double)numRecords;

    printf("=== Logical data stats from %s ===\n", student_txt);
    printf("Number of records        : %lld\n", numRecords);
    printf("Total logical data bytes : %lld\n", totalBytes);
    printf("Average record length    : %.2f bytes\n", avgLen);
    printf("Maximum record length    : %d bytes\n\n", maxLen);

    // ------------ 2. Get size of student.hf (slotted-page heap file) -----------
    struct stat st;
    if (stat(student_hf, &st) != 0) {
        perror("stat student.hf");
        return 1;
    }

    long long hfBytes = st.st_size;
    double hfPages   = (double)hfBytes / (double)PF_PAGE_SIZE;

    double utilSlotted = (double)totalBytes / (double)hfBytes;

    printf("=== Slotted-page file (%s) ===\n", student_hf);
    printf("File size on disk        : %lld bytes\n", hfBytes);
    printf("Approx. number of PF pages: %.2f (page size = %d bytes)\n",
           hfPages, PF_PAGE_SIZE);
    printf("Space utilisation (slotted) = total_data / file_size = %.4f (%.2f%%)\n\n",
           utilSlotted, utilSlotted * 100.0);

    // ------------ 3. Static-record variants for given record sizes -------------
    printf("=== Static record layouts (for comparison) ===\n");
    printf("Assuming file size = numRecords * recSize (simple fixed-length packing)\n\n");

    for (int i = 3; i < argc; i++) {
        int recSize = atoi(argv[i]);
        if (recSize <= 0) {
            printf("[recSize=%s] Skipping (not a positive integer)\n", argv[i]);
            continue;
        }

        long long staticBytes = numRecords * (long long)recSize;
        double staticPages = ceil((double)staticBytes / (double)PF_PAGE_SIZE);
        double utilStatic = (double)totalBytes / (double)staticBytes;

        printf("--- recSize = %d bytes ---\n", recSize);
        printf("Static file size         : %lld bytes\n", staticBytes);
        printf("Approx. PF pages         : %.0f\n", staticPages);
        printf("Space utilisation (static) = total_data / file_size = %.4f (%.2f%%)\n\n",
               utilStatic, utilStatic * 100.0);
    }

    return 0;
}
