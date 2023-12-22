/*
 File: vm_pool.C

 Author:
 Date  :

 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
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
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long _base_address,
               unsigned long _size,
               ContFramePool *_frame_pool,
               PageTable *_page_table)
{
    base_address = _base_address;
    size = _size;
    frame_pool = _frame_pool;
    page_table = _page_table;
    updated_size = size;
    page_table->register_pool(this); // registering the pool with the page table
    vm_regions head_region = vm_regions(base_address, Machine::PAGE_SIZE);
    regions = &head_region;
    updated_size = updated_size - Machine::PAGE_SIZE;
    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size)
{
    // allocate address in virtual memeory space
    if (_size > updated_size)
    {
        Console::puts("Size is less than the available.\n");
        assert(false);
    }
    vm_regions *_regions = regions;
    while (_regions->next_region != NULL)
    {
        _regions = _regions->next_region;
    }
    unsigned long num_page = (_size / Machine::PAGE_SIZE) + ((_size % Machine::PAGE_SIZE) > 0 ? 1 : 0);
    unsigned long next_base_address = regions->base_address + regions->length;
    unsigned long next_length = num_page * Machine::PAGE_SIZE;
    vm_regions allocated_region = vm_regions(next_base_address, next_length);
    _regions->next_region = &allocated_region;
    updated_size = updated_size - next_length;
    Console::puts("Allocated region of memory.\n");
    return next_base_address;
}

void VMPool::release(unsigned long _start_address)
{
    vm_regions *_regions = regions;
    int found = 0;
    vm_regions *prev = nullptr;
    while (_regions->next_region != NULL)
    {

        if (_regions->base_address == _start_address)
        {
            found = 1;
            break;
        }
        prev = _regions;
        _regions = _regions->next_region;
    }

    if (found == 0)
    {
        Console::puts("This start address does not exist.\n");
        assert(false);
    }

    unsigned long num_pages = (_regions->length) / Machine::PAGE_SIZE;
    prev->next_region = _regions->next_region; // remove the deallocated region
    updated_size = updated_size + _regions->length;

    while (num_pages > 0)
    {
        page_table->free_page(_start_address);
        num_pages--;
        _start_address = _start_address + Machine::PAGE_SIZE;
    }

    Console::puts("Released region of memory.\n");
}

bool VMPool::is_legitimate(unsigned long _address)
{

    if ((_address > (base_address + size)) || (_address < base_address)) // should be in the range of _base_address and limit to size
        return false;
    return true;
    Console::puts("Checked whether address is part of an allocated region.\n");
}
