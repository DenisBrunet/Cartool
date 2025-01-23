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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "Geometry.TDisplaySpaces.h"

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const char  DisplaySpaceName[ NumDisplaySpaces ][ 32 ] = {
        "3D Space",
        "2D Projected",
        "2D Projected + Height",
        };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TDisplaySpace::TDisplaySpace ()
{
Points.Reset ();
MedianDistance      = 0;
PointRadius         = 1;
Bounding.Reset ();
}


        TDisplaySpace::TDisplaySpace ( const TDisplaySpace &op )
{
Points              = op.Points;
MedianDistance      = op.MedianDistance;
PointRadius         = op.PointRadius;
Bounding            = op.Bounding;
}


TDisplaySpace&  TDisplaySpace::operator= ( const TDisplaySpace &op2 )
{
if ( &op2 == this )
    return  *this;


Points              = op2.Points;
MedianDistance      = op2.MedianDistance;
PointRadius         = op2.PointRadius;
Bounding            = op2.Bounding;


return  *this;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TDisplaySpaces::TDisplaySpaces ()
{
Reset ();
}


        TDisplaySpaces::TDisplaySpaces ( int numspaces )
{
Set ( numspaces );
}


        TDisplaySpaces::~TDisplaySpaces ()
{
Reset ();
}


//----------------------------------------------------------------------------
void    TDisplaySpaces::Reset ()
{
NumSpaces       = 0;
Spaces.clear ();
}


//----------------------------------------------------------------------------
void    TDisplaySpaces::Set ( int numspaces )
{
if ( numspaces <= 0 )
//  numspaces = 1;                      // allocate at least one space maybe?
    numspaces = 0;

if ( numspaces == NumSpaces )
    return;


Reset ();

NumSpaces       = numspaces;
Spaces.resize ( NumSpaces );
}


//----------------------------------------------------------------------------
        TDisplaySpaces::TDisplaySpaces ( const TDisplaySpaces &op )
{
NumSpaces   = op.NumSpaces;
Spaces      = op.Spaces;
}


TDisplaySpaces& TDisplaySpaces::operator= ( const TDisplaySpaces &op2 )
{
if ( &op2 == this )
    return  *this;


NumSpaces   = op2.NumSpaces;
Spaces      = op2.Spaces;

return  *this;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
