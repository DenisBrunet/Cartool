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
#include    "TBaseDialog.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr char*     NormalizeTitleOne       = "Preprocessing MRI";
constexpr char*     NormalizeTitleMany      = "Preprocessing MRIs";


enum        PreprocessMrisPresetsEnum
            {
            MriPreprocPresetOff,
            MriPreprocPresetSep1,
            MriPreprocPresetCleaning,
            MriPreprocPresetResampling,
            MriPreprocPresetReorienting,
            MriPreprocPresetReorigin,
            MriPreprocPresetSkullStripping,
            MriPreprocPresetSkullStrippingGreySP,
            MriPreprocPresetSep2,
            MriPreprocPresetAll,
            MriPreprocPresetInverse,

            NumMriPreprocPresets,

            MriPreprocPresetDefault     = MriPreprocPresetInverse,
            };

extern const char   PreprocessMrisPresetsNames[ NumMriPreprocPresets ][ 64 ];


enum        MRISequenceType
            {
            MRISequenceUnknown,
            MRISequenceT1,
            MRISequenceT1Gad,
//          MRISequenceT2,              // not yet

            NumMRISequenceTypes,

            MRISequenceDefault          = MRISequenceT1,
            };

extern const char   MRISequenceNames[ NumMRISequenceTypes ][ 16 ];



enum        ResizingType
            {
            ResizingNone,
            ResizingDimensions,
            ResizingVoxels,
            ResizingRatios,
            };


enum        ReorientingType
            {
            ReorientingNone,
            ReorientingRAS,
            ReorientingArbitrary,
            };


enum        OriginFlags
            {
            OriginNone          = 0x00,

            OriginReset         = 0x01,
            OriginSetAlways     = 0x02,
            OriginSetNoDefault  = 0x04,
            OriginHowMask       = 0x07,

            OriginMniFlag       = 0x10,
            OriginCenterFlag    = 0x20,
            OriginArbitrary     = 0x40,
            OriginLocationMask  = 0x70,
            };

inline  bool    IsOrigin                ( OriginFlags o )       { return  o & OriginHowMask;        }
inline  bool    IsOriginReset           ( OriginFlags o )       { return  o & OriginReset;          }
inline  bool    IsOriginSetAlways       ( OriginFlags o )       { return  o & OriginSetAlways;      }
inline  bool    IsOriginSetNoDefault    ( OriginFlags o )       { return  o & OriginSetNoDefault;   }
                                                                         
inline  bool    IsOriginMni             ( OriginFlags o )       { return  o & OriginMniFlag;        }
inline  bool    IsOriginCenter          ( OriginFlags o )       { return  o & OriginCenterFlag;     }
inline  bool    IsOriginArbitrary       ( OriginFlags o )       { return  o & OriginArbitrary;      }


enum        MriSkullStrippingPresetsEnum
            {
            MriSkullStrippingPresetOff,
            MriSkullStrippingPreset1,
            MriSkullStrippingPreset2,
            MriSkullStrippingPreset3,

            NumMriSkullStrippingPresets,

            MriSkullStrippingPresetDefault  = MriSkullStrippingPreset1,
            };


extern const char   MriSkullStrippingPresetsNames[ NumMriSkullStrippingPresets ][ 64 ];


enum        GreyMaskPresetsEnum
            {
            GreyMaskPresetOff,
            GreyMaskPresetThin,
            GreyMaskPresetReg,
            GreyMaskPresetFat,
            GreyMaskPresetWhole,

            NumGreyMaskPresets,

            GreyMaskPresetDefault       = GreyMaskPresetFat,
            };


extern const char   GreyMaskPresetsNames[ NumGreyMaskPresets ][ 32 ];


enum        SPPresetsEnum
            {
            SPPresetOff,
            SPPresetCompute,
            SPPresetPorting,

            NumSPPresets,

            SPPresetDefault     = SPPresetPorting,
            };


extern const char   SPPresetsNames[ NumSPPresets ][ 64 ];


//----------------------------------------------------------------------------
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


class  TPreprocessMrisStruct
{
public:
                        TPreprocessMrisStruct ();

        
    TComboBoxData       Presets;
    TComboBoxData       PresetsMriType;

    TCheckBoxData       FullHeadCleanUp;

    TCheckBoxData       Isotropic;

    TCheckBoxData       Resizing;
    TRadioButtonData    ResizingVoxel;
    TRadioButtonData    ResizingRatio;
    TEditData           ResizingVoxelValue  [ EditSizeValue ];
    TEditData           ResizingRatioValue  [ EditSizeValue ];

    TCheckBoxData       Reorienting;
    TRadioButtonData    ReorientingRas;
    TRadioButtonData    ReorientingOther;
    TEditData           ReorientingX        [ 2 ];
    TEditData           ReorientingY        [ 2 ];
    TEditData           ReorientingZ        [ 2 ];
    TCheckBoxData       AlignSagittal;
    TCheckBoxData       AlignTransverse;

    TCheckBoxData       SettingOrigin;
    TRadioButtonData    SettingOriginIfNotSet;
    TRadioButtonData    SettingOriginAlways;
    TRadioButtonData    OriginMni;
    TRadioButtonData    OriginCenter;
    TRadioButtonData    OriginFixed;
    TEditData           OriginX             [ EditSizeValue ];
    TEditData           OriginY             [ EditSizeValue ];
    TEditData           OriginZ             [ EditSizeValue ];

