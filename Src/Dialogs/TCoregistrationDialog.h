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
#include    "Time.TAcceleration.h"
#include    "Math.TMatrix44.h"
#include    "Geometry.TPoints.h"
#include    "TCoregistrationTransform.h"
#include    "TVolume.h"

#include    "TCartoolApp.h"
#include    "TBaseDialog.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Handy wrapper to dialog
class   TStrings;
class   TVolumeDoc;

bool    CoregisterXyzToMriInteractive   (   TPoints&        xyzpoints,  TStrings&       xyznames,
                                            TVolumeDoc*     mridoc,     Volume&         FullVolume, 
//                                          TMatrix44       mriabstoguillotine, 
                                            bool            init,
                                            TMatrix44&      XyzCoregToNorm 
                                        );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define         CoregistrationTitle     "Coregistration"

                                        // Electrodes could be a tiny bit too inside scalp after Gluing, because we use a very smoothed out mask - Compensate by this amount
constexpr double PostGluingInflate          = 0.5;


enum            CoregistrationType
                {
                CoregisterNone              = -1,   // NOT part of the visible options, just used to control the processing
                CoregisterXyzToMriScratch,
                CoregisterXyzToMriReload,

                NumCoregitrationType,
                CoregitrationTypeDefault    = CoregisterXyzToMriScratch,
                };

extern const char   CoregitrationTypeString[ NumCoregitrationType ][ 64 ];


//----------------------------------------------------------------------------
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


class  TCoregistrationStruct
{
public:
                        TCoregistrationStruct ();


    TComboBoxData       Presets;

    TEditData           InputSourceFile     [ EditSizeText ];
    TEditData           InputTargetFile     [ EditSizeText ];

    TCheckBoxData       Glue;

    TEditData           BaseFileName        [ EditSizeText ];
    TEditData           SavingAltElectrodes [ EditSizeText ];
};


EndBytePacking

//----------------------------------------------------------------------------

class   TBaseView;
class   TVolumeView;
class   TElectrodesView;


class   TCoregistrationDialog   :   public  TBaseDialog,
                                    public  TAcceleration
{
public:
                    TCoregistrationDialog ( owl::TWindow* parent, owl::TResId resId, bool usingopenview = true, TMatrix44* xyzcoregtonorm = 0 );
                   ~TCoregistrationDialog ();


protected:

    owl::TComboBox* Presets;

    owl::TEdit*     InputSourceFile;
    owl::TEdit*     InputTargetFile;

    owl::TCheckBox* Glue;

    owl::TEdit*     BaseFileName;
    owl::TEdit*     SavingAltElectrodes;


    TBaseView*      BaseView;           // main view to process from
    TVolumeView*    MriView;            // optional view to process from
    bool            CanCloseBaseView;
    bool            CanCloseMriView;

    TElectrodesView*XyzView;            // an alias to BaseView field


    CoregistrationType  Processing;     // type of processing
                                        // Original and copy of points
    TPoints         XyzOrig;
    TPoints         XyzCopy;
                                        // The whole transform is stored here
    TCoregistrationTransform    Transform;
    TCoregistrationTransform    TransformInit;
                                        // Needed for local operations
    TPointDouble    MriCenterOrig;
    TPointDouble    MriCenterGeom;
//  TMatrix44       NormToMri;
    TMatrix44       MriAbsToGuillotine;
    double          TranslateStep;
    Volume          VolumeGradient;
    Volume          VolumeMask;
                                        // Whole history of operations
    crtl::TList<TCoregistrationTransform>   UndoMatrices;
    crtl::TList<TCoregistrationTransform>   RedoMatrices;
                                        // Storing last keyboard partial operations
    char            Operation;
    char            Axis;
    char            Direction;

    TMatrix44*      XyzCoregToNorm;     // used to optionally return the transform to the caller


    void            SetupWindow             ();
    void            EvPresetsChange         ();
    bool            PreProcessMsg           ( MSG& msg );
    void            EvKeyDown               ( owl::uint key, owl::uint repeatCount, owl::uint flags );

    void            CmOk                    ();
    void            CmOkEnable              ( owl::TCommandEnabler &tce );
    void            CmCancel                ();

    void            CleanUp                 ( bool destroydialog );
//  void            CmSetProcessing         ();
    void            SetProcessing           ( bool computemasks, bool docleanup );

    void            CmBrowseInputSourceFile ();
    void            SetInputSourceFile      ( char *file );
    void            CmBrowseInputTargetFile ();
    void            SetInputTargetFile      ( char *file );
    void            EvDropFiles             ( owl::TDropInfo drop );
    void            CmUpOneDirectory        ();
    void            CmBrowseOutputFile      ();
    void            CmUndoEnable            ( owl::TCommandEnabler &tce );
    void            CmRedoEnable            ( owl::TCommandEnabler &tce );

    void            CmAction                ( owlwparam w );

    DECLARE_RESPONSE_TABLE ( TCoregistrationDialog );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
