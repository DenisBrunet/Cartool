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

#include    "Strings.Utils.h"

#include    "Geometry.TOrientation.h"

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const char  OrientationLabels[ NumOrientationTypes ][ 32 ] = {
        "Unknown",

        "Left/Right",
        "Superior/Inferior",        
        "Anterior/Posterior",

        "Left", 
        "Right",    
        "Superior",  
        "Inferior",    
        "Anterior", 
        "Posterior",

        "XMin", 
        "XMax",     
        "YMin", 
        "YMax",             
        "ZMin",     
        "ZMax",
        "XAxis",            
        "YAxis",                    
        "ZAxis" 
        };


//----------------------------------------------------------------------------
        TOrientation::TOrientation ()
{
ResetOrientation ();
}


void    TOrientation::ResetOrientation ()
{
                                        // set a default orientation
BackVector. Reset ();
DownVector. Reset ();
RightVector.Reset ();

                                        // PIR default, from which know how to convert to PIR
BackVector.X    = DownVector.Y  = RightVector.Z = 1;
                                        // RAS default
//RightVector.X   =  1;
//BackVector. Y   = -1;
//DownVector. Z   = -1;

                                        // set a default labeling of the sides
BoxSides[ XMinSide ]    = ToXMin;
BoxSides[ XMaxSide ]    = ToXMax;
BoxSides[ YMinSide ]    = ToYMin;
BoxSides[ YMaxSide ]    = ToYMax;
BoxSides[ ZMinSide ]    = ToZMin;
BoxSides[ ZMaxSide ]    = ToZMax;
BoxSides[    XAxis ]    = ToXAxis;
BoxSides[    YAxis ]    = ToYAxis;
BoxSides[    ZAxis ]    = ToZAxis;
                                        // reset matrix
StandardOrientMatrix.SetIdentity();
}


//----------------------------------------------------------------------------
bool    IsOrientationOK ( const char orientation[ 3 ] )
{
                                        // process the 3 chars
TOrientationType  xaxis           = LetterToOrientation ( orientation[ 0 ] );
TOrientationType  yaxis           = LetterToOrientation ( orientation[ 1 ] );
TOrientationType  zaxis           = LetterToOrientation ( orientation[ 2 ] );

return  IsOrientationOK ( xaxis, yaxis, zaxis );
}


//----------------------------------------------------------------------------
bool    IsOrientationOK ( TOrientationType xaxis, TOrientationType yaxis, TOrientationType zaxis )
{
                                        // some axis undefined?
if ( xaxis == OrientationUnknown || yaxis == OrientationUnknown || zaxis == OrientationUnknown )
    return  false;

                                        // some repeated axis?
if ( xaxis == yaxis || xaxis == zaxis || yaxis == zaxis )
    return  false;


                                        // get the vectors
TVector3Double      backvector;
TVector3Double      downvector;
TVector3Double      rightvector;

backvector. Reset ();
downvector. Reset ();
rightvector.Reset ();


if      ( xaxis == ToBack  )    backvector.X    =  1;
else if ( xaxis == ToFront )    backvector.X    = -1;
else if ( yaxis == ToBack  )    backvector.Y    =  1;
else if ( yaxis == ToFront )    backvector.Y    = -1;
else if ( zaxis == ToBack  )    backvector.Z    =  1;
else if ( zaxis == ToFront )    backvector.Z    = -1;

if      ( xaxis == ToDown )     downvector.X    =  1;
else if ( xaxis == ToUp   )     downvector.X    = -1;
else if ( yaxis == ToDown )     downvector.Y    =  1;
else if ( yaxis == ToUp   )     downvector.Y    = -1;
else if ( zaxis == ToDown )     downvector.Z    =  1;
else if ( zaxis == ToUp   )     downvector.Z    = -1;

if      ( xaxis == ToRight )    rightvector.X   =  1;
else if ( xaxis == ToLeft  )    rightvector.X   = -1;
else if ( yaxis == ToRight )    rightvector.Y   =  1;
else if ( yaxis == ToLeft  )    rightvector.Y   = -1;
else if ( zaxis == ToRight )    rightvector.Z   =  1;
else if ( zaxis == ToLeft  )    rightvector.Z   = -1;

                                        // problem here, some vector is undefined
if ( backvector == 0 || downvector == 0 || rightvector == 0 )
    return  false;

                                        // left-handed orientation not allowed
if ( ( backvector.VectorialProduct ( downvector ) ).ScalarProduct ( rightvector ) < 0 )
    return  false;


return  true;
}


