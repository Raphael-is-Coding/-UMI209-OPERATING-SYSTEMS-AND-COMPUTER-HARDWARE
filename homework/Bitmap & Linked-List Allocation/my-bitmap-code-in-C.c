// BITMAP ALLOCATOR - 
// includes first
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
// timing stuff
#include <time.h>
#include <sys/time.h>

// here i defining our disk size
#define HOW_MANY_BLOCKS_WE_HAVE 64
// each byte has 8 bits
#define HOW_MANY_BITS_IN_ONE_BYTE 8
// calculate bitmap array size
#define BITMAP_ARRAY_NEEDED ((HOW_MANY_BLOCKS_WE_HAVE + HOW_MANY_BITS_IN_ONE_BYTE -1) / HOW_MANY_BITS_IN_ONE_BYTE)

// global bitmap array - each bit = 1 block
uint8_t the_bitmap_thing[BITMAP_ARRAY_NEEDED];

// ---
// Function to start fresh
void start_bitmap_from_scratch() {
    // set all bytes to zero = all blocks free
    memset(the_bitmap_thing, 0, BITMAP_ARRAY_NEEDED);
    // in real system we'd log this maybe
}

// ---
// check if block is taken or free
int check_block_status(int block_num) {
    // which byte has our bit?
    int byte_position = block_num / HOW_MANY_BITS_IN_ONE_BYTE;
    // which bit inside that byte?
    int bit_offset = block_num % HOW_MANY_BITS_IN_ONE_BYTE;
    // get the bit value 0 or 1
    return (the_bitmap_thing[byte_position] >> bit_offset) & 1;
}

// ---
// MAIN ALLOCATION FUNCTION - finds consecutive blocks
int find_and_take_blocks(int how_many_we_want) {
    // basic error check
    if (how_many_we_want <= 0 || how_many_we_want > HOW_MANY_BLOCKS_WE_HAVE) {
        return -1; // invalid request
    }
    int consecutive_counter = 0; // how many free blocks in a row
    int maybe_start_here = -1;   // where we might start allocation

    // scan through all blocks 0 to 63
    for (int current_block = 0; current_block < HOW_MANY_BLOCKS_WE_HAVE; current_block++) {
        // calculate byte and bit positions
        int which_byte = current_block / HOW_MANY_BITS_IN_ONE_BYTE;
        int which_bit = current_block % HOW_MANY_BITS_IN_ONE_BYTE;

        // check if this block is free (bit = 0)
        if ((the_bitmap_thing[which_byte] & (1 << which_bit)) == 0) {
            // it's free!
            if (consecutive_counter == 0) {
                maybe_start_here = current_block;  // mark as potential start
            }
            consecutive_counter++;  // increment our counter

            // did we find enough consecutive blocks?
            if (consecutive_counter == how_many_we_want) {
                // here we found enough space
                // now mark all these blocks as taken (set bits to 1)
                for (int i = 0; i < how_many_we_want; i++) {
                    int block_to_mark = maybe_start_here + i;
                    int mark_byte = block_to_mark / HOW_MANY_BITS_IN_ONE_BYTE;
                    int mark_bit = block_to_mark % HOW_MANY_BITS_IN_ONE_BYTE;
                    // set the bit to 1 using OR operation
                    the_bitmap_thing[mark_byte] |= (1 << mark_bit);
                }
                return maybe_start_here;  // return starting block
            }
        } else {
            // block is taken - reset our search
            consecutive_counter = 0;
            maybe_start_here = -1;
        }
    }

    // if we get here, we didn't find enough consecutive blocks
    return -1;  // failure
}

// function to free blocks
void give_back_blocks(int start_block, int block_count) {
    // just clean the bits for each block
    for (int i = 0; i < block_count; i++) {
        int block_num = start_block + i;
        int byte_idx = block_num / HOW_MANY_BITS_IN_ONE_BYTE;
        int bit_pos = block_num % HOW_MANY_BITS_IN_ONE_BYTE;
        // clean bit to 0 using AND with complement
        the_bitmap_thing[byte_idx] &= ~(1 << bit_pos);
    }
}

// ---
// show_current_bitmap_state
void show_current_bitmap() {
    for (int blk = 0; blk < HOW_MANY_BLOCKS_WE_HAVE; blk++) {
        printf("%d", check_block_status(blk));
    }
    printf("\n");
}

