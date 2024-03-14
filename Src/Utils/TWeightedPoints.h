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

#include    "MemUtil.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // float case
//constexpr double  TWeightedPoints4SumWeights    = 1.0;
                                        // unsigned byte case for less memory print
constexpr int       TWeightedPoints4SumWeights  = UCHAR_MAX;

                                        // Templatize this class maybe?
class   TWeightedPoints4
{
public:
                    TWeightedPoints4 ()                     { Reset (); }

                                        
    USHORT          i1, i2, i3, i4;     // 4 indexes to 4 Solution Points -> max 65535 points
//  float           w1, w2, w3, w4;     // & the corresponding weights for each of them - float is more precise, but takes 4 times space storage
    UCHAR           w1, w2, w3, w4;     // & the corresponding weights for each of them - byte is approximate, but is for fast display mainly - and it looks ugly anway
                                        // Sum w's = TWeightedPoints4SumWeights
                                        // w1 = TWeightedPoints4SumWeights and w2=w3=w4=0 if exactly on an existing points


    inline bool     IsAllocated     ()  const               {   return  w1 != 0; }
    inline bool     IsNotAllocated  ()  const               {   return  w1 == 0; }

    void            Reset           ()                      {   ClearVirtualMemory ( this, sizeof ( *this ) ); }


    TWeightedPoints4&   operator    =   ( UCHAR op2 )       { w1 = w2 = w3 = w4 = op2; return *this; }

};


class   TWeightedPoints8
{
public:
                    TWeightedPoints8 ()                     { Reset (); }


    USHORT          i1, i2, i3, i4, i5, i6, i7, i8;     // 8 indexes to 8 Solution Points
    float           w1, w2, w3, w4, w5, w6, w7, w8;     // & the corresponding weights for each of them - sum of weights = 1 - possible special case: w1 = 1, the others w's = 0 for spot one value


    bool            IsAllocated     ()  const               {   return  w1 != 0; }
    bool            IsNotAllocated  ()  const               {   return  w1 == 0; }

    void            Reset           ()                      {   ClearVirtualMemory ( this, sizeof ( *this ) ); }


    TWeightedPoints8&    operator    =  ( float op2 )       { w1 = w2 = w3 = w4 = w5 = w6 = w7 = w8 = op2; return *this; }

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
