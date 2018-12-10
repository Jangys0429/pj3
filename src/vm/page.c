#include "vm/page.h"
#include <inttypes.h>
#include <stdio.h>
#include <hash.h>
#include <stdlib.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "userprog/syscall.h"
#include "userprog/exception.h"
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
	hash_init(spt, &hash_func, &hash_less, NULL);
}

bool
spt_insert(struct hash* spt, struct file *file, uint32_t offset, uint8_t *upage,
	uint32_t read_bytes, bool writable) {
	struct spt_entry* e = malloc(sizeof(struct spt_entry));
	e->file = file;
	e->offset = offset;
	e->upage = upage;
	e->read_bytes = read_bytes;
	e->writable = writable;

	lock_acquire(&thread_current()->spt_lock);
	if(hash_insert(&spt, &e->elem) == NULL){
		lock_release(&thread_current()->spt_lock);
		return false;
	}

	lock_release(&thread_current()->spt_lock);
	return true;
}

struct spt_entry*
spt_get_page(struct hash* spt, void* upage) {
	struct spt_entry temp;
	struct hash_elem e;
	temp.upage = upage; 

	if (e = hash_find(&spt, &temp->elem) != NULL)
		return hash_entry(e, struct spt_entry, elem);
	else
		return NULL;

}

//lazy load
bool
spt_load(struct hash* spt, void* upage) {
	struct spt_entry *e;
	uint8_t *kpage;

	e = spt_get_page(&thread_current()->spt, upage);
	if (e == NULL)
		eturn false;
	
	file_seek(e->file, e->offset);
	
	//get physical memory space
	kpage = palloc_get_page(PAL_USER);
 	if (kpage =r= NULL) {
		frame_lock_acquire();
		kpage = frame_find_to_evict();
		frame_lock_release();
	}

	//copy file from there
	if(file_read (e->file, kpage, e->read_bytes) != (int) e->read_bytes){
		palloc_free_page (kpage);
         	return false; 
	}
	memset (kpage + e->read_bytes, 0, PGSIZE - e->read_bytes);


	//map page and frame and add to the frame table
	if (!install_page (e->upage, kpage, e->writable)) 
        {
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




