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

#include    <complex>

#include    "System.h"
#include    "TBaseDialog.h"

#include    "FrequencyAnalysis.h"       // FFTRescalingType, FreqOutputAtomType

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum        FreqPresetsEnum
            {
            FreqPresetSurfacePowermaps,
            FreqPresetSurfacePowermapsAvg,
            FreqPresetSurfacePowermapsSt,
            FreqPresetSurfaceFftapprox,
            FreqPresetSurfaceFftapproxSt,
            FreqPresetSurfaceStransf,
            FreqPresetSurfaceStransfForESI,

            FreqPresetSeparator1,

            FreqPresetIntraFft,
            FreqPresetIntraFftAvg,
            FreqPresetIntraFftSt,
            FreqPresetIntraStransf,
            FreqPresetIntraStransfPhase,

            FreqPresetSeparator2,

            FreqPresetFft,
            FreqPresetFftAvg,
            FreqPresetFftSt,
            FreqPresetStransf,

            NumFreqPresets,

            FreqPresetDefault               = FreqPresetSurfacePowermaps,
            };


extern const char   FreqPresetsString[ NumFreqPresets ][ 128 ];


inline  bool    IsSurfaceAnalysis       ( int preset )      
{
return  IsInsideLimits ( preset, (int) FreqPresetSurfacePowermaps, (int) FreqPresetSurfaceStransfForESI );
}


inline  bool    IsIntraCranialAnalysis  ( int preset )      
{
return  IsInsideLimits ( preset, (int) FreqPresetIntraFft, (int) FreqPresetIntraStransfPhase );
}


inline  bool    IsGeneralCaseAnalysis   ( int preset )      
{
return  IsInsideLimits ( preset, (int) FreqPresetFft, (int) FreqPresetStransf );
}

                                        // All FFT family, including Power Maps & FFT Approximation
inline  bool    IsAnyTypeFFT    ( int preset )
{
return  IsInsideLimits ( preset, (int) FreqPresetSurfacePowermaps, (int) FreqPresetSurfaceFftapproxSt )
     || IsInsideLimits ( preset, (int) FreqPresetIntraFft,         (int) FreqPresetIntraFftSt )
     || IsInsideLimits ( preset, (int) FreqPresetFft,              (int) FreqPresetFftSt );
}

                                        // Only Power-Maps FFT
inline  bool    IsPowerMaps     ( int preset )
{
return  IsInsideLimits ( preset, (int) FreqPresetSurfacePowermaps, (int) FreqPresetSurfacePowermapsSt );
}

                                        // Only Frequency Approximation FFT
inline  bool    IsFFTApproximation  ( int preset )
{
return  IsInsideLimits ( preset, (int) FreqPresetSurfaceFftapprox, (int) FreqPresetSurfaceFftapproxSt );
}

                                        // Other FFTs than Power Maps or FFT Approx
inline  bool    IsRegularFFT    ( int preset )
{
return  IsInsideLimits ( preset, (int) FreqPresetIntraFft, (int) FreqPresetIntraFftSt )
     || IsInsideLimits ( preset, (int) FreqPresetFft,      (int) FreqPresetFftSt      );
}

                                        // Only Short-Term FFT
inline  bool    IsStFFT         ( int preset )
{
return  preset  == FreqPresetSurfacePowermapsSt
     || preset  == FreqPresetSurfaceFftapproxSt
     || preset  == FreqPresetIntraFftSt
     || preset  == FreqPresetFftSt;
}

                                        // Only S-Transform
inline  bool    IsSTransform    ( int preset )
{
return  preset  == FreqPresetSurfaceStransf
     || preset  == FreqPresetSurfaceStransfForESI
     || preset  == FreqPresetIntraStransf
     || preset  == FreqPresetIntraStransfPhase
     || preset  == FreqPresetStransf;
}


//----------------------------------------------------------------------------
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


class   TFrequencyAnalysisStruct
{
public:
                        TFrequencyAnalysisStruct ();


    TEditData           Channels        [ EditSizeTextLong ];
    TCheckBoxData       UseXyzDoc;
    TEditData           XyzDocFile      [ EditSizeText ];

    TEditData           TimeMin         [ EditSizeValue ];
    TEditData           TimeMax         [ EditSizeValue ];
    TEditData           ClippedTimeMax  [ EditSizeValue ];
    TCheckBoxData       EndOfFile;
    TEditData           NumBlocks       [ EditSizeValue ];
    TEditData           BlockSize       [ EditSizeValue ];
    TRadioButtonData    WindowOverlap0;
    TRadioButtonData    WindowOverlap75;
    TRadioButtonData    WindowOverlapMax;
    TEditData           WindowsInterval [ EditSizeValue ];
    TCheckBoxData       SkipBadEpochs;
    TEditData           SkipMarkers     [ EditSizeText ];

