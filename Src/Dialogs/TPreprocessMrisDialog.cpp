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

#include    "TPreprocessMrisDialog.h"
#include    "PreprocessMris.h"

#include    "Strings.Utils.h"
#include    "CartoolTypes.h"
#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"

#include    "Geometry.TOrientation.h"

#include    "TVolumeDoc.h"
#include    "TCreateInverseMatricesDialog.h"    // DefaultNumSolutionPoints, DefaultResSolutionPoints

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

OptimizeOff

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // declare this global variable
TPreprocessMrisStruct   MriPreprocTransfer;


const char  PreprocessMrisPresetsNames[ NumMriPreprocPresets ][ 64 ] =
            {
            "No Pre-Processing",
            "",
            "Full Head Clean-Up",
            "Resampling",
            "Axis Reorienting",
            "Setting New Origin",
            "Skull-Stripping",
            "Skull-Stripping, Grey Mask & Computing Solution Points",
            "",
            "Cleaning, Resampling, Reorienting & Setting New Origin",
            "Inverse Matrices Full Head Pipe-Line",
            };


const char  MRISequenceNames[ NumMRISequenceTypes ][ 16 ] =
            {
            "Unknown",
            "T1",
            "T1 + Gadolinium",
//          "T2",
            };


const char  MriSkullStrippingPresetsNames[ NumMriSkullStrippingPresets ][ 64 ] =
            {
            "No Skull-Stripping",
            "Skull-Stripping Method 1 - Best",
            "Skull-Stripping Method 2 - Good",
            "Skull-Stripping Method 3 - OK-ish",
            };

const char  GreyMaskPresetsNames[ NumGreyMaskPresets ][ 32 ] =
            {
            "No Grey Mask",
            "Thin Grey Mask",
            "Regular Grey Mask",
            "Thick Grey Mask",
            "Whole Brain Mask",
            };

const char  SPPresetsNames[ NumSPPresets ][ 64 ] =
            {
            "No Solution Points",
            "Computing Solution Points",
            "Porting Template Solution Points",
            };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TPreprocessMrisStruct::TPreprocessMrisStruct ()
{
Presets.Clear ();
for ( int i = 0; i < NumMriPreprocPresets; i++ )
    Presets.AddString ( PreprocessMrisPresetsNames[ i ], i == MriPreprocPresetDefault );

PresetsMriType.Clear ();
for ( int i = 0; i < NumMRISequenceTypes; i++ )
    PresetsMriType.AddString ( MRISequenceNames[ i ], i == MRISequenceDefault );

Isotropic               = BoolToCheck ( true  );

FullHeadCleanUp         = BoolToCheck ( true  );

Resizing                = BoolToCheck ( true  );
ResizingVoxel           = BoolToCheck ( true  );
ResizingRatio           = BoolToCheck ( false );
StringCopy              ( ResizingVoxelValue,   "1" );
StringCopy              ( ResizingRatioValue,   "2" );

Reorienting             = BoolToCheck ( true  );
ReorientingRas          = BoolToCheck ( true  );
ReorientingOther        = BoolToCheck ( false );
StringCopy              ( ReorientingX, "R" );
StringCopy              ( ReorientingY, "A" );
StringCopy              ( ReorientingZ, "S" );
AlignSagittal           = BoolToCheck ( true  );
AlignTransverse         = BoolToCheck ( true  );

SettingOrigin           = BoolToCheck ( true  );
SettingOriginIfNotSet   = BoolToCheck ( false );
SettingOriginAlways     = BoolToCheck ( true  );
OriginMni               = BoolToCheck ( true  );
OriginCenter            = BoolToCheck ( false );
OriginFixed             = BoolToCheck ( false );
StringCopy              ( OriginX, "128" );
StringCopy              ( OriginY, "128" );
StringCopy              ( OriginZ, "128" );

SizeTransformed         = BoolToCheck ( false );
SizeOptimal             = BoolToCheck ( true  );
SizeUser                = BoolToCheck ( false );
StringCopy              ( SizeUserX, "256" );
StringCopy              ( SizeUserY, "256" );
StringCopy              ( SizeUserZ, "256" );

PresetsSkullStripping.Clear ();
for ( int i = 0; i < NumMriSkullStrippingPresets; i++ )
    PresetsSkullStripping.AddString ( MriSkullStrippingPresetsNames[ i ], i == MriSkullStrippingPresetDefault );
BFC                     = BoolToCheck ( true  );

PresetsGrey.Clear ();
for ( int i = 0; i < NumGreyMaskPresets; i++ )
    PresetsGrey.AddString ( GreyMaskPresetsNames[ i ], i == GreyMaskPresetDefault );

PresetsSP.Clear ();
for ( int i = 0; i < NumSPPresets; i++ )
    PresetsSP.AddString ( SPPresetsNames[ i ], i == SPPresetDefault );

SPNumber                = BoolToCheck ( false );
IntegerToString         ( SPNumberEdit,     DefaultNumSolutionPoints );
SPResolution            = BoolToCheck ( true  );
FloatToString           ( SPResolutionEdit, DefaultResSolutionPoints );
ClearString             ( SPFromBrain );
ClearString             ( SPFromSP );

ClearString             ( InfixFilename );
OpenAuto                = BoolToCheck ( true  );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TPreprocessMrisDialog, TBaseDialog )

    EV_WM_DROPFILES,

    EV_CBN_SELCHANGE            ( IDC_MRIPREPROCPRESETS,        EvPresetsChange ),
    EV_CBN_SELCHANGE            ( IDC_MRITYPEPRESETS,           EvPresetsMriTypeChange ),

    EV_COMMAND_ENABLE           ( IDC_RESIZEVOXEL,              CmResizingEnable ),
    EV_COMMAND_ENABLE           ( IDC_RESIZERATIO,              CmResizingEnable ),
    EV_COMMAND_ENABLE           ( IDC_RESIZEVOXELVALUE,         CmVoxelEnable ),
    EV_COMMAND_ENABLE           ( IDC_RESIZERATIOVALUE,         CmRatioEnable ),

    EV_COMMAND                  ( IDC_REORIENTING,              CmAlignTransverse ),
    EV_COMMAND                  ( IDC_ALIGNSAGITTAL,            CmAlignTransverse ),
    EV_COMMAND                  ( IDC_ALIGNTRANSVERSE,          CmAlignTransverse ),
    EV_COMMAND_ENABLE           ( IDC_REORIENTINGRAS,           CmReorientingEnable ),
    EV_COMMAND_ENABLE           ( IDC_REORIENTINGOTHER,         CmReorientingEnable ),
    EV_COMMAND_ENABLE           ( IDC_REORIENTINGX,             CmReorientingOtherEnable ),
    EV_COMMAND_ENABLE           ( IDC_REORIENTINGY,             CmReorientingOtherEnable ),
    EV_COMMAND_ENABLE           ( IDC_REORIENTINGZ,             CmReorientingOtherEnable ),
    EV_COMMAND_ENABLE           ( IDC_ALIGNSAGITTAL,            CmReorientingEnable ),
    EV_COMMAND_ENABLE           ( IDC_ALIGNTRANSVERSE,          CmAlignSagittalEnable ),

    EV_COMMAND                  ( IDC_SETTINGORIGIN,            CmOriginMni ),
    EV_COMMAND                  ( IDC_ORIGINMNI,                CmOriginMni ),
    EV_COMMAND_ENABLE           ( IDC_SETORIGINIFNOTSET,        CmOriginEnable ),
    EV_COMMAND_ENABLE           ( IDC_SETORIGNALWAYS,           CmOriginEnable ),
    EV_COMMAND_ENABLE           ( IDC_ORIGINMNI,                CmOriginEnable ),
    EV_COMMAND_ENABLE           ( IDC_ORIGINCENTER,             CmOriginEnable ),
    EV_COMMAND_ENABLE           ( IDC_ORIGINFIXED,              CmOriginEnable ),
    EV_COMMAND_ENABLE           ( IDC_ORIGINX,                  CmOriginOtherEnable ),
    EV_COMMAND_ENABLE           ( IDC_ORIGINY,                  CmOriginOtherEnable ),
    EV_COMMAND_ENABLE           ( IDC_ORIGINZ,                  CmOriginOtherEnable ),

    EV_COMMAND_ENABLE           ( IDC_SIZEUSERX,                CmSizeUserEnable ),
    EV_COMMAND_ENABLE           ( IDC_SIZEUSERY,                CmSizeUserEnable ),
    EV_COMMAND_ENABLE           ( IDC_SIZEUSERZ,                CmSizeUserEnable ),

    EV_COMMAND_ENABLE           ( IDC_BFC,                      CmSkullStrippingEnable ),

    EV_COMMAND_ENABLE           ( IDC_GREYMASKTYPEPRESETS,      CmSkullStrippingEnable ),

    EV_COMMAND                  ( IDC_BROWSESPFILE,             CmBrowseSpFile ),
    EV_COMMAND                  ( IDC_BROWSEMRIFILE,            CmBrowseBrainFile ),

    EV_COMMAND_ENABLE           ( IDC_SPTYPEPRESETS,            CmSPPresetsEnable ),
    EV_COMMAND_ENABLE           ( IDC_SPNUMBER,                 CmSPButtonEnable ),
    EV_COMMAND_ENABLE           ( IDC_SPNUMBEREDIT,             CmSPComputeNumEnable ),
    EV_COMMAND_ENABLE           ( IDC_SPRESOLUTION,             CmSPButtonEnable ),
    EV_COMMAND_ENABLE           ( IDC_SPRESOLUTIONEDIT,         CmSPComputeRezEnable ),
    EV_COMMAND_ENABLE           ( IDC_SPFROMBRAIN,              CmSPPortEnable ),
    EV_COMMAND_ENABLE           ( IDC_SPFROMSP,                 CmSPPortEnable ),
    EV_COMMAND_ENABLE           ( IDC_BROWSEMRIFILE,            CmSPPortEnable ),
    EV_COMMAND_ENABLE           ( IDC_BROWSESPFILE,             CmSPPortEnable ),

    EV_COMMAND_ENABLE           ( IDC_PROCESSCURRENT,           CmProcessCurrentEnable ),
    EV_COMMAND_ENABLE           ( IDC_PROCESSBATCH,             CmProcessBatchEnable ),

