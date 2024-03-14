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
#include    "Geometry.TPoint.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class   TDipole
{
public:
                    TDipole         ()                          {   Reset ();   }

    void            Reset           ()                          {   Position .Reset ();
                                                                    Direction.Reset ();
                                                                    SolPointIndex   = -1;
                                                                    Power           = 0;
                                                                    Frequency       = 0;
                                                                    Phase           = 0;
                                                                }

    TPointFloat     Position;
    TVector3Float   Direction;
    int             SolPointIndex;
    double          Power;
    double          Frequency;
    double          Phase;

                                        // Setting up direction toward an electrode
    void            SetDirection    ( const TPointFloat& electrodepos )     {   Direction   = electrodepos - Position;  Direction.Normalize (); }

    TDipole                         ( const TDipole& op  )      {   CopyVirtualMemory ( this, &op,  sizeof ( TDipole ) ); }
    TDipole&        operator    =   ( const TDipole& op2 )      {   if ( &op2 != this )     CopyVirtualMemory ( this, &op2, sizeof ( TDipole ) );   return *this; }
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
