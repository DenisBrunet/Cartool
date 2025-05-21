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

#if defined(CHECKASSERT)
#include    <assert.h>
#endif

#include    "TCreateInverseMatricesDialog.h"

#include    "Strings.Utils.h"
#include    "Strings.TFixedString.h"
#include    "Files.Extensions.h"
#include    "Files.TSplitLinkManyFile.h"
#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"
#include    "TExportTracks.h"
#include    "Geometry.TPoint.h"
#include    "TVolume.h"
#include    "Volumes.SagittalTransversePlanes.h"
#include    "Volumes.AnalyzeNifti.h"
#include    "GlobalOptimize.Tracks.h"
#include    "GlobalOptimize.Points.h"
#include    "GlobalOptimize.Volumes.h"
#include    "TCoregistrationDialog.h"
#include    "Math.Resampling.h"

#include    "TVolumeDoc.h"
#include    "TLeadField.h"
#include    "TInverseMatrixDoc.h"

#include    "TElectrodesView.h"
#include    "TVolumeView.h"

#include    "ESI.TissuesThicknesses.h"
#include    "ESI.SolutionPoints.h"
#include    "ESI.HeadSphericalModel.h"
#include    "ESI.LeadFields.h"
#include    "ESI.TissuesConductivities.h"
#include    "ESI.InverseModels.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Global transfer buffer definition
TCreateInverseMatricesStruct    CrisTransfer;


//----------------------------------------------------------------------------

const char  CrisMriPresetsString[ NumCrisMriPresets ][ 64 ] =
            {
            "Skip MRI Preprocessing",
            "",
            "Real Head MRI / Preprocessing MRIs",
            "Real Head MRI / Already Preprocessed MRIs",
            "",
            "Template MRI  / Preprocessing MRIs",
            "Template MRI  / Already Preprocessed MRIs",
            };


const char  CrisXyzPresetsString[ NumCrisXyzPresets ][ 128 ] =
            {
            "Skip Electrodes Preprocessing",
            "",
            "Interactive Coregistration    / Coregistering from scratch",
            "Interactive Coregistration    / Refining an existing coregistration",
            "",
            "Coregistration already done / Just load the file already",
            };


const char  CrisSPPresetsString[ NumCrisSPPresets ][ 64 ] =
            {
            "Skip Solution Space Processing",
            "",
            "Computing the Solution Points / Adult",
            "Computing the Solution Points / Children  (above 9 months)",
            "Computing the Solution Points / Newborn (below 9 months)",
            "",
            "Reloading an existing Solution Points file",
            "Using the Solution Points from the reloaded Lead Field below",
            };


const OneLeadFieldPreset    AllLeadFieldPresets[ NumCrisLFPPresets ] =
            {
            {   CrisLFPresetNothing,                    "Skip Lead Field Processing",                                           "",                         NoLeadField,                                                                                                     0,  0,      0       },
                                                                                                                                                                                                                                                                                 
            {   CrisLFPresetSeparator1,                 "",                                                                     "",                         NoLeadField,                                                                                                     0,  0,      0       },
            {   CrisLFPresetLSMAC3ShellAry,             "LSMAC / 3-Shell / Ary Approximation (obsolete)",                       "Ary Lead Field",           (LeadFieldFlag) ( GeometryLSMAC | Human   | IsotropicAry                  | SkullRadiusGiven        ),           3,  0,      0       },  // 3-Shell Ary approximation - for backward compatibility

            {   CrisLFPresetSeparator2,                 "",                                                                     "",                         NoLeadField,                                                                                                     0,  0,      0       },
            {   CrisLFPresetLSMAC3ShellExact,           "LSMAC / 3-Shell / Exact Equations",                                    "3-Shell Lead Field",       (LeadFieldFlag) ( GeometryLSMAC | Human   | IsotropicNShellSpherical      | SkullRadiusGiven        ),           3,  0,      0       },  // 3-Shell model
            {   CrisLFPresetLSMAC4ShellExact,           "LSMAC / 4-Shell / Exact Equations",                                    "4-Shell Lead Field",       (LeadFieldFlag) ( GeometryLSMAC | Human   | IsotropicNShellSpherical      | SkullRadiusGiven        ),           4,  0,      0       },  // 4-Shell model
            {   CrisLFPresetLSMAC6ShellExact,           "LSMAC / 6-Shell / Exact Equations",                                    "6-Shell Lead Field",       (LeadFieldFlag) ( GeometryLSMAC | Human   | IsotropicNShellSpherical      | SkullRadiusGiven        ),           6,  0,      0       },  // 6-Shell model

//          {   CrisLFPresetSeparator3,                 "",                                                                     "",                         NoLeadField,                                                                                                     0,  0,      0       },
//          {   CrisLFPresetLSMACMonkey,                "LSMAC / 3-Shell / Ary Approx. / Macaque Monkey",                       "Monkey Lead Field",        (LeadFieldFlag) ( GeometryLSMAC | Macaque | IsotropicAry                  | SkullRadiusFixedRatio   ),           3,  0.780,  0.680   },  // scalp vary, skull thickness follows - Enzo 2012, approximately from some measures
//          {   CrisLFPresetLSMACMouse,                 "LSMAC / 3-Shell / Ary Approx. / Mouse",                                "Mouse Lead Field",         (LeadFieldFlag) ( GeometryLSMAC | Mouse   | IsotropicAry                  | SkullRadiusFixedRatio   ),           3,  1.00,   0.925   },  // scalp vary, skull thickness follows - !Scalp has been removed, the outer radius is scalp = skull = 1!

            {   CrisLFPresetSeparator4,                 "",                                                                     "",                         NoLeadField,                                                                                                     0,  0,      0       },
            {   CrisLFPresetReload,                     "Reloading an existing Lead Field & Solution Points files",             "Reloading Lead Field",     NoLeadField,                                                                                                     0,  0,      0       }
            };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TCreateInverseMatricesStruct::TCreateInverseMatricesStruct ()
{
MriPresets.Clear ();
for ( int i = 0; i < NumCrisMriPresets; i++ )
    MriPresets.AddString ( CrisMriPresetsString[ i ], i == 0 );

ClearString ( HeadMriFile  );
ClearString ( BrainMriFile );
ClearString ( GreyMriFile  );
ClearString ( TissuesFile  );

BiasField           = BoolToCheck ( false );
SmoothMri           = BoolToCheck ( false );
ReorientMris        = BoolToCheck ( false );
SagittalPlaneMri    = BoolToCheck ( false );
EquatorPlaneMri     = BoolToCheck ( false );


XyzPresets.Clear ();
for ( int i = 0; i < NumCrisXyzPresets; i++ )
    XyzPresets.AddString ( CrisXyzPresetsString[ i ], i == 0 );

ClearString ( XyzFile );


SpPresets.Clear ();
for ( int i = 0; i < NumCrisSPPresets; i++ )
    SpPresets.AddString ( CrisSPPresetsString[ i ], i == 0 );

IntegerToString ( NumSolPoints,      DefaultNumSolutionPoints );
GreyFat             = BoolToCheck (  true );
GreyAll             = BoolToCheck ( false );
ClearString ( LoadSolPointsFile );


LeadFieldPresets.Clear ();
for ( int i = 0; i < NumCrisLFPPresets; i++ )
    LeadFieldPresets.AddString ( AllLeadFieldPresets[ i ].Text, i == 0 );

ThicknessFromMri    = BoolToCheck (  true );
ThicknessFromTissues= BoolToCheck ( false );

ClearString ( LeadFieldTargetAge );
ClearString ( SkullConductivity );
AdjustSkullMeanThickness= BoolToCheck (  true );
ClearString ( SkullMeanThickness );

ClearString ( LeadFieldFile );
ClearString ( LeadFieldSolPointsFile );

double              age             = DefaultInverseAge;
double              skullcond       = AgeToSkullConductivity ( age );
double              skullthickness  = AgeToSkullThickness    ( age );

FloatToString ( LeadFieldTargetAge, age,            1 );
FloatToString ( SkullConductivity,  skullcond,      4 );
FloatToString ( SkullMeanThickness, skullthickness, 2 );

InverseMN           = BoolToCheck ( false );
InverseWMN          = BoolToCheck ( false );
InverseDale         = BoolToCheck ( false );
InverseSLoreta      = BoolToCheck ( false );
InverseLoreta       = BoolToCheck ( false );
InverseLaura        = BoolToCheck ( false );
InverseELoreta      = BoolToCheck ( false );

TFileName           cp;
cp.GetCurrentDir ();
StringAppend ( cp, "\\InverseSolutions" );
StringCopy ( BaseFileName, cp );

Interactive            = BoolToCheck (  true );
Automatic              = BoolToCheck ( false );
}


DEFINE_RESPONSE_TABLE1 ( TCreateInverseMatricesDialog, TBaseDialog )
    EV_WM_DROPFILES,

    EV_COMMAND                  ( IDC_UPDIRECTORY,              CmUpOneDirectory ),
    EV_COMMAND                  ( IDC_BROWSEBASEFILENAME,       CmBrowseBaseFileName ),

    EV_CBN_SELCHANGE            ( IDC_MRIPRESETS,               EvMriPresetsChange ),
    EV_CBN_SELCHANGE            ( IDC_SPPRESETS,                EvSpPresetsChange ),
    EV_CBN_SELCHANGE            ( IDC_LEADFIELDPRESETS,         EvLeadFieldPresetsChange ),

    EV_COMMAND                  ( IDOK,                         CmOk ),
    EV_COMMAND_ENABLE           ( IDOK,                         CmOkEnable ),

    EV_COMMAND_AND_ID           ( IDC_BROWSEMRIFILE1,           CmBrowseMriFile ),
    EV_COMMAND_AND_ID           ( IDC_BROWSEMRIFILE2,           CmBrowseMriFile ),
    EV_COMMAND_AND_ID           ( IDC_BROWSEMRIFILE3,           CmBrowseMriFile ),
    EV_COMMAND_ENABLE           ( IDC_SETSAGITTALPLANEMRI,      CmNormalizeEnable ),
    EV_COMMAND_ENABLE           ( IDC_SETEQUATORPLANEMRI,       CmNormalizeEnable ),
    EV_COMMAND_ENABLE           ( IDC_SMOOTHMRI,                CmMrisEnable ),
    EV_COMMAND_ENABLE           ( IDC_REORIENTMRIS,             CmMrisEnable ),
    EV_COMMAND_ENABLE           ( IDC_BIASFIELD,                CmBrainMriEnable ),

    EV_COMMAND_ENABLE           ( IDC_XYZFILE,                  CmXyzEnable ),
    EV_COMMAND                  ( IDC_BROWSEXYZFILE,            CmBrowseXyzFile ),
    EV_COMMAND_ENABLE           ( IDC_BROWSEXYZFILE,            CmXyzEnable ),

    EV_COMMAND_ENABLE           ( IDC_NUMSOLPOINTS,             CmNumSPEnable ),
    EV_COMMAND_ENABLE           ( IDC_GREYFAT,                  CmSPComputeEnable ),
    EV_COMMAND_ENABLE           ( IDC_GREYALL,                  CmSPComputeEnable ),
    EV_COMMAND_ENABLE           ( IDC_OUTPUTSPFILE,             CmSPLoadEnable ),
    EV_COMMAND_AND_ID           ( IDC_BROWSEINPUTSPFILE,        CmBrowseSolPointsFile ),
    EV_COMMAND_AND_ID           ( IDC_BROWSEOUTPUTSPFILE,       CmBrowseSolPointsFile ),
    EV_COMMAND_ENABLE           ( IDC_BROWSEINPUTSPFILE,        CmLoadLeadFieldEnable ),
    EV_COMMAND_ENABLE           ( IDC_BROWSEOUTPUTSPFILE,       CmSPLoadEnable ),

    EV_COMMAND_ENABLE           ( IDC_THICKNESSFROMMRI,         CmComputeLeadFieldEnable ),
    EV_COMMAND_ENABLE           ( IDC_THICKNESSFROMTISSUES,     CmComputeLeadFieldEnable ),
    EV_COMMAND                  ( IDC_BROWSETISSUESFILE,        CmBrowseTissuesFile ),
    EV_COMMAND_ENABLE           ( IDC_LEAFIELDTARGETAGE,        CmComputeLeadFieldEnable ),
    EV_COMMAND_ENABLE           ( IDC_SKULLCONDUCTIVITY,        CmComputeLeadFieldEnable ),
    EV_COMMAND_ENABLE           ( IDC_ADJUSTSKULLMEANTHICKNESS, CmComputeLeadFieldEnable ),
    EV_COMMAND_ENABLE           ( IDC_SKULLMEANTHICKNESS,       CmAdjustSkullThicknessEnable ),
    EV_EN_CHANGE                ( IDC_LEAFIELDTARGETAGE,        EvAgeChange ),
    EV_COMMAND                  ( IDC_BROWSELEADFIELDFILE,      CmBrowseLeadFieldFile ),
    EV_COMMAND_ENABLE           ( IDC_LEADFIELDFILE,            CmLoadLeadFieldEnable ),
    EV_COMMAND_ENABLE           ( IDC_BROWSELEADFIELDFILE,      CmLoadLeadFieldEnable ),
    EV_COMMAND_ENABLE           ( IDC_INPUTSPFILE,              CmLoadLeadFieldEnable ),

    EV_COMMAND                  ( IDC_INVERSEALL,               CmInverseAllNone ),
    EV_COMMAND_ENABLE           ( IDC_INVERSEMN,                CmInverseMatricesEnable ),
    EV_COMMAND_ENABLE           ( IDC_INVERSEWMN,               CmInverseMatricesEnable ),
    EV_COMMAND_ENABLE           ( IDC_INVERSEDALE,              CmInverseMatricesEnable ),
    EV_COMMAND_ENABLE           ( IDC_INVERSESLORETA,           CmInverseMatricesEnable ),
    EV_COMMAND_ENABLE           ( IDC_INVERSELORETA,            CmInverseMatricesEnable ),
    EV_COMMAND_ENABLE           ( IDC_INVERSELAURA,             CmInverseMatricesEnable ),
    EV_COMMAND_ENABLE           ( IDC_INVERSEELORETA,           CmInverseMatricesEnable ),
    EV_COMMAND_ENABLE           ( IDC_INVERSEALL,               CmInverseMatricesEnable ),

END_RESPONSE_TABLE;


        TCreateInverseMatricesDialog::TCreateInverseMatricesDialog ( TWindow* parent, TResId resId )
      : TBaseDialog ( parent, resId )
{
LeadFieldAndSolutionPointsMatch = true;
LeadFieldAndXyzMatch            = true;


MriPresets              = new TComboBox ( this, IDC_MRIPRESETS );
HeadMriFile             = new TEdit ( this, IDC_MRIFILE1, EditSizeText );
BrainMriFile            = new TEdit ( this, IDC_MRIFILE2, EditSizeText );
GreyMriFile             = new TEdit ( this, IDC_MRIFILE3, EditSizeText );
TissuesFile             = new TEdit ( this, IDC_TISSUESFILE, EditSizeText );
BiasField               = new TCheckBox ( this, IDC_BIASFIELD );
SmoothMri               = new TCheckBox ( this, IDC_SMOOTHMRI );
ReorientMris            = new TCheckBox ( this, IDC_REORIENTMRIS );
SagittalPlaneMri        = new TCheckBox ( this, IDC_SETSAGITTALPLANEMRI );
EquatorPlaneMri         = new TCheckBox ( this, IDC_SETEQUATORPLANEMRI );

XyzPresets              = new TComboBox ( this, IDC_XYZPRESETS );
XyzFile                 = new TEdit ( this, IDC_XYZFILE, EditSizeText );

SpPresets               = new TComboBox ( this, IDC_SPPRESETS );
NumSolPoints            = new TEdit ( this, IDC_NUMSOLPOINTS, EditSizeValue );
NumSolPoints->SetValidator ( new TFilterValidator ( ValidatorPositiveFloat ) );     // allowing for either number of resolution
GreyFat                 = new TRadioButton ( this, IDC_GREYFAT );
GreyAll                 = new TRadioButton ( this, IDC_GREYALL );
LoadSolPointsFile       = new TEdit ( this, IDC_OUTPUTSPFILE, EditSizeText );

LeadFieldPresets        = new TComboBox ( this, IDC_LEADFIELDPRESETS );
ThicknessFromMri        = new TRadioButton ( this, IDC_THICKNESSFROMMRI );
ThicknessFromTissues    = new TRadioButton ( this, IDC_THICKNESSFROMTISSUES );
LeadFieldTargetAge      = new TEdit ( this, IDC_LEAFIELDTARGETAGE, EditSizeValue );
SkullConductivity       = new TEdit ( this, IDC_SKULLCONDUCTIVITY, EditSizeValue );
AdjustSkullMeanThickness= new TCheckBox ( this, IDC_ADJUSTSKULLMEANTHICKNESS );
SkullMeanThickness      = new TEdit ( this, IDC_SKULLMEANTHICKNESS, EditSizeValue );
LeadFieldFile           = new TEdit ( this, IDC_LEADFIELDFILE, EditSizeText );
LeadFieldSolPointsFile  = new TEdit ( this, IDC_INPUTSPFILE, EditSizeText );

InverseMN               = new TCheckBox ( this, IDC_INVERSEMN );
InverseWMN              = new TCheckBox ( this, IDC_INVERSEWMN );
InverseDale             = new TCheckBox ( this, IDC_INVERSEDALE );
InverseSLoreta          = new TCheckBox ( this, IDC_INVERSESLORETA );
InverseLoreta           = new TCheckBox ( this, IDC_INVERSELORETA );
InverseLaura            = new TCheckBox ( this, IDC_INVERSELAURA );
InverseELoreta          = new TCheckBox ( this, IDC_INVERSEELORETA );

BaseFileName            = new TEdit ( this, IDC_BASEFILENAME, EditSizeText );
Interactive             = new TRadioButton ( this, IDC_INTERACTIVE );
Automatic               = new TRadioButton ( this, IDC_AUTOMATIC );


SetTransferBuffer ( &CrisTransfer );


StringCopy ( BatchFilesExt, AllMriFilesExt );
}


        TCreateInverseMatricesDialog::~TCreateInverseMatricesDialog ()
{
delete  MriPresets;
delete  HeadMriFile;            delete  BrainMriFile;           delete  GreyMriFile;
delete  TissuesFile;
delete  BiasField;              delete  SmoothMri;
delete  ReorientMris;           delete  SagittalPlaneMri;       delete  EquatorPlaneMri;
delete  XyzPresets;
delete  XyzFile;
delete  SpPresets;
delete  NumSolPoints;
delete  GreyFat;                delete  GreyAll;
delete  LoadSolPointsFile;
delete  LeadFieldPresets;
delete  ThicknessFromMri;       delete  ThicknessFromTissues;
delete  LeadFieldTargetAge;     delete  SkullConductivity;
delete  AdjustSkullMeanThickness;delete SkullMeanThickness;
delete  LeadFieldFile;          delete  LeadFieldSolPointsFile;
delete  InverseMN;              delete  InverseWMN;             delete  InverseDale;            delete  InverseSLoreta;         
delete  InverseLoreta;          delete  InverseLaura;           delete  InverseELoreta;
delete  BaseFileName;
delete  Interactive;            delete  Automatic;
}


//----------------------------------------------------------------------------
                                        // position each edit field to the right end, so we can see the filename in priority, not the path
void    TCreateInverseMatricesDialog::ResetCarets ()
{
HeadMriFile             ->ResetCaret;
BrainMriFile            ->ResetCaret;
GreyMriFile             ->ResetCaret;
XyzFile                 ->ResetCaret;
LoadSolPointsFile       ->ResetCaret;
TissuesFile             ->ResetCaret;
LeadFieldFile           ->ResetCaret;
LeadFieldSolPointsFile  ->ResetCaret;
BaseFileName            ->ResetCaret;
}


//----------------------------------------------------------------------------
void    TCreateInverseMatricesDialog::EvMriPresetsChange ()
{
if ( StringIsEmpty ( CrisMriPresetsString[ MriPresets->GetSelIndex() ] ) )
    return;

                                        // reset all options
BiasField             ->SetCheck ( BoolToCheck ( false ) );
SmoothMri             ->SetCheck ( BoolToCheck ( false ) );
ReorientMris          ->SetCheck ( BoolToCheck ( false ) );
SagittalPlaneMri      ->SetCheck ( BoolToCheck ( false ) );
EquatorPlaneMri       ->SetCheck ( BoolToCheck ( false ) );

                                        // then set only the right ones
switch ( MriPresets->GetSelIndex() ) {

    case    CrisMriPresetSubjectReorientSMAC :
        BiasField            ->SetCheck ( BoolToCheck (  true ) );
        SmoothMri            ->SetCheck ( BoolToCheck (  true ) );
        ReorientMris         ->SetCheck ( BoolToCheck (  true ) );
        SagittalPlaneMri     ->SetCheck ( BoolToCheck (  true ) );
        EquatorPlaneMri      ->SetCheck ( BoolToCheck (  true ) );
        break;

    case    CrisMriPresetTemplateReorientSMAC :
//      BiasField            ->SetCheck ( BoolToCheck (  true ) );
        ReorientMris         ->SetCheck ( BoolToCheck (  true ) );
//      SagittalPlaneMri     ->SetCheck ( BoolToCheck (  true ) );
//      EquatorPlaneMri      ->SetCheck ( BoolToCheck (  true ) );
        break;


    case    CrisMriPresetSubjectSMAC :
//      BiasField            ->SetCheck ( BoolToCheck (  true ) );
        SmoothMri            ->SetCheck ( BoolToCheck (  true ) );
        break;

    case    CrisMriPresetTemplateSMAC :
//      BiasField            ->SetCheck ( BoolToCheck (  true ) );
        break;

    }
}


//----------------------------------------------------------------------------
void    TCreateInverseMatricesDialog::EvSpPresetsChange ()
{
if ( StringIsEmpty ( CrisSPPresetsString[ SpPresets->GetSelIndex() ] ) )
    return;


switch ( SpPresets->GetSelIndex() ) {

    case    CrisSPPresetComputeAdult    :
    case    CrisSPPresetComputeChildren :
        GreyFat              ->SetCheck ( BoolToCheck (  true ) );
        GreyAll              ->SetCheck ( BoolToCheck ( false ) );
        break;

    case    CrisSPPresetComputeNewborn  :
        GreyFat              ->SetCheck ( BoolToCheck ( false ) );
        GreyAll              ->SetCheck ( BoolToCheck (  true ) );
        break;

    case    CrisSPPresetFromLeadField :
                                        // force switch the Lead Field preset - however when quitting this mode, we are not switching back, because we don't know toward which preset?
        LeadFieldPresets->SetSelIndex ( CrisLFPresetReload );
        break;

    }
}


//----------------------------------------------------------------------------
void    TCreateInverseMatricesDialog::EvLeadFieldPresetsChange ()
{
if ( AllLeadFieldPresets[ LeadFieldPresets->GetSelIndex() ].IsUndefined () )
    return;

CrisLeadFieldPresets    preset          = (CrisLeadFieldPresets) LeadFieldPresets->GetSelIndex ();

                                        // ?updating these?
//ThicknessFromMri        ->SetCheck ( BoolToCheck ( false ) );
//ThicknessFromTissues    ->SetCheck ( BoolToCheck ( false ) );
//AdjustSkullMeanThickness->SetCheck ( BoolToCheck ( false ) );

                                        // and always set these:
InverseMN               ->SetCheck ( BoolToCheck (  true ) );
InverseWMN              ->SetCheck ( BoolToCheck (  true ) );
InverseDale             ->SetCheck ( BoolToCheck (  true ) );
InverseSLoreta          ->SetCheck ( BoolToCheck (  true ) );
InverseLoreta           ->SetCheck ( BoolToCheck (  true ) );
InverseLaura            ->SetCheck ( BoolToCheck (  true ) );
InverseELoreta          ->SetCheck ( BoolToCheck (  true ) );

                                        // then set only the right ones
switch ( preset ) {

    case    CrisLFPresetReload :

        break;

    default:
                                        // All the other presets that need the radii to be estimated, by whatever means
//      ThicknessFromMri    ->SetCheck ( BoolToCheck (  true ) );

//      NumSolPoints        ->SetIntValue ( DefaultNumSolutionPoints );

                                        // Update processing flag if some inconsistency has been spotted
        char                    buff[ EditSizeText ];
        TissuesFile->GetText ( buff, EditSizeText );

        if ( StringIsEmpty ( buff ) ) {
            ThicknessFromMri        ->SetCheck ( BoolToCheck (  true ) );
            ThicknessFromTissues    ->SetCheck ( BoolToCheck ( false ) );
            }

        break;
    }


CheckLeadFieldSolPointsFile ();
}


//----------------------------------------------------------------------------
void    TCreateInverseMatricesDialog::CmBrowseMriFile ( owlwparam w )
{
SetMriFile ( 0, w - IDC_BROWSEMRIFILE1 );
}


void    TCreateInverseMatricesDialog::SetMriFile ( const char* file, int mriindex )
{
static GetFileFromUser  getfile ( "MRI File", AllMriFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( mriindex == 0  ?   CrisTransfer.HeadMriFile 
                           : mriindex == 1  ?   CrisTransfer.BrainMriFile
                           :                    CrisTransfer.GreyMriFile  ) )
        return;

    TransferData ( tdSetData );
    }
else {
    StringCopy ( mriindex == 0  ?   CrisTransfer.HeadMriFile 
               : mriindex == 1  ?   CrisTransfer.BrainMriFile
               :                    CrisTransfer.GreyMriFile,   file );

    getfile.SetOnly ( file );
    }


TransferData ( tdSetData );

SetBaseFilename ();

ResetCarets ();
}


//----------------------------------------------------------------------------
void    TCreateInverseMatricesDialog::CmBrowseXyzFile ()
{
SetXyzFile ( 0 );
}


void    TCreateInverseMatricesDialog::SetXyzFile ( const char* file )
{
static GetFileFromUser  getfile ( "Electrodes Coordinates File", AllCoordinatesFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( CrisTransfer.XyzFile ) )
        return;

    TransferData ( tdSetData );
    }
else {
    StringCopy ( CrisTransfer.XyzFile, file );
    getfile.SetOnly ( file );
    }

                                        // switch mode?
if ( StringIsNotEmpty ( CrisTransfer.XyzFile ) && CrisTransfer.XyzPresets.GetSelIndex () == CrisXyzPresetNothing ) {
    CrisTransfer.XyzPresets.Select ( CrisXyzPresetCoregistrationDone );
    }


TransferData ( tdSetData );

SetBaseFilename ();

ResetCarets ();

CheckXyzFile ();
}


