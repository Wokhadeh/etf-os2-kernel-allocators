#include "slab.h"
#include "buddy.h"
#include  <string.h>
void kmem_init(void *space, int block_num)
{
    //initialize buddy
    buddy_init(space, block_num);
    //initialize slab
    slab_info *slab_info = get_slab_info();
    slab_info->max_number_of_caches = (PAGE_RESERVED_CACHES * BLOCK_SIZE) / sizeof(kmem_cache_t); //calculate how many caches can fit in remaining free space in first page
    slab_info->number_of_caches = 0;                                                              //current numbe of caches is 0
    slab_info->array_of_caches = buddy_alloc(PAGE_RESERVED_CACHES * BLOCK_SIZE);                    // set start address od array of caches (7 pages reserved for caches)
    //initialize caches for small memory buffers 
    for (int i = 0; i < NUMBER_OF_SMALL_MEMORY_BUFFERS; i++) {
        // add name SIZE-N
        strcpy_s(slab_info->small_memory_buffers[i].name,30,"Small memory buffer");
        // no constructor or destructor
        slab_info->small_memory_buffers[i].constructor = NULL;
        slab_info->small_memory_buffers[i].destructor = NULL;
        // object size
        slab_info->small_memory_buffers[i].object_size = 1 << (i + 5);
        //calculate slab size
        unsigned num_of_pages = 1;
        unsigned num_of_objects = (BLOCK_SIZE - sizeof(slab)) / (slab_info->small_memory_buffers[i].object_size + sizeof(void*)); //slab metadata + free_slots + void* (pointer to next slot)
        while (num_of_objects < 1)
        {
            num_of_pages <<= 1;
            num_of_objects = ((num_of_pages * BLOCK_SIZE) - sizeof(slab)) / (slab_info->small_memory_buffers[i].object_size + sizeof(void*));
        }

        // set num of objects,slab_size,unused_space,alignment,error code
        slab_info->small_memory_buffers[i].num_of_objects = num_of_objects;
        slab_info->small_memory_buffers[i].slab_size = num_of_pages;
        slab_info->small_memory_buffers[i].unused_space = (num_of_pages * BLOCK_SIZE) - sizeof(slab) - (num_of_objects * (slab_info->small_memory_buffers[i].object_size + sizeof(void*)));
        slab_info->small_memory_buffers[i].alignment = 0;
        slab_info->small_memory_buffers[i].error_code = 0;
        //allocate slab
        slab_info->small_memory_buffers[i].slabs[EMPTY] = NULL;
        slab_info->small_memory_buffers[i].slabs[PARTIALLY_FULL] = NULL;
        slab_info->small_memory_buffers[i].slabs[FULL] = NULL;
        allocate_slab(&(slab_info->small_memory_buffers[i]));
        //kmem_cache_info(&(slab_info->small_memory_buffers[i]));
    }

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
    strcpy_s(slab_info->array_of_caches[slab_info->number_of_caches].name,30, name); //copy cache name
    slab_info->array_of_caches[slab_info->number_of_caches].object_size = size;
    //constructor and destructor
    slab_info->array_of_caches[slab_info->number_of_caches].constructor = ctor;
    slab_info->array_of_caches[slab_info->number_of_caches].destructor = dtor;
    //calculate slab size
    unsigned num_of_pages = 1;
    unsigned num_of_objects = (BLOCK_SIZE - sizeof(slab)) / (size + sizeof(void *)); //slab metadata + free_slots + void* (pointer to next slot)
    while (num_of_objects < 1)
    {
        num_of_pages <<= 1;
        num_of_objects = ((num_of_pages * BLOCK_SIZE) - sizeof(slab)) / (size + sizeof(void *));
    }
    // set num of objects,slab_size,unused_space,alignment,error code
    slab_info->array_of_caches[slab_info->number_of_caches].num_of_objects = num_of_objects;
    slab_info->array_of_caches[slab_info->number_of_caches].slab_size = num_of_pages;
    slab_info->array_of_caches[slab_info->number_of_caches].unused_space = (num_of_pages * BLOCK_SIZE) - sizeof(slab) - (num_of_objects * (size + sizeof(void *)));
    slab_info->array_of_caches[slab_info->number_of_caches].alignment = 0;
    slab_info->array_of_caches[slab_info->number_of_caches].error_code = 0;
    //allocate slab
    slab_info->array_of_caches[slab_info->number_of_caches].slabs[EMPTY]=NULL;
    slab_info->array_of_caches[slab_info->number_of_caches].slabs[PARTIALLY_FULL] = NULL;
    slab_info->array_of_caches[slab_info->number_of_caches].slabs[FULL] = NULL;
    allocate_slab(&(slab_info->array_of_caches[slab_info->number_of_caches]));
    
    //increment number of caches
    slab_info->number_of_caches++;

    return &slab_info->array_of_caches[slab_info->number_of_caches-1];
}
void* kmem_cache_alloc(kmem_cache_t* cachep) {
    if (cachep == NULL) {
        return NULL; // error wrong arguments
    }
    //check if cache has partially full,empty slabs or should allocate new slab 
    slab* slab = cachep->slabs[PARTIALLY_FULL];
    int slab_list_type = PARTIALLY_FULL;
    if (slab == NULL) {
       slab = cachep->slabs[EMPTY];
       slab_list_type = EMPTY;
       if (slab == NULL) {
           allocate_slab(cachep);
           slab = cachep->slabs[EMPTY];
           if (slab == NULL)
               return NULL; // error, couldnt create slab
       }
    }
    
    void* retAllocatedObject = (char*)slab->free_slots + sizeof(void*); // determine object start address
    slab->free_slots = (*((void**)slab->free_slots)); //move free list head to next free slot
    slab->num_of_free_objects--; // decrement number of free slots
    if (slab->num_of_free_objects == 0) { // move slab to full list
        move_slab(slab, slab_list_type, FULL);
    }
    else if (slab_list_type == EMPTY) { // move slab to partially full list
        move_slab(slab, slab_list_type, PARTIALLY_FULL);
    }

    return retAllocatedObject;
}

