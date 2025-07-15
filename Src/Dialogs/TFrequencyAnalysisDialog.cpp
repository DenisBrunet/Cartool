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

#include    "TFrequencyAnalysisDialog.h"

#include    "Math.FFT.h"
#include    "Dialogs.Input.h"

#include    "TTracksDoc.h"
 
#include    "TTracksView.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // declare this global variable
TFrequencyAnalysisStructEx  FrequencyAnalysisTransfer;


const char  FreqPresetsString[ NumFreqPresets ][ 128 ] =
            {
            "EEG / Surface / FFT (Power Maps)",
            "EEG / Surface / FFT (Power Maps) Average Spectrum",
            "EEG / Surface / Short-Term FFT (Power Maps)",
            "EEG / Surface / FFT Approximation",
            "EEG / Surface / Short-Term FFT Approximation",
            "EEG / Surface / Wavelet (S-Transform)",
            "EEG / Surface / Wavelet (S-Transform) for Sources Localization",
            "",
            "EEG / Intra-Cranial / FFT",
            "EEG / Intra-Cranial / FFT Average Spectrum",
            "EEG / Intra-Cranial / Short-Term FFT",
            "EEG / Intra-Cranial / Wavelet (S-Transform)",
            "EEG / Intra-Cranial / Wavelet (S-Transform) for Phase Analysis",
            "",
            "General case / FFT",
            "General case / FFT Average Spectrum",
            "General case / Short-Term FFT",
            "General case / Wavelet (S-Transform)",
            };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TFrequencyAnalysisStruct::TFrequencyAnalysisStruct ()
{
StringCopy  ( Channels, "*" );
UseXyzDoc           = BoolToCheck ( false );
ClearString ( XyzDocFile );

ClearString ( TimeMin );
ClearString ( TimeMax );
StringCopy  ( ClippedTimeMax, TimeMax );
EndOfFile           = BoolToCheck ( false );
ClearString ( NumBlocks );
ClearString ( BlockSize );
WindowOverlap0      = BoolToCheck ( DefaultWindowOverlap == 0.00 );
WindowOverlap75     = BoolToCheck ( DefaultWindowOverlap == 0.75 );
WindowOverlapMax    = BoolToCheck ( DefaultWindowOverlap == 1.00 );
ClearString ( WindowsInterval );
SkipBadEpochs       = BoolToCheck ( false );
ClearString ( SkipMarkers );

ClearString ( SamplingFrequency );
ClearString ( FreqMin );
ClearString ( FreqMax );
ClearString ( FreqStep );

SaveInterval      = BoolToCheck ( true  );
SaveBands         = BoolToCheck ( false );
ClearString ( SaveFreqMin );
ClearString ( SaveFreqMax );
ClearString ( SaveFreqStep );
SaveLogInterval   = BoolToCheck ( false );
IntegerToString ( SaveDecadeStep, DefaultSaveLogDecade );

ClearString ( SaveBandsValue );

Presets.Clear ();
for ( int i = 0; i < NumFreqPresets; i++ )
    Presets.AddString ( FreqPresetsString[ i ], i == FreqPresetDefault );

Fft                 = BoolToCheck ( true  );
FftApproximation    = BoolToCheck ( false );
STransform          = BoolToCheck ( false );

WindowingNone       = BoolToCheck ( false );
WindowingHanning    = BoolToCheck ( true  );

Mean                = BoolToCheck ( false );
Sequence            = BoolToCheck ( true  );


FFTNormalization.Clear ();
for ( int i = 0; i < NumFFTRescalingType; i++ )
    FFTNormalization.AddString ( FFTRescalingString[ i ], i == DefaultFFTRescaling );

WriteType.Clear ();
for ( int i = 0; i < NumFreqOutputAtomType; i++ )
    WriteType.AddString ( FreqOutputAtomString[ i ], i == DefaultFreqOutputAtom );

NoRef               = BoolToCheck ( false );
CurrentRef          = BoolToCheck ( false );
AvgRef              = BoolToCheck ( true  );
OtherRef            = BoolToCheck ( false );
ClearString ( RefList );

ClearString ( InfixFilename );
FileTypeFreq        = BoolToCheck ( true  );
CreateSubdir        = BoolToCheck ( false );
OpenAuto            = BoolToCheck ( true  );

SplitByElectrode    = BoolToCheck ( false );
SplitByFrequency    = BoolToCheck ( false );
SplitBySpectrum     = BoolToCheck ( false );

OptimalDownsampling = BoolToCheck ( false );
}


        TFrequencyAnalysisStructEx::TFrequencyAnalysisStructEx ()
{
ChannelsSel             = 0;
XyzLinked               = false;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TFrequencyAnalysisDialog, TBaseDialog )
    EV_WM_DROPFILES,

//  EV_COMMAND_ENABLE           ( IDC_CHANNELS,                 CmChannelsEnable ),

    EV_COMMAND                  ( IDC_BROWSEXYZDOC,             CmBrowseXyzDoc ),
    EV_COMMAND                  ( IDC_USEXYZDOC,                CmXyzChange ),
    EV_EN_CHANGE                ( IDC_XYZDOCFILE,               CmXyzChange ),
    EV_COMMAND_ENABLE           ( IDC_USEXYZDOC,                CmXyzEnable ),
    EV_COMMAND_ENABLE           ( IDC_XYZDOCFILE,               CmUseXyzDocEnable ),
    EV_COMMAND_ENABLE           ( IDC_BROWSEXYZDOC,             CmUseXyzDocEnable ),

    EV_EN_CHANGE                ( IDC_SAMPLINGFREQUENCY,        CmSamplingFrequencyChange ),
    EV_COMMAND_ENABLE           ( IDC_SAMPLINGFREQUENCY,        CmSamplingFrequencyEnable ),
//  EV_EN_CHANGE                ( IDC_SAVEFREQMIN,              CmSaveFrequenciesChange ),
//  EV_EN_CHANGE                ( IDC_SAVEFREQMAX,              CmSaveFrequenciesChange ),

    EV_COMMAND_ENABLE           ( IDC_SAVEFREQMIN,              CmIntervalEnable ),
    EV_COMMAND_ENABLE           ( IDC_SAVEFREQMAX,              CmIntervalEnable ),
    EV_COMMAND_ENABLE           ( IDC_SAVELOGINTERVAL,          CmIntervalEnable ),
    EV_COMMAND_ENABLE           ( IDC_SAVEFREQSTEP,             CmIntervalStepEnable  ),
    EV_COMMAND_ENABLE           ( IDC_DECADESTEP,               CmDecadeStepEnable ),
    EV_COMMAND_ENABLE           ( IDC_SAVEBANDSVALUE,           CmBandsEnable ),

    EV_EN_CHANGE                ( IDC_TIMEMIN,                  CmTimeLimitsChange ),
    EV_EN_CHANGE                ( IDC_TIMEMAX,                  CmTimeLimitsChange ),
    EV_COMMAND_ENABLE           ( IDC_TIMEMAX,                  CmTimeMaxEnable ),
    EV_COMMAND                  ( IDC_TOENDOFFILE,              CmEndOfFile ),
    EV_COMMAND_ENABLE           ( IDC_CLIPPEDTIMEMAX,           CmTimeMaxEnable ),

//  EV_EN_CHANGE                ( IDC_NUMBLOCKS,                CmNumBlocksChange ),
    EV_EN_CHANGE                ( IDC_BLOCKSIZE,                CmBlockSizeChange ),
    EV_COMMAND_ENABLE           ( IDC_BLOCKSIZE,                CmBlockSizeEnable ),
    EV_COMMAND_ENABLE           ( IDC_NUMBLOCKS,                CmTimeMaxEnable ),

    EV_COMMAND                  ( IDC_WINDOWOVERLAP0,           CmWindowOverlapChange ),
    EV_COMMAND                  ( IDC_WINDOWOVERLAP75,          CmWindowOverlapChange ),
    EV_COMMAND                  ( IDC_WINDOWOVERLAPMAX,         CmWindowOverlapChange ),
    EV_COMMAND_ENABLE           ( IDC_WINDOWOVERLAP75,          CmWindowOverlapEnable ),
    EV_COMMAND_ENABLE           ( IDC_WINDOWOVERLAPMAX,         CmWindowOverlapEnable ),

    EV_COMMAND_ENABLE           ( IDC_SKIPMARKERS,              CmSkipBadEpochsEnable ),

    EV_CBN_SELCHANGE            ( IDC_PRESETS,                  EvPresetsChange ),

    EV_COMMAND                  ( IDC_SIMPLEFFT,                CmSetAnalysis ),
    EV_COMMAND                  ( IDC_FFTAPPROXIMATION,         CmSetAnalysis ),
    EV_COMMAND                  ( IDC_STRANSFORM,               CmSetAnalysis ),
    EV_COMMAND_ENABLE           ( IDC_SIMPLEFFT,                CmAnalysisTypeEnable ),
    EV_COMMAND_ENABLE           ( IDC_STRANSFORM,               CmAnalysisTypeEnable ),
    EV_COMMAND_ENABLE           ( IDC_FFTAPPROXIMATION,         CmAnalysisTypeEnable ),


