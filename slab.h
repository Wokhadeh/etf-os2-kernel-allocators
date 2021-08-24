#pragma once
// File: slab.h
#include <stdlib.h>
#define BLOCK_SIZE (4096)
#define CACHE_L1_LINE_SIZE (64)
#define PAGE_RESERVED_CACHES (7)
#define EMPTY (0)
#define PARTIALLY_FULL (1)
#define FULL (2)
#define NUMBER_OF_SMALL_MEMORY_BUFFERS (13)
typedef struct slab
{
    struct slab *next_slab;   //link to next slab
    struct kmem_cache_s* my_cache; //this slab's cache
    int num_of_free_objects;
    void *slots_start_addres; //addres where begin slot space
    void* free_slots; //start of free slots implicit list list
} slab;

typedef struct kmem_cache_s
{
    //struct kmem_cache_s *next_cache; // link to next cache in list
    slab* slabs[3];             // slabs of this cache,empty 0,partially full 1, full 2
    const char name[30];    //objects type name
    size_t object_size;      // size of objects belonging to this cache
    unsigned num_of_objects; // num of objects in slab

    unsigned slab_size;    // size of slab (number of pages)
    unsigned unused_space; // size of unused space (bytes)
    unsigned alignment;    // alignment to L1 cache lines

    void (*constructor)(void *); // object constructor
    void (*destructor)(void *);  // object destructor

    int error_code; //error code
} kmem_cache_t;

typedef struct slab_info
{
    unsigned max_number_of_caches;      // max size of array of caches
    unsigned number_of_caches;          //current number of caches
    struct kmem_cache_s *array_of_caches; //array of caches
    struct kmem_cache_s small_memory_buffers[NUMBER_OF_SMALL_MEMORY_BUFFERS]; // small memory buffers ( 2^5 B to 2^17 B)
} slab_info;

//interface
void kmem_init(void *space, int block_num);
kmem_cache_t *kmem_cache_create(const char *name, size_t size,
                                void (*ctor)(void *),
                                void (*dtor)(void *));  // Allocate cache
int kmem_cache_shrink(kmem_cache_t *cachep);            // Shrink cache
void *kmem_cache_alloc(kmem_cache_t *cachep);           // Allocate one object from cache
void kmem_cache_free(kmem_cache_t *cachep, void *objp); // Deallocate one object from cache
void *kmalloc(size_t size);                             // Alloacate one small memory buffer
void kfree(const void *objp);                           // Deallocate one small memory buffer
void kmem_cache_destroy(kmem_cache_t *cachep);          // Deallocate cache
void kmem_cache_info(kmem_cache_t *cachep);             // Print cache info
int kmem_cache_error(kmem_cache_t *cachep);             // Print error message

//slab helper functions
slab_info *get_slab_info(); //get slab_info*
void allocate_slab(kmem_cache_t* cache); //allocate slab for specific cache
void move_slab(slab* slab, int from, int to); // move slab from one list to another
slab* find_objs_slab(kmem_cache_t* cachep, void* objp); // find slab to whom obj belongs