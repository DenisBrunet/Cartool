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

#include    "TMicroStatesSegDialog.h"

#include    "Math.Utils.h"
#include    "Math.Resampling.h"
#include    "Strings.Utils.h"
#include    "Strings.TFixedString.h"
#include    "Strings.Grep.h"
#include    "Files.SpreadSheet.h"
#include    "Files.TSplitLinkManyFile.h"
#include    "Files.BatchAveragingFiles.h"
#include    "Files.PreProcessFiles.h"
#include    "Dialogs.Input.h"

#include    "CartoolTypes.h"            // EpochsType SkippingEpochsType ResamplingType GfpPeaksDetectType ZScoreType CentroidType
#include    "BadEpochs.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

TMicroStatesSegStruct       SegTransfer;

TGoGoF                      TMicroStatesSegDialog::GoGoF;

                                        // !NOT const, as we will update some fields!
                                        // Note that resampling parameters have to be set at the file panel level, but is used as the last time parameter, after epochs and GFP peaks
SegPresetFilesSpec  SegPresetsFiles[ NumSegPresetFiles ] =
            {
            { SegPresetFilesERP,                            "ERPs                /  On Grand Mean(s)",                                  0,    0, 0.00, 0.00, (TypeFilesFlags) ( SegPresetWholeTimeRange                                       ) },
                                                                                                                                                                                                                                        
            { SegPresetFileSeparator1,                      "",                                                                         0,    0, 0.00, 0.00, (TypeFilesFlags) ( SegPresetFilesNone                                            ) },
                                                                                                                                                                                                                                        
            { SegPresetFilesRestingStatesSubjAllData,       "Resting States  /  First Stage on Individual(s)  /  Entire Dataset",       0,    0, 0.00, 0.00, (TypeFilesFlags) ( SegPresetWholeTimeRange                                       ) },
            { SegPresetFilesRestingStatesSubjResampling,    "Resting States  /  First Stage on Individual(s)  /  Resampling",          20,   30, 0.00, 0.95, (TypeFilesFlags) ( SegPresetResampling          | SegPresetPrioritySampleSize    ) },

            { SegPresetFileSeparator2,                      "",                                                                         0,    0, 0.00, 0.00, (TypeFilesFlags) ( SegPresetFilesNone                                            ) },

            { SegPresetFilesRestingStatesGroupAllData,      "Resting States  /  Second Stage on Group(s)   /  Entire Dataset",          0,    0, 0.00, 0.00, (TypeFilesFlags) ( SegPresetWholeTimeRange                                       ) },
            { SegPresetFilesRestingStatesGroupResampling,   "Resting States  /  Second Stage on Group(s)   /  Resampling",            100,    0, 0.25, 0.95, (TypeFilesFlags) ( SegPresetResampling          | SegPresetPriorityNumResampling ) },
            };

                                        // These are the needed parameters to "push on the dialog's button" for each preset - it does NOT contain all actual parameters
const SegPresetSpec SegPresets[ NumSegPresetParams ] =
            {
            { SegPresetEegSurfaceErpTAAHC,              "EEG / Surface / ERPs                                 / T-AAHC (recommended)",      0, 1, 20, -100, SmoothingDefaultHalfSize, (TypeClusteringFlags) ( SegPresetERP      | SegPresetTAAHC  | SegPresetAllData      | SegPresetSpatialFilterOff | SegPresetSignedData   | SegPresetAccountPolarity | SegPresetAveRef | SegPresetZScoreOff  | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSequentializeOn  | SegPresetSmoothingOn  | SegPresetRejectSmallOn  | SegPresetMergeCorrOff ) },
            { SegPresetEegSurfaceErpKMeans,             "EEG / Surface / ERPs                                 / K-Means (classical)",     300, 1, 20, -100, SmoothingDefaultHalfSize, (TypeClusteringFlags) ( SegPresetERP      | SegPresetKMeans | SegPresetAllData      | SegPresetSpatialFilterOff | SegPresetSignedData   | SegPresetAccountPolarity | SegPresetAveRef | SegPresetZScoreOff  | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSequentializeOn  | SegPresetSmoothingOn  | SegPresetRejectSmallOn  | SegPresetMergeCorrOff ) },
            { SegPresetEegSurfaceSpontKMeans,           "EEG / Surface / Rest. States / Individuals    / K-Means (recommended)",          100, 1, 12,   50, 0,                        (TypeClusteringFlags) ( SegPresetRSIndiv  | SegPresetKMeans | SegPresetGfpPeaksOnly | SegPresetSpatialFilterOn  | SegPresetSignedData   | SegPresetIgnorePolarity  | SegPresetAveRef | SegPresetZScoreOff  | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSequentializeOff | SegPresetSmoothingOff | SegPresetRejectSmallOff | SegPresetMergeCorrOff ) },
            { SegPresetEegSurfaceSpontKMeansAll,        "EEG / Surface / Rest. States / Individuals    / K-Means on All Data",            100, 1, 12,   50, SmoothingDefaultHalfSize, (TypeClusteringFlags) ( SegPresetRSIndiv  | SegPresetKMeans | SegPresetAllData      | SegPresetSpatialFilterOn  | SegPresetSignedData   | SegPresetIgnorePolarity  | SegPresetAveRef | SegPresetZScoreOff  | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSequentializeOff | SegPresetSmoothingOff | SegPresetRejectSmallOff | SegPresetMergeCorrOff ) },
            { SegPresetEegSurfaceCollectionKMeans,      "EEG / Surface / Rest. States / Grand Clust. / K-Means",                          200, 1, 15,   50, 0,                        (TypeClusteringFlags) ( SegPresetRSGroup  | SegPresetKMeans | SegPresetAllData      | SegPresetSpatialFilterOff | SegPresetSignedData   | SegPresetIgnorePolarity  | SegPresetAveRef | SegPresetZScoreOff  | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSequentializeOff | SegPresetSmoothingOff | SegPresetRejectSmallOff | SegPresetMergeCorrOff ) },
                                                                                                                                                                                                                // spatial filter not defined for other topologies thant 2D, so f.ex. strips doesn't exist
            { SegPresetSeparator1,                      "",                                                                                 0, 0,  0, -100, 0, SegPresetClusteringNone, SegPresetPostProcNone },

            { SegPresetEegIntraErpTAAHC,                "EEG / Intra-Cranial / ERPs                                 / T-AAHC",              0, 1, 20, -100, SmoothingDefaultHalfSize, (TypeClusteringFlags) ( SegPresetERP      | SegPresetTAAHC  | SegPresetAllData      | SegPresetSpatialFilterOff | SegPresetSignedData   | SegPresetAccountPolarity | SegPresetNoRef  | SegPresetZScoreOff  | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSequentializeOn  | SegPresetSmoothingOn  | SegPresetRejectSmallOn  | SegPresetMergeCorrOff ) },
            { SegPresetEegIntraErpKMeans,               "EEG / Intra-Cranial / ERPs                                 / K-Means",           300, 1, 20, -100, SmoothingDefaultHalfSize, (TypeClusteringFlags) ( SegPresetERP      | SegPresetKMeans | SegPresetAllData      | SegPresetSpatialFilterOff | SegPresetSignedData   | SegPresetAccountPolarity | SegPresetNoRef  | SegPresetZScoreOff  | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSequentializeOn  | SegPresetSmoothingOn  | SegPresetRejectSmallOn  | SegPresetMergeCorrOff ) },
            { SegPresetEegIntraSpontKMeans,             "EEG / Intra-Cranial / Rest. States / Individuals    / K-Means (recommended)",    100, 1, 12,   50, 0,                        (TypeClusteringFlags) ( SegPresetRSIndiv  | SegPresetKMeans | SegPresetGfpPeaksOnly | SegPresetSpatialFilterOff | SegPresetSignedData   | SegPresetIgnorePolarity  | SegPresetNoRef  | SegPresetZScoreOff  | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSequentializeOff | SegPresetSmoothingOff | SegPresetRejectSmallOff | SegPresetMergeCorrOff ) },
            { SegPresetEegIntraSpontKMeansAll,          "EEG / Intra-Cranial / Rest. States / Individuals    / K-Means on All Data",      100, 1, 12,   50, SmoothingDefaultHalfSize, (TypeClusteringFlags) ( SegPresetRSIndiv  | SegPresetKMeans | SegPresetAllData      | SegPresetSpatialFilterOff | SegPresetSignedData   | SegPresetIgnorePolarity  | SegPresetNoRef  | SegPresetZScoreOff  | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSequentializeOff | SegPresetSmoothingOff | SegPresetRejectSmallOff | SegPresetMergeCorrOff ) },
            { SegPresetEegIntraCollectionKMeans,        "EEG / Intra-Cranial / Rest. States / Grand Clust. / K-Means",                    200, 1, 15,   50, 0,                        (TypeClusteringFlags) ( SegPresetRSGroup  | SegPresetKMeans | SegPresetAllData      | SegPresetSpatialFilterOff | SegPresetSignedData   | SegPresetIgnorePolarity  | SegPresetNoRef  | SegPresetZScoreOff  | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSequentializeOff | SegPresetSmoothingOff | SegPresetRejectSmallOff | SegPresetMergeCorrOff ) },

            { SegPresetSeparator2,                      "",                                                                                 0, 0,  0, -100, 0, SegPresetClusteringNone, SegPresetPostProcNone },

            { SegPresetMegSurfaceErpTAAHC,              "MEG / ERPs                                 / T-AAHC (recommended)",                0, 1, 20, -100, SmoothingDefaultHalfSize, (TypeClusteringFlags) ( SegPresetERP      | SegPresetTAAHC  | SegPresetAllData      | SegPresetSpatialFilterOff | SegPresetPositiveData | SegPresetAccountPolarity | SegPresetNoRef | SegPresetZScoreOff  | SegPresetEnvelopeOff ),     (TypePostProcFlags) ( SegPresetSequentializeOn  | SegPresetSmoothingOn  | SegPresetRejectSmallOn  | SegPresetMergeCorrOff ) },
            { SegPresetMegSurfaceErpKMeans,             "MEG / ERPs                                 / K-Means (classical)",               300, 1, 20, -100, SmoothingDefaultHalfSize, (TypeClusteringFlags) ( SegPresetERP      | SegPresetKMeans | SegPresetAllData      | SegPresetSpatialFilterOff | SegPresetPositiveData | SegPresetAccountPolarity | SegPresetNoRef | SegPresetZScoreOff  | SegPresetEnvelopeOff ),     (TypePostProcFlags) ( SegPresetSequentializeOn  | SegPresetSmoothingOn  | SegPresetRejectSmallOn  | SegPresetMergeCorrOff ) },
            { SegPresetMegSurfaceSpontKMeans,           "MEG / Rest. States / Individuals    / K-Means (recommended)",                    100, 1, 12,   50, 0,                        (TypeClusteringFlags) ( SegPresetRSIndiv  | SegPresetKMeans | SegPresetGfpPeaksOnly | SegPresetSpatialFilterOn  | SegPresetPositiveData | SegPresetAccountPolarity | SegPresetNoRef | SegPresetZScoreOff  | SegPresetEnvelopeOff ),     (TypePostProcFlags) ( SegPresetSequentializeOff | SegPresetSmoothingOff | SegPresetRejectSmallOff | SegPresetMergeCorrOff ) },
            { SegPresetMegSurfaceSpontKMeansAll,        "MEG / Rest. States / Individuals    / K-Means on All Data",                      100, 1, 12,   50, SmoothingDefaultHalfSize, (TypeClusteringFlags) ( SegPresetRSIndiv  | SegPresetKMeans | SegPresetAllData      | SegPresetSpatialFilterOn  | SegPresetPositiveData | SegPresetAccountPolarity | SegPresetNoRef | SegPresetZScoreOff  | SegPresetEnvelopeOff ),     (TypePostProcFlags) ( SegPresetSequentializeOff | SegPresetSmoothingOff | SegPresetRejectSmallOff | SegPresetMergeCorrOff ) },
            { SegPresetMegSurfaceCollectionKMeans,      "MEG / Rest. States / Grand Clust. / K-Means",                                    200, 1, 15,   50, 0,                        (TypeClusteringFlags) ( SegPresetRSGroup  | SegPresetKMeans | SegPresetAllData      | SegPresetSpatialFilterOff | SegPresetPositiveData | SegPresetAccountPolarity | SegPresetNoRef | SegPresetZScoreOff  | SegPresetEnvelopeOff ),     (TypePostProcFlags) ( SegPresetSequentializeOff | SegPresetSmoothingOff | SegPresetRejectSmallOff | SegPresetMergeCorrOff ) },

            { SegPresetSeparator3,                      "",                                                                                 0, 0,  0, -100, 0, SegPresetClusteringNone, SegPresetPostProcNone },

            { SegPresetRisScalErpTAAHC,                 "ESI / ERPs                                 / T-AAHC",                              0, 1, 20, -100, SmoothingDefaultHalfSize, (TypeClusteringFlags) ( SegPresetERP      | SegPresetTAAHC  | SegPresetAllData      | SegPresetSpatialFilterOff | SegPresetPositiveData | SegPresetAccountPolarity | SegPresetNoRef | SegPresetZScoreOn   | SegPresetEnvelopeOff ),     (TypePostProcFlags) ( SegPresetSequentializeOn  | SegPresetSmoothingOn  | SegPresetRejectSmallOn  | SegPresetMergeCorrOff ) },
            { SegPresetRisScalErpKMeans,                "ESI / ERPs                                 / K-Means",                           100, 1, 20, -100, SmoothingDefaultHalfSize, (TypeClusteringFlags) ( SegPresetERP      | SegPresetKMeans | SegPresetAllData      | SegPresetSpatialFilterOff | SegPresetPositiveData | SegPresetAccountPolarity | SegPresetNoRef | SegPresetZScoreOn   | SegPresetEnvelopeOff ),     (TypePostProcFlags) ( SegPresetSequentializeOn  | SegPresetSmoothingOn  | SegPresetRejectSmallOn  | SegPresetMergeCorrOff ) },
            { SegPresetRisScalSpontKMeans,              "ESI / Rest. States / Individuals    / K-Means (recommended)",                     50, 1, 15, -100, 0,                        (TypeClusteringFlags) ( SegPresetRSIndiv  | SegPresetKMeans | SegPresetGfpPeaksOnly | SegPresetSpatialFilterOff | SegPresetPositiveData | SegPresetAccountPolarity | SegPresetNoRef | SegPresetZScoreOn   | SegPresetEnvelopeOff ),     (TypePostProcFlags) ( SegPresetSequentializeOff | SegPresetSmoothingOff | SegPresetRejectSmallOff | SegPresetMergeCorrOff ) },
            { SegPresetRisScalSpontKMeansAll,           "ESI / Rest. States / Individuals    / K-Means on All Data",                       50, 1, 15, -100, SmoothingDefaultHalfSize, (TypeClusteringFlags) ( SegPresetRSIndiv  | SegPresetKMeans | SegPresetAllData      | SegPresetSpatialFilterOff | SegPresetPositiveData | SegPresetAccountPolarity | SegPresetNoRef | SegPresetZScoreOn   | SegPresetEnvelopeOff ),     (TypePostProcFlags) ( SegPresetSequentializeOff | SegPresetSmoothingOff | SegPresetRejectSmallOff | SegPresetMergeCorrOff ) },
            { SegPresetRisScalCollectionKMeans,         "ESI / Rest. States / Grand Clust. / K-Means",                                    100, 1, 25, -100, 0,                        (TypeClusteringFlags) ( SegPresetRSGroup  | SegPresetKMeans | SegPresetAllData      | SegPresetSpatialFilterOff | SegPresetPositiveData | SegPresetAccountPolarity | SegPresetNoRef | SegPresetZScoreOff  | SegPresetEnvelopeOff ),     (TypePostProcFlags) ( SegPresetSequentializeOff | SegPresetSmoothingOff | SegPresetRejectSmallOff | SegPresetMergeCorrOff ) },

//          { SegPresetSeparator4,                      "",                                                                                 0, 0,  0, -100, 0, SegPresetClusteringNone, SegPresetPostProcNone },

//          { SegPresetPosTAAHC,                        "Positive Data / T-AAHC",                                                           0, 1, 20, -100, 0,                        (TypeClusteringFlags) ( SegPresetERP      | SegPresetTAAHC  | SegPresetAllData      | SegPresetSpatialFilterOff | SegPresetPositiveData | SegPresetAccountPolarity | SegPresetNoRef | SegPresetZScoreOff  | SegPresetEnvelopeOff ),     (TypePostProcFlags) ( SegPresetSequentializeOff | SegPresetSmoothingOff | SegPresetRejectSmallOff | SegPresetMergeCorrOff ) },
//          { SegPresetPosKMeans,                       "Positive Data / K-Means",                                                        300, 1, 20, -100, 0,                        (TypeClusteringFlags) ( SegPresetERP      | SegPresetKMeans | SegPresetAllData      | SegPresetSpatialFilterOff | SegPresetPositiveData | SegPresetAccountPolarity | SegPresetNoRef | SegPresetZScoreOff  | SegPresetEnvelopeOff ),     (TypePostProcFlags) ( SegPresetSequentializeOff | SegPresetSmoothingOff | SegPresetRejectSmallOff | SegPresetMergeCorrOff ) },
            };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TMicroStatesSegFilesStruct::TMicroStatesSegFilesStruct ()
{
StringCopy ( NumGroups, "0" );
GroupsSummary.Clear ();


PresetsFiles.Clear ();
for ( int i = 0; i < NumSegPresetFiles; i++ )
    PresetsFiles.AddString ( SegPresetsFiles[ i ].Text, i == SegPresetFilesDefault );


SegPresetFilesSpec &presetf         = SegPresetsFiles[ SegPresetFilesDefault ];
TypeFilesFlags      fflags          = presetf.FilesFlags;

NoEpochs            = BoolToCheck ( fflags & SegPresetWholeTimeRange );
ListEpochs          = BoolToCheck ( fflags & SegPresetEpochs         );
ResamplingEpochs    = BoolToCheck ( fflags & SegPresetResampling     );


EpochFrom.Clear ();
EpochTo  .Clear ();
//IntegerToString ( NumResamplingEpochs,  presetf.NumResampling  );
//IntegerToString ( ResamplingEpochsSize, presetf.SampleSizeSeconds );
//FloatToString   ( ResamplingCoverage,   presetf.ResamplingCoverage * 100, 1 );
ClearString     ( NumResamplingEpochs  );
ClearString     ( ResamplingEpochsSize );
ClearString     ( ResamplingCoverage   );


TFileName           cp;
cp.GetCurrentDir ();
StringAppend ( cp, "\\Seg" );
StringCopy ( BaseFileName, cp );

WriteSeg            = BoolToCheck ( true  );
WriteMaps           = BoolToCheck ( true  );
WriteClusters       = BoolToCheck ( false );
WriteSynth          = BoolToCheck ( false );

ForceSingleFiles    = BoolToCheck ( false );
BestDir             = BoolToCheck ( false );
DeleteIndivDirs     = BestDir;
}


        TMicroStatesSegParamsStruct::TMicroStatesSegParamsStruct ()
{
PresetsParams.Clear ();
for ( int i = 0; i < NumSegPresetParams; i++ )
    PresetsParams.AddString ( SegPresets[ i ].Text, i == SegPresetParamsDefault );

                                        // use the exact parameters from default preset
const SegPresetSpec&    preset          = SegPresets[ SegPresetParamsDefault ];
uint                    clflags         = preset.ClusteringFlags;
uint                    ppflags         = preset.PostProcFlags;


KMeans              = BoolToCheck ( clflags & SegPresetKMeans             );
TAAHC               = BoolToCheck ( clflags & SegPresetTAAHC              );

PositiveData        = BoolToCheck ( clflags & SegPresetPositiveData       );
SignedData          = BoolToCheck ( clflags & SegPresetSignedData         );
VectorData          = BoolToCheck ( clflags & SegPresetVectorData         );

AccountPolarity     = BoolToCheck ( clflags & SegPresetAccountPolarity    );
IgnorePolarity      = BoolToCheck ( clflags & SegPresetIgnorePolarity     );

NoRef               = BoolToCheck ( clflags & SegPresetNoRef              );
AveRef              = BoolToCheck ( clflags & SegPresetAveRef             );

IntegerToString ( RandomTrials, preset.RandomTrials > 0 ? preset.RandomTrials : 300 );

IntegerToString ( MinClusters,  preset.MinClusters );   // always 1, and not editable anymore
IntegerToString ( MaxClusters,  preset.MaxClusters );

AllTFs              = BoolToCheck ( clflags & SegPresetAllData            );
GfpPeaksOnly        = BoolToCheck ( clflags & SegPresetGfpPeaksOnly       );
GfpPeaksAuto        = BoolToCheck ( true  );
GfpPeaksList        = BoolToCheck ( false );
ClearString ( AnalyzeMarkers );

SkipBadEpochs       = BoolToCheck ( false );
SkipBadEpochsAuto   = BoolToCheck ( false );
SkipBadEpochsList   = BoolToCheck ( true  );
ClearString ( SkipMarkers );

SpatialFilter       = BoolToCheck ( clflags & SegPresetSpatialFilterOn    );
ClearString ( XyzFile );

ClipCorrelation     = BoolToCheck ( preset.CorrelationThreshold > -100 );
IntegerToString ( MinCorrelation, max ( 50, Clip ( preset.CorrelationThreshold, -100, 100 ) ) );

DualDataset         = BoolToCheck ( false );

Sequentialize       = BoolToCheck ( ppflags & SegPresetSequentializeOn    );

MergeCorr           = BoolToCheck ( ppflags & SegPresetMergeCorrOn        );
StringCopy ( MergeCorrThresh, "95" );

Smoothing           = BoolToCheck ( ppflags & SegPresetSmoothingOn        );
IntegerToString ( WindowSize, preset.SmoothingSize );
FloatToString   ( BesagFactor, SmoothingDefaultBesag );

RejectSmall         = BoolToCheck ( ppflags & SegPresetRejectSmallOn      );
IntegerToString ( RejectSize, preset.SmoothingSize );
}


        TMicroStatesSegStruct::TMicroStatesSegStruct ()
{
LastDialogId                = IDD_SEGMENT1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TMicroStatesSegFilesDialog, TMicroStatesSegDialog )

    EV_WM_DROPFILES,

    EV_CBN_SELCHANGE            ( IDC_PRESETSFILES,             EvPresetsChange ),

    EV_COMMAND                  ( IDC_UPDIRECTORY,              CmUpOneDirectory ),
    EV_COMMAND                  ( IDC_BROWSEBASEFILENAME,       CmBrowseBaseFileName ),

    EV_COMMAND                  ( IDC_ADDGROUP,                 CmAddGroup ),
    EV_COMMAND                  ( IDC_REMFROMLIST,              CmRemoveGroup ),
    EV_COMMAND                  ( IDC_CLEARLISTS,               CmClearGroups ),
    EV_COMMAND                  ( IDC_SORTLISTS,                CmSortGroups ),
    EV_COMMAND                  ( IDC_READPARAMS,               CmReadParams ),
    EV_COMMAND                  ( IDC_WRITEPARAMS,              CmWriteParams ),

    EV_COMMAND                  ( IDC_ADDEPOCH,                 CmAddEpoch ),
    EV_COMMAND                  ( IDC_REMOVEEPOCH,              CmRemoveEpoch ),
    EV_COMMAND_ENABLE           ( IDC_EPOCHFROM,                CmListEpochsEnable ),
    EV_COMMAND_ENABLE           ( IDC_EPOCHTO,                  CmListEpochsEnable ),
    EV_EN_CHANGE                ( IDC_NUMRESAMPLINGEPOCHS,      UpdateResamplingCoverage ),
    EV_EN_CHANGE                ( IDC_RESAMPLINGEPOCHSSIZE,     UpdateResamplingCoverage ),
    EV_EN_CHANGE                ( IDC_RESAMPLINGCOVERAGE,       UpdateNumResampling ),
    EV_COMMAND_ENABLE           ( IDC_NUMRESAMPLINGEPOCHS,      CmResamplingEpochsEnable ),
    EV_COMMAND_ENABLE           ( IDC_RESAMPLINGEPOCHSSIZE,     CmResamplingEpochsEnable ),
    EV_COMMAND_ENABLE           ( IDC_RESAMPLINGCOVERAGE,       CmResamplingEpochsEnable ),

    EV_COMMAND_ENABLE           ( IDC_DELINDIVDIRS,             CmDeleteIndivDirsEnable ),

    EV_COMMAND_ENABLE           ( IDOK,                         CmOkEnable ),

