#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h> // for better timing

// constants
#define TOTAL_BLOCKS_IN_DISK 64
#define END_MARKER -1 // means "no next block"

// each block structure
typedef struct disk_block_node {
    char some_data[32];    // would be file data in real system
    int pointer_to_next;    // next block in chain
} BlockNode;

// global variables
BlockNode my_disk[TOTAL_BLOCKS_IN_DISK]; // the whole disk
int first_free_block = END_MARKER;        // head of free list

// ---
// setup function - initializes everything
void initialize_linked_system() {
    // link all blocks into one big free list
    for (int i = 0; i < TOTAL_BLOCKS_IN_DISK; i++) {
        // put some data
        snprintf(my_disk[i].some_data, 32, "block-%d-initial", i);

        // chain them: 0 -> 1 -> 2 -> ... -> END_MARKER
        if (i < TOTAL_BLOCKS_IN_DISK - 1) {
            my_disk[i].pointer_to_next = i + 1;
        } else {
            my_disk[i].pointer_to_next = END_MARKER; // last one
        }
    }
    // free list starts at block 0
    first_free_block = 0;
}

// allocate ONE block
int grab_one_free_block() {
    if (first_free_block == END_MARKER) {
        return END_MARKER; // no free blocks!
    }

    // take from front of free list LIFO style
    int block_we_took = first_free_block;

    // update free list head
    first_free_block = my_disk[first_free_block].pointer_to_next;

    // clean the next pointer in taken block
    my_disk[block_we_took].pointer_to_next = END_MARKER;

    return block_we_took;
}

// allocate MULTIPLE blocks for a file
int allocate_file_blocks(int how_many_needed) {
    // check input
    if (how_many_needed <= 0) {
        return END_MARKER;
    }
    int first_block_of_file = END_MARKER;
    int previous_block = END_MARKER;

    // try to get each block we need
    for (int i = 0; i < how_many_needed; i++) {
        int new_block = grab_one_free_block();

        // check if we ran out of space
        if (new_block == END_MARKER) {
            // out of space
            // need to free any blocks we already got
            while (first_block_of_file != END_MARKER) {
                int next_in_chain = my_disk[first_block_of_file].pointer_to_next;
                my_disk[first_block_of_file].pointer_to_next = first_free_block;
                first_free_block = first_block_of_file;
                first_block_of_file = next_in_chain;
            }
            return END_MARKER; // fail
        }

        // put some data in the block
        snprintf(my_disk[new_block].some_data, 32, "file-data-%d", i);

        // link it into our file chain
        if (previous_block != END_MARKER) {
            my_disk[previous_block].pointer_to_next = new_block;
        } else {
            first_block_of_file = new_block; // this is first block
        }
        previous_block = new_block;
    }

    return first_block_of_file;
}

// free a file (all its blocks)
void free_up_file_blocks(int file_start_block) {
    if (file_start_block == END_MARKER) {
        return; // nothing to free
    }
    // find the last block in this file's chain
    int current = file_start_block;
    int last_block = file_start_block;
    while (my_disk[current].pointer_to_next != END_MARKER) {
        last_block = current;
        current = my_disk[current].pointer_to_next;
    }
    // add entire chain to front of free list
    my_disk[last_block].pointer_to_next = first_free_block;
    first_free_block = file_start_block;
}

// display current state (similar to bitmap output)
void show_linked_list_state() {
    // mark all blocks as "allocated" (1) initially
    int block_status[TOTAL_BLOCKS_IN_DISK];
    for (int i = 0; i < TOTAL_BLOCKS_IN_DISK; i++) {
        block_status[i] = 1; // assuming allocated
    }
    // mark blocks in free list as free (0)
    int curr = first_free_block;
    while (curr != END_MARKER) {
        block_status[curr] = 0; // free
        curr = my_disk[curr].pointer_to_next;
    }
    // print the status
    for (int blk = 0; blk < TOTAL_BLOCKS_IN_DISK; blk++) {
        printf("%d", block_status[blk]);
    }
    printf("\n");
}

