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

#include    "TInterpolateTracks.h"

#include    "TBaseDialog.h"
#include    "TCartoolApp.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Predefined electrodes systems:
enum        PredefLandmarksSystems
            {
            PredefClear,

            Predef1010A,
            Predef1010B,
            PredefEGI2,
            PredefEGI3,
            PredefEGIHydrocel1,
            PredefEGIHydrocel2,
            PredefNS,
            PredefBiosemi128,
            PredefBiosemi192,
            PredefBiosemi256,

            NumLandmarksSystems
            };


constexpr char*     LandmarksDescrAutoDetect    = "10-10 System detected";

                                        // Known "fiducial" electrode names for a few system
extern const char   PredefLandmarksStrings[ NumLandmarksSystems ][ NumLandmarksInfo ][ 32 ];


//----------------------------------------------------------------------------
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


class   TInterpolateTracksStruct
{
public:
                        TInterpolateTracksStruct ();


    TEditData           FromXyz     [ EditSizeText ];
    TEditData           FromBadElectrodes[ EditSizeTextLong ];
    TEditData           FromFront   [ EditSizeValue ];
    TEditData           FromLeft    [ EditSizeValue ];
    TEditData           FromTop     [ EditSizeValue ];
    TEditData           FromRight   [ EditSizeValue ];
    TEditData           FromRear    [ EditSizeValue ];

    TRadioButtonData    SameXyz;
    TRadioButtonData    AnotherXyz;
    TEditData           ToXyz       [ EditSizeText ];
    TEditData           ToFront     [ EditSizeValue ];
    TEditData           ToLeft      [ EditSizeValue ];
    TEditData           ToTop       [ EditSizeValue ];
    TEditData           ToRight     [ EditSizeValue ];
    TEditData           ToRear      [ EditSizeValue ];

    TRadioButtonData    SurfaceSpline;
    TRadioButtonData    SphericalSpline;
    TRadioButtonData    ThreeDSpline;
    TRadioButtonData    CDSpherical;
    TEditData           Degree      [ EditSizeValue ];

    TComboBoxData       FileTypes;
    TCheckBoxData       CleanUp;
    TCheckBoxData       OpenAuto;
    TEditData           InfixFilename[ EditSizeText ];
    };


class   TInterpolateTracksStructEx:   public  TInterpolateTracksStruct
{
public:
                        TInterpolateTracksStructEx ();


    TSelection          BadElectrodesSelection; // set from ProcessCurrent
    bool                FromXyzLinked;          // set from lm
    };


EndBytePacking

//----------------------------------------------------------------------------

class       TTracksDoc;
class       TGoF;


class   TInterpolateTracksDialog    :   public  TBaseDialog
{
public:
                        TInterpolateTracksDialog ( owl::TWindow* parent, owl::TResId resId, TTracksDoc* doc );
                       ~TInterpolateTracksDialog ();


protected:
    owl::TEdit          *FromXyz;
    owl::TEdit          *FromBadElectrodes;
    owl::TEdit          *FromFront;
    owl::TEdit          *FromLeft;
    owl::TEdit          *FromTop;
    owl::TEdit          *FromRight;
    owl::TEdit          *FromRear;
    owl::TStatic        *FromDescription;

    owl::TRadioButton   *SameXyz;
    owl::TRadioButton   *AnotherXyz;
    owl::TEdit          *ToXyz;
    owl::TEdit          *ToFront;
    owl::TEdit          *ToLeft;
    owl::TEdit          *ToTop;
    owl::TEdit          *ToRight;
    owl::TEdit          *ToRear;
    owl::TStatic        *ToDescription;

    owl::TRadioButton   *SurfaceSpline;
    owl::TRadioButton   *SphericalSpline;
    owl::TRadioButton   *ThreeDSpline;
    owl::TRadioButton   *CDSpherical;
    owl::TEdit          *Degree;

    owl::TComboBox      *FileTypes;
    owl::TCheckBox      *CleanUp;
    owl::TCheckBox      *OpenAuto;
    owl::TEdit          *InfixFilename;


    PredefLandmarksSystems  FromPredef;
    PredefLandmarksSystems  ToPredef;
    bool                    FromGrid;


    TInterpolateTracks  IT;                 // wrapping and handling all intermediate files and objects together


    void                BatchProcess            ()                                                      final;
    void                ProcessCurrent          ( void* usetransfer = 0, const char* moreinfix = 0 )    final;


    void                EvDropFiles             ( owl::TDropInfo drop );

    void                CmBrowseXyzFileName     ( owlwparam w );
    void                CmToXyzEnable           ( owl::TCommandEnabler &tce );
    void                CmToLandmarksEnable     ( owl::TCommandEnabler &tce );
    void                CmChoiceTo              ( owlwparam w );
    void                CmSetTargetSpace        ();
    void                CmFromXyzChange         ();
    void                CmToXyzChange           ();
    void                CmPredefLandmarks       ( owlwparam w );
    void                CmCopyLandmarks         ( owlwparam w );
    void                CmSplineEnable          ( owl::TCommandEnabler &tce );
    void                CmSplineDegreeEnable    ( owl::TCommandEnabler &tce );
    bool                AreParametersOk         ();
    void                CmProcessCurrentEnable  ( owl::TCommandEnabler &tce );
    void                CmProcessBatchEnable    ( owl::TCommandEnabler &tce );
    void                CmFromXyzEnable         ( owl::TCommandEnabler &tce );


    DECLARE_RESPONSE_TABLE ( TInterpolateTracksDialog );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
