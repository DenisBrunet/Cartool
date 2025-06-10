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

#include    "TComputingRisDialog.h"

#include    "CartoolTypes.h"            // BackgroundNormalization ZScoreType CentroidType
#include    "Strings.Utils.h"
#include    "Strings.TFixedString.h"
#include    "Files.SpreadSheet.h"

#include    "TInverseResults.h"
#include    "TFreqDoc.h"

#include    "TMicroStates.h"            // PolarityEvaluate
#include    "ESI.ComputingRis.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Global, persistent variables that will remain between calls
TGoGoF          GoGoF;                  // Groups of Groups of EEG files
TGoF            Inverses;               // Groups of Inverse files

                                        // Functions to be converted to methods at some points, hence the systematic parameters
bool            HasSubjects             ( const TGoGoF& gogof )                     { return  gogof.IsNotEmpty ();      }
int             NumSubjects             ( const TGoGoF& gogof )                     { return  gogof.NumGroups ();       }   // 1 Group == 1 Subject
int             NumConditions           ( const TGoF&   gof   );                                                            // !Accounts for epoch case!
int             NumConditions           ( const TGoGoF& gogof );                                                            // !Accounts for epoch case!
int             NumClusters             ( const TGoGoF& gogof )                     { return NumConditions ( gogof );   }   // Clusters == Conditions
int             NumEpochs               ( const TGoF&   gof,   const char* fileprefix );
int             NumEpochs               ( const TGoGoF& gogof, int subji, const char* fileprefix );


bool            HasInverses             ( const TGoF&   gofi  )                     { return  gofi.IsNotEmpty ();       }
int             NumInverses             ( const TGoF&   gofi  )                     { return  gofi.NumFiles   ();       }
bool            IsSingleTemplateInverse ( const TGoF&   gofi  )                     { return  HasInverses ( gofi ) && NumInverses ( gofi ) == 1;                        }   // !these 2 tests are not mutually exclusive!
bool            AreIndividualInverses   ( const TGoF&   gofi, const TGoGoF& gogof ) { return  HasInverses ( gofi ) && NumInverses ( gofi ) == NumSubjects ( gogof );    }   // !these 2 tests are not mutually exclusive!


//----------------------------------------------------------------------------
int             NumConditions ( const TGoF& gof )
{
if ( gof.IsEmpty () )
    return  0;

bool                areepochs       = gof.AllStringsGrep ( InfixEpochConcatGrep, GrepOptionDefaultFiles );

if ( areepochs ) {

    TGoGoF          splitgogof;
                                        // Default to split by our epoch name
                                        // !Missing: splitting by "Concat" / "Concatenate" strings!
    gof.SplitByNames ( "." InfixEpoch, splitgogof );

    return  (int) splitgogof;
    }
else
    return  gof.NumFiles  ();
}


int             NumConditions ( const TGoGoF& gogof )
{
if ( gogof.IsEmpty () )
    return  0;

//bool                areepochs       = gogof.AllStringsGrep ( InfixEpochConcatGrep, GrepOptionDefaultFiles );

return  NumConditions ( gogof[ 0 ] );   // looking at first group should be good enough
}


int             NumEpochs ( const TGoF& gof, const char* fileprefix )
{
TGoGoF              splitgogof;
                                        // Default to split by our epoch name
                                        // !Missing: splitting by "Concat" / "Concatenate" strings!
gof.SplitByNames ( "." InfixEpoch, splitgogof );

return  splitgogof.NumFiles ();

/*
TGoF                gofsplitepochs;
                                        // !The returned epochs can be any size, as specified by the markers themselves!
                                        // !The Batch Averaging below also seems to not really care, taking the first file size for all epochs!
                                        // ?We could add a fall-back option to split into blocks of known size?
gof.SplitByEpochs ( InfixEpochConcatGrep, -1, fileprefix /*localfileprefix* /, gofsplitepochs );

if ( gofsplitepochs.IsEmpty () )
    return  0;
*/
}


int             NumEpochs ( const TGoGoF& gogof, int subji, const char* fileprefix )
{
if ( ! IsInsideLimits ( subji, 0, NumSubjects ( gogof ) - 1 ) )
    return  0;

return  NumEpochs ( gogof[ subji ], fileprefix );
}


//----------------------------------------------------------------------------

CRISPresetSpec  CRISPresets[ NumComputingRisPresets ] =
            {
            {   ComputingRisPresetErpGroupMeans,    "ERP Data               - Grand Averages",                      (CRISPresetFlags) ( CRISPresetERP   | CRISPresetNorm | CRISPresetVect                                                               ),    CRISPresetNorm,     false     },  // norm or vector doesn't matter
            {   ComputingRisPresetErpIndivMeans,    "ERP Data               - Subjects' Averages",                  (CRISPresetFlags) ( CRISPresetERP   | CRISPresetNorm | CRISPresetVect                                                               ),    CRISPresetVect,     true      },  // norm only, subjects dipoles vary too much even after ERP
            {   ComputingRisPresetErpIndivEpochs,   "ERP Data               - Subjects' Epochs",                    (CRISPresetFlags) ( CRISPresetERP   | CRISPresetNorm | CRISPresetVect                                            | CRISPresetEpochs ),    CRISPresetVect,     false     },  // vector only, we need vectors from same SP to cancel each others
            {   ComputingRisPresetErpSegClusters,   "ERP Data               - Clusters from Segmentation output",   (CRISPresetFlags) ( CRISPresetERP   | CRISPresetNorm | CRISPresetVect | CRISPresetClusters | CRISPresetCentroids                    ),    CRISPresetVect,     false     },  // norm or vector doesn't matter
            {   ComputingRisPresetErpFitClusters,   "ERP Data               - Clusters from Fitting output",        (CRISPresetFlags) ( CRISPresetERP   | CRISPresetNorm | CRISPresetVect | CRISPresetClusters | CRISPresetCentroids                    ),    CRISPresetVect,     false     },  // norm only, vectors vary too much already within a subject, and worse across subjects

            {   ComputingRisPresetSeparator1,       "",                                                             CRISPresetNone,                                                                                                                           CRISPresetNone,     false     },
                                                                                                                                                                                                              // Not sure about centroids here
            {   ComputingRisPresetIndIndivEpochs,   "Induced Responses - Subjects' Epochs",                         (CRISPresetFlags) ( CRISPresetInd   | CRISPresetNorm                                       | CRISPresetCentroids | CRISPresetEpochs ),    CRISPresetNorm,     false     },  // norm only, we need to ditch out phase and keep only the power contribution

            {   ComputingRisPresetSeparator2,       "",                                                             CRISPresetNone,                                                                                                                           CRISPresetNone,     false     },
                                                                                                                                                                        // vectorial spontaneous not really needed, but let's give the option
            {   ComputingRisPresetSpont,            "Resting States        - Spontaneous Data",                     (CRISPresetFlags) ( CRISPresetSpont | CRISPresetNorm | CRISPresetVect                                                               ),    CRISPresetNorm,     true      },
                                                                                                                                                                        // vectorial spontaneous centroids has not been tested - it checks polarity per solution point
            {   ComputingRisPresetSpontClusters,    "Resting States        - Clusters from Fitting output",         (CRISPresetFlags) ( CRISPresetSpont | CRISPresetNorm | CRISPresetVect | CRISPresetClusters | CRISPresetCentroids                    ),    CRISPresetNorm,     false     },

            {   ComputingRisPresetSeparator3,       "",                                                             CRISPresetNone,                                                                                                                           CRISPresetNone,     false     },

            {   ComputingRisPresetFreq,             "Frequency Data      - Complex results",                        (CRISPresetFlags) ( CRISPresetFreq  | CRISPresetNorm | CRISPresetVect                                                               ),    CRISPresetNorm,     true      },
            };

                                        // Various messages for the InverseFile edit field
constexpr char*     InverseFileNone         = ""; // "<No Inverse>";
constexpr char*     InverseFileNotOK        = "<Inverse(s) Not OK!>";
constexpr char*     InverseFileIndiv        = "<Individual Inverses>";
constexpr char*     InverseFileTooFew       = "<More Subjects than Inverses>";
constexpr char*     InverseFileTooMany      = "<More Inverses than Subjects>";


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // declare this global variable
TComputingRisStructEx   ComputingRisTransfer;


        TComputingRisStruct::TComputingRisStruct ()
{
EegPresets.Clear ();
for ( int i = 0; i < NumComputingRisPresets; i++ )
    EegPresets.AddString ( CRISPresets[ i ].Text, i == ComputingRisPresetDefault );

GroupCond       = BoolToCheck (   CRISPresets[ ComputingRisPresetDefault ].GroupCond );
GroupSubj       = BoolToCheck ( ! CRISPresets[ ComputingRisPresetDefault ].GroupCond );

GroupsSummary.Clear ();

IntegerToString ( NumSubjectsEdit,      0 );
IntegerToString ( NumConditionsEdit,    0 );
IntegerToString ( NumInversesEdit,      0 );

SpatialFilter   = BoolToCheck ( ComputingRisPresetDefault == ComputingRisPresetErpIndivMeans 
                             || ComputingRisPresetDefault == ComputingRisPresetErpIndivEpochs 
                             || ComputingRisPresetDefault == ComputingRisPresetErpFitClusters 
                             || ComputingRisPresetDefault == ComputingRisPresetIndIndivEpochs
                             || ComputingRisPresetDefault == ComputingRisPresetSpont
                             || ComputingRisPresetDefault == ComputingRisPresetSpontClusters );
ClearString     ( XyzFile );

StringCopy      ( InverseFile, InverseFileNone );
PositiveData    = BoolToCheck ( CRISPresets[ ComputingRisPresetDefault ].IsNorm () );
VectorData      = BoolToCheck ( CRISPresets[ ComputingRisPresetDefault ].IsVect () );
RegAuto         = BoolToCheck ( true  );
RegFixed        = BoolToCheck ( false );
StringCopy      ( RegFixedEdit, DefaultRegularization );

TimeZScore      = BoolToCheck ( true  );
ComputeZScore   = BoolToCheck ( true  );
LoadZScoreFile  = BoolToCheck ( false );

Rank            = BoolToCheck ( false );

Envelope        = BoolToCheck ( false );
ClearString     ( EnvelopeLowFreq );

ApplyRois       = BoolToCheck ( false );
ClearString     ( RoisFile );

ClearString     ( BaseFileName );
}


        TComputingRisStructEx::TComputingRisStructEx ()
{
SavingIndividualFiles   = BoolToCheck ( true );
SavingEpochFiles        = BoolToCheck ( false );
ComputeGroupsAverages   = BoolToCheck ( false /*IsComputingRisErp ( ComputingRisPresetDefault )*/ );
ComputeGroupsCentroids  = BoolToCheck ( false /*IsComputingRisErp ( ComputingRisPresetDefault )*/ );
SavingZScoreFactors     = BoolToCheck ( true );


                                        // Storing all checks across all dimensions
CompatInverse   .Reset ();
CompatEegs      .Reset ();
CompatElectrodes.Reset ();
CompatSp        .Reset ();
CompatRois      .Reset ();

IsInverseOK             = false;
IsEegOK                 = false;
IsXyzOK                 = false;
IsRoisOK                = false;
IsInverseAndEegOK       = false;
IsInverseAndXyzOK       = false;
IsInverseAndRoisOK      = false;
IsXyzAndEegOK           = false;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TComputingRisDialog, TBaseDialog )

    EV_WM_DROPFILES,

    EV_CBN_SELCHANGE            ( IDC_RISPRESETS,               EvEegPresetsChange ),

    EV_COMMAND                  ( IDC_GROUPCOND,                UpdateDialog ),
    EV_COMMAND                  ( IDC_GROUPSUBJ,                UpdateDialog ),
    EV_COMMAND_ENABLE           ( IDC_GROUPCOND,                CmGroupPresetsEnable ),
    EV_COMMAND_ENABLE           ( IDC_GROUPSUBJ,                CmGroupPresetsEnable ),
    EV_COMMAND_ENABLE           ( IDC_POSITIVEDATA,             CmDataTypeEnable ),
    EV_COMMAND_ENABLE           ( IDC_VECTORDATA,               CmDataTypeEnable ),

    EV_COMMAND                  ( IDC_BROWSEXYZFILE,            CmBrowseXyzFile ),
    EV_COMMAND_ENABLE           ( IDC_SPATIALFILTER,            CmNotFrequencyEnable ),
    EV_COMMAND_ENABLE           ( IDC_BROWSEXYZFILE,            CmXyzEnable ),
    EV_COMMAND_ENABLE           ( IDC_XYZFILE,                  CmXyzEnable ),

    EV_COMMAND                  ( IDC_BROWSEISFILE,             CmBrowseISFile ),

    EV_COMMAND_ENABLE           ( IDC_REGFIXEDEDIT,             CmRegularizationEnable ),

    EV_COMMAND                  ( IDC_BROWSEROISFILE,           CmBrowseRoiDoc ),
    EV_COMMAND                  ( IDC_ROIS,                     CmRoisChange ),
    EV_COMMAND_ENABLE           ( IDC_ROISFILE,                 CmRoisEnable ),
    EV_COMMAND_ENABLE           ( IDC_BROWSEROISFILE,           CmRoisEnable ),

    EV_COMMAND                  ( IDC_COMPUTEZSCORE,            CmZScoreChange ),
    EV_COMMAND                  ( IDC_LOADZSCOREFILE,           CmZScoreChange ),
    EV_COMMAND_ENABLE           ( IDC_COMPUTEZSCORE,            CmZScoreFactorsEnable ),
    EV_COMMAND_ENABLE           ( IDC_LOADZSCOREFILE,           CmZScoreFactorsEnable ),
    EV_COMMAND_ENABLE           ( IDC_SAVEZSCOREFACTORS,        CmZScoreFactorsEnable ),

    EV_COMMAND_ENABLE           ( IDC_THRESHOLDRIS,             CmThresholdEnable ),

    EV_COMMAND_ENABLE           ( IDC_ENVELOPE,                 CmEnvelopeEnable ),
    EV_COMMAND_ENABLE           ( IDC_ENVELOPELOWFREQ,          CmEnvelopeLowFreqEnable ),

    EV_COMMAND_ENABLE           ( IDC_COMPUTEGROUPSAVERAGES,    CmGroupsAveragingEnable ),
    EV_COMMAND_ENABLE           ( IDC_COMPUTEGROUPSCENTROIDS,   CmGroupsCentroidsEnable ),
    EV_COMMAND_ENABLE           ( IDC_SAVEEPOCHFILES,           CmSaveEpochsEnable ),

    EV_COMMAND                  ( IDC_ADDGROUP,                 CmAddEegGroup ),
    EV_COMMAND                  ( IDC_REMFROMLIST,              CmRemoveLastGroup ),
    EV_COMMAND                  ( IDC_CLEARLISTS,               CmClearEegGroups ),
    EV_COMMAND                  ( IDC_READPARAMS,               CmReadParams ),
    EV_COMMAND                  ( IDC_WRITEPARAMS,              CmWriteParams ),

    EV_COMMAND_ENABLE           ( IDOK,                         CmOkEnable ),
    EV_COMMAND                  ( IDOK,                         CmOk ),