void    TCreateInverseMatricesDialog::CheckXyzFile ()
{
TransferData ( tdGetData );

LeadFieldAndXyzMatch    = true;

if ( ! ( CrisTransfer.HasXyz () && CrisTransfer.HasLeadFieldFile () ) )
    return;

                                        // We don't need the XYZ and reading a Lead Field at the same time,
                                        // but just in case the user wants a XYZ, we can check it is also consistent with the LF
TPoints             xyz ( CrisTransfer.XyzFile );
int                 xyznumel        = xyz.GetNumPoints ();
int                 lfnumel         = TLeadField ( CrisTransfer.LeadFieldFile ).GetNumElectrodes ();


if ( xyznumel != lfnumel ) {
    LeadFieldAndXyzMatch    = false;

    char                buff[ 256 ];

    StringCopy  ( buff, "Not the same amount of Electrodes:" NewLine 
                        NewLine
                        "  - Lead Field "      Tab "= ", IntegerToString ( lfnumel  ), NewLine 
                        "  - Electrodes file " Tab "= ", IntegerToString ( xyznumel ), NewLine 
                        NewLine 
                        "Please check again your files!" 
                );
    ShowMessage ( buff, CreatingInverseTitle, ShowMessageWarning, this );
    }
}


//----------------------------------------------------------------------------
void    TCreateInverseMatricesDialog::CmBrowseTissuesFile ()
{
SetTissuesFile ( 0 );
}


void    TCreateInverseMatricesDialog::SetTissuesFile ( const char* file )
{
static GetFileFromUser  getfile ( "MRI Segmented Tissues File", AllMriFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( CrisTransfer.TissuesFile ) )
        return;

    TransferData ( tdSetData );
    }
else {
    StringCopy ( CrisTransfer.TissuesFile, file );
    getfile.SetOnly ( file );
    }

                                        // switch mode?
if ( StringIsNotEmpty ( CrisTransfer.TissuesFile ) && CrisTransfer.IsThicknessFromMri () ) {
    CrisTransfer.ThicknessFromMri       = BoolToCheck ( false );
    CrisTransfer.ThicknessFromTissues   = BoolToCheck (  true );
    }


TransferData ( tdSetData );

SetBaseFilename ();

ResetCarets ();
}


//----------------------------------------------------------------------------
void    TCreateInverseMatricesDialog::CmBrowseLeadFieldFile ()
{
SetLeadFieldFile ( 0 );
}


void    TCreateInverseMatricesDialog::SetLeadFieldFile ( const char* file )
{
static GetFileFromUser  getfile ( "Lead Field File", AllLeadFieldFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( CrisTransfer.LeadFieldFile ) )
        return;

    TransferData ( tdSetData );
    }
else {
    StringCopy ( CrisTransfer.LeadFieldFile, file );
    getfile.SetOnly ( file );
    }

                                        // switch mode?
if ( StringIsNotEmpty ( CrisTransfer.LeadFieldFile ) ) {
    CrisTransfer.LeadFieldPresets.Select ( CrisLFPresetReload );
                                        // if you load a Lead Field / Solution Points, usually it's for processing an inverse?
    CrisTransfer.InverseMN          = BoolToCheck (  true );
    CrisTransfer.InverseWMN         = BoolToCheck (  true );
    CrisTransfer.InverseDale        = BoolToCheck (  true );
    CrisTransfer.InverseSLoreta     = BoolToCheck (  true );
    CrisTransfer.InverseLoreta      = BoolToCheck (  true );
    CrisTransfer.InverseLaura       = BoolToCheck (  true );
    CrisTransfer.InverseELoreta     = BoolToCheck (  true );
    }


TransferData ( tdSetData );

SetBaseFilename ();

ResetCarets ();

CheckLeadFieldSolPointsFile ();
}


//----------------------------------------------------------------------------
void    TCreateInverseMatricesDialog::CmBrowseSolPointsFile ( owlwparam w )
{
if ( w == IDC_BROWSEINPUTSPFILE )
    SetLeadFieldSolPointsFile ( 0 );
else
    SetLoadSolPointsFile      ( 0 );
}


void    TCreateInverseMatricesDialog::SetLeadFieldSolPointsFile ( const char* file )
{
static GetFileFromUser  getfile ( "Lead Field Solution Points File", AllSolPointsFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( CrisTransfer.LeadFieldSolPointsFile ) )
        return;

    TransferData ( tdSetData );
    }
else {
    StringCopy ( CrisTransfer.LeadFieldSolPointsFile, file );
    getfile.SetOnly ( file );
    }

                                        // switch mode?
if ( StringIsNotEmpty ( CrisTransfer.LeadFieldSolPointsFile ) ) {

    CrisTransfer.LeadFieldPresets.Select ( CrisLFPresetReload );
                                        // if you load a Lead Field / Solution Points, usually it's for processing an inverse?
    CrisTransfer.InverseMN          = BoolToCheck (  true );
    CrisTransfer.InverseWMN         = BoolToCheck (  true );
    CrisTransfer.InverseDale        = BoolToCheck (  true );
    CrisTransfer.InverseSLoreta     = BoolToCheck (  true );
    CrisTransfer.InverseLoreta      = BoolToCheck (  true );
    CrisTransfer.InverseLaura       = BoolToCheck (  true );
    CrisTransfer.InverseELoreta     = BoolToCheck (  true );
    }

                                        // switch mode?
if ( StringIsEmpty ( CrisTransfer.LoadSolPointsFile ) )
    CrisTransfer.SpPresets.Select ( CrisSPPresetFromLeadField );


TransferData ( tdSetData );

//SetBaseFilename ();

ResetCarets ();

CheckLeadFieldSolPointsFile ();
}


void    TCreateInverseMatricesDialog::SetLoadSolPointsFile ( const char* file )
{
static GetFileFromUser  getfile ( "Loading Solution Points File", AllSolPointsFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( CrisTransfer.LoadSolPointsFile ) )
        return;

    TransferData ( tdSetData );
    }
else {
    StringCopy ( CrisTransfer.LoadSolPointsFile, file );
    getfile.SetOnly ( file );
    }

                                        // switch mode?
if ( StringIsNotEmpty ( CrisTransfer.LoadSolPointsFile ) )
    CrisTransfer.SpPresets.Select ( CrisSPPresetLoad );


TransferData ( tdSetData );

//SetBaseFilename ();

ResetCarets ();


if ( ! TPoints ( file ).IsGrid () ) {
    ShowMessage ( "The given Solution Points do not seem to be regularly spaced on a grid." NewLine 
                  "This will very likely cause some errors later on." NewLine 
                  NewLine 
                  "Check this is really the file you intend to use!", CrisTransfer.LoadSolPointsFile, ShowMessageWarning, this );
    } // ! regular

CheckLeadFieldSolPointsFile ();
}


void    TCreateInverseMatricesDialog::CheckLeadFieldSolPointsFile ()
{
TransferData ( tdGetData );
                                        // check the coherence between LF and XYZ
CheckXyzFile ();

LeadFieldAndSolutionPointsMatch     = true;

if ( ! CrisTransfer.HasLeadFieldSolPointsFile () )
    return;


                                        // We either need a grid or an aligned grid, according to if we need to interpolate the LF
TPoints             lfsp ( CrisTransfer.LeadFieldSolPointsFile );
int                 lfspnumsolp             = lfsp.GetNumPoints ();


if (   CrisTransfer.LeadFieldPresets.GetSelIndex () == CrisLFPresetReload 
    && CrisTransfer.SpPresets       .GetSelIndex () == CrisSPPresetFromLeadField ) {

    IntegerToString ( CrisTransfer.NumSolPoints, lfspnumsolp );

    TransferData ( tdSetData );
    }

                                        // if only 1 type of file has been updated, we might end-up with an incoherent state
                                        // so check if current LF is compatible with current SP of LF
if ( CrisTransfer.HasLeadFieldFile () ) {

    int                 lfnumsolp       = TLeadField ( CrisTransfer.LeadFieldFile ).GetNumSolutionPoints ();

    if ( lfspnumsolp != lfnumsolp ) {
        LeadFieldAndSolutionPointsMatch     = false;

        char                buff[ 256 ];

        StringCopy  ( buff, "Not the same amount of Solution Points:" NewLine 
                            NewLine
                            "  - Lead Field "      Tab "= ", IntegerToString ( lfnumsolp   ), NewLine 
                            "  - Solution Points " Tab "= ", IntegerToString ( lfspnumsolp ), NewLine 
                            NewLine 
                            "Please check again your files!" 
                    );

        ShowMessage ( buff, CreatingInverseTitle, ShowMessageWarning, this );
        }
    }

                                        // check grid, aligned or not according to requested processing
bool                computesps              = CrisTransfer.IsComputeSPs ();

bool                loadsps                 = CrisTransfer.IsLoadSPs () 
                                           && CrisTransfer.HasLoadSolPointsFile ();

bool                loadleadfield           = CrisTransfer.IsLoadLeadField ()
                                           && CrisTransfer.HasLeadFieldFile () 
                                           && CrisTransfer.HasLeadFieldSolPointsFile ();

bool                interpolateleadfield    = loadleadfield 
                                           && ( computesps || loadsps );


if ( interpolateleadfield ) {

    if ( ! lfsp.IsGridAligned () ) {

        LeadFieldAndSolutionPointsMatch     = false;

        ShowMessage ( "The given Solution Points do not seem to be regularly spaced on a grid," NewLine 
                      "aligned on the XYZ axis."                                                NewLine 
                      "This will very likely cause some errors later on."                       NewLine 
                      NewLine 
                      "Check this is really the file you intend to use!", CrisTransfer.LeadFieldSolPointsFile, ShowMessageWarning, this );
        }
    }
else {

    if ( ! lfsp.IsGrid () ) {

        LeadFieldAndSolutionPointsMatch     = false;

        ShowMessage ( "The given Solution Points do not seem to be regularly spaced on a grid." NewLine 
                      "This will very likely cause some errors later on."                       NewLine 
                      NewLine 
                      "Check this is really the file you intend to use!", CrisTransfer.LeadFieldSolPointsFile, ShowMessageWarning, this );
        }
    }
}


//----------------------------------------------------------------------------
void    TCreateInverseMatricesDialog::CmInverseAllNone ()
{
int                 count           = CheckToBool ( InverseMN       ->GetCheck () )
                                    + CheckToBool ( InverseWMN      ->GetCheck () )
                                    + CheckToBool ( InverseDale     ->GetCheck () )
                                    + CheckToBool ( InverseSLoreta  ->GetCheck () )
                                    + CheckToBool ( InverseLoreta   ->GetCheck () )
                                    + CheckToBool ( InverseLaura    ->GetCheck () )
                                    + CheckToBool ( InverseELoreta  ->GetCheck () );


uint32              check           = BoolToCheck ( count == 0 ? true   // all off              -> all on
                                                  : count == 7 ? false  // all on               -> all off
                                                  :              true );// some on & some off   -> all on


InverseMN       ->SetCheck ( check );
InverseWMN      ->SetCheck ( check );
InverseDale     ->SetCheck ( check );
InverseSLoreta  ->SetCheck ( check );
InverseLoreta   ->SetCheck ( check );
InverseLaura    ->SetCheck ( check );
InverseELoreta  ->SetCheck ( check );
}


//----------------------------------------------------------------------------
void    TCreateInverseMatricesDialog::CmUpOneDirectory ()
{
RemoveLastDir ( CrisTransfer.BaseFileName );

BaseFileName->SetText ( CrisTransfer.BaseFileName );

ResetCarets ();
}


void    TCreateInverseMatricesDialog::CmBrowseBaseFileName ()
{
static GetFileFromUser  getfile ( "Base File Name", AllFilesFilter, 1, GetFilePath );


if ( ! getfile.Execute ( CrisTransfer.BaseFileName ) )
    return;


TransferData ( tdSetData );

ResetCarets ();
}

                                        // generate a smart base name
void    TCreateInverseMatricesDialog::SetBaseFilename ()
{
TGoF                gof;
TFileName           match;

if ( CrisTransfer.HasHeadMri () )                           gof.Add ( CrisTransfer.HeadMriFile      );
if ( CrisTransfer.HasBrainMri () )                          gof.Add ( CrisTransfer.BrainMriFile     );
if ( CrisTransfer.HasGreyMri () )                           gof.Add ( CrisTransfer.GreyMriFile      );
if ( gof.IsEmpty () && CrisTransfer.HasXyz () )             gof.Add ( CrisTransfer.XyzFile          );
if ( gof.IsEmpty () && CrisTransfer.HasLeadFieldFile () )   gof.Add ( CrisTransfer.LeadFieldFile    );


if ( gof.IsEmpty () )
    return;


gof.GetCommonString ( match, true /*, true*/ );


if ( ! IsAbsoluteFilename ( match ) )
    return;

                                        // finally, compose the base name
//PostfixFilename ( match, " Inv" );


//DBGM ( match, "BaseFileName" );

BaseFileName->SetText ( match );

ResetCarets ();
}


//----------------------------------------------------------------------------
void    TCreateInverseMatricesDialog::EvDropFiles ( TDropInfo drop )
{
TPointInt           where;
TGoF                mrifiles        ( drop, AllMriFilesExt, &where );
TGoF                xyzfiles        ( drop, AllCoordinatesFilesExt );
TGoF                lffiles         ( drop, AllLeadFieldFilesExt   );
TGoF                oldlffiles      ( drop, FILEEXT_BIN            );   // not supported anymore due to lack of real format (header with sizes mainly)
TGoF                spfiles         ( drop, AllSolPointsFilesExt   );
TGoF                lmfiles         ( drop, FILEEXT_LM              );
TGoF                remainingfiles  ( drop, AllMriFilesExt " " AllCoordinatesFilesExt " " AllLeadFieldFilesExt " " FILEEXT_BIN " " AllSolPointsFilesExt " " FILEEXT_LM, 0, true );

drop.DragFinish ();

//where.Show ( "Dropped @" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) mrifiles; i++ ) {

    MriContentType  guess       = GuessTypeFromFilename ( mrifiles[ i ] );

    if      ( IsGrey      ( guess ) )       SetMriFile      ( mrifiles[ i ], 2 );
    else if ( IsBrain     ( guess ) )       SetMriFile      ( mrifiles[ i ], 1 );
    else if ( IsFullHead  ( guess ) )       SetMriFile      ( mrifiles[ i ], 0 );
    else if ( IsRoiMask   ( guess ) )       SetTissuesFile  ( mrifiles[ i ] );
    else if ( (int) mrifiles == 1 ) {
        if      ( where.Y >= 169 )          SetTissuesFile  ( mrifiles[ i ] );
        else if ( where.Y >= 137 )          SetMriFile      ( mrifiles[ i ], 2 );
        else if ( where.Y >= 106 )          SetMriFile      ( mrifiles[ i ], 1 );
        else                                SetMriFile      ( mrifiles[ i ], 0 );
        }
    else                                    SetMriFile      ( mrifiles[ i ], 0 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) xyzfiles; i++ )

    SetXyzFile ( xyzfiles[ i ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set Lead Field first, as this is unambiguous
for ( int i = 0; i < (int) lffiles; i++ ) {

    SetLeadFieldFile ( lffiles[ i ] );

    if ( CrisTransfer.HasLeadFieldFile ()
        && ! ( (bool) spfiles || CrisTransfer.HasLeadFieldSolPointsFile () ) ) {

                                        // guess spi file name from lf file name
        TFileName               spfilefromlf ( lffiles[ i ] );
        StringReplace ( spfilefromlf, "." InfixLeadField "." FILEEXT_LF, "." FILEEXT_SPIRR );

        if ( spfilefromlf.CanOpenFile () )

            SetLeadFieldSolPointsFile ( spfilefromlf ); // no need for checking(?)

        else

            ShowMessage ( "Do not forget to also specify the Solution Points file" NewLine "associated to this Lead Field!", CreatingInverseTitle, ShowMessageWarning, this );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we have some guessing to do here, as SPs could be either associated to the LF or to the Solution Space
if ( (bool) spfiles ) {
                                        // note that LF field has been set before reaching here!
    if ( CrisTransfer.HasLeadFieldFile () ) {
                                        // guess spi file name from lf file name
        TFileName               spfilefromlf ( CrisTransfer.LeadFieldFile );
        StringReplace ( spfilefromlf, "." InfixLeadField "." FILEEXT_LF, "." FILEEXT_SPIRR );

                                        // be smart and try to direct the SPs to the right box
        int                 lfnumsolp       = TLeadField ( CrisTransfer.LeadFieldFile ).GetNumSolutionPoints ();

                                        // loop through all SP files
        for ( int i = 0; i < (int) spfiles; i++ ) {

            if ( StringIs ( spfiles[ i ], spfilefromlf ) )  SetLeadFieldSolPointsFile ( spfiles[ i ] );

            else {

                int                 spnumsolp       = TPoints ( spfiles[ i ] ).GetNumPoints ();

                if ( spnumsolp == lfnumsolp )   SetLeadFieldSolPointsFile ( spfiles[ i ] );
                else                            SetLoadSolPointsFile      ( spfiles[ i ] );
                }
            }
        } // HasLeadFieldFile

    else {                              // no LF set

                                        // second guess: biggest SP files usually associates with the LF
        if ( (int) spfiles == 2 ) {
            int                 spnumsolp0      = TPoints ( spfiles[ 0 ] ).GetNumPoints ();
            int                 spnumsolp1      = TPoints ( spfiles[ 1 ] ).GetNumPoints ();
                                        // biggest file is usually the input SP (?)
            if ( spnumsolp0 > spnumsolp1 ) {
                SetLeadFieldSolPointsFile ( spfiles[ 0 ] );
                SetLoadSolPointsFile      ( spfiles[ 1 ] );
                }
            else {
                SetLeadFieldSolPointsFile ( spfiles[ 1 ] );
                SetLoadSolPointsFile      ( spfiles[ 0 ] );
                }
            } // 2 SP

        else {                          // not 2 SP files (1, 3, 4 etc...)
                                        // set according to position
            if ( where.Y >= 600 )           SetLeadFieldSolPointsFile ( spfiles[ 0 ] );
            else                            SetLoadSolPointsFile      ( spfiles[ 0 ] );
            }
        } // ! HasLeadFieldFile


    if ( CrisTransfer.HasLeadFieldSolPointsFile ()
        && ! ( (bool) lffiles || CrisTransfer.HasLeadFieldFile () ) )

        ShowMessage ( "Do not forget to also specify the Lead Field file" NewLine "associated to these Solution Points!", CreatingInverseTitle, ShowMessageWarning, this );
    } // some SP files


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) lmfiles ) {

    for ( int i = 0; i < (int) lmfiles; i++ ) {

        TSplitLinkManyFile      lm ( lmfiles[ i ] );
        TListIterator<char>     iterator;


        if ( (bool) lm.lmri ) {

            foreachin ( lm.lmri, iterator  ) {

                MriContentType  guess       = GuessTypeFromFilename ( iterator );

                if      ( IsGrey      ( guess ) )       SetMriFile      ( iterator, 2 );
                else if ( IsBrain     ( guess ) )       SetMriFile      ( iterator, 1 );
                else if ( IsFullHead  ( guess ) )       SetMriFile      ( iterator, 0 );
                else if ( IsRoiMask   ( guess ) )       SetTissuesFile  ( iterator );
                else if ( (int) lm.lmri == 1 ) {
                    if      ( where.Y >= 169 )          SetTissuesFile  ( mrifiles[ i ] );
                    else if ( where.Y >= 137 )          SetMriFile      ( mrifiles[ i ], 2 );
                    else if ( where.Y >= 106 )          SetMriFile      ( mrifiles[ i ], 1 );
                    else                                SetMriFile      ( mrifiles[ i ], 0 );
                    }
                else                                    SetMriFile      ( iterator, 0 );
                }
            }


        if ( (bool) lm.lxyz ) {

            foreachin ( lm.lxyz, iterator  )

                SetXyzFile ( iterator () );
            }


        if ( (bool) lm.lsp ) {

            foreachin ( lm.lsp, iterator  ) {
                                        // set according to position
                if ( where.Y >= 600 )           SetLeadFieldSolPointsFile ( iterator () );
                else                            SetLoadSolPointsFile      ( iterator () );
                }
            }
        }
    } // .lm files


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // for now, let's be nice and give an explicit message about not supporting the FILEEXT_BIN files
if ( (bool) oldlffiles ) {

    char                buff[ 1024 ];

    StringCopy ( buff, "'." FILEEXT_BIN "'  files are not supported anymore as valid Lead Field input." NewLine 
                       NewLine 
                       "You can use instead a  '." FILEEXT_LF "' or '*.LeadField." FILEEXT_RIS "'  file," NewLine 
                       "or any of the following file types:" NewLine 
                       NewLine 
                       Tab AllLeadFieldFilesExt 
                );

    ShowMessage ( buff, CreatingInverseTitle, ShowMessageWarning, this );
    }

if ( (bool) remainingfiles )
    remainingfiles.Show ( IrrelevantErrorMessage );
}


//----------------------------------------------------------------------------
void    TCreateInverseMatricesDialog::CmOkEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

                                        // Adding some constrains  Head > Brain > Grey
if ( CrisTransfer.HasAnyMri ()
    && (    CrisTransfer.HasBrainMri () && ! CrisTransfer.HasHeadMri  () 
         || CrisTransfer.HasGreyMri  () && ! CrisTransfer.HasBrainMri () ) ) {
    tce.Enable ( false );
    return;
    }

if ( CrisTransfer.IsReorientMris ()
//&& ( ! CheckToBool ( CrisTransfer.SagittalPlaneMri ) || ! CrisTransfer.HasHeadMri  ()
  && ( ! CrisTransfer.HasHeadMri  ()
    ||   CheckToBool ( CrisTransfer.EquatorPlaneMri  ) && ! CrisTransfer.HasBrainMri ()
     ) ) {
    tce.Enable ( false );
    return;
    }

if (   CrisTransfer.IsBiasField ()
  && ! CrisTransfer.HasBrainMri () ) {
    tce.Enable ( false );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! LeadFieldAndXyzMatch ) {
    tce.Enable ( false );
    return;
    }

if ( CrisTransfer.IsXyzCoregistrationDone () && ! ( CrisTransfer.HasXyz () && CrisTransfer.HasAnyMri () ) ) {
    tce.Enable ( false );
    return;
    }
                                        // these processings need at least the full MRI
if ( CrisTransfer.IsXyzCoregisterToMri ()
  && ! ( CrisTransfer.HasXyz () && CrisTransfer.HasHeadMri () ) ) {
    tce.Enable ( false );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( CrisTransfer.IsComputeSPs ()
  && ! ( ( CrisTransfer.HasBrainMri () || CrisTransfer.HasGreyMri () ) && StringToFloat ( CrisTransfer.NumSolPoints ) > 0 ) ) {
    tce.Enable ( false );
    return;
    }

if ( CrisTransfer.IsSameSPasLF ()
  && ! ( CrisTransfer.HasLeadFieldSolPointsFile () /*&& StringToFloat ( CrisTransfer.NumSolPoints ) > 0*/ ) ) {
    tce.Enable ( false );
    return;
    }

if ( CrisTransfer.IsLoadSPs ()
  && ! ( CrisTransfer.HasLoadSolPointsFile () /*&& StringToInteger( CrisTransfer.LoadSPsMaxSize ) > 0*/ ) ) {
    tce.Enable ( false );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! LeadFieldAndSolutionPointsMatch ) {
    tce.Enable ( false );
    return;
    }

if ( CrisTransfer.IsComputeLeadField ()
  && ! (   CrisTransfer.IsThicknessFromMri ()     && CrisTransfer.HasHeadMri () 
        || CrisTransfer.IsThicknessFromTissues () && CrisTransfer.HasTissuesFile () 
       ) ) {
    tce.Enable ( false );
    return;
    }

if ( CrisTransfer.IsComputeLeadField ()
  && ! (    IsInsideLimits ( StringToDouble ( CrisTransfer.LeadFieldTargetAge ), SkullThicknessMinAge, SkullThicknessMaxAge )
         && StringToDouble ( CrisTransfer.SkullConductivity ) > 0 
         && ! CrisTransfer.IsAdjustSkullMeanThickness () || StringToDouble ( CrisTransfer.SkullMeanThickness ) > 0 
       ) ) {
    tce.Enable ( false );
    return;
    }

if ( CrisTransfer.IsComputeLeadField ()
  && ! ( CrisTransfer.IsXyzCoregistrationDone () || CrisTransfer.IsXyzCoregisterToMri () ) ) {
    tce.Enable ( false );
    return;
    }

if ( CrisTransfer.IsLoadLeadField ()
  && ! ( CrisTransfer.HasLeadFieldFile () && CrisTransfer.HasLeadFieldSolPointsFile () ) ) {
    tce.Enable ( false );
    return;
    }


if ( CrisTransfer.IsInverseProcessing ()
  && ! ( CrisTransfer.IsSPsProcessing () && CrisTransfer.IsLeadFieldProcessing () ) ) {
    tce.Enable ( false );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! IsAbsoluteFilename ( CrisTransfer.BaseFileName ) ) {
    tce.Enable ( false );
    return;
    }


tce.Enable ( true );
}


void    TCreateInverseMatricesDialog::CmMrisEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable ( CrisTransfer.HasAnyMri () );
}


void    TCreateInverseMatricesDialog::CmBrainMriEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable ( CrisTransfer.HasBrainMri () );
}


void    TCreateInverseMatricesDialog::CmNormalizeEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable ( CrisTransfer.HasAnyMri () && CrisTransfer.IsReorientMris () );
}


void    TCreateInverseMatricesDialog::CmXyzEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable ( ! CrisTransfer.IsXyzNoTransform () );
}


//void    TCreateInverseMatricesDialog::CmSPComputeButtonEnable ( TCommandEnabler &tce )
//{
////TransferData ( tdGetData );
//tce.Enable ( CrisTransfer.HasBrainMri () );
//}


void    TCreateInverseMatricesDialog::CmNumSPEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable (   CrisTransfer.HasBrainMri () && CrisTransfer.IsComputeSPs () 
            || CrisTransfer.IsSameSPasLF () );
}


void    TCreateInverseMatricesDialog::CmSPComputeEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable ( CrisTransfer.HasBrainMri () && ! CrisTransfer.HasGreyMri () );
}


void    TCreateInverseMatricesDialog::CmSPLoadEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable ( CrisTransfer.IsLoadSPs () );
}


void    TCreateInverseMatricesDialog::CmComputeLeadFieldEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable (    CrisTransfer.IsComputeLeadField () 
             && AllLeadFieldPresets[ CrisTransfer.LeadFieldPresets.GetSelIndex() ].IsHuman () );
}


void    TCreateInverseMatricesDialog::CmAdjustSkullThicknessEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable (    CrisTransfer.IsComputeLeadField () 
             && CrisTransfer.IsAdjustSkullMeanThickness ()
             && AllLeadFieldPresets[ CrisTransfer.LeadFieldPresets.GetSelIndex() ].IsHuman () );
}


