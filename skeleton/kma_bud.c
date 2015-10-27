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

typedef struct page_node
{
    void* ptr;
    kma_page_t* pagePtr;
    void* next;
    char bitmap[32];
    //bitmap struct???
} pagenode;


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
    void* ptrs[9];
    int counter;
    void* pageListHead;
} pageheader;


/************Global Variables*********************************************/

//number of free lists for a minimum size of 32 is 8:
//free lists for sizes 32, 64, 128, 256, 512, 1024, 2048, 4096
kma_page_t* globalPtr = NULL;
int requestNumber = 0;

/************Function Prototypes******************************************/
void initialize_books();
void allocate_new_page();
blocknode* getFreeBlock(kma_size_t size);
void update_bitmap(void* ptr,kma_size_t size);
void initialize_books();
int getListIndex(kma_size_t size);
kma_size_t adjustSize(kma_size_t num);
blocknode* split_free_to_size(kma_size_t size, blocknode* node);
void remove_from_list(blocknode* node);
void fix_pointers();
blocknode* add_to_list(void* ptr,kma_size_t size, kma_page_t* pagePtr);
void coalesce_blocks(void* ptr,kma_size_t size, int fromRecursion);
blocknode* findBlock(void* ptr, kma_size_t size);
int findBuddy(void* buddyAddr,kma_size_t size);
void free_pages();
void* findPagePtr(void* ptr);
void remove_from_pagelist(void* pagePtr);
void addPageNode(void* ptr,void* pagePtr);
void* findPagePtr(void* ptr);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void* kma_malloc(kma_size_t size)
{
    requestNumber++;
    printf("\n\n REQUEST NUMBER %d TO ALLOCATE BLOCK OF SIZE %d\n",requestNumber,size);

    size = adjustSize(size);
    printf("Adjusted size to: %d\n",size);
    if (size>PAGE_SIZE)
    {
        return NULL;
    }
    
    if (!globalPtr)
    {
        initialize_books();
    }
    
    //return address
    blocknode* returnAddress;
    
    returnAddress = getFreeBlock(size);

    printf("RETURN ADDRESS %p\n", returnAddress);

    if (returnAddress!=NULL)
    {
	printf("RETURN ADDRESS IS ----------------------->>>>> %p\n",returnAddress->ptr);
	void* rAddress = (void*)(returnAddress->ptr);
    	remove_from_list(returnAddress);
	    //found a free block, update the bitmap
        //update_bitmap(returnAddress,size);
        return rAddress;
    }
    
    //did not find a free block
    //allocates new page and puts it in the list of blocks 
    if (size==PAGE_SIZE)
    {
        //allocate new page
        //add that as a 'free block' to blocks of size PAGESIZE
        //and return that
        kma_page_t* newFirstPage = get_page();
        //add_to_list(newFirstPage->ptr,PAGE_SIZE,newFirstPage);
	addPageNode(newFirstPage->ptr,(void*)newFirstPage);
        return newFirstPage->ptr;
    }

    allocate_new_page(); 
    
    //try again, this time it should be good
    returnAddress = getFreeBlock(size);
	printf("RETURN ADDRESS IS ----------------------->>>>>%p-> %p\n",returnAddress,returnAddress->ptr);
    
    if (returnAddress!=NULL)
    {
        //update bitmap, return
	void* rAddress = (void*)(returnAddress->ptr);
	remove_from_list(returnAddress);
        //update_bitmap(returnAddress,size);  
      return rAddress;
    }
    
    //else, not capable of giving that request, return null
    return NULL;

}


