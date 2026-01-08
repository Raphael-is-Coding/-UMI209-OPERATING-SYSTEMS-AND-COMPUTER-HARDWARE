"""
 Best Fit, Worst Fit, Next Fit Implementations
"""

import random
import time


# ==================== CORE DATA STRUCTURES ====================
class FreeBlock:
    """Represents a free memory block in the linked list"""

    def __init__(self, start, size):
        self.start = start  # Starting address of the free block
        self.size = size  # Size of the free block
        self.next = None  # Pointer to next free block


class MemoryAllocator:
    """Main memory allocator implementing all three algorithms"""

    def __init__(self, total_memory=100):
        """Initialize allocator with total memory size"""
        self.total_memory = total_memory
        # Initially one big free block covering entire memory
        self.free_list = FreeBlock(0, total_memory)
        # Track allocated blocks for freeing (block_id -> (start, size))
        self.allocated_blocks = {}
        # Roving pointer for Next Fit algorithm
        self.next_fit_ptr = self.free_list
        # Counter for generating unique block IDs
        self.block_id_counter = 0
        # For tracking allocation sequence for experiments
        self.allocation_sequence = []

    # ==================== UTILITY METHODS ====================

    def print_free_list(self, algorithm=""):
        """Print the current free list in readable format"""
        print(f"{algorithm:10s} Free List: ", end="")
        current = self.free_list
        while current:
            print(f"[{current.start:3d}-{current.start + current.size - 1:3d}]({current.size:3d})", end="")
            if current.next:
                print(" → ", end="")
            current = current.next
        print()

    def coalesce_free_blocks(self):
        """Merge adjacent free blocks in the free list"""
        if not self.free_list:
            return

        current = self.free_list
        while current and current.next:
            # If current block ends where next block starts, merge them
            if current.start + current.size == current.next.start:
                current.size += current.next.size
                current.next = current.next.next
            else:
                current = current.next

    def reset(self):
        """Reset allocator to initial state"""
        self.free_list = FreeBlock(0, self.total_memory)
        self.allocated_blocks = {}
        self.next_fit_ptr = self.free_list
        self.block_id_counter = 0
        self.allocation_sequence = []

    # ==================== ALLOCATION ALGORITHMS ====================

    def allocate_best_fit(self, size, block_id=None):
        """
        BEST FIT: Find smallest free block that fits the request
        Scans entire list to minimize leftover space
        """
        if block_id is None:
            block_id = self.block_id_counter
            self.block_id_counter += 1

        if not self.free_list:
            return None  # No free memory

        best_block = None
        best_prev = None
        current = self.free_list
        prev = None

        # Search entire list for the best (smallest) fit
        while current:
            if current.size >= size:
                # Check if this is better than current best
                if best_block is None or current.size < best_block.size:
                    best_block = current
                    best_prev = prev
            prev = current
            current = current.next

        if not best_block:
            return None  # No block large enough

        # Allocate from the best block
        allocated_start = best_block.start

        # Update the free list
        if best_block.size == size:  # Exact fit - remove entire block
            if best_prev:
                best_prev.next = best_block.next
            else:
                self.free_list = best_block.next
        else:  # Split the block - allocate from beginning
            best_block.start += size
            best_block.size -= size

        # Record the allocation
        self.allocated_blocks[block_id] = (allocated_start, size)
        self.allocation_sequence.append(('allocate', block_id, size, 'best_fit'))

        return allocated_start

    def allocate_worst_fit(self, size, block_id=None):
        """
        WORST FIT: Find largest free block available
        Always picks biggest block to maximize leftover space
        """
        if block_id is None:
            block_id = self.block_id_counter
            self.block_id_counter += 1

        if not self.free_list:
            return None

        worst_block = None
        worst_prev = None
        current = self.free_list
        prev = None

        # Search entire list for the worst (largest) fit
        while current:
            if current.size >= size:
                if worst_block is None or current.size > worst_block.size:
                    worst_block = current
                    worst_prev = prev
            prev = current
            current = current.next

        if not worst_block:
            return None

        # Allocate from the worst (largest) block
        allocated_start = worst_block.start

        # Update the free list
        if worst_block.size == size:  # Exact fit
            if worst_prev:
                worst_prev.next = worst_block.next
            else:
                self.free_list = worst_block.next
        else:  # Split the block
            worst_block.start += size
            worst_block.size -= size

        self.allocated_blocks[block_id] = (allocated_start, size)
        self.allocation_sequence.append(('allocate', block_id, size, 'worst_fit'))

        return allocated_start

    def allocate_next_fit(self, size, block_id=None):
        """
        NEXT FIT: Start search from last allocation position
        Uses roving pointer to distribute allocations across memory
        """
        if block_id is None:
            block_id = self.block_id_counter
            self.block_id_counter += 1

        if not self.free_list:
            return None

        # Start from roving pointer or beginning if None
        start_node = self.next_fit_ptr if self.next_fit_ptr else self.free_list
        current = start_node
        first_pass = True

        while current:
            if current.size >= size:
                # Found a fitting block
                allocated_start = current.start
                
                # Update free list
                if current.size == size:  # Exact fit
                    # Remove current node from free list
                    # Find previous node
                    prev = None
                    temp = self.free_list
                    while temp and temp != current:
                        prev = temp
                        temp = temp.next
                    
                    if prev:
                        prev.next = current.next
                    else:
                        self.free_list = current.next
                    
                    # Update next_fit_ptr to next node or beginning
                    self.next_fit_ptr = current.next if current.next else self.free_list
                else:  # Split the block
                    current.start += size
                    current.size -= size
                    # Update roving pointer to current block (the remainder)
                    self.next_fit_ptr = current
                
                # Record allocation
                self.allocated_blocks[block_id] = (allocated_start, size)
                self.allocation_sequence.append(('allocate', block_id, size, 'next_fit'))
                return allocated_start
            
            # Move to next block
            current = current.next
            
            # If we've reached the end, wrap around to beginning
            if not current and first_pass:
                current = self.free_list
                first_pass = False
            
            # If we've come back to start_node, break to avoid infinite loop
            if current == start_node and not first_pass:
                break

        return None  # No block found

    def free_block(self, block_id):
        """Free an allocated block and merge with adjacent free blocks"""
        if block_id not in self.allocated_blocks:
            return False

        start, size = self.allocated_blocks[block_id]
        del self.allocated_blocks[block_id]

        # Create new free block
        new_block = FreeBlock(start, size)

        # Insert into free list in sorted position (by start address)
        if not self.free_list or start < self.free_list.start:
            # Insert at beginning
            new_block.next = self.free_list
            self.free_list = new_block
        else:
            # Find insertion point
            current = self.free_list
            prev = None
            while current and current.start < start:
                prev = current
                current = current.next

            # Insert between prev and current
            new_block.next = current
            if prev:
                prev.next = new_block

        # Coalesce adjacent blocks
        self.coalesce_free_blocks()

        # Update next_fit_ptr if it points to a block that was freed
        # If next_fit_ptr is None or points to a block that's no longer in free list,
        # reset it to free_list
        if self.next_fit_ptr:
            # Check if next_fit_ptr is still in the free list
            temp = self.free_list
            found = False
            while temp:
                if temp == self.next_fit_ptr:
                    found = True
                    break
                temp = temp.next
            if not found:
                self.next_fit_ptr = self.free_list
        else:
            self.next_fit_ptr = self.free_list

        self.allocation_sequence.append(('free', block_id, size, None))
        return True