END_RESPONSE_TABLE;



        TMicroStatesSegFilesDialog::TMicroStatesSegFilesDialog ( TWindow* parent, TResId resId )
      : TMicroStatesSegDialog ( parent, resId )
{
XyzAndEegMatch      = true;
GroupsAllRis        = false;
SamplingFrequency   = 0;


NumGroups           = new TEdit         ( this, IDC_NUMGROUPS, EditSizeValue );
GroupsSummary       = new TListBox      ( this, IDC_GROUPSSUMMARY );

NoEpochs            = new TRadioButton  ( this, IDC_NOEPOCHS );
ListEpochs          = new TRadioButton  ( this, IDC_MANUALEPOCHS );
EpochFrom           = new TComboBox     ( this, IDC_EPOCHFROM );
EpochTo             = new TComboBox     ( this, IDC_EPOCHTO );
ResamplingEpochs    = new TRadioButton  ( this, IDC_RESAMPLINGEPOCHS );
NumResamplingEpochs = new TEdit         ( this, IDC_NUMRESAMPLINGEPOCHS, EditSizeValue );
ResamplingEpochsSize= new TEdit         ( this, IDC_RESAMPLINGEPOCHSSIZE, EditSizeValue );
ResamplingCoverage  = new TEdit         ( this, IDC_RESAMPLINGCOVERAGE, EditSizeValue );
ResamplingCoverage->SetValidator ( new TFilterValidator ( ValidatorPositiveFloat ) );


PresetsFiles        = new TComboBox     ( this, IDC_PRESETSFILES );

BaseFileName        = new TEdit         ( this, IDC_BASEFILENAME, EditSizeText );
WriteSeg            = new TCheckBox     ( this, IDC_WRITESEG );
WriteMaps           = new TCheckBox     ( this, IDC_WRITEMAPS );
WriteClusters       = new TCheckBox     ( this, IDC_WRITECLUSTERS );
WriteSynth          = new TCheckBox     ( this, IDC_WRITESYNTH );

ForceSingleFiles    = new TCheckBox     ( this, IDC_FORCESINGLEFILES );
BestDir             = new TCheckBox     ( this, IDC_BESTTODIR );
DeleteIndivDirs     = new TCheckBox     ( this, IDC_DELINDIVDIRS );


SetTransferBuffer ( dynamic_cast <TMicroStatesSegFilesStruct*> ( &SegTransfer ) );


LockResampling      = false;
}


        TMicroStatesSegFilesDialog::~TMicroStatesSegFilesDialog ()
{
delete  NumGroups;              delete  GroupsSummary;
delete  NoEpochs;
delete  ListEpochs;             delete  EpochFrom;              delete  EpochTo;
delete  ResamplingEpochs;       delete  NumResamplingEpochs;    delete  ResamplingEpochsSize;   delete  ResamplingCoverage;
delete  PresetsFiles;
delete  BaseFileName;           
delete  WriteSeg;               delete  WriteMaps;
delete  WriteClusters;          delete  WriteSynth;
delete  ForceSingleFiles;
delete  BestDir;                delete  DeleteIndivDirs;
}


void    TMicroStatesSegFilesDialog::SetupWindow ()
{
                                        // !preventing any premature update!
LockResampling  = true;
static bool     init    = true;


TMicroStatesSegDialog::SetupWindow ();


//SetTransferBuffer ( dynamic_cast <TMicroStatesSegFilesStruct*> ( &SegTransfer ) );
                                        // reloading presets & flags
TransferData ( tdSetData );


GroupsSummary->ClearList ();
NumGroups->Clear ();
NumGroups->Insert ( "0" );



for ( int gofi = 0; gofi < (int) GoGoF; gofi++ )

    AddGroupSummary ( gofi, init );


GroupsAllRis        = GoGoF.AllExtensionsAre ( FILEEXT_RIS );

SetSamplingFrequency ();

                                        // updating depends on parameter
SegPresetFilesSpec &presetf         = SegPresetsFiles[ PresetsFiles->GetSelIndex () ];
if      ( IsFlag ( presetf.FilesFlags, SegPresetPrioritySampleSize    ) )   SampleSizeToNumResampling ();
else if ( IsFlag ( presetf.FilesFlags, SegPresetPriorityNumResampling ) )   UpdateResamplingCoverage  ();


init            = false;
LockResampling  = false;


auto&               tooltip         = *TTooltip::Make ( this );

tooltip.AddTool (   GetHandle (),   IDC_GROUPSSUMMARY,          "Summary of all groups of files" );
tooltip.AddTool (   GetHandle (),   IDC_NOEPOCHS,               "Use all data at once" );
tooltip.AddTool (   GetHandle (),   IDC_MANUALEPOCHS,           "Segmentation will run sequentially on each epoch" );
tooltip.AddTool (   GetHandle (),   IDC_RESAMPLINGEPOCHS,       "Epochs will be randomly generated" );
//tooltip.AddTool (   GetHandle (),   IDC_NUMRESAMPLINGEPOCHS,    "Changing the number of epochs will update the percentage of Coverage" ); // TEdit fields kind of "blinks"?
//tooltip.AddTool (   GetHandle (),   IDC_RESAMPLINGEPOCHSSIZE,   "Changing the epochs' size will update the percentage of Coverage" );
//tooltip.AddTool (   GetHandle (),   IDC_RESAMPLINGCOVERAGE,     "Changing the percentage of Coverage will update the number of epochs" );
tooltip.AddTool (   GetHandle (),   IDC_FORCESINGLEFILES,       "Ignoring the groups of files structure, files will be processed 1 by 1" );
tooltip.AddTool (   GetHandle (),   IDC_BESTTODIR,              "The optimal number of maps will be saved into a single common directory" );
tooltip.AddTool (   GetHandle (),   IDC_DELINDIVDIRS,           "Available with 'Best Clustering Directory' option, force delete the intermediate results" );
}