void kma_free(void* ptr, kma_size_t size)
{

    requestNumber++;
    printf("\n\n REQUEST NUMBER %d TO FREE BLOCK %p  OF SIZE %d\n",requestNumber,ptr,size);

  size = adjustSize(size);
    
  if (size==PAGE_SIZE){
    
        //figure out the page that will be freed
        //pageheader* page = (pageheader*)(globalPtr->ptr);    
        //move on to the list page
        //page = page->next;
        //move on to the full page blocks page
        //pageheader* blockPage = (pageheader*)(page->ptrs[8]);
        //blocknode* firstNode = (blocknode*)blockPage->firstBlock;
        //now find it
        //while (firstNode->ptr!=ptr)
        //{
        //    firstNode = firstNode->next;
        //}
    
        //remove it as if it was a free node
        void* pagePtr = (void*)(findPagePtr(ptr));;
        remove_from_pagelist(pagePtr);
        free_page(pagePtr);
        //update_bitmap(ptr,size);
        free_pages();
        return;
  }

  //update_bitmap(ptr,size);
  
	printf("coalescing blocks if possible\n");
  coalesce_blocks(ptr,size,0);
    
  free_pages();

}

void free_pages()
{

    
    printf("should be scanning bitmap by page and deleting pages as needed\n");
    
    //right now, it just prints all the pages and the freelist at each page

    pageheader* page = (pageheader*)(globalPtr->ptr);
    printf("Bitmap page at %p \n ",page);
    //page = page->next;
    printf("Lists page at %p \n ",page);
    printf("Printing lists: \n");
    int flag = 1;
    int j=0;
    for (j=0;j<9;j++)
    {
        printf("\t %d -> %p \n",j,page->ptrs[j]);
        if (page->ptrs[j]!=NULL)
        {
		flag = 0;
            pageheader* blockPage = (pageheader*)(page->ptrs[j]);
            blocknode* current = blockPage->firstBlock;
            while (current!=NULL)
            {
                printf(" %p -> ",current->ptr);
                current = current->next;
            }
        }
    }
	if (flag==1)
	{
		printf("freeing everything \n");
		//free_page(page->ptr);
		free_page(globalPtr);
		globalPtr = NULL;
	}
	else {
		printf("some stuff left.. wait \n");
	}


}

void allocate_new_page()
{
    //allocates new page split in 2 blocks of PAGE_SIZE/2

    kma_page_t* newPage = get_page();
    addPageNode((void*)newPage->ptr, (void*)newPage);
    
    void* leftChildAddr = newPage->ptr;
    void* rightChildAddr = (void*) ((int)(newPage->ptr)+(PAGE_SIZE/2));

    add_to_list(leftChildAddr,PAGE_SIZE/2,newPage);
    add_to_list(rightChildAddr,PAGE_SIZE/2,newPage);
    return;

}


void update_bitmap(void* ptr,kma_size_t size)
{
    //the boolean above is true if you are setting a block to used
    //false if you are setting a block to free
    
    //IDEA: don't need a used boolean because whether you are freeing or allocating
    //you always need to flip the bits in question to their opposite state
    //this can be done using an XOR
    
    //need to implement the following sequence of steps:
    //find the page where the pointer points to using the addresses of each page in the page list and the page size
    //find which bits in the bitmap to change
    //XOR the bits in question with 1 to flip them
    
    pageheader* page = (pageheader*)(globalPtr->ptr);
    
    pagenode* currentPageNode = (pagenode*)(page->pageListHead);
    
    while (!((void*)(ptr) > (void*)(currentPageNode->ptr) && (void*)(ptr) < (void*)((int)(currentPageNode->ptr) + PAGE_SIZE)))
    {
        currentPageNode = currentPageNode->next;
    }
    //now you have reached the page that contains the specified address
    
    //offset within the page
    int pageOffset = (int)(ptr) - (int)(currentPageNode->ptr);
    int startingBit = pageOffset / MIN_SIZE;
    
    //the starting char within the page's bitmap
    int startingChar = startingBit / 8;
    //the starting bit within that char in the page's bitmap
    int charOffset = startingBit % 8;
    
    //this is the number of full chars that will be changed
    //if size is less than 256 bytes, less than a full char will be changed
    int sizeInChars = size / MIN_SIZE / 8;
    
    //also need to know how many bits of partial char need to be changed
    int bitsInChar = size / MIN_SIZE % 8;
    
    //since size is always an even power of two:
    //it will either have sizeInChars > 0 OR bitsInChar > 0
    //never both, so you can make these two separate cases
    //in case where sizeInChars > 0, charOffset should be 0
    //because it will always start at the beginning of a char so it is aligned
    
    //size fits into a part of one char of the bitmap
    if (sizeInChars == 0)
    {
        //go through the bits in the char that need to be changed
        int i;
        for (i = charOffset; i < charOffset + bitsInChar; i++)
        {
            currentPageNode->bitmap[startingChar] ^= 1 << (8 - i - 1);
        }
    }
    //size fills at least one full char in the bitmap
    else
    {
        //go through all the chars that you need to update, and update all of their bits
        int j;
        for (j = startingChar; j < startingChar + sizeInChars; j++)
        {
            int k;
            for (k = 0; k < 8; k++)
            {
                currentPageNode->bitmap[j] ^= 1 << (k);
            }
        }
    }
    
    
    return;   
}

