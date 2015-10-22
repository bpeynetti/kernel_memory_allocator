/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the resource map algorithm
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
 *    Revision 1.1  2004/11/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *
 *    Revision 1.3  2004/12/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *
 
 ************************************************************************/

/************************************************************************
 Project Group: NetID1, NetID2, NetID3
 
 ***************************************************************************/

#ifdef KMA_RM
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
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
#define PAGE_SIZE 8192

  
typedef struct block_head
{
  int size;
  //void* ptr;
  void* next;
  //kma_page_t* pagePtr;
 // void* previous;
  //enum BLOCK_STATE state;
} blockheader;

typedef struct page_header
{
    //int id;
    kma_page_t* ptr;
    int counter;
    void* next;
    blockheader* blockHead;
} pageheader;

// free list pointer
// blocknode *freeList = NULL;

// busy list pointer
// blocknode *usedList = NULL;

// free page list
// pagenode *pageList = NULL;

/************Global Variables*********************************************/

//define a pointer kma_struct_t that points to the beginning of everything
kma_page_t* globalPtr = NULL;


/************Function Prototypes******************************************/
void* kma_malloc(kma_size_t size);
void* findFreeBlock(kma_size_t size);
void kma_free(void* ptr, kma_size_t size);
void addToList(void* ptr,kma_size_t size);
void freeMyPage(pageheader* page);
void printPageList();


/************External Declaration*****************************************/

/**************Implementation***********************************************/
int requests = 0;
int maxPages = 0;
int currentPages = 0;

void* kma_malloc(kma_size_t size)
{
    
    printf("Allocating a new block of memory of size %d\n", size);
    if (globalPtr != NULL)
    {
        printf("    ");
        pageheader* firstPage = (pageheader*) (globalPtr->ptr);
        blockheader* currentBlock = (blockheader*)(firstPage->blockHead);
        while (currentBlock != NULL)
        {
            printf("%d->", currentBlock->size);
            currentBlock = currentBlock->next;
        }
        printf("\n");
    }
    
    
    
    //ignore requests for more size than a page
    if(size + sizeof(void*)>PAGESIZE) return NULL;
    
    if (globalPtr==NULL)
    {
        //initialize everything    
        //get a new page, put it to global ptr
        globalPtr = get_page();

        //put it in the first place 
        *((kma_page_t**)globalPtr->ptr) = globalPtr;
        
        printf("global is at %p \n",globalPtr->ptr);
        
        //create the page header
        pageheader* pageHead;
        //and set the location (address) of the head to the beginning of the page
        pageHead = (pageheader*) (globalPtr->ptr);
        pageHead->ptr = globalPtr;
        pageHead->counter = 1;
        pageHead->next = NULL;
        pageHead->blockHead = globalPtr->ptr + sizeof(pageheader) + size;
        
        printf("First block at %p \n",pageHead->blockHead);
        
        blockheader* listHead;
        listHead = (blockheader*)(pageHead->blockHead);
        listHead->size = PAGE_SIZE  - sizeof(pageheader)  - size;
        listHead->next = NULL;
        
        return globalPtr->ptr + sizeof(pageheader);
    }
    
   void* returnAddress;
   
    returnAddress = findFreeBlock(size);
    
    if (returnAddress!= NULL)
    {
        return returnAddress;
    }
    
    pageheader* currentPage = (pageheader*) (globalPtr->ptr);
    
    while (currentPage->next != NULL)
    {
        currentPage = currentPage->next;
    }
    
    kma_page_t* pageTemp = get_page();
    
    *((kma_page_t**)pageTemp->ptr) = pageTemp;

    
    pageheader* newPageHead;
    newPageHead = (pageheader*) (pageTemp->ptr);
    newPageHead->ptr = pageTemp;
    newPageHead->counter = 1;
    newPageHead->next = NULL;
    
    currentPage->next = newPageHead;
    
    pageheader* firstPageHead = (pageheader*) (globalPtr->ptr);
    newPageHead->blockHead = firstPageHead->blockHead;
    
    blockheader* currentBlock;
    currentBlock = (blockheader*)(newPageHead->blockHead);
    
    while (currentBlock->next != NULL)
    {
        currentBlock = currentBlock->next;
    }
    
    blockheader* newBlock = (blockheader*)(newPageHead->ptr + sizeof(pageheader) + size);
    newBlock->size = PAGE_SIZE - sizeof(pageheader) - size;
    newBlock->next = NULL;
    
    currentBlock->next = newBlock;
    
    return findFreeBlock(size);
}

