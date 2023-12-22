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
VMPool *PageTable::VMPoolList_HEAD = NULL; // list of all VMs to maintain

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

    unsigned long *page_table = (unsigned long *)((kernel_mem_pool->get_frames(1)) * 4096);
    page_directory = (unsigned long *)(process_mem_pool->get_frames(1) * 4096);
    // Console::putui((unsigned int)page_directory);
    // Console::putui((unsigned int)page_table);
    unsigned long address = 0; // holds the physical address of where a page is
    unsigned int i;
    page_directory[1023] = (unsigned long)(page_directory) | 3; // Recursive page lookup
    // map the first 4MB of memory -> 4MB/4KB = 1024 entries -> this is for first _shared_memory and all is set to present because they are directly mapped
    for (i = 0; i < 1024; i++)
    {
        page_table[i] = address | 3; // attribute set to: supervisor level, read/write, present(011 in binary), this is for shared memory
        address = address + 4096;    // 4096 = 4kb
    };
    page_directory[0] = (unsigned long)page_table;
    page_directory[0] = page_directory[0] | 3;
    for (i = 1; i < 1023; i++)
    {
        page_directory[i] = 2; // attribute set to: supervisor level, read/write, not present(010 in binary)
    };
    page_directory[1023] = (unsigned long)page_directory| 3;
    paging_enabled = 0; // we will load our page directory before enabling paging
}

void PageTable::load()
{
    Console::putui((unsigned int)page_directory);
    current_page_table = this;                                    // load current page table
    write_cr3((unsigned long)current_page_table->page_directory); // load page_directory to CR3
    Console::putui((unsigned int)read_cr3());
    Console::putui((unsigned int)current_page_table->ENTRIES_PER_PAGE);
}

void PageTable::enable_paging()
{
    Console::puts("Enabling paging......");
    write_cr0(read_cr0() | 0x80000000); // bit 31 of CR0 needs to be set
    paging_enabled = 1;
}

void PageTable::handle_fault(REGS *_r)
{

    unsigned long *page_directory_current = (unsigned long *) 0xFFFFF000; //1023-1023-X 
    // Console::putui((unsigned long) page_directory_current);
    // 32-bit address of the address that caused the page fault is stored in register CR2,
    unsigned long add_fault = read_cr2(); // virtual address, 10-10-12 bits
    // Console::putui(add_fault);
    	/*check if address is legitimate*/
	unsigned int addr_present = 0;
	VMPool *ptr = PageTable::VMPoolList_HEAD;
	for(;ptr!=NULL;ptr=ptr->vm_pool_next_ptr)
	{
		if(ptr->is_legitimate(add_fault) == true)
		{
			addr_present = 1;
		    break;
		}
	}
	
	if(addr_present == 0 && ptr!= NULL)
	{
      Console::puts("INVALID ADDRESS \n");
      assert(false);	  	
	}


    unsigned long pd_addr = (unsigned long) (add_fault &  0xFFC00000) >> 22;  // fetching offset for X 
    unsigned long pt_addr = (unsigned long) (add_fault >> 12) & 0x03FF; // Y -> offset within PD
    unsigned long *page_table;
    unsigned long *entry_frame;
    //
    if ((page_directory_current[pd_addr] & 1) == 0) // page table exists ? No:Yes
    {
        // getting the memeory space for the table which takes one kernel frame
        page_table = (unsigned long *)((process_mem_pool->get_frames((unsigned int)1)) * 4096);
        page_directory_current[pd_addr] = (unsigned long)page_table; // make an entry in page_directory
        // make an entry to page directory and then set the present bit of the directory
        page_directory_current[pd_addr] = page_directory_current[pd_addr] | 3; // set it to present
        // make entries of table as not-present
        for (unsigned long int i = 0; i < 1024; i++)
        {
            page_table[i] = 6; // attribute set to: user level, read/write, not present(110 in binary)
        }
    }

    page_table = (unsigned long *) (0xFFC00000 | (pd_addr << 12)); // get base of page table 1023-X offset Y
    if ((page_table[pt_addr] & 1) == 0)
    {
        entry_frame = (unsigned long *)((process_mem_pool->get_frames(1)) * 4096);
        page_table[pt_addr] = (unsigned long)entry_frame;
        page_table[pt_addr] = page_table[pt_addr] | 7; // last 3 bits as 111 (user,r/w,present)
    }
}

void PageTable::register_pool(VMPool *_vm_pool)
{
    

    if (PageTable::VMPoolList_HEAD == NULL)
    {
        PageTable::VMPoolList_HEAD = _vm_pool;
    }
    else
    {
        VMPool *ptr = PageTable::VMPoolList_HEAD;
        while (ptr->vm_pool_next_ptr != NULL)
        {
            ptr = ptr->vm_pool_next_ptr;
        }

        ptr->vm_pool_next_ptr = _vm_pool;
    }

    Console::puts("registered VM pool\n");
}

void PageTable::free_page(unsigned long _page_no)
{
    // X(10 bits)--Y(10 bits)--Offset(12 bits)
    unsigned long pd_index_free = (unsigned long) (_page_no & 0xFFC00000) >> 22;   // X
    unsigned long * page_table = (unsigned long *) (0xFFC00000 | (pd_index_free << 12));   // Adress <1023(10 bits)><X><pt_index_free> -> this will give offset>
    //Console::puti((unsigned int)page_table);
    unsigned long pt_index_free = (unsigned long) (_page_no >> 12)&0x3FF; // Y -> offset within page table to fetch frame number
    //Console::puti((unsigned int)pt_index_free);
    //Console::puts("\n");
    unsigned long frame_no = (page_table[pt_index_free]) / Machine::PAGE_SIZE;
    process_mem_pool->release_frames(frame_no);
    page_table[pt_index_free] = 0;
    load(); // relaoding CR3, flushes out TLB 
 
    Console::puts("free page\n");
}