# ==================== EXPERIMENT 1: ALLOCATION TRACE ====================
def experiment1_allocation_trace():
    """Run Experiment 1: Fixed allocation/free sequence"""
    print("\n" + "=" * 70)
    print("EXPERIMENT 1: Allocation Trace")
    print("=" * 70)
    print("Sequence: [10, 5, 20, -5, 12, -10, 8, 6, 7, 3, 10]")
    print("Positive = allocate, Negative = free that size\n")

    sequence = [10, 5, 20, -5, 12, -10, 8, 6, 7, 3, 10]

    # Create three allocators, one for each algorithm
    allocators = {
        'Best Fit': MemoryAllocator(100),
        'Worst Fit': MemoryAllocator(100),
        'Next Fit': MemoryAllocator(100)
    }

    # Track allocations for each algorithm
    allocations_by_size = {
        'Best Fit': {},
        'Worst Fit': {},
        'Next Fit': {}
    }

    for step, request in enumerate(sequence):
        print(f"\n{'=' * 50}")
        print(f"Step {step + 1}: Request {request}")
        print('=' * 50)

        for algo_name, allocator in allocators.items():
            if request > 0:  # Allocation
                # Generate block ID
                block_id = allocator.block_id_counter

                # Store mapping for later freeing
                if request not in allocations_by_size[algo_name]:
                    allocations_by_size[algo_name][request] = []
                allocations_by_size[algo_name][request].append(block_id)

                # Perform allocation based on algorithm
                if algo_name == 'Best Fit':
                    result = allocator.allocate_best_fit(request, block_id)
                elif algo_name == 'Worst Fit':
                    result = allocator.allocate_worst_fit(request, block_id)
                else:  # Next Fit
                    result = allocator.allocate_next_fit(request, block_id)

                allocator.print_free_list(algo_name)

            else:  # Freeing (negative request)
                size_to_free = -request

                if (size_to_free in allocations_by_size[algo_name] and
                        allocations_by_size[algo_name][size_to_free]):
                    # Free the first block of this size
                    block_id = allocations_by_size[algo_name][size_to_free].pop(0)
                    allocator.free_block(block_id)
                    allocator.print_free_list(algo_name)
                else:
                    # No block of this size to free
                    allocator.print_free_list(algo_name)

    print("\n" + "=" * 70)
    print("EXPERIMENT 1 ANALYSIS")
    print("=" * 70)

    print("\n1. BEHAVIORAL DIFFERENCES:")
    print("   • BEST FIT:")
    print("     - Tends to allocate from smallest fitting blocks")
    print("     - Creates many small fragments quickly")
    print("     - External fragmentation appears early")

    print("\n   • WORST FIT:")
    print("     - Always picks the largest available block")
    print("     - Leaves larger leftover blocks")
    print("     - Better at preserving large contiguous spaces")

    print("\n   • NEXT FIT:")
    print("     - Starts search from last allocation position")
    print("     - Distributes allocations around memory")
    print("     - More uniform fragmentation pattern")

    print("\n2. FREE LIST DIVERGENCE:")
    print("   • As fragmentation grows, algorithms produce")
    print("     completely different free list structures")
    print("   • Best Fit: Many small scattered blocks")
    print("   • Worst Fit: Fewer but larger blocks")
    print("   • Next Fit: More evenly distributed blocks")


