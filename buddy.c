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

block_info *remove_first_block(int size)
{
    buddy_info *buddy = (buddy_info *)kernel_space;
    block_info *ret;
    ret = buddy->blocks[size].next;
    buddy->blocks[size].next = buddy->blocks[size].next->next;
    return ret;
}
int remove_block(block_info *block)
{
    buddy_info *buddy = (buddy_info *)kernel_space;
    block_info *prev = &(buddy->blocks[block->block_size]);
    block_info *tmp = buddy->blocks[block->block_size].next;
    while (tmp)
    {
        if (tmp == block)
        {
            prev->next = tmp->next;
            tmp->next = NULL;
            return 1; // block found and removed!
        }
        tmp = tmp->next;
    }
    return 0; //block not found!
}

block_info *find_buddy(block_info *start_adress) // TODO: CHANGE TO x64!!! int to long int!
{
    if (start_adress == NULL)
    {
        printf("Error! Finding buddy for nullptr! \n");
        return NULL;
    }
    int mask = 0x1 << ((start_adress->block_size) + pow_of_two(BLOCK_SIZE));
    return (block_info *)((long long int)start_adress ^ mask);
}

void split(char *seg, int upper, int lower) // upper and lower are pows of 2, lower is limit for splitting
{
    while (--upper >= lower)
        add_block(seg + (1 << upper) * BLOCK_SIZE, upper);
}

block_info *merge(block_info *block)
{
    int size = block->block_size;
    block_info *buddy_block = find_buddy(block);
    block_info *first = ((long long)block > (long long)buddy_block) ? buddy_block : block;
    if (size == buddy_block->block_size)
    {
        if (remove_block(buddy_block))
        {
            remove_block(block);
            add_block(first, size + 1);
        }
    }
    return NULL;
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
    printf("Buddy info size : %u B \n", sizeof(buddy_info));
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

    buddy->buddy_start_adress = (char*)space + BLOCK_SIZE;

    for (int i = 0; i < MAX_POW_OF_TWO_BLOCKS; i++)
    {
        buddy->blocks[i].next = NULL;
        if (i == max_pow)
        {
            buddy->blocks[i].next = buddy->buddy_start_adress;
            buddy->blocks[i].next->next = NULL;
            buddy->blocks[i].next->block_size = i;
        }
    }
    //adding unused blocks that have no buddies
    char *unused_space_start = (char *)((char*)buddy->buddy_start_adress + buddy->max_buddy_block_size * BLOCK_SIZE);
    while (buddy->unused_space)
    {
        buddy->blocks[pow_of_two(less_or_equal_pow_of_two(buddy->unused_space))].next = (block_info *)unused_space_start;
        ((block_info *)unused_space_start)->next = NULL;
        ((block_info *)unused_space_start)->block_size = pow_of_two(less_or_equal_pow_of_two(buddy->unused_space));
        unused_space_start += less_or_equal_pow_of_two(buddy->unused_space) * BLOCK_SIZE;
        buddy->unused_space -= less_or_equal_pow_of_two(buddy->unused_space);
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
    for (unsigned i = actual_size; i <= pow_of_two(buddy->max_buddy_block_size); i++)
    {
        if (buddy->blocks[i].next != NULL)
        {
            split((char *)(buddy->blocks[i].next), i, actual_size);
            ret = buddy->blocks[i].next;
            ret->next = NULL;
            ret->block_size = actual_size;
            remove_first_block(i);
            break;
        }
    }
    if (ret == NULL)
    {
        printf("Not enough free space! \n");
        return NULL;
    }
    return (char*)ret + sizeof(block_info);
}

void buddy_free(void *block)
{
    block_info *block_start = (((block_info *)block) - 1);
    add_block(block_start, block_start->block_size);
    //
    block_info *tmp = merge(block_start);
    while (tmp)
    {
        tmp = merge(block_start);
    }
}
_Bool is_power_of_two(unsigned x)
{
    return (x != 0) && ((x & (x - 1)) == 0);
}