END_RESPONSE_TABLE;



        TComputingRisDialog::TComputingRisDialog ( TWindow* parent, TResId resId )
      : TBaseDialog ( parent, resId )
{

EegPresets              = new TComboBox     ( this, IDC_RISPRESETS );

GroupCond               = new TRadioButton  ( this, IDC_GROUPCOND );
GroupSubj               = new TRadioButton  ( this, IDC_GROUPSUBJ );
GroupsSummary           = new TListBox      ( this, IDC_GROUPSSUMMARY );
NumSubjectsEdit         = new TEdit         ( this, IDC_NUMSUBJECTS,    EditSizeValue );
NumConditionsEdit       = new TEdit         ( this, IDC_NUMCONDITIONS,  EditSizeValue );
NumInversesEdit         = new TEdit         ( this, IDC_NUMINVERSES,    EditSizeValue );

SpatialFilter           = new TCheckBox     ( this, IDC_SPATIALFILTER );
XyzFile                 = new TEdit         ( this, IDC_XYZFILE,        EditSizeText );

InverseFile             = new TEdit         ( this, IDC_ISFILE,         EditSizeText );
PositiveData            = new TRadioButton  ( this, IDC_POSITIVEDATA );
VectorData              = new TRadioButton  ( this, IDC_VECTORDATA );
RegAuto                 = new TRadioButton  ( this, IDC_REGAUTO );
RegFixed                = new TRadioButton  ( this, IDC_REGFIXED );
RegFixedEdit            = new TEdit         ( this, IDC_REGFIXEDEDIT,   EditSizeValue );

TimeZScore              = new TCheckBox     ( this, IDC_TIMEZSCORE );
ComputeZScore           = new TRadioButton  ( this, IDC_COMPUTEZSCORE );
LoadZScoreFile          = new TRadioButton  ( this, IDC_LOADZSCOREFILE );

Rank                    = new TCheckBox     ( this, IDC_RANKRIS );

Envelope                = new TRadioButton  ( this, IDC_ENVELOPE );
EnvelopeLowFreq         = new TEdit         ( this, IDC_ENVELOPELOWFREQ,EditSizeValue );
EnvelopeLowFreq->SetValidator ( new TFilterValidator ( ValidatorPositiveFloat ) );

ApplyRois               = new TRadioButton  ( this, IDC_ROIS );
RoisFile                = new TEdit         ( this, IDC_ROISFILE,       EditSizeText );

BaseFileName            = new TEdit         ( this, IDC_BASEFILENAME,   EditSizeText );

SavingIndividualFiles   = new TCheckBox     ( this, IDC_SAVEINDIVIDUALFILES );
SavingEpochFiles        = new TCheckBox     ( this, IDC_SAVEEPOCHFILES );
ComputeGroupsAverages   = new TCheckBox     ( this, IDC_COMPUTEGROUPSAVERAGES );
ComputeGroupsCentroids  = new TCheckBox     ( this, IDC_COMPUTEGROUPSCENTROIDS );
SavingZScoreFactors     = new TCheckBox     ( this, IDC_SAVEZSCOREFACTORS );


SetTransferBuffer ( dynamic_cast <TComputingRisStruct *> ( &ComputingRisTransfer ) );
}


        TComputingRisDialog::~TComputingRisDialog ()
{
delete  EegPresets;
delete  GroupCond;              delete  GroupSubj;
delete  GroupsSummary;
delete  NumSubjectsEdit;        delete  NumConditionsEdit;      delete  NumInversesEdit;
delete  SpatialFilter;          delete  XyzFile;
delete  InverseFile;
delete  PositiveData;           delete  VectorData;
delete  RegAuto;                delete  RegFixed;               delete  RegFixedEdit;
delete  TimeZScore;             delete  ComputeZScore;          delete  LoadZScoreFile;
delete  Rank;
delete  Envelope;               delete  EnvelopeLowFreq;
delete  ApplyRois;              delete  RoisFile;
delete  BaseFileName;
delete  SavingIndividualFiles;  delete  SavingEpochFiles;
delete  ComputeGroupsAverages;  delete  ComputeGroupsCentroids;
delete  SavingZScoreFactors;
}