    TEditData           SamplingFrequency   [ EditSizeValue ];
    TEditData           FreqMin             [ EditSizeValue ];
    TEditData           FreqMax             [ EditSizeValue ];
    TEditData           FreqStep            [ EditSizeValue ];

    TRadioButtonData    SaveInterval;
    TRadioButtonData    SaveBands;
    TEditData           SaveFreqMin     [ EditSizeValue ];
    TEditData           SaveFreqMax     [ EditSizeValue ];
    TEditData           SaveFreqStep    [ EditSizeValue ];
    TCheckBoxData       SaveLogInterval;
    TEditData           SaveDecadeStep  [ EditSizeValue ];
    TEditData           SaveBandsValue  [ EditSizeText ];

    TComboBoxData       Presets;            // Presets pilot the Analysis Type, not the analysis type

    TRadioButtonData    Fft;
    TRadioButtonData    FftApproximation;
    TRadioButtonData    STransform;

    TRadioButtonData    WindowingNone;
    TRadioButtonData    WindowingHanning;

    TRadioButtonData    Mean;
    TRadioButtonData    Sequence;

    TComboBoxData       FFTNormalization;

    TComboBoxData       WriteType;

    TRadioButtonData    NoRef;
    TRadioButtonData    CurrentRef;
    TRadioButtonData    AvgRef;
    TRadioButtonData    OtherRef;
    TEditData           RefList         [ EditSizeText ];

    TEditData           InfixFilename   [ EditSizeText ];
    TCheckBoxData       FileTypeFreq;
    TCheckBoxData       CreateSubdir;
    TCheckBoxData       OpenAuto;

    TCheckBoxData       SplitByElectrode;
    TCheckBoxData       SplitByFrequency;
    TCheckBoxData       SplitBySpectrum;

    TCheckBoxData       OptimalDownsampling;
    };


class   TFrequencyAnalysisStructEx  :   public  TFrequencyAnalysisStruct
{
public:
                        TFrequencyAnalysisStructEx ();


    TSelection*         ChannelsSel;        // from process current
    bool                XyzLinked;          // from lm
    };


EndBytePacking

//----------------------------------------------------------------------------

constexpr FFTRescalingType      DefaultFFTRescaling     = FFTRescalingParseval;
constexpr FreqOutputAtomType    DefaultFreqOutputAtom   = OutputAtomNorm2;

constexpr int                   MinBlockSize            =    4;
constexpr int                   MaxInitBlockSize        = 2000;
constexpr double                DefaultWindowOverlap    = 0.75;
constexpr double                DefaultSaveFreqMin      = 1; // 0 or 1
constexpr double                DefaultSaveFreqMax      = 40;
constexpr int                   DefaultSaveLogDecade    = 20;


int                 ComputeTimeMax  ( bool isstransform, bool blockstep1tf, int timemin, int blocksize, double blocksoverlap, int numblocks );
int                 ComputeNumBlocks( bool isstransform, bool blockstep1tf, int timespan, int blocksize, double blocksoverlap );
int                 ComputeStep     ( bool isstransform, int blocksize, double blocksoverlap );


//----------------------------------------------------------------------------

class   TTracksDoc;


class   TFrequencyAnalysisDialog    :   public  TBaseDialog
{
public:
                        TFrequencyAnalysisDialog ( owl::TWindow* parent, owl::TResId resId, TTracksDoc* doc );
                       ~TFrequencyAnalysisDialog ();

protected:
    owl::TEdit          *Channels;
    owl::TCheckBox      *UseXyzDoc;
    owl::TEdit          *XyzDocFile;

    owl::TEdit          *TimeMin;
    owl::TEdit          *TimeMax;
    owl::TEdit          *ClippedTimeMax;
    owl::TCheckBox      *EndOfFile;
    owl::TEdit          *NumBlocks;
    owl::TEdit          *BlockSize;
    owl::TRadioButton   *WindowOverlap0;
    owl::TRadioButton   *WindowOverlap75;
    owl::TRadioButton   *WindowOverlapMax;
    owl::TEdit          *WindowsInterval;
    owl::TCheckBox      *SkipBadEpochs;
    owl::TEdit          *SkipMarkers;

    owl::TEdit          *SamplingFrequency;
    owl::TEdit          *FreqMin;
    owl::TEdit          *FreqMax;
    owl::TEdit          *FreqStep;

    owl::TRadioButton   *SaveInterval;
    owl::TRadioButton   *SaveBands;
    owl::TEdit          *SaveFreqMin;
    owl::TEdit          *SaveFreqMax;
    owl::TEdit          *SaveFreqStep;
    owl::TCheckBox      *SaveLogInterval;
    owl::TEdit          *SaveDecadeStep;
    owl::TEdit          *SaveBandsValue;