void    TCreateInverseMatricesDialog::CmLoadLeadFieldEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable ( CrisTransfer.IsLoadLeadField () );
}


void    TCreateInverseMatricesDialog::CmInverseMatricesEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
tce.Enable (    CrisTransfer.IsComputeLeadField () 
             || CrisTransfer.IsLoadLeadField () && CrisTransfer.HasLeadFieldFile () && CrisTransfer.HasLeadFieldSolPointsFile () );
}


//----------------------------------------------------------------------------
void    TCreateInverseMatricesDialog::EvAgeChange ()
{
char                buff[ 256 ];
double              age;

LeadFieldTargetAge->GetDoubleValue ( age );

                                        // empty input? reset everything and get out
if ( StringIsEmpty ( buff ) ) {
    Reset ( SkullConductivity  );
    Reset ( SkullMeanThickness );
    return;
    }

                                        // otherwise clip value to limits, and continue
if ( ! IsInsideLimits ( StringToDouble ( CrisTransfer.LeadFieldTargetAge ), SkullThicknessMinAge, SkullThicknessMaxAge ) ) {
    return;
    }

                                        // age is OK, update conductivity and thickness from (human) curves
double              skullcond       = AgeToSkullConductivity ( age );
double              skullthickness  = AgeToSkullThickness    ( age );


SkullConductivity   ->SetText ( FloatToString ( buff, skullcond,      4 ) );
SkullMeanThickness  ->SetText ( FloatToString ( buff, skullthickness, 2 ) );
}


//----------------------------------------------------------------------------
                                        // Copying volume + optional default filtering
void    GetVolumeCopy ( TVolumeDoc* MriHead, Volume& HeadVolume, bool smoothmri, double& fullvolumerescale )
{
                                        // copy volume data
HeadVolume          = *MriHead->GetData ();
fullvolumerescale   = 1;


if ( smoothmri ) {
                                        // optional smoothing
//  TBoundingBox<double>    boxbefore ( HeadVolume );
//  double                  sizebefore  = boxbefore.Radius ();

                                        // for all our processings, we prefer to have something smoother:
                                        // median keeps the shape (slightly smaller, in fact), while removing a lot of ripples, noise and tumburuk
    FctParams           p;

    p ( FilterParamDiameter )     = 2.83;
    HeadVolume.Filter ( FilterTypeMedian,   p, true );

                                        // smooth the angles, & make it a little wider too (but also a bit round)
    p ( FilterParamDiameter )     = 6.5;
    HeadVolume.Filter ( FilterTypeGaussian, p, true );


                                        // getting an idea of the rescaling induced by the filters
//  TBoundingBox<double>    boxafter ( HeadVolume, backvalue );
//  double                  sizeafter   = boxafter.Radius ();

                                      // this formula is logically correct, but it yields values of ~ 1.05 instead of < 1?
//  fullvolumerescale   = sizebefore / sizeafter;

                                        // so let's override this manually for the moment...
//  fullvolumerescale   = RescaleFilteredToNonFiltered;

//  DBGV2 ( fullvolumerescale, RescaleFilteredToNonFiltered, "fullvolumerescale / fixed rescaling" );

                                        // save this file for checking
//  AddExtension ( filefulloriginal, "Filtered.img" );
//  HeadVolume.Write ( filefulloriginal );
//  RemoveExtension ( filefulloriginal, 2 );
    }

}


