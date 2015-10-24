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
    int page_count;
    void* next;
    blockheader* blockHead;
    int pageid;
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

int lineCounter = 0;


/************Function Prototypes******************************************/
void* kma_malloc(kma_size_t size);
void* findFreeBlock(kma_size_t size);
void kma_free(void* ptr, kma_size_t size);
void addToList(void* ptr,kma_size_t size);
void freeMyPage(pageheader* page);
void printPageList();
void new_page(kma_page_t* newPage);


/************External Declaration*****************************************/

/**************Implementation***********************************************/

void* kma_malloc(kma_size_t size)
{

    lineCounter++;
    void* returnAddress = NULL;

    printf("THIS IS REQUEST NUMBER %d\n", lineCounter);


    if ((size + sizeof(kma_page_t*)) > PAGE_SIZE)
    {
        return NULL;
    }
    //
    if (size < sizeof(kma_page_t))
    {
        size = sizeof(kma_page_t);
    }
    //
    if (globalPtr==NULL)
    {
        globalPtr = get_page();
        new_page(globalPtr);
    }

    returnAddress = findFreeBlock(size);

    if (returnAddress==NULL)
    {
        new_page(get_page());
    }

    returnAddress = findFreeBlock(size);

    return returnAddress;







   // if (size<16){
//	size=16;
  // }
    
    //printf("Allocating a new block of memory of size %d\n", size);
   //if (globalPtr != NULL)
    //{
    //    printf("    ");
    //    pageheader* firstPage = (pageheader*) (globalPtr->ptr);
    //    printf("First page: %p",firstPage);
	//printf("that page points to %p \n",firstPage->ptr);
	//firstPage->ptr = (kma_page_t*)firstPage;    
   	// blockheader* currentBlock = (blockheader*)(firstPage->blockHead);
       // while (currentBlock != NULL)
       // {
       //     printf("%d->", currentBlock->size);
        //    currentBlock = currentBlock->next;
       // }
       // printf("\n");
   // }
    
    
    ///////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////
 //    //ignore requests for more size than a page
 //    if(size + sizeof(void*)>PAGESIZE) return NULL;
    
 //    if (globalPtr==NULL)
 //    {
 //        //initialize everything    
 //        //get a new page, put it to global ptr
 //        kma_page_t* newFirstPage = get_page();
	
 //        //put it in the first place 
 //        *((kma_page_t**)newFirstPage->ptr) = newFirstPage;
 //        globalPtr = newFirstPage;

 //        //printf("global points to  %p \n",globalPtr->ptr);
	// //printf("globalPtr is at %p \n",globalPtr);
        
 //        //create the page header
 //        pageheader* pageHead;
 //        //and set the location (address) of the head to the beginning of the page
 //        pageHead = (pageheader*) (newFirstPage->ptr);
 //        pageHead->ptr = newFirstPage;
	// //printf("pageHead->ptr points to %p\n",pageHead->ptr);
 //        pageHead->counter = 1;
 //        //pageHead->pagecount=1;
 //        pageHead->next = NULL;
 //        pageHead->blockHead = globalPtr->ptr + sizeof(pageheader) + size;
        
 //       // printf("First block at %p \n",pageHead->blockHead);
        
 //        blockheader* listHead;
 //        listHead = (blockheader*)(pageHead->blockHead);
 //        listHead->size = PAGE_SIZE  - sizeof(pageheader)  - size;
 //        listHead->next = NULL;
        
 //        return globalPtr->ptr + sizeof(pageheader);
 //    }
    
 //   void* returnAddress;
   
 //    returnAddress = findFreeBlock(size);
    
 //    if (returnAddress!= NULL)
 //    {
 //        //printf("ALLOCATED a new block of memory of size %d\n", size);
 //        //if (globalPtr != NULL)
 //        //{
 //        //    printf("    ");
 //        //    pageheader* firstPage = (pageheader*) (globalPtr->ptr);
 //        //    blockheader* currentBlock = (blockheader*)(firstPage->blockHead);
 //        //    while (currentBlock != NULL)
 //        //    {
 //        //        printf("%d->", currentBlock->size);
 //        //        currentBlock = currentBlock->next;
 //        //    }
 //        //  printf("\n");
 //       // }
 //        return returnAddress;
 //    }
	// printf("\n\n\n ********************* GETTING NEW PAGE **** \n");
	// printf("THIS IS REQUEST NUMBER %d\n", lineCounter);    
 //    pageheader* currentPage = (pageheader*) (globalPtr->ptr);
    
 //    while (currentPage->next != NULL)
 //    {
 //        currentPage = currentPage->next;
 //    }
    
 //    kma_page_t* pageTemp = get_page();
 //    printf("PageTemp kma_struct_t* at  %p\n",pageTemp);
 //    *((kma_page_t**)pageTemp->ptr) = pageTemp;
 //    kma_page_t* localPtr = pageTemp;
    
 //    pageheader* newPageHead;
 //    newPageHead = (pageheader*) (pageTemp->ptr);
 //    newPageHead->ptr = pageTemp;
 //    newPageHead->counter = 0;
 //    newPageHead->next = NULL;
    
 //    currentPage->next = newPageHead;
 //    newPageHead->next=NULL;    
 //    pageheader* firstPageHead = (pageheader*) (globalPtr->ptr);
 //    newPageHead->blockHead = firstPageHead->blockHead;
    
 //    blockheader* currentBlock;
 //    currentBlock = (blockheader*)(newPageHead->blockHead);
   
 //    printf("New page starts at %p\n",newPageHead);
 //    printf("New page points to %p\n",newPageHead->ptr);
 
 //    while (currentBlock->next != NULL)
 //    {
 //        currentBlock = currentBlock->next;
 //    }
    
 //    blockheader* newBlock = (blockheader*)((int)newPageHead + sizeof(pageheader));
 //    newBlock->size = PAGE_SIZE - sizeof(pageheader);
 //    newBlock->next = NULL;
    
 //    currentBlock->next = newBlock;
 //    //printf("newBlock address is %p and size %d\n",newBlock,newBlock->size);
   
 //    printf("Before freeing the page the page list looks like this: \n");
 //        pageheader* pageIterator = (pageheader*) (globalPtr->ptr);
 //        while (pageIterator != NULL)
 //        {
 //                printf("Addr: %p, Ctr: %d -> ", pageIterator, pageIterator->counter);
 //                pageIterator = pageIterator->next;
 //        }
 //        printf("\n");


 
 //    return findFreeBlock(size);
}


