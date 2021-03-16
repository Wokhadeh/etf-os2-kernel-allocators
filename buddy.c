#include "buddy.h"

void add_block(void *start_adress, int size) // size is pow of 2 of numbers of blocks in segment
{
    if (size < 0)
    {
        printf("Error! Requested negative size! (adding block) \n");
        exit(-1);
    }
    buddy_info *buddy = (buddy_info *)kernel_space;
    block_info *block = (block_info *)start_adress;
    block->block_size = size;
    block->next = buddy->blocks[size].next;
    buddy->blocks[size].next = start_adress;
}

block_info *remove_block(int size)
{
    buddy_info *buddy = (buddy_info *)kernel_space;
    block_info *ret;
    ret = buddy->blocks[size].next;
    buddy->blocks[size].next = buddy->blocks[size].next->next;
    return ret;
}

void split(char *seg, int upper, int lower) // upper and lower are pows of 2, lower is limit for splitting
{
    while (--upper >= lower)
        add_block(seg + (1 << upper) * BLOCK_SIZE, upper);
}

unsigned less_or_equal_pow_of_two(unsigned num)
{
    // invalid input
    if (num < 1)
        return 0;

    unsigned res = 1;

    // try all powers starting from 2^1
    for (int i = 0; i < 8 * sizeof(unsigned); i++)
    {
        unsigned curr = 1 << i;

        // if current power is more than num, break
        if (curr > num)
            break;

        res = curr;
    }

    return res;
}

unsigned greater_or_equal_pow_of_two(unsigned num)
{
    // works for 32bit unisgned or intw
    --num;
    num |= num >> 1;
    num |= num >> 2;
    num |= num >> 4;
    num |= num >> 8;
    num |= num >> 16;
    return num + 1;
}
unsigned pow_of_two(unsigned num)
{
    // only for numbers that are powers of two
    int i = 0;
    while (num)
    {
        i++;
        num >>= 1;
    }
    return i - 1;
}

void print_buddy()
{
    buddy_info *buddy = (buddy_info *)kernel_space;
    printf("Buddy info size : %d B \n", sizeof(buddy_info));
    printf("Kernel space start adress : %p \n", kernel_space);
    printf("Buddy start adress : %p \n", buddy->buddy_start_adress);
    for (int i = 0; i < MAX_POW_OF_TWO_BLOCKS; i++)
    {
        block_info *tmp = buddy->blocks[i].next;
        while (tmp)
        {
            printf("Free block of size 2^%d : %p \n", i, tmp);
            tmp = tmp->next;
        }
    }
    printf("Buddy unused space : %d blocks \n", buddy->unused_space);
    printf("Buddy size: %d blocks \n", buddy->max_buddy_block_size);
}

void buddy_init(void *space, int block_num)
{
    // setting global variables
    kernel_space = space;
    kernel_space_size = block_num;

    // initialaizing buddy allocator
    buddy_info *buddy = (buddy_info *)space;
    buddy->max_buddy_block_size = less_or_equal_pow_of_two(block_num - 1);
    buddy->unused_space = (block_num - 1) - buddy->max_buddy_block_size;
    unsigned max_pow = pow_of_two(buddy->max_buddy_block_size);

    if (max_pow > 20)
    {
        printf("Initialization error! Number of blocks is over the limit! \n");
        exit(-1);
    }

    buddy->buddy_start_adress = space + sizeof(buddy_info);

    for (int i = 0; i < MAX_POW_OF_TWO_BLOCKS; i++)
    {
        buddy->blocks[i].next = NULL;
        if (i == max_pow){
            buddy->blocks[i].next = buddy->buddy_start_adress;
            buddy->blocks[i].next->next=NULL;
            buddy->blocks[i].next->block_size=i;
        }
    }
    //adding unused blocks that have no buddies
    char* unused_space_start = (char*)(buddy->buddy_start_adress+buddy->max_buddy_block_size*BLOCK_SIZE); 
    while(buddy->unused_space){
        buddy->blocks[pow_of_two(less_or_equal_pow_of_two(buddy->unused_space))].next=(block_info*)unused_space_start;
        ((block_info*)unused_space_start)->next=NULL;
        ((block_info*)unused_space_start)->block_size=pow_of_two(less_or_equal_pow_of_two(buddy->unused_space));
        unused_space_start+=less_or_equal_pow_of_two(buddy->unused_space)*BLOCK_SIZE;
        buddy->unused_space-=less_or_equal_pow_of_two(buddy->unused_space);
    }

    //buddy->blocks[pow_of_two(buddy->max_buddy_block_size)]->next=
}

void *buddy_alloc(int size)
{
    if (size < 0)
    {
        printf("Error! Requested negative size! \n");
        exit(-1);
    }
    unsigned actual_size = ((size + sizeof(buddy_info)) % BLOCK_SIZE) == 0 ? ((size + sizeof(buddy_info)) / BLOCK_SIZE)
                                                                           : ((size + sizeof(buddy_info)) / BLOCK_SIZE + 1); // adding size of block header to requested size and converting to num of blocks
    actual_size = pow_of_two(greater_or_equal_pow_of_two(actual_size));                                                      //converting to pow of 2 blocks
    //printf("Actual size 2^%d \n", actual_size);
    buddy_info *buddy = (buddy_info *)kernel_space;
    block_info *ret = NULL;
    for (int i = actual_size; i <= pow_of_two(buddy->max_buddy_block_size); i++)
    {
        if (buddy->blocks[i].next != NULL)
        {
            split((char*)(buddy->blocks[i].next), i, actual_size);
            ret = buddy->blocks[i].next;
            ret->next=NULL;
            ret->block_size=actual_size;
            remove_block(i);
            break;
        }
    }
    if (ret == NULL)
    {
        printf("Not enough free space! \n");
        return NULL;
    }
    return ret + sizeof(block_info);
}

