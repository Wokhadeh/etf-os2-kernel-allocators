#include "buddy.h"

int main()
{

    void *space = calloc(1000, BLOCK_SIZE);;
    //printf("Pointer %p  and size %llu and sizeof longint %llu \n", space, sizeof(space), sizeof(long long int));
    kmem_init(space, 1000);
    kmem_cache_t* cache = kmem_cache_create("OBJECT 1", 60, NULL, NULL);
    kmem_cache_info(cache);
    void* obj = kmem_cache_alloc(cache);
    void* small_memory = kmalloc(64);
    kmem_cache_info(cache);
    kmem_cache_info(&get_slab_info()->small_memory_buffers[1]);
    kmem_cache_free(cache, obj);
    kfree(small_memory);
    kmem_cache_info(&get_slab_info()->small_memory_buffers[1]);
    kmem_cache_info(cache);
    print_buddy();
    printf("Free space in first block: %d \n", (BLOCK_SIZE - sizeof(buddy_info) - sizeof(slab_info)) / sizeof(kmem_cache_t));
  
   
  
  

    //print_buddy();
    return 0;
}