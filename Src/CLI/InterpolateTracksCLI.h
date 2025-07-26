/************************************************************************\
© 2025 Denis Brunet, University of Geneva, Switzerland.

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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "System.CLI11.h"
#include    "CLIDefines.h"

#include    "TInterpolateTracks.h"
#include    "Files.TVerboseFile.h"

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Defining the interface
inline void     InterpolateTracksCLIDefine ( CLI::App* interpol )
{
if ( interpol == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionFile     ( interpol, "",     __fromxyz,              "From Electrodes Coordinates file" RequiredString );

DefineCLIOptionString   ( interpol, "",     __fromfront,            "From front landmark" )
->TypeOfOption          ( "TRACKS" );
DefineCLIOptionString   ( interpol, "",     __fromleft,             "From left landmark" )
->TypeOfOption          ( "TRACKS" );
DefineCLIOptionString   ( interpol, "",     __fromtop,              "From top landmark" )
->TypeOfOption          ( "TRACKS" );
DefineCLIOptionString   ( interpol, "",     __fromright,            "From right landmark" )
->TypeOfOption          ( "TRACKS" );
DefineCLIOptionString   ( interpol, "",     __fromrear,             "From rear landmark" )
->TypeOfOption          ( "TRACKS" );

DefineCLIOptionString   ( interpol, "",     __badelec,              "Bad electrodes" )
->TypeOfOption          ( "TRACKS" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionFile     ( interpol, "",     __toxyz,                "To Electrodes Coordinates file" );

DefineCLIOptionString   ( interpol, "",     __tofront,              "To front landmark" )
->TypeOfOption          ( "TRACKS" );
DefineCLIOptionString   ( interpol, "",     __toleft,               "To left landmark" )
->TypeOfOption          ( "TRACKS" );
DefineCLIOptionString   ( interpol, "",     __totop,                "To top landmark" )
->TypeOfOption          ( "TRACKS" );
DefineCLIOptionString   ( interpol, "",     __toright,              "To right landmark" )
->TypeOfOption          ( "TRACKS" );
DefineCLIOptionString   ( interpol, "",     __torear,               "To rear landmark" )
->TypeOfOption          ( "TRACKS" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionEnum     ( interpol, "",     __splinemethod,         "Interpolation method" RequiredString )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __splinesurface, __splinespherical, __spline3d, __splinecurrentdensity } ) ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string                  splinedegreetext    = "Spline degree, in ["
                                            + (string) IntegerToString ( MinInterpolationDegree ) + ".." + (string) IntegerToString ( MaxInterpolationDegree ) + "] range"
                                            + " (Default:" + (string) IntegerToString ( DefaultInterpolationDegree ) + ")";
DefineCLIOptionInt      ( interpol, "",     __splinedegree,         splinedegreetext );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionFile     ( interpol, "",     __inputdir,             __inputdir_descr )
->TypeOfOption          ( __inputdir_type )
->CheckOption           ( CLI::ExistingDirectory ); // could be incomplete, but it helps a bit, though

DefineCLIOptionString   ( interpol, "",     __infix,                __infix_descr );

DefineCLIOptionEnum     ( interpol, __ext,  __extension,            __ext_descr )
->DefaultString         ( SavingEegFileExtPreset[ PresetFileTypeDefaultEEG ] )
->CheckOption           ( CLI::IsMember ( vector<string> ( SavingEegFileExtPreset, SavingEegFileExtPreset + NumSavingEegFileTypes ) ) );

DefineCLIFlag           ( interpol, "",     __nocleanup,            "Saving intermediate files" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIFlag           ( interpol, __h,    __help,                 __help_descr );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Repeating positional files option seems OK
DefineCLIOptionFiles    ( interpol, -1,      "",     __files,       __files_descr );
}


//----------------------------------------------------------------------------
                                        // Running the command
inline void     InterpolateTracksCLI ( CLI::App* interpol )
{
if ( ! IsSubCommandUsed ( interpol )  )
    return;


TFileName           inputdir        = GetCLIOptionDir   ( interpol, __inputdir );
TGoF                gof             = GetCLIOptionFiles ( interpol, __files, __inputdir );

if ( gof.IsEmpty () ) {

    ConsoleErrorMessage ( 0, "No input files provided!" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           fromxyzfile     = GetCLIOptionFile   ( interpol, __fromxyz, __inputdir );

string              fromfront       = GetCLIOptionString ( interpol, __fromfront );
string              fromleft        = GetCLIOptionString ( interpol, __fromleft  );
string              fromtop         = GetCLIOptionString ( interpol, __fromtop   );
string              fromright       = GetCLIOptionString ( interpol, __fromright );
string              fromrear        = GetCLIOptionString ( interpol, __fromrear  );
int                 numfromlandmarks= ! fromfront.empty () + ! fromleft.empty () + ! fromtop.empty () + ! fromright.empty () + ! fromrear.empty ();

                                        // either no landmarks at all, using the input "as is", or all 5 landmarks
if ( numfromlandmarks > 0 && numfromlandmarks < 5 ) {

    ConsoleErrorMessage ( 0, "You must either provide the 5 'From' landmarks, or none at all!" );
    return;
    }

                                        // !If no landmarks are provided, it will assume the xyz is already centered, oriented and (somehow) normalized!
ElectrodesNormalizationOptions  fromnormalized  = numfromlandmarks == 5 ? FiducialNormalization : AlreadyNormalized;


string              badelec         = GetCLIOptionString ( interpol, __badelec  );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           toxyzfile       = GetCLIOptionFile   ( interpol, __toxyz, __inputdir );

string              tofront         = GetCLIOptionString ( interpol, __tofront );
string              toleft          = GetCLIOptionString ( interpol, __toleft  );
string              totop           = GetCLIOptionString ( interpol, __totop   );
string              toright         = GetCLIOptionString ( interpol, __toright );
string              torear          = GetCLIOptionString ( interpol, __torear  );
int                 numtolandmarks  = ! tofront.empty () + ! toleft.empty () + ! totop.empty () + ! toright.empty () + ! torear.empty ();

                                        // either no landmarks at all, using the input "as is", or all 5 landmarks
if ( numtolandmarks > 0 ) {

    if      ( toxyzfile.IsEmpty () ) {

        ConsoleErrorMessage ( 0, "You can not specify 'To' landmarks without providing a 'To' electrodes file!" );
        return;
        }

    else if ( numtolandmarks < 5 ) {

        ConsoleErrorMessage ( 0, "You must either provide the 5 'To' landmarks, or none at all!" );
        return;
        }
    }

                                        // !If no landmarks are provided, it will assume the xyz is already centered, oriented and (somehow) normalized!
ElectrodesNormalizationOptions  tonormalized    = numtolandmarks == 5 ? FiducialNormalization : AlreadyNormalized;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( badelec.empty () && toxyzfile.IsEmpty () ) {

    ConsoleErrorMessage ( 0, "You have not specified either bad electrodes nor a different target space!" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string                  splinemethod    = GetCLIOptionEnum ( interpol, __splinemethod );

TracksInterpolationType interpolationtype   = splinemethod == __splinesurface        ? Interpolation2DSurfaceSpline
                                            : splinemethod == __splinespherical      ? InterpolationSphericalSpline
                                            : splinemethod == __spline3d             ? Interpolation3DSpline
                                            : splinemethod == __splinecurrentdensity ? InterpolationSphericalCurrentDensitySpline
                                            :                                          DefaultInterpolationType;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 splinedegree        = GetCLIOptionInt       ( interpol, __splinedegree );

if ( ! HasCLIOption ( interpol, __splinedegree ) )
    splinedegree    = DefaultInterpolationDegree;

if ( ! IsInsideLimits ( splinedegree, MinInterpolationDegree, MaxInterpolationDegree ) ) {

    string                  splinedegreetext    = "Spline degree shoule be in ["
                                                + (string) IntegerToString ( MinInterpolationDegree ) + ".." + (string) IntegerToString ( MaxInterpolationDegree ) + "] range!";

    ConsoleErrorMessage ( __splinedegree, splinedegreetext.c_str () );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              extension       = GetCLIOptionEnum ( interpol, __extension );

SavingEegFileTypes  filetype        = ExtensionToSavingEegFileTypes ( extension.c_str () );

string              infix           = GetCLIOptionString ( interpol, __infix );

bool                nocleanup       = HasCLIFlag ( interpol, __nocleanup );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Console output prototype
//#define     CLIConsoleOutput
#undef      CLIConsoleOutput

#if defined(CLIConsoleOutput)

CreateConsole ();


TVerboseFile        verbose ( "cout", VerboseFileDefaultWidth );


verbose.NextTopic ( "'From' Space:" );
{
verbose.Put ( "Electrodes coordinates file:", fromxyzfile );
verbose.NextLine ();

verbose.Put ( "Bad electrodes:", badelec.empty () ? "None" : badelec.c_str () );

verbose.Put ( "Re-orient, center & normalize:", fromnormalized == AlreadyNormalized       ? "Using electrodes coordinates from file 'as is'" 
                                                                /*FiducialNormalization*/ : "Transforming to a Fiducial coordinate system" );

