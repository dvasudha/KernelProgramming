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
}

void Scheduler::yield()
{
  if (Machine::interrupts_enabled())   //disable interrupts
    Machine::disable_interrupts();
  Console::puts("Yield.\n");
  if (queue_size != 0)
  {
    queue_size--;
    Thread *next_thread = threads_queue.dequeue();
    Thread::dispatch_to(next_thread);
  }
  if (!Machine::interrupts_enabled())  //enable interrupts 
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

RRScheduler::RRScheduler()
{
  hz = 5;
  queue_size = 0; // initialise
  ticks = 0;
  set_frequency(hz); // implementation reference -> simple_timer.C
  Console::puts("Constructed RRScheduler.\n"); 
}


void RRScheduler::set_frequency(int _hz) // implementation reference -> simple_timer.C
{
  /* Set the interrupt frequency for the simple timer.
     Preferably set this before installing the timer handler!                 */

  hz = _hz;                                /* Remember the frequency.           */
  int divisor = 1193180 / _hz;             /* The input clock runs at 1.19MHz   */
  Machine::outportb(0x43, 0x34);           /* Set command byte to be 0x36.      */
  Machine::outportb(0x40, divisor & 0xFF); /* Set low byte of divisor.          */
  Machine::outportb(0x40, divisor >> 8);   /* Set high byte of divisor.         */
}

void RRScheduler::handle_interrupt(REGS *_r) // implementation reference -> simple_timer.C
{
  ticks++;
  if (ticks >= hz)
  {
    ticks =0;
    Console::puts("Time Quantum limit\n");
    resume(Thread::CurrentThread());
    yield();
  }
}

void RRScheduler::yield()
{
  Machine::outportb(0x20, 0x20); // EOI
  if (Machine::interrupts_enabled())
    Machine::disable_interrupts();
  Console::puts("Yield.\n");
  if (queue_size != 0)
  {
    queue_size--;
    ticks = 0; /*The 'yield' function must be modified to account for unused quantum
         time. If a thread voluntarily yields, the EOQ timer must be reset in order
         to not penalize the next thread.*/
    Console::puts("tock made to 0");
    Thread *next_thread = threads_queue.dequeue();
    Thread::dispatch_to(next_thread);
  }

    if (!Machine::interrupts_enabled())
    Machine::enable_interrupts();

}

// terminate and resume is same as FIFO
void RRScheduler::resume(Thread *_thread)
{
  if (Machine::interrupts_enabled())
    Machine::disable_interrupts();
  Console::puts("Resume.\n");
  queue_size++;
  threads_queue.enqueue(_thread);
  if (!Machine::interrupts_enabled())
    Machine::enable_interrupts();
}

void RRScheduler::add(Thread *_thread)
{
  Console::puts("Thread\n");
  Console::putui(_thread->ThreadId());
  if (Machine::interrupts_enabled())
    Machine::disable_interrupts();
  Console::puts("AddRRSchedule\n");
  queue_size++;
  threads_queue.enqueue(_thread);
  if (!Machine::interrupts_enabled())
    Machine::enable_interrupts();
}

void RRScheduler::terminate(Thread *_thread)
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
  Thread *temp;
  if (Machine::interrupts_enabled())
    Machine::disable_interrupts();
  for (int i = 0; i < queue_size; ++i)
  {
    temp = threads_queue.dequeue();
    if (temp == _thread)
      thread_found = true;
    else
      threads_queue.enqueue(temp);
  }
  if (thread_found)
  {
    queue_size--;
  }
  if (!Machine::interrupts_enabled())
    Machine::enable_interrupts();
}