//----------------------------------------------------------------------------
void    TMicroStatesSegFilesDialog::EvPresetsChange ()
{
SegPresetFilesEnum  presetfile      = (SegPresetFilesEnum) PresetsFiles->GetSelIndex ();
SegPresetFilesSpec &presetf         = SegPresetsFiles[ presetfile ];
TypeFilesFlags      fflags          = presetf.FilesFlags;


if ( fflags == 0 )
    return;


NoEpochs        ->SetCheck ( fflags & SegPresetWholeTimeRange );
ListEpochs      ->SetCheck ( fflags & SegPresetEpochs         );
ResamplingEpochs->SetCheck ( fflags & SegPresetResampling     );


WriteMaps       ->SetCheck ( BoolToCheck ( true  ) );

//bool                nfiles          = GoGoF.NumFiles () > 1;  // for single files & bestdir?


switch ( presetfile ) {

  case  SegPresetFilesERP:
                                        // if any existing epoch, suggest to use them
    if ( EpochFrom->GetCount () > 0 || EpochFrom->GetTextLen () > 0 ) {
        NoEpochs        ->SetCheck ( BoolToCheck ( false ) );
        ListEpochs      ->SetCheck ( BoolToCheck ( true  ) );
        ResamplingEpochs->SetCheck ( BoolToCheck ( false ) );
        }

    WriteSeg        ->SetCheck ( BoolToCheck ( true  ) );
    WriteClusters   ->SetCheck ( BoolToCheck ( false ) );
    ForceSingleFiles->SetCheck ( BoolToCheck ( false ) );
    BestDir         ->SetCheck ( BoolToCheck ( false ) );
    DeleteIndivDirs ->SetCheck ( BestDir->GetCheck ()  );
    break;


  case  SegPresetFilesRestingStatesSubjAllData:
  case  SegPresetFilesRestingStatesSubjResampling:

    WriteSeg        ->SetCheck ( BoolToCheck ( false ) );
    WriteClusters   ->SetCheck ( BoolToCheck ( false ) );
    ForceSingleFiles->SetCheck ( BoolToCheck ( true  ) );
    BestDir         ->SetCheck ( BoolToCheck ( true  ) );
    DeleteIndivDirs ->SetCheck ( BestDir->GetCheck ()  );
    break;


  case  SegPresetFilesRestingStatesGroupAllData:

    WriteSeg        ->SetCheck ( BoolToCheck ( true  ) );
    WriteClusters   ->SetCheck ( BoolToCheck ( true  ) );
    ForceSingleFiles->SetCheck ( BoolToCheck ( false ) );
    BestDir         ->SetCheck ( BoolToCheck ( false ) );
    DeleteIndivDirs ->SetCheck ( BestDir->GetCheck ()  );
    break;

  case  SegPresetFilesRestingStatesGroupResampling:

    WriteSeg        ->SetCheck ( BoolToCheck ( false ) );
    WriteClusters   ->SetCheck ( BoolToCheck ( false ) );
    ForceSingleFiles->SetCheck ( BoolToCheck ( false ) );
    BestDir         ->SetCheck ( BoolToCheck ( true  ) );
    DeleteIndivDirs ->SetCheck ( BestDir->GetCheck ()  );
    break;

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( fflags & SegPresetResampling ) {
                                        // lock to prevent NumResamplingEpochs / ResamplingEpochsSize to update Coverage
    LockResampling  = true;

    SetInteger  ( NumResamplingEpochs, presetf.NumResampling );

    SetDouble   ( ResamplingCoverage, RoundTo ( presetf.ResamplingCoverage * 100, 0.1 ) );

    LockResampling  = false;


    UpdateSampleSize ();

                                        // updating depends on parameter
    if      ( IsFlag ( fflags, SegPresetPrioritySampleSize    ) )   SampleSizeToNumResampling ();
    else if ( IsFlag ( fflags, SegPresetPriorityNumResampling ) )   UpdateResamplingCoverage  ();
    }


SetBaseFilename       ();
}


//----------------------------------------------------------------------------
void    TMicroStatesSegFilesDialog::CmDeleteIndivDirsEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( BestDir->GetCheck () ) );
}


//----------------------------------------------------------------------------
void    TMicroStatesSegFilesDialog::CmBrowseBaseFileName ()
{
static GetFileFromUser  getfile ( "Base File Name", AllFilesFilter, 1, GetFilePath );


if ( ! getfile.Execute ( SegTransfer.BaseFileName ) )
    return;


TransferData ( tdSetData );

BaseFileName->ResetCaret;
}


//----------------------------------------------------------------------------
void    TMicroStatesSegFilesDialog::CmAddGroup ()
{
static GetFileFromUser  getfiles ( "Open Files", AllErpEegRisFilesFilter, 1, GetFileMulti );

TransferData ( tdGetData );


if ( ! getfiles.Execute () )
    return;


if ( ! CheckTracksGroup ( getfiles ) ) 
    return;


GoGoF.Add ( getfiles, true, MaxPathShort );

AddGroupSummary ( GoGoF.NumGroups () - 1 );

GroupsAllRis        = GoGoF.AllExtensionsAre ( FILEEXT_RIS );

SetSamplingFrequency ();


UpdateSampleSize ();
}


bool    TMicroStatesSegFilesDialog::CheckTracksGroup ( const TGoF& gof )
{
                                        // Check whole group dimensions
TracksCompatibleClass   tc;
                                        // Checks for the most important dimensions (note that numsolpoints is numtracks for .ris)
gof.AllTracksAreCompatible ( tc );


if      ( tc.NumTracks == CompatibilityNotConsistent ) {
    ShowMessage (   "Files don't seem to have the same number of electrodes/tracks!" NewLine 
                    "Check again your input files...", 
                    SegmentationTitle, ShowMessageWarning );
    return  false;
    }
else if ( tc.NumTracks == CompatibilityIrrelevant ) {
    ShowMessage (   "Files don't seem to have any electrodes/tracks at all!" NewLine 
                    "Check again your input files...", 
                    SegmentationTitle, ShowMessageWarning );
    return  false;
    }

                                        // this test is quite weak, as ReadFromHeader does a lousy job at retrieving the aux tracks (it needs the strings, so opening the whole file)
if      ( tc.NumAuxTracks > 0 ) {
    ShowMessage (   "It is not allowed to run the segmentation process with remaining auxiliary tracks!" NewLine 
                    "Check again your input files...", 
                    SegmentationTitle, ShowMessageWarning );
    return  false;
    }


if      ( tc.SamplingFrequency == CompatibilityNotConsistent ) {
    ShowMessage (   "Files don't seem to have the same sampling frequencies!" NewLine 
                    "Check again your input files...", 
                    SegmentationTitle, ShowMessageWarning );
//  if ( ! GetAnswerFromUser (  "Files don't seem to have the same sampling frequencies!" NewLine 
//                              "Do you want to proceed anyway (not recommended)?", 
//                              SegmentationTitle ) )
        return  false;
    }

                                        // updating depends on parameter
SegPresetFilesSpec &presetf         = SegPresetsFiles[ PresetsFiles->GetSelIndex () ];
if      ( IsFlag ( presetf.FilesFlags, SegPresetPrioritySampleSize    ) )   SampleSizeToNumResampling ();
else if ( IsFlag ( presetf.FilesFlags, SegPresetPriorityNumResampling ) )   UpdateResamplingCoverage  ();
                                        // Testing file extensions, too?

return true;
}


void    TMicroStatesSegFilesDialog::AddGroupSummary ( int gofi, bool updatebasefilename )
{
if ( gofi < 0 || gofi >= (int) GoGoF )
    return;


const TGoF&         gof             = GoGoF[ gofi ];
TFileName           buff;

                                        // update dialog
StringCopy  ( buff, "Group ", IntegerToString ( gofi + 1, 2 ), ":  ", IntegerToString ( gof.NumFiles () ) );

if ( gof.NumFiles () == 1 ) StringAppend    ( buff, " File  ( ",  ToFileName ( gof.GetFirst () ), " )" );
else                        StringAppend    ( buff, " Files  ( ", ToFileName ( gof.GetFirst () ), " .. ", ToFileName ( gof.GetLast  () ), " )" );


GroupsSummary->InsertString ( buff, 0 );

IntegerToString ( buff, GoGoF.NumGroups () );
NumGroups->Clear ();
NumGroups->Insert ( buff );

if ( updatebasefilename )
    SetBaseFilename ();
}


//----------------------------------------------------------------------------
                                        // Estimate the number of data for resampling
int     TMicroStatesSegFilesDialog::GetNumData ()     const
{
if ( GoGoF.IsEmpty () )
    return  0;


int                 numtf           = CheckToBool ( ForceSingleFiles->GetCheck () ) ? GoGoF.GetSumNumTF () / (double) GoGoF.NumFiles  ()      // average duration of a single file
                                                                                    : GoGoF.GetSumNumTF () / (double) GoGoF.NumGroups ();     // average duration of a single group
return  numtf;
}

                                        // Getting a wild guess on whether data has already been GfpMax'ed, and if not, if it will
bool    TMicroStatesSegFilesDialog::IsGfpMax ()   const
{
if ( GoGoF.IsEmpty () )
    return  false;

                                        // unfortunately not visible from here - also not already set by user
//return    CheckToBool ( GfpPeaksOnly->GetCheck () );

                                        // user mixing GfpMax and non-peak data is a real no-no
                                        // using SomeStringsGrep insterad of AllStringsGrep just in case some files get misspelled - which happens
bool                lookslikegfpmax = TGoF ( GoGoF ).SomeStringsGrep ( "Gfp.?(Max|Peak)(|s|es)", GrepOptionDefaultFiles );

return  lookslikegfpmax;
}


void    TMicroStatesSegFilesDialog::SetSamplingFrequency ()
{
SamplingFrequency   = GoGoF.IsEmpty ()  ?   0 
                    : IsGfpMax ()       ?   GfpPeaksSamplingFrequency 
                    :                       GoGoF.GetSamplingFrequency ();  // will be 0 for templates files, which should be OK as working in Time Frames
}


//----------------------------------------------------------------------------
                                        // When user is updating the sample size, this will adjust the number of resampling according to current coverage
void    TMicroStatesSegFilesDialog::SampleSizeToNumResampling ()
{
if ( GoGoF.IsEmpty () 
  || LockResampling 
  || ! CheckToBool ( ResamplingEpochs->GetCheck () ) 
   )
    return;

                                        // lock to prevent looping changes
LockResampling  = true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TResampling         resampling;
double              coverage;


coverage    = GetDouble ( ResamplingCoverage );

coverage    = Clip ( coverage / 100, 0.0, 1.0 );


resampling.SetNumData       ( GetNumData () );

resampling.SetSampleSize    ( ResamplingEpochsSize );

resampling.GetNumResampling ( coverage, SegmentMinNumResampling, SegmentMaxNumResampling );


                                        // Some fool-proof tests
if ( resampling.NumData         < SegmentMinNumData
  || resampling.SampleSize      < SegmentMinSampleSize
  || resampling.NumResampling   < SegmentMinNumResampling
   )

    SetInteger  ( NumResamplingEpochs, /*SegmentMinNumResampling*/ 0 );
else
    SetInteger  ( NumResamplingEpochs, resampling.NumResampling );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

LockResampling  = false;
}


//----------------------------------------------------------------------------
                                        // When user is updating the number of resampling / sample size, update the coverage for info
