#pragma once
// File: slab.h
#include <stdlib.h>
#define BLOCK_SIZE (4096)
#define CACHE_L1_LINE_SIZE (64)
#define PAGE_RESERVED_CACHES (7)
typedef struct slab
{
    struct slab *next_slab;   //link to next slab
    struct kmem_cache_s* my_cache; //this slab's cache
    void *slots_start_addres; //addres where begin slot space

} slab;

typedef struct kmem_cache_s
{
    //struct kmem_cache_s *next_cache; // link to next cache in list
    slab *slabs;             // slabs of this cache
    const char *name[30];    //objects type name
    size_t object_size;      // size of objects belonging to this cache
    unsigned num_of_objects; // num of objects in slab

    unsigned slab_size;    // size of slab (number of pages)
    unsigned unused_space; // size of unused space (bytes)
    unsigned alignment;    // alignment to L1 cache lines

    void (*constructor)(void *); // object constructor
    void (*destructor)(void *);  // object destructor
} kmem_cache_t;

typedef struct slab_info
{
    unsigned max_number_of_caches;      // max size of array of caches
    unsigned number_of_caches;          //current number of caches
    struct kmem_cache_s *arrayOfCaches; //array of caches
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