void* findFreeBlock(kma_size_t size)
{
    //if there's a list of stuff, look through it and find first fit 
    blockheader* previous = NULL;
    pageheader* pageHead;
    //it's at the beginning of the list. make it point to the first free node
    pageHead = (pageheader*) (globalPtr->ptr);
    blockheader* current = pageHead->blockHead;
    
    pageheader* currentPage = pageHead;
    blockheader* returnAddr = NULL;
    
    kma_size_t oldSize;
    blockheader* tempNext;

    printf("free block starts at %p\n",current);
    
    while (current!=NULL)
    {
        printf("Free block: %p\n",current);
        while (current>(currentPage+PAGE_SIZE))
        {
            currentPage = currentPage->next;
        }
        //now go through the list and find a free block
        if (current->size >= size)
        {
            returnAddr = current;
            oldSize = current->size;
            
            printf("return address at %p \n",returnAddr);
            // take out of linked list
            //if head of the blocks
            if (previous==NULL)
            {

                //change the ptr to the next free block
                if (current->next==NULL){
                    //only one block!
                    
                    //if there is no space for the new block and at least a header for a new block
                    // if (((void*)currentPage+(void*)PAGE_SIZE-((void*)current))<((void*)size+sizeof(blockheader)))
                    // {
                    //     //not enough space in the last free block, return null
                    //     return (void*)NULL;
                    // }
                    // else 
                    // {
                        //enough space, so split the block 
                        //figure out the previous address and size 
                        //figure out the location for the next block
                        printf("Current pointer: %p\n", current);
                        current = (void*)((int)(current)+size);
                        printf("Current pointer: %p\n", current);
                        //and replicate the size from the old size - size
                        current->size = oldSize - size;
                        current->next = NULL;
                        
                        //update the head of list to this one
                        currentPage->blockHead = current;
                        
                        currentPage->counter++;
                        //and return the old address 
                        return (void*)returnAddr;
                    // }
                }
                else 
                {
                    //you're at the beginning of your list and the list is more than one
                    //you know it's big enough so just change its size and change the location of the header
                    tempNext = current->next;
                    current = current+size;
                    current->size = oldSize - size;
                    //and copy pointer to next
                    current->next = tempNext;
                    //no previous one so update header
                    currentPage->blockHead = current;
                    //add one to page counter
                    currentPage->counter++;
                    //nothing else to update
                    return (void*)returnAddr;
                }
            }
            else 
            {
                //not at the head
                //if you're at the tail
                if (current->next==NULL)
                {
                    //add one node to the end 
                    // if (((void*)currentPage+(void*)PAGE_SIZE-((void*)current))<((void*)size+sizeof(blockheader)))
                    // {
                    //     //not enough space in the last free block, return null
                    //     return (void*)NULL;
                    // }
                    // else 
                    // {
                        //enough space, so split the block 
                        //figure out the previous address and size 
                        //figure out the location for the next block
                        current = current+size;
                        //and replicate the size from the old size - size
                        current->size = oldSize - size;
                        current->next = NULL;
                        
                        //update the previous next to this one
                        previous->next = current;
                        
                        currentPage->counter++;
                        //and return the old address 
                        return (void*)returnAddr;
                    // }
                }
                else 
                {
                    //normal case, something in front and something behind
                    returnAddr = current;
                    //if size of block is within accepted  to size+sizeof(blocknode)
                    if (current->size-size<=sizeof(blockheader)) //if the difference between them is smaller than what we need for a new node
                    {
                        //you lose a few things to fragmentation
                        
                        //take it out of the list
                        previous->next = current->next;
                        //and add 1 more
                        currentPage->counter++;
                        return (void*)returnAddr;
                    }
                    else 
                    {
                        //block is too big, so allocate new smaller block 
                        //copy over (shift the free node)
                        tempNext = current->next;
                        current = current+size;
                        current->size = oldSize - size;
                        current->next = tempNext;
                        currentPage->counter++;
                        
                        previous->next = current;
                        return (void*)returnAddr;
                    }
                }
            }
        }
        else 
        {
            //block is not good. step through
            previous = current;
            current = current->next;
        }
    }
    return (void*)NULL; //no block found
}

