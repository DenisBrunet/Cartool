/************************************************************************\
Copyright 2024 CIBM (Center for Biomedical Imaging), Lausanne, Switzerland

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
\************************************************************************/

#pragma once

#if defined(CHECKASSERT)
#include    <assert.h>
#endif

#include    <windows.h>                 // will include <windef.h>, <winbase.h>, <winuser.h>, <wingdi.h> etc..
#include    <algorithm>                 // min, max

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr char*     MemoryAllocationErrorTitle  = "Memory Allocation Error";
constexpr char*     NotEnoughMemoryErrorMessage = "Some operations requested too much memory,\ntry breaking-up your processing into smaller steps!\n\nProgram is going to crash, sorry about that..";


//----------------------------------------------------------------------------
                                        // Stand-alone functions to handle memory blocks

enum        MemoryAllocationType
            {
            MemoryUndefined     = 0x00,

            MemoryAuto          = 0x01,     // Automatically picking the method that best fits the requested size
            MemoryCHeap         = 0x02,     // Classical C-style
            MemoryCppHeap       = 0x03,     // Classical C++-style
            MemoryWindowsHeap   = 0x04,     // Windows Heap
            MemoryVirtual       = 0x05,     // Windows Virtual Memory
            MemoryTypeMask      = MemoryAuto | MemoryCHeap | MemoryCppHeap | MemoryWindowsHeap | MemoryVirtual,

            ResizeNoReset       = 0x10,     // No explicit resetting requested, mainly used to discriminate against the other flags - resetting is undefined, it could or could not occur
            ResizeClearMemory   = 0x20,     // Force reset the allocated memory
            ResizeKeepMemory    = 0x30,     // Will keep the current memory content after the resize - !Use with care, as this implies some sort of local copy!
            ResizeTypeMask      = ResizeNoReset | ResizeClearMemory | ResizeKeepMemory,

            MemoryDefault       = MemoryAuto | ResizeClearMemory,
            };


inline  MemoryAllocationType    ClearMemoryType     ( MemoryAllocationType  m )                             {   return  (MemoryAllocationType) ( m & ~MemoryTypeMask ); }
inline  MemoryAllocationType    ClearResizeType     ( MemoryAllocationType  m )                             {   return  (MemoryAllocationType) ( m & ~ResizeTypeMask ); }

inline  MemoryAllocationType    GetMemoryType       ( MemoryAllocationType  m )                             {   return  (MemoryAllocationType) ( m &  MemoryTypeMask ); }
inline  MemoryAllocationType    GetResizeType       ( MemoryAllocationType  m )                             {   return  (MemoryAllocationType) ( m &  ResizeTypeMask ); }

inline  MemoryAllocationType    SetMemoryType       ( MemoryAllocationType  m, MemoryAllocationType how )   {   return  (MemoryAllocationType) ( ClearMemoryType ( m ) | GetMemoryType ( how ) ); }
inline  MemoryAllocationType    SetResizeType       ( MemoryAllocationType  m, MemoryAllocationType how )   {   return  (MemoryAllocationType) ( ClearResizeType ( m ) | GetResizeType ( how ) ); }

inline  bool                    IsMemoryAuto        ( MemoryAllocationType  m )                             {   return  GetMemoryType ( m ) == MemoryAuto;          }
inline  bool                    IsMemoryCHeap       ( MemoryAllocationType  m )                             {   return  GetMemoryType ( m ) == MemoryCHeap;         }
inline  bool                    IsMemoryCppHeap     ( MemoryAllocationType  m )                             {   return  GetMemoryType ( m ) == MemoryCppHeap;       }
inline  bool                    IsMemoryWindowsHeap ( MemoryAllocationType  m )                             {   return  GetMemoryType ( m ) == MemoryWindowsHeap;   }
inline  bool                    IsMemoryVirtual     ( MemoryAllocationType  m )                             {   return  GetMemoryType ( m ) == MemoryVirtual;       }
inline  bool                    IsResizeNoReset     ( MemoryAllocationType  m )                             {   return  GetResizeType ( m ) == ResizeNoReset;       }
inline  bool                    IsResizeClearMemory ( MemoryAllocationType  m )                             {   return  GetResizeType ( m ) == ResizeClearMemory;   }
inline  bool                    IsResizeKeepMemory  ( MemoryAllocationType  m )                             {   return  GetResizeType ( m ) == ResizeKeepMemory;    }