void    TComputingRisDialog::SetupWindow ()
{
TBaseDialog::SetupWindow ();
                                        // update controls with restored transfer buffer
TransferData ( tdSetData );
                                        // ?
//CheckEeg ();
                                        // mainly to update buttons' text
UpdateDialog ();
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::EvEegPresetsChange ()
{

ComputingRisPresetsEnum esicase         = (ComputingRisPresetsEnum) EegPresets->GetSelIndex ();

if ( CRISPresets[ esicase ].Flags == CRISPresetNone )
    return;


//ResetCheck  ( SpatialFilter );

ResetCheck  ( Rank );

SetCheck    ( PositiveData, CRISPresets[ esicase ].IsNorm () );
SetCheck    ( VectorData,   CRISPresets[ esicase ].IsVect () );

ResetCheck  ( RegAuto   );
ResetCheck  ( RegFixed  );
SetText     ( RegFixedEdit, DefaultRegularization );   // force the recommended mean value in case user switches to fixed

ResetCheck  ( Envelope                  );                      // force it off each time, for the moment

ResetCheck  ( SavingIndividualFiles     );
if ( ! CRISPresets[ esicase ].IsEpochs () )                     // in case of Epochs preset, not resetting this button which is user's preference
    ResetCheck  ( SavingEpochFiles );
ResetCheck  ( ComputeGroupsAverages     );
ResetCheck  ( ComputeGroupsCentroids    );
SetCheck    ( SavingZScoreFactors       );


switch ( esicase ) {

    case ComputingRisPresetErpGroupMeans:

        SetCheck    ( RegAuto );

        SetCheck    ( SavingIndividualFiles );
        break;

    case ComputingRisPresetErpIndivMeans:
                                        // check file names if it seems Spatial Filter was already applied?
      //SetCheck    ( SpatialFilter, ! GoGoF.AllStringsGrep ( PostfixSpatialGrep, GrepOptionDefaultFiles ) );
        SetCheck    ( RegAuto );
                                        // only well structured data can have a global averaging
        SetCheck    ( ComputeGroupsAverages, NumSubjects ( GoGoF ) > 1 );
        SetCheck    ( SavingIndividualFiles );
        break;

    case ComputingRisPresetErpIndivEpochs:
                                        // check file names if it seems Spatial Filter was already applied?
      //SetCheck    ( SpatialFilter, ! GoGoF.AllStringsGrep ( PostfixSpatialGrep, GrepOptionDefaultFiles ) );
        SetCheck    ( RegAuto );
                                        // only well structured data can have a global averaging
        SetCheck    ( ComputeGroupsAverages, NumSubjects ( GoGoF ) > 1 );
        SetCheck    ( SavingIndividualFiles );
        break;

    case ComputingRisPresetErpSegClusters:

        SetCheck    ( RegAuto );
        SetCheck    ( Rank    );
                                        // only well structured data can have a global averaging
        SetCheck    ( ComputeGroupsCentroids );
        break;

    case ComputingRisPresetErpFitClusters:
                                        // check file names if it seems Spatial Filter was already applied?
      //SetCheck    ( SpatialFilter, ! GoGoF.AllStringsGrep ( PostfixSpatialGrep, GrepOptionDefaultFiles ) );
        SetCheck    ( RegAuto );
        SetCheck    ( Rank    );
                                        // only well structured data can have a global averaging
        SetCheck    ( ComputeGroupsCentroids );
        break;


    case ComputingRisPresetIndIndivEpochs:
                                        // check file names if it seems Spatial Filter was already applied?
      //SetCheck    ( SpatialFilter, ! GoGoF.AllStringsGrep ( PostfixSpatialGrep, GrepOptionDefaultFiles ) );
        SetCheck    ( RegAuto );
                                        // force it on each time
        SetCheck    ( Envelope );
                                        // only well structured data can have a global averaging
        SetCheck    ( ComputeGroupsAverages, NumSubjects ( GoGoF ) > 1 );
        SetCheck    ( SavingIndividualFiles  );
//      SetCheck    ( ComputeGroupsCentroids );
        break;


    case ComputingRisPresetSpont:
                                        // check file names if it seems Spatial Filter was already applied?
      //SetCheck    ( SpatialFilter, ! GoGoF.AllStringsGrep ( PostfixSpatialGrep, GrepOptionDefaultFiles ) );
        SetCheck    ( RegAuto );

        SetCheck    ( SavingIndividualFiles  );
        ResetCheck  ( ComputeGroupsCentroids );
        break;

    case ComputingRisPresetSpontClusters:
                                        // check file names if it seems Spatial Filter was already applied?
      //SetCheck    ( SpatialFilter, ! GoGoF.AllStringsGrep ( PostfixSpatialGrep, GrepOptionDefaultFiles ) );
        SetCheck    ( RegAuto );
        SetCheck    ( Rank    );

        ResetCheck  ( SavingIndividualFiles  );
        SetCheck    ( ComputeGroupsCentroids );
        break;


    case ComputingRisPresetFreq:

        SetCheck    ( RegFixed );

        SetCheck    ( SavingIndividualFiles );
        break;

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Special handling when general presets goes or leaves the Epochs averaging cases

static  ComputingRisPresetsEnum oldesicase  = ComputingRisPresetDefault;

                                        // quitting state -> save group layout, which user might have changed to his/her liking
CRISPresets[ oldesicase ].GroupCond     = IsChecked ( GroupCond );

                                        // storing for next call
oldesicase  = esicase;

                                        // restore previous state
SetCheck    ( GroupCond,   CRISPresets[ esicase ].GroupCond );
SetCheck    ( GroupSubj, ! CRISPresets[ esicase ].GroupCond );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // update transfer buffer(?)
//TransferData ( tdGetData );

UpdateDialog ();
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::UpdateDialog ()
{
                                        // only for dialog update
UpdateSubjectsConditions ();

UpdateGroupSummary ();
                                        // refine some buttons' description
SetDlgItemText  ( IDC_ADDGROUP,     IsChecked ( GroupSubj ) ? "&Add Files of New Subject"     : "&Add Files of New Condition"     );
SetDlgItemText  ( IDC_REMFROMLIST,  IsChecked ( GroupSubj ) ? "&Remove Files of Last Subject" : "&Remove Files of Last Condition" );
}


void    TComputingRisDialog::CmGroupPresetsEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! CRISPresets[ EegPresets->GetSelIndex () ].IsEpochs () );
}


void    TComputingRisDialog::CmDataTypeEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsNotChecked ( ApplyRois )
            && CRISPresets[ EegPresets->GetSelIndex () ].CmDataTypeEnable () );
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::CmWriteParams ()
{
if ( GoGoF.IsEmpty () )
    return;


TSpreadSheet        sf;

if ( ! sf.InitFile () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numgroups           = GoGoF.NumGroups           ();
int                 maxfiles            = GoGoF.GetMaxFilesPerGroup ();
bool                individualinverses  = AreIndividualInverses ( Inverses, GoGoF );

                                        // header line describes the attributes / fields
sf.WriteAttribute ( "numfiles" );

for ( int fi = 0; fi < maxfiles; fi++ )
    sf.WriteAttribute ( "file", fi + 1 );

if ( individualinverses )
    sf.WriteAttribute ( "inverse" );

sf.WriteNextRecord ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // now write each row
for ( int gogofi = 0; gogofi < numgroups; gogofi++ ) {

                                        // write number of files
    sf.WriteAttribute ( GoGoF[ gogofi ].NumFiles () );

                                        // write all files
    for ( int fi = 0; fi < GoGoF[ gogofi ].NumFiles (); fi++ )
        sf.WriteAttribute ( GoGoF[ gogofi ][ fi ] );

                                        // complete line with empty attributes
    for ( int fi = GoGoF[ gogofi ].NumFiles (); fi < maxfiles; fi++ )
        sf.WriteAttribute ( "" );

                                        // add a column to the subject?
    if ( individualinverses )
        sf.WriteAttribute ( Inverses[ gogofi ] );


    sf.WriteNextRecord ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

sf.WriteFinished ();
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::CmReadParams ()
{
ReadParams ();
}


void    TComputingRisDialog::ReadParams ( const char* filename )
{
                                        // TODO: reading .lm files
TSpreadSheet        sf;

if ( ! sf.ReadFile ( filename ) )
    return;


CsvType             csvtype         = sf.GetType ();

                                        // We can deal with statistics type, too...
if ( ! (    IsCsvComputingRis ( csvtype ) 
         || IsCsvStatFiles    ( csvtype ) ) ) {
    ShowMessage ( SpreadSheetErrorMessage, "Read list file", ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           attr;
TFileName           buff;
TGoF                gof;
int                 numfiles;

                                        // sort out now if we have either 1 row or 1 column of inverse files
bool                hascolinverse   = sf.HasAttribute ( "inverse" );
bool                hasrowinverse   = false;

if ( ! hascolinverse ) {

    int             row             = sf.GetNumRecords () - 1;

    sf.GetRecord ( row, "numfiles",    attr );
    numfiles    = StringToInteger ( attr );

                                        // transfer the filenames...
    gof.Reset ();

    for ( int fi = 0; fi < numfiles; fi++ ) {

        sprintf ( buff, "file%0d", fi + 1 );

        sf.GetRecord ( row, buff, attr );

        gof.Add ( attr );
        } // for file

                                        // inverses: last row
    hasrowinverse   = gof.AllExtensionsAre ( AllInverseFilesExt );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Ask how to add the new data in case we already have some loaded
bool                appendrows          = true;             // Natural option, we just append rows to rows. Otherwise we have to do some sorts of permutations
bool                firstread           = GoGoF.IsEmpty ();
//int                 numinputconditions  = sf.GetNumRecords    ()     - hasrowinverse;   // remove optional row of inverses
//int                 numinputsubjects    = sf.GetNumAttributes () - 1 - hascolinverse;   // remove optional col of inverses

                                        // we could somehow skip this tests once 2 groups have been dropped, as the option has been chosen already
if ( ! firstread ) {

    char            answer      = GetOptionFromUser ( "Do you want to append the new data as (S)ubjects, or as (C)onditions?",
                                                      "Appending Data", "S C", "" );
                                        // appending rows is the natural way
    appendrows  =                            answer == EOS
               || IsChecked ( GroupSubj ) && answer == 'S'
               || IsChecked ( GroupCond ) && answer == 'C';
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int row = 0; row < sf.GetNumRecords (); row++ ) {

    bool        islastrow       = row == sf.GetNumRecords () - 1;
    bool        isrowinverse    = hasrowinverse && islastrow;   // current input is the row of subjects


    sf.GetRecord ( row, "numfiles",    attr );
    numfiles    = StringToInteger ( attr );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // transfer the filenames...
    gof.Reset ();

    for ( int fi = 0; fi < numfiles; fi++ ) {

        sprintf ( buff, "file%0d", fi + 1 );

        sf.GetRecord ( row, buff, attr );


        if      ( isrowinverse ) {
            if ( firstread || ! appendrows )    Inverses    .Add ( attr );  // skip if user requested it
            }
        else if (   appendrows )                gof         .Add ( attr );  // usual case

        else /*if ( ! appendrows )*/            GoGoF[ row ].Add ( attr );  // permutating case - this needs some more tests
        } // for file


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( appendrows && ! isrowinverse ) 

        AddEegGroup ( gof );            // checks & updates EEGs


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // inverses: last column
    if ( hascolinverse )
        if ( firstread || appendrows ) {

        sf.GetRecord ( row, "inverse",     attr );
        Inverses.Add ( attr );
        }
    }

                                        // Full checks
CheckEeg ();
//TransferData ( tdSetData );

ComputingRisTransfer.CheckInverses       ();
TransferData ( tdSetData );

ComputingRisTransfer.CheckInverseAndEeg  ();
TransferData ( tdSetData );

ComputingRisTransfer.CheckInverseAndXyz  ();
TransferData ( tdSetData );

ComputingRisTransfer.CheckInverseAndRois ();
TransferData ( tdSetData );

ComputingRisTransfer.CheckXyzAndEeg      ();
TransferData ( tdSetData );

UpdateSubjectsConditions ();

UpdateGroupSummary ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // update spatial filter flag live
//if ( ! (    EegPresets->GetSelIndex () == ComputingRisPresetErpGroupMeans
//         || EegPresets->GetSelIndex () == ComputingRisPresetErpSegClusters
//         || EegPresets->GetSelIndex () == ComputingRisPresetErpFitClusters
//         || EegPresets->GetSelIndex () == ComputingRisPresetFreq            ) )
//                                        // reset Spatial Filter if file names already contain .SpatialFilter
//    SetCheck    ( SpatialFilter, ! GoGoF.AllStringsGrep ( PostfixSpatialGrep, GrepOptionDefaultFiles );
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::EvDropFiles ( TDropInfo drop )
{
TGoF                isfiles         ( drop, AllInverseFilesExt      );
TGoF                xyzfiles        ( drop, AllCoordinatesFilesExt  );
TGoF                roifiles        ( drop, AllRoisFilesExt         );
TGoF                eegfiles        ( drop, AllEegFilesExt          );
TGoF                freqfiles       ( drop, AllFreqFilesExt         );
TGoF                spreadsheetfiles( drop, SpreadSheetFilesExt     );
TGoF                remainingfiles  ( drop, AllInverseFilesExt " " AllSolPointsFilesExt " " AllMriFilesExt " " AllCoordinatesFilesExt " " AllRoisFilesExt " " AllEegFilesExt " " AllFreqFilesExt " " SpreadSheetFilesExt, 0, true );

drop.DragFinish ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Sequence matters here: give priority to the inverse matrix, then EEG, then XYZ, then ROIs
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Just appending inverse files
if ( (bool) isfiles )

    AddISGroup ( isfiles );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) eegfiles && (bool) freqfiles )

    ShowMessage ( "You can not process EEG and Frequency files together!" NewLine 
                  "Just compute one type of files at a time.", ComputingRisTitle, ShowMessageWarning );

else {
                                        // will possibly split / dispatch in more groups, in case of clusters of data
    if ( (bool) eegfiles  )     AddEegGroups    ( eegfiles );

    if ( (bool) freqfiles )     AddEegGroup     ( freqfiles );

                                        // update spatial filter flag live - maybe a bit too intrusive, when D&D the inverse matrix, user already had multiple chances to set the xyz file...
//  if ( ! (   EegPresets->GetSelIndex () == ComputingRisPresetErpGroupMeans 
//          || EegPresets->GetSelIndex () == ComputingRisPresetErpSegClusters
//          || EegPresets->GetSelIndex () == ComputingRisPresetErpFitClusters
//          || EegPresets->GetSelIndex () == ComputingRisPresetFreq          ) )
//                                      // reset Spatial Filter if file names already contain .SpatialFilter
//      SetCheck    ( SpatialFilter, ! GoGoF.AllStringsGrep ( PostfixSpatialGrep, GrepOptionDefaultFiles );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) spreadsheetfiles; i++ )

    ReadParams ( spreadsheetfiles[ i ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) xyzfiles; i++ )

    SetXyzFile ( xyzfiles[ i ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) roifiles; i++ )

    SetRoisFile ( roifiles[ i ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Let's be nice and try to load D&D Directories
for ( int i = 0; i < (int) remainingfiles; i++ ) {

    if ( ! IsDirectory ( remainingfiles[ i ] ) )
        continue;


    TGoF                subgof;

    subgof.GrepFiles    ( remainingfiles[ i ], AllTracksFilesGrep, GrepOptionDefaultFiles, true );
                                        // AllTracksFilesGrep included RIS files, which we don't want here; we also wish to skip ZScore factors files
    subgof.GrepRemove   ( "(" AllRisFilesGrep "|" InfixStandardizationFactorsGrep ")", GrepOptionDefaultFiles );

//  subgof.Show ( "found eeg" );

                                        // process if some files were found, complain otherwise (users always LOVES a bit of whining)
    if ( (bool) subgof ) {
                                        // will possibly split / dispatch in more groups, in case of clusters of data
        AddEegGroups ( subgof );

        remainingfiles.RemoveRef ( remainingfiles[ i ] );
        i--;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) remainingfiles )
    remainingfiles.Show ( IrrelevantErrorMessage );


InverseFile->ResetCaret;
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::CmBrowseISFile ()
{
AddISGroup ( TGoF () );
}


void    TComputingRisDialog::AddISGroup ( const TGoF& gofis )
{
static GetFileFromUser  getfile ( "Inverse Solution Matrix Files:", AllInverseFilesFilter, 1, GetFileMulti );

TransferData ( tdGetData );


if ( gofis.IsEmpty () ) {

    if ( ! getfile.Execute () || getfile.GetNumFiles () == 0 )
        return;

    Inverses.Add ( (const TGoF&) getfile );
    }
else
    Inverses.Add ( gofis );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ComputingRisTransfer.CheckInverses       ();
TransferData ( tdSetData );

ComputingRisTransfer.CheckInverseAndEeg  ();
TransferData ( tdSetData );

ComputingRisTransfer.CheckInverseAndXyz  ();
TransferData ( tdSetData );

ComputingRisTransfer.CheckInverseAndRois ();
TransferData ( tdSetData );


                                        // in case user dropped a matrix which is incompatible, clear-up EEG?
if ( ComputingRisTransfer.IsInverseOK && ComputingRisTransfer.IsEegOK && ! ComputingRisTransfer.IsInverseAndEegOK ) {
                                        // or asking? or silent?
//  ShowMessage ( "Clearing-up the existing EEG files...", ComputingRisTitle, ShowMessageWarning );

    CmClearEegGroups () ;

    ComputingRisTransfer.IsEegOK             = false;
    ComputingRisTransfer.IsInverseAndEegOK   = false;
    }


                                        // in case user dropped XYZ file which is incompatible
if ( ComputingRisTransfer.IsInverseOK && ComputingRisTransfer.IsXyzOK && ! ComputingRisTransfer.IsInverseAndXyzOK ) {
                                        // give priority to existing matrix
//  ShowMessage ( "Clearing-up the existing Electrodes Coordinates...", ComputingRisTitle, ShowMessageWarning );

    ClearText   ( XyzFile );
    ResetCheck  ( SpatialFilter );

    ComputingRisTransfer.IsXyzOK             = false;
    ComputingRisTransfer.IsInverseAndXyzOK   = false;
    }


                                        // in case user dropped ROIs file which is incompatible
if ( ComputingRisTransfer.IsInverseOK && ComputingRisTransfer.IsRoisOK && ! ComputingRisTransfer.IsInverseAndRoisOK ) {
                                        // give priority to existing matrix
//  ShowMessage ( "Clearing-up the existing ROIs...", ComputingRisTitle, ShowMessageWarning );

    ClearText   ( RoisFile );
    ResetCheck  ( ApplyRois );

    ComputingRisTransfer.IsRoisOK            = false;
    ComputingRisTransfer.IsInverseAndRoisOK  = false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

UpdateGroupSummary ();

InverseFile->ResetCaret;
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::CmNotFrequencyEnable ( TCommandEnabler &tce )
{
tce.Enable ( EegPresets->GetSelIndex () != ComputingRisPresetFreq );
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::CmBrowseXyzFile ()
{
SetXyzFile ( 0 );
}


void    TComputingRisDialog::SetXyzFile ( const char* file )
{
                                        // We can also test here that we are not in the frequency preset
                                        // and return right away. Or allow it, as a transient state before
                                        // changing the preset f.ex.

static GetFileFromUser  getfile ( "Electrodes Coordinates File", AllCoordinatesFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( ComputingRisTransfer.XyzFile ) )
        return;

    TransferData ( tdSetData );
    }
else {
    StringCopy ( ComputingRisTransfer.XyzFile, file );

    getfile.SetOnly ( file );
    }


TransferData ( tdSetData );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ComputingRisTransfer.CheckXyz            ();

TransferData ( tdSetData );

ComputingRisTransfer.CheckXyzAndEeg      ();

TransferData ( tdSetData );

ComputingRisTransfer.CheckInverseAndXyz  ();

TransferData ( tdSetData );


                                        // in case user dropped a XYZ which is incompatible, clear-up EEG?
if ( ComputingRisTransfer.IsXyzOK && ComputingRisTransfer.IsEegOK && ! ComputingRisTransfer.IsXyzAndEegOK ) {
                                        // give priority to existing EEG, which should be compatible with the matrix
//  ShowMessage ( "Clearing-up the existing Electrodes Coordinates...", ComputingRisTitle, ShowMessageWarning );

    ClearText   ( XyzFile );
    ResetCheck  ( SpatialFilter );

    ComputingRisTransfer.IsXyzOK         = false;
    ComputingRisTransfer.IsXyzAndEegOK   = false;
    }


                                        // in case user dropped XYZ file which is incompatible
if ( ComputingRisTransfer.IsXyzOK && ComputingRisTransfer.IsInverseOK && ! ComputingRisTransfer.IsInverseAndXyzOK ) {
                                        // give priority to existing matrix
//  ShowMessage ( "Clearing-up the existing Electrodes Coordinates...", ComputingRisTitle, ShowMessageWarning );

    ClearText   ( XyzFile );
    ResetCheck  ( SpatialFilter );

    ComputingRisTransfer.IsXyzOK             = false;
    ComputingRisTransfer.IsInverseAndXyzOK   = false;
    }


XyzFile->ResetCaret;
}


void    TComputingRisDialog::CmXyzEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsChecked ( SpatialFilter )
          && EegPresets->GetSelIndex () != ComputingRisPresetFreq );
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::CmRegularizationEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsChecked ( RegFixed ) );
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::CmBrowseRoiDoc ()
{
SetRoisFile ( 0 );
}


void    TComputingRisDialog::SetRoisFile ( const char* file )
{
static GetFileFromUser  getfile ( "ROIs File", AllRoisFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( ComputingRisTransfer.RoisFile ) )
        return;

    TransferData ( tdSetData );
    }
else {
    StringCopy ( ComputingRisTransfer.RoisFile, file );

    getfile.SetOnly ( file );
    }


TransferData ( tdSetData );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ComputingRisTransfer.CheckRois           ();

TransferData ( tdSetData );

ComputingRisTransfer.CheckInverseAndRois ();

TransferData ( tdSetData );


                                        // in case user dropped ROIs file which is incompatible
if ( ComputingRisTransfer.IsRoisOK && ComputingRisTransfer.IsInverseOK && ! ComputingRisTransfer.IsInverseAndRoisOK ) {
                                        // give priority to existing matrix
//  ShowMessage ( "Clearing-up the existing Electrodes Coordinates...", ComputingRisTitle, ShowMessageWarning );

    ClearText   ( RoisFile );
    ResetCheck  ( ApplyRois );

    ComputingRisTransfer.IsRoisOK            = false;
    ComputingRisTransfer.IsInverseAndRoisOK  = false;
    }


RoisFile->ResetCaret;
}


void    TComputingRisDialog::CmRoisChange ()
{
TransferData ( tdGetData );

                                        // turning option on? check existing ROIs with SPs
if ( CheckToBool ( ComputingRisTransfer.ApplyRois ) ) {

    ComputingRisTransfer.VectorData     = false;
    ComputingRisTransfer.PositiveData   = true;
    TransferData ( tdSetData );

    if ( StringIsNotEmpty ( ComputingRisTransfer.RoisFile ) )

        SetRoisFile ( ComputingRisTransfer.RoisFile );
    }

}


void    TComputingRisDialog::CmRoisEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsChecked ( ApplyRois ) );
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::CmEnvelopeEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsNotChecked ( VectorData ) );
}


void    TComputingRisDialog::CmEnvelopeLowFreqEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsNotChecked ( VectorData ) && IsChecked ( Envelope ) );
}


void    TComputingRisDialog::CmGroupsAveragingEnable ( TCommandEnabler &tce )
{
tce.Enable ( CRISPresets[ EegPresets->GetSelIndex () ].CmGroupsAveragingEnable () );
}


void    TComputingRisDialog::CmGroupsCentroidsEnable ( TCommandEnabler &tce )
{
tce.Enable ( CRISPresets[ EegPresets->GetSelIndex () ].IsCentroids () );
}


void    TComputingRisDialog::CmSaveEpochsEnable ( TCommandEnabler &tce )
{
tce.Enable ( CRISPresets[ EegPresets->GetSelIndex () ].IsEpochs () );
}


void    TComputingRisDialog::CmZScoreFactorsEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsChecked ( TimeZScore ) );
}


void    TComputingRisDialog::CmZScoreChange ()
{
if ( IsNotChecked ( TimeZScore ) )
    return;

if ( IsChecked ( ComputeZScore ) )
    SetCheck    ( SavingZScoreFactors );

if ( IsChecked ( LoadZScoreFile ) )
    ResetCheck  ( SavingZScoreFactors );
}


void    TComputingRisDialog::CmThresholdEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsChecked ( Rank ) );
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::CmAddEegGroup ()
{
GetFileFromUser     getfiles ( "Open Files", AllErpEegFilesFilter "|" AllFreqFilesFilter, 1, GetFileMulti );


ComputingRisPresetsEnum esicase         = (ComputingRisPresetsEnum) EegPresets->GetSelIndex ();


getfiles.SetFileFilterIndex ( esicase == ComputingRisPresetErpGroupMeans    ? 1
                            : esicase == ComputingRisPresetErpIndivMeans    ? 1
                            : esicase == ComputingRisPresetErpIndivEpochs   ? 1
                            : esicase == ComputingRisPresetErpSegClusters   ? 2
                            : esicase == ComputingRisPresetErpFitClusters   ? 2
                            : esicase == ComputingRisPresetIndIndivEpochs   ? 1
                            : esicase == ComputingRisPresetSpont            ? 2
                            : esicase == ComputingRisPresetSpontClusters    ? 2
                            : esicase == ComputingRisPresetFreq             ? 3
                            :                                                 4 
                            );


if ( ! getfiles.Execute () )
    return;


AddEegGroups ( getfiles );
}

                                        // This part deciphers these special cases: clusters of data from fitting, or epochs for ERPs
