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

typedef struct list_head
{
    void* lists[8];
} listhead;

typedef struct block_node
{
  int size;
  void* ptr;
  void* next;
  kma_page_t* pagePtr;
} blocknode;

typedef enum 
{
    BITMAP, LISTS,DATA
} page_type;

typedef struct page_head
{
    kma_page_t* ptr;
    void* next;
    page_type pType;
    void* firstBlock;
    listhead ptrs;
} pageheader;


/************Global Variables*********************************************/

//number of free lists for a minimum size of 32 is 8:
//free lists for sizes 32, 64, 128, 256, 512, 1024, 2048, 4096
kma_page_t* globalPtr = NULL;
int requestNumber = 0;

/************Function Prototypes******************************************/
void initialize_books();
blocknode* getFreeBlock(kma_page_t size);
void update_bitmap(void* ptr,kma_size_t size);
void initialize_books();
blocknode* getFreeBlock(kma_page_t size);
int getListIndex(kma_size_t size);
kma_size_t adjustSize(kma_size_t num);
blocknode* split_free_to_size(kma_size_t size, blocknode* node);
void remove_from_list(blocknode* node);
void fix_pointers();
blocknode* add_to_list(void* ptr,kma_size_t size, kma_page_t* pagePtr);
void coalesce_blocks(void* ptr,kma_size_t size);
blocknode* findBlock(void* ptr, kma_size_t size);
int findBuddy(void* buddyAddr,kma_size_t size);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void* kma_malloc(kma_size_t size)
{
    requestNumber++;
    printf("REQUEST NUMBER %d TO ALLOCATE BLOCK OF SIZE %d\n",requestNumber,size);

    size = adjustSize(size);
    if (size>PAGE_SIZE)
    {
        return NULL;
    }
    
    if (!globalPtr)
    {
        initialize_books();
    }
    
    //return address
    void* returnAddress;
    
    returnAddress = get_free_block(size);

    if (returnAddress!=NULL)
    {
        //found a free block, update the bitmap
        update_bitmap(returnAddress,size);
        return returnAddress->ptr;
    }
    
    //did not find a free block
    //allocates new page and puts it in the list of blocks 
    if (size==PAGE_SIZE)
    {
        //allocate new page
        //add that as a 'free block' to blocks of size PAGESIZE
        //and return that
        kma_page_t* newFirstPage = get_page();
        addtoList(newFirstPage->ptr,PAGE_SIZE,newFirstPage);

        return newFirstPage->ptr;
    }

    allocate_new_page(); 
    
    //try again, this time it should be good
    returnAddress = getFreeBlock(size);
    
    if (returnAddress!=NULL)
    {
        //update bitmap, return
        update_bitmap();
        return returnAddress->ptr;
    }
    
    //else, not capable of giving that request, return null
    return NULL;

}


void kma_free(void* ptr, kma_size_t size)
{

    requestNumber++;
    printf("REQUEST NUMBER %d TO FREE BLOCK OF SIZE %d\n",requestNumber,size);

  size = adjustSize(size);
    
  if (size==PAGE_SIZE){

    //figure out the page that will be freed
    pageheader* page = (pageheader*)(globalPtr->ptr);    
    //move on to the list page
    page = page->next;
    //move on to the full page blocks page
    pageheader* page = (pageheader*)(page->ptrs[7]);
    blocknode* firstNode = page->firstBlock;
    //now find it
    while (firstNode->ptr!=ptr)
    {
        firstNode = firstNode->next;
    }

    //remove it as if it was a free node
    remove_from_list(firstNode);
  }

  update_bitmap(ptr,size);
  

  coalesce_blocks(ptr,size);
  
  add_to_free_list(ptr,size);
  
  free_pages();

}

void update_bitmap(void* ptr,kma_size_t size)
{
    return;   
}