//----------------------------------------------------------------------------
bool    TOrientation::SetOrientation ( TOrientationType xaxis, TOrientationType yaxis, TOrientationType zaxis )
{
ResetOrientation ();

                                        // parameters OK?
if ( ! IsOrientationOK ( xaxis, yaxis, zaxis ) ) {
//  ResetOrientation ();
    return  false;
    }

                                        // set the orientation vectors & labeling
BackVector. Reset ();
DownVector. Reset ();
RightVector.Reset ();
                                        // set orientation matrix
                                        // such as it turns to the Geneva "standard" orientation PIR
StandardOrientMatrix.SetIdentity();


if      ( xaxis == ToBack  )    { BackVector.X    =  1;   BoxSides[ XMinSide ] = ToFront;   BoxSides[ XMaxSide ] = ToBack;  BoxSides[ XAxis ] = FrontBack; }
else if ( xaxis == ToFront )    { BackVector.X    = -1;   BoxSides[ XMinSide ] = ToBack;    BoxSides[ XMaxSide ] = ToFront; BoxSides[ XAxis ] = FrontBack; }
else if ( yaxis == ToBack  )    { BackVector.Y    =  1;   BoxSides[ YMinSide ] = ToFront;   BoxSides[ YMaxSide ] = ToBack;  BoxSides[ YAxis ] = FrontBack; }
else if ( yaxis == ToFront )    { BackVector.Y    = -1;   BoxSides[ YMinSide ] = ToBack;    BoxSides[ YMaxSide ] = ToFront; BoxSides[ YAxis ] = FrontBack; }
else if ( zaxis == ToBack  )    { BackVector.Z    =  1;   BoxSides[ ZMinSide ] = ToFront;   BoxSides[ ZMaxSide ] = ToBack;  BoxSides[ ZAxis ] = FrontBack; }
else if ( zaxis == ToFront )    { BackVector.Z    = -1;   BoxSides[ ZMinSide ] = ToBack;    BoxSides[ ZMaxSide ] = ToFront; BoxSides[ ZAxis ] = FrontBack; }
                                                                                                                                          
if      ( xaxis == ToDown  )    { DownVector.X    =  1;   BoxSides[ XMinSide ] = ToUp;      BoxSides[ XMaxSide ] = ToDown;  BoxSides[ XAxis ] = UpDown;    }
else if ( xaxis == ToUp    )    { DownVector.X    = -1;   BoxSides[ XMinSide ] = ToDown;    BoxSides[ XMaxSide ] = ToUp;    BoxSides[ XAxis ] = UpDown;    }
else if ( yaxis == ToDown  )    { DownVector.Y    =  1;   BoxSides[ YMinSide ] = ToUp;      BoxSides[ YMaxSide ] = ToDown;  BoxSides[ YAxis ] = UpDown;    }
else if ( yaxis == ToUp    )    { DownVector.Y    = -1;   BoxSides[ YMinSide ] = ToDown;    BoxSides[ YMaxSide ] = ToUp;    BoxSides[ YAxis ] = UpDown;    }
else if ( zaxis == ToDown  )    { DownVector.Z    =  1;   BoxSides[ ZMinSide ] = ToUp;      BoxSides[ ZMaxSide ] = ToDown;  BoxSides[ ZAxis ] = UpDown;    }
else if ( zaxis == ToUp    )    { DownVector.Z    = -1;   BoxSides[ ZMinSide ] = ToDown;    BoxSides[ ZMaxSide ] = ToUp;    BoxSides[ ZAxis ] = UpDown;    }
                                                                                                                                          
if      ( xaxis == ToRight )    { RightVector.X   =  1;   BoxSides[ XMinSide ] = ToLeft;    BoxSides[ XMaxSide ] = ToRight; BoxSides[ XAxis ] = LeftRight; }
else if ( xaxis == ToLeft  )    { RightVector.X   = -1;   BoxSides[ XMinSide ] = ToRight;   BoxSides[ XMaxSide ] = ToLeft;  BoxSides[ XAxis ] = LeftRight; }
else if ( yaxis == ToRight )    { RightVector.Y   =  1;   BoxSides[ YMinSide ] = ToLeft;    BoxSides[ YMaxSide ] = ToRight; BoxSides[ YAxis ] = LeftRight; }
else if ( yaxis == ToLeft  )    { RightVector.Y   = -1;   BoxSides[ YMinSide ] = ToRight;   BoxSides[ YMaxSide ] = ToLeft;  BoxSides[ YAxis ] = LeftRight; }
else if ( zaxis == ToRight )    { RightVector.Z   =  1;   BoxSides[ ZMinSide ] = ToLeft;    BoxSides[ ZMaxSide ] = ToRight; BoxSides[ ZAxis ] = LeftRight; }
else if ( zaxis == ToLeft  )    { RightVector.Z   = -1;   BoxSides[ ZMinSide ] = ToRight;   BoxSides[ ZMaxSide ] = ToLeft;  BoxSides[ ZAxis ] = LeftRight; }



StandardOrientMatrix[ 0 ]   = xaxis == ToBack  ?  1 : xaxis == ToFront ? -1 : 0;
StandardOrientMatrix[ 1 ]   = xaxis == ToDown  ?  1 : xaxis == ToUp    ? -1 : 0;
StandardOrientMatrix[ 2 ]   = xaxis == ToRight ?  1 : xaxis == ToLeft  ? -1 : 0;

StandardOrientMatrix[ 4 ]   = yaxis == ToBack  ?  1 : yaxis == ToFront ? -1 : 0;
StandardOrientMatrix[ 5 ]   = yaxis == ToDown  ?  1 : yaxis == ToUp    ? -1 : 0;
StandardOrientMatrix[ 6 ]   = yaxis == ToRight ?  1 : yaxis == ToLeft  ? -1 : 0;

StandardOrientMatrix[ 8 ]   = zaxis == ToBack  ?  1 : zaxis == ToFront ? -1 : 0;
StandardOrientMatrix[ 9 ]   = zaxis == ToDown  ?  1 : zaxis == ToUp    ? -1 : 0;
StandardOrientMatrix[10 ]   = zaxis == ToRight ?  1 : zaxis == ToLeft  ? -1 : 0;



return  true;
}


