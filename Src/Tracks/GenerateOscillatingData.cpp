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

#include    "GenerateOscillatingData.h"

#include    "Math.Stats.h"
#include    "Files.Utils.h"
#include    "Dialogs.TSuperGauge.h"
#include    "TMaps.h"

#include    "TLeadField.h"
#include    "TTracks.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Write maps and/or inverse for a set of oscillating dipoles
void    GenerateOscillatingData (   TLeadField&         leadfield,          TTracks<float>& K,                  
                                    TDipole             dipoles[],          int             numsources,
                                    int                 fileduration,       double          samplingfrequency,
                                    const char*         basefilename,
                                    bool                savemaps,           bool            saveris
                                )
{
if ( ! ( leadfield.IsOpen () && K.IsAllocated () ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // base directory & file names
TFileName           BaseDir;
TFileName           BaseFileName;
TFileName           fileoutprefix;
TFileName           buff;
TFileName           filename;


StringCopy      ( BaseDir,          basefilename                            );

StringCopy      ( fileoutprefix,    ToFileName ( basefilename ) );

StringCopy      ( BaseFileName,     BaseDir,     "\\",   fileoutprefix );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;
Gauge.Set       ( GenerateOscillatingDataTitle, SuperGaugeLevelInter );

Gauge.AddPart   ( 0,    fileduration,     90 );
Gauge.AddPart   ( 1,    2,                10 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numel           = leadfield.GetNumElectrodes     ();
int                 numsolp         = leadfield.GetNumSolutionPoints ();
int                 numsolp3        = 3 * numsolp;

TMaps               eegmaps    ( fileduration, numel    );
TArray1<TMapAtomType>   eegmap ( numel );
TMaps               sourcemaps ( fileduration, numsolp3 );


TEasyStats          stat;
int                 spi3;
double              time;
double              power;
TVector3Float       dipole;


for ( int mi = 0; mi < fileduration; mi++ ) {

    Gauge.Next ( 0 );
    CartoolObjects.CartoolApplication->SetMainTitle        ( GenerateOscillatingDataTitle, basefilename, Gauge );


    for ( int di = 0; di < numsources; di++ ) {

        time    = TimeFrameToSeconds ( mi, samplingfrequency );
                                        // global power
        power   = dipoles[ di ].Power 
                                        // opt for a sine                      phase is already in radian
                * sin ( time * dipoles[ di ].Frequency * TwoPi + dipoles[ di ].Phase );

        dipole  = dipoles[ di ].Direction * power;

                                        // cumulate all sources
        spi3    = 3 * dipoles[ di ].SolPointIndex;

        sourcemaps ( mi, spi3++ )  += dipole.X;
        sourcemaps ( mi, spi3++ )  += dipole.Y;
        sourcemaps ( mi, spi3++ )  += dipole.Z;
        }


    eegmap  = K * sourcemaps ( mi );

                                        // transfer results
    for ( int i = 0; i < numel; i++ )
        eegmaps ( mi, i )   = eegmap ( i );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

StringCopy      ( filename,     BaseFileName );
StringAppend    ( filename,     ".Oscillating ", IntegerToString ( buff, numsources ) );

for ( int di = 0; di < numsources; di++ )
//  StringAppend    ( filename,     ".", FloatToString ( buff, dipoles[ di ].Frequency, 2 ), "Hz" );
    StringAppend    ( filename,     di ? " " : ".", IntegerToString ( buff, Round ( dipoles[ di ].Frequency ) ), "Hz" );


                                        // saving maps
if ( savemaps ) {
    AddExtension        ( filename,     FILEEXT_EEGSEF );
    eegmaps.WriteFile   ( filename, false, samplingfrequency );
    RemoveExtension     ( filename );
    }

Gauge.Next ( 1 );

                                        // saving sources
if ( saveris ) {
    AddExtension        ( filename,     FILEEXT_RIS );
    sourcemaps.WriteFile( filename, true, samplingfrequency  );
    RemoveExtension     ( filename );
    }

Gauge.Next ( 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

WindowMaximize ( CartoolObjects.CartoolMainWindow );

CartoolObjects.CartoolApplication->SetMainTitle    ( GenerateOscillatingDataTitle, BaseFileName, Gauge );

Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