void    TMicroStatesSegFilesDialog::UpdateResamplingCoverage ()
{
if ( GoGoF.IsEmpty () 
  || LockResampling 
  || ! CheckToBool ( ResamplingEpochs->GetCheck () ) 
   )
    return;

                                        // lock to prevent looping changes
LockResampling  = true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TResampling         resampling;


resampling.SetNumData       ( GetNumData () );

resampling.SetSampleSize    ( ResamplingEpochsSize );

resampling.SetNumResampling ( NumResamplingEpochs );

resampling.GetCoverage ();


SetDouble   ( ResamplingCoverage, RoundTo ( resampling.Coverage * 100, 0.1 ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // saving current state
SegPresetFilesSpec &presetf         = SegPresetsFiles[ PresetsFiles->GetSelIndex () ];

if      ( IsFlag ( presetf.FilesFlags, SegPresetPrioritySampleSize ) ) {

    if      ( presetf.SampleSizeSeconds )   presetf.SampleSizeSeconds   = TimeFrameToSeconds ( resampling.SampleSize, SamplingFrequency );
    else if ( presetf.SampleRatio       )   presetf.SampleRatio         = resampling.GetRelativeSampleSize ();
    }
else if ( IsFlag ( presetf.FilesFlags, SegPresetPriorityNumResampling ) )
                                            presetf.NumResampling       = resampling.NumResampling;

presetf.ResamplingCoverage  = resampling.Coverage;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

LockResampling  = false;
}


//----------------------------------------------------------------------------
                                        // When user is updating the coverage, don't change the sample size, but adjust the number of resampling
void    TMicroStatesSegFilesDialog::UpdateNumResampling ()
{
if ( GoGoF.IsEmpty () 
  || LockResampling 
  || ! CheckToBool ( ResamplingEpochs->GetCheck () )
  || IsFlag ( SegPresetsFiles[ PresetsFiles->GetSelIndex () ].FilesFlags, SegPresetPriorityNumResampling )  // forbid auto-updating the number of resampling for Grand Clustering, which should remain to 100 - or to user's choice either
   )
    return;

                                        // lock to prevent looping changes
LockResampling  = true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TResampling         resampling;
double              coverage;


coverage    = GetDouble ( ResamplingCoverage );

coverage    = Clip ( coverage / 100, 0.0, 1.0 );


resampling.SetNumData       ( GetNumData () );

resampling.SetSampleSize    ( ResamplingEpochsSize );

resampling.GetNumResampling ( coverage, /*SegmentMinNumResampling*/ 0, SegmentMaxNumResampling );


SetInteger  ( NumResamplingEpochs, resampling.NumResampling );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // test for clipped number of resampling, which should force update the coverage
                                        // Turned off for the moment, as it does not behave correctly as before
//if ( resampling.NumResampling == SegmentMinNumResampling 
//  || resampling.NumResampling == SegmentMaxNumResampling ) {
//
//    coverage    = resampling.GetCoverage ();
//                                        // !glitch: will force update itself again, and resulting display is all wrong!
//    SetDouble   ( ResamplingCoverage, RoundTo ( coverage * 100, 0.1 ) );
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // saving current state
SegPresetFilesSpec &presetf         = SegPresetsFiles[ PresetsFiles->GetSelIndex () ];

presetf.NumResampling       = resampling.NumResampling;
presetf.ResamplingCoverage  = coverage;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

LockResampling  = false;
}


//----------------------------------------------------------------------------
                                        // Used to adjust sample size to GfpMax case or not
void    TMicroStatesSegFilesDialog::UpdateSampleSize ()
{
SegPresetFilesSpec &presetf         = SegPresetsFiles[ PresetsFiles->GetSelIndex () ];


if ( LockResampling 
  || ! CheckToBool ( ResamplingEpochs->GetCheck () ) 
  || ! ( presetf.FilesFlags & SegPresetResampling ) 
   )
    return;

                                        // lock to prevent looping changes
LockResampling  = true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if      ( GoGoF.IsEmpty ()          )   ResamplingEpochsSize->SetText ( "" );

else if ( presetf.SampleSizeSeconds ) {

    SetSamplingFrequency ();

    SetInteger  ( ResamplingEpochsSize, SecondsToTimeFrame ( presetf.SampleSizeSeconds, SamplingFrequency ) );
    }

else if ( presetf.SampleRatio       )   

    SetInteger  ( ResamplingEpochsSize, Round ( presetf.SampleRatio * GetNumData () ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

LockResampling  = false;
}


//----------------------------------------------------------------------------
                                        // generate a smart base name
void    TMicroStatesSegFilesDialog::SetBaseFilename ()
{
if ( GoGoF.IsEmpty () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SegPresetFilesEnum  presetfile      = (SegPresetFilesEnum) PresetsFiles->GetSelIndex ();
char                prefix[ 256 ];


if      ( presetfile == SegPresetFilesERP                           )   StringCopy ( prefix, "Seg"      );
else if ( presetfile == SegPresetFilesRestingStatesSubjAllData      )   StringCopy ( prefix, "RS"       );
else if ( presetfile == SegPresetFilesRestingStatesSubjResampling   )   StringCopy ( prefix, "RSResamp" );
else if ( presetfile == SegPresetFilesRestingStatesGroupAllData     )   StringCopy ( prefix, "GC"       );
else if ( presetfile == SegPresetFilesRestingStatesGroupResampling  )   StringCopy ( prefix, "GCResamp" );
else                                                                    StringCopy ( prefix, "Seg"      );

StringAppend ( prefix, " " );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const TGoF&         gof         = GoGoF.GetFirst ();
TFileName           match;


if      ( presetfile == SegPresetFilesERP                 
       || presetfile == SegPresetFilesRestingStatesSubjAllData
       || presetfile == SegPresetFilesRestingStatesSubjResampling ) {

    gof.GetCommonString ( match, true /*, true*/ );
    }

else if ( presetfile == SegPresetFilesRestingStatesGroupAllData
       || presetfile == SegPresetFilesRestingStatesGroupResampling ) {
                                        // Re-use the individual RS Clustering directory name
                                        // adding "GC" and removing ".Best Clustering"
    StringCopy                      ( match, gof[ 0 ] );

    RemoveFilename                  ( match );

    AppendFilenameAsSubdirectory    ( match );

                                        // remove useless end of string
    if ( StringEndsWith ( match, "." InfixBestClustering ) )
        *StringContains ( match, '.', StringContainsBackward )  = EOS;
    }


if ( ! IsAbsoluteFilename ( match ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // finally, compose the base name
PrefixFilename ( match, prefix );

//DBGM ( match, "BaseFileName" );

BaseFileName->SetText ( match );

BaseFileName->ResetCaret;
}


//----------------------------------------------------------------------------
void    TMicroStatesSegFilesDialog::CmUpOneDirectory ()
{
RemoveLastDir ( SegTransfer.BaseFileName );

BaseFileName->SetText ( SegTransfer.BaseFileName );

BaseFileName->ResetCaret;
}


//----------------------------------------------------------------------------
void    TMicroStatesSegFilesDialog::CmRemoveGroup ()
{
if ( ! GoGoF.RemoveLastGroup () )
    return;

GroupsAllRis        = GoGoF.AllExtensionsAre ( FILEEXT_RIS );

SetSamplingFrequency ();

SetInteger  ( NumGroups, GoGoF.NumGroups () );

GroupsSummary->DeleteString ( 0 );

SetBaseFilename ();

UpdateSampleSize ();
}


void    TMicroStatesSegFilesDialog::CmClearGroups ()
{
GoGoF.Reset ();

GroupsAllRis        = GoGoF.AllExtensionsAre ( FILEEXT_RIS );

SetSamplingFrequency ();

SetInteger  ( NumGroups, GoGoF.NumGroups () );

GroupsSummary->ClearList ();

SetBaseFilename ();

UpdateSampleSize ();
}


void    TMicroStatesSegFilesDialog::CmSortGroups ()
{
if ( GoGoF.IsEmpty () )
    return;


GroupsSummary->ClearList ();


for ( int gofi = 0; gofi < (int) GoGoF; gofi++ ) {

    GoGoF[ gofi ].Sort ();

    AddGroupSummary ( gofi );
    }
}


//----------------------------------------------------------------------------
void    TMicroStatesSegFilesDialog::EvDropFiles ( TDropInfo drop )
{
TGoF                tracksfiles     ( drop, BatchFilesExt   );
TGoF                lmfiles         ( drop, FILEEXT_LM      );
TGoF                spreadsheetfiles( drop, SpreadSheetFilesExt );
TGoF                xyzfiles        ( drop, AllCoordinatesFilesExt );

char                buff[ 256 ];
StringCopy ( buff, BatchFilesExt, " " FILEEXT_LM " " SpreadSheetFilesExt " " AllCoordinatesFilesExt );
TGoF                remainingfiles  ( drop, buff, 0, true );

drop.DragFinish ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // xyz is not part of the current panel, but for convenience allow the user to drop here anyway
for ( int i = 0; i < (int) xyzfiles; i++ ) 
                                        // the file will be checked upon entering the parameters panel
    StringCopy ( SegTransfer.XyzFile, xyzfiles[ i ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) spreadsheetfiles; i++ )
    ReadParams ( spreadsheetfiles[ i ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) tracksfiles ) {

    for ( int i = 0; i < (int) tracksfiles; i++ )
        AddFileToGroup ( tracksfiles[ i ], i == 0 );


    if ( GoGoF.IsNotEmpty () ) {

        if ( CheckTracksGroup ( GoGoF.GetLast () ) )
            AddGroupSummary ( GoGoF.NumGroups () - 1 );
        else
            GoGoF.RemoveLastGroup ();
        }

//  CheckXyzFile ();    // should be in TMicroStatesSegDialog class...
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) lmfiles ) {

    bool                first;

    for ( int i = 0; i < (int) lmfiles; i++ ) {

        TSplitLinkManyFile      lm ( lmfiles[ i ] );
        TListIterator<char>     iterator;

                                        // coming from .lm, process separately the eeg
        first = true;

        if ( (bool) lm.leeg ) {

            foreachin ( lm.leeg, iterator )

                if ( ! IsExtension ( iterator (), FILEEXT_FREQ ) ) {    // filters out frequency files
                    AddFileToGroup ( iterator (), first );
                    first = false;
                    }

            if ( GoGoF.IsNotEmpty () && ! first ) { // security check, in case of only frequency files

                if ( CheckTracksGroup ( GoGoF.GetLast () ) )
                    AddGroupSummary ( GoGoF.NumGroups () - 1 );
                else
                    GoGoF.RemoveLastGroup ();
                }
            }

                                        // then process separately the ris
        first = true;

        if ( (bool) lm.lris ) {

            foreachin ( lm.lris, iterator ) {

                AddFileToGroup ( iterator (), first );
                first = false;
                }

            if ( GoGoF.IsNotEmpty () ) {

                if ( CheckTracksGroup ( GoGoF.GetLast () ) )
                    AddGroupSummary ( GoGoF.NumGroups () - 1 );
                else
                    GoGoF.RemoveLastGroup ();
                }
            }
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Let's be nice and try to load D&D Directories
                                        // For the moment, retrieving only the .ep files
for ( int i = 0; i < (int) remainingfiles; i++ ) {

    if ( ! IsDirectory ( remainingfiles[ i ] ) )
        continue;


    TGoF                    subgof;

    subgof.GrepFiles    ( remainingfiles[ i ], AllTracksFilesGrep, GrepOptionDefaultFiles, true );

//  subgof.Show ( "found eeg" );

                                    // process if found some files, otherwise, complain
    if ( (bool) subgof ) {

        for ( int sgi = 0; sgi < (int) subgof; sgi++ )
            AddFileToGroup ( subgof[ sgi ], sgi == 0 );


        if ( GoGoF.IsNotEmpty () ) {

            if ( CheckTracksGroup ( GoGoF.GetLast () ) )
                AddGroupSummary ( GoGoF.NumGroups () - 1 );
            else
                GoGoF.RemoveLastGroup ();
            }

        remainingfiles.RemoveRef ( remainingfiles[ i ] );
        i--;
        }
    }

                                        // Still remaining files? time to complain a bit, user always love a bit of whining once in a while...
if ( (bool) remainingfiles )

    remainingfiles.Show ( IrrelevantErrorMessage );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

UpdateSampleSize ();


SegPresetFilesSpec &presetf         = SegPresetsFiles[ PresetsFiles->GetSelIndex () ];

                                        // updating depends on parameter
if      ( IsFlag ( presetf.FilesFlags, SegPresetPrioritySampleSize    ) )   SampleSizeToNumResampling ();
else if ( IsFlag ( presetf.FilesFlags, SegPresetPriorityNumResampling ) )   UpdateResamplingCoverage  ();


GroupsAllRis        = GoGoF.AllExtensionsAre ( FILEEXT_RIS );

SetSamplingFrequency ();
}


//----------------------------------------------------------------------------
void    TMicroStatesSegFilesDialog::CmReadParams ()
{
ReadParams ();
}


void    TMicroStatesSegFilesDialog::AddFileToGroup ( const char* filename, bool first )
{
if ( GoGoF.IsEmpty () || first )        // add a new group
    GoGoF.Add ( new TGoF );


GoGoF.GetLast ().Add ( filename, MaxPathShort );

GroupsAllRis        = GoGoF.AllExtensionsAre ( FILEEXT_RIS );

SetSamplingFrequency ();
}


void    TMicroStatesSegFilesDialog::ReadParams ( char *filename )
{
                                        // TODO: glups .lm files also!!!!!!!!
TSpreadSheet        sf;

if ( ! sf.ReadFile ( filename ) )
    return;


CsvType             csvtype         = sf.GetType ();

                                        // We can deal with statistics type, too...
if ( ! ( IsCsvSegmentation ( csvtype ) || IsCsvStatFiles ( csvtype ) ) ) {
    ShowMessage ( SpreadSheetErrorMessage, "Read list file", ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           attr;
TFileName           buff;
TGoF*               gof;


for ( int file = 0; file < sf.GetNumRecords (); file++ ) {

    gof     = new TGoF;
    GoGoF.Add ( gof );


    sf.GetRecord ( file, "numfiles",    attr );
    int     numfiles    = StringToInteger ( attr );

                                        // transfer the filenames...
    for ( int i = 0; i < numfiles; i++ ) {

        StringCopy   ( buff, "file", IntegerToString ( i + 1 ) );

        sf.GetRecord ( file, buff, attr );

        gof->Add ( attr, MaxPathShort );
        } // for file

                                        // don't blindfully trust the parameter files(?)
    if ( GoGoF.IsNotEmpty () ) {

        if ( CheckTracksGroup ( GoGoF.GetLast () ) )
            AddGroupSummary ( GoGoF.NumGroups () - 1 );
        else
            GoGoF.RemoveLastGroup ();
        }
    }


GroupsAllRis        = GoGoF.AllExtensionsAre ( FILEEXT_RIS );

SetSamplingFrequency ();
}


void    TMicroStatesSegFilesDialog::CmWriteParams ()
{
if ( GoGoF.IsEmpty () )
    return;


int                 i;
TSpreadSheet        sf;


if ( ! sf.InitFile () )
    return;

                                        // header line describes the attributes / fields
sf.WriteAttribute ( "numfiles" );
                                        // we need to give the largest amount of files
int                     minnumfiles     = 0;

TListIterator<TGoF>     gofiterator;

foreachin ( GoGoF, gofiterator )
    minnumfiles = max ( minnumfiles, gofiterator()->NumFiles () );


for ( i = 0; i < minnumfiles; i++ )
    sf.WriteAttribute ( "file", i + 1 );

sf.WriteNextRecord ();

                                        // now write each line
foreachin ( GoGoF, gofiterator ) {
                                        // write parameters
    sf.WriteAttribute ( gofiterator ()->NumFiles () );

                                        // write all files
    TListIterator<char>     iterator;

    foreachin ( *gofiterator (), iterator )
        sf.WriteAttribute ( iterator () );

                                        // complete line with empty attributes
    for ( i = gofiterator ()->NumFiles (); i < minnumfiles; i++ )
        sf.WriteAttribute ( "" );

    sf.WriteNextRecord ();
    }


sf.WriteFinished ();
}


//----------------------------------------------------------------------------
void    TMicroStatesSegFilesDialog::CmAddEpoch ()
{
                                        // error if no TFs
if ( EpochFrom->GetTextLen() == 0 || EpochTo->GetTextLen() == 0 ) {
    ShowMessage ( "You must provide a starting and an ending TF.", "Add Epoch", ShowMessageWarning );
    return;
    }

char                from[ EditSizeValue ];
char                to  [ EditSizeValue ];
//int               delta;
//int               max = StringToInteger ( ((TMicroStatesSegStruct*) TransferBuffer)->MaxClusters );

                                        // transfer from edit to list
EpochFrom->GetText ( from, EditSizeValue );
EpochTo  ->GetText ( to,   EditSizeValue );

                                        // check boundaries
int                 ifrom           = AtLeast ( 0, StringToInteger ( from ) );
//else if ( ifrom >= MaxNumTF ) ifrom = MaxNumTF - 1;

int                 ito             = AtLeast ( 0, StringToInteger ( to   ) );
//else if ( ito >= MaxNumTF )   ito = MaxNumTF - 1;

CheckOrder ( ifrom, ito );

IntegerToString ( from, ifrom );
IntegerToString ( to,   ito   );

/*
                                        // we can't test anymore here, as we deal with batch segmentation
delta   = ito - ifrom + 1;
if ( max != 0 && delta < max ) {
    ShowMessage (   "You must provide an epoch longer" NewLine 
                    "than the maximum number of clusters!", 
                    "Add Epoch", ShowMessageWarning );
    return;
    }
*/

                                        // now transfer to the list
EpochFrom->InsertString ( from, 0 );
EpochTo  ->InsertString ( to,   0 );

                                        // clear the edit lines
EpochFrom ->Clear();
EpochTo   ->Clear();
EpochFrom->SetFocus();
}


void    TMicroStatesSegFilesDialog::CmRemoveEpoch ()
{
char                mess[ 256 ];

if ( EpochFrom->GetString ( mess, 0 ) < 0 )
    ClearString ( mess );
EpochFrom->DeleteString ( 0 );
EpochFrom->SetText ( mess );

if ( EpochTo->GetString ( mess, 0 ) < 0 )
    ClearString ( mess );
EpochTo->DeleteString ( 0 );
EpochTo->SetText ( mess );
}


void    TMicroStatesSegFilesDialog::CmListEpochsEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( ListEpochs->GetCheck () ) );
}


void    TMicroStatesSegFilesDialog::CmResamplingEpochsEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( ResamplingEpochs->GetCheck () ) );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TMicroStatesSegParamsDialog, TMicroStatesSegDialog )

    EV_WM_DROPFILES,

    EV_CBN_SELCHANGE            ( IDC_PRESETSPARAMS,            EvPresetsChange ),

//  EV_COMMAND_ENABLE           ( IDC_IGNOREPOLARITY,           CmKMeansEnable ),
    EV_COMMAND_ENABLE           ( IDC_RANDOMTRIALS,             CmKMeansEnable ),
    EV_COMMAND_ENABLE           ( IDC_CONVERGENCE,              CmKMeansEnable ),
    EV_COMMAND_ENABLE           ( IDC_SIMPLERFASTER,            CmHierarchicalEnable ),

    EV_COMMAND_ENABLE           ( IDC_GFPPEAKSAUTO,             CmGfpPeaksEnable ),
    EV_COMMAND_ENABLE           ( IDC_GFPPEAKSLIST,             CmGfpPeaksEnable ),
    EV_COMMAND_ENABLE           ( IDC_ANALYZEMARKERS,           CmGfpPeaksListEnable ),
    EV_COMMAND_ENABLE           ( IDC_SKIPBADEPOCHSAUTO,        CmSkipBadEpochsEnable ),
    EV_COMMAND_ENABLE           ( IDC_SKIPBADEPOCHSLIST,        CmSkipBadEpochsEnable ),
    EV_COMMAND_ENABLE           ( IDC_SKIPMARKERS,              CmSkipBadEpochsListEnable ),

    EV_COMMAND_ENABLE           ( IDC_WINDOWSIZE,               CmSmoothingEnable ),
    EV_COMMAND_ENABLE           ( IDC_STRENGTH,                 CmSmoothingEnable ),
    EV_COMMAND_ENABLE           ( IDC_REJECTSIZE,               CmRejectSmallEnable ),
    EV_EN_CHANGE                ( IDC_WINDOWSIZE,               SmoothingWindowsSizeChanged ),

    EV_COMMAND_ENABLE           ( IDC_MERGECORRTHRESH,          CmMergeCorrEnable ),
    EV_COMMAND_ENABLE           ( IDC_MINCORRELATION,           CmMinCorrelationEnable ),

    EV_COMMAND                  ( IDC_SPATIALFILTER,            CheckXyzFile ),     // clicking On will launch check-up
    EV_COMMAND                  ( IDC_BROWSEXYZFILE,            CmBrowseXyzFile ),
    EV_COMMAND_ENABLE           ( IDC_SPATIALFILTER,            CmNotESIEnable ),
    EV_COMMAND_ENABLE           ( IDC_BROWSEXYZFILE,            CmXyzEnable ),
    EV_COMMAND_ENABLE           ( IDC_XYZFILE,                  CmXyzEnable ),

    EV_COMMAND_ENABLE           ( IDOK,                         CmOkEnable ),

END_RESPONSE_TABLE;


        TMicroStatesSegParamsDialog::TMicroStatesSegParamsDialog ( TWindow* parent, TResId resId )
      : TMicroStatesSegDialog ( parent, resId )
{
ClearString ( BatchFilesExt );          // not used here
XyzAndEegMatch      = true;
GroupsAllRis        = GoGoF.AllExtensionsAre ( FILEEXT_RIS );


PresetsParams       = new TComboBox     ( this, IDC_PRESETSPARAMS );

PositiveData        = new TRadioButton  ( this, IDC_POSITIVEDATA );
SignedData          = new TRadioButton  ( this, IDC_SIGNEDDATA );
VectorData          = new TRadioButton  ( this, IDC_VECTORDATA );

NoRef               = new TRadioButton  ( this, IDC_NOREF );
AveRef              = new TRadioButton  ( this, IDC_AVEREF );

AccountPolarity     = new TRadioButton  ( this, IDC_ACCOUNTPOLARITY );
IgnorePolarity      = new TRadioButton  ( this, IDC_IGNOREPOLARITY );

KMeans              = new TRadioButton  ( this, IDC_KMEANS );
TAAHC               = new TRadioButton  ( this, IDC_TAAHC );

RandomTrials        = new TEdit         ( this, IDC_RANDOMTRIALS, EditSizeValue );

MinClusters         = new TEdit         ( this, IDC_MINCLUSTERS, EditSizeValue );
MaxClusters         = new TEdit         ( this, IDC_MAXCLUSTERS, EditSizeValue );

AllTFs              = new TRadioButton  ( this, IDC_ALLTFS );
GfpPeaksOnly        = new TRadioButton  ( this, IDC_ONLYGFPPEAKS );
GfpPeaksAuto        = new TRadioButton  ( this, IDC_GFPPEAKSAUTO );
GfpPeaksList        = new TRadioButton  ( this, IDC_GFPPEAKSLIST );
AnalyzeMarkers      = new TEdit         ( this, IDC_ANALYZEMARKERS, EditSizeText );

SkipBadEpochs       = new TCheckBox     ( this, IDC_SKIPBADEPOCHS );
SkipBadEpochsAuto   = new TRadioButton  ( this, IDC_SKIPBADEPOCHSAUTO );
SkipBadEpochsList   = new TRadioButton  ( this, IDC_SKIPBADEPOCHSLIST );
SkipMarkers         = new TEdit         ( this, IDC_SKIPMARKERS, EditSizeText );

SpatialFilter       = new TCheckBox     ( this, IDC_SPATIALFILTER );
XyzFile             = new TEdit         ( this, IDC_XYZFILE, EditSizeText );

ClipCorrelation     = new TCheckBox     ( this, IDC_CLIPCORRELATION );
MinCorrelation      = new TEdit         ( this, IDC_MINCORRELATION, EditSizeValue );
DualDataset         = new TCheckBox     ( this, IDC_DUALDATASET );

Sequentialize       = new TCheckBox     ( this, IDC_SEQUENTIALIZE );

MergeCorr           = new TCheckBox     ( this, IDC_MERGECORR );
MergeCorrThresh     = new TEdit         ( this, IDC_MERGECORRTHRESH, EditSizeValue );

Smoothing           = new TCheckBox     ( this, IDC_SMOOTHING );
WindowSize          = new TEdit         ( this, IDC_WINDOWSIZE, EditSizeValue );
BesagFactor         = new TEdit         ( this, IDC_STRENGTH, EditSizeValue );
RejectSmall         = new TCheckBox     ( this, IDC_REJECTSMALL );
RejectSize          = new TEdit         ( this, IDC_REJECTSIZE, EditSizeValue );


SetTransferBuffer ( dynamic_cast <TMicroStatesSegParamsStruct*> ( &SegTransfer ) );
}


        TMicroStatesSegParamsDialog::~TMicroStatesSegParamsDialog ()
{
delete  PresetsParams;
delete  PositiveData;           delete  SignedData;             delete  VectorData;
delete  NoRef;                  delete  AveRef;
delete  AccountPolarity;        delete  IgnorePolarity;
delete  KMeans;                 delete  TAAHC;
delete  RandomTrials;
delete  MinClusters;            delete  MaxClusters;
delete  AllTFs;                 delete  GfpPeaksOnly;           
delete  GfpPeaksAuto;           delete  GfpPeaksList;           delete  AnalyzeMarkers;         
delete  SkipBadEpochs;          
delete  SkipBadEpochsAuto;      delete  SkipBadEpochsList;      delete  SkipMarkers;
delete  SpatialFilter;          delete  XyzFile;
delete  ClipCorrelation;        delete  MinCorrelation;
delete  DualDataset;
delete  Sequentialize;
delete  MergeCorr;              delete  MergeCorrThresh;
delete  Smoothing;              delete  WindowSize;             delete  BesagFactor;
delete  RejectSmall;            delete  RejectSize;
}


void    TMicroStatesSegParamsDialog::SetupWindow ()
{
TMicroStatesSegDialog::SetupWindow ();
                                        // A .xyz file was previously set, went back to the files dialog, then again to the parameters:
                                        // (the reciprocate could be done when entering the Files dialog...)
TransferData ( tdSetData );

CheckXyzFile ();

                                        // when switching back to this panel, files might have changed
const SegPresetSpec&    preset          = SegPresets[ PresetsParams->GetSelIndex () ];
uint                    clflags         = preset.ClusteringFlags;


if ( ( clflags & SegPresetSpatialFilterOn ) && ! GroupsAllRis )
                                        // reset Spatial Filter if file names already contain .SpatialFilter
    SpatialFilter   ->SetCheck ( ! GoGoF.AllStringsGrep ( PostfixSpatialGrep, GrepOptionDefaultFiles ) );


if ( preset.RandomTrials > 0 ) {
                                        // reduce the number of random trials in case of resampling
    double          resamplingratio     = IsSegResamplingFilePreset ( (SegPresetFilesEnum) SegTransfer.PresetsFiles.GetSelIndex () ) ? SegmentResamplingRandomTrialsRatio : 1;

    SetInteger  ( RandomTrials, Round ( preset.RandomTrials * resamplingratio ) );
    }


auto&               tooltip         = *TTooltip::Make ( this );

tooltip.AddTool (   GetHandle (),   IDC_KMEANS,                 "Well-known clusering method, but will return different results on each run" );
tooltip.AddTool (   GetHandle (),   IDC_TAAHC,                  "Dedicated Hierarchical Clustering that will always return the same results on each run" );
tooltip.AddTool (   GetHandle (),   IDC_ALLTFS,                 "Using all data provided" );
tooltip.AddTool (   GetHandle (),   IDC_ONLYGFPPEAKS,           "We can shrink the data 6 fold by using only the GFP peaks data points" );
tooltip.AddTool (   GetHandle (),   IDC_SKIPBADEPOCHS,          "Skipping artefacted time periods is definitely a good idea!" );
tooltip.AddTool (   GetHandle (),   IDC_SPATIALFILTER,          "Spatial filter is recommended no matter what - just make sure you apply it only once, though..." );
tooltip.AddTool (   GetHandle (),   IDC_CLIPCORRELATION,        "Data which don't fit well to any cluster will be unlabelled" );
tooltip.AddTool (   GetHandle (),   IDC_DUALDATASET,            "For EEG clustering, computes the corresponding RIS templates; for RIS clustering, computes the corresponding EEG templates" );
tooltip.AddTool (   GetHandle (),   IDC_SEQUENTIALIZE,          "For ERP, force split clusters that span over disjointed time epochs" );
tooltip.AddTool (   GetHandle (),   IDC_MERGECORR,              "Force merge correlated clusters - Do NOT use unless you know what you are doing!" );
tooltip.AddTool (   GetHandle (),   IDC_SMOOTHING,              "Remove small segments, the gentle way" );
tooltip.AddTool (   GetHandle (),   IDC_REJECTSMALL,            "Remove small segments, the hard way" );
}


//----------------------------------------------------------------------------
void    TMicroStatesSegParamsDialog::EvPresetsChange ()
{
SegPresetParamsEnum     presetparam     = (SegPresetParamsEnum) PresetsParams->GetSelIndex ();
const SegPresetSpec&    preset          = SegPresets[ presetparam ];
uint                    clflags         = preset.ClusteringFlags;
uint                    ppflags         = preset.PostProcFlags;


if ( clflags == 0 )
    return;

                                        // will set and/or reset all our options
KMeans          ->SetCheck ( BoolToCheck ( clflags & SegPresetKMeans             ) );
TAAHC           ->SetCheck ( BoolToCheck ( clflags & SegPresetTAAHC              ) );

AllTFs          ->SetCheck ( BoolToCheck ( clflags & SegPresetAllData            ) );
GfpPeaksOnly    ->SetCheck ( BoolToCheck ( clflags & SegPresetGfpPeaksOnly       ) );
SkipBadEpochs   ->SetCheck ( BoolToCheck ( clflags & SegPresetRSIndiv            ) );

SpatialFilter   ->SetCheck ( BoolToCheck ( clflags & SegPresetSpatialFilterOn    ) && ! GroupsAllRis );

PositiveData    ->SetCheck ( BoolToCheck ( clflags & SegPresetPositiveData       ) );
SignedData      ->SetCheck ( BoolToCheck ( clflags & SegPresetSignedData         ) );
VectorData      ->SetCheck ( BoolToCheck ( clflags & SegPresetVectorData         ) );

AccountPolarity ->SetCheck ( BoolToCheck ( clflags & SegPresetAccountPolarity    ) );
IgnorePolarity  ->SetCheck ( BoolToCheck ( clflags & SegPresetIgnorePolarity     ) );

NoRef           ->SetCheck ( BoolToCheck ( clflags & SegPresetNoRef              ) );
AveRef          ->SetCheck ( BoolToCheck ( clflags & SegPresetAveRef             ) );



if ( ( clflags & SegPresetSpatialFilterOn ) && ! GroupsAllRis )
                                        // reset Spatial Filter if file names already contain .SpatialFilter
    SpatialFilter   ->SetCheck ( ! GoGoF.AllStringsGrep ( PostfixSpatialGrep, GrepOptionDefaultFiles ) );


//                                        // ERP needs global regularization?
//if ( IsInsideLimits ( presetparam, SegPresetESIRange1, SegPresetESIRange2 ) ) {
//                                        // "manually" setting the regularization - maybe add a preset later on
//    if      ( presetparam == SegPresetRisScalErpTAAHC 
//           || presetparam == SegPresetRisScalErpKMeans )    
//                                        // ERP with global reg?
//        Regularization->SetText ( RegularizationAutoGlobalStringL );
//    else
//        Regularization->SetText ( RegularizationAutoLocalStringL );
//    }


Sequentialize   ->SetCheck ( BoolToCheck ( ppflags & SegPresetSequentializeOn    ) );    // can also test for  SegPresetSequentializeOff

if ( ! ( ppflags & SegPresetSmoothingDontCare ) )
    Smoothing   ->SetCheck ( BoolToCheck ( ppflags & SegPresetSmoothingOn        ) );    // can also test for  SegPresetSmoothingOff

if ( ! ( ppflags & SegPresetRejectSmallDontCare ) )
    RejectSmall ->SetCheck ( BoolToCheck ( ppflags & SegPresetRejectSmallOn      ) );    // can also test for  SegPresetRejectSmallOff

if ( ! ( ppflags & SegPresetMergeCorrDontCare ) )
    MergeCorr   ->SetCheck ( BoolToCheck ( ppflags & SegPresetMergeCorrOn        ) );    // can also test for  SegPresetMergeCorrOff


if ( preset.RandomTrials > 0 ) {
                                        // reduce the number of random trials in case of resampling
    double          resamplingratio     = IsSegResamplingFilePreset ( (SegPresetFilesEnum) SegTransfer.PresetsFiles.GetSelIndex () ) ? SegmentResamplingRandomTrialsRatio : 1;

    SetInteger  ( RandomTrials, Round ( preset.RandomTrials * resamplingratio ) );
    }

SetInteger  ( MinClusters, preset.MinClusters );
SetInteger  ( MaxClusters, preset.MaxClusters );


if ( preset.SmoothingSize > 0 ) {
    SetInteger  ( WindowSize, preset.SmoothingSize );
    SetInteger  ( RejectSize, preset.SmoothingSize );
    }


if ( preset.CorrelationThreshold > -100 ) {
    ClipCorrelation ->SetCheck ( BoolToCheck ( true ) );
    SetInteger  ( MinCorrelation, Clip ( preset.CorrelationThreshold, -100, 100 ) );
    }
else
    ClipCorrelation ->SetCheck ( BoolToCheck ( false ) );


if ( IsSegESIPreset ( presetparam ) )
    DualDataset->SetCheck ( BoolToCheck ( true ) );
}


//----------------------------------------------------------------------------
void    TMicroStatesSegParamsDialog::CmKMeansEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( KMeans->GetCheck () ) );
}


void    TMicroStatesSegParamsDialog::CmHierarchicalEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! CheckToBool ( KMeans->GetCheck () ) );
}


void    TMicroStatesSegParamsDialog::CmSmoothingEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( Smoothing->GetCheck () ) );
}


