# Fibonacci Buddy Allocation Algorithm

## Overview

This implementation uses the Fibonacci sequence (1, 2, 3, 5, 8, 13...) for memory block sizes instead of traditional power-of-two sizes. This reduces internal fragmentation for common allocation sizes.

## Key Operations

### Splitting
When allocating memory:
1. Find the smallest Fibonacci block ≥ requested size
2. If no exact match, split larger blocks recursively:
   - Fₙ → Fₙ₋₁ (left buddy) + Fₙ₋₂ (right buddy)

### Merging
When freeing memory:
1. Check if adjacent blocks are Fibonacci buddies
2. If so, merge them:
   - Fₙ₋₁ + Fₙ₋₂ → Fₙ

### Free List Management
- Maintained in address order for efficient coalescing
- Uses doubly-linked list for O(1) removals

## Advantages
- More flexible block sizes than binary buddy system
- Better fit for common allocation patterns
- Reduced internal fragmentation

## Complexity
- Allocation: O(n) where n is free list length
- Free: O(1) plus merge operations
