#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <math.h>
#include <iostream>
#include <cstdlib>

#define OFF_SET_SIZE log2(PAGE_SIZE)
#define ERROR_MSG  "system error:"
#define BIT64 64


uint64_t getPAdreess(uint64_t pageNumber, uint64_t offset);

void checkVMDist(uint64_t &maxDistAdd, uint64_t curVMadd, uint64_t pageNumber);

uint64_t getNextLevel(uint64_t pageNumber, int iterNum);

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
//    ////std::cout<<"Mask is " << valentina << " OFFSET width " << OFFSET_WIDTH <<  std::endl;
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
                //std::cout << "EVICT : from frame " << luke << " on index " << i << std::endl;
//                for (int j = 0; j < PAGE_SIZE; ++j)
//                {
//                    //std::cout<< "PAGE_SIZE * luke is " << PAGE_SIZE * luke << " and j is "<< j << std::endl;
//                }
//                word_t stam;
//                PMread(15, &stam);
//                //std::cout<< "stam is before : "<< stam<<std::endl;
//                PMwrite(PAGE_SIZE * luke + offset, (word_t) 0);
//                //std::cout<< "PMWrite Zero to : "<< PAGE_SIZE * luke + offset<<std::endl;
//                PMread(15, &stam);
//                //std::cout<< "stam is after : "<< stam<<std::endl;
            }
            PMwrite(PAGE_SIZE * darthVader + i, (word_t) 0);
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

    ////std::cout << "NUM_PAGES is " << NUM_PAGES << std::endl;
    int curCalc = abs((int) (pageNumber - maxDistAdd));
    int curCalc2 = (int) (NUM_PAGES - (pageNumber - maxDistAdd));
    uint64_t cur = std::min(curCalc, curCalc2);
    int checkedCalc = abs((int) (pageNumber - curVMadd));
    int checkedCalc2 = (int) (NUM_PAGES - (pageNumber - curVMadd));
    uint64_t checked = std::min(checkedCalc, checkedCalc2);
    maxDistAdd = cur < checked ? curVMadd : maxDistAdd;

}


void safelyRemoveFrame(uint64_t pageNumber, uint64_t targetFrame, uint64_t offset)
{
    //Naive implementation. no edge cases
    uint64_t curAddidx ;
    word_t curFrame = 0;

    for (int i = 0; i < TABLES_DEPTH - 1; ++i)
    {
        curAddidx = getNextLevel(pageNumber, i);
        PMread(PAGE_SIZE* (uint64_t) curFrame + curAddidx, &curFrame);

    }

    safelyRemoveFather((uint64_t) curFrame, targetFrame, pageNumber, true);

}

/**
 * get the next physical address of a page in the tree
 * @param pageNumber
 * @param cur_add
 * @param iterNum
 * @return
 */
uint64_t getNextLevel(uint64_t pageNumber, int iterNum)
{
    ///////
//    int windowSize = VIRTUAL_ADDRESS_WIDTH / TABLES_DEPTH;
//    std::cout << "WindowSize is " << OFFSET_WIDTH << std::endl;
//    std::cout << "Page number is " << pageNumber << " iteration number is " << iterNum << std::endl;
//    std::cout << "VIRTUAL_ADDRESS_WIDTH is " << VIRTUAL_ADDRESS_WIDTH << " TABLES_DEPTH is " << TABLES_DEPTH << std::endl;
    int shift2 = VIRTUAL_ADDRESS_WIDTH - OFFSET_WIDTH - (OFFSET_WIDTH) * (iterNum +
                                                                          1); //BIT64 - OFFSET_WIDTH - log2(PAGE_SIZE) - iterNum * log2(PAGE_SIZE);SHIFT SHOULD BE ZERO
    ////std::cout << "shift size is  " << shift2 << std::endl;
    ///BIT64 -  (BIT64 - OFFSET_SIZE) /DEPTH - ((BIT64 - OFFSET_SIZE) /DEPTH )*iterNum;
    uint64_t valentina = PAGE_SIZE - 1; //not giving the right number
    ////std::cout << "mask  is " << valentina << std::endl;
    valentina = valentina << shift2;
    uint64_t curPageNumber = pageNumber & valentina;
    //return
    ///////
    uint64_t idx = curPageNumber >> shift2; //getRowIdx(pageNumber, iterNum);
    ////std::cout << "idx  is " << idx << std::endl;
//    idx = cur_add * PAGE_SIZE + idx; //todo delete!
    ////std::cout << "PA address  is " << cur_add * PAGE_SIZE + idx << std::endl;
//    ////std::cout << nextAdd << std::endl; //todo delete
    return idx;
}


/**
 * This function gets a new frame if needed, does all the cleaning and searching
 * @return new frame ready to use
 */