void initialize_books()
{
    //int usablePageSize = (unsigned int)PAGESIZE - sizeof(kma_page_t) - sizeof(page_t) - sizeof(listhead);
    //there is nothing here yet
    
    //get a page for bitmap 
    
    kma_page_t* newFirstPage = get_page();
globalPtr = newFirstPage;
    pageheader* firstPageHead;

    //put it in the first place 
    *((kma_page_t**)newFirstPage->ptr) = newFirstPage;
   // globalPtr = (kma_page_t*)(newFirstPage->ptr);

    firstPageHead = newFirstPage->ptr;
    firstPageHead->next = NULL;
    firstPageHead->pType = BITMAP;
    firstPageHead->counter=0;
    firstPageHead->firstBlock = NULL;
    int i=0;
    for (i=0;i<=8;i++)
    {
        firstPageHead->ptrs[i] = NULL;
	printf("Pointer %d location is %p \n", i, firstPageHead->ptrs[i]);
    }
    // firstPageHead->ptrs = NULL;

    
    //kma_page_t* newListPage = get_page();
    //*((kma_page_t**)newListPage->ptr) = newListPage;
    
    //pageheader* newListPageHead = newListPage->ptr;
    //pageheader* secondPagePtr = newListPage->ptr;
    //newListPageHead->next = NULL;
    //newListPageHead->counter=0;
    //newListPageHead->pType = LISTS;
    //newListPageHead->firstBlock = NULL;
    //i=0;
    //for (i=0;i<=8;i++)
    //{
    //    newListPageHead->ptrs[i] = NULL;
	//printf("Pointer %d location is %p \n",i,newListPageHead->ptrs[i]);
    //}
    
    //make them connected
    //firstPageHead->next = newListPageHead;
	//printf("ptr first: %p \n",firstPageHead->next);
	//firstPageHead->next = newListPage->ptr;    
	//printf("ptr after: %p \n",firstPageHead->next);
//	globalPtr->ptr = firstPageHead;
	printf("global Ptr points to : %p\n",globalPtr->ptr); 
    printf("bitmap page: %p points to %p \n",firstPageHead,firstPageHead->next);
    //printf("list page: %p points to %p\n",newListPageHead,newListPageHead->next);
	//firstPageHead->next = newListPage->ptr;
	printf("bitmap page: %p points to : %p \n",firstPageHead,firstPageHead->next);  
	printf("global ptr points to %p \n",globalPtr->ptr);
	printf("END OF INITIALIZING BOOK KEEPING -----------------\n\n");
  return;
}

