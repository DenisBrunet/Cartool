/************************************************************************\
© 2024-2025 Denis Brunet, University of Geneva, Switzerland.

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

#include    <omp.h>

namespace crtl {
                                        // OpenMP utilities

                                        // general parallel block
#define OmpParallelBegin                __pragma( omp parallel ) {
#define OmpParallelEnd                  }

                                        // Explicit list of sections
#define OmpParallelSectionsBegin        __pragma( omp parallel sections ) {
#define OmpParallelSectionsEnd          }
#define OmpSectionBegin                 __pragma( omp section ) {
#define OmpSectionEnd                   }

                                        // general purpose lock block, with forced naming in case of parallel nested block callss
//#define OmpCriticalBegin              __pragma( omp critical ) {
#define OmpCriticalBegin(SECTIONNAME)   __pragma( omp critical(SECTIONNAME) ) {
#define OmpCriticalEnd                  }
                                        // wraps a small critical section, like for 1 (or a few) instruction(s) - kind of OmpAtomic when not recognized
#define OmpCritical(INSTR)              __pragma( omp critical ) { INSTR; }
                                        // specialized lock to a single, specifically recognized operation (like ++)
#define OmpAtomic                       __pragma( omp atomic )

#define OmpFlush(...)                   __pragma( omp flush (__VA_ARGS__) )

                                        // single parallel for, WITHIN an existing parallel block
#define OmpFor                          __pragma( omp for )
#define OmpForSum(...)                    __pragma( omp for reduction (+:__VA_ARGS__) )
                                        // STAND-ALONE, single parallel for(s) - NOT WITHIN an existing parallel block
#define OmpParallelFor                  __pragma( omp parallel for )
#define OmpParallelForSum(...)          __pragma( omp parallel for reduction (+:__VA_ARGS__) )

                                        // current thread Id
inline int  GetThreadId     ()      { return  omp_get_thread_num (); }
                                        // also working for serialized code
inline bool IsMainThread    ()      { return  GetThreadId () == 0; }
                                        // current number of threads - will nicely return 1 for serialized code
inline int  GetNumThreads   ()      { return  omp_get_num_threads (); }
                                        // taken from environment variable OMP_NUM_THREADS
inline int  GetNumMaxThreads()      { return  omp_get_max_threads (); }
                                        // stepping used for progress bar that uses a parallel block
inline int  StepThread      ()      { return  GetNumThreads (); }
                                        // if code is inside any sort of parallel code
inline bool IsInParallelCode()      { return  omp_in_parallel (); }


//SYSTEM_INFO sysinfo; GetSystemInfo ( &sysinfo ); int numCPU = sysinfo.dwNumberOfProcessors;

}
