/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the SVR4 lazy budy
 *             algorithm
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
 Project Group: bpv512,jjk612
 
 ***************************************************************************/
 
/********************************************************

IDEA OF LAZY BUDDY
	1. after adding a buffer to the free list
		 calculate slack = A - L for nodes of that size
		a) if slack < 2 -> try to coalesce
		b) if slack >=2 -> just add to list and keep new updated L value
	
	2. after getting a buffer to return an address
		a) update A (it will increase)
		b) if you removed anything from free list, then you would've updated there (L-1) = S++

	functions that will change the slack value
	1. malloc : add 1 to A right before you return (allocated block)
	2. split_node : add 1 to A when you set a node to busy ?? maybe not
	3. add_to_list: add 1 to L when adding some node to the list
	4. remove_from_list: subtract 1 from L when taking something out of the list

	- notes
	* when freeing, procedure will be:
		- remove 1 from A (the node that will be put into the free list, that is not allocated anymore)
		- 1st, get a free node of that size and put it in the free list (will automatically update slack value)
		- 2nd, read the slack value
		- if (slack < 2)
			- try to coalesce that level only

***************************************************************/

#ifdef KMA_LZBUD
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
  int local;
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
    //slack will have the counter for slack on each node
    int slack[9];
    //counter is the number of page nodes inside it for the BITMAP page types
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
blocknode* coalesce_blocks(void* ptr,kma_size_t size, int fromRecursion);
blocknode* findBlock(void* ptr, kma_size_t size);
int findBuddy(void* buddyAddr,kma_size_t size);
void free_pages();
void* findPagePtr(void* ptr);
void remove_from_pagelist(void* pagePtr);
void addPageNode(void* ptr,void* pagePtr);
void* findPagePtr(void* ptr);

