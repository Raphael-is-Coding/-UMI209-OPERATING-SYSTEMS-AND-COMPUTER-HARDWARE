#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define MAX_BLOCKS 64
#define NO_BLOCK -1

typedef struct {
    char info[30];
    int nxt;
} BlockNode;

BlockNode storage[MAX_BLOCKS];
int free_start = NO_BLOCK;

void setup() {
    for (int idx = 0; idx < MAX_BLOCKS; idx++) {
        if (idx < MAX_BLOCKS - 1) {
            storage[idx].nxt = idx + 1;
        } else {
            storage[idx].nxt = NO_BLOCK;
        }
    }
    free_start = 0;
}

int get_one_block() {
    if (free_start == NO_BLOCK) return NO_BLOCK;

    int taken = free_start;
    free_start = storage[free_start].nxt;
    storage[taken].nxt = NO_BLOCK;
    return taken;
}

int get_blocks(int num) {
    if (num <= 0) return NO_BLOCK;

    int first = NO_BLOCK;
    int prev = NO_BLOCK;

    for (int cnt = 0; cnt < num; cnt++) {
        int newb = get_one_block();
        if (newb == NO_BLOCK) {
            while (first != NO_BLOCK) {
                int nxt = storage[first].nxt;
                storage[first].nxt = free_start;
                free_start = first;
                first = nxt;
            }
            return NO_BLOCK;
        }

        if (prev != NO_BLOCK) {
            storage[prev].nxt = newb;
        } else {
            first = newb;
        }
        prev = newb;
    }
    return first;
}

void release_blocks(int begin) {
    if (begin == NO_BLOCK) return;

    int cur = begin;
    int last = begin;
    while (storage[cur].nxt != NO_BLOCK) {
        last = cur;
        cur = storage[cur].nxt;
    }

    storage[last].nxt = free_start;
    free_start = begin;
}

void display_state() {
    int status[MAX_BLOCKS] = {0};

    int cur = free_start;
    while (cur != NO_BLOCK) {
        status[cur] = 0;
        cur = storage[cur].nxt;
    }

    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (status[i] == 0) {
            int isfree = 0;
            cur = free_start;
            while (cur != NO_BLOCK) {
                if (cur == i) {
                    isfree = 1;
                    break;
                }
                cur = storage[cur].nxt;
            }
            printf("%d", isfree ? 0 : 1);
        }
    }
    printf("\n");
}

void speed_test_list() {
    printf("Linked-List Speed Test (100 iterations):\n");

    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

    for (int iter = 0; iter < 100; iter++) {
        setup();
        srand(iter + 200);

        int allocs[100];
        int sizes[100];

        for (int i = 0; i < 100; i++) {
            sizes[i] = (rand() % 5) + 1;
            allocs[i] = get_blocks(sizes[i]);
        }
        for (int i = 0; i < 100; i++) {
            if (allocs[i] != NO_BLOCK) {
                release_blocks(allocs[i]);
            }
        }
    }

    gettimeofday(&t2, NULL);

    long start_u = t1.tv_sec * 1000000 + t1.tv_usec;
    long end_u = t2.tv_sec * 1000000 + t2.tv_usec;
    double total_t = (end_u - start_u) / 1000000.0;

    printf("Time: %.6f seconds\n", total_t);
    printf("Total allocations: 10,000\n");
    printf("Total frees: 10,000\n");
    printf("Operations/sec: %.0f\n\n", 20000.0 / total_t);
}

void fragment_test_list() {
    printf("Linked-List Fragmentation Test:\n");
    setup();
    srand(777);

    int starts[20];
    int counts[20];

    for (int i = 0; i < 20; i++) {
        counts[i] = (rand() % 5) + 1;
        starts[i] = get_blocks(counts[i]);
    }

    int freed[5];
    for (int i = 0; i < 5; i++) {
        freed[i] = rand() % 20;
        release_blocks(starts[freed[i]]);
    }
    int big = get_blocks(12);
    if (big != NO_BLOCK) {
        printf("Success: got 12 blocks at %d\n", big);
        release_blocks(big);
    } else {
        printf("Failed\n");
    }

    for (int i = 0; i < 20; i++) {
        int skipit = 0;
        for (int j = 0; j < 5; j++) if (i == freed[j]) skipit = 1;
        if (!skipit && starts[i] != NO_BLOCK) release_blocks(starts[i]);
    }
    printf("\n");
}

void trace_test_list() {
    printf("Linked-List Trace (15 steps):\n");
    setup();

    int seq[] = {2,3,5,2,4,6,1,3,5,2,4,3,2,1,5};

    for (int s = 0; s < 15; s++) {
        get_blocks(seq[s]);
        printf("Step %2d (%d): ", s+1, seq[s]);
        display_state();
    }
    printf("\n");
}

int main() {
    speed_test_list();
    fragment_test_list();
    trace_test_list();
    return 0;
}