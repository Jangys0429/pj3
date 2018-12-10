#ifndef VM_PAGE_H
#define VM_PAGE_H

#include "filesys/file.h"
#include <hash.h>

struct spt_entry {
	struct file* file;
	uint32_t offset;
	uint8_t* upage;
	uint32_t read_bytes;
	bool writable;
	struct hash_elem elem;
};

void spt_init(struct hash *spt);
bool spt_insert(struct hash *spt, struct file *file, uint32_t offset, uint8_t *upage, uint32_t read_bytes, bool writable);
struct spt_entry* spt_get_page(struct hash *spt, void * upage);
bool spt_load(struct hash *spt, void * upage);
void spt_delete(struct hash *spt, void * upage);
void spt_destroy(struct hash *spt);

#endif /* userprog/exception.h */
