# Design Document
## Memory Management Simulator

---

## 1. Purpose of This Document

This document explains the **internal architecture and design decisions**
of the Memory Management Simulator. It focuses on *how the system works*
internally rather than how it is used.

---

## 2. High-Level System Architecture

The simulator models the memory subsystem of an operating system using
modular components connected in a fixed execution pipeline.

### Overall Flow Diagram
```
+-------------------+
| Virtual Address   |
+-------------------+
          |
          v
+-------------------+
| Page Table        |
| (Per Process)     |
+-------------------+
          |
          v
+-------------------+
| Physical Address  |
+-------------------+
          |
          v
+-------------------+
| Cache Hierarchy   |
| L1 → L2 → L3      |
+-------------------+
          |
          v
+-------------------+
| Memory Allocator  |
| Physical / Buddy  |
+-------------------+
```
---

## 3. Memory Layout and Assumptions

### Physical Memory Layout

Physical memory is modeled as a contiguous address space.
```
0x00000000
+----------------------------------------------------+
|                                                    |
|              Simulated Physical Memory             |
|                                                    |
+----------------------------------------------------+
0x00FFFFFF
```
- Addresses are byte-addressable
- Address 0 is the base of memory
- Total size is configurable

---

## 4. Physical Memory Allocation Design

### Block Representation

Memory is divided into variable-sized blocks.
```
+-----------+-----------+-----------+
| Block A   | Block B   | Block C   |
| ALLOCATED | FREE      | ALLOCATED |
+-----------+-----------+-----------+
```
Each block contains:
- Start address
- Size
- Allocation status
- Process ID
- Block ID

---

### Allocation and Splitting

When a free block is larger than requested:
```
+-----------------------------+
|         FREE BLOCK          |
+-----------------------------+
```
After allocation:
```
+-----------+-----------------+
| ALLOCATED |      FREE       |
+-----------+-----------------+
```
---

### Deallocation and Coalescing

Adjacent free blocks are merged.
```
Before:
+-----------+-----------+
| FREE      | FREE      |
+-----------+-----------+

After:
+-----------------------+
|        FREE           |
+-----------------------+
```
This reduces external fragmentation.

---

## 5. Buddy Allocation System

### Buddy Memory Model

Memory size is a power of two.

Example (1024 bytes):

Order 10: 1024
Order 9 : 512 | 512
Order 8 : 256 | 256 | 256 | 256

---

### Buddy Splitting
```
Initial:
+-------------------------------+
|            1024               |
+-------------------------------+

Split:
+---------------+---------------+
|      512      |      512      |
+---------------+---------------+
```
Further split until required size is reached.

---

### Buddy Coalescing

Buddy addresses are computed using XOR.
```
If both buddies are free:

+---------------+---------------+
|      FREE     |      FREE     |
+---------------+---------------+

They merge into:

+-------------------------------+
|            FREE               |
+-------------------------------+
```
---

### Internal Fragmentation

Internal fragmentation occurs when:

Requested size < Allocated block size

Example:
- Requested: 100 bytes
- Allocated: 128 bytes
- Internal fragmentation: 28 bytes

---

## 6. Allocation Mode System

The simulator supports dynamic allocator selection.

Modes:

AUTO:
- Power-of-two request → Buddy allocator
- Otherwise → Physical allocator

PHYSICAL:
- Always use physical allocator

BUDDY:
- Always use buddy allocator

FORCED:
- Used internally when switching allocation strategies

---

## 7. Virtual Memory System

### Virtual Address Structure
```
Virtual Address
+-------------------+----------------+
| Virtual Page No   | Offset         |
+-------------------+----------------+
```
---

### Page Table Structure

Each process has its own page table.

Virtual Page → Page Table Entry
```
+-------------+-----------+-----------+
| Present Bit | Frame No  | Metadata  |
+-------------+-----------+-----------+
```
---

### Address Translation Flow
```
Virtual Address
      |
      v
Page Number + Offset
      |
      v
Page Table Lookup
      |
      v
Frame Number
      |
      v
Physical Address
```
---

### Page Fault Handling
```
On page fault:

+------------------+
| Page Fault       |
+------------------+
          |
          v
+------------------+
| Select Victim    |
| (FIFO / LRU /   |
| CLOCK)           |
+------------------+
          |
          v
+------------------+
| Evict Page       |
+------------------+
          |
          v
+------------------+
| Load New Page    |
+------------------+
```
---

## 8. Cache Hierarchy Design

### Cache Levels
```
+---------+      +---------+      +---------+
|  L1     | ---> |  L2     | ---> |  L3     |
+---------+      +---------+      +---------+
      |                                 |
      +------------- MISS --------------+
                    |
                    v
              Main Memory
```
---

### Cache Line Model

Each cache contains sets of cache lines.
```
+-----------+-----------+-----------+
| Tag       | Data      | Metadata  |
+-----------+-----------+-----------+
```
Replacement policies determine eviction.

---

### Cache Access Flow

1. Check L1
2. On miss → L2
3. On miss → L3
4. On miss → Main Memory
5. Data is propagated upward

---

## 9. Statistics and Observability

The simulator records:

- Allocation requests, successes, failures
- External fragmentation (Physical allocator)
- Internal fragmentation (Buddy allocator)
- Memory utilization
- Page faults and replacements
- Cache hits and misses per level
- Average Memory Access Time (AMAT)

These statistics are exposed via the CLI.

---

## 10. Limitations and Simplifications

- Single-threaded execution
- No TLB simulation
- No real disk I/O
- No NUMA support
- Fixed page size per run

These choices simplify implementation while preserving correctness.

---

## 11. Summary

This simulator provides a structured and modular simulation of operating
system memory management. The inclusion of allocation strategies, buddy
allocation, paging-based virtual memory, and multi-level caching allows
realistic modeling of OS behavior and performance trade-offs.

