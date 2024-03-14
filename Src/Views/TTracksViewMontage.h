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

#include    "TArray1.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr int       MONTAGE_MAXCHARELECNAME             = 2 * ElectrodeNameSize + 2;

                                        // Class that defines a single track of montage
class   TTracksViewMontageSlot
{
public:

    inline          TTracksViewMontageSlot ()   { Reset (); }

    float*          ToData1;            // directly pointing to the correct line in data buffer
    float*          ToData2;            // could be null
    int             El1;
    int             El2;
    char            Name[ MONTAGE_MAXCHARELECNAME ];


    inline void     Reset           ()          { ToData1 = 0; ToData2 = 0; El1 = -1; El2 = -1; *Name = EOS; }

    inline bool     IsEmpty         ()  const   { return ! ToData1;              }
    inline bool     IsNotEmpty      ()  const   { return   ToData1;              }
    inline bool     OneTrack        ()  const   { return   ToData1 && ! ToData2; }
    inline bool     TwoTracks       ()  const   { return   ToData1 &&   ToData2; }

    inline          operator bool   ()  const   { return ToData1; } // cast: return true if montage is valid
};


//----------------------------------------------------------------------------

class   TTracksViewMontage
{
public:

    inline          TTracksViewMontage ()               {}
    inline         ~TTracksViewMontage ()               { Reset (); }


    inline void     Set                 ( int numel );
    inline void     Reset               ()              { Slots.DeallocateMemory (); }


    inline int      GetNumActiveSlots   ()  const;


    inline                                  operator    bool    ()          const   { return (bool) Slots; }
    inline const TTracksViewMontageSlot&    operator    []      ( int i )   const   { return Slots[ i ]; }  // !no boundary check!
    inline       TTracksViewMontageSlot&    operator    []      ( int i )           { return Slots[ i ]; }


protected:

    TArray1<TTracksViewMontageSlot> Slots;
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------

void    TTracksViewMontage::Set ( int numel )
{
Slots.Resize ( numel );

for ( int i = 0; i < (int) Slots; i++ )    // clean-up the array
    Slots[ i ].Reset ();
}


int     TTracksViewMontage::GetNumActiveSlots ()   const
{
if ( ! (bool) (*this) )
    return  0;

int                 numact          = 0;

for ( int i = 0; i < (int) Slots; i++ )
    if ( Slots[ i ].IsNotEmpty () )
        numact++;

return  numact;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
