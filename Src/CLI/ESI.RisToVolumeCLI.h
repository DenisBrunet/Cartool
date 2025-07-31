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
DefineCLIOptionFile     ( ristovol,         "",     __spfile,               "Solution Points file" RequiredString );
//->Required ();    // interferes with --help

DefineCLIOptionFile     ( ristovol,         "",     __greyfile,             "Grey Mask file" RequiredString );
//->Required ();    // interferes with --help

NeedsCLIOption          ( ristovol,         __spfile,       __greyfile );
NeedsCLIOption          ( ristovol,         __greyfile,     __spfile   );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionInt      ( ristovol,         "",     __timemin,              __timemin_descr );
DefineCLIOptionInt      ( ristovol,         "",     __timemax,              __timemax_descr );
DefineCLIOptionInt      ( ristovol,         "",     __timestep,             "Stepping by time frames (Default is 1)" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionEnum     ( ristovol,         "",     __interpolation,        "Type of interpolation" )
->CheckOption           ( CLI::IsMember ( vector<string> ( { __1NN, __4NN, __linear, __cubickernel } ) ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DefineCLIOptionFile     ( ristovol,         "",     __inputdir,             __inputdir_descr )
->TypeOfOption          ( __inputdir_type )
->CheckOption           ( CLI::ExistingDirectory ); // could be incomplete, but it helps a bit, though

DefineCLIOptionFile     ( ristovol,         "",     __outputdir,            __outputdir_descr )
->TypeOfOption          ( __outputdir_type );

DefineCLIOptionString   ( ristovol,         "",     __prefix,               __prefix_descr );


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

DefineCLIFlag           ( ristovol,         __h,    __help,                 __help_descr );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Repeating positional files option seems OK
DefineCLIOptionFiles    ( ristovol, -1,     "",     __files,                __files_descr );
}


//----------------------------------------------------------------------------
                                        // Running the command
inline void     RisToVolumeCLI ( CLI::App* ristovol)
{
if ( ! IsSubCommandUsed ( ristovol )  )
    return;


TFileName           inputdir        = GetCLIOptionDir   ( ristovol, __inputdir );
TGoF                gof             = GetCLIOptionFiles ( ristovol, __files, __inputdir );

if ( gof.IsEmpty () ) {

    ConsoleErrorMessage ( 0, "No input files provided!" );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           spfile          = GetCLIOptionFile ( ristovol, __spfile  , __inputdir );

if ( spfile.IsEmpty () ) {

    ConsoleErrorMessage ( __spfile, "Missing Solution Points file!" );
    return;
    }


TFileName           greyfile        = GetCLIOptionFile ( ristovol, __greyfile, __inputdir );

if ( greyfile.IsEmpty () ) {

    ConsoleErrorMessage ( __greyfile, "Missing Grey Mask file!" );
    return;
    }


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

    ConsoleErrorMessage ( __interpolation, "Error while trying to compute the Solution Points Interpolation" );
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

TFileName           outputdir       = GetCLIOptionDir    ( ristovol, __outputdir );

string              prefix          = GetCLIOptionString ( ristovol, __prefix );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Console output prototype
//#define     CLIConsoleOutput
#undef      CLIConsoleOutput

#if defined(CLIConsoleOutput)

CreateConsole ();


TVerboseFile        verbose ( "cout", VerboseFileDefaultWidth );


verbose.NextTopic ( "Input Files:" );
{
verbose.Put ( "Input directory:",       inputdir.IsEmpty () ? "None" : inputdir );
verbose.Put ( "Solution Points file:",  spfile );
verbose.Put ( "MRI Grey Mask file:",    greyfile );

verbose.NextLine ();
verbose.Put ( "Number of input files:", (int) gof );
for ( int i = 0; i < (int) gof; i++ )
    verbose.Put ( "Input file:", gof[ i ] );
}


verbose.NextTopic ( "Volume Parameters:" );
{
//verbose.Put ( "Interpolation:",       interpolation.empty () ? "Default" : interpolation );
verbose.Put ( "Using interpolation:", VolumeInterpolationPresetString[ interpol ] );
//verbose.Put ( "SP interpolation:",    SPInterpolationTypeNames[ spinterpol ] );
}


verbose.NextTopic ( "Time Parameters:" );
{
verbose.Put ( "From     [TF]:", timemin == 0                   ? "Beginning of file" : IntegerToString ( timemin ) );
verbose.Put ( "To       [TF]:", timemax == Highest ( timemax ) ? "End of file"       : IntegerToString ( timemax ) );
verbose.Put ( "By steps [TF]:", timestep );
verbose.Put ( "Averaging each block:", merging == FilterTypeNone ? "None" : FilterPresets[ merging ].Ext /*Text*/ );
}


verbose.NextTopic ( "Output Files:" );
{
verbose.Put ( "Output directory:",  outputdir.IsEmpty () ? "None" : outputdir );
verbose.Put ( "File name prefix:",  prefix   .empty   () ? "None" : prefix    );
verbose.Put ( "Output data type:",  AtomFormatTypePresets[ atomformat ].Text );
verbose.Put ( "Output format:",     VolumeFileTypeString[ filetype ] );
verbose.Put ( "Output dimensions:", IsFileTypeN3D ( filetype ) ? 3 : 4 );
}


verbose.Close ();

cout << NewLine;
cout << NewLine;

#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int filei = 0; filei < (int) gof; filei++ ) {

#if defined(CLIConsoleOutput)
    cout << "Now Processing: " << gof  [ filei ]<< NewLine;
#endif

    TGoF                gofvol;

    RisToVolume (   gof[ filei ],
                    SPDoc,              interpol, 
                    GreyMaskDoc, 
                    timemin,            timemax,            timestep,
                    merging,
                    atomformat,             
                    outputdir,
                    prefix.c_str (),
                    filetype,
                    gofvol,         // not used
                    Silent
                );

    } // for file


#if defined(CLIConsoleOutput)
DeleteConsole ( true );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
