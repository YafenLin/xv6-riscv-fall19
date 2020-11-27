// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKETS 13   /*素数个哈希桶*/

struct {
  struct buf buf[NBUF]; 
  struct spinlock lock[NBUCKETS];
  struct buf hashbucket[NBUCKETS];  //这里的hashbucket[i]相当于原来的head，这里变成了13个head
 /*每个哈希桶为双向链表加一把lock*/
} bcache;

void
binit(void)
{
  struct buf *b;
  for (int i = 0; i < NBUCKETS; i++)
  {
    initlock(&bcache.lock[i], "bcache");  
  }

  //为每个Hash桶创建循环链表
  for (int i = 0; i < NBUCKETS; i++)
  {
    bcache.hashbucket[i].prev = &bcache.hashbucket[i];
    bcache.hashbucket[i].next = &bcache.hashbucket[i];
  }
  
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    uint hash = (b->blockno)%NBUCKETS;
    b->next = bcache.hashbucket[hash].next;
    b->prev = &bcache.hashbucket[hash];
    initsleeplock(&b->lock, "buffer");
    bcache.hashbucket[hash].next->prev = b;
    bcache.hashbucket[hash].next = b;
    // b->next = bcache.hashbucket[0].next;
    // b->prev = &bcache.hashbucket[0];
    // initsleeplock(&b->lock, "buffer");
    // bcache.hashbucket[0].next->prev = b;
    // bcache.hashbucket[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  //要获取的块应该在哪个hash桶里
  uint hash = blockno%NBUCKETS;

  acquire(&bcache.lock[hash]);

  // Is the block already cached?
  for(b = bcache.hashbucket[hash].next; b != &bcache.hashbucket[hash]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[hash]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // 没有命中，从当前hash桶里寻找没有用过的块
  // Not cached; recycle an unused buffer.
  for(b = bcache.hashbucket[hash].prev; b != &bcache.hashbucket[hash]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[hash]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // release(&bcache.lock[hash]);

  //在本hash桶里没有找到，就从别的hash桶里找
  for (int i = 0; i < NBUCKETS; i++)
  {
    // if(i == hash) continue;
    acquire(&bcache.lock[i]);
    for(b = bcache.hashbucket[i].prev; b != &bcache.hashbucket[i]; b = b->prev){
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        //这里要断开，连接到新的hash桶
        b->prev->next = b->next;
        b->next->prev = b->prev;
        release(&bcache.lock[i]);

        // acquire(&bcache.lock[hash]);
        //连接到新的Hash桶
        b->next = bcache.hashbucket[hash].next;
        b->prev = &bcache.hashbucket[hash];
        bcache.hashbucket[hash].next->prev = b;
        bcache.hashbucket[hash].next = b;
        release(&bcache.lock[hash]);

        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[i]);
  }
  
  //所有的hash桶都满了
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b->dev, b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  uint hash = (b->blockno)%NBUCKETS;
  acquire(&bcache.lock[hash]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[hash].next;
    b->prev = &bcache.hashbucket[hash];
    bcache.hashbucket[hash].next->prev = b;
    bcache.hashbucket[hash].next = b;
  }
  
  release(&bcache.lock[hash]);
}

void
bpin(struct buf *b) {
  uint hash = (b->blockno)%NBUCKETS;
  acquire(&bcache.lock[hash]);
  b->refcnt++;
  release(&bcache.lock[hash]);
}

void
bunpin(struct buf *b) {
  uint hash = (b->blockno)%NBUCKETS;
  acquire(&bcache.lock[hash]);
  b->refcnt--;
  release(&bcache.lock[hash]);
}