void update_slack(kma_size_t size, int delta);
int getSlack(kma_size_t size);
bool isGloballyFree(void* ptr,kma_size_t size);
void manageFreeSlack(void* ptr, kma_size_t size, kma_size_t origSize);


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
	    //update_slack(size,1);
        update_bitmap(rAddress,size);
	free_pages();
        return rAddress;
    }
    
    //did not find a free block
    //allocates new page and puts it in the list of blocks 
    if (size==PAGE_SIZE)
    {
        //allocate new page
        //add that as a 'free block' to blocks of size PAGESIZE
        //and return that
		//	free_pages();
       	printf("allocating an entire page \n");
		kma_page_t* newPage = get_page();
        //add_to_list(newFirstPage->ptr,PAGE_SIZE,newFirstPage);
		printf("adding to page node list: pagePtr= %p and page at %p \n",newPage,newPage->ptr);
		addPageNode((void*)newPage->ptr,(void*)newPage);
		printf("RETURN ADDRESS IS %p \n",newPage->ptr);
       // free_pages();
		update_bitmap(newPage->ptr,PAGE_SIZE);
		//update_slack(PAGE_SIZE,1);
		return newPage->ptr;
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
        update_bitmap(rAddress,size);  
		//update_slack(size,1);
	free_pages();
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
        void* pagePtr = (void*)(findPagePtr(ptr));
		update_bitmap(ptr,size);
		remove_from_pagelist(pagePtr);
		printf("230 \n");
        free_page(pagePtr);
       // update_bitmap(ptr,size);
        // decrease A (slack = A-L by one)
        free_pages();
        return;
  }

  void* pagePtr = (void*)(findPagePtr(ptr));


  manageFreeSlack(ptr, size, size);

  
  /*update_bitmap(ptr,size);
  
  //printf("Changing the slack value by adding node of size %d to list \n",size);
  //get pagePtr required for the free node
  void* pagePtr = (void*)(findPagePtr(ptr));

  //usage of add_to_list: add_to_list(void* ptr,kma_size_t size, kma_page_t* pagePtr)

  add_to_list(ptr,size,pagePtr);
  //this will have changed the slack value already

  //evaluate slack for that size
  int slack = getSlack(size);

	printf("coalescing blocks if possible\n");

	//we need to coalesce this only by calling with a parameter 1 (as if it were recursion)
	//since the difference was that not from recursion assumes you didn't add to the free list
	//but we will have added the node to the free list to look at slack
  coalesce_blocks(ptr,size,0);
    */
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
    int count=0;
    for (j=0;j<9;j++)
    {
        //printf("\t %d -> %p -- %p| \n",j,page->ptrs[j],(int)(page->ptrs[j])+8192);
	count=0;
        if (page->ptrs[j]!=NULL)
        {
		count=0;
		flag = 0;
            pageheader* blockPage = (pageheader*)(page->ptrs[j]);
            blocknode* current = blockPage->firstBlock;
            blocknode* previous = NULL;
	    while (current!=NULL)
            {
               // printf(" %p (%p) -> ",current->ptr,current);
		previous = current;
                current = current->next;
            	count++;
	    }
	}
	    printf("\nlist: %d  COUNT: %d  and SLACK=%d \n",j,count,getSlack((int)(32<<j)));
        
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
		printf("size of node is %d ",sizeof(blocknode));
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
 //  printf("hi, trying to update bitmap at %p of size %d \n",ptr,size); 
    pageheader* page = (pageheader*)(globalPtr->ptr);
    
    pagenode* currentPageNode = (pagenode*)(page->pageListHead);
    void* searchingPage = (void*)((((int)(ptr))>>13)<<13); 
    
//	printf("searching for page %p \n",searchingPage);
//	printf("as a start, %p vs %p \n",currentPageNode->ptr,searchingPage);	
while (currentPageNode->ptr!=searchingPage)
   // while (!((void*)(ptr) > (void*)(currentPageNode->ptr) && (void*)(ptr) < (void*)((int)(currentPageNode->ptr) + PAGE_SIZE)))
    {
//	printf("current page node points to page: %p \n",currentPageNode->ptr);
        currentPageNode = currentPageNode->next;
    }
    //now you have reached the page that contains the specified address
  //  printf("here\n");
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
//	printf("this case \n");
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
//	printf("this other case \n");
        //go through all the chars that you need to update, and update all of their bits
        int j;
        for (j = startingChar; j < startingChar + sizeInChars; j++)
        {
	//	printf("j:%d : ",j);
            int k;
            for (k = 0; k < 8; k++)
            {
                currentPageNode->bitmap[j] ^= 1 << (k);
            }
        //	printf("%p \n",currentPageNode->bitmap[j]);
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
        firstPageHead->slack[i] = 0;
    }

	// printf("global Ptr points to : %p\n",globalPtr->ptr); 
 //    printf("bitmap page: %p points to %p \n",firstPageHead,firstPageHead->next);
 //    //printf("list page: %p points to %p\n",newListPageHead,newListPageHead->next);
	// //firstPageHead->next = newListPage->ptr;
	// printf("bitmap page: %p points to : %p \n",firstPageHead,firstPageHead->next);  
	// printf("global ptr points to %p \n",globalPtr->ptr);
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
        blocknode* firstFreeBlock = (blocknode*)(blockPage->firstBlock);
        if (firstFreeBlock->local == 0)
        {
            page->slack[index] += 1;
        }
        else
        {
            page->slack[index] += 2;
        }
        return firstFreeBlock;
        
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
    if (node->local == 1)
    {
        update_slack(node->size, 1);
        update_slack(node->size / 2, -2);
    }
    // else
    // {
    //     update_slack(node->size, 2);
    // }
    
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
        blocknode* left = (blocknode*)(leftChild);
        blocknode* right = (blocknode*)(rightChild);
        left->local = node->local;
        right->local = node->local;
        kma_page_t* childrenPage = (kma_page_t*)(node->pagePtr);
        kma_size_t childrenSize = node->size/2;
        remove_from_list(node);
        
        blocknode* lChild = add_to_list(rightChild,childrenSize,childrenPage);

        lChild = add_to_list(leftChild,childrenSize,childrenPage);
        return split_free_to_size(size,lChild);
    }
}

