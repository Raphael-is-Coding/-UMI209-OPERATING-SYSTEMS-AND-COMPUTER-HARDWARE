#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define BLOCK_COUNT 64
#define BITS_IN_BYTE 8
#define MAP_SIZE ((BLOCK_COUNT + BITS_IN_BYTE - 1) / BITS_IN_BYTE)

uint8_t allocation_map[MAP_SIZE];

void map_init() {
    memset(allocation_map, 0, MAP_SIZE);
}

int allocate_contiguous(int need) {
    if (need <= 0 || need > BLOCK_COUNT) return -1;

    int seq = 0;
    int begin = -1;

    for (int i = 0; i < BLOCK_COUNT; i++) {
        int by = i / BITS_IN_BYTE;
        int bi = i % BITS_IN_BYTE;

        if ((allocation_map[by] & (1 << bi)) == 0) {
            if (seq == 0) begin = i;
            seq++;
            if (seq == need) {
                for (int j = 0; j < need; j++) {
                    int blk = begin + j;
                    int b = blk / BITS_IN_BYTE;
                    int bt = blk % BITS_IN_BYTE;
                    allocation_map[b] |= (1 << bt);
                }
                return begin;
            }
        } else {
            seq = 0;
            begin = -1;
        }
    }
    return -1;
}

void free_contiguous(int start, int cnt) {
    for (int i = 0; i < cnt; i++) {
        int blk = start + i;
        int by = blk / BITS_IN_BYTE;
        int bi = blk % BITS_IN_BYTE;
        allocation_map[by] &= ~(1 << bi);
    }
}

void show_map() {
    for (int i = 0; i < BLOCK_COUNT; i++) {
        int by = i / BITS_IN_BYTE;
        int bi = i % BITS_IN_BYTE;
        printf("%d", (allocation_map[by] >> bi) & 1);
    }
    printf("\n");
}

void speed_test_bitmap() {
    printf("Bitmap Speed Test (100 iterations):\n");

    struct timeval begin_time, end_time;
    gettimeofday(&begin_time, NULL);

    for (int run = 0; run < 100; run++) {
        map_init();
        srand(run + 100);

        int allocs[100];
        int sizes[100];

        for (int i = 0; i < 100; i++) {
            sizes[i] = (rand() % 5) + 1;
            allocs[i] = allocate_contiguous(sizes[i]);
        }
        for (int i = 0; i < 100; i++) {
            if (allocs[i] != -1) {
                free_contiguous(allocs[i], sizes[i]);
            }
        }
    }

    gettimeofday(&end_time, NULL);

    long start_us = begin_time.tv_sec * 1000000 + begin_time.tv_usec;
    long end_us = end_time.tv_sec * 1000000 + end_time.tv_usec;
    double elapsed = (end_us - start_us) / 1000000.0;

    printf("Time: %.6f seconds\n", elapsed);
    printf("Total allocations: 10,000\n");
    printf("Total frees: 10,000\n");
    printf("Operations/sec: %.0f\n\n", 20000.0 / elapsed);
}

void fragment_test_bitmap() {
    printf("Bitmap Fragmentation Test:\n");
    map_init();
    srand(555);

    int file_st[20];
    int file_sz[20];

    for (int i = 0; i < 20; i++) {
        file_sz[i] = (rand() % 5) + 1;
        file_st[i] = allocate_contiguous(file_sz[i]);
    }

    int free_idx[5];
    for (int i = 0; i < 5; i++) {
        free_idx[i] = rand() % 20;
        free_contiguous(file_st[free_idx[i]], file_sz[free_idx[i]]);
    }
    int big = allocate_contiguous(12);
    if (big != -1) {
        printf("Success: got 12 blocks at %d\n", big);
        free_contiguous(big, 12);
    } else {
        printf("Failed: no 12 contiguous blocks\n");
    }

    for (int i = 0; i < 20; i++) {
        int skip = 0;
        for (int j = 0; j < 5; j++) if (i == free_idx[j]) skip = 1;
        if (!skip && file_st[i] != -1) free_contiguous(file_st[i], file_sz[i]);
    }
    printf("\n");
}

void trace_test_bitmap() {
    printf("Bitmap Trace (15 steps):\n");
    map_init();

    int steps[] = {2,3,5,2,4,6,1,3,5,2,4,3,2,1,5};

    for (int s = 0; s < 15; s++) {
        allocate_contiguous(steps[s]);
        printf("Step %2d (%d): ", s+1, steps[s]);
        show_map();
    }
    printf("\n");
}

int main() {
    printf("=== BITMAP AI GENERATED ===\n\n");
    speed_test_bitmap();
    fragment_test_bitmap();
    trace_test_bitmap();
    return 0;
}