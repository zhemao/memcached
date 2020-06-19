#define MAX_REMOTE_MEMS 16
#define MAX_EXTENTS 16
#define PAGE_SIZE 4096
#define EXTENT_SIZE (1L << 30)

struct rmem_info {
    int suffixes[MAX_REMOTE_MEMS];
    char *mem;
    size_t npages;
    int nmems;
    int fd;
};

void rmem_init(struct rmem_info *rmem);
void rmem_add_suffix(struct rmem_info *rmem, int suffix);
int rmem_mmap_open(size_t maxbytes, struct rmem_info *rmem, void **mem_base);
void rmem_mmap_close(struct rmem_info *rmem);
