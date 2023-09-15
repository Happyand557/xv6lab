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

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;

struct bucket {
  struct spinlock lock;
  struct buf* buf;
};
struct bucket bhash[NSLOT];
void
binit(void)
{

  initlock(&bcache.lock, "bcache");
  for(int i = 0; i < NSLOT; i++) {
    initlock(&bhash[i].lock, "bcache.bucket");
  }
  for(struct buf *b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = 0;
    b->prev = 0;
    initsleeplock(&b->lock, "buffer");
  }
  /*
  // Create linked list of buffers
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  */
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  /*
  for(int i = 0; i < NSLOT; i++) {
    struct buf *b = bhash[i].buf;
    printf("hash slot %d:\n", i);
    while(b) {
      printf("%d\n", b->blockno);
      b = b->next;
    }
  }
  */
  // Is the block already cached?
  uint id = blockno % NSLOT;
  acquire(&bhash[id].lock);
  struct buf *biter = bhash[id].buf; 
  while(biter != 0) {
    if(biter->blockno == blockno && biter->dev == dev) {
      biter->refcnt++;
      release(&bhash[id].lock);
      acquiresleep(&biter->lock);
      return biter;
    }
    biter = biter->next;
  } 
  release(&bhash[id].lock);

  acquire(&bcache.lock);
  for(int i = 0; i < NBUF; i++) {
     if(bcache.buf[i].refcnt == 0) {
        bcache.buf[i].dev = dev;
        bcache.buf[i].blockno = blockno;
        bcache.buf[i].valid = 0;
        bcache.buf[i].refcnt = 1;
        //添加到新slot中
        id = blockno % NSLOT;
        acquire(&bhash[id].lock);
        biter = bhash[id].buf;
        if(biter == 0) {
          bhash[id].buf = &bcache.buf[i];
          bhash[id].buf->next = 0;
        } else {
          while(biter->next){
            biter = biter->next;
          }
          biter->next = &bcache.buf[i];
          biter->next->next = 0;
        }
        release(&bcache.lock);
        release(&bhash[id].lock);
        acquiresleep(&bcache.buf[i].lock);
        return &bcache.buf[i];
      }
    }
  release(&bcache.lock);
  /*
  acquire(&bcache.lock);
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  */
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  /*
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  */
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
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
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int id = b->blockno % NSLOT;
  acquire(&bhash[id].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    int no1 = b->blockno;
    int dev1 = b->dev;
    struct buf *pre = bhash[id].buf;
    if(pre != 0) {
      struct buf *next = pre->next;
      if(pre->dev == dev1 && pre->blockno == no1) {
        bhash[id].buf = next;
      } else {
        while(next) {
          if(next->dev == dev1 && next->blockno == no1) {
            pre->next = next->next;
            break;
          }
          pre = pre->next;
          next = next->next;
        }
      }
    } 
  }
  release(&bhash[id].lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


