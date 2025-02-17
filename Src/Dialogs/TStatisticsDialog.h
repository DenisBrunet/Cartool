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

#include    "System.h"
#include    "TCartoolApp.h"
#include    "Files.TGoF.h"
#include    "TBaseDialog.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // All implemented tests
enum        TestTypesEnum
            {      
            tTest                   = 0x001,
            RandomizationTest       = 0x002,
            TAnovaTest              = 0x004,

            ParametricTest          = 0x010,
            NonParametricTest       = 0x020,

            UnpairedTest            = 0x100,
            PairedTest              = 0x200,

                                        // The actual implemented tests:
            UnpairedTTest           = UnpairedTest | ParametricTest    | tTest,
            UnpairedRandomization   = UnpairedTest | NonParametricTest | RandomizationTest,
            UnpairedTAnova          = UnpairedTest | NonParametricTest | TAnovaTest,
            PairedTTest             = PairedTest   | ParametricTest    | tTest,
            PairedRandomization     = PairedTest   | NonParametricTest | RandomizationTest,
            PairedTAnova            = PairedTest   | NonParametricTest | TAnovaTest,
            };

constexpr int   NumStatisticalTests = 6;


inline bool IsUnpaired          ( TestTypesEnum t )     { return  IsFlag ( t, UnpairedTest      ); }
inline bool IsPaired            ( TestTypesEnum t )     { return  IsFlag ( t, PairedTest        ); }

inline bool IsParametric        ( TestTypesEnum t )     { return  IsFlag ( t, ParametricTest    ); }
inline bool IsNonParametric     ( TestTypesEnum t )     { return  IsFlag ( t, NonParametricTest ); }

inline bool IsTTest             ( TestTypesEnum t )     { return  IsFlag ( t, tTest             ); }
inline bool IsRandomization     ( TestTypesEnum t )     { return  IsFlag ( t, RandomizationTest ); }
inline bool IsTAnova            ( TestTypesEnum t )     { return  IsFlag ( t, TAnovaTest        ); }

inline bool HasMoreOutputs      ( TestTypesEnum t )     { return  IsTTest ( t ) || IsRandomization ( t ); }


//----------------------------------------------------------------------------
                                        // Presets for Data Types
enum        StatPresetsData
            {
            StatPresetEegSurfaceErp,
            StatPresetEegSurfaceSpont,
            StatPresetEegIntraErp,
            StatPresetEegIntraSpont,
            StatPresetSeparator1,

            StatPresetMegSurfaceErp,
            StatPresetMegSurfaceSpont,
            StatPresetSeparator2,

//          StatPresetRisVectErp,
            StatPresetRisScalErp,
//          StatPresetRisVectSpont,
            StatPresetRisScalSpont,
            StatPresetSeparator3,

            StatPresetFFTPower,
            StatPresetFFTApprox,
            StatPresetSeparator4,

            StatPresetFitting,
            StatPresetMarkov,
            StatPresetSeparator5,

            StatPresetPos,

            NumStatPresets,

            StatPresetDataDefault       = StatPresetFitting,
            };


extern const char   StatPresetsDataString[ NumStatPresets ][ 64 ];


                                        // Presets for Correction
enum        StatPresetsCorrection
            {
            StatCorrPresetNone,
            StatCorrPresetBonferroni,
            StatCorrPresetThresholdingPValues,
//          StatCorrPresetCorrecting,   // better use the Adjusted q-values
            StatCorrPresetAdjusting,

            NumStatCorrPresets,

            StatCorrDefault     = StatCorrPresetAdjusting,
            };


extern const char   StatPresetsCorrectionString[ NumStatCorrPresets ][ 64 ];


//----------------------------------------------------------------------------
                                        // Presets for Time preprocessing
enum        StatTimeType
            {
                                        // EEG stands for signed data, so it could be MEG Magnetometer too...
            StatTimeSequential,
            StatTimeMean,
            StatTimeMedian,
            StatTimeMin,
            StatTimeMax,

            NumStatTimes,

            StatTimeDefault     = StatTimeSequential,
            };

