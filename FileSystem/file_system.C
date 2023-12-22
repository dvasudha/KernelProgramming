/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/
#define MB *(0x1 << 20)
#define KB *(0x1 << 10)
#include "assert.H"
#include "console.H"
#include "simple_disk.H"
#include "file_system.H"
#define SYSTEM_DISK_SIZE (10 MB);
unsigned long FileSystem::current_block = 2;
Inode *FileSystem::head_pointer = nullptr;
Inode *FileSystem::tail_pointer = nullptr;
unsigned char *FileSystem::super_block = new unsigned char[SimpleDisk::BLOCK_SIZE];
unsigned char *FileSystem::free_data_block = new unsigned char[SimpleDisk::BLOCK_SIZE];
/*--------------------------------------------------------------------------*/
/* CLASS Inode */
/*--------------------------------------------------------------------------*/

/* You may need to add a few functions, for example to help read and store
   inodes from and to disk. */

/*--------------------------------------------------------------------------*/
/* CLASS FileSystem */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/
Inode::Inode()
{
    id = NULL;
    next = NULL;
    file = NULL;
}
FileSystem::FileSystem()
{
    file_count = 0;
    head_pointer = NULL;  // Inode is maintained using linked list 
    tail_pointer = NULL;
}

FileSystem::~FileSystem()
{
    Console::puts("unmounting file system\n");
    disk = NULL;   // Deleting file system -> remove the disk 
}

/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/

bool FileSystem::Mount(SimpleDisk *_disk)
{
    Console::puts("mounting file system\n");
    disk = _disk;   // Assign disk to the argument disk 
    return true;
}

int FileSystem::shift_after_write(int offset)
{
    int initial_number = 0x80;
    int result = initial_number >> offset % 8;
    return result;
}

bool FileSystem::Format(SimpleDisk *_disk, unsigned int _size)
{ // static!
    Console::puts("formatting disk\n");  
    unsigned int numBlocks = _size / SimpleDisk::BLOCK_SIZE;
    for (int i = 0; i < SimpleDisk::BLOCK_SIZE; i++)
    {
        free_data_block[i] = 0x0;
    }
    _disk->write(0, super_block);   // 
    free_data_block[0 / 8] = free_data_block[0 / 8] | shift_after_write(0); // 1000000000000 -> free block bitmap
    free_data_block[1 / 8] = free_data_block[1 / 8] | shift_after_write(1); // 1100000000000 -> free block bitmap updated
    _disk->write(1, free_data_block);                                       // 2nd block is reserved by bitmap for free data regions
    // please note that free block for inode is not maintained because we are mainiting inode as a linked list and not an array
    return true;
}

Inode *FileSystem::LookupFile(int _file_id)
{
    Console::puts("looking up file with id = ");
    Console::puti(_file_id);
    Console::puts("\n");
    if (head_pointer == NULL)
        return NULL;
    Inode *current = head_pointer;
    while (current != NULL)
    {
        if (current->id == _file_id)
            return current;
        current = current->next;
    }
    return NULL;
}

bool FileSystem::CreateFile(int _file_id)
{
    Console::puts("creating file with id:");
    Console::puti(_file_id);
    Console::puts("\n");

    if (this->LookupFile(_file_id))
        return false;

    Inode *new_inode = new Inode();
    if (head_pointer == NULL)
    {

        head_pointer = tail_pointer = new_inode;
        head_pointer->id = _file_id;
        head_pointer->next = NULL;
    }
    else
    {
        tail_pointer->next = new_inode;
        tail_pointer = tail_pointer->next;
        tail_pointer->id = _file_id;
    }
    tail_pointer->block_no = current_block;
    current_block = current_block + 128;
    // Console::puts("Printing current block");
    // Console::puti(current_block);
    // Console::puts("\n");
    if (current_block > (10 * 1024 * 1024) / SimpleDisk::BLOCK_SIZE)
    {
        current_block = 2;
    }
    return true;
}

bool FileSystem::DeleteFile(int _file_id)
{
    Console::puts("Deleting file with ID:");
    Console::puti(_file_id);
    Console::puts("\n");
    Inode *fileToDelete = this->LookupFile(_file_id);

    if (fileToDelete == NULL)
    {
        Console::puts("File does not exist.\n");
        return false;
    }
    else
    {
        Console::puts("Existing file found.\n");

        Inode *current = head_pointer;
        Inode *prev = NULL;

        // Locate the file by its ID in the inode list
        while (current != NULL && current->id != _file_id)
        {
            prev = current;
            current = current->next;
        }

        // If the file is the first node in the list
        if (prev == NULL)
        {
            head_pointer = current->next;
        }
        else if (current != NULL)
        {
            prev->next = current->next;
        }

        Console::puts("File deleted successfully.\n");
        return true;
    }
}
