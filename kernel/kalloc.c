// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem {
  struct spinlock lock;
  struct run *freelist;
};

struct kmem kmems[NCPU];



//初始化锁
void
kinit()
{
  for(int i=0;i<NCPU;i++){
    initlock(&kmems[i].lock, "kmem");
  }
  freerange(end, (void*)PHYSTOP);
}

//freerange为所有运行freerange的CPU分配空闲的内存；
void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  //保证4k对齐
  p = (char*)PGROUNDUP((uint64)pa_start);
  
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);//调用kfree()将页面从头部插入到链表kmems[].freelist中进行管理。
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  push_off();
  int current_cpu_id = cpuid();
  pop_off();

  r = (struct run*)pa;
  acquire(&kmems[current_cpu_id].lock);
  r->next = kmems[current_cpu_id].freelist;
  kmems[current_cpu_id].freelist = r;
  release(&kmems[current_cpu_id].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  push_off();
  int current_cpu_id = cpuid();
  pop_off();

  acquire(&kmems[current_cpu_id].lock);
  r = kmems[current_cpu_id].freelist;
  if(r)
    kmems[current_cpu_id].freelist = r->next;
  release(&kmems[current_cpu_id].lock);
  
  if(!r){
    for(int i=0;i<NCPU;i++){
        acquire(&kmems[i].lock);
        r = kmems[i].freelist;
        if(r)
          kmems[i].freelist = r->next;
        release(&kmems[i].lock);
        if(r) break;
    }
  }



  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