void    TMicroStatesSegParamsDialog::CmRejectSmallEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( RejectSmall->GetCheck () ) );
}


void    TMicroStatesSegParamsDialog::CmMergeCorrEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( MergeCorr->GetCheck () ) );
}


void    TMicroStatesSegParamsDialog::CmMinCorrelationEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( ClipCorrelation->GetCheck () ) );
}


void    TMicroStatesSegParamsDialog::CmGfpPeaksEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( GfpPeaksOnly->GetCheck () ) );
}


void    TMicroStatesSegParamsDialog::CmGfpPeaksListEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( GfpPeaksOnly->GetCheck () ) && CheckToBool ( GfpPeaksList->GetCheck () ) );
}


void    TMicroStatesSegParamsDialog::CmSkipBadEpochsEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( SkipBadEpochs->GetCheck () ) );
}


void    TMicroStatesSegParamsDialog::CmSkipBadEpochsListEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( SkipBadEpochs->GetCheck () ) && CheckToBool ( SkipBadEpochsList->GetCheck () ) );
}


void    TMicroStatesSegParamsDialog::CmNotESIEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! SegTransfer.IsSegESIPreset () && ! GroupsAllRis );
}


//----------------------------------------------------------------------------
/*void    TMicroStatesSegParamsDialog::CmBrowseGreyFile ()
{
SetGreyFile ( 0 );
}


void    TMicroStatesSegParamsDialog::SetGreyFile ( char *file )
{
static GetFileFromUser  getfile ( "Grey Mask MRI File:", AllMriFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( SegTransfer.GreyFile ) )
        return;

    TransferData ( tdSetData );
    }
else {
    StringCopy ( SegTransfer.GreyFile, file );
    getfile.SetOnly ( file );
    }


TransferData ( tdSetData );


ISFile  ->ResetCaret;
GreyFile->ResetCaret;
SPFile  ->ResetCaret;
}


//----------------------------------------------------------------------------
void    TMicroStatesSegParamsDialog::CmBrowseSPFile ()
{
SetSPFile ( 0 );
}


void    TMicroStatesSegParamsDialog::SetSPFile ( char *file )
{
static GetFileFromUser  getfile ( "Solution Points File:", AllSolPointsFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( SegTransfer.SPFile ) )
        return;

    TransferData ( tdSetData );
    }
else {
    StringCopy ( SegTransfer.SPFile, file );
    getfile.SetOnly ( file );
    }


TransferData ( tdSetData );


ISFile  ->ResetCaret;
GreyFile->ResetCaret;
SPFile  ->ResetCaret;


//CheckInverseSolPointsFile ();
}
*/

//----------------------------------------------------------------------------
void    TMicroStatesSegParamsDialog::SmoothingWindowsSizeChanged ()
{
if ( ! ( BoolToCheck ( Smoothing->GetCheck () ) && BoolToCheck ( RejectSmall->GetCheck () ) ) )
    return;


char                buff[ 256 ];

WindowSize->GetText ( buff, 256 );

if ( ! StringIsSpace ( buff ) )
    RejectSize->SetText ( buff );
}


//----------------------------------------------------------------------------
void    TMicroStatesSegParamsDialog::CmBrowseXyzFile ()
{
SetXyzFile ( 0 );
}


void    TMicroStatesSegParamsDialog::SetXyzFile ( char *file )
{
static GetFileFromUser  getfile ( "Electrodes Coordinates File", AllCoordinatesFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( SegTransfer.XyzFile ) )
        return;

    TransferData ( tdSetData );
    }
else {
    StringCopy ( SegTransfer.XyzFile, file );
    getfile.SetOnly ( file );
    }


TransferData ( tdSetData );

CheckXyzFile ();

XyzFile->ResetCaret;
}


void    TMicroStatesSegParamsDialog::CheckXyzFile ()
{
TransferData ( tdGetData );

XyzAndEegMatch      = true;

if ( /*! CheckToBool ( SpatialFilter->GetCheck () ) ||*/ StringIsEmpty ( SegTransfer.XyzFile ) || GoGoF.IsEmpty () )
    return;


TPoints             xyz ( SegTransfer.XyzFile );
int                 xyznumel        = xyz.GetNumPoints ();


TracksCompatibleClass   tc;

GoGoF.AllTracksAreCompatible ( tc );


if      ( tc.NumTracks != xyznumel ) {

    XyzAndEegMatch      = false;

    char                buff[ 256 ];

    StringCopy      ( buff, "Not the same amount of Electrodes:" NewLine );
    StringAppend    ( buff, NewLine  );
    StringAppend    ( buff, "  - EEG files "       Tab "= ", IntegerToString ( tc.NumTracks ), NewLine );
    StringAppend    ( buff, "  - Electrodes file " Tab "= ", IntegerToString ( xyznumel     ), NewLine );
    StringAppend    ( buff, NewLine  );
    StringAppend    ( buff, "Please check again your files!" );

    ShowMessage     ( buff, SegmentationTitle, ShowMessageWarning, this );

    ClearString ( SegTransfer.XyzFile );
    TransferData ( tdSetData );
    }

}


