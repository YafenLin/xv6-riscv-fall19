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

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

void
kinit()
{
  for (int i = 0; i < NCPU; i++)
  {
    //为所有CPU分配锁
    initlock(&kmem[i].lock, "kmem");
  }
  //只有CPU0分配了内存
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
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

  r = (struct run*)pa;
  //获取当前CPU的id
  push_off();
  int cpuId = cpuid();
  pop_off();
  acquire(&kmem[cpuId].lock);
  r->next = kmem[cpuId].freelist;
  kmem[cpuId].freelist = r;
  release(&kmem[cpuId].lock);
  
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  //关中断
  push_off();
  //获取当前CPU的id
  int cpuId = cpuid();
  pop_off();

  acquire(&kmem[cpuId].lock);
  r = kmem[cpuId].freelist;
  if(r){
    kmem[cpuId].freelist = r->next;
    release(&kmem[cpuId].lock);
  }
  else
  {
    release(&kmem[cpuId].lock);
    //窃取其他CPU的内存
    int i=0;
    while(i<NCPU){      
      //获取其他选中的CPU的锁
      acquire(&kmem[i].lock);
      r = kmem[i].freelist;
      //如果选中的CPU有内存
      if(r){
        kmem[i].freelist = r->next;
        release(&kmem[i].lock);
        break;
      }
      release(&kmem[i].lock);
      i++;
    }
  }
  

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;

}
