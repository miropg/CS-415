#include <sys/mman.h>
void* shared_malloc(size_t size)
{
    int prot = PROT_READ | PROT_WRITE; // read and write access
    //processes have shared access, no file involved
    int flags = MAP_SHARED | MAP_ANONYMOUS; 
    return mmap(NULL, size, prot, flags, -1, 0); 
}

// Name: mmap, munmap - map or unmap files or devices into memory
//Synopsis
void *mmap(void *addr, size_t length, int prot, int flags,
                int fd, off_t offset);
    int munmap(void *addr, size_t length);