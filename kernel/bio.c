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

#define NBUCKETS 13

struct{
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];
  struct buf hashbucket[NBUCKETS];
  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  // struct buf head;
}bcache ;

void
binit(void)
{
  struct buf *b;
  for (int i = 0; i < NBUCKETS; i++)
  {
    initlock(&bcache.lock[i], "bcache");
    bcache.hashbucket[i].prev = &bcache.hashbucket[i];
    bcache.hashbucket[i].next = &bcache.hashbucket[i];
  }
  // Create linked list of buffers
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.hashbucket[0].next;
    b->prev = &bcache.hashbucket[0];
    initsleeplock(&b->lock, "buffer");
    bcache.hashbucket[0].next->prev = b;
    bcache.hashbucket[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  //锁谁呢？
  int block_num = blockno % NBUCKETS;
  acquire(&bcache.lock[block_num]);
  // Is the block already cached?
  for(b = bcache.hashbucket[block_num].next; b != &bcache.hashbucket[block_num]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[block_num]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock[block_num]);//解锁

  //没找到的情况下，尝试其他的素数buffer桶子
  for(int i=0; i< NBUCKETS;i++){
    acquire(&bcache.lock[i]);
    for(b = bcache.hashbucket[i].next; b != &bcache.hashbucket[i]; b = b->next){
      if(b->dev == dev && b->blockno == blockno){
        b->refcnt++;
        release(&bcache.lock[i]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[i]); //解锁
  }
  

  acquire(&bcache.lock[block_num]);//重新获得锁
  // Not cached; recycle an unused buffer.有限自己的素数桶
  for(b = bcache.hashbucket[block_num].prev; b != &bcache.hashbucket[block_num]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[block_num]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock[block_num]);


  for(int i = 0; i<NBUCKETS;i++){
    acquire(&bcache.lock[i]);
    for(b = bcache.hashbucket[i].prev; b != &bcache.hashbucket[i]; b = b->prev){
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        release(&bcache.lock[i]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[i]);
  }
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

  int hash_block_num = b->blockno % NBUCKETS;

  acquire(&bcache.lock[hash_block_num]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[hash_block_num].next;
    b->prev = &bcache.hashbucket[hash_block_num];
    bcache.hashbucket[hash_block_num].next->prev = b;
    bcache.hashbucket[hash_block_num].next = b;
  }
  
  release(&bcache.lock[hash_block_num]);
}

void
bpin(struct buf *b) {
  int hash_block_num = b->blockno % NBUCKETS;

  acquire(&bcache.lock[hash_block_num]);
  b->refcnt++;
  release(&bcache.lock[hash_block_num]);
}

void
bunpin(struct buf *b) {
  int hash_block_num = b->blockno % NBUCKETS;
  acquire(&bcache.lock[hash_block_num]);
  b->refcnt--;
  release(&bcache.lock[hash_block_num]);
}


