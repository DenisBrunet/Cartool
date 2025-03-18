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

#include    "TMicroStatesFitDialog.h"

#include    "Strings.Utils.h"
#include    "Strings.Grep.h"
#include    "Files.TVerboseFile.h"
#include    "Files.SpreadSheet.h"
#include    "Files.TSplitLinkManyFile.h"
#include    "Files.PreProcessFiles.h"
#include    "Dialogs.Input.h"

#include    "CartoolTypes.h"            // EpochsType SkippingEpochsType

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

TMicroStatesFitStruct       FitTransfer;

TGoGoF                      TMicroStatesFitDialog::GoGoF;

                                        // These are the needed parameters to "push on the dialog's button" for each preset - it does NOT contain all actual parameters
const FitPresetSpec FitPresets[ NumFitPresets ] =
            {                                                                                                                                                                    // SegPresetRSGroup used to bypass some options
            { FitPresetEegSurfaceErp,                   "EEG / Surface / ERPs",                                             -100, SmoothingDefaultHalfSize, (TypeClusteringFlags) ( SegPresetERP      | SegPresetSignedData   | SegPresetAccountPolarity | SegPresetAveRef | SegPresetSpatialFilterOff | SegPresetZScoreOff | SegPresetGfpNormOn  | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSmoothingOn  | SegPresetRejectSmallOn  | SegPresetVariablesAll    )  },
            { FitPresetEegSurfaceSpont,                 "EEG / Surface / Spontaneous",                                        50, SmoothingDefaultHalfSize, (TypeClusteringFlags) ( SegPresetRSIndiv  | SegPresetSignedData   | SegPresetIgnorePolarity  | SegPresetAveRef | SegPresetSpatialFilterOn  | SegPresetZScoreOff | SegPresetGfpNormOn  | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSmoothingOn  | SegPresetRejectSmallOn  | SegPresetVariablesCyclic )  },
            { FitPresetEegSurfaceSpontGfpPeaks,         "EEG / Surface / Spontaneous GFP Peaks",                              50, 1,                        (TypeClusteringFlags) ( SegPresetRSIndiv  | SegPresetSignedData   | SegPresetIgnorePolarity  | SegPresetAveRef | SegPresetSpatialFilterOn  | SegPresetZScoreOff | SegPresetGfpNormOn  | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSmoothingOn  | SegPresetRejectSmallOn  | SegPresetVariablesCyclic )  },
            { FitPresetEegSurfaceSpontTemplates,        "EEG / Surface / Collection of Spontaneous Templates",                50, 0,                        (TypeClusteringFlags) ( SegPresetRSGroup  | SegPresetSignedData   | SegPresetIgnorePolarity  | SegPresetAveRef | SegPresetSpatialFilterOff | SegPresetZScoreOff | SegPresetGfpNormOff | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSmoothingOff | SegPresetRejectSmallOff | SegPresetVariablesNoTime )  },

            { FitPresetSeparator1,                      "",                                                                 -100, 0, SegPresetClusteringNone, SegPresetPostProcNone },
                                                                                                                                                                                                                                                                          // spatial filter not defined for other topologies than 2D, so f.ex. strips doesn't exist
            { FitPresetEegIntraErp,                     "EEG / Intra-Cranial / ERPs",                                       -100, SmoothingDefaultHalfSize, (TypeClusteringFlags) ( SegPresetERP      | SegPresetSignedData   | SegPresetAccountPolarity | SegPresetNoRef  | SegPresetSpatialFilterOff | SegPresetZScoreOff | SegPresetGfpNormOn  | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSmoothingOn  | SegPresetRejectSmallOn  | SegPresetVariablesAll    )  },
            { FitPresetEegIntraSpont,                   "EEG / Intra-Cranial / Spontaneous",                                  50, SmoothingDefaultHalfSize, (TypeClusteringFlags) ( SegPresetRSIndiv  | SegPresetSignedData   | SegPresetIgnorePolarity  | SegPresetNoRef  | SegPresetSpatialFilterOff | SegPresetZScoreOff | SegPresetGfpNormOn  | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSmoothingOn  | SegPresetRejectSmallOn  | SegPresetVariablesCyclic )  },
            { FitPresetEegIntraSpontGfpPeaks,           "EEG / Intra-Cranial / Spontaneous GFP Peaks",                        50, 1,                        (TypeClusteringFlags) ( SegPresetRSIndiv  | SegPresetSignedData   | SegPresetIgnorePolarity  | SegPresetNoRef  | SegPresetSpatialFilterOff | SegPresetZScoreOff | SegPresetGfpNormOn  | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSmoothingOn  | SegPresetRejectSmallOn  | SegPresetVariablesCyclic )  },
            { FitPresetEegIntraSpontTemplates,          "EEG / Intra-Cranial / Collection of Spontaneous Templates",          50, 0,                        (TypeClusteringFlags) ( SegPresetRSGroup  | SegPresetSignedData   | SegPresetIgnorePolarity  | SegPresetNoRef  | SegPresetSpatialFilterOff | SegPresetZScoreOff | SegPresetGfpNormOff | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSmoothingOff | SegPresetRejectSmallOff | SegPresetVariablesNoTime )  },
                                                                                                                                                                                                                                                       
            { FitPresetSeparator2,                      "",                                                                 -100, 0, SegPresetClusteringNone, SegPresetPostProcNone },

            { FitPresetMegSurfaceErp,                   "MEG / ERPs",                                                       -100, SmoothingDefaultHalfSize, (TypeClusteringFlags) ( SegPresetERP      | SegPresetPositiveData | SegPresetAccountPolarity | SegPresetNoRef  | SegPresetSpatialFilterOff | SegPresetZScoreOff | SegPresetGfpNormOn  | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSmoothingOn  | SegPresetRejectSmallOn  | SegPresetVariablesAll    )  },
            { FitPresetMegSurfaceSpont,                 "MEG / Spontaneous",                                                  50, SmoothingDefaultHalfSize, (TypeClusteringFlags) ( SegPresetRSIndiv  | SegPresetPositiveData | SegPresetAccountPolarity | SegPresetNoRef  | SegPresetSpatialFilterOn  | SegPresetZScoreOff | SegPresetGfpNormOn  | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSmoothingOn  | SegPresetRejectSmallOn  | SegPresetVariablesCyclic )  },
            { FitPresetMegSurfaceSpontGfpPeaks,         "MEG / Spontaneous GFP Peaks",                                        50, 1,                        (TypeClusteringFlags) ( SegPresetRSIndiv  | SegPresetPositiveData | SegPresetAccountPolarity | SegPresetNoRef  | SegPresetSpatialFilterOn  | SegPresetZScoreOff | SegPresetGfpNormOn  | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSmoothingOn  | SegPresetRejectSmallOn  | SegPresetVariablesCyclic )  },
            { FitPresetMegSurfaceSpontTemplates,        "MEG / Collection of Spontaneous Templates",                          50, 0,                        (TypeClusteringFlags) ( SegPresetRSGroup  | SegPresetPositiveData | SegPresetAccountPolarity | SegPresetNoRef  | SegPresetSpatialFilterOff | SegPresetZScoreOff | SegPresetGfpNormOff | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSmoothingOff | SegPresetRejectSmallOff | SegPresetVariablesNoTime )  },
                                                                                                                                                                                                                                                       
            { FitPresetSeparator3,                      "",                                                                 -100, 0, SegPresetClusteringNone, SegPresetPostProcNone },
                                                                                                                                                                                                                                                                          // spatial filter now exists for 3D solution points - but at that point, outliers should have been addressed beforehand
            { FitPresetRisScalErp,                      "ESI / ERPs",                                                       -100, SmoothingDefaultHalfSize, (TypeClusteringFlags) ( SegPresetERP      | SegPresetPositiveData | SegPresetAccountPolarity | SegPresetNoRef  | SegPresetSpatialFilterOff | SegPresetZScoreOn  | SegPresetGfpNormOff | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSmoothingOn  | SegPresetRejectSmallOn  | SegPresetVariablesAll    )  },
            { FitPresetRisScalSpont,                    "ESI / Spontaneous EEG",                                            -100, SmoothingDefaultHalfSize, (TypeClusteringFlags) ( SegPresetRSIndiv  | SegPresetPositiveData | SegPresetAccountPolarity | SegPresetNoRef  | SegPresetSpatialFilterOff | SegPresetZScoreOn  | SegPresetGfpNormOff | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSmoothingOn  | SegPresetRejectSmallOn  | SegPresetVariablesCyclic )  },
            { FitPresetRisScalSpontGfpPeaks,            "ESI / Spontaneous EEG GFP Peaks",                                  -100, 1,                        (TypeClusteringFlags) ( SegPresetRSIndiv  | SegPresetPositiveData | SegPresetAccountPolarity | SegPresetNoRef  | SegPresetSpatialFilterOff | SegPresetZScoreOn  | SegPresetGfpNormOff | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSmoothingOn  | SegPresetRejectSmallOn  | SegPresetVariablesCyclic )  },
            { FitPresetRisScalSpontTemplates,           "ESI / Collection of Spontaneous Templates",                        -100, 0,                        (TypeClusteringFlags) ( SegPresetRSGroup  | SegPresetPositiveData | SegPresetAccountPolarity | SegPresetNoRef  | SegPresetSpatialFilterOff | SegPresetZScoreOff | SegPresetGfpNormOff | SegPresetEnvelopeOff ),    (TypePostProcFlags) ( SegPresetSmoothingOff | SegPresetRejectSmallOff | SegPresetVariablesNoTime )  },
            };