END_RESPONSE_TABLE;


        TPreprocessMrisDialog::TPreprocessMrisDialog ( TWindow* parent, TResId resId, TVolumeDoc* doc )
      : TBaseDialog ( parent, resId ), MRIDoc ( doc )
{
                                        // can also export .data and .seg
StringCopy ( BatchFilesExt, AllMriFilesExt );


Presets                 = new TComboBox ( this, IDC_MRIPREPROCPRESETS );
PresetsMriType          = new TComboBox ( this, IDC_MRITYPEPRESETS );

FullHeadCleanUp         = new TCheckBox ( this, IDC_FULLHEADCLEANUP );

Isotropic               = new TCheckBox ( this, IDC_ISOTROPIC );

Resizing                = new TCheckBox ( this, IDC_RESIZING );
ResizingVoxel           = new TRadioButton ( this, IDC_RESIZEVOXEL );
ResizingRatio           = new TRadioButton ( this, IDC_RESIZERATIO );
ResizingVoxelValue      = new TEdit ( this, IDC_RESIZEVOXELVALUE, EditSizeValue );
ResizingVoxelValue->    SetValidator ( new TFilterValidator ( ValidatorPositiveFloat ) );
ResizingRatioValue      = new TEdit ( this, IDC_RESIZERATIOVALUE, EditSizeValue );
ResizingRatioValue->    SetValidator ( new TFilterValidator ( ValidatorPositiveFloat ) );

Reorienting             = new TCheckBox ( this, IDC_REORIENTING );
ReorientingRas          = new TRadioButton ( this, IDC_REORIENTINGRAS );
ReorientingOther        = new TRadioButton ( this, IDC_REORIENTINGOTHER );
ReorientingX            = new TEdit ( this, IDC_REORIENTINGX, 2 );
ReorientingY            = new TEdit ( this, IDC_REORIENTINGY, 2 );
ReorientingZ            = new TEdit ( this, IDC_REORIENTINGZ, 2 );
ReorientingX->SetValidator ( new TFilterValidator ( ValidatorOrientation ) );
ReorientingY->SetValidator ( new TFilterValidator ( ValidatorOrientation ) );
ReorientingZ->SetValidator ( new TFilterValidator ( ValidatorOrientation ) );
AlignSagittal           = new TCheckBox ( this, IDC_ALIGNSAGITTAL );
AlignTransverse         = new TCheckBox ( this, IDC_ALIGNTRANSVERSE );

SettingOrigin           = new TCheckBox    ( this, IDC_SETTINGORIGIN );	
SettingOriginIfNotSet   = new TRadioButton ( this, IDC_SETORIGINIFNOTSET	 );
SettingOriginAlways     = new TRadioButton ( this, IDC_SETORIGNALWAYS		 );
OriginMni               = new TRadioButton ( this, IDC_ORIGINMNI );
OriginCenter            = new TRadioButton ( this, IDC_ORIGINCENTER	);
OriginFixed             = new TRadioButton ( this, IDC_ORIGINFIXED );
OriginX                 = new TEdit ( this, IDC_ORIGINX, EditSizeValue );
OriginY                 = new TEdit ( this, IDC_ORIGINY, EditSizeValue );
OriginZ                 = new TEdit ( this, IDC_ORIGINZ, EditSizeValue );
OriginX->SetValidator ( new TFilterValidator ( ValidatorSignedFloat ) );
OriginY->SetValidator ( new TFilterValidator ( ValidatorSignedFloat ) );
OriginZ->SetValidator ( new TFilterValidator ( ValidatorSignedFloat ) );

SizeTransformed         = new TRadioButton ( this, IDC_SIZETRANSFORMED );
SizeOptimal             = new TRadioButton ( this, IDC_SIZEOPTIMAL );
SizeUser                = new TRadioButton ( this, IDC_SIZEUSER );
SizeUserX               = new TEdit ( this, IDC_SIZEUSERX, EditSizeValue );
SizeUserY               = new TEdit ( this, IDC_SIZEUSERY, EditSizeValue );
SizeUserZ               = new TEdit ( this, IDC_SIZEUSERZ, EditSizeValue );
SizeUserX->SetValidator ( new TFilterValidator ( ValidatorPositiveInteger ) );
SizeUserY->SetValidator ( new TFilterValidator ( ValidatorPositiveInteger ) );
SizeUserZ->SetValidator ( new TFilterValidator ( ValidatorPositiveInteger ) );

PresetsSkullStripping   = new TComboBox ( this, IDC_MRISKULLSTRIPPINGPRESETS );
BFC                     = new TCheckBox ( this, IDC_BFC );

PresetsGrey             = new TComboBox ( this, IDC_GREYMASKTYPEPRESETS );

PresetsSP               = new TComboBox ( this, IDC_SPTYPEPRESETS );
SPNumber                = new TRadioButton ( this, IDC_SPNUMBER );
SPNumberEdit            = new TEdit ( this, IDC_SPNUMBEREDIT, EditSizeValue );
SPResolution            = new TRadioButton ( this, IDC_SPRESOLUTION );
SPResolutionEdit        = new TEdit ( this, IDC_SPRESOLUTIONEDIT, EditSizeValue );
SPFromBrain             = new TEdit ( this, IDC_SPFROMBRAIN, EditSizeText );
SPFromSP                = new TEdit ( this, IDC_SPFROMSP, EditSizeText );

InfixFilename           = new TEdit ( this, IDC_INFIXFILENAME, EditSizeText );
OpenAuto                = new TCheckBox ( this, IDC_OPENAUTO );


                                        // are we entering batch?
BatchProcessing     = doc == 0;


SetTransferBuffer ( &MriPreprocTransfer );

                                        // force setting auto-opening according to interactive or batch mode
//MriPreprocTransfer.OpenAuto     = BoolToCheck ( ! BatchProcessing );
}


        TPreprocessMrisDialog::~TPreprocessMrisDialog ()
{
delete  Presets;                delete  PresetsMriType;
delete  FullHeadCleanUp;
delete  Isotropic;
delete  Resizing;               delete  ResizingVoxel;          delete  ResizingRatio;
delete  ResizingVoxelValue;     delete  ResizingRatioValue;
delete  Reorienting;            delete  ReorientingRas;         delete  ReorientingOther;
delete  ReorientingX;           delete  ReorientingY;           delete  ReorientingZ;
delete  AlignSagittal;          delete  AlignTransverse;
delete  SettingOrigin;          delete  SettingOriginIfNotSet;  delete  SettingOriginAlways;
delete  OriginMni;              delete  OriginCenter;           delete  OriginFixed;
delete  OriginX;                delete  OriginY;                delete  OriginZ;
delete  SizeTransformed;        delete  SizeOptimal;            delete  SizeUser;
delete  SizeUserX;              delete  SizeUserY;              delete  SizeUserZ;
delete  PresetsSkullStripping;  delete  BFC;
delete  PresetsGrey;
delete  PresetsSP;
delete  SPNumber;               delete  SPNumberEdit;
delete  SPResolution;           delete  SPResolutionEdit;
delete  SPFromBrain;            delete  SPFromSP;
delete  InfixFilename;          delete  OpenAuto;
}


