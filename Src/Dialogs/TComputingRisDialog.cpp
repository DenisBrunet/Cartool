/************************************************************************\
� 2024-2025 Denis Brunet, University of Geneva, Switzerland.

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
TGoGoF          GoGoF;                      // Groups of Groups of EEG files
TGoF            Inverses;                   // Groups of Inverse files
int             NumSubjects         = 0;    // Computed from GoGoF
int             NumConditions       = 0;

int             NumInverses             ()  { return  (int)  Inverses; }
bool            AreIndividualInverses   ()  { return  (bool) Inverses && NumInverses () == NumSubjects; }   // !these 2 tests are not mutually exclusive!
bool            IsTemplateInverse       ()  { return  NumInverses () == 1; }


//----------------------------------------------------------------------------

const char  GroupsLayoutString[ NumGroupsLayout ][ 64 ] =
            {
            "All Subjects, 1 Condition",
            "1 Subject, All Conditions",
            "1 Subject, All Epochs  AND  All Conditions",
            };


CRISPresetSpec  CRISPresets[ NumComputingRisPresets ] =
            {
            {   ComputingRisPresetErpGroupMeans,    "ERP Data               - Grand Averages",                      (CRISPresetFlags) ( CRISPresetERP   | CRISPresetNorm | CRISPresetVect                                                               ),    CRISPresetNorm,     GroupsLayout1SubjAllCond            },  // norm or vector doesn't matter
            {   ComputingRisPresetErpIndivMeans,    "ERP Data               - Subjects' Averages",                  (CRISPresetFlags) ( CRISPresetERP   | CRISPresetNorm | CRISPresetVect                                                               ),    CRISPresetVect,     GroupsLayoutAllSubj1Cond            },  // norm only, subjects dipoles vary too much even after ERP
            {   ComputingRisPresetErpIndivEpochs,   "ERP Data               - Subjects' Epochs",                    (CRISPresetFlags) ( CRISPresetERP   | CRISPresetNorm | CRISPresetVect                                            | CRISPresetEpochs ),    CRISPresetVect,     GroupsLayout1SubjAllCondAllEpochs   },  // vector only, we need vectors from same SP to cancel each others
            {   ComputingRisPresetErpSegClusters,   "ERP Data               - Clusters from Segmentation output",   (CRISPresetFlags) ( CRISPresetERP   | CRISPresetNorm | CRISPresetVect | CRISPresetClusters | CRISPresetCentroids                    ),    CRISPresetVect,     GroupsLayoutAllSubj1Cond            },  // norm or vector doesn't matter
            {   ComputingRisPresetErpFitClusters,   "ERP Data               - Clusters from Fitting output",        (CRISPresetFlags) ( CRISPresetERP   | CRISPresetNorm | CRISPresetVect | CRISPresetClusters | CRISPresetCentroids                    ),    CRISPresetVect,     GroupsLayoutAllSubj1Cond            },  // norm only, vectors vary too much already within a subject, and worse across subjects

            {   ComputingRisPresetSeparator1,       "",                                                             CRISPresetNone,                                                                                                                           CRISPresetNone,     GroupsLayoutUnknown                 },
                                                                                                                                                                                                              // Not sure about centroids here
            {   ComputingRisPresetIndIndivEpochs,   "Induced Responses - Subjects' Epochs",                         (CRISPresetFlags) ( CRISPresetInd   | CRISPresetNorm                                       | CRISPresetCentroids | CRISPresetEpochs ),    CRISPresetNorm,     GroupsLayout1SubjAllCondAllEpochs   },  // norm only, we need to ditch out phase and keep only the power contribution

            {   ComputingRisPresetSeparator2,       "",                                                             CRISPresetNone,                                                                                                                           CRISPresetNone,     GroupsLayoutUnknown                 },
                                                                                                                                                                        // vectorial spontaneous not really needed, but let's give the option
            {   ComputingRisPresetSpont,            "Resting States        - Spontaneous Data",                     (CRISPresetFlags) ( CRISPresetSpont | CRISPresetNorm | CRISPresetVect                                                               ),    CRISPresetNorm,     GroupsLayoutAllSubj1Cond            },
                                                                                                                                                                        // vectorial spontaneous centroids has not been tested - it checks polarity per solution point
            {   ComputingRisPresetSpontClusters,    "Resting States        - Clusters from Fitting output",         (CRISPresetFlags) ( CRISPresetSpont | CRISPresetNorm | CRISPresetVect | CRISPresetClusters | CRISPresetCentroids                    ),    CRISPresetNorm,     GroupsLayoutAllSubj1Cond            },

            {   ComputingRisPresetSeparator3,       "",                                                             CRISPresetNone,                                                                                                                           CRISPresetNone,     GroupsLayoutUnknown                 },

            {   ComputingRisPresetFreq,             "Frequency Data      - Complex results",                        (CRISPresetFlags) ( CRISPresetFreq  | CRISPresetNorm | CRISPresetVect                                                               ),    CRISPresetNorm,     GroupsLayoutAllSubj1Cond            },
            };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // declare this global variable
TComputingRisStructEx   ComputingRisTransfer;


        TComputingRisStruct::TComputingRisStruct ()
{
EegPresets.Clear ();
for ( int i = 0; i < NumComputingRisPresets; i++ )
    EegPresets.AddString ( CRISPresets[ i ].Text, i == ComputingRisPresetDefault );

GroupPresets.Clear ();
for ( int i = 0; i < NumGroupsLayout; i++ )
    GroupPresets.AddString ( GroupsLayoutString[ i ], i == CRISPresets[ ComputingRisPresetDefault ].GroupsLayout );

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

ClearString     ( InverseFile );
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

    EV_CBN_SELCHANGE            ( IDC_GROUPPRESETS,             EvGroupPresetsChange ),
    EV_COMMAND_ENABLE           ( IDC_GROUPPRESETS,             CmGroupPresetsEnable ),
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
    EV_COMMAND                  ( IDC_REMFROMLIST,              CmRemoveEegGroup ),
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

GroupPresets            = new TComboBox     ( this, IDC_GROUPPRESETS );
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


static bool     init    = true;

if ( init ) {
    init    = false;
                                        // Internal variables
    NumSubjects         = 0;
    NumConditions       = 0;
    }
}


        TComputingRisDialog::~TComputingRisDialog ()
{
delete  EegPresets;
delete  GroupPresets;
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
                                        // setting these 2 guys
UpdateSubjectsConditions ( false );
                                        // ?
//CheckEeg ();
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::EvEegPresetsChange ()
{

ComputingRisPresetsEnum esicase         = (ComputingRisPresetsEnum) EegPresets->GetSelIndex ();

if ( CRISPresets[ esicase ].Flags == CRISPresetNone )
    return;


//SpatialFilter           ->SetCheck ( BoolToCheck ( false ) );

Rank                    ->SetCheck ( BoolToCheck ( false ) );

PositiveData            ->SetCheck ( BoolToCheck ( CRISPresets[ esicase ].IsNorm () ) );
VectorData              ->SetCheck ( BoolToCheck ( CRISPresets[ esicase ].IsVect () ) );

RegAuto                 ->SetCheck ( BoolToCheck ( false ) );
RegFixed                ->SetCheck ( BoolToCheck ( false ) );
RegFixedEdit            ->SetText  ( DefaultRegularization );           // force the recommended mean value in case user switches to fixed

Envelope                ->SetCheck ( BoolToCheck ( false ) );           // force it off each time, for the moment

SavingIndividualFiles   ->SetCheck ( BoolToCheck ( false ) );
if ( ! CRISPresets[ esicase ].IsEpochs () )                             // in case of Epochs preset, not resetting this button which is user's preference
    SavingEpochFiles    ->SetCheck ( BoolToCheck ( false ) );
ComputeGroupsAverages   ->SetCheck ( BoolToCheck ( false ) );
ComputeGroupsCentroids  ->SetCheck ( BoolToCheck ( false ) );
SavingZScoreFactors     ->SetCheck ( BoolToCheck ( true  ) );


switch ( esicase ) {

    case ComputingRisPresetErpGroupMeans:

        RegAuto                 ->SetCheck ( BoolToCheck ( true  ) );

        SavingIndividualFiles   ->SetCheck ( BoolToCheck ( true ) );
        break;

    case ComputingRisPresetErpIndivMeans:
                                        // check file names if it seems Spatial Filter was already applied?
//      SpatialFilter           ->SetCheck ( ! GoGoF.AllStringsGrep ( PostfixSpatialGrep ) );
        RegAuto                 ->SetCheck ( BoolToCheck ( true  ) );
                                        // only well structured data can have a global averaging
        ComputeGroupsAverages   ->SetCheck ( BoolToCheck ( NumSubjects > 1 ) );
        SavingIndividualFiles   ->SetCheck ( BoolToCheck ( true ) );
        break;

    case ComputingRisPresetErpIndivEpochs:
                                        // check file names if it seems Spatial Filter was already applied?
//      SpatialFilter           ->SetCheck ( ! GoGoF.AllStringsGrep ( PostfixSpatialGrep ) );
        RegAuto                 ->SetCheck ( BoolToCheck ( true  ) );
                                        // only well structured data can have a global averaging
        ComputeGroupsAverages   ->SetCheck ( BoolToCheck ( NumSubjects > 1 ) );
        SavingIndividualFiles   ->SetCheck ( BoolToCheck ( true ) );
        break;

    case ComputingRisPresetErpSegClusters:

        RegAuto                 ->SetCheck ( BoolToCheck ( true  ) );
        Rank                    ->SetCheck ( BoolToCheck ( true  ) );
                                        // only well structured data can have a global averaging
        ComputeGroupsCentroids  ->SetCheck ( BoolToCheck ( true ) );
        break;

    case ComputingRisPresetErpFitClusters:
                                        // check file names if it seems Spatial Filter was already applied?
//      SpatialFilter           ->SetCheck ( ! GoGoF.AllStringsGrep ( PostfixSpatialGrep ) );
        RegAuto                 ->SetCheck ( BoolToCheck ( true  ) );
        Rank                    ->SetCheck ( BoolToCheck ( true  ) );
                                        // only well structured data can have a global averaging
        ComputeGroupsCentroids  ->SetCheck ( BoolToCheck ( true ) );
        break;


    case ComputingRisPresetIndIndivEpochs:
                                        // check file names if it seems Spatial Filter was already applied?
//      SpatialFilter           ->SetCheck ( ! GoGoF.AllStringsGrep ( PostfixSpatialGrep ) );
        RegAuto                 ->SetCheck ( BoolToCheck ( true  ) );
                                        // force it on each time
        Envelope                ->SetCheck ( BoolToCheck ( true  ) );
                                        // only well structured data can have a global averaging
        ComputeGroupsAverages   ->SetCheck ( BoolToCheck ( NumSubjects > 1 ) );
        SavingIndividualFiles   ->SetCheck ( BoolToCheck ( true ) );
//      ComputeGroupsCentroids  ->SetCheck ( BoolToCheck ( true ) );
        break;


    case ComputingRisPresetSpont:
                                        // check file names if it seems Spatial Filter was already applied?
//      SpatialFilter           ->SetCheck ( ! GoGoF.AllStringsGrep ( PostfixSpatialGrep ) );
        RegAuto                 ->SetCheck ( BoolToCheck ( true  ) );

        SavingIndividualFiles   ->SetCheck ( BoolToCheck ( true  ) );
        ComputeGroupsCentroids  ->SetCheck ( BoolToCheck ( false ) );
        break;

    case ComputingRisPresetSpontClusters:
                                        // check file names if it seems Spatial Filter was already applied?
//      SpatialFilter           ->SetCheck ( ! GoGoF.AllStringsGrep ( PostfixSpatialGrep ) );
        RegAuto                 ->SetCheck ( BoolToCheck ( true  ) );
        Rank                    ->SetCheck ( BoolToCheck ( true  ) );

        SavingIndividualFiles   ->SetCheck ( BoolToCheck ( false ) );
        ComputeGroupsCentroids  ->SetCheck ( BoolToCheck ( true  ) );
        break;


    case ComputingRisPresetFreq:

        RegFixed                ->SetCheck ( BoolToCheck ( true  ) );

        SavingIndividualFiles   ->SetCheck ( BoolToCheck ( true ) );
        break;

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Special handling when general presets goes or leaves the Epochs averaging cases

static  ComputingRisPresetsEnum oldesicase  = ComputingRisPresetDefault;

                                        // quitting state -> save group layout, which user might have changed to his/her liking
CRISPresets[ oldesicase ].GroupsLayout  = (GroupsLayoutEnum) GroupPresets->GetSelIndex ();

                                        // avoid storing weird case of Epochs if preset is not for epochs(?)
//if ( ! CRISPresets[ oldesicase ].IsEpochs ()
//  && CRISPresets[ oldesicase ].GroupsLayout == GroupsLayout1SubjAllCondAllEpochs )
//
//    CRISPresets[ oldesicase ].GroupsLayout  = GroupsLayoutDefault;

                                        // getting ready for the next call
oldesicase  = esicase;

                                        // restore previous state
GroupPresets->SetSelIndex ( CRISPresets[ esicase ].GroupsLayout );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // update transfer buffer(?)
//TransferData ( tdGetData );
                                        // only for dialog update
UpdateSubjectsConditions ();

UpdateGroupSummary ();
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::EvGroupPresetsChange ()
{
                                        // only for dialog update
UpdateSubjectsConditions ();

UpdateGroupSummary ();
}


void    TComputingRisDialog::CmGroupPresetsEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! (   CRISPresets[ EegPresets->GetSelIndex () ].IsEpochs   ()
                || CRISPresets[ EegPresets->GetSelIndex () ].IsClusters () ) );
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

int                 numgroups           = GoGoF.NumGroups   ();
int                 maxfiles            = GoGoF.GetMaxFiles ();
bool                individualinverses  = AreIndividualInverses ();

                                        // header line describes the attributes / fields
sf.WriteAttribute ( "numfiles" );

for ( int fi = 0; fi < maxfiles; fi++ )
    sf.WriteAttribute ( "file", fi + 1 );

if ( individualinverses && Is1Group1Subject () )
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
    if ( individualinverses && Is1Group1Subject () )
        sf.WriteAttribute ( Inverses[ gogofi ] );


    sf.WriteNextRecord ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Add a single row, for all subjects?
if ( individualinverses && Is1Group1Condition () ) {

                                        // write number of inverses
    sf.WriteAttribute ( NumInverses () );

                                        // write all files
    for ( int fi = 0; fi < NumInverses (); fi++ )
        sf.WriteAttribute ( Inverses[ fi ] );

                                        // complete line with empty attributes
    for ( int fi = NumInverses (); fi < maxfiles; fi++ )
        sf.WriteAttribute ( "" );


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
    appendrows  =                          answer == EOS
               || Is1Group1Subject   () && answer == 'S'
               || Is1Group1Condition () && answer == 'C';
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
//    SpatialFilter   ->SetCheck ( ! GoGoF.AllStringsGrep ( PostfixSpatialGrep, GrepOptionDefaultFiles ) );
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
//      SpatialFilter   ->SetCheck ( ! GoGoF.AllStringsGrep ( PostfixSpatialGrep ) );
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

    XyzFile         ->SetText ( "" );
    SpatialFilter   ->SetCheck ( BoolToCheck ( false ) );

    ComputingRisTransfer.IsXyzOK             = false;
    ComputingRisTransfer.IsInverseAndXyzOK   = false;
    }


                                        // in case user dropped ROIs file which is incompatible
if ( ComputingRisTransfer.IsInverseOK && ComputingRisTransfer.IsRoisOK && ! ComputingRisTransfer.IsInverseAndRoisOK ) {
                                        // give priority to existing matrix
//  ShowMessage ( "Clearing-up the existing ROIs...", ComputingRisTitle, ShowMessageWarning );

    RoisFile    ->SetText ( "" );
    ApplyRois   ->SetCheck ( BoolToCheck ( false ) );

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

    XyzFile         ->SetText ( "" );
    SpatialFilter   ->SetCheck ( BoolToCheck ( false ) );

    ComputingRisTransfer.IsXyzOK         = false;
    ComputingRisTransfer.IsXyzAndEegOK   = false;
    }


                                        // in case user dropped XYZ file which is incompatible
if ( ComputingRisTransfer.IsXyzOK && ComputingRisTransfer.IsInverseOK && ! ComputingRisTransfer.IsInverseAndXyzOK ) {
                                        // give priority to existing matrix
//  ShowMessage ( "Clearing-up the existing Electrodes Coordinates...", ComputingRisTitle, ShowMessageWarning );

    XyzFile         ->SetText ( "" );
    SpatialFilter   ->SetCheck ( BoolToCheck ( false ) );

    ComputingRisTransfer.IsXyzOK             = false;
    ComputingRisTransfer.IsInverseAndXyzOK   = false;
    }


XyzFile->ResetCaret;
}


void    TComputingRisDialog::CmXyzEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( SpatialFilter->GetCheck () )
          && EegPresets->GetSelIndex () != ComputingRisPresetFreq );
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::CmRegularizationEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( RegFixed->GetCheck () ) );
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

    RoisFile    ->SetText ( "" );
    ApplyRois   ->SetCheck ( BoolToCheck ( false ) );

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
tce.Enable ( CheckToBool ( ApplyRois->GetCheck() ) );
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::CmEnvelopeEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! CheckToBool ( VectorData->GetCheck () ) );
}


void    TComputingRisDialog::CmEnvelopeLowFreqEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! CheckToBool ( VectorData->GetCheck () ) && CheckToBool ( Envelope->GetCheck () ) );
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
tce.Enable ( CheckToBool ( TimeZScore->GetCheck () ) );
}


void    TComputingRisDialog::CmZScoreChange ()
{
if ( ! CheckToBool ( TimeZScore->GetCheck() ) )
    return;

if ( CheckToBool ( ComputeZScore->GetCheck() ) )
    SavingZScoreFactors->SetCheck ( BoolToCheck ( true  ) );

if ( CheckToBool ( LoadZScoreFile->GetCheck() ) )
    SavingZScoreFactors->SetCheck ( BoolToCheck ( false ) );
}


void    TComputingRisDialog::CmThresholdEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( Rank->GetCheck () ) );
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
                                        // Clusters can have any numbers now, as long as all groups are equal
    #define             MaxRisClusters  1000
    TGoGoF              gogof ( MaxRisClusters );
    TSelection          selcl ( MaxRisClusters, OrderSorted );
    TStringGrep         grepcl;
    TStrings            matched;
    int                 numclusters;

                                        // scan all files, dispatch them by cluster numbers, which can be totally arbitrary
    for ( int i = 0; i < (int) gofeeg; i++ ) {
                               // named group, used to isolate the cluster number
        grepcl.Set   ( InfixClusterGrep, GrepOptionDefaultFiles );

        if ( grepcl.Matched ( gofeeg ( i ), &matched ) && matched.NumStrings () >= 2 ) {

            int     clusteri    = StringToInteger ( matched ( 3 ) );

            selcl.Set ( clusteri );

            gogof[ clusteri ].Add ( gofeeg ( i ) );
            
//            DBGV ( clusteri, matched ( 0 ) );
            }
        else
            ShowMessage ( "File doesn't seem to be a proper Cluster file," NewLine 
                          "skipping it...", gofeeg ( i ), ShowMessageWarning );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check new groups
    int             maxfilespercluster  = gogof.GetMaxFiles ();

    for ( TIteratorSelectedForward cli ( selcl ); (bool) cli; ++cli ) {

//        gogof[ cli() ].Show ( IntegerToString ( cli() ) );

                                        // Check all groups have the same number of files among themselves
        if ( gogof[ cli() ].NumFiles () != maxfilespercluster ) {
            char        buff[ 256 ];
            StringCopy  ( buff, "Cluster ", IntegerToString ( cli() ), " is missing some files..." NewLine "Check you didn't forget some files, or put some outliers..." );
            ShowMessage ( buff, ComputingRisTitle, ShowMessageWarning );
            return;
            }

                                        // check if groups already exist, and if we find duplicate files
        if ( cli.GetIndex () < GoGoF.NumGroups () && GoGoF[ cli.GetIndex () ].ContainsAny ( gogof[ cli() ] ) ) {
            ShowMessage ( "It seems you are trying to add already existing files!" NewLine 
                          "Please check again your files...", ComputingRisTitle, ShowMessageWarning );
            return;
            }
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    numclusters     = selcl.NumSet ();


    if ( GoGoF.IsNotEmpty () && numclusters != GoGoF.NumGroups () ) {
        ShowMessage ( "It seems you are trying to add a different number of clusters!" NewLine 
                      "Please check again your files...", ComputingRisTitle, ShowMessageWarning );
        return;
        }

                                        // try to be smart: if user is dropping a set of files with the same # of conditions, that most probably mean adding more subjects to existing groups...
    bool        updatingtogroups    = GoGoF.IsNotEmpty (); // numclusters == GoGoF.NumGroups ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    for ( TIteratorSelectedForward cli ( selcl ); (bool) cli; ++cli ) {
                                        // updating an existing GoGoF could be wrong, if the same number of groups is used, but they actually differ
        if ( updatingtogroups )     GoGoF[ cli.GetIndex () ].Add ( gogof[ cli() ] );
        else                        AddEegGroup                  ( gogof[ cli() ] );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // "manually" updating the groups, bypassing checks for the moment
    if ( updatingtogroups ) {

        UpdateSubjectsConditions ();

        UpdateGroupSummary ();
        }

    } // areclusters

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( areepochs ) {
                                // force change file organization
//  if ( ! CRISPresets[ EegPresets->GetSelIndex () ].IsEpochs () ) {
//      EegPresets  ->SetSelIndex ( ComputingRisPresetErpIndivEpochs  );
//      GroupPresets->SetSelIndex ( GroupsLayout1SubjAllCondAllEpochs );
//      EvEegPresetsChange ();
//      }

    AddEegGroup ( gofeeg );
    } // areepochs

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else                            // all group at once
    AddEegGroup ( gofeeg );


InverseFile->ResetCaret;
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::AddEegGroup ( const TGoF& gofeeg )
{
                                        // We enforce stronger compatibilities whthin groups, allowing only identical number of subjects or conditions
                                        // Checked are done before CheckEeg, which otherwise will find subgroups by itself
if ( (bool) GoGoF ) {

    GroupsLayoutEnum        grouplayout     = (GroupsLayoutEnum) GroupPresets->GetSelIndex ();

//                                      // the only preset that can have different number of conditions - might turn this feature off, though
//  if      ( grouplayout == ComputingRisPresetErpGroupMeans )
//      ;
//  else
                                        // epochs are a bit tricky, it needs some massaging to retrieve the number of conditions as there can be more files than conds
    if      ( grouplayout == GroupsLayout1SubjAllCondAllEpochs ) {

        TGoGoF              splitgogof;
                                      // Default to split by our epoch name
        gofeeg    .SplitByNames ( "." InfixEpoch, splitgogof );

        int                 numconds        = (int) splitgogof;

        if ( numconds != NumConditions )   {

            ShowMessage ( "Not the same number of conditions!", ComputingRisTitle, ShowMessageWarning );

            return;
            }
        }

    else if ( gofeeg.NumFiles () != GoGoF[ 0 ].NumFiles () ) {

        ShowMessage ( grouplayout == GroupsLayout1SubjAllCond   ? "Not the same number of conditions!" 
                                                                : "Not the same number of subjects!", ComputingRisTitle, ShowMessageWarning );

        return;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We have new values now!
UpdateSubjectsConditions ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TransferData ( tdGetData );

                                        // temporarily adding this GoF
GoGoF.Add ( &gofeeg, true, MaxPathShort );

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
    InverseFile->SetText ( "" );
    ComputingRisTransfer.IsInverseOK = false;
    }

                                        // in case user dropped EEG files which are incompatible, clear-up matrix?
if ( ComputingRisTransfer.IsEegOK && ComputingRisTransfer.IsXyzOK && ! ComputingRisTransfer.IsXyzAndEegOK ) {
                                        // or asking? or silent?
//  ShowMessage ( "Clearing-up the existing Electrodes Coordinates...", ComputingRisTitle, ShowMessageWarning );

    XyzFile         ->SetText ( "" );
    SpatialFilter   ->SetCheck ( BoolToCheck ( false ) );

    ComputingRisTransfer.IsXyzOK     = false;
    }

                                        // off if the inverse has been cleared up, and the EEG files given priority
if ( ComputingRisTransfer.IsEegOK /*&& ComputingRisTransfer.IsInverseAndEegOK*/ ) {

    //SetBaseFilename ();

    UpdateSubjectsConditions ();

    UpdateGroupSummary ();
    }