void initialize_books()
{
    int usablePageSize = (unsigned int)PAGESIZE - sizeof(kma_page_t) - sizeof(page_t) - sizeof(listhead);
    //there is nothing here yet
    
    //get a page for bitmap 
    
    kma_page_t* newFirstPage = get_page();

    //put it in the first place 
    *((kma_page_t**)newFirstPage->ptr) = newFirstPage;
    globalPtr = (kma_page_t*)(newFirstPage->ptr);

    newFirstPage->next = NULL;
    newFirstPage->type = BITMAP;
    newFirstPage->counter=0;
    newFirstPage->firstBlock = NULL;
    newFirstPage->pointers = NULL;
    

    kma_page_t* newListPage = get_page();
    *((kma_page_t**)newListPage->ptr) = newListPage;
    
    newListPage->next = NULL;
    newListPage->counter=0;
    newListPage->type = LISTS;
    newListPage->firstBlock = NULL;
    int i=0;
    for (i=0;i<8;i++)
    {
        newListPage->ptrs[i] = NULL;
    }
    
    //make them connected
    newFirstPage->next = newListPage;
    
    printf("bitmap page: %p\n",newFirstPage);
    printf("list page: %p\n",newListPage);
    return;
}

blocknode* getFreeBlock(kma_page_t size)
{
    //walk through the blocks until you find one that fits
    pagehader* page = (pageheader*)(globalPtr->ptr);    

    //move on to the list page
    page = page->next;
    
    listhead pointers = page->ptrs;
    
    //step through size 
    int index = getListIndex(size);
    int origIndex = index;
    while (index<7)
    {
        if (page->ptrs[index]!=NULL)
        {
            break;
        }
        index++;
    }

    if (index==7)
    {
        //did not find a good page
        return NULL;
    }
    
    if (index==origIndex)
    {
        //go into that page
        pageheader* page = (pageheader*)(page->ptrs[index]);
        return (blocknode*)(page->firstBlock);
    }
    else
    {
        //split into blocks and then return
        return split_free_to_size(size,(blocknode*)(page->ptrs[index]));
    }
    
}

int getListIndex(kma_size_t size)
{
    //gets log base 2 and decreases 5 (32 byte minimum)
    kma_size_t start = size;
    kma_size_t result = 0;
    while (start > 1)
    {
        result++;
        start >> 1; 
    }
    return result-5;
    
    
}

