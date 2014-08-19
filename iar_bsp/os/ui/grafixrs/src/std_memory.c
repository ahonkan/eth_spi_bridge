/******************************************************************************
*
*           Copyright 1998 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
****************************************************************************
*******************************************************************************
*
* FILE NAME                                                             
*
*    std_memory.c                                                     
*                                                                        
*  DESCRIPTION                                                           
*                                                                        
*    Contains memory allocation and deallocation functions. Also contains global
*    data structure RecentMalloc which keeps track of memory allocations and 
*    deallocations.
*                                                                        
*  DATA STRUCTURES                                                       
*                                                                        
*    DebugAllocStruct                         
*                                                                        
*  FUNCTIONS    
*
*    Debug_Allocation
*    FindDebugAlloc
*    Debug_Deallocation
*                                                                        
*  DEPENDENCIES   
*
*    rs_base.h                                                                     
*    rsfonts.h
*    global.h
*                                                     
******************************************************************************/
#include "ui/rs_base.h"
#include "ui/rsfonts.h"
#include "ui/global.h"

#if defined GRAFIX_DEBUG_ALLOC_FOR_SYSTEM_MEMORY
#ifndef  NU_NO_ERROR_CHECKING
#include <string.h>

extern NU_MEMORY_POOL  System_Memory;

extern VOID    STR_mem_cpy(VOID *, VOID *, INT32 );
extern INT32   STR_str_cmp( const UNICHAR *, const UNICHAR *);

static UINT32 DbgAllocationCount = 0;
static UINT32 DbgAllocSequenceCounter = 0;
static UINT32 TotalMemoryAllocated = 0;
static UINT32 TotalMemoryAllocations = 0;
static UINT32 MaxTotalMemoryAllocated = 0;
static UINT32 MaxTotalMemoryAllocations = 0;

#define         NU_NULL                 0

static const UINT8 DebugAllocHead[] = {'H','e','a','d'};
   /* size of 4 is hardcoded below */
static const UINT8 DebugAllocFoot[] = {'F','o','o','t'};
   /* size of 4 is hardcoded below */

static struct DebugAllocStruct *RecentMalloc = NU_NULL;

STATUS Debug_Allocation(NU_MEMORY_POOL *pool_ptr,VOID **alloc,UNSIGNED_INT size,
						UINT32 suspend, UINT32 line, const CHAR* file);

STATUS Debug_Deallocation(VOID *ptr);

struct DebugAllocStruct 
{
   struct DebugAllocStruct *Prev;
   UNSIGNED_INT size; 
   UINT32 AllocSequenceCounter;
   UINT32 line;
   const CHAR * file; 
   UINT8 Head[4];
   UINT8 data[1];
};

/***************************************************************************
* FUNCTION
*
*    Debug_Allocation
*
* DESCRIPTION
*
*    Function Debug_Allocation allocates memory.
*
* INPUTS
*
*   NU_MEMORY_POOL *pool_ptr - Memory pool pointer
*   VOID **alloc             - Pointer to the destination memory pointer
*   UNSIGNED_INT size        - Number of bytes requested                  
*   UINT32 suspend           - Suspension option if full
*   UINT32 line              - Line number where the memory allocation was requested
*   const CHAR* file         - File Name
*    
* OUTPUTS
*
*    STATUS -  NU_SUCCESS if memory is allocated
*           - ~NU_SUCCESS if memory allocation is failed
*
***************************************************************************/
STATUS Debug_Allocation(NU_MEMORY_POOL *pool_ptr,VOID **alloc,UNSIGNED_INT size, UINT32 suspend, UINT32 line, const CHAR* file)
{
   		
   struct DebugAllocStruct **alloc_ptr;
   STATUS status = 0;	
   INT wrong_Memory_Pool = 0;

   status = DMCE_Allocate_Memory(pool_ptr, alloc,
	         sizeof((struct DebugAllocStruct **)alloc) - sizeof(((struct DebugAllocStruct *)alloc)->data) + size + 100, suspend);
   alloc_ptr = (struct DebugAllocStruct **)alloc;	
   if ( status != NU_SUCCESS)
   {
	   return 1;
   } 

   if(&System_Memory != pool_ptr)
   {
	   wrong_Memory_Pool = 1;
   }
   if(wrong_Memory_Pool == 1)
   {
	   return(status);
   }
   (*alloc_ptr)->line = line;
   (*alloc_ptr)->file = file;

   STR_mem_cpy((*alloc_ptr)->Head,(UNICHAR *)DebugAllocHead,4);
   STR_mem_cpy(&((*alloc_ptr)->data[size]),(UNICHAR *)DebugAllocFoot,4);
   (*alloc_ptr)->size = size;
   
  (*alloc_ptr)->Prev = RecentMalloc;
  RecentMalloc = *alloc_ptr;
  DbgAllocationCount++;
  (*alloc_ptr)->AllocSequenceCounter = DbgAllocSequenceCounter++;

  TotalMemoryAllocated += size;
  TotalMemoryAllocations++;
  if ( MaxTotalMemoryAllocated < TotalMemoryAllocated )
     MaxTotalMemoryAllocated = TotalMemoryAllocated;
  if ( MaxTotalMemoryAllocations < TotalMemoryAllocations )
     MaxTotalMemoryAllocations = TotalMemoryAllocations;
   
   (*alloc) = (*alloc_ptr)->data;
   return(0);
}

