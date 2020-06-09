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
    int shift2 = BIT64 - OFFSET_WIDTH -  PAGE_SIZE - iterNum * PAGE_SIZE;
    uint64_t mask = pow(log2(PAGE_SIZE), 2) - 1;
    mask  = mask << shift2;
    uint64_t curPageNumber = pageNumber & mask;
    return  curPageNumber >> shift2;
}

/**
 * Get the physical address by given a Virtual address
 * @param pageNumber the part of the uint64_t that  need to get the address
 * @return the physical memory address
 */
uint getPAdreess(uint64_t pageNumber)
{
    //Naive implementation. no edge cases

    int curPage = 0; // todo: check what's the location of the tree's root.




}



/**
 * This function gets a new frame iof needed, does all the cleaning and searching
 * @return new frame ready to use
 */
uint getNewFrame() {

}

int getInPageAddress() {
    if (!PAGE_SIZE) {
        std::cerr << ERROR_MSG << " Page size is zero: " << '\n';
        exit(1);
    }
    return log2(PAGE_SIZE);
}