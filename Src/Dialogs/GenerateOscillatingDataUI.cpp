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

#include    <owl/pch.h>

#include    "Files.Utils.h"
#include    "Math.Random.h"
#include    "Strings.Utils.h"
#include    "Strings.Grep.h"
#include    "Geometry.TDipole.h"
#include    "Dialogs.Input.h"

#include    "TSolutionPointsDoc.h" 
#include    "TLeadField.h"
#include    "TTracks.h"

#include    "GenerateOscillatingData.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    TCartoolMdiClient::GenerateOscillatingDataUI ()
{
char                title[ 256 ];
char                buff [ 256 ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TLeadField          leadfield;
TTracks<float>      K;


GetFileFromUser     getlffile ( "Opening Lead Field file:", AllLeadFieldFilesFilter, 1, GetFileRead );
TFileName           leadfieldfile;

if ( ! getlffile.Execute () || (int) getlffile < 1 )
    return;
else 
    StringCopy ( leadfieldfile, getlffile[ 0 ] );

                                        // try opening & check
if ( ! leadfield.Set ( leadfieldfile ) ) {
    ShowMessage ( "There seems to be a problem with the Lead Field file, which can be either:\n\n\t- The file type is not recognized\n\t- The Lead Field data type is not of vectorial type\n\t- The file itself might be somehow corrupted\n\nThis will end your processing now, bye bye...", ToFileName ( leadfieldfile ), ShowMessageWarning );
    return;
    }

                                        // try retrieving the matrix & check
if ( ! leadfield.ReadFile ( K ) ) {
    ShowMessage ( "There seems to be a problem with the Lead Field file,\n check the file type is correct.\n\nThis will end your processing now, bye bye...", ToFileName ( leadfieldfile ), ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TOpenDoc<TSolutionPointsDoc>    spdoc;


GetFileFromUser     getspfile ( "Opening Solution Points file:", AllSolPointsFilesFilter, 1, GetFileRead );
TFileName           spfile;

if ( ! getspfile.Execute () || (int) getspfile < 1 )
    return;
else 
    StringCopy ( spfile, getspfile[ 0 ] );

                                        // try opening & check
spdoc.Open ( spfile, OpenDocHidden );

if ( ! spdoc.IsOpen () ) {
    ShowMessage ( "There seems to be a problem with the Solution Points file!\n\nThis will end your processing now, bye bye...", ToFileName ( spfile ), ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

constexpr int       MaxDipolesOscillation   = 10;
TDipole             Dipoles[ MaxDipolesOscillation ];
int                 numsources;
TPointFloat         p;
int                 index;
TRandSpherical      randsph;


GetValueFromUser ( "Number of dipole sources (max 10):", GenerateOscillatingDataTitle, numsources, "2", this );

Clipped     ( numsources, 1, MaxDipolesOscillation );


for ( int di = 0; di < numsources; di++ ) {

    StringCopy      ( title, "Dipole #", IntegerToString ( buff, di + 1 ) );


                                        // Get position, with various syntaxes
    GetInputFromUser ( "Position of dipole:\n\tSol. Point Index (starting from 1)\nor\tSol. Point Name\nor\tX Y Z:", title, buff, "", this );

    StringCleanup   ( buff );
                                        // a bit of exploration

                                        // 3 floats?
    #define     GrepOneFloat        "[-+]?[0-9]*\\.?[0-9]+"
    #define     GrepSomeSeparators  "[\\s,;:]+"

    if      ( StringGrep ( buff, GrepOneFloat "" GrepSomeSeparators "" GrepOneFloat "" GrepSomeSeparators "" GrepOneFloat, GrepOptionDefault ) ) {

        sscanf  ( buff, "%f %f %f", &p.X, &p.Y, &p.Z );

        index   = spdoc->GetNearestElementIndex ( p );

        if ( index == -1 ) {
            ShowMessage ( "Can not find a solution point near your given position!", GenerateOscillatingDataTitle, ShowMessageWarning, this );
            return;
            }
        }
                                        // an index?
    else if ( StringGrep ( buff, "^\\d+$", GrepOptionDefault ) ) {
                                        // !don't forget to shift by 1!
        index   = StringToInteger ( buff ) - 1;

        if ( ! IsInsideLimits ( index, 0, spdoc->GetNumSolPoints () - 1 ) ) {
            ShowMessage ( "Solution point's index seems wrong!", GenerateOscillatingDataTitle, ShowMessageWarning, this );
            return;
            }
        }
                                        // a name?
    else if ( StringGrep ( buff, "((sp|SP)\\d+|[lrLR][apAP][siSI]\\d+)", GrepOptionDefault ) ) {

        index   = spdoc->GetIndex ( buff );

        if ( index == -1 ) {
            ShowMessage ( "Can not find the solution point's name!", GenerateOscillatingDataTitle, ShowMessageWarning, this );
            return;
            }
        }
                                        // ..wat?
    else {
        ShowMessage ( "Can not understand what solution point's you're talking about!", GenerateOscillatingDataTitle, ShowMessageWarning, this );
        return;
        }

                                        // here we good with position & index
    Dipoles[ di ].SolPointIndex     = index;
    Dipoles[ di ].Position          = spdoc->GetPoints ( DisplaySpace3D )[ index ];


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get dipole direction
    GetInputFromUser ( "Direction of dipole:\n\tX Y Z \nor\t(a)uto \nor\t(r)andom:", title, buff, "", this );

    StringCleanup   ( buff );

    if      ( StringStartsWith ( buff, "a" ) ) {
                                        // point from center to position, close to a normal
        TPointFloat         center;

//      spdoc->GetBounding ()->GetMiddle ( middle );
        center  = spdoc->GetPoints ( DisplaySpace3D ).GetCenter ();

        Dipoles[ di ].Direction = Dipoles[ di ].Position - center;
        }
    else if ( StringStartsWith ( buff, "r" ) ) {

        Dipoles[ di ].Direction = randsph ();
        }
    else
        sscanf  ( buff, "%f %f %f", &Dipoles[ di ].Direction.X , &Dipoles[ di ].Direction.Y , &Dipoles[ di ].Direction.Z );

    Dipoles[ di ].Direction.Normalize ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get dipole temporal coefficients
    GetInputFromUser ( "Dipole power, frequency [Hz] and phase [0..360):", title, buff, "1 10 0", this );

    StringCleanup   ( buff );

    sscanf  ( buff, "%lf %lf %lf", &Dipoles[ di ].Power, &Dipoles[ di ].Frequency, &Dipoles[ di ].Phase );

    Dipoles[ di ].Phase = DegreesToRadians ( Dipoles[ di ].Phase );

    } // for dipole


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              samplingfrequency;
char                durationstring[ 256 ];
int                 fileduration;


GetInputFromUser ( "Sampling Frequency and file duration:", GenerateOscillatingDataTitle, buff, "250 1min", this );

sscanf ( buff, "%lf %s", &samplingfrequency, durationstring );

if ( samplingfrequency <= 0 ) {
    ShowMessage ( "Erroneous sampling frequency, sorry!", GenerateOscillatingDataTitle, ShowMessageWarning );
    return;
    }


fileduration    = Round ( StringToTimeFrame ( durationstring, samplingfrequency ) );

if ( fileduration <= 0 ) {
    ShowMessage ( "Erroneous file duration, sorry!", GenerateOscillatingDataTitle, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Noise for later
//double              mapnoisesnrmin;
//double              mapnoisesnrmax;
//double              mapnoisesnrstep;
//
//GetInputFromUser ( "Range of Noise level, in dB (high=less noise, low=more noise) (max, min, step):", GenerateOscillatingDataTitle, buff, "50 0 10", this );
//
//sscanf ( buff, "%lf %lf %lf", &mapnoisesnrmax, &mapnoisesnrmin, &mapnoisesnrstep );
//
//CheckOrder  ( mapnoisesnrmin, mapnoisesnrmax );
//Maxed       ( mapnoisesnrstep, 1.0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           basefilename;

GetFileFromUser     getbasefile ( "Output directory:", AllFilesFilter, 1, GetFileDirectory );

if ( ! getbasefile.Execute ( (const char*) leadfieldfile ) || (int) getbasefile < 1 )
    return;
else 
    StringCopy ( basefilename, getbasefile[ 0 ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Go!
GenerateOscillatingData     (
                            leadfield,          K,                  
                            Dipoles,            numsources,
                            fileduration,       samplingfrequency,
                            basefilename,
                            true,               true
                            );

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