inline bool         IsTimeSequential    ( StatTimeType stt )    { return    stt == StatTimeSequential; }  // each time frame is used seuqentially
inline bool         IsTimeAveraged      ( StatTimeType stt )    { return    stt != StatTimeSequential; }  // all time frames are used at once for a mean / median / min / max

extern const char   StatTimeString      [ NumStatTimes ][ 64 ];
extern const char   StatTimeStringShort [ NumStatTimes ][ 32 ];


//----------------------------------------------------------------------------

enum        PairedType
            {
            TestUnpaired,
            TestPaired,
            NumPairedType,
            };

extern const char   PairedTypeString[ NumPairedType ][ 16 ];

                                        // Multiple tests correction
enum        CorrectionType
            {
            CorrectionNone,
            CorrectionBonferroni,
            CorrectionFDRThresholdedP,
            CorrectionFDRCorrectedP,
            CorrectionFDRAdjustedP,
            NumCorrectionType,
            };

extern const char   CorrectionTypeString[ NumCorrectionType ][ 64 ];

inline  bool    IsCorrectionFDR         ( CorrectionType how )      { return    how == CorrectionFDRThresholdedP || how == CorrectionFDRCorrectedP || how == CorrectionFDRAdjustedP; }
inline  bool    IsCorrectionFDROutputQ  ( CorrectionType how )      { return                                        how == CorrectionFDRCorrectedP || how == CorrectionFDRAdjustedP; }

                                        // Different ways to output the p-values
enum        OutputPType
            {
            OutputP,
            Output1MinusP,
            OutputMinusLogP,

            NumOutputType,
            };

extern const char   OutputPTypeString[ NumOutputType ][ 16 ];


enum        SamplesInFiles
            {
            SamplesInNFiles,
            SamplesIn1File,
            SamplesInCsvFile
            };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Derive a specialized version from the general Group of Files
class   TStatGoF    :   public TGoF
{
public:
                    TStatGoF ();

                                        // We need some more info associated with the files:
    long            TimeMin;
    long            TimeMax;
    bool            EndOfFile;
    StatTimeType    StatTime;

                                        // Specific to Fitting results:
    int             GroupIndex;         // relative Group index within the file (starting from 0)
    char            GroupName[ 256 ];   // real group name, arbitrary and absolute
    int             NumSamples;


    void            Reset           ()  final;

    bool            IsTimeSequential()  const                   { return  crtl::IsTimeSequential ( StatTime ); }
    bool            IsTimeAveraged  ()  const                   { return  crtl::IsTimeAveraged   ( StatTime ); }
    int             GetTimeDuration ()  const                   { return  TimeMax - TimeMin + 1; }
};


//----------------------------------------------------------------------------
                                        // Overriding access functions
class   TStatGoGoF  :   public TGoGoF
{
public:

    const TStatGoF& GetFirst ()                                 const   { return   (TStatGoF &) TGoGoF::GetFirst (); }  // !Check group is not empty before calling!
          TStatGoF& GetFirst ()                                         { return   (TStatGoF &) TGoGoF::GetFirst (); }
    const TStatGoF& GetLast  ()                                 const   { return   (TStatGoF &) TGoGoF::GetLast  (); }  // !Check group is not empty before calling!
          TStatGoF& GetLast  ()                                         { return   (TStatGoF &) TGoGoF::GetLast  (); }


    bool            AllAreAverages  ( int gofi1, int gofi2 )    const;
    bool            IsSomeAverage   ( int gofi1, int gofi2 )    const;


    const TStatGoF& operator    []  ( int index )               const   { return    *( (TStatGoF *) Group[ index ] ); } // !Check index before calling!
          TStatGoF& operator    []  ( int index )                       { return    *( (TStatGoF *) Group[ index ] ); }
};


//----------------------------------------------------------------------------

void        CheckGroupsDurations        (   TStatGoGoF&     gogof,      int         gofi1,      int     gofi2, 
                                            bool            paired,     bool        unpaired,
                                            bool            isnfiles,   bool        iscsvfile 
                                        );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