//  EV_COMMAND_ENABLE           ( IDC_WINDOWINGNONE,            CmSequenceMeanEnable ),
//  EV_COMMAND_ENABLE           ( IDC_WINDOWINGHANNING,         CmSequenceMeanEnable ),

    EV_COMMAND                  ( IDC_MEAN,                     AnalysisAtomtypeCompatibility ),
    EV_COMMAND                  ( IDC_SEQUENCE,                 AnalysisAtomtypeCompatibility ),
    EV_COMMAND_ENABLE           ( IDC_MEAN,                     CmSequenceMeanEnable ),
    EV_COMMAND_ENABLE           ( IDC_SEQUENCE,                 CmSequenceMeanEnable ),

    EV_COMMAND_ENABLE           ( IDC_FFTNORMALIZATION,         CmFFTNormalizationEnable ),

    EV_COMMAND                  ( IDC_SAVEBANDS,                AnalysisAtomtypeCompatibility ),
    EV_CBN_SELCHANGE            ( IDC_WRITETYPE,                AnalysisAtomtypeCompatibility ),
    EV_COMMAND_ENABLE           ( IDC_WRITETYPE,                CmWriteTypeEnable ),

    EV_COMMAND_ENABLE           ( IDC_NOREF,                    CmReferenceEnable ),
    EV_COMMAND_ENABLE           ( IDC_CURRENTREF,               CmReferenceEnable ),
    EV_COMMAND_ENABLE           ( IDC_AVEREF,                   CmReferenceEnable ),
    EV_COMMAND_ENABLE           ( IDC_OTHERREF,                 CmReferenceEnable ),
    EV_COMMAND_ENABLE           ( IDC_REFLIST,                  CmOtherRefEnable ),

    EV_COMMAND_ENABLE           ( IDC_EXPMARKERS,               CmMarkersEnable ),

    EV_COMMAND_ENABLE           ( IDC_SPLITBYELECTRODE,         CmSplitEnable ),
    EV_COMMAND_ENABLE           ( IDC_SPLITBYFREQUENCY,         CmSplitEnable ),
    EV_COMMAND_ENABLE           ( IDC_SPLITBYSPECTRUM,          CmSplitEnable ),
    EV_COMMAND_ENABLE           ( IDC_OPTIMALDOWNSAMPLING,      CmOptimalDownsamplingEnable ),

    EV_COMMAND_ENABLE           ( IDC_PROCESSCURRENT,           CmProcessCurrentEnable ),
    EV_COMMAND_ENABLE           ( IDC_PROCESSBATCH,             CmProcessBatchEnable ),
END_RESPONSE_TABLE;


        TFrequencyAnalysisDialog::TFrequencyAnalysisDialog ( TWindow* parent, TResId resId, TTracksDoc* doc  )
      : TBaseDialog ( parent, resId, doc )
{
StringCopy ( BatchFilesExt, AllEegRisFilesExt );


Channels            = new TEdit ( this, IDC_CHANNELS, EditSizeTextLong );
UseXyzDoc           = new TCheckBox ( this, IDC_USEXYZDOC );
XyzDocFile          = new TEdit ( this, IDC_XYZDOCFILE, EditSizeText );

TimeMin             = new TEdit ( this, IDC_TIMEMIN, EditSizeValue );
TimeMax             = new TEdit ( this, IDC_TIMEMAX, EditSizeValue );
ClippedTimeMax      = new TEdit ( this, IDC_CLIPPEDTIMEMAX, EditSizeValue );
EndOfFile           = new TCheckBox ( this, IDC_TOENDOFFILE );
NumBlocks           = new TEdit ( this, IDC_NUMBLOCKS, EditSizeValue );
BlockSize           = new TEdit ( this, IDC_BLOCKSIZE, EditSizeValue );
WindowOverlap0      = new TRadioButton ( this, IDC_WINDOWOVERLAP0 );
WindowOverlap75     = new TRadioButton ( this, IDC_WINDOWOVERLAP75 );
WindowOverlapMax    = new TRadioButton ( this, IDC_WINDOWOVERLAPMAX );
WindowsInterval     = new TEdit ( this, IDC_WINDOWSINTERVAL, EditSizeValue );
SkipBadEpochs       = new TCheckBox     ( this, IDC_SKIPBADEPOCHS );
SkipMarkers         = new TEdit         ( this, IDC_SKIPMARKERS, EditSizeText );

SamplingFrequency   = new TEdit ( this, IDC_SAMPLINGFREQUENCY, EditSizeValue );
FreqMin             = new TEdit ( this, IDC_FREQMIN, EditSizeValue );
FreqMax             = new TEdit ( this, IDC_FREQMAX, EditSizeValue );
FreqStep            = new TEdit ( this, IDC_FREQSTEP, EditSizeValue );

SaveInterval        = new TRadioButton ( this, IDC_SAVEINTERVAL );
SaveBands           = new TRadioButton ( this, IDC_SAVEBANDS );
SaveFreqMin         = new TEdit ( this, IDC_SAVEFREQMIN, EditSizeValue );
SaveFreqMax         = new TEdit ( this, IDC_SAVEFREQMAX, EditSizeValue );
SaveFreqStep        = new TEdit ( this, IDC_SAVEFREQSTEP, EditSizeValue );
SaveLogInterval     = new TCheckBox ( this, IDC_SAVELOGINTERVAL );
SaveDecadeStep      = new TEdit ( this, IDC_DECADESTEP, EditSizeValue );
SaveBandsValue      = new TEdit ( this, IDC_SAVEBANDSVALUE, EditSizeText );

SamplingFrequency->SetValidator ( new TFilterValidator ( ValidatorPositiveFloat     ) );
SaveFreqMin      ->SetValidator ( new TFilterValidator ( ValidatorPositiveFloat     ) );
SaveFreqMax      ->SetValidator ( new TFilterValidator ( ValidatorPositiveFloat     ) );
SaveFreqStep     ->SetValidator ( new TFilterValidator ( ValidatorPositiveFloat     ) );
SaveBandsValue   ->SetValidator ( new TFilterValidator ( ValidatorPositiveFloatList ) );

Presets             = new TComboBox ( this, IDC_PRESETS );

Fft                 = new TRadioButton ( this, IDC_SIMPLEFFT );
FftApproximation    = new TRadioButton ( this, IDC_FFTAPPROXIMATION );
STransform          = new TRadioButton ( this, IDC_STRANSFORM );

WindowingNone       = new TRadioButton ( this, IDC_WINDOWINGNONE );
WindowingHanning    = new TRadioButton ( this, IDC_WINDOWINGHANNING );

Mean                = new TRadioButton ( this, IDC_MEAN );
Sequence            = new TRadioButton ( this, IDC_SEQUENCE );

FFTNormalization    = new TComboBox ( this, IDC_FFTNORMALIZATION );

WriteType           = new TComboBox ( this, IDC_WRITETYPE );

NoRef               = new TRadioButton ( this, IDC_NOREF );
CurrentRef          = new TRadioButton ( this, IDC_CURRENTREF );
AvgRef              = new TRadioButton ( this, IDC_AVEREF );
OtherRef            = new TRadioButton ( this, IDC_OTHERREF );
RefList             = new TEdit ( this, IDC_REFLIST, EditSizeText );

InfixFilename       = new TEdit ( this, IDC_INFIXFILENAME, EditSizeText );
FileTypeFreq        = new TRadioButton ( this, IDC_FILETYPE_FREQ );
CreateSubdir        = new TCheckBox ( this, IDC_CREATESUBDIR );
OpenAuto            = new TCheckBox ( this, IDC_OPENAUTO );

SplitByElectrode    = new TRadioButton ( this, IDC_SPLITBYELECTRODE );
SplitByFrequency    = new TRadioButton ( this, IDC_SPLITBYFREQUENCY );
SplitBySpectrum     = new TRadioButton ( this, IDC_SPLITBYSPECTRUM );

OptimalDownsampling = new TCheckBox ( this, IDC_OPTIMALDOWNSAMPLING );

                                        // are we entering batch?
BatchProcessing     = doc == 0;


SetTransferBuffer ( dynamic_cast <TFrequencyAnalysisStruct*> ( &FrequencyAnalysisTransfer ) );


static bool     init    = true;

PreviousPreset  = -1;

if ( init ) {

    init            = false;

    CurrentPreset   = FreqPresetDefault;

    FrequencyAnalysisTransfer.EndOfFile         = BoolToCheck ( EEGDoc == 0 );
    }
else
                                        // restore previous preset
    CurrentPreset   = GetIndex ( FrequencyAnalysisTransfer.Presets );
}


//----------------------------------------------------------------------------
void    TFrequencyAnalysisDialog::SetupWindow ()
{
TBaseDialog::SetupWindow ();

                                        // update dialog from transfer buffer
TransferData ( tdSetData );


double              sf              = EEGDoc ? EEGDoc->GetSamplingFrequency () : 0;
int                 blocksize       = 0;
double              fstep           = 0;
double              savefstep       = GetDouble ( SaveFreqStep );

                                        // keep any existing blocksize when entering batch mode
if ( ! BatchProcessing && sf ) {

    if ( StringToInteger ( FrequencyAnalysisTransfer.BlockSize ) == 0 ) {

        blocksize   = GetDefaultBlocksize ();

        if ( blocksize )    SetInteger  ( BlockSize, blocksize );
        else                ClearText   ( BlockSize );
        }
    else
        blocksize   = StringToInteger ( FrequencyAnalysisTransfer.BlockSize );
    }

                                        // keep any existing frequencies when entering batch mode
if ( ! BatchProcessing && sf && blocksize ) {
                                        // reset frequency limits
    double  nyquist     = GetNyquist ( sf, IsSTransform ( CurrentPreset ) );
            fstep       = sf / blocksize;

    SetDouble   ( SamplingFrequency, sf );
    SetInteger  ( FreqMin, 0 );
    SetDouble   ( FreqMax,  nyquist );
    SetDouble   ( FreqStep, fstep );

                                        // set this only if found blank
    if ( StringIsEmpty ( FrequencyAnalysisTransfer.SaveFreqMin ) )  // only reading existing transfer buffer
        SetDouble ( SaveFreqMin, DefaultSaveFreqMin );

    if ( StringIsEmpty ( FrequencyAnalysisTransfer.SaveFreqMax ) )  // only reading existing transfer buffer
        SetDouble ( SaveFreqMax, NoMore ( DefaultSaveFreqMax, nyquist ) );

    if ( ! IsSTransform ( CurrentPreset ) )
        CopyText    ( SaveFreqStep, FreqStep );
    }
//else {
//    ClearText ( SamplingFrequency );
//    ClearText ( FreqMin );
//    ClearText ( FreqMax );
//    ClearText ( FreqStep );
//    ClearText ( SaveFreqMin );
//    ClearText ( SaveFreqMax );
//    }

                                        // update everything, especially in batch mode
CmTimeLimitsChange ();

                                        // some overrides, done on each call
SetCheck ( OpenAuto,     ! BatchProcessing );
//SetCheck ( CreateSubdir, ! BatchProcessing );

                                        // done here, as CmTimeLimitsChange changes many values
if ( savefstep )
    SetDouble ( SaveFreqStep, max ( savefstep, fstep ) );   // max with fstep, in case we were coming from wider blocks and lower steps

                                        // finally update transfer buffer from dialog
TransferData ( tdGetData );


if ( ! BatchProcessing )                // if a file was provided in ProcessCurrent
    CmXyzChange ();
}


