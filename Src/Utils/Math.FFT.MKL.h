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

#include    "mkl_dfti.h"                // Intel MKL library
#include    "mkl_service.h"
#include    "Math.Armadillo.h"
                                        // Isolate this into its own namespace
namespace mkl {

using   AReal       = crtl::AReal;
using   AComplex    = crtl::AComplex;

//----------------------------------------------------------------------------
                                        // Changing the max threads is only a wish, MKL might decide otherwise
                                        // Actually, it tries to optimize the actual number of cores, bypassing the hyper threading
inline int      GetMKLMaxThreads        ()                      { return  mkl_get_max_threads ();                   }
inline int      GetMKLMaxThreadsDomain  ( int domain )          { return  mkl_domain_get_max_threads ( domain );    }  // domains: MKL_DOMAIN_ALL MKL_DOMAIN_BLAS MKL_DOMAIN_FFT MKL_DOMAIN_VML MKL_DOMAIN_PARDISO MKL_DOMAIN_LAPACK
inline void     SetMKLMaxThreads        ( int nt )              {         mkl_set_num_threads ( nt );               }
inline void     SetMKLMaxThreadsLocal   ( int nt )              {         mkl_set_num_threads_local ( nt );         }   // for a given thread
inline void     SetMKLMaxThreadsDomain  ( int nt, int domain )  {         mkl_domain_set_num_threads ( nt, domain );}   // for a given domain


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Simulating sequential behavior from parallel MKL
                                        // Recommended parameters, but not working
//omp_set_dynamic     ( 0 );              // OpenMP will not limit the number of threads
//mkl_set_dynamic     ( 0 );              // MKL will not limit the number of threads
//omp_set_nested      ( 1 );              // Allow an OpenMP inner loop to actually use multiple threads when called from an outer OpenMP loop - deprecated, though
//mkl_set_num_threads ( 1 );              // MKL forced to work sequentially


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


//----------------------------------------------------------------------------
                                        // Each object has only a single purpose, though MKL allows descriptors to be modified to do other processing
enum    MklProcessing
        {
        UnknownProcessing,
        FromReal,
        FromComplex,
        BackToReal,
        BackToComplex,
        };

                                        // Controling either FFT or FFTI rescaling
enum    MklRescaling
        {
        NoRescaling,
        SqrtRescaling,
        FullRescaling,

        ForwardRescaling            = NoRescaling,
        BackwardRescaling           = FullRescaling,    // default rescaling used for forward AND backward (FFT then FFTI) to get back to original scaling
        ForwardRescalingParseval    = FullRescaling,    // FFT forward only Parseval rescaling, which preserves the energy of the signal - still be careful of real signal "folded" output
        };

                                        // Wrap descriptor handle & hide ugly settings
                                        // MKL documentation asserts all MKL routines (except one in LAPACK) are indeed thread-safe, so caller could totally use a single object instance in parrallel blocks
class   TMklFft
{
public:
                    TMklFft     ();
                    TMklFft     ( MklProcessing how, MklRescaling rescaling, int numdata );
                   ~TMklFft     ();

    void            Set         ( MklProcessing how, MklRescaling rescaling, int numdata );
    void            Reset       ();
                                        // independent if FFT or FFTI, what is the size of original vs frequency domains:
    int             GetDirectDomainSize     ()  const                       { return DataSize; }
    int             GetFrequencyDomainSize  ()  const                       { return How == FromReal || How == BackToReal ? DataSize / 2 + 1 : DataSize; }

                                        // Caller should pay extra attention to the arguments being used, as this will dispatch to different processing
    void            operator()  ( const AReal*    data, AReal*    freq );                               // unambiguous - returns the NORM of FFT of real data
    void            operator()  ( const AReal*    data, AComplex* freq )    { FFT  ( data, freq ); }    // unambiguous
    void            operator()  ( const AComplex* freq, AReal*    data )    { FFTI ( freq, data ); }    // unambiguous
    void            operator()  ( const AComplex* from, AComplex* to   )    { if ( How == FromComplex )  FFT ( from, to ); else /*BackToComplex*/ FFTI ( from, to ); } // ambiguous

protected:

    MklProcessing           How;
    int                     DataSize;
    DFTI_DESCRIPTOR_HANDLE  mklh;
    MKL_LONG                Status;

    void            FFT         ( const AReal*    data, AComplex* freq )    { Status = DftiComputeForward  ( mklh, (void*) data, freq ); }
    void            FFT         ( const AComplex* data, AComplex* freq )    { Status = DftiComputeForward  ( mklh, (void*) data, freq ); }
    void            FFTI        ( const AComplex* freq, AReal*    data )    { Status = DftiComputeBackward ( mklh, (void*) freq, data ); }
    void            FFTI        ( const AComplex* freq, AComplex* data )    { Status = DftiComputeBackward ( mklh, (void*) freq, data ); }

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}







