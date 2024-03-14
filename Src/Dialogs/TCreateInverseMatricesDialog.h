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

#pragma once

#include    "TBaseDialog.h"

#include    "Files.TVerboseFile.h"
#include    "ESI.LeadFields.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr char*     CreatingInverseTitle            = "Creating Inverse Matrices";
constexpr char*     LeadFieldInterpolationTitle     = "Lead Field Interpolation";

                                        // Default target age for inverse
constexpr double    DefaultInverseAge               = 35;
                                        // optimal between 5000 to 6000
constexpr int       DefaultNumSolutionPoints        = 6000;
                                        // or default solution points resolution, in [mm]
constexpr double    DefaultResSolutionPoints        = 6.0;
                                        // for a MRI target size of 256
constexpr double    RescaleFilteredToNonFiltered    = 0.995;


//----------------------------------------------------------------------------

enum        CrisMriPresets
            {
            CrisMriPresetNothing,

            CrisMriPresetSeparator1,
            CrisMriPresetSubjectReorientSMAC,
            CrisMriPresetSubjectSMAC,

            CrisMriPresetSeparator2,
            CrisMriPresetTemplateReorientSMAC,
            CrisMriPresetTemplateSMAC,

            NumCrisMriPresets
            };

extern const char   CrisMriPresetsString[ NumCrisMriPresets ][ 64 ];


//----------------------------------------------------------------------------

enum        CrisXyzPresets
            {
            CrisXyzPresetNothing,

            CrisXyzPresetSeparator1,
            CrisXyzCoregisterInteractive,
            CrisXyzCoregisterInteractiveRedo,

            CrisXyzPresetSeparator2,
            CrisXyzPresetCoregistrationDone,
            NumCrisXyzPresets
            };

extern const char   CrisXyzPresetsString[ NumCrisXyzPresets ][ 128 ];


//----------------------------------------------------------------------------

enum        CrisSPPresets
            {
            CrisSPPresetNothing,

            CrisSPPresetSeparator1,
            CrisSPPresetComputeAdult,
            CrisSPPresetComputeChildren,
            CrisSPPresetComputeNewborn,

            CrisSPPresetSeparator2,
            CrisSPPresetLoad,
            CrisSPPresetFromLeadField,

            NumCrisSPPresets,

            CrisSPPresetComputeFirst    = CrisSPPresetComputeAdult,
            CrisSPPresetComputeLast     = CrisSPPresetComputeNewborn,
            };

extern const char   CrisSPPresetsString[ NumCrisSPPresets ][ 64 ];


//----------------------------------------------------------------------------
                                        // Lead Field dialog presets
enum        CrisLeadFieldPresets
            {
            CrisLFPresetNothing,

            CrisLFPresetSeparator1,
            CrisLFPresetLSMAC3ShellAry,

            CrisLFPresetSeparator2,
            CrisLFPresetLSMAC3ShellExact,
            CrisLFPresetLSMAC4ShellExact,
            CrisLFPresetLSMAC6ShellExact,

//          CrisLFPresetSeparator3,                 // These 2 presets are commented because they need to be FULLY investigated again, f.ex. head model, and using the Exact Spherical instead of Human Ary stuff.
//          CrisLFPresetLSMACMonkey,                // !not confirmed!
//          CrisLFPresetLSMACMouse,                 // !not confirmed!

            CrisLFPresetSeparator4,
            CrisLFPresetReload,

            NumCrisLFPPresets
            };

extern const OneLeadFieldPreset     AllLeadFieldPresets[ NumCrisLFPPresets ];


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


class  TCreateInverseMatricesStruct
{
public:
                        TCreateInverseMatricesStruct ();