//----------------------------------------------------------------------------
        TFrequencyAnalysisDialog::~TFrequencyAnalysisDialog ()
{
delete  Channels;           delete  UseXyzDoc;          delete  XyzDocFile;
delete  TimeMin;            delete  TimeMax;            delete  ClippedTimeMax;     delete  EndOfFile;
delete  NumBlocks;          delete  BlockSize;
delete  WindowOverlap0;     delete  WindowOverlap75;    delete  WindowOverlapMax;
delete  WindowsInterval;
delete  SkipBadEpochs;      delete  SkipMarkers;
delete  SamplingFrequency;
delete  FreqMin;            delete  FreqMax;            delete  FreqStep;
delete  SaveInterval;       delete  SaveBands;
delete  SaveFreqMin;        delete  SaveFreqMax;        delete  SaveFreqStep;       
delete  SaveLogInterval;    delete  SaveDecadeStep;
delete  SaveBandsValue;
delete  Presets;
delete  Fft;                delete  FftApproximation;
delete  STransform;
delete  WindowingNone;      delete  WindowingHanning;
delete  Mean;               delete  Sequence;
delete  FFTNormalization;
delete  WriteType;
delete  NoRef;              delete  CurrentRef;         delete  AvgRef;
delete  OtherRef;           delete  RefList;
delete  InfixFilename;
delete  FileTypeFreq;       delete  CreateSubdir;       delete  OpenAuto;
delete  SplitByElectrode;   delete  SplitByFrequency;   delete  SplitBySpectrum;
delete  OptimalDownsampling;
}


//----------------------------------------------------------------------------
int     TFrequencyAnalysisDialog::GetDefaultBlocksize ()
{
int                 blocksize;

                                        // some override, on each call
if ( IsSTransform ( CurrentPreset ) ) {

    if ( IsChecked ( EndOfFile ) )

        blocksize = 0;

    else {
        int                 timemin         = GetInteger ( TimeMin );
        int                 timemax         = GetInteger ( TimeMax );

//      double              sf              = GetDouble ( SamplingFrequency );

//      blocksize   = ( (int) ( ( timemax - timemin + 1 ) / ( sf ? sf : 1 ) ) ) * sf;

        blocksize   = timemax - timemin + 1;

        if ( blocksize < 0 ) blocksize = 0;
        }
    }
else

    if ( BatchProcessing ) {

        ResetCheck  ( OpenAuto );

//      SetInteger  ( TimeMin, 0 );
//      ClearText   ( TimeMax );
//      SetCheck    ( EndOfFile, EEGDoc == 0 );

    //  blocksize   = MaxInitBlockSize;

        double          sf          = GetDouble ( SamplingFrequency );

        if ( sf )   blocksize   = Clip ( Round ( SecondsToTimeFrame ( 2, sf ) ),    MinBlockSize, MaxInitBlockSize );
        else        blocksize   = Clip ( GetInteger ( BlockSize ),                  MinBlockSize, MaxInitBlockSize );
        }
    else {
                                            // propose a good window size, to have a freq res of 1 Hz, and time res of 1 s
//      blocksize   = max ( min ( EEGDoc->GetSamplingFrequency () ? Power2Rounded  ( EEGDoc->GetSamplingFrequency () ) : MaxInitBlockSize,
//                                Power2Truncated ( EEGDoc->GetNumTimeFrames ()     )  ),
//                           MinBlockSize );
                                                                           // set to a default of 0.5 Hz resolution
//      blocksize   = max ( min ( (int) ( ( EEGDoc->GetSamplingFrequency () ? 2 * EEGDoc->GetSamplingFrequency () : MaxInitBlockSize ) + 0.5 ),
//                                (int) EEGDoc->GetNumTimeFrames () ),
//                          MinBlockSize );

                                        // try to smartly find a multiple of sampling frequency as the initial block size
                                        // - big files should have a 0.5 Hz resolution
                                        // - smaller files should decrease block size to get more blocks, though increasing resolution
        for ( double multiple = EEGDoc->IsErp () ? 1 : 2; multiple >= 0.25; multiple /= 2 ) {

            blocksize   = NoMore ( (int) EEGDoc->GetNumTimeFrames (),
                                   Round ( EEGDoc->GetSamplingFrequency () ? multiple * EEGDoc->GetSamplingFrequency () : MaxInitBlockSize ) );
            if ( blocksize <= EEGDoc->GetNumTimeFrames () )
                break;
            }

        Clipped ( blocksize, MinBlockSize, (int) EEGDoc->GetNumTimeFrames () );
        }


return blocksize;
}


//----------------------------------------------------------------------------
void    TFrequencyAnalysisDialog::EvPresetsChange ()
{
if ( StringIsEmpty ( FreqPresetsString[ GetIndex ( Presets ) ] ) )
    return;


PreviousPreset  = CurrentPreset;
CurrentPreset   = GetIndex ( Presets );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Taking care not to overwrite user's choice of the FFT Normalization
static int      CurrentFFTNormalization     = DefaultFFTRescaling;

                                        // Saving current state when not in S-Transform
if      ( ! IsSTransform ( PreviousPreset ) && ! IsSTransform ( CurrentPreset ) )
    CurrentFFTNormalization     = GetIndex ( FFTNormalization );

                                        // Quitting S-Transform settings, restore saved state
else if (   IsSTransform ( PreviousPreset ) && ! IsSTransform ( CurrentPreset ) )
    SetIndex    ( FFTNormalization, CurrentFFTNormalization );

                                        // Entering S-Transform, save old state then reset it
else if ( ! IsSTransform ( PreviousPreset ) &&   IsSTransform ( CurrentPreset ) ) {
    CurrentFFTNormalization     = GetIndex ( FFTNormalization );
    SetIndex    ( FFTNormalization, FFTRescalingNone );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//CmSetAnalysis ( owlwparam w );

                                        // reset all options
ResetCheck  ( WindowOverlap0   );
ResetCheck  ( WindowOverlap75  );
ResetCheck  ( WindowOverlapMax );

ResetCheck  ( Fft              );
ResetCheck  ( FftApproximation );
ResetCheck  ( STransform       );

ResetCheck  ( Mean             );
SetCheck    ( Sequence         );

//if ( ! IsGeneralCaseAnalysis ( CurrentPreset ) ) {
    ResetCheck  ( NoRef        );
    ResetCheck  ( CurrentRef   );
    ResetCheck  ( AvgRef       );
    ResetCheck  ( OtherRef     );
//    }

                                        // then set only the right ones
switch ( CurrentPreset ) {

    case    FreqPresetSurfacePowermaps :
        SetCheck    ( Fft              );
        SetCheck    ( AvgRef           );
        SetCheck    ( WindowOverlap75  );
        SetIndex    ( WriteType, OutputAtomNorm2 );
        break;

    case    FreqPresetSurfacePowermapsAvg :
        SetCheck    ( Fft              );
        SetCheck    ( AvgRef           );
        SetCheck    ( WindowOverlap75  );
        SetIndex    ( WriteType, OutputAtomNorm2 );
                                       
        SetCheck    ( Mean             );
        ResetCheck  ( Sequence         );
        break;

    case    FreqPresetSurfacePowermapsSt:
        SetCheck    ( Fft              );
        SetCheck    ( AvgRef           );
        SetCheck    ( WindowOverlapMax );
        SetIndex    ( WriteType, OutputAtomNorm2 );
        break;


    case    FreqPresetSurfaceFftapprox:
        SetCheck    ( FftApproximation );
        SetCheck    ( AvgRef           );
        SetCheck    ( WindowOverlap75  );
        SetIndex    ( WriteType, OutputAtomReal );
        break;

    case    FreqPresetSurfaceFftapproxSt:
        SetCheck    ( FftApproximation );
        SetCheck    ( AvgRef           );
        SetCheck    ( WindowOverlapMax );
        SetIndex    ( WriteType, OutputAtomReal );
        break;


    case    FreqPresetSurfaceStransf:
        SetCheck    ( STransform       );
        SetCheck    ( AvgRef           );
        SetCheck    ( WindowOverlap0   );
        SetIndex    ( WriteType, OutputAtomNorm2 );
        break;

    case    FreqPresetSurfaceStransfForESI:
        SetCheck    ( STransform       );
        SetCheck    ( AvgRef           );
        SetCheck    ( WindowOverlap0   );
        SetIndex    ( WriteType, OutputAtomComplex );
        break;


    case    FreqPresetIntraFft:
    case    FreqPresetFft:
        SetCheck    ( Fft              );
        SetCheck    ( NoRef            );
        SetCheck    ( WindowOverlap75  );
        SetIndex    ( WriteType, OutputAtomNorm2 );
        break;

    case    FreqPresetIntraFftAvg:
    case    FreqPresetFftAvg:
        SetCheck    ( Fft              );
        SetCheck    ( NoRef            );
        SetCheck    ( WindowOverlap75  );
        SetIndex    ( WriteType, OutputAtomNorm2 );

        SetCheck    ( Mean             );
        ResetCheck  ( Sequence         );
        break;

    case    FreqPresetIntraFftSt:
    case    FreqPresetFftSt:
        SetCheck    ( Fft              );
        SetCheck    ( NoRef            );
        SetCheck    ( WindowOverlapMax );
        SetIndex    ( WriteType, OutputAtomNorm2 );
        break;


    case    FreqPresetIntraStransf:
    case    FreqPresetIntraStransfPhase:
    case    FreqPresetStransf:
        SetCheck    ( STransform       );
        SetCheck    ( NoRef            );
        SetCheck    ( WindowOverlap0   );

        if ( CurrentPreset == FreqPresetIntraStransfPhase ) {
            SetCheck    ( SaveInterval     );
            ResetCheck  ( SaveBands        );

            SetIndex    ( WriteType, OutputAtomPhase );
            }
        else {
            SetIndex    ( WriteType, OutputAtomNorm2 );
            }
        break;
    }


UpdateFreqMinMaxStep ();

                                        // will also handle the windowing & sequence (off for STransform)
CmWindowOverlapChange ();
}


//----------------------------------------------------------------------------
void    TFrequencyAnalysisDialog::EvDropFiles ( TDropInfo drop )
{
TGoF                tracksfiles     ( drop, BatchFilesExt           );
TGoF                xyzfiles        ( drop, AllCoordinatesFilesExt  );

char                buff[ 256 ];
StringCopy ( buff, BatchFilesExt, " " AllCoordinatesFilesExt );
TGoF                remainingfiles  ( drop, buff, 0, true );

drop.DragFinish ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) xyzfiles && FrequencyAnalysisTransfer.XyzLinked )

    ShowMessage (   "You can not change the coordinates file now,"      NewLine 
                    "as it is already provided by the current link."    NewLine 
                    NewLine 
                    "Close the link file first if you wish to use another coordinates file.", 
                    FrequencyAnalysisTitle, ShowMessageWarning );