//----------------------------------------------------------------------------
bool    TOrientation::SetOrientation      ( const char orientation[ 3 ] )
{
                                        // process the 3 chars
TOrientationType  xaxis           = LetterToOrientation ( orientation[ 0 ] );
TOrientationType  yaxis           = LetterToOrientation ( orientation[ 1 ] );
TOrientationType  zaxis           = LetterToOrientation ( orientation[ 2 ] );

return  SetOrientation ( xaxis, yaxis, zaxis );
}


void    TOrientation::OrientationToString ( char orientation[ 3 ] )     const
{
orientation[ 0 ]    = 0;
orientation[ 1 ]    = 0;
orientation[ 2 ]    = 0;

if ( BackVector == 0 )
    return;

                                        // use the first letter of the X, Y, Z axis direction
orientation[ 0 ]    = OrientationLabels[ BoxSides[ XMaxSide ] ][ 0 ];
orientation[ 1 ]    = OrientationLabels[ BoxSides[ YMaxSide ] ][ 0 ];
orientation[ 2 ]    = OrientationLabels[ BoxSides[ ZMaxSide ] ][ 0 ];
}


//----------------------------------------------------------------------------
                                        // finish the job with a vectorial product
                                        // as the system ToBack/ToDown/ToRight is direct
void    TOrientation::CompleteOrientation ( TOrientationType &xaxis, TOrientationType &yaxis, TOrientationType &zaxis )
{
                                        // we can complete only 1 axis
//if ( ( xaxis == OrientationUnknown ) + ( yaxis == OrientationUnknown ) + ( zaxis == OrientationUnknown ) != 1 )
//    return;


TVector3Double      back;
TVector3Double      down;
TVector3Double      right;

back. Reset ();
down. Reset ();
right.Reset ();

                                        // currently, detects missing Left Right or Up Down
if      ( xaxis == ToBack  )    back.X   =  1;
else if ( xaxis == ToFront )    back.X   = -1;
else if ( yaxis == ToBack  )    back.Y   =  1;
else if ( yaxis == ToFront )    back.Y   = -1;
else if ( zaxis == ToBack  )    back.Z   =  1;
else if ( zaxis == ToFront )    back.Z   = -1;

if      ( xaxis == ToDown )     down.X   =  1;
else if ( xaxis == ToUp   )     down.X   = -1;
else if ( yaxis == ToDown )     down.Y   =  1;
else if ( yaxis == ToUp   )     down.Y   = -1;
else if ( zaxis == ToDown )     down.Z   =  1;
else if ( zaxis == ToUp   )     down.Z   = -1;

if      ( xaxis == ToLeft  )    right.X  = -1;
else if ( xaxis == ToRight )    right.X  =  1;
else if ( yaxis == ToLeft  )    right.Y  = -1;
else if ( yaxis == ToRight )    right.Y  =  1;
else if ( zaxis == ToLeft  )    right.Z  = -1;
else if ( zaxis == ToRight )    right.Z  =  1;

                                        // check if only one direction is missing, else return
if ( ( right == 0 ) + ( down == 0 ) + ( back == 0 ) != 1 )
    return;

                                        // missing right vector?
if ( right == 0 ) {
    right = back.VectorialProduct ( down );

    if      ( right.X ==  1 )   xaxis = ToRight;
    else if ( right.X == -1 )   xaxis = ToLeft;
    else if ( right.Y ==  1 )   yaxis = ToRight;
    else if ( right.Y == -1 )   yaxis = ToLeft;
    else if ( right.Z ==  1 )   zaxis = ToRight;
    else if ( right.Z == -1 )   zaxis = ToLeft;
    }
                                        // missing down vector?
else if ( down == 0 ) {
    down = right.VectorialProduct ( back );

    if      ( down.X ==  1 )    xaxis = ToDown;
    else if ( down.X == -1 )    xaxis = ToUp;
    else if ( down.Y ==  1 )    yaxis = ToDown;
    else if ( down.Y == -1 )    yaxis = ToUp;
    else if ( down.Z ==  1 )    zaxis = ToDown;
    else if ( down.Z == -1 )    zaxis = ToUp;
    }
                                        // missing back vector?
else /*if ( back == 0 )*/ {
    back = down.VectorialProduct ( right );

    if      ( back.X ==  1 )    xaxis = ToBack;
    else if ( back.X == -1 )    xaxis = ToFront;
    else if ( back.Y ==  1 )    yaxis = ToBack;
    else if ( back.Y == -1 )    yaxis = ToFront;
    else if ( back.Z ==  1 )    zaxis = ToBack;
    else if ( back.Z == -1 )    zaxis = ToFront;
    }

}