const char  FitVarNames[ 2 ][ fitnumvar ][ 32 ] =
            {
            {
            "FOnset",                   // First Onset
            "LOffset",                  // Last Offset
            "NumTF",                    // Number of TF
            "TfCentroid",               // Tf of centroid (barycenter)
            "MeanCorr",                 // Mean Correlation
            "Gev",                      // Gev
            "BCorr",                    // Best Correlation
            "TfBCorr",                  // Tf of Best Correlation
            "GfpTfBCorr",               // Gfp of Tf of Best Correlation
            "MaxGfp",                   // Max Gfp over each segment
            "TfMaxGfp",                 // Tf of Max Gfp over each segment
            "MeanGfp",                  // Mean Gfp over each segment
            "MeanDur",                  //
            "TimeCov",                  //
            "SegDensity",               //
            },
            {
            "F",
            "L",
            "N",
            "C",
            "M",
            "G",
            "B",
            "T",
            "P",
            "X",
            "A",
            "E",
            "D",
            "V",
            "S",
            }
            };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TMicroStatesFitFilesStruct::TMicroStatesFitFilesStruct ()
{
ClearString ( TemplateFileName );

StringCopy ( NumGroups, "0" );
GroupsSummary.Clear ();
StringCopy ( NumWithinSubjects, "1" );
StringCopy ( MaxWithinSubjects, "1" );

EpochFrom.Clear ();
EpochTo  .Clear ();
EpochMaps.Clear ();

TFileName           cp;
cp.GetCurrentDir ();
StringAppend ( cp, "\\Fit" );
StringCopy ( BaseFileName, cp );

WriteSegFiles       = BoolToCheck ( true  );
WriteClusters       = BoolToCheck ( true  );
DeletePreProcFiles  = BoolToCheck ( true  );
}


        TMicroStatesFitParamsStruct::TMicroStatesFitParamsStruct ()
{
PresetsParams.Clear ();
for ( int i = 0; i < NumFitPresets; i++ )
    PresetsParams.AddString ( FitPresets[ i ].Text, i == FitPresetParamsDefault );

                                        // use the exact parameters from default preset
const FitPresetSpec&    preset          = FitPresets[ FitPresetParamsDefault ];
uint                    clflags         = preset.ClusteringFlags;
uint                    ppflags         = preset.PostProcFlags;


SpatialFilter       = BoolToCheck ( clflags & SegPresetSpatialFilterOn    );
ClearString ( XyzFile );

NormalizationNone   = BoolToCheck ( clflags & SegPresetGfpNormOff );
NormalizationGfp    = BoolToCheck ( clflags & SegPresetGfpNormOn  );

SkipBadEpochs       = BoolToCheck ( false );
SkipBadEpochsAuto   = BoolToCheck ( false );
SkipBadEpochsList   = BoolToCheck ( true  );
StringCopy  ( SkipMarkers, "art" );

PositiveData        = BoolToCheck ( clflags & SegPresetPositiveData       );
SignedData          = BoolToCheck ( clflags & SegPresetSignedData         );
VectorData          = BoolToCheck ( clflags & SegPresetVectorData         );

AccountPolarity     = BoolToCheck ( clflags & SegPresetAccountPolarity    );
IgnorePolarity      = BoolToCheck ( clflags & SegPresetIgnorePolarity     );

FitSingleSeg        = BoolToCheck ( false );

NoRef               = BoolToCheck ( clflags & SegPresetNoRef              );
AveRef              = BoolToCheck ( clflags & SegPresetAveRef             );


Smoothing           = BoolToCheck ( ppflags & SegPresetSmoothingOn        );
IntegerToString ( WindowSize, preset.SmoothingSize );
FloatToString   ( BesagFactor, SmoothingDefaultBesag );

RejectSmall         = BoolToCheck ( ppflags & SegPresetRejectSmallOn      );
IntegerToString ( RejectSize, preset.SmoothingSize );

ClipCorrelation     = BoolToCheck ( preset.CorrelationThreshold > -100 );
IntegerToString ( MinCorrelation, Clip ( preset.CorrelationThreshold, -100, 100 ) );
}


        TMicroStatesFitResultsStruct::TMicroStatesFitResultsStruct ()
{
FOnset              = BoolToCheck ( false );
LOffset             = BoolToCheck ( false );
NumTf               = BoolToCheck ( false );
TfCentroid          = BoolToCheck ( false );
MeanCorr            = BoolToCheck ( false );
Gev                 = BoolToCheck ( false );
BestCorr            = BoolToCheck ( false );
TfBestCorr          = BoolToCheck ( false );
GfpTfBestCorr       = BoolToCheck ( false );
MaxGfp              = BoolToCheck ( false );
TfMaxGfp            = BoolToCheck ( false );
MeanGfp             = BoolToCheck ( false );
MeanDuration        = BoolToCheck ( false );
TimeCoverage        = BoolToCheck ( false );
SegDensity          = BoolToCheck ( false );


LinesAsSamples      = BoolToCheck ( true  );
ColumnsAsFactors    = BoolToCheck ( false );
OneFilePerGroup     = BoolToCheck ( true  );
OneFilePerVariable  = BoolToCheck ( false );
ShortVariableNames  = BoolToCheck ( false );


WriteStatDurations  = BoolToCheck ( false );
WriteCorrFiles      = BoolToCheck ( true  );
WriteSegFrequency   = BoolToCheck ( false );


MarkovSegment       = BoolToCheck ( false );
StringCopy ( MarkovNumTransitions, "1" );
}


        TMicroStatesFitStruct::TMicroStatesFitStruct ()
{
LastDialogId                = IDD_FITTING1;

                                        // !See  EvPresetsChange!
SetVariables ( FitPresets[ FitPresetParamsDefault ] );
}


//----------------------------------------------------------------------------
void    TMicroStatesFitStruct::SetVariablesOn ()
{
FOnset         = BoolToCheck (  true );
LOffset        = BoolToCheck (  true );
NumTf          = BoolToCheck (  true );
TfCentroid     = BoolToCheck (  true );
MeanCorr       = BoolToCheck (  true );
Gev            = BoolToCheck (  true );
BestCorr       = BoolToCheck (  true );
TfBestCorr     = BoolToCheck (  true );
GfpTfBestCorr  = BoolToCheck (  true );
MaxGfp         = BoolToCheck (  true );
TfMaxGfp       = BoolToCheck (  true );
MeanGfp        = BoolToCheck (  true );
MeanDuration   = BoolToCheck (  true );
TimeCoverage   = BoolToCheck (  true );
SegDensity     = BoolToCheck (  true );


SetNonCompetitive ();
}


void    TMicroStatesFitStruct::SetVariablesOff ()
{
FOnset         = BoolToCheck ( false );
LOffset        = BoolToCheck ( false );
NumTf          = BoolToCheck ( false );
TfCentroid     = BoolToCheck ( false );
MeanCorr       = BoolToCheck ( false );
Gev            = BoolToCheck ( false );
BestCorr       = BoolToCheck ( false );
TfBestCorr     = BoolToCheck ( false );
GfpTfBestCorr  = BoolToCheck ( false );
MaxGfp         = BoolToCheck ( false );
TfMaxGfp       = BoolToCheck ( false );
MeanGfp        = BoolToCheck ( false );
MeanDuration   = BoolToCheck ( false );
TimeCoverage   = BoolToCheck ( false );
SegDensity     = BoolToCheck ( false );


SetNonCompetitive ();
}


void    TMicroStatesFitStruct::SetNonCompetitive ()
{
                                        // Overriding some variables
if ( ! CheckToBool ( FitSingleSeg ) )
    return;


FOnset              = BoolToCheck ( false );
LOffset             = BoolToCheck ( false );
NumTf               = BoolToCheck ( false );
TfCentroid          = BoolToCheck ( false );
MaxGfp              = BoolToCheck ( false );
TfMaxGfp            = BoolToCheck ( false );
MeanGfp             = BoolToCheck ( false );
MeanDuration        = BoolToCheck ( false );
TimeCoverage        = BoolToCheck ( false );
SegDensity          = BoolToCheck ( false );

//WriteSegFiles       = BoolToCheck ( false );
WriteStatDurations  = BoolToCheck ( false );
WriteSegFrequency   = BoolToCheck ( false );
}


void    TMicroStatesFitStruct::SetVariables ( const FitPresetSpec& preset )
{
uint                flags           = preset.PostProcFlags;

                                        // separator has no flags..
if ( flags == 0 )
    return;
                                        // Set all variables in either case

                                        // Note that "No Time" has a different meaning than "Non Competitive / Single Segment"
                                        // - No Time means data are a set of maps, without time structure
                                        // - Non-Competitive means data has a time structure, and the variables are computed for a single template at a time

bool            varnotime       = flags & SegPresetVariablesNoTime;
bool            varcyclic       = flags & SegPresetVariablesCyclic;
bool            varall          = flags & SegPresetVariablesAll;


FOnset          = BoolToCheck ( varall                           );
LOffset         = BoolToCheck ( varall                           );
NumTf           = BoolToCheck ( varall                           );
TfCentroid      = BoolToCheck ( varall                           );
MeanCorr        = BoolToCheck ( varall || varcyclic || varnotime );
Gev             = BoolToCheck ( varall || varcyclic || varnotime );
BestCorr        = BoolToCheck ( varall || varcyclic || varnotime );
TfBestCorr      = BoolToCheck ( varall                           );
GfpTfBestCorr   = BoolToCheck ( varall                           );
MaxGfp          = BoolToCheck ( varall || varcyclic              );
TfMaxGfp        = BoolToCheck ( varall                           );
MeanGfp         = BoolToCheck ( varall || varcyclic              );
MeanDuration    = BoolToCheck (           varcyclic              );
TimeCoverage    = BoolToCheck ( varall || varcyclic || varnotime );
SegDensity      = BoolToCheck (           varcyclic              );


SetNonCompetitive ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TMicroStatesFitFilesDialog, TMicroStatesFitDialog )

    EV_WM_DROPFILES,

    EV_COMMAND                  ( IDC_BROWSELSFILE,         CmBrowseTemplateFileName ),
    EV_COMMAND                  ( IDC_UPDIRECTORY,          CmUpOneDirectory ),
    EV_COMMAND                  ( IDC_BROWSEBASEFILENAME,   CmBrowseOutputFileName ),

    EV_COMMAND                  ( IDC_ADDEPOCH,             CmAddEpoch ),
    EV_COMMAND                  ( IDC_REMOVEEPOCH,          CmRemoveEpoch ),

    EV_COMMAND                  ( IDC_ADDGROUP,             CmAddGroup ),
    EV_COMMAND                  ( IDC_REMFROMLIST,          CmRemoveGroup ),
    EV_COMMAND                  ( IDC_CLEARLISTS,           CmClearGroups ),
    EV_COMMAND                  ( IDC_SORTLISTS,            CmSortGroups ),
    EV_COMMAND                  ( IDC_READPARAMS,           CmReadParams ),
    EV_COMMAND                  ( IDC_WRITEPARAMS,          CmWriteParams ),

    EV_COMMAND_ENABLE           ( IDOK,                     CmOkEnable ),

    EV_COMMAND_ENABLE           ( IDC_ADDEPOCH,             CmAddEpochEnable ),

    EV_COMMAND_ENABLE           ( IDC_WRITESEGFILES,        CmVariableEnable ),

END_RESPONSE_TABLE;


        TMicroStatesFitFilesDialog::TMicroStatesFitFilesDialog ( TWindow* parent, TResId resId )
      : TMicroStatesFitDialog ( parent, resId )
{
XyzAndEegMatch      = true;
GroupsAllRis        = false;


TemplateFileName    = new TEdit ( this, IDC_LSFILENAME,  EditSizeText );

NumGroups           = new TEdit ( this, IDC_NUMGROUPS, EditSizeValue );
GroupsSummary       = new TListBox ( this, IDC_GROUPSSUMMARY );
NumWithinSubjects   = new TEdit ( this, IDC_NUMWITHINSUBJECTS, EditSizeValue );
MaxWithinSubjects   = new TEdit ( this, IDC_MAXWITHINSUBJECTS, EditSizeValue );

EpochFrom           = new TComboBox ( this, IDC_EPOCHFROM );
EpochTo             = new TComboBox ( this, IDC_EPOCHTO   );
EpochMaps           = new TComboBox ( this, IDC_EPOCHMAPS );

BaseFileName        = new TEdit ( this, IDC_BASEFILENAME, EditSizeText );

WriteSegFiles       = new TCheckBox ( this, IDC_WRITESEGFILES );
WriteClusters       = new TCheckBox ( this, IDC_WRITECLUSTERS );
DeletePreProcFiles  = new TCheckBox ( this, IDC_DELETEPREPROCFILES );


SetTransferBuffer ( dynamic_cast <TMicroStatesFitFilesStruct*> ( &FitTransfer ) );

                                        // TMicroStatesFitDialog constructor updates: MaxFilesPerGroup, MaxGroupsWithinSubjects (MaxWithinSubjects but not NumWithinSubjects), AllGroupsEqual
CheckXyzFile ();
}


        TMicroStatesFitFilesDialog::~TMicroStatesFitFilesDialog ()
{
delete  TemplateFileName;
delete  NumGroups;              delete  GroupsSummary;
delete  NumWithinSubjects;      delete  MaxWithinSubjects;
delete  EpochFrom;              delete  EpochTo;                delete  EpochMaps;
delete  BaseFileName;
delete  WriteSegFiles;          delete  WriteClusters;          delete  DeletePreProcFiles;          
}


void    TMicroStatesFitFilesDialog::SetupWindow ()
{
TMicroStatesFitDialog::SetupWindow ();

SetEnumEpochs ();

GroupsAllRis        = GoGoF.AllExtensionsAre ( FILEEXT_RIS );
}


//----------------------------------------------------------------------------
void    TMicroStatesFitFilesDialog::SetEnumEpochs ( int maxtf )
{
//DBGV3 ( maxtf, GoGoF.NumGroups (), MaxNumTF, "" );

if ( maxtf == 0 ) {

    if ( GoGoF.IsEmpty () )
        return;
                                        // take the biggest length!
    MaxNumTF    = GoGoF.GetMaxNumTF ();
    }
else                                    // overriden by caller
    MaxNumTF    = maxtf;


EnumEpochs          = TSelection ( MaxNumTF, OrderSorted );
                                        // fill with existing values
char                from[EditSizeValue];
char                to  [EditSizeValue];
int                 ifrom;
int                 ito;

EnumEpochs.Reset ();

for ( int i=0; i < EpochMaps->GetCount(); i++ ) {
    EpochFrom->GetString ( from, i );
    EpochTo  ->GetString ( to,   i );

    ifrom   = StringToInteger ( from );
    ito     = StringToInteger ( to   );

    EnumEpochs.Set ( ifrom, ito );
    }
}