//----------------------------------------------------------------------------
void    TPreprocessMrisDialog::CmResizingEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( Resizing->GetCheck() ) );
}


void    TPreprocessMrisDialog::CmVoxelEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( Resizing->GetCheck() ) && CheckToBool ( ResizingVoxel->GetCheck() ) );
}


void    TPreprocessMrisDialog::CmRatioEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( Resizing->GetCheck() ) && CheckToBool ( ResizingRatio->GetCheck() ) );
}


void    TPreprocessMrisDialog::CmReorientingEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( Reorienting->GetCheck() ) );
}


void    TPreprocessMrisDialog::CmReorientingOtherEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( Reorienting->GetCheck() ) && CheckToBool ( ReorientingOther->GetCheck() ) );
}


void    TPreprocessMrisDialog::CmAlignSagittalEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( Reorienting->GetCheck() ) && CheckToBool ( AlignSagittal->GetCheck() ) );
}


void    TPreprocessMrisDialog::CmOriginEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( SettingOrigin->GetCheck() ) );
}


void    TPreprocessMrisDialog::CmOriginOtherEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( SettingOrigin->GetCheck() ) && CheckToBool ( OriginFixed->GetCheck() ) );
}


void    TPreprocessMrisDialog::CmSizeUserEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( SizeUser->GetCheck() ) );
}


