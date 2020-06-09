//
// Created by ado_b on 09/06/2020.
//
#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <math.h>
#include <iostream>
#include <cstdint>

void splitVAddress2(uint64_t virtualAddress, uint &offset, uint &pageNumber) {
    uint64_t mask = pow(OFFSET_WIDTH, 2) - 1;
    offset = virtualAddress & mask;
    pageNumber = (virtualAddress & ~mask) >> OFFSET_WIDTH;

}

uint64_t getRowIdx2(uint64_t pageNumber, int iterNum)
{
    int shift =iterNum * ceil((log2(VIRTUAL_MEMORY_SIZE) - log2(PAGE_SIZE)) / log2(PAGE_SIZE)) ; //todo: Check if row number = page size
    int shift2 = BIT64 - OFFSET_WIDTH -  PAGE_SIZE - iterNum * PAGE_SIZE;
    uint64_t mask = pow(log2(PAGE_SIZE), 2) - 1;
    mask  = mask << shift2;
    uint64_t curPageNumber = pageNumber & mask;
    return  curPageNumber >> shift2;
}

int main() {
    uint x;
    uint y;
    uint64_t  va = pow(2,16);
    splitVAddress2(va, x, y);
    getRowIdx2(y,2);
    return 0;
}
