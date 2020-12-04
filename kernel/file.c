//
// Support functions for system calls that involve file descriptors.
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "stat.h"
#include "proc.h"

struct devsw devsw[NDEV];
struct {
  struct spinlock lock;
  // struct file file[NFILE];
} ftable;

void
fileinit(void)
{
  initlock(&ftable.lock, "ftable");
}

// Allocate a file structure.
// 让我使用bd_malloc？？
  //f 是什么
  // f是一个文件指针，
  // f->ref 保存是否被使用
  // 欧克 理解了 下面一段代码就是循环查找file 找到一个空的文件描述符并返回，无则返回0
  /*
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    if(f->ref == 0){
      f->ref = 1;
      release(&ftable.lock);
      return f;
    }
  }*/

  //怎么用bd_malloc
  //需要传进去什么 一个unit
  //一个文件描述符大小的字节数

  //返回的是什么  一个文件描述符的内存地址
struct file*
filealloc(void)
{
  struct file *f; //文件描述符

  acquire(&ftable.lock);

  uint num = sizeof(struct file);
  f = bd_malloc(num);
  

  if(f){
    f -> ref = 1; //第一次分配空间，ref置1
    release(&ftable.lock);
    return f;
  }
  release(&ftable.lock);
  return 0;
}

// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("filedup");
  f->ref++;
  release(&ftable.lock);
  return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
  // struct file ff;

  //ff是干什么的
  //保存将要释放的文件描述符
  //为什么一开始需要ff？？？

  /*
    因为当该文件描述符需要释放，但是释放操作可能比较耗时，
    用 f 去操作后续操作会很消耗时间，所以重新复刻了一个 ff去操作
  */

  //后来为什么不需要ff？？

  /*
    malloc 分配文件描述符没有限制，不着急释放所以可以操作完了再释放，
    即使新建了一个ff，也还是占用了内存，换汤不换药。
  */
 
  acquire(&ftable.lock);

  if(f->ref < 1)
    panic("fileclose");
  if(--f->ref > 0){
    release(&ftable.lock);
    return;
  }
  //f->ref = 0;
  //放在这里对吗？
  release(&ftable.lock);
  // ff = *f;
  

  if(f->type == FD_PIPE){
    pipeclose(f->pipe, f->writable);
  } else if(f->type == FD_INODE || f->type == FD_DEVICE){
    begin_op(f->ip->dev);
    iput(f->ip);
    end_op(f->ip->dev);
  }
  f->type = FD_NONE;
  bd_free(f);
  
  // 不同的文件所对应的操作 管道、设备
  // if(ff.type == FD_PIPE){
  //   pipeclose(ff.pipe, ff.writable);
  // } else if(ff.type == FD_INODE || ff.type == FD_DEVICE){
  //   begin_op(ff.ip->dev);
  //   iput(ff.ip);
  //   end_op(ff.ip->dev);
  // }
}

// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
int
filestat(struct file *f, uint64 addr)
{
  struct proc *p = myproc();
  struct stat st;
  
  if(f->type == FD_INODE || f->type == FD_DEVICE){
    ilock(f->ip);
    stati(f->ip, &st);
    iunlock(f->ip);
    if(copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
      return -1;
    return 0;
  }
  return -1;
}

// Read from file f.
// addr is a user virtual address.
int
fileread(struct file *f, uint64 addr, int n)
{
  int r = 0;

  if(f->readable == 0)
    return -1;

  if(f->type == FD_PIPE){
    r = piperead(f->pipe, addr, n);
  } else if(f->type == FD_DEVICE){
    r = devsw[f->major].read(1, addr, n);
  } else if(f->type == FD_INODE){
    ilock(f->ip);
    if((r = readi(f->ip, 1, addr, f->off, n)) > 0)
      f->off += r;
    iunlock(f->ip);
  } else {
    panic("fileread");
  }

  return r;
}

// Write to file f.
// addr is a user virtual address.
int
filewrite(struct file *f, uint64 addr, int n)
{
  int r, ret = 0;

  if(f->writable == 0)
    return -1;

  if(f->type == FD_PIPE){
    ret = pipewrite(f->pipe, addr, n);
  } else if(f->type == FD_DEVICE){
    ret = devsw[f->major].write(1, addr, n);
  } else if(f->type == FD_INODE){
    // write a few blocks at a time to avoid exceeding
    // the maximum log transaction size, including
    // i-node, indirect block, allocation blocks,
    // and 2 blocks of slop for non-aligned writes.
    // this really belongs lower down, since writei()
    // might be writing a device like the console.
    int max = ((MAXOPBLOCKS-1-1-2) / 2) * BSIZE;
    int i = 0;
    while(i < n){
      int n1 = n - i;
      if(n1 > max)
        n1 = max;

      begin_op(f->ip->dev);
      ilock(f->ip);
      if ((r = writei(f->ip, 1, addr + i, f->off, n1)) > 0)
        f->off += r;
      iunlock(f->ip);
      end_op(f->ip->dev);

      if(r < 0)
        break;
      if(r != n1)
        panic("short filewrite");
      i += r;
    }
    ret = (i == n ? n : -1);
  } else {
    panic("filewrite");
  }

  return ret;
}