//----------------------------------------------------------------------------
                                        // returns a copy of a reorientation matrix
TMatrix44   TOrientation::GetStandardOrientation ( StandardOrientationEnum orient ) const
{
TMatrix44           m ( StandardOrientMatrix );


switch ( orient ) {

    case    LocalToPIR:
                                        // Local -> PIR
        break;

    case    LocalToRAS:
                                        // Local -> PIR -> RAS
        m.OrientationPirToRas ( MultiplyLeft );

        break;

    case    PIRToLocal:
                                        // PIR -> Local
        m.Invert ();

        break;

    case    RASToLocal:
                                        //        PIR -> Local
        m.Invert ();
                                        // RAS -> PIR -> Local
        m.OrientationRasToPir ( MultiplyRight );

        break;

    }


return  m; 
}

//----------------------------------------------------------------------------
                                        // returns the box side corresponding to the requested orientation
TBoxSide  TOrientation::GetAxis ( TOrientationType o )  const
{
for ( int i = XMinSide; i < NumBoxSides; i++ )
    if ( BoxSides[ i ] == o )
        return (TBoxSide) i;

                                        // if missed above, return a default orientation
if      ( o == FrontBack )  return XAxis;
else if ( o == UpDown    )  return YAxis;
else if ( o == LeftRight )  return ZAxis;
else                        return XAxis;

//return (TBoxSide) -1; // don't!
}

                                                       // only for FrontBack, UpDown, LeftRight
int     TOrientation::GetAxisIndex ( TOrientationType o )   const
{
return  GetAxis ( o ) - XAxis;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