//----------------------------------------------------------------------------
void    TMicroStatesFitFilesDialog::CmBrowseTemplateFileName ()
{
SetTemplateFileName ( 0 );
}


void    TMicroStatesFitFilesDialog::SetTemplateFileName ( const char* file )
{
static GetFileFromUser  getfile ( "Templates File", AllErpRisFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {
    if ( ! getfile.Execute ( FitTransfer.TemplateFileName ) )
        return;

    TransferData ( tdSetData );
    }
else {
    StringCopy ( FitTransfer.TemplateFileName, file );
    getfile.SetOnly ( file );
    }


TransferData ( tdSetData );

TemplateFileName->ResetCaret;


CmClearEpochs ();

SetBaseFilename ();

                                        // test the compatibility between the new template and the existing groups of files
if ( GoGoF.IsNotEmpty () && ! CheckTracksGroup ( GoGoF.GetLast () ) )
                                        // only then erasing the whole existing group
    CmClearGroups ();
}


//----------------------------------------------------------------------------
bool    TMicroStatesFitFilesDialog::CheckTracksGroup ( const TGoF& gof )
{
                                        // 1) Check gof by itself (same as Segmentation)
TracksCompatibleClass   tc;

gof.AllTracksAreCompatible ( tc );


if      ( tc.NumTracks == CompatibilityNotConsistent ) {
    ShowMessage (   "Files don't seem to have the same number of electrodes/tracks!" NewLine 
                    "Check again your input files...", FittingTitle, ShowMessageWarning );
    return  false;
    }
else if ( tc.NumTracks == CompatibilityIrrelevant ) {
    ShowMessage (   "Files don't seem to have any electrodes/tracks at all!" NewLine 
                    "Check again your input files...", FittingTitle, ShowMessageWarning );
    return  false;
    }

                                        // this test is quite weak, as ReadFromHeader does a lousy job at retrieving the aux tracks (it needs the strings, so opening the whole file)
if      ( tc.NumAuxTracks > 0 ) {
    ShowMessage (   "It is not allowed to run the fitting process with remaining auxiliary tracks!" NewLine 
                    "Check again your input files...", FittingTitle, ShowMessageWarning );
    return  false;
    }


if      ( tc.SamplingFrequency == CompatibilityNotConsistent ) {
    ShowMessage (   "Files don't seem to have the same sampling frequencies!" NewLine 
                    "Check again your input files...", FittingTitle, ShowMessageWarning );
//  if ( ! GetAnswerFromUser (  "Files don't seem to have the same sampling frequencies!" NewLine 
//                              "Do you want to proceed anyway (not recommended)?", FittingTitle ) )
        return  false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Check gof against template
TransferData ( tdGetData );


char               *templfile;
int                 numtrackstempl  = 0;


if ( StringIsNotEmpty ( FitTransfer.TemplateFileName ) ) {

    templfile   = FitTransfer.TemplateFileName;

    if ( ! ReadFromHeader ( templfile, ReadNumElectrodes, &numtrackstempl ) ) {
        ShowMessage ( "Can not read the number of electrodes from templates files!", templfile, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND );
        return false;
        }


    bool                alldateeg       = GoGoF.AllExtensionsAre    ( AllEegFilesExt );
    bool                allmapeeg       = IsExtensionAmong          ( FitTransfer.TemplateFileName, AllEegFilesExt );
    bool                alleeg          = alldateeg && allmapeeg;

    bool                alldatris       = GoGoF.AllExtensionsAre    ( AllRisFilesExt );
    bool                allmapris       = IsExtensionAmong          ( FitTransfer.TemplateFileName, AllRisFilesExt );
    bool                allris          = alldatris && allmapris;


    if ( alldatris && allmapeeg ) {
        ShowMessage (   "Hu? Are you trying to fit some EEG directly into some sorts of ESI?" NewLine 
                        "Nope is the answer.", templfile, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND );
        return false;
        }
                                        // check same number of electrodes for EEG/EEG or RIS/RIS cases - allow the case EEG data with RIS templates
    if ( numtrackstempl != tc.NumTracks && ( alleeg || allris ) ) {
        ShowMessage ( "Not the same number of electrodes with template file!", templfile, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND );
        return false;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Check gof with all the other groups

                                        // temporarily join the gof to the GoGoF, so we can test the compatibility of gof with GoGoF
TGoF ( GoGoF, gof ).AllTracksAreCompatible ( tc );


if      ( tc.NumTracks == CompatibilityNotConsistent ) {
    ShowMessage (   "Files don't seem to have the same number of electrodes/tracks across groups!" NewLine 
                    "Check again your input files...", FittingTitle, ShowMessageWarning );
    return  false;
    }


if      ( tc.SamplingFrequency == CompatibilityNotConsistent ) {
    ShowMessage (   "Files don't seem to have the same sampling frequencies across groups!" NewLine 
                    "Check again your input files...", FittingTitle, ShowMessageWarning );
//  if ( ! GetAnswerFromUser (  "Files don't seem to have the same sampling frequencies!" NewLine 
//                              "Do you want to proceed anyway (not recommended)?", FittingTitle ) )
        return  false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  true;
}


void    TMicroStatesFitFilesDialog::AddGroupSummary ( int gofi )
{
if ( GoGoF.IsEmpty () || gofi < 0 || gofi >= (int) GoGoF )
    return;


TGoF&               gof             = GoGoF[ gofi ];
TFileName           buff;
TFileName           buff2;
TFileName           buff3;

                                        // update dialog
StringCopy ( buff2, ToFileName ( gof.GetFirst () ) );
StringCopy ( buff3, ToFileName ( gof.GetLast  () ) );


if ( gof.NumFiles () == 1 )
    sprintf ( buff, "Group %2d:  %0d File  ( %s )",
              gofi + 1,
              gof.NumFiles (),
              (char*) buff2 );
else
    sprintf ( buff, "Group %2d:  %0d Files  ( %s .. %s )",
              gofi + 1,
              gof.NumFiles (),
              (char*) buff2, (char*) buff3 );


GroupsSummary->InsertString ( buff, 0 );

IntegerToString ( buff, GoGoF.NumGroups () );
NumGroups->Clear ();
NumGroups->Insert ( buff );


SetEnumEpochs ();

//SetBaseFilename ();
}


//----------------------------------------------------------------------------
                                        // generate a smart base name
void    TMicroStatesFitFilesStruct::ComposeBaseFilename ( char* bigmatch )  const
{
/*
                                        // use all the files from groups
if ( GoGoF.IsEmpty () )
    return;


TListIterator<TGoF>     gofiterator;

foreachin ( GoGoF, gofiterator ) {

    TFileName           match;

    gofiterator ()->GetCommonString ( match, ! (int) gofiterator );


    if ( (int) gofiterator ) {
        if ( ! StringContains ( bigmatch, match ) )
            StringAppend ( bigmatch, " ", match );
        }
    else
        StringCopy ( bigmatch, match );
    }
*/

if ( StringIsEmpty ( TemplateFileName ) )
    return;

                                        // use the (single) template file path
StringCopy ( bigmatch, TemplateFileName );

RemoveExtension ( bigmatch );

                                        // finally, compose the base name
PrefixFilename ( bigmatch, "Fit " );
}


void    TMicroStatesFitFilesDialog::SetBaseFilename ()
{
TFileName           basefilename;

FitTransfer.ComposeBaseFilename ( basefilename );

BaseFileName->SetText ( basefilename );

BaseFileName->ResetCaret;
}


void    TMicroStatesFitFilesDialog::CmUpOneDirectory ()
{
RemoveLastDir ( FitTransfer.BaseFileName );

BaseFileName->SetText ( FitTransfer.BaseFileName );

BaseFileName->ResetCaret;
}


void    TMicroStatesFitFilesDialog::CmBrowseOutputFileName ()
{
static GetFileFromUser  getfile ( "Base File Name", AllFilesFilter, 1, GetFilePath );


if ( ! getfile.Execute ( FitTransfer.BaseFileName ) )
    return;


TransferData ( tdSetData );

BaseFileName->ResetCaret;
}


//----------------------------------------------------------------------------
void    TMicroStatesFitFilesDialog::CmAddGroup ()
{
static GetFileFromUser  getfiles ( "Open Files", AllErpEegRisFilesFilter, 1, GetFileMulti );


if ( ! getfiles.Execute () )
    return;


if ( CheckTracksGroup ( getfiles ) ) {

    GoGoF.Add ( getfiles, true, MaxPathShort );

    //SetBaseFilename ();
    SetMaxFilesPerGroup ();
    SetMaxGroupsWithinSubjects ();
    AddGroupSummary ( GoGoF.NumGroups () - 1 );

    GroupsAllRis    = GoGoF.AllExtensionsAre ( FILEEXT_RIS );
    }
}


void    TMicroStatesFitFilesDialog::CmRemoveGroup ()
{
if ( ! GoGoF.RemoveLastGroup () )
    return;

GroupsAllRis    = GoGoF.AllExtensionsAre ( FILEEXT_RIS );


NumGroups->SetIntValue ( GoGoF.NumGroups () );


GroupsSummary->DeleteString ( 0 );

//SetBaseFilename ();
SetMaxFilesPerGroup ();
SetMaxGroupsWithinSubjects ();
}


void    TMicroStatesFitFilesDialog::CmClearGroups ()
{
GoGoF.Reset ();


GroupsAllRis    = GoGoF.AllExtensionsAre ( FILEEXT_RIS );


NumGroups->SetIntValue ( GoGoF.NumGroups () );


GroupsSummary->ClearList ();

//SetBaseFilename ();
SetMaxFilesPerGroup ();
SetMaxGroupsWithinSubjects ();

CmClearEpochs ();
}


void    TMicroStatesFitFilesDialog::CmSortGroups ()
{
if ( GoGoF.IsEmpty () )
    return;


GroupsSummary->ClearList ();


TListIterator<TGoF>     gofiterator;

foreachin ( GoGoF, gofiterator ) {

    gofiterator ()->Sort ();

    AddGroupSummary ( (int) gofiterator );
    }
}


//----------------------------------------------------------------------------
void    TMicroStatesFitFilesDialog::EvDropFiles ( TDropInfo drop )
{
TPointInt           where;
TGoF                tracksfiles     ( drop, BatchFilesExt,      &where  );
TGoF                lmfiles         ( drop, FILEEXT_LM                  );
TGoF                spreadsheetfiles( drop, SpreadSheetFilesExt         );

char                buff[ 256 ];
StringCopy ( buff, BatchFilesExt, " " FILEEXT_LM " " SpreadSheetFilesExt );
TGoF                remainingfiles  ( drop, buff, 0, true );

drop.DragFinish ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // process templates first, as it will reset the groups of data files
if ( (bool) tracksfiles ) {

    bool            templatos       = false;

                                        // D&D a single track file is ambiguous
    if ( (int) tracksfiles == 1
      && where.Y < 121          ) {

        int                 numtf;
        if ( ReadFromHeader ( tracksfiles[ 0 ], ReadNumTimeFrames, &numtf ) )
                                        // 1 file && D&D in template area && reasonably low number of maps
            templatos   = IsInsideLimits ( numtf, 1, 50 );
        }


    for ( int i = 0; i < (int) tracksfiles; i++ )

        if ( templatos )    SetTemplateFileName ( tracksfiles[ i ] );
        else                AddFileToGroup      ( tracksfiles[ i ], i == 0 );


    if ( GoGoF.IsNotEmpty () && ! templatos ) {

        if ( CheckTracksGroup ( GoGoF.GetLast () ) ) {

            SetMaxFilesPerGroup ();
            SetMaxGroupsWithinSubjects ();

            AddGroupSummary ( GoGoF.NumGroups () - 1 );
            }
        else
            GoGoF.RemoveLastGroup ();
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) spreadsheetfiles; i++ )

    ReadParams ( spreadsheetfiles[ i ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) lmfiles ) {

    bool                first;

    for ( int i = 0; i < (int) lmfiles; i++ ) {

        TSplitLinkManyFile      lm ( lmfiles[ i ] );
        TListIterator<char>     iterator;

                                        // coming from .lm, process separately the eeg
        first = true;

        if ( (bool) lm.leeg ) {

            foreachin ( lm.leeg, iterator  )

                if ( ! IsExtension ( iterator (), FILEEXT_FREQ ) ) {    // filters out frequency files
                    AddFileToGroup ( iterator (), first );
                    first = false;
                    }

            if ( GoGoF.IsNotEmpty () && ! first ) { // security check, in case of only frequency files

                if ( CheckTracksGroup ( GoGoF.GetLast () ) ) {

                    SetMaxFilesPerGroup ();
                    SetMaxGroupsWithinSubjects ();

                    AddGroupSummary ( GoGoF.NumGroups () - 1 );
                    }
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

                if ( CheckTracksGroup ( GoGoF.GetLast () ) ) {

                    SetMaxFilesPerGroup ();
                    SetMaxGroupsWithinSubjects ();

                    AddGroupSummary ( GoGoF.NumGroups () - 1 );
                    }
                else
                    GoGoF.RemoveLastGroup ();
                }
            }
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // in case of both drops, the test has to be here
//if ( (int) tracksfiles == 1 )
//    if ( ! CheckTemplate () ) {
//        TemplateFileName->SetText ( "" );
//        }


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

            if ( CheckTracksGroup ( GoGoF.GetLast () ) ) {

                SetMaxFilesPerGroup ();
                SetMaxGroupsWithinSubjects ();

                AddGroupSummary ( GoGoF.NumGroups () - 1 );
                }
            else
                GoGoF.RemoveLastGroup ();
            }

        remainingfiles.RemoveRef ( remainingfiles[ i ] );
        i--;
        }
    }


if ( (bool) remainingfiles )
    remainingfiles.Show ( IrrelevantErrorMessage );
}


//----------------------------------------------------------------------------
void    TMicroStatesFitFilesDialog::CmReadParams ()
{
ReadParams ();
}


void    TMicroStatesFitFilesDialog::AddFileToGroup ( const char* filename, bool first )
{
if ( GoGoF.IsEmpty () || first )        // add a new group
    GoGoF.Add ( new TGoF );


GoGoF.GetLast ().Add ( filename, MaxPathShort );

GroupsAllRis    = GoGoF.AllExtensionsAre ( FILEEXT_RIS );
}


//----------------------------------------------------------------------------
void    TMicroStatesFitFilesDialog::ReadParams ( const char* filename )
{
                                        // TODO: glups .lm files also!!!!!!!!
                                        // TODO: glups whole directory also!!!!!!!!
TSpreadSheet        sf;

if ( ! sf.ReadFile ( filename ) )
    return;


CsvType             csvtype         = sf.GetType ();

                                        // We can deal with statistics type, too...
if ( ! ( IsCsvFitting ( csvtype ) || IsCsvStatFiles ( csvtype ) ) ) {
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
    for ( int fi = 0; fi < numfiles; fi++ ) {
        sprintf ( buff, "file%0d", fi + 1 );
        sf.GetRecord ( file, buff, attr );

        gof->Add ( attr, MaxPathShort );
        } // for file


    if ( GoGoF.IsNotEmpty () ) {

        if ( CheckTracksGroup ( GoGoF.GetLast () ) ) {

            SetMaxFilesPerGroup ();
            SetMaxGroupsWithinSubjects ();

            AddGroupSummary ( GoGoF.NumGroups () - 1 );
            }
        else
            GoGoF.RemoveLastGroup ();
        }
    }


GroupsAllRis    = GoGoF.AllExtensionsAre ( FILEEXT_RIS );
}


void    TMicroStatesFitFilesDialog::CmWriteParams ()
{
if ( GoGoF.IsEmpty () )
    return;


TSpreadSheet        sf;


if ( ! sf.WriteFile () )
    return;

                                        // header line describes the attributes / fields
sf.WriteAttribute ( "numfiles" );


int                 maxfiles        = GoGoF.GetMaxFiles ();


for ( int fi = 0; fi < maxfiles; fi++ )
    sf.WriteAttribute ( "file", fi + 1 );

sf.WriteNextRecord ();

                                        // now write each line
for ( int gogofi = 0; gogofi < (int) GoGoF; gogofi++ ) {

                                        // write parameters
    sf.WriteAttribute ( GoGoF[ gogofi ].NumFiles () );

                                        // write all files
    for ( int fi = 0; fi < GoGoF[ gogofi ].NumFiles (); fi++ )
        sf.WriteAttribute ( GoGoF[ gogofi ][ fi ] );

                                        // complete line with empty attributes
    for ( int fi = GoGoF[ gogofi ].NumFiles (); fi < maxfiles; fi++ )
        sf.WriteAttribute ( "" );

    sf.WriteNextRecord ();
    }


sf.WriteFinished ();
}


//----------------------------------------------------------------------------

constexpr char*     FittingAddEpoch     = "Add Epoch";

void    TMicroStatesFitFilesDialog::CmAddEpoch ()
{
                                        // error if no TFs
if ( EpochFrom->GetTextLen() == 0 || EpochTo->GetTextLen() == 0 ) {
    ShowMessage ( "You must provide a starting and an ending TF.", FittingAddEpoch, ShowMessageWarning );
    return;
    }


char                from[ EditSizeValue ];
char                to  [ EditSizeValue ];
char                maps[ EditSizeText ];
int                 ifrom;
int                 ito;

                                        // transfer from edit to list
EpochFrom->GetText ( from, EditSizeValue );
EpochTo  ->GetText ( to,   EditSizeValue );
EpochMaps->GetText ( maps, EditSizeText );

                                        // check boundaries
ifrom   = StringToInteger ( from );
ito     = StringToInteger ( to   );

Clipped ( ifrom, ito, 0, MaxNumTF - 1 );

                                        // check against previous epochs
                                        // it is not allowed to have some time overlaps
                                        // while it is allowed to have maps overlaps, that is some maps can be fitted in more than one epoch
if ( EnumEpochs.NumSet ( ifrom, ito ) > 0 ) {

    ifrom               = EnumEpochs.FirstNotSelected ( ifrom, ito );

    if ( /*firstnotseli.IsNotOk () ||*/ ifrom < 0 ) {

        ShowMessage (   "Time interval completely overlaps with another interval," NewLine 
                        "epoch rejected.", FittingAddEpoch, ShowMessageWarning );

        ifrom   = EnumEpochs.FirstNotSelected ( ito + 1, MaxNumTF - 1 );

        if ( ifrom >= 0 ) {
            sprintf ( from, "%0i", ifrom );
            EpochFrom->SetText ( from );
            EpochTo  ->SetText ( ""   );
            }
        return;
        }


    int                 oldito          = ito;
                        ito             = EnumEpochs.FirstSelected ( ifrom + 1 ) - 1;

    if ( ito < 0 )
        ito     = oldito;

    sprintf ( from, "%0i", ifrom );
    sprintf ( to,   "%0i", ito   );
    EpochFrom->SetText ( from );
    EpochTo  ->SetText ( to   );

    ShowMessage (   "Time interval partly overlaps with another interval," NewLine 
                    "the first eligible epoch will be suggested.", FittingAddEpoch, ShowMessageWarning );
    return;
    }


EnumEpochs.Set ( ifrom, ito );

sprintf ( from, "%0i", ifrom );
sprintf ( to,   "%0i", ito   );
                                        // default: all maps
if ( StringIsEmpty ( maps ) )
    StringCopy ( maps, "*" );

                                        // now transfer to the list
EpochFrom->InsertString ( from, 0 );
EpochTo  ->InsertString ( to,   0 );
EpochMaps->InsertString ( maps, 0 );

                                        // clear the edit lines
EpochFrom->Clear();
EpochTo  ->Clear();
EpochMaps->Clear();
EpochFrom->SetFocus();

                                        // provide the next 'from'
ito     = EnumEpochs.FirstNotSelected ( ifrom + 1 );
if ( ito >= 0 ) {
    sprintf ( from, "%0i", ito );
    EpochFrom->SetText ( from );
    }
}


void    TMicroStatesFitFilesDialog::CmAddEpochEnable ( TCommandEnabler &tce )
{
tce.Enable ( EnumEpochs.NumSet () < MaxNumTF );
}


void    TMicroStatesFitFilesDialog::CmClearEpochs ()
{
char                buff[ 256 ];

do {
    EpochFrom->DeleteString ( 0 );
    EpochTo  ->DeleteString ( 0 );
    EpochMaps->DeleteString ( 0 );
    } while ( EpochFrom ->GetString ( buff, 0 ) >= 0 );

EpochFrom->SetText ( "" );
EpochTo  ->SetText ( "" );
EpochMaps->SetText ( "" );

EnumEpochs.Reset ();
}


void    TMicroStatesFitFilesDialog::CmRemoveEpoch ()
{
char                mess[ 256 ];
char                from[EditSizeValue];
char                to  [EditSizeValue];
int                 ifrom;
int                 ito;

if ( EpochFrom ->GetString ( from, 0 ) < 0 )
    ClearString ( from );
EpochFrom->DeleteString ( 0 );
EpochFrom->SetText ( from );
ifrom   = StringToInteger ( from );

if ( EpochTo   ->GetString ( to,   0 ) < 0 )
    ClearString ( to );
EpochTo->DeleteString ( 0 );
EpochTo->SetText ( to );
ito     = StringToInteger ( to );

if ( EpochMaps ->GetString ( mess, 0 ) < 0 )
    ClearString ( mess );
EpochMaps->DeleteString ( 0 );
EpochMaps->SetText ( mess );

EnumEpochs.Reset ( ifrom, ito );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TMicroStatesFitParamsDialog, TMicroStatesFitDialog )

    EV_WM_DROPFILES,

    EV_CBN_SELCHANGE            ( IDC_PRESETS,                  EvPresetsChange ),

    EV_COMMAND_ENABLE           ( IDC_SKIPBADEPOCHSAUTO,        CmSkipBadEpochsEnable ),
    EV_COMMAND_ENABLE           ( IDC_SKIPBADEPOCHSLIST,        CmSkipBadEpochsEnable ),
    EV_COMMAND_ENABLE           ( IDC_SKIPMARKERS,              CmSkipBadEpochsListEnable ),

    EV_COMMAND_ENABLE           ( IDC_WINDOWSIZE,               CmSmoothingEnable ),
    EV_COMMAND_ENABLE           ( IDC_STRENGTH,                 CmSmoothingEnable ),
    EV_EN_CHANGE                ( IDC_WINDOWSIZE,               SmoothingWindowsSizeChanged ),
    EV_COMMAND_ENABLE           ( IDC_REJECTSIZE,               CmRejectSmallEnable ),

    EV_COMMAND_ENABLE           ( IDC_MINCORRELATION,           CmMinCorrelationEnable ),

    EV_COMMAND                  ( IDC_SPATIALFILTER,            CmCheckXyzFile ),   // clicking On will launch check-up
    EV_COMMAND                  ( IDC_BROWSEXYZFILE,            CmBrowseXyzFile ),
    EV_COMMAND_ENABLE           ( IDC_SPATIALFILTER,            CmNotESIEnable ),
    EV_COMMAND_ENABLE           ( IDC_BROWSEXYZFILE,            CmXyzEnable ),
    EV_COMMAND_ENABLE           ( IDC_XYZFILE,                  CmXyzEnable ),

    EV_COMMAND_ENABLE           ( IDOK,                         CmOkEnable ),

END_RESPONSE_TABLE;


        TMicroStatesFitParamsDialog::TMicroStatesFitParamsDialog ( TWindow* parent, TResId resId )
      : TMicroStatesFitDialog ( parent, resId )
{
XyzAndEegMatch      = true;
GroupsAllRis        = GoGoF.AllExtensionsAre ( FILEEXT_RIS );


Presets             = new TComboBox ( this, IDC_PRESETS );

SpatialFilter       = new TCheckBox ( this, IDC_SPATIALFILTER );
XyzFile             = new TEdit ( this, IDC_XYZFILE, EditSizeText );

NormalizationNone   = new TRadioButton ( this, IDC_RESCALE_NONE );
NormalizationGfp    = new TRadioButton ( this, IDC_RESCALE_GFP );

SkipBadEpochs       = new TCheckBox ( this, IDC_SKIPBADEPOCHS );
SkipBadEpochsAuto   = new TRadioButton  ( this, IDC_SKIPBADEPOCHSAUTO );
SkipBadEpochsList   = new TRadioButton  ( this, IDC_SKIPBADEPOCHSLIST );
SkipMarkers         = new TEdit ( this, IDC_SKIPMARKERS, EditSizeText );

PositiveData        = new TRadioButton ( this, IDC_POSITIVEDATA );
SignedData          = new TRadioButton ( this, IDC_SIGNEDDATA );
VectorData          = new TRadioButton ( this, IDC_VECTORDATA );

NoRef               = new TRadioButton ( this, IDC_NOREF );
AveRef              = new TRadioButton ( this, IDC_AVEREF );

AccountPolarity     = new TRadioButton ( this, IDC_ACCOUNTPOLARITY );
IgnorePolarity      = new TRadioButton ( this, IDC_IGNOREPOLARITY );

FitSingleSeg        = new TCheckBox ( this, IDC_FITSINGLESEG );

ClipCorrelation     = new TCheckBox ( this, IDC_CLIPCORRELATION );
MinCorrelation      = new TEdit ( this, IDC_MINCORRELATION, EditSizeValue );

Smoothing           = new TCheckBox ( this, IDC_SMOOTHING );
WindowSize          = new TEdit ( this, IDC_WINDOWSIZE, EditSizeValue );
BesagFactor         = new TEdit ( this, IDC_STRENGTH, EditSizeValue );

RejectSmall         = new TCheckBox ( this, IDC_REJECTSMALL );
RejectSize          = new TEdit ( this, IDC_REJECTSIZE, EditSizeValue );


SetTransferBuffer ( dynamic_cast <TMicroStatesFitParamsStruct*> ( &FitTransfer ) );

                                        // TMicroStatesFitDialog constructor updates: MaxFilesPerGroup, MaxGroupsWithinSubjects (MaxWithinSubjects but not NumWithinSubjects), AllGroupsEqual
}


        TMicroStatesFitParamsDialog::~TMicroStatesFitParamsDialog ()
{
delete  Presets;
delete  SpatialFilter;          delete  XyzFile;
delete  NormalizationNone;      delete  NormalizationGfp;
delete  SkipBadEpochs;          
delete  SkipBadEpochsAuto;      delete  SkipBadEpochsList;      delete  SkipMarkers;
delete  PositiveData;           delete  SignedData;             delete  VectorData;
delete  NoRef;                  delete  AveRef;
delete  AccountPolarity;        delete  IgnorePolarity;
delete  FitSingleSeg;
delete  ClipCorrelation;        delete  MinCorrelation;
delete  Smoothing;              delete  WindowSize;             delete  BesagFactor;
delete  RejectSmall;            delete  RejectSize;
}


void    TMicroStatesFitParamsDialog::SetupWindow ()
{
TMicroStatesFitDialog::SetupWindow ();
                                        // A .xyz file was previously set, went back to the files dialog, then again to the parameters:
                                        // (the reciprocate could be done when entering the Files dialog...)
CheckXyzFile ();

                                        // when switching back to this panel, files might have changed
uint                clflags         = FitPresets[ Presets->GetSelIndex () ].ClusteringFlags;

if ( ( clflags & SegPresetSpatialFilterOn ) && ! GroupsAllRis )
                                        // reset Spatial Filter if file names already contain .SpatialFilter
    SpatialFilter   ->SetCheck ( ! GoGoF.AllStringsGrep ( PostfixSpatialGrep, GrepOptionDefaultFiles ) );
}


//----------------------------------------------------------------------------
void    TMicroStatesFitParamsDialog::EvPresetsChange ()
{
int                     presetparam     = Presets->GetSelIndex ();
const FitPresetSpec&    preset          = FitPresets[ presetparam ];
uint                    clflags         = preset.ClusteringFlags;
uint                    ppflags         = preset.PostProcFlags;


if ( clflags == 0 )
    return;

                                        // will set and/or reset all our options
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

                                        // !actually, no - only for spontaneous case!
SkipBadEpochs    ->SetCheck ( BoolToCheck ( clflags & SegPresetRSIndiv            ) );


NormalizationNone->SetCheck ( BoolToCheck ( clflags & SegPresetGfpNormOff         ) );
NormalizationGfp ->SetCheck ( BoolToCheck ( clflags & SegPresetGfpNormOn          ) );


if ( ! ( ppflags & SegPresetSmoothingDontCare ) )
    Smoothing   ->SetCheck ( BoolToCheck ( ppflags & SegPresetSmoothingOn        ) );    // can also test for  SegPresetSmoothingOff

if ( ! ( ppflags & SegPresetRejectSmallDontCare ) )
    RejectSmall ->SetCheck ( BoolToCheck ( ppflags & SegPresetRejectSmallOn      ) );    // can also test for  SegPresetRejectSmallOff


if ( preset.SmoothingSize > 0 ) {
    WindowSize->SetIntValue ( preset.SmoothingSize );
    RejectSize->SetIntValue ( preset.SmoothingSize );
    }


if ( preset.CorrelationThreshold > -100 ) {
    ClipCorrelation ->SetCheck ( BoolToCheck ( true ) );
    MinCorrelation  ->SetIntValue ( Clip ( preset.CorrelationThreshold, -100, 100 ) );
    }
else
    ClipCorrelation ->SetCheck ( BoolToCheck ( false ) );

                                        // Set variables of dialog #3 from the preset, just in case user doesn't go there
FitTransfer.SetVariables ( preset );
}


//----------------------------------------------------------------------------
void    TMicroStatesFitParamsDialog::CmNotESIEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( ! FitTransfer.IsFitESIPreset () && ! GroupsAllRis );
}


//----------------------------------------------------------------------------
void    TMicroStatesFitParamsDialog::CmCheckXyzFile ()
{
CheckXyzFile ();
}


void    TMicroStatesFitParamsDialog::CmBrowseXyzFile ()
{
SetXyzFile ( 0 );
}


void    TMicroStatesFitParamsDialog::SetXyzFile ( const char* file )
{
static GetFileFromUser  getfile ( "Electrodes Coordinates File", AllCoordinatesFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( FitTransfer.XyzFile ) )
        return;

    TransferData ( tdSetData );
    }
else {
    StringCopy ( FitTransfer.XyzFile, file );
    getfile.SetOnly ( file );
    }


TransferData ( tdSetData );

CheckXyzFile ();
}


void    TMicroStatesFitParamsDialog::CmXyzEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! FitTransfer.IsFitESIPreset () && ! GroupsAllRis && CheckToBool ( SpatialFilter->GetCheck () ) );
}


