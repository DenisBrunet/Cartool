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

#include    <vector>

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr int       WindowSlotNameLength        = std::max ( RoiNameLength, 64 );

                                        // Everything we need to specify a sub-window with any subset of tracks:
                                        // size, orientation (both actually in 3D), origin of plot, scaling, color
class   TTracksViewWindowSlot
{
public:
    inline                  TTracksViewWindowSlot ();

                                        // specify a plane in 3D that will "contain" the drawing
    TGLCoordinates<GLfloat> LowLeft;
    TGLCoordinates<GLfloat> ToRight;
    TGLCoordinates<GLfloat> ToUp;

                                        // handy storage:
    TGLCoordinates<GLfloat> TrOrg;      // origin of first track
    TGLCoordinates<GLfloat> TrDelta;    // translation to next track
    double                  ScaleH;     // scaling horizontal


    TGLColor<GLfloat>       Color;      // of the tracks
    char                    Name[ WindowSlotNameLength ];


    TSelection              SelTracks;  // tracks to be put in
    TSelection              SelFreqs;   // frequencies to be put in


    int                     UserData;   // for whatever (index, time, pointer...)
    bool                    RegularTracks;

                                        // set the frame from "low-left" origin, "to right" and "to up" corners
    inline void             SetFrame (  double llx,  double lly,  double llz,
                                        double torx, double tory, double torz,
                                        double toux, double touy, double touz );


    inline int              NumRoisPerColumn ( int NUMROIS, int NUMSPLIT )                                          const   { return  ( NUMROIS + NUMSPLIT - 1 ) / NUMSPLIT; }


    inline double           RegLowLeftX     ( double X, double W, int CURRSLOT, int NUMSLOTS                    )   const   { return  X + W * (double) CURRSLOT / NUMSLOTS; }
    inline double           RegLowLeftY     ( double Y, double H, double hreg                                   )   const   { return  Y + H * ( 1 - hreg ); }

    inline double           RoiLowLeftX     ( double X, double W, int CURRSLOT, int NUMSLOTS, int NUMSPLIT      )   const   { return  X + W * (double) ( CURRSLOT / NUMSLOTS ) / NUMSPLIT; }
    inline double           RoiLowLeftY     ( double Y, double H, int CURRSLOT, int NUMSLOTS, double hreg       )   const   { return  Y + H * ( ( 1 - (double) ( CURRSLOT % NUMSLOTS + 1 ) / NUMSLOTS ) * hreg + ( 1 - hreg ) ); }

    inline double           PseudosLowLeftX ( double X, double W, int CURRSLOT, int NUMSLOTS                    )   const   { return  X + W * (double) CURRSLOT / NUMSLOTS; }
    inline double           PseudosLowLeftY ( double Y                                                          )   const   { return  Y; }

    inline double           RegToRightX     (           double W,               int NUMSLOTS                    )   const   { return  W        / (double) NUMSLOTS; }
    inline double           RegToUpY        (           double H,               int NUMSLOTS, double hreg       )   const   { return  H * hreg / (double) NUMSLOTS; }

    inline double           PseudosToRightX (           double W,               int NUMSLOTS                    )   const   { return  RegToRightX ( W, NUMSLOTS ); }
    inline double           PseudosToUpY    (           double H,                             double hpseudos   )   const   { return  H * hpseudos; }

};


//----------------------------------------------------------------------------
                                        // Everything we need to tile tracks according to user's choice:
                                        // 1 big window with all tracks, 2 sub-windows with half tracks in each,
                                        // and up to a single tiny window for each track
class   TTracksViewWindowSlots
{
public:

    inline                          TTracksViewWindowSlots ();
    inline                         ~TTracksViewWindowSlots ()           { DeallocateWindowSlots (); }


    inline bool                     HasWindowSlots          ()  const   { return ! WindowSlots.empty ();    }
    inline int                      GetNumWindowSlots       ()  const   { return WindowSlots.size ();       }

