#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#ifndef NL
#define NL "\n"
#endif

static const char * __progname;

static int usage(int argc, const char * argv[]);

int opt_cpu = 0;

int cpu_num = -1;

static int parse_args(int argc, const char * argv[]) {
    int ret = EXIT_SUCCESS;
    int i;

    printf("argv[%d]:%s\n", 0, argv[0]);
    for (i=1; i<argc; i++) {
        const char * arg = argv[i];
        if (*arg == '-') {
            if ((strcmp(arg, "-h") == 0) || (strcmp(arg, "--help") == 0)) {
                return usage(argc, argv);
            } else {
                printf("Unknown argument argv[%d]:%s\n", 0, argv[i]);
            }
        } else if (!opt_cpu) {
            cpu_num = atoi(argv[i]);
            opt_cpu = 1;
        } else {
            printf("Unknown argument argv[%d]:%s\n", 0, argv[i]);
        }
    }

    if (opt_cpu == 0) {
        return usage(argc, argv);
    }

    printf("CPU=%d\n", cpu_num);

    return ret;
}
static int is_file(const char * fname, struct stat *sb_return) {
    int ret;
    struct stat sb;

    ret = stat(fname, &sb);
    if (ret != 0) {
        printf("%s: Cannot stat file '%s'!\n", __progname, fname);
        return EXIT_FAILURE;
    }
    if (S_ISDIR(sb.st_mode)) {
        printf("%s: file '%s' is not a file!\n", __progname, fname);
        return EXIT_FAILURE;
    }
    if (sb_return) {
        memcpy(sb_return, &sb, sizeof(sb));
    }
    return EXIT_SUCCESS;
}
static uint64_t swap64(uint64_t rv) {
    uint64_t lv;

    lv = 0
        | (((rv >>  0) & 0xFFllu) << 56)
        | (((rv >>  8) & 0xFFllu) << 48)
        | (((rv >> 16) & 0xFFllu) << 40)
        | (((rv >> 24) & 0xFFllu) << 32)
        | (((rv >> 32) & 0xFFllu) << 24)
        | (((rv >> 40) & 0xFFllu) << 16)
        | (((rv >> 48) & 0xFFllu) <<  8)
        | (((rv >> 56) & 0xFFllu) <<  0)
        ;
    return lv;
}
static int get_val64(const char * fname, uint64_t *val_return) {
    int ret, fd;
    uint64_t val;

    ret = is_file(fname, NULL);
    if (ret != EXIT_SUCCESS) {
        return ret;
    }
    fd = open(fname, O_RDONLY);
    if (fd < 0) {
        printf("cannot open %s", fname);
        return (EXIT_FAILURE);
    }
    read(fd, &val, sizeof(val));
    close(fd);
    val = swap64(val);
    if (val_return) {
        *val_return = val;
    }
    return EXIT_SUCCESS;
}
static int get_reg(const char * fname, uint32_t *addr_return, uint32_t *size_return) {
    int ret, fd;
    uint8_t reg[2 * 8];
    struct stat sb;
    uint32_t i, addr, size, reg_size, addr_cells, size_cells;

    ret = is_file(fname, &sb);
    if (ret != EXIT_SUCCESS) {
        return ret;
    }
    fd = open(fname, O_RDONLY);
    if (fd < 0) {
        printf("cannot open %s", fname);
        return (EXIT_FAILURE);
    }
    read(fd, reg, sizeof(reg));
    close(fd);

    reg_size = sb.st_size;
    if (reg_size < 12) {
        addr_cells = 1 * 4;
        size_cells = 1 * 4;
    } else if (reg_size < 16) {
        addr_cells = 1 * 8;
        size_cells = 1 * 4;
    } else {
        addr_cells = 1 * 8;
        size_cells = 1 * 8;
    }
#if 0
    for (i=0; i<reg_size; i++) {
        printf("reg[%d] = 0x%02X\n", i, reg[i]);
    }
#endif
    addr = 0;
    for (i=0; i<addr_cells; i++) {
        // printf("addr[%d] = 0x%02X\n", i, reg[i]);
        addr = (addr << 8) + reg[i];
    }
    size = 0;
    for (; i<(addr_cells+size_cells); i++) {
        // printf("size[%d] = 0x%02X\n", i, reg[i]);
        size = (size << 8) + reg[i];
    }
    // printf("addr = 0x%02X\n", addr);
    // printf("size = 0x%02X\n", size);

    if (addr_return) {
        *addr_return = addr;
    }
    if (size_return) {
        *size_return = size;
    }

    return EXIT_SUCCESS;
}
int main(int argc, const char * argv[]) {
    int ret = 0;
    uint64_t val64;
    uint32_t addr, size;
    char fname[256];

    __progname = argv[0];

    if ((ret = parse_args(argc, argv)) != EXIT_SUCCESS) {
        return ret;
    }

    sprintf(fname, "/proc/device-tree/cpus/cpu@%d/cpu-release-addr", cpu_num);
    printf("Checking \"%s\"..."NL, fname);
    ret = get_val64(fname, &val64);
    if (ret == EXIT_SUCCESS) {
        printf("CPU[%d] 0x%016lX"NL, cpu_num, val64);
    }

    sprintf(fname, "/proc/device-tree/uio@%d/reg", cpu_num);
    printf("Checking \"%s\"..."NL, fname);
    ret = get_reg(fname, &addr, &size);
    if (ret == EXIT_SUCCESS) {
        printf("CPU[%d] addr=0x%08X size=0x%X"NL, cpu_num, addr, size);
    }

    sprintf(fname, "/sys/class/uio/uio%d/device/of_node/reg", cpu_num);
    printf("Checking \"%s\"..."NL, fname);
    ret = get_reg(fname, &addr, &size);
    if (ret == EXIT_SUCCESS) {
        printf("CPU[%d] addr=0x%08X size=0x%X"NL, cpu_num, addr, size);
    }

    return 0;
}

static int usage(int argc, const char * argv[]) {
    (void)argc;
    printf(
            "usage: %s [OPTIONS] CPU"NL
            ""NL
            "Check Spin Table Release Address for CPU(n)."NL
            ""NL
            "CPU                CPU core number"NL
            ""NL
            "OPTIONS"NL
            "-h, --help         Show this message"NL
            ""NL
            , argv[0]);
    fflush(stdout);
    return -1;
}

