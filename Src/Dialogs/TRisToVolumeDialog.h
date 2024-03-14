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

#include    "System.h"
#include    "Files.TOpenDoc.h"
#include    "TBaseDialog.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr char*     RisToVolumeTitle        = "RIS to Volume";

                                        // not bothering user, using sltealth mode:
//constexpr TOpenDocType    DefaultOpeningMode              = OpenDocHidden;
                                        // or showing a reminder that we are alive, and which files are actually in use:
constexpr TOpenDocType      DefaultOpeningMode              = OpenDocVisible;

constexpr AtomFormatType    RisToVolumeDefaultAtomFormat    = AtomFormatFloat;


                                        // Presets for conversion
enum        RisToVolumePreset
            {
            RisToVolumeNiftiFloat,
//          RisToVolumeNiftiByte,
            RisToVolumeAnalyzeFloat,
//          RisToVolumeAnalyzeByte,

            NumRisToVolumePreset,
            RisToVolumePresetDefault    = RisToVolumeNiftiFloat,
            };

extern const char   RisToVolumePresetString[ NumRisToVolumePreset ][ 16 ];

                                        // Presets for interpolation
enum        VolumeInterpolationPreset
            {
            VolumeInterpolation1NN,
            VolumeInterpolation4NN,
            VolumeInterpolationLinearRect,
//          VolumeInterpolationLinearSpherical,
//          VolumeInterpolationQuadraticFastSplineSpherical,
            VolumeInterpolationCubicFastSplineSpherical,

            NumVolumeInterpolationPreset,
            VolumeInterpolationPresetDefault    = VolumeInterpolationCubicFastSplineSpherical,
            };

extern const char   VolumeInterpolationPresetString[ NumVolumeInterpolationPreset ][ 128 ];

inline  bool    IsVoxelScan             ( VolumeInterpolationPreset pi )    {   return  pi == VolumeInterpolation1NN || pi == VolumeInterpolation4NN; } 
inline  bool    IsSolutionPointsScan    ( VolumeInterpolationPreset pi )    {   return  pi == VolumeInterpolationLinearRect /*|| pi == VolumeInterpolationLinearSpherical || pi == VolumeInterpolationQuadraticFastSplineSpherical */ || pi == VolumeInterpolationCubicFastSplineSpherical; } 

                                        // Presets for file type
enum        VolumeFileType
            {
            VolumeNifti2,
            VolumeAnalyze,

            NumVolumeFileTypes,
            VolumeTypeDefault       = VolumeNifti2,
            };

extern const char   VolumeFileTypeString[ NumVolumeFileTypes ][ 32 ];


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


class  TRisToVolumeStruct
{
public:
                        TRisToVolumeStruct ();

        
    TComboBoxData       Presets;

    TEditData           GreyMaskFile        [ EditSizeText ];
    TEditData           SpFile              [ EditSizeText ];
    TComboBoxData       InterpolationPresets;

    TRadioButtonData    TimeAll;
    TRadioButtonData    TimeInterval;
    TEditData           TimeMin             [ EditSizeValue ];
    TEditData           TimeMax             [ EditSizeValue ];
    TEditData           StepTF              [ EditSizeValue ];

    TEditData           BaseFileName        [ EditSizeText ];
    TRadioButtonData    TypeUnsignedByte;
    TRadioButtonData    TypeFloat;
    TComboBoxData       FileTypes;
    TCheckBoxData       OpenAuto;
};


class  TRisToVolumeStructEx:    public  TRisToVolumeStruct
{
public:
                    TRisToVolumeStructEx ();

                                        // Useful to check and save consistencies across runs
    TracksCompatibleClass       CompatRis;
    InverseCompatibleClass      CompatSp;

    bool            IsSpOK;
    bool            IsGreyMaskOK;
    bool            IsSpAndGreyMaskOK;
    bool            IsRisOK;
    bool            IsRisAndSpOK;

    void            CheckSp             ();
    void            CheckGreyMask       ();
    void            CheckSpAndGreyMask  ();
    void            CheckRisAndSp       ();
};


EndBytePacking

//----------------------------------------------------------------------------

class               TTracksDoc;
class               TSolutionPointsDoc;
class               TVolumeDoc;

class   TRisToVolumeDialog  :   public  TBaseDialog
{
public:
                    TRisToVolumeDialog ( owl::TWindow* parent, owl::TResId resId, TTracksDoc* doc );
                   ~TRisToVolumeDialog ();


protected:
    owl::TComboBox      *Presets;

    owl::TEdit          *GreyMaskFile;
    owl::TEdit          *SpFile;
    owl::TComboBox      *InterpolationPresets;

    owl::TRadioButton   *TimeAll;
    owl::TRadioButton   *TimeInterval;
    owl::TEdit          *TimeMin;
    owl::TEdit          *TimeMax;
    owl::TEdit          *StepTF;

    owl::TEdit          *BaseFileName;
    owl::TRadioButton   *TypeUnsignedByte;
    owl::TRadioButton   *TypeFloat;
    owl::TComboBox      *FileTypes;
    owl::TCheckBox      *OpenAuto;

                                        // optionally needed docs - global variables avoid closing/opening many times when batching a set of files
    TOpenDoc<TSolutionPointsDoc>    SPDoc;
    TOpenDoc<TVolumeDoc>            GreyMaskDoc;

                                        // Use to test files consistencies, with all the asynchronous D&D
    void            CheckRis                ( const TGoF& gofris )  const;
    void            CheckRisAndSp           (       TGoF& gofris )  const;


    void            ProcessCurrent          ( void *usetransfer = 0, const char *moreinfix = 0 );
    bool            CmProcessEnable         ();
    void            CmProcessCurrentEnable  ( owl::TCommandEnabler &tce );
    void            CmProcessBatchEnable    ( owl::TCommandEnabler &tce );

    void            EvPresetsChange         ();
    void            EvDropFiles             ( owl::TDropInfo drop );

    void            CmBrowseSpFile          ();
    void            SetSpFile               ( char *file );
    void            CmBrowseGreyMaskFile    ();
    void            SetGreyMaskFile         ( char *file );

    void            CmTimeIntervalEnable    ( owl::TCommandEnabler &tce );


    DECLARE_RESPONSE_TABLE ( TRisToVolumeDialog );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