if ( (bool) xyzfiles && ! FrequencyAnalysisTransfer.XyzLinked ) {

    for ( int i = 0; i < (int) xyzfiles; i++ ) {

        SetCheck    ( UseXyzDoc );
        XyzDocFile->SetText ( xyzfiles[ i ] );

//        FrequencyAnalysisTransfer.UseXyzDoc  = BoolToCheck ( true  );
//        StringCopy ( FrequencyAnalysisTransfer.XyzDocFile, xyzfiles[ i ] );
//        TransferData ( tdSetData );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) tracksfiles && ! BatchProcessing )

    ShowMessage ( BatchNotAvailMessage, FrequencyAnalysisTitle, ShowMessageWarning );


if ( (bool) tracksfiles && BatchProcessing )
    BatchProcessDropped ( tracksfiles );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) remainingfiles )
    remainingfiles.Show ( IrrelevantErrorMessage );
}


//----------------------------------------------------------------------------
//void    TFrequencyAnalysisDialog::CmChannelsEnable ( TCommandEnabler &tce )
//{
//tce.Enable ( ! IsSurfaceAnalysis ( CurrentPreset ) );
//}


void    TFrequencyAnalysisDialog::CmBrowseXyzDoc ()
{
static GetFileFromUser  getfile ( "Electrodes Coordinates File", AllCoordinatesFilesFilter, 1, GetFileRead );


if ( ! getfile.Execute ( FrequencyAnalysisTransfer.XyzDocFile ) )
    return;


TransferData ( tdSetData );             // this will activate CmXyzChange (but not SetText ( .. ) )

XyzDocFile->ResetCaret;
}


void    TFrequencyAnalysisDialog::CmXyzEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! FrequencyAnalysisTransfer.XyzLinked );
}


void    TFrequencyAnalysisDialog::CmUseXyzDocEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsChecked ( UseXyzDoc ) && ! FrequencyAnalysisTransfer.XyzLinked );
}


void    TFrequencyAnalysisDialog::CmXyzChange ()
{
                                        // ProcessCurrent may have given us some selection, so use it now that we now the XYZ
TFrequencyAnalysisStructEx  &transfer       = FrequencyAnalysisTransfer;


if ( ! BatchProcessing && transfer.ChannelsSel ) {
    TFileName           filexyz;
    char                buff    [ EditSizeTextLong ];
    TElectrodesDoc*     DocXyz;


    if ( IsChecked ( UseXyzDoc ) )
        XyzDocFile->GetText ( filexyz, EditSizeText );
    else
        ClearString ( filexyz );


    if ( ! CanOpenFile ( filexyz ) ) {
        transfer.ChannelsSel->ToText ( buff, EEGDoc->GetElectrodesNames(), AuxiliaryTracksNames );
        Channels->SetText ( buff );

        return;
        }


    DocXyz = dynamic_cast< TElectrodesDoc* > ( CartoolDocManager->OpenDoc ( filexyz, dtOpenOptionsNoView ) );


    transfer.ChannelsSel->ToText ( buff, DocXyz->GetElectrodesNames(), AuxiliaryTracksNames );
    Channels->SetText ( buff );


    if ( DocXyz->CanClose ( true ) )
        CartoolDocManager->CloseDoc ( DocXyz );
    }
}


//----------------------------------------------------------------------------
void    TFrequencyAnalysisDialog::CmTimeMaxEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! IsChecked ( EndOfFile ) );
}


void    TFrequencyAnalysisDialog::CmBlockSizeEnable ( TCommandEnabler &tce )
{
//tce.Enable ( ! IsChecked ( EndOfFile ) && ! IsSTransform ( CurrentPreset ) );
tce.Enable ( ! IsSTransform ( CurrentPreset ) );
}


void    TFrequencyAnalysisDialog::CmOtherRefEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( IsChecked ( OtherRef )
             && (    IsRegularFFT ( CurrentPreset ) 
                  || IsSTransform ( CurrentPreset ) ) );
}


void    TFrequencyAnalysisDialog::CmSamplingFrequencyEnable ( TCommandEnabler &tce )
{
//SamplingFrequency->SetReadOnly ( ! ( EEGDoc && EEGDoc->GetSamplingFrequency () > 0 ) );
tce.Enable ( ! ( EEGDoc && EEGDoc->GetSamplingFrequency () > 0 ) );
}


void    TFrequencyAnalysisDialog::CmWindowOverlapEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! IsSTransform ( CurrentPreset ) );
}


void    TFrequencyAnalysisDialog::CmSkipBadEpochsEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsChecked ( SkipBadEpochs ) );
}


//----------------------------------------------------------------------------
bool    TFrequencyAnalysisDialog::IsValidTime ()
{
int                 timemin         = GetInteger ( TimeMin );
int                 timemax         = GetInteger ( TimeMax );
int                 timenum         = timemax - timemin + 1;
int                 numblocks       = GetInteger ( NumBlocks );
int                 blocksize       = GetInteger ( BlockSize );


return  ( IsChecked ( EndOfFile )        || ( numblocks > 0 && timenum > 0 ) )
     && ( IsSTransform ( CurrentPreset ) || blocksize >= MinBlockSize /*&& IsPower2 ( blocksize )*/ );
}


bool    TFrequencyAnalysisDialog::IsValidFreq ()
{
double              sf              = GetDouble ( SamplingFrequency );

if ( sf <= 0 )                                                          return false;


if (    IsChecked ( SaveBands ) 
     && IsSpace ( SaveBandsValue ) )                                    return false;


double              fmin            = GetDouble ( FreqMin );
double              fmax            = GetDouble ( FreqMax );

                                        // these might not exist, set them manually
if ( IsSTransform ( CurrentPreset ) && IsChecked ( EndOfFile ) ) {
    fmin    = 0;
    fmax    = GetNyquist ( sf, IsSTransform ( CurrentPreset ) );
    }


double              savefmin        = IsSpace ( SaveFreqMin ) ? -1 : GetDouble ( SaveFreqMin );
double              savefmax        = IsSpace ( SaveFreqMax ) ? -1 : GetDouble ( SaveFreqMax );

if (    IsChecked ( SaveInterval ) 
     && ( ! IsInsideLimits ( savefmin, savefmax, fmin, fmax )
         || savefmin > savefmax )                       )               return false;


double              fstep           = GetDouble ( FreqStep );
double              savefstep       = GetDouble ( SaveFreqStep );

if ( savefstep < fstep )                                                return false;


return  true;
}


bool    TFrequencyAnalysisDialog::IsProcessEnable ()
{
if ( ! IsValidTime () )                                                 return false;

if ( ! IsValidFreq () )                                                 return false;

if ( IsSpace ( Channels ) )                                             return false;

if ( IsChecked ( UseXyzDoc ) && IsSpace ( XyzDocFile ) )                return false;

if ( IsChecked ( OtherRef  ) && IsSpace ( RefList ) )                   return false;

if ( IsChecked ( SaveBands ) && IsSpace ( SaveBandsValue ) )            return false;

if ( ! (   IsChecked ( FileTypeFreq     ) 
        || IsChecked ( SplitByElectrode ) 
        || IsChecked ( SplitByFrequency ) 
        || IsChecked ( SplitBySpectrum  ) ) )                           return false;


return  true;
}