void    TMicroStatesSegParamsDialog::CmXyzEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

//tce.Enable ( CheckToBool ( SpatialFilter->GetCheck () && ! GroupsAllRis ) 
//          && ! SegTransfer.IsSegESIPreset () );
                                        // allow Spatial Filtering before ESI
tce.Enable ( CheckToBool ( SpatialFilter->GetCheck () ) && ! GroupsAllRis );
}


//----------------------------------------------------------------------------
void    TMicroStatesSegParamsDialog::EvDropFiles ( TDropInfo drop )
{
//TGoF                mrifiles        ( drop, AllMriFilesExt          );
//TGoF                spfiles         ( drop, AllSolPointsFilesExt    );
TGoF                xyzfiles        ( drop, AllCoordinatesFilesExt  );
TGoF                remainingfiles  ( drop, AllInverseFilesExt " " AllCoordinatesFilesExt, 0, true );

drop.DragFinish ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//for ( int i = 0; i < (int) mrifiles; i++ ) {
//
//    SetGreyFile ( mrifiles[ i ] );
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//for ( int i = 0; i < (int) spfiles; i++ ) {
//
//    SetSPFile ( spfiles[ i ] );
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) xyzfiles; i++ ) {

    SetXyzFile ( xyzfiles[ i ] );

    SpatialFilter->SetCheck ( BoolToCheck ( XyzAndEegMatch ) );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) remainingfiles )
    remainingfiles.Show ( IrrelevantErrorMessage );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TMicroStatesSegDialog, TBaseDialog )

//    EV_WM_DROPFILES,

    EV_COMMAND                  ( IDOK,                     CmOk ),
    EV_COMMAND_ENABLE           ( IDOK,                     CmOkEnable ),

    EV_COMMAND_AND_ID           ( IDC_TOPREVDIALOG,         CmToDialog ),
    EV_COMMAND_AND_ID           ( IDC_TONEXTDIALOG,         CmToDialog ),

END_RESPONSE_TABLE;


        TMicroStatesSegDialog::TMicroStatesSegDialog ( TWindow* parent, TResId resId )
      : TBaseDialog ( parent, resId )
{
StringCopy ( BatchFilesExt, AllEegRisFilesExt );
}


//        TMicroStatesSegDialog::~TMicroStatesSegDialog ()
//{
//}


//----------------------------------------------------------------------------
void    TMicroStatesSegDialog::CmToDialog ( owlwparam w )
{
uint                ResId           = PtrToUint ( Attr.Name );

                                        // goto new id
uint                todialogid      = Clip ( ResId + ( w == IDC_TOPREVDIALOG ? -1 : 1 ), (uint) IDD_SEGMENT1, (uint) IDD_SEGMENT2 );

                                        // avoid calling itself
if ( todialogid == ResId )
    return;


Destroy ();                             // remove the window, not the object

                                        // remember the last dialog
SegTransfer.LastDialogId    = todialogid;


if      ( todialogid == IDD_SEGMENT1 )  TMicroStatesSegFilesDialog  ( CartoolMdiClient, IDD_SEGMENT1 ).Execute ();
else if ( todialogid == IDD_SEGMENT2 )  TMicroStatesSegParamsDialog ( CartoolMdiClient, IDD_SEGMENT2 ).Execute ();
}


//----------------------------------------------------------------------------
void    TMicroStatesSegDialog::CmOkEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );


if ( GoGoF.IsEmpty () ) {
    tce.Enable ( false );
    return;
    }


int                 min             = StringToInteger ( SegTransfer.MinClusters );
int                 max             = StringToInteger ( SegTransfer.MaxClusters );

if ( min <= 0 || max <= 0 || min > max ) {
    tce.Enable ( false );
    return;
    }


//if ( min > NumTimeFrames || min >= NumElectrodes - 1
//  || max > NumTimeFrames || max >= NumElectrodes - 1 ) {
//    tce.Enable ( false );
//    return;
//    }
//
//if ( min > NumTimeFrames
//  || max > NumTimeFrames ) {
//    tce.Enable ( false );
//    return;
//    }


int                 ntr             = StringToInteger ( SegTransfer.RandomTrials );

if ( ntr < 1 ) {
    tce.Enable ( false );
    return;
    }

if ( CheckToBool ( SegTransfer.SpatialFilter ) && ( ! XyzAndEegMatch || StringIsEmpty ( SegTransfer.XyzFile ) ) ) {
    tce.Enable ( false );
    return;
    }

if ( CheckToBool ( SegTransfer.GfpPeaksOnly ) && CheckToBool ( SegTransfer.GfpPeaksList ) && StringIsEmpty ( SegTransfer.AnalyzeMarkers ) ) {
    tce.Enable ( false );
    return;
    }

if ( CheckToBool ( SegTransfer.SkipBadEpochs ) && CheckToBool ( SegTransfer.SkipBadEpochsList ) && StringIsEmpty ( SegTransfer.SkipMarkers ) ) {
    tce.Enable ( false );
    return;
    }

if ( SegTransfer.IsSegESIPreset () && ! GroupsAllRis ) {
    tce.Enable ( false );
    return;
    }

if ( CheckToBool ( SegTransfer.ClipCorrelation ) && StringIsEmpty ( SegTransfer.MinCorrelation ) ) {
    tce.Enable ( false );
    return;
    }


int                 win             = StringToInteger ( SegTransfer.WindowSize );
double              besag           = StringToDouble  ( SegTransfer.BesagFactor );

if ( CheckToBool ( SegTransfer.Smoothing ) && ( win <= 0 || besag <= 0 ) ) {
    tce.Enable ( false );
    return;
    }


int                 ssz             = StringToInteger ( SegTransfer.RejectSize );

if ( CheckToBool ( SegTransfer.RejectSmall ) && ssz <= 0 ) {
    tce.Enable ( false );
    return;
    }


int                 resnumepochs    = StringToInteger ( SegTransfer.NumResamplingEpochs );
int                 resepochsize    = StringToInteger ( SegTransfer.ResamplingEpochsSize );

if ( CheckToBool ( SegTransfer.ResamplingEpochs ) && ( resnumepochs <= 0 || resepochsize <= 0 ) ) {
    tce.Enable ( false );
    return;
    }

                                        // directly playing with the buffer does not seem to be a good idea, maybe its updated during the test?
TFileName           buff;
StringCopy ( buff, SegTransfer.BaseFileName );

if ( ! IsAbsoluteFilename ( buff ) ) {
    tce.Enable ( false );
    return;
    }


tce.Enable ( true );
}


//----------------------------------------------------------------------------
                                        // Preprocess the input groups of files to get ready for a segmentation
                                        // which means, epochs, filter, do ESI stuff etc..., creating temp files if needed
                                        // then calling the segmentation on these files
                                        // It also updates the base file name accordingly.
OptimizeOff

