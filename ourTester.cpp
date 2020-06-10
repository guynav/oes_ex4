//
// Created by ado_b on 09/06/2020.
//
#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#include <math.h>
#include <iostream>
#include <cstdint>
#define BIT64 64
void splitVAddress2(uint64_t virtualAddress, uint64_t &offset, uint64_t &pageNumber)
{
    uint64_t mask = pow(OFFSET_WIDTH, 2) - 1;
    offset = virtualAddress & mask;
    pageNumber = (virtualAddress & ~mask) >> OFFSET_WIDTH;
}

uint64_t getRowIdx2(uint64_t pageNumber, int iterNum)
{
    int shift =iterNum * ceil((log2(VIRTUAL_MEMORY_SIZE) - log2(PAGE_SIZE)) / log2(PAGE_SIZE)) ; //todo: Check if row number = page size
    int shift2 = BIT64 - OFFSET_WIDTH -  log2(PAGE_SIZE) - iterNum * log2(PAGE_SIZE);
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
    uint64_t cur_add = 0;
    uint64_t idx;
    word_t nextAdd;

    for (int i = 0; i < (BIT64 - OFFSET_WIDTH) /  log2(PAGE_SIZE) ; ++i)
    {
        idx = getRowIdx2(pageNumber, i);
        idx = cur_add * PAGE_SIZE + idx;
        PMread(idx, &nextAdd);
        cur_add = (uint64_t) nextAdd;
        if (!cur_add)
        {
//            cur_add = getNewFrame();
        }
    }

}


int main() {
    uint64_t x;
    uint64_t y;
    uint64_t  va = pow(2,63);
    splitVAddress2(va, x, y);
    getRowIdx2(y,0);
    return 0;
}