// ---
// TEST 1: SPEED TEST (100 iterations)
void run_speed_test_linked_human() {
    printf(">>> TEST 1: LINKED-LIST SPEED TEST <<<\n");
    printf("Running 100 complete cycles (100 alloc + 100 free each time)\n");
    printf("Total: 100 × 200 = 20,000 operations\n\n");

    // high precision timing
    struct timeval begin_time, finish_time;
    gettimeofday(&begin_time, NULL);

    // main test loop - 100 iterations
    for (int iteration = 0; iteration < 100; iteration++) {
        // fresh start each iteration
        initialize_linked_system();
        // consistent but varying seed
        srand(iteration * 456);

        // track allocations
        int file_starts[100];
        int file_sizes[100];

        // --- ALLOCATION PHASE ---
        for (int i = 0; i < 100; i++) {
            file_sizes[i] = (rand() % 5) + 1; // 1-5 blocks
            file_starts[i] = allocate_file_blocks(file_sizes[i]);
        }
        // --- FREEING PHASE ---
        for (int i = 0; i < 100; i++) {
            if (file_starts[i] != END_MARKER) {
                free_up_file_blocks(file_starts[i]);
            }
        }
    }

    // end timing
    gettimeofday(&finish_time, NULL);

    // calculate time
    long start_usec = begin_time.tv_sec * 1000000 + begin_time.tv_usec;
    long end_usec = finish_time.tv_sec * 1000000 + finish_time.tv_usec;
    double total_sec = (end_usec - start_usec) / 1000000.0;

    // results
    printf("RESULTS:\n");
    printf("Total time: %.6f seconds\n", total_sec);
    printf("Iterations completed: 100\n");
    printf("Operations per iteration: 200\n");
    printf("TOTAL operations: 20,000\n");
    printf("Operations per second: %.0f ops/sec\n", 20000.0 / total_sec);
    printf("Average time per operation: %.3f microseconds\n\n",
           (total_sec * 1000000) / 20000.0);
}

// ---
// TEST 2: FRAGMENTATION TEST
void run_fragmentation_test_linked() {
    printf(">>> TEST 2: LINKED-LIST FRAGMENTATION TEST <<<\n");

    initialize_linked_system();
    srand(888); // fixed seed

    int file_beginnings[20];
    int how_many_blocks[20];

    printf("Step 1: Creating 20 random files...\n");
    for (int f = 0; f < 20; f++) {
        how_many_blocks[f] = (rand() % 5) + 1; // 1-5 blocks
        file_beginnings[f] = allocate_file_blocks(how_many_blocks[f]);
        // printf(" File %d: %d blocks, starts at %d\n",
        //        f+1, how_many_blocks[f], file_beginnings[f]);
    }
    printf("Step 2: Randomly deleting 5 files...\n");
    int deleted_files[5];
    for (int d = 0; d < 5; d++) {
        deleted_files[d] = rand() % 20;
        printf(" Deleting file %d\n", deleted_files[d]+1);
        free_up_file_blocks(file_beginnings[deleted_files[d]]);
    }

    printf("Step 3: Trying to allocate large file (12 blocks)...\n");
    int big_file_start = allocate_file_blocks(12);

    if (big_file_start != END_MARKER) {
        printf(" Successful 12-block file allocated starting at %d\n",
               big_file_start);
        free_up_file_blocks(big_file_start);
    } else {
        printf(" Failed \n");
    }

    // cleanup
    for (int f = 0; f < 20; f++) {
        int was_deleted = 0;
        for (int d = 0; d < 5; d++) {
            if (f == deleted_files[d]) was_deleted = 1;
        }
        if (!was_deleted && file_beginnings[f] != END_MARKER) {
            free_up_file_blocks(file_beginnings[f]);
        }
    }
    printf("\n");
}

// TEST 3: ALLOCATION TRACE
void run_allocation_trace_linked() {
    printf(">>> TEST 3: LINKED-LIST ALLOCATION TRACE <<<\n");
    printf("Same sequence: 2, 3, 5, 2, 4, 6, 1, 3, 5, 2, 4, 3, 2, 1, 5\n\n");

    initialize_linked_system();

    int the_sequence[15] = {2, 3, 5, 2, 4, 6, 1, 3, 5, 2, 4, 3, 2, 1, 5};

    for (int step = 0; step < 15; step++) {
        allocate_file_blocks(the_sequence[step]);
        printf("Step %2d (allocate %d): ", step+1, the_sequence[step]);
        show_linked_list_state();
    }
}

// ---
// MAIN
int main() {
    run_speed_test_linked_human();
    run_fragmentation_test_linked();
    run_allocation_trace_linked();

    printf("END\n");
    return 0;
}