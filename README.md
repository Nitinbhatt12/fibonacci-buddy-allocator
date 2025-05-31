# Fibonacci Buddy Memory Allocator

A memory allocator implementation using the Fibonacci buddy system, which uses Fibonacci numbers for block sizes instead of traditional power-of-two sizes.

## Features

- Implements Fibonacci buddy allocation algorithm
- Efficient splitting and merging of memory blocks
- Reduced internal fragmentation compared to binary buddy systems
- Free list management with address-ordered coalescing
- Validation and debugging utilities

## How It Works

The allocator:
1. Initializes a table of Fibonacci numbers up to the heap size
2. Manages memory blocks using Fibonacci-sized chunks (Fₙ = Fₙ₋₁ + Fₙ₋₂)
3. Splits blocks according to Fibonacci sequence when allocating
4. Merges adjacent Fibonacci buddies when freeing memory

## Building and Running

```bash
gcc fiboheap.c -o fiboheap
./fiboheap
