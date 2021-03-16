#pragma once
#include "slab.h"
#include "stdio.h"
#define MAX_POW_OF_TWO_BLOCKS 20 // maximum space managed by buddy is 2^12 * 2^20 B= 4GB

void* kernel_space; //pointer to given kernel space
int kernel_space_size; // kernel space size in blocks

typedef struct block_info{
    struct block_info* next;
    unsigned block_size;
} block_info;

typedef struct buddy_info
{
    block_info blocks[MAX_POW_OF_TWO_BLOCKS];
    void* buddy_start_adress;
    int unused_space;
    int max_buddy_block_size;

} buddy_info;


void buddy_init(void *space, int block_num);
void* buddy_alloc(int size);
void buddy_free(void* block);

block_info* remove_block(int size);
void add_block(void *start_adress, int size);
void split(char *seg, int upper, int lower);

// helper functions (powers of two,arithmetics)
unsigned less_or_equal_pow_of_two(unsigned num);
unsigned greater_or_equal_pow_of_two(unsigned num);
unsigned pow_of_two(unsigned num);

void print_buddy();
