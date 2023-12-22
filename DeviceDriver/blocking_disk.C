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
#include "blocking_disk.H"
#include "scheduler.H"
extern Scheduler *SYSTEM_SCHEDULER;
/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/
BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size)
    : SimpleDisk(_disk_id, _size)
{
  block_queue_size = 0;
  block_queue = new Queue();  // queue to maintain the disk using threads
  block_disk = false; // to check if the disk is blocked 
}

void BlockingDisk::disk_enqueue()
{
  if (block_queue_size > 0)
  {
    Thread *t = block_queue->dequeue();
    block_queue_size--;
    SYSTEM_SCHEDULER->resume(t);
  }
}

bool BlockingDisk::is_ready()
{
  return SimpleDisk::is_ready();
}

  void BlockingDisk::read(unsigned long _block_no, unsigned char *_buf)
  {
    if (block_disk == false)  //if disk is not blocked, no disk operation is to be performed i.e. skipped 
    {
      block_disk = true;
      SimpleDisk::issue_operation(DISK_OPERATION::READ, _block_no);
      if (!is_ready())
      {
        Thread *t = Thread::CurrentThread();
        //Console::puts("Disk is not ready so adding to block queue......\n");
        block_queue->enqueue(t);
        block_queue_size++;
        SYSTEM_SCHEDULER->yield();
        //Console::puts("Lets move to next thread......\n");
      }

      int i;
      unsigned short tmpw;
      for (i = 0; i < 256; i++)
      {
        tmpw = Machine::inportw(0x1F0);
        _buf[i * 2] = (unsigned char)tmpw;
        _buf[i * 2 + 1] = (unsigned char)(tmpw >> 8);
      }
      block_disk = false;
      //Console::putch('\n');
    }
    else{
      Console::puts("Disk is busy....\n");
    }
  }

void BlockingDisk::write(unsigned long _block_no, unsigned char *_buf)
{
  if (block_disk == false) //if disk is not blocked, no disk operation is to be performed i.e. skipped 
  {
    block_disk = true;
    SimpleDisk::issue_operation(DISK_OPERATION::WRITE, _block_no);
    if (!is_ready())
    {
      Thread *t = Thread::CurrentThread();
      block_queue->enqueue(t);
      block_queue_size++;
      SYSTEM_SCHEDULER->yield();
    }
    int i;
    unsigned short tmpw;
    for (i = 0; i < 256; i++)
    {
      tmpw = _buf[2 * i] | (_buf[2 * i + 1] << 8);
      Machine::outportw(0x1F0, tmpw);
    }
    block_disk = false;
  }
  else{
    Console::puts("Disk is busy....\n");
  }
}

void BlockingDisk::handle_interrupt(REGS *_r)
{
  disk_enqueue();  // if interrupt 14 occurs, we are removing the current running thread from blocked queue and appending it back to thread 
}