void    TPreprocessMrisDialog::CmSkullStrippingEnable ( TCommandEnabler &tce )
{
tce.Enable ( PresetsSkullStripping->GetSelIndex () != MriSkullStrippingPresetOff ); 
}


void    TPreprocessMrisDialog::CmSPPresetsEnable ( TCommandEnabler &tce )
{
tce.Enable ( PresetsSkullStripping->GetSelIndex () != MriSkullStrippingPresetOff
          && PresetsGrey          ->GetSelIndex () != GreyMaskPresetOff         ); 
}


void    TPreprocessMrisDialog::CmSPButtonEnable ( TCommandEnabler &tce )
{
tce.Enable ( PresetsSkullStripping->GetSelIndex () != MriSkullStrippingPresetOff
          && PresetsGrey          ->GetSelIndex () != GreyMaskPresetOff
          && PresetsSP            ->GetSelIndex () == SPPresetCompute           ); 
}


void    TPreprocessMrisDialog::CmSPComputeNumEnable ( TCommandEnabler &tce )
{
tce.Enable ( PresetsSkullStripping->GetSelIndex () != MriSkullStrippingPresetOff
          && PresetsGrey          ->GetSelIndex () != GreyMaskPresetOff
          && PresetsSP            ->GetSelIndex () == SPPresetCompute
          && CheckToBool ( SPNumber->GetCheck() )                               ); 
}


void    TPreprocessMrisDialog::CmSPComputeRezEnable ( TCommandEnabler &tce )
{
tce.Enable ( PresetsSkullStripping->GetSelIndex () != MriSkullStrippingPresetOff
          && PresetsGrey          ->GetSelIndex () != GreyMaskPresetOff
          && PresetsSP            ->GetSelIndex () == SPPresetCompute
          && CheckToBool ( SPResolution->GetCheck() )                           ); 
}


void    TPreprocessMrisDialog::CmSPPortEnable ( TCommandEnabler &tce )
{
tce.Enable ( PresetsSkullStripping->GetSelIndex () != MriSkullStrippingPresetOff
          && PresetsGrey          ->GetSelIndex () != GreyMaskPresetOff
          && PresetsSP            ->GetSelIndex () == SPPresetPorting
          && CheckToBool ( SPResolution->GetCheck() )                           ); 
}


//----------------------------------------------------------------------------
bool    TPreprocessMrisDialog::CmProcessEnable ()
{
TransferData ( tdGetData );


if ( CheckToBool ( MriPreprocTransfer.Resizing ) ) {

    double              voxel           = StringToDouble ( MriPreprocTransfer.ResizingVoxelValue );
    double              ratio           = StringToDouble ( MriPreprocTransfer.ResizingRatioValue );

    if ( CheckToBool ( MriPreprocTransfer.ResizingVoxel ) && voxel <= 0 )
        return  false;

    if ( CheckToBool ( MriPreprocTransfer.ResizingRatio ) && ratio <= 0 )
        return  false;
    }


if ( CheckToBool ( MriPreprocTransfer.Reorienting ) 
  && CheckToBool ( MriPreprocTransfer.ReorientingOther ) ) {

    char*       tox         = MriPreprocTransfer.ReorientingX;
    char*       toy         = MriPreprocTransfer.ReorientingY;
    char*       toz         = MriPreprocTransfer.ReorientingZ;

    if ( ! (    *tox && *toy && *toz 
             && *tox != *toy && *tox != *toz && *toy != *toz ) )
        return  false;

                                        // test this is a legal orientation
    if ( ! IsOrientationOK ( LetterToOrientation ( *tox ), LetterToOrientation ( *toy ), LetterToOrientation ( *toz ) ) )
        return  false;
    }


if ( CheckToBool ( MriPreprocTransfer.SettingOrigin ) 
  && CheckToBool ( MriPreprocTransfer.OriginFixed )
  && ( StringIsEmpty ( MriPreprocTransfer.OriginX ) 
    || StringIsEmpty ( MriPreprocTransfer.OriginY ) 
    || StringIsEmpty ( MriPreprocTransfer.OriginZ ) ) )

    return  false;


if ( MriPreprocTransfer.PresetsSkullStripping.GetSelIndex () != MriSkullStrippingPresetOff
  && MriPreprocTransfer.PresetsGrey          .GetSelIndex () != GreyMaskPresetOff
  && MriPreprocTransfer.PresetsSP            .GetSelIndex () == SPPresetPorting
  && ( StringIsEmpty ( MriPreprocTransfer.SPFromBrain ) 
    || StringIsEmpty ( MriPreprocTransfer.SPFromSP    ) ) )

    return  false;


return  true;
}


void    TPreprocessMrisDialog::CmProcessCurrentEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! BatchProcessing && CmProcessEnable () );
}


void    TPreprocessMrisDialog::CmProcessBatchEnable ( TCommandEnabler &tce )
{
tce.Enable (   BatchProcessing && CmProcessEnable () );
}


//----------------------------------------------------------------------------
                                        // Transverse Plane on or off will impact the origin
                                        // Also considering the reverse? origing -> transverse?
