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

#include    "Electrodes.TransformElectrodes.h"

#include    "Strings.Utils.h"
#include    "Geometry.TPoints.h"
#include    "Math.TMatrix44.h"
#include    "Files.TVerboseFile.h"
#include    "TCoregistrationTransform.h"
#include    "TCoregistrationDialog.h"       // CoregitrationTypeString

#include    "TElectrodesDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

OptimizeOff

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool    TransformElectrodes (   const char*             xyzsourcefile,
                                const char*             mritargetfile,      // not actually needed for computation

                                const TCoregistrationTransform& transform,
                                CoregistrationType      processing,

                                const Volume&           mask,                   const Volume&           gradient,
                                const TPointDouble&     origin, 
                                const TMatrix44&        mriabstoguillotine,
                                double                  inflating,

                                const char*             basefilename,
                                char*                   xyztransfile,
                                char*                   altxyztransfile,        const char*             altelectrodes,
                                TMatrix44*              xyzcoregtonorm
                            )
{
ClearString ( xyztransfile    );
ClearString ( altxyztransfile );

if ( xyzcoregtonorm != 0 )
    xyzcoregtonorm->Reset ();

if ( mask    .IsNotAllocated ()
  || gradient.IsNotAllocated ()
  || StringIsEmpty ( xyzsourcefile )
  || StringIsEmpty ( basefilename  ) )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           fileverbose;

StringCopy          ( fileverbose,      basefilename,       "." FILEEXT_VRB       );
StringCopy          ( xyztransfile,     basefilename,       "." FILEEXT_XYZ       );

CheckNoOverwrite ( fileverbose  );
CheckNoOverwrite ( xyztransfile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TStrings            pointsnames;
TPoints             points ( xyzsourcefile, &pointsnames );
                                        // 4x4 transform + optional non-linear gluing operation
transform.Apply (   points, 
                    mask,       gradient, 
                    origin, 
                    mriabstoguillotine,
                    inflating
                );

points.WriteFile ( xyztransfile, &pointsnames );


if ( xyzcoregtonorm != 0 )
    *xyzcoregtonorm = transform.ComposeTransform ();    // should be the transform needed, given XYZ and MRI are already in normalized space...


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Any optional alternate electrodes set?
TOpenDoc<TElectrodesDoc>    xyzdoc ( xyztransfile, OpenDocHidden ); // extraction method is currently in TElectrodesDoc

TSelection          elsel ( points.GetNumPoints (), OrderArbitrary );
char                buff[ 4 * KiloByte ];

elsel.Set ( altelectrodes, &pointsnames, true );

bool                savingaltelectrodes =  xyzdoc.IsOpen ()
                                        && elsel.NumSet () > 0 
                                        && elsel.NumSet () < xyzdoc->GetNumElectrodes ()
                                        && altxyztransfile != 0;
                                      //&& StringIsNot ( savingaltelectrodes, "*" );


if ( savingaltelectrodes ) {

    StringCopy          ( altxyztransfile,       xyztransfile );
    PostfixFilename     ( altxyztransfile, StringCopy ( buff,  ".", IntegerToString ( elsel.NumSet () ) ) );

    CheckNoOverwrite    ( altxyztransfile );

    xyzdoc->ExtractToFile ( altxyztransfile, elsel, false );
    }


xyzdoc.Close ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TVerboseFile        Verbose;

Verbose.Open        ( fileverbose, VerboseFileDefaultWidth );

Verbose.PutTitle    ( "Coregistering" );


Verbose.NextTopic   ( "Input Files:" );
{
Verbose.Put         ( "File to be coregistered:", xyzsourcefile );
Verbose.Put         ( "File to coregister to:",   mritargetfile );
}


Verbose.NextTopic   ( "Output Files:" );
{
Verbose.Put         ( "Verbose file (this):", fileverbose );
Verbose.Put         ( "Coregistered file:", xyztransfile );

if ( savingaltelectrodes ) {

    Verbose.NextLine ();
    Verbose.Put     ( "Alternate electrodes to save:", elsel.ToText ( buff, &pointsnames ) );
    Verbose.Put     ( "Alternate coregistered file:", altxyztransfile );
    }
}


Verbose.NextTopic   ( "Method:" );
{
Verbose.Put         ( "Piloted:", "Manually" );
Verbose.Put         ( "Type of coregistration:", CoregitrationTypeString[ processing ] );

Verbose.NextLine ();
Verbose.Put         ( "Final gluing:", transform.Gluing );
}

// also outputting the transform parameters? by sub-types of transforms?

return  true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}

OptimizeOn
