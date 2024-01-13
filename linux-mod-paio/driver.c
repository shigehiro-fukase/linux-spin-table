#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>
#include <linux/uaccess.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <asm/uaccess.h>

MODULE_LICENSE("Dual BSD/GPL");
#ifndef DRIVER_NAME
#define DRIVER_NAME	"mod-paio"	/* /proc/devices, /proc/modules */
#endif /* DRIVER_NAME */
#ifndef PROC_NAME
#define PROC_NAME	"paio"		/* procfs name of /proc/ */
#endif /* PROC_NAME */

#define LINUX_VERSION_CODE_RELEASE	(((LINUX_VERSION_CODE)>>16)&0xff)
#define LINUX_VERSION_CODE_MAJOR	(((LINUX_VERSION_CODE)>>8)&0xff)
#define LINUX_VERSION_CODE_MINOR	(((LINUX_VERSION_CODE)>>0)&0xff)

/* default value */

static bool procfs_io = true;
module_param(procfs_io, bool, S_IRUGO);
MODULE_PARM_DESC(procfs_io, "(optional) use procfs i/o");

static bool verbose = false;
module_param(verbose, bool, S_IRUGO);
MODULE_PARM_DESC(verbose, "(optional) verbose mode");

static int __init mod_init(void);
static void __exit mod_exit(void);

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

static void __iomem *mod_ioremap(resource_size_t phys_addr, unsigned long size) {
	if (verbose) printk(DRIVER_NAME ": %s version=%d %d.%d.%d\n", __func__
			, LINUX_VERSION_CODE
			, LINUX_VERSION_CODE_RELEASE
			, LINUX_VERSION_CODE_MAJOR
			, LINUX_VERSION_CODE_MINOR
			);
#if defined(ioremap_nocache)
	return ioremap_nocache(phys_addr, size);
#elif defined(ioremap_wc)
	return ioremap_wc(phys_addr, size);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0)
	pgprot_t pgprot = __pgprot(PROT_NORMAL_NC);
	return __ioremap(phys_addr, size, pgprot);
#else
#error "Unsupported kernel version"
#endif
}
static int strsplit(char * s, char* av[], int avsz) {
	int i;
	if (!s || !av || !avsz) return 0;
	for (i=0; i<(avsz-1); ) {
		for (; s && *s; s++) if (!isspace(*s)) break;
		if (!s || !*s) break;
		av[i++] = s;
		for (; s && *s; s++) if (!isgraph(*s)) break;
		if (!s || !*s) break;
		*s++ = '\0';
	}
	av[i] = NULL;
	return i;
}
static int mod_parse_args(int argc, const char * argv[]) {
	int ret, i = 0;
	enum iocmd cmd = 0;
	int dir = -1;
	unsigned byte_width = 0;
	unsigned long long phys_addr = 0;
	unsigned long phys_size = 0;
	unsigned long count = 0;
	unsigned long long value = 0;
	void __iomem * iomem = NULL;
	const char* s;

	//
	// av[0] iocmd
	//
	s = argv[i];
	if (!s) return -1;
	if (verbose) printk(DRIVER_NAME ": %s argv[%d]='%s'\n", __func__, i, s);
	if (0) {
	} else if (strcasecmp(s, "rb") == 0) { cmd = CMD_RB; byte_width = 1; dir = 0;
	} else if (strcasecmp(s, "rw") == 0) { cmd = CMD_RW; byte_width = 2; dir = 0;
	} else if (strcasecmp(s, "rd") == 0) { cmd = CMD_RD; byte_width = 4; dir = 0;
	} else if (strcasecmp(s, "rq") == 0) { cmd = CMD_RQ; byte_width = 8; dir = 0;
	} else if (strcasecmp(s, "wb") == 0) { cmd = CMD_WB; byte_width = 1; dir = 1;
	} else if (strcasecmp(s, "ww") == 0) { cmd = CMD_WW; byte_width = 2; dir = 1;
	} else if (strcasecmp(s, "wd") == 0) { cmd = CMD_WD; byte_width = 4; dir = 1;
	} else if (strcasecmp(s, "wq") == 0) { cmd = CMD_WQ; byte_width = 8; dir = 1;
	} else {
		printk(DRIVER_NAME ": %s BAD argument[%d] '%s'\n", __func__, i, s);
		return -1;
	}
	i++;
	s = argv[i];
	if (!s) return -1;

	//
	// argv[1] address
	//
	ret = kstrtoull(s, 0, &phys_addr);
	printk(DRIVER_NAME ": phys_addr=0x%llX ret=%d\n", phys_addr, ret);
	i++;
	s = argv[i];
	if (!s) return -1;

	//
	// argv[2] r(read count) w(write value...)
	//
	if (dir == 0) {
		ret = kstrtoul(s, 0, &count);
		printk(DRIVER_NAME ": count=%ld ret=%d\n", count, ret);
	} else /* if (dir == 1) */ {
		count = argc - i;
		printk(DRIVER_NAME ": count=%ld\n", count);
	}
	phys_size = byte_width * count;
	printk(DRIVER_NAME ": phys_size=%ld\n", phys_size);

	iomem = mod_ioremap((resource_size_t) phys_addr, phys_size);
	if (verbose) printk(DRIVER_NAME ": iomem=0x%p\n", iomem);
	if (!iomem) {
		printk(DRIVER_NAME ": %s Cannot map memory 0x%llX 0x%lX\n", __func__, phys_addr, phys_size);
		return -EFAULT;
	}

	if (dir == 0) {
		unsigned long off;
		for (off=0; off<count; off++) {
			if (cmd == CMD_RB) {
				uint8_t * p = ((uint8_t*)iomem) + off;
				printk(DRIVER_NAME ": RB 0x%08llX: 0x%02X\n", phys_addr+ (off*byte_width), *p);
			} else if (cmd == CMD_RW) {
				uint16_t * p = ((uint16_t*)iomem) + off;
				printk(DRIVER_NAME ": RW 0x%08llX: 0x%04X\n", phys_addr+ (off*byte_width), *p);
			} else if (cmd == CMD_RD) {
				uint32_t * p = ((uint32_t*)iomem) + off;
				printk(DRIVER_NAME ": RD 0x%08llX: 0x%08X\n", phys_addr+ (off*byte_width), *p);
			} else if (cmd == CMD_RQ) {
				uint64_t * p = ((uint64_t*)iomem) + off;
				printk(DRIVER_NAME ": RQ 0x%08llX: 0x%016llX\n", phys_addr+ (off*byte_width), *p);
			}
		}
	} else /* if (dir == 1) */ {
		count = argc - i;
		printk(DRIVER_NAME ": count=%ld\n", count);
		unsigned long off;
		for (off=0; i<argc; i++, off++) {
			s = argv[i];
			ret = kstrtoull(s, 0, &value);
			if (verbose) printk(DRIVER_NAME ": argv[%d]=0x%llX ret=%d\n", i, value, ret);
			if (cmd == CMD_WB) {
				uint8_t * p = ((uint8_t*)iomem) + off;
				uint8_t prev = *p;
				*p = (uint8_t)(value & 0xFFu);
				printk(DRIVER_NAME ": WB 0x%08llX: 0x%02X <- 0x%02X\n", phys_addr+ (off*byte_width), prev, *p);
			} else if (cmd == CMD_WW) {
				uint16_t * p = ((uint16_t*)iomem) + off;
				uint16_t prev = *p;
				*p = (uint16_t)(value & 0xFFFFu);
				printk(DRIVER_NAME ": WB 0x%08llX: 0x%04X <- 0x%04X\n", phys_addr+ (off*byte_width), prev, *p);
			} else if (cmd == CMD_WD) {
				uint32_t * p = ((uint32_t*)iomem) + off;
				uint32_t prev = *p;
				*p = (uint32_t)(value & 0xFFFFFFFFul);
				printk(DRIVER_NAME ": WB 0x%08llX: 0x%08X <- 0x%08X\n", phys_addr+ (off*byte_width), prev, *p);
			} else if (cmd == CMD_WQ) {
				uint64_t * p = ((uint64_t*)iomem) + off;
				uint64_t prev = *p;
				*p = (uint64_t)(value & 0xFFFFFFFFFFFFFFFFull);
				printk(DRIVER_NAME ": WB 0x%08llX: 0x%016llX <- 0x%016llX\n", phys_addr+ (off*byte_width), prev, *p);
			}
		}
	}
	iounmap(iomem);
	return argc;
}
static int mod_parse_line(char * buf, char* av[], int avsz) {
	int ac;
	if (verbose) printk(DRIVER_NAME ": %s line='%s'\n", __func__, buf);
	ac = strsplit(buf, av, avsz);
	if (ac <= 0) return ac;
	return mod_parse_args(ac, (const char**)av);
}