void kma_free(void* ptr, kma_size_t size)
{
    
    
    printf("Freeing a block of memory of size %d at address %p \n", size,ptr);
    if (globalPtr != NULL)
    {
        printf("    ");
        pageheader* firstPage = (pageheader*) (globalPtr->ptr);
        blockheader* currentBlock = (blockheader*)(firstPage->blockHead);
        while (currentBlock != NULL)
        {
            printf("%d->", currentBlock->size);
            currentBlock = currentBlock->next;
        }
        printf("\n");
    }
    
    //first need to add the requested memory location to the free list
    addToList(ptr,size);
    
    //then need to find the page the freed block belongs to and decrement counter for that page
    pageheader* currentPage = (pageheader*) (globalPtr->ptr);
    pageheader* nextPage = currentPage->next;
    
    //the pointer specified is in the first page
    if (ptr < (void*)(currentPage+PAGE_SIZE))
    {
        currentPage->counter--;
    }
    
    //the pointer specified is not in the first page
    else
    {
        while (nextPage != NULL)
        {
            //if the pointer is between the addresses of the next two pages
            if (ptr > (void*)(currentPage) && ptr < (void*)(nextPage))
            {
                currentPage->counter--;
                break;
            }
        
            currentPage = currentPage->next;
            nextPage = nextPage->next;
        }
        
        //if you make it this far and are at the end of the list, the pointer is in the last page
        if (nextPage == NULL)
        {
            currentPage->counter--;
        }
        
    }
    
    
    //then, if the counter has reached 0, free the page
    if (currentPage->counter==0)
    {
        freeMyPage(currentPage);
    }
    
    return;
    
}


void addToList(void* ptr,kma_size_t size)
{
    blockheader* previous = NULL;
    pageheader* pageHead;
    //it's at the beginning of the list. make it point to the first free node
    pageHead = (pageheader*) (globalPtr->ptr);
    blockheader* current = pageHead->blockHead;
    pageheader* currentPage = pageHead;
    blockheader* newBlock;

    newBlock = (blockheader*) ptr;  
    
    if (ptr<current)
    {
        //check if the next one is adjacent and free
        //if yes, merge them 
        
        //no
        newBlock->size = size;
        newBlock->next = current;
        pageHead->blockHead = newBlock;
        
        return;   
    }
    while (current!=NULL)
    {
        printf("\tWe are at free node %p of size %d\n",current,current->size);
        // while (current>((void*)((int)currentPage+PAGE_SIZE)))
        // {
        //     currentPage = currentPage->next;
        // }
        if ((void*)current>ptr)
        {
            //stepped over, now we have previous and next
            //link them 
            printf("\t Found a place to add the block. \n");
            newBlock->size = size;
            newBlock->next = previous->next;
            previous->next = newBlock;
            return;
        }
        current = current->next;
    }
    //if you reached the end of the list
    //add it to the end (previous is the tail now)
    printf("\t couldn't find a place, put it at the end \n");
    newBlock->size = size;
    newBlock->next = NULL;
    previous->next = newBlock;

}

void freeMyPage(pageheader* page)
{
    pageheader* currentPage;
    pageheader* previousPage = NULL;

    currentPage = (pageheader*) (globalPtr->ptr);

    blockheader* previous = NULL;
    blockheader* current = currentPage->blockHead;

    //it's at the beginning of the list. make it point to the first free node
    
    //
    // if it's the first one to free
    if (page==currentPage)
    {
        //if no more pages
        if (currentPage->next==NULL)
        {
            //you're done!
            free_page((kma_page_t*)page);
            return;
        }
        else
        {
            //find the first node in a next page
            while (((void*)current < ((void*)page+PAGE_SIZE)) && (current!=NULL))
            {
                current = current->next;
            }

            //current has the new first node
            //and new first page is next page
            //*((kma_page_t**)globalPtr->ptr) = page->next;
            globalPtr = (kma_page_t*)page->next;
            
            pageheader* nextPage = page->next;
            nextPage->blockHead = current;
            free_page((kma_page_t*)page);
            return;
        }
    }
    else 
    {
        //not the first page
        //find the previous page
        pageheader* tempPage = (pageheader*)globalPtr->ptr;
        while (tempPage<page)
        {
            previousPage = tempPage;
            tempPage = tempPage->next;
        }
        
        if (page->next==NULL)
        {
            //the last one
            while ((void*)current<(void*)page)
            {
                //step through until you find the first block in the page
                previous = current;
                current = current->next;
            }
            //now set next from there to null
            previous->next = NULL;
            //free the page
            free_page((kma_page_t*)page);
            return;
            
        }
        pageheader* nextPage = page->next;
        previousPage->next = nextPage;
        while(((void*)current < ((void*)page+PAGE_SIZE)) && current!=NULL)
        {
            if ((void*)current < (void*)page)
            {
                //update previous
                previous = current;
            }
            //update current
            current = current->next;
        }
        previous->next = current;
        free_page((kma_page_t*)page);
        return;
    }
    
 //shouldn't get here at all
 free_page((kma_page_t*)page);
}


#endif // KMA_RM
