#include "mm.h"
#include <string.h>

static Header base;
static Header *free_list = NULL;


void* my_malloc(size_t bytes)
{
    size_t units;
    Header *prev, *now;

    units = (bytes + sizeof(Header) - 1)/sizeof(Header) + 1;
    
    // no free list
    prev = free_list;
    if(prev == NULL)
    {
        base.meta.next = &base;
        free_list = &base;
        prev = &base;
        base.meta.size = 0;
    }

    for(now = prev->meta.next; ; prev = now, now=now->meta.next)
    {
        // big enough
        if(now->meta.size >= units)
        {
            if(now->meta.size == units) // equal
            {
                prev->meta.next = now->meta.next;
            }
            else                        // larger
            {
                // split
                now->meta.size -= units;
                now += now->meta.size;
                now->meta.size = units;
            }

            free_list = prev;
            return (void*)(now + 1);
        }

        // go around and back to the free_list (start)
        if(now == free_list)
        {
            // allocate a new memory
            now = morecore(units);

            if(now == NULL)
                return NULL;
        }
    }
}

void* my_calloc(size_t num, size_t size)
{
    void *ptr = my_malloc(num*size);

    if(ptr)
        memset(ptr, 0, num*size);
    
    return ptr;
}

void* my_realloc(void *ptr, size_t bytes)
{
    // NULL ptr, act like malloc
    if(!ptr)
        return my_malloc(bytes);

    // bytes == 0, like free
    if(bytes == 0)
    {
        my_free(ptr);
        return NULL;
    }

    size_t units = (bytes + sizeof(Header) - 1)/sizeof(Header) + 1;
    size_t old_units = ((Header*)ptr - 1)->meta.size;
    size_t max = (units >= old_units ? units*sizeof(Header) : old_units*sizeof(Header));

    void *new_ptr = my_malloc(units * sizeof(Header));

    if(new_ptr)
        memcpy(new_ptr, ptr, max);

    my_free(ptr);
    return new_ptr;
}

void my_free(void *p)
{
    Header *ptr;
    Header *header = (Header*)p - 1;

    // !(header > ptr && header < ptr->meta.next)  --> true : header is not between ptr and ptr->meta.next
    for(ptr = free_list; !(header > ptr && header < ptr->meta.next); ptr = ptr->meta.next)
        // ptr >= ptr->meta.next  --> ptr is the rightmost of the free list
        // header > ptr  --> header is at right hand side of free list
        // header < ptr->meta.next --> header is at left hand side of free list
        // ---> true : ptr is the rightmost of the free list and 
        //      header is righter or lefter than both end points of the free list
        if(ptr >= ptr->meta.next && (header > ptr || header < ptr->meta.next))
            break;


    // now there are  conditions
    // 1. header is between ptr and ptr->meta.next
    // 2. header is righter than the free list
    // 3. header is lefter than the free list

    // check the right hand side of the header
    if(header + header->meta.size == ptr->meta.next)    // if header is adjacent to the ptr->meta.next
    {
        // merge
        header->meta.size += ptr->meta.size;
        header->meta.next = ptr->meta.next->meta.next;
    }
    else
        header->meta.next = ptr->meta.next->meta.next;

    // check the left hand side of the header
    if(ptr + ptr->meta.size == header)
    {
        // merge
        ptr->meta.size += header->meta.size;
        ptr->meta.next = header->meta.next;
    }
    else
        ptr->meta.next = header;

    free_list = ptr;
}

static Header* morecore(size_t num)
{
    Header *header;
    void *ptr;

    if(num < MIN_ALLOC_SIZE)
        num = MIN_ALLOC_SIZE;

    ptr = __os_alloc(num * sizeof(Header));

    if(!ptr)
        return NULL;

    header = (Header*)ptr;
    header->meta.size = num;

    my_free((void*)(header+1));

    return free_list;
}

#ifdef __WINDOWS__
static void* __os_alloc(size_t num)
{
	static HANDLE h_heap = NULL;

	if (!h_heap)
		h_heap = GetProcessHeap();

	return HeapAlloc(h_heap, 0, num);
}
#endif

#ifdef __LINUX__
static void* __os_alloc(size_t num)
{
    void* ptr = sbrk(num);
	return (ptr == (void*)-1) ? NULL : ptr;
}
#endif