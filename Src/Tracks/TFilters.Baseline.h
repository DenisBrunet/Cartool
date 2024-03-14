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

namespace crtl {

//----------------------------------------------------------------------------
                                        // Temporal filter
//----------------------------------------------------------------------------

template <class TypeD>
class   TFilterBaseline : public TFilter<TypeD>
{
public:

    void            Apply                   ( TypeD* data, int numpts );

};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------

template <class TypeD>
void    TFilterBaseline<TypeD>::Apply ( TypeD* data, int numpts )
{
double              sum             = 0;

                                        // straightforward solution to remove DC
for ( int ti = 0; ti < numpts; ti++ )
    sum        += data[ ti ];

sum    /= numpts;

for ( int ti = 0; ti < numpts; ti++ )
    data[ ti ] -= sum;
}


/*                                      // Sliding window DC
template <class TypeD>
void    TFilterBaseline<TypeD>::Apply ( TypeD* data, int numpts )
{
double              sum;

//int                 wsize           = /*fsamp* / 250 / ButterworthHighRequested;
int                 wsize           = 100;
wsize  |= 1;
int                 wsize2          = wsize / 2;


NotchBuff.Resize ( max ( numpts, NotchBuff.GetDim1 () ), (MemoryAllocationType) ( MemoryAuto | ResizeNoReset ) );  // resize without shrinking


CopyVirtualMemory ( &NotchBuff, data, numpts * sizeof ( TypeD ) );


sum     = wsize2 * NotchBuff[ 0 ];

for ( int i = 0; i <= wsize2; i++ )
    sum        += NotchBuff[ i ];


for ( int i = 0; i < numpts; i++ ) {

//    sum     = 0;
//    for ( int j = 0; j < wsize; j++ )
//        sum    += NotchBuff[ Clip ( i - wsize2, 0, numpts - 1 ) ];
//    sum    /= numpts;
//    data[ i ]  -= sum;


    data[ i ]  -= sum / wsize;

    sum         = sum - NotchBuff[ Clip ( i - wsize2, 0, numpts - 1 ) ] + NotchBuff[ Clip ( i + wsize2, 0, numpts - 1 ) ];
    }

}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
