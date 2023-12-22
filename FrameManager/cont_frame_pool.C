/*
 File: ContFramePool.C

 Author:  Vasudha Devarakonda
 Date  : 17 September 2023

 */

/*--------------------------------------------------------------------------*/
/*
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates
 *single* frames at a time. Because it does allocate one frame at a time,
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.

 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.

 This can be done in many ways, ranging from extensions to bitmaps to
 free-lists of frames etc.

 IMPLEMENTATION:

 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame,
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool.
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.

 NOTE: If we use this scheme to allocate only single frames, then all
 frames are marked as either FREE or HEAD-OF-SEQUENCE.

 NOTE: In SimpleFramePool we needed only one bit to store the state of
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work,
 revisit the implementation and change it to using two bits. You will get
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.

 DETAILED IMPLEMENTATION:

 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:

 Constructor: Initialize all frames to FREE, except for any frames that you
 need for the management of the frame pool, if any.

 get_frames(_n_frames): Traverse the "bitmap" of states and look for a
 sequence of at least _n_frames entries that are FREE. If you find one,
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.

 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.

 needed_info_frames(_n_frames): This depends on how many bits you need
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.


 A WORD ABOUT RELEASE_FRAMES():

 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e.,
 not associated with a particular frame pool.

 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete

 */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#define MB *(0x1 << 20)
#define KB *(0x1 << 10)
#define KERNEL_POOL_START_FRAME ((2 MB) / (4 KB))
#define KERNEL_POOL_SIZE ((2 MB) / (4 KB))
#define PROCESS_POOL_START_FRAME ((4 MB) / (4 KB))
#define PROCESS_POOL_SIZE ((28 MB) / (4 KB))

ContFramePool::FrameState ContFramePool::get_state(unsigned long _frame_no)
{
    unsigned int bitmap_index = _frame_no - base_frame_no;

    if (bitmap[bitmap_index] == 'F')
    {
        return FrameState::Free;
    }
    else if (bitmap[bitmap_index] == 'U')
    {
        return FrameState::Used;
    }
    else
    {
        return FrameState::HoS;
    }
}

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no)
{

    // assert(_n_frames <= FRAME_SIZE * 4);    // because each frame requires 2 bits, and we want to set bitmap in 1 frame max n_frames = Frame size *8/2
    base_frame_no = _base_frame_no;
    // Console::puts("Base Frame Number : ");
    // Console::puti(base_frame_no);
    // Console::puts("\n ");
    nframes = _n_frames;            // Number of frames required by the frame pool
    nFreeFrames = _n_frames;        // initialise all frames to be free
    info_frame_no = _info_frame_no; // frame number of managing bit map frame

    // If _info_frame_no is zero then we keep management info in the first
    // frame, else we use the provided frame to keep management info
    unsigned long needed_frames = needed_info_frames(nframes);
    unsigned long start_frame;
    if (info_frame_no == 0)
    {
        bitmap = (char *)(base_frame_no * FRAME_SIZE); // start of bitmap
        start_frame = base_frame_no;
    }
    else
    {
        bitmap = (char *)(info_frame_no * FRAME_SIZE);
        start_frame = info_frame_no;
    }
    // initialise all the frames to be free
    for (int fno = base_frame_no; fno < base_frame_no + _n_frames; fno++)
    {
        set_state(fno, FrameState::Free);
    } // this is working - unit testing done

    // Console::puts("\n Needed Frames: ");
    // Console::puti(needed_frames);

    // Set all info frames to be Used
    for (unsigned int i = 0; i < needed_frames; i++)
    {
        // Console::puts("\n Setting State ");
        // Console::puts("\n Frames: ");
        // Console::puti(start_frame);
        set_state(start_frame, FrameState::Used);
        start_frame++;
        nFreeFrames--;
    }
    // Console::puts("\n Const Frame Pool initialized\n ");
    // Console::puti(nframes);
}