//----------------------------------------------------------------------------
void    TCreateInverseMatricesDialog::CmOk ()
{
Destroy ();                             // ..the window, not the object!


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                mriprocessing           = CrisTransfer.IsMriProcessing       ();
bool                xyzprocessing           = CrisTransfer.IsXyzProcessing       ();
bool                spsprocessing           = CrisTransfer.IsSPsProcessing       ();
bool                leadfieldprocessing     = CrisTransfer.IsLeadFieldProcessing ();
bool                inverseprocessing       = CrisTransfer.IsInverseProcessing   ();

if ( ! (   CrisTransfer.HasAnyMri () 
        || mriprocessing 
        || xyzprocessing 
        || spsprocessing 
        || leadfieldprocessing 
        || inverseprocessing         ) ) {

    ShowMessage ( "You didn't select any processing!" NewLine 
                  "Thanks for coming anyway...", CreatingInverseTitle, ShowMessageWarning, this );

    return;
    }

                                        // Changing priority
SetProcessPriority ( BatchProcessingPriority );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // for debugging
//#define             InvMatrixMoreOutputs


bool                biasfield               = CrisTransfer.HasBrainMri () 
                                           && CrisTransfer.IsBiasField ();

bool                smoothmri               = CrisTransfer.HasAnyMri   () 
                                           && CrisTransfer.IsSmoothMri ();

bool                reorientmris            = CrisTransfer.HasHeadMri     ()                    // just axis permutations
                                           && CrisTransfer.IsReorientMris ();    

bool                setsagittalplanemri     = reorientmris 
                                           && CrisTransfer.HasHeadMri ()
                                           && CheckToBool ( CrisTransfer.SagittalPlaneMri );

bool                setequatorplanemri      = reorientmris                                      // Head for initial transform, Brain for adjusting
                                           && CrisTransfer.HasHeadBrainMris () 
                                           && CheckToBool ( CrisTransfer.EquatorPlaneMri  );

bool                realignmri              = setsagittalplanemri                               // rotations to adjust cutting planes
                                           || setequatorplanemri;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//bool              xyznotransform          = CrisTransfer.IsXyzNoTransform ();

bool                xyzcoregistrationdone   = CrisTransfer.IsXyzCoregistrationDone () 
                                           && CrisTransfer.HasAnyMri               ();

bool                xyzcoregistertomrii     = CrisTransfer.IsXyzCoregisterToMriI () 
                                           && CrisTransfer.HasHeadMri            ();

bool                xyzcoregistertomriiredo = CrisTransfer.IsXyzCoregisterToMriIRedo () 
                                           && CrisTransfer.HasHeadMri                ();

bool                xyzcoregistertomri      = xyzcoregistertomrii 
                                           || xyzcoregistertomriiredo;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Solution Points are computed / loaded on their own
bool                computesps              = CrisTransfer.IsComputeSPs ();

bool                loadsps                 = CrisTransfer.IsLoadSPs            () 
                                           && CrisTransfer.HasLoadSolPointsFile ();

bool                samespsaslf             = CrisTransfer.IsSameSPasLF              ()
                                           && CrisTransfer.HasLeadFieldSolPointsFile ();

bool                hassps                  = computesps 
                                           || loadsps 
                                           || samespsaslf;


int                 numsolpoints            = StringToInteger ( CrisTransfer.NumSolPoints );

double              ressolpoints            = StringToDouble  ( CrisTransfer.NumSolPoints );
                                        // pick either the number of solution points, or the resolution, but not both
if ( numsolpoints > 50 )    ressolpoints    = 0;
else                        numsolpoints    = 0;
                    
int                 samespsaslfmaxsolpoints = numsolpoints; // !now re-using the computation field!

                                        // Compute Grey if not already given, and only if solution space are required
bool                computegrey             = ! CrisTransfer.HasGreyMri () 
                                           &&   CrisTransfer.HasBrainMri () 
                                           &&   hassps;

bool                hasgrey                 = CrisTransfer.HasGreyMri () 
                                           || computegrey;


GreyMatterFlags     greyflagspreproc        = /*GreyMatterGaussian*/ /*| GreyMatterBiasField*/ GreyMatterNothing;

GreyMatterFlags     greyflagsproc           = CheckToBool ( CrisTransfer.GreyFat  ) ? GreyMatterFat           // current Fat is not actually that fat
                                            : CheckToBool ( CrisTransfer.GreyAll  ) ? GreyMatterWhole         // brain == grey, useful for unusual Grey/White distribution, like bonkeys and mabies
                                            :                                         GreyMatterRegular;

GreyMatterFlags     greyflagspostproc       = (GreyMatterFlags) ( GreyMatterPostprocessing | GreyMatterAsymmetric );

GreyMatterFlags     greyflags               = (GreyMatterFlags) ( greyflagspreproc | greyflagsproc | greyflagspostproc );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Lead Field is computed / loaded on its own
const OneLeadFieldPreset& lfpreset          = AllLeadFieldPresets[ CrisTransfer.LeadFieldPresets.GetSelIndex () ];


bool                isthicknessfrommri      = CrisTransfer.IsThicknessFromMri ()
                                           && CrisTransfer.HasHeadMri         ();

bool                isthicknessfromtissues  = CrisTransfer.IsThicknessFromTissues () 
                                           && CrisTransfer.HasTissuesFile         ();

bool                computeleadfield        = CrisTransfer.IsComputeLeadField ();

bool                loadleadfield           = CrisTransfer.IsLoadLeadField           () 
                                           && CrisTransfer.HasLeadFieldFile          () 
                                           && CrisTransfer.HasLeadFieldSolPointsFile ();

bool                hasleadfield            = computeleadfield 
                                           || loadleadfield;

bool                interpolateleadfield    = loadleadfield 
                                           && ( computesps || loadsps );

bool                getheadmodel            =  computeleadfield
                                            && lfpreset.IsGeometryLSMAC ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                inversemn               = CrisTransfer.IsInverseMN       ();
bool                inversewmn              = CrisTransfer.IsInverseWMN      ();
bool                inversedale             = CrisTransfer.IsInverseDale     ();
bool                inversesloreta          = CrisTransfer.IsInverseSLoreta  ();
bool                inverseloreta           = CrisTransfer.IsInverseLoreta   ();
bool                inverselaura            = CrisTransfer.IsInverseLaura    ();
bool                inverseeloreta          = CrisTransfer.IsInverseELoreta  ();
bool                computeresmat           = CrisTransfer.IsInverseProcessing ();
int                 numinversesmat          = inversemn + inversewmn + inversedale + inversesloreta + inverseloreta + inverselaura + inverseeloreta;


bool                    regularization      = CrisTransfer.IsRegularization ();
NeighborhoodType        loretaneighborhood  = InverseNeighborhood;
NeighborhoodType        lauraneighborhood   = InverseNeighborhood;
NeighborhoodReduction   neighbors26to18     = InverseNeighborhood26to18;    // how to reduce neighborhood from 26 to 18, strictly not over 18


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Now can set spflags
GreyMatterFlags     spflagspostproc         = GreyMatterAsymmetric;

GreyMatterFlags     spflagscheck            = GreyMatterFlags   (
                                              loretaneighborhood == lauraneighborhood       ? GreyMatterLauraCheck                         | GreyMatterSinglePointCheck
                                            :                                                 GreyMatterLauraCheck | GreyMatterLoretaCheck | GreyMatterSinglePointCheck
                                                                );

GreyMatterFlags     spflags                 = GreyMatterFlags ( spflagspostproc | spflagscheck );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // can we get rid of some heavy computing?
if (   loadleadfield 
    && CrisTransfer.HasLeadFieldFile () 
    && CrisTransfer.HasLeadFieldSolPointsFile () )

    xyzcoregistertomri      =
    xyzcoregistertomrii     = 
    xyzcoregistertomriiredo = false;

bool                interactive             = CheckToBool ( CrisTransfer.Interactive );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // base directory & file names
TFileName           BaseDir;
TFileName           BaseDirMore;
TFileName           BaseDirLocal;

TFileName           BaseFileName;
TFileName           BaseFileNameMore;
TFileName           BaseFileNameLocal;

TFileName           fileoutprefix;
TFileName           buff;


StringCopy      ( BaseDir,                       CrisTransfer.BaseFileName );
                                        // extract string "prefix"
StringCopy      ( fileoutprefix,                 ToFileName ( BaseDir ) );
                                        // compose path access and main prefix "full path\prefix\prefix"
StringCopy      ( BaseFileName,     BaseDir,        "\\",   fileoutprefix );


StringCopy      ( BaseDirMore,      BaseDir,        "\\",   DirMore       );
StringCopy      ( BaseFileNameMore, BaseDirMore,    "\\",   fileoutprefix );

StringCopy      ( BaseDirLocal,     BaseDir,        "\\",   "Reorient"    );
StringCopy      ( BaseFileNameLocal,BaseDirLocal,   "\\",   fileoutprefix );


CreatePath      ( BaseDir,      false );
CreatePath      ( BaseDirMore,  false );
CreatePath      ( BaseDirLocal, false );

                                        // delete any previous files, with my prefix only!
StringCopy      ( buff,                             BaseFileName,       ".*" );
DeleteFiles     ( buff );
StringCopy      ( buff,                             BaseFileNameMore,   ".*" );
DeleteFiles     ( buff );
StringCopy      ( buff,                             BaseFileNameLocal,  ".*" );
DeleteFiles     ( buff );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// For the sake of sanity of mind, files are organized into 4 distinct groups:
// - Input files, given by the user from the Dialog, from their original directories
// - Output ORIGINAL files, straight copies of some of these files, located on the main output root directory
// - Output LOCALLY TRANSFORMED files, the original files with some (geometrical) transforms applied
// - All others output files generated from the processing itself
// 
// Each output group can have/has a different naming policy:
// - Original files will replicate their source file name, for consistency and to allow for better versions tracking
// - All the other output files will be generated from the base file name, like  <MyComputation>.vrb, <MyComputation>.LORETA.is, <MyComputation>.SomeModel.SomeExt...
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Input files
TFileName           FileInputMriHead;
TFileName           FileInputMriBrain;
TFileName           FileInputMriGrey;
TFileName           FileInputMriTissues;
TFileName           FileInputXyz;
TFileName           FileInputSpLoad;
TFileName           FileInputSpLeadField;
TFileName           FileInputLeadFieldLoad;


if ( CrisTransfer.HasHeadMri  () )      StringCopy  ( FileInputMriHead,     CrisTransfer.HeadMriFile  );
if ( CrisTransfer.HasBrainMri () )      StringCopy  ( FileInputMriBrain,    CrisTransfer.BrainMriFile );
if ( CrisTransfer.HasGreyMri  () )      StringCopy  ( FileInputMriGrey,     CrisTransfer.GreyMriFile  );
if ( isthicknessfromtissues )           StringCopy  ( FileInputMriTissues,  CrisTransfer.TissuesFile  );
if ( CrisTransfer.IsXyzProcessing () )  StringCopy  ( FileInputXyz,         CrisTransfer.XyzFile      );


if      ( samespsaslf )                 StringCopy  ( FileInputSpLoad,      CrisTransfer.LeadFieldSolPointsFile );
else if ( loadsps     )                 StringCopy  ( FileInputSpLoad,      CrisTransfer.LoadSolPointsFile      );

if ( samespsaslf || loadleadfield )     StringCopy  ( FileInputSpLeadField, CrisTransfer.LeadFieldSolPointsFile );


if ( loadleadfield )                    StringCopy  ( FileInputLeadFieldLoad, CrisTransfer.LeadFieldFile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Output files - Local copies of the ORIGINAL input files
TFileName           FileOriginalMriHead;
TFileName           FileOriginalMriBrain;
TFileName           FileOriginalMriGrey;
TFileName           FileOriginalMriTissues;
TFileName           FileOriginalXyz;
TFileName           FileOriginalXyzCoreg;
TFileName           FileOriginalXyzDisplay;
TFileName           FileOriginalSp;

                                        // Try our best to generate missing file names from the existing ones, for consistency
if ( FileInputMriHead.IsNotEmpty () ) {

    FileOriginalMriHead = FileInputMriHead;
    FileOriginalMriHead.ReplaceExtension ( DefaultMriExt );
    FileOriginalMriHead.ReplaceDir       ( BaseDir );

//  if      ( StringContains ( (const char*) FileOriginalMriGrey, ".pro" ) ) {
//      StringReplace ( FileOriginalMriGrey, ".Pro", ".Head" );
//      StringReplace ( FileOriginalMriGrey, ".pro", ".head" );
//      }
//  else if ( StringContains ( (const char*) FileOriginalMriGrey, "_pro" ) ) {
//      StringReplace ( FileOriginalMriGrey, "_Pro", "_Head" );
//      StringReplace ( FileOriginalMriGrey, "_pro", "_head" );
//      }
    }


if ( FileInputMriBrain.IsNotEmpty () ) {

    FileOriginalMriBrain = FileInputMriBrain;
    FileOriginalMriBrain.ReplaceExtension ( DefaultMriExt );
    FileOriginalMriBrain.ReplaceDir       ( BaseDir );
    }


if ( FileInputMriGrey.IsNotEmpty () ) {

    FileOriginalMriGrey = FileInputMriGrey;
    FileOriginalMriGrey.ReplaceExtension ( DefaultMriExt );
    FileOriginalMriGrey.ReplaceDir       ( BaseDir );
    }
                                        // no input Grey file -> compute Grey Mask - it still needs a file name derived from Brain / Head file names
else if ( FileInputMriBrain.IsNotEmpty () || FileInputMriHead.IsNotEmpty () ) {

    FileOriginalMriGrey = FileInputMriBrain.IsNotEmpty () ? FileInputMriBrain : FileInputMriHead;
    FileOriginalMriGrey.ReplaceExtension ( DefaultMriExt );
    FileOriginalMriGrey.ReplaceDir       ( BaseDir );

                                        // replace any "Brain" by some "Grey"
    if      ( StringContains ( (const char*) FileOriginalMriGrey, "." InfixBrain ) ) {
        StringReplace ( FileOriginalMriGrey, ".Brain", ".Grey" );
        StringReplace ( FileOriginalMriGrey, ".brain", ".grey" );
        }
    else if ( StringContains ( (const char*) FileOriginalMriGrey, "_" InfixBrain ) ) {
        StringReplace ( FileOriginalMriGrey, "_Brain", "_Grey" );
        StringReplace ( FileOriginalMriGrey, "_brain", "_grey" );
        }
    else if ( StringContains ( (const char*) FileOriginalMriGrey, "." InfixHead ) ) {
        StringReplace ( FileOriginalMriGrey, ".Head", ".Grey" );
        StringReplace ( FileOriginalMriGrey, ".head", ".grey" );
        }
    else if ( StringContains ( (const char*) FileOriginalMriGrey, "_" InfixHead ) ) {
        StringReplace ( FileOriginalMriGrey, "_Head", "_Grey" );
        StringReplace ( FileOriginalMriGrey, "_head", "_grey" );
        }
    else if ( StringContains ( (const char*) FileOriginalMriGrey, ".pro" ) ) {
        StringReplace ( FileOriginalMriGrey, ".Pro", ".Grey" );
        StringReplace ( FileOriginalMriGrey, ".pro", ".grey" );
        }
    else if ( StringContains ( (const char*) FileOriginalMriGrey, "_pro" ) ) {
        StringReplace ( FileOriginalMriGrey, "_Pro", "_Grey" );
        StringReplace ( FileOriginalMriGrey, "_pro", "_grey" );
        }
    else
        PostfixFilename ( FileOriginalMriGrey, "." InfixGrey );
    }


if ( FileInputMriTissues.IsNotEmpty () ) {

    FileOriginalMriTissues = FileInputMriTissues;
    FileOriginalMriTissues.ReplaceExtension ( DefaultMriExt );
    FileOriginalMriTissues.ReplaceDir       ( BaseDir );
    }


if ( FileInputXyz.IsNotEmpty () ) {

    FileOriginalXyz = FileInputXyz;
    FileOriginalXyz.ReplaceExtension ( FILEEXT_XYZ );
    FileOriginalXyz.ReplaceDir       ( BaseDir );


    if ( FileInputMriHead.IsNotEmpty () ) {

        FileOriginalXyzCoreg = FileInputMriHead;
        FileOriginalXyzCoreg.ReplaceExtension ( FILEEXT_XYZ );
        FileOriginalXyzCoreg.ReplaceDir       ( BaseDir );

        StringReplace ( FileOriginalXyzCoreg, ".Head", "." );
        StringReplace ( FileOriginalXyzCoreg, ".Pro",  "." );
        StringReplace ( FileOriginalXyzCoreg, "..",    "." ); // in case of .Head. collapsed to ..

        StringReplace ( FileOriginalXyzCoreg, "_Head", "_" );
        StringReplace ( FileOriginalXyzCoreg, "_Pro",  "_" );
        StringReplace ( FileOriginalXyzCoreg, "__",    "_" ); // in case of _Head_ collapsed to __
        }
    else { // shouldn't happen
        FileOriginalXyzCoreg    = FileOriginalXyz;
//      PostfixFilename ( FileOriginalXyzCoreg, "." InfixCoregistered );
        }


    FileOriginalXyzDisplay  = xyzcoregistertomri ? FileOriginalXyzCoreg : FileOriginalXyz;
    PostfixFilename ( FileOriginalXyzDisplay, "." InfixDisplay );
    }


if ( FileInputSpLoad.IsNotEmpty () ) {

    FileOriginalSp = FileInputSpLoad;
    FileOriginalSp.ReplaceExtension ( FILEEXT_SPIRR );
    FileOriginalSp.ReplaceDir       ( BaseDir );
    }

else if ( FileInputMriHead.IsNotEmpty () ) {

    FileOriginalSp = FileInputMriHead;
    FileOriginalSp.ReplaceExtension ( FILEEXT_SPIRR );
    FileOriginalSp.ReplaceDir       ( BaseDir );

    StringReplace ( FileOriginalSp, ".Head", "." );
    StringReplace ( FileOriginalSp, ".Pro",  "." );
    StringReplace ( FileOriginalSp, "..",    "." ); // in case of .Head. collapsed to ..

    StringReplace ( FileOriginalSp, "_Head", "_" );
    StringReplace ( FileOriginalSp, "_Pro",  "_" );
    StringReplace ( FileOriginalSp, "__",    "_" ); // in case of _Head_ collapsed to __
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Local copies after some geometrical TRANSFORMS (axis reorientation, Sagittal & Transverse planes)
TFileName           FileLocalMriHead;
TFileName           FileLocalMriBrain;
TFileName           FileLocalMriGrey;
TFileName           FileLocalMriTissues;
TFileName           FileLocalXyz;
TFileName           FileLocalXyzCoreg;
TFileName           FileLocalXyzDisplay;
TFileName           FileLocalSp;
TFileName           FileLocalRadiiLayers;

                                        // Local copies are now in a sub-directory - we give them some consistent naming
StringCopy  ( FileLocalMriHead,                 BaseFileNameLocal,  "." InfixHead,                                          "." DefaultMriExt       );
StringCopy  ( FileLocalMriBrain,                BaseFileNameLocal,  "." InfixBrain,                                         "." DefaultMriExt       );
StringCopy  ( FileLocalMriGrey,                 BaseFileNameLocal,  "." InfixGrey,                                          "." DefaultMriExt       );
StringCopy  ( FileLocalMriTissues,              BaseFileNameLocal,  "." InfixTissues,                                       "." DefaultMriExt       );
StringCopy  ( FileLocalXyz,                     BaseFileNameLocal,                                                          "." FILEEXT_XYZ         );
StringCopy  ( FileLocalXyzCoreg,                BaseFileNameLocal,                                                          "." FILEEXT_XYZ         );
StringCopy  ( FileLocalXyzDisplay,              BaseFileNameLocal,  "." InfixDisplay,                                       "." FILEEXT_XYZ         );
StringCopy  ( FileLocalSp,                      BaseFileNameLocal,                                                          "." FILEEXT_SPIRR       );
StringCopy  ( FileLocalRadiiLayers,             BaseFileNameLocal,  "." "Boundaries.All Layers",                            "." FILEEXT_ELS         );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Output files - generated by Cartool
TFileName           fileverbose;

//TFileName         filesagittaloriginal;
//TFileName         fileequatororiginal;

TFileName           file3Dheadmodel;

TFileName           fileleadfield;
TFileName           fileleadfieldris;
TFileName           fileleadfieldeeg;

TFileName           filemneis;
TFileName           filewmneis;
TFileName           fileloretais;
TFileName           filelaurais;
TFileName           filedaleis;
TFileName           filesloretais;
TFileName           fileeloretais;

TFileName           filemnresmat;
TFileName           filewmnresmat;
TFileName           fileloretaresmat;
TFileName           filelauraresmat;
TFileName           filedaleresmat;
TFileName           filesloretaresmat;
TFileName           fileeloretaresmat;

TFileName           fileradii;
TFileName           fileradiilayers;

TFileName           filelmall;
TFileName           filelmxyz;
TFileName           filelmis;
TFileName           filelmris;


StringCopy  ( fileverbose,                      BaseFileName,                                                               "." FILEEXT_VRB         );

//StringCopy( filesagittaloriginal,             BaseFileNameMore,   "." InfixOriginal "." InfixMRI " " InfixSagittal,       "." FILEEXT_ELS         );
//StringCopy( fileequatororiginal,              BaseFileNameMore,   "." InfixOriginal "." InfixMRI " " InfixTransverse,     "." FILEEXT_ELS         );

StringCopy  ( file3Dheadmodel,                  reorientmris ? BaseFileNameLocal : BaseFileNameMore,   "." InfixHeadModel,  "." FILEEXT_XYZ         );

StringCopy  ( fileleadfield,                    BaseFileName,       "." InfixLeadField,                                     "." FILEEXT_LF          );  // higher precision version
StringCopy  ( fileleadfieldris,                 BaseFileNameMore,   "." InfixLeadField,                                     "." FILEEXT_RIS         );  // lower precision version (still usable) but can be displayed by Cartool
StringCopy  ( fileleadfieldeeg,                 BaseFileNameMore,   "." InfixLeadField "." InfixDisplay,                    "." FILEEXT_EEGSEF      );  // same

StringCopy  ( filemneis,                        BaseFileName,       "." InfixInverseMN,                                     "." FILEEXT_IS          );
StringCopy  ( filewmneis,                       BaseFileName,       "." InfixInverseWMN,                                    "." FILEEXT_IS          );
StringCopy  ( fileloretais,                     BaseFileName,       "." InfixInverseLoreta,                                 "." FILEEXT_IS          );
StringCopy  ( filelaurais,                      BaseFileName,       "." InfixInverseLaura,                                  "." FILEEXT_IS          );
StringCopy  ( filedaleis,                       BaseFileName,       "." InfixInverseDale,                                   "." FILEEXT_IS          );
StringCopy  ( filesloretais,                    BaseFileName,       "." InfixInverseSLoreta,                                "." FILEEXT_IS          );
StringCopy  ( fileeloretais,                    BaseFileName,       "." InfixInverseELoreta,                                "." FILEEXT_IS          );

StringCopy  ( filemnresmat,                     BaseFileNameMore,   "." InfixInverseMN,         "." InfixResMatrix "." InfixResMatrixPointSpread,   "." FILEEXT_RIS         );
StringCopy  ( filewmnresmat,                    BaseFileNameMore,   "." InfixInverseWMN,        "." InfixResMatrix "." InfixResMatrixPointSpread,   "." FILEEXT_RIS         );
StringCopy  ( fileloretaresmat,                 BaseFileNameMore,   "." InfixInverseLoreta,     "." InfixResMatrix "." InfixResMatrixPointSpread,   "." FILEEXT_RIS         );
StringCopy  ( filelauraresmat,                  BaseFileNameMore,   "." InfixInverseLaura,      "." InfixResMatrix "." InfixResMatrixPointSpread,   "." FILEEXT_RIS         );
StringCopy  ( filedaleresmat,                   BaseFileNameMore,   "." InfixInverseDale,       "." InfixResMatrix "." InfixResMatrixPointSpread,   "." FILEEXT_RIS         );
StringCopy  ( filesloretaresmat,                BaseFileNameMore,   "." InfixInverseSLoreta,    "." InfixResMatrix "." InfixResMatrixPointSpread,   "." FILEEXT_RIS         );
StringCopy  ( fileeloretaresmat,                BaseFileNameMore,   "." InfixInverseELoreta,    "." InfixResMatrix "." InfixResMatrixPointSpread,   "." FILEEXT_RIS         );

StringCopy  ( fileradii,                        BaseFileNameMore,   "." "Thicknesses"                                       "." FILEEXT_EEGSEF      );
StringCopy  ( fileradiilayers,                  BaseFileNameMore,   "." "Boundaries.All Layers",                            "." FILEEXT_ELS         );

StringCopy  ( filelmall,                        BaseFileName,       "." "All",                                              "." FILEEXT_LM          );
StringCopy  ( filelmxyz,                        BaseFileName,       "." "XYZ",                                              "." FILEEXT_LM          );
StringCopy  ( filelmis,                         BaseFileName,       "." "IS",                                               "." FILEEXT_LM          );
StringCopy  ( filelmris,                        BaseFileName,       "." "RIS",                                              "." FILEEXT_LM          );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TPoints             loadsolpoints;
TStrings            loadsolpointsnames;

TPoints             leadfieldsolpoints;
//TStrings          leadfieldsolpointsnames;

TPoints             solpoints;                  // points actually used
TStrings            solpointsnames;
//TPoints           solpointsspherized;
TPoints             solpointsgrid;              // points still aligned on a grid, so distances computation are fine

TPoints             headsurfacepoints;          // head surface points used to compute the geometrical head model

TPoints             xyzpoints;
TStrings            xyznames;
TPoints             xyzpointsdisplay;
//TPoints           xyzpointsspherized;
TPoints             xyzpointscoregtonorm;

                                        // Load the appropriate solution points file(s)
if ( FileInputSpLoad.IsNotEmpty () )

    loadsolpoints     .ReadFile ( FileInputSpLoad, &loadsolpointsnames );

                                        // Cases where we need the SPs from the LF
if ( FileInputSpLeadField.IsNotEmpty () )

    leadfieldsolpoints.ReadFile ( FileInputSpLeadField /*, &leadfieldsolpointsnames*/ );    // names not need


int                 numleadfieldsolpoints   = (int) leadfieldsolpoints;

                                        
if ( samespsaslf )
                                        // !not tested when resolution has been given!
    if (    samespsaslfmaxsolpoints == 0                                // if field is 0 or empty, allow full loading
         || samespsaslfmaxsolpoints > numleadfieldsolpoints * 0.98 )    // don't be too picky on the downsampling, like targetting 5000 but LF is 5015

        samespsaslfmaxsolpoints = numleadfieldsolpoints;

                                        // now test if the target number of SPs is still under the LF
bool                downsampleleadfield     = samespsaslf && samespsaslfmaxsolpoints < numleadfieldsolpoints;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // build some .lm files

                                        // XYZ only
if ( xyzprocessing ) {

    ofstream            olm ( TFileName ( filelmxyz, TFilenameExtendedPath ) );

                                        olm << FileOriginalXyzDisplay   << NewLine;
    }
else
    ClearString ( filelmxyz );


                                        // inverse only
if (   CrisTransfer.IsInverseProcessing () 
    || hasgrey 
    || hassps ) {

    ofstream            olm ( TFileName ( filelmis, TFilenameExtendedPath ) );
                                        // MRIs
    if ( hasgrey )                      olm << FileOriginalMriGrey      << NewLine;
    if ( CrisTransfer.HasBrainMri () )  olm << FileOriginalMriBrain     << NewLine;
    if ( CrisTransfer.HasHeadMri () )   olm << FileOriginalMriHead      << NewLine;
                                        // SPIs
    if ( hassps )                       olm << FileOriginalSp           << NewLine;
                                        // IS
    if ( inversemn      )               olm << filemneis                << NewLine;
    if ( inversewmn     )               olm << filewmneis               << NewLine;
    if ( inversedale    )               olm << filedaleis               << NewLine;
    if ( inversesloreta )               olm << filesloretais            << NewLine;
    if ( inverseloreta  )               olm << fileloretais             << NewLine;
    if ( inverselaura   )               olm << filelaurais              << NewLine;
    if ( inverseeloreta )               olm << fileeloretais            << NewLine;
    }
else
    ClearString ( filelmis );


                                        // MRIs only
if ( (    CrisTransfer.HasHeadMri () 
       || CrisTransfer.HasBrainMri () 
       || hasgrey                     ) && hassps ) {

    ofstream            olm ( TFileName ( filelmris, TFilenameExtendedPath ) );
                                        // MRIs
    if ( hasgrey )                      olm << FileOriginalMriGrey      << NewLine;
    if ( CrisTransfer.HasBrainMri () )  olm << FileOriginalMriBrain     << NewLine;
    if ( CrisTransfer.HasHeadMri () )   olm << FileOriginalMriHead      << NewLine;
                                        // SPIs
    if ( hassps )                       olm << FileOriginalSp           << NewLine;
    }
else
    ClearString ( filelmris );


                                        // all files together
ofstream            olm ( TFileName ( filelmall, TFilenameExtendedPath ) );
                                        // XYZ
//if ( xyzcoregistertomri )             olm << FileOriginalXyzCoreg     << NewLine;
if ( xyzprocessing )                    olm << FileOriginalXyzDisplay   << NewLine;
                                        // MRIs
if ( hasgrey )                          olm << FileOriginalMriGrey      << NewLine;
if ( CrisTransfer.HasBrainMri () )      olm << FileOriginalMriBrain     << NewLine;
if ( isthicknessfromtissues )           olm << FileOriginalMriTissues   << NewLine;
if ( CrisTransfer.HasHeadMri () )       olm << FileOriginalMriHead      << NewLine;
                                        // SPIs
if ( hassps )                           olm << FileOriginalSp           << NewLine;
                                        // IS
if ( inversemn      )                   olm << filemneis                << NewLine;
if ( inversewmn     )                   olm << filewmneis               << NewLine;
if ( inversedale    )                   olm << filedaleis               << NewLine;
if ( inversesloreta )                   olm << filesloretais            << NewLine;
if ( inverseloreta  )                   olm << fileloretais             << NewLine;
if ( inverselaura   )                   olm << filelaurais              << NewLine;
if ( inverseeloreta )                   olm << fileeloretais            << NewLine;

olm.close ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // These will hold the current head, brain, grey etc.. data
                                        // They will correspond to different files according to where in the code: input file, original local copy or reoriented one
TOpenDoc<TVolumeDoc>        MriHead;
TOpenDoc<TVolumeDoc>        MriBrain;
TOpenDoc<TVolumeDoc>        MriBrainOrig;
TOpenDoc<TVolumeDoc>        MriGrey;
TOpenDoc<TVolumeDoc>        MriTissues;
TOpenDoc<TElectrodesDoc>    Model;
TOpenDoc<TElectrodesDoc>    Xyz;

Volume              HeadVolume;

TMatrix44           NormToMRI;              // standard oriented & centered slice -> current MRI
TMatrix44           NormToOriginalMRI;      // Absolute Normalized MRI space -> Relative (voxel) Original MRI space
TMatrix44           RelOriginalMRI_To_AbsOriginalXYZ;
TMatrix44           NormToAbsOriginalMRI;   // Absolute Normalized MRI space -> Absolute Original MRI space
TMatrix44           AbsOriginalMRIToNorm;   // Absolute Original MRI space -> Absolute Normalized MRI space
TMatrix44           XyzCoregToNorm;
TPointDouble        MriCenter;              // Center provided in MRI space
TPointDouble        InverseCenter;          // Optimally chosen center in MRI space
TPointDouble        OriginalMriOrigin;
TVector3Int         OriginalMriSize;
char                OriginalMriOrientation[ 4 ];
TFitModelOnPoints   SurfaceModel;
TPointFloat         MriCenterToInverseCenter;
TMatrix44           xyztonorm;
TMatrix44           normtoxyz;
TMatrix44           mriabstoguillotine;


                                        // reset these variables
NormToMRI.                          SetIdentity ();
NormToOriginalMRI.                  SetIdentity ();
RelOriginalMRI_To_AbsOriginalXYZ.   SetIdentity ();
XyzCoregToNorm.                     SetIdentity ();
ClearString ( OriginalMriOrientation, 4 );
SurfaceModel.Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // For HUMAN cases only
double              age                 = StringToDouble ( CrisTransfer.LeadFieldTargetAge  );  // in [year]

double              skullcond           = StringToDouble ( CrisTransfer.SkullConductivity   );  // absolute conductivity in [S/m]
double              skullcompactcond;
double              skullspongycond;
int                 numlayers           = lfpreset.NumLayers;
TSelection          seltissues;
TArray1<double>     sigma;

                                        // Compute the conductivities of each layer, starting from the deepest one
lfpreset.GetLayersConductivities    (   skullcond,  skullcompactcond,   skullspongycond,
                                        seltissues, sigma   );

                                        // Mean skull thickness curve is for human only AND if user opted in
bool                ajustskullthickness = lfpreset.IsHuman () 
                                       && CrisTransfer.IsAdjustSkullMeanThickness ();

double              skullthickness      = StringToDouble ( CrisTransfer.SkullMeanThickness  );  // in [mm]


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Verbose story
Verbose.Open        ( fileverbose, VerboseFileDefaultWidth );

Verbose.PutTitle    ( "Creating Inverse Solution Matrices" );


if (   CrisTransfer.HasAnyMri () 
    || isthicknessfromtissues ) {

    Verbose.NextTopic ( "MRIs:" );

    if ( FileInputMriHead .IsNotEmpty () )      Verbose.Put ( "Full Head       MRI file:", FileInputMriHead    );
    if ( FileInputMriBrain.IsNotEmpty () )      Verbose.Put ( "Segmented Brain MRI file:", FileInputMriBrain   );
    if ( FileInputMriGrey .IsNotEmpty () )      Verbose.Put ( "Grey Mask       MRI file:", FileInputMriGrey    );
    if ( isthicknessfromtissues )               Verbose.Put ( "Segmented Tissues   file:", FileInputMriTissues );
    Verbose.NextLine ();

    Verbose.Put ( "Correcting Bias Field of Brain MRI:", biasfield );
    Verbose.Put ( "Filtering     MRIs                :", smoothmri );

    Verbose.Put ( "Reorienting   MRIs                :", reorientmris );
    if ( reorientmris ) {
        Verbose.Put ( "Setting origin:", "Middle" );
        Verbose.Put ( "Setting orientation:", NiftiOrientation );
        Verbose.Put ( "Setting best   Sagittal   plane:", setsagittalplanemri );
        Verbose.Put ( "Setting widest Equatorial plane:", setequatorplanemri );
        }
    }


if ( CrisTransfer.IsXyzProcessing () ) {

    Verbose.NextTopic ( "Electrodes Coordinates:" );

    Verbose.Put ( "Electrodes Coordinates file:", FileInputXyz );

    if      ( CrisTransfer.IsXyzCoregistrationDone ()   )   Verbose.Put ( "Coregistration to MRI:", "Already coregistered" );
    else if ( CrisTransfer.IsXyzCoregisterToMriI ()     )   Verbose.Put ( "Coregistration to MRI:", "Interactive" );
    else if ( CrisTransfer.IsXyzCoregisterToMriIRedo () )   Verbose.Put ( "Coregistration to MRI:", "Interactive" );
    }


if ( hasgrey ) {

    Verbose.NextTopic ( "Grey Matter Mask:" );

    Verbose.Put ( "Grey Matter Mask is:",           computegrey ? "Extracted from Brain"                    : "Loaded from file" );

    if ( computegrey ) {
        Verbose.Put ( "Grey Matter Mask thickness:",    GreyMatterProcessingToString ( greyflags ) );
        Verbose.Put ( "Grey Matter Mask symmetry:",     GreyMatterSymmetryToString   ( greyflags ) );
        }
    else
        Verbose.Put ( "Grey Matter Mask file:", FileInputMriGrey );
    }


if ( hassps ) {

    Verbose.NextTopic ( "Solution Points:" );

    if ( computesps ) {

        Verbose.Put ( "Solution Points are:", "Extracted from Grey Matter Mask" );

        if ( numsolpoints > 0 )
            Verbose.Put ( "Requested number of Solution Points:", numsolpoints );
        else
            Verbose.Put ( "Requested Solution Points resolution:", ressolpoints, 2, " [mm]" );

        Verbose.NextLine ();
        Verbose.Put     ( "Points final distribution shape:", ( spflags & GreyMatterSymmetric ) ? "Symmetric" : "Asymmetric" );

        bool        sploreta    = ( spflags & GreyMatterLoretaCheck ) || loretaneighborhood == lauraneighborhood && ( spflags & GreyMatterLauraCheck );
        if ( sploreta )
            Verbose.Put ( "Remove points unfit for LORETA :", sploreta );
        if ( spflags & GreyMatterLauraCheck       )   Verbose.Put ( "Remove points unfit for LAURA  :", (bool) ( spflags & GreyMatterLauraCheck       ) );
        if ( spflags & GreyMatterSinglePointCheck )   Verbose.Put ( "Remove isolated points         :", (bool) ( spflags & GreyMatterSinglePointCheck ) );
        } // computesps

    else if ( samespsaslf ) {

        Verbose.Put ( "Solution Points are:", "Loaded from Lead Field file" );
        Verbose.Put ( "Solution Points file:", FileInputSpLeadField );
        Verbose.Put ( "Solution Points downsampling:", downsampleleadfield );
                                        // check if downsampling to occur
        if ( downsampleleadfield ) {
            Verbose.Put ( "Clipping to a Maximum Points:", samespsaslfmaxsolpoints );
            }
        } // samespsaslf
    else if ( loadsps ) {

        Verbose.Put ( "Solution Points are:", "Loaded from file" );
        Verbose.Put ( "Solution Points file:", FileInputSpLoad );
        } // loadsps
    } // hassps


if ( hasleadfield ) {

    Verbose.NextTopic ( "Lead Field:" );

    if ( computeleadfield ) {

        Verbose.Put ( "Lead Field is:", "Computed from Solution Points and Electrodes" );

        if      ( lfpreset.IsGeometryLSMAC ()        )          Verbose.Put ( "Lead Field Geometrical Model    :", "LSMAC (Locally Spherical Model with Anatomical Constraints)" );

        if      ( lfpreset.IsIsotropic3ShellSphericalAry() )    Verbose.Put ( "Lead Field Electromagnetic Model:", "Isotropic, 3-Shell Model, Ary spherical approximation" );
        else if ( lfpreset.IsIsotropicNShellSpherical () )      Verbose.Put ( "Lead Field Electromagnetic Model:", "Isotropic, N-Shell Model, Exact spherical equations" );
        
                                                                Verbose.Put ( "Lead Field number of layers     :", numlayers );
                                                                Verbose.Put ( "Lead Field tissues thicknesses are:", isthicknessfrommri ? "Estimated from T1 MRI" : "Computed from segmented tissues" );

        Verbose.NextLine ();
        Verbose.Put ( "Targeted species:", lfpreset.IsHuman () ? "Human" : lfpreset.IsMacaque () ? "Macaque monkey" : lfpreset.IsMouse () ? "Mouse" : "Unknown" );

        if ( lfpreset.IsHuman () ) {

            Verbose.Put ( "Targeted age:", age, 1, " [Years]" );
            Verbose.Put ( "Skull absolute conductivity:", skullcond, 4, " [S/m]" );

            if ( lfpreset.NumLayers == 6 ) {
                Verbose.Put ( "Compact skull absolute conductivity:", skullcompactcond, 4, " [S/m]" );
                Verbose.Put ( "Spongy skull absolute conductivity :", skullspongycond,  4, " [S/m]" );
                }
            Verbose.Put ( "Brain to skull conductivity ratio:", sigma[ 0 ] / NonNull ( skullcond ), 2, " : 1" );    // !brain index is 0 in all cases!

            Verbose.Put ( "Adjusting skull mean thickness:", ajustskullthickness );
            if ( ajustskullthickness )
                Verbose.Put ( "Targeted skull mean thickness:", skullthickness, 2, " [mm]" );
            }


        Verbose.NextLine ();

        if ( lfpreset.IsGeometryLSMAC () ) { 
            Verbose.Put ( "Absolute conductivities of each layer:", "in [S/m]" );
            for (int i = 0; i < numlayers; i++) {
                IntegerToString ( buff, i + 1 );
                StringAppend    ( buff, i == 0 ? " (Inner layer)" : i == numlayers - 1 ? " (Outer layer)" : "" );
                Verbose.Put     ( buff, sigma[ i ], 4 );
                }
            } // spherical


        Verbose.NextLine ();
                                        // Scalp radius
        if      ( lfpreset.IsGeometryLSMAC () )                     Verbose.Put ( "Outer absolute scalp radius is:", "Set by electrode" );

                                        // Skull radius
        if      ( lfpreset.IsSkullRadiusFixedRatio      () )    {   Verbose.Put ( "Skull absolute thickness is:",   "Modulated by electrode" );     Verbose.Put ( "Skull relative thickness is:",    "Fixed" );                     }
        else if ( lfpreset.IsSkullRadiusModulatedRatio  () )    {   Verbose.Put ( "Skull absolute thickness is:",   "Fixed" );                      Verbose.Put ( "Skull relative thickness is:",    "Modulated by electrode" );    }
        else if ( isthicknessfrommri                       )    {   Verbose.Put ( "Skull absolute thickness is:",   "Estimated from T1 MRI" );              }
        else if ( isthicknessfromtissues                   )    {   Verbose.Put ( "Skull absolute thickness is:",   "Computed from segmented tissues" );   }


        if ( lfpreset.IsGeometryLSMAC () ) {
                                        // Other radii
            if      ( lfpreset.NumLayers == 4 )                 {   Verbose.Put ( "CSF absolute thickness is:",             isthicknessfrommri ? "Estimated from T1 MRI" : "Computed from segmented tissues" );               }

            else if ( lfpreset.NumLayers == 6 )                 {   Verbose.Put ( "Compact skull absolute thickness is:",   "Computed from skull thickness" /*isthicknessfrommri ? "Estimated from T1 MRI" : "Estimated from segmented tissues"*/ ); 
                                                                    Verbose.Put ( "Spongy skull absolute thickness is:",    "Computed from skull thickness" /*isthicknessfrommri ? "Estimated from T1 MRI" : "Estimated from segmented tissues"*/ ); 
                                                                    Verbose.Put ( "CSF absolute thickness is:",             isthicknessfrommri ? "Estimated from T1 MRI" : "Computed from segmented tissues" );  
                                                                }
            }


        if ( lfpreset.IsSkullRadiusFixedRatio () || lfpreset.IsSkullRadiusModulatedRatio () ) {
            Verbose.NextLine ();
            Verbose.Put ( "Shell radii:", lfpreset.IsSkullRadiusFixedRatio () ? "Constants in spherical coordinates" : "Constants in real coordinates" );
            Verbose.Put ( "Head relative radius       :", 1.0,                       3 );
            Verbose.Put ( "Outer skull relative radius:", lfpreset.OuterSkullRadius, 3 );
            Verbose.Put ( "Inner skull relative radius:", lfpreset.InnerSkullRadius, 3 );
            }

        } // computeleadfield

    else if ( loadleadfield ) {
        Verbose.Put ( "Lead Field is:", "Loaded from file" );
        Verbose.Put ( "Lead Field file:", FileInputLeadFieldLoad );
        Verbose.Put ( "Associated Solution Points file:", FileInputSpLeadField );


        Verbose.NextLine ();
        Verbose.Put ( "Lead Field processing:", interpolateleadfield ? "Interpolation" : downsampleleadfield ? "Downsampling" : "None" );

        if ( interpolateleadfield )
            Verbose.Put ( "Interpolation method:", "Linear" );
        } // loadleadfield
    } // hasleadfield


if ( CrisTransfer.IsInverseProcessing () ) {

    Verbose.NextTopic ( "Inverse Solution:" );

    bool            previous        = false;

    if ( inversemn ) {
        if ( previous )     Verbose.NextLine ();
        previous    = true;

        Verbose.Put ( "Inverse Solution model:", /*InfixInverseMN*/ "Minimum Norm" );

        Verbose.Put ( "Regularization     :", regularization );
        if ( regularization ) {
            Verbose.Put ( "Regularization step:", "Max Eigenvalue ( K.Kt ) / ", EigenvalueToRegularizationFactorMN );
            Verbose.Put ( "Regularization from:", FirstRegularization );
            Verbose.Put ( "Regularization to  :", LastRegularization  );
            }
        }

    if ( inversewmn ) {
        if ( previous )     Verbose.NextLine ();
        previous    = true;

        Verbose.Put ( "Inverse Solution model:", /*InfixInverseWMN*/ "Weighted Minimum Norm" );

        Verbose.Put ( "Regularization     :", regularization );
        if ( regularization ) {
            Verbose.Put ( "Regularization step:", "Max Eigenvalue ( K.W.Kt ) / ", EigenvalueToRegularizationFactorWMN );
            Verbose.Put ( "Regularization from:", FirstRegularization );
            Verbose.Put ( "Regularization to  :", LastRegularization  );
            }
        }

    if ( inverseloreta ) {
        if ( previous )     Verbose.NextLine ();
        previous    = true;

        Verbose.Put ( "Inverse Solution model:", InfixInverseLoreta );

        Verbose.Put ( "LORETA neighborhood:", Neighborhood[ loretaneighborhood ].Name );
        if ( loretaneighborhood == Neighbors26 && IsNeighbors26To18 ( neighbors26to18 ) ) {
            Verbose.Put ( "LORETA neighborhood:", "Recentered to 18 Neighbors" );
//          Verbose.Put ( "LORETA neighborhood strictly recentered:", IsNeighbors26To18Strict ( neighbors26to18 ) );
            }

        Verbose.Put ( "Regularization     :", regularization );
        if ( regularization ) {
            Verbose.Put ( "Regularization step:", "Max Eigenvalue ( K.W.Kt ) / ", EigenvalueToRegularizationFactorLORETA );
            Verbose.Put ( "Regularization from:", FirstRegularization );
            Verbose.Put ( "Regularization to  :", LastRegularization  );
            }
        }

    if ( inverselaura ) {
        if ( previous )     Verbose.NextLine ();
        previous    = true;

        Verbose.Put ( "Inverse Solution model:", InfixInverseLaura );

        Verbose.Put ( "LAURA neighborhood:", Neighborhood[ lauraneighborhood ].Name );
        if ( lauraneighborhood == Neighbors26 && IsNeighbors26To18 ( neighbors26to18 ) ) {
            Verbose.Put ( "LAURA neighborhood:", "Recentered to 18 Neighbors" );
//          Verbose.Put ( "LAURA neighborhood strictly recentered:", IsNeighbors26To18Strict ( neighbors26to18 ) );
            }
        Verbose.Put ( "LAURA power:", LauraPower );

        Verbose.Put ( "Regularization     :", regularization );
        if ( regularization ) {
            Verbose.Put ( "Regularization step:", "Max Eigenvalue ( K.W.Kt ) / ", EigenvalueToRegularizationFactorLAURA );
            Verbose.Put ( "Regularization from:", FirstRegularization );
            Verbose.Put ( "Regularization to  :", LastRegularization  );
            }
        }

    if ( inversedale ) {
        if ( previous )     Verbose.NextLine ();
        previous    = true;

        Verbose.Put ( "Inverse Solution model:", InfixInverseDale );

        Verbose.Put ( "Regularization     :", regularization );
        if ( regularization ) {
            Verbose.Put ( "Regularization step:", "Max Eigenvalue ( K.Kt ) / ", EigenvalueToRegularizationFactorDale );
            Verbose.Put ( "Regularization from:", FirstRegularization );
            Verbose.Put ( "Regularization to  :", LastRegularization  );
            }
        }

    if ( inversesloreta ) {
        if ( previous )     Verbose.NextLine ();
        previous    = true;

        Verbose.Put ( "Inverse Solution model:", InfixInverseSLoreta );

        Verbose.Put ( "Regularization     :", regularization );
        if ( regularization ) {
            Verbose.Put ( "Regularization step:", "Max Eigenvalue ( K.Kt ) / ", EigenvalueToRegularizationFactorSLORETA );
            Verbose.Put ( "Regularization from:", FirstRegularization );
            Verbose.Put ( "Regularization to  :", LastRegularization  );
            }
        }

    if ( inverseeloreta ) {
        if ( previous )     Verbose.NextLine ();
        previous    = true;

        Verbose.Put ( "Inverse Solution model:", InfixInverseELoreta );

        Verbose.Put ( "Regularization     :", regularization );
        if ( regularization ) {
            Verbose.Put ( "Regularization step:", "Max Eigenvalue ( K.Kt ) / ", EigenvalueToRegularizationFactorELORETA );
            Verbose.Put ( "Regularization from:", FirstRegularization );
            Verbose.Put ( "Regularization to  :", LastRegularization  );
            }
        }
    }


Verbose.NextTopic ( "Output Files:" );
{
Verbose.Put ( "Verbose File (this):", fileverbose );

Verbose.NextLine ();
Verbose.Put ( "Complimentary Link Many file:", filelmall );

if ( filelmxyz.IsNotEmpty () )
    Verbose.Put ( "Complimentary Link Many file:", filelmxyz );

if ( filelmis.IsNotEmpty () )
    Verbose.Put ( "Complimentary Link Many file:", filelmis );

if ( filelmris.IsNotEmpty () )
    Verbose.Put ( "Complimentary Link Many file:", filelmris );


if ( CrisTransfer.HasAnyMri () )
    Verbose.NextLine ();

if ( FileInputMriHead.IsNotEmpty () )
    Verbose.Put ( "Full head         MRI file:", FileOriginalMriHead );

if ( FileInputMriBrain.IsNotEmpty () )
    Verbose.Put ( "Brain             MRI file:", FileOriginalMriBrain );

if ( hasgrey )
    Verbose.Put ( "Grey  mask        MRI file:", FileOriginalMriGrey );

if ( isthicknessfromtissues )
    Verbose.Put ( "Segmented Tissues     file:", FileOriginalMriTissues );

if ( isthicknessfrommri
  || isthicknessfromtissues ) {

    Verbose.NextLine ();
    Verbose.Put ( "Skull Thicknesses file:",     fileradii  );
    Verbose.Put ( "All layers boundaries file:", reorientmris ? FileLocalRadiiLayers : fileradiilayers );
    }


if ( getheadmodel ) {
    Verbose.NextLine ();
    Verbose.Put ( "Head Model 3D file:", file3Dheadmodel );
    }


if ( hassps ) {
    Verbose.NextLine ();
    Verbose.Put ( "Solution Points file:", FileOriginalSp );
    }


if ( xyzprocessing )
    Verbose.NextLine ();

if     ( xyzcoregistertomri )
    Verbose.Put ( "Electrode Coordinates file:",            FileOriginalXyzCoreg );
else if ( xyzcoregistrationdone )
    Verbose.Put ( "Electrode Coordinates file:",            FileOriginalXyz );

if ( xyzprocessing )
    Verbose.Put ( "Electrode Coordinates file for display:",FileOriginalXyzDisplay );


if ( reorientmris ) {

    Verbose.NextLine ();

    if ( FileInputMriHead.IsNotEmpty () )
        Verbose.Put ( "Reoriented Full head         MRI file:", FileLocalMriHead );

    if ( FileInputMriBrain.IsNotEmpty () )
        Verbose.Put ( "Reoriented Brain             MRI file:", FileLocalMriBrain );

    if ( hasgrey )
        Verbose.Put ( "Reoriented Grey  mask        MRI file:", FileLocalMriGrey );

    if ( isthicknessfromtissues )
        Verbose.Put ( "Segmented Reoriented Tissues     file:", FileLocalMriTissues );

    if ( hassps )
        Verbose.Put ( "Reoriented Solution Points       file:", FileLocalSp );

    if     ( xyzcoregistertomri )
        Verbose.Put ( "Reoriented Electrode Coordinates file:",            FileLocalXyzCoreg    );
    else if ( xyzcoregistrationdone )
        Verbose.Put ( "Reoriented Electrode Coordinates file:",            FileLocalXyz    );

    if ( xyzprocessing )
        Verbose.Put ( "Reoriented Electr. Coord. for display:",FileLocalXyzDisplay    );
    }


if ( hasleadfield ) {
    Verbose.NextLine ();
    Verbose.Put ( "Lead Field file:",               fileleadfield       );
    Verbose.Put ( "Lead Field file:",               fileleadfieldris    );
    Verbose.Put ( "Lead Field file for display:",   fileleadfieldeeg    );
    }


if ( CrisTransfer.IsInverseProcessing () )
    Verbose.NextLine ();
if ( inversemn          )   Verbose.Put ( "Matrix file Minimum Norm:",              filemneis       );
if ( inversewmn         )   Verbose.Put ( "Matrix file Weighted Minimum Norm:",     filewmneis      );
if ( inversedale        )   Verbose.Put ( "Matrix file " InfixInverseDale    ":",   filedaleis      );
if ( inversesloreta     )   Verbose.Put ( "Matrix file " InfixInverseSLoreta ":",   filesloretais   );
if ( inverseloreta      )   Verbose.Put ( "Matrix file " InfixInverseLoreta  ":",   fileloretais    );
if ( inverselaura       )   Verbose.Put ( "Matrix file " InfixInverseLaura   ":",   filelaurais     );
if ( inverseeloreta     )   Verbose.Put ( "Matrix file " InfixInverseELoreta ":",   fileeloretais   );

if ( computeresmat ) {
    Verbose.NextLine ();
    if ( inversemn          )   Verbose.Put ( "Resolution Matrix file MN:",                         filemnresmat        );
    if ( inversewmn         )   Verbose.Put ( "Resolution Matrix file WMN:",                        filewmnresmat       );
    if ( inversedale        )   Verbose.Put ( "Resolution Matrix file " InfixInverseDale    ":",    filedaleresmat      );
    if ( inversesloreta     )   Verbose.Put ( "Resolution Matrix file " InfixInverseSLoreta ":",    filesloretaresmat   );
    if ( inverseloreta      )   Verbose.Put ( "Resolution Matrix file " InfixInverseLoreta  ":",    fileloretaresmat    );
    if ( inverselaura       )   Verbose.Put ( "Resolution Matrix file " InfixInverseLaura   ":",    filelauraresmat     );
    if ( inverseeloreta     )   Verbose.Put ( "Resolution Matrix file " InfixInverseELoreta ":",    fileeloretaresmat   );
    }

}


//Verbose.NextLine ( 2 );


Verbose.Flush ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
enum                {
                    gaugecimglobal,

                    gaugecimmrifull,
                    gaugecimmribrain,
                    gaugecimmrigrey,
                    gaugecimmritissues,
                    gaugecimmriproc,

                    gaugecimxyz,

                    gaugecimsp,

                    gaugecimheadmodel,
                    gaugecimleadfield,
                    gaugeciminverse,
                    };


//if ( ! ( interactive || xyzcoregistertomri ) )
//                                        // shrinking size to keep the progress bar under observation
//    WindowSetGaugeSize ( CartoolMainWindow ); 


TSuperGauge         Gauge;
Gauge.Set           ( CreatingInverseTitle, SuperGaugeLevelBatch );

Gauge.AddPart       ( gaugecimglobal,                                     5,            1 );

Gauge.AddPart       ( gaugecimmrifull,      CrisTransfer.HasHeadMri  () ? 3 + 2 : 0,    1 );
Gauge.AddPart       ( gaugecimmribrain,     CrisTransfer.HasBrainMri () ? 2 + 2 : 0,    1 );
Gauge.AddPart       ( gaugecimmrigrey,      hasgrey ? computegrey       ? 6   : 2 + 2 : 0,    1 );
Gauge.AddPart       ( gaugecimmritissues,   isthicknessfromtissues      ? 4 : 0,        1 );
Gauge.AddPart       ( gaugecimmriproc,      reorientmris                ? 7     : 0,    2 );

Gauge.AddPart       ( gaugecimxyz,          xyzprocessing               ? 1     : 0,    1 );

Gauge.AddPart       ( gaugecimsp,
                                            ( computesps                ? 2     : 0 )
                                          + ( samespsaslf || loadsps    ? 4     : 0 )
                                          + ( hassps                    ? 1     : 0 ),  2 );

Gauge.AddPart       ( gaugecimheadmodel,    getheadmodel                ? 1 + 3 : 0,    2 );
Gauge.AddPart       ( gaugecimleadfield,
                                            ( computeleadfield          ? 3 + ( isthicknessfrommri ? 7 : 0 ) : 0 )
                                          + ( loadleadfield             ? 3     : 0 )
                                          + ( leadfieldprocessing       ? 1     : 0 ),  5 );

Gauge.AddPart       ( gaugeciminverse,      inverseprocessing           ? 1 + 2 * numinversesmat : 0,   5 );

                                        // rescale ranges while ignoring the null ones
Gauge.AdjustOccupy ( true );


CartoolApplication->SetMainTitle ( CreatingInverseTitle, BaseDir, Gauge );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
    spsrejected use through code execution:

                    Compute SPs             Load SPs                Same SPs as LF
------------------|-----------------------|-----------------------|--------------------------------------------
Solution Points
    Compute LF      Empty                   Empty                   Points outside grid
    Load    LF      Missing 8 neighbors     Missing 8 neighbors     Points outside grid
                                                                    (downsampling SPs)
Lead Field
    Compute LF      Empty                   Empty                   Points outside grid, then set new size & empty
    Load    LF      Missing 8 neighbors     Missing 8 neighbors     Points outside grid, then set new size & empty
                    (interpolating LF)      (interpolating LF)      (downsampling LF)
Inversion
    All             Add points with null Lead Field columns

    LORETA          Add isolated points
    LAURA           Add isolated points
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // A few variables needed later on
double              fullvolumerescale   = 1;
bool                mritransformed      = false;
TSelection          spsrejected;        // used in cases: computesps && loadleadfield / samespsaslf / loadsps && interpolateleadfield

//int                 outputnumsolp       = 0;
AMatrix             K;                  // Lead Field, a.k.a. forward solution, is here


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Opening MRIs
                                        // full MRI?
if ( CrisTransfer.HasHeadMri () ) {

    Gauge.Next ( gaugecimmrifull, SuperGaugeUpdateTitle );


    MriHead.Open ( FileInputMriHead, interactive || xyzcoregistertomri ? OpenDocVisibleLocked : OpenDocHidden );

                                        // save the real original size & origin
    OriginalMriOrigin   = MriHead->GetOrigin ();
    MriHead->GetData ()->GetDims ( OriginalMriSize        );
    MriHead->OrientationToString ( OriginalMriOrientation );


    Gauge.Next ( gaugecimmrifull, SuperGaugeUpdateTitle );


    if ( interactive && ! GetAnswerFromUser ( reorientmris ? "Is the Full Head MRI orientation detection OK (Anterior, Superior..)?" NewLine 
                                                             NewLine 
                                                             "(check within the MRI window)"
                                                           : "Is the head MRI in 'RAS' orientation?" NewLine 
                                                             NewLine 
                                                             Tab "X -> Right"    NewLine 
                                                             Tab "Y -> Anterior" NewLine 
                                                             Tab "Z -> Superior" NewLine 
                                                             NewLine 
                                                             "(check within the MRI window)", MriHead->GetTitle (), this ) ) {

        ShowMessage ( "Please, manually re-orient the full MRI, then come back here." NewLine 
                      NewLine 
                      "This will end your processing now, bye bye...", MriHead->GetTitle (), ShowMessageWarning, this );

        goto endprocessing;
        }


    MriHead->MinimizeViews ();

                                        // have local copy for us to mess with
    Gauge.Next ( gaugecimmrifull, SuperGaugeUpdateTitle );


    GetVolumeCopy ( MriHead, HeadVolume, smoothmri, fullvolumerescale );
    } // load Head


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // brain MRI?
if ( CrisTransfer.HasBrainMri () ) {

    Gauge.Next ( gaugecimmribrain, SuperGaugeUpdateTitle );


    MriBrain.Open ( FileInputMriBrain, interactive ? OpenDocVisibleLocked : OpenDocHidden );

                                        // getting the brain origin
    TPointDouble        OriginalBrainMriOrigin  = MriBrain->GetOrigin ();

                                        // oopsie, Full Head and Brain appear to have different origins - that shouldn't happen
    if ( ( OriginalBrainMriOrigin - OriginalMriOrigin ).Norm () > 0.001 ) {

        if ( interactive ) {

            ShowMessage ( "It seems that the Brain and the Full Head MRIs don't have the same origin!" NewLine 
                          "Check both MRIs, and set a single common origin in both files." NewLine 
                          NewLine 
                          "This will end your processing now, bye bye...", MriBrain->GetTitle (), ShowMessageWarning, this );

            goto endprocessing;
            }
        else {
                                        // otherwise, try to set the null one from the non-null one
            if      ( OriginalMriOrigin     .IsNull () )    OriginalMriOrigin       = OriginalBrainMriOrigin;
//          else if ( OriginalBrainMriOrigin.IsNull () )    OriginalBrainMriOrigin  = OriginalMriOrigin;    // not used
                                        // not null, but only one is fishy?
            else if (    ! MriHead ->IsValidOrigin () 
                      &&   MriBrain->IsValidOrigin () )     OriginalMriOrigin       = OriginalBrainMriOrigin;

            else if (      MriHead ->IsValidOrigin () 
                      && ! MriBrain->IsValidOrigin () )     OriginalBrainMriOrigin  = OriginalMriOrigin;

                                        // reset origin, which will trigger normalization if not already set
            else                                            OriginalMriOrigin.Reset ();
            }
        }


    Gauge.Next ( gaugecimmribrain, SuperGaugeUpdateTitle );


    if ( interactive && ! GetAnswerFromUser ( reorientmris ? "Is the Brain MRI orientation detection OK (Anterior, Superior..)?" NewLine
                                                             NewLine 
                                                             "(check within the MRI window)"
                                                           : "Is the brain MRI in 'RAS' orientation?" NewLine 
                                                             NewLine 
                                                             Tab "X -> Right"    NewLine 
                                                             Tab "Y -> Anterior" NewLine 
                                                             Tab "Z -> Superior" NewLine 
                                                             NewLine 
                                                             "(check within the MRI window)", MriBrain->GetTitle (), this ) ) {

        ShowMessage ( "Please, manually re-orient the brain MRI, then come back here." NewLine 
                      NewLine 
                      "This will end your processing now, bye bye...", MriBrain->GetTitle (), ShowMessageWarning, this );

        goto endprocessing;
        }


    MriBrain    ->MinimizeViews ();
    } // loading Brain


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // grey MRI?
if ( CrisTransfer.HasGreyMri () ) {

    Gauge.Next ( gaugecimmrigrey, SuperGaugeUpdateTitle );

    MriGrey.Open ( FileInputMriGrey, interactive ? OpenDocVisibleLocked : OpenDocHidden );


    Gauge.Next ( gaugecimmrigrey, SuperGaugeUpdateTitle );

    if ( interactive && ! GetAnswerFromUser ( reorientmris ? "Is the Grey Mask orientation detection OK (Anterior, Superior..)?" NewLine 
                                                             NewLine 
                                                             "(check within the MRI window)"
                                                           : "Is the grey mask MRI in 'RAS' orientation?" NewLine 
                                                             NewLine 
                                                             Tab "X -> Right"    NewLine 
                                                             Tab "Y -> Anterior" NewLine 
                                                             Tab "Z -> Superior" NewLine 
                                                             NewLine 
                                                             "(check within the MRI window)", MriGrey->GetTitle (), this ) ) {

        ShowMessage ( "Please, manually re-orient the grey mask MRI, then come back here." NewLine 
                      NewLine 
                      "This will end your processing now, bye bye...", MriGrey->GetTitle (), ShowMessageWarning, this );

        goto endprocessing;
        }


    MriGrey     ->MinimizeViews ();
    } // loading Grey


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Tissues file?
if ( isthicknessfromtissues ) {

    Gauge.Next ( gaugecimmritissues, SuperGaugeUpdateTitle );

    MriTissues.Open ( FileInputMriTissues, interactive ? OpenDocVisibleLocked : OpenDocHidden );


    Gauge.Next ( gaugecimmritissues, SuperGaugeUpdateTitle );

    if ( interactive && ! GetAnswerFromUser ( reorientmris ? "Is the Tissues volume orientation detection OK (Anterior, Superior..)?" NewLine 
                                                             NewLine 
                                                             "(check within the MRI window)"
                                                           : "Is the tissues volume in 'RAS' orientation?" NewLine 
                                                             NewLine 
                                                             Tab "X -> Right"    NewLine 
                                                             Tab "Y -> Anterior" NewLine 
                                                             Tab "Z -> Superior" NewLine 
                                                             NewLine 
                                                             "(check within the MRI window)", MriTissues->GetTitle (), this ) ) {

        ShowMessage ( "Please, manually re-orient the tissues volume, then come back here." NewLine 
                      NewLine 
                      "This will end your processing now, bye bye...", MriTissues->GetTitle (), ShowMessageWarning, this );

        goto endprocessing;
        }


    MriTissues  ->MinimizeViews ();
    } // loading Grey


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Save / copy all input documents as local files, so nothing can happen to the original ones
if ( MriHead.IsOpen () ) {

    Gauge.Next ( gaugecimmrifull, SuperGaugeUpdateTitle );

    if ( IsExtension ( FileInputMriHead, ToExtension ( FileOriginalMriHead ) ) )
                                        // Faster, and could preserve some original peculiarities
        CopyFileExtended ( FileInputMriHead, FileOriginalMriHead );

    else
                                        // write the file to the new location
        MriHead->ExportDownsampled ( FileOriginalMriHead );

                                        // reload document
    MriHead.Close ();

    Gauge.Next ( gaugecimmrifull, SuperGaugeUpdateTitle );

    MriHead.Open ( FileOriginalMriHead, interactive || xyzcoregistertomri ? OpenDocVisibleLocked : OpenDocHidden );

    MriHead->MinimizeViews ();
    }


if ( MriBrain.IsOpen () ) {

    Gauge.Next ( gaugecimmribrain, SuperGaugeUpdateTitle );

                                        // user might have inputted a grey mask (yes, these things happened), BFC is therefore useless
    bool        dobiasfield     = biasfield && ! MriBrain->IsMask ();


    if ( ! dobiasfield && IsExtension ( FileInputMriBrain, ToExtension ( FileOriginalMriBrain ) ) )
                                        // Faster, and could preserve some original peculiarities
        CopyFileExtended ( FileInputMriBrain, FileOriginalMriBrain );

    else {

        if ( dobiasfield ) {

            FctParams           p;
            p ( FilterParamBiasFieldRepeat )     = 1;
            MriBrain->GetData ()->Filter ( FilterTypeBiasField, p, SuperGaugeUpdateTitle );
            }

                                        // will save the BFC, or at least write the file to the new location
        MriBrain->ExportDownsampled ( FileOriginalMriBrain );
        }

                                        // reload document
    MriBrain.Close ();

    Gauge.Next ( gaugecimmribrain, SuperGaugeUpdateTitle );

    MriBrain.Open ( FileOriginalMriBrain, interactive ? OpenDocVisibleLocked : OpenDocHidden );

    MriBrain->MinimizeViews ();
    }


if ( MriGrey.IsOpen () ) {

    Gauge.Next ( gaugecimmrigrey, SuperGaugeUpdateTitle );

    if ( IsExtension ( FileInputMriGrey, ToExtension ( FileOriginalMriGrey ) ) )
                                        // Faster, and could preserve some original peculiarities
        CopyFileExtended ( FileInputMriGrey, FileOriginalMriGrey );

    else
                                        // write the file to the new location
        MriGrey->ExportDownsampled ( FileOriginalMriGrey );

                                        // reload document
    MriGrey.Close ();

    Gauge.Next ( gaugecimmrigrey, SuperGaugeUpdateTitle );

    MriGrey.Open ( FileOriginalMriGrey, interactive ? OpenDocVisibleLocked : OpenDocHidden );

    MriGrey->MinimizeViews ();
    }


if ( MriTissues.IsOpen () ) {

    Gauge.Next ( gaugecimmritissues, SuperGaugeUpdateTitle );

    if ( IsExtension ( FileInputMriTissues, ToExtension ( FileOriginalMriTissues ) ) )
                                        // Faster, and could preserve some original peculiarities
        CopyFileExtended ( FileInputMriTissues, FileOriginalMriTissues, FILEEXT_TXT );

    else {
                                        // write the file to the new location
        MriTissues->ExportDownsampled ( FileOriginalMriTissues );

                                        // manually copying the associated text file
        TFileName           fileinputtissuestxt     = FileInputMriTissues;
        TFileName           fileoriginaltissuestxt  = FileOriginalMriTissues;

        ReplaceExtension ( fileinputtissuestxt,    FILEEXT_TXT );
        ReplaceExtension ( fileoriginaltissuestxt, FILEEXT_TXT );

        CopyFileExtended ( fileinputtissuestxt, fileoriginaltissuestxt );

//      TFileName           fileolocaltissuestxt    = FileLocalMriTissues;
//      ReplaceExtension ( fileolocaltissuestxt, FILEEXT_TXT );
//      CopyFileExtended ( fileinputtissuestxt, fileolocaltissuestxt );
        }

                                        // reload document
    MriTissues.Close ();

    Gauge.Next ( gaugecimmritissues, SuperGaugeUpdateTitle );

    MriTissues.Open ( FileOriginalMriTissues, interactive ? OpenDocVisibleLocked : OpenDocHidden );

    MriTissues->MinimizeViews ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check Head / Brain consistency
                                        // This is off now, as Bias Field Correction alter so much the brain levels that no matter what, it will think it is badly aligned...
/*
if ( MriHead.IsOpen () && MriBrain.IsOpen () ) {
                                        // Check brain is actually included into head
    if ( ! (    *MriHead->GetSize () == *MriBrain->GetSize ()

             &&  MriHead->GetData ()->IsIncluding ( *MriBrain->GetData (),
                                                     MriHead ->IsMask () || MriBrain->IsMask (),
                                                     MriHead ->GetBackgroundValue (),
                                                     MriBrain->GetBackgroundValue ()  ) ) ) {

        if ( interactive ) {

            if ( ! GetAnswerFromUser ( "It seems that the Brain MRI does not fit exactly into the Full Head MRI," NewLine 
                                       "it would be wise to check this (orientation, coregistration, size...)" NewLine 
                                       "or re-do the Skull-Stripping." NewLine 
                                       NewLine 
                                       "Do you want to proceed anyway (not recommended)?", MriBrain->GetTitle (), this ) )

                goto endprocessing;
            }
                                        // NOT aborting, trust the user in batch mode(?)
        else {
            Verbose.NextBlock ();
            Verbose.Put ( "MRIs compatibility:", "Full Head MRI and Brain MRI are not exactly fitting, this will stop the computation!" );
            Verbose.Flush ();

            ShowMessage ( "It seems that the Brain MRI does not fit exactly into the Full Head MRI," NewLine
                          "it would be wise to check this (orientation, coregistration, size...)" NewLine 
                          "or re-do the Skull-Stripping." NewLine 
                          NewLine 
                          "This will end your processing now, bye bye...", MriBrain->GetTitle (), ShowMessageWarning, this );

            goto endprocessing;
            }
        }
    }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here we know OriginalMriOrigin (and if irrelevant, set to 0)

                                        // all in one shift: origin translation + reverting the downsampling shift
RelOriginalMRI_To_AbsOriginalXYZ.Translate ( -OriginalMriOrigin.X,
                                             -OriginalMriOrigin.Y,
                                             -OriginalMriOrigin.Z, MultiplyLeft );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Summary of transformation matrices:
                                        // Absolute: real space coordinates, centered on origin 
                                        // Relative: voxel coordinates, from corner (0,0,0)

// NormToMRI (Absolute Realigned downsampled MRI to Relative downsampled MRI):
//      * Adjustment rotations (if any)
//      * Reorientation rotations (if any)
//      + Normalized center translation
//
// NormToOriginalMRI (Absolute Realigned downsampled MRI to Relative original MRI) adds, if downsampling occured:
//      * Scaling back by downsampling factor
//      + Original MRI center truncation error
//      - Downsampling shift error (MRI only)
//
// RelOriginalMRI_To_AbsOriginalXYZ to convert from Relative original MRI to Absolute original XYZ:
//      - Original MRI center
//      + Downsampling shift error (to cancel the MRI only translation)


                                        // !If no MRI preprocessing, but origin is null, force reorientation so we can actually compute it!
if ( MriHead.IsOpen () /*&& MriBrain.IsOpen ()*/ && OriginalMriOrigin.IsNull () ) {
    
    mriprocessing   = true;             // set a for default origin
    reorientmris    = true;             // apply origin

    Verbose.NextLine ();
    Verbose.Put ( "Origin is null, forcing MRIs reorientation:", reorientmris );
    Verbose.Flush ();
    }

                                        // Setting default variables
if ( MriHead.IsOpen () && mriprocessing ) {
                                                                        // estimating center?
    bool    estimatecenter = reorientmris && realignmri
                          || OriginalMriOrigin.IsNull () ;

    GetNormalizationTransform   (   MriHead,        MriBrain, 
                                    estimatecenter, MriCenter, 
                                    &NormToMRI,     0 
                                );

//  NormToMRI.Show ( "NormToMRI" );
//  MriCenter.Show ( "MriCenter after initial GetNormalizationTransform" );

                                        // save transformation
    if ( reorientmris )     NormToOriginalMRI.SetIdentity ();
    else                    NormToOriginalMRI   = NormToMRI;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Original files were loaded and copied to output directory
                                        // But there might be some additional pre-processing needed, like re-alignment
if ( reorientmris ) {

    Gauge.Next ( gaugecimmriproc, SuperGaugeUpdateTitle );
                                        // full head could be used here, using wider / full shape of it rather than small sagittal slice
    if ( setsagittalplanemri ) {
                                        // set NormToMRI & MriCenter
                                        // better to use the brain, when available
        SetSagittalPlaneMri (   MriHead, 
                                MriHead->GetStandardOrientation ( LocalToRAS ),
                                NormToMRI,  // input & output
                                MriCenter,  // input & output
                                0,
                                "Sagittal Plane Search"
                            );

//      MriCenter.Show ( "MriCenter after Sagittal" );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Gauge.Next ( gaugecimmriproc, SuperGaugeUpdateTitle );

    if ( setequatorplanemri && MriBrain.IsOpen () ) {

                                        // updates NormToMRI & MriCenter
        SetTransversePlaneMri   (   MriBrain, 
                                    MriBrain->GetStandardOrientation ( LocalToRAS ),
                                    NormToMRI,  // input & output
                                    MriCenter,  // input & output
                                    0,
                                    "Transverse Plane"
                                );

//      MriCenter.Show ( "MriCenter after Equator" );
        }

                                        // save transformation
    NormToOriginalMRI   = NormToMRI;


//  MriCenter.Show ( "MriCenter after final Sagittal / Transverse" );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Overwrite MRIs
    Gauge.Next ( gaugecimmriproc, SuperGaugeUpdateTitle );

                                           // nothing                                     discrete values                                   continuous values
    FilterTypes         filtertype          = ! realignmri ? FilterTypeNone             : MriHead->IsMask () ? FilterTypeMedian             : FilterTypeNone;
    InterpolationType   interpolate         = ! realignmri ? InterpolateNearestNeighbor : MriHead->IsMask () ? InterpolateNearestNeighbor   : InterpolateLanczos3;
    int                 numsubsampling      = ! realignmri ? 1                          : MriHead->IsMask () ? 1                            : 1;

                                        // allows fully automatic size (& origin)
    MriHead->GetData ()->TransformAndSave ( NormToMRI,
                                            TransformAndSaveFlags ( TransformToSource | TransformSourceRelative | TransformTargetAbsolute ),
                                            filtertype,                 interpolate,            numsubsampling,
                                            0,                          0, 
                                            MriHead->GetVoxelSize (),   0,
                                            realignmri ? BoundingSizeOptimal : BoundingSizeBiggest, 0,
                                            NiftiOrientation,
                                            AtLeast ( NiftiTransformDefault, MriHead->GetNiftiTransform () ),   MriHead->GetNiftiIntentCode (),   NiftiIntentNameDefault,
                                            FileLocalMriHead,           0,  0,
                                            "Head MRI Realignment" 
                                            );

                                        // We're done, but we have to reload / reset 1 or 2 things
                                        // Close MRIs
    MriHead.Close ();


    Gauge.Next ( gaugecimmriproc, SuperGaugeUpdateTitle );

    bool                showsagequ          = interactive && realignmri;

    MriHead.Open ( FileLocalMriHead, interactive || showsagequ || xyzcoregistertomri ? OpenDocVisibleLocked : OpenDocHidden );

//  MriHead->GetOrigin ().Show ( "MriCenter of realigned Full Head" );

                                        // refreshing local copy of volume
    GetVolumeCopy ( MriHead, HeadVolume, smoothmri, fullvolumerescale );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    TVolumeView*        fullmriview     = showsagequ ? dynamic_cast< TVolumeView * > ( MriHead->GetViewList () ) : 0;

    if ( fullmriview ) {

        fullmriview->SendMessage ( WM_COMMAND, IDB_CUTPLANESAGITTAL, 0 );

                                        // visually check end result of transformations
        if ( ! GetAnswerFromUser ( reorientmris ? "Does the Sagittal and/or Equator transformation seems OK in the full MRI?" NewLine 
                                                  NewLine 
                                                  "(check within the MRI window)"
                                                : "Does the Sagittal and/or Equator transformation seems OK in the full MRI?" NewLine 
                                                  NewLine 
                                                  "(check within the MRI window)", MriHead->GetTitle (), this ) ) {

            ShowMessage ( "Something may have fooled the Sagittal and/or Equator detection, such as:" NewLine
                          NewLine 
                          "- Artifacts in the MRI -> clean-up MRI" NewLine 
                          "- Bad background threshold detection -> check for noise" NewLine 
                          "- Bad orientation detection -> check orientation" NewLine 
                          NewLine 
                          "This will end your processing now, bye bye...", MriHead->GetTitle (), ShowMessageWarning, this );

            goto endprocessing;
            }


        fullmriview->SendMessage ( WM_COMMAND, IDB_CUTPLANESAGITTAL, 0 );
        fullmriview->SendMessage ( WM_COMMAND, IDB_CUTPLANESAGITTAL, 0 );
        fullmriview->WindowMinimize ();
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // brain is optional, re-align & save it if it exists
    Gauge.Next ( gaugecimmriproc, SuperGaugeUpdateTitle );

                                        // we want the same size as the new MriHead, for the inverse display mainly
    TVector3Int         MriSize;
    MriHead->GetData ()->GetDims ( MriSize );
    TPointDouble        MriOrigin       = MriHead->GetOrigin ();
    TVector3Double      MriVoxelSize    = MriHead->GetVoxelSize ();


    if ( MriBrain.IsOpen () ) {
                                               // nothing                                     discrete values                                   continuous values
        FilterTypes         filtertype          = ! realignmri ? FilterTypeNone             : MriBrain->IsMask () ? FilterTypeMedian           : FilterTypeNone;
        InterpolationType   interpolate         = ! realignmri ? InterpolateNearestNeighbor : MriBrain->IsMask () ? InterpolateNearestNeighbor : InterpolateLanczos3;
        int                 numsubsampling      = ! realignmri ? 1                          : MriBrain->IsMask () ? 3                          : 1;


        MriBrain->GetData ()->TransformAndSave (    NormToMRI,
                                                    TransformAndSaveFlags ( TransformToSource | TransformSourceRelative | TransformTargetAbsolute ),
                                                    filtertype,                 interpolate,            numsubsampling,
                                                    0,                          &MriOrigin, 
                                                    MriVoxelSize,               0,
                                                    BoundingSizeGivenNoShift,   &MriSize,
                                                    NiftiOrientation,
                                                    AtLeast ( NiftiTransformDefault, MriBrain->GetNiftiTransform () ),  MriBrain->GetNiftiIntentCode (),    NiftiIntentNameBrain,
                                                    FileLocalMriBrain,          0,  0,
                                                    "Brain MRI Realignment" 
                                                );

                                        // don't close original brain
        MriBrain.Close ();

        MriBrain.Open ( FileLocalMriBrain, /*interactive ? OpenDocVisibleLocked :*/ OpenDocHidden );

//      MriBrain->GetOrigin ().Show ( "MriCenter of realigned Brain" );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // grey mask is optional, re-align & save it if it exists
    Gauge.Next ( gaugecimmriproc, SuperGaugeUpdateTitle );


    if ( MriGrey.IsOpen () ) {
                                               // nothing                                     discrete values                                   continuous values
        FilterTypes         filtertype          = ! realignmri ? FilterTypeNone             : MriGrey->IsMask () ? FilterTypeMedian           : FilterTypeNone;
        InterpolationType   interpolate         = ! realignmri ? InterpolateNearestNeighbor : MriGrey->IsMask () ? InterpolateNearestNeighbor : InterpolateLanczos3;
        int                 numsubsampling      = ! realignmri ? 1                          : MriGrey->IsMask () ? 3                          : 1;


        MriGrey ->GetData ()->TransformAndSave (    NormToMRI,
                                                    TransformAndSaveFlags ( TransformToSource | TransformSourceRelative | TransformTargetAbsolute ),
                                                    filtertype,                 interpolate,            numsubsampling,
                                                    0,                          &MriOrigin, 
                                                    MriVoxelSize,               0,
                                                    BoundingSizeGivenNoShift,   &MriSize,
                                                    NiftiOrientation,
                                                    AtLeast ( NiftiTransformDefault, MriGrey->GetNiftiTransform () ),  NiftiIntentCodeGreyMask,    NiftiIntentNameGreyMask,
                                                    FileLocalMriGrey,           0,  0,
                                                    "Grey MRI Realignment" 
                                                );

                                        // don't close original brain
        MriGrey.Close ();

        MriGrey.Open ( FileLocalMriGrey, /*interactive ? OpenDocVisibleLocked :*/ OpenDocHidden );

//      MriGrey->GetOrigin ().Show ( "MriCenter of realigned Brain" );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Tissues volume is optional, re-align & save it if it exists
    Gauge.Next ( gaugecimmriproc, SuperGaugeUpdateTitle );


    if ( MriTissues.IsOpen () ) {
                                               // nothing                                     discrete values                                   continuous values
        FilterTypes         filtertype          = ! realignmri ? FilterTypeNone             : MriTissues->IsMask () ? FilterTypeMedian           : FilterTypeNone;
        InterpolationType   interpolate         = ! realignmri ? InterpolateNearestNeighbor : MriTissues->IsMask () ? InterpolateNearestNeighbor : InterpolateLanczos3;
        int                 numsubsampling      = ! realignmri ? 1                          : MriTissues->IsMask () ? 3                          : 1;


        MriTissues->GetData ()->TransformAndSave (  NormToMRI,
                                                    TransformAndSaveFlags ( TransformToSource | TransformSourceRelative | TransformTargetAbsolute ),
                                                    filtertype,                 interpolate,            numsubsampling,
                                                    0,                          &MriOrigin, 
                                                    MriVoxelSize,               0,
                                                    BoundingSizeGivenNoShift,   &MriSize,
                                                    NiftiOrientation,
                                                    AtLeast ( NiftiTransformDefault, MriTissues->GetNiftiTransform () ),  NiftiIntentCodeLabels,    NiftiIntentNameLabels,
                                                    FileLocalMriTissues,        0,  0,
                                                    "Tissues Volume Realignment" 
                                                );

                                        // don't close original brain
        MriTissues.Close ();

        MriTissues.Open ( FileLocalMriTissues, /*interactive ? OpenDocVisibleLocked :*/ OpenDocHidden );

//      MriTissues->GetOrigin ().Show ( "MriCenter of realigned Tissues" );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // re-compute center & orientation
    GetNormalizationTransform   (   MriHead,    MriBrain, 
                                    false,      MriCenter, 
                                    &NormToMRI, 0           
                                );

//  MriCenter.Show ( "MriCenter after final GetNormalizationTransform" );

    } // if reorientmris

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here MriBrain is ready for use, and can be either:
                                        // - original brain or downsampled brain  (FileOriginalMriBrain)
                                        // - optionally reoriented                (FileLocalMriBrain)
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Either by normalization or reloading, we should have a non-null center (in voxel coordinates)
if ( ( MriHead.IsOpen () || MriBrain.IsOpen () ) &&
    (    reorientmris && MriCenter        .IsNull ()
    || ! reorientmris && OriginalMriOrigin.IsNull () ) ) {

    ShowMessage ( "Your input MRI files together with your current parameters seem to end up with" NewLine 
                  "an undefined MRI center/origin:" NewLine 
                  NewLine 
                  Tab "- You could use the 'Re-Align' options to search for an origin" NewLine 
                  "or" NewLine 
                  Tab "- Pick some MRIs with correctly defined origins." NewLine 
                  NewLine 
                  "This will end your processing now, bye bye...", CreatingInverseTitle, ShowMessageWarning, this );

    goto endprocessing;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here we know if the original MRIs have been modified
mritransformed      = reorientmris;

                                        // then set these 2 matrices
if ( mritransformed ) {

    NormToAbsOriginalMRI    = RelOriginalMRI_To_AbsOriginalXYZ * NormToOriginalMRI;
    AbsOriginalMRIToNorm    = TMatrix44 ( NormToAbsOriginalMRI ).Invert ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // find the neck cutting plane
Gauge.Next ( gaugecimglobal, SuperGaugeUpdateTitle );

if ( MriHead.IsOpen () && ( xyzprocessing || computeleadfield ) ) {

    bool                guillotineok    = SetGuillotinePlane    (   MriHead, 
                                                                    mriabstoguillotine,
                                                                    0,
                                                                    "Guillotine Plane Search"
                                                                );


    if ( ! guillotineok )                 // default is to continue anyway

        if ( interactive && ! GetAnswerFromUser ( "The Guillotine Plane might be wrongly detected, do you want to proceed anyway?" NewLine 
                                                  "(maybe not a good idea)", MriHead->GetTitle (), this ) )
            goto endprocessing;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // surface points needed?
if ( computeleadfield ) {

    Gauge.Next ( gaugecimheadmodel, SuperGaugeUpdateTitle );
                                        // points returned in absolute coordinates
                                        // skipping bottom points now, which are not of interest anyway
    headsurfacepoints.GetSurfacePoints ( HeadVolume, TPointFloat ( MriCenter ), false /*true*/ );


#if defined(InvMatrixMoreOutputs)
    TFileName       _file;
    StringCopy      ( _file,    (char*) BaseFileNameMore,   ".HeadModel.SurfacePoints.",    FILEEXT_SPIRR );
    headsurfacepoints.WriteFile ( _file );
#endif

    } // computeleadfield


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Computing the Grey mask, if it was not provided - !Brain should of course exist and be already opened!
if ( computegrey && MriBrain.IsOpen () ) {

                                        // This shouldn't really happen, but let's roll with it so we can still save some processing
    bool                brainisgreymask     = MriBrain->IsMask ();
                                        // Wraps everything from input document to output file
    bool                greymaskok          = SegmentGreyMatter ( MriBrain, greyflags, mritransformed ? FileLocalMriGrey : FileOriginalMriGrey );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Brain was actually a mask, so we are going to make it a real brain, for a more coherent output(?)
    if ( brainisgreymask ) {

        Volume&             data            = *MriBrain->GetData ();
        FctParams           p;

                                        // mask can be quite sparse, inflate it before converting to mask
        Gauge.Next ( gaugecimmrigrey, SuperGaugeUpdateTitle );
        p ( FilterParamDiameter )     = 3;
        data.Filter ( FilterTypeDilate, p );

                                        // filling the grey mask
        Gauge.Next ( gaugecimmrigrey, SuperGaugeUpdateTitle );
        p ( FilterParamToMaskThreshold )     = MriBrain->GetBackgroundValue ();
        p ( FilterParamToMaskNewValue  )     = 1;
        p ( FilterParamToMaskCarveBack )     = true;
        data.Filter ( FilterTypeToMask, p, SuperGaugeUpdateTitle );

                                        // come back to a reasonable size
        Gauge.Next ( gaugecimmrigrey, SuperGaugeUpdateTitle );
        p ( FilterParamDiameter )     = 2;
        data.Filter ( FilterTypeErode, p );

                                        // optionally making it smoother?
//      p ( FilterParamDiameter )     = 5;
//      p ( FilterParamNumRelax )     = 1;
//      data.Filter ( FilterTypeRelax, p, true );

                                        // fill in with data
        if ( MriHead.IsOpen () )
            data.BinaryOp ( *MriHead->GetData (), OperandMask, OperandData, OperationMultiply );

                                        // save actual "brain"
        Gauge.Next ( gaugecimmrigrey, SuperGaugeUpdateTitle );

        MriBrain->Commit ( true );

        MriBrain.Close ();

        MriBrain.Open ( mritransformed ? FileLocalMriBrain : FileOriginalMriBrain, interactive ? OpenDocVisibleLocked : OpenDocHidden );

        MriBrain->MinimizeViews ();
        } // alreadygreymask
    else
        Gauge.Next ( gaugecimmrigrey, SuperGaugeUpdateTitle, 4 );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here we are sure Brain != Grey
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Gauge.Next ( gaugecimmrigrey, SuperGaugeUpdateTitle );
                                        // Open Grey Mask
    MriGrey.Open ( mritransformed ? FileLocalMriGrey : FileOriginalMriGrey, interactive ? OpenDocVisibleLocked : OpenDocHidden );


    if ( ! greymaskok )            // default is to continue anyway

        if ( interactive && ! GetAnswerFromUser ( "The Grey Matter Mask extraction might have gone wrong, do you want to proceed anyway?" NewLine 
                                                  NewLine 
                                                  "(check the Grey Matter window)", MriGrey->GetTitle (), this ) )
            goto endprocessing;


    MriGrey->MinimizeViews ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // complimentary grey matter back to the original MRI space
    Gauge.Next ( gaugecimmrigrey, SuperGaugeUpdateTitle );


    if ( mritransformed ) {
                                        // Not really sure why, but the grey back to original needs these adjustments...
        TMatrix44           NormToOriginalMRIGrey ( NormToOriginalMRI );

        NormToOriginalMRIGrey.Translate ( -0.50, -0.50, -0.50, MultiplyRight );


        FilterTypes         filtertype          = FilterTypeMedian;
        InterpolationType   interpolate         = InterpolateTruncate;
        int                 numsubsampling      = 3;


        MriGrey->GetData ()->TransformAndSave ( NormToOriginalMRIGrey,
                                                TransformAndSaveFlags ( TransformToTarget | TransformSourceAbsolute | TransformTargetRelative ),
                                                filtertype,                 interpolate,            numsubsampling,
                                                &MriCenter,                 &OriginalMriOrigin,
                                                MriBrain->GetVoxelSize (),  0,
                                                BoundingSizeGivenNoShift,   &OriginalMriSize,        
                                                OriginalMriOrientation,
                                                AtLeast ( NiftiTransformDefault, MriBrain->GetNiftiTransform () ),  NiftiIntentCodeGreyMask,    NiftiIntentNameGreyMask,
                                                FileOriginalMriGrey,           0,  0,
                                                "Original Grey Matter" );


        Verbose.NextLine ();
        Verbose.Put ( "Grey mask MRI file, original space:", FileOriginalMriGrey );
        Verbose.Flush ();
        } // complimentary outputs

    } // computegrey


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get the Solution Points (compute or load)
if ( hassps ) {

    if ( computesps ) {
                                        // numsolpoints: absolute min is 4000 SPs, 5000 to 6000 is a good spot
        Gauge.Next ( gaugecimsp, SuperGaugeUpdateTitle );


        bool                solpointsok     = ComputeSolutionPoints (
                                                                    MriBrain,           MriGrey, 
                                                                    numsolpoints,       ressolpoints,
                                                                    spflags, 
                                                                    loretaneighborhood, lauraneighborhood,
                                                                    solpoints,          solpointsnames,
                                                                    reorientmris ? FileLocalSp : FileOriginalSp
                                                                    );

        if ( ! solpointsok ) {
            if ( ! interactive || ! GetAnswerFromUser ( "The Solution Points extraction seems to be wrong, do you want to proceed anyway (not recommended)?", MriGrey->GetTitle (), this ) )
                goto endprocessing;
            }


        if ( ressolpoints > 0 ) {
            Verbose.NextLine ();
            Verbose.Put ( "Computed number of Solution Points:", solpoints.GetNumPoints () );
            Verbose.Flush ();
            }


        Gauge.Next ( gaugecimsp, SuperGaugeUpdateTitle );

                                        // here we know the size
        spsrejected     = TSelection ( (int) solpoints, OrderSorted );

                                        // reject points that are outside the Lead Field SPs (i.e. extrapolated)
        if ( interpolateleadfield ) {

            TPoints             solpointsback ( solpoints );

            if ( mritransformed )
                NormToAbsOriginalMRI.Apply ( solpointsback );
                                        // generate a selection of points outside the Lead Field  SPs
                                        // !we need the reference points to be on a grid!
                                        // build a list of rejected points
            ScanForOutsidePoints ( leadfieldsolpoints, solpointsback, spsrejected );


            if ( (bool) spsrejected ) {

                char                buff[ 1024 ];
                                        // always show to the user
                sprintf ( buff, "%0d Solution Points don't have enough neighbors for a proper Lead Field interpolation." NewLine, (int) spsrejected );
                StringAppend ( buff, "These points will be kept in the output inverse space, but setting the Inverse Matrix to 0 at these positions." NewLine );

                if ( interactive )
                    ShowMessage ( buff, LeadFieldInterpolationTitle, ShowMessageWarning, this );

                                        // save this to verbose!
                Verbose.NextLine ( 1 );
                Verbose.NextTopic ( "Computing Solution Points Warning:" );
                (ofstream&) Verbose << buff;

                                        // write list of outside points
                Verbose.NextLine ();

                Verbose.ResetTable ();

                Verbose.TableColNames.Add ( "" );
                Verbose.TableColNames.Add ( "Index" );
                Verbose.TableColNames.Add ( "X" );
                Verbose.TableColNames.Add ( "Y" );
                Verbose.TableColNames.Add ( "Z" );
                Verbose.TableColNames.Add ( "Name" );


                Verbose.BeginTable ( 10 );


                for ( int sp = 0; sp < (int) solpoints; sp++ )

                    if ( spsrejected [ sp ] ) {

                        Verbose.PutTable ( sp + 1 );
                        Verbose.PutTable ( solpoints[ sp ].X, 1 );
                        Verbose.PutTable ( solpoints[ sp ].Y, 1 );
                        Verbose.PutTable ( solpoints[ sp ].Z, 1 );
                        Verbose.PutTable ( solpointsnames[ sp ] );
                        }


                Verbose.EndTable ();

                Verbose.NextLine ();

                Verbose.Flush ();

                // we keep the bad points in, but flagged with  spsrejected

/*                                      // generate new list of points
                                        // here we generate our own SPs, so we can do whatever rejection we think fit without telling
                RejectPointsFromList ( solpoints, solpointsnames, spsrejected );

                                        // overwrite previous version
                solpoints.WriteFile ( reorientmris ? FileLocalSp : FileOriginalSp, &solpointsnames );
                                        // re-allocate new size
                spsrejected     = TSelection ( (int) solpoints, OrderSorted );
                spsrejected.Reset ();*/
                } // if spsrejected

            } // interpolateleadfield


//      outputnumsolp       = (int) solpoints;
        solpointsgrid       =       solpoints;  // brain has been realigned, SPs are extracted in the new space
        } // computesps


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    else if ( samespsaslf || loadsps ) {

        Gauge.Next ( gaugecimsp, SuperGaugeUpdateTitle );

                                        // copy loaded solpoints
        solpoints           = loadsolpoints;
        solpointsnames      = loadsolpointsnames;

        solpointsgrid       = solpoints;// loaded points SHOULD BE aligned on a grid, though they could be rotated according to X,Y,Z axis

                                        // we need to reload the ORIGINAL Grey / Brain file (definitely NOT MriBrain at that point, so no worry about closing it)
        MriBrainOrig.Open (   CrisTransfer.HasBrainMri () ? (const char*) FileInputMriBrain 
                            :                               (const char*) 0, OpenDocHidden );

                                        // rescaling is now directly done in TPoints, to be universal to other Cartool processing
//      if ( IsExtension ( FileInputSpLoad, FILEEXT_LOC ) )
//                                              // rescaling from [m] to [mm]
//          solpoints  *= 1000;

                                        // here we know the size
        spsrejected         = TSelection ( (int) solpoints, OrderSorted );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we DO need regular points, otherwise distance computation will likely fail
        if ( ! solpoints.IsGrid () ) {
                                        // save this to verbose!
            Verbose.NextLine ( 2 );
            (ofstream&) Verbose << "Loaded Solution Points do not seem to be regularly spaced on a grid, with the current preprocessing parameters." << NewLine;
            (ofstream&) Verbose << "Try some of the following solutions:" << NewLine;
            (ofstream&) Verbose << Tab << "- Check your input file is really the right one, that is un-transformed" << NewLine;
            (ofstream&) Verbose << Tab << "- Check you don't use neither the Sagittal nor the Equatorial re-alignment" << NewLine;
            (ofstream&) Verbose << Tab << "- Otherwise, that sucks, sorry about that..." << NewLine;
            (ofstream&) Verbose << NewLine;
            (ofstream&) Verbose << "Processing aborted." << NewLine;
            (ofstream&) Verbose << NewLine;
            Verbose.Flush ();

            if ( interactive ) {
                if ( ! GetAnswerFromUser    (   "The given Solution Points file and the current MRI preprocessing parameters" NewLine 
                                                "seem to give points which are not on a regular grid." NewLine 
                                                "This will very likely cause some errors later on." NewLine 
                                                NewLine 
                                                "Do you still want to proceed with these parameters (not recommended)?", FileInputSpLoad, this ) )
                    goto endprocessing;
                } // interactive
            else {
                ShowMessage (   "The given Solution Points file and the current MRI preprocessing parameters" NewLine 
                                "seem to give points which are not on a regular grid." NewLine 
                                "This will very likely cause some errors later on." NewLine 
                                NewLine 
                                "This will end your processing now, bye bye...", FileInputSpLoad, ShowMessageWarning, this );
                goto endprocessing;
                }
            } // ! IsGrid


                                        // Check Brain / SPs consistency
        Gauge.Next ( gaugecimsp, SuperGaugeUpdateTitle );


        if ( MriBrainOrig.IsOpen () ) {
                                        // Test size compatibility
            double              sizemri         = MriBrainOrig->GetBounding ()     ->MeanSize ();
            double              sizesps         = TBoundingBox<double> ( solpoints ).MeanSize ();

                                        // there shouldn't be a major size difference - max OK: 0.11  min not OK: 0.68
            bool                sizeok          = RelativeDifference ( sizemri, sizesps ) < 0.30;

//            DBGV4 ( sizemri, sizesps, RelativeDifference ( sizemri, sizesps ), sizeok, "sizemri sizesps reldiff -> sizeok" );

            if ( ! sizeok )
                if ( interactive ) {
                    if ( ! GetAnswerFromUser    (   "It seems that the Solution Points and the Brain Mri do not have the same size!" NewLine 
                                                    "Make sure you picked the right files." NewLine 
                                                    NewLine 
                                                    "Do you want to proceed anyway (not recommended)?", FileInputSpLoad, this ) )
                        goto endprocessing;
                    }
                else {
                   ShowMessage  (   "It seems that the Solution Points and the Brain Mri do not have the same size!" NewLine 
                                    "Make sure you picked the right files." NewLine 
                                    NewLine 
                                    "This will end your processing now, bye bye...", FileInputSpLoad, ShowMessageWarning, this );
                   goto endprocessing;
                   }

                                        // Test inclusion (should be of same size, as a smaller SPs will easily fit into a bigger MRI!)
            double              backv           = 0; // MriBrainOrig->GetBackgroundValue ();
            Volume*             data            = MriBrainOrig->GetData ();
            int                 countout        = 0;
            TPointDouble        center          = MriBrainOrig->GetOrigin ();

            for ( int i = 0; i < (int) solpoints; i++ )

                countout   += data->GetValueChecked ( solpoints[ i ].X + center.X,
                                                      solpoints[ i ].Y + center.Y,
                                                      solpoints[ i ].Z + center.Z ) <= backv;

                                        // there shouldn't be too much points outside the brain - max OK: 0.027  min not OK: 0.24
            bool                inclok          = (double) countout / (int) solpoints <= 0.15;

//          DBGV4 ( countout, (int) solpoints, (double) countout / (int) solpoints * 100, inclok, "#out #points %out -> inclok" );

            if ( ! inclok )
                if ( interactive ) {
                    if ( ! GetAnswerFromUser    (   "It seems that the Solution Points and the Brain Mri do not fit very well!" NewLine 
                                                    "Make sure you picked the right files." NewLine 
                                                    NewLine 
                                                    "Do you want to proceed anyway (not recommended)?", FileInputSpLoad, this ) )
                        goto endprocessing;
                    }
                                        // NOT aborting, trust the user in batch mode(?)
//              else {
//                  ShowMessage     (   "It seems that the Solution Points and the Brain Mri do not fit very well!" NewLine 
//                                      "Make sure you picked the right files." NewLine 
//                                      NewLine 
//                                      "This will end your processing now, bye bye...", FileInputSpLoad, ShowMessageWarning, this );
//                 goto endprocessing;
//                 }
            } // SP / Brain checks


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do we remove SPs from the given file, that are outside the interpolation capabilities?
        Gauge.Next ( gaugecimsp, SuperGaugeUpdateTitle );


        if ( loadsps && interpolateleadfield ) {

                                                    // use the loaded points, in the original space (?)
            ScanForOutsidePoints ( leadfieldsolpoints, solpoints, spsrejected );


            if ( (bool) spsrejected ) {

                char                buff[ 1024 ];
                                        // always show to the user
                sprintf         ( buff, "%0d Solution Points don't have enough neighbors for a proper Lead Field interpolation." NewLine, (int) spsrejected );
                StringAppend    ( buff, "These points will be kept in the output inverse space, but setting the Inverse Matrix to 0 at these positions." NewLine );

                if ( interactive )
                    ShowMessage ( buff, LeadFieldInterpolationTitle, ShowMessageWarning, this );

                                        // save this to verbose!
                Verbose.NextLine ( 1 );
                Verbose.NextTopic ( "Loading Solution Points Warning:" );
                (ofstream&) Verbose << buff;

                                        // write list of outside points
                Verbose.NextLine ();

                Verbose.ResetTable ();

                Verbose.TableColNames.Add ( "" );
                Verbose.TableColNames.Add ( "Index" );
                Verbose.TableColNames.Add ( "X" );
                Verbose.TableColNames.Add ( "Y" );
                Verbose.TableColNames.Add ( "Z" );
                Verbose.TableColNames.Add ( "Name" );


                Verbose.BeginTable ( 10 );


                for ( int sp = 0; sp < (int) solpoints; sp++ )

                    if ( spsrejected [ sp ] ) {

                        Verbose.PutTable ( sp + 1 );
                        Verbose.PutTable ( solpoints[ sp ].X, 1 );
                        Verbose.PutTable ( solpoints[ sp ].Y, 1 );
                        Verbose.PutTable ( solpoints[ sp ].Z, 1 );
                        Verbose.PutTable ( solpointsnames[ sp ] );
                        }


                Verbose.EndTable ();

                Verbose.NextLine ();

                Verbose.Flush ();

                // now we can keep the points in, with  spsrejected  given to the inversion, it should be OK

/*                                        // ask user what to do
                if ( interactive ) {
                    sprintf ( buff, "%0d Solution Points don't have enough neighbors for a proper Lead Field interpolation." NewLine "Can we remove these points, therefore modifying your loaded file?", (int) spsrejected );

                    if      ( GetAnswerFromUser ( buff, "Loading Solution Points", this ) ) {
                        (ofstream&) Verbose << "These points will be removed from the output inverse space." << NewLine;

                                        // generate new list of points
                        RejectPointsFromList ( solpoints, solpointsnames, spsrejected );

                                        // replace with filtered version
                        solpointsgrid           = solpoints;
                        }                       // removing points

                    else if ( GetAnswerFromUser ( "OK, we are keeping the points outside the Lead Field." NewLine NewLine "Another option is to reset the Lead Field at these positions (your last chance)?", "Loading Solution Points", this ) ) {
                                                // nothing to do here, Lead Field will set the columns to 0
                        (ofstream&) Verbose << "These points will be kept in the output inverse space, but setting the Lead Field to 0 at these positions." << NewLine;
                        }
                    else {
                        ShowMessage ( "There are no other options for the points outside the Lead Field..." NewLine NewLine "This will end your processing now, bye bye...", "Loading Solution Points", ShowMessageWarning, this );
                        (ofstream&) Verbose << "User chose to abort the processing here..." << NewLine;
                        goto endprocessing;     // we're done
                        }
                    } // interactive
                else { // ! interactive
                                                // default is to set to 0
                    (ofstream&) Verbose << "These points will be kept in the output inverse space, but setting the Lead Field to 0 at these positions." << NewLine;
                    }
*/
                } // if spsrejected


//          spsrejected.Reset ();       // now we need it for inversion
            } // loadsps


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // downsampling loaded SPs?
        Gauge.Next ( gaugecimsp, SuperGaugeUpdateTitle );
                                        // also note there is no need to check for SPs outside the LF, because they are the same points downsampled
                                        // There is some tolerance in the number of target solution points - see  samespsaslfmaxsolpoints
        if ( samespsaslf && downsampleleadfield ) {

            bool                cliptogrey          = MriBrainOrig.IsOpen ();   // removing points not in the grey - !using the real original file, which matches the original SP!
            NeighborhoodType    neighborhood        = inverseloreta ? loretaneighborhood : lauraneighborhood;
            double              neighborradius      = solpoints.GetMedianDistance () * Neighborhood[ neighborhood ].MidDistanceCut;

                                        // provide a rejection list, returns a new list of points (still with old labels)
            DownsampleSolutionPoints ( cliptogrey ? MriBrainOrig.GetDoc () : 0, solpoints, neighborradius, samespsaslfmaxsolpoints, spsrejected );

                                        // generate new list of points
            RejectPointsFromList ( solpoints, solpointsnames, spsrejected );


            if ( (bool) spsrejected ) {
                                        // replace with downsampled version
                solpointsgrid           = solpoints;        // downloaded points should be in an aligned grid

                                        // save this to verbose!
                Verbose.NextLine ( 2 );
                Verbose.Put ( "Downsampling loaded Solution Points to:", (int) solpoints /*samespsaslfmaxsolpoints*/ );
                Verbose.Put ( "Clipping to grey matter:", cliptogrey );
                Verbose.Flush ();
                } // (bool) spsrejected

            // !loadleadfield  needs spsrejected, don't reset it!

            } // downsampling SPs


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // apply now any realignment transform, AFTER the downsampling to avoid losing the grid alignment!
                                        // DON'T apply to  solpointsgrid, this is precisely the point of duplicating the list!
        if ( mritransformed )
            AbsOriginalMRIToNorm.Apply ( solpoints );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // finally, we can save our local file
        solpoints.WriteFile ( reorientmris ? FileLocalSp : FileOriginalSp, &solpointsnames );
        } // samespsaslf || loadsps


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // complimentary solution points back to the original MRI space
    Gauge.Next ( gaugecimsp, SuperGaugeUpdateTitle );


    if ( mritransformed ) {

        if ( samespsaslf || loadsps )
                                        // these guys are still in the original space - let save us some rounding errors!
            solpointsgrid.WriteFile ( FileOriginalSp, &solpointsnames );

        else if ( computesps ) {

            TPoints             solpointsback ( solpoints );

            NormToAbsOriginalMRI.Apply ( solpointsback );

            solpointsback.WriteFile ( FileOriginalSp, &solpointsnames );
            }


        Verbose.NextLine ();
        Verbose.Put ( "Solution Points file, original space:", FileOriginalSp );
        Verbose.Flush ();
        } // complimentary outputs

    } // hassps


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Opening XYZ
if ( xyzprocessing ) {

    Gauge.Next ( gaugecimxyz, SuperGaugeUpdateTitle );


    Xyz.Open ( FileInputXyz, interactive || xyzcoregistertomri ? OpenDocVisibleLocked : OpenDocHidden );

                                        // set sagittal display, like the MRI
    TElectrodesView*        xyzview         =  dynamic_cast< TElectrodesView* > ( Xyz->GetViewList () );

    if ( xyzview ) {
        xyzview->SendMessage ( WM_COMMAND, IDB_ORIENT, 0 );
        xyzview->SendMessage ( WM_COMMAND, IDB_ORIENT, 0 );
        xyzview->SendMessage ( WM_COMMAND, IDB_ORIENT, 0 );
        xyzview->SendMessage ( WM_COMMAND, IDB_ORIENT, 0 );
        }

                     // don't ask anything if user set to coregistered
    if ( interactive && ! xyzcoregistrationdone
      && ! GetAnswerFromUser (  "Is the Electrodes orientation detection OK (Anterior, Superior..)?" NewLine 
                                NewLine 
                                "(check within the Electrodes window)", Xyz->GetTitle (), this ) ) {

        ShowMessage (   "Please, manually re-orient the electrodes,and come back here." NewLine 
                        NewLine 
                        "This will end your processing now, bye bye...", Xyz->GetTitle (), ShowMessageWarning, this );

        goto endprocessing;
        }

    Xyz->MinimizeViews ();

                                        // get the electrodes list
    xyzpoints   = Xyz->GetPoints ( DisplaySpace3D );
                                        // remove any fake points at the end that might have been added to cope with pseudo-tracks
    xyzpoints.RemoveLast ( xyzpoints.GetNumPoints () - Xyz->GetNumElectrodes () );
                                        // also get the electrode names for proper savings
    xyznames    = *Xyz->GetElectrodesNames ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute an optimal spherization center
if ( computeleadfield ) {

//    Gauge.Next ( gaugecimleadfield, SuperGaugeUpdateTitle );
                                        // Be careful to the correct use of this translation, either add or subtract if applied to points or to a center
    MriCenterToInverseCenter    = ComputeOptimalInverseTranslation ( &headsurfacepoints, &solpoints, &xyzpoints, false );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Entering processing output section

Verbose.NextTopic ( "Piloting processing:" );
{
Verbose.Put ( "Processing is:", interactive ? "User controlled" : "Automatic" );
}

Verbose.Flush ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute head model for spherization, relative to MriCenter
if ( getheadmodel ) {

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Get the relevant points from the surface: around top head, back head, a bit of face, but without nose nor ears
    Gauge.Next ( gaugecimheadmodel, SuperGaugeUpdateTitle );


    TPoints             topsurfacepoints ( headsurfacepoints );

                                        // keep a shape like top of head + on the back, avoiding any clipping (guillotine)
    topsurfacepoints.KeepTopHeadPoints  ( *MriHead, MriCenterToInverseCenter, false, &mriabstoguillotine );

#if defined(InvMatrixMoreOutputs)
                                        // save the final surface points
    TFileName           _file;
    StringCopy      ( _file,    (char*) BaseFileNameMore,   ".HeadModel.SurfacePoints.Model.",    FILEEXT_SPIRR );
    topsurfacepoints.WriteFile ( _file );
#endif

    Gauge.Next ( gaugecimheadmodel, SuperGaugeUpdateTitle );

                                        // we don't need all these points for the model (which is smooth), plus too much points is too long for computation
    topsurfacepoints.DownsamplePoints   ( min ( HeadModelNumPoints, GlobalOptimizeMaxPoints ) );

                                        // Not used anymore:
                                        // - Estimate of the scaling is not very reliable
                                        // - Even though the Full Head has been smoothed out doesn't mean the surface model is wrong...
//  if ( fullvolumerescale != 1 )
//                                      // adjust global scale to compensate for the filtered volume new size
//                                      // this is an approximation, though: the scaling is radial from the current center, so points further
//                                      // will move more in absolute distance, but the results seem fair enough from observation...
//      topsurfacepoints  *= fullvolumerescale;


#if defined(InvMatrixMoreOutputs)
                                        // save the final, downsampled surface points
    StringCopy      ( _file,    (char*) BaseFileNameMore,   ".HeadModel.SurfacePoints.Model.Downsampled.",    FILEEXT_SPIRR );
    topsurfacepoints.WriteFile ( _file );
#endif


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Compute the spherization equation
    Gauge.Next ( gaugecimheadmodel, SuperGaugeUpdateTitle );

                                        // apply new center before computing model, which will not use any translation
    topsurfacepoints   += MriCenterToInverseCenter;

                                        // init model with our best points
    SurfaceModel        = TFitModelOnPoints ( topsurfacepoints );

                                        // run the (top) head modeling
    bool                modelok         = ComputeSpherizationModel ( SurfaceModel );

                                        // write model with correct origin
    SurfaceModel.WriteFile       ( file3Dheadmodel, &MriCenterToInverseCenter );


    if ( interactive ) {

                                        // show the resulting model
        Model.Open ( file3Dheadmodel, interactive ? OpenDocVisibleLocked : OpenDocHidden );

                                        // get the 2 windows
        TVolumeView*        fullmriview     =  dynamic_cast< TVolumeView*     > ( MriHead->GetViewList () );
        TElectrodesView*    modelview       =  dynamic_cast< TElectrodesView* > ( Model  ->GetViewList () );

                                        // modify their appearance
        modelview  ->WindowMinimize    ();
        fullmriview->WindowRestore     ();
        fullmriview->WindowSetPosition ( 0, 0, 800, 800 );

                                        // plug one into the other
        fullmriview->Using. Append ( modelview   );
        modelview  ->UsedBy.Append ( fullmriview );
        fullmriview->ShowNow ();

                                        // then ask the user
        if ( ! GetAnswerFromUser (  modelok ?   "Can you confirm the model is correctly fitting the upper part of the head?" NewLine 
                                                NewLine 
                                                "(check within the MRI window)"
                                            :   "The model might be wrong, do you still want to proceed?" NewLine 
                                                NewLine 
                                                "(check within the MRI window)",
                                    MriHead->GetTitle (), this ) ) {

            ShowMessage (   "There seems to be a problem with the head model," NewLine 
                            "try to figure out what is wrong with your data and come back here." NewLine 
                            NewLine 
                            "This will end your processing now, bye bye...", MriHead->GetTitle (), ShowMessageWarning, this );
            goto endprocessing;
            }

                                        // unplug from display
        fullmriview->Using. Remove ( modelview  , DontDeallocate );
        modelview  ->UsedBy.Remove ( fullmriview, DontDeallocate );
        fullmriview->ShowNow ();
        }
                                        // choosing to proceed anyway(?)
//  else { // automatic
//      if ( ! modelok ) {
//          ShowMessage (   "There seems to be a problem with the head model," NewLine 
//                          "try to figure out what is wrong with your data and come back here." NewLine 
//                          NewLine 
//                          "This will end your processing now, bye bye...", MriHead->GetTitle (), ShowMessageWarning, this );
//          goto endprocessing;
//          }
//      }

    } // getheadmodel


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setting default variables
Gauge.Next ( gaugecimglobal, SuperGaugeUpdateTitle );


if ( xyzcoregistrationdone ) {
                                        // complimentary copy
    xyzpoints.WriteFile ( FileOriginalXyz, &xyznames );


    if ( mritransformed ) {
                                        // if Absolute xyz, convert to relative-original MRI first, including downsampling shift
                                        // re-orient using MRI transform
        AbsOriginalMRIToNorm.Apply ( xyzpoints );
                                        // for convenience, copy the original electrodes file
        xyzpoints.WriteFile ( FileLocalXyz, &xyznames );
        }
    }

else if ( xyzcoregistertomri ) {
                                        // Set center & orientation from the points themselves

    if ( xyzcoregistertomriiredo ) {    // reload an already coregistered xyz, just to refine/adjust it

        if      ( realignmri   )    normtoxyz   = NormToAbsOriginalMRI;
        else if ( reorientmris )    normtoxyz   = NormToAbsOriginalMRI;
        else                        normtoxyz.SetIdentity ();
        }

    else { // xyzcoregistertomrii
                                        // Center
        TPointDouble            XyzCenter;
        TBoundingBox<double>    bounding ( xyzpoints );


        XyzCenter   = bounding.GetCenter ();

                                        // there can be some slight shifts in the centering which are not so nice, though not a big issue
        if ( bounding.MeanSize () > 10 )    // truncation should not happen in normalized coordinates!
            XyzCenter.Truncate ();
                                        // Center & Orientation
                                        // going from XYZ normalized orientation to actual XYZ
        if      ( realignmri   )    normtoxyz   = Xyz->GetStandardOrientation ( RASToLocal );
        else if ( reorientmris )    normtoxyz   = NormToAbsOriginalMRI;
        else                        normtoxyz.SetIdentity ();

                                        // force some reasonable offset
        normtoxyz.Translate ( XyzCenter.X, XyzCenter.Y, XyzCenter.Z, MultiplyLeft );
        }

                                        // back transform
    xyztonorm           = TMatrix44 ( normtoxyz ).Invert ();

                                        // re-orient to default, & center
    xyztonorm.Apply ( xyzpoints );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//if ( xyzprocessing ) {
//    xyzpoints.WriteFile ( FileLocalXyz, &xyznames );
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Next ( gaugecimglobal, SuperGaugeUpdateTitle );


if ( MriHead.IsOpen () && xyzcoregistertomri ) {

    bool            coregistrationok;

    if ( xyzcoregistertomri )

        coregistrationok    = CoregisterXyzToMriInteractive (   xyzpoints,          xyznames,
                                                                MriHead,            HeadVolume, 
//                                                              mriabstoguillotine, 
                                                                xyzcoregistertomrii,
                                                                XyzCoregToNorm 
                                                             );

                                        // don't forget this has been applied beforehand!
    XyzCoregToNorm.Multiply ( xyztonorm, MultiplyRight );


    if ( ! coregistrationok ) {

//      if ( interactive )
            ShowMessage (   "You cancelled the electrodes coregistration!" NewLine 
                            NewLine 
                            "This will end your processing now, bye bye...", Xyz->GetTitle (), ShowMessageWarning, this );

        goto endprocessing;
        }

                                        // save file, and negotiate
    xyzpoints.WriteFile ( reorientmris ? FileLocalXyzCoreg : FileOriginalXyzCoreg, &xyznames );

                                        // Close XYZ & reopen
    Xyz.Close ();

    Xyz.Open ( reorientmris ? FileLocalXyzCoreg : FileOriginalXyzCoreg, interactive ? OpenDocVisibleLocked : OpenDocHidden );


    if ( interactive ) {
                                        // get the 2 windows
        TVolumeView*        fullmriview     =  dynamic_cast< TVolumeView*     > ( MriHead->GetViewList () );
        TElectrodesView*    xyzview         =  dynamic_cast< TElectrodesView* > ( Xyz    ->GetViewList () );

                                        // modify their appearance
        xyzview    ->WindowMinimize ();
        fullmriview->WindowRestore     ();
        fullmriview->WindowSetPosition ( 0, 0, 800, 800 );

                                        // plug one into the other
        fullmriview->Using. Append ( xyzview     );
        xyzview    ->UsedBy.Append ( fullmriview );
        fullmriview->ShowNow ();

                                        // then ask the user
        if ( ! GetAnswerFromUser (  "Are the electrodes correctly coregistered?" NewLine
                                    NewLine 
                                    "(check within the MRI window)", Xyz->GetTitle (), this ) ) {

            ShowMessage (   "Check your input data and parameters for something wrong," NewLine 
                            "and come back here." NewLine 
                            NewLine 
                            "This will end your processing now, bye bye...", Xyz->GetTitle (), ShowMessageWarning, this );
            goto endprocessing;
            }

                                        // unplug from display
        fullmriview->Using. Remove ( xyzview    , DontDeallocate );
        xyzview    ->UsedBy.Remove ( fullmriview, DontDeallocate );
        fullmriview->ShowNow ();
        fullmriview->WindowMinimize ();
        } // if interactive
                                        // NOT aborting, trust the user in batch mode(?)
//  else
//      if ( ! interactive && ! coregistrationok )
//          goto endprocessing;

    }

                                        // here we can save this for later use (coregistration to original)
xyzpointscoregtonorm    = xyzpoints;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // More electrodes outputs?
constexpr double    VoxelsAboveSurface      = 2.5;

Gauge.Next ( gaugecimglobal, SuperGaugeUpdateTitle );

                                        // can output a coregistered xyz on the surface, for display?
if ( xyzprocessing ) {
                                        // send the points back to the MRI surface + 2 voxels, using a smoothed volume
                                        // !don't modify the real xyzpoints!
    xyzpointsdisplay    = xyzpoints;

    xyzpointsdisplay.ResurfacePoints    (   
                                        HeadVolume,         // surface can reliably use the volume, as points are already on it
                                        HeadVolume,         // gradient can also use the volume for the same reason - plus we don't mind small potential inconsistencies
                                        MriCenter,
                                        &mriabstoguillotine,
                                        VoxelsAboveSurface
                                        );

    xyzpointsdisplay.WriteFile ( reorientmris ? FileLocalXyzDisplay : FileOriginalXyzDisplay, &xyznames );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // switch from relative original MRI to absolute original MRI
                                        // so the transform is from absolute realigned MRI to absolute original MRI
NormToOriginalMRI.Multiply ( RelOriginalMRI_To_AbsOriginalXYZ, MultiplyLeft );

                                        // Transformations outputs
if ( mritransformed ) {

    TFileName           filematrix;


    sprintf ( filematrix,                   "%s." InfixTransform ".MRI Realignment.Realigned MRI to Original MRI.%s",                 (char *) BaseFileNameMore,               "txt" );
    NormToOriginalMRI.WriteFile ( filematrix );


    TMatrix44           matinv      = TMatrix44 ( NormToOriginalMRI ).Invert ();
    sprintf ( filematrix,                   "%s." InfixTransform ".MRI Realignment.Original MRI to Realigned MRI.%s",                 (char *) BaseFileNameMore,               "txt" );
    matinv.WriteFile ( filematrix );
    }


if ( MriHead.IsOpen () && xyzcoregistertomri ) {

    TFileName           filematrix;


    sprintf ( filematrix,                   "%s." InfixTransform ".Electrodes Coregistration.Electrodes to Realigned MRI.%s",         (char *) BaseFileNameMore,               "txt" );
    XyzCoregToNorm.WriteFile ( filematrix );


    TMatrix44           matinv      = TMatrix44 ( XyzCoregToNorm ).Invert ();
    sprintf ( filematrix,                   "%s." InfixTransform ".Electrodes Coregistration.Realigned MRI to Electrodes.%s",         (char *) BaseFileNameMore,               "txt" );
    matinv.WriteFile ( filematrix );


    if ( mritransformed ) {

        TMatrix44           xyzcoregtooriginal  = NormToOriginalMRI * XyzCoregToNorm;
        sprintf ( filematrix,                   "%s." InfixTransform ".Electrodes Coregistration.Electrodes to Original MRI.%s",          (char *) BaseFileNameMore,               "txt" );
        xyzcoregtooriginal.WriteFile ( filematrix );


        TMatrix44           matinv      = TMatrix44 ( xyzcoregtooriginal ).Invert ();
        sprintf ( filematrix,                   "%s." InfixTransform ".Electrodes Coregistration.Original MRI to Electrodes.%s",          (char *) BaseFileNameMore,               "txt" );
        matinv.WriteFile ( filematrix );

                                        // save coregistered electrodes to original MRI
        TPoints             xyzpointscoregtooriginal ( xyzpointscoregtonorm );
        NormToOriginalMRI .Apply ( xyzpointscoregtooriginal );
        xyzpointscoregtooriginal.WriteFile ( FileOriginalXyzCoreg, &xyznames );


        TPoints             xyzpointsdisplaycoregtooriginal ( xyzpointsdisplay );
        NormToOriginalMRI.Apply ( xyzpointsdisplaycoregtooriginal );
        xyzpointsdisplaycoregtooriginal.WriteFile ( FileOriginalXyzDisplay, &xyznames );

                                        // also save the center used for gluing
        TPoints             projcenter;
        projcenter.Add ( TPointFloat ( 0, 0, 0 ) );

        normtoxyz        .Apply ( projcenter );     // cancel the XYZ normalization, so we can start from center 0,0,0
        XyzCoregToNorm   .Apply ( projcenter );     // into realigned MRI
        sprintf ( filematrix,                   "%s." InfixTransform ".Electrodes Coregistration.Projection Center in Realigned MRI.%s",  (char *) BaseFileNameMore,               "txt" );
        projcenter.WriteFile ( filematrix );

        NormToOriginalMRI.Apply ( projcenter );     // then into original MRI
        sprintf ( filematrix,                   "%s." InfixTransform ".Electrodes Coregistration.Projection Center in Original MRI.%s",   (char *) BaseFileNameMore,               "txt" );
        projcenter.WriteFile ( filematrix );
        }
    }
                                        // complimentary original copy
else if ( MriHead.IsOpen () && xyzcoregistrationdone && mritransformed ) {

    TPoints             xyzpointsdisplaylocaltooriginal ( xyzpointsdisplay );
    NormToOriginalMRI.Apply ( xyzpointsdisplaylocaltooriginal );
    xyzpointsdisplaylocaltooriginal.WriteFile ( FileOriginalXyzDisplay, &xyznames );
    }

                                        // switch back to relative original MRI coordinates
NormToOriginalMRI.Multiply ( TMatrix44 ( RelOriginalMRI_To_AbsOriginalXYZ ).Invert (), MultiplyLeft );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Lead Field processing: Compute / load / interpolate
if ( leadfieldprocessing ) {

                                        // Lead Field
    int                 numel           = (int) xyzpoints;
    int                 numsolp;
    int                 numsolp3;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( computeleadfield ) {

        Gauge.Next ( gaugecimleadfield, SuperGaugeUpdateTitle );


        numsolp         = (int) solpoints;
        numsolp3        = 3 * numsolp;
        K.resize ( numel, numsolp3 );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Force translate all points to inverse center, Lead Field functions need centered points for the sake of simplicity
        headsurfacepoints  += MriCenterToInverseCenter;     // ExtractSkullMask
        xyzpoints          += MriCenterToInverseCenter;     // xyzpointsspherized, EstimateRadiuses, ComputeLeadFieldLSMAC
        solpoints          += MriCenterToInverseCenter;     // solpointsspherized,

                                        // Head Model has already been computed with MriCenterToInverseCenter, there are no translation anymore
//      SurfaceModel.SetValue ( TranslationX, 0 );
//      SurfaceModel.SetValue ( TranslationY, 0 );
//      SurfaceModel.SetValue ( TranslationZ, 0 );

                                        // Translate the inverse MRI center
        InverseCenter       = MriCenter - TPointDouble ( MriCenterToInverseCenter ); // EstimateRadiuses, ExtractSkullMask

                                        // notify the verbose for the change of origin
        Verbose.NextBlock ();
        Verbose.Put ( "MRI Origin:",                    MriCenter       );
        Verbose.Put ( "Inverse Geometrical Center:",    InverseCenter   );
        Verbose.Put ( "MRI Origin to Inverse Center:",  MriCenterToInverseCenter );
        Verbose.Flush ();


#if defined(InvMatrixMoreOutputs)
                                        // show some coordinates to the user, he/she loves that
//        MriCenterToInverseCenter.Show ( "MriCenterToInverseCenter" );
//        MriCenter           .Show ( "MriCenter" );
//        InverseCenter       .Show ( "InverseCenter" );

                                        // Checking what the spherization is actually doing
        TFileName               _file;

        StringCopy  ( _file, CrisTransfer.BaseFileName, "\\Inverse Centered Solution Points.spi" );
        solpoints.WriteFile ( _file );

        StringCopy  ( _file, CrisTransfer.BaseFileName, "\\Inverse Centered Electrodes.xyz" );
        xyzpoints.WriteFile ( _file );

        TPoints             sphsolpoints    = SurfaceModel.Spherize ( solpoints );
        StringCopy  ( _file, CrisTransfer.BaseFileName, "\\Spherized Solution Points.spi" );
        sphsolpoints.WriteFile ( _file );

        TPoints             sphxyzpoints    = SurfaceModel.Spherize ( xyzpoints );
        StringCopy  ( _file, CrisTransfer.BaseFileName, "\\Spherized Electrodes by Model.xyz" );
        sphxyzpoints.WriteFile ( _file );

        sphxyzpoints = xyzpoints;
        sphxyzpoints += SurfaceModel.GetTranslation ();
        sphxyzpoints.Normalize ();
        StringCopy  ( _file, CrisTransfer.BaseFileName, "\\Spherized Electrodes by Norm.xyz" );
        sphxyzpoints.WriteFile ( _file );
#endif

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        Gauge.Next ( gaugecimleadfield, SuperGaugeUpdateTitle );

                                    // Normalize the electrodes - could also be controlled by some parameter, to have everything explicitly stated(?)
//      xyzpointsspherized  = xyzpoints;
//      xyzpointsspherized.Normalize ();                            // mathematically spherical, centered on inverse center

//      xyzpointsspherized  = SurfaceModel.Spherize ( xyzpoints );  // alternative way: model spherization, not perfectly spherical, and spherization is relative to model center


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        TArray3<float>          TissuesRadii;
        bool                    radiusok        = true;

                                        // Tissues thicknesses / radii will be estimated from the T1 MRIs
                                        // Historically the only method available, and although it lacks a bit of accuracy,
                                        // it seems to be robust enough for nearly all cases, including the difficult ones
        if ( isthicknessfrommri ) {

            FctParams           p;

            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

            Gauge.Next ( gaugecimleadfield, SuperGaugeUpdateTitle );

            Volume              head ( *MriHead ->GetData () );

            if ( smoothmri ) {
                p ( FilterParamDiameter )     = 3.47; // 5;
                head.Filter ( FilterTypeGaussian, p, true );
                }


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

            Gauge.Next ( gaugecimleadfield, SuperGaugeUpdateTitle );
                                        // Safe limit for brain surface
            Volume              brainlimit  ( *MriBrain->GetData () );

                                        // Force converting to mask
            if ( MriBrain->IsMask () ) {
                p ( FilterParamBinarized )     = 1;

                brainlimit.Filter ( FilterTypeBinarize, p );
                }
            else {
                p ( FilterParamThresholdMin )     = MriBrain->GetBackgroundValue ();
                p ( FilterParamThresholdMax )     = FLT_MAX;
                p ( FilterParamThresholdBin )     = 1;

                brainlimit.Filter ( FilterTypeThresholdBinarize, p );
                }


            p ( FilterParamDiameter )   = 1;
            brainlimit.Filter ( FilterTypeClose, p );


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Some users still provide brains without cerebellum, which we need to assess brain limits
                                        // We are going to run some local skull stripping and use it to upgrade our current brain
            Gauge.Next ( gaugecimleadfield, SuperGaugeUpdateTitle );

            Volume              localbrain ( head );

            p ( FilterParamSkullStrippingMethod     ) = SkullStripping1B;   // current best
            p ( FilterParamSkullStrippingVoxelSize  ) = 1.5 * MriHead->GetVoxelSize ().Mean ();         // it seems to work with templates files, too...
            p ( FilterParamSkullStrippingIsTemplate ) = IsTemplateFileName ( MriHead->GetDocPath () );

            localbrain.Filter   ( FilterTypeSkullStripping, p, true );

                                        // upgrade using the cerebellum part only - we want to avoid any possible artifacts on the top, cortex part
            for ( int x = 0; x < brainlimit.GetDim1 ();     x++ )
            for ( int y = 0; y < brainlimit.GetDim2 () / 2; y++ )   // rear
            for ( int z = 0; z < brainlimit.GetDim3 () / 2; z++ )   // bottom
                                        // inject into brainlimit directly
                brainlimit ( x, y, z )  = (bool) brainlimit ( x, y, z )  || (bool) localbrain ( x, y, z );
 
                                        // upgrade using the whole new brain?
            //brainlimit |= localbrain;


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Safe limit for skull radius search
            Gauge.Next ( gaugecimleadfield, SuperGaugeUpdateTitle );

            Volume              skulllimit ( brainlimit );

                                        // add a bit of post-processing
            p ( FilterParamDiameter )   = 8.0;
            skulllimit.Filter ( FilterTypeDilate, p );


            Gauge.Next ( gaugecimleadfield, SuperGaugeUpdateTitle );

            p ( FilterParamDiameter )   = 9.0;      // could erode even a tiny bit more, by using a Min filter
            skulllimit.Filter ( FilterTypeErode,  p );


            Gauge.Next ( gaugecimleadfield, SuperGaugeUpdateTitle );

            p ( FilterParamDiameter )   = 6.0;
            p ( FilterParamNumRelax )   = 1;
            skulllimit.Filter ( FilterTypeRelax,  p );

                                        // Assessing those intermediate volumes?
          //TFileName       _file;
          //StringCopy      ( _file,    BaseFileName,   ".localbrain", "." DefaultMriExt       );
          //localbrain.WriteFile    ( _file );
          //StringCopy      ( _file,    BaseFileName,   ".skulllimit", "." DefaultMriExt       );
          //skulllimit.WriteFile    ( _file );
          //StringCopy      ( _file,    BaseFileName,   ".brainlimit", "." DefaultMriExt       );
          //brainlimit.WriteFile    ( _file );

            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // estimate for each electrode
            Gauge.Next ( gaugecimleadfield, SuperGaugeUpdateTitle );

            radiusok    =

            EstimateTissuesRadii_T1     (
                                        xyzpoints,
//                                      SpatialFilterNone,                                  0,          // could lead to bad radii, but results are more anatomically precise - but do we really need that for our case?
                                        SpatialFilterInterseptileWeightedMean,              Xyz,        // safer, results are smoother but a bit less precise
                                        head,                           skulllimit,         brainlimit,
                                        TPointFloat ( InverseCenter ),
                                        MriHead->GetVoxelSize (),
                                        ajustskullthickness,            skullthickness,                 // could also work for the species than human, but not tested
                                        TissuesRadii
                                        );

            } // isthicknessfrommri


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Tissues thicknesses / radii will be estimated from the already segmented volume
                                        // This is the latest and best method, as the segmentation has to be done and assessed beforehand.
        else if ( isthicknessfromtissues ) {

//          Gauge.Next ( gaugecimmritissues, SuperGaugeUpdateTitle );

            if ( lfpreset.IsGeometryLSMAC () ) {

                radiusok    =

                EstimateTissuesRadii_Segmentation (
                                            xyzpoints,
                                          //SpatialFilterNone,                      0,              // smoothing off
                                            SpatialFilterInterquartileMean,         Xyz,            // heavy filter (actually applied only to the CSF tissue)
                                            *MriTissues->GetData (),                MriTissues->GetOrigin (),
                                            MriCenter,                              MriHead->GetVoxelSize (),
                                            TPointFloat ( InverseCenter ),
                                            ajustskullthickness,                    skullthickness, // could also work for the species than human, but not tested
                                            TissuesRadii,
                                            &Verbose
                                        );
                } // IsGeometryLSMAC


            MriTissues.Close ();
            } // isthicknessfromtissues


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        #if defined(InvMatrixMoreOutputs)
                                        // save spherized inner and outer skull
        TFileName           _filexyz;
        TPoints             sphintcsf   ( xyzpoints );
        TPoints             sphintskull ( xyzpoints );
        TPoints             sphextskull ( xyzpoints );

        sphintcsf  .Normalize ();
        sphintskull.Normalize ();
        sphextskull.Normalize ();

        for ( int i = 0; i < (int) sphintcsf; i++ )     sphintcsf  [ i ]  *= TissuesRadii ( i, CsfIndex,InnerRel );
        for ( int i = 0; i < (int) sphintskull; i++ )   sphintskull[ i ]  *= TissuesRadii ( i, SkullIndex,InnerRel );
        for ( int i = 0; i < (int) sphextskull; i++ )   sphextskull[ i ]  *= TissuesRadii ( i, SkullIndex,OuterRel );

        StringCopy ( _filexyz, CrisTransfer.BaseFileName, "\\More\\", "Spherized.Inner Csf."   FILEEXT_XYZ );
        sphintcsf  .WriteFile ( _filexyz, &xyznames );
        StringCopy ( _filexyz, CrisTransfer.BaseFileName, "\\More\\", "Spherized.Inner Skull." FILEEXT_XYZ );
        sphintskull.WriteFile ( _filexyz, &xyznames );
        StringCopy ( _filexyz, CrisTransfer.BaseFileName, "\\More\\", "Spherized.Outer Skull." FILEEXT_XYZ );
        sphextskull.WriteFile ( _filexyz, &xyznames );
        #endif


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( TissuesRadii.IsAllocated () ) {

                                        // verbose update - even if there were errors
            {
            Verbose.NextBlock ();

            TGoEasyStats        statradius ( NumTissuesLimits, numel );

                                        // Output stats tissues by tissues
            for ( TIteratorSelectedForward ti ( seltissues ); (bool) ti; ++ti ) {

                                        // tissues was not actually scanned, like brain
                if ( TissuesRadii ( 0, ti(), 0 ) == 0 )
                    continue;

                                        // first, do the stats on all variables
                statradius.Reset ();

                for ( int ei = 0; ei < numel; ei++ )
                for ( int li = 0; li < NumTissuesLimits; li++ )
                    statradius[ li ].Add ( TissuesRadii ( ei, ti(), li ) );


                //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                Verbose.NextTopic ( StringCopy  ( buff, TissuesSpecs[ ti() ].Text, " Radii:" ) );
                Verbose.NextLine ( 1 );


                Verbose.ResetTable ();

                                                    // field names
                Verbose.TableColNames.Add ( "Electrode" );
                for ( int li = 0; li < NumTissuesLimits; li++ )
                    Verbose.TableColNames.Add ( StringCopy ( buff, TissuesSpecs[ ti() ].Text, " ", TissuesLimitsString[ li ] ) );

                for ( int ei = 0; ei < numel; ei++ )
                    Verbose.TableRowNames.Add ( xyznames ( ei ) );

                Verbose.TableRowNames.Add ( "Median" );
                Verbose.TableRowNames.Add ( "Sn" );


                Verbose.BeginTable ( 22 );

                                                    // data & stats
                for ( int ei = 0; ei < numel; ei++ )
                for ( int li = 0; li < NumTissuesLimits; li++ )

                    Verbose.PutTable ( TissuesRadii ( ei, ti(), li )     * ( IsTissuesLimitRel ( li ) ? 100 : 1 ), 2 );

                for ( int li = 0; li < NumTissuesLimits; li++ )
                    Verbose.PutTable ( statradius[ li ].Median ( false ) * ( IsTissuesLimitRel ( li ) ? 100 : 1 ), 2 );

                for ( int li = 0; li < NumTissuesLimits; li++ )
                    Verbose.PutTable ( statradius[ li ].Sn ( 1000 )      * ( IsTissuesLimitRel ( li ) ? 100 : 1 ), 2 );


                Verbose.EndTable ();
                } // for seltissues

            Verbose.Flush ();
            }

                                        // Handy file to visualize the thicknesses as maps
            {
            TExportTracks       exprad;

            StringCopy  ( exprad.Filename, fileradii );
            exprad.SetAtomType ( AtomTypeScalar );
            exprad.NumTracks        = numel;
            exprad.NumTime          = (int) seltissues - 1; // BrainIndex not outputted

            exprad.ElectrodesNames.Set ( exprad.NumTracks, 256 );

            for ( int ei = 0; ei < numel; ei++ )
                StringCopy   ( exprad.ElectrodesNames[ ei ], Xyz->GetElectrodeName ( ei ) );

            for ( TIteratorSelectedForward ti ( seltissues ); (bool) ti; ++ti )
                if ( ti() != BrainIndex )
                    for ( int ei = 0; ei < numel; ei++ )    exprad.Write ( TissuesRadii ( ei, ti(), ThickAbs ) );
            }
            } // TissuesRadii


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( ! radiusok ) {

//          if ( interactive )
                ShowMessage (   "There appear to be some major errors in the scalp or skull thicknesses estimations!" NewLine 
                                "(check the verbose file for details)" NewLine 
                                NewLine 
                                "This will end your processing now, bye bye...", MriHead->GetTitle (), ShowMessageWarning, this );

            if ( interactive )

                for ( int ei = 0; ei < numel; ei++ ) {

                    if ( TissuesRadii ( ei, CsfIndex,           ThickAbs ) < 0
                      || TissuesRadii ( ei, SkullIndex,         ThickAbs ) < 0
                      || TissuesRadii ( ei, SkullSpongyIndex,   ThickAbs ) < 0
                      || TissuesRadii ( ei, ScalpIndex,         ThickAbs ) < 0 ) {

                        StringCopy      ( buff, "Electrode ", Xyz->GetElectrodeName ( ei ), " / #", IntegerToString ( ei + 1 ), " has the wrong radii:" NewLine );

                        if ( seltissues[ CsfIndex           ] )     StringAppend    ( buff, NewLine Tab "Csf          Thickness = ", FloatToString ( TissuesRadii ( ei, CsfIndex,          ThickAbs ), 2 ) );
                        if ( seltissues[ SkullIndex         ] )     StringAppend    ( buff, NewLine Tab "Skull        Thickness = ", FloatToString ( TissuesRadii ( ei, SkullIndex,        ThickAbs ), 2 ) );
                        if ( seltissues[ SkullSpongyIndex   ] )     StringAppend    ( buff, NewLine Tab "Spongy Skull Thickness = ", FloatToString ( TissuesRadii ( ei, SkullSpongyIndex,  ThickAbs ), 2 ) );
                        if ( seltissues[ ScalpIndex         ] )     StringAppend    ( buff, NewLine Tab "Scalp        Thickness = ", FloatToString ( TissuesRadii ( ei, ScalpIndex,        ThickAbs ), 2 ) );

                        ShowMessage     ( buff, "Estimating Radii", ShowMessageWarning, this );
                        }
                    }

            goto endprocessing;
            } // ! radiusok


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( TissuesRadii.IsAllocated () )
                                        // saving each electrodes with their corresponding positions for each layer
            WriteTissuesRadiiToFile (   TissuesRadii,       seltissues,
                                        xyzpoints,          xyznames,
                                        MriCenter,          InverseCenter,
                                        reorientmris ? FileLocalRadiiLayers : fileradiilayers
                                    );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Finally ready to compute

#if defined(InvMatrixMoreOutputs)
                                        // Only for output
                                        // spherized & normalized
        solpointsspherized  = SurfaceModel.Spherize ( solpoints );

        StringCopy ( _file, BaseFileNameMore, ".Spherized SPs." FILEEXT_SPIRR );
        solpointsspherized.WriteFile ( _file, &solpointsnames );
        StringCopy ( _file, BaseFileNameMore, ".Spherized Electrodes." FILEEXT_XYZ );
        xyzpointsspherized.WriteFile ( _file, &xyznames );

//                                      // ? SP transformed by Head Model ?
//      TPoints             solpointsspherizedhm;
//
//      for ( int i = 0; i < (int) solpoints; i++ ) {
//
//          TPointFloat         surf         ( solpoints[ i ] );
//          TPointFloat         sphm         ( solpoints[ i ] );
//
//          surf      .Normalize ();
//          sphm      .Normalize ();
//
//          SurfaceModel.Transform ( surf, true );
//                                      // instead of norm=1, use a percentage toland INSIDE model, even though not build for that purpose...
//          sphm   *= solpoints[ i ]->Norm () / surf.Norm ();
//
//          SurfaceModel.Transform ( sphm, true );
//                                      // now, point is scaled according to the model surface, rescale to be normalized
//          sphm   /= surf.Norm ();
//
//          solpointsspherizedhm.Add ( sphm );
//          }
//
//      StringCopy ( _file, BaseFileNameMore, ".Spherized SPs.Head Model." FILEEXT_SPIRR );
//      solpointsspherizedhm.WriteFile ( _file, &solpointsnames );
#endif

                                        // note that spsrejected is not needed at that point, and is (or is considered) empty
        Gauge.Next ( gaugecimleadfield, SuperGaugeUpdateTitle );


        if ( lfpreset.IsGeometryLSMAC () ) {

            ComputeLeadFieldLSMAC   (   lfpreset,
                                        xyzpoints,
                                        solpoints, 
                                        SurfaceModel, 
                                        sigma,
                                        TissuesRadii,
                                        K
                                    );

            } // if IsGeometryLSMAC

        } // computeleadfield


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    else if ( loadleadfield ) {

        Gauge.Next ( gaugecimleadfield, SuperGaugeUpdateTitle );

                                        // try to load the electrodes names
        if ( CrisTransfer.HasXyz () && xyznames.IsEmpty () )
                                        // retrieve the names of electrodes
            xyzpoints.ReadFile ( FileInputXyz, &xyznames );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Go through a dedicated class
        Gauge.Next ( gaugecimleadfield, SuperGaugeUpdateTitle );


        TLeadField          leadfield;

                                        // try opening & check
        if ( ! leadfield.Set ( FileInputLeadFieldLoad ) ) {

            ShowMessage     (   "There seems to be a problem with the Lead Field file, which can be either:" NewLine 
                                NewLine 
                                Tab "- The file type is not recognized" NewLine 
                                Tab "- The Lead Field data type is not of vectorial type" NewLine 
                                Tab "- The file itself might be somehow corrupted" NewLine 
                                NewLine 
                                "This will end your processing now, bye bye...", FileInputLeadFieldLoad, ShowMessageWarning, this );
            goto endprocessing;
            }

                                        // try retrieving the matrix & check
        if ( ! leadfield.ReadFile ( K ) ) {

            ShowMessage     (   "There seems to be a problem with the Lead Field file," NewLine 
                                "check the file type is correct." NewLine 
                                NewLine 
                                "This will end your processing now, bye bye...", FileInputLeadFieldLoad, ShowMessageWarning, this );
            goto endprocessing;
            }

                                        // Lead Field file opened successfully
        numel           = leadfield.GetNumElectrodes     ();
        numsolp         = leadfield.GetNumSolutionPoints ();
        numsolp3        = 3 * numsolp;

                                        // now compare #SPs from original solution points file and LF file
        if ( samespsaslf && loadleadfield && numsolp != numleadfieldsolpoints ) {

            if ( interactive ) {

                if ( ! GetAnswerFromUser    (   "The given Solution Points and Lead Field files seem to have different sizes." NewLine 
                                                "Please check again your input." NewLine 
                                                NewLine 
                                                "Do you still want to proceed (not recommended)?", FileInputSpLeadField, this ) )
                    goto endprocessing;
                } // interactive
            else {
                ShowMessage     (   "The given Solution Points and Lead Field files seem to have different sizes." NewLine 
                                    "Please check again your input." NewLine 
                                    NewLine 
                                    "This will end your processing now, bye bye...", FileInputSpLeadField, ShowMessageWarning, this );
                goto endprocessing;
                }
            } // ! regular


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        Gauge.Next ( gaugecimleadfield, SuperGaugeUpdateTitle );
                                        // interpolate loaded LF into another one, with our computed SPs
                                        // we could discriminate loading a SP file which is actually a downsampling of the existing points?
        if      ( interpolateleadfield ) {

            TPoints             solpointsback ( solpoints );

            if ( mritransformed )
                NormToAbsOriginalMRI.Apply ( solpointsback );

                                        // problematic points are still there and can also be updated here
                                        // the LF is set to 0 at these positions (?)
            InterpolateLeadFieldLinear ( K, leadfieldsolpoints, solpointsback, spsrejected );


//            if ( computesps && (bool) spsrejected )
//                DBGV ( (int) spsrejected, "FYI, computing SPs has some points rejected - this shouldn't happen" );
                // remove points, overwrite again? maybe not, don't restrict our points, as they can be reused later
            } // interpolateleadfield


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // if the SPs have been downsampled, then downsample the LF too!
        else if ( downsampleleadfield ) {
                                        // solpoints have already been downsampled, now do the same on the LF
            RejectPointsFromLeadField ( K, spsrejected );

            } // downsampleleadfield


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        } // loadleadfield


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // time to save the Lead Field files
    Gauge.Next ( gaugecimleadfield, SuperGaugeUpdateTitle );


    if ( ! TLeadField::WriteFile ( K, fileleadfield    ) && interactive )
        ShowMessage ( "Lead Field has not been saved correctly!", fileleadfield,    ShowMessageWarning, this );

    if ( ! TLeadField::WriteFile ( K, fileleadfieldris ) && interactive )
        ShowMessage ( "Lead Field has not been saved correctly!", fileleadfieldris, ShowMessageWarning, this );

    if ( ! TLeadField::WriteFile ( K, fileleadfieldeeg ) && interactive )
        ShowMessage ( "Lead Field has not been saved correctly!", fileleadfieldeeg, ShowMessageWarning, this );

                                        // we need to re-allocate & reset in this case
    if ( samespsaslf )
        spsrejected     = TSelection ( (int) solpoints, OrderSorted );


    } // leadfieldprocessing


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // This implies having a valid Lead Field here!
if ( inverseprocessing ) {

    Gauge.Next ( gaugeciminverse, SuperGaugeUpdateTitle );
                                        // by security, scan for null LF's SP -> update spsrejected
                                        // use a local selection, for proper output
    TSelection          nullsps ( spsrejected.Size (), OrderSorted );

    CheckNullLeadField ( K, nullsps );


    if ( (bool) nullsps ) {

        char                buff[ 1 * KiloByte ];
                                // always show to the user
        StringCopy      ( buff, IntegerToString ( (int) nullsps ), " Solution Points have an erroneous Lead Field column (null or NaN values)." NewLine );
        StringAppend    ( buff, "These points will be kept in the output inverse space, but setting the Inverse Matrix to 0 at these positions." NewLine );

        if ( interactive )
            ShowMessage ( buff, "Lead Field", ShowMessageWarning, this );

                                        // save this to verbose!
        Verbose.NextBlock ();
        Verbose.NextTopic ( "Lead Field Warning:" );
        (ofstream&) Verbose << buff;

                                        // write list of outside points
        Verbose.NextLine ();

        Verbose.ResetTable ();

        Verbose.TableColNames.Add ( "" );
        Verbose.TableColNames.Add ( "Index" );
        Verbose.TableColNames.Add ( "X" );
        Verbose.TableColNames.Add ( "Y" );
        Verbose.TableColNames.Add ( "Z" );
        Verbose.TableColNames.Add ( "Name" );


        Verbose.BeginTable ( 10 );


        for ( int sp = 0; sp < (int) solpoints; sp++ )

            if ( nullsps [ sp ] ) {

                Verbose.PutTable ( sp + 1 );
                Verbose.PutTable ( solpoints[ sp ].X, 1 );
                Verbose.PutTable ( solpoints[ sp ].Y, 1 );
                Verbose.PutTable ( solpoints[ sp ].Z, 1 );
                Verbose.PutTable ( solpointsnames[ sp ] );
                }


        Verbose.EndTable ();

        Verbose.NextLine ();

        Verbose.Flush ();

                                        // cumulate to our global list of rejected SPs
        spsrejected    += nullsps;
        }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Each inverse could add more points to be rejected, for reason peculiar to each processing
                                        // f.ex. LORETA and LAURA needs a minimum amount of neighbors (at least 1)
                                        // so spsrejected is always given as a copy
    if ( inversemn ) {
        Gauge.Next ( gaugeciminverse, SuperGaugeUpdateTitle );

        ComputeInverseMN    (
                            K,          spsrejected, 
                            regularization, 
                            xyznames,   solpointsnames, 
                            filemneis 
                            );

        Gauge.Next ( gaugeciminverse, SuperGaugeUpdateTitle );

        if ( computeresmat )
            ComputeResolutionMatrix (   K,              solpointsgrid,
                                        filemneis,
                                        StringToRegularization ( DefaultRegularization ),
                                        0,              filemnresmat,               0 );
        }


    if ( inversewmn ) {
        Gauge.Next ( gaugeciminverse, SuperGaugeUpdateTitle );

        ComputeInverseWMN   (   
                            K,          spsrejected, 
                            regularization, 
                            xyznames,   solpointsnames, 
                            filewmneis 
                            );

        Gauge.Next ( gaugeciminverse, SuperGaugeUpdateTitle );

        if ( computeresmat )
            ComputeResolutionMatrix (   K,              solpointsgrid,
                                        filewmneis,
                                        StringToRegularization ( DefaultRegularization ),
                                        0,              filewmnresmat,              0 );
        }


    if ( inversedale ) {
        Gauge.Next ( gaugeciminverse, SuperGaugeUpdateTitle );

        ComputeInverseDale  (
                            K,          spsrejected,
                            regularization,
                            xyznames,   solpointsnames,    
                            filedaleis
                            );

        Gauge.Next ( gaugeciminverse, SuperGaugeUpdateTitle );

        if ( computeresmat )
            ComputeResolutionMatrix (   
                                    K,              solpointsgrid,
                                    filedaleis,
                                    StringToRegularization ( DefaultRegularization ),
                                    0,              filedaleresmat,             0 
                                    );
        }


    if ( inversesloreta ) {
        Gauge.Next ( gaugeciminverse, SuperGaugeUpdateTitle );

        ComputeInverseSLoreta   (   
                                K,          spsrejected,
                                regularization,
                                xyznames,   solpointsnames,    
                                filesloretais
                                );

        Gauge.Next ( gaugeciminverse, SuperGaugeUpdateTitle );

        if ( computeresmat )
            ComputeResolutionMatrix (   K,              solpointsgrid,
                                        filesloretais,
                                        StringToRegularization ( DefaultRegularization ),
                                        0,              filesloretaresmat,          0 );
        }


    if ( inverseloreta ) {
        Gauge.Next ( gaugeciminverse, SuperGaugeUpdateTitle );

        ComputeInverseLoreta    (
                                K,                  solpointsgrid,      spsrejected,
                                loretaneighborhood, neighbors26to18,
                                regularization,
                                xyznames,           solpointsnames,            
                                fileloretais,
                                &Verbose
                                );

        Gauge.Next ( gaugeciminverse, SuperGaugeUpdateTitle );

        if ( computeresmat )
            ComputeResolutionMatrix (   K,              solpointsgrid,
                                        fileloretais,
                                        StringToRegularization ( DefaultRegularization ),
                                        0,              fileloretaresmat,           0 );
        }


    if ( inverselaura ) {
        Gauge.Next ( gaugeciminverse, SuperGaugeUpdateTitle );

        ComputeInverseLaura (
                            K,                  solpointsgrid,      spsrejected,
                            lauraneighborhood,  LauraPower,         neighbors26to18,
                            regularization,
                            xyznames,           solpointsnames,
                            filelaurais,
                            &Verbose
                            );

        Gauge.Next ( gaugeciminverse, SuperGaugeUpdateTitle );

        if ( computeresmat )
            ComputeResolutionMatrix (   K,              solpointsgrid,
                                        filelaurais,
                                        StringToRegularization ( DefaultRegularization ),
                                        0,              filelauraresmat,            0 );
        }


    if ( inverseeloreta ) {
        Gauge.Next ( gaugeciminverse, SuperGaugeUpdateTitle );

        ComputeInverseELoreta   (
                                K,          spsrejected,
                                regularization,
                                xyznames,   solpointsnames,    
                                fileeloretais
                                );

        Gauge.Next ( gaugeciminverse, SuperGaugeUpdateTitle );

        if ( computeresmat )
            ComputeResolutionMatrix (   K,              solpointsgrid,
                                        fileeloretais,
                                        StringToRegularization ( DefaultRegularization ),
                                        0,              fileeloretaresmat,          0 );
        }

    } // inverseprocessing


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

endprocessing:

                                        // if this directory is empty, just delete it
if ( IsEmptyDirectory ( BaseDirMore ) )
    NukeDirectory ( BaseDirMore );

if ( IsEmptyDirectory ( BaseDirLocal ) )
    NukeDirectory ( BaseDirLocal );


SetProcessPriority ();

WindowMaximize ( CartoolMainWindow );

Gauge.FinishParts ();

CartoolApplication->SetMainTitle ( CreatingInverseTitle, BaseDir /*fileoutprefix*/, Gauge );

Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
