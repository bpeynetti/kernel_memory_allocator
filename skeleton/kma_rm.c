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
  
typedef struct block_list
{
  int size;
  void* ptr;
  void* next;
 // void* previous;
  //enum BLOCK_STATE state;
  
} blocklist;

// free list pointer
blocklist *freeList = NULL;

// busy list pointer
blocklist *usedList = NULL;

// 

/************Global Variables*********************************************/

//define a pointer kma_struct_t that points to the beginning of everything



/************Function Prototypes******************************************/
void addToUsed(kma_size_t size, void* address);
void printLists(bool choose);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void* kma_malloc(kma_size_t size)
{
    printf("Printing list of free nodes \n");
    printLists(TRUE);
    
    void* address;
    kma_page_t* page;
    
    //empty, so get a new page
    if (freeList == NULL){
        //get page
        page = get_page();
        
        //get size needed from the page and allocate that (add to used list)
        freeList = malloc(sizeof(blocklist));
        
        freeList-> ptr = page->ptr + sizeof(kma_page_t*) + size;
        freeList-> size = page->size - sizeof(kma_page_t*) - size;
        freeList-> next = NULL;
        //add the remainder as a node to the list of free space
        
        address = page->ptr+sizeof(kma_page_t*);
        
        //add to used list: size and ptr
        addToUsed(size, address);
        
        return address;
        
    }
    
    else
    {
        
        blocklist* nextFree = freeList;
        blocklist* previousFree = NULL;
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
                addToUsed(size,address);
                return address;
            }
            previousFree = nextFree;
            nextFree = nextFree-> next;
        }
        
        page = get_page();
        
        blocklist* newPageFreeBlock = malloc(sizeof(blocklist));
        
        newPageFreeBlock->ptr = page->ptr + sizeof(kma_page_t*) + size;
        newPageFreeBlock->size = page->size - sizeof(kma_page_t*) - size;
        newPageFreeBlock->next = freeList;
        freeList = newPageFreeBlock;
        
        address = page->ptr + sizeof(kma_page_t*);
        
        addToUsed(size, address);
        
        return address;
        
    }
    
    
    
  //never gets here
 
  return NULL;
}

void kma_free(void* ptr, kma_size_t size)
{
    printf("Printing list of used nodes \n");
    printLists(FALSE);
    
    blocklist* current = usedList;
    blocklist* previous = NULL;
    blocklist* temp = NULL;
    //now step through checking the address
    //when found, switch that node over from the used to the free list
    void* address = ptr;
    while (current != NULL){
        if (current->ptr==address){
            //found, so move it to the free list
            temp = current;
            
            if (previous!=NULL){
                //link previous to the next one
                previous->next = temp->next;
            }
            
            //and add the temp to the start of the free list
            temp->next = freeList;
            freeList = temp;
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


void addToUsed(kma_size_t size, void* address) 
{
    if (usedList == NULL){
        //then nothing in the list. add it to the list
        usedList = malloc(sizeof(blocklist));
        usedList->ptr = address;
        usedList->size = size;
        usedList->next = NULL;
        
    }
    else 
    {
        //already some list, so add it to  the end of the list
        blocklist* newNode;
        //create a new node to store the stuff
        newNode = malloc(sizeof(blocklist));
        newNode->ptr = address;
        newNode->size = size;
        newNode->next = usedList;
        
        //and make it the beginning of the list
        usedList = newNode;
        
    }
}

void printLists(bool choose){
    blocklist* current = NULL;
    if (choose==TRUE){
        current = freeList;
    }
    else{
        current = usedList;
    }
    int id=0;
    while(current!=NULL){
        printf("block %d with size: %d \n ",id,current->size);
        id++;
        current = current->next;
    }
    printf("end of list\n");
}


#endif // KMA_RM
