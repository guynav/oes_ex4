#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <math.h>
#include <iostream>

#define OFF_SET_SIZE log2(PAGE_SIZE)
#define ERROR_MSG  "system error:"
#define BIT64 64
void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

void VMinitialize() {
    clearTable(0);
}


int VMread(uint64_t virtualAddress, word_t *value) {
    return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value) {


    return 1;
}

/**
 * Splits virtual Address into the offset and the page number
 */
void splitVAddress(uint64_t virtualAddress, uint &offset, uint &pageNumber) {
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
        maxVal =  std::max((uint64_t) val, maxVal);
    }
    return maxVal;
}

/**
 * This function gets a new frame iof needed, does all the cleaning and searching
 * @return new frame ready to use
 */
uint64_t getNewFrame()
{
    uint64_t maxUsedIDX = 0;
    uint64_t curMax;

    // op 1 empty frame (all entries are 0)
    for (int i = 0; i < NUM_FRAMES; ++i)
    {
        if ((curMax  = isEmpty(i)) == 0)
        {
            return i;
        } else
        {
            maxUsedIDX = std::max(maxUsedIDX, curMax);
        }

    }
    // op 2 unused frame
    if (maxUsedIDX < NUM_FRAMES)
    {
        return maxUsedIDX + 1;
    }
    // op 3 formula
}

/**
 * Get the physical address by given a Virtual address
 * @param pageNumber the part of the uint64_t that  need to get the address
 * @return the physical memory address
 */
uint getPAdreess(uint64_t pageNumber)
{

    //Naive implementation. no edge cases
    uint64_t cur_add = 0;
    uint64_t idx;
    word_t nextAdd;

    for (int i = 0; i < (BIT64 - OFFSET_WIDTH) /  log2(PAGE_SIZE) ; ++i)
    {
        idx = getRowIdx(pageNumber, i);
        idx = cur_add * PAGE_SIZE + idx;
        PMread(idx, &nextAdd);
        cur_add = (uint64_t) nextAdd;
        if (!cur_add)
        {
            cur_add = getNewFrame();
        }
    }

}





int getInPageAddress() {
    if (!PAGE_SIZE) {
        std::cerr << ERROR_MSG << " Page size is zero: " << '\n';
        exit(1);
    }
    return log2(PAGE_SIZE);
}