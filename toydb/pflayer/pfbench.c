#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "pf.h"

#define NUM_PAGES 50       // how many pages we keep in the file
#define NUM_OPS   1000     // how many operations per experiment

void run_experiment(const char *label, int policy, int writePercent);

int main() {
    PF_Init();
    PF_SetBufferSize(5);        // small buffer to force replacements

    srand(time(NULL));

    // LRU experiments
    run_experiment("LRU 0W/100R",   PF_REPL_LRU, 0);
    run_experiment("LRU 25W/75R",   PF_REPL_LRU, 25);
    run_experiment("LRU 50W/50R",   PF_REPL_LRU, 50);
    run_experiment("LRU 75W/25R",   PF_REPL_LRU, 75);
    run_experiment("LRU 100W/0R",   PF_REPL_LRU, 100);

    // MRU experiments
    run_experiment("MRU 0W/100R",   PF_REPL_MRU, 0);
    run_experiment("MRU 25W/75R",   PF_REPL_MRU, 25);
    run_experiment("MRU 50W/50R",   PF_REPL_MRU, 50);
    run_experiment("MRU 75W/25R",   PF_REPL_MRU, 75);
    run_experiment("MRU 100W/0R",   PF_REPL_MRU, 100);

    return 0;
}

void run_experiment(const char *label, int policy, int writePercent) {
    int fd, pagenum, i;
    char *pagebuf;
    char filename[64];

    // choose policy BEFORE opening file (matches “select at open” idea)
    PF_SetReplacementPolicy(policy);

    snprintf(filename, sizeof(filename), "pfbench_%d_%d.dat",
             policy, writePercent);

    // (Re)create file fresh for this experiment
    PF_DestroyFile(filename);          // ignore error if not exists
    if (PF_CreateFile(filename) != PFE_OK) {
        PF_PrintError("PF_CreateFile");
        return;
    }

    if ((fd = PF_OpenFile(filename)) < 0) {
        PF_PrintError("PF_OpenFile");
        return;
    }

    // allocate NUM_PAGES logical pages once
    for (i = 0; i < NUM_PAGES; i++) {
        if (PF_AllocPage(fd, &pagenum, &pagebuf) != PFE_OK) {
            PF_PrintError("PF_AllocPage");
            return;
        }
        // write some dummy content
        pagebuf[0] = (char)i;
        if (PF_UnfixPage(fd, pagenum, TRUE) != PFE_OK) {
            PF_PrintError("PF_UnfixPage");
            return;
        }
    }

    // reset stats before workload
    PF_ResetStats();

    // workload: random page accesses with writePercent% writes
    for (i = 0; i < NUM_OPS; i++) {
        int page = rand() % NUM_PAGES;
        int r = rand() % 100;

        if (PF_GetThisPage(fd, page, &pagebuf) != PFE_OK) {
            PF_PrintError("PF_GetThisPage");
            return;
        }

        if (r < writePercent) {
            // write: modify content and unfix with dirty = TRUE
            pagebuf[0] = (char)(pagebuf[0] + 1);
            if (PF_UnfixPage(fd, page, TRUE) != PFE_OK) {
                PF_PrintError("PF_UnfixPage");
                return;
            }
        } else {
            // read: just look at content and unfix with dirty = FALSE
            volatile char c = pagebuf[0];
            (void)c;
            if (PF_UnfixPage(fd, page, FALSE) != PFE_OK) {
                PF_PrintError("PF_UnfixPage");
                return;
            }
        }
    }

    printf("\n=== %s ===\n", label);
    PF_PrintStats();

    if (PF_CloseFile(fd) != PFE_OK) {
        PF_PrintError("PF_CloseFile");
        return;
    }
}