//----------------------------------------------------------------------------
void    TMicroStatesFitParamsDialog::EvDropFiles ( TDropInfo drop )
{
TGoF                xyzfiles        ( drop, AllCoordinatesFilesExt );
TGoF                remainingfiles  ( drop, AllInverseFilesExt " " AllCoordinatesFilesExt, 0, true );

drop.DragFinish ();


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
void    TMicroStatesFitParamsDialog::CmSmoothingEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( Smoothing->GetCheck() ) );
}


void    TMicroStatesFitParamsDialog::CmRejectSmallEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( RejectSmall->GetCheck() ) );
}


void    TMicroStatesFitParamsDialog::SmoothingWindowsSizeChanged ()
{
if ( ! ( BoolToCheck ( Smoothing->GetCheck () ) && BoolToCheck ( RejectSmall->GetCheck () ) ) )
    return;


char                buff[ 256 ];

WindowSize->GetText ( buff, 256 );

if ( ! StringIsSpace ( buff ) )
    RejectSize->SetText ( buff );
}


//----------------------------------------------------------------------------
void    TMicroStatesFitParamsDialog::CmReferenceEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( SignedData->GetCheck() ) );
}


void    TMicroStatesFitParamsDialog::CmPolarityEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! CheckToBool ( PositiveData->GetCheck() ) );
}