uint64_t getNewFrame(uint64_t pageNumber, uint64_t curFrame, uint64_t offset)
{
    uint64_t maxDistAdd = pageNumber, maxFrame = 0, emptyFrame = 0;

    // generate a frame to use
    getNewFrameHelper(0, 0, maxDistAdd, 0, maxFrame, pageNumber, emptyFrame, curFrame, 0, offset);
//    newFrame = getNewFrameHelper(0, 1, maxDistAdd, 0, maxFrame, pageNumber,&emptyFrame,0);
//     option 1- find empty frame
    if (emptyFrame)
    {
        std::cout << "Option 1: returns newFrame : " << emptyFrame << std::endl;
        return emptyFrame;
    }
//     option 2 - if not all frames are used, given the max + 1
    if (maxFrame < NUM_FRAMES - 1)
    {
        std::cout << "Option 2: returns newFrame : " << maxFrame + 1 << std::endl;
        return maxFrame + 1;
    }
    // option 3  -
    emptyFrame = getPAdreess(maxDistAdd, offset);
    std::cout << "Option 3: returns newFrame : " << emptyFrame << std::endl;

    safelyRemoveFrame(maxDistAdd, emptyFrame, offset);

    return emptyFrame;

}

/**
 * Get the physical address by given a Virtual address
 * @param pageNumber the part of the uint64_t that  need to get the address
 * @return the physical memory address
 */
uint64_t getPAdreess(uint64_t pageNumber, uint64_t offset)
{

    ////std::cout << " TABLE DEPTH is :" << TABLES_DEPTH << std::endl;
    uint64_t curAdd = 0, nextAddIdx;
    word_t nextAdd;
    for (int i = 0; i < TABLES_DEPTH; ++i)
    {
        nextAddIdx = getNextLevel(pageNumber, i);
        PMread(PAGE_SIZE * curAdd + nextAddIdx, &nextAdd);
        //////std::cout << i << " index add is " << nextAddIdx << " and the Value in the index is " << (uint64_t) nextAdd
//                  << " (if 0 need new frame) " << std::endl; //todo delete

        if (!(uint64_t) nextAdd)
        {
            uint64_t newFrame = getNewFrame(pageNumber, curAdd,offset);
            if(!newFrame)
                {
                std::cout << "OOPS WE ARE FUCKED! NEW FRAME IS -  " << newFrame << std::endl;
                }
//            ////std::cout << "next add after getNewFrame: " << newFrame << std::endl; //todo delete
//            uint64_t idx = curAdd * PAGE_SIZE + nextAddIdx;
            //std::cout << "PAGE UPDATE: writes  " << newFrame << " on page " << curAdd << " in line " << nextAddIdx << " which translates into index " << idx << std::endl;
            PMwrite(PAGE_SIZE * curAdd + nextAddIdx, newFrame);
            ////pprint();

            bool isLeaf = i == TABLES_DEPTH - 1;
            if (isLeaf)
            {
                //std::cout << "BEFORE restore:  " << std::endl;
//                //pprint();
                PMrestore(newFrame, pageNumber);

//                ////std::cout << "AFTER restore:  " << std::endl;
//                //pprint();
                ////std::cout << "RESTORE : from frame  " << newFrame << " on index " << pageNumber << std::endl;
            }
            else{
                clearTable(newFrame);
            }

            nextAdd = newFrame;
        }
        curAdd = (uint64_t) nextAdd;
    }
    return curAdd;
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
        std::cerr << ERROR_MSG << " illegal virtual address" << std::endl;
        return 0;
    }
    uint64_t pageNumber, offSet;
    splitVAddress(virtualAddress, offSet, pageNumber);
    std::cout << "READING: virtual address: " << virtualAddress << " offset: " << offSet << " pageNumber: "<< pageNumber << std::endl;

    uint64_t PA = getPAdreess(pageNumber, offSet);
    PMread(PA * PAGE_SIZE + offSet, value);
    std::cout << "reads value " << *value << " from physical Address - " << PA * PAGE_SIZE + offSet << std::endl;
    //pprint();
    return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value)
{
    if (isLegalVAddress(virtualAddress))
    {
        std::cerr << ERROR_MSG << " illegal virtual address" << std::endl;
        return 0;
    }
    uint64_t pageNumber, offSet;
    splitVAddress(virtualAddress, offSet, pageNumber);
    std::cout << "WIRTING: virtual address: " << virtualAddress << " offset: " << offSet << " pageNumber: "<< pageNumber << std::endl;
    uint64_t PA = getPAdreess(pageNumber, offSet);
    std::cout << "writes the value " << (uint64_t) value << " to physical Address - " << PA * PAGE_SIZE + offSet<< std::endl;
    PMwrite(PA * PAGE_SIZE + offSet, value);
    //pprint();
    return 1;
}