void remove_from_list(blocknode* node)
{
	printf("removing node %p of size %d \n",node,node->size);
    //removes a block from a list of just the block that we have, does not coalesce. just remove
    int listIndex = getListIndex(node->size);
    pageheader* page = (pageheader*)(globalPtr->ptr);    

    //update_slack(node->size,1);

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

	 //   printf("The previous pointer is %p\n", previous->ptr);
	 //   printf("Previous->next is %p\n", previous->next);
	 //   printf("The current pointer is %p\n", current->ptr);
	 //   printf("Current is %p\n", current);

            //copy the block node in front to the back
            previous->size = current->size;
            previous->ptr = current->ptr;
            //previous->next = current->next;
            previous->pagePtr = current->pagePtr;
	    
//	    printf("The previous pointer is now %p\n", previous->ptr);
//	    printf("Previous->next is %p\n", previous->next);
//	    printf("The current pointer is now %p\n", current->ptr);
//	    printf("Current is %p\n", current);    

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

    //update_slack(size,-1);  

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
    previous->next = (void*)((int)(previous)+sizeof(blocknode));
    blocknode* newNode = previous->next;
    newNode->ptr = ptr;
    newNode->size = size;
    newNode->next = NULL;
    newNode->pagePtr = pagePtr;
    if (getSlack(size) >= 2)
    {
        newNode->local = 1;
    }
    else
    {
        newNode->local = 0;
    }
	printf("created new node at %p whose previous is %p and size is 16 so prev+16 =%p \n",newNode,previous,(void*)((int)previous + 16));
    return newNode;
}

void manageFreeSlack(void* ptr, kma_size_t size, kma_size_t origSize)
{
    
    void* pagePtr = (void*)(findPagePtr(ptr));
    /*pageheader* page = (pageheader*)(globalPtr->ptr);
    pageheader* blockList = (pageheader*)(page->ptrs[getListIndex(size)]);
    blocknode* node = (blocknode*)(blockList->firstBlock);
    while ((void*)(node->ptr) != ptr)
    {
        node = node->next;
    }*/
    
    if (getSlack(size) >= 2)
  {
      //node->local = 1;
      add_to_list(ptr,size,pagePtr);
      update_slack(size, -2);
  }
/*
  else if (getSlack(size) == 1)
  {
      update_bitmap(ptr, size);
      add_to_list(ptr, size, pagePtr);
      coalesce_blocks(ptr, size, 0);
      set_slack(size, 0);
  }
*/
  else
  {
      //node->local = 0;
      update_bitmap(ptr, size);
      //add_to_list(ptr, size, pagePtr);
      int fromRecursion;
      if (size == origSize)
      {
          fromRecursion = 0;
      	  add_to_list(ptr,size,pagePtr);
	}
      else
      {
          fromRecursion = 1;
      }
      blocknode* parentBlock = coalesce_blocks(ptr, size, fromRecursion);
      //need to change coalesce so that it has a return:
      //if coalescing succeeds, return the coalesced block
      //otherwise, return the first item in the list in the next size up
      if (parentBlock != NULL)
      {
          //coalesce_blocks returns null only if you reach the page size or the next size up has an empty list
          manageFreeSlack((void*)parentBlock->ptr, size * 2, origSize);
      }
	//if slack is 0, try to coalesce the first one in the new list
	if (getSlack(size)==0)
	{
		//try to coalesce the first block in the list index+1
		pageheader* firstPage = (pageheader*)(globalPtr->ptr);
		firstPage = firstPage->ptrs[getListIndex(size*2)];
		if (firstPage!=NULL)
		{
			//coalesce on the first block
			blocknode* victimBlock = firstPage->firstBlock;
			blocknode* otherParentBlock = coalesce_blocks(victimBlock->ptr,size*2,fromRecursion);
			//
			if (otherParentBlock != NULL)
			{
				manageFreeSlack((void*)otherParentBlock->ptr,size*4,origSize*2);
			}
		}
	}		

      set_slack(size, 0);
  }
}

