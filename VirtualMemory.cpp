#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <math.h>
#include <cstdlib>

#define OFF_SET_SIZE log2(PAGE_SIZE)
#define BIT64 64
#define GET_PA(fatherFrame, offSet) PAGE_SIZE * fatherFrame + offSet



uint64_t getPAdreess(uint64_t pageNumber, uint64_t offset);

void checkVMDist(uint64_t &maxDistAdd, uint64_t curVMadd, uint64_t pageNumber);

uint64_t getNextLevel(uint64_t pageNumber, int iterNum);

uint64_t getPAdreess2(uint64_t pageNumber);

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
 * Splits virtual Address into the offset and the page number
 */
void splitVAddress(uint64_t virtualAddress, uint64_t &offset, uint64_t &pageNumber)
{
    uint64_t valentina = pow(2, OFFSET_WIDTH) - 1;
    offset = (virtualAddress & valentina);
    pageNumber = virtualAddress & ~valentina;
    pageNumber = pageNumber >> OFFSET_WIDTH;
}

uint64_t getRowIdx(uint64_t pageNumber, int iterNum)
{
    int shift2 = BIT64 - OFFSET_WIDTH - log2(PAGE_SIZE) - iterNum * log2(PAGE_SIZE);
    uint64_t mask = pow(log2(PAGE_SIZE), 2) - 1;
    mask = mask << shift2;
    uint64_t curPageNumber = pageNumber & mask;
    return curPageNumber >> shift2;
}


void safelyRemoveFather(uint64_t darthVader, uint64_t luke, uint64_t pageNumber, bool toEvict = false)
{
    word_t nextAdd;
    uint64_t idx;
    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        idx = darthVader * PAGE_SIZE + i;
        PMread(idx, &nextAdd);
        if ((uint64_t) nextAdd == luke)
        {
            if (toEvict)
            {
                PMevict(luke ,  pageNumber);
            }
            PMwrite(GET_PA(darthVader, i), (word_t) 0);
            break;
        }
    }

}

// DFS on the tree
void getNewFrameHelper(uint64_t curFrame, uint64_t depth, uint64_t &maxDistAdd, uint64_t curVMadd,
                       uint64_t &maxFrame, uint64_t pageNumber, uint64_t &emptyFrame, uint64_t father, uint64_t prevFrame, uint64_t offset)
{

    maxFrame = curFrame > maxFrame ? curFrame : maxFrame;
    if (depth == TABLES_DEPTH)
    {
        checkVMDist(maxDistAdd, curVMadd, pageNumber);
        return;
    }
    word_t nextAdd;
    uint64_t idx;
    bool empty = true;
    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        idx = curFrame * PAGE_SIZE + i;
        PMread(idx, &nextAdd);
        if ((uint64_t) nextAdd)
        {
            empty = false;
            uint64_t nextVMadd = curVMadd << (int) log2(PAGE_SIZE);
            nextVMadd += i;
            getNewFrameHelper((uint64_t) nextAdd, depth + 1,
                              maxDistAdd, nextVMadd, maxFrame, pageNumber, emptyFrame, father, curFrame, offset);
            if (emptyFrame)
            {
                return;
            }
        }
    }
    if (empty &&  curFrame != father)
    {
        emptyFrame =  curFrame;
        safelyRemoveFather(prevFrame, curFrame, pageNumber);

    }

//    safelyRemoveFather(father, curFrame);
//    return curFrame;

}