blocknode* getFreeBlock(kma_size_t size)
{
    //walk through the blocks until you find one that fits
    pageheader* page = (pageheader*)(globalPtr->ptr);    
	//printf("page at is %p and points to %p\n",globalPtr->ptr,page->next);
    //move on to the list page
    //page = page->next;
   //printf("list page is %p \n",page);
    //listhead pointers = page->ptrs;
    
    //step through size 
    int index = getListIndex(size);
    int origIndex = index;
//	printf("trying to get free block of size %d \n",size);
    while (index<=7)
    {
	void* pointer = page->ptrs[index];
        if (pointer!=NULL)
        {
  //      	printf("Found free block! in page %p \n",pointer);
	    break;
        }
    //    printf("couldn't find at level %d, looking above\n",index);
	index++;
    }

    if (index==8)
    {
        //did not find a good page
        return NULL;
    }
    
    if (index==origIndex)
    {
        //go into that page
        pageheader* blockPage = (pageheader*)(page->ptrs[index]);
        return (blocknode*)(blockPage->firstBlock);
    }
    else
    {
        //split into blocks and then return
	pageheader* blockPage = (pageheader*)(page->ptrs[index]);
        return split_free_to_size(size,(blocknode*)(blockPage->firstBlock));
    }
    return NULL;
    
}