void    TMicroStatesSegDialog::CmOk ()
{
Destroy ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Set epochs: whole time range vs list of epochs
EpochsType          epochs;

if      ( CheckToBool ( SegTransfer.ListEpochs          ) )     epochs  = EpochsFromList;
else if ( CheckToBool ( SegTransfer.ResamplingEpochs    ) )     epochs  = EpochWholeTime;
else    /*CheckToBool ( SegTransfer.NoEpochs )*/                epochs  = EpochWholeTime;

TStrings            epochfrom;
TStrings            epochto;

if      ( epochs == EpochsFromList ) {

    epochfrom.Set ( SegTransfer.EpochFrom.GetStrings () );
    epochto  .Set ( SegTransfer.EpochTo  .GetStrings () );

                                        // check for input parameters consistency
    if ( epochfrom.IsEmpty () || (int) epochfrom != (int) epochto ) {
        epochs      = EpochWholeTime;   // reset flag!
        epochfrom.Reset ();
        epochto  .Reset ();
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Once epochs are set, we can optionally extract the GFP Peaks
GfpPeaksDetectType  gfppeaks    = ! CheckToBool ( SegTransfer.GfpPeaksOnly  ) ? NoGfpPeaksDetection
                                :   CheckToBool ( SegTransfer.GfpPeaksAuto  ) ? GfpPeaksDetectionAuto
                                :   CheckToBool ( SegTransfer.GfpPeaksList  ) ? GfpPeaksDetectionList
                                :                                               NoGfpPeaksDetection;


char                listgfppeaks  [ 4 * KiloByte ];

if ( gfppeaks == GfpPeaksDetectionList ) {

    StringCopy ( listgfppeaks,    SegTransfer.AnalyzeMarkers );

    if ( StringIsEmpty ( listgfppeaks ) )
        gfppeaks    = GfpPeaksDetectionAuto;
    }
else
    ClearString ( listgfppeaks );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Finally we can set resample data from previous steps
                                        // We currently enforce  gfppeaks  and  resampling  to be exclusive
                                        // However, it is allowed to have  gfppeaks  and  resampling, though
ResamplingType      resampling;

if      ( CheckToBool ( SegTransfer.ListEpochs          ) )     resampling  = NoTimeResampling;
else if ( CheckToBool ( SegTransfer.ResamplingEpochs    ) )     resampling  = TimeResampling;
else    /*CheckToBool ( SegTransfer.NoEpochs )*/                resampling  = NoTimeResampling;

                                        // shouldn't happen, but let's make sure we have exclusive options for the moment (would be technically possible, though)
if ( epochs == EpochsFromList && resampling == TimeResampling )
    resampling  = NoTimeResampling;     // reset flag!


int                 numresampling       = resampling == TimeResampling ? StringToInteger ( SegTransfer.NumResamplingEpochs  ) : 0;
int                 reqresamplingsize   = resampling == TimeResampling ? StringToInteger ( SegTransfer.ResamplingEpochsSize ) : 0;
int                 resamplingsize      = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SkippingEpochsType  badepochs       = ! CheckToBool ( SegTransfer.SkipBadEpochs     ) ? NoSkippingBadEpochs
                                    :   CheckToBool ( SegTransfer.SkipBadEpochsAuto ) ? SkippingBadEpochsAuto
                                    :   CheckToBool ( SegTransfer.SkipBadEpochsList ) ? SkippingBadEpochsList
                                    :                                                   NoSkippingBadEpochs;


char                listbadepochs [ EditSizeText ];
ClearString ( listbadepochs );

if ( badepochs == SkippingBadEpochsList ) {

    StringCopy      ( listbadepochs,    SegTransfer.SkipMarkers );
    StringCleanup   ( listbadepochs );

    if ( StringIsEmpty ( listbadepochs ) )
        badepochs   = NoSkippingBadEpochs;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SegPresetFilesEnum  presetfile      = (SegPresetFilesEnum ) SegTransfer.PresetsFiles .GetSelIndex ();
SegPresetParamsEnum presetparam     = (SegPresetParamsEnum) SegTransfer.PresetsParams.GetSelIndex ();

TypeFilesFlags      fflags          = SegPresetsFiles[ presetfile  ].FilesFlags;
TypeClusteringFlags cflags          = SegPresets     [ presetparam ].ClusteringFlags;

                                        // transform presets into parameters (could also be directly defines in predefined presets):
AnalysisType        analysis        = IsFlag ( cflags, SegPresetERP     )         ? AnalysisERP
                                    : IsFlag ( cflags, SegPresetRSIndiv )         ? AnalysisRestingStatesIndiv
                                    : IsFlag ( cflags, SegPresetRSGroup )         ? AnalysisRestingStatesGroup
                                    :                                               UnknownAnalysis;
                                                                                  
ModalityType        modality        = IsSegEEGPreset ( presetparam )              ? ModalityEEG
                                    : IsSegMEGPreset ( presetparam )              ? ModalityMEG
                                    : IsSegESIPreset ( presetparam )              ? ModalityESI
                                    :                                               UnknownModality;

SamplingTimeType    time            = IsFlag ( fflags, SegPresetWholeTimeRange  ) ? SamplingTimeWhole
                                    : IsFlag ( fflags, SegPresetResampling      ) ? SamplingTimeResampling
                                    :                                               UnknownSamplingTime;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check groups are all of the same type, that is not a good idea to mix ESI and EEG!
bool                alleeg          = GoGoF.AllExtensionsAre ( AllEegFilesExt );
bool                allris          = GoGoF.AllExtensionsAre ( AllRisFilesExt );
bool                allrisv         = GoGoF.AllExtensionsAre ( AllRisFilesExt, AtomTypeVector );


if ( ! ( alleeg || allris ) ) {
    ShowMessage (   "You're mixing different types in your groups of files,"    NewLine 
                    "like EEG and ESI together, which you shouldn't."           NewLine 
                    "Please check your input groups and come back...", 
                    SegmentationTitle, ShowMessageWarning );
    return;
    }


//if ( allrisv ) {
//
//    ShowMessage (     "Inverse files of vectorial type are currently not allowed for the segmentation." NewLine 
//                      "Please check your input groups or convert your files to scalar and come back...", 
//                      SegmentationTitle, ShowMessageWarning );
//    return;
//    }

/*                                        // Test is done for ERPs and Individual Resting States only
if ( allris && ! IsSegGroupPreset ( presetparam ) ) {

    char            searchzscorepo[ 256 ];
//  char            searchzscorepa[ 256 ];
//  char            searchzscorepp[ 256 ];
//  char            searchzscorevs[ 256 ];


    StringCopy          ( searchzscorepo, ".", ZScoreEnumToInfix ( ZScorePositive_CenterScaleOffset ) );
//  StringCopy          ( searchzscorepa, ".", ZScoreEnumToInfix ( ZScorePositive_CenterScaleAbs    ) );
//  StringCopy          ( searchzscorepp, ".", ZScoreEnumToInfix ( ZScorePositive_CenterScalePlus   ) );
//  StringCopy          ( searchzscorevs, ".", ZScoreEnumToInfix ( ZScoreVectorial_CenterVectors_CenterScale ) );  // equivalent to ZScorePositive_CenterScaleOffset on the norm of vectors

    StringGrepNeutral   ( searchzscorepo );
//  StringGrepNeutral   ( searchzscorepa );
//  StringGrepNeutral   ( searchzscorepp );
//  StringGrepNeutral   ( searchzscorevs );

    bool            allriszscorepo      = GoGoF.AllStringsGrep ( searchzscorepo );
//  bool            allriszscorepa      = GoGoF.AllStringsGrep ( searchzscorepa );
//  bool            allriszscorepp      = GoGoF.AllStringsGrep ( searchzscorepp );
//  bool            allriszscorevs      = GoGoF.AllStringsGrep ( searchzscorevs );


    if ( ! allriszscorepo ) {

        ShowMessage (   "The .ris inverse files don't seem to have the proper Background Normalization." NewLine 
                        "Please check your input groups and come back...", 
                        SegmentationTitle, ShowMessageWarning );
        return;
        }

    }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Set user's wish, but this will be checked against a few mandatory conditions
SpatialFilterType   spatialfilter   =       CheckToBool ( SegTransfer.SpatialFilter ) 
                                    && StringIsNotEmpty ( SegTransfer.XyzFile       ) ? SpatialFilterDefault : SpatialFilterNone;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check ESI parameters
bool                isesipreset     = IsSegESIPreset ( presetparam );


if ( isesipreset  ) {
                                        // either we load ris files, or we compute them, all other cases will fail...
    if ( ! allris ) {
        ShowMessage ( "Sorry, can't process in the ESI space with the given parameters!", SegmentationTitle, ShowMessageWarning );
        return;
        }

                                            // We currently don't have a 3D filter for ESI in terms of Solution Points...
    if ( spatialfilter != SpatialFilterNone && allris )
        spatialfilter   = SpatialFilterNone;

    } // if isesipreset


if ( ! isesipreset && allris  ) {

    ShowMessage ( "Sorry, you have to select some ESI preset to match your input .ris files!", SegmentationTitle, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Data type & reference

                                        // We have to link datatype and reference: average ref <-> scalar, so that the norm of data corresponds to the GFP, not the RMS
AtomType            datatype;
if      ( CheckToBool ( SegTransfer.PositiveData   ) )      datatype    = AtomTypePositive;
else if ( CheckToBool ( SegTransfer.SignedData     ) )      datatype    = AtomTypeScalar;
else if ( CheckToBool ( SegTransfer.VectorData     ) )      datatype    = AtomTypeVector;
else                                                        datatype    = AtomTypeScalar;

                                        // downgrade vectorial type to scalar if not all data are vectorial
if ( IsVector ( datatype ) && ! allrisv )                   datatype    = AtomTypePositive;

                                        // this also shouldn't happen...
if ( IsVector ( datatype ) && spatialfilter != SpatialFilterNone )
    spatialfilter   = SpatialFilterNone;

                                        // Note: no reference processing is done here, we just use ReferenceNone all along

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Be general here, we might need some Butterworth or other temporal filter later
bool                timelyfilter        = spatialfilter != SpatialFilterNone;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                                                                   // not Positive and not Vectorial cases
//PolarityType      polarity        = CheckToBool ( SegTransfer.IgnorePolarity ) && ! IsAbsolute ( datatype ) ? PolarityEvaluate : PolarityDirect;
                                                                                   // not Positive case, Vectorial case is dealt later on
                                                                                   // We should have more presets, like EvaluateMap vs EvaluateSP, or EvaluateGlobal vs EvaluateLocal
PolarityType        polarity        = CheckToBool ( SegTransfer.IgnorePolarity ) && ! IsPositive ( datatype ) ? PolarityEvaluate : PolarityDirect;

                                        // Special case: vectorial inverse space and ignore polarity
if ( polarity == PolarityEvaluate && IsVector ( datatype ) )
                                        // cancel polarity - !it would be cleaner to use another variable like 'processingpolarity', or having another type!
    polarity = PolarityDirect;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ReferenceType       dataref;
if        ( CheckToBool ( SegTransfer.AveRef ) )            dataref     = ReferenceAverage;
else /*if ( CheckToBool ( SegTransfer.NoRef  ) )*/          dataref     = ReferenceAsInFile;


CheckReference ( dataref, datatype );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Set the segmentation type
ClusteringType      clusteringmethod;


if      ( CheckToBool ( SegTransfer.KMeans ) )  clusteringmethod = ClusteringKMeans;
else if ( CheckToBool ( SegTransfer.TAAHC  ) )  clusteringmethod = ClusteringTAAHC;
else                                            clusteringmethod = UnknownClustering;

                                        // retrieve user's request for range of clustering, even if out of actual bounds
int                 reqminclusters      = StringToInteger ( SegTransfer.MinClusters );
int                 reqmaxclusters      = StringToInteger ( SegTransfer.MaxClusters );


int                 numrandomtrials     = StringToInteger ( SegTransfer.RandomTrials );
CentroidType        centroid            = isesipreset ? ESICentroidMethod
                                                      : EEGCentroidMethod;


bool                dolimitcorr         = CheckToBool ( SegTransfer.ClipCorrelation );
double              limitcorr           = dolimitcorr   ? Clip ( StringToDouble ( SegTransfer.MinCorrelation ) / 100.0, MinCorrelationThreshold, MaxCorrelationThreshold ) 
                                                        : IgnoreCorrelationThreshold;
                                        // force reset flag?
if ( limitcorr <= MinCorrelationThreshold ) {
    dolimitcorr     = false;
    limitcorr       = IgnoreCorrelationThreshold;
    }



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Process files one by one, for all groups

bool                sequentialize       = CheckToBool ( SegTransfer.Sequentialize );

bool                mergecorr           = CheckToBool ( SegTransfer.MergeCorr );
double              mergecorrthresh     = StringToDouble ( SegTransfer.MergeCorrThresh ) / 100.0;

bool                rejectsmall         = CheckToBool ( SegTransfer.RejectSmall );
int                 rejectsize          = StringToInteger ( SegTransfer.RejectSize );

bool                smoothing           = CheckToBool ( SegTransfer.Smoothing );
int                 smoothinghalfsize   = StringToInteger ( SegTransfer.WindowSize );
double              smoothinglambda     = StringToInteger ( SegTransfer.BesagFactor );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Set the many output flags
MicroStatesOutFlags outputflags         = NoMicroStatesOutFlags;

if ( CheckToBool ( SegTransfer.WriteSeg             ) )     SetFlags ( outputflags, WriteSegFiles               );
if ( CheckToBool ( SegTransfer.WriteMaps            ) )     SetFlags ( outputflags, WriteTemplatesFiles         );
if ( CheckToBool ( SegTransfer.WriteClusters        ) )     SetFlags ( outputflags, WriteClustersFiles          );
//if ( CheckToBool ( SegTransfer.WriteDispersion    ) )     SetFlags ( outputflags, WriteDispersionFiles        );
if ( AllowEmptyClusterFiles                         )       SetFlags ( outputflags, WriteEmptyClusters          );  // controlled by global variable
if ( SavingNormalizedClusters                       )       SetFlags ( outputflags, WriteNormalizedClusters     );  // controlled by global variable
if ( CheckToBool ( SegTransfer.WriteSynth           ) )     SetFlags ( outputflags, WriteSyntheticFiles         );

if ( CheckToBool ( SegTransfer.BestDir              ) )     SetFlags ( outputflags, CommonDirectory             );
if ( CheckToBool ( SegTransfer.BestDir              )
  && CheckToBool ( SegTransfer.DeleteIndivDirs      ) )     SetFlags ( outputflags, DeleteIndivDirectories      );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                forcesinglefiles    = CheckToBool ( SegTransfer.ForceSingleFiles );
TFileName           outputcommondir;
TFileName           outputresamplingdir;

                                        // using the real original base file name
if ( IsFlag ( outputflags, CommonDirectory ) ) {

    StringCopy  ( outputcommondir, SegTransfer.BaseFileName, "." InfixBestClustering );

    CreatePath  ( outputcommondir, false );

    if ( presetfile == SegPresetFilesRestingStatesGroupResampling ) {
                                        // it becomes a huge mess to have all the resampling files in the root directory - put them all apart
        StringCopy  ( outputresamplingdir,  outputcommondir, "\\" PostfixResampling );

        CreatePath  ( outputresamplingdir, false );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // extract min substrings range needed to discriminate of file names
bool                includefilename     = presetfile == SegPresetFilesRestingStatesSubjAllData
                                       || presetfile == SegPresetFilesRestingStatesSubjResampling;
int                 fromchars           = 0;
int                 tochars             = 0;


if ( includefilename ) {

    GoGoF.GetFilenamesSubRange ( fromchars, tochars );
                                        // this looks more user-friendly
    fromchars   = 0;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Decoding parameters done
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here we have these important flags tested & set: 
                                        //  - epochs, gfppeaks, badepochs, resampling
                                        //  - spatialfilter, 
                                        //  - isesipreset, 
                                        //  - forcesinglefiles

bool                ispreprocessing     = timelyfilter 
                                       || epochs        != EpochWholeTime 
                                       || gfppeaks      != NoGfpPeaksDetection 
                                       || badepochs     != NoSkippingBadEpochs;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // First spread into 1 file per group if needed
TGoGoF				gogof;

TFileName           tempbasefilename;
TGoF                baselist;


if ( forcesinglefiles ) {
                                        // create a new list of groups of files, each group having only 1 file into it
    TGoF				gof1file;
    TFileName           groupname;
    TStrings            groupnames;
    char                buff[ 256 ];


                                        // generate all possible additional postfixes to the base file names
    if ( GoGoF.NumGroups () > 1 ) {

        for ( int absg = 0; absg < GoGoF.NumGroups (); absg++ ) {

            ClearString ( groupname );

            if ( includefilename ) {
                                        // use last directory just above first file of the group (assume all files come from the same directory in a group...)
                StringCopy      ( groupname, GoGoF[ absg ][ 0 ] );
                RemoveFilename  ( groupname );
                RemoveDir       ( groupname );
                }

            if ( ! includefilename || StringIsEmpty ( groupname ) )

                StringCopy      ( groupname, InfixGroup, " ", IntegerToString ( buff, absg + 1, NumIntegerDigits ( GoGoF.NumGroups () ) ) );

            groupnames.Add ( groupname );
            }

                                        // finally, all groups belonged to the same directory, clear the list off
        if ( groupnames.HasOnlyDuplicates () )
            groupnames.Reset ();
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    for ( int absg = 0; absg < GoGoF.NumGroups ();        absg++ )
    for ( int fi   = 0; fi   < GoGoF[ absg ].NumFiles (); fi++   ) {
                                        // 1 file in this group
        gof1file.SetOnly ( GoGoF[ absg ][ fi ] );
                                        // copy group and add to list of groups
        gogof.Add ( &gof1file, true, MaxPathShort );

                                        // Generate new base file names, in case we process more than 1 group coming from different directories
                                        // We have to update the BaseFileName so we don't mix the original groups together!
        if ( (bool) groupnames ) {
                                        // add last directory just above file
            StringCopy      ( tempbasefilename, SegTransfer.BaseFileName, ".", groupnames[ absg ] );
            baselist.Add    ( tempbasefilename );
            }
        else                            // nothing to do, just copy base file name
            baselist.Add    ( SegTransfer.BaseFileName );

        } // for fi


//    gogof.Show ( 0, gogof.NumGroups () - 1, "GoGoF: 1) Force single files" );
    }

else { // ! forcesinglefiles

    gogof   = GoGoF;

    for ( int absg = 0; absg < GoGoF.NumGroups (); absg++ )

        baselist.Add    ( SegTransfer.BaseFileName );
    }


//baselist.Show ( "baselist: 1) After Single File" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Alternative dataset
                                        // Currently, files are retrieved based on guessing from the main input files
                                        // It would be nicer to have a more explicit way to that, through the dialog..


                                        // alternative ESI / EEG group?
                                        // !NOT done: alternative is MEG!
DualDataType        dualdata        = ! CheckToBool ( SegTransfer.DualDataset ) ?   NoDualData
                                    : alleeg                                    ?   DualRis     // EEG -> RIS
                                    : allris                                    ?   DualEeg     // RIS -> EEG
                                    :                                               NoDualData; // files not fully recognized - abort

AtomType            dualdatatype        = DualDataPresets[ dualdata ].DataType;

TGoGoF				gogofalt;
TGoGoF				preprocgogofalt;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // given data is of EEG type, look for alternative version in RIS
if      ( dualdata == DualRis ) {

    char                postfilename[ 256 ];
    bool                someretrieved   = false;

                                        // First case is working on the original files, so we need to account for any potential ZScore postfix
    StringCopy          ( postfilename, ".", ZScoreEnumToInfix ( ZScorePositive_CenterScaleOffset ) );
    StringGrepNeutral   ( postfilename );
    StringPrepend       ( postfilename, ".*" );
                                        // retrieve all files, so that in case of ambiguities / duplicates, the number of files test will likely fail
    gogofalt.GrepGoF    ( gogof, ".*", postfilename, AllRisFilesExt, true );

    someretrieved   |= gogofalt.NumFiles () > 0;

                                        // Second case is working on resampled results - in that case there are no ZScore postfixes
    if ( gogofalt.NumFiles () != gogof.NumFiles () ) {

        gogofalt.GrepGoF ( gogof, "", "", AllRisFilesExt, true );

        someretrieved   |= gogofalt.NumFiles () > 0;
        }

                                        // still not OK? reset everything
    if ( gogofalt.NumFiles () != gogof.NumFiles () ) {
                                        // show warning only if some files were found
        if ( someretrieved )
            ShowMessage (   "Alternative Inverse Solution dataset was only partially retrieved,"    NewLine 
                            "either some files were missing or some were in excess!"                NewLine 
                            "Processing will continue without dual dataset...", 
                            SegmentationTitle, ShowMessageWarning );

        dualdata    = NoDualData;
        gogofalt.Reset ();
        }
    } // if DualRis


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( dualdata == DualEeg ) {

    char                postfilename[ 256 ];
    bool                someretrieved   = false;

                                        // First case is working on resampled results - in that case there are no ZScore postfixes, which is less tricky
    gogofalt.GrepGoF ( gogof, "", "", AllEegFilesExt, true );

    someretrieved   |= gogofalt.NumFiles () > 0;

                                        // Second case is working on the original files, so we need to account for any potential ZScore postfix
    if ( gogofalt.NumFiles () != gogof.NumFiles () ) {

        StringCopy          ( postfilename, ".", ZScoreEnumToInfix ( ZScorePositive_CenterScaleOffset ) );
        StringGrepNeutral   ( postfilename );
        StringPrepend       ( postfilename, ".*" );

        gogofalt.RevertGrepGoF  ( gogof, ".*", postfilename, AllEegFilesExt );

        someretrieved   |= gogofalt.NumFiles () > 0;
        }

                                        // still not OK? reset everything
    if ( gogofalt.NumFiles () != gogof.NumFiles () ) {
                                        // show warning only if some files were found
        if ( someretrieved )
            ShowMessage (   "Alternative EEG dataset was only partially retrieved,"     NewLine 
                            "either some files were missing or some were in excess!"    NewLine 
                            "Processing will continue without dual dataset...", 
                            SegmentationTitle, ShowMessageWarning );

        dualdata    = NoDualData;
        gogofalt.Reset ();
        }
    } // DualEeg


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Update all base file name with the correct group
TGoF*               togof;
TFileName           buff;


for ( int absg = 0; absg < gogof.NumGroups (); absg++ ) {

    togof   = &gogof[ absg ];
                                        // !yes, we take a pointer to string, so that the string can be updated!
    char*       currentbasefilename     = baselist[ absg ];


    if ( includefilename ) {
                                        // including the file name will / should disambiguate everything(?)
        StringCopy ( tempbasefilename, currentbasefilename, "." );


//      for ( int fi = 0; fi < gof->NumFiles (); fi++ ) {   // use all files(?), or a common part to all files?
        int fi = 0;                                         // use only first file

            StringCopy  ( buff, (*togof)[ fi ] );
            GetFilename ( buff );                           // get rid of directory/path and extension
            StringClip  ( buff, fromchars, tochars );

            StringAppend ( tempbasefilename, buff );
//          }
        }

    else if ( gogof.NumGroups () > 1 ) {
                                        // generic groups name - !Files/Groups numbering will be absolute!
        if ( forcesinglefiles )
            StringCopy  ( tempbasefilename, currentbasefilename, "." "File"     " ", IntegerToString ( absg + 1, 3 ) );
        else
            StringCopy  ( tempbasefilename, currentbasefilename, "." InfixGroup " ", IntegerToString ( absg + 1, 3 ) );
        }
    else
                                        // single group
        StringCopy ( tempbasefilename, currentbasefilename );

                                        // udpates baselist because we deal with a pointer
    StringCopy ( currentbasefilename, tempbasefilename );

                                        // postfixing with clustering type?
//  if      ( clusteringmethod == ClusteringKMeans  )   StringAppend ( BaseFileName, "." InfixKMean );
////else if ( clusteringmethod == ClusteringAHHC    )   StringAppend ( BaseFileName, "." InfixAAHC  );
//  else if ( clusteringmethod == ClusteringTAHHC   )   StringAppend ( BaseFileName, "." InfixTAAHC  );
    }

//baselist.Show ( "baselist: 2) After Group" );

                                        // give the list of base file names a check
if ( ! baselist.HasNoDuplicates () ) {
    ShowMessage (   "There seems to be some duplicate base file names, according to the given parameters."  NewLine 
                    NewLine 
                    "Please check your output parameters, and come back...", 
                    SegmentationTitle, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

enum                GaugeGroupSegEnum 
                    {
                    gaugegroupseggroup,
                    gaugegroupsegpreproc,
                    gaugegroupsegprocoffset,
                    };


if ( gogof.NumGroups () > 1 
  || ispreprocessing 
  || resampling != NoTimeResampling ) {
                                        // Init gauge
    GroupGauge.Set ( "Batch Segmentation", SuperGaugeLevelInter );

                                        // global counter
    GroupGauge.AddPart ( gaugegroupseggroup,    
                         gogof.NumGroups (),        ispreprocessing                 ?  05 
                                                  : resampling != NoTimeResampling  ?   0
                                                  :                                   100 );
                                        // preprocessing counter
//  GroupGauge.AddPart ( gaugegroupsegpreproc,  gogof.NumGroups ()  * PreProcessFilesGaugeSize ( 1 ),   ispreprocessing ? 45 :   0 );
    GroupGauge.AddPart ( gaugegroupsegpreproc,
                         gogof.NumGroups () * 7 
                       + gogof.NumFiles  () * 2,    ispreprocessing                 ?  45
                                                  : resampling != NoTimeResampling  ?   0 
                                                  :                                     0 );

                                        // processing each sub-file(s)
    for ( int absg = 0; absg < gogof.NumGroups (); absg++ )
                                        // range is not known, i.e. how many sub-files will be generated
        GroupGauge.AddPart ( gaugegroupsegprocoffset + absg,   
                             0,                     ispreprocessing                 ? 50 / (double) gogof.NumGroups () 
                                                  : resampling != NoTimeResampling  ? 95 / (double) gogof.NumGroups () 
                                                  :                                    0                               );
                                        // fake part, just to avoid 100%
    GroupGauge.AddPart ( gaugegroupsegprocoffset + 1, 1, 1 );

    GroupGauge.AdjustOccupy ( false );
    }


if ( gogof.NumGroups () > 1 )
                                        // batch processing can be long, hide Cartool until we are done
    WindowMinimize ( CartoolMainWindow );

                                        // Changing priority
SetProcessPriority ( BatchProcessingPriority );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                usetempdir      = true;
TFileName           temppath;
TFileName           basefilename;
TFileName           concatfile;
TFileName           concatfilealt;
TGoGoF		        preprocgogof;
TGoF                preprocbaselist;
TGoGoF              resampgogof;
TGoGoF              resampgogofalt;
TGoF                goferrdata;
bool                newfiles;
bool                concatgof;
bool                owningfiles;
bool                complimentaryopen;

                                        // loop through all groups of groups of files, 1 at a time
for ( int absg = 0; absg < gogof.NumGroups (); absg++ ) {


    if ( GroupGauge.IsAlive () )
        GroupGauge.CurrentPart  = gaugegroupsegpreproc;


    CartoolApplication->SetMainTitle        ( "Segmentation Preprocessing of", baselist[ absg ], GroupGauge );


    PreProcessFiles (   gogof    [ absg ],          datatype,
                        dualdata,       dualdata ? &gogofalt[ absg ] : 0,
                        spatialfilter,              SegTransfer.XyzFile,
                        false,          0,          RegularizationNone,     0,
                        false,                                  // no complex case
                        false,                                  // no GFP normalization
                        BackgroundNormalizationNone,            ZScoreNone, 0,
                        false,                                  // no ranking
                        false,          0,                      // no thresholding
                        FilterTypeNone, 0,                      // no Envelope
                        0,              FilterTypeNone,         // no ROIS
                        epochs,        &epochfrom,             &epochto,
                        gfppeaks,       listgfppeaks,
                        badepochs,      listbadepochs,          BadEpochsToleranceDefault,
                        baselist[ absg ],
                        0,
                        0 /*fromchars*/,tochars,                // also clip preprocessed file, so file name can be optimally smaller
                        usetempdir,     temppath,               // there can be problems if we generate files within the original directory, in case of parallel computations
                        true,           preprocgogof,   dualdata ? &preprocgogofalt : 0,    preprocbaselist,    newfiles,
                        true,           0,
                        ExecFlags ( ( Cartool.IsInteractive () ? Interactive : Silent ) | DefaultOverwrite ),
                        &GroupGauge 
                    );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // ?or all the time, for n files?
    concatgof           = IsSegGroupFilePreset ( presetfile );

    owningfiles         = newfiles                                  // files generated by PreProcessFiles - we can own them at will
                       || resampling == TimeResampling              // files generated by resampling - we can own them too
                       || concatgof;                                // Grand Clustering intermediate concatenation - it's ours
                                        // update auto-opening
    complimentaryopen   = (    epochs == EpochWholeTime
                            || epochs == EpochsFromList && (int) epochfrom == 1
//                          || TMicroStatesSegDialog::GoGoF.NumGroups () <= 1
                            || preprocgogof.NumGroups () <= 1       // split epochs
                          ) 
                       && resampling != TimeResampling;             // resampling can generate a lot of files


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we can have more than 1 GoF here, due to cutting into epochs
                                        // it can also be a copy of the current GoF if no preprocessing was set

                                        // current group has its own part, which range is the number of sub-parts it is split into
    if ( GroupGauge.IsAlive () )
        GroupGauge.SetRange ( gaugegroupsegprocoffset + absg, preprocgogof.NumGroups () * ( 1 + AtLeast ( 1, numresampling ) ) );



    for ( int absg2 = 0; absg2 < preprocgogof.NumGroups (); absg2++ ) {


        CartoolApplication->SetMainTitle    ( "Segmentation of", preprocbaselist[ absg2 ], GroupGauge );

        if ( GroupGauge.IsAlive () )
            GroupGauge.Next ( gaugegroupsegprocoffset + absg, SuperGaugeUpdateTitle );

//      if ( VkEscape () && GetAnswerFromUser ( "Do you really want to abort current segmentation?", SegmentationTitle ) )
//          goto endofprocessing;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // resampling can go shopping on a lot of files, let's concatenate these guys instead so it makes a single file access
        if ( concatgof ) {
            TGoMaps             concatmaps ( &preprocgogof[ absg2 ], datatype, ReferenceAsInFile );


            StringCopy      ( concatfile,   preprocgogof[ absg2 ][ 0 ] );
            RemoveFilename  ( concatfile,   true );
            StringAppend    ( concatfile,   ToFileName ( SegTransfer.BaseFileName ), ".",  PostfixConcat, ".", StringRandom ( buff, 4 ) );
            AddExtension    ( concatfile,   GroupsAllRis ? SegmentationRisFileExt : SegmentationEegFileExt );

                                        // write all maps in a single file
            concatmaps.WriteFile ( concatfile );

                                        // replace all GoF files to single concat file
            preprocgogof[ absg2 ].SetOnly ( concatfile );


            if ( dualdata ) {
                                        // same procedure - we can reuse the temp concatmaps var
                concatmaps.ReadFiles( preprocgogofalt[ absg2 ], dualdatatype, ReferenceAsInFile );

                StringCopy          ( concatfilealt, concatfile );
                ReplaceExtension    ( concatfilealt, DualDataPresets[ dualdata ].SavingExtension );

                concatmaps.WriteFile( concatfilealt );

                preprocgogofalt[ absg2 ].SetOnly ( concatfilealt );
                }

            } // concatgof


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // handling the optional resampling here, so it applies to all files/epochs together
        if ( resampling == NoTimeResampling ) {
                                        // just copying the current single TGoF - !resampgogof now has only 1 TGoF inside it!
            resampgogof     = preprocgogof[ absg2 ];

            if ( dualdata )
                resampgogofalt  = preprocgogofalt[ absg2 ];
            }

        else {
                                        // reduce sample size to approximately match the same relative resample size after Gfp Peaks
                                        // this value looks to be linked to the high frequency filter that has been used (30/40 Hz)
            resamplingsize  = gfppeaks != NoGfpPeaksDetection   ? reqresamplingsize / GfpPeaksDownsamplingRatio
                                                                : reqresamplingsize;

            preprocgogof[ absg2 ].ResampleFiles (   resampling,     numresampling,  resamplingsize,
                                                    dualdata ? &preprocgogofalt[ absg2  ] : 0,
                                                    resampgogof, dualdata ? &resampgogofalt : 0,
                                                    ExecFlags ( Interactive | DefaultOverwrite )
                                                );
            }

//        resampgogof    .Show ( "PreProcessFiles Resampled: output" );
//        resampgogofalt .Show ( "PreProcessFiles Resampled: output alt" );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // now we are ready to get things done for real...
        for ( int rsi = 0; rsi < (int) resampgogof; rsi++ ) {

            if ( GroupGauge.IsAlive () )
                GroupGauge.Next ( gaugegroupsegprocoffset + absg, SuperGaugeUpdateTitle );

                                        // output base file name
            if ( resampling == NoTimeResampling )   StringCopy ( basefilename, preprocbaselist[ absg2 ] );
            else                                    StringCopy ( basefilename, preprocbaselist[ absg2 ], "." PostfixResampled, IntegerToString ( buff, rsi + 1, NumIntegerDigits ( (int) resampgogof ) ) );


            bool    segok   =

            Segmentation    (   resampgogof [ rsi ],                                        // the one to be processed
                                gogof       [ absg  ],                                      // original - used for proper verbose output
                                dualdata,           dualdata? &resampgogofalt[ rsi ] : 0,   // alternative dataset, after preprocessing / resampling
                                analysis,           modality,               time,

                                epochs,
                                badepochs,          listbadepochs,
                                gfppeaks,           listgfppeaks,
                                resampling,         numresampling,          resamplingsize,
                                spatialfilter,      SegTransfer.XyzFile,
                                datatype,           polarity,               dataref,

                                clusteringmethod,
                                reqminclusters,     reqmaxclusters,
                                numrandomtrials,    centroid,
                                dolimitcorr,        limitcorr,

                                sequentialize,
                                mergecorr,          mergecorrthresh,
                                smoothing,          smoothinghalfsize,      smoothinglambda,
                                rejectsmall,        rejectsize,
                                MapOrderingContextual,  0,
                                             // here we know if we own the files
                                CombineFlags ( outputflags, owningfiles ? OwningFiles : NoMicroStatesOutFlags ),
                                basefilename,       StringIsEmpty ( outputresamplingdir ) ? outputcommondir : outputresamplingdir,
                                complimentaryopen
                            );

            if ( ! segok )  
                CmCancel ();    // supposedly something really went bad
            } // for resampling


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // getting rid of temp files (could be the same sometimes)
        if ( concatgof )                        DeleteFileExtended ( concatfile    );
        if ( concatgof && dualdata )            DeleteFileExtended ( concatfilealt );
//      if ( resampling == TimeResampling )     resampgogof.DeleteFiles ( true );   // don't - either kept into PreProcData or whole directory will be cleared-up

        } // for preprocgogof


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // put this here AFTER processing, so the user does not think it's done at 100%
    if ( GroupGauge.IsAlive () )
        GroupGauge.Next ( gaugegroupseggroup, SuperGaugeUpdateTitle );

    } // for gof


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Complimentary averaging all error.data in case of Grand Clustering + resampling
if ( presetfile == SegPresetFilesRestingStatesGroupResampling /*IsSegResamplingFilePreset ( presetfile )*/
  && IsFlag ( outputflags, CommonDirectory )
                                        // grab anything that looks like error.data within the common directory...
  && goferrdata.GrepFiles ( outputresamplingdir, AllErrorDataGrep, GrepOptionDefaultFiles ) ) {

    TFileName           errdataavg;
                                        // !NOT your regular averaging here - see code!
    BatchAveragingErrorData (   goferrdata, 
                                errdataavg, 
                                ExecFlags ( Silent | DefaultOverwrite ) );

                                        // copy & rename
    StringCopy      ( buff, outputcommondir, "\\", ToFileName ( SegTransfer.BaseFileName ), "." PostfixResampled );
    AddExtension    ( buff, FILEEXT_ERRORDATA );

    MoveFileExtended ( errdataavg, buff );

                                        // remaining .mrk file
    DeleteFileExtended ( errdataavg, FILEEXT_MRK );

                                        // open the final destination file
    CartoolDocManager->OpenDoc ( buff, dtOpenOptions );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Complimentary clustering all maps with the same number of clusters together
if ( presetfile == SegPresetFilesRestingStatesGroupResampling
  && IsFlag ( outputflags, CommonDirectory ) ) {

    AnalysisType        gcanalysis      = AnalysisRestingStatesGroup;
    ModalityType        gcmodality      = modality;
    SamplingTimeType    gctime          = SamplingTimeWhole;

                                        // because CommonDirectory
    TFileName           mergedir;
    StringCopy      ( mergedir, SegTransfer.BaseFileName, ".", InfixBestClustering, "\\", ToFileName ( SegTransfer.BaseFileName ), "." PostfixResampled );


    char                grepcluster[ 256 ];
    TGoF                gofcluster;
    TGoF                gofclusteralt;


    for ( int nclusters = reqminclusters; nclusters <= reqmaxclusters; nclusters++ ) {


        StringCopy      ( grepcluster,  "\\.0*", IntegerToString ( nclusters ), "\\.\\([0-9]+\\).*\\.", GroupsAllRis ? SegmentationRisTemplateExt : SegmentationEegTemplateExt, "$" );

                                        // grab all files for that amount of clusters
        if ( ! gofcluster.GrepFiles ( outputresamplingdir, grepcluster, GrepOptionDefaultFiles ) )
            continue;

                                        // 1 file only -> simply copy & rename
        if ( (int) gofcluster == 1 ) {
                                                                                                    // not needed as resampled data are in their own directory
            StringCopy      ( buff, outputcommondir, "\\", ToFileName ( SegTransfer.BaseFileName ), "." PostfixResampled );
            StringAppend    ( buff, ".",    IntegerToString ( nclusters, NumIntegerDigits ( reqmaxclusters ) ),  ".(",    IntegerToString ( nclusters, NumIntegerDigits ( reqmaxclusters ) ), ")" );
            AddExtension    ( buff, GroupsAllRis ? SegmentationRisTemplateExt : SegmentationEegTemplateExt );

                                        // also copy alternative dataset
            CopyFileExtended ( gofcluster[ 0 ], buff, dualdata == DualRis ? SegmentationRisTemplateExt 
                                                    : dualdata == DualEeg ? SegmentationEegTemplateExt 
                                                    :                       0 );

            continue;
            }


        if ( dualdata ) {

            gofclusteralt   = gofcluster;

            gofclusteralt.ReplaceExtension ( dualdata == DualRis ? SegmentationRisTemplateExt : SegmentationEegTemplateExt );
            }


        Segmentation    (   gofcluster, 
                            gofcluster,
                            dualdata,           dualdata ? &gofclusteralt : 0,
                            gcanalysis,         gcmodality,     gctime,

                            EpochWholeTime,
                            NoSkippingBadEpochs,0,
                            NoGfpPeaksDetection,0,
                            NoTimeResampling,   0,              0,
                            SpatialFilterNone,  0,

                            datatype,           polarity,       dataref,// same as the real clustering
                            ClusteringKMeans,
                            nclusters,          nclusters,              // restrict to only current nclusters
                            50,                 centroid,               // third level of template-ing here
                            false,              0.50,                   // cloud of maps should be consistent at that point

                            false,
                            false,              0.95,
                            false,              0,              0,
                            false,              0,
                            MapOrderingContextual,  0,

                            (MicroStatesOutFlags) ( WriteTemplatesFiles ),
                            mergedir,           outputcommondir,
                            false
                        );

                                        // copy clusters above, back to current directory - all EEG & RIS at once
        StringCopy              ( buff,     "\\.(", SegmentationRisTemplateExt, "|", SegmentationEegTemplateExt, ")$" );
        gofcluster.GrepFiles    ( mergedir, buff, GrepOptionDefaultFiles );

        gofcluster.CopyFilesTo  ( outputcommondir, CopyAndKeepOriginals, TracksBuddyExt );
        } // for merging clusters

                                        // temp directory
    NukeDirectory ( mergedir );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // if we used a (single) temp directory, we can dispose of it now
if ( usetempdir && StringIsNotEmpty ( temppath ) )

    NukeDirectory ( temppath );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

endofprocessing:

SetProcessPriority ();

WindowMaximize ( CartoolMainWindow );

if ( GroupGauge.IsAlive () ) {

    GroupGauge.FinishParts ();

    CartoolApplication->SetMainTitle    ( "Segmentation of", SegTransfer.BaseFileName, GroupGauge );
    }
else
    GroupGauge.Set ( SegmentationTitle, SuperGaugeLevelDefault );


GroupGauge.HappyEnd ();
}

OptimizeOn

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