blocknode* coalesce_blocks(void* ptr,kma_size_t size,int fromRecursion)
{
//from recurstion -> if 1, will find and remove the block if it was found. otherwise, it won't look for it
//when fromRecursion = 1, you need to update the bitmap of the current size you are on
//


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
        	//add_to_list(ptr,size,pagePtr);
        //	int listIndex = getListIndex(size);
        //	pageheader* page = (pageheader*)(globalPtr->ptr);
        //	pageheader* blockPage = (pageheader*)(page->ptrs[listIndex]);
       	//return blockPage->firstBlock;
    		return NULL;
	}
    	else
    	{
   	 printf("from recursion, so do nothing \n");
   	 	//do nothing. block already in list
        //	int listIndex = getListIndex(size);
        //	pageheader* page = (pageheader*)(globalPtr->ptr);
        //	pageheader* blockPage = (pageheader*)(page->ptrs[listIndex]);
       	//return blockPage->firstBlock;
    		return NULL;
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
   		 blocknode* node = findBlock(ptr,size);
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
            	printf("784 \n");//  remove_from_list(node);
           	//remove the buddy
            	remove_from_list(buddy);
            	return NULL;
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
            	remove_from_list(node);
            	remove_from_list(buddy);

            	return parentNode;
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
   	   	printf("820 \n");
		remove_from_list(node);
   	   	remove_from_list(buddy);
   	   	return NULL;
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
   			 return parentNode;
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
	//printf("Page to find: %p \n",pageToFind);
	pageheader* page = (pageheader*)(globalPtr->ptr);
    
	//no longer have a list page
	//page = page->next;
    
	pagenode* currentPage = (pagenode*)(page->pageListHead);
	//printf("firstPage: %p ",currentPage);
	while (currentPage!=NULL)
	{
    //printf("currentPage: %p and looking for %p \n",currentPage->ptr,pageToFind);
    	if (currentPage->ptr==pageToFind)
    	{
   	 //printf("found page, points to %p \n",currentPage->pagePtr);
        	return currentPage->pagePtr;
    	}
    currentPage = currentPage->next;
	}
	return NULL;
    
}
void addPageNode(void* ptr,void* pagePtr)
{
    
	pageheader* page = (pageheader*)(globalPtr->ptr);
	pageheader* globalPg = (pageheader*)(globalPtr->ptr);
	pagenode* firstPageNode = (pagenode*)(globalPg->pageListHead);
    printf("adding page node for the page located at %p and pagePtr %p \n",ptr,pagePtr);    
	
	while (page->next != NULL)
	{
	    if (page->pType==BITMAP) { printf("type bitmap at %p and counter is %d \n",page,page->counter); }
	    if (page->pType==LISTS) { printf("type lists at %p \n ",page); }
	    page = page->next;
	}
	printf("Page where we add the node is %p \n",page);	
	//if the last page for page nodes is full, need to create a new page to add more
	if (page->counter >= 180)
	{
		printf("creating new page of pagenodes \n");
	    pagenode* beforeNew = firstPageNode;
	    while (beforeNew->next != NULL)
	    {
	        beforeNew = beforeNew->next;
	    }
	    
	    kma_page_t* newPage = get_page();
       		 pageheader* newPageHead;

        //put it in the first place 
        	*((kma_page_t**)newPage->ptr) = newPage;
		printf("I'm here! line 952 \n");
        	newPageHead = newPage->ptr;
        	newPageHead->next = NULL;
        	newPageHead->pType = BITMAP;
        	newPageHead->counter=1;
        	newPageHead->pageListHead = firstPageNode;
        	int i=0;
        	for (i=0;i<=8;i++)
        	{
        	    newPageHead->ptrs[i] = page->ptrs[i];
        	}
		printf("I made it to line 963 \n");
        	pagenode* newPageNode;
		printf("new page is at location %p and kma_ptr is %p \n",newPage,newPage->ptr);
        	newPageNode = (pagenode*)((int)(newPageHead) + sizeof(pageheader));
        	printf("and the new page node is at %p \n",newPageNode);
        	newPageNode->ptr = ptr;
    		newPageNode->pagePtr = pagePtr;
		newPageNode->next = NULL;
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
		return;
    }
	else
	{
//		free_pages();
		printf("not at a full page yet \n");
		printf("page at %p and counter is %d \n",page,page->counter);
	    int count = 0;
		printf("stepping through nodes \n");
	void* cPage; 
	int flag=0;
    	while (currentPageNode!=NULL && flag==0)
    	{
		count++;
		if (count>500) {	printf("%d pages and node at %p . last page is %p - count: %d vs  %d \n", count,currentPageNode,page,(count%180),page->counter); }
        	cPage = (void*)((((int)currentPageNode)>>13)<<13);
	//	if (count>500) { printf("cpage: %p and page: %p\n ",cPage,page); }
		if (cPage==page && ((count)%180==page->counter)) { flag=1; }
		previousPageNode = currentPageNode;
        	currentPageNode = currentPageNode->next;
		
    	}
	printf(" we have %d pagenodes in the total in this page \n",count);

	    printf("Do we get here?\n");

    	//at the tail nde
    	previousPageNode->next = (pagenode*)((int)(previousPageNode) + sizeof(pagenode));
    	newPageNode = previousPageNode->next;
    	newPageNode->ptr = ptr;
    	newPageNode->pagePtr = pagePtr;
	newPageNode->next = NULL;
    	for (i=0;i<32;i++)
    	{
        	newPageNode->bitmap[i] = 0;
    	}
    	printf("The last pagenode is at %p and the new one at %p \n",previousPageNode,newPageNode);
	printf("New page node points to data page %p \n",newPageNode->ptr);
    	//add to the page counter
    	pageheader* currentPage = (pageheader*)(((int)(newPageNode)>>13)<<13);
	printf("Address of page that holds the page node is %p \n",currentPage);
    	page->counter++;
        printf("New page counter is at %d \n",currentPage->counter);
	return;
	}
}