/***************************************************************************
* FUNCTION
*
*    FindDebugAlloc
*
* DESCRIPTION
*
*    Function FindDebugAlloc finds the data(memory pointer) to be removed 
*    from the DebugAllocStruct list.
*
* INPUTS
*
*   VOID *ptr - memory pointer
*    
* OUTPUTS
*
*    struct DebugAllocStruct ** -  Returns a pointer
*
***************************************************************************/
static struct DebugAllocStruct **FindDebugAlloc(VOID *ptr)
{
   struct DebugAllocStruct **alloc_ptr;

   INT ptrOffset;
   struct DebugAllocStruct *alloc;
   INT wrong_Memory_Pool = 0;

   ptrOffset = (INT)(((struct DebugAllocStruct *)0)->data);
   alloc = (struct DebugAllocStruct *)(((UINT8 *)ptr) - ptrOffset);
   
   if(ptr != NU_NULL && alloc != NU_NULL) 	
   {
	   for ( alloc_ptr = &RecentMalloc; *alloc_ptr != alloc; alloc_ptr = &((*alloc_ptr)->Prev) )
	   {
	      if ( NU_NULL == *alloc_ptr )
	         break;
	   }
   }
    /* endfor */
   if ( NU_NULL == *alloc_ptr ) 
   {
	   if(STR_str_cmp((UNICHAR *)(System_Memory.dm_name),(UNICHAR *)(((NU_MEMORY_POOL*)ptr)->dm_name))!=0)
	   {
		   wrong_Memory_Pool = 1;
	   }
	   if( wrong_Memory_Pool)
	   {
		   return(ptr);
	   }
#ifdef NU_SIMULATION
	   else
	   {
		   return(-1);
	   }
#endif
   }

   if ( 0 != memcmp(DebugAllocHead,alloc->Head,4) )
    {
#ifdef MNT
    return(-2);
#endif
    }
    return(alloc_ptr);
}


/***************************************************************************
* FUNCTION
*
*    Debug_Deallocation
*
* DESCRIPTION
*
*    Function Debug_Deallocation deallocates memory.
*
* INPUTS
*
*   VOID *ptr - Memory block pointer
*    
* OUTPUTS
*
*   STATUS status - The status of the deallocation.
*
***************************************************************************/
STATUS Debug_Deallocation(VOID *ptr)
{
   struct DebugAllocStruct **alloc_ptr;
   struct DebugAllocStruct *alloc;
   STATUS status = 0;


   if(ptr != NU_NULL)
   {
      alloc_ptr = FindDebugAlloc(ptr);
      alloc = *alloc_ptr;
      TotalMemoryAllocated -= alloc->size;
      TotalMemoryAllocations--;
   }
   

   *alloc_ptr = alloc->Prev;
   if(DbgAllocationCount > 0)
     DbgAllocationCount--;

   status = DMCE_Deallocate_Memory(alloc);
   if(status != NU_SUCCESS)
   {
#ifdef NU_SIMULATION
	   return(-3);
#endif
   }
   
   return status;
}
#endif /* NU_NO_ERROR_CHECKING */
#endif /* GRAFIX_DEBUG_ALLOC_FOR_SYSTEM_MEMORY */
