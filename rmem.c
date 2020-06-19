#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "rmem.h"

void rmem_init(struct rmem_info *rmem)
{
    memset(rmem->suffixes, 0, sizeof(rmem->suffixes));
    rmem->mem = NULL;
    rmem->npages = 0;
    rmem->nmems = 0;
    rmem->fd = -1;
}

void rmem_add_suffix(struct rmem_info *rmem, int suffix)
{
    rmem->suffixes[rmem->nmems++] = suffix;
}

int rmem_mmap_open(size_t maxbytes, struct rmem_info *rmem, void **mem_base)
{
    volatile uint64_t *exttab;
    int etfd, i;

    etfd = open("/dev/dram-cache-exttab", O_RDWR);
    if (etfd < 0) {
	perror("open");
	return etfd;
    }

    exttab = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, etfd, 0);
    if (exttab == MAP_FAILED) {
	perror("mmap");
	return -1;
    }

    // Round-Robin extent mappings between memory blades
    for (i = 0; i < MAX_EXTENTS; i++) {
	uint64_t suffix = rmem->suffixes[i % rmem->nmems];
	uint64_t offset = 1 + (i / rmem->nmems);
	exttab[i] = (suffix << 48) | (1L << 47) | offset;
    }

    munmap((void *) exttab, PAGE_SIZE);
    close(etfd);

    rmem->fd = open("/dev/dram-cache-mem", O_RDWR);
    if (rmem->fd < 0) {
	perror("open");
	return rmem->fd;
    }

    rmem->mem = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, rmem->fd, 0);
    rmem->npages++;

    if (rmem->mem == MAP_FAILED) {
	perror("mmap");
	return -1;
    }

    // Round-Robin page mappings between extents
    while (rmem->npages * PAGE_SIZE < maxbytes) {
	void *res;
	off_t extoff = (rmem->npages % MAX_EXTENTS) * EXTENT_SIZE;
	off_t pgoff = (rmem->npages / MAX_EXTENTS) * PAGE_SIZE;
	res = mmap(rmem->mem + (PAGE_SIZE * rmem->npages),
			PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED,
			rmem->fd, extoff + pgoff);
	if (res == MAP_FAILED) {
	    perror("mmap");
	    return -1;
	}
	rmem->npages++;
    }

    *mem_base = rmem->mem;

    return 0;
}

void rmem_mmap_close(struct rmem_info *rmem)
{
    munmap(rmem->mem, rmem->npages * PAGE_SIZE);
    close(rmem->fd);
}
