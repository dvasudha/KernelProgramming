/*
 File: scheduler.C

 Author: Vasudha Devarakonda
 Date  :  5 Nov 2023

 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "blocking_disk.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler()
{
  queue_size = 0; // initialise
  Console::puts("Constructed Scheduler.\n");
  this->disk = NULL;
}

void Scheduler::yield()
{
  if (Machine::interrupts_enabled()) // disable interrupts
    Machine::disable_interrupts();
  // Console::puts("Yield. Getting queue size:");
  // Console::puti(disk->block_queue_size);
  // Console::putch('\n');
  if (disk != NULL && disk->is_ready() && disk->block_queue_size > 0)
  {
    Console::puts("READDDDDDDDYYYYYYY");
    Thread *disk_top_thread = disk->block_queue->dequeue();
    disk->block_queue_size--;
    // Thread::dispatch_to(disk_top_thread);
    threads_queue.enqueue(disk_top_thread);
    queue_size++;
  }
  if(queue_size > 0){
  Thread *next_thread = threads_queue.dequeue();
  queue_size--;
  Thread::dispatch_to(next_thread);
  }
  if (!Machine::interrupts_enabled()) // enable interrupts
    Machine::enable_interrupts();
  
}

void Scheduler::resume(Thread *_thread)
{
  if (Machine::interrupts_enabled())
    Machine::disable_interrupts();
  Console::puts("Resume.\n");
  queue_size++;
  threads_queue.enqueue(_thread);
  if (!Machine::interrupts_enabled())
    Machine::enable_interrupts();
}

void Scheduler::add(Thread *_thread)
{
  if (Machine::interrupts_enabled())
    Machine::disable_interrupts();
  Console::puts("Add\n");
  queue_size++;
  threads_queue.enqueue(_thread);
  if (!Machine::interrupts_enabled())
    Machine::enable_interrupts();
}

void Scheduler::terminate(Thread *_thread)
{

  // A->B->C->D-E

  // remove C

  // start rotating

  // B->C->D>-E->A

  // C->D->E->A->B

  // C Found -> delete

  // D->E->A->B deque and enque beyound C

  // A->B->D->E

  if (Machine::interrupts_enabled())
    Machine::disable_interrupts();
  // remove the thread from schedular
  Console::puts("Terminate.\n");
  bool thread_found = false;
  if (Machine::interrupts_enabled())
    Machine::disable_interrupts();
  for (int i = 0; i < queue_size; ++i)
  {
    Thread *temp = threads_queue.dequeue();
    if (temp == _thread)
      thread_found = true;
    else
      threads_queue.enqueue(temp);
  }
  if (thread_found)
    queue_size--;
  if (!Machine::interrupts_enabled())
    Machine::enable_interrupts();
}

void Scheduler::initilase_disk(BlockingDisk *_disk)
{
  Console::puts("Disk has been initialsed");
  disk = _disk;
}
