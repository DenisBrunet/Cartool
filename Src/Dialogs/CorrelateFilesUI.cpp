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

#include    "CorrelateFiles.h"

#include    "Dialogs.Input.h"

#include    "TFreqDoc.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void    TCartoolMdiClient::CorrelateFilesUI ()
{
                                        // give a generic title at first
const char*         corrtitle       = "Computing Correlation";

char                answer          = GetOptionFromUser (   "Please specify the type of correlation/coherence you want:" NewLine 
                                                            Tab "- (S)patial Correlation between data and templates" NewLine 
                                                            Tab "- (T)ime-course Correlation" NewLine 
                                                            Tab "- P(H)ase-Intensity Coupling", 
                                                            corrtitle, "S T H", "", this );

if ( answer == EOS )   return;


CorrelateType       correlate       = answer == 'S' ? CorrelateTypeSpatialCorrelation
                                    : answer == 'T' ? CorrelateTypeTimeCorrelation
                                    : answer == 'H' ? CorrelateTypePhaseIntCoupling
                                    :                 CorrelateTypeNone;

if ( correlate == CorrelateTypeNone ) {
    ShowMessage ( "Oops, invalid option, bye-bye!", corrtitle, ShowMessageWarning );
    return;
    }

                                        // now we can give a more specific title
corrtitle   = CorrelateTypeNames[ correlate ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // for now, not all processing can handle explicit frequency files
                                        // the other processing can handle either tracks or frequency files
bool                nofreqsallowed  = ! CorrelateTypeFrequenciesAllowed ( correlate );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TGoF                filenames1;


GetFileFromUser     getfiles1 ( correlate == CorrelateTypeSpatialCorrelation ? "Data file(s) to be correlated:"
                              : correlate == CorrelateTypeTimeCorrelation    ? "First time series of data:"
//                            : correlate == CorrelateTypeCoherence          ? "First frequency time series:"
                              : correlate == CorrelateTypePhaseIntCoupling   ? "First frequency time series, either Phase or Intensity:"
                              : "First group of files:",
                                   
                                nofreqsallowed ? AllErpEegRisFilesFilter : AllErpEegFreqRisFilesFilter,

                                nofreqsallowed ? 3 : 4, GetFileMulti );

if ( ! getfiles1.Execute () )
    return;


filenames1  = (TGoF&) getfiles1;
                                        // no files? get out
if ( filenames1.IsEmpty () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TracksCompatibleClass   tc1;
FreqsCompatibleClass    fc1;
TracksGroupClass        tg1;

                                        // Check first for the new group's own coherence, as tracks files
filenames1.AllTracksAreCompatible ( tc1 );


if ( tc1.NumTracks == CompatibilityIrrelevant ) {
    ShowMessage ( "Files don't seem to have any electrodes or tracks at all!", corrtitle, ShowMessageWarning );
    return;
    }


if ( CorrelateTypeSameTracks ( correlate ) ) { 
    
    if      ( tc1.NumTracks == CompatibilityNotConsistent ) {
        ShowMessage ( "Files don't seem to share the same number of electrodes!", corrtitle, ShowMessageWarning );
        return;
        }
    else if ( tc1.NumTracks <= 1 ) {
        ShowMessage ( "There is only 1 track, which is not enough for a map!", corrtitle, ShowMessageWarning );
        return;
        }

    if ( tc1.NumAuxTracks > 0 ) {
        ShowMessage ( "It is not allowed to compute the correlation with some remaining auxiliary tracks!", corrtitle, ShowMessageWarning );
        return;
        }
    }


if ( tc1.NumTF == CompatibilityIrrelevant ) {
    ShowMessage ( "Files don't seem to have any samples or time at all!", corrtitle, ShowMessageWarning );
    return;
    }


if ( CorrelateTypeSameDuration ( correlate ) ) { 
    
    if      ( tc1.NumTF == CompatibilityNotConsistent ) {
        ShowMessage ( "Files don't seem to share the same number of time frames!", corrtitle, ShowMessageWarning );
        return;
        }
    else if ( tc1.NumTF <= 1 ) {
        ShowMessage ( "There is only 1 time point, which is not enough for a time series!", corrtitle, ShowMessageWarning );
        return;
        }
    }


if ( nofreqsallowed && tc1.NumFreqs != CompatibilityIrrelevant ) {
    ShowMessage ( "Frequency files are currently not allowed for your selected processing!", corrtitle, ShowMessageWarning );
    return;
    }


if ( CorrelateTypeFrequenciesAllowed ( correlate ) ) {

    if ( CorrelateTypeSameFrequencies ( correlate ) && tc1.NumFreqs == CompatibilityNotConsistent ) {
        ShowMessage ( "Files don't seem to have the same number of frequencies!", corrtitle, ShowMessageWarning );
        return;
        }
    }


                                        // Group is coherent regarding file types? (doing this first, maybe?)
if ( ! filenames1.AnyTracksGroup ( tg1 ) ) {

    if      (   tg1.noneofthese )   ShowMessage ( "Oops, files don't really look like tracks,\nare you trying to fool me?!", corrtitle, ShowMessageWarning );
    else if ( ! tg1.alleeg      )   ShowMessage ( "Files don't seem to be consistently of the EEG type!", corrtitle, ShowMessageWarning );
    else if ( ! tg1.allfreq     )   ShowMessage ( "Files don't seem to be consistently of the Frequency type!", corrtitle, ShowMessageWarning );
    else if ( ! tg1.allris      )   ShowMessage ( "Files don't seem to be consistently of the RIS type!", corrtitle, ShowMessageWarning );
    else                            ShowMessage ( "Files don't seem to be consistently of the same type!", corrtitle, ShowMessageWarning );

    return;
    }

                                        // Check the new group's own coherence, as frequency files
if ( tg1.allfreq ) {

    filenames1.AllFreqsAreCompatible ( fc1 );

                                        // do we actually care?
//  if      ( fc1.OriginalSamplingFrequency == CompatibilityNotConsistent ) {
//      ShowMessage ( "Files don't seem to have the same original sampling frequencies!", corrtitle, ShowMessageWarning );
//      return;
//      }


    if      ( fc1.FreqType == CompatibilityNotConsistent ) {
        ShowMessage ( "Files don't seem to be of the same frequency types!", corrtitle, ShowMessageWarning );
        return;
        }

    if ( correlate == CorrelateTypePhaseIntCoupling
      && ! ( IsFreqTypePhase    ( (FrequencyAnalysisType) fc1.FreqType ) 
          || IsFreqTypePositive ( (FrequencyAnalysisType) fc1.FreqType ) ) ) {
        ShowMessage ( "Files do seem to be of Phase or Intensity types!", corrtitle, ShowMessageWarning );
        return;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // store some of the extracted variables
bool                alleeg1         = tg1.alleeg;
bool                alleegerp1      = tg1.alleeg && filenames1.AllExtensionsAre ( AllEegErpFilesExt   );
bool                alleegspont1    = tg1.alleeg && filenames1.AllExtensionsAre ( AllEegSpontFilesExt );
bool                allfreq1        = tg1.allfreq;
//bool                allris1         = tg1.allris;
bool                allrisv1        = tg1.allrisv;

                                        // Here we have a TGoF which has some kind of coherence

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We ask spatial filter only for tracks
bool                spatialfilter1  = nofreqsallowed 
                                   && correlate == CorrelateTypeSpatialCorrelation  // right now, we cannot do a spatial filter if data are not ordered as spatial maps...
                                   && tc1.NumTracks > 0 
                                   && alleeg1
                                   && GetAnswerFromUser ( "Applying Spatial filtering on this first group?", corrtitle, this );


TFileName           xyzfile;


if ( spatialfilter1 ) {
    GetFileFromUser     getxyzfile ( "", AllCoordinatesFilesFilter, 1, GetFileRead );

    if ( ! getxyzfile.Execute () || (int) getxyzfile < 1 )
        spatialfilter1  = false;
    else 
        StringCopy ( xyzfile, getxyzfile[ 0 ] );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TGoF                filenames2;


GetFileFromUser     getfiles2 ( correlate == CorrelateTypeSpatialCorrelation ? "Template file(s) to correlate the data with:"
                              : correlate == CorrelateTypeTimeCorrelation    ? "Second time series of data:"
//                            : correlate == CorrelateTypeCoherence          ? "Second frequency time series:"
                              : correlate == CorrelateTypePhaseIntCoupling   ? "Second frequency time series, either Phase or Intensity:"
                              : "Second group of files:",
                                   
                                nofreqsallowed ? AllErpEegRisFilesFilter : AllErpEegFreqRisFilesFilter,

                                alleegerp1 ? 2 : alleegspont1 ? 3 : tg1.allfreq ? 4 : tg1.allris ? 5 : 1 /*6*/, // we can help the user by preselecting the right type of files
                                GetFileMulti );

if ( ! getfiles2.Execute () )
    return;


filenames2  = (TGoF&) getfiles2;
                                        // no files? get out
if ( filenames2.IsEmpty () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TracksCompatibleClass   tc2;
FreqsCompatibleClass    fc2;
TracksGroupClass        tg2;

                                        // Check first for the new group's own coherence, as tracks files
filenames2.AllTracksAreCompatible ( tc2 );


if ( tc2.NumTracks == CompatibilityIrrelevant ) {
    ShowMessage ( "Files don't seem to have any electrodes or tracks at all!", corrtitle, ShowMessageWarning );
    return;
    }


if ( CorrelateTypeSameTracks ( correlate ) ) { 
    
    if      ( tc2.NumTracks == CompatibilityNotConsistent ) {
        ShowMessage ( "Files don't seem to share the same number of electrodes!", corrtitle, ShowMessageWarning );
        return;
        }
    else if ( tc2.NumTracks <= 1 ) {
        ShowMessage ( "There is only 1 track, which is not enough for a map!", corrtitle, ShowMessageWarning );
        return;
        }

    if ( tc2.NumAuxTracks > 0 ) {
        ShowMessage ( "It is not allowed to compute the correlation with some remaining auxiliary tracks!", corrtitle, ShowMessageWarning );
        return;
        }
    }


if ( tc2.NumTF == CompatibilityIrrelevant ) {
    ShowMessage ( "Files don't seem to have any samples or time at all!", corrtitle, ShowMessageWarning );
    return;
    }


if ( CorrelateTypeSameDuration ( correlate ) ) { 
    
    if      ( tc2.NumTF == CompatibilityNotConsistent ) {
        ShowMessage ( "Files don't seem to share the same number of time frames!", corrtitle, ShowMessageWarning );
        return;
        }
    else if ( tc2.NumTF <= 1 ) {
        ShowMessage ( "There is only 1 time point, which is not enough for a time series!", corrtitle, ShowMessageWarning );
        return;
        }
    }


if ( nofreqsallowed && tc2.NumFreqs != CompatibilityIrrelevant ) {
    ShowMessage ( "Frequency files are currently not allowed for your selected processing!", corrtitle, ShowMessageWarning );
    return;
    }


if ( CorrelateTypeFrequenciesAllowed ( correlate ) ) {

    if ( CorrelateTypeSameFrequencies ( correlate ) && tc2.NumFreqs == CompatibilityNotConsistent ) {
        ShowMessage ( "Files don't seem to have the same number of frequencies!", corrtitle, ShowMessageWarning );
        return;
        }
    }

                                        // Group is coherent regarding file types? (doing this first, maybe?)
if ( ! filenames2.AnyTracksGroup ( tg2 ) ) {

    if      (   tg2.noneofthese )   ShowMessage ( "Oops, files don't really look like tracks,\nare you trying to fool me?!", corrtitle, ShowMessageWarning );
    else if ( ! tg2.alleeg      )   ShowMessage ( "Files don't seem to be consistently of the EEG type!", corrtitle, ShowMessageWarning );
    else if ( ! tg2.allfreq     )   ShowMessage ( "Files don't seem to be consistently of the Frequency type!", corrtitle, ShowMessageWarning );
    else if ( ! tg2.allris      )   ShowMessage ( "Files don't seem to be consistently of the RIS type!", corrtitle, ShowMessageWarning );
    else                            ShowMessage ( "Files don't seem to be consistently of the same type!", corrtitle, ShowMessageWarning );

    return;
    }

                                        // Check the new group's own coherence, as frequency files
if ( tg2.allfreq ) {

    filenames2.AllFreqsAreCompatible ( fc2 );

                                        // do we actually care?
//  if      ( fc2.OriginalSamplingFrequency == CompatibilityNotConsistent ) {
//      ShowMessage ( "Files don't seem to have the same original sampling frequencies!", corrtitle, ShowMessageWarning );
//      return;
//      }


    if      ( fc2.FreqType == CompatibilityNotConsistent ) {
        ShowMessage ( "Files don't seem to be of the same frequency types!", corrtitle, ShowMessageWarning );
        return;
        }

    if ( correlate == CorrelateTypePhaseIntCoupling
      && ! ( IsFreqTypePhase    ( (FrequencyAnalysisType) fc2.FreqType ) 
          || IsFreqTypePositive ( (FrequencyAnalysisType) fc2.FreqType ) ) ) {
        ShowMessage ( "Files do seem to be of Phase or Intensity types!", corrtitle, ShowMessageWarning );
        return;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // store some of the extracted variables
bool                alleeg2         = tg2.alleeg;
bool                allfreq2        = tg2.allfreq;
//bool                allris2         = tg2.allris;
bool                allrisv2        = tg2.allrisv;

                                        // Here we have a TGoF which has some kind of coherence

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We ask spatial filter only for tracks
bool                spatialfilter2  = nofreqsallowed 
                                   && correlate == CorrelateTypeSpatialCorrelation  // right now, we cannot do a spatial filter if data are not ordered as spatial maps...
                                   && tc2.NumTracks > 0 
                                   && alleeg2
                                   && GetAnswerFromUser ( "Applying Spatial filtering on this second group?", corrtitle, this );

                                        // !actually, we might have to ask for another xyz, in case of time correlation, data might have different dimensions as input!
if ( spatialfilter2 && StringIsEmpty ( xyzfile ) ) {
    GetFileFromUser     getxyzfile ( "", AllCoordinatesFilesFilter, 1, GetFileRead );

    if ( ! getxyzfile.Execute () || (int) getxyzfile < 1 )
        spatialfilter1  = spatialfilter2  = false;
    else 
        StringCopy ( xyzfile, getxyzfile[ 0 ] );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Finally, do some checking on the 2 groups of files

if ( CorrelateTypeSameTracks ( correlate ) && tc1.NumTracks != tc2.NumTracks ) {
    ShowMessage ( "Your two groups of files have different number of tracks!", corrtitle, ShowMessageWarning );
    return;
    }


if ( CorrelateTypeSameDuration ( correlate ) && tc1.NumTF != tc2.NumTF ) {
    ShowMessage ( "Your two groups of files have different durations!", corrtitle, ShowMessageWarning );
    return;
    }


if ( CorrelateTypeSameFrequencies ( correlate ) && tc1.NumFreqs != tc2.NumFreqs ) {
    ShowMessage ( "Your two groups of files have different number of frequencies!", corrtitle, ShowMessageWarning );
    return;
    }

                                        // what about vectorial data?
                                        // correlation somehow works, but there some details to take care (dimensions and stuff), so it's not usable for the moment
if ( allrisv1 || allrisv2 ) {
    ShowMessage ( "Correlation on vectorial data is not yet fully implemented,\nsorry about that!", corrtitle, ShowMessageWarning );
    return;
    }

if ( allrisv1 ^ allrisv2 ) {
    ShowMessage ( "Your two groups of files should either be scalar or vectorial,\nbut not a combination of both!", corrtitle, ShowMessageWarning );
    return;
    }

                                        // for now, only allows 2D on 2D (tracks), or 3D on 3D (freqs)
if ( allfreq1 ^ allfreq2 ) {
    ShowMessage ( "You can not mix FREQ files with other types of files!", corrtitle, ShowMessageWarning );
    return;
    }

// we could check we have phase and intensity

                                        // we could test also that we have eeg x eeg, or ris x ris, but we can also allow it as a trick, as long as the sizes match...

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optionally clipping beginning and ending time frames?
int                 cliptf          = 0;

                                        // for those temporal correlations
                                        // this is aimed toward frequency files, but it might also be useful on regular EEG files, to clip artifacted beginning and ending
if ( correlate == CorrelateTypeTimeCorrelation
  || correlate == CorrelateTypePhaseIntCoupling ) {

    char                buff[ 256 ];

    GetInputFromUser ( "Temporal margin removed on each side, specified in [ms], [s] or [TF]:", corrtitle, buff, "2s", this );

    cliptf  = Round ( StringToTimeFrame ( buff, tc1.SamplingFrequency ) );

    Maxed ( cliptf, 0 );

//  DBGV2 ( tc1.samplingfrequency, cliptf, buff );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optionally running some randomization
int                 numrand         = 0;

                                        // Randomization only done on phase-intensity coupling
if ( correlate == CorrelateTypePhaseIntCoupling ) {

    GetValueFromUser ( "Number of randomization trials, or 0 to skip randomization:", corrtitle, numrand, "1000", this );

    Maxed ( numrand, 0 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // !vectorial case not tested!
AtomType           	datatype1       = allrisv1 ? AtomTypeVector : AtomTypeScalar;
AtomType           	datatype2       = allrisv2 ? AtomTypeVector : AtomTypeScalar;


bool                ignorepolarity  = ( alleeg1 || alleeg2 ) 
                                    && correlate == CorrelateTypeSpatialCorrelation
                                    && ! ( IsAbsolute ( datatype1 ) || IsAbsolute ( datatype2 ) )
                                    && GetAnswerFromUser ( "Ignore polarity?", corrtitle, this );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here done asking user
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge     Gauge;
Gauge.Set       ( corrtitle );

Gauge.AddPart   ( gaugecorrglobal,  1,  5 );

if      ( correlate == CorrelateTypeSpatialCorrelation ) {
    Gauge.AddPart   ( gaugecorrfile1,   (int) filenames1,                     15 );
    Gauge.AddPart   ( gaugecorrfile2,   (int) filenames1 * (int) filenames2,  80 );
    }
else if ( correlate == CorrelateTypeTimeCorrelation ) {
    Gauge.AddPart   ( gaugecorrfile1,   AtLeast ( 1, tc1.NumFreqs )   * (int) filenames1 * (int) filenames2,    45 );
    Gauge.AddPart   ( gaugecorrfile2,   AtLeast ( 1, tc1.NumFreqs )   * (int) filenames1 * (int) filenames2,    50 );
    }
else if ( correlate == CorrelateTypePhaseIntCoupling ) {
    Gauge.AddPart   ( gaugecorrfile1,   tc1.NumTracks                 * (int) filenames1 * (int) filenames2 * ( numrand ? 2 : 1 ),    25 );
    Gauge.AddPart   ( gaugecorrfile2,   tc1.NumTracks * tc2.NumTracks * (int) filenames1 * (int) filenames2 * ( numrand ? 2 : 1 ),    70 );
    }


if ( (int) getfiles1 + (int) getfiles2 >= 5 )
                                        // batch can be long, hide Cartool until we are done
    WindowMinimize ( CartoolMainWindow );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Changing priority
SetProcessPriority ( BatchProcessingPriority );


CorrelateFiles (    filenames1,         filenames2, 
                    correlate, 
                    ignorepolarity,
                    spatialfilter1,     spatialfilter2,     xyzfile,
                    cliptf,
                    numrand,
                    corrtitle,         &Gauge 
                );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SetProcessPriority ();

WindowMaximize ( CartoolMainWindow );

//CartoolApplication->ResetMainTitle ();
Gauge.FinishParts ();
CartoolApplication->SetMainTitle ( corrtitle, "", Gauge );

Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
