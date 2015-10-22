/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the buddy algorithm
 *    Author: Stefan Birrer
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    Revision 1.2  2009/10/31 21:28:52  jot836
 *    This is the current version of KMA project 3.
 *    It includes:
 *    - the most up-to-date handout (F'09)
 *    - updated skeleton including
 *        file-driven test harness,
 *        trace generator script,
 *        support for evaluating efficiency of algorithm (wasted memory),
 *        gnuplot support for plotting allocation and waste,
 *        set of traces for all students to use (including a makefile and README of the settings),
 *    - different version of the testsuite for use on the submission site, including:
 *        scoreboard Python scripts, which posts the top 5 scores on the course webpage
 *
 *    Revision 1.1  2005/10/24 16:07:09  sbirrer
 *    - skeleton
 *
 *    Revision 1.2  2004/11/05 15:45:56  sbirrer
 *    - added size as a parameter to kma_free
 *
 *    Revision 1.3  2004/12/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *

 ************************************************************************/
 
 /************************************************************************
 Project Group: NetID1, NetID2, NetID3
 
 ***************************************************************************/

#ifdef KMA_BUD
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

/************Private include**********************************************/
#include "kma_page.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */
#define MIN_POWER2 5
#define MIN_SIZE 32
#define PAGE_SIZE 8192
#define NODE_SIZE 12

typedef struct block_node
{
  int size;
  void* ptr;
  void* next;
  void* pageptr;
 //WE DON'T NEED THE VARIABLE TO SEE IF LEFT OR RIGHT
 //BECAUSE THE XOR TO FIND NEIGHBOR GOES BOTH DIRECTIONS
  //enum BLOCK_STATE state;
  
} blocknode;

/************Global Variables*********************************************/

//number of free lists for a minimum size of 32 is 8:
//free lists for sizes 32, 64, 128, 256, 512, 1024, 2048, 4096
blocknode* freelistheads[8];
blocnode* freelisttails[8];

/************Function Prototypes******************************************/
	
/************External Declaration*****************************************/

/**************Implementation***********************************************/

void* kma_malloc(kma_size_t size)
{
    
    if(size + sizeof(void*)>PAGESIZE) return NULL;
    
    kma_size_t asize = getClosestPowerOfTwo(size);
    int index = getLog(asize);
    if (index < getLog(MIN_SIZE))
    {
        index = getLog(MIN_SIZE);
    }
    index = index - getLog(MIN_SIZE);
    int maxIndex = getLog(PAGE_SIZE) - 5;
    for (int i = index; i < maxIndex; i++)
    {
        if (freelistheads[i] != NULL)
        {
            return breakFreeBlock(i, asize);
        }
    }
    
    
    
    //if you get here you need to fetch a new page and break it into pieces
    
  return NULL;
}

void* breakFreeBlock(int index, kma_size_t allocateSize)
{
    blocknode* myFreeBlock;
    void* myFreeBlockPtr;
    if (allocateSize == freelistheads[index]->size)
    {
        adjustBitmap(freelistheads[index]->pageId, freelistheads[index]->ptr , allocateSize, FALSE);
        myFreeBlock = freelistheads[index];
        myFreeBlockPtr = freelistheads[index]->ptr;
        freelistheads[index] = freelistheads[index]->next;
        if (freelistheads[index] == NULL)
        {
            freelisttails[index] = NULL;
        }
        free(myFreeBlock);
        return myFreeBlockPtr;
    }
    
    //this executes if the initial call is to a memory block that is too large for the allocation
    //in this case need to split the memory block into two, and then call the function with the first half
    
    //take the head of the specified free list
    myFreeBlock = freelistheads[index];
    
    //adjusts size and removes from current list
    myFreeBlock->size = myFreeBlock->size / 2;
    freelistheads[index] = freelistheads[index]->next;
    if (freelistheads[index] == NULL)
    {
        freelisttails[index] = NULL;
    }
    
    //find the address of the buddy block and add it to the free list
    void* buddyAddress = myFreeBlock->ptr ^ myFreeBlock->size;
    addToFreeListFront(buddyAddress, myFreeBlock->size);
    
    myFreeBlock->next = freelistheads[index - 1];
    freelistheads[index - 1] = myFreeBlock;
    
    return breakFreeBlock(index - 1, allocateSize);
    
}

void adjustBitmap(int pageId, void* blockPtr, kma_size_t blockSize, bool setFree)
{
    //adjusts the bitmap with specified page ID, a pointer to the block and its size
    //adjusts to free if setFree is true, otherwise adjusts to used
    ;
    
}

void kma_free(void* ptr, kma_size_t size)
{

  // free a memory located at some pointer and size: size
  // memory located at place: ptr 
  // end of ptr (memory) is at place: ptr + (findShift)
  
  //find the closest power of 2 block that holds the space
  kma_size_t ceilingSize = getClosestPowerOfTwo(size);

  coalesceRecursive(ptr,ceilingSize);

}

kma_size_t getLog(kma_size_t num)
{
    kma_size_t start = num;
    kma_size_t result = 0;
    while (start > 1)
    {
        result++
        start >> 1; 
    }
    return result;
}

kma_size_t getClosestPowerOfTwo(kma_size_t num)
{
    power = num - 1;
    power |= power >> 1;
    power |= power >> 2;
    power |= power >> 4;
    power |= power >> 8;
    power |= power >> 16;
    power++;
    if (power < MIN_SIZE)
    {
        power = MIN_SIZE;
    }
    return power;
}

void addToFreeListBack(void* ptr,kma_size_t size)
{
    //find the list that we want to put this on
    int listIndex = getLog(size) - MIN_POWER2;
    
    blocknode* listTail = freelisttails[listIndex];
    blocknode* listHead = freelistheads[listIndex];
    
    //add to tail of the list
    blocknode* newPageFreeBlock = malloc(sizeof(blocknode));
    newPageFreeBlock->ptr = ptr;
    newPageFreeBlock->size = size;
    newPageFreeBlock->next = NULL;
    
    
    //null list (tail is null)
    if (listTail==NULL){
        //then add and mark the head and tail as those
        freelistheads[listIndex] = newPageFreeBlock;
        freelisttails[listIndex] = newPageFreeBlock;
    }
    else {
        //just add it to the end
        freelistttails[listIndex]->next = newPageFreeBlock;
        freelisttails[listIndex] = newPageFreeBlock;
    }
    
}

void addToFreeListFront(void* ptr,kma_size_t size)
{
    //find the list that we want to put this on
    int listIndex = getLog(size) - MIN_POWER2;
    
    blocknode* listTail = freelisttails[listIndex];
    blocknode* listHead = freelistheads[listIndex];
    
    //add to front of the list
    blocknode* newPageFreeBlock = malloc(sizeof(blocknode));
    newPageFreeBlock->ptr = ptr;
    newPageFreeBlock->size = size;
    newPageFreeBlock->next = freeListheads[listIndex];
    
    freelistheads[listIndex] = newPageFreeBlock;
    //null list (head is null)
    if (listTail==NULL){
        //then also  mark tail as the node
        freelisttails[listIndex] = newPageFreeBlock;
    }
    
}


blocknode* findBuddy(void* address,kma_size_t size)
{
    //finds the buddy, takes it out of the list and puts it in the return value
    
    
    
    //get the size and index
    int listIndex = getLog(size)- MIN_POWER2;
    
    blocknode* current = freelistheads[listIndex];
    blocknode* listTail = freelisttails[listIndex];
    blocknode* previous = NULL;
    

    //step through the list of possible buddies
    //if find one, return, else return NULL
    while (current!=NULL){
        
        if (current->ptr==address){
            
            //get rid of it from the list of free elements
            if ((previous==NULL)&&(current->next==NULL))
            {
                //only one in the list
                freelistheads[listIndex] = NULL;
                freelisttails[listIndex] = NULL;
                return current;
            }
            
            if (previous==NULL)
            {
                //only the head
                freelistheads[listIndex] = current->next;
                return current;
            }
            
            if (current->next==NULL)
            {
                //only the tail
                freelisttails[listIndex] = previous;
                previous->next = NULL;
                return;
            }
            
            //otherwise, point previous to next and return
            previous->next = current->next;
            return current;

        }
        else{
            previous = current;
            current = current->next;
        }
    }
    return NULL;
    
}

void  coalesceRecursive(void* ptr, kma_size_t ceilingSize);
{
    if (ceilingSize == PAGE_SIZE )
    {
        //ideally, the ptr to the page node where the memory starts should be here
        freeMyPage(ptr);    
    }
        
    
    
    blocknode* buddy;
    void* buddyAddr;
    
    //figure out the buddy address
    buddyAddr = ptr ^ ceilingSize;
    
    //search for buddy that has the proper size
    buddy = findBuddy(buddyAddr,ceilingSize);
    
    if (buddy!=NULL){
        //found a buddy and it's free!
      
        //merge these 2 (coalesce)
        //1. change the size of the free node (the buddy) to twice that
        buddy->size = 2*buddy->size;
        
        //2. put the smaller address in the ptr
        if (buddyAddr>ptr)
        {
            buddyAddr = ptr;
        }
        //else, it's already done
        
        //3. put this node in the right place in the right free list
        //by passing the correct address and the size of the node
        addtoFreeListBack(buddyAddr,ceilingSize*2);
        
        //4. delete the previous node that we had
        free(buddy);
      
        //and coalesce again the new stuff
        coalesceRecursive(buddyAddr,ceilingSize*2);
    }
    else {
        //buddy is busy
        //add that block to free list
        addtoFreeListBack(ptr,size);
    }


}

freeMyPage(void* pagePtr){
    
    //find the page in the list of pages and free that
    //the nodes should be gone by now
    
    //go through the list comparing pointers to where the block starts (or decrease by kma_pointer stuff to get page ptr)
    
}

#endif // KMA_BUD
