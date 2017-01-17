#ifndef skynet_malloc_h
#define skynet_malloc_h

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void skynet_memory_init(void* error_fn, void* handle_fn);
void * skynet_malloc(size_t sz);
void * skynet_calloc(size_t nmemb,size_t size);
void * skynet_realloc(void *ptr, size_t size);
void skynet_free(void *ptr);
char * skynet_strdup(const char *str);
void * skynet_lalloc(void *ptr, size_t osize, size_t nsize);	// use for lua
#ifdef __cplusplus
}
#endif

#endif
#ifdef _MSC_VER
#include <unistd.h>
#endif