kma_size_t adjustSize(kma_size_t num)
{
    kma_size_t power = num - 1;
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

blocknode* split_free_to_size(kma_size_t size, blocknode* node)
{
    if (node->size==size)
    {
        return node;
    }
    else {
        //node is too big
        //make children and recurse on left child
        void* leftChild = (void*)(node->ptr);
        void* rightchild = (void*)((int)(leftChild) + ((int)node->size)/2);
        kma_page_t* childrenPage = (kma_page_t*)(node->pagePtr);
        kma_size_t childrenSize = node->size/2;
        remove_from_list(node);

        blocknode* lChild = add_to_list(leftChild,childrenSize,childrenPage);
        blocknode* rChild = add_to_list(rightChild,childrenSize,childrenPage);
        return split_free_to_size(size,lChild);
    }
}

void remove_from_list(blocknode* node)
{
    //removes a block from a list of just the block that we have, does not coalesce. just remove
    int listIndex = getListIndex(node->size);
    pageheader* page = (pageheader*)(globalPtr->ptr);    

    //move to list page
    page = page->next;
    blocknode* firstNode = (blocknode*)(page->ptrs[listIndex]);
    blocknode* current = firstNode;
    
    if (current==NULL)
    {
        //why? 
        return;
    }

    blocknode* previous = NULL;

    while (current!=node)
    {
        previous = previous->current;
        current = current->next;
    }

    //now current has the pointer to the node that we will remove
    if (previous==NULL)
    {
        //only one node
        page->ptrs[listIndex] = NULL;
        //free the page where that node existed
        (pageheader*) pageTop = (void*)(((int)node>>13)<<13);
        free_page(pageTop->ptr);
        return;
    }
    else 
    {
        //move everything back by 1 node (within the same size)
        //move current ahead by one
        current = current->next;
        previous = previous->next;

        //block to remove is at previous, and step through
        while(current!=NULL)
        {
            //copy the block node in front to the back
            previous->size = current->size;
            previous->ptr = current->ptr;
            previous->next = current->next;
            previous->pagePtr = current->pagePtr;


            //and step to the next element
            previous = current;
            current = current->next;
        }

        //fix pointers
        //fix_pointers();
    }
}

void fix_pointers()
{
    //steps through the list and fixes any pointers that we did not align 
    pageheader* page = (pageheader*)(globalPtr->ptr);
    page = page->next;
    blocknode* firstNode = (blocknode*) (page->firstBlock);
    blocknode* current = firstNode;

    int nodeSize;
    int listIndex;
    int oldSize = 0;
    int marked[8] = {0,0,0,0,0,0,0,0};
    while (current!=NULL)
    {
        if (current->size != oldSize)
        {
            //get list index
            listIndex = getListIndex(current->size);
            page->ptrs[listIndex] = (void*)current;
            oldSize = current->size;
            marked[listIndex] = 1;
        }
    }
    //set to null if not in the list
    for (int i=0;i<8;i++)
    {
        if (marked[listIndex]==0)
        {
            page->ptrx[listIndex] = NULL;
        }
    }
}


blocknode* add_to_list(void* ptr,kma_size_t size, kma_page_t* pagePtr)
{
    //add to end of the list of its size 
    int listIndex = getListIndex(size);
    pageheader* page = (pageheader*)(globalPtr->ptr);    

    //move to list page
    page = page->next;
    blocknode* firstNode = (blocknode*)(page->ptrs[listIndex]);
    blocknode* current = firstNode;


    //if firstNode is null, get a new page for the list of these nodes
    if (firstNode==NULL)
    {
        kma_page_t* newListPage = get_page();
        *((kma_page_t**)newListPage->ptr) = newListPage;
        
        newListPage->next = NULL;
        newListPage->counter=0;
        newListPage->type = LISTS;
        newListPage->firstBlock = (void*)((int)newListPage + sizeof(pageheader));; 

        //and put the first node in the list
        firstBlock->ptr = ptr;
        firstBlock->size = size;
        firstBlock->next = NULL;
        firstBlock->pagePtr = pagePtr;


        return firstBlock;
    }

    //otherwise, go through list and add it at the end
    blocknode* previous = NULL;
    while(current!=NULL)
    {
        //go through the list
        previous = current;
        current = current->next;
    }
    //now add it at the end of that list
    previous->next = previous+sizeof(blocknode);
    blocknode* newNode = previous->next;
    newNode->ptr = ptr;
    newNode->size = size;
    newNode->next = NULL;
    newNode->pagePtr = pagePtr;

    return newNode;
}

void coalesce_blocks(void* ptr,kma_size_t size)
{

    //trying to free a block at ptr and of size: size

    //find buddy
    void* buddyAddr = (void*)((int)ptr ^ (int)size);
    int findBuddy(buddyAddr,size);
    //if buddy found, coalesce
    if (findBuddy==0)
    {
        //buddy is busy, so not in free block
        //just take this block out of the list
        //find the block in the list of blocks
        blocknode* node = findBlock(ptr,size);
        //and remove from list(block)
        remove_from_list(node);
        return;
    }
    
    if (findBuddy==1)
    {
        //coalesce these 2 and remove from list
        //find one
        blocknode* node = findBlock(ptr,size);
        //find the other
        blocknode* buddy = findBlock(ptr,size);

        //if size is PAGE_SIZE
        if (node->size*2 == PAGE_SIZE)
        {
            //delete and free the page
            kma_page_t* pagePtr = node->pagePtr;
            free_page(pagePtr);
            remove_from_list(node);
            remove_from_list(buddy);
            return;
        }
        else
        {
            //add a new one to list
            void* lowerAddress = node->ptr;
            if (node->ptr > buddy->ptr)
            {
                lowerAddress = buddy->ptr;
            }
            //add to list
            blocknode* parentNode = add_to_list(lowerAddress, node->size*2, node->pagePtr);
            //and remove previous ones from list
            remove_from_list(node);
            remove_from_list(buddy);

            coalesce_blocks(parentNode,parentNode->size);
        }
    }
}

blocknode* findBlock(void* ptr, kma_size_t size)
{
    int listIndex = getListIndex(size);
    pageheader* page = (pageheader*)(globalPtr->ptr);    

    //move to list page
    page = page->next;
    blocknode* firstNode = (blocknode*)(page->ptrs[listIndex]);
    blocknode* current = firstNode;

    while (current->ptr != ptr)
    {
        current = current->next;
    }
    return current;
}


int findBuddy(void* buddyAddr,kma_size_t size)
{
    int listIndex = getListIndex(size);
    kma_page_t* page = (kma_page_t*)(globalPtr->ptr);    

    //move to list page
    page = page->next;
    blocknode* firstNode = (blocknode*)(page->ptrs[listIndex]);
    blocknode* current = firstNode;

    while (current!=NULL)
    {
        if (current->ptr == buddyAddr)
        {
            return 1;
        }
        current = current->next;
    }
    //couldn't find it, return 0
    return 0;
}


#endif // KMA_BUD
