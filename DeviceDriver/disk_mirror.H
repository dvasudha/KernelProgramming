/*
     File        : 

     Author      : Vasudha Devarakonda

     Date        :
     Description :

*/

#ifndef _DISK_MIRROR_H_
#define _DISK_MIRROR_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "simple_disk.H"
#include "scheduler.H"
#include "blocking_disk.H"
#include "thread.H"


/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* B l o c k i n g D i s k  */
/*--------------------------------------------------------------------------*/

class DiskMirror : public BlockingDisk
{
private:
    BlockingDisk *master;
    BlockingDisk *slave;
public:
    DiskMirror(DISK_ID _disk_id, unsigned int _size);
    virtual void read(unsigned long _block_no, unsigned char *_buf);
    virtual void write(unsigned long _block_no, unsigned char *_buf);
};

#endif