    TComboBoxData       MriPresets;
    TEditData           HeadMriFile             [ EditSizeText ];
    TEditData           BrainMriFile            [ EditSizeText ];
    TEditData           GreyMriFile             [ EditSizeText ];
    TEditData           TissuesFile             [ EditSizeText ];
    TCheckBoxData       BiasField;
    TCheckBoxData       SmoothMri;
    TCheckBoxData       ReorientMris;
    TCheckBoxData       SagittalPlaneMri;
    TCheckBoxData       EquatorPlaneMri;

    TComboBoxData       XyzPresets;
    TEditData           XyzFile                 [ EditSizeText ];

    TComboBoxData       SpPresets;
    TEditData           NumSolPoints            [ EditSizeValue ];
    TRadioButtonData    GreyFat;
    TRadioButtonData    GreyAll;
    TEditData           LoadSolPointsFile       [ EditSizeText ];

    TComboBoxData       LeadFieldPresets;
    TRadioButtonData    ThicknessFromMri;
    TRadioButtonData    ThicknessFromTissues;
    TEditData           LeadFieldTargetAge      [ EditSizeValue ];
    TEditData           SkullConductivity       [ EditSizeValue ];
    TCheckBoxData       AdjustSkullMeanThickness;
    TEditData           SkullMeanThickness      [ EditSizeValue ];
    TEditData           LeadFieldFile           [ EditSizeText ];
    TEditData           LeadFieldSolPointsFile  [ EditSizeText ];

    TCheckBoxData       InverseMN;
    TCheckBoxData       InverseWMN;
    TCheckBoxData       InverseDale;
    TCheckBoxData       InverseSLoreta;
    TCheckBoxData       InverseLoreta;
    TCheckBoxData       InverseLaura;
    TCheckBoxData       InverseELoreta;

    TEditData           BaseFileName            [ EditSizeText ];
    TRadioButtonData    Interactive;
    TRadioButtonData    Automatic;


    bool                HasHeadMri                  ()  { return    StringIsNotEmpty ( HeadMriFile  ); }
    bool                HasBrainMri                 ()  { return    StringIsNotEmpty ( BrainMriFile ); }
    bool                HasGreyMri                  ()  { return    StringIsNotEmpty ( GreyMriFile  ); }
    bool                HasTissuesFile              ()  { return    StringIsNotEmpty ( TissuesFile ); }
    bool                HasAnyMri                   ()  { return    HasHeadMri () || HasBrainMri () || HasGreyMri ();   }
    bool                HasHeadBrainMris            ()  { return    HasHeadMri () && HasBrainMri ();   }
    bool                HasXyz                      ()  { return    StringIsNotEmpty ( XyzFile ); }
    bool                HasLeadFieldFile            ()  { return    StringIsNotEmpty ( LeadFieldFile ); }
    bool                HasLeadFieldSolPointsFile   ()  { return    StringIsNotEmpty ( LeadFieldSolPointsFile ); }
    bool                HasLoadSolPointsFile        ()  { return    StringIsNotEmpty ( LoadSolPointsFile ); }

    bool                IsBiasField                 ()  { return    CheckToBool ( BiasField              ); }
    bool                IsSmoothMri                 ()  { return    CheckToBool ( SmoothMri              ); }
    bool                IsReorientMris              ()  { return    CheckToBool ( ReorientMris           ); }
    bool                IsMriProcessing             ()  { return     ( IsBiasField () || IsSmoothMri () || IsReorientMris () )
                                                                   || IsSPsProcessing () || IsComputeLeadField () || IsXyzCoregisterToMri (); }

    bool                IsXyzNoTransform            ()  { return    XyzPresets.GetSelIndex() == CrisXyzPresetNothing || StringIsEmpty ( CrisXyzPresetsString[ XyzPresets.GetSelIndex() ] ); }
    bool                IsXyzCoregistrationDone     ()  { return    /* ! IsXyzNoTransform () && */ XyzPresets.GetSelIndex() == CrisXyzPresetCoregistrationDone;     }
    bool                IsXyzCoregisterToMriI       ()  { return    /* ! IsXyzNoTransform () && */ XyzPresets.GetSelIndex() == CrisXyzCoregisterInteractive;        }
    bool                IsXyzCoregisterToMriIRedo   ()  { return    /* ! IsXyzNoTransform () && */ XyzPresets.GetSelIndex() == CrisXyzCoregisterInteractiveRedo;    }
    bool                IsXyzCoregisterToMri        ()  { return    IsXyzCoregisterToMriI () || IsXyzCoregisterToMriIRedo (); }
    bool                IsXyzProcessing             ()  { return    IsXyzCoregistrationDone () || IsXyzCoregisterToMri (); }

