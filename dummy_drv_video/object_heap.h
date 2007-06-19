/*
 * @COPYRIGHT@ Intel Confidential - Unreleased Software
 */

#ifndef _OBJECT_HEAP_H_
#define _OBJECT_HEAP_H_

#define OBJECT_HEAP_OFFSET_MASK		0x7F000000
#define OBJECT_HEAP_ID_MASK			0x00FFFFFF

typedef struct object_base *object_base_p;
typedef struct object_heap *object_heap_p;

struct object_base {
    int id;
    int next_free;
};

struct object_heap {
    int	object_size;
    int id_offset;
    void *heap_index;
    int next_free;
    int heap_size;
    int heap_increment;
};

typedef int object_heap_iterator;

/*
 * Return 0 on success, -1 on error
 */
int object_heap_init( object_heap_p heap, int object_size, int id_offset);

/*
 * Allocates an object
 * Returns the object ID on success, returns -1 on error
 */
int object_heap_allocate( object_heap_p heap );

/*
 * Lookup an allocated object by object ID
 * Returns a pointer to the object on success, returns NULL on error
 */
object_base_p object_heap_lookup( object_heap_p heap, int id );

/*
 * Iterate over all objects in the heap.
 * Returns a pointer to the first object on the heap, returns NULL if heap is empty.
 */
object_base_p object_heap_first( object_heap_p heap, object_heap_iterator *iter );

/*
 * Iterate over all objects in the heap.
 * Returns a pointer to the next object on the heap, returns NULL if heap is empty.
 */
object_base_p object_heap_next( object_heap_p heap, object_heap_iterator *iter );

/*
 * Frees an object
 */
void object_heap_free( object_heap_p heap, object_base_p obj );

/*
 * Destroys a heap, the heap must be empty.
 */
void object_heap_destroy( object_heap_p heap );

#endif /* _OBJECT_HEAP_H_ */
