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
#include    "Files.TOpenDoc.h"

#include    "TSolutionPointsDoc.h"
#include    "TVolumeDoc.h"
#include    "ESI.RisToVolume.h"
#include    "TRisToVolumeDialog.h"

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Defining the interface
inline void     RisToVolumeCLIDefine ( CLI::App* ristovol )
{
if ( ristovol == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Parameters appearance follow the dialog's visual design
DefineCLIOptionFile     ( ristovol,         "",     __spfile,               "Solution Points file (Required)" );
//->Required ();    // interferes with --help

DefineCLIOptionFile     ( ristovol,         "",     __greyfile,             "Grey Mask file (Required)" );
//->Required ();    // interferes with --help

NeedsCLIOption          ( ristovol,         __spfile,       __greyfile );
NeedsCLIOption          ( ristovol,         __greyfile,     __spfile   );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionInt      ( ristovol,         "",     __timemin,              "Converting from Time Frame (default 0)" );
DefineCLIOptionInt      ( ristovol,         "",     __timemax,              "Converting to Time Frame (default End-Of-File)" );
DefineCLIOptionInt      ( ristovol,         "",     __timestep,             "Converting by Step Time Frames (default 1)" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionEnum     ( ristovol,         "",     __interpolation,        "Type of interpolation" )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __1NN, __4NN, __linear, __cubickernel } ) ) );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionString   ( ristovol,         "",     __prefix,               "Output files optional prefix" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionEnum     ( ristovol,         "",     __fileformat,           "File format" )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __nifti, __analyze } ) ) )
->DefaultString         ( "nifti" );


DefineCLIOptionEnum     ( ristovol,         "",     __typeformat,           "Internal file type" )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __byte, __float } ) ) )
->DefaultString         ( "float" );


DefineCLIOptionEnum     ( ristovol,         "",     __dimensions,           "File dimensions" )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __3D, __4D } ) ) )
->DefaultString         ( "4" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIFlag           ( ristovol,         __h,    __help,                 "This message" );
}


//----------------------------------------------------------------------------
                                        // Running the command
inline void     RisToVolumeCLI ( CLI::App* ristovol, const TGoF& gof )
{
if ( ! IsSubCommandUsed ( ristovol )
  || gof.IsEmpty () )

    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           spfile          = GetCLIOptionFile ( ristovol, __spfile   );
TFileName           greyfile        = GetCLIOptionFile ( ristovol, __greyfile );

if ( spfile.IsEmpty () || greyfile.IsEmpty () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Wraps everything for us: existing file name & opening doc
TOpenDoc<TSolutionPointsDoc>    SPDoc       ( spfile  , OpenDocHidden );
TOpenDoc<TVolumeDoc>            GreyMaskDoc ( greyfile, OpenDocHidden );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Volume parameters

string              interpolation   = GetCLIOptionEnum ( ristovol, __interpolation );


RisToVolumeInterpolationType    interpol    = interpolation == __1NN                ?   VolumeInterpolation1NN
                                            : interpolation == __4NN                ?   VolumeInterpolation4NN
                                            : interpolation == __linear             ?   VolumeInterpolationLinearRect
                                            : interpolation == __cubickernel        ?   VolumeInterpolationCubicFastSplineSpherical
                                            :                                           VolumeInterpolationPresetDefault;

                                        // Some volume interpolation needs the SP interpolation
SPInterpolationType             spinterpol  = interpol == VolumeInterpolation1NN    ?   SPInterpolation1NN
                                            : interpol == VolumeInterpolation4NN    ?   SPInterpolation4NN
                                            :                                           SPInterpolationNone;

                                        // Initialize the requested SP interpolation
if ( ! SPDoc->BuildInterpolation ( spinterpol, GreyMaskDoc ) ) {

    //ShowMessage ( "Error while trying to compute the Solution Points Interpolation", RisToVolumeTitle, ShowMessageWarning );
    return;
    }
    

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Time parameters, exclusively between time interval, triggers to keep or triggers to exclude
                                        // default values
long                timemin         = 0;
long                timemax         = Highest ( timemax );
long                timestep        = 1;


if  ( HasCLIOption ( ristovol, __timemin ) )
    timemin     = GetCLIOptionInt ( ristovol, __timemin );

if  ( HasCLIOption ( ristovol, __timemax ) )
    timemax     = GetCLIOptionInt ( ristovol, __timemax );

CheckOrder ( timemin, timemax );

if  ( HasCLIOption ( ristovol, __timestep ) )
    timestep    = AtLeast ( 1, GetCLIOptionInt ( ristovol, __timestep ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

FilterTypes         merging         = timestep > 1 ? FilterTypeMedian : FilterTypeNone;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Output parameters

string              typeformat      = GetCLIOptionEnum ( ristovol, __typeformat );


AtomFormatType      atomformat      = typeformat == __byte  ?   AtomFormatByte
                                    : typeformat == __float ?   AtomFormatFloat
                                    :                           RisToVolumeDefaultAtomFormat;


string              fileformat      = GetCLIOptionEnum ( ristovol, __fileformat );
string              dimensions      = GetCLIOptionEnum ( ristovol, __dimensions );

RisToVolumeFileType filetype        = fileformat == __nifti   && dimensions == __3D ?   VolumeNifti2N3D
                                    : fileformat == __nifti   && dimensions == __4D ?   VolumeNifti24D
                                    : fileformat == __analyze && dimensions == __3D ?   VolumeAnalyzeN3D
                                    : fileformat == __analyze && dimensions == __4D ?   VolumeAnalyze4D
                                    :                                                   VolumeTypeDefault;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

string              fileprefix      = GetCLIOptionString ( ristovol, __prefix );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                silent          = true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Console output prototype
//#define     CLIConsoleOutput
#undef      CLIConsoleOutput

#if defined(CLIConsoleOutput)

CreateConsole ();

cout << "SP file:        "          << spfile << NewLine;
cout << "Grey Mask File: "          << greyfile << NewLine;
cout << "Interpolation:  "          << ( interpolation.empty () ? "Default" : interpolation ) << NewLine;
cout << "Volume Interpolation: "    << VolumeInterpolationPresetString[ interpol ] << NewLine;
cout << "SP Interpolation: "        << SPInterpolationTypeNames[ spinterpol ] << NewLine;
cout << NewLine;

cout << "Time Min: "                << ( timemin == 0 ? "Beginning of file" : IntegerToString ( timemin ) ) << NewLine;
cout << "Time Max: "                << ( timemax == Highest ( timemax ) ? "End of file" : IntegerToString ( timemax ) ) << NewLine;
cout << "Time Step: "               << timestep << NewLine;
cout << NewLine;

cout << "Merging: "                 << FilterPresets[ merging ].Text << NewLine;
cout << NewLine;

cout << "File Format: "             << fileformat << NewLine;
cout << "File Type: "               << typeformat << NewLine;
cout << "File Dimensions: "         << dimensions << NewLine;
cout << NewLine;

#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int filei = 0; filei < (int) gof; filei++ ) {

#if defined(CLIConsoleOutput)
    cout << "Processing: " << gof  [ filei ]<< NewLine;
#endif

    TGoF                volgof;

    RisToVolume (   gof[ filei ],
                    SPDoc,              interpol, 
                    GreyMaskDoc, 
                    timemin,            timemax,            timestep,
                    merging,
                    atomformat,             
                    filetype,           fileprefix.c_str (),
                    volgof,         // not used
                    silent
                );

    } // for file


#if defined(CLIConsoleOutput)
DeleteConsole ( true );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