void ContFramePool::set_state(unsigned long _frame_no, FrameState _state)
{
    unsigned int bitmap_index = _frame_no - base_frame_no;
    // static int number =0;
    switch (_state)
    {
    case FrameState::Used:
    {
        bitmap[bitmap_index] = 'U';
        // const char* myConstChar1 = &bitmap[bitmap_index];
        // Console::puts(myConstChar1);
        // number ++;
        // Console::puti(number);

        break;
    }
    case FrameState::Free:
    {
        bitmap[bitmap_index] = 'F';
        // const char* myConstChar2 = &bitmap[bitmap_index];
        // Console::puts(myConstChar2);
        // number ++;
        // Console::puti(number);
        break;
    }
    case FrameState::HoS:
    {
        bitmap[bitmap_index] = 'H';
        // const char* myConstChar3 = &bitmap[bitmap_index];
        // Console::puts(myConstChar3);
        // number ++;
        // Console::puti(number);
        break;
    }
    }
}
unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    // Console::puts("Getting frames\n ");
    assert(nFreeFrames >= _n_frames);
    unsigned int frame_no = base_frame_no;
    while (get_state(frame_no) != FrameState::Free)
    {
        // Console::puti(frame_no);
        // Console::puts(" Used\n ");
        frame_no++;
    }
    // Console::puts("\n Current frame number ");
    // Console::puti(frame_no);
    bool more_frames = true;
    unsigned int current_frame_no = frame_no;
    // because of continuous of allocations we need not check if we are last
    unsigned int free_n_frames = _n_frames;
    // Console::puts("\n free frames: ");
    // Console::puti(free_n_frames);
    // Console::puts("\n ftesting bitMap ");
    char c = bitmap[frame_no - base_frame_no];
    // Console::putch(c);
    while (free_n_frames > 0 && bitmap[frame_no - base_frame_no] == 'F')
    {
        // Console::puts("\n free frames: ");
        // Console::puti(free_n_frames);
        free_n_frames--;
        frame_no++;
    }
    if (free_n_frames == 0)
    {
        // Console::puts("HoS: ");
        // Console::puti(current_frame_no);
        set_state(current_frame_no, FrameState::HoS);
        nFreeFrames--;
        for (unsigned long frame = current_frame_no + 1; frame < (current_frame_no + _n_frames); frame++)
        {
            set_state(frame, FrameState::Used);
            nFreeFrames--;
        }

        // Console::puts("\n Free Frames: ");
        // Console::puti(nFreeFrames);
        return (frame_no + base_frame_no);
    }

    return -1;
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{

    set_state(_base_frame_no, FrameState::HoS);
    for (int fno = _base_frame_no + 1; fno < _base_frame_no + _n_frames; fno++)
    {
        set_state(fno - this->base_frame_no, FrameState::Used);
    }
}

void ContFramePool::within_pool(unsigned long frame_no)
{
    // Console::puts("Within Pool \n ");
    // Console::puti(this->base_frame_no);

    if (get_state(frame_no) != FrameState::HoS)
    {

        return;
    }

    set_state(frame_no, FrameState::Free);
    frame_no++;
    while (get_state(frame_no) == FrameState::Used && frame_no - this->base_frame_no >= nframes)
    {
        set_state(frame_no, FrameState::Free);
    }
    // Console::puts("Free Frames: ");
    // Console::puti(this->nFreeFrames);
}

void ContFramePool::release_frames(ContFramePool *pools[], unsigned long _first_frame_no)
{

    for (int i = 0; i < 2; i++)
    {

        ContFramePool *check_frame = pools[i];
        if (_first_frame_no >= check_frame->base_frame_no && _first_frame_no < check_frame->base_frame_no + check_frame->nframes)
        {
            check_frame->within_pool(_first_frame_no);
        }
    }
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    /*
     Returns the number of frames needed to manage a frame pool of size _n_frames.
     Here we have 3 states for each frame, so two bits each. i.e each frame would require 8 bits each for representation.
     _n_frames will require (_n_frames * 8) bits. each frame can manage 4K*8 bits i.e 32k bits. Number of frames requires = (_n_frames*8/32k)+ (_n_frames%4k > 0 ? 1:0)

     Example. If _n_frames = 2, then we will need 8 bits i.e one frame round of. If we 4k frames i.e we need 32k bits to represent which is accomodated by 1 frame which has 32k bits.

     _n_frames is equal to 17k, so we need 17*2k bits to represent i.e 34k bits. Each frame can represent 32k bits and to accomodate 34k bits we need 2 frames and from formula above we get (1 + 1= 2 ) frames
     */

    return (_n_frames / 4096 + (_n_frames % 4096 > 0 ? 1 : 0));
}
