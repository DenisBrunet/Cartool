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

#include    <owl/pch.h>

#include    "Files.TGoF.h"
#include    "Dialogs.Input.h"

#include    "TFreqDoc.h"

#include    "Files.BatchAveragingFiles.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Get a group of files from user, do some checking, then average according to file type
void    TCartoolMdiClient::BatchAveragingFilesUI ( owlwparam w )
{
using namespace crtl;

GetFileFromUser     files ( "Batch Averaging Files", AllErpEegFreqRisFilesFilter "|" AllOtherTracksFilter,
                            w == CM_BATCHAVERAGEEEG         ?   1
                          : w == CM_BATCHAVERAGEERRORDATA   ?   6
                          : w == CM_BATCHAVERAGERIS         ?   5
                          : w == CM_BATCHAVERAGEFREQ        ?   4
                          :                                     1, GetFileMulti );

if ( ! files.Execute () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Now run a lot of consistency checks
const TGoF&         gof             = (const TGoF&) files;


if ( (int) gof < 2 ) {
    ShowMessage ( "You didn't select enough files, please pick at least 2!", BatchAveragingTitle, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Too restrictive, we could bear to average different extensions together
//if ( ! gof.CheckExtension ( "Group of files doesn't share the same extension!" ) )
//    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check whole group file types
TracksGroupClass        tg;


if ( ! gof.AnyTracksGroup ( tg ) ) {
    if ( tg.noneofthese )   ShowMessage ( "Oops, files don't really look like tracks,\nare you trying to fool me?!\nCheck again your input files...", BatchAveragingTitle, ShowMessageWarning );
    else                    ShowMessage ( "Files don't seem to be consistently of the same type!\nCheck again your input files...", BatchAveragingTitle, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check whole group dimensions
TracksCompatibleClass   tc;

                                        // Checks for the most important dimensions (note that numsolpoints is NumTracks for .ris)
gof.AllTracksAreCompatible ( tc );

//DBGV6 ( NumTracks, numauxtracks, numsolpoints, numtf, NumFreqs, samplingfrequency, "NumTracks, numauxtracks, numsolpoints, numtf, NumFreqs, samplingfrequency" );


if      ( tc.NumTracks == CompatibilityNotConsistent ) {
    ShowMessage ( "Files don't seem to have the same number of electrodes/tracks!\nCheck again your input files...", BatchAveragingTitle, ShowMessageWarning );
    return;
    }
else if ( tc.NumTracks == CompatibilityIrrelevant ) {
    ShowMessage ( "Files don't seem to have any electrodes/tracks at all!\nCheck again your input files...", BatchAveragingTitle, ShowMessageWarning );
    return;
    }


//if      ( tc.NumAuxTracks > 0 ) {
//    ShowMessage ( "Files shouldn't have auxiliary channels!\nCheck again your input files...", BatchAveragingTitle, ShowMessageWarning );
//    return;
//    }
                                        // this test is quite weak, as ReadFromHeader does a lousy job at retrieving the aux tracks (it needs the strings, so opening the whole file)
if      ( tc.NumAuxTracks == CompatibilityNotConsistent ) {
    ShowMessage ( "Files don't seem to have the same number of auxiliary tracks!\nCheck again your input files...", BatchAveragingTitle, ShowMessageWarning );
    return;
    }


if      ( tc.NumTF == CompatibilityNotConsistent ) {
    ShowMessage ( "Files don't seem to have the same number of samples/time range!\nCheck again your input files...", BatchAveragingTitle, ShowMessageWarning );
    return;
    }
else if ( tc.NumTF == CompatibilityIrrelevant ) {
    ShowMessage ( "Files don't seem to have any samples or time at all!\nCheck again your input files...", BatchAveragingTitle, ShowMessageWarning );
    return;
    }


if      ( tc.NumFreqs == CompatibilityNotConsistent ) {
    ShowMessage ( "Files don't seem to have the same number of frequencies!\nCheck again your input files...", BatchAveragingTitle, ShowMessageWarning );
    return;
    }


if      ( tc.SamplingFrequency == CompatibilityNotConsistent ) {
//  ShowMessage ( "Files don't seem to have the same sampling frequencies!\nCheck again your input files...", BatchAveragingTitle, ShowMessageWarning );
    if ( ! GetAnswerFromUser ( "Files don't seem to have the same sampling frequencies!\nDo you want to proceed anyway (not recommended)?", BatchAveragingTitle ) )
        return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check frequency files coherence
FreqsCompatibleClass    fc;
FrequencyAnalysisType   freqtype                = FreqUnknown;
PolarityType            fftapproxpolarity       = PolarityDirect;


if ( tg.allfreq /*or NumFreqs > CompatibilityConsistent*/ ) {


    gof.AllFreqsAreCompatible ( fc );

                                        // NumTracks, numtf, NumFreqs and samplingfrequency have already been investigated before...

    if ( fc.OriginalSamplingFrequency == CompatibilityNotConsistent ) {
        ShowMessage ( "Files don't seem to have the same original sampling frequencies!\nCheck again your input files...", BatchAveragingTitle, ShowMessageWarning );
        return;
        }


    if ( fc.FreqType == CompatibilityNotConsistent ) {
        ShowMessage ( "Files don't seem to be of the same frequency types!\nCheck again your input files...", BatchAveragingTitle, ShowMessageWarning );
        return;
        }

                                        // store this
    freqtype    = (FrequencyAnalysisType) fc.FreqType;

                                        // FFT approximation case needs to know about polarity
    if ( IsFreqTypeFFTApproximation ( freqtype ) )
//      if ( ! GetPolarityFromUser ( FrequencyAnalysisNames[ FreqFFTApproximation ], fftapproxpolarity ) )
//          return;
                                        // Due to FFT Approximation uncertainty about polarity, it has to be evaluated in all cases anyway as is done in Frequency Analysis or Interactive Averaging
        fftapproxpolarity       = PolarityEvaluate;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // From here, files are assumed to be compatibles

                                        // Extra care for vectorial ris
bool                risvtos             = false;
bool                risvpooled          = false;
int                 poolednumlocalavg   = 0;
int                 poolednumrepeatavg  = 0;


if ( tg.allrisv ) {

    risvtos     = GetAnswerFromUser ( "Do you want to convert the vectorial RIS to Scalar?", BatchAveragingTitle );


//  if ( ! risvtos )
//      risvpooled  = GetAnswerFromUser ( "Do you want to run a pooled vectorial averaging?", BatchAveragingTitle );

    if ( risvpooled ) {

        int                 numfiles        = (int) gof;

        if ( ! GetValueFromUser ( "Number of epochs to locally average:", "Pooled Vectorial Averaging", poolednumlocalavg, "6" ) )  return;

        Clipped ( poolednumlocalavg, 1, numfiles );

        if ( ! GetValueFromUser ( "Number of repetitions:", "Pooled Vectorial Averaging", poolednumrepeatavg, "100" ) )             return;

        if ( poolednumrepeatavg <= 0 )    return;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           meanfile;
//TFileName         nmeanfile;
TFileName           sdfile;
//TFileName         snrfile;
//TFileName         medianfile;
//TFileName         madfile;
//TFileName         sphmeanfile;
TFileName           sphsdfile;
//TFileName         sphsnrfile;


if      (   tg.alleeg 
                                        // Vectorial to scalar ris is also done here
       || ( tg.allris && ( ! tg.allrisv || risvtos ) ) )    BatchAveragingScalar    (   gof,
                                                                                        meanfile,           sdfile,             0,
                                                                                        0,                  0,
//                                                                                      meanfile,           sdfile,             snrfile,
//                                                                                      medianfile,   madfile,
                                                                                        true,               true
                                                                                    );
else if   ( tg.allrisv ) {

    if ( risvpooled )                                       BatchPoolAveragingVectorial (   gof,
                                                                                            meanfile,           0,                  0,
                                                                                            0,                  sphsdfile,          0,
//                                                                                          meanfile,           nmeanfile,          snrfile,
//                                                                                          sphmeanfile,        sphsdfile,          sphsnrfile,
                                                                                            poolednumlocalavg,  poolednumrepeatavg,
                                                                                            true,               true
                                                                                        );

    else                                                    BatchAveragingVectorial (   gof,
                                                                                        meanfile,           0,                  0,
                                                                                        0,                  sphsdfile,          0,
//                                                                                      meanfile,           nmeanfile,          snrfile,
//                                                                                      sphmeanfile,        sphsdfile,          sphsnrfile,
                                                                                        true,               true
                                                                                    );
    } // allrisv

else if   ( tg.allfreq )                                    BatchAveragingFreq      (   gof,
                                                                                        freqtype,           fftapproxpolarity,
                                                                                        meanfile,           0,
//                                                                                      freqtype,           fftapproxpolarity,
//                                                                                      meanfile,           sdfile,
                                                                                        true,               true    
                                                                                    );

else if   ( tg.alldata )                                    BatchAveragingErrorData (   gof,    
                                                                                        meanfile,
                                                                                        true,               true
                                                                                    );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