class   TStatisticsFilesStruct
{
public:
                        TStatisticsFilesStruct ();


    TRadioButtonData    SamplesNFiles;
    TRadioButtonData    Samples1File;
    TRadioButtonData    SamplesCsvFile;

    TEditData           TimeMin     [ EditSizeValue ];
    TEditData           TimeMax     [ EditSizeValue ];
    TCheckBoxData       EndOfFile;

    TComboBoxData       PresetsTime;

    TEditData           NumGroups   [ EditSizeValue ];
    TListBoxData        GroupsSummary;

    TEditData           BaseFileName[ EditSizeText ];
    TComboBoxData       FileTypes;
    TRadioButtonData    OutputP;
    TRadioButtonData    Output1MinusP;
    TRadioButtonData    OutputMinusLogP;

    TCheckBoxData       OpenAuto;
    TCheckBoxData       MoreOutputs;
};


//----------------------------------------------------------------------------

class   TStatisticsParamsStruct
{
public:
                        TStatisticsParamsStruct ();


    TRadioButtonData    ExportTracks;
    TEditData           Channels    [ EditSizeTextLong ];
    TCheckBoxData       UseXyzDoc;
    TEditData           XyzDocFile  [ EditSizeText ];

    TRadioButtonData    ExportRois;
    TEditData           RoisDocFile [ EditSizeText ];

    TRadioButtonData    UnpairedTest;
    TRadioButtonData    PairedTest;

    TCheckBoxData       TTest;
    TCheckBoxData       Randomization;
    TCheckBoxData       TAnova;

    TComboBoxData       PresetsData;

    TRadioButtonData    SignedData;
    TRadioButtonData    PositiveData;

    TRadioButtonData    AccountPolarity;
    TRadioButtonData    IgnorePolarity;

    TRadioButtonData    NoRef;
    TRadioButtonData    AveRef;

    TRadioButtonData    NormalizationNone;
    TRadioButtonData    NormalizationGfp;
    TRadioButtonData    NormalizationGfpPaired;
    TRadioButtonData    NormalizationGfp1TF;

    TCheckBoxData       CheckMissingValues;
    TEditData           CheckMissingValuesValue [ EditSizeValue ];
    TEditData           NumberOfRandomization   [ EditSizeValue ];

    TComboBoxData       PresetsCorrection;

    TEditData           FDRCorrectionValue      [ EditSizeValue ];

    TCheckBoxData       ThresholdingPValues;
    TEditData           ThresholdingPValuesValue[ EditSizeValue ];

    TCheckBoxData       MinSignificantDuration;
    TEditData           MinSignificantDurationValue[ EditSizeValue ];
};


//----------------------------------------------------------------------------
                                        // join together the 2 structures
class   TStatStruct :   public  TStatisticsFilesStruct,
                        public  TStatisticsParamsStruct
{
public:
                    TStatStruct ();


    int             LastDialogId;
    owlwparam       FileType;           // given by dialog, used to differentiate between EEG / RIS / Freq statistics

                                        // add some convenient test functions
    bool            HasTTest            ()  const   { return CheckToBool ( TTest            );  }
    bool            HasRandomization    ()  const   { return CheckToBool ( Randomization    );  }
    bool            HasTAnova           ()  const   { return CheckToBool ( TAnova           );  }
    bool            HasPaired           ()  const   { return CheckToBool ( PairedTest       );  }
    bool            HasUnpaired         ()  const   { return CheckToBool ( UnpairedTest     );  }
    bool            HasTwoSample        ()  const   { return HasTTest () || HasRandomization () || HasTAnova ();  }
    bool            UseRandomization    ()  const   { return HasRandomization () || HasTAnova ();  }
    bool            HasParametric       ()  const   { return HasTTest (); }
    bool            HasNonParametric    ()  const   { return UseRandomization (); }

    bool            IsNFiles            ()  const   { return CheckToBool ( SamplesNFiles  ); }
    bool            Is1File             ()  const   { return CheckToBool ( Samples1File   ); }
    bool            IsCsvFile           ()  const   { return CheckToBool ( SamplesCsvFile ); }

    bool            IsChannels          ()  const   { return CheckToBool ( ExportTracks   ) && ! IsCsvFile (); }
    bool            IsRois              ()  const   { return CheckToBool ( ExportRois     ) && ! IsCsvFile (); }

    int             GetNumTF            ( const TStatGoF&   gof )                           const;
    int             GetNumSamples       ( const TStatGoF&   gof )                           const;
    int             GetNumSamples       ( const TStatGoGoF& gogof, int gofi1, int gofi2 )   const;
    int             GetNumSequentialTF  ( const TStatGoGoF& gogof, int gofi1, int gofi2 )   const;
};


