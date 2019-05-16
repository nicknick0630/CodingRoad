#ifndef _MM_H
#define _MM_H

#ifdef _WIN32
    #define __WINDOWS__
    #include <Windows.h>
#endif /* _WIN32 */

#ifdef __linux__
    #define __LINUX__
    #include <unistd.h>
#endif /* __linux__ */

#define MIN_ALLOC_SIZE 128
#define ALIGN long

typedef union _Header
{
    struct 
    {
        union _Header *next;
        size_t size;
    } meta;
    
    ALIGN a;
} Header;

void* my_malloc(size_t bytes);
void* my_calloc(size_t num, size_t size);
void* my_realloc(void *ptr, size_t bytes);
void my_free(void *ptr);


static void* __os_alloc(size_t num);

static Header* morecore(size_t num);




#endif