void    TMicroStatesFitParamsDialog::CmMinCorrelationEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( ClipCorrelation->GetCheck () ) );
}


void    TMicroStatesFitParamsDialog::CmSkipBadEpochsEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( SkipBadEpochs->GetCheck () ) );
}


void    TMicroStatesFitParamsDialog::CmSkipBadEpochsListEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( SkipBadEpochs->GetCheck () ) && CheckToBool ( SkipBadEpochsList->GetCheck () ) );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TMicroStatesFitResultsDialog, TMicroStatesFitDialog )

//    EV_WM_DROPFILES,

    EV_COMMAND_AND_ID           ( IDC_ALLON,                CmAllOnOff ),
    EV_COMMAND_AND_ID           ( IDC_ALLOFF,               CmAllOnOff ),

    EV_COMMAND_ENABLE           ( IDC_FONSET,               CmVariableEnable ),
    EV_COMMAND_ENABLE           ( IDC_LOFFSET,              CmVariableEnable ),
    EV_COMMAND_ENABLE           ( IDC_NUMTF,                CmVariableEnable ),
    EV_COMMAND_ENABLE           ( IDC_MAXGFP,               CmVariableEnable ),
    EV_COMMAND_ENABLE           ( IDC_TFMAXGFP,             CmVariableEnable ),
    EV_COMMAND_ENABLE           ( IDC_MEANGFP,              CmVariableEnable ),
    EV_COMMAND_ENABLE           ( IDC_TFCENTROID,           CmVariableEnable ),
    EV_COMMAND_ENABLE           ( IDC_MEANDURATION,         CmVariableEnable ),
    EV_COMMAND_ENABLE           ( IDC_TIMECOVERAGE,         CmVariableEnable ),
    EV_COMMAND_ENABLE           ( IDC_SEGDENSITY,           CmVariableEnable ),
//  EV_COMMAND_ENABLE           ( IDC_MEANCORR,             CmVariableEnable ),
//  EV_COMMAND_ENABLE           ( IDC_GEV,                  CmVariableEnable ),

    EV_COMMAND_ENABLE           ( IDC_MARKOVSEGMENT,        CmVariableEnable ),
    EV_COMMAND_ENABLE           ( IDC_MARKOVNUMTRANSITIONS, CmMarkovEnable ),

    EV_COMMAND_ENABLE           ( IDC_WRITESTATDURATIONS,   CmVariableEnable ),
    EV_COMMAND_ENABLE           ( IDC_WRITESEGFREQ,         CmVariableEnable ),

    EV_COMMAND_ENABLE           ( IDOK,                     CmOkEnable ),