void kmem_cache_free(kmem_cache_t* cachep, void* objp) {
    if (cachep == NULL || objp == NULL) {
        return; // error wrong args
    }
    slab* slab = find_objs_slab(cachep, objp);
    if (slab == NULL) {
        // error,cannot find slab
        return; 
    }
    // call destructor and constructor
    if (cachep->destructor) {
        cachep->destructor(NULL);
    }
    if (cachep->constructor) {
        cachep->constructor(NULL);
    }
    // add object to free slots list
    *((void**)objp - 1) = slab->free_slots;
    slab->free_slots = (void**)objp - 1;

    // increment number of free slots and move slab if needed
    slab->num_of_free_objects++;
    if (slab->num_of_free_objects == 1) {
        move_slab(slab,FULL,PARTIALLY_FULL);
    }
    else if(slab->num_of_free_objects == slab->my_cache->num_of_objects)
        move_slab(slab,PARTIALLY_FULL,EMPTY);
}
void* kmalloc(size_t size) {
    if (is_power_of_two(size)) {
        unsigned pow = pow_of_two(size);
        if (pow < 5 || pow > 17) {
            // error,cannot allocate memory buffer of requested size
            return NULL;
        }
        else {
            return kmem_cache_alloc(&get_slab_info()->small_memory_buffers[pow - 5]);
        }
        
    }
    else {
        //error argument is not power of two
        return NULL;
    }
}
void kfree(const void* objp) {
    for (int i = 0; i < NUMBER_OF_SMALL_MEMORY_BUFFERS; i++) {
        kmem_cache_free(&get_slab_info()->small_memory_buffers[i], objp);
    }
}
void kmem_cache_info(kmem_cache_t* cachep) {
    //calculate number of slabs and number of allocated objects
    int num_of_slabs = 0;
    int num_of_allocated_objects = 0;
    for (int i = 0; i < 3; i++) {
        slab* temp = cachep->slabs[i];
        while (temp) {
            num_of_slabs++;
            num_of_allocated_objects += (cachep->num_of_objects - temp->num_of_free_objects);
            temp = temp->next_slab;
        }
    }
    // (used slots / all slots) * 100%
    double used_space = ((double)num_of_allocated_objects / ((double)num_of_slabs * cachep->num_of_objects)) * 100;
    int cache_size = num_of_slabs * cachep->slab_size;
    printf("Cache name : %s \n Object size : %d B \n Cache size : %d page(s) \n Number of objects in one slab : %d \n Used space : %.2f %% \n", 
        cachep->name, cachep->object_size, cache_size, cachep->num_of_objects, used_space);
}