    inline void                     AllocateWindowSlots     ( int n );
    inline void                     DeallocateWindowSlots   ()          { WindowSlots.resize ( 0 );         }
    inline TTracksViewWindowSlot*   ClosestWindowSlot       ( double x, double y );


protected:

    std::vector<TTracksViewWindowSlot>  WindowSlots;
    int                             MaxRegularTracksPerSlot;
    int                             MaxPseudoTracksPerSlot;
    int                             SelSize;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TTracksViewWindowSlot::TTracksViewWindowSlot ()
{
ScaleH              = 1;
ClearString ( Name );
UserData            = 0;
RegularTracks       = true;
Color.Set ( /*TrackColor*/ (GLfloat) 0.00,   (GLfloat) 0.00,   (GLfloat) 0.00,   (GLfloat) 1.00 );
}


//----------------------------------------------------------------------------
                                        // Assuming frame is parallel to axis
void    TTracksViewWindowSlot::SetFrame (   double llx,  double lly,  double llz,
                                            double torx, double tory, double torz,
                                            double toux, double touy, double touz   )
{
LowLeft.Set ( (GLfloat) llx,  (GLfloat) lly,  (GLfloat) llz  );
ToRight.Set ( (GLfloat) torx, (GLfloat) tory, (GLfloat) torz );
ToUp.   Set ( (GLfloat) toux, (GLfloat) touy, (GLfloat) touz );

TrOrg   = LowLeft;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TTracksViewWindowSlots::TTracksViewWindowSlots ()
{
MaxRegularTracksPerSlot = 1;
MaxPseudoTracksPerSlot  = 1;
SelSize                 = 0;
}


//----------------------------------------------------------------------------
void    TTracksViewWindowSlots::AllocateWindowSlots ( int n )
{
if ( n <= 0 ) {                         // shouldn't happen, but reset everything anyway

    DeallocateWindowSlots ();
    return;
    }


WindowSlots.resize ( n );

for ( int s = 0; s < GetNumWindowSlots (); s++ ) {

    WindowSlots[ s ].SelTracks  = TSelection ( SelSize, OrderArbitrary );

    ClearString ( WindowSlots[ s ].Name );
    }
}


//----------------------------------------------------------------------------
TTracksViewWindowSlot*  TTracksViewWindowSlots::ClosestWindowSlot ( double x, double y )
{
TTracksViewWindowSlot*  slot;
double                  xmin;
double                  xmax;
double                  ymin;
double                  ymax;

/*
for ( int s = 0; s < GetNumWindowSlots (); s++ ) {
    slot = WindowSlots + s;

//  xmin = slot->LowLeft.X;
//  xmax = xmin            + slot->ToRight.Norm();
    xmin = slot->TrOrg.X;
    xmax = slot->LowLeft.X + slot->ToRight.Norm();
    ymin = slot->LowLeft.Y;
    ymax = ymin            + slot->ToUp.Y;

    if ( x >= xmin && x <= xmax && y >= ymin && y <= ymax )
        return slot;
    }
*/
                                        // not inside a slot? rescan to closest
double              dx;
double              dy;
double              d;
double              dmin            = DBL_MAX;
int                 closest         = -1;


for ( int s = 0; s < GetNumWindowSlots (); s++ ) {

    slot    = &WindowSlots[ s ];

    xmin    = slot->TrOrg.X;
    xmax    = slot->LowLeft.X + slot->ToRight.Norm();
    ymin    = slot->LowLeft.Y;
    ymax    = ymin            + slot->ToUp.Y;

    if      ( x < xmin )    dx  = x - xmin;
    else if ( x > xmax )    dx  = x - xmax;
    else                    dx  = 0;

    if      ( y < ymin )    dy  = y - ymin;
    else if ( y > ymax )    dy  = y - ymax;
    else                    dy  = 0;

    d   = Square ( dx ) + Square ( dy );

    if ( d == 0 )                       // directly inside? don't search any further

        return slot;

    else if ( d < dmin ) {              // else is it the closest we've seen?
        dmin    = d;
        closest = s;
        }
    }


return  closest >= 0 ? &WindowSlots[ closest ] : 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