END_RESPONSE_TABLE;


        TMicroStatesFitResultsDialog::TMicroStatesFitResultsDialog ( TWindow* parent, TResId resId )
      : TMicroStatesFitDialog ( parent, resId )
{
XyzAndEegMatch      = true;
GroupsAllRis        = GoGoF.AllExtensionsAre ( FILEEXT_RIS );


FOnset              = new TCheckBox ( this, IDC_FONSET );
LOffset             = new TCheckBox ( this, IDC_LOFFSET );
NumTf               = new TCheckBox ( this, IDC_NUMTF );
TfCentroid          = new TCheckBox ( this, IDC_TFCENTROID );
MeanCorr            = new TCheckBox ( this, IDC_MEANCORR );
Gev                 = new TCheckBox ( this, IDC_GEV );
BestCorr            = new TCheckBox ( this, IDC_BCORR );
TfBestCorr          = new TCheckBox ( this, IDC_TFBCORR );
GfpTfBestCorr       = new TCheckBox ( this, IDC_GFPTFBCORR );
MaxGfp              = new TCheckBox ( this, IDC_MAXGFP );
TfMaxGfp            = new TCheckBox ( this, IDC_TFMAXGFP );
MeanGfp             = new TCheckBox ( this, IDC_MEANGFP );
MeanDuration        = new TCheckBox ( this, IDC_MEANDURATION );
TimeCoverage        = new TCheckBox ( this, IDC_TIMECOVERAGE );
SegDensity          = new TCheckBox ( this, IDC_SEGDENSITY );

LinesAsSamples      = new TCheckBox ( this, IDC_LINESASSAMPLES );
ColumnsAsFactors    = new TCheckBox ( this, IDC_COLUMNSASFACTORS );
OneFilePerGroup     = new TCheckBox ( this, IDC_SEPARATEGROUPS );
OneFilePerVariable  = new TCheckBox ( this, IDC_SEPARATEVARIABLES );
ShortVariableNames  = new TCheckBox ( this, IDC_SHORTVARIABLENAMES );

MarkovSegment       = new TRadioButton ( this, IDC_MARKOVSEGMENT );
MarkovNumTransitions= new TEdit ( this, IDC_MARKOVNUMTRANSITIONS, EditSizeValue );

WriteStatDurations  = new TCheckBox ( this, IDC_WRITESTATDURATIONS );
WriteCorrFiles      = new TCheckBox ( this, IDC_WRITECORRFILES );
WriteSegFrequency   = new TCheckBox ( this, IDC_WRITESEGFREQ );


SetTransferBuffer ( dynamic_cast <TMicroStatesFitResultsStruct*> ( &FitTransfer ) );


CheckXyzFile ();
//CheckISFile  ();

                                        // TMicroStatesFitDialog constructor updates: MaxFilesPerGroup, MaxGroupsWithinSubjects (MaxWithinSubjects but not NumWithinSubjects), AllGroupsEqual

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Init output flags
                                        // Problem is that it will override any previous user's choice, when coming back and forth to this dialog
                                        // but this is still better than keeping the wrong variables set from another preset...
int                     presetparam     = FitTransfer.GetPresetParamsIndex ();
const FitPresetSpec&    preset          = FitPresets[ presetparam ];


FitTransfer.SetVariables ( preset );
}


        TMicroStatesFitResultsDialog::~TMicroStatesFitResultsDialog ()
{
delete  FOnset;                 delete  LOffset;
delete  NumTf;                  delete  TfCentroid;
delete  MeanCorr;               delete  Gev;
delete  BestCorr;               delete  TfBestCorr;             delete  GfpTfBestCorr;
delete  MaxGfp;                 delete  TfMaxGfp;               delete  MeanGfp;
delete  MeanDuration;           delete  TimeCoverage;           delete  SegDensity;
delete  LinesAsSamples;         delete  ColumnsAsFactors;
delete  OneFilePerGroup;        delete  OneFilePerVariable;
delete  ShortVariableNames;
delete  MarkovSegment;          delete  MarkovNumTransitions;
delete  WriteStatDurations;     delete  WriteCorrFiles;         delete  WriteSegFrequency;
}


//----------------------------------------------------------------------------
void    TMicroStatesFitResultsDialog::CmAllOnOff ( owlwparam w )
{
TransferData ( tdGetData );

if      ( w == IDC_ALLON  )     FitTransfer.SetVariablesOn ();
else if ( w == IDC_ALLOFF )     FitTransfer.SetVariablesOff ();


TransferData ( tdSetData );
}


void    TMicroStatesFitResultsDialog::CmMarkovEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! CheckToBool ( FitTransfer.FitSingleSeg ) && CheckToBool ( MarkovSegment->GetCheck() ) );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TMicroStatesFitDialog, TBaseDialog )

//    EV_WM_DROPFILES,

    EV_COMMAND                  ( IDOK,                     CmOk ),
    EV_COMMAND_ENABLE           ( IDOK,                     CmOkEnable ),

    EV_COMMAND_AND_ID           ( IDC_TOPREVDIALOG,         CmToDialog ),
    EV_COMMAND_AND_ID           ( IDC_TONEXTDIALOG,         CmToDialog ),

END_RESPONSE_TABLE;


        TMicroStatesFitDialog::TMicroStatesFitDialog ( TWindow* parent, TResId resId )
      : TBaseDialog ( parent, resId )
{
StringCopy ( BatchFilesExt, AllEegRisFilesExt );

MaxFilesPerGroup        = 0;
MaxGroupsWithinSubjects = 1;
AllGroupsEqual          = true;

                                        // GoGoF is static and therefore saved across calls and dialogs

SetMaxFilesPerGroup ();                 // updates MaxFilesPerGroup
SetMaxGroupsWithinSubjects ( false );   // updates MaxGroupsWithinSubjects, MaxWithinSubjects but not NumWithinSubjects (this could be set by the user - it will be reset when adding/removing files, though)
SetAllGroupsEqual ();                   // updates AllGroupsEqual
}


//----------------------------------------------------------------------------
void    TMicroStatesFitDialog::CmToDialog ( owlwparam w )
{
uint                ResId           = PtrToUint ( Attr.Name );

                                        // goto new id
uint                todialogid      = Clip ( ResId + ( w == IDC_TOPREVDIALOG ? -1 : 1 ), (uint) IDD_FITTING1, (uint) IDD_FITTING3 );

                                        // avoid calling itself
if ( todialogid == ResId )
    return;


Destroy ();                             // remove the window, not the object

                                        // remember the last dialog
FitTransfer.LastDialogId    = todialogid;


if      ( todialogid == IDD_FITTING1 )  TMicroStatesFitFilesDialog  ( CartoolMdiClient, IDD_FITTING1 ).Execute ();
else if ( todialogid == IDD_FITTING2 )  TMicroStatesFitParamsDialog ( CartoolMdiClient, IDD_FITTING2 ).Execute ();
else if ( todialogid == IDD_FITTING3 )  TMicroStatesFitResultsDialog( CartoolMdiClient, IDD_FITTING3 ).Execute ();
}


//----------------------------------------------------------------------------
void    TMicroStatesFitDialog::SetMaxFilesPerGroup ( int gofi1 , int gofi2 )
{
MaxFilesPerGroup    = 0;

if ( GoGoF.IsEmpty () )
    return;
                                        // retrieve the max # of files across specifed groups (used for matrix output)
                                        // OK for gofi1 == -1 / gofi2 == -1
MaxFilesPerGroup    = GoGoF.GetMaxFiles    ( gofi1, gofi2 );
}


//----------------------------------------------------------------------------
                                        // Find the biggest number of consecutive groups with the same number of files
void    TMicroStatesFitDialog::SetMaxGroupsWithinSubjects ( bool updateNumWithinSubjects )
{
MaxGroupsWithinSubjects     = GoGoF.GetMaxGroupsWithinSubjects ();

if ( GoGoF.IsEmpty () )
    return;


TMicroStatesFitFilesDialog* dlgfiles    = dynamic_cast< TMicroStatesFitFilesDialog* > ( this );

if ( dlgfiles ) {
                                        // update dialog
                                        // set limit
    (dlgfiles->MaxWithinSubjects)->SetIntValue ( MaxGroupsWithinSubjects );

                                        // suggest to user the biggest one
    if ( updateNumWithinSubjects )
        (dlgfiles->NumWithinSubjects)->SetIntValue ( MaxGroupsWithinSubjects );
    }
}


//----------------------------------------------------------------------------
void    TMicroStatesFitDialog::SetAllGroupsEqual ()
{
                                        // checks the # within subjects is OK, AND there is no group left over
int                 numwithinsubjects   = StringToInteger ( FitTransfer.NumWithinSubjects );

                                        // MaxGroupsWithinSubjects has the hardest part already done, just check there are no leftovers...
AllGroupsEqual      = GoGoF.NumGroups () <= 1 || MaxGroupsWithinSubjects % numwithinsubjects == 0;
}


//----------------------------------------------------------------------------
void    TMicroStatesFitDialog::CheckXyzFile ()
{
//TransferData ( tdGetData );

XyzAndEegMatch      = true;

if ( /*! CheckToBool ( SpatialFilter->GetCheck () ) ||*/ StringIsEmpty ( FitTransfer.XyzFile ) || GoGoF.IsEmpty () )
    return;


TPoints             xyz ( FitTransfer.XyzFile );
int                 xyznumel        = xyz.GetNumPoints ();

                                        // used to test only the first file of each group
TracksCompatibleClass   tc;

GoGoF.AllTracksAreCompatible ( tc );


if      ( tc.NumTracks != xyznumel ) {

    XyzAndEegMatch      = false;

    char                buff[ 256 ];
    sprintf ( buff, "Not the same amount of Electrodes:" NewLine NewLine 
                    "  - EEG files "       Tab "= %0d" NewLine 
                    "  - Electrodes file " Tab "= %0d" NewLine NewLine 
                    "Please check again your files!", tc.NumTracks, xyznumel );
    ShowMessage ( buff, FittingTitle, ShowMessageWarning, this );

    ClearString ( FitTransfer.XyzFile );
    TransferData ( tdSetData );
    }

}


//----------------------------------------------------------------------------
void    TMicroStatesFitDialog::CmVariableEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! CheckToBool ( FitTransfer.FitSingleSeg ) );
}


//----------------------------------------------------------------------------
void    TMicroStatesFitDialog::CmOkEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );


if ( ! CanOpenFile ( FitTransfer.TemplateFileName ) ) {
    tce.Enable ( false );
    return;
    }

if ( GoGoF.IsEmpty () ) {
    tce.Enable ( false );
    return;
    }

int                 win             = StringToInteger ( FitTransfer.WindowSize );
double              besag           = StringToDouble  ( FitTransfer.BesagFactor );

if ( CheckToBool ( FitTransfer.Smoothing ) && ( win <= 0 || besag <= 0 ) ) {
    tce.Enable ( false );
    return;
    }


int                 ssz             = StringToInteger ( FitTransfer.RejectSize );

if ( CheckToBool ( FitTransfer.RejectSmall ) && ssz <= 0 ) {
    tce.Enable ( false );
    return;
    }

                                        // checks the # within subjects is OK, AND there is no group left over
int                 numwithinsubjects   = StringToInteger ( FitTransfer.NumWithinSubjects );

if ( ! IsInsideLimits ( numwithinsubjects, 1, MaxGroupsWithinSubjects ) ) {
    tce.Enable ( false );
    return;
    }


SetAllGroupsEqual ();

if ( ! AllGroupsEqual ) {
    tce.Enable ( false );
    return;
    }


if ( CheckToBool ( FitTransfer.SkipBadEpochs ) && CheckToBool ( FitTransfer.SkipBadEpochsList ) && StringIsEmpty ( FitTransfer.SkipMarkers ) ) {
    tce.Enable ( false );
    return;
    }


if ( CheckToBool ( FitTransfer.SpatialFilter ) && ( ! XyzAndEegMatch || StringIsEmpty ( FitTransfer.XyzFile ) ) ) {
    tce.Enable ( false );
    return;
    }


if ( FitTransfer.IsFitESIPreset () && ! GroupsAllRis ) {
    tce.Enable ( false );
    return;
    }


if ( CheckToBool ( FitTransfer.ClipCorrelation ) && StringIsEmpty ( FitTransfer.MinCorrelation ) ) {
    tce.Enable ( false );
    return;
    }


int                 mnt             = StringToInteger ( FitTransfer.MarkovNumTransitions );

if ( CheckToBool ( FitTransfer.MarkovSegment )
  && mnt <= 0 ) {
    tce.Enable ( false );
    return;
    }

                                        // at least a little bit of output?