void    TFrequencyAnalysisDialog::CmProcessCurrentEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! BatchProcessing && IsProcessEnable () );
}


void    TFrequencyAnalysisDialog::CmProcessBatchEnable ( TCommandEnabler &tce )
{
tce.Enable (   BatchProcessing && IsProcessEnable () );
}


//----------------------------------------------------------------------------
void    TFrequencyAnalysisDialog::CmMarkersEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsChecked ( Sequence ) );
}


void    TFrequencyAnalysisDialog::CmIntervalEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsChecked ( SaveInterval ) );
}


void    TFrequencyAnalysisDialog::CmIntervalStepEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsChecked ( SaveInterval ) && ! IsChecked ( SaveLogInterval ) );
}


void    TFrequencyAnalysisDialog::CmDecadeStepEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsChecked ( SaveInterval ) && IsChecked ( SaveLogInterval ) );
}


void    TFrequencyAnalysisDialog::CmBandsEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsChecked ( SaveBands ) );
}


void    TFrequencyAnalysisDialog::CmSequenceMeanEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! IsSTransform ( CurrentPreset ) );
}


void    TFrequencyAnalysisDialog::CmSplitEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsChecked ( FileTypeFreq ) );
}


void    TFrequencyAnalysisDialog::CmOptimalDownsamplingEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsSTransform ( CurrentPreset ) );
}


//----------------------------------------------------------------------------
double  TFrequencyAnalysisDialog::GetWindowOverlap ( int blocksize )
{
if ( IsChecked ( WindowOverlap0   ) )                   return  0.00;
if ( IsChecked ( WindowOverlap75  ) )                   return  0.75;
if ( IsChecked ( WindowOverlapMax ) )                   return  blocksize ? (double) ( blocksize - 1 ) / blocksize : 0;
                                                        return  DefaultWindowOverlap; // to return the correct default when called from init
}


int     ComputeNumBlocks    ( bool isstransform, bool blockstep1tf, int timespan, int blocksize, double blocksoverlap )
{
if      ( isstransform  )   return  timespan >= blocksize ? 1 : 0;
else if ( blockstep1tf  )   return  timespan >= blocksize ? timespan - ( blocksize - 1 ) : 0;   // avoid rounding error
else                        return  timespan >= blocksize ? (int) ( ( (double) timespan / blocksize - blocksoverlap ) / ( 1 - blocksoverlap ) ) : 0;
}


int     TFrequencyAnalysisDialog::ComputeNumBlocks ( int timespan, int blocksize, double blocksoverlap )
{
return crtl::ComputeNumBlocks ( IsSTransform ( CurrentPreset ), IsChecked ( WindowOverlapMax ), timespan, blocksize, blocksoverlap );
}


int     ComputeTimeMax ( bool isstransform, bool blockstep1tf, int timemin, int blocksize, double blocksoverlap, int numblocks )
{
if      ( isstransform  )   return  timemin + blocksize - ( blocksize ? 1 : 0 );
else if ( blockstep1tf  )   return  timemin + blocksize + ( numblocks - 1 ) - 1;
else                        return  timemin + blocksize * ( blocksoverlap + ( 1 - blocksoverlap ) * numblocks ) - 1;
}


int     TFrequencyAnalysisDialog::ComputeTimeMax ( int timemin, int blocksize, double blocksoverlap, int numblocks )
{
return  crtl::ComputeTimeMax ( IsSTransform ( CurrentPreset ), IsChecked ( WindowOverlapMax ), timemin, blocksize, blocksoverlap, numblocks );
}
                                        // For the FFT, it could be efficient to round the block size - we don't want odd/prime numbers block size
                                        // As a first version, we try to truncate to either some multiples of 128 (efficient), or 100 (user friendly, still efficient)
int     RoundFFTBlockSize ( int blocksize )
{
int                 bs1             = TruncateTo ( blocksize, 128 );
int                 bs2             = TruncateTo ( blocksize, 100 );
                                    // pick the one closer to the original interval
return  abs ( (double) blocksize - bs1 ) < abs ( (double) blocksize - bs2 ) ? bs1 : bs2;
}


double  TFrequencyAnalysisDialog::ComputeWindowsInterval ( double sf, int blocksize, double blocksoverlap )
{
if      ( IsSTransform ( CurrentPreset ) || ! sf )      return  0;
else if ( IsChecked ( WindowOverlapMax ) )              return  TimeFrameToMilliseconds ( 1, sf );      // avoid rounding error
else                                                    return  TimeFrameToMilliseconds ( Truncate ( blocksize * ( 1 - blocksoverlap ) ), sf );
}


int     ComputeStep ( bool isstransform, int blocksize, double blocksoverlap )
{
if      ( isstransform )    return  0;
//else if ( blockstep1tf )    return  1;
else                        return  AtLeast ( 1, Truncate ( blocksize * ( 1 - blocksoverlap ) ) );  // should also return 1 for WindowOverlapMax / blockstep1tf
}


int     TFrequencyAnalysisDialog::ComputeStep ( int blocksize, double blocksoverlap )
{
return  crtl::ComputeStep ( IsSTransform ( CurrentPreset ), blocksize, blocksoverlap );
}


//----------------------------------------------------------------------------
/*
double      RoundLog ( double v )
{
double      lr          = Log10 ( v );
int         lrint       = Truncate ( lr );
double      lrfrac      = Fraction ( lr );
double      normfrac    = Power10 ( lrfrac );

//double      d00         = lrfrac == 0 ? 0 : 1;
double      d00         = fabs ( normfrac - 0.00 );
double      d10         = fabs ( normfrac - 0.10 );
double      d20         = fabs ( normfrac - 0.20 );
double      d50         = fabs ( normfrac - 0.50 );
double      round;

if      ( d00 < d10 )   round   = 1.00;
else if ( d10 < d20 )   round   = 0.10;
else if ( d20 < d50 )   round   = 0.20;
else                    round   = 0.50;

//double      result  = Power10 ( lrint ) / ( lrfrac > 0.5 ? 1 : 2 );
//double      result  = Power10 ( lrint );
double      result  = Power10 ( TruncateTo ( lrfrac, round ) + lrint );

return result;
}
*/