# ==================== EXPERIMENT 2: FRAGMENTATION TEST ====================
def experiment2_fragmentation_test():
    """Run Experiment 2: Random allocations then large allocation"""
    print("\n" + "=" * 70)
    print("EXPERIMENT 2: Fragmentation Test")
    print("=" * 70)
    print("\nSteps:")
    print("1. Perform 12 random allocations (sizes 3-12)")
    print("2. Free exactly 4 previously allocated blocks at random")
    print("3. Attempt one large allocation of size 25")
    print("\n" + "-" * 70)

    # Set random seed for reproducible results
    random.seed(42)

    algorithms = ['Best Fit', 'Worst Fit', 'Next Fit']
    results = {}

    for algo_name in algorithms:
        print(f"\n{'=' * 50}")
        print(f"ALGORITHM: {algo_name}")
        print('=' * 50)

        # Create allocator
        allocator = MemoryAllocator(100)

        # Step 1: 12 random allocations
        print("\n1. Making 12 random allocations (size 3-12):")
        allocated_blocks = []  # List of (block_id, size)

        for i in range(12):
            size = random.randint(3, 12)
            block_id = allocator.block_id_counter

            if algo_name == 'Best Fit':
                result = allocator.allocate_best_fit(size, block_id)
            elif algo_name == 'Worst Fit':
                result = allocator.allocate_worst_fit(size, block_id)
            else:  # Next Fit
                result = allocator.allocate_next_fit(size, block_id)

            if result is not None:
                allocated_blocks.append((block_id, size))
                # print(f"   Allocated {size} units at address {result}")

        allocator.print_free_list("After 12 allocs")

        # Step 2: Free 4 random blocks
        print("\n2. Freeing 4 random blocks:")
        if len(allocated_blocks) >= 4:
            to_free = random.sample(allocated_blocks, 4)
            freed_info = []
            for block_id, size in to_free:
                allocator.free_block(block_id)
                freed_info.append(f"size {size}")
                # Remove from allocated_blocks
                allocated_blocks = [b for b in allocated_blocks if b[0] != block_id]
            print(f"   Freed blocks: {', '.join(freed_info)}")

        allocator.print_free_list("After freeing 4")

        # Step 3: Attempt to allocate 25 units
        print("\n3. Attempting to allocate size 25:")
        if algo_name == 'Best Fit':
            result = allocator.allocate_best_fit(25)
        elif algo_name == 'Worst Fit':
            result = allocator.allocate_worst_fit(25)
        else:  # Next Fit
            result = allocator.allocate_next_fit(25)

        # Analyze free list
        current = allocator.free_list
        total_free = 0
        largest_block = 0
        block_count = 0

        while current:
            total_free += current.size
            if current.size > largest_block:
                largest_block = current.size
            block_count += 1
            current = current.next

        if result is not None:
            print(f"   ✅ SUCCESS: Allocated at address {result}")
            success = True
            # Free it for consistency
            allocator.free_block(allocator.block_id_counter - 1)
        else:
            print(f"   ❌ FAILED: Cannot allocate 25 units")
            success = False

        print(f"\n   Free List Analysis:")
        print(f"   • {block_count} free blocks")
        print(f"   • Total free memory: {total_free} units")
        print(f"   • Largest contiguous block: {largest_block} units")

        if success:
            print(f"   • Reason: Found block ≥ 25 units ({largest_block})")
        else:
            print(f"   • Reason: External fragmentation")
            print(f"     (Total free: {total_free}, but no block ≥ 25)")

        results[algo_name] = {
            'success': success,
            'total_free': total_free,
            'largest_block': largest_block,
            'block_count': block_count
        }

    print("\n" + "=" * 70)
    print("EXPERIMENT 2 SUMMARY")
    print("=" * 70)

    print("\nRESULTS:")
    for algo_name, result in results.items():
        status = "SUCCESS" if result['success'] else "FAILED"
        print(f"  {algo_name:10s}: {status}")
        print(f"    Free blocks: {result['block_count']}, Largest: {result['largest_block']}")

    print("\nKEY INSIGHTS:")
    print("1. BEST FIT often fails due to external fragmentation")
    print("   • Creates many small blocks")
    print("   • Even with enough total free memory, no single block is large enough")

    print("\n2. WORST FIT usually succeeds")
    print("   • Preserves larger free blocks")
    print("   • Better chance of having a block ≥ 25 units")

    print("\n3. NEXT FIT results vary")
    print("   • Depends on roving pointer position")
    print("   • May succeed or fail based on fragmentation distribution")


