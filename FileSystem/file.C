/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file.H"
#include "file_system.H"
extern FileSystem *FILE_SYSTEM;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File(FileSystem *_fs, int _id)
{
    Console::puts("Opening file.\n");
    file_id = _id;
    size = 0;
    position = 0;
    inode = _fs->LookupFile(file_id);
    my_block_number = inode->block_no;
    Console::puts("My block number as assigned by file system");
    Console::puti(my_block_number);
    Console::putch('\n');
    inode->file = this;
    memset(block_cache, 0, SimpleDisk::BLOCK_SIZE);
}

File::~File()
{
    Console::puts("Closing file.\n");
    // FILE_SYSTEM->disk->write(current_block, block_cache);
    position = 0;
    // empty the blocks assigned to the file
    /* Make sure that you write any cached data to disk. */
    /* Also make sure that the inode in the inode list is updated. */
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char *_buf)
{

    int my_blocks = (_n + SimpleDisk::BLOCK_SIZE - 1) / SimpleDisk::BLOCK_SIZE;
    int read_size = 0;
    int i = my_blocks;
    int start_block = my_block_number;
    while (i >0)
    {
        if (start_block <= 1)
        {
            Console::puts("File not initialized\n");
            assert(false)
        }
        memset(buf, 0, SimpleDisk::BLOCK_SIZE);
        FILE_SYSTEM->disk->read(start_block, buf);
        position = 0;
        while (!EoF() && _n >0)
        {
            _buf[read_size++] = (char)buf[position++];
            _n--;
        }
        start_block ++;
        i--;
    }
    Console::puts("\nRead bytes:");
    Console::puti(read_size);
    Console::puts("\n");
    return read_size;
}
int File::shift_after_write(int offset)
{
    int initial_number = 0x80;
    int result = initial_number >> offset % 8;
    return result;
}
int File::Write(unsigned int _n, const char *_buf)
{
    int block = 0;
    int number_blocks = (_n + SimpleDisk::BLOCK_SIZE - 1) / SimpleDisk::BLOCK_SIZE;
    Console::puts("\n");
    int i = my_block_number;
    while (number_blocks > 0)
    {
        if (i <= 1)
        {
            Console::puts("File not initialized\n");
            assert(false)
        }
        position = 0;
        memset(buf, 0, SimpleDisk::BLOCK_SIZE);
        while (position < SimpleDisk::BLOCK_SIZE & _n > 0)
        {
            buf[position++] = _buf[block++];
            if (position == SimpleDisk::BLOCK_SIZE || _n <=0)
                    break;
            _n--;
        }
        FILE_SYSTEM->disk->write(i, buf);
        FILE_SYSTEM->free_data_block[i / 8] = FILE_SYSTEM->free_data_block[i / 8] | shift_after_write(i);
        FILE_SYSTEM->disk->write(1, FILE_SYSTEM->free_data_block);
        i++;
        number_blocks--;
    }
    return block;
}
void File::Reset()
{
    Console::puts("resetting file\n");
    position = 0;
}

bool File::EoF()
{
    return position >= SimpleDisk::BLOCK_SIZE;
}