int getListIndex(kma_size_t size)
{
    //gets log base 2 and decreases 5 (32 byte minimum)
    kma_size_t start = size;
    kma_size_t result = 0;
    while (start > 1)
    {
        result++;
        start = start >> 1; 
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
//	printf("splitting node from size %d to size %d\n",node->size,size);
        //node is too big
        //make children and recurse on left child
        void* leftChild = (void*)(node->ptr);
        void* rightChild = (void*)((int)(leftChild) + ((int)node->size)/2);
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
	printf("removing node %p of size %d \n",node,node->size);
    //removes a block from a list of just the block that we have, does not coalesce. just remove
    int listIndex = getListIndex(node->size);
    pageheader* page = (pageheader*)(globalPtr->ptr);    

    //move to list page
    //page = page->next;
    pageheader* blockPage = (pageheader*)(page->ptrs[listIndex]);
    blocknode* firstNode = (blocknode*)(blockPage->firstBlock);
    blocknode* current = firstNode;
    
    if (current==NULL)
    {
        //why? 
        return;
    }

    blocknode* previous = NULL;

    while (current!=node)
    {
        previous = current;
        current = current->next;
    }
    //now current has the pointer to the node that we will remove
    if (previous==NULL && current->next==NULL)
    {
        //only one node
        page->ptrs[listIndex] = NULL;
        //free the page where that node existed
        
    	pageheader* pageTop = (void*)(((int)node>>13)<<13);
    	printf("*** freeing page for lists of size %d\n",listIndex);        
    	free_page(pageTop->ptr);
        return;
    }

    if (current->next!=NULL) 
    {
        //move everything back by 1 node (within the same size)
        //move current ahead by one
	previous = current;
        current = current->next;
       // previous = previous->next;

        //block to remove is at previous, and step through
        while(current!=NULL)
        {

	    printf("The previous pointer is %p\n", previous->ptr);
	    printf("Previous->next is %p\n", previous->next);
	    printf("The current pointer is %p\n", current->ptr);
	    printf("Current is %p\n", current);

            //copy the block node in front to the back
            previous->size = current->size;
            previous->ptr = current->ptr;
            //previous->next = current->next;
            previous->pagePtr = current->pagePtr;
	    
	    printf("The previous pointer is now %p\n", previous->ptr);
	    printf("Previous->next is %p\n", previous->next);
	    printf("The current pointer is now %p\n", current->ptr);
	    printf("Current is %p\n", current);    

	    if (current->next == NULL)
	    {
	    	previous->next = NULL;
	    }
	    else
	    {
		previous = current;
	    }
            current = current->next;
	    
	    
        }

        //fix pointers
        //fix_pointers();
	previous->next = NULL;
	return;
    }
   //last case, it's the last one but not the first one
    previous->next = NULL;
	return;
}

void fix_pointers()
{
    // //steps through the list and fixes any pointers that we did not align 
    // pageheader* page = (pageheader*)(globalPtr->ptr);
    // page = page->next;
    // blocknode* firstNode = (blocknode*) (page->firstBlock);
    // blocknode* current = firstNode;

    // int nodeSize;
    // int listIndex;
    // int oldSize = 0;
    // int marked[8] = {0,0,0,0,0,0,0,0};
    // while (current!=NULL)
    // {
    //     if (current->size != oldSize)
    //     {
    //         //get list index
    //         listIndex = getListIndex(current->size);
    //         page->ptrs[listIndex] = (void*)current;
    //         oldSize = current->size;
    //         marked[listIndex] = 1;
    //     }
    // }
    // //set to null if not in the list
    // for (int i=0;i<8;i++)
    // {
    //     if (marked[listIndex]==0)
    //     {
    //         page->ptrx[listIndex] = NULL;
    //     }
    // }
    return;
}


blocknode* add_to_list(void* ptr,kma_size_t size, kma_page_t* pagePtr)
{
//	printf("adding size %d to end of list\n",size);
    //add to end of the list of its size 
    int listIndex = getListIndex(size);
    pageheader* page = (pageheader*)(globalPtr->ptr);    

    //there is no longer a list page
    //page = page->next;
    pageheader* blockPage = (pageheader*)(page->ptrs[listIndex]);
   // printf("Adding to list size %d at %p and page for this is %p \n",size,ptr,firstNode);

    //if firstNode is null, get a new page for the list of these nodes
    if (blockPage==NULL)
    {
//	printf("getting new page \n");
        kma_page_t* newListPage = get_page();
        *((kma_page_t**)newListPage->ptr) = newListPage;
  //      printf("new page points to %p \n",newListPage->ptr); 
        pageheader* newListPageHead = (pageheader*)newListPage->ptr;
	
        newListPageHead->next = NULL;
        newListPageHead->counter=0;
        newListPageHead->pType = LISTS;
        newListPageHead->firstBlock = (void*)((int)newListPageHead + sizeof(pageheader));; 

        //link the array to this page
        page->ptrs[listIndex] = (void*)newListPageHead;
//	printf("pointer of list %d points to new pageList %p \n",listIndex,page->ptrs[listIndex]);
        //and put the first node in the list
        blocknode* firstBlock = newListPageHead->firstBlock;
        firstBlock->ptr = ptr;
        firstBlock->size = size;
        firstBlock->next = NULL;
        firstBlock->pagePtr = pagePtr;


        return firstBlock;
    }

    blocknode* current = blockPage->firstBlock;

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
//	printf("created new node %p which %p points to -> %p (%p) \n",newNode,previous,previous->next,newNode->ptr);
    return newNode;
}
void coalesce_blocks(void* ptr,kma_size_t size,int fromRecursion)
{
//from recurstion -> if 1, will find and remove the block if it was found. otherwise, it won't look for it

	printf("trying to coalesce a block at ptr %p and of size: %d \n",ptr,size);

	//find buddy
	void* buddyAddr = (void*)((int)ptr ^ (int)size);
//    printf("buddy address is %p \n",buddyAddr);
	int fBuddy;
	fBuddy =  findBuddy(buddyAddr,size);
	//if buddy found, coalesce
	if (fBuddy==0)
	{
    	//buddy is busy, so not in free block
   	 
    	//add the block to the list if not already there
   	printf("couldn't find buddy \n");   
   	if (fromRecursion==0)
    	{
        	//find the block's pagePtri
   	 printf("not from recursion, so find pointer and add to list \n");
        	void* pagePtr = findPagePtr(ptr);
        	add_to_list(ptr,size,pagePtr);
       	return;
    	}
    	else
    	{
   	 printf("from recursion, so do nothing \n");
   	 	//do nothing. block already in list
        	return;
    	}
	}
    
	if (fBuddy==1)
	{
    	//    printf("found buddy \n");
    	//coalesce these 2 and remove from list
    	//find one
   	//no need to find this one because it's not there! looking for its buddy so must be free
    printf("found buddy \n");
   	if (fromRecursion==0)
    	{
   		  //blocknode* node = findBlock(ptr,size);
        	//find the other
        	blocknode* buddy = findBlock(buddyAddr,size);
        	//    printf("buddy address is %p \n",buddy);
        	//if size is PAGE_SIZE
        	if (buddy->size*2 == PAGE_SIZE)
        	{
            	//delete and free the page
            	kma_page_t* pagePtr = buddy->pagePtr;
            	free_page(pagePtr);
            	remove_from_pagelist(pagePtr);
            	//  remove_from_list(node);
           	//remove the buddy
            	remove_from_list(buddy);
            	return;
        	}
        	else
        	{
            	//add a new one to list
            	void* lowerAddress = ptr;
            	if (ptr > buddy->ptr)
            	{
               	lowerAddress = buddy->ptr;
            	}
                	//add to list
    	//   	 printf("adding to list node of size %d*2 at %p \n",buddy->size,lowerAddress);
            	blocknode* parentNode = add_to_list(lowerAddress, buddy->size*2, buddy->pagePtr);
            	//and remove previous ones from list
   	       	//remove only the buddy
            	//remove_from_list(node);
            	remove_from_list(buddy);

            	return coalesce_blocks(parentNode->ptr,parentNode->size,1);
        	}
    	}   
    	else {
   		 printf("looking for block \n");
   	 blocknode* node = findBlock(ptr,size);
   		 printf("looking for buddy \n");
   	 blocknode* buddy = findBlock(buddyAddr,size);// printf("buddy address is %p \n",buddy);
   		 printf("evaluating on size to free page or add to list \n");
   	 if (buddy->size*2==PAGE_SIZE) {
   	   	printf("freeing page of buddy %p  at ptr %p \n",buddy,buddy->pagePtr);
   		 void* pagePtr = findPagePtr(buddy->ptr);
   		 printf("Freeing page with pagePtr: %p \n",pagePtr);
   		 free_page(pagePtr);
   	   	remove_from_pagelist(buddy->pagePtr);
   	   	remove_from_list(node);
   	   	remove_from_list(buddy);
   	   	return;
        	}
        	else {
            	void* lowerAddress = ptr;
            	if (ptr>buddy->ptr) {
                	lowerAddress = buddy->ptr;
            	}
           		 printf("Adding to list node of size %d*2 at %p \n",buddy->size,lowerAddress);
   			 blocknode* parentNode = add_to_list(lowerAddress,buddy->size*2,buddy->pagePtr);
   			 remove_from_list(node);
   			 remove_from_list(buddy);
   			 return coalesce_blocks(parentNode->ptr,parentNode->size,1);
       	}
    	}
	}
}

blocknode* findBlock(void* ptr, kma_size_t size)
{
	int listIndex = getListIndex(size);
	pageheader* page = (pageheader*)(globalPtr->ptr);    

	//there is no longer a list page
	//page = page->next;
    pageheader* blockPage = (pageheader*)(page->ptrs[listIndex]);
    blocknode* firstNode = (blocknode*)(blockPage->firstBlock);
   // blocknode* firstNode = (blocknode*)(page->ptrs[listIndex]);
	blocknode* current = firstNode;

	while (current->ptr != ptr)
	{
    	current = current->next;
	}
	return current;
}


int findBuddy(void* buddyAddr,kma_size_t size)
{
  // printf("looking for buddy \n");
	int listIndex = getListIndex(size);
	pageheader* page = (pageheader*)(globalPtr->ptr);    

	//no longer have a list page
	//page = page->next;
    pageheader* blockPage = (pageheader*)(page->ptrs[listIndex]);
    if (blockPage==NULL){
   	 //there are no free blocks here
   	 return 0;
    }
    blocknode* firstNode = (blocknode*)(blockPage->firstBlock);   
// blocknode* firstNode = (blocknode*)(page->ptrs[listIndex]);
	blocknode* current = firstNode;
//    printf("nodes begin at %p \n",current);
	while (current!=NULL)
	{
    	if (current->ptr == buddyAddr)
    	{
//   	 printf("found it! %p->%p \n",current,current->ptr);
        	return 1;
    	}
    	current = current->next;
	}
	//couldn't find it, return 0
//    printf("couldn't find it. return 0 \n");
	return 0;
}

void* findPagePtr(void* ptr)
{
	//go through the list of pages until we find one that corresponds
	//return its pagePtr (the one used to free the page)
    
	void* pageToFind = (void*)(((int)ptr>>13)<<13);
	printf("Page to find: %p \n",pageToFind);
	pageheader* page = (pageheader*)(globalPtr->ptr);
    
	//no longer have a list page
	//page = page->next;
    
	pagenode* currentPage = (pagenode*)(page->pageListHead);
	printf("firstPage: %p ",currentPage);
	while (currentPage!=NULL)
	{
    printf("currentPage: %p and looking for %p \n",currentPage->ptr,pageToFind);
    	if (currentPage->ptr==pageToFind)
    	{
   	 printf("found page, points to %p \n",currentPage->pagePtr);
        	return currentPage->pagePtr;
    	}
    currentPage = currentPage->next;
	}
	return NULL;
    
}
void addPageNode(void* ptr,void* pagePtr)
{
    
	pageheader* page = (pageheader*)(globalPtr->ptr);
    printf("adding page node for the page located at %p \n",ptr);    
	
	while (page->next != NULL)
	{
	    page = page->next;
	}
	
	//if the last page for page nodes is full, need to create a new page to add more
	if (page->counter >= 180)
	{
	    pagenode* beforeNew = (pagenode*)(page->pageListHead);
	    while (beforeNew->next != NULL)
	    {
	        beforeNew = beforeNew->next;
	    }
	    
	    kma_page_t* newPage = get_page();
        pageheader* newPageHead;

        //put it in the first place 
        *((kma_page_t**)newPage->ptr) = newPage;

        newPageHead = newPage->ptr;
        newPageHead->next = NULL;
        newPageHead->pType = BITMAP;
        newPageHead->counter=0;
        newPageHead->pageListHead = page->pageListHead;
        int i=0;
        for (i=0;i<=8;i++)
        {
            newPageHead->ptrs[i] = page->ptrs[i];
        }
        pagenode* newPageNode;
        newPageNode = (pagenode*)((int)(newPageHead) + sizeof(pageheader));
        
        newPageNode->ptr = ptr;
    	newPageNode->pagePtr = pagePtr;
    	for (i=0;i<32;i++)
    	{
        	newPageNode->bitmap[i] = 0;
    	}
    	
    	beforeNew->next = newPageNode;
    	page->next = newPageHead;
    	
    	return;
	    
	}
	
	//page with list of pages is first page so don't need to go to the next
	//page = page->next;
    
	pagenode* currentPageNode = (pagenode*)(page->pageListHead);
	pagenode* previousPageNode  = NULL;
	pagenode* newPageNode;
	int i=0;

	if (currentPageNode==NULL)
	{
    	currentPageNode = (pagenode*)((int)(page) + sizeof(pageheader));
	    printf("empty list starts at %p \n",currentPageNode);
    	currentPageNode->ptr = ptr;
    	currentPageNode->pagePtr = pagePtr;
    	for (i=0;i<32;i++)
    	{
        	currentPageNode->bitmap[i] = 0;
    	}
        currentPageNode->next = NULL;
        printf("added page node at %p and pagePtr points to %p \n",currentPageNode,currentPageNode->pagePtr);
   	    page->pageListHead =(void*)currentPageNode;
   	    
   	    //add to the list
   	    page->counter=1;
    }
	else
	{
	    int count = 0;
    	while (currentPageNode!=NULL)
    	{
		count++;
		printf("%d pages\n", count);
        	previousPageNode = currentPageNode;
        	currentPageNode = currentPageNode->next;
    	}

	    printf("Do we get here?\n");

    	//at the tail now
    	previousPageNode->next = (pagenode*)((int)(previousPageNode) + sizeof(pagenode));
    	newPageNode = previousPageNode->next;
    	newPageNode->ptr = ptr;
    	newPageNode->pagePtr = pagePtr;
    	for (i=0;i<32;i++)
    	{
        	newPageNode->bitmap[i] = 0;
    	}
    	
    	//add to the page counter
    	pageheader* currentPage = (pageheader*)(((int)(newPageNode)>>13)<<13);
    	currentPage->counter++;
        printf("New page counter is at %d \n",currentPage->counter);

	}
}

void remove_from_pagelist(void* pagePtr)
{
	pageheader* page = (pageheader*)(globalPtr->ptr);
    
	//there is no longer a list page
	//page = page->next;
    
	pagenode* currentPageNode = (pagenode*)(page->pageListHead);
	pagenode* previousPageNode  = NULL;

    printf("removing pageNode \n");
	//removes a block from a list of just the block that we have, does not coalesce. just remove
    
	if (currentPageNode==NULL)
	{
    	//why?
    	return;
	}

	while (currentPageNode->pagePtr!=pagePtr)
	{
    	previousPageNode = currentPageNode;
    	currentPageNode = currentPageNode->next;
	}
	//now current has the pointer to the node that we will remove
	if (previousPageNode==NULL && currentPageNode->next==NULL)
	{
    	//only one node
    	page->pageListHead = NULL;
    	page->counter--;
    	return;
	}

	if (currentPageNode->next!=NULL)
	{
    	//move everything back by 1 node (within the same size)
    	//move current ahead by one
    	previousPageNode = currentPageNode;
    	currentPageNode = currentPageNode->next;
   	// previous = previous->next;

    	//block to remove is at previous, and step through
    	while(currentPageNode!=NULL)
    	{
        	//copy the block node in front to the back
        	previousPageNode->ptr = currentPageNode->ptr;
        	//previousPageNode->next = currentPageNode->next;
        	previousPageNode->pagePtr = currentPageNode->pagePtr;
        	int j=0;
        	for (j=0;j<32;j++)
        	{
            	previousPageNode->bitmap[j] = currentPageNode->bitmap[j];
        	}

		if (currentPageNode->next == NULL)
		{
		    previousPageNode->next = NULL;
		    previousPageNode = currentPageNode;
		}
		else
		{
		    previousPageNode = currentPageNode;
		}
        	currentPageNode = currentPageNode->next;
    	}
    	
    	//decrease the counter for the page wherever previousPageNode is
    	pageheader* currentPage = (pageheader*)(((int)(previousPageNode)>>13)<<13);
    	currentPage->counter--;
    	
    	if (currentPage->counter==0)
    	{
    	    //free that page
    	    printf("freeing a bookkeping page of pagenodes \n");
    	    pageheader* pPage = (pageheader*)(globalPtr->ptr);
    	    if (currentPage->ptr!=globalPtr)
    	    {
    	        //step through until you get to the last one
    	        while (pPage->next!=currentPage)
    	        {
    	            pPage = pPage->next;
    	        }
    	        printf("found it, freeing and setting pointer to null \n");
    	        //at the one before the one you will free
    	        //free the page
    	        free_page(currentPage->ptr);
    	        //and set the pointer of next page to null 
    	        pPage->next = NULL;
    	        return;
    	    }
    	    printf("that was the first page, so not freeing yet. Also, you should never be here \n");
    	}

    	//fix pointers
    	//fix_pointers();
        return;
	}
   //last case, it's the last one but not the first one
	previousPageNode->next = NULL;
    return;
    
    
}


#endif // KMA_BUD