if ( fromnormalized == FiducialNormalization ) {
    verbose.Put ( "Fiducial Front:", fromfront );
    verbose.Put ( "Fiducial Left :", fromleft  );
    verbose.Put ( "Fiducial Top  :", fromtop   );
    verbose.Put ( "Fiducial Right:", fromright );
    verbose.Put ( "Fiducial Rear :", fromrear  );
    }
}


verbose.NextTopic ( "'To' Space:" );
{
verbose.Put ( "Destination space is:", toxyzfile.IsEmpty () ? "Identical to 'From' space" : "Another space" );

if ( toxyzfile.IsNotEmpty () ) {

    verbose.Put ( "Electrodes coordinates file:", toxyzfile );
    verbose.NextLine ();

    verbose.Put ( "Re-orient, center & normalize:", tonormalized == AlreadyNormalized       ? "Using electrodes coordinates from file 'as is'" 
                                                                  /*FiducialNormalization*/ : "Transforming to a Fiducial coordinate system" );

    if ( tonormalized == FiducialNormalization ) {
        verbose.Put ( "Fiducial Front:", tofront );
        verbose.Put ( "Fiducial Left :", toleft  );
        verbose.Put ( "Fiducial Top  :", totop   );
        verbose.Put ( "Fiducial Right:", toright );
        verbose.Put ( "Fiducial Rear :", torear  );
        }
    }
}


