#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <math.h>
#include <iostream>
#include <cstdlib>

#define OFF_SET_SIZE log2(PAGE_SIZE)
#define ERROR_MSG  "system error:"
#define BIT64 64
#define GET_PA(fatherFrame, offSet) PAGE_SIZE * fatherFrame + offSet



uint64_t getPAdreess(uint64_t pageNumber, uint64_t offset);

void checkVMDist(uint64_t &maxDistAdd, uint64_t curVMadd, uint64_t pageNumber);

uint64_t getNextLevel(uint64_t pageNumber, int iterNum);

uint64_t getPAdreess2(uint64_t pageNumber);

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
//    ////std::cout<<"Mask is " << valentina << " OFFSET width " << OFFSET_WIDTH <<  std::endl;
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

//    std::cout << "curVMadd is " << curVMadd << std::endl;
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

//     option 1- find empty frame
    if (emptyFrame)
    {
        std::cout << "Option 1: returns newFrame : " << emptyFrame << std::endl;
    }
//     option 2 - if not all frames are used, given the max + 1
    else if (maxFrame < NUM_FRAMES - 1)
    {
        std::cout << "Option 2: returns newFrame : " << maxFrame + 1 << std::endl;
        emptyFrame =  maxFrame + 1;
    }
        // option 3  -
    else {
        std::cout << "Option 3: returns newFrame : " << emptyFrame << std::endl;
        emptyFrame = getPAdreess(maxDistAdd,offset);
        safelyRemoveFrame(maxDistAdd, emptyFrame, offset);
        clearTable(emptyFrame);
    }
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
            nextAdd = newFrame;
        }
        curAdd = (uint64_t) nextAdd;
    }
    return curAdd;
}



uint64_t getNextLevel2(uint64_t pageNumber, int iterNum)
{

    uint64_t shift, valentina, offset;
    shift =   OFFSET_WIDTH * (TABLES_DEPTH - iterNum);
//    std::cout << "Shift is : " << shift << std::endl;
    valentina =(uint64_t) std::min(VIRTUAL_ADDRESS_WIDTH - (int) shift, OFFSET_WIDTH);
    offset = (pageNumber >> shift) & (uint64_t) (pow(2, valentina) -1);
    return offset % PAGE_SIZE;
}


void safelyRemoveFather2(uint64_t darthVader, uint64_t luke, uint64_t pageNumber, bool toEvict = false)
{
    if (toEvict)
    {
        std::cout<< "EVICT: PMevict("<<luke <<','<< pageNumber<<")"<< std::endl;
        PMevict(luke  , pageNumber);
    }
    uint64_t offSet = getNextLevel2(pageNumber,TABLES_DEPTH);
    std::cout << "Deletes: " << luke  << " from page: " << darthVader <<" in offset: " << offSet<< "which is PA: " <<GET_PA(darthVader, offSet) << std::endl;
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
//        std::cout << "DFS on Frame: " << curFrame << ". On index " << i << " Which is " << nextAdd << "( 0 Means need a new frame)"  <<std::endl;
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

//    safelyRemoveFather(checkedFrame, curFrame);
//    return curFrame;

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
//    newFrame = getNewFrameHelper(0, 1, maxDistAdd, 0, maxFrame, pageNumber,&emptyFrame,0);

//     option 2 - if not all frames are used, given the max + 1
    if (maxFrame < NUM_FRAMES - 1 && !emptyFrame)
    {
        std::cout << "Option 2: returns newFrame : " << maxFrame + 1 << std::endl;
        emptyFrame =  maxFrame + 1;
        clearTable(emptyFrame);
    }
    // option 3  -
    else if(!emptyFrame) {
        emptyFrame = maxDistFrame;
        std::cout << "Option 3: returns newFrame : " << emptyFrame << std::endl;
        safelyRemoveFather2(fatheRefrence, emptyFrame, maxDistAdd, true);
//        safelyRemoveFrame2(maxDistAdd, emptyFrame);
        clearTable(emptyFrame);
    } else{
        std::cout << "Option 1: returns newFrame : " << emptyFrame << std::endl;
//        safelyRemoveFather2(fatheRefrence, emptyFrame, 0, false);
        PMwrite(GET_PA(fatheRefrence, maxDistFrame),0);
    }
    return emptyFrame;

}

uint64_t getPAdreess2(uint64_t virtualAddress)
{

    ////std::cout << " TABLE DEPTH is :" << TABLES_DEPTH << std::endl;
    uint64_t curAdd = 0, offSet;
    word_t nextAdd;
    for (int i = 0; i < TABLES_DEPTH; ++i)
    {
        offSet = getNextLevel2(virtualAddress, i); // This is the offset of the current tree level
//        std::cout<< "Current offSet  of VA " << virtualAddress << "in iteration" << i << " is "  << offSet << std::endl;
//        std::cout << "Physical Address trying reading from is " << GET_PA(curAdd, offSet) << std::endl; //todo delete
        PMread(GET_PA(curAdd, offSet), &nextAdd);
        if (!(uint64_t) nextAdd)
        {
            uint64_t pageNumber = virtualAddress >> OFFSET_WIDTH;
             uint64_t newFrame = getNewFrame2(pageNumber, curAdd);
            if(!newFrame){std::cout << "OOPS WE ARE FUCKED! NEW FRAME IS -  " << newFrame << std::endl;}
            std::cout << "next add after getNewFrame: " << newFrame << std::endl;
//            uint64_t idx = curAdd * PAGE_SIZE + offSet;
            //std::cout << "PAGE UPDATE: writes  " << newFrame << " on page " << curAdd << " in line " << offSet << " which translates into index " << idx << std::endl;
            PMwrite(PAGE_SIZE * curAdd + offSet, newFrame);
            ////pprint();

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
//    std::cout<< "Last offSet  of VA " << virtualAddress << " is "  << offSet << std::endl;
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
        std::cerr << ERROR_MSG << " illegal virtual address" << std::endl;
        return 0;
    }
    std::cout << "READING: From virtual address: " << virtualAddress <<  std::endl;
    uint64_t PA = getPAdreess2(virtualAddress);
    PMread(PA , value);

        std::cout << "READ value " << *value << " from physical Address - " << PA  << std::endl;
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
//    splitVAddress(virtualAddress, offSet, pageNumber);
    std::cout << "WIRTING: on virtual address: " << virtualAddress << "  The value :" << value <<  std::endl;
    uint64_t PA = getPAdreess2(virtualAddress);
    std::cout << "WRITE the value " << (uint64_t) value << " to physical Address - " << PA << " VM "<< virtualAddress <<  std::endl;
    pprint();
    PMwrite(PA , value);
    pprint();
    return 1;
}