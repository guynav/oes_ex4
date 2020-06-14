#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <math.h>
#include <iostream>
#include <cstdlib>
#define OFF_SET_SIZE log2(PAGE_SIZE)
#define ERROR_MSG  "system error:"
#define BIT64 64


uint64_t getPAdreess(uint64_t pageNumber);

void checkVMDist(uint64_t &maxDistAdd, uint64_t curVMadd, uint64_t pageNumber);

uint64_t getNextLevel(uint64_t pageNumber, uint64_t cur_add,  int iterNum);

void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}


/**
 * Splits virtual Address into the offset and the page number
 */
void splitVAddress(uint64_t virtualAddress, uint64_t &offset, uint64_t &pageNumber) {
    uint64_t mask = pow(OFFSET_WIDTH, 2) - 1;
    offset = virtualAddress & mask;
    pageNumber = virtualAddress & !mask;
}

uint64_t getRowIdx(uint64_t pageNumber, int iterNum)
{
    int shift2 = BIT64 - OFFSET_WIDTH -  log2(PAGE_SIZE) - iterNum * log2(PAGE_SIZE);
    uint64_t mask = pow(log2(PAGE_SIZE), 2) - 1;
    mask  = mask << shift2;
    uint64_t curPageNumber = pageNumber & mask;
    return  curPageNumber >> shift2;
}


uint64_t isEmpty(uint64_t frame)
{
    uint64_t idx;
    word_t val;
    uint64_t maxVal = 0;
    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        idx = frame * PAGE_SIZE + i;
        PMread(idx, &val);
        if(val) {return frame;}
    }
    return 0;
}

void safelyRemoveFather(uint64_t darthVader, uint64_t luke, bool toEvict= false)
{
    word_t nextAdd;
    uint64_t idx;
    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        idx = darthVader * PAGE_SIZE + i;
        PMread(idx, &nextAdd);
        if ((uint64_t) nextAdd == luke)
        {
            if (toEvict){PMevict(darthVader, idx);}
            PMwrite(idx, (word_t) 0);
        }
    }

}

uint64_t getNewFrameHelper(uint64_t curFrame, uint64_t depth, uint64_t &maxDistAdd, uint64_t curVMadd,
        uint64_t &maxFrame, uint64_t pageNumber, uint64_t father)
{
    maxFrame = curFrame > maxFrame ? curFrame : maxFrame;
    if (depth == log2(NUM_FRAMES))
    {
        checkVMDist(maxDistAdd, curVMadd, pageNumber);
        return 0;
    }
    word_t nextAdd;
    uint64_t idx;
    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        idx = curFrame * PAGE_SIZE + i;
        PMread(idx, &nextAdd);
        if ((uint64_t) nextAdd)
        {
            curVMadd = curVMadd << (int) log2(PAGE_SIZE);
            curVMadd += i;
            return getNewFrameHelper((uint64_t) nextAdd, depth + 1,
                                     maxDistAdd, curVMadd, maxFrame, pageNumber, curFrame);
        }
    }
    safelyRemoveFather(father, curFrame);
    return curFrame;

}
/**
 * For noption 3 of the getting new frame
 * @param maxDistAdd
 * @param curVMadd
 * @param pageNumber
 */
void checkVMDist(uint64_t &maxDistAdd, uint64_t curVMadd, uint64_t pageNumber) {
    if( curVMadd) {
        int cur_calc = abs((int) (pageNumber - maxDistAdd));
        uint64_t cur = std::min(cur_calc, (int) NUM_FRAMES - cur_calc);
        int checked_calc = abs((int) (pageNumber - curVMadd));
        uint64_t checked = std::min(checked_calc, (int) NUM_FRAMES - checked_calc);
        maxDistAdd = !curVMadd || cur < checked ? maxDistAdd : curVMadd;
    } else
    {
        maxDistAdd = curVMadd;
    }
}


void safelyRemoveFrame(uint64_t pageNumber, uint64_t targetFrame)
{
    //Naive implementation. no edge cases
    uint64_t cur_add = 0;


    for (int i = 0; i < (BIT64 - OFFSET_WIDTH) /  log2(PAGE_SIZE) - 1 ; ++i) {
        cur_add = getNextLevel(pageNumber, cur_add, i);
    }
    safelyRemoveFather(cur_add, targetFrame, true);
}
/**
 * get the next physical address of a page in the tree
 * @param pageNumber
 * @param cur_add
 * @param iterNum
 * @return
 */
uint64_t getNextLevel(uint64_t pageNumber, uint64_t cur_add, int iterNum) {
    word_t nextAdd;
    uint64_t idx = getRowIdx(pageNumber, iterNum);
    idx = cur_add * PAGE_SIZE + idx;
    PMread(idx, &nextAdd);
    return (uint64_t) nextAdd;
}


/**
 * This function gets a new frame if needed, does all the cleaning and searching
 * @return new frame ready to use
 */
uint64_t getNewFrame(uint64_t pageNumber)
{
    uint64_t maxDistAdd = 0, maxFrame = 0, newFrame;
    word_t nextAdd;
    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        PMread(i, &nextAdd);
        if ((uint64_t) nextAdd)
        {
            if( (newFrame = getNewFrameHelper((uint64_t) nextAdd, 1, maxDistAdd, i, maxFrame, pageNumber, 0)))
            {
                return newFrame;
            }
        }
    }
    // option 2 - if not all frames are used, given the max + 1
    if(maxFrame < NUM_FRAMES - 1)
    {
        return maxFrame + 1;
    }
    // option 3  -
    uint64_t emptyFrame = getPAdreess(maxDistAdd);

    safelyRemoveFrame(maxDistAdd, emptyFrame);
    return emptyFrame;

}

/**
 * Get the physical address by given a Virtual address
 * @param pageNumber the part of the uint64_t that  need to get the address
 * @return the physical memory address
 */
uint64_t getPAdreess(uint64_t pageNumber)
{


    uint64_t curAdd = 0, nextAdd;

    for (int i = 0; i < (BIT64 - OFFSET_WIDTH) /  log2(PAGE_SIZE) ; ++i)
    {
        nextAdd =  getNextLevel(pageNumber, curAdd, i);
        if (!curAdd)
        {
            nextAdd = getNewFrame(pageNumber);
            uint64_t idx = curAdd * PAGE_SIZE + getRowIdx(pageNumber, i);
            PMwrite(idx,  nextAdd);

        }
        curAdd = nextAdd;
    }
    return curAdd;

}


int getInPageAddress() {
    if (!PAGE_SIZE) {
        std::cerr << ERROR_MSG << " Page size is zero: " << '\n';
        exit(1);
    }
    return log2(PAGE_SIZE);
}


void VMinitialize() {
    clearTable(0);
}


int VMread(uint64_t virtualAddress, word_t *value) {
    uint64_t pageNumber, offSet;
    splitVAddress(virtualAddress,  offSet, pageNumber);
    uint64_t PA = getPAdreess(pageNumber);
    PMread(PA + offSet, value);
    return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value)
{
    uint64_t pageNumber, offSet;
    splitVAddress(virtualAddress,  offSet, pageNumber);
    uint64_t PA = getPAdreess(pageNumber);
    PMwrite(PA + offSet, value);
    return 1;
}