static char proc_linebuf[0x400];
static char av[32];

static int mod_proc_open(struct inode *inode, struct file *file) {
	printk(DRIVER_NAME ": %s\n", __func__);
	return 0;
}
static int mod_proc_close(struct inode *inode, struct file *file) {
	printk(DRIVER_NAME ": %s\n", __func__);
	return 0;
}
static ssize_t mod_proc_read(struct file *file, char __user *buf, size_t count, loff_t *fpos) {
	printk(DRIVER_NAME ": %s\n", __func__);
	return 0;
}
static ssize_t mod_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *fpos) {
	printk(DRIVER_NAME ": %s\n", __func__);
	if (count > sizeof(proc_linebuf)) count = sizeof(proc_linebuf) -1;
	if (__copy_from_user(proc_linebuf, buf, count)) {
		return -EFAULT;
	}
	proc_linebuf[count] = '\0';
	int ret;
	ret = mod_parse_line(proc_linebuf, (char**)&av, sizeof(av));
	if (ret < 0) {
		printk(DRIVER_NAME ": %s E(%d)\n", __func__, ret);
	}
	// printk(DRIVER_NAME ": %s '%s'\n", __func__, proc_linebuf);
	return count;
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0)
static struct file_operations mod_proc_fops = {
	.owner = THIS_MODULE,
	.open = mod_proc_open,
	.release = mod_proc_close,
	.read = mod_proc_read,
	.write = mod_proc_write,
};
#else
static struct proc_ops mod_proc_fops = {
	.proc_open = mod_proc_open,
	.proc_release = mod_proc_close,
	.proc_read = mod_proc_read,
	.proc_write = mod_proc_write,
};
#endif

/* At load (insmod) */
static int __init mod_init(void) {
	printk(DRIVER_NAME ": %s\n", __func__);

	if (procfs_io) {
		struct proc_dir_entry *entry;
		entry = proc_create(PROC_NAME, S_IRUGO | S_IWUGO, NULL, &mod_proc_fops);
		if (entry == NULL) {
			printk(KERN_ERR "proc_create\n");
			return -ENOMEM;
		}
	}

	printk(DRIVER_NAME ": %s done.\n", __func__);
	return 0;
}

/* At unload (rmmod) */
static void __exit mod_exit(void) {
	printk(DRIVER_NAME ": %s\n", __func__);
	if (procfs_io) {
		remove_proc_entry(PROC_NAME, NULL);
	}
}

module_init(mod_init);
module_exit(mod_exit);
