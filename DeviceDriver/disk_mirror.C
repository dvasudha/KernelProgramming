/*
     File        :

     Author      : Vasudha Devarakonda
     Modified    :

     Description :

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "disk_mirror.H"
#include "scheduler.H"
#include "blocking_disk.H"
#define DISK_BLOCK_SIZE ((1 KB) / 4)
extern Scheduler *SYSTEM_SCHEDULER;
/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/
DiskMirror::DiskMirror(DISK_ID _disk_id, unsigned int _size) : BlockingDisk(_disk_id, _size)
{
  master = new BlockingDisk(DISK_ID::MASTER, _size);

  slave = new BlockingDisk(DISK_ID::DEPENDENT, _size);
}

void DiskMirror::read(unsigned long _block_no, unsigned char *_buf) // check which disk is ready, master or slave and then perform read operation on the disk which is ready first 
{
  Console::puts("Reading in disk mirror...");
  bool master_ready = master->is_ready() && !master->block_disk;
  bool slave_ready = slave->is_ready() && !slave->block_disk;
  while (master_ready || slave_ready)
  {
    if (master_ready)
    {
      master->read(_block_no, _buf);
    }
    else if (slave_ready)
    {
      slave->read(_block_no, _buf);
    }
    else
    {
      Thread *t = Thread::CurrentThread();
      master->block_queue->enqueue(t);
      slave->block_queue->enqueue(t);
      master->block_queue_size++;
      slave->block_queue_size++;
      
    }
  }
}

void DiskMirror::write(unsigned long _block_no, unsigned char *_buf)  // write to both slave and master disks 
{
  Console::puts("Writing to slave and master............");
  master->write(_block_no, _buf);
  slave->write(_block_no, _buf);
}
