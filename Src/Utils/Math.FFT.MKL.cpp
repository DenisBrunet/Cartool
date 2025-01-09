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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "Strings.Utils.h"
#include    "TArray1.h"
#include    "TVector.h"
#include    "Math.FFT.MKL.h"

using namespace std;
using namespace crtl;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace mkl {

//----------------------------------------------------------------------------
                                        // Input data
//status = DftiSetValue           (  mklh, DFTI_COMPLEX_STORAGE, DFTI_REAL_REAL or DFTI_COMPLEX_COMPLEX );
//status = DftiSetValue           (  mklh, DFTI_REAL_STORAGE, DFTI_REAL_REAL );
//status = DftiSetValue           (  mklh, DFTI_CONJUGATE_EVEN_STORAGE, DFTI_COMPLEX_COMPLEX );

//status = DftiSetValue           (  mklh, DFTI_PLACEMENT, DFTI_NOT_INPLACE );    // results in another data storage

//status = DftiSetValue           (  mklh, DFTI_FORWARD_SCALE, 1 / fftscaling );  // forward scaling
//status = DftiSetValue           (  mklh, DFTI_BACKWARD_SCALE, ... );            // backward scaling
// 
//status = DftiSetValue           (  mklh, DFTI_INPUT_STRIDES, <real_strides> );    // for multidimensional data
//status = DftiSetValue           (  mklh, DFTI_OUTPUT_STRIDES, <complex_strides>); // for multidimensional data

//status = DftiSetValue           (  mklh, DFTI_NUMBER_OF_TRANSFORMS, <morethan1> );    // doing multiple FFTs
//status = DftiSetValue           (  mklh, DFTI_INPUT_DISTANCE, .. );    // stepping in data
//status = DftiSetValue           (  mklh, DFTI_OUTPUT_DISTANCE , .. );    // 
                                        // Output data
//status = DftiSetValue           (  mklh, DFTI_PACKED_FORMAT, DFTI_CCS_FORMAT );     // Complex conjugate-symmetric
//status = DftiSetValue           (  mklh, DFTI_PACKED_FORMAT, DFTI_PACK_FORMAT );    // Pack format for real DFT
//status = DftiSetValue           (  mklh, DFTI_PACKED_FORMAT, DFTI_PERM_FORMAT );    // Perm format for real DFT
//status = DftiSetValue           (  mklh, DFTI_PACKED_FORMAT, DFTI_CCE_FORMAT );     // Complex conjugate-even - Only option for Real input


//----------------------------------------------------------------------------

        TMklFft::TMklFft ()
{
mklh        = NULL;
Reset ();
}


        TMklFft::TMklFft ( MklProcessing how, FFTRescalingType rescaling, int numdata )
{
mklh        = NULL;
Set ( how, rescaling, numdata );
}


        TMklFft::~TMklFft ()
{
Reset ();
}


void    TMklFft::Reset ()
{
if ( mklh )
    DftiFreeDescriptor ( &mklh );

How         = UnknownProcessing;
DataSize    = 0;
mklh        = NULL;
Status      = 0;
}

                                        // A nice property of descriptor is that it can be reused multiple times, even while changing its parameters
void    TMklFft::Set ( MklProcessing how, FFTRescalingType rescaling, int numdata )
{
Reset ();

How         = how;
DataSize    = AtLeast ( 0, numdata );

if ( DataSize <= 0 )
    How = UnknownProcessing;

if ( How == UnknownProcessing )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

switch ( How ) {

    case    FromReal:
    case    BackToReal:     Status = DftiCreateDescriptor   ( &mklh, DFTI_SINGLE, DFTI_REAL,    1, DataSize );
                            break;

    case    FromComplex:
    case    BackToComplex:  Status = DftiCreateDescriptor   ( &mklh, DFTI_SINGLE, DFTI_COMPLEX, 1, DataSize );
                            break;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // MKL is kind enough to offer any kind of rescaling to the caller
if ( rescaling != FFTRescalingNone ) {

    if      ( How == FromReal  
           || How == FromComplex )

        DftiSetValue ( mklh, DFTI_FORWARD_SCALE,  rescaling == FFTRescalingSqrtWindowSize   ? 1 / sqrt ( (double) DataSize ) 
                                                : rescaling == FFTRescalingWindowSize       ? 1 /        (double) DataSize
                                                :            /*FFTRescalingNone*/             1                              );

    else if ( How == BackToReal 
           || How == BackToComplex )

        DftiSetValue ( mklh, DFTI_BACKWARD_SCALE, rescaling == FFTRescalingSqrtWindowSize   ? 1 / sqrt ( (double) DataSize ) 
                                                : rescaling == FFTRescalingWindowSize       ? 1 /        (double) DataSize
                                                :            /*FFTRescalingNone*/             1                              );
    } // some rescaling


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Results are in another storage
Status  = DftiSetValue          (  mklh, DFTI_PLACEMENT, DFTI_NOT_INPLACE );
                                        // Does not seem to be necessary(?)
//Status= DftiSetValue          (  mklh, DFTI_CONJUGATE_EVEN_STORAGE, DFTI_COMPLEX_COMPLEX );
                                        // Does not seem to be necessary(?)
//Status= DftiSetValue          (  mklh, DFTI_PACKED_FORMAT, DFTI_CCE_FORMAT );
                                        // We are all set here
Status  = DftiCommitDescriptor  (  mklh );
}


//----------------------------------------------------------------------------
                                        // returns the norm of complex
void    TMklFft::operator()  ( const AReal* data, AReal* freq )
{
                                        // we could save on allocating on each call...
TVector<AComplex>   R        ( DataSize );

FFT ( data, R );
                                        // n/2+1 results
for ( int fi0 = 0; fi0 <= DataSize / 2; fi0++ )
    freq[ fi0 ] = abs ( R[ fi0 ] );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}