void new_page(kma_size_t* newPage)
{
    printf("getting new page \n \n ");
    //gets a pointer to a new page
    //allocates a free block size of the page - header
    //links the last block to the first new block
    //copies information from the previous page

    *((kma_page_t**)newPage->ptr) = newPage;
    pageheader* newPageHead;
    newPageHead = (pageheader*) (newPage->ptr);
    newPageHead->ptr = newPage;
    newPageHead->counter = 0;
    newPageHead->next = NULL;
    newPageHead->blockHead = NULL;
    newPageHead->pageid = 0


    //link pages to each other
    if (globalPtr==NULL)
    {
        globalPtr = newPageHead;
    }
    else 
    {
        pageheader* firstPage = globalPtr;
        pageheader* previousPage = NULL;
        while (firstPage!=NULL)
        {
            previousPage = firstPage;
            firstPage = firstPage->nextPage;
        }

        //now add to next one
        newPageHead->pageid = previousPage->pageid++;
        previousPage->next = newPageHead;
        newPageHead->blockHead = previousPage->blockHead;
    }

    //add to list a block of size pageSize - header at location page+pageheader
    addToList((void*)((int)newPageHead+sizeof(pageheader)),(PAGE_SIZE-sizeof(pageheader));
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

    //printf("free block starts at %p\n",current);
    
    while (current!=NULL)
    {
        printf("Free block: %p\n",current);
        while (!((void*)current>(void*)(currentPage) && (void*)current < (void*)((int)(currentPage) + PAGE_SIZE)))
        {
	    	if (currentPage->next == NULL)
	    	{
				break;
	    	}
            currentPage = currentPage->next;
        }
        //now go through the list and find a free block
        if (current->size >= size)
        {
            returnAddr = current;
            oldSize = current->size;
               
	
            //printf("return address at %p \n",returnAddr);
            // take out of linked list
            //if head of the blocks
            if (previous==NULL)
            {

                //change the ptr to the next free block
                if (current->next==NULL){
                    //only one block!
                    
			if (current->size - size < 8)
			{
				//need to skip remaining block
				pageHead->blockHead = NULL;
			}
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

			else
			{

	                        //printf("Current pointer: %p\n", current);
        	                current = (void*)((int)(current)+size);
                	        //printf("Current pointer: %p\n", current);
                      	 	   //and replicate the size from the old size - size
                    	 	   current->size = oldSize - size;
                       		 current->next = NULL;
                        
                       		 //update the head of list to this one
                       		 pageHead->blockHead = current;
                        }
                        currentPage->counter++;
                        //and return the old address 
                        return (void*)returnAddr;
                    // }
                }
                else 
                {
                    //you're at the beginning of your list and the list is more than one
                    //you know it's big enough so just change its size and change the location of the header
                   
		    if (current->size - size < 8)
		    {
			 pageHead->blockHead = current->next;
		    }
		    else
		    {
			 //printf("Size of block found is %d\n", current->size);
			 tempNext = current->next;
                         current = (void*)((int)(current)+size);

                   	 current->size = oldSize - size;
			 //printf("After allocating, size is %d\n", current->size);
                  	  //and copy pointer to next
                  	  current->next = tempNext;
                   	 //no previous one so update header
                   	 pageHead->blockHead = current;
                   	 //add one to page counter
		    }
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
		    if (current->size - size < 8)
		    {
			previous->next = NULL;
		    }
		    else
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
                                                current = (void*)((int)(current)+size);

                        //and replicate the size from the old size - size
                        current->size = oldSize - size;
                        current->next = NULL;
                        
                        //update the previous next to this one
                        previous->next = current;
                   }     
                    currentPage->counter++;
                    
		    printf("Before freeing the page the page list looks like this: \n");
       		    pageheader* pageIterator = (pageheader*) (globalPtr->ptr);
         	    while (pageIterator != NULL)
        	    {
        	        printf("Addr: %p, Ctr: %d -> ", pageIterator, pageIterator->counter);
               		 pageIterator = pageIterator->next;
        	    }
       		     printf("\n");


		    //and return the old address 
                    return (void*)returnAddr;
                    // }
                }
                else 
                {
                    //normal case, something in front and something behind
                    returnAddr = current;
                    //if size of block is within accepted  to size+sizeof(blocknode)
                    if (current->size-size<sizeof(blockheader)) //if the difference between them is smaller than what we need for a new node
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
                                                current = (void*)((int)(current)+size);

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

	lineCounter++;	

	printf("THIS IS REQUEST NUMBER %d\n", lineCounter);

  // if(size<16){ size=16; } 
    
    //printf("Freeing a block of memory of size %d at address %p \n", size,ptr);
    //if (globalPtr != NULL)
    //{
        //printf("    ");
       // pageheader* firstPage = (pageheader*) (globalPtr->ptr);
	//printf("first page at %p\n",firstPage);
        //blockheader* currentBlock = (blockheader*)(firstPage->blockHead);
        //printf("First free block of size %d at %p and points to %p \n",currentBlock->size,currentBlock,currentBlock->next);
	//while (currentBlock != NULL)
        //{
            //printf("%d-> (%p)", currentBlock->size,currentBlock->next);
        //    currentBlock = currentBlock->next;
       // }
       // printf("\n");
    //}
    
    //first need to add the requested memory location to the free list
    addToList(ptr,size);
    
    //then need to find the page the freed block belongs to and decrement counter for that page
    pageheader* currentPage = (pageheader*) (globalPtr->ptr);
    pageheader* nextPage = currentPage->next;
    //printf("CurrentPage->next = %p\n",nextPage);
    int pageID=1; 
    //the pointer specified is in the first page
    if (ptr < (void*)((int)currentPage+PAGE_SIZE))
    {
        //printf("First page counter is %d and decrement by 1 \n",currentPage->counter);
	currentPage->counter--;
    }
    
    //the pointer specified is not in the first page
    else
    {
        while (nextPage != NULL && (nextPage!=currentPage))
        {
            //if the pointer is between the addresses of the next two pages
            if (ptr > (void*)(currentPage) && ptr < (void*)((int)(currentPage) + PAGE_SIZE))
            {
		printf("Page %p counter %d, decrement 1 \n",currentPage,currentPage->counter);
                currentPage->counter--;
                break;
            }
        
            currentPage = currentPage->next;
            nextPage = nextPage->next;
        }
        
        //if you make it this far and are at the end of the list, the pointer is in the last page
        if (nextPage == NULL || nextPage==currentPage)
        {
		//printf("counter of last page: %d \n",currentPage->counter);
            currentPage->counter--;
        }
        
    }
    
    
    //then, if the counter has reached 0, free the page
    if (currentPage->counter==0)
    {
	printf("TRYING TO FREE A PAGE------------------------------\n");
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
    newBlock->size = size; 
    newBlock->next = NULL;

    //if the first one in the list. point to that one and return
    if (current==NULL)
    { 
        pageHead->blockHead = newBlock;
        return;
    }

    //if before the first entry in the list, add before and connect them
    if (ptr<current)
    {
        newBlock->next = current;
        pageHead->blockHead = newBlock;
        return;   
    }

    //ALL OTHER CASES
    //go through the blocks and find a place to put it
    bool inPage = false;

    //move through pages to find where the block belongs in the pages
    while (((void*)ptr >> 13) != ((void*)currentPage >> 13))
    {
        if (currentPage->next == NULL) {break;}
        currentPage = currentPage->next;
    }

    //now step through until you find out the block that you need
    //we want to put the block in page currentPage
    //current has the blockhead
    while (current!=NULL)
    {
        if ( (current>>13) == (ptr>>13) )
        {
            inPage = TRUE;
            pageVisited = TRUE;          
        }
        pageheader* cPage = (pageheader*)((current>>13)<<13);
        pageheader* pPage = (pageheader*)((ptr>>13)<<13);

        if (cPage->pageid > pPage->pageid)
        {
            //current passed the page
            break;
        }

        if (inPage==TRUE)
        {
            //if it's inside the page and current ahead of ptr
            if (current > ptr)
            {
                break;
            }
        }

        //otherwise, just keep going through
        previous = current;
        current = current->next;
    }

    if (current==NULL)
    {
        newBlock->size = size;
        newBlock->next = NULL;
        previous->next = NULL;
    }
    else
    {
        newBlock->next = current;
        previous->next = newBlock;
    }
    return;
}





// 	bool inPage = false;
// 	bool pageVisited = false;
// 	//figure out what page your pointer is at
// 	 while (!((void*)ptr>(void*)(currentPage) && (void*)ptr < (void*)((int)(currentPage) + PAGE_SIZE)))
//         {
// 	    	if (currentPage->next == NULL)
// 	    	{
// 				break;
// 	    	}
//             currentPage = currentPage->next;
//     }
//     while ((pageVisited==false || inPage==true)&&(current!=NULL))
//     {
//     	if (((void*)current>(void*)(currentPage) && (void*)current < (void*)((int)(currentPage) + PAGE_SIZE)))
//     	{
//     		//within the bounds of the page
//     		inPage = true;

//     	}
//     	if (inPage==true && !(((void*)current>(void*)(currentPage) && (void*)current < (void*)((int)(currentPage) + PAGE_SIZE))))
//     	//if current passed over ptr but previous hasn't 
//     	{
//     		pageVisited = true;
//     		inPage = false;
//     	}

//         if (inPage==true && (void*)ptr<(void*)current)
//         {

//             //stepped over, now we have previous and next
//             //link them 
//             //printf("\t Found a place to add the block. \n");
//             //size done before
// 		//just connect the previous to this one
// 		//and this one to the next one
//             newBlock->next = current;
//             previous->next = newBlock;
// 	  //  printf("Node at %p with size %d pointing to it is  %p (%p) and from this, next is %p",newBlock,newBlock->size,previous,previous->next,newBlock->next);
//             return;
//         }
//         previous = current;
//         //current = current->next;
// 		//printf("Next to look at is %p\n",previous->next);
// 		current = previous->next;
//     }

//     //if you reached the end of the list
//     //add it to the end (previous is the tail now)
//     //printf("\t couldn't find a place, put it at the end \n");
//     if (current==NULL)
//     {
//     	newBlock->size = size;
//     	newBlock->next = NULL;
//     	previous->next = newBlock;
//     	return;
//     }

//     newBlock->next = current;
//     previous->next = newBlock;
//     return;

// }

void freeMyPage(pageheader* page)
{
	printf("Attempting to free page %p, %p\n",page,page->ptr);
    	printf("THIS IS REQUEST NUMBER %d\n", lineCounter);
    pageheader* currentPage;
    pageheader* previousPage = NULL;

    currentPage = (pageheader*) (globalPtr->ptr);

    blockheader* previous = NULL;
    blockheader* current = currentPage->blockHead;

    //it's at the beginning of the list. make it point to the first free node
   
	printf("Before freeing the page the page list looks like this: \n");
	pageheader* pageIterator = (pageheader*) (globalPtr->ptr);
	while (pageIterator != NULL)
	{
		printf("Addr: %p, Ctr: %d -> ", pageIterator, pageIterator->counter);
		pageIterator = pageIterator->next;
	}
	printf("\n");
 	

    //
    // if it's the first one to free
    if (page==currentPage)
    {
        //if no more pages
        if (currentPage->next==NULL)
        {
            //you're done!
		printf("freeing page %p",page);
            free_page((kma_page_t*)page->ptr);
            return;
        }
        else
        {
            //find the first node in a next page
	     while (((void*)current > (void*)currentPage) && ((void*)current < (void*)((int)currentPage+PAGE_SIZE)))
	     {
		if (current->next==NULL) { break;}
		current = current->next; 
	     }
         //   while (((void*)current < ((void*)((int)page+PAGE_SIZE))) && (current!=NULL))
         //   {
         //       current = current->next;
         //   }

            //current has the new first node
            //and new first page is next page
            //*((kma_page_t**)globalPtr->ptr) = page->next;
            globalPtr = (kma_page_t*)page->next;
            
            pageheader* nextPage = page->next;
            nextPage->blockHead = current;
            printf("New head page is %p and block head at %p\n",globalPtr,nextPage->blockHead);
		free_page(page->ptr);
            return;
        }
    }
    else 
    {
        //not the first page
        //find the previous page
        pageheader* tempPage = (pageheader*)globalPtr->ptr;
        while (tempPage!=page)
        {
            previousPage = tempPage;
            tempPage = tempPage->next;
        }
        
        if (page->next==NULL)
        {
            //the last one
            while (!((void*)current > ((void*)page)) && ((void*)current<((void*)((int)page)+PAGE_SIZE)))
            {
                //step through until you find the first block in the page
                previous = current;
                current = current->next;
            }
            //now set next from there to null
            previous->next = NULL;
            //free the page
	    previousPage->next = NULL;
            free_page((kma_page_t*)page->ptr);
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
        free_page((kma_page_t*)page->ptr);
        return;
    }
    
 //shouldn't get here at all
// free_page((kma_page_t*)page);
}


#endif // KMA_RM