if ( toxyzfile.IsNotEmpty () ) {

    verbose.NextTopic ( "Coregistration between 'From' and 'To' Spaces:" );

    verbose.Put ( "Coregistration method:", toxyzfile.IsEmpty () ? "None" : "Affine transformation (12 parameters)" );
    }


verbose.NextTopic ( "Interpolation:" );
{
verbose.Put ( "Interpolation method:" );

if      ( interpolationtype == Interpolation2DSurfaceSpline                 )   (ofstream&) verbose << "Surface Spline"                         << NewLine;
else if ( interpolationtype == InterpolationSphericalSpline                 )   (ofstream&) verbose << "Spherical Spline"                       << NewLine;
else if ( interpolationtype == InterpolationSphericalCurrentDensitySpline   )   (ofstream&) verbose << "Current Density with Spherical Spline"  << NewLine;
else if ( interpolationtype == Interpolation3DSpline                        )   (ofstream&) verbose << "3D Spline"                              << NewLine;

verbose.Put ( "Degree of the Spline:", splinedegree );
}


verbose.NextTopic ( "Input files:" );
{
verbose.Put ( "Number of input files:", (int) gof );

for ( int i = 0; i < (int) gof; i++ )
    verbose.Put ( "Input file:", gof[ i ] );
}


verbose.NextTopic ( "Options:" );
{
verbose.Put ( "Input directory:",           inputdir.IsEmpty () ? "None" : inputdir );
verbose.Put ( "File name infix:",           infix );
verbose.Put ( "Saving intermediate files:", nocleanup );
}


verbose.Close ();

cout << NewLine;
cout << NewLine;

#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TInterpolateTracks  IT;

                                        // Init points, transform & matrices - better to do it only once for a batch of files, for efficiency reasons
                                        // !If caller didn't give the 5 fiducial landmarks, it will be assumed that the 2 electrodes sets are centered, normalized and in the same orientation!
IT.Set  (   interpolationtype,      splinedegree,

            fromxyzfile,
            fromnormalized,
            fromfront.c_str (),     fromleft.c_str (),      fromtop.c_str (),   fromright.c_str (),     fromrear.c_str (),      
            badelec.c_str (),
            
            toxyzfile,                  // to missing = back to from
            tonormalized,
            tofront.c_str (),       toleft.c_str (),        totop.c_str (),     toright.c_str (),       torear.c_str (),      

            gof[ 0 ],                   // temp files in the same directory as first EEG file
            Silent
        );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           neweegfile;

                            // change loop to account for multi-sessions EEG?
for ( int filei = 0; filei < (int) gof; filei++ ) {

#if defined(CLIConsoleOutput)
    cout << "Now Processing: " << gof  [ filei ]<< NewLine;
#endif

    TOpenDoc<TTracksDoc>    EEGDoc ( gof[ filei ], OpenDocHidden );

                                        // !Does NOT run any consistency check on electrodes!
    IT.InterpolateTracks    (   EEGDoc,
                                infix.c_str (),     SavingEegFileExtPreset[ filetype ],     neweegfile,
                                Silent
                            );

    } // for file


if ( ! nocleanup )
                                        // removing all temp files, if requested
    IT.FilesCleanUp ();


#if defined(CLIConsoleOutput)
DeleteConsole ( true );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