// ---
// TEST 1: SPEED TEST - run 100 times
void do_speed_test_bitmap_human() {
    printf(">>> TEST 1: BITMAP SPEED TEST <<<\n");
    printf("We'll run 100 complete cycles (allocate 100 + free 100 each time)\n");
    printf("That's 100 × 200 = 20,000 total operations\n\n");

    // high precision timer setup it will be explained by handwritten
    struct timeval time_start, time_end;
    gettimeofday(&time_start, NULL); // start timing

    // main test loop - 100 iterations
    for (int iteration_num = 0; iteration_num < 100; iteration_num++) {
        // fresh start for each iteration
        start_bitmap_from_scratch();
        // use different seed each time but consistent
        srand(iteration_num * 123);

        // arrays to track what we allocated
        int allocation_starts[100];
        int allocation_sizes[100];

        // --- ALLOCATION PHASE ---
        for (int i = 0; i < 100; i++) {
            // random size between 1 and 5 blocks
            allocation_sizes[i] = (rand() % 5) + 1;
            // try to allocate
            allocation_starts[i] = find_and_take_blocks(allocation_sizes[i]);
        }

        // --- FREEING PHASE ---
        for (int i = 0; i < 100; i++) {
            if (allocation_starts[i] != -1) { // if allocation succeeded
                give_back_blocks(allocation_starts[i], allocation_sizes[i]);
            }
        }
    }

    // end timing
    gettimeofday(&time_end, NULL);

    // calculate elapsed time in seconds
    long start_micro = time_start.tv_sec * 1000000 + time_start.tv_usec;
    long end_micro = time_end.tv_sec * 1000000 + time_end.tv_usec;
    double total_seconds = (end_micro - start_micro) / 1000000.0;

    // print results
    printf("RESULTS:\n");
    printf("Total time: %.6f seconds\n", total_seconds);
    printf("Total iterations: 100\n");
    printf("Operations per iteration: 200 (100 alloc + 100 free)\n");
    printf("TOTAL operations: 20,000\n");
    printf("Operations per second: %.0f ops/sec\n", 20000.0 / total_seconds);
    printf("Average time per operation: %.3f microseconds\n\n",
            (total_seconds * 1000000) / 20000.0);
}

// ---
// TEST 2: FRAGMENTATION TEST
void do_fragmentation_test_bitmap() {
    printf(">>> TEST 2: BITMAP FRAGMENTATION TEST <<<\n");

    start_bitmap_from_scratch();
    srand(999); // fixed seed for reproducibility

    int file_start_positions[20];
    int file_block_counts[20];

    printf("Step 1: Creating 20 random files...\n");
    for (int file_id = 0; file_id < 20; file_id++) {
        // random file size 1-5 blocks
        file_block_counts[file_id] = (rand() % 5) + 1;
        file_start_positions[file_id] = find_and_take_blocks(file_block_counts[file_id]);
        // printf(" File %d: %d blocks starting at %d\n",
        // file_id+1, file_block_counts[file_id], file_start_positions[file_id]);
    }

    printf("Step 2: Randomly selecting 5 files to delete...\n");
    int files_to_delete[5];
    for (int d = 0; d < 5; d++) {
        files_to_delete[d] = rand() % 20;
        printf(" Deleting file %d (frees up %d blocks)\n",
               files_to_delete[d]+1, file_block_counts[files_to_delete[d]]);
        give_back_blocks(file_start_positions[files_to_delete[d]],
                         file_block_counts[files_to_delete[d]]);
    }

    printf("Step 3: Trying to allocate a large file (12 blocks)...\n");
    int big_file_start = find_and_take_blocks(12);

    if (big_file_start != -1) {
        printf(" Successful large file allocated starting at block %d\n",
               big_file_start);
        give_back_blocks(big_file_start, 12);
    } else {
        printf(" Failed Cannot find 12 consecutive free blocks.\n");
    }

    // cleanup - free remaining files
    for (int f = 0; f < 20; f++) {
        int already_freed = 0;
        for (int d = 0; d < 5; d++) {
            if (f == files_to_delete[d]) already_freed = 1;
        }
        if (!already_freed && file_start_positions[f] != -1) {
            give_back_blocks(file_start_positions[f], file_block_counts[f]);
        }
    }
    printf("\n");
}

// TEST 3: ALLOCATION TRACE
void do_allocation_trace_bitmap() {
    printf(">>> TEST 3: BITMAP ALLOCATION TRACE <<<\n");
    printf("Allocation sequence: 2, 3, 5, 2, 4, 6, 1, 3, 5, 2, 4, 3, 2, 1, 5\n\n");

    start_bitmap_from_scratch();

    // the fixed sequence from the assignment
    int allocation_sequence[15] = {2, 3, 5, 2, 4, 6, 1, 3, 5, 2, 4, 3, 2, 1, 5};

    for (int step_number = 0; step_number < 15; step_number++) {
        find_and_take_blocks(allocation_sequence[step_number]);
        printf("After step %2d (allocate %d): ", step_number+1,
               allocation_sequence[step_number]);
        show_current_bitmap();
    }
}

// MAIN FUNCTION
int main() {
    // run all three tests
    do_speed_test_bitmap_human();
    do_fragmentation_test_bitmap();
    do_allocation_trace_bitmap();

    printf("=== END ===\n");
    return 0;
}