EndBytePacking

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // common to all dialogs
class   TStatisticsDialog   :   public  TBaseDialog
{
public:
                        TStatisticsDialog ( owl::TWindow *parent, int resId ) : TBaseDialog ( parent, resId ) /*, SubResId ( resId )*/ {}

protected:
                                        // store the lists of files, shared between the 2 dialogs
    static TStatGoGoF   GoGoF;


    void                ProcessGroups       ( TGoGoF* gogof, int gofi1, int gofi2, void* usetransfer ); // TBaseDialog


    void                CmOk                ();
    virtual void        CmOkEnable          ( owl::TCommandEnabler& tce );
    void                CmToDialog          ( owlwparam w );
    void                SamplesParams       ();

    DECLARE_RESPONSE_TABLE ( TStatisticsDialog );
};


//----------------------------------------------------------------------------

class   TStatisticsFilesDialog  :   public  TStatisticsDialog
{
public:
                        TStatisticsFilesDialog  ( owl::TWindow* parent, int resId, owlwparam w = 0 );
                       ~TStatisticsFilesDialog  ();
                       
    void                CmClearGroups           ();
    void                ReadSpreadSheet         ( char *filename = 0 );


protected:
    owl::TRadioButton   *SamplesNFiles;
    owl::TRadioButton   *Samples1File;
    owl::TRadioButton   *SamplesCsvFile;

    owl::TEdit          *TimeMin;
    owl::TEdit          *TimeMax;
    owl::TCheckBox      *EndOfFile;

    owl::TComboBox      *PresetsTime;

    owl::TEdit          *NumGroups;
    owl::TListBox       *GroupsSummary;

    owl::TEdit          *BaseFileName;
    owl::TComboBox      *FileTypes;
    owl::TRadioButton   *OutputP;
    owl::TRadioButton   *Output1MinusP;
    owl::TRadioButton   *OutputMinusLogP;

    owl::TCheckBox      *OpenAuto;
    owl::TCheckBox      *MoreOutputs;


    bool                CheckGroups             ( const TGoF& gof ) const;
    void                AddFileToGroup          ( const char* filename, bool first );
    void                GuessOutputFileExtension();
    void                AddGroupSummary         ( int gofi );
    void                SetBaseFilename         ();

    void                CmOkEnable              ( owl::TCommandEnabler &tce )   { TStatisticsDialog::CmOkEnable ( tce ); }

    void                CmUpOneDirectory        ();
    void                CmBrowseBaseFileName    ();
    void                CmSamplesParams         ( owlwparam w );
    void                CmAddGroup              ( owlwparam w );
    void                CmRemoveGroup           ();
    void                CmSortGroups            ();
    void                EvDropFiles             ( owl::TDropInfo drop );
    void                CmReadSpreadSheet       ();
    void                CmWriteParams           ();
    void                CmTimeMaxEnable         ( owl::TCommandEnabler &tce );
    void                CmAddGroupEnable        ( owl::TCommandEnabler &tce );
    void                CmLastGroupEnable       ( owl::TCommandEnabler &tce );
    void                CmSamplesNFilesEnable   ( owl::TCommandEnabler &tce );
    void                CmSamplesCsvFileEnable  ( owl::TCommandEnabler &tce );
//  void                CmToNextDialogEnable    ( owl::TCommandEnabler &tce );