# ==================== EXPERIMENT 3: SPEED TEST ====================
def experiment3_speed_test():
    """Run Experiment 3: Performance comparison"""
    print("\n" + "=" * 70)
    print("EXPERIMENT 3: Speed Test")
    print("=" * 70)
    print("\nProcedure:")
    print("1. Repeat 200 times:")
    print("   a. Allocate random block (size 1-10)")
    print("   b. Free one previously allocated block")
    print("2. Measure total execution time")
    print("\n" + "-" * 70)

    random.seed(123)  # For reproducible results
    algorithms = ['Best Fit', 'Worst Fit', 'Next Fit']
    results = {}

    for algo_name in algorithms:
        print(f"\nTesting {algo_name}...")

        allocator = MemoryAllocator(100)
        allocated_blocks = []  # Track block IDs for freeing
        allocations_made = 0
        frees_made = 0

        # Start timing
        start_time = time.perf_counter()

        for i in range(200):
            # Allocate random block
            size = random.randint(1, 10)
            block_id = allocator.block_id_counter

            if algo_name == 'Best Fit':
                result = allocator.allocate_best_fit(size, block_id)
            elif algo_name == 'Worst Fit':
                result = allocator.allocate_worst_fit(size, block_id)
            else:  # Next Fit
                result = allocator.allocate_next_fit(size, block_id)

            if result is not None:
                allocated_blocks.append(block_id)
                allocations_made += 1

            # Free a block if we have any allocated (with 50% probability)
            if allocated_blocks and random.random() < 0.5:
                block_to_free = random.choice(allocated_blocks)
                allocator.free_block(block_to_free)
                allocated_blocks.remove(block_to_free)
                frees_made += 1

        # End timing
        end_time = time.perf_counter()
        elapsed = end_time - start_time

        results[algo_name] = {
            'time': elapsed,
            'allocations': allocations_made,
            'frees': frees_made
        }

        print(f"  Time: {elapsed:.6f} seconds")
        print(f"  Allocations: {allocations_made}, Frees: {frees_made}")
        print(f"  Operations/sec: {(allocations_made + frees_made) / elapsed:.0f}")

    print("\n" + "=" * 70)
    print("SPEED TEST RESULTS")
    print("=" * 70)

    # Find fastest and slowest
    fastest_algo = min(results, key=lambda x: results[x]['time'])
    slowest_algo = max(results, key=lambda x: results[x]['time'])

    print("\nRanking (fastest to slowest):")
    for algo_name in sorted(results, key=lambda x: results[x]['time']):
        time_val = results[algo_name]['time']
        ops_sec = (results[algo_name]['allocations'] + results[algo_name]['frees']) / time_val
        marker = ""
        if algo_name == fastest_algo:
            marker = " (FASTEST)"
        elif algo_name == slowest_algo:
            marker = " (SLOWEST)"
        print(f"  {algo_name:10s}: {time_val:.6f}s ({ops_sec:.0f} ops/sec){marker}")

    print("\n" + "=" * 70)
    print("PERFORMANCE ANALYSIS")
    print("=" * 70)

    print("\n1. WHY NEXT FIT IS FASTEST:")
    print("   • Search Strategy: Starts from last position, not beginning")
    print("   • Average Search Length: ~50% of free list")
    print("   • Time Complexity: O(n/2) on average")

    print("\n2. WHY BEST FIT IS SLOWEST:")
    print("   • Search Strategy: Must scan ENTIRE free list every time")
    print("   • Comparison: Compares all blocks to find smallest fit")
    print("   • Time Complexity: O(n) always")

    print("\n3. WORST FIT PERFORMANCE:")
    print("   • Similar to Best Fit: Scans entire list")
    print("   • Slightly faster: Can stop early in some implementations")
    print("   • Still O(n) complexity")

    print("\nSCANNING ANALYSIS PER ALLOCATION:")
    print("  Algorithm    | Avg. Nodes Scanned | Complexity")
    print("  ------------ | ------------------ | ----------")
    print("  Next Fit     | ~n/2               | O(n/2)")
    print("  Best Fit     | n                  | O(n)")
    print("  Worst Fit    | n                  | O(n)")

    print("\nCONCLUSION:")
    print("• For speed: Next Fit is the clear winner")
    print("• For fragmentation: Worst Fit performs better")
    print("• For space utilization: Best Fit is most efficient")
    print("• Real systems often use hybrid approaches")