    owl::TComboBox      *Presets;

    owl::TRadioButton   *Fft;
    owl::TRadioButton   *FftApproximation;
    owl::TRadioButton   *STransform;

    owl::TRadioButton   *WindowingNone;
    owl::TRadioButton   *WindowingHanning;

    owl::TRadioButton   *Mean;
    owl::TRadioButton   *Sequence;

    owl::TComboBox      *FFTNormalization;

    owl::TComboBox      *WriteType;

    owl::TRadioButton   *NoRef;
    owl::TRadioButton   *CurrentRef;
    owl::TRadioButton   *AvgRef;
    owl::TRadioButton   *OtherRef;
    owl::TEdit          *RefList;

    owl::TEdit          *InfixFilename;
    owl::TCheckBox      *FileTypeFreq;
    owl::TCheckBox      *CreateSubdir;
    owl::TCheckBox      *OpenAuto;

    owl::TCheckBox      *SplitByElectrode;
    owl::TCheckBox      *SplitByFrequency;
    owl::TCheckBox      *SplitBySpectrum;

    owl::TCheckBox      *OptimalDownsampling;

                                        // for a smoother UI, we need to know how the user transitions from/to another processing - maybe put this in the struct, or a saved copy?
    int                 PreviousPreset;
    int                 CurrentPreset;


    void                BatchProcess                    ()                                                      final;
    void                ProcessCurrent                  ( void *usetransfer = 0, const char *moreinfix = 0 )    final;


    void                SetupWindow                     ()                                                      final;
    void                EvDropFiles                     ( owl::TDropInfo drop );

    int                 GetDefaultBlocksize             ();
    void                UpdateFreqMinMaxStep            ();
    void                SetDefaultWindowing             ();

    double              GetWindowOverlap                ( int blocksize );
    int                 ComputeNumBlocks                ( int timespan, int blocksize, double overlap );
    int                 ComputeTimeMax                  ( int timemin, int blocksize, double overlap, int numblocks );
    double              ComputeWindowsInterval          ( double sf, int blocksize, double overlap );
    int                 ComputeStep                     ( int blocksize, double overlap );


    void                EvPresetsChange                 ();
//  void                CmChannelsEnable                ( owl::TCommandEnabler &tce );
    void                CmBrowseXyzDoc                  ();
    void                CmTimeMaxEnable                 ( owl::TCommandEnabler &tce );
    void                CmOtherRefEnable                ( owl::TCommandEnabler &tce );

    bool                IsValidTime                     ();
    bool                IsValidFreq                     ();
    bool                IsProcessEnable                 ();
    void                CmProcessCurrentEnable          ( owl::TCommandEnabler &tce );
    void                CmProcessBatchEnable            ( owl::TCommandEnabler &tce );

    void                CmSamplingFrequencyEnable       ( owl::TCommandEnabler &tce );
    void                CmTimeLimitsChange              ();
    void                CmNumBlocksChange               ();
    void                CmBlockSizeChange               ();
    void                CmSamplingFrequencyChange       ();
//  void                CmSaveFrequenciesChange         ();
    void                CmEndOfFile                     ();
    void                CmSetAnalysis                   ();
    void                AnalysisAtomtypeCompatibility                     ();
    void                CmFFTNormalizationEnable        ( owl::TCommandEnabler &tce );
    void                CmWriteTypeEnable               ( owl::TCommandEnabler &tce );
    void                CmReferenceEnable               ( owl::TCommandEnabler &tce );
    void                CmWindowOverlapChange           ();
    void                CmMarkersEnable                 ( owl::TCommandEnabler &tce );
    void                CmBlockSizeEnable               ( owl::TCommandEnabler &tce );
    void                CmWindowOverlapEnable           ( owl::TCommandEnabler &tce );
    void                CmSkipBadEpochsEnable           ( owl::TCommandEnabler &tce );
    void                CmIntervalEnable                ( owl::TCommandEnabler &tce );
    void                CmIntervalStepEnable            ( owl::TCommandEnabler &tce );
    void                CmDecadeStepEnable              ( owl::TCommandEnabler &tce );
    void                CmBandsEnable                   ( owl::TCommandEnabler &tce );
    void                CmSequenceMeanEnable            ( owl::TCommandEnabler &tce );
    void                CmXyzChange                     ();
    void                CmUseXyzDocEnable               ( owl::TCommandEnabler &tce );
    void                CmXyzEnable                     ( owl::TCommandEnabler &tce );
    void                CmAnalysisTypeEnable            ( owl::TCommandEnabler &tce );
    void                CmSplitEnable                   ( owl::TCommandEnabler &tce );
    void                CmOptimalDownsamplingEnable     ( owl::TCommandEnabler &tce );

    DECLARE_RESPONSE_TABLE ( TFrequencyAnalysisDialog );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