void    TComputingRisDialog::AddEegGroups ( const TGoF& gofeeg )
{
                                        // are we potentially dropping a set of cluster files?
bool                areclusters     = gofeeg.AllStringsGrep ( InfixClusterGrep,     GrepOptionDefaultFiles );
bool                areepochs       = gofeeg.AllStringsGrep ( InfixEpochConcatGrep, GrepOptionDefaultFiles );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( areclusters ) {
                                        // A lot is going on here so to give the user a lot of freedom in how he/she drops new groups of files

                                        // Clusters can have any numbers now, as long as all groups are equal
    constexpr int       MaxRisClusters      = 1000;
    TGoGoF              gogofc ( MaxRisClusters );
    TSelection          selcl  ( MaxRisClusters, OrderSorted );
    TStringGrep         grepcl;
    TStrings            matched;

                                        // scan all files, dispatch them by cluster numbers, which can be totally arbitrary
    for ( int i = 0; i < (int) gofeeg; i++ ) {
                               // named group, used to isolate the cluster number
        grepcl.Set   ( InfixClusterGrep, GrepOptionDefaultFiles );

        if ( grepcl.Matched ( gofeeg ( i ), &matched ) && matched.NumStrings () >= 2 ) {

            int     clusteri    = StringToInteger ( matched ( 3 ) );

            selcl.Set ( clusteri );

            gogofc[ clusteri ].Add ( gofeeg ( i ) );
            }
        else
            ShowMessage ( "File name seems to indiciate that this is not a proper Cluster file." NewLine 
                          "Skipping file...", gofeeg ( i ), ShowMessageWarning );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check if new groups are consistents within themselves
    int                 maxfilespercluster  = gogofc.GetMaxFilesPerGroup ();

    for ( TIteratorSelectedForward cli ( selcl ); (bool) cli; ++cli ) {
                                        // Check all groups have the same number of files among themselves
        if ( gogofc[ cli() ].NumFiles () != maxfilespercluster ) {
            char        buff[ 256 ];
            StringCopy  ( buff, "Cluster ", IntegerToString ( cli() ), " is missing some files..." NewLine "Check you didn't forget some files, or put some outliers..." );
            ShowMessage ( buff, ComputingRisTitle, ShowMessageWarning );
            return;
            }
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check if new files already exist
    if ( GoGoF.ContainsAny ( gogofc ) ) {
        
        ShowMessage ( "It seems you are trying to add files already included!" NewLine 
                      "Please check again your files...", ComputingRisTitle, ShowMessageWarning );
        return;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Early check on the number of dropped files
    if ( GoGoF.IsNotEmpty () ) {

        if      ( IsChecked ( GroupSubj ) ) {

            int                 numselclusters  = selcl.NumSet ();

            if ( numselclusters != NumClusters ( GoGoF ) ) {
                ShowMessage ( "It seems you are trying to add a different number of clusters!" NewLine 
                              "Please check again your files...", ComputingRisTitle, ShowMessageWarning );
                return;
                }
            }

        else if ( IsChecked ( GroupCond ) ) {

            int                 numselsubjects  = gogofc[ selcl.FirstSelected () ];

            if ( numselsubjects != NumSubjects ( GoGoF ) ) {
                ShowMessage ( "It seems you are trying to add a different number of subjects!" NewLine 
                              "Please check again your files...", ComputingRisTitle, ShowMessageWarning );
                return;
                }
            }
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Adding subjects or conditions need to be handled differently
    if      ( IsChecked ( GroupSubj ) ) {
                                        // transpose groups to have subjects first
        TGoGoF              gogofs;
        gogofc.ConditionsToSubjects ( 0, gogofc.NumGroups () - 1, gogofs );

                                            // insert checked and reordered groups of files
        for ( int si = 0; si < (int) gogofs; si++ )
            AddEegGroup ( gogofs[ si ] );
        }

    else if ( IsChecked ( GroupCond ) ) {
                                        // no need to transpose, adding subjects
        for ( TIteratorSelectedForward cli ( selcl ); (bool) cli; ++cli )
            AddEegGroup ( gogofc[ cli() ] );
        }

    } // areclusters

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( areepochs ) {

    AddEegGroup ( gofeeg );
    } // areepochs

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else { // ERPs
                                        // all group at once
    AddEegGroup ( gofeeg );
    }


InverseFile->ResetCaret;
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::AddEegGroup ( const TGoF& gofeeg )
{
                                        // We enforce stronger compatibilities whthin groups, allowing only identical number of subjects or conditions
                                        // Checked are done before CheckEeg, which otherwise will find subgroups by itself

int             gofnumsubj      = IsChecked ( GroupCond ) ? gofeeg.NumFiles  () : 1;
int             gofnumcond      = IsChecked ( GroupSubj ) ? gofeeg.NumFiles  () : 1;


if ( IsChecked ( GroupSubj ) 
  && NumConditions ( GoGoF  ) > 0 
  && gofnumcond != NumConditions ( GoGoF ) ) {

    ShowMessage ( "Not the same number of conditions!", ComputingRisTitle, ShowMessageWarning );
    return;
    }


if ( IsChecked ( GroupCond ) 
  && NumSubjects ( GoGoF ) > 0 
  && gofnumsubj != NumSubjects ( GoGoF ) ) {

    ShowMessage ( "Not the same number of subjects!", ComputingRisTitle, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Force refresh
UpdateSubjectsConditions ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TransferData ( tdGetData );

                                        // temporarily adding this GoF
if      ( IsChecked ( GroupSubj ) ) {
                                        // Received a single subject - just append it
    GoGoF.Add ( &gofeeg, true, TFilenameSize );
    }

else if ( IsChecked ( GroupCond ) ) {
                                        // Receive a group of subject - we need to transpose and copy
    if ( GoGoF.IsEmpty () )             // First time only: adding subjects' groups
        for ( int fi = 0; fi < gofnumsubj; fi++ )
            GoGoF.Add ( new TGoF, false );


    for ( int fi = 0; fi < gofnumsubj; fi++ )
        GoGoF[ fi ].Add ( gofeeg[ fi ], MaxPathShort );
    }

                                        // then checking all the groups together
CheckEeg ();
//TransferData ( tdSetData );           // not part of transfer buffer - maybe later

ComputingRisTransfer.CheckInverseAndEeg  ();
TransferData ( tdSetData );

ComputingRisTransfer.CheckXyzAndEeg      ();
TransferData ( tdSetData );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // in case user dropped EEG files which are incompatible, clear-up matrix?
if ( ComputingRisTransfer.IsEegOK && ComputingRisTransfer.IsInverseOK && ! ComputingRisTransfer.IsInverseAndEegOK ) {
                                        // or asking? or silent?
//  ShowMessage ( "Clearing-up the existing Inverse Matrix...", ComputingRisTitle, ShowMessageWarning );

    Inverses.Reset ();
    ClearText   ( InverseFile );
    ComputingRisTransfer.IsInverseOK = false;
    }

                                        // in case user dropped EEG files which are incompatible, clear-up matrix?
if ( ComputingRisTransfer.IsEegOK && ComputingRisTransfer.IsXyzOK && ! ComputingRisTransfer.IsXyzAndEegOK ) {
                                        // or asking? or silent?
//  ShowMessage ( "Clearing-up the existing Electrodes Coordinates...", ComputingRisTitle, ShowMessageWarning );

    ClearText   ( XyzFile );
    ResetCheck  ( SpatialFilter );

    ComputingRisTransfer.IsXyzOK     = false;
    }

                                        // off if the inverse has been cleared up, and the EEG files given priority
if ( ComputingRisTransfer.IsEegOK /*&& ComputingRisTransfer.IsInverseAndEegOK*/ ) {

    //SetBaseFilename ();

    UpdateSubjectsConditions ();

    UpdateGroupSummary ();
    }
else {
    //GoGoF.RemoveLastGroup ();


    if ( true /* IsChecked ( GroupSubj ) */ )
        GoGoF.RemoveLastGroup ();
    else /*if ( IsChecked ( GroupCond ) )*/ {
        for ( int si = 0; si < GoGoF.NumGroups (); si++ )
            GoGoF[ si ].RemoveLast ();

        if ( GoGoF[ 0 ].NumFiles () == 0 ) GoGoF.Reset ();
        }


    UpdateSubjectsConditions ();
    }


InverseFile->ResetCaret;
}


//----------------------------------------------------------------------------
                                        // Removes either a subject or a condition, and possibly inverses
void    TComputingRisDialog::CmRemoveLastGroup ()
{
if ( ! HasSubjects ( GoGoF ) && ! HasInverses ( Inverses ) )
    return;


int                 numsubj         = NumSubjects               ( GoGoF );
int                 numinv          = NumInverses               ( Inverses );
bool                templinv        = IsSingleTemplateInverse   ( Inverses );
bool                indivinv        = AreIndividualInverses     ( Inverses, GoGoF );

                                        // Removing 1 subject
if ( IsChecked ( GroupSubj ) ) {
                                        // Removing EEG files?
    if ( numsubj > 0 ) {

        if ( numinv == 0                // no inverses at all
          || templinv                   // no individual inverses
          || numsubj >= numinv )        // individual inverses, either in sync, or more subjects than inverses

            GoGoF.RemoveLastGroup ();
        }

                                        // Removing Inverse files?
    if ( numinv > 0 ) {

        if ( numinv >= numsubj )        // either in sync, or more inverses than subjects

            Inverses.RemoveLast ();
        }
    }

                                        // Removing 1 condition
else if ( IsChecked ( GroupCond ) ) {
                                        // Removing last condition per subject
    if ( numsubj > 0 ) {

        for ( int si = 0; si < numsubj; si++ )
            GoGoF[ si ].RemoveLast ();
        }


    if      ( numsubj == 0 )            // Already no subjects -> one more click allows to remove all inverses at once

        Inverses.Reset ();


    else if ( numsubj > 0 && GoGoF[ 0 ].NumFiles () == 0 ) {// Removed last condition

        GoGoF   .Reset ();
      //Inverses.Reset ();              // Remove inverses at the same time? or next click (above)?
        }
    }


if  ( /*numinv > 0 &&*/ ! HasInverses ( Inverses ) )
    ComputingRisTransfer.IsInverseOK = false;


//SetBaseFilename ();

UpdateSubjectsConditions ();

UpdateGroupSummary ();
}


void    TComputingRisDialog::CmClearEegGroups ()
{
GoGoF   .Reset ();
Inverses.Reset ();


//SetBaseFilename ();

UpdateSubjectsConditions ();

UpdateGroupSummary ();

ComputingRisTransfer.IsEegOK     = false;
ComputingRisTransfer.IsInverseOK = false;
}


//----------------------------------------------------------------------------
                                        // Testing IS file(s) by itself/themselves
void    TComputingRisStructEx::CheckInverses ()
{
CompatInverse.Reset ();
IsInverseOK         = false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Clearing up field
ClearString ( InverseFile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // getting number of tracks is also possible with IS file

                                        // no files? try again
if ( ! HasInverses ( Inverses ) )
    return;


if ( ! Inverses.AllExtensionsAre ( AllInverseFilesExt ) ) {
    ShowMessage ( "Please provide legal Inverse matrices!" NewLine 
                  "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
    return;
    }


Inverses.AllInverseAreCompatible ( CompatInverse );


if      ( CompatInverse.NumElectrodes == CompatibilityNotConsistent ) {
    ShowMessage ( "Inverse matrices don't seem to have the same number of electrodes!" NewLine 
                  "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
    return;
    }
else if ( CompatInverse.NumElectrodes == CompatibilityIrrelevant ) {
    ShowMessage ( "Inverse matrices don't seem to have any electrodes at all!" NewLine 
                  "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
    return;
    }


if      ( CompatInverse.NumSolPoints == CompatibilityNotConsistent ) {
    ShowMessage ( "Inverse matrices don't seem to have the same number of solution points!" NewLine 
                  "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
    return;
    }
else if ( CompatInverse.NumSolPoints == CompatibilityIrrelevant ) {
    ShowMessage ( "Inverse matrices don't seem to have any solution points at all!" NewLine 
                  "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here matrices are OK by themselves
IsInverseOK         = true;

if      ( ! HasInverses ( Inverses )                    )   StringCopy  ( InverseFile, InverseFileNone  );
else if ( ! IsInverseOK                                 )   StringCopy  ( InverseFile, InverseFileNotOK );
//else if ( IsEegOK && IsInverseOK && ! IsInverseAndEegOK ) ClearString ( InverseFile );
else if ( IsSingleTemplateInverse ( Inverses         )  )   StringCopy  ( InverseFile, Inverses[ 0 ]    );
else if ( AreIndividualInverses   ( Inverses, GoGoF  )  )   StringCopy  ( InverseFile, InverseFileIndiv );
else                                                        StringCopy  ( InverseFile, NumInverses ( Inverses ) < NumSubjects ( GoGoF ) ? InverseFileTooFew : InverseFileTooMany );
}


//----------------------------------------------------------------------------
                                        // Testing the joint between IS and EEG
void    TComputingRisStructEx::CheckInverseAndEeg ()
{

IsInverseAndEegOK   = false;


if ( ! ( IsInverseOK && IsEegOK ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( CompatEegs.NumTracks != CompatInverse.NumElectrodes ) {
    ShowMessage ( "EEG files don't seem to have the same number of electrodes with the Inverse Matrix!", ComputingRisTitle, ShowMessageWarning );
    return;
    }


IsInverseAndEegOK   = true;
}


//----------------------------------------------------------------------------
                                        // Testing the joint between IS and XYZ
void    TComputingRisStructEx::CheckInverseAndXyz ()
{

IsInverseAndXyzOK   = false;


if ( ! ( IsInverseOK && IsXyzOK ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( CompatElectrodes.NumElectrodes != CompatInverse.NumElectrodes ) {
    ShowMessage ( "Electrodes Coordinates file doesn't seem to have the same number of electrodes as the Inverse Matrix!", ComputingRisTitle, ShowMessageWarning );
    return;
    }


IsInverseAndXyzOK   = true;
}


//----------------------------------------------------------------------------
                                        // Testing the joint between IS and ROIs
void    TComputingRisStructEx::CheckInverseAndRois ()
{

IsInverseAndRoisOK  = false;


if ( ! ( IsInverseOK && IsRoisOK ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // ROIs apply to the output of the IS matrix, so the # of solution points
if ( CompatRois.NumSolPoints != CompatInverse.NumSolPoints ) {

    ShowMessage ( "ROIs' dimension doesn't match the number of Solution Points from the Inverse matrix!" NewLine 
                  "Check again your input files...", ComputingRisTitle, ShowMessageWarning );

    ClearString ( RoisFile );
    ApplyRois   = BoolToCheck ( false );

    return;
    }


IsInverseAndRoisOK  = true;
}


//----------------------------------------------------------------------------
                                        // Testing EEG / Freqs across themselves - A lot of testing ahead
void    TComputingRisDialog::CheckEeg ()
{

ComputingRisTransfer.IsEegOK             = false;

ComputingRisTransfer.CompatEegs.Reset ();

if ( GoGoF.IsEmpty () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ComputingRisPresetsEnum esicase         = (ComputingRisPresetsEnum) EegPresets->GetSelIndex ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Do an early check on the preset regarding frequency or not, then update the preset for the user
                                        // Just test for the file extensions

TracksGroupClass        tg;
//bool                  freqgroups          = GoGoF.AnyTracksGroup ( tg ) && tg.allfreq;            // testing all groups
bool                    freqgroup           = GoGoF.GetLast ().AnyTracksGroup ( tg ) && tg.allfreq; // testing only the last group - more what we want


if (   freqgroup && esicase != ComputingRisPresetFreq ) {

//  ShowMessage ( "Your files seem to be all frequency ones, but your preset wasn't set to Frequency!" NewLine 
//                "Please check again your parameters...", ComputingRisTitle, ShowMessageWarning );

                                        // Update preset for the user, and clean-up EEGs - usage will tell if this is bothersome

    const TGoF*         copylastgof     = new TGoF ( GoGoF.GetLast () );

    CmClearEegGroups ();    // deleting all - needs to reintroduce the last gof, though

    GoGoF.Add ( copylastgof, false, MaxPathShort );
                                        // conveniently switching preset for the user
    esicase         = ComputingRisPresetFreq;

    EegPresets->SetSelIndex ( esicase );

    EvEegPresetsChange ();
    }


if ( ! freqgroup && esicase == ComputingRisPresetFreq ) {

//  ShowMessage ( "Your files don't seem to be frequency ones, but your preset was set to Frequency!" NewLine 
//                "Please check again your parameters...", ComputingRisTitle, ShowMessageWarning );

                                        // Update preset for the user, and clean-up EEGs - usage will tell if this is bothersome

    const TGoF*         copylastgof     = new TGoF ( GoGoF.GetLast () );

    CmClearEegGroups ();    // deleting all - needs to reintroduce the last gof, though

    GoGoF.Add ( copylastgof, false, MaxPathShort );
                                        // conveniently switching preset for the user
    bool                allerps         = GoGoF.GetLast ().AllExtensionsAre    ( AllEegErpFilesExt );

    esicase         = allerps ? ComputingRisPresetErpIndivMeans : ComputingRisPresetSpont;

    EegPresets->SetSelIndex ( esicase );

    EvEegPresetsChange ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Begins by testing the last group alone, usually the last one added
const TGoF&         lastgof         = GoGoF.GetLast ();

                                        // no files? try again
if ( lastgof.IsEmpty () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check first the last group's own coherence, as tracks files
lastgof.AllTracksAreCompatible ( ComputingRisTransfer.CompatEegs );


if      ( ComputingRisTransfer.CompatEegs.NumTracks == CompatibilityNotConsistent ) {
    ShowMessage ( "Files don't seem to have the same number of electrodes!" NewLine 
                  "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
    return;
    }
else if ( ComputingRisTransfer.CompatEegs.NumTracks == CompatibilityIrrelevant ) {
    ShowMessage ( "Files don't seem to have any electrodes at all!" NewLine 
                  "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
    return;
    }

                                        // this test is quite weak, as ReadFromHeader does a lousy job at retrieving the aux tracks (it needs the strings, so opening the whole file)
if      ( ComputingRisTransfer.CompatEegs.NumAuxTracks > 0 ) {
    ShowMessage ( "It is not allowed to compute ESI with some remaining auxiliary tracks!" NewLine 
                  "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Extra testing across conditions
bool                acrossconditions    = NumSubjects ( GoGoF ) > 1;


if      ( acrossconditions
       && ComputingRisTransfer.CompatEegs.SamplingFrequency == CompatibilityNotConsistent ) {
//  ShowMessage ( "Files don't seem to have the same sampling frequencies!" NewLine 
//                "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
    if ( ! GetAnswerFromUser ( "Files don't seem to have the same sampling frequencies!" NewLine 
                               "Do you want to proceed anyway (not recommended)?", ComputingRisTitle ) ) {
        return;
        }
    }


if      ( ComputingRisTransfer.CompatEegs.NumTF == CompatibilityIrrelevant ) {
    ShowMessage ( "Files don't seem to have any samples or time at all!" NewLine 
                  "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
    return;
    }
else if ( acrossconditions                              // we don't care for durations consistency
       && esicase  == ComputingRisPresetErpIndivMeans   // even within conditions, spontaneous files can have very different durations
       && ComputingRisTransfer.CompatEegs.NumTF == CompatibilityNotConsistent ) {
                                        // we need all files from a group to have the same length!
//  ShowMessage ( "Files don't seem to have the same number of samples/time range!" NewLine 
//                "Processing will proceed anyway...", ComputingRisTitle, ShowMessageWarning );
    ShowMessage ( "Files don't seem to have the same number of samples/time range!" NewLine 
                  "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
    return;
    }

                                        // frequencies are optional, just check that in case they exist, they should be consistent
if ( acrossconditions
  && ComputingRisTransfer.CompatEegs.NumFreqs == CompatibilityNotConsistent ) {
                                        // we need all files from a group to have the same length!
    ShowMessage ( "Files don't seem to have the same number of frequencies!" NewLine 
                  "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Group is coherent regarding file types? (doing this first, maybe?)

if ( ! lastgof.AnyTracksGroup ( tg ) ) {

    if      (   tg.noneofthese  )   ShowMessage ( "Oops, files don't really look like tracks," NewLine "are you trying to fool me?!" NewLine "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
    else if ( ! tg.alleeg       )   ShowMessage ( "Files don't seem to be consistently of the EEG type!" NewLine "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
    else if ( ! tg.allfreq      )   ShowMessage ( "Files don't seem to be consistently of the Frequency type!" NewLine "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
    else                            ShowMessage ( "Files don't seem to be consistently of the same type!" NewLine "Check again your input files...", ComputingRisTitle, ShowMessageWarning );

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check the new group's own coherence, as frequency files
FreqsCompatibleClass    fc;


if ( tg.allfreq /*or NumFreqs > CompatibilityConsistent*/ ) {


    lastgof.AllFreqsAreCompatible ( fc );

                                        // NumTracks, numtf, NumFreqs and samplingfrequency have already been investigated before...

    if ( acrossconditions
      && fc.OriginalSamplingFrequency == CompatibilityNotConsistent ) {
        ShowMessage ( "Files don't seem to have the same original sampling frequencies!" NewLine 
                      "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
        return;
        }


    if ( acrossconditions
      && fc.FreqType == CompatibilityNotConsistent ) {
        ShowMessage ( "Files don't seem to be of the same frequency types!" NewLine 
                      "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
        return;
        }

    if ( ! (    IsFreqTypeComplex ( (FrequencyAnalysisType) fc.FreqType ) 
             || IsFreqTypeReal    ( (FrequencyAnalysisType) fc.FreqType ) ) ) {
        ShowMessage ( "Files are not of Complex or Real types, which are needed to compute the inverse!" NewLine 
                      "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
        return;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check across all groups of EEG now
/*                                      // This one is a bit sensitive: the numwithinsubjects has not been updated, so D&D a different number of files can be blocked here
if ( acrossconditions 
  && esicase == ComputingRisPresetErpIndivMeans
  && ! GoGoF.AreGroupsEqual () ) {
                                    // computation is done within a (statistical) group, each group of files should have the same number of files!
    ShowMessage ( "Groups of Files don't seem to have the same number of files!" NewLine 
                  "Please do again your last group...", ComputingRisTitle, ShowMessageWarning );
    return;
    }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Now across group compatibiliy check
GoGoF.AllTracksAreCompatible ( ComputingRisTransfer.CompatEegs );


if ( ComputingRisTransfer.CompatEegs.NumTracks == CompatibilityNotConsistent ) {
    ShowMessage ( "Groups of Files don't seem to have the same number of electrodes/tracks!" NewLine 
                  "Please do again your last group...", ComputingRisTitle, ShowMessageWarning );
    return;
    }


if ( acrossconditions
  && ComputingRisTransfer.CompatEegs.SamplingFrequency == CompatibilityNotConsistent ) {
//  ShowMessage ( "Files don't seem to have the same sampling frequencies!" NewLine 
//                "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
    if ( ! GetAnswerFromUser ( "Groups of Files don't seem to have the same sampling frequencies!" NewLine 
                               "Do you want to proceed anyway (not recommended)?", ComputingRisTitle ) ) {
        return;
        }
    }

                                        // Check all groups file types(?)

if ( ! GoGoF.AnyTracksGroup ( tg ) ) {
//  if ( noneofthese )  ShowMessage ( "Oops, files don't really look like tracks," NewLine "are you trying to fool me?!" NewLine "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
//                      ShowMessage ( "Files don't seem to be consistently of the same type!" NewLine "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
    if ( ! tg.alleeg ) {
        ShowMessage ( "Groups of Files don't seem to be consistently of the EEG type across all groups!" NewLine 
                      "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
        return;
        }
    return;
    }

                                        // !this flag should be set before call to be able to run this test!
if ( CRISPresets[ esicase ].IsEpochs () ) {

    if ( ! GoGoF.AllSplitGroupsAreCompatible () ) {
        ShowMessage ( "While the number of epochs can be different per subject,"                    NewLine 
                      "the number of conditions they define should be identical across all groups." NewLine 
                      "It seems your last group is not compatible with the other existing groups!"  NewLine 
                      "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
        return;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( tg.allfreq /*or NumFreqs > CompatibilityConsistent*/ ) {

    GoGoF.AllFreqsAreCompatible ( fc );

                                    // NumTracks, numtf, NumFreqs and samplingfrequency have already been investigated before...

    if ( acrossconditions
      && fc.OriginalSamplingFrequency == CompatibilityNotConsistent ) {
        ShowMessage ( "Groups of Files don't seem to have the same original sampling frequencies!" NewLine 
                      "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
        return;
        }


    if ( acrossconditions
      && fc.FreqType == CompatibilityNotConsistent ) {
        ShowMessage ( "Groups of Files don't seem to be of the same frequency types!" NewLine 
                      "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
        return;
        }

    if ( ! (    IsFreqTypeComplex ( (FrequencyAnalysisType) fc.FreqType ) 
             || IsFreqTypeReal    ( (FrequencyAnalysisType) fc.FreqType ) ) ) {
        ShowMessage ( "Groups of Files are not of Complex or Real types, which are needed to compute the inverse!" NewLine 
                      "Check again your input files...", ComputingRisTitle, ShowMessageWarning );
        return;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We did it!
ComputingRisTransfer.IsEegOK             = true;
}


//----------------------------------------------------------------------------
                                        // Testing XYZ file(s) by itself/themselves
void    TComputingRisStructEx::CheckXyz ()
{
CompatElectrodes.Reset ();
IsXyzOK             = false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Make a local copy of the file name, and preemptively clearing up the dialog's one
TFileName           xyzfile;

StringCopy  ( xyzfile, XyzFile );

if ( StringIsEmpty ( xyzfile ) )
    return;

ClearString ( XyzFile );
SpatialFilter   = BoolToCheck ( false );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // getting number of tracks is also possible with xyz file
TGoF                xyzgof;

xyzgof.Add ( xyzfile );


if ( ! xyzgof.AllExtensionsAre ( AllCoordinatesFilesExt ) ) {
    ShowMessage ( "Please provide some legal Electrodes Coordinates data!", ComputingRisTitle, ShowMessageWarning );
    return;
    }


xyzgof.AllElectrodesAreCompatible ( CompatElectrodes );


if      ( CompatElectrodes.NumElectrodes == CompatibilityNotConsistent ) {
    ShowMessage ( "Electrodes Coordinates don't seem to have the same number of electrodes!", ComputingRisTitle, ShowMessageWarning );
    return;
    }
else if ( CompatElectrodes.NumElectrodes == CompatibilityIrrelevant ) {
    ShowMessage ( "Electrodes Coordinates don't seem to have any electrodes at all!", ComputingRisTitle, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here matrices are OK by themselves
IsXyzOK             = true;
                                        // restore file name, now it that it has been checked
StringCopy  (  XyzFile, xyzfile );
SpatialFilter   = BoolToCheck ( true  );
}


//----------------------------------------------------------------------------
                                        // Testing the joint between XYZ and EEG
void    TComputingRisStructEx::CheckXyzAndEeg ()
{

IsXyzAndEegOK       = false;


if ( ! ( IsXyzOK && IsEegOK ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( CompatEegs.NumTracks != CompatElectrodes.NumElectrodes ) {
//    char                buff[ 256 ];
//    sprintf ( buff, "Not the same amount of Electrodes:" NewLine NewLine "  - EEG files " NewLine "= %0d" NewLine "  - Electrodes file " Tab "= %0d" NewLine NewLine "Please check again your files!", CompatEegs.NumTracks, xyznumel );

    ShowMessage ( "EEG files don't seem to have the same number of electrodes with the Electrodes Coordinates!" NewLine 
                  "Check again your input files...", ComputingRisTitle, ShowMessageWarning );

    ClearString ( XyzFile );
    SpatialFilter   = BoolToCheck ( false );
    return;
    }


IsXyzAndEegOK       = true;
}


//----------------------------------------------------------------------------
                                        // Testing ROIs file(s) by itself/themselves
void    TComputingRisStructEx::CheckRois ()
{
CompatRois.Reset ();
IsRoisOK            = false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Make a local copy of the file name, and preemptively clearing up the dialog's one
TFileName           roisfile;

StringCopy  ( roisfile, RoisFile );

if ( StringIsEmpty ( roisfile ) )
    return;

ClearString ( RoisFile );
ApplyRois       = BoolToCheck ( false );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // getting number of tracks is also possible with xyz file
TGoF                roisgof;

roisgof.Add ( roisfile );


if ( ! roisgof.AllExtensionsAre ( AllRoisFilesExt ) ) {
    ShowMessage ( "Please provide some legal ROIs data!", ComputingRisTitle, ShowMessageWarning );
    return;
    }

                                        // There is currently no handy function for that, just try to open the ROIs and see
roisgof.AllRoisAreCompatible ( CompatRois );


if      ( CompatRois.NumDimensions == CompatibilityNotConsistent ) {
    ShowMessage ( "ROIs don't seem to have the same number of Solution Points!", ComputingRisTitle, ShowMessageWarning );
    return;
    }
else if ( CompatRois.NumDimensions == CompatibilityIrrelevant ) {
    ShowMessage ( "ROIs don't seem to have the right dimension at all!", ComputingRisTitle, ShowMessageWarning );
    return;
    }


if      ( CompatRois.NumRois == CompatibilityNotConsistent ) {
    ShowMessage ( "ROIs don't seem to have the same number regions!", ComputingRisTitle, ShowMessageWarning );
    return;
    }
else if ( CompatRois.NumRois == CompatibilityIrrelevant ) {
    ShowMessage ( "ROIs don't seem to have the right dimension at all!", ComputingRisTitle, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here matrices are OK by themselves
IsRoisOK            = true;
                                        // restore file name, now it that it has been checked
StringCopy  (  RoisFile, roisfile );
ApplyRois       = BoolToCheck ( true  );
                                        // ROIs can not be computed on vectorial data
VectorData      = false;
PositiveData    = true;
}


//----------------------------------------------------------------------------
                                        // Completely refresh the whole list of groups
void    TComputingRisDialog::UpdateGroupSummary ()
{
                                        // clearing the whole list
GroupsSummary->ClearList ();


ComputingRisPresetsEnum esicase         = (ComputingRisPresetsEnum) EegPresets  ->GetSelIndex ();
TFixedString<KiloByte>  buff;
int                     maxlength       = 0;


int                 numsubjects         = NumSubjects   ( GoGoF    );
int                 numconditions       = NumConditions ( GoGoF    );
int                 numinverses         = NumInverses   ( Inverses );

                                        // We can specialize the names of subjects and conditions for better understanding
const char*         subjstr             = CRISPresets[ esicase ].Code == ComputingRisPresetErpGroupMeans ?  "Group" 
                                                                                                         :  "Subject";

const char*         condstr             = CRISPresets[ esicase ].IsEpochs   ()                           ?  "Epoch File" 
                                        : CRISPresets[ esicase ].IsClusters ()                           ?  "Cluster" 
                                                                                                         :  "Condition";

                                        // Recap by conditions
if ( IsChecked ( GroupCond ) ) {

    for ( int ci = 0; ci < numconditions; ci++ ) {

        buff    = condstr;
        buff   += " " + IntegerToString ( ci + 1, NumIntegerDigits ( numconditions ) ) + ":";


        int         numsubjs    = 0;
        for ( int si = 0; si < numsubjects; si++ )
            numsubjs   += ci < GoGoF[ si ].NumFiles ();

        buff   += "  ";
        buff   += IntegerToString ( numsubjs ) + " " + subjstr + StringPlural ( numsubjs );


        if ( numsubjs == 1 )    StringAppend    ( buff, "  ", ToFileName ( GoGoF[ 0 ][ ci ] )                                                      );
        else                    StringAppend    ( buff, "  ", ToFileName ( GoGoF[ 0 ][ ci ] ), " .. ", ToFileName ( GoGoF[ numsubjs - 1 ][ ci ] ) );


        GroupsSummary->InsertString ( buff, 0 );

        Maxed ( maxlength, (int) StringLength ( buff ) );
        }
    }

                                        // Recap by subjects
if ( IsChecked ( GroupSubj ) ) {

    for ( int si = 0; si < numsubjects; si++ ) {

        buff    = subjstr;
        buff   += " " + IntegerToString ( si + 1, NumIntegerDigits ( numsubjects ) ) + ":";


        const TGoF&     conds       = GoGoF[ si ];

        buff   += "  ";
        buff   += IntegerToString ( conds.NumFiles () ) + " " + condstr + StringPlural ( conds.NumFiles () );


        if ( conds.NumFiles () == 1 )   StringAppend    ( buff, "  ", ToFileName ( conds.GetFirst () )                                          );
        else                            StringAppend    ( buff, "  ", ToFileName ( conds.GetFirst () ), " .. ", ToFileName ( conds.GetLast () ) );


        if ( HasInverses ( Inverses ) ) {
            
            StringAppend    ( buff, "; Inverse  " );

            if      ( IsSingleTemplateInverse ( Inverses ) )    StringAppend    ( buff, ToFileName ( Inverses[  0 ] ) );
            else if ( si < numinverses )                        StringAppend    ( buff, ToFileName ( Inverses[ si ] ) );
            else                                                StringAppend    ( buff, "!Missing!" );
            }


        GroupsSummary->InsertString ( buff, 0 );

        Maxed ( maxlength, (int) StringLength ( buff ) );
        }
    }


UpdateHorizontalScroller ( GroupsSummary, maxlength );


SetInteger  ( NumSubjectsEdit,   numsubjects                );
SetInteger  ( NumConditionsEdit, numconditions              );
SetInteger  ( NumInversesEdit,   NumInverses ( Inverses )   );

//SetBaseFilename ();
}


//----------------------------------------------------------------------------
                                        // Updating the number of subjects & conditions - we now enforce strong consistency, every added group should be compatible with all the others
void    TComputingRisDialog::UpdateSubjectsConditions ( bool updategroups )
{
ComputingRisPresetsEnum esicase         = (ComputingRisPresetsEnum) EegPresets  ->GetSelIndex ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Updating dialog
SetInteger  ( NumSubjectsEdit  , NumSubjects   ( GoGoF    ) );
SetInteger  ( NumConditionsEdit, NumConditions ( GoGoF    ) );
SetInteger  ( NumInversesEdit  , NumInverses   ( Inverses ) );


if      ( ! HasInverses ( Inverses )                    )   SetText ( InverseFile, InverseFileNone  );
else if ( ! ComputingRisTransfer.IsInverseOK            )   SetText ( InverseFile, InverseFileNotOK );
else if ( IsSingleTemplateInverse ( Inverses         )  )   SetText ( InverseFile, Inverses[ 0 ]    );
else if ( AreIndividualInverses   ( Inverses, GoGoF  )  )   SetText ( InverseFile, InverseFileIndiv );
else                                                        SetText ( InverseFile, NumInverses ( Inverses ) < NumSubjects ( GoGoF ) ? InverseFileTooFew : InverseFileTooMany );

InverseFile->ResetCaret;


if ( updategroups )
                                        // Setting Group Averages if more than 1 subject for a few presets
    SetCheck    ( ComputeGroupsAverages, (    esicase == ComputingRisPresetErpIndivMeans 
                                           || esicase == ComputingRisPresetErpIndivEpochs
                                           || esicase == ComputingRisPresetIndIndivEpochs )
                                         && NumSubjects ( GoGoF ) > 1                       );
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::CmOkEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );


if ( ! ( IsSingleTemplateInverse ( Inverses ) || AreIndividualInverses ( Inverses, GoGoF ) )
  || ! ComputingRisTransfer.IsInverseOK ) {
    tce.Enable ( false );
    return;
    }


if ( GoGoF.IsEmpty () 
  || ! ComputingRisTransfer.IsEegOK ) {
    tce.Enable ( false );
    return;
    }


if ( CheckToBool ( ComputingRisTransfer.SpatialFilter ) 
  && ( StringIsEmpty ( ComputingRisTransfer.XyzFile ) 
    || ! ComputingRisTransfer.IsXyzOK ) ) {
    tce.Enable ( false );
    return;
    }


if ( CheckToBool ( ComputingRisTransfer.RegFixed ) 
  && ! IsRegularRegularization ( ComputingRisTransfer.RegFixedEdit ) ) {
    tce.Enable ( false );
    return;
    }


if ( CheckToBool ( ComputingRisTransfer.Envelope ) 
  && ! CheckToBool ( ComputingRisTransfer.VectorData )
  && StringToDouble ( ComputingRisTransfer.EnvelopeLowFreq ) <= 0 ) {
    tce.Enable ( false );
    return;
    }


if ( CheckToBool ( ComputingRisTransfer.ApplyRois ) 
  && ( StringIsEmpty ( ComputingRisTransfer.RoisFile ) 
    || ! ComputingRisTransfer.IsRoisOK ) ) {
    tce.Enable ( false );
    return;
    }

                                        // any processing?
if ( ! (    CheckToBool ( ComputingRisTransfer.SavingIndividualFiles ) 
      ||    CheckToBool ( ComputingRisTransfer.TimeZScore            ) 
         && CheckToBool ( ComputingRisTransfer.ComputeZScore         )  
         && CheckToBool ( ComputingRisTransfer.SavingZScoreFactors   ) 
      || CheckToBool ( ComputingRisTransfer.SavingEpochFiles         )
      || CheckToBool ( ComputingRisTransfer.ComputeGroupsAverages    )
      || CheckToBool ( ComputingRisTransfer.ComputeGroupsCentroids   ) ) ) {
    tce.Enable ( false );
    return;
    }


tce.Enable ( true );
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::CmOk ()
{
Destroy ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // well, we do need some data...
if ( GoGoF.IsEmpty () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ComputingRisPresetsEnum esicase         = (ComputingRisPresetsEnum) EegPresets->GetSelIndex ();

                                        // Tracks output options
bool                savingindividualfiles   = CheckToBool ( ComputingRisTransfer.SavingIndividualFiles  );

bool                savingepochfiles        = CheckToBool ( ComputingRisTransfer.SavingEpochFiles       );

bool                computegroupsaverages   = CheckToBool ( ComputingRisTransfer.ComputeGroupsAverages  );

bool                computegroupscentroids  = CheckToBool ( ComputingRisTransfer.ComputeGroupsCentroids );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // This should have been done before when calling CheckEeg
//bool                freqgroups          = GoGoF.AllFreqsGroup ();
//
//FreqsCompatibleClass    fc;
//
//if ( freqgroups )
//
//    GoGoF.AllFreqsAreCompatible ( fc );
//
//
//bool                freqtypecomplex   = freqgroups && IsFreqTypeComplex ( (FrequencyAnalysisType) fc.FreqType );
////bool              freqtypereal      = freqgroups && IsFreqTypeReal    ( (FrequencyAnalysisType) fc.freqtype );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numsubjects         = NumSubjects   ( GoGoF    );
int                 numconditions       = NumConditions ( GoGoF    );
int                 numinverses         = NumInverses   ( Inverses );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We can have either 1 inverse matrix per subject, or 1 matrix for all subjects
bool                individualinverses  = AreIndividualInverses ( Inverses, GoGoF );
                                        // Already tested(?)
//if ( ! Inverses.CanOpenFiles () )
//    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // It might be a good idea to confirm the data layout has been well understood
char                buff[ KiloByte ];

StringCopy      ( buff, "Do you confirm your data layout is:" NewLine 
                        NewLine 
                        "    " );

StringAppend    ( buff, IntegerToString ( numsubjects ),   " ",    esicase == ComputingRisPresetErpGroupMeans 
                                                                || esicase == ComputingRisPresetErpSegClusters  ? "Group"   : "Subject",   StringPlural ( numsubjects ) );
StringAppend    ( buff, "  x  " );

StringAppend    ( buff, IntegerToString ( numconditions ), " ", CRISPresets[ esicase ].IsClusters ()            ? "Cluster" : "Condition", StringPlural ( numconditions ) );

StringAppend    ( buff, NewLine
                        NewLine
                        "    " );

if ( individualinverses )
    StringAppend    ( buff, IntegerToString ( numsubjects ), " Individual Inverse Matrices" );
else
    StringAppend    ( buff, "1 Template Inverse Matrix" );


if ( ( numsubjects > 1 || numconditions > 1 ) &&    // no need to t ask for single file processing
   ! GetAnswerFromUser   ( buff, ComputingRisTitle, this ) ) {

    ShowMessage ( "Please check your presets and/or input files and come back again...", ComputingRisTitle, ShowMessageWarning, this );

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Offer to review the subjects / inverses pairing?
if ( individualinverses && numinverses > 1 
  && GetAnswerFromUser   ( "Do you wish to review all subjects' EEGs and Inverse pairing?", ComputingRisTitle, this ) ) {

    char        title[ 64 ];

    for ( int subji = 0; subji < numsubjects; subji++ ) {

        ClearString ( buff );

        StringAppend ( buff, "EEG:", NewLine );
        for ( int fi = 0; fi < numconditions; fi++ )
            StringAppend ( buff, Tab, ToFileName ( GoGoF[ subji ][ fi ] ), NewLine );

        StringAppend ( buff, NewLine );
        StringAppend ( buff, "Inverse Matrix:", NewLine );
        StringAppend ( buff, Tab, ToFileName ( Inverses[ subji ] ) );

        StringCopy  ( title, "Confirm for Subject #", IntegerToString ( subji + 1, NumIntegerDigits ( numsubjects ) ) );
        
        if ( ! GetAnswerFromUser ( buff, title, this ) ) {
            ShowMessage ( "Please check again your input files and come back again...", ComputingRisTitle, ShowMessageWarning, this );
            return;
            }
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // No Spatial Filter if frequency input
SpatialFilterType   spatialfilter   = (    CheckToBool ( ComputingRisTransfer.SpatialFilter ) 
                                        && esicase != ComputingRisPresetFreq                  ) ? SpatialFilterDefault : SpatialFilterNone;
TFileName           xyzfile;


if ( spatialfilter != SpatialFilterNone ) {

    StringCopy ( xyzfile, ComputingRisTransfer.XyzFile );

    if ( ! CanOpenFile ( xyzfile ) ) {
        spatialfilter   = SpatialFilterNone;
        xyzfile.Clear ();
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Data types - we need some extra care here

                                        // INTERMEDIATE Epochs data type: ERP always using Vectors; Induced always using Norm; there are currently no other epochs cases
AtomType            datatypeepochs  = CRISPresets[ esicase ].GetAtomTypeEpochs ();

                                        // FINAL requested data type
AtomType            datatypefinal   = CheckToBool ( ComputingRisTransfer.PositiveData   )   ?   AtomTypePositive
                                    : CheckToBool ( ComputingRisTransfer.VectorData     )   ?   AtomTypeVector
                                    :                                                           AtomTypePositive;
                                        // ACTUAL processing data type + Z-Score + Envelope
AtomType            datatypeproc    = CRISPresets[ esicase ].GetAtomTypeProcessing ( datatypeepochs, datatypefinal );

                                        // check compatiblity betweeen these 2
                    datatypefinal   = CRISPresets[ esicase ].GetAtomTypeFinal ( datatypeproc, datatypefinal );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // epochs / subjects initial ranking
bool                ranking         = CheckToBool ( ComputingRisTransfer.Rank      );

bool                thresholding    = false;    // note that only the combo ranking + thresholding is currently allowed
                                        // Using a fixed threshold at each step
double              keepingtopdata  = 0.10;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Set regularization
RegularizationType  regularization      = CheckToBool ( ComputingRisTransfer.RegAuto ) ? RegularizationAutoGlobal
                                                                                       : Clip ( (RegularizationType) StringToInteger ( ComputingRisTransfer.RegFixedEdit ), FirstRegularization, LastRegularization );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Z-Score options
BackgroundNormalization     backnorm    = ! CheckToBool ( ComputingRisTransfer.TimeZScore       ) ? BackgroundNormalizationNone
                                        :   CheckToBool ( ComputingRisTransfer.ComputeZScore    ) ? BackgroundNormalizationComputingZScore
                                        :   CheckToBool ( ComputingRisTransfer.LoadZScoreFile   ) ? BackgroundNormalizationLoadingZScoreFile
                                        :                                                           BackgroundNormalizationNone;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Envelope mainly for Induced filtered data - but option is still open for the others
bool                envelope            = CheckToBool ( ComputingRisTransfer.Envelope )
                                       && ! IsVector ( datatypeproc );
FilterTypes         envelopetype;
double              envelopelowfreq     = envelope ? StringToDouble ( ComputingRisTransfer.EnvelopeLowFreq ) : 0;
double              envelopeduration;


if ( envelope && envelopelowfreq > 0 ) {
    envelopetype        = RisEnvelopeMethod;
    envelopeduration    = FrequencyToMilliseconds ( envelopelowfreq );
    }
else {
    envelope            = false;
    envelopetype        = FilterTypeNone;
    envelopelowfreq     = 0;
    envelopeduration    = 0;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                roiing          = CheckToBool ( ComputingRisTransfer.ApplyRois ) 
                                    && ! IsVector ( datatypeproc );     // we have to do something to forbid ROIing on non-scalar data
TFileName           roifile;


if ( roiing ) {

    StringCopy ( roifile, ComputingRisTransfer.RoisFile );

    if ( ! CanOpenFile ( roifile ) ) {
        roiing      = false;
        roifile.Clear ();
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // More tracks output options
bool                savingzscorefactors     = CheckToBool ( ComputingRisTransfer.SavingZScoreFactors    )
                                           && backnorm != BackgroundNormalizationNone;
//                                         && backnorm == BackgroundNormalizationComputingZScore;       // note that we could allow a new copy of Z-Scores in case of loading from file...


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           basedir;
TFileName           basefilename    = ComputingRisTransfer.BaseFileName;
                                        // use first file directory as the base directory
StringCopy      ( basedir,  GoGoF[ 0 ][ 0 ] );
RemoveFilename  ( basedir );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! ComputingRis (   esicase,

                        GoGoF,              
                        numsubjects,            numconditions,

                        Inverses,               regularization,         backnorm,
                        datatypeepochs,         datatypefinal,
                        RisCentroidMethod,
                        
                        spatialfilter,          xyzfile,
                        ranking,
                        thresholding,           keepingtopdata,
                        envelope,               envelopetype,           envelopelowfreq,    envelopeduration,
                        roiing,                 roifile,                RisRoiMethod,

                        savingindividualfiles,  savingepochfiles,       savingzscorefactors,
                        computegroupsaverages,  computegroupscentroids,
                        basedir,                basefilename,
                        Interactive
                    ) )

    ShowMessage ( "Something went wrong during the RIS processing!" NewLine 
                  "Check your parameters as well as your input files...", ComputingRisTitle, ShowMessageWarning );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