void    TFrequencyAnalysisDialog::CmTimeLimitsChange ()
{
int                 timemin             = GetInteger ( TimeMin );
int                 timemax             = GetInteger ( TimeMax );
int                 timenum;
int                 numblocks;
int                 blocksize;
double              overlap;
static int          savedblocksize      = 0;

                                        // do some checking
if ( timemin < 0 || ( EEGDoc && timemin >= EEGDoc->GetNumTimeFrames () ) ) {

    timemin = 0;

    SetInteger ( TimeMin, timemin );
    TimeMin->ResetCaret;
//    return;
    }

if ( timemax < 0 || ( EEGDoc && timemax >= EEGDoc->GetNumTimeFrames () ) ) {

    timemax = EEGDoc ? EEGDoc->GetNumTimeFrames () - 1 : timemin;

    SetInteger ( TimeMax, timemax );
    TimeMax->ResetCaret;
//    return;
    }


timenum     = AtLeast ( 0, timemax - timemin + 1 );

//DBGV3 ( timemin, timemax, timenum, "timemin, timemax, timenum" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // force update the BlockSize if we change of processing time (FFT <-> S-Transform)
                                        // we have to do this quite early

                                        // be convenient to the user: save the current block size, which might not be the default anymore
if ( ! IsSTransform ( PreviousPreset )  )

    savedblocksize  = GetInteger ( BlockSize );

                                        // transitionning to a S-Transform?
if ( IsSTransform ( CurrentPreset ) && ! IsSTransform ( PreviousPreset ) ) {

    timenum     = RoundFFTBlockSize ( timenum );

    SetInteger ( BlockSize, timenum );

    if ( timenum <= 1 )
        ClippedTimeMax->SetText ( IsChecked ( EndOfFile ) ? "<File End>" : "<Invalid>" );

//  DBGV4 ( timemin, timemax, timenum, sf, "timemin, timemax, timenum, sf - ST" );
    }

                                        // quitting a S-Transform, or simply needing to refresh the number of blocks
else if ( /*IsSTransform ( PreviousPreset ) &&*/ ! IsSTransform ( CurrentPreset ) ) {
                                        // restore the saved block size, which the user might be happy to see back
    SetInteger ( BlockSize, savedblocksize );
    }


blocksize   = GetInteger ( BlockSize );
overlap     = GetWindowOverlap ( blocksize );

//DBGV ( blocksize, "blocksize" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // init block size?
if ( blocksize <= 0 )
    if ( IsChecked ( EndOfFile ) ) {
        SetInteger ( BlockSize, 256 ); // set a default
        return;
        }
    else {
        blocksize = Power2Rounded ( timenum );
        if ( blocksize < MinBlockSize )
            blocksize = MinBlockSize;

        SetInteger ( BlockSize, blocksize );
        }
*/
                                        // params uncheckables?
if ( blocksize <= 0 || IsChecked ( EndOfFile ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

numblocks   = ComputeNumBlocks ( timenum, blocksize, overlap );


//DBGV ( numblocks, "numblocks" );

/*                                      // adjust for the user to a new block size
if ( numblocks == 0 ) {
    if ( ! IsPower2 ( timenum ) ) {     // not power of 2?
        blocksize = Power2Rounded ( timenum);
        if ( blocksize < MinBlockSize )
            blocksize = MinBlockSize;

        numblocks   = timenum / blocksize;

        SetInteger ( BlockSize, blocksize );
        }
    }
*/

                                        // updates
if ( numblocks ) {
//  timemax = timemin + numblocks * blocksize - 1;
    timemax = ComputeTimeMax ( timemin, blocksize, overlap, numblocks );

    SetInteger ( ClippedTimeMax, timemax );
//    DBGV ( timemax, "timemax  set 1" );
    }
else {
//  timemax = 0;
    ClippedTimeMax->SetText ( IsChecked ( EndOfFile ) ? "<File End>" : "<Invalid>" );
    }


//SetInteger ( TimeMin, timemin );       // not needed to set, and it will generate a call to this procedure!
TimeMin->ResetCaret;
SetInteger ( NumBlocks, numblocks );
NumBlocks->ResetCaret;

UpdateFreqMinMaxStep ();

//DBGV2 ( numblocks, timemax, "TimeLimits -> NumBlocks, TimeMax" );
}


//----------------------------------------------------------------------------
void    TFrequencyAnalysisDialog::CmNumBlocksChange ()
{
int                 timemin         = GetInteger ( TimeMin );
int                 timemax         = GetInteger ( TimeMax );
int                 timenum         = AtLeast ( 0,  timemax - timemin + 1 );
int                 numblocks       = GetInteger ( NumBlocks );
int                 blocksize       = GetInteger ( BlockSize );
double              overlap         = GetWindowOverlap ( blocksize );

                                        // params uncheckables?
if (    timenum <= 0 
     || blocksize < MinBlockSize /*|| ! IsPower2 ( blocksize )*/
     || numblocks <= 0 
     || IsChecked ( EndOfFile ) ) {
//if ( ! IsValidTime () ) {

    ClippedTimeMax->SetText ( IsChecked ( EndOfFile ) ? "<File End>" : "<Invalid>" );

    if ( numblocks != 0 )               // be cautious, otherwise it will loop for ever within this procedure!
        NumBlocks->SetText ( "" );

    if ( IsSTransform ( CurrentPreset ) ) {
        BlockSize->SetText ( "" );
        ClippedTimeMax->SetText ( IsChecked ( EndOfFile ) ? "<File End>" : "<Invalid>" );
        }

    UpdateFreqMinMaxStep ();
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 maxblocks       = ComputeNumBlocks ( timenum, blocksize, overlap );


//if ( numblocks > maxblocks )
    numblocks = maxblocks;

                                        // updates
if ( numblocks ) {
    timemax = ComputeTimeMax ( timemin, blocksize, overlap, numblocks );
    SetInteger ( ClippedTimeMax, timemax );
    }
else {
//  timemax = 0;
    ClippedTimeMax->SetText ( IsChecked ( EndOfFile ) ? "<File End>" : "<Invalid>" );
    }


SetInteger ( NumBlocks, numblocks );
NumBlocks->ResetCaret;

UpdateFreqMinMaxStep ();

//DBGV2 ( maxblocks, timemax, "NumBlocks -> numblocks, TimeMax" );
}


//----------------------------------------------------------------------------
void    TFrequencyAnalysisDialog::CmBlockSizeChange ()
{
int                 timemin         = GetInteger ( TimeMin );
int                 timemax         = GetInteger ( TimeMax );
int                 timenum         = AtLeast ( 0,  timemax - timemin + 1 );
int                 numblocks;
int                 blocksize       = GetInteger ( BlockSize );
double              overlap         = GetWindowOverlap ( blocksize );
double              sf              = GetDouble ( SamplingFrequency );

                                        // here we can update this, before an early exit
double              winint          = ComputeWindowsInterval ( sf, blocksize, overlap );

if ( winint )
    SetDouble   ( WindowsInterval, winint );
else
    ClearText   ( WindowsInterval );


                                        // params uncheckables?
if ( timenum <= 0 || blocksize < MinBlockSize /*|| !IsPower2 ( blocksize )*/ || IsChecked ( EndOfFile ) ) {

    ClearText   ( NumBlocks );
    UpdateFreqMinMaxStep ();
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

numblocks   = ComputeNumBlocks ( timenum, blocksize, overlap );

                                        // updates
if ( numblocks ) {
    timemax = ComputeTimeMax ( timemin, blocksize, overlap, numblocks );

    if ( timenum <= 0 )
        ClippedTimeMax->SetText ( IsChecked ( EndOfFile ) ? "<File End>" : "<Invalid>" );
    else
        SetInteger ( ClippedTimeMax, timemax );
    }
else {
//  timemax = 0;
    ClippedTimeMax->SetText ( IsChecked ( EndOfFile ) ? "<File End>" : "<Invalid>" );
    }


SetInteger ( NumBlocks, numblocks );
NumBlocks->ResetCaret;

UpdateFreqMinMaxStep ();

//DBGV2 ( numblocks, timemax, "BlocksSize -> NumBlocks, TimeMax" );
}


//----------------------------------------------------------------------------
void    TFrequencyAnalysisDialog::CmSamplingFrequencyChange ()
{
int                 blocksize       = GetInteger ( BlockSize );
double              overlap         = GetWindowOverlap ( blocksize );
double              sf              = GetDouble ( SamplingFrequency );

                                        // params uncheckables?
if ( sf <= 0 || blocksize <= 0 ) {
    ClearText   ( FreqMin );
    ClearText   ( FreqMax );
    ClearText   ( FreqStep );
//  ClearText   ( SaveFreqMin );
//  ClearText   ( SaveFreqMax );
    ClearText   ( WindowsInterval );
    return;
    }


double              winint          = ComputeWindowsInterval ( sf, blocksize, overlap );

if ( winint )
    SetDouble   ( WindowsInterval, winint );
else
    ClearText   ( WindowsInterval );

UpdateFreqMinMaxStep ();
}


//----------------------------------------------------------------------------
/*
void    TFrequencyAnalysisDialog::CmSaveFrequenciesChange ()
{
double              savefmin        = GetDouble ( SaveFreqMin );
double              savefmax        = GetDouble ( SaveFreqMax );
double              sf              = GetDouble ( SamplingFrequency );

                                        // params uncheckables?
if ( sf <= 0 )
    return;

double              fmax            = GetNyquist ( sf, IsSTransform ( CurrentPreset ) );

if ( savefmin < 0 || savefmin > fmax )
    savefmin = 0;

if ( savefmax < 0 || savefmax > fmax )
    savefmax = fmax;

SetDouble ( SaveFreqMin, savefmin );
SetDouble ( SaveFreqMax, savefmax );
}
*/

//----------------------------------------------------------------------------
void    TFrequencyAnalysisDialog::UpdateFreqMinMaxStep ()
{
int                 blocksize       = GetInteger ( BlockSize );
double              savefmax        = GetDouble ( SaveFreqMax );
double              savefstep; //   = GetDouble ( SaveFreqStep );
double              sf              = GetDouble ( SamplingFrequency );


                                        // do some checking
if ( sf <= 0 || blocksize < MinBlockSize /*|| !IsPower2 ( blocksize )*/ ) {
//if ( ! IsValidTime () ) {
    ClearText   ( FreqMin );
    ClearText   ( FreqMax );
    ClearText   ( FreqStep );
//  ClearText   ( SaveFreqMin );
//  ClearText   ( SaveFreqMax );
    return;
    }


double              fmin            = 0;
double              fmax            = GetNyquist ( sf, IsSTransform ( CurrentPreset ) );
double              fstep           = sf / ( blocksize ? blocksize : 1 );

                                        // force clipping to the new boundaries (especially in the case of S-Transform)
Clipped ( savefmax, fmin, fmax );

                                        // don't allow too little steps (especially in the case of S-Transform)
savefstep   = AtLeast ( IsSTransform ( CurrentPreset ) ? 0.5 : 0.25, fstep );   // when not retrieving SaveFreqStep
//Maxed ( savefstep, fstep );                                                   // when retrieving SaveFreqStep
    

                                        // update dialog
SetDouble ( FreqMin     , fmin      );
SetDouble ( FreqMax     , fmax      );
SetDouble ( FreqStep    , fstep     );
SetDouble ( SaveFreqMax , savefmax  );
SetDouble ( SaveFreqStep, savefstep );
}


//----------------------------------------------------------------------------
                                        // update things when toggling
void    TFrequencyAnalysisDialog::CmEndOfFile ()
{
CmTimeLimitsChange ();
CmBlockSizeChange  ();
CmNumBlocksChange  ();
}


//----------------------------------------------------------------------------
void    TFrequencyAnalysisDialog::SetDefaultWindowing ()
{                                       // good options
ResetCheck  ( WindowOverlap0   );
SetCheck    ( WindowOverlap75  );
ResetCheck  ( WindowOverlapMax );
 
ResetCheck  ( WindowingNone    );
SetCheck    ( WindowingHanning );
                                        // update window calculations
CmTimeLimitsChange ();
}


void    TFrequencyAnalysisDialog::CmWindowOverlapChange ()
{
                                        // no window overlap sets no windowing
if      ( IsChecked ( WindowOverlap0 ) && ! IsSTransform ( CurrentPreset ) ) {

    ResetCheck  ( WindowingHanning );
    SetCheck    ( WindowingNone    );
    }
else if ( ! IsChecked ( WindowingHanning ) ) {

    ResetCheck  ( WindowingNone    );
    SetCheck    ( WindowingHanning );
    }


int                 blocksize       = GetInteger ( BlockSize );
double              overlap         = GetWindowOverlap ( blocksize );
double              sf              = GetDouble ( SamplingFrequency );


double              winint          = ComputeWindowsInterval ( sf, blocksize, overlap );

if ( winint )
    SetDouble   ( WindowsInterval, winint );
else
    ClearText   ( WindowsInterval );

                                        // update window calculations
CmTimeLimitsChange ();
}


//----------------------------------------------------------------------------
                                        // set/reset correct parameters - Makes use of transfer buffer
void    TFrequencyAnalysisDialog::CmSetAnalysis ()
{
TransferData ( tdGetData );

TFrequencyAnalysisStructEx  &transfer   = FrequencyAnalysisTransfer;

                                        // set reference
if ( IsPowerMaps        ( CurrentPreset ) 
  || IsFFTApproximation ( CurrentPreset ) ) {
    transfer.NoRef      = BoolToCheck ( false );
    transfer.CurrentRef = BoolToCheck ( false );
    transfer.OtherRef   = BoolToCheck ( false );
    transfer.AvgRef     = BoolToCheck ( true  );
    }
/*else if ( IsRegularFFT ( CurrentPreset ) 
         || IsSTransform ( CurrentPreset ) ) {
    transfer.CurrentRef = BoolToCheck ( false );
    transfer.OtherRef   = BoolToCheck ( false );
    transfer.AvgRef     = BoolToCheck ( false );
    transfer.NoRef      = BoolToCheck ( true  );
    }*/

                                        // set output format
if      ( IsRegularFFT ( CurrentPreset ) && IsIndex ( transfer.WriteType, OutputAtomReal )
       || IsPowerMaps  ( CurrentPreset ) 
       || IsSTransform ( CurrentPreset ) ) {

    SetIndex ( transfer.WriteType, OutputAtomNorm2 );
    }

else if ( IsFFTApproximation ( CurrentPreset ) ) {

    SetIndex ( transfer.WriteType, OutputAtomReal );
    }

                                        // other settings
if ( IsSTransform ( CurrentPreset ) ) {
    transfer.Mean               = BoolToCheck ( false );
    transfer.Sequence           = BoolToCheck ( true  );

    transfer.WindowOverlap75    = BoolToCheck ( false );
    transfer.WindowOverlapMax   = BoolToCheck ( false );
    transfer.WindowOverlap0     = BoolToCheck ( true  );

//  transfer.WindowingHanning   = BoolToCheck ( false );
//  transfer.WindowingNone      = BoolToCheck ( true  );
    }

                                        // update block size
if ( ! IsSTransform ( CurrentPreset ) && ! StringToInteger ( transfer.BlockSize ) )
    IntegerToString ( transfer.BlockSize, GetDefaultBlocksize () );


TransferData ( tdSetData );


//UpdateFreqMinMaxStep ();  // that's quite annoying

                                        // set windowing
if ( IsPowerMaps        ( CurrentPreset ) 
  || IsFFTApproximation ( CurrentPreset ) ) {
    if ( CheckToBool ( transfer.WindowOverlap0 ) && CheckToBool ( transfer.WindowingHanning ) )
        SetDefaultWindowing ();
    }

                                        // especially for STransform, resetting the block size -> possible freqs
CmEndOfFile ();
}


//----------------------------------------------------------------------------
                                        // Single method to assess the consistency between analysis and output type
                                        // Used for Averaging blocks, Frequency Bands and Output type preset changes
void    TFrequencyAnalysisDialog::AnalysisAtomtypeCompatibility ()
{
FreqAnalysisType    analysis    = IsRegularFFT       ( CurrentPreset )  ? FreqAnalysisFFT
                                : IsPowerMaps        ( CurrentPreset )  ? FreqAnalysisPowerMaps
                                : IsFFTApproximation ( CurrentPreset )  ? FreqAnalysisFFTApproximation
                                : IsSTransform       ( CurrentPreset )  ? FreqAnalysisSTransform
                                :                                         (FreqAnalysisType) -1;

const char*         aac         = crtl::AnalysisAtomtypeCompatibility ( analysis, 
                                                                        IsChecked ( SaveBands ), 
                                                                        IsChecked ( Mean      ), 
                                                                        (FreqOutputAtomType) GetIndex ( WriteType ) );

if ( StringIsNotEmpty ( aac ) ) {

    ShowMessage ( aac, FrequencyAnalysisTitle, ShowMessageWarning );
                                        // easiest on the user: resetting to default of current preset
    EvPresetsChange ();
    }
}


void    TFrequencyAnalysisDialog::CmWriteTypeEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! ( IsFFTApproximation ( CurrentPreset ) 
              || CurrentPreset == FreqPresetSurfaceStransfForESI
              || CurrentPreset == FreqPresetIntraStransfPhase    ) );
}


//----------------------------------------------------------------------------
void    TFrequencyAnalysisDialog::CmFFTNormalizationEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! IsSTransform ( CurrentPreset ) );
}


