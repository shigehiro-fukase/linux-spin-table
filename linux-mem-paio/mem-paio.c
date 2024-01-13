// SPDX-License-Identifier: MIT
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "mem.h"

#ifndef NL
#define NL "\n"
#endif

static const char * __progname;

static int usage(int argc, const char * argv[]);

enum iocmd {
    CMD_RB,
    CMD_RW,
    CMD_RD,
    CMD_RQ,
    CMD_WB,
    CMD_WW,
    CMD_WD,
    CMD_WQ,
};

int opt_verbose = 0;
int opt_cmd = 0;
int opt_pa = 0;
int opt_count = 0;

int cpu_num = -1;
intptr_t phys_addr = ~0;
size_t mem_sz = 0;

static int parse_args(int argc, const char * argv[]) {
    int ret = EXIT_SUCCESS;
    int i;
    enum iocmd cmd = 0;
    int dir = -1;
    unsigned byte_width = 0;
    unsigned long phys_size = 0;
    unsigned long count = 0;
    unsigned long long value = 0;
    int mmap_fd = NOFD;
    void * va;

    if (opt_verbose) printf("argv[%d]:%s"NL, 0, argv[0]);
    for (i=1; i<argc; i++) {
        const char * arg = argv[i];
        if (*arg == '-') {
            if ((strcmp(arg, "-h") == 0) || (strcmp(arg, "--help") == 0)) {
                return usage(argc, argv);
            } else if (strcmp(arg, "--verbose") == 0) {
                opt_verbose = 1;
            } else {
                printf("Unknown argument argv[%d]:%s"NL, 0, argv[i]);
                return EXIT_FAILURE;
            }
        } else if (!opt_cmd) { // CMD
            if (opt_verbose) printf("%s argv[%d]='%s'"NL, __func__, i, arg);
            if (0) {
            } else if (strcasecmp(arg, "rb") == 0) { cmd = CMD_RB; byte_width = 1; dir = 0;
            } else if (strcasecmp(arg, "rw") == 0) { cmd = CMD_RW; byte_width = 2; dir = 0;
            } else if (strcasecmp(arg, "rd") == 0) { cmd = CMD_RD; byte_width = 4; dir = 0;
            } else if (strcasecmp(arg, "rq") == 0) { cmd = CMD_RQ; byte_width = 8; dir = 0;
            } else if (strcasecmp(arg, "wb") == 0) { cmd = CMD_WB; byte_width = 1; dir = 1;
            } else if (strcasecmp(arg, "ww") == 0) { cmd = CMD_WW; byte_width = 2; dir = 1;
            } else if (strcasecmp(arg, "wd") == 0) { cmd = CMD_WD; byte_width = 4; dir = 1;
            } else if (strcasecmp(arg, "wq") == 0) { cmd = CMD_WQ; byte_width = 8; dir = 1;
            } else {
                printf("%s BAD argument[%d] '%s'"NL, __func__, i, arg);
                return EXIT_FAILURE;
            }
            opt_cmd = 1;
        } else if (!opt_pa) { // ADDRESS
            phys_addr = strtoull(arg, NULL, 0);
            opt_pa = 1;
            printf("phys_addr=0x%lX"NL, phys_addr);
            if (dir == 1) {
                count = argc - i;
                printf("count=%ld"NL, count);
                opt_count = 1;
                break;
            }
        } else if ((dir == 0) && (!opt_count)) {
            count = strtoul(argv[i], NULL, 0);
            printf("count=%ld"NL, count);
            opt_count = 1;
            break;
        } else {
            printf("Unknown argument argv[%d]:%s"NL, 0, arg);
            return EXIT_FAILURE;
        }
    }
    if (!opt_cmd || !opt_pa || !opt_count) {
        return usage(argc, argv);
    }

    phys_size = byte_width * count;
    {
        unsigned long pagesize = mmap_sys_pagesize();
        printf("ADDRESS=0x%lX SIZE=0x%lX PAGESIZE=0x%lX"NL, 
                phys_addr, phys_size, pagesize);
    }

    if ((va = mmap_device_memory(NULL, NULL, phys_size, phys_addr, &mmap_fd)) == NULL) {
        printf("Error (E) cannot mmap memory 0x%016lX"NL, phys_addr);
        return EXIT_FAILURE;
    }
    if (opt_verbose) printf("VA=0x%p"NL, va);

    if (dir == 0) {
	    unsigned long off;
	    for (off=0; off<count; off++) {
		    if (cmd == CMD_RB) {
			    uint8_t * p = ((uint8_t*)va) + off;
			    printf("RB 0x%08lX: 0x%02X"NL, phys_addr+ (off*byte_width), *p);
		    } else if (cmd == CMD_RW) {
			    uint16_t * p = ((uint16_t*)va) + off;
			    printf("RW 0x%08lX: 0x%04X"NL, phys_addr+ (off*byte_width), *p);
		    } else if (cmd == CMD_RD) {
			    uint32_t * p = ((uint32_t*)va) + off;
			    printf("RD 0x%08lX: 0x%08X"NL, phys_addr+ (off*byte_width), *p);
		    } else if (cmd == CMD_RQ) {
			    uint64_t * p = ((uint64_t*)va) + off;
			    printf("RQ 0x%08lX: 0x%016lX"NL, phys_addr+ (off*byte_width), *p);
		    }
	    }
    } else /* if (dir == 1) */ {
	    count = argc - i;
	    printf("count=%ld"NL, count);
	    unsigned long off;
	    for (off=0; i<argc; i++, off++) {
            value = strtoull(argv[i], NULL, 0);
		    if (opt_verbose) printf("argv[%d]=0x%llX ret=%d"NL, i, value, ret);
		    if (cmd == CMD_WB) {
			    uint8_t * p = ((uint8_t*)va) + off;
			    uint8_t prev = *p;
			    *p = (uint8_t)(value & 0xFFu);
			    printf("WB 0x%08lX: 0x%02X <- 0x%02X"NL, phys_addr+ (off*byte_width), prev, *p);
		    } else if (cmd == CMD_WW) {
			    uint16_t * p = ((uint16_t*)va) + off;
			    uint16_t prev = *p;
			    *p = (uint16_t)(value & 0xFFFFu);
			    printf("WB 0x%08lX: 0x%04X <- 0x%04X"NL, phys_addr+ (off*byte_width), prev, *p);
		    } else if (cmd == CMD_WD) {
			    uint32_t * p = ((uint32_t*)va) + off;
			    uint32_t prev = *p;
			    *p = (uint32_t)(value & 0xFFFFFFFFul);
			    printf("WB 0x%08lX: 0x%08X <- 0x%08X"NL, phys_addr+ (off*byte_width), prev, *p);
		    } else if (cmd == CMD_WQ) {
			    uint64_t * p = ((uint64_t*)va) + off;
			    uint64_t prev = *p;
			    *p = (uint64_t)(value & 0xFFFFFFFFFFFFFFFFull);
			    printf("WB 0x%08lX: 0x%016lX <- 0x%016lX"NL, phys_addr+ (off*byte_width), prev, *p);
		    }
	    }
    }
    munmap_device_memory(mmap_fd);
    return ret;
}
int main(int argc, const char * argv[]) {
    int ret = 0;

    __progname = argv[0];
    if ((ret = parse_args(argc, argv)) != EXIT_SUCCESS) {
        return ret;
    }

    return EXIT_SUCCESS;
}

static int usage(int argc, const char * argv[]) {
    (void)argc;
    printf(
            "usage: %s [OPTIONS] CMD ADDRESS PARAM..."NL
            ""NL
            "memory I/O via /dev/mem."NL
            ""NL
            "CMD                Read: RB, RW, RD, RQ"NL
            "                   Write: WB, WW, WD, WQ"NL
            "ADDRESS            Physical Address"NL
            "PARAM              Read: count"NL
            "PARAM...           Write: value..."NL
            ""NL
            "OPTIONS"NL
            "-h, --help         Show this message"NL
            ""NL
            , argv[0]);
    fflush(stdout);
    return -1;
}