//----------------------------------------------------------------------------
                                        // Not specific to Virtual Memory:
//----------------------------------------------------------------------------
inline  void    ClearVirtualMemory  (   void*   block,                          size_t  numbytes                            );
inline  void    SetVirtualMemory    (   void*   block,                          size_t  numbytes,   unsigned char   fill    );
inline  void*   CopyVirtualMemory   (   void*   dest,   const void* from,       size_t  numbytes                            );  // NON-overlapping blocks
inline  void    MoveVirtualMemory   (   void*   dest,   const void* from,       size_t  numbytes                            );  // Overlapping blocksinline  void    ClearVirtualMemory  (   void*   block,      size_t  numbytes    );


//----------------------------------------------------------------------------
                                        // Specific to Virtual Memory - relies on the low levels of Windows
                                        //  pros: page aligned (= easy to swap, 64K blocks); pages are consecutives (reading); pages can be protected  
                                        //  cons: min size of 1 page (4K blocks)
//----------------------------------------------------------------------------
inline  void*   GetVirtualMemory    ( size_t    numbytes );
inline  bool    FreeVirtualMemory   ( void*     block );
                                    // Picking from the process' heap
inline  void*   GetHeapMemory       ( size_t    numbytes );
inline  bool    FreeHeapMemory      ( void*     block );

inline  DWORD   GetPageSize         ();
inline  DWORD   GetMemoryGranularity();


//----------------------------------------------------------------------------
                                        // Selects either C Heap or Virtual Memory allocation, depending on options and requested size
                                        // Will update  memoryblock  and  memtype
inline  void*   GetSmartMemory      ( void*&    memoryblock,    size_t  numbytes,   MemoryAllocationType    how,    MemoryAllocationType&   memtype );  // updates memoryblock and memtype
inline  void    FreeSmartMemory     ( void*&    memoryblock,                                                        MemoryAllocationType    memtype );
inline  void    ClearSmartMemory    ( void*&    memoryblock,    size_t  numbytes,                                   MemoryAllocationType    memtype );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // A class to wrap all memory allocations.
class   TMemory
{
public:
    inline                          TMemory ();
    inline                          TMemory ( size_t numbytes, MemoryAllocationType how = MemoryAuto );
    virtual inline                 ~TMemory ();


    inline  bool                    IsMemoryAllocated   ()  const           { return  MemoryBlock != 0; }

    inline  void*                   GetMemoryAddress    ()  const           { return  MemoryBlock; }
    inline  size_t                  GetMemorySize       ()  const           { return  MemorySize;  }
    inline  MemoryAllocationType    GetMemoryType       ()  const           { return  MemoryType;  }


    virtual inline  void            ResetMemory         ();                 // shouldn't be virtual, but compiler seems to bug if not(?)
    virtual inline  void            SetMemory           ( unsigned char byte );

    virtual inline  void*           AllocateMemory      ( size_t numbytes, MemoryAllocationType how = MemoryAuto    );
    virtual inline  void*           ResizeMemory        ( size_t numbytes, MemoryAllocationType how = MemoryDefault );
    virtual inline  void            DeallocateMemory    ();                 // must be virtual

    inline  void                    CopyMemoryFrom      ( void* frommem, size_t size = 0 );


private:
                                        // These are über-confidential variables, even derived classes shouldn't access them
    void*                   MemoryBlock;
    size_t                  MemorySize;
    MemoryAllocationType    MemoryType;


