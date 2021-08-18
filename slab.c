#include "slab.h"
#include "buddy.h"

void kmem_init(void *space, int block_num)
{
    buddy_init(space, block_num);
    slab_info *slab_info = get_slab_info();
    slab_info->max_number_of_caches = (PAGE_RESERVED_CACHES * BLOCK_SIZE) / sizeof(kmem_cache_t); //calculate how many caches can fit in remaining free space in first page
    slab_info->number_of_caches = 0;                                                              //current numbe of caches is 0
    slab_info->arrayOfCaches = buddy_alloc(PAGE_RESERVED_CACHES * BLOCK_SIZE);                    // set start address od array of caches (7 pages reserved for caches)
    //printf("Maksimalni broj keseva %d",slab_info->max_number_of_caches);
}

slab_info *get_slab_info()
{
    return (slab_info *)(kernel_space + sizeof(buddy_info));
}
kmem_cache_t *kmem_cache_create(const char *name, size_t size,
                                void (*ctor)(void *),
                                void (*dtor)(void *))
{
    if (size == 0) //check argument
        return NULL;
    slab_info *slab_info = get_slab_info();
    if (slab_info->number_of_caches == slab_info->max_number_of_caches) //check if there is free cache slot
        return NULL;                                                    //cant allocate new cache,should expand cache space

    // add cache
    strcpy(slab_info->arrayOfCaches[slab_info->number_of_caches].name, name); //copy cache name
    slab_info->arrayOfCaches[slab_info->number_of_caches].object_size = size;
    //constructor and destructor
    slab_info->arrayOfCaches[slab_info->number_of_caches].constructor = ctor;
    slab_info->arrayOfCaches[slab_info->number_of_caches].destructor = dtor;
    //calculate slab size
    unsigned num_of_pages = 1;
    unsigned num_of_objects = (BLOCK_SIZE - sizeof(slab)) / (size + sizeof(void *)); //slab metadata + free_slots + void* (pointer to next slot)
    while (num_of_objects < 1)
    {
        num_of_pages <<= 1;
        num_of_objects = ((num_of_pages * BLOCK_SIZE) - sizeof(slab)) / (size + sizeof(void *));
    }
    // set num of objects,slab_size,unused_space
    slab_info->arrayOfCaches[slab_info->number_of_caches].num_of_objects = num_of_objects;
    slab_info->arrayOfCaches[slab_info->number_of_caches].slab_size = num_of_pages;
    slab_info->arrayOfCaches[slab_info->number_of_caches].unused_space = (num_of_pages * BLOCK_SIZE) - sizeof(slab) - (num_of_objects * (size + sizeof(void *)));
    //allocate slab
    slab_info->arrayOfCaches[slab_info->number_of_caches].slabs=NULL;
    
    //increment number of caches
    slab_info->number_of_caches++;
}