else {
    GoGoF.RemoveLastGroup ();

    UpdateSubjectsConditions ();
    }


InverseFile->ResetCaret;
}


//----------------------------------------------------------------------------
                                        // Until better solution, we kind of sync the EEG list with the Inverse one
void    TComputingRisDialog::CmRemoveEegGroup ()
{
if ( ! GoGoF.RemoveLastGroup () )
    return;


if  ( (bool) Inverses ) {

    if      ( AreIndividualInverses () && Is1Group1Subject   () )
                                        // removed a subject -> remove last inverse
        Inverses.RemoveLast ();

    else if ( Is1Group1Condition () && GoGoF.IsEmpty () )
                                        // removed last conditions (and therefor subjects, too) -> remove all inverses
        Inverses.Reset ();


    if ( NumInverses () == 0 )
        ComputingRisTransfer.IsInverseOK = false;
    }


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
if ( Inverses.IsEmpty () )
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


if      ( NumInverses () == 0       )                       ClearString ( InverseFile );
else if ( ! IsInverseOK             )                       StringCopy  ( InverseFile, "<Inverses Not OK!>" );
//else if ( IsEegOK && IsInverseOK && ! IsInverseAndEegOK )   ClearString ( InverseFile );
else if ( IsTemplateInverse ()      )                       StringCopy  ( InverseFile, Inverses[ 0 ] );
else if ( AreIndividualInverses ()  )                       StringCopy  ( InverseFile, "<Subjects Inverses>" );
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

    TGoF*               copylastgof     = new TGoF ( GoGoF.GetLast () );

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

    TGoF*               copylastgof     = new TGoF ( GoGoF.GetLast () );

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
bool                acrossconditions = NumSubjects > 1;


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
if ( GroupPresets->GetSelIndex () == GroupsLayout1SubjAllCondAllEpochs ) {

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
GroupsLayoutEnum        grouplayout     = (GroupsLayoutEnum)        GroupPresets->GetSelIndex ();
TFileName               buff;
int                     maxlength       = 0;


for ( int gofi = 0; gofi < (int) GoGoF; gofi++ ) {

    TGoF&       gof     = GoGoF[ gofi ];

                                        // being more explicit on the groups' label
    if      ( grouplayout == GroupsLayoutAllSubj1Cond          )    StringCopy      ( buff, CRISPresets[ esicase ].IsClusters ()                            ? "Cluster" : "Condition" ); // although equivalent, we can be more specific
    else if ( grouplayout == GroupsLayout1SubjAllCond          )    StringCopy      ( buff, CRISPresets[ esicase ].Code == ComputingRisPresetErpGroupMeans  ? "Group"   : "Subject"   ); // special case for Grand Average
    else if ( grouplayout == GroupsLayout1SubjAllCondAllEpochs )    StringCopy      ( buff, "Subject" );
    else                                                            StringCopy      ( buff, "Group"   );
    StringAppend    ( buff, " ",  IntegerToString ( gofi + 1, NumIntegerDigits ( (int) GoGoF ) ), ":" );

    StringAppend    ( buff, "  ", IntegerToString ( gof.NumFiles () ), " " );

    if      ( grouplayout == GroupsLayoutAllSubj1Cond          )    StringAppend    ( buff, "Subject"    );
    else if ( grouplayout == GroupsLayout1SubjAllCond          )    StringAppend    ( buff, CRISPresets[ esicase ].IsClusters ()                            ? "Cluster" : "Condition" ); // although equivalent, we can be more specific
    else if ( grouplayout == GroupsLayout1SubjAllCondAllEpochs )    StringAppend    ( buff, "Epoch File" );
    else                                                            StringAppend    ( buff, "File"       );
    StringAppend    ( buff, StringPlural ( gof.NumFiles () ) );

    if ( gof.NumFiles () == 1 )     StringAppend    ( buff, "  ( ", ToFileName ( gof.GetFirst () ),                                        " )" );
    else                            StringAppend    ( buff, "  ( ", ToFileName ( gof.GetFirst () ), " .. ", ToFileName ( gof.GetLast () ), " )" );


    if ( AreIndividualInverses () ) {

        if      ( Is1Group1Condition () ) {
            StringAppend    ( buff, " + <Subjects Inverses>" );
            //StringAppend    ( buff, " + ", ToFileName ( Inverses.GetFirst () ) );
            //if ( NumInverses () > 1 )
            //    StringAppend    ( buff, " .. ", ToFileName ( Inverses.GetLast () ) );
            }
        else if ( Is1Group1Subject () )
            StringAppend    ( buff, " + ", ToFileName ( Inverses[ gofi ] ) );    // we can be more precise
        }


    GroupsSummary->InsertString ( buff, 0 );

    Maxed ( maxlength, (int) StringLength ( buff ) );
    }


UpdateHorizontalScroller ( GroupsSummary, maxlength );


NumSubjectsEdit  ->SetIntValue ( NumSubjects    );
NumConditionsEdit->SetIntValue ( NumConditions  );
NumInversesEdit  ->SetIntValue ( NumInverses () );

//SetBaseFilename ();
}


//----------------------------------------------------------------------------
                                        // Updating the number of subjects & conditions - we now enforce strong consistency, every added group should be compatible with all the others
void    TComputingRisDialog::UpdateSubjectsConditions ( bool updategroups )
{
ComputingRisPresetsEnum esicase         = (ComputingRisPresetsEnum) EegPresets  ->GetSelIndex ();
GroupsLayoutEnum        grouplayout     = (GroupsLayoutEnum)        GroupPresets->GetSelIndex ();


if ( GoGoF.IsEmpty () ) {
    NumSubjects     = 0;
    NumConditions   = 0;
    }
else {
    if      ( grouplayout == GroupsLayout1SubjAllCondAllEpochs ) {

        NumSubjects     = GoGoF.NumGroups ();

        TGoGoF              splitgogof0;
                                        // Default to split by our epoch name
        GoGoF[ 0 ].SplitByNames ( "." InfixEpoch, splitgogof0 );    // maybe we can store that somewhere, like NumConds

        NumConditions   = (int) splitgogof0;
        }

    else if ( grouplayout == GroupsLayout1SubjAllCond ) {

        NumSubjects     = GoGoF     .NumGroups ();
        NumConditions   = GoGoF[ 0 ].NumFiles  ();
        }

    else if ( grouplayout == GroupsLayoutAllSubj1Cond ) {

        NumSubjects     = GoGoF[ 0 ].NumFiles  ();
        NumConditions   = GoGoF     .NumGroups ();
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Updating dialog
NumSubjectsEdit  ->SetIntValue ( NumSubjects    );
NumConditionsEdit->SetIntValue ( NumConditions  );
NumInversesEdit  ->SetIntValue ( NumInverses () );


if      ( NumInverses () == 0 )                 InverseFile->SetText ( "" );
else if ( ! ComputingRisTransfer.IsInverseOK )  InverseFile->SetText ( "<Inverses Not OK!>" );
else if ( IsTemplateInverse () )                InverseFile->SetText ( Inverses[ 0 ] );
else if ( AreIndividualInverses () )            InverseFile->SetText ( "<Subjects Inverses>" );

InverseFile->ResetCaret;


if ( updategroups )
                                        // Setting Group Averages if more than 1 subject for a few presets
    ComputeGroupsAverages->SetCheck ( BoolToCheck ( (    esicase == ComputingRisPresetErpIndivMeans 
                                                      || esicase == ComputingRisPresetErpIndivEpochs
                                                      || esicase == ComputingRisPresetIndIndivEpochs )
                                                    && NumSubjects > 1                                 ) );
}


//----------------------------------------------------------------------------
void    TComputingRisDialog::CmOkEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );


if ( ! ( IsTemplateInverse () || AreIndividualInverses () )
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

GroupsLayoutEnum    grouplayout         = (GroupsLayoutEnum) GroupPresets->GetSelIndex ();

int                 numsubjects         = NumSubjects;
int                 numconditions       = NumConditions;
int                 numinverses         = NumInverses ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We can have either 1 inverse matrix per subject, or 1 matrix for all subjects
bool                individualinverses  = AreIndividualInverses ();
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
            StringAppend ( buff, Tab, ToFileName ( Is1Group1Subject () ? GoGoF[ subji ][ fi ] : GoGoF[ fi ][ subji ] ), NewLine );

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
        xyzfile.Reset ();
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
        roifile.Reset ();
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
                        grouplayout,            NumSubjects,            NumConditions,

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