if ( ! ( CheckToBool ( FitTransfer.FOnset        ) || CheckToBool ( FitTransfer.LOffset      ) || CheckToBool ( FitTransfer.NumTf        )
      || CheckToBool ( FitTransfer.Gev           ) || CheckToBool ( FitTransfer.BestCorr     ) || CheckToBool ( FitTransfer.TfBestCorr   )
      || CheckToBool ( FitTransfer.GfpTfBestCorr ) || CheckToBool ( FitTransfer.MaxGfp       ) || CheckToBool ( FitTransfer.TfMaxGfp     )
      || CheckToBool ( FitTransfer.MeanGfp       ) || CheckToBool ( FitTransfer.TfCentroid   ) || CheckToBool ( FitTransfer.MeanCorr     )
      || CheckToBool ( FitTransfer.MeanDuration  ) || CheckToBool ( FitTransfer.TimeCoverage ) || CheckToBool ( FitTransfer.SegDensity   )
      || CheckToBool ( FitTransfer.WriteSegFiles      )
      || CheckToBool ( FitTransfer.WriteClusters      )
      || CheckToBool ( FitTransfer.WriteStatDurations )
      || CheckToBool ( FitTransfer.WriteCorrFiles     )
      || CheckToBool ( FitTransfer.WriteSegFrequency  )
      || CheckToBool ( FitTransfer.MarkovSegment      ) 
       ) ) {
    tce.Enable ( false );
    return;
    }

                                        // directly playing with the buffer does not seem to be a good idea, maybe its updated during the test?
TFileName           buff;
StringCopy ( buff, FitTransfer.BaseFileName );

if ( ! IsAbsoluteFilename ( buff ) ) {
    tce.Enable ( false );
    return;
    }


tce.Enable ( true );
}


//----------------------------------------------------------------------------
                                        // Preprocess the input groups of files to get ready for a segmentation
                                        // which means, crop, filter, do ESI stuff etc..., creating temp files if needed
                                        // then calling the segmentation on these files
                                        // It also updates the base file name accordingly.
