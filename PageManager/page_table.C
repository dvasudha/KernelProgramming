#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable *PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool *PageTable::kernel_mem_pool = NULL;
ContFramePool *PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;

void PageTable::init_paging(ContFramePool *_kernel_mem_pool,
                            ContFramePool *_process_mem_pool,
                            const unsigned long _shared_size)
{
   kernel_mem_pool = _kernel_mem_pool;
   process_mem_pool = _process_mem_pool;
   shared_size = _shared_size;
}

PageTable::PageTable()
{
   
   page_directory = (unsigned long *)(kernel_mem_pool->get_frames(1) * 4096);
   unsigned long *page_table = (unsigned long *)((kernel_mem_pool->get_frames(1)) * 4096);  
   // Console::putui((unsigned int)page_directory);
   // Console::putui((unsigned int)page_table);
   unsigned long address = 0; // holds the physical address of where a page is
   unsigned int i;

   // map the first 4MB of memory -> 4MB/4KB = 1024 entries -> this is for first _shared_memory and all is set to present because they are directly mapped 
   for (i = 0; i < 1024; i++)
   {
      page_table[i] = address | 3; // attribute set to: supervisor level, read/write, present(011 in binary), this is for shared memory
      address = address + 4096;    // 4096 = 4kb
   };
   page_directory[0] = (unsigned long)page_table;
   page_directory[0] = page_directory[0] | 3;
   for (i = 1; i < 1024; i++)
   {
      page_directory[i] = 2; // attribute set to: supervisor level, read/write, not present(010 in binary)
   };
   paging_enabled = 0;  // we will load our page directory before enabling paging 
}

void PageTable::load()
{
   Console::putui((unsigned int)page_directory);
   current_page_table = this;  // load current page table 
   write_cr3((unsigned long)current_page_table->page_directory); // load page_directory to CR3
   Console::putui((unsigned int)read_cr3());
   Console::putui((unsigned int)current_page_table->ENTRIES_PER_PAGE);
}

void PageTable::enable_paging()
{
   Console::puts("Enabling paging......");
   write_cr0(read_cr0() | 0x80000000); //bit 31 of CR0 needs to be set 
   paging_enabled = 1;
}

void PageTable::handle_fault(REGS *_r)
{

   // reading directory from cr3 i.e the current PD
   unsigned long *page_directory_current = (unsigned long *)read_cr3();  //load() -> directs to the active page direvtory
   // Console::putui((unsigned long) page_directory_current);
   // 32-bit address of the address that caused the page fault is stored in register CR2,
   unsigned long add_fault = read_cr2(); // virtual address, 10-10-12 bits
   // Console::putui(add_fault);
   unsigned long pd_addr = add_fault >> 22;                    // shifting by 10+12
   unsigned long pt_addr = (add_fault >> 12) & 0x03FF; // this will give the PT address  10-10-12 and i have to extract the 10 bits so first performed right shift operation by 12 and then mask the remaining 20 bits by performing and operation
   // Console::putui(pd_addr);
   // Console::putui(pt_addr);
   unsigned long *page_table;
   unsigned long *entry_frame;
   //
   if ((page_directory_current[pd_addr] & 1) == 0)  // page table exists ? No:Yes
   {
      // getting the memeory space for the table which takes one kernel frame
      page_table = (unsigned long *)((kernel_mem_pool->get_frames((unsigned int)1)) * 4096);
      page_directory_current[pd_addr] = (unsigned long)page_table;  //make an entry in page_directory
      // make an entry to page directory and then set the present bit of the directory
      page_directory_current[pd_addr] = page_directory_current[pd_addr] | 3;  // set it to present 
      // make entries of table as not-present
      for (unsigned long int i = 0; i < 1024; i++)
      {
         page_table[i] = 6; // attribute set to: user level, read/write, not present(110 in binary)
      }
   }

   // getting the page_table for that index-> present or not in PD. if not present above code block made entry to PD.

   page_table = (unsigned long *)(page_directory_current[pd_addr] & ~0xFFF);
   if ((page_table[pt_addr] & 1) == 0)
   {
      entry_frame = (unsigned long *)((process_mem_pool->get_frames(1)) * 4096);
      page_table[pt_addr] = (unsigned long)entry_frame;
      page_table[pt_addr] = page_table[pt_addr] | 7; // last 3 bits as 111 (user,r/w,present)
   }
}
