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

#include    "Geometry.TPoint.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum        InitType
            {
            InitFast,
            InitExact,
            };

                                        // Simply handles the limits: storage and simple management
class   TLimits
{
public:

    inline              TLimits ()                  { ResetLimits (); }


    inline void         ResetLimits     ();
    inline virtual void InitLimits      ( InitType how )    = 0;   // overriding methods are responsible to set all fields


    inline double       GetMinValue     ()  const   { return MinValue;      }
    inline double       GetMaxValue     ()  const   { return MaxValue;      }
    inline double       GetAbsMaxValue  ()  const   { return AbsMaxValue;   }

    inline long         GetAbsMaxTF     ()  const   { return AbsMaxTF;      }
    inline double       GetMaxGfp       ()  const   { return MaxGfp;        }
    inline long         GetMaxGfpTF     ()  const   { return MaxGfpTF;      }
    inline TPointFloat  GetMinPosition  ()  const   { return MinPosition;   }
    inline TPointFloat  GetMaxPosition  ()  const   { return MaxPosition;   }


    inline              TLimits         ( const TLimits& op  );
    inline TLimits&     operator    =   ( const TLimits& op2 );


protected:
                                        // Values
    double          MinValue;
    double          MaxValue;
    double          AbsMaxValue;
                                        // Positions - use might vary according to class
    long            AbsMaxTF;           // Time positions
    double          MaxGfp;
    long            MaxGfpTF;

    TPointFloat     MinPosition;        // 3D positions
    TPointFloat     MaxPosition;
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------

void    TLimits::ResetLimits ()
{
MinValue            = 0;
MaxValue            = 0;
AbsMaxValue         = 0;
AbsMaxTF            = 0;
MaxGfp              = 0;
MaxGfpTF            = 0;
MinPosition.Reset ();
MaxPosition.Reset ();
}



            TLimits::TLimits    ( const TLimits& op  )
{
MinValue            = op.MinValue;
MaxValue            = op.MaxValue;  
AbsMaxValue         = op.AbsMaxValue;
AbsMaxTF            = op.AbsMaxTF;
MaxGfp              = op.MaxGfp;  
MaxGfpTF            = op.MaxGfpTF;
MinPosition         = op.MinPosition;
MaxPosition         = op.MaxPosition;
}


TLimits&    TLimits::operator=  ( const TLimits& op2 )
{
                                        // in case of self assignation
if ( &op2 == this )
    return  *this;

MinValue            = op2.MinValue;
MaxValue            = op2.MaxValue;  
AbsMaxValue         = op2.AbsMaxValue;
AbsMaxTF            = op2.AbsMaxTF;
MaxGfp              = op2.MaxGfp;  
MaxGfpTF            = op2.MaxGfpTF;
MinPosition         = op2.MinPosition;
MaxPosition         = op2.MaxPosition;

return  *this;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