    bool                IsComputeSPs                ()  { return    IsInsideLimits ( SpPresets.GetSelIndex(), (int) CrisSPPresetComputeFirst, (int) CrisSPPresetComputeLast ); }
    bool                IsLoadSPs                   ()  { return    SpPresets.GetSelIndex() == CrisSPPresetLoad; }
    bool                IsSameSPasLF                ()  { return    SpPresets.GetSelIndex() == CrisSPPresetFromLeadField; }
    bool                IsSPsProcessing             ()  { return    IsComputeSPs () || IsLoadSPs () || IsSameSPasLF (); }

    bool                IsComputeLeadField          ()  { return    AllLeadFieldPresets[ LeadFieldPresets.GetSelIndex() ].Flags != NoLeadField; }
    bool                IsThicknessFromMri          ()  { return    CheckToBool ( ThicknessFromMri          ); }
    bool                IsThicknessFromTissues      ()  { return    CheckToBool ( ThicknessFromTissues      ); }
    bool                IsAdjustSkullMeanThickness  ()  { return    CheckToBool ( AdjustSkullMeanThickness  ); }
    bool                IsLoadLeadField             ()  { return    LeadFieldPresets.GetSelIndex() == CrisLFPresetReload; }
    bool                IsLeadFieldProcessing       ()  { return    IsComputeLeadField () || IsLoadLeadField (); }

    bool                IsInverseMN                 ()  { return    CheckToBool ( InverseMN        ) && IsLeadFieldProcessing (); } // currently synced on WMN
    bool                IsInverseWMN                ()  { return    CheckToBool ( InverseWMN       ) && IsLeadFieldProcessing (); }
    bool                IsInverseDale               ()  { return    CheckToBool ( InverseDale      ) && IsLeadFieldProcessing (); }
    bool                IsInverseSLoreta            ()  { return    CheckToBool ( InverseSLoreta   ) && IsLeadFieldProcessing (); }
    bool                IsInverseLoreta             ()  { return    CheckToBool ( InverseLoreta    ) && IsLeadFieldProcessing (); }
    bool                IsInverseLaura              ()  { return    CheckToBool ( InverseLaura     ) && IsLeadFieldProcessing (); }
    bool                IsInverseELoreta            ()  { return    CheckToBool ( InverseELoreta   ) && IsLeadFieldProcessing (); }
    bool                IsInverseProcessing         ()  { return    IsLeadFieldProcessing () && ( IsInverseLoreta () || IsInverseLaura () || IsInverseMN () || IsInverseWMN () || IsInverseDale () || IsInverseSLoreta () || IsInverseELoreta () ); }
    bool                IsRegularization            ()  { return    IsInverseProcessing (); }
};


EndBytePacking

//----------------------------------------------------------------------------

class   TCreateInverseMatricesDialog    :   public  TBaseDialog
{
public:
                        TCreateInverseMatricesDialog ( owl::TWindow* parent, owl::TResId resId );
                       ~TCreateInverseMatricesDialog ();

protected:

    owl::TComboBox      *MriPresets;
    owl::TEdit          *HeadMriFile;
    owl::TEdit          *BrainMriFile;
    owl::TEdit          *GreyMriFile;
    owl::TEdit          *TissuesFile;
    owl::TCheckBox      *BiasField;
    owl::TCheckBox      *SmoothMri;
    owl::TCheckBox      *ReorientMris;
    owl::TCheckBox      *SagittalPlaneMri;
    owl::TCheckBox      *EquatorPlaneMri;

