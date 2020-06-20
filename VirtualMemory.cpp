#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <math.h>
#include <cstdlib>

#define GET_PA(fatherFrame, offSet) PAGE_SIZE * fatherFrame + offSet

void checkVMDist(uint64_t &maxDistAdd, uint64_t curVMadd, uint64_t pageNumber);

uint64_t getNextLevel(uint64_t pageNumber, int iterNum);

uint64_t getPAddress(uint64_t virtualAddress);

/**
 * This functions clears a frame from content
 * @param frameIndex the index of the frame in physical memory
 */
void clearTable(uint64_t frameIndex)
{
    for (uint64_t i = 0; i < PAGE_SIZE; ++i)
    {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

/**
 * For noption 3 of the getting new frame
 * @param maxDistAdd the address which corresponds to the current max distance
 * @param curVMadd the address to compare to
 * @param pageNumber the page number
 */
void checkVMDist(uint64_t &maxDistAdd, uint64_t curVMadd, uint64_t pageNumber)
{

    int curCalc = abs((int) (pageNumber - maxDistAdd));
    int curCalc2 = (int) (NUM_PAGES - (pageNumber - maxDistAdd));
    uint64_t cur = std::min(curCalc, curCalc2);
    int checkedCalc = abs((int) (pageNumber - curVMadd));
    int checkedCalc2 = (int) (NUM_PAGES - (pageNumber - curVMadd));
    uint64_t checked = std::min(checkedCalc, checkedCalc2);
    maxDistAdd = cur < checked ? curVMadd : maxDistAdd;
}

/**
 * gets the next physical address of a page in the tree
 * @param pageNumber the number of the page
 * @param cur_add th address we are currently in
 * @param iterNum the depth in the virtual tree
 * @return the physical address of the next page in the tree
 */

uint64_t getNextLevel(uint64_t pageNumber, int iterNum)
{

    uint64_t shift, mask, offset;
    shift =   OFFSET_WIDTH * (TABLES_DEPTH - iterNum);
    mask =(uint64_t) std::min(VIRTUAL_ADDRESS_WIDTH - (int) shift, OFFSET_WIDTH);
    offset = (pageNumber >> shift) & (uint64_t) (pow(2, mask) - 1);
    return offset % PAGE_SIZE;
}

/**
 * This cuntion removes a page from its parent and diconnects the page from the parent
 * @param father the parent page
 * @param child the child page
 * @param pageNumber the number of page to evict
 * @param toEvict to evict to hard drive or not
 */
void safelyRemoveFather(uint64_t father, uint64_t child, uint64_t pageNumber, bool toEvict = false)
{
    if (toEvict)
    {
        PMevict(child  , pageNumber);
    }
    uint64_t offSet = getNextLevel(pageNumber, TABLES_DEPTH);
    PMwrite(GET_PA(father, offSet), (word_t) 0);
}

/**
 * Helper function for finding a new frame. Recursively iterates over the virtual tree to find a new frame
 * @param curFrame the current frame
 * @param depth the depth in the virtual tree
 * @param maxDistAdd the maximal distance of an address
 * @param pageNumber the number of the page we are in
 * @param emptyFrame the number of the current empty frame
 * @param checkedFrame the frame we are currently checking to replace
 * @param maxDistAdd the virtual address of the frame with the maximal cyclic distance
 * @param prevFrame the frame we came from
 * @param fatheRef reference to the father frame
 */

void getNewFrameHelper(uint64_t curFrame,
                       uint64_t depth,
                       uint64_t &maxDistAdd,
                       uint64_t curVMadd,
                       uint64_t &maxFrame,
                       uint64_t pageNumber,
                       uint64_t &emptyFrame,
                       uint64_t checkedFrame,
                       uint64_t &maxDistFrame,
                       uint64_t prevFrame,
                       uint64_t &fatheRef)
{
    maxFrame = curFrame > maxFrame ? curFrame : maxFrame;
    if (depth == TABLES_DEPTH)
    {
        checkVMDist(maxDistAdd, curVMadd, pageNumber);
        if(maxDistAdd == curVMadd )
        {
            maxDistFrame = curFrame;
            fatheRef = prevFrame;
        }
        return;
    }
    word_t nextAdd;
    bool empty = true;
    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        if (emptyFrame){return;} //If an empty frame was found then no need for whole DFS
        PMread(GET_PA(curFrame, i), &nextAdd);
        if ((uint64_t) nextAdd)
        {
            empty = false;
            uint64_t nextVMadd = (curVMadd << OFFSET_WIDTH) | i;
            getNewFrameHelper((uint64_t) nextAdd, depth + 1,
                              maxDistAdd, nextVMadd, maxFrame, pageNumber, emptyFrame, checkedFrame, maxDistFrame,
                              curFrame, fatheRef);
        }
    }
    if (empty && curFrame != checkedFrame)
    {
        fatheRef = prevFrame;
        emptyFrame =  curFrame;
        maxDistFrame = getNextLevel(curVMadd, TABLES_DEPTH);
    }
}

/**
 * Gets a new frame
 * @param pageNumber
 * @param curFrame
 * @return the new frame number to be used
 */
uint64_t getNewFrame(uint64_t pageNumber, uint64_t curFrame)
{
    uint64_t maxDistAdd = pageNumber,maxDistFrame, maxFrame = 0, emptyFrame = 0, fatheRefrence;
    // generate a frame to use
    getNewFrameHelper(0, //curFrame
                      0, // depth
                      maxDistAdd, //maxDistAdd
                      0, // curVMadd
                      maxFrame, // maxFrame
                      pageNumber, // pageNumber
                      emptyFrame, //emptyFrame
                      curFrame, //checkedFrame
                      maxDistFrame, //maxDistFrame
                      0, // maxDistFrame
                      fatheRefrence // fatheReference
    );

    //option 2 - if not all frames are used, given the max + 1
    if (maxFrame < NUM_FRAMES - 1 && !emptyFrame)
    {
        emptyFrame =  maxFrame + 1;
        clearTable(emptyFrame);
    }
    // option 3  - find new frame with max cyclical distance
    else if(!emptyFrame)
    {
        emptyFrame = maxDistFrame;
        safelyRemoveFather(fatheRefrence, emptyFrame, maxDistAdd, true);
        clearTable(emptyFrame);
    } else{
        PMwrite(GET_PA(fatheRefrence, maxDistFrame),0);
    }
    return emptyFrame;

}

/**
 * Converts vritual address into a physical address
 * @param virtualAddress the address to convert
 * @return the matching physical address
 */
uint64_t getPAddress(uint64_t virtualAddress)
{

    uint64_t curAdd = 0, offSet;
    word_t nextAdd;
    for (int i = 0; i < TABLES_DEPTH; ++i)
    {
        offSet = getNextLevel(virtualAddress, i); // This is the offset of the current tree level
        PMread(GET_PA(curAdd, offSet), &nextAdd);
        if (!(uint64_t) nextAdd)
        {
            uint64_t pageNumber = virtualAddress >> OFFSET_WIDTH;
             uint64_t newFrame = getNewFrame(pageNumber, curAdd);
            PMwrite(PAGE_SIZE * curAdd + offSet, newFrame);


            bool isLeaf = i == TABLES_DEPTH - 1;
            if (isLeaf)
            {
                PMrestore(newFrame, pageNumber);
            }
            nextAdd = newFrame;
        }
        curAdd = (uint64_t) nextAdd;
    }
    offSet = getNextLevel(virtualAddress, TABLES_DEPTH);
    return GET_PA(curAdd, offSet);
}

/**
 * Initialize the virtual memory
 */
void VMinitialize()
{
    clearTable(0);
}

/**
 * verifies an address is in legal format
 * @param virtualAddress
 * @return true is illegal, false otherwise
 */
bool isLegalVAddress(uint64_t virtualAddress)
{
    return virtualAddress >= VIRTUAL_MEMORY_SIZE;
}

/**
 * Reads the word from the virtual address into value
 * @param virtualAddress the virtual address
 * @param value the value read into
 * @return 1 on success, 0 on failure
 */
int VMread(uint64_t virtualAddress, word_t * value)
{
    if (isLegalVAddress(virtualAddress))
    {
        return 0;
    }
    uint64_t PA = getPAddress(virtualAddress);
    PMread(PA , value);
    return 1;
}

/**
 * Writes into the word from the virtual address into value
 * @param virtualAddress the virtual address
 * @param value the value written into
 * @return 1 on success, 0 on failure
 */
int VMwrite(uint64_t virtualAddress, word_t value)
{
    if (isLegalVAddress(virtualAddress))
    {
        return 0;
    }
    uint64_t PA = getPAddress(virtualAddress);
    PMwrite(PA , value);
    return 1;
}