    TRadioButtonData    SizeTransformed;
    TRadioButtonData    SizeOptimal;
    TRadioButtonData    SizeUser;
    TEditData           SizeUserX           [ EditSizeValue ];
    TEditData           SizeUserY           [ EditSizeValue ];
    TEditData           SizeUserZ           [ EditSizeValue ];

    TComboBoxData       PresetsSkullStripping;
    TCheckBoxData       BFC;

    TComboBoxData       PresetsGrey;

    TComboBoxData       PresetsSP;
    TRadioButtonData    SPNumber;
    TEditData           SPNumberEdit        [ EditSizeValue ];
    TRadioButtonData    SPResolution;
    TEditData           SPResolutionEdit    [ EditSizeValue ];
    TEditData           SPFromBrain         [ EditSizeText ];
    TEditData           SPFromSP            [ EditSizeText ];

    TEditData           InfixFilename       [ EditSizeText ];
    TCheckBoxData       OpenAuto;

};


EndBytePacking

//----------------------------------------------------------------------------

class       TVolumeDoc;


class   TPreprocessMrisDialog   :   public  TBaseDialog
{
public:                
                        TPreprocessMrisDialog ( owl::TWindow* parent, owl::TResId resId, TVolumeDoc* doc = 0 );
                       ~TPreprocessMrisDialog ();


protected:
    owl::TComboBox      *Presets;
    owl::TComboBox      *PresetsMriType;

    owl::TCheckBox      *FullHeadCleanUp;

    owl::TCheckBox      *Isotropic;

    owl::TCheckBox      *Resizing;
    owl::TRadioButton   *ResizingVoxel;
    owl::TRadioButton   *ResizingRatio;
    owl::TEdit          *ResizingVoxelValue;
    owl::TEdit          *ResizingRatioValue;

    owl::TCheckBox      *Reorienting;
    owl::TRadioButton   *ReorientingRas;
    owl::TRadioButton   *ReorientingOther;
    owl::TEdit          *ReorientingX;
    owl::TEdit          *ReorientingY;
    owl::TEdit          *ReorientingZ;
    owl::TCheckBox      *AlignSagittal;
    owl::TCheckBox      *AlignTransverse;

    owl::TCheckBox      *SettingOrigin;
    owl::TRadioButton   *SettingOriginIfNotSet;
    owl::TRadioButton   *SettingOriginAlways;
    owl::TRadioButton   *OriginMni;
    owl::TRadioButton   *OriginCenter;
    owl::TRadioButton   *OriginFixed;
    owl::TEdit          *OriginX;
    owl::TEdit          *OriginY;
    owl::TEdit          *OriginZ;

    owl::TRadioButton   *SizeTransformed;
    owl::TRadioButton   *SizeOptimal;
    owl::TRadioButton   *SizeUser;
    owl::TEdit          *SizeUserX;
    owl::TEdit          *SizeUserY;
    owl::TEdit          *SizeUserZ;

    owl::TComboBox      *PresetsSkullStripping;
    owl::TCheckBox      *BFC;

    owl::TComboBox      *PresetsGrey;

    owl::TComboBox      *PresetsSP;
    owl::TRadioButton   *SPNumber;
    owl::TEdit          *SPNumberEdit;
    owl::TRadioButton   *SPResolution;
    owl::TEdit          *SPResolutionEdit;
    owl::TEdit          *SPFromBrain;
    owl::TEdit          *SPFromSP;

    owl::TEdit          *InfixFilename;
    owl::TCheckBox      *OpenAuto;


    TVolumeDoc*         MRIDoc;             // current MRI to process


    void                CmBatchProcess          ()                                                      final;
    void                CmProcessCurrent        ()                                                      final;
    void                BatchProcess            ()                                                      final;
    void                ProcessCurrent          ( void *usetransfer = 0, const char *moreinfix = 0 )    final;


    void                EvDropFiles             ( owl::TDropInfo drop );
    void                EvPresetsChange         ();
    void                EvPresetsMriTypeChange  ();

    void                CmAlignTransverse       ();
    void                CmOriginMni             ();


    bool                CmProcessEnable         ();
    void                CmProcessCurrentEnable  ( owl::TCommandEnabler &tce );
    void                CmProcessBatchEnable    ( owl::TCommandEnabler &tce );

    void                CmResizingEnable        ( owl::TCommandEnabler &tce );
    void                CmVoxelEnable           ( owl::TCommandEnabler &tce );
    void                CmRatioEnable           ( owl::TCommandEnabler &tce );
    void                CmReorientingEnable     ( owl::TCommandEnabler &tce );
    void                CmAlignSagittalEnable   ( owl::TCommandEnabler &tce );
    void                CmReorientingOtherEnable( owl::TCommandEnabler &tce );
    void                CmOriginEnable          ( owl::TCommandEnabler &tce );
    void                CmOriginOtherEnable     ( owl::TCommandEnabler &tce );
    void                CmSizeUserEnable        ( owl::TCommandEnabler &tce );

    void                CmSkullStrippingEnable  ( owl::TCommandEnabler &tce );

    void                CmBrowseSpFile          ();
    void                SetSpFile               ( char *file );
    void                CmBrowseBrainFile       ();
    void                SetBrainFile            ( char *file );
    void                CmSPPresetsEnable       ( owl::TCommandEnabler &tce );
    void                CmSPButtonEnable        ( owl::TCommandEnabler &tce );
    void                CmSPComputeNumEnable    ( owl::TCommandEnabler &tce );
    void                CmSPComputeRezEnable    ( owl::TCommandEnabler &tce );
    void                CmSPPortEnable          ( owl::TCommandEnabler &tce );


    DECLARE_RESPONSE_TABLE ( TPreprocessMrisDialog );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