    owl::TComboBox      *XyzPresets;
    owl::TEdit          *XyzFile;

    owl::TComboBox      *SpPresets;
    owl::TEdit          *NumSolPoints;
    owl::TRadioButton   *GreyFat;
    owl::TRadioButton   *GreyAll;
    owl::TEdit          *LoadSolPointsFile;

    owl::TComboBox      *LeadFieldPresets;
    owl::TRadioButton   *ThicknessFromMri;
    owl::TRadioButton   *ThicknessFromTissues;
    owl::TEdit          *LeadFieldTargetAge;
    owl::TEdit          *SkullConductivity;
    owl::TCheckBox      *AdjustSkullMeanThickness;
    owl::TEdit          *SkullMeanThickness;
    owl::TEdit          *LeadFieldFile;
    owl::TEdit          *LeadFieldSolPointsFile;

    owl::TCheckBox      *InverseMN;
    owl::TCheckBox      *InverseWMN;
    owl::TCheckBox      *InverseDale;
    owl::TCheckBox      *InverseSLoreta;
    owl::TCheckBox      *InverseLoreta;
    owl::TCheckBox      *InverseLaura;
    owl::TCheckBox      *InverseELoreta;

    owl::TEdit          *BaseFileName;
    owl::TRadioButton   *Interactive;
    owl::TRadioButton   *Automatic;


    TVerboseFile        Verbose;
    bool                LeadFieldAndSolutionPointsMatch;    // used to store time-consuming checking
    bool                LeadFieldAndXyzMatch;


    void                CmOk                        ();
    void                CmOkEnable                  ( owl::TCommandEnabler& tce );

    void                EvDropFiles                 ( owl::TDropInfo drop );

    void                CmUpOneDirectory            ();
    void                ResetCarets                 ();
    void                CmBrowseBaseFileName        ();
    void                SetBaseFilename             ();

    void                EvMriPresetsChange          ();
    void                CmBrowseMriFile             ( owlwparam w );
    void                SetMriFile                  ( const char* file, int mriindex );
    void                CmMrisEnable                ( owl::TCommandEnabler &tce );
    void                CmNormalizeEnable           ( owl::TCommandEnabler &tce );
    void                CmBrainMriEnable            ( owl::TCommandEnabler &tce );

    void                CmBrowseXyzFile             ();
    void                SetXyzFile                  ( const char* file );
    void                CheckXyzFile                ();
    void                CmXyzEnable                 ( owl::TCommandEnabler &tce );

    void                EvSpPresetsChange           ();
    void                CmBrowseSolPointsFile       ( owlwparam w );
    void                SetLoadSolPointsFile        ( const char* file );
    void                CmNumSPEnable               ( owl::TCommandEnabler &tce );
    void                CmSPComputeEnable           ( owl::TCommandEnabler &tce );
    void                CmSPLoadEnable              ( owl::TCommandEnabler &tce );

    void                EvLeadFieldPresetsChange    ();
    void                CmBrowseTissuesFile         ();
    void                SetTissuesFile              ( const char* file );
    void                EvAgeChange                 ();
    void                CmBrowseLeadFieldFile       ();
    void                SetLeadFieldFile            ( const char* file );
    void                SetLeadFieldSolPointsFile   ( const char* file );
    void                CheckLeadFieldSolPointsFile ();
    void                CmComputeLeadFieldEnable    ( owl::TCommandEnabler &tce );
    void                CmAdjustSkullThicknessEnable( owl::TCommandEnabler &tce );
    void                CmLoadLeadFieldEnable       ( owl::TCommandEnabler &tce );

    void                CmInverseAllNone            ();
    void                CmInverseMatricesEnable     ( owl::TCommandEnabler &tce );

    DECLARE_RESPONSE_TABLE ( TCreateInverseMatricesDialog );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