slab_info* get_slab_info()
{
    return (slab_info*)((char*)kernel_space + sizeof(buddy_info));
}
void allocate_slab(kmem_cache_t* cache)
{
    if (cache == NULL) {
        return; // error, wrong arguments
    }
    //request space for slab from buddy
    slab* slab_ = buddy_alloc(cache->slab_size*BLOCK_SIZE);
    if (slab_ == NULL) {
        cache->error_code = 1; 
        return; //error not enough space
    }
    // add slab to empty list (front or back?), increase allignment, 
    slab_->my_cache = cache;
    // add slab to empty list front
    slab_->next_slab = cache->slabs[EMPTY];
    cache->slabs[EMPTY] = slab_;
    // determine slots start address 
    slab_->slots_start_addres = ((char*)slab_ + sizeof(slab) + cache->alignment);
    slab_->free_slots = slab_->slots_start_addres;
    //change alignment
    cache->alignment += CACHE_L1_LINE_SIZE;
    if (cache->alignment > cache->unused_space)
        cache->alignment = 0;
    //initialize objects 
    slab_->num_of_free_objects = cache->num_of_objects;
    void** iterator = slab_->slots_start_addres; 
    for (int i = 0; i < slab_->num_of_free_objects-1; i++) {
        *iterator = (char*)iterator + sizeof(void*) + cache->object_size;
        iterator = *iterator;
        if(cache->constructor)
            cache->constructor(NULL); // call constructor
    }
    *iterator = NULL;
}
void move_slab(slab* slab_, int from, int to) {
    kmem_cache_t* cache = slab_->my_cache;
    // remove from first list
    slab* temp = cache->slabs[from];
    slab* prev = NULL;
    //find slab
    while (temp != slab_) {
        prev = temp;
        temp = temp->next_slab;
    }
    if (prev == NULL) { // first in the list
        cache->slabs[from] = slab_->next_slab;
    }
    else { // not first in the list
        prev->next_slab = slab_->next_slab;
    }


    // add to second list
    slab_->next_slab = cache->slabs[to];
    cache->slabs[to] = slab_;
}
slab* find_objs_slab(kmem_cache_t* cachep, void* objp) {
    if (cachep == NULL || objp == NULL) {
        return NULL; //error wrong arguments
    }
    slab* temp = cachep->slabs[PARTIALLY_FULL];
    while (temp) {
        // check if obj belongs to slab
        if ((char*)objp > (char*)temp && (char*)objp < ((char*)temp + BLOCK_SIZE * cachep->slab_size)) 
            return temp;
        temp = temp->next_slab;
    }
    temp = cachep->slabs[FULL];
    while (temp) {
        // check if obj belongs to slab
        if ((char*)objp > (char*)temp && (char*)objp < ((char*)temp + BLOCK_SIZE * cachep->slab_size))
            return temp;
        temp = temp->next_slab;
    }
    
    return NULL; // error, slab not found
}



