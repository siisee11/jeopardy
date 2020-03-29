# Radix Tree in userspace

This repository contain radix tree implementation can be used in userspace.

It is totally based on linux kernel radix tree and most of the code is identical except tags.


## Files
  - main.c : main function only. 
  - benchmark.c : benchmark code using item interface.
  - test.c : manupulating items using radix tree interface.
  - test.h : contains struct item.
  - radix-tree.c : impletation of radix tree.
  - list.c/h : list data structure implementation.
  - radix-tree.h : contains structure, variables, and mecros about radix tree.
  - error.h : defines error.
  - others : original kernel implemantation.


## TODO
  - [x] ~~Basic operation~~
  - [x] ~~Userspace RCU~~
  - [x] ~~Benchmark~~
  - [x] tag
