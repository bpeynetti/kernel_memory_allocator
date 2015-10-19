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
enum BLOCK_STATE
  {
    FREE,
    USED
  };
  
typedef struct block_node
{
  int size;
  void* ptr;
  void* next;
  int pageId;
 // void* previous;
  //enum BLOCK_STATE state;
  
} blocknode;

typedef struct page_node
{
    int id;
    kma_page_t* ptr;
    int counter;
    void* next;
} pagenode;

// free list pointer
blocknode *freeList = NULL;

// busy list pointer
blocknode *usedList = NULL;

// free page list
pagenode *pageList = NULL;

/************Global Variables*********************************************/

//define a pointer kma_struct_t that points to the beginning of everything



/************Function Prototypes******************************************/
void addToUsed(kma_size_t size, void* address,int pageId);
void addPageToList(kma_page_t* page);
void printLists(bool choose);
void freeMyPage(pagenode* page);
pagenode* changePageCounter(int pageid, int delta);
void printPageList();



/************External Declaration*****************************************/

/**************Implementation***********************************************/

void* kma_malloc(kma_size_t size)
{
    //printf("ADDING A BLOCK OF SIZE %d\n",size);

    //printf("\t\tPrinting list of used nodes \n");
    //printLists(FALSE);
    //printf("\t\tPrinting list of free nodes \n");
    //printLists(TRUE);
    
    void* address;
    kma_page_t* page;
    
    //empty, so get a new page
    if (freeList == NULL){
        //get page
        page = get_page();
        addPageToList(page);
        
        //get size needed from the page and allocate that (add to used list)
        freeList = malloc(sizeof(blocknode));
        
        freeList-> ptr = page->ptr + sizeof(kma_page_t*) + size;
        freeList-> size = page->size - sizeof(kma_page_t*) - size;
        freeList-> next = NULL;
        freeList->pageId = page->id;
        //add the remainder as a node to the list of free space
        
        address = page->ptr+sizeof(kma_page_t*);
        
        //add to used list: size and ptr
        addToUsed(size, address,page->id);
        
        return address;
        
    }
    
    else
    {
        
        blocknode* nextFree = freeList;
        blocknode* previousFree = NULL;
        while (nextFree != NULL)
        {
            if (nextFree->size >= size)
            {
                address = nextFree->ptr;
                if (nextFree->size == size)
                {
                    if (previousFree == NULL)
                    {
                        freeList = nextFree->next;
                    }
                    else
                    {
                        previousFree->next = nextFree->next;
                    }
                    //delete the node 
                    free(nextFree);
                }
                else
                {
                    nextFree->size = nextFree->size - size;
                    nextFree->ptr =address + size;
                }
                addToUsed(size,address,nextFree->pageId);
                return address;
            }
            previousFree = nextFree;
            nextFree = nextFree-> next;
        }
        
        page = get_page();
        printf("Adding new page %d \n",page->id);
        addPageToList(page);
        printPageList();

        
        blocknode* newPageFreeBlock = malloc(sizeof(blocknode));
        
        newPageFreeBlock->ptr = page->ptr + sizeof(kma_page_t*) + size;
        newPageFreeBlock->size = page->size - sizeof(kma_page_t*) - size;
        newPageFreeBlock->next = freeList;
        newPageFreeBlock->pageId = page->id;
        freeList = newPageFreeBlock;
        
        address = page->ptr + sizeof(kma_page_t*);
        
        addToUsed(size, address,page->id);
        
        return address;
        
    }
    
    
    
  //never gets here
 
  return NULL;
}

void kma_free(void* ptr, kma_size_t size)
{
    //printf("FREEING A BLOCK OF SIZE %d\n",size);
    pagenode* pageNode;

    //printf("\t\tPrinting list of used nodes \n");
    //printLists(FALSE);
    //printf("\t\tPrinting list of free nodes \n");
    //printLists(TRUE);
    
    blocknode* current = usedList;
    blocknode* previous = NULL;
    //now step through checking the address
    //when found, switch that node over from the used to the free list
    void* address = ptr;
    while (current != NULL){
        if (current->ptr==address){
            //found, so move it to the free list

            if (previous!=NULL){
                //link previous to the next one
                previous->next = current->next;
            }
            else{
                usedList = current->next;
            }
            
            //and add the temp to the start of the free list
            current->next = freeList;
            freeList = current;
            
            //decrease from number of blocks in the pagenode
            pageNode = changePageCounter(current->pageId,-1);
            if (pageNode->counter==0){
                  freeMyPage(pageNode);
            }
            return;
        }
        else {
            //just step to the next one
            previous = current;
            current = current->next;
        }
    }
    //should never get here
    printf("Error -> address to free is not in the list of used blocks\n");
    
}


void addToUsed(kma_size_t size, void* address,int pageId) 
{
    blocknode* newNode;
    newNode = malloc(sizeof(blocknode));
    newNode->ptr = address;
    newNode->size = size;
    newNode->next = usedList;
    newNode->pageId = pageId;
    usedList=newNode;
    
    pagenode* Page;
    Page = changePageCounter(newNode->pageId,1);
    if (Page==NULL){
        
    }
    
}

void printLists(bool choose){
    blocknode* current = NULL;
    if (choose==TRUE){
        current = freeList;
    }
    else{
        current = usedList;
    }
    int id=0;
    while(current!=NULL){
        printf("\t\tblock %d in page %d with size: %d \n ",id,current->pageId,current->size);
        id++;
        current = current->next;
    }
    printf("end of list --------   \n");
}

void addPageToList(kma_page_t* page)
{
    pagenode* newPage;
    newPage = malloc(sizeof(pagenode));
    newPage->id = page->id;
    newPage->ptr = page;
    newPage->counter = 0;
    newPage->next = pageList;
    
    pageList = newPage;
}

pagenode* changePageCounter(int pageid, int delta)
{
    pagenode* current = pageList;
    while (current != NULL)
    {
        if (current->id == pageid)
        {
            current->counter += delta;
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void freeMyPage(pagenode* page)
{
    printf("Freeing page %d \n",page->id);
    blocknode* currentFreeNode = freeList;
    blocknode* previousFreeNode = NULL;
    //first delete all nodes from free list
    while (currentFreeNode != NULL)
    {
        if (currentFreeNode->pageId == page->id)
        //get rid of node from free list
        {
            if (previousFreeNode == NULL)
            {
                freeList = currentFreeNode->next;
            }
            else
            {
                previousFreeNode->next = currentFreeNode->next;
            }
            free(currentFreeNode);
            break;

        }
        previousFreeNode = currentFreeNode;
        currentFreeNode = currentFreeNode->next;
    }
    //now delete the node from the list of page nodes
    pagenode* currentPageNode = pageList;
    pagenode* previousPageNode = NULL;
    
    while (currentPageNode != NULL)
    {
        if (currentPageNode->id == page->id)
        {
            if (previousPageNode == NULL)
            {
                pageList = currentPageNode->next;
            }
            else
            {
                previousPageNode->next = currentPageNode->next;
            }
            free(currentPageNode);
            break;
        }
        previousPageNode = currentPageNode;
        currentPageNode = currentPageNode->next;
    }
    free_page(page->ptr);
    printPageList();
}

void printPageList()
{
    pagenode* current = pageList;
    while (current!=NULL){
        printf("%d -> ",current->id);
        current = current->next;
    }
    printf("\n");
}


#endif // KMA_RM
