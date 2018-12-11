#include "vm/page.h"
#include <inttypes.h>
#include <stdio.h>
#include <hash.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "filesys/file.h"
#include "userprog/syscall.h"
#include "userprog/exception.h"
#include "userprog/process.h"
#include "vm/frame.h"

static unsigned
hash_func(const struct hash_elem *e, void *aux) {
	return (unsigned)hash_entry(e, struct spt_entry, elem)->upage;
}

static bool
hash_less(const struct hash_elem *a, const struct hash_elem *b,	void *aux) {
	return (unsigned)hash_entry(a, struct spt_entry, elem)->upage < (unsigned)hash_entry(b, struct spt_entry, elem)->upage;
}

void
spt_init (struct hash* spt) {
	hash_init(spt, hash_func, hash_less, NULL);
}

bool
spt_insert(struct hash* spt, struct file *file, uint32_t page_offset, uint8_t *upage,
	uint32_t read_bytes, bool writable, uint32_t ofs) {



	struct spt_entry* e = malloc(sizeof(struct spt_entry));
	e->file = file;
	e->page_offset = page_offset;
	e->ofs = ofs;
	e->upage = upage;
	e->read_bytes = read_bytes;
	e->writable = writable;

	//printf("spt_insert ofs: %d\n", ofs);

	lock_acquire(&thread_current()->spt_lock);
	if(hash_insert(spt, &e->elem) != NULL){
		lock_release(&thread_current()->spt_lock);
		return false;
	}

	lock_release(&thread_current()->spt_lock);
	return true;
}

struct spt_entry*
spt_get_page(struct hash* spt, void* upage) {
	struct spt_entry temp;
	struct hash_elem *e;
	temp.upage = upage; 

	e = hash_find(spt, &temp.elem);

	if (e != NULL)
		return hash_entry(e, struct spt_entry, elem);
	else
		return NULL;

}

//lazy load
bool
spt_load(struct hash* spt, void* upage) {
	struct spt_entry *e;
	uint8_t *kpage;

	e = spt_get_page(spt, upage);
	
	if (e == NULL)
		return false;
	//printf("file position: %d\n\n\n", file_tell(e->file));

	file_seek(e->file, e->page_offset + e->ofs);
	//printf("file position: %d\n", file_tell(e->file));

	//get physical memory space
	kpage = palloc_get_page(PAL_USER | PAL_ZERO);
 	if (kpage == NULL) {
		frame_lock_acquire();
		kpage = frame_find_to_evict();
		frame_lock_release();
	}

	//copy file from there
	
	if(file_read (e->file, kpage, e->read_bytes) != (int) e->read_bytes){
		palloc_free_page (kpage);
         	return false; 
	}
	
	//printf("Read bytes: %d\n", e->read_bytes);
	
	memset (kpage + e->read_bytes, 0, PGSIZE - e->read_bytes);

	
	
	if (!install_page (e->upage, kpage, e->writable)) {
          palloc_free_page (kpage);
          return false; 
        }

	
	return true;
}

//
void
spt_delete(struct hash* spt,void* upage) {

}

void
spt_destroy(struct hash* spt) {

}