void remove_from_pagelist(void* pagePtr)
{
	pageheader* page = (pageheader*)(globalPtr->ptr);
    
	//there is no longer a list page
	//page = page->next;
    
	pagenode* currentPageNode = (pagenode*)(page->pageListHead);
	pagenode* previousPageNode  = NULL;

    printf("removing pageNode to page with ptr %p \n",pagePtr);
	//removes a block from a list of just the block that we have, does not coalesce. just remove
    
	if (currentPageNode==NULL)
	{
    	//why?
    	return;
	}

	while (currentPageNode->pagePtr!=pagePtr)
	{

		printf("IN THE LOOp %p \n",currentPageNode);

    	previousPageNode = currentPageNode;
    	currentPageNode = currentPageNode->next;
	}
	printf("out of loop \n");
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
	   //pagenode* node_to_disappear = previousPageNode;
    	previousPageNode = currentPageNode;
    	currentPageNode = currentPageNode->next;
   	// previous = previous->next;
    	//block to remove is at previous, and step through
    	while(currentPageNode!=NULL)
    	{
		printf("IN LOOPP %p -> %p (next) \n",currentPageNode,currentPageNode->next);
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
		    //node_to_disappear->next = NULL;
		    printf("Add %p we set the next to null \n",previousPageNode);
    		    previousPageNode->next = NULL;
    		    previousPageNode = currentPageNode;
    		}
    		else
    		{
			//node_to_disappear = previousPageNode;
    		    previousPageNode = currentPageNode;
    		}
            	currentPageNode = currentPageNode->next;
    	}
        printf("Prev: %p, current: %p \n",previousPageNode,currentPageNode);	
    	//decrease the counter for the page wherever previousPageNode is 
}
else {
	previousPageNode->next = NULL;
}
//now decrease the counter 
    	pageheader* currentPage = (pageheader*)(((int)(previousPageNode)>>13)<<13);
	printf("Page to decrease counter: %p from %d \n",currentPage,currentPage->counter);
    	currentPage->counter--;
    	
    	if (currentPage->counter==0)
    	{
    	    //free that page
    	    printf("freeing a bookkeping page of pagenodes \n");
    	    pageheader* pPage = (pageheader*)(globalPtr->ptr);
    	    if (currentPage->ptr!=globalPtr)
    	    {
    	        //step through until you get to the last one
    	        printf("loop here qwerty \n");
    	        while (pPage->next!=currentPage)
    	        {
		    printf("going through page %p \n",pPage);
    	            pPage = pPage->next;
    	        }
			printf("The current pPage is %p -> %p \n",pPage,pPage->next);
    	        printf("found it, we are freeing %p and setting pointer to null \n",pPage->next);
    	        //at the one before the one you will free
    	        //free the page
			pageheader* evictedPage = pPage->next;
    	        free_page(evictedPage->ptr);
    	        //and set the pointer of next page to null 
    	        pPage->next = NULL;
    	        return;
    	    }
    	    printf("that was the first page, so not freeing yet. Also, you should never be here \n");
    	}

    	//fix pointers
    	//fix_pointers();
        return;