# ==================== MAIN PROGRAM ====================
def main():
    """Main program with menu interface"""
    print("=" * 80)
    print("MEMORY ALLOCATION ALGORITHMS - COMPLETE IMPLEMENTATION")
    print("Question 3: Best Fit, Worst Fit, and Next Fit Comparison")
    print("=" * 80)

    while True:
        print("\n" + "=" * 80)
        print("MAIN MENU")
        print("=" * 80)
        print("\nSelect an option:")
        print("1. Run Experiment 1: Allocation Trace")
        print("2. Run Experiment 2: Fragmentation Test")
        print("3. Run Experiment 3: Speed Test")
        print("4. Run ALL Experiments")
        print("5. Exit Program")
        print("\n" + "-" * 80)

        choice = input("Enter your choice (1-5): ").strip()

        if choice == "1":
            experiment1_allocation_trace()
            input("\nPress Enter to continue...")

        elif choice == "2":
            experiment2_fragmentation_test()
            input("\nPress Enter to continue...")

        elif choice == "3":
            experiment3_speed_test()
            input("\nPress Enter to continue...")

        elif choice == "4":
            print("\nRunning all experiments sequentially...\n")
            experiment1_allocation_trace()
            print("\n" + "=" * 80)
            experiment2_fragmentation_test()
            print("\n" + "=" * 80)
            experiment3_speed_test()
            print("\n" + "=" * 80)
            print("ALL EXPERIMENTS COMPLETED!")
            input("\nPress Enter to return to menu...")

        elif choice == "5":
            print("\nThank you for using the Memory Allocator Simulator!")
            print("Goodbye!")
            break

        else:
            print("Invalid choice. Please enter 1-5.")


# ==================== PROGRAM ENTRY POINT ====================
if __name__ == "__main__":
    # Run the main program
    main()