    DECLARE_RESPONSE_TABLE ( TStatisticsFilesDialog );
};


//----------------------------------------------------------------------------

class   TStatisticsParamsDialog :  public  TStatisticsDialog
{
public:
                        TStatisticsParamsDialog ( owl::TWindow *parent, int resId );
                       ~TStatisticsParamsDialog ();

protected:
    owl::TRadioButton   *ExportTracks;
    owl::TEdit          *Channels;
    owl::TCheckBox      *UseXyzDoc;
    owl::TEdit          *XyzDocFile;

    owl::TRadioButton   *ExportRois;
    owl::TEdit          *RoisDocFile;

    owl::TRadioButton   *UnpairedTest;
    owl::TRadioButton   *PairedTest;

    owl::TCheckBox      *TTest;
    owl::TCheckBox      *Randomization;
    owl::TCheckBox      *TAnova;

    owl::TComboBox      *PresetsData;

    owl::TRadioButton   *SignedData;
    owl::TRadioButton   *PositiveData;

    owl::TRadioButton   *AccountPolarity;
    owl::TRadioButton   *IgnorePolarity;

    owl::TRadioButton   *NoRef;
    owl::TRadioButton   *AveRef;

    owl::TRadioButton   *NormalizationNone;
    owl::TRadioButton   *NormalizationGfp;
    owl::TRadioButton   *NormalizationGfpPaired;
    owl::TRadioButton   *NormalizationGfp1TF;

    owl::TCheckBox      *CheckMissingValues;
    owl::TEdit          *CheckMissingValuesValue;
    owl::TEdit          *NumberOfRandomization;

    owl::TComboBox      *PresetsCorrection;

    owl::TEdit          *FDRCorrectionValue;

    owl::TCheckBox      *ThresholdingPValues;
    owl::TEdit          *ThresholdingPValuesValue;

    owl::TCheckBox      *MinSignificantDuration;
    owl::TEdit          *MinSignificantDurationValue;



    void                SetupWindow                         ();
    void                EvDropFiles                         ( owl::TDropInfo drop );

    void                CmOkEnable                          ( owl::TCommandEnabler &tce )   { TStatisticsDialog::CmOkEnable ( tce ); }


    void                CmBrowseXyzDoc                      ();
    void                CmTracksEnable                      ( owl::TCommandEnabler &tce );
    void                CmChannelsEnable                    ( owl::TCommandEnabler &tce );
    void                CmXyzDocFileEnable                  ( owl::TCommandEnabler &tce );

    void                CmRoiChange                         ();
    void                CmBrowseRoiDoc                      ();
    void                CmCsvDisable                        ( owl::TCommandEnabler &tce );
    void                CmRoisFileEnable                    ( owl::TCommandEnabler &tce );

    void                EvPresetsChange                     ();
    void                CmCheckTestParams                   ();
//  void                CmPolarityEnable                    ( owl::TCommandEnabler &tce );
    void                CmMinSignificantDurationEnable      ( owl::TCommandEnabler &tce );
    void                CmMinSignificantDurationValueEnable ( owl::TCommandEnabler &tce );
//  void                CmUnpairedEnable                    ( owl::TCommandEnabler &tce );
    void                CmSamplesNFilesEnable               ( owl::TCommandEnabler &tce );
    void                CmFDRCorrectionThresholdingEnable   ( owl::TCommandEnabler &tce );
    void                CmThresholdingPValuesEnable         ( owl::TCommandEnabler &tce );
    void                CmThresholdingPValuesValueEnable    ( owl::TCommandEnabler &tce );
    void                CmRandomizationEnable               ( owl::TCommandEnabler &tce );
    void                CmCheckMissingValuesEnable          ( owl::TCommandEnabler &tce );
    void                CmMissingValuesEnable               ( owl::TCommandEnabler &tce );
//  void                CmPairedEnable                      ( owl::TCommandEnabler &tce );

    DECLARE_RESPONSE_TABLE ( TStatisticsParamsDialog );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