    inline  void            ResetVariables ();
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------

inline  void    ClearVirtualMemory  (   void*   block,      size_t  numbytes    )
{
if ( ! ( block == 0 || numbytes == 0 ) )

//  ZeroMemory ( block, numbytes );             // macro that ends up to be memset ( ..,0 )
    RtlSecureZeroMemory ( block, numbytes );    // new secure function
}


//----------------------------------------------------------------------------
                                        // actually not specific to Virtual Memory
inline  void    SetVirtualMemory    (   void*   block,      size_t  numbytes,   unsigned char   fill    )
{
if ( ! ( block == 0 || numbytes == 0 ) )

//  FillMemory ( block, numbytes, fill );
    RtlFillMemory ( block, numbytes, fill );
}


//----------------------------------------------------------------------------
                                        // !Be careful to have NON-overlapping memory blocks - Otherwise use MoveVirtualMemory!
inline  void*   CopyVirtualMemory   (   void*   dest,       const void*     from,   size_t  numbytes    )
{
if ( ! ( dest == 0 || from == 0 || dest == from || numbytes == 0 ) )

//  CopyMemory ( dest, from, numbytes );
    RtlCopyMemory ( dest, from, numbytes );

return  dest;
}


//----------------------------------------------------------------------------
                                        // !Use this for overlapping blocks!
inline  void    MoveVirtualMemory   (   void*   dest,       const void*     from,   size_t  numbytes    )
{
if ( ! ( dest == 0 || from == 0 || dest == from || numbytes == 0 ) )

//  MoveMemory ( dest, from, numbytes );
    RtlMoveMemory ( dest, from, numbytes );
}


//----------------------------------------------------------------------------
inline  void*   GetVirtualMemory    (   size_t  numbytes    )
{
                                        // memory is (supposedly) reset to 0 after allocation
void*               mem             = numbytes > 0 ? VirtualAlloc ( NULL, numbytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE ) : 0;

//#if defined(CHECKASSERT)
//assert ( numbytes == 0 || mem != 0 );
//#endif

if ( numbytes > 0 && mem == 0 ) {

    MessageBoxA (   0,
                    NotEnoughMemoryErrorMessage,
                    MemoryAllocationErrorTitle,
                    MB_SETFOREGROUND | MB_OK | MB_ICONEXCLAMATION 
                );

    abort ();
    }

return  mem;
}


//----------------------------------------------------------------------------
inline  bool    FreeVirtualMemory   (   void*   block   )
{
return  block ? VirtualFree ( block, 0, MEM_RELEASE ) != FALSE : true;
}


//----------------------------------------------------------------------------
                                        // Specific to Heap, which calls VirtualAlloc but allows for smaller blocks
inline  void*   GetHeapMemory   (   size_t  numbytes    )
{
#if defined (_DEBUG)
void*               mem             = numbytes > 0 ? HeapAlloc ( GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS /*| HEAP_ZERO_MEMORY*/, numbytes ) : 0;
#else
void*               mem             = numbytes > 0 ? HeapAlloc ( GetProcessHeap(), 0 /*HEAP_ZERO_MEMORY*/, numbytes ) : 0;
#endif


//#if defined(CHECKASSERT)
//assert ( numbytes == 0 || mem != 0 );
//#endif

if ( numbytes > 0 && mem == 0 ) {

    MessageBoxA (   0,
                    NotEnoughMemoryErrorMessage,
                    MemoryAllocationErrorTitle,
                    MB_SETFOREGROUND | MB_OK | MB_ICONEXCLAMATION 
                );

    abort ();
    }

return  mem;
}


//----------------------------------------------------------------------------
inline  bool    FreeHeapMemory  (   void*   block   )
{
return  block ? HeapFree ( GetProcessHeap(), 0, block ) != FALSE : true;
}


//----------------------------------------------------------------------------
                                        // Size of a virtual memory page swap
inline  DWORD   GetPageSize ()
{
                                        // we can cache it!
static  DWORD       PageSize        = 0;

if ( PageSize == 0 ) {

    SYSTEM_INFO     si;

    GetSystemInfo ( &si );

    PageSize    = si.dwPageSize;
    }

return  PageSize;
}


//----------------------------------------------------------------------------
inline  DWORD   GetMemoryGranularity ()
{
                                        // we can cache it!
static  DWORD       MemoryGranularity   = 0;

if ( MemoryGranularity == 0 ) {

    SYSTEM_INFO     si;

    GetSystemInfo ( &si );

    MemoryGranularity   = si.dwAllocationGranularity;
    }

return  MemoryGranularity;
}


//----------------------------------------------------------------------------
                                        // Selects either C Heap or Virtual Memory allocation, depending on options and requested size
                                        // Will update  memoryblock  and  memtype
inline  void*   GetSmartMemory  (   void*&  memoryblock,    size_t  numbytes,   MemoryAllocationType    how,    MemoryAllocationType&   memtype )
{
                                        // Resolve type of memory allocation - Make it simple: use either malloc or virtual memory
                                        // Windows Heap could be used as intermediate granularity, but is it worth the added complexity? It also seems to fragment itself for small chunks of memory, then crash the app...
if ( IsMemoryAuto ( how ) )     memtype = numbytes >= GetMemoryGranularity ()   ? MemoryVirtual         // more than 65K, we can ignore the Windows granularity
//                                      : numbytes >= GetPageSize ()            ? MemoryWindowsHeap     // more than 4K, go to the current process Heap
//                                      :                                         MemoryCppHeap;        // C++ style
                                        :                                         MemoryCHeap;          // C style
else                            memtype = GetMemoryType ( how );


                                        // Allocate the amount of requested type of memory
if      ( IsMemoryVirtual     ( memtype ) ) memoryblock     = GetVirtualMemory ( numbytes );                    // virtual memory
else if ( IsMemoryWindowsHeap ( memtype ) ) memoryblock     = GetHeapMemory    ( numbytes );                    // Windows heap
else if ( IsMemoryCppHeap     ( memtype ) ) memoryblock     = numbytes > 0 ? (void*) new char [ numbytes ] : 0; // C++ Heap
else if ( IsMemoryCHeap       ( memtype ) ) memoryblock     = numbytes > 0 ? malloc           ( numbytes ) : 0; // C Heap (is cheap)
else                                        memoryblock     = 0;


//#if defined(CHECKASSERT)
//assert ( numbytes == 0 || memoryblock != 0 );
//#endif
                                        // Always reset new allocated memory (GetVirtualMemory / VirtualAlloc is doing that for us already)
                                        // !Except for a bug when successive VirtualAlloc / VirtualFree are called in a row!
                                        // https://www.reddit.com/r/programming/comments/8cn9ef/7zip_exposes_a_bug_in_windowss_large_memory_pages/
//if ( ! IsMemoryVirtual ( memtype ) )
    ClearSmartMemory    ( memoryblock, numbytes, memtype );


return  memoryblock;
}


//----------------------------------------------------------------------------
inline  void    FreeSmartMemory     (   void*&  memoryblock,    MemoryAllocationType    memtype )
{
if ( memoryblock == 0 )
    return;

if      ( IsMemoryVirtual     ( memtype ) ) FreeVirtualMemory ( memoryblock );
else if ( IsMemoryWindowsHeap ( memtype ) ) FreeHeapMemory    ( memoryblock );
else if ( IsMemoryCppHeap     ( memtype ) ) delete[]            memoryblock;
else if ( IsMemoryCHeap       ( memtype ) ) free              ( memoryblock );

memoryblock     = 0;
}


//----------------------------------------------------------------------------
inline  void    ClearSmartMemory    (   void*&  memoryblock,    size_t  numbytes,   MemoryAllocationType    memtype )
{
if ( memoryblock == 0 || numbytes == 0 )
    return;

if      ( IsMemoryVirtual     ( memtype ) ) ClearVirtualMemory ( memoryblock,    numbytes );
else if ( IsMemoryWindowsHeap ( memtype ) ) ClearVirtualMemory ( memoryblock,    numbytes );
else if ( IsMemoryCppHeap     ( memtype ) ) memset             ( memoryblock, 0, numbytes );
else if ( IsMemoryCHeap       ( memtype ) ) memset             ( memoryblock, 0, numbytes );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TMemory::TMemory ()
{
ResetVariables ();
}


        TMemory::TMemory ( size_t numbytes, MemoryAllocationType how )
{
ResetVariables ();

AllocateMemory ( numbytes, how );
}


        TMemory::~TMemory ()
{
DeallocateMemory ();
}


void    TMemory::ResetVariables ()
{
MemoryBlock     = 0;
MemorySize      = 0;
MemoryType      = MemoryUndefined;
}


//----------------------------------------------------------------------------
void    TMemory::DeallocateMemory ()
{
if ( ! IsMemoryAllocated () )
    return;

FreeSmartMemory ( MemoryBlock, MemoryType );

ResetVariables ();
}


//----------------------------------------------------------------------------
void*   TMemory::AllocateMemory ( size_t numbytes, MemoryAllocationType how )
{
return  ResizeMemory ( numbytes, SetResizeType ( how, ResizeClearMemory ) );
}


//----------------------------------------------------------------------------
                                        // This is the work-horse function, which allows multiple legal combinations of options
void*   TMemory::ResizeMemory ( size_t numbytes, MemoryAllocationType how )
{
                                        // nothing to do (also OK if numbytes == 0)?
if ( numbytes == MemorySize ) {
                                        // reset memory if requested
    if      ( IsResizeClearMemory ( how ) )

        ResetMemory ();

//  else if ( IsResizeKeepMemory ( how ) 
//         || IsResizeNoReset    ( how ) ) // nothing to do!

    return  MemoryBlock;
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here amount of memory requested is different from current amount
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // can dispose of current content?
if ( ! IsResizeKeepMemory ( how ) || numbytes == 0 ) {

//  DeallocateMemory ();                // !overridden function will clear-up their local variables (like dimensions), which we DON'T want!

    TMemory::DeallocateMemory ();       // only deletes locally, to avoid resetting any child variable
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // save old state
void*                   oldmemoryblock  = MemoryBlock;
size_t                  oldmemorysize   = MemorySize;
MemoryAllocationType    oldmemorytype   = MemoryType;

                                        // Start clean from scratch
ResetVariables ();

                                        // Set new size
MemorySize      = numbytes;

                                        // no need to allocate?
if ( MemorySize == 0 )

    return  MemoryBlock;                // which is 0


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Allocate the amount of memory + choose the actual type of allocation
GetSmartMemory ( MemoryBlock, MemorySize, how, MemoryType );

                                        // Test for error
//#if defined(CHECKASSERT)
//assert ( MemoryBlock != 0 );
//#endif

if ( MemoryBlock == 0 ) {

    MessageBoxA (   0,
                    NotEnoughMemoryErrorMessage,
                    MemoryAllocationErrorTitle,
                    MB_SETFOREGROUND | MB_OK | MB_ICONEXCLAMATION 
                );

    ResetVariables ();

    abort ();
    }
                                        // Here we're good

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Finally copy the old data, if requested
if ( IsResizeKeepMemory ( how ) ) {
                                        // don't exceed any buffer
    CopyVirtualMemory ( MemoryBlock, oldmemoryblock, std::min ( oldmemorysize, MemorySize ) );
                                        // we can dispose of the old data now
    if ( oldmemorysize != 0 )

        FreeSmartMemory ( oldmemoryblock, oldmemorytype );
    }


return  MemoryBlock;
}


//----------------------------------------------------------------------------
void    TMemory::ResetMemory ()
{
ClearSmartMemory ( MemoryBlock, MemorySize, MemoryType );
}


void    TMemory::SetMemory ( unsigned char byte )
{
SetVirtualMemory   ( MemoryBlock, MemorySize, byte );
}


//----------------------------------------------------------------------------
void    TMemory::CopyMemoryFrom (   void*   frommem,    size_t      size    )
{
if ( IsMemoryAllocated () && frommem )

    CopyVirtualMemory ( MemoryBlock, frommem, size == 0 ? MemorySize : std::min ( size, MemorySize ) );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------



}