/**
 * For noption 3 of the getting new frame
 * @param maxDistAdd
 * @param curVMadd
 * @param pageNumber
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
    int shift2 = VIRTUAL_ADDRESS_WIDTH - OFFSET_WIDTH - (OFFSET_WIDTH) * (iterNum +
                                                                          1); //BIT64 - OFFSET_WIDTH - log2(PAGE_SIZE) - iterNum * log2(PAGE_SIZE);SHIFT SHOULD BE ZERO
    uint64_t valentina = PAGE_SIZE - 1; //not giving the right number
    valentina = valentina << shift2;
    uint64_t curPageNumber = pageNumber & valentina;
    uint64_t idx = curPageNumber >> shift2; //getRowIdx(pageNumber, iterNum);
    return idx;
}

uint64_t getNextLevel2(uint64_t pageNumber, int iterNum)
{

    uint64_t shift, mask, offset;
    shift =   OFFSET_WIDTH * (TABLES_DEPTH - iterNum);
    mask =(uint64_t) std::min(VIRTUAL_ADDRESS_WIDTH - (int) shift, OFFSET_WIDTH);
    offset = (pageNumber >> shift) & (uint64_t) (pow(2, mask) - 1);
    return offset % PAGE_SIZE;
}

void safelyRemoveFather2(uint64_t darthVader, uint64_t luke, uint64_t pageNumber, bool toEvict = false)
{
    if (toEvict)
    {
        PMevict(luke  , pageNumber);
    }
    uint64_t offSet = getNextLevel2(pageNumber,TABLES_DEPTH);
    PMwrite(GET_PA(darthVader, offSet), (word_t) 0);
}

void safelyRemoveFrame2(uint64_t pageNumber, uint64_t targetFrame)
{
    //Naive implementation. no edge cases
    uint64_t curAddidx ;
    word_t curFrame = 0;

    for (int i = 0; i < TABLES_DEPTH ; ++i)
    {
        curAddidx = getNextLevel2(pageNumber, i);
        if(i < TABLES_DEPTH - 1) {
            PMread(GET_PA(curFrame, curAddidx), &curFrame);
        }
    }
    safelyRemoveFather2((uint64_t) curFrame, targetFrame, pageNumber, true);

}

void getNewFrameHelper2(uint64_t curFrame,
                        uint64_t depth,
                        uint64_t &maxDistAdd,
                        uint64_t curVMadd,
                        uint64_t &maxFrame,
                        uint64_t pageNumber,
                        uint64_t &emptyFrame,
                        uint64_t checkedFrame,
                        uint64_t &maxDistFrame,
                        uint64_t prevFrame,
                        uint64_t &fatheRefrence)
{

    maxFrame = curFrame > maxFrame ? curFrame : maxFrame;
    if (depth == TABLES_DEPTH)
    {
        checkVMDist(maxDistAdd, curVMadd, pageNumber);
        if(maxDistAdd == curVMadd )
        {
            maxDistFrame = curFrame;
            fatheRefrence = prevFrame;
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
            getNewFrameHelper2((uint64_t) nextAdd, depth + 1,
                               maxDistAdd, nextVMadd, maxFrame, pageNumber, emptyFrame, checkedFrame, maxDistFrame, curFrame, fatheRefrence);
        }
    }
    if (empty && curFrame != checkedFrame)
    {
        fatheRefrence = prevFrame;
        emptyFrame =  curFrame;
        maxDistFrame = getNextLevel2(curVMadd, TABLES_DEPTH);
    }
}

uint64_t getNewFrame2(uint64_t pageNumber, uint64_t curFrame)
{

    uint64_t maxDistAdd = pageNumber,maxDistFrame, maxFrame = 0, emptyFrame = 0, fatheRefrence;
    // generate a frame to use
    getNewFrameHelper2(0, //curFrame
            0, // depth
            maxDistAdd, //maxDistAdd
            0, // curVMadd
            maxFrame, // maxFrame
            pageNumber, // pageNumber
            emptyFrame, //emptyFrame
            curFrame, //checkedFrame
                       maxDistFrame, //maxDistFrame
                       0, // maxDistFrame
                       fatheRefrence // fatheRefrence
                       );

    //option 2 - if not all frames are used, given the max + 1
    if (maxFrame < NUM_FRAMES - 1 && !emptyFrame)
    {
        emptyFrame =  maxFrame + 1;
        clearTable(emptyFrame);
    }
    // option 3  -
    else if(!emptyFrame) {
        emptyFrame = maxDistFrame;
        safelyRemoveFather2(fatheRefrence, emptyFrame, maxDistAdd, true);
        clearTable(emptyFrame);
    } else{
        PMwrite(GET_PA(fatheRefrence, maxDistFrame),0);
    }
    return emptyFrame;

}

uint64_t getPAdreess2(uint64_t virtualAddress)
{

    uint64_t curAdd = 0, offSet;
    word_t nextAdd;
    for (int i = 0; i < TABLES_DEPTH; ++i)
    {
        offSet = getNextLevel2(virtualAddress, i); // This is the offset of the current tree level
        PMread(GET_PA(curAdd, offSet), &nextAdd);
        if (!(uint64_t) nextAdd)
        {
            uint64_t pageNumber = virtualAddress >> OFFSET_WIDTH;
             uint64_t newFrame = getNewFrame2(pageNumber, curAdd);
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
    offSet = getNextLevel2(virtualAddress, TABLES_DEPTH);
    return GET_PA(curAdd, offSet);
}

void VMinitialize()
{
    clearTable(0);
}

bool isLegalVAddress(uint64_t virtualAddress)
{
    return virtualAddress >= VIRTUAL_MEMORY_SIZE;
}

int VMread(uint64_t virtualAddress, word_t * value)
{
    if (isLegalVAddress(virtualAddress))
    {
        return 0;
    }
    uint64_t PA = getPAdreess2(virtualAddress);
    PMread(PA , value);
    return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value)
{
    if (isLegalVAddress(virtualAddress))
    {
        return 0;
    }
    uint64_t PA = getPAdreess2(virtualAddress);
    PMwrite(PA , value);
    return 1;
}