void    TMicroStatesFitDialog::CmOk ()
{
Destroy ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We shouldn't fit any duplicated files
                                        // Plus it will mess up the creation of the temp preprocessed files, as some will be overwritten...
if ( ! GoGoF.HasNoDuplicates () ) {
    ShowMessage (   "There are some duplicate input files, which is quite suspicious." NewLine 
                    "Check your input files and please come back...", FittingTitle, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

FitPresetParamsEnum presetparam         = (FitPresetParamsEnum) FitTransfer.PresetsParams.GetSelIndex ();

TypeClusteringFlags cflags          = FitPresets[ presetparam ].ClusteringFlags;

                                        // transform presets into parameters (could also be directly defines in predefined presets):
AnalysisType        analysis        = IsFlag ( cflags, SegPresetERP     )         ? AnalysisERP
                                    : IsFlag ( cflags, SegPresetRSIndiv )         ? AnalysisRestingStatesIndiv
                                    : IsFlag ( cflags, SegPresetRSGroup )         ? AnalysisRestingStatesIndiv // Group flag is not actually used for computation, it is slightly abused to avoid Skip Bad Epochs in EvPresetsChange
                                    :                                               UnknownAnalysis;
                                                                                  
ModalityType        modality        = IsFitEEGPreset ( presetparam )              ? ModalityEEG
                                    : IsFitMEGPreset ( presetparam )              ? ModalityMEG
                                    : IsFitESIPreset ( presetparam )              ? ModalityESI
                                    :                                               UnknownModality;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TStrings            epochfrom ( FitTransfer.EpochFrom.GetStrings () );
TStrings            epochto   ( FitTransfer.EpochTo  .GetStrings () );
TStrings            epochmaps ( FitTransfer.EpochMaps.GetStrings () );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // listbadepochs should be non-empty, it is not generated automatically here (yet)
SkippingEpochsType  badepochs       = ! CheckToBool ( FitTransfer.SkipBadEpochs     ) ? NoSkippingBadEpochs
                                    :   CheckToBool ( FitTransfer.SkipBadEpochsAuto ) ? SkippingBadEpochsAuto
                                    :   CheckToBool ( FitTransfer.SkipBadEpochsList ) ? SkippingBadEpochsList
                                    :                                                   NoSkippingBadEpochs;


char                listbadepochs [ 4 * KiloByte ];

if ( badepochs == SkippingBadEpochsList ) {

    StringCopy ( listbadepochs,    FitTransfer.SkipMarkers );

    if ( StringIsEmpty ( listbadepochs ) )
        badepochs   = SkippingBadEpochsAuto;
    }
else
    ClearString ( listbadepochs );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Set user's wish, but this will be checked against a few mandatory conditions
SpatialFilterType   spatialfilter   = (    CheckToBool ( FitTransfer.SpatialFilter ) 
                                        && StringIsNotEmpty ( FitTransfer.XyzFile ) ) ? SpatialFilterDefault : SpatialFilterNone;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check groups and templates are all of the same type, that is not a good idea to mix ESI and EEG!
bool                alldateeg       = GoGoF.AllExtensionsAre    ( AllEegFilesExt );
bool                allmapeeg       = IsExtensionAmong          ( FitTransfer.TemplateFileName, AllEegFilesExt );
//bool              alleeg          = alldateeg && allmapeeg;

bool                alldatris       = GoGoF.AllExtensionsAre    ( AllRisFilesExt );
bool                allmapris       = IsExtensionAmong          ( FitTransfer.TemplateFileName, AllRisFilesExt );
bool                allris          = alldatris && allmapris;

bool                alldatrisv      = alldatris && GoGoF.AllExtensionsAre ( AllRisFilesExt, AtomTypeVector );
char                isscalar;
bool                allmaprisv      = allmapris && ReadFromHeader ( FitTransfer.TemplateFileName, ReadInverseScalar, &isscalar ) && ! isscalar;

bool                allrisv         = alldatrisv && allmaprisv;


                                        // test groups consistency by themselves
if ( ! ( alldateeg || alldatris ) ) {
    ShowMessage (   "Input files are not consistently of EEG or RIS types." NewLine 
                    "Check your input files...", FittingTitle, ShowMessageWarning );
    return;
    }

if ( ! ( allmapeeg || allmapris ) ) {
    ShowMessage (   "Input templates are not consistently of EEG or RIS types." NewLine 
                    "Check your templates file...", FittingTitle, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Number of groups inputted
int                 numgroups           = GoGoF.NumGroups ();
                                        // WITHIN subjects bypass, f.ex. user dropped 4 groups, but only 2 withing subjects so we will process 2 groups, then 2 groups  again
int                 numwithinsubjects   = Clip ( StringToInteger ( FitTransfer.NumWithinSubjects ), 1, numgroups );
                                        // Fitting my repeat across groups
int                 numrepeat           = numgroups / numwithinsubjects;
                                        // Used for the progress bar
int                 maxnumsubjects      = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // It might be a good idea to confirm the data layout has been well understood
char                buff[ KiloByte ];


for ( int numri = 0; numri < numrepeat; numri++ ) {
                                        // Each repetition can legally have different number of subjects...
    int                 gofi1               =   numri       * numwithinsubjects;
    int                 gofi2               = ( numri + 1 ) * numwithinsubjects - 1;
    int                 numgof              = gofi2 - gofi1 + 1;
    int                 numsubjects         = GoGoF.NumFiles ( gofi1, gofi2 ) / numwithinsubjects;

    Maxed ( maxnumsubjects, numsubjects );

    ClearString     ( buff );

                                        // Skip this if it implicitely applies to all groups at once
    if ( numrepeat > 1 ) {
        if ( numgof == 1 )
            StringAppend    ( buff, "For group of files ",  IntegerToString ( gofi1 + 1 ),                                        NewLine NewLine );
        else
            StringAppend    ( buff, "For groups of files ", IntegerToString ( gofi1 + 1 ), " to ", IntegerToString ( gofi2 + 1 ), NewLine NewLine );
        }

    StringAppend    ( buff, "Do you confirm your data layout is:" NewLine NewLine "    " );

    StringAppend    ( buff, IntegerToString ( numsubjects ),        " ", "Subject",   StringPlural ( numsubjects ) );

    StringAppend    ( buff, "  x  " );

    StringAppend    ( buff, IntegerToString ( numwithinsubjects ),  " ", "Condition", StringPlural ( numwithinsubjects ) );


    if ( ! GetAnswerFromUser   ( buff, FittingTitle, this ) ) {
        ShowMessage ( "Please check your presets and/or input files and come back again...", FittingTitle, ShowMessageWarning, this );
        return;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check ESI parameters
bool                isesipreset     = FitTransfer.IsFitESIPreset ();

if ( isesipreset  ) {
                                        // either we load ris files, or we compute them, all other cases will fail...
    if ( ! allris ) {
        ShowMessage ( "Sorry, can't process in the ESI space with the given parameters!", FittingTitle, ShowMessageWarning );
        return;
        }

                                            // We currently don't have a 3D filter for ESI in terms of Solution Points...
    if ( spatialfilter != SpatialFilterNone && allris )
        spatialfilter   = SpatialFilterNone;

    } // if isesipreset

else {                                  // NOT computing the ESI, input shouldn't mix EEG and RIS

    if ( alldatris && ! allmapris 
      || alldateeg && ! allmapeeg ) {
        ShowMessage (   "File types are not consistent between the Templates file" NewLine 
                        "and the Data files.", FittingTitle, ShowMessageWarning );
        return;
        }
                                        // don't mix RIS vectorial and RIS scalar  which is certainly an error or a misunderstanding
                                        // we CAN load any vectorial ris into scalar, but we won't
    if (   alldatrisv                   && ( allmapris && ! allmaprisv )
      || ( alldatris  && ! alldatrisv ) &&   allmaprisv ) {
        ShowMessage (   "You are trying to mix RIS files of types Scalar and Vectorial" NewLine 
                        "which is certainly not a good idea.", FittingTitle, ShowMessageWarning );
        return;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Data type & reference

                                        // We have to link datatype and reference: average ref <-> scalar, so that the norm of data corresponds to the GFP, not the RMS
AtomType            datatype;
if      ( CheckToBool ( FitTransfer.PositiveData   ) )      datatype    = AtomTypePositive;
else if ( CheckToBool ( FitTransfer.SignedData     ) )      datatype    = AtomTypeScalar;
else if ( CheckToBool ( FitTransfer.VectorData     ) )      datatype    = AtomTypeVector;
else                                                        datatype    = AtomTypeScalar;

                                        // downgrade vectorial type to scalar if not all data are vectorial
if ( IsVector ( datatype ) && ! allrisv )                   datatype    = AtomTypePositive;

                                        // this also shouldn't happen...
if ( IsVector ( datatype ) && spatialfilter != SpatialFilterNone )
    spatialfilter   = SpatialFilterNone;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ReferenceType       dataref;
if      ( CheckToBool ( FitTransfer.AveRef ) )              dataref     = ReferenceAverage;
else /*if ( CheckToBool ( FitTransfer.NoRef ) )*/           dataref     = ReferenceAsInFile;


CheckReference ( dataref, datatype );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // !Segmentation code has been updated to handle Vectorial case + polarity
                                        // !but as we don't plan to use it anymore, Fitting code is therefore NOT updated
                                        // !Should it be the case one day, see the Segmentation code
                                                                                   // not Positive, not Vectorial
PolarityType        polarity        = CheckToBool ( FitTransfer.IgnorePolarity ) && ! IsAbsolute ( datatype )   ? PolarityEvaluate 
                                                                                                                : PolarityDirect;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Be general here, we might need some Butterworth or other temporal filter later
bool                timelyfilter        = spatialfilter != SpatialFilterNone;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // some consistency: avoiding doing twice a Z-Score
bool                dataallzscore       = GoGoF.AllStringsGrep (                               PostfixStandardizationGrep,   GrepOptionDefaultFiles );
//bool              dataallnozscore     = GoGoF.AllStringsGrep (                               PostfixNoStandardizationGrep, GrepOptionDefaultFiles );
bool                templzscore         =       StringGrep     ( FitTransfer.TemplateFileName, PostfixStandardizationGrep,   GrepOptionDefaultFiles );
//bool              templnozscore       =       StringGrep     ( FitTransfer.TemplateFileName, PostfixNoStandardizationGrep, GrepOptionDefaultFiles );

                                        // more testing ZScore across templates and data
if ( templzscore && ! dataallzscore )
    ShowMessage (   "You seemed to load some standardized templates, but not all input data files seemed standardized either!" NewLine 
                    "Just to let you know that this is not the best configuration, but processing will continue anyway...", FittingTitle, ShowMessageWarning );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                gfpnormalize        = CheckToBool ( FitTransfer.NormalizationGfp );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                noncompetitive      = CheckToBool ( FitTransfer.FitSingleSeg );
bool                competitive         = ! noncompetitive;

bool                dolimitcorr         = CheckToBool ( FitTransfer.ClipCorrelation );
double              limitcorr           = dolimitcorr   ? Clip ( StringToDouble ( FitTransfer.MinCorrelation ) / 100.0, MinCorrelationThreshold, MaxCorrelationThreshold ) 
                                                        : IgnoreCorrelationThreshold;
                                        // force reset flag?
if ( limitcorr <= MinCorrelationThreshold ) {
    dolimitcorr     = false;
    limitcorr       = IgnoreCorrelationThreshold;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                smoothing           = CheckToBool     ( FitTransfer.Smoothing );
int                 smoothinghalfsize   = StringToInteger ( FitTransfer.WindowSize );
double              smoothinglambda     = StringToInteger ( FitTransfer.BesagFactor );

bool                rejectsmall         = CheckToBool     ( FitTransfer.RejectSmall );
int                 rejectsize          = StringToInteger ( FitTransfer.RejectSize );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Set the many output flags
MicroStatesOutFlags outputflags         = NoMicroStatesOutFlags;

if ( CheckToBool ( FitTransfer.ShortVariableNames   ) )                 SetFlags ( outputflags, VariablesShortNames         );
else                                                                    SetFlags ( outputflags, VariablesLongNames          );

if ( CheckToBool ( FitTransfer.LinesAsSamples       ) )                 SetFlags ( outputflags, SheetLinesAsSamples         );
if ( CheckToBool ( FitTransfer.ColumnsAsFactors     ) )                 SetFlags ( outputflags, SheetColumnsAsFactors       );
if ( CheckToBool ( FitTransfer.OneFilePerGroup      ) )                 SetFlags ( outputflags, SaveOneFilePerGroup         );
if ( CheckToBool ( FitTransfer.OneFilePerVariable   ) )                 SetFlags ( outputflags, SaveOneFilePerVariable      );

if ( CheckToBool ( FitTransfer.WriteSegFiles        ) && competitive )  SetFlags ( outputflags, WriteSegFiles               );
if ( CheckToBool ( FitTransfer.WriteClusters        ) )                 SetFlags ( outputflags, WriteClustersFiles          );
if ( AllowEmptyClusterFiles                         )                   SetFlags ( outputflags, WriteEmptyClusters          );  // controlled by global variable
if ( SavingNormalizedClusters                       )                   SetFlags ( outputflags, WriteNormalizedClusters     );  // controlled by global variable
if ( CheckToBool ( FitTransfer.WriteStatDurations   ) && competitive )  SetFlags ( outputflags, WriteStatDurationsFiles     );
if ( CheckToBool ( FitTransfer.WriteCorrFiles       ) )                 SetFlags ( outputflags, WriteCorrelationFiles       );
if ( CheckToBool ( FitTransfer.WriteSegFrequency    ) && competitive )  SetFlags ( outputflags, WriteSegFrequencyFiles      );

//if ( newfiles )                                                       SetFlags ( outputflags, OwningFiles                 );  // PreProcessFiles will tell us if it created new files or not
if ( CheckToBool ( FitTransfer.DeletePreProcFiles   ) )                 SetFlags ( outputflags, DeleteTempFiles             );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Markov Chains parameters
bool                markov              = CheckToBool     ( FitTransfer.MarkovSegment ) && competitive;
int                 markovtransmax      = StringToInteger ( FitTransfer.MarkovNumTransitions );

bool                markovsavefreq      = markov;
bool                markovsavecsv       = markov;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSelection          varout   ( fitnumvar,       OrderSorted );

varout.Set ( fitfonset,         CheckToBool ( FitTransfer.FOnset        ) );
varout.Set ( fitloffset,        CheckToBool ( FitTransfer.LOffset       ) );
varout.Set ( fitnumtf,          CheckToBool ( FitTransfer.NumTf         ) );
varout.Set ( fittfcentroid,     CheckToBool ( FitTransfer.TfCentroid    ) );
varout.Set ( fitmeancorr,       CheckToBool ( FitTransfer.MeanCorr      ) );
varout.Set ( fitgev,            CheckToBool ( FitTransfer.Gev           ) );
varout.Set ( fitbcorr,          CheckToBool ( FitTransfer.BestCorr      ) );
varout.Set ( fittfbcorr,        CheckToBool ( FitTransfer.TfBestCorr    ) );
varout.Set ( fitgfptfbcorr,     CheckToBool ( FitTransfer.GfpTfBestCorr ) );
varout.Set ( fitmaxgfp,         CheckToBool ( FitTransfer.MaxGfp        ) );
varout.Set ( fittfmaxgfp,       CheckToBool ( FitTransfer.TfMaxGfp      ) );
varout.Set ( fitmeangfp,        CheckToBool ( FitTransfer.MeanGfp       ) );
varout.Set ( fitmeanduration,   CheckToBool ( FitTransfer.MeanDuration  ) );
varout.Set ( fittimecoverage,   CheckToBool ( FitTransfer.TimeCoverage  ) );
varout.Set ( fitsegdensity,     CheckToBool ( FitTransfer.SegDensity    ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We stop browsing parameters here, the fitting process will handle:
                                        // - reference
                                        // - dual versions
                                        // - epochs
                                        // - skipping bad epochs
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here we have these important flags tested & set: 

bool                ispreprocessing     = timelyfilter
                                       || gfpnormalize;
//                                     || badepochs != NoSkippingBadEpochs; // nope, this is handled by the fitting function itself


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

enum                GaugeGroupFitEnum 
                    {
                    gaugegroupfitgroup,
                    gaugegroupfitpreproc,
                    };


if ( ispreprocessing ) {
                                        // Init gauge
    GroupGauge.Set ( "Fitting Preprocessing", SuperGaugeLevelInter );

    GroupGauge.AddPart ( gaugegroupfitgroup,    maxnumsubjects,                                                   ispreprocessing ? 25 : 100 );
    GroupGauge.AddPart ( gaugegroupfitpreproc,  maxnumsubjects * PreProcessFilesGaugeSize ( numwithinsubjects ),  ispreprocessing ? 75 :   0 );

    GroupGauge.AdjustOccupy ( false );
    }

                                        // batch can be long, hide Cartool until we are done
//WindowMinimize ( CartoolMainWindow );

                                        // Changing priority
SetProcessPriority ( BatchProcessingPriority );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // First reorganize the groups of files "horizontally": 1 group per subject, which includes all conditions together
bool                usetempdir      = true;
TFileName           temppath;
TGoGoF				gogofpersubject;
TGoGoF		        gogofallsubjectspreproc;
TGoGoF		        finalgogof;
TGoGoF		        tempgogof;
TGoF                baselistpreproc;
bool                newfiles        = false;



for ( int gofi1 = 0, gofi2 = gofi1 + numwithinsubjects - 1; gofi1 < numgroups && gofi2 < numgroups; gofi1 += numwithinsubjects, gofi2 += numwithinsubjects ) {

                                        // For pre-processing purpose, we need to transpose the groups' layout
    GoGoF.ConditionsToSubjects ( gofi1, gofi2, gogofpersubject );

//  gogofpersubject.Show ( 0, (int) gogofpersubject - 1, "Fitting: Groups Reordered" );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // loop through all subjects, of all sets of groups
    gogofallsubjectspreproc .Reset ();
    baselistpreproc         .Reset ();


    for ( int absg = 0; absg < gogofpersubject.NumGroups (); absg++ ) {

        if ( GroupGauge.IsAlive () )
            GroupGauge.CurrentPart  = gaugegroupfitpreproc;

        CartoolApplication->SetMainTitle    ( "Fitting Preprocessing of", FitTransfer.BaseFileName, GroupGauge );


        PreProcessFiles (   gogofpersubject[ absg ],    datatype,
                            NoDualData,         0,
                            spatialfilter,              FitTransfer.XyzFile,
                            false,              0,      RegularizationNone, 0,
                            false,                                          // no complex case
                            gfpnormalize,
                            BackgroundNormalizationNone,    ZScoreNone,     0,
                            false,                                          // no ranking
                            false,              0,                          // no thresholding
                            FilterTypeNone,     0,                          // no Envelope
                            0,                  FilterTypeNone,             // no ROIS
                            EpochWholeTime,     0,          0,              // no cropping, epochs are managed in the actual fitting, we want the output to be the same size as the input
                            NoGfpPeaksDetection,0,
                            NoSkippingBadEpochs,0,          0,              // no Skipping Bad Epochs, this is handled in the actual fitting
                            FitTransfer.BaseFileName,       0,
                            0,                  30,                         // not optimal, should look for enough chars to discriminate all files
                            usetempdir,         temppath,                   // there can be problems if we generate files within the original directory, in case of parallel computations
                            true,               tempgogof,      0,          baselistpreproc,    newfiles,
                            true,               0,
                            &GroupGauge
                        );

                                        // cumulate results, actually only 1 GoF here (no epochs)
        gogofallsubjectspreproc.Add ( tempgogof, MaxPathShort );

                                        // put this here AFTER processing, so the user does not think it's done at 100%
        if ( GroupGauge.IsAlive () )
            GroupGauge.Next ( gaugegroupfitgroup, SuperGaugeUpdateTitle );

        } // for gof

//    gogofpersubjectpreproc.Show ( 0, gogofpersubjectpreproc.NumGroups () - 1,  "GoGoF: preprocessed" );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // restore original groups organization
    gogofallsubjectspreproc.SubjectsToConditions ( tempgogof );

                                        // cumulate all preprocessed GoGoF's
    finalgogof.Add ( tempgogof, MaxPathShort );
    } // for gofi


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // GroupGauge is of no use from here, Gauge will take control
if ( GroupGauge.IsAlive () )
    GroupGauge.FinishParts ();

                                        // tell BackFitting it can own the new files
if ( newfiles )     SetFlags ( outputflags, OwningFiles );

                                        // also check not to delete files
if ( ! newfiles && IsFlag ( outputflags, DeleteTempFiles ) )
    ResetFlags ( outputflags, DeleteTempFiles );

                                        // Run only 1 Fitting here, with all preprocessed groups at once
                                        // baselistpreproc should actually be BaseFileName here
                                        // newfiles flag has been set in  PreProcessFiles
bool    fitok   =

BackFitting (   FitTransfer.TemplateFileName,
                finalgogof,             numwithinsubjects,
                analysis,               modality,
                epochfrom,              epochto,            epochmaps,

                spatialfilter,          FitTransfer.XyzFile,
                badepochs,              listbadepochs,

                datatype,               polarity,           dataref,
                dolimitcorr,            limitcorr,

                gfpnormalize,
                noncompetitive,

                smoothing,              smoothinghalfsize,  smoothinglambda,
                rejectsmall,            rejectsize,

                markov,                 markovtransmax,

                varout,
                outputflags,
                baselistpreproc[ 0 ]
            );

//if ( ! fitok )  
//    CmCancel ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // if we used a (single) temp directory, we can dispose of it now
if ( usetempdir && StringIsNotEmpty ( temppath ) )

    NukeDirectory ( temppath );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

endofprocessing:

SetProcessPriority ();

WindowMaximize ( CartoolMainWindow );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