void    TPreprocessMrisDialog::CmAlignTransverse ()
{
                                        // transverse all OK ? -> force MNI Center
if      ( CheckToBool ( Reorienting    ->GetCheck() )
       && CheckToBool ( AlignSagittal  ->GetCheck() )
       && CheckToBool ( AlignTransverse->GetCheck() ) ) {

    SettingOrigin   ->SetCheck ( BoolToCheck ( true  ) );
    OriginMni       ->SetCheck ( BoolToCheck ( true  ) );
    OriginCenter    ->SetCheck ( BoolToCheck ( false ) );
    OriginFixed     ->SetCheck ( BoolToCheck ( false ) );
    }
                                        // transverse NOT OK - reset MNI Center if needed
else if ( /*CheckToBool ( SettingOrigin->GetCheck() ) 
      && */ CheckToBool ( OriginMni    ->GetCheck() ) ) {

    OriginMni       ->SetCheck ( BoolToCheck ( false ) );
    OriginCenter    ->SetCheck ( BoolToCheck ( true  ) );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Sagittal vs Optimal Size
if      (   CheckToBool ( Reorienting  ->GetCheck() )
       &&   CheckToBool ( AlignSagittal->GetCheck() ) 
       && ! CheckToBool ( SizeOptimal  ->GetCheck() ) ) {

    SizeTransformed ->SetCheck ( BoolToCheck ( false ) );
    SizeOptimal     ->SetCheck ( BoolToCheck ( true  ) );
    SizeUser        ->SetCheck ( BoolToCheck ( false ) );
    }

else if ( ! CheckToBool ( Reorienting  ->GetCheck() )
       || ! CheckToBool ( AlignSagittal->GetCheck() ) 
         && CheckToBool ( SizeOptimal  ->GetCheck() ) ) {

    SizeTransformed ->SetCheck ( BoolToCheck ( true  ) );
    SizeOptimal     ->SetCheck ( BoolToCheck ( false ) );
    SizeUser        ->SetCheck ( BoolToCheck ( false ) );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TransferData ( tdGetData );
}


void    TPreprocessMrisDialog::CmOriginMni ()
{
                                        // MNI Origin ? -> force trasnverse plane
if ( CheckToBool ( SettingOrigin->GetCheck() ) 
  && CheckToBool ( OriginMni    ->GetCheck() ) ) {

    Reorienting     ->SetCheck ( BoolToCheck ( true  ) );
    AlignSagittal   ->SetCheck ( BoolToCheck ( true  ) );
    AlignTransverse ->SetCheck ( BoolToCheck ( true  ) );
    }

TransferData ( tdGetData );
}


//----------------------------------------------------------------------------
void    TPreprocessMrisDialog::EvPresetsChange ()
{
PreprocessMrisPresetsEnum   mripreset       = (PreprocessMrisPresetsEnum) Presets->GetSelIndex ();


if ( StringIsEmpty ( PreprocessMrisPresetsNames[ mripreset ] ) )
    return;


FullHeadCleanUp ->SetCheck ( BoolToCheck ( false ) );
Isotropic       ->SetCheck ( BoolToCheck ( true  ) );   // by safety, this is alywas on
Resizing        ->SetCheck ( BoolToCheck ( false ) );
Reorienting     ->SetCheck ( BoolToCheck ( false ) );
SettingOrigin   ->SetCheck ( BoolToCheck ( false ) );

//SizeTransformed ->SetCheck ( BoolToCheck ( false ) );
//SizeOptimal     ->SetCheck ( BoolToCheck ( false ) );
//SizeUser        ->SetCheck ( BoolToCheck ( false ) );

PresetsSkullStripping   ->SetSelIndex ( MriSkullStrippingPresetOff );
BFC             ->SetCheck ( BoolToCheck ( false ) );
PresetsGrey     ->SetSelIndex ( GreyMaskPresetOff );
PresetsSP       ->SetSelIndex ( SPPresetOff );


switch ( mripreset ) {

    case MriPreprocPresetOff:
        break;

    case MriPreprocPresetCleaning:
        FullHeadCleanUp         ->SetCheck ( BoolToCheck ( true  ) );
        break;

    case MriPreprocPresetResampling:
        Resizing                ->SetCheck ( BoolToCheck ( true  ) );
        break;

    case MriPreprocPresetReorienting:
        Reorienting             ->SetCheck ( BoolToCheck ( true  ) );
        break;

    case MriPreprocPresetReorigin:
        SettingOrigin           ->SetCheck ( BoolToCheck ( true  ) );
        OriginMni               ->SetCheck ( BoolToCheck ( false ) );           // reorientation is off, we can't find the MNI Center
        OriginCenter            ->SetCheck ( BoolToCheck ( true  ) );
        OriginFixed             ->SetCheck ( BoolToCheck ( false ) );
        break;

    case MriPreprocPresetSkullStripping:
        PresetsSkullStripping   ->SetSelIndex ( MriSkullStrippingPresetDefault );
        BFC                     ->SetCheck ( BoolToCheck ( true  ) );
        PresetsGrey             ->SetSelIndex ( GreyMaskPresetOff );
        PresetsSP               ->SetSelIndex ( SPPresetOff );
        break;

    case MriPreprocPresetSkullStrippingGreySP:
        PresetsSkullStripping   ->SetSelIndex ( MriSkullStrippingPresetDefault );
        BFC                     ->SetCheck ( BoolToCheck ( true  ) );
        PresetsGrey             ->SetSelIndex ( GreyMaskPresetDefault );
        PresetsSP               ->SetSelIndex ( SPPresetCompute );
        break;

    case MriPreprocPresetAll:
        FullHeadCleanUp         ->SetCheck ( BoolToCheck ( true  ) );

        Resizing                ->SetCheck ( BoolToCheck ( true  ) );
        ResizingVoxel           ->SetCheck ( BoolToCheck ( true  ) );
        ResizingRatio           ->SetCheck ( BoolToCheck ( false ) );
        ResizingVoxelValue      ->SetText  ( "1" );

        Reorienting             ->SetCheck ( BoolToCheck ( true  ) );
        AlignSagittal           ->SetCheck ( BoolToCheck ( true  ) );           // for MNI Center
        AlignTransverse         ->SetCheck ( BoolToCheck ( true  ) );

        SettingOrigin           ->SetCheck ( BoolToCheck ( true  ) );
        SettingOriginIfNotSet   ->SetCheck ( BoolToCheck ( false ) );
        SettingOriginAlways     ->SetCheck ( BoolToCheck ( true  ) );
        OriginMni               ->SetCheck ( BoolToCheck ( true  ) );
        OriginCenter            ->SetCheck ( BoolToCheck ( false ) );
        OriginFixed             ->SetCheck ( BoolToCheck ( false ) );
        break;

    case MriPreprocPresetInverse:
        FullHeadCleanUp         ->SetCheck ( BoolToCheck ( true  ) );

        Resizing                ->SetCheck ( BoolToCheck ( true  ) );
        ResizingVoxel           ->SetCheck ( BoolToCheck ( true  ) );
        ResizingRatio           ->SetCheck ( BoolToCheck ( false ) );
        ResizingVoxelValue      ->SetText ( "1" );

        Reorienting             ->SetCheck ( BoolToCheck ( true  ) );
        ReorientingRas          ->SetCheck ( BoolToCheck ( true  ) );
        ReorientingOther        ->SetCheck ( BoolToCheck ( false ) );
        AlignSagittal           ->SetCheck ( BoolToCheck ( true  ) );           // for MNI Center
        AlignTransverse         ->SetCheck ( BoolToCheck ( true  ) );

        SettingOrigin           ->SetCheck ( BoolToCheck ( true  ) );
        SettingOriginIfNotSet   ->SetCheck ( BoolToCheck ( false ) );
        SettingOriginAlways     ->SetCheck ( BoolToCheck ( true  ) );
        OriginMni               ->SetCheck ( BoolToCheck ( true  ) );
        OriginCenter            ->SetCheck ( BoolToCheck ( false ) );
        OriginFixed             ->SetCheck ( BoolToCheck ( false ) );

        PresetsSkullStripping   ->SetSelIndex ( MriSkullStrippingPresetDefault );
        BFC                     ->SetCheck    ( BoolToCheck ( true  ) );
        PresetsGrey             ->SetSelIndex ( GreyMaskPresetDefault );
        PresetsSP               ->SetSelIndex ( SPPresetPorting );
        break;

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We can infer some resizing from the current options
if      ( CheckToBool ( Reorienting     ->GetCheck () ) 
       && CheckToBool ( AlignSagittal   ->GetCheck () ) ) {

    SizeTransformed ->SetCheck ( BoolToCheck ( false ) );
    SizeOptimal     ->SetCheck ( BoolToCheck ( true  ) );
    SizeUser        ->SetCheck ( BoolToCheck ( false ) );
    }
else if ( CheckToBool ( SizeOptimal     ->GetCheck () ) ) { // && ! Sagittal
    SizeTransformed ->SetCheck ( BoolToCheck ( true  ) );
    SizeOptimal     ->SetCheck ( BoolToCheck ( false ) );
    SizeUser        ->SetCheck ( BoolToCheck ( false ) );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // update transfer buffer
TransferData ( tdGetData );
}


//----------------------------------------------------------------------------
void    TPreprocessMrisDialog::EvPresetsMriTypeChange ()
{
MRISequenceType     mrisequence     = (MRISequenceType) PresetsMriType->GetSelIndex ();


if ( StringIsEmpty ( MRISequenceNames[ mrisequence ] ) )
    return;


PresetsSkullStripping->SetSelIndex ( MriSkullStrippingPresetOff );


switch ( mrisequence ) {

    case MRISequenceUnknown:
    case MRISequenceT1:
        PresetsSkullStripping->SetSelIndex ( MriSkullStrippingPreset2 );
        break;

    case MRISequenceT1Gad:
//      PresetsSkullStripping->SetSelIndex ( MriSkullStrippingPreset1 );
        PresetsSkullStripping->SetSelIndex ( MriSkullStrippingPreset2 );
        break;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // update transfer buffer
TransferData ( tdGetData );
}


//----------------------------------------------------------------------------
void    TPreprocessMrisDialog::EvDropFiles ( TDropInfo drop )
{
TPointInt           where;
TGoF                mrifiles        ( drop, AllMriFilesExt, &where );
TGoF                spfiles         ( drop, AllSolPointsFilesExt    );
TGoF                remainingfiles  ( drop, AllSolPointsFilesExt " " AllMriFilesExt, 0, true );

drop.DragFinish ();

//where.Show ( "Dropped @" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) remainingfiles )
    remainingfiles.Show ( IrrelevantErrorMessage );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // trying to resolve brain & sp parameters first
if ( (int) mrifiles == 1 && (int) spfiles == 1 ) {
    
    SetBrainFile    ( mrifiles[ 0 ] );
    SetSpFile       ( spfiles [ 0 ] );

    PresetsSP->SetSelIndex ( SPPresetPorting );

    return;
    }

if ( (int) mrifiles == 1 && (int) spfiles == 0
   && IsInsideLimits ( where.Y, 700, 930 )     ) {
    
    SetBrainFile    ( mrifiles[ 0 ] );

    PresetsSP->SetSelIndex ( SPPresetPorting );

    return;
    }

if ( (int) mrifiles == 0 && (int) spfiles == 1
   && IsInsideLimits ( where.Y, 700, 930 )     ) {
    
    SetSpFile       ( spfiles [ 0 ] );

    PresetsSP->SetSelIndex ( SPPresetPorting );

    return;
    }

                                        // at that point, it should be MRIs to process here...
if ( (bool) mrifiles && ! BatchProcessing )
    ShowMessage ( BatchNotAvailMessage, NormalizeTitleMany, ShowMessageWarning );


if ( (bool) mrifiles &&   BatchProcessing ) {

    if ( CmProcessEnable () )
        BatchProcessDropped ( mrifiles );
    else
        ShowMessage ( BatchErrorMessage, NormalizeTitleMany, ShowMessageWarning );
    }
}


//----------------------------------------------------------------------------
void    TPreprocessMrisDialog::CmBrowseSpFile ()
{
SetSpFile ( 0 );
}


void    TPreprocessMrisDialog::SetSpFile ( char *file )
{
static GetFileFromUser  getfile ( "Solution Points File:", AllSolPointsFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( MriPreprocTransfer.SPFromSP ) )
        return;

    TransferData ( tdSetData );
    }
else {
    StringCopy ( MriPreprocTransfer.SPFromSP, file );

    getfile.SetOnly ( file );
    }


TransferData ( tdSetData );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SPFromBrain->ResetCaret;
SPFromSP   ->ResetCaret;
}


//----------------------------------------------------------------------------
void    TPreprocessMrisDialog::CmBrowseBrainFile ()
{
SetBrainFile ( 0 );
}


void    TPreprocessMrisDialog::SetBrainFile ( char *file )
{
static GetFileFromUser  getfile ( "Brain MRI File:", AllMriFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( MriPreprocTransfer.SPFromBrain ) )
        return;

    TransferData ( tdSetData );
    }
else {
    StringCopy ( MriPreprocTransfer.SPFromBrain, file );

    getfile.SetOnly ( file );
    }


TransferData ( tdSetData );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SPFromBrain->ResetCaret;
SPFromSP   ->ResetCaret;
}


//----------------------------------------------------------------------------
void    TPreprocessMrisDialog::CmProcessCurrent ()
{
if ( MRIDoc == 0 )
    return;


TBaseDialog::CmProcessCurrent ( MRIDoc->GetDocPath () );
}


//----------------------------------------------------------------------------
                                        // Here, we loop through MRIs
void    TPreprocessMrisDialog::CmBatchProcess ()
{
if ( ! BatchProcessing ) {
    ShowMessage ( BatchNotAvailMessage, NormalizeTitleMany, ShowMessageWarning );
    return;
    }


TBaseDialog::CmBatchProcess ( AllMriFilesFilter );
}


//----------------------------------------------------------------------------
                                        // Overloading to process MRIs instead of EEGS (the default)
void    TPreprocessMrisDialog::BatchProcess ()
{
Destroy ();

                                        // no file(s) in list?
if ( ! HasBatchFiles () )
    return;

                                        // processing current, but doc pointer is null?
if ( ! BatchProcessing )
    if ( MRIDoc == 0 )
        return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;
int                 numinfiles      = BatchFileNames.NumFiles ();


if ( numinfiles > 1 ) {

    Gauge.Set           ( "File", SuperGaugeLevelBatch, SuperGaugeCount );
    Gauge.AddPart       ( 0, numinfiles, 100 );
    Gauge.CurrentPart   = 0;
    }

                                        // loop through the eeg files
for ( BatchFileIndex = 0; BatchFileIndex < numinfiles ; BatchFileIndex++ ) {

    Gauge.Next ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if ( BatchProcessing ) {            // open the doc myself

        MRIDoc      = dynamic_cast<TVolumeDoc*> ( CartoolDocManager->OpenDoc ( BatchFileNames[ BatchFileIndex ], dtOpenOptionsNoView ) );

        if ( ! MRIDoc )                 // oops, not gonna like it
            continue;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    BatchProcessCurrent ();             // calls ProcessCurrent


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // then close eeg
    if ( BatchProcessing && MRIDoc )

        if ( MRIDoc->CanClose ( true ) )
            CartoolDocManager->CloseDoc ( MRIDoc );

    }


Gauge.Finished ();

                                        // reset reference to current doc
MRIDoc  = 0;
}


//----------------------------------------------------------------------------
void    TPreprocessMrisDialog::ProcessCurrent ( void* usetransfer, const char* /*moreinfix*/ )
{
if ( ! MRIDoc )
    return;


TPreprocessMrisStruct*  transfer    = usetransfer ? (TPreprocessMrisStruct *) usetransfer : &MriPreprocTransfer;


if ( ! transfer )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

MRISequenceType     mrisequence     = (MRISequenceType) transfer->PresetsMriType.GetSelIndex ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                cleanup         = CheckToBool ( transfer->FullHeadCleanUp );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                isotropic       = CheckToBool ( transfer->Isotropic );


ResizingType        resizing        = ! CheckToBool ( transfer->Resizing      ) ? ResizingNone
//                                  :   CheckToBool ( transfer->ResizingDim   ) ? ResizingDimensions
                                    :   CheckToBool ( transfer->ResizingVoxel ) ? ResizingVoxels
                                    :   CheckToBool ( transfer->ResizingRatio ) ? ResizingRatios
                                    :                                             ResizingNone;

double              resizingvoxel   = StringToDouble ( transfer->ResizingVoxelValue );
double              resizingratio   = StringToDouble ( transfer->ResizingRatioValue );
double              resizingvalue   = resizing == ResizingVoxels    ?   resizingvoxel
                                    : resizing == ResizingRatios    ?   resizingratio
                                    :                                   0;

bool                anyresizing     = true;                 // force resizing


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

BoundingSizeType    targetsize      = CheckToBool ( transfer->SizeOptimal ) ? BoundingSizeOptimal   // cut around final data, with some margin, as to save on the final size
                                                                            : BoundingSizeGiven;

TPointInt           sizeuser;

if      ( CheckToBool ( transfer->SizeUser ) )
                                        // non-null value overrides all sizes
    sizeuser.Set ( StringToDouble ( transfer->SizeUserX ), 
                   StringToDouble ( transfer->SizeUserY ), 
                   StringToDouble ( transfer->SizeUserZ ) );

else // if ( CheckToBool ( transfer->SizeTransformed ) )
                                        // to signify Transformed size - might as well use a new state like  BoundingSizeTransformed?
    sizeuser.Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ReorientingType     reorienting     = ! CheckToBool ( transfer->Reorienting      )  ? ReorientingNone
                                    :   CheckToBool ( transfer->ReorientingRas   )  ? ReorientingRAS
                                    :   CheckToBool ( transfer->ReorientingOther )  ? ReorientingArbitrary
                                    :                                                 ReorientingNone;

char                reorientingstring [ 16 ];

StringCopy          ( reorientingstring, transfer->ReorientingX, transfer->ReorientingY, transfer->ReorientingZ );

StringToUppercase   ( reorientingstring );

                                        // test this is a legal orientation - should have been done in the UI, though
if ( reorienting == ReorientingArbitrary && ! IsOrientationOK ( reorientingstring ) ) {

//  ShowMessage ( "Not a correct re-orientation!", NormalizeTitleMany, ShowMessageWarning );

    reorienting     = ReorientingNone;
    ClearString ( reorientingstring );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                sagittalplane   = reorienting != ReorientingNone && CheckToBool ( transfer->AlignSagittal   );
bool                transverseplane = reorienting != ReorientingNone && CheckToBool ( transfer->AlignTransverse ) && sagittalplane;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // !This parameter can have some side-effects on the Sagittal plane detection!
OriginFlags         origin          = ! CheckToBool ( transfer->SettingOrigin         ) ? OriginNone
                                    :   CheckToBool ( transfer->SettingOriginAlways   ) ? OriginSetAlways
                                    :   CheckToBool ( transfer->SettingOriginIfNotSet ) ? OriginSetNoDefault
                                    :                                                     OriginNone;

if ( IsOriginSetAlways    ( origin ) 
  || IsOriginSetNoDefault ( origin ) )
                    origin          = (OriginFlags) ( origin |
                                    (   CheckToBool ( transfer->OriginMni             ) ? OriginMniFlag
                                    :   CheckToBool ( transfer->OriginCenter          ) ? OriginCenterFlag
                                    :   CheckToBool ( transfer->OriginFixed           ) ? OriginArbitrary
                                    :                                                     0                )
                                                   );

TPointDouble        arbitraryorigin;

arbitraryorigin.X   = StringToDouble ( transfer->OriginX );
arbitraryorigin.Y   = StringToDouble ( transfer->OriginY );
arbitraryorigin.Z   = StringToDouble ( transfer->OriginZ );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

MriSkullStrippingPresetsEnum  skullstrippingpreset  = (MriSkullStrippingPresetsEnum) transfer->PresetsSkullStripping.GetSelIndex ();

bool                doskullstripping= skullstrippingpreset != MriSkullStrippingPresetOff;

SkullStrippingType  skullstripping  = skullstrippingpreset == MriSkullStrippingPresetOff    ?   SkullStrippingNone
                                    : skullstrippingpreset == MriSkullStrippingPreset1      ?   SkullStripping1B
                                    : skullstrippingpreset == MriSkullStrippingPreset2      ?   SkullStripping2
                                    : skullstrippingpreset == MriSkullStrippingPreset3      ?   SkullStripping3
                                    :                                                           SkullStrippingNone;

bool                bfc             = doskullstripping && CheckToBool ( transfer->BFC );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GreyMaskPresetsEnum greypreset          = (GreyMaskPresetsEnum) transfer->PresetsGrey.GetSelIndex ();

bool                computegrey         = doskullstripping && greypreset != GreyMaskPresetOff;

GreyMatterFlags     greyflagsproc       = greypreset == GreyMaskPresetOff   ?   GreyMatterNothing
                                        : greypreset == GreyMaskPresetThin  ?   GreyMatterSlim
                                        : greypreset == GreyMaskPresetReg   ?   GreyMatterRegular
                                        : greypreset == GreyMaskPresetFat   ?   GreyMatterFat
                                        : greypreset == GreyMaskPresetWhole ?   GreyMatterWhole
                                        :                                       GreyMatterNothing;

GreyMatterFlags     greyflagspostproc   = GreyMatterFlags ( GreyMatterPostprocessing | GreyMatterAsymmetric );

GreyMatterFlags     greyflags           = GreyMatterFlags ( greyflagsproc | greyflagspostproc );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SPPresetsEnum       sppreset            = (SPPresetsEnum) transfer->PresetsSP.GetSelIndex ();

bool                dosps               = computegrey && sppreset != SPPresetOff;

int                 numsps              = dosps && sppreset == SPPresetCompute && CheckToBool ( transfer->SPNumber     ) ? StringToInteger ( transfer->SPNumberEdit     ) : 0;
double              ressps              = dosps && sppreset == SPPresetCompute && CheckToBool ( transfer->SPResolution ) ? StringToDouble  ( transfer->SPResolutionEdit ) : 0;

TFileName           spfrombrainfile     = dosps && sppreset == SPPresetPorting && CanOpenFile ( transfer->SPFromBrain ) ? transfer->SPFromBrain : "";
TFileName           spfromspfile        = dosps && sppreset == SPPresetPorting && CanOpenFile ( transfer->SPFromSP    ) ? transfer->SPFromSP    : "";


GreyMatterFlags     spflagspostproc     = greyflagspostproc;    // get GreyMatterAsymmetric / GreyMatterSymmetric from grey

GreyMatterFlags     spflagscheck        = GreyMatterFlags ( GreyMatterLauraCheck | GreyMatterSinglePointCheck );

GreyMatterFlags     spflags             = GreyMatterFlags ( spflagspostproc | spflagscheck );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                openauto        = CheckToBool ( transfer->OpenAuto ) 
                                   && ( ! BatchProcessing || (int) BatchFileNames <= 5 );


char                infixfilename[ EditSizeText ];
StringCopy      ( infixfilename, transfer->InfixFilename );
StringCleanup   ( infixfilename );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                showstepgauge       = CartoolObjects.CartoolApplication->IsInteractive ();      // global step for current file
bool                showprocessgauge    = showstepgauge; // && ( ! BatchProcessing || (int) BatchFileNames == 1 );  // each sub-processing can also show-up

TSuperGauge         Gauge;

                                        // second level of 
if ( showstepgauge ) {
    Gauge.Set           ( NormalizeTitleOne, SuperGaugeLevelInter );

    Gauge.AddPart       ( 0, PreProcessMrisNumGauge, 100 );

    Gauge.CurrentPart   = 0;
    }


if ( IsBatchFirstCall () && (int) BatchFileNames > 5 )
                                        // batch can be long, hide Cartool until we are done
    WindowMinimize ( CartoolMainWindow );

                                        // Changing priority
SetProcessPriority ( BatchProcessingPriority );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TGoF                gofin;
TGoF                gofout;

gofin.SetOnly ( (char*) MRIDoc->GetDocPath () );


CartoolApplication->SetMainTitle ( NormalizeTitleOne, gofin[ 0 ], Gauge );


PreprocessMris (    gofin,
                    mrisequence,
                    cleanup,
                    isotropic, 
                    resizing,           resizingvalue,
                    anyresizing,        targetsize,         sizeuser,
                    reorienting,        reorientingstring,
                    sagittalplane,      transverseplane,
                    origin,             arbitraryorigin,
                    skullstripping,     bfc,
                    computegrey,        greyflags,
                    sppreset,           spflags,
                    numsps,             ressps,
                    spfrombrainfile,    spfromspfile,
                    infixfilename,
                    gofout,
                    showstepgauge ? &Gauge : 0, showprocessgauge );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SetProcessPriority ();

                                        // complimentary opening the file for the user
if ( openauto )
    gofout.OpenFiles ();


if ( IsBatchLastCall () ) {

    WindowMaximize ( CartoolMainWindow );

    Gauge.FinishParts ();
    CartoolApplication->SetMainTitle ( NormalizeTitleMany, "", Gauge );

    Gauge.HappyEnd ();
    }

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OptimizeOn

}