//----------------------------------------------------------------------------
void    TFrequencyAnalysisDialog::CmReferenceEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsGeneralCaseAnalysis ( CurrentPreset ) );
}

                                        // user can't push analysis buttons anymore, everything is done throuh the Presets
void    TFrequencyAnalysisDialog::CmAnalysisTypeEnable ( TCommandEnabler &tce )
{
//tce.Enable ( IsGeneralCaseAnalysis ( CurrentPreset ) );
tce.Enable ( false );
}


//----------------------------------------------------------------------------
void    TFrequencyAnalysisDialog::ProcessCurrent ( void* usetransfer, const char* /*moreinfix*/ )
{
if ( ! EEGDoc )
    return;


TFrequencyAnalysisStructEx*     transfer    = usetransfer ? (TFrequencyAnalysisStructEx*) usetransfer : &FrequencyAnalysisTransfer;

if ( ! transfer )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                openxyz             =  CheckToBool ( transfer->UseXyzDoc ) 
                                        && CanOpenFile ( transfer->XyzDocFile );
const char*         xyzfile             = openxyz ? transfer->XyzDocFile : 0;


TOpenDoc<TElectrodesDoc>    XYZDoc;

bool                isxyzdocalreadyopen = openxyz && CartoolObjects.CartoolDocManager->IsOpen ( xyzfile );
                                        // see if a coordinates file has been provided
if ( openxyz )
                                        // open in advance if needed
    XYZDoc.Open ( xyzfile, OpenDocHidden );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // the actual job of sorting the analysis type is already done in these functions
FreqAnalysisType    analysis            = IsRegularFFT       ( CurrentPreset )  ? FreqAnalysisFFT
                                        : IsPowerMaps        ( CurrentPreset )  ? FreqAnalysisPowerMaps
                                        : IsFFTApproximation ( CurrentPreset )  ? FreqAnalysisFFTApproximation
                                        : IsSTransform       ( CurrentPreset )  ? FreqAnalysisSTransform
                                        :                                         (FreqAnalysisType) -1;

if ( analysis == (FreqAnalysisType) -1 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // choose the right reference
ReferenceType       ref;

AtomType            datatypein          = EEGDoc->GetAtomType ( AtomTypeUseOriginal );

TSelection          refsel ( EEGDoc->GetTotalElectrodes (), OrderSorted );     // electrodes for specified reference
char                buff[ EditSizeTextLong ];

if      ( CheckToBool ( transfer->NoRef      ) )    ref = ReferenceAsInFile;
else if ( CheckToBool ( transfer->AvgRef     ) )    ref = ReferenceAverage;
else if ( CheckToBool ( transfer->CurrentRef ) )    ref = ReferenceUsingCurrent;
else if ( CheckToBool ( transfer->OtherRef   ) ) {

    refsel.Reset ();

    refsel.Set    ( transfer->RefList, XYZDoc.IsOpen () ? XYZDoc->GetElectrodesNames() : EEGDoc->GetElectrodesNames() );

                                        // that should not be...
    if ( refsel.NumSet () == 0 ) {

        ref     = ReferenceAsInFile;    // either setting to no reference, as is currently done in  TTracksDoc::SetReferenceType
                                        
        //CmCancel ();                  // aborting now?
        //return;
        }
    else {

        ref     = ReferenceArbitraryTracks;
                                        // list clean-up
        refsel.ToText ( transfer->RefList, XYZDoc.IsOpen () ? XYZDoc->GetElectrodesNames() : EEGDoc->GetElectrodesNames(), AuxiliaryTracksNames );

        TransferData ( tdSetData );
        }
    }
else                                                ref = ReferenceAsInFile;


CheckReference ( ref, datatypein );

                                        // rare case, user asked for average reference on 1 track...
if ( EEGDoc->GetNumElectrodes () == 1 && ref == ReferenceAverage )
    ref     = ReferenceAsInFile;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // cooking the time interval, number and block sizes
double              samplingfrequency   = StringToDouble    ( transfer->SamplingFrequency );

long                timemin             = StringToInteger   ( transfer->TimeMin );

bool                endoffile           = CheckToBool       ( transfer->EndOfFile );

long                timemax             = endoffile ? EEGDoc->GetNumTimeFrames () - 1               // use real, full length when using EOF option 
                                                    : StringToInteger ( transfer->ClippedTimeMax ); // using OPTIMIZED time instead of TimeMax, which will make the FFT faster
long                timenum             = timemax - timemin + 1;

                                        // if S-Transform, block size is the full time window
int                 blocksizedialog     = StringToInteger   ( transfer->BlockSize );

int                 blocksize           = analysis == FreqAnalysisSTransform ? timenum
                                                                             : blocksizedialog;
double              blocksoverlap       = GetWindowOverlap  ( blocksize );

//int               numblocks           = ComputeNumBlocks  ( timenum, blocksize, blocksoverlap );
                                        // final timemax update?
//                  timemax             = ComputeTimeMax    ( timemin, blocksize, blocksoverlap, numblocks );

                                        // because we have real input, signal can be processed faster and holding less memory
int                 freqsize            = blocksize / 2 + 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SkippingEpochsType  badepochs       = CheckToBool ( transfer->SkipBadEpochs     ) ? SkippingBadEpochsList
                                    :                                               NoSkippingBadEpochs;


TFixedString<EditSizeText>  listbadepochs;

if ( badepochs == SkippingBadEpochsList ) {

    listbadepochs   = transfer->SkipMarkers;
    listbadepochs.CleanUp ();

    if ( listbadepochs.IsEmpty () )
        badepochs   = NoSkippingBadEpochs;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

FreqOutputBands     outputbands         = CheckToBool ( transfer->SaveInterval     ) ? CheckToBool ( transfer->SaveLogInterval ) ? OutputLogInterval 
                                                                                                                                 : OutputLinearInterval
                                        : CheckToBool ( transfer->SaveBands        ) ?                                             OutputBands
                                        :                                                                                          (FreqOutputBands) -1;

if ( outputbands == (FreqOutputBands) -1 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

FFTRescalingType    fftnorm             = (FFTRescalingType)    GetIndex ( FFTNormalization );

FreqOutputAtomType  outputatomtype      = (FreqOutputAtomType)  GetIndex ( WriteType );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                outputsequential    = CheckToBool ( transfer->Sequence ) 
                                       || analysis == FreqAnalysisSTransform;

FreqWindowingType   windowing           = transfer->WindowingHanning ? analysis != FreqAnalysisSTransform ? FreqWindowingHanning 
                                                                                                          : FreqWindowingHanningBorder 
                                                                     :                                      FreqWindowingNone;

bool                outputmarkers       = outputsequential;

TTracksView*        eegview             = dynamic_cast<TTracksView*> ( EEGDoc->GetViewList () );
MarkerType          outputmarkerstype   = eegview ? eegview->GetMarkerType () : AllMarkerTypes;     // refining to current view's marker type


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              outputfreqmin       = StringToDouble ( transfer->SaveFreqMin    );
double              outputfreqmax       = StringToDouble ( transfer->SaveFreqMax    );
double              outputfreqstep      = StringToDouble ( transfer->SaveFreqStep   );     
int                 outputdecadestep    = StringToInteger( transfer->SaveDecadeStep );

CheckOrder ( outputfreqmin, outputfreqmax );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              datafreqstep_hz     = samplingfrequency / (double) blocksize; 
int                 limitfreqmax_i      = freqsize - 1;                             // = blocksize / 2 - was:  GetNyquist ( blocksize, analysis == FreqAnalysisSTransform )
double              limitfreqmax_hz     = limitfreqmax_i * datafreqstep_hz;
double              limitfreqmin_i      = 1;
double              limitfreqmin_hz     = limitfreqmin_i * datafreqstep_hz;

                                        // we need this value for some sanity checks
double              savedfreqmax        = 0;


if      ( outputbands == OutputBands ) {

    TSplitStrings   bandlist ( transfer->SaveBandsValue, UniqueStrings );
    int             numfreqbands        = (int) bandlist;
    double          fminhz;
    double          fmaxhz;

    for ( int fbi = 0; fbi < numfreqbands; fbi++ ) {

        sscanf ( bandlist[ fbi ], "%lf-%lf", &fminhz, &fmaxhz );
                                        // this is a safe over-estimate, as the resulting band will have a mean frequency lower than that
        Maxed ( savedfreqmax, fmaxhz );
        } // for freqband

    } // OutputBands

else if ( outputbands == OutputLogInterval ) {
                                        // due to log things, we don't want no stinking 0Hz in here
    double          fmaxhz          = outputfreqmax;

    Clipped ( fmaxhz, limitfreqmin_hz, limitfreqmax_hz );

                                        // explicit number of division within a decade ( log1 to log10, log10 to log100 etc...)
    int             stepsperdecade  = AtLeast ( 1, outputdecadestep );

    double          decaderes       = AtLeast ( Log10 ( AtLeast ( FreqMinLog10Value, datafreqstep_hz ) ), 1 / (double) stepsperdecade );

                                        // or extrapolate the given linear step to a decade resolution
//  double          decaderes       = AtLeast    ( Log10 ( AtLeast ( FreqMinLog10Value, datafreqstep_hz ) ), outputfreqstep / ( 10 - 1 ) );

    double          logfmaxhz       = TruncateTo ( Log10 ( AtLeast ( FreqMinLog10Value, fmaxhz ) ), decaderes );
                                        // double check the truncation in log didn't introduce some inconsistencies
    Clipped ( logfmaxhz, Log10 ( AtLeast ( FreqMinLog10Value, limitfreqmin_hz ) ), Log10 ( AtLeast ( FreqMinLog10Value, limitfreqmax_hz ) ) );

    savedfreqmax    = Power10 ( logfmaxhz );
    } // OutputLogInterval

else if ( outputbands == OutputLinearInterval ) {

    savedfreqmax        = outputfreqmax;
    } // OutputLinearInterval


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                optimaldownsampling = CheckToBool ( transfer->OptimalDownsampling )
                                       && analysis == FreqAnalysisSTransform                                // S-Transform is very smooth, and we don't need the extra time resolution (?)
                                     //&& ( IsStFFT ( CurrentPreset ) || IsSTransform ( CurrentPreset ) )   // StFFT case needs more work, i.e. changing the block step & number of time frames...
                                       && savedfreqmax > 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                createsubdir        = CheckToBool ( transfer->CreateSubdir     );

bool                savefreq            = CheckToBool ( transfer->FileTypeFreq );
bool                splitelectrode      = savefreq && CheckToBool ( transfer->SplitByElectrode ) && ( outputatomtype != OutputAtomComplex );
bool                splitfrequency      = savefreq && CheckToBool ( transfer->SplitByFrequency ) && ( outputatomtype != OutputAtomComplex );
bool                splitspectrum       = savefreq && CheckToBool ( transfer->SplitBySpectrum  ) && ( outputatomtype != OutputAtomComplex );

bool                savefftapprox       = analysis == FreqAnalysisFFTApproximation;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // silencing in these cases
VerboseType         verbosey            = NumBatchFiles () > 1 ? Silent : Interactive;


if ( IsBatchFirstCall () && BatchFileNames.NumFiles () > 1 )
//if ( BatchProcessing && IsBatchFirstCall () )

    WindowMinimize ( CartoolMainWindow );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           fileoutfreq;
TFileName           fileoutsplitelec;
TFileName           fileoutsplitfreq;
TFileName           fileoutspectrum;
TFileName           fileoutapprfreqs;


FrequencyAnalysis   (   EEGDoc,
                        xyzfile,
                        analysis,
                        transfer->Channels,
                        ref,                transfer->RefList,
                        timemin,            timemax,
                        badepochs,          listbadepochs,
                        samplingfrequency,
                        blocksize,          blocksoverlap,
                        fftnorm,
                        outputbands,
                        outputatomtype,
                        outputmarkers,      outputmarkerstype,
                        transfer->SaveBandsValue,
                        outputfreqmin,      outputfreqmax,      outputfreqstep,     
                        outputdecadestep,
                        outputsequential,
                        windowing,
                        optimaldownsampling,
                        transfer->InfixFilename,    createsubdir,
                        savefreq       ? (char*) fileoutfreq        : (char*) 0,
                        splitelectrode ? (char*) fileoutsplitelec   : (char*) 0,
                        splitfrequency ? (char*) fileoutsplitfreq   : (char*) 0,
                        splitspectrum  ? (char*) fileoutspectrum    : (char*) 0,
                        savefftapprox  ? (char*) fileoutapprfreqs   : (char*) 0,
                        verbosey
                    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // closing XYZDoc?
if ( XYZDoc.IsOpen () && XYZDoc->CanClose ( true ) )

    XYZDoc.Close ( isxyzdocalreadyopen || transfer->XyzLinked || BatchProcessing ? CloseDocLetOpen : CloseDocRestoreAsBefore );

                                        // complimentary open the file for the user
bool                openauto            = CheckToBool ( transfer->OpenAuto ) 
                                        && (int) BatchFileNames <= MaxFilesToOpen;

if ( openauto ) {

    if ( savefreq )
        fileoutfreq    .Open ();

    if ( splitspectrum && ! outputsequential )
        fileoutspectrum.Open ();

    if ( splitspectrum && analysis == FreqAnalysisFFTApproximation && ! outputsequential )
        fileoutapprfreqs.Open ();
    }


//if ( ! BatchProcessing || IsBatchLastCall () ) {
if ( IsBatchLastCall () && BatchFileNames.NumFiles () > 1 ) {

  //WindowMaximize ( CartoolMainWindow );
    WindowRestore  ( CartoolMainWindow );

    //CartoolApplication->ResetMainTitle ();
    CartoolApplication->SetMainTitle ( "Frequency Analysis of ", ToFileName ( EEGDoc->GetDocPath () ) );
    }


UpdateApplication;
}


//----------------------------------------------------------------------------
void    TFrequencyAnalysisDialog::BatchProcess ()
{
//TransferData ( tdGetData );
                                        // Changing priority
SetProcessPriority ( BatchProcessingPriority );


if ( (int) BatchFileNames > MaxFilesToOpen )
    FrequencyAnalysisTransfer.OpenAuto  = BoolToCheck ( false );


TBaseDialog::BatchProcess ();


SetProcessPriority ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
