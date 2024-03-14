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

#include    "Math.TMatrix44.h"
#include    "Geometry.TPoint.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum    TOrientationType {
        OrientationUnknown,

        LeftRight,          // box side to axis meaning, without known direction
        UpDown,
        FrontBack,

        ToLeft,             // box side to axis meaning, with proper direction
        ToRight,
        ToUp,       
        ToDown,
        ToFront,    
        ToBack,

        ToXMin,             // box side to default axis, like "X box side is X axis"
        ToXMax,
        ToYMin,     
        ToYMax,
        ToZMin,     
        ToZMax,
        ToXAxis,
        ToYAxis,
        ToZAxis,

        NumOrientationTypes
        };

extern const char   OrientationLabels[ NumOrientationTypes ][ 32 ];

inline  TOrientationType    LetterToOrientation ( char o )          {   return  o == 'A' ? ToFront 
                                                                              : o == 'P' ? ToBack 
                                                                              : o == 'L' ? ToLeft 
                                                                              : o == 'R' ? ToRight 
                                                                              : o == 'I' ? ToDown 
                                                                              : o == 'S' ? ToUp 
                                                                              :            OrientationUnknown; }
bool                        IsOrientationOK     ( const char orientation[ 3 ] );
bool                        IsOrientationOK     ( TOrientationType xaxis, TOrientationType yaxis, TOrientationType zaxis );

                                        // Enum for each box sides
enum    TBoxSide {
        XMinSide,
        XMaxSide,
        YMinSide,   
        YMaxSide,
        ZMinSide,   
        ZMaxSide,

        XAxis,
        YAxis,
        ZAxis,

        NumBoxSides
        };

                                        // Common axis re-orientations
enum    StandardOrientationEnum {
        LocalToPIR,         // current object to PIR
        LocalToRAS,         // current object to RAS
        PIRToLocal,         // PIR to current object
        RASToLocal,         // RAS to current object

        NumStandardOrientationEnum
        };


//----------------------------------------------------------------------------
// Class used to give some meaning to the X/Y/Z axis of a 3D object:
//
// - Correspondance between objects' X/Y/Z axis and some meaning, like "left" or "Superior"
// - Stores 3 3D vectors to point to the back, the down and the right directions
// - Stores a 4x4 matrix to reorient to default RAS orientation

class   TOrientation
{
public:
                        TOrientation ();
                        TOrientation ( const char orientation[ 3 ] )        { ResetOrientation (); SetOrientation ( orientation ); }


    void                ResetOrientation    ();
    bool                SetOrientation      ( TOrientationType xaxis,  TOrientationType yaxis,  TOrientationType zaxis );
    bool                SetOrientation      ( const char orientation[ 3 ] );
    virtual void        FindOrientation     ()                              { ResetOrientation (); }

    void                OrientationToString ( char orientation[ 3 ] )   const;
    void                CompleteOrientation ( TOrientationType &xaxis, TOrientationType &yaxis, TOrientationType &zaxis );


    TMatrix44           GetStandardOrientation ( StandardOrientationEnum orient )   const;

    const TOrientationType* GetBoxSides     ()                      const   { return BoxSides; }
    TBoxSide            GetAxis             ( TOrientationType o )  const;
    int                 GetAxisIndex        ( TOrientationType o )  const;


protected:

    TVector3Double      BackVector;             // Orientation -> X/Y/Z vector values
    TVector3Double      DownVector;
    TVector3Double      RightVector;

    TOrientationType    BoxSides[ NumBoxSides ];// give each side a label, X/Y/Z -> orientation

    TMatrix44           StandardOrientMatrix;   // Local -> PIR orientation (for historical reasons)

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