//	}
   //last case, it's the last one but not the first one
//	previousPageNode->next = NULL;
 //   return;
}

void update_slack(kma_size_t size, int delta)
{
	//find the appropriate value and change it
	pageheader* page = (pageheader*)(globalPtr->ptr);

	int listIndex = getListIndex(size);

	//change by however much we need 
	page->slack[listIndex] = page->slack[listIndex] + delta;
	return;

	//uses
	// line 715 - add_to_list -> +1 to L for the given size. SLACK = A-L => -1
	// line 586 - remove_from_list -> -1 to L for the given size. SLACK = A-L => +1
	// lines 201 and 238 - kma_malloc -> +1 since SLACK = A-L, adding 1 to A
}

void set_slack(kma_size_t size, int value)
{
    pageheader* page = (pageheader*)(globalPtr->ptr);
    
    int listIndex = getListIndex(size);
    
    page->slack[listIndex] = value;
    return;
}

int getSlack(kma_size_t size)
{
	pageheader* page = (pageheader*)(globalPtr->ptr);

	int listIndex = getListIndex(size);
	//return the value of the slack
	return page->slack[listIndex];
}

bool isGloballyFree(void* ptr,kma_size_t size)
{
    //this function checks whether the bits in question in the bitmap are set
    
    pageheader* page = (pageheader*)(globalPtr->ptr);
    pagenode* currentPageNode = (pagenode*)(page->pageListHead);
    void* searchingPage = (void*)((((int)(ptr))>>13)<<13);
    
    while (currentPageNode->ptr!=searchingPage)
    {
        currentPageNode = currentPageNode->next;
    }
    
    int pageOffset = (int)(ptr) - (int)(currentPageNode->ptr);
    int startingBit = pageOffset / MIN_SIZE;
    
    int startingChar = startingBit / 8;
    int charOffset = startingBit % 8;
    
    //mask should have a 1 in the bit where the block starts in the bitmap
    char mask = 1 << (7 - charOffset);
    
    char globalOrLocal = mask & currentPageNode->bitmap[startingChar];
    return (globalOrLocal == 0);
    
    
}

#endif // KMA_LZBUD
