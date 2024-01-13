// SPDX-License-Identifier: MIT
#ifndef ___MEM_H___
#define ___MEM_H___

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h> /* sysconf */

static inline unsigned long mmap_sys_pagesize(void) {
    unsigned long size = 0;
    long ret = sysconf(_SC_PAGE_SIZE);
    if (ret > 0) size = (unsigned long) ret;
    else size = 0x1000U; // error, default 4Kib
    return size;
}

#ifndef PROT_NOCACHE
#define PROT_NOCACHE 0
#endif // PROT_NOCACHE
#ifndef MAP_PHYS
#define MAP_PHYS 0
#endif // MAP_PHYS
#ifndef NOFD
#define NOFD -1
#endif // NOFD
static inline void * mmap_device_memory(const char * dev, void * addr, size_t len, uintptr_t physical, int * fd) {
    static int mmap_fd = NOFD;
    void * va;
    int prot = PROT_NOCACHE | PROT_READ | PROT_WRITE;
    int flags = 0;
    unsigned long page_size;
    uintptr_t map_pa, map_off, page_mask;
    size_t map_sz;

    if (!dev) {
        dev = "/dev/mem";
    }

    if (mmap_fd < 0) {
        mmap_fd = open(dev, O_RDWR | O_SYNC);
        if (mmap_fd < 0) {
            char msg[256];
            sprintf(msg, "open %s", dev);
            perror(msg);
            exit(EXIT_FAILURE);
        }
    }

    if (strncmp(dev, "/dev/uio", strlen("/dev/uio")) == 0) {
        lseek(mmap_fd, physical, SEEK_SET);
    }

    page_size = mmap_sys_pagesize();
    page_mask = ((uintptr_t)page_size -1);
    map_pa = physical & ~page_mask;
    map_off = physical - map_pa;
    map_sz = (len + map_off + page_mask) & ~page_mask;

    va = mmap(addr, map_sz, prot,
            (flags & ~MAP_TYPE)
            // | MAP_ANONYMOUS
            | MAP_PHYS
            | MAP_SHARED
            , mmap_fd, map_pa);
    if (!va || ((uintptr_t)va == ~0ull)) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    if (fd) {
        *fd = mmap_fd;
    }

#if 0
    if (strncmp(dev, "/dev/uio", strlen("/dev/uio")) == 0) {
        return (void*)(((uint8_t*)va) + map_off + physical);
    } else {
        return (void*)(((uint8_t*)va) + map_off);
    }
#else
    return (void*)(((uint8_t*)va) + map_off);
#endif
}
static inline void munmap_device_memory(int fd) {
    if (fd >= 0) {
        close(fd);
        fd = NOFD;
    }
}

#endif /* ___MEM_H___ */

