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

#include    "TCartoolApp.h"
#include    "TBaseDialog.h"
#include    "Time.TTimer.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


class  TCreateRoisStruct
{
public:
                        TCreateRoisStruct ();

    TEditData           InputFile           [ EditSizeText ];
    TCheckBoxData       UseRoisMriFile;
    TEditData           RoisMriFile         [ EditSizeText ];
    TEditData           RoisLabelsFile      [ EditSizeText ];

    TRadioButtonData    Automatic;
    TRadioButtonData    Interactive;

    TRadioButtonData    AutoTalairach;
    TRadioButtonData    AutoAAL;
    TRadioButtonData    AutoOtherRois;
    TEditData           GenerateRoisFile    [ EditSizeText ];

    TEditData           RoiName             [ EditSizeText ];
    TEditData           Selection           [ EditSizeTextLong ];

    TStaticData         MessageInteractive  [ EditSizeText ];

    TEditData           NumRois             [ EditSizeValue ];
    TListBoxData        RoisSummary;

    TEditData           BaseFileName        [ EditSizeText ];
    TCheckBoxData       OpenAuto;


                                        // convenience functions
    bool                IsRoisFile          ()  const   { return CheckToBool ( UseRoisMriFile ); }
    bool                IsRoisMriFile       ()  const   { return IsRoisFile () && StringIsNotEmpty ( RoisMriFile    ) && IsExtensionAmong ( RoisMriFile, AllMriFilesExt ); }
    bool                IsRoisLabelsFile    ()  const   { return IsRoisFile () && StringIsNotEmpty ( RoisLabelsFile ); }

    bool                IsInteractive       ()  const   { return CheckToBool ( Interactive ); }
    bool                IsAutomatic         ()  const   { return CheckToBool ( Automatic   ); }

    bool                IsAutoTalairach     ()  const   { return IsAutomatic () && CheckToBool ( AutoTalairach ); }
    bool                IsAutoAAL           ()  const   { return IsAutomatic () && CheckToBool ( AutoAAL       ); }
    bool                IsAutoOtherRois     ()  const   { return IsAutomatic () && CheckToBool ( AutoOtherRois ); }
    bool                IsAutoSpMri         ()  const   { return IsAutoAAL () || IsAutoOtherRois (); }  // these 2 processings have a lot in common

    bool                IsGenerateRoisFile  ()  const   { return IsAutomatic () && StringIsNotEmpty ( GenerateRoisFile ); }
};


EndBytePacking

//----------------------------------------------------------------------------

enum            CreateRoisTypes
                {
                CreateRoisNone,

                CreateRoisInteractiveTracks,
                CreateRoisInteractiveXyz,
                CreateRoisInteractiveSp,

                CreateRoisAutomaticSpMri,
                CreateRoisAutomaticSpAAL,           // !AAL3v1 version!
                CreateRoisAutomaticSpTalairach,
                };


class   TBaseView;
class   TVolumeView;
class   TTracksView;
class   TElectrodesView;
class   TSolutionPointsView;
class   TRois;
class   TSelection;
class   TStrings;

                                        // This dialog is used to create TRois objects and save them to file.
                                        // ROIs are basically lists of indexes with constraints, and creation time is crucial to make sure these lists are consistent:
                                        // no duplicates across lists, limits, empty rois, you name it.
                                        // BTW it is not advised to create ROIs yourself, as the journey is definitely full of traps...
                                        // 
                                        // Dialog behavior itself is a bit complex:
                                        //  - it can create ROIs from totally different types of input: tracks (EEG, RIS), electrodes or solution points
                                        //  - creation process can be interactive, by clicking on tracks / points, or automatic
                                        //  - dialog is modeless, i.e. it can lose focus for the benefit of another window
                                        //  - some files can be optional or mandatory according to current state
                                        // 
                                        // The interactive case is the trickiest one, as it has to keep track of 1 or 2 windows, all while clicking on stuff.
                                        // It is what the dialog is mostly about, as tThe automatic case is much simpler to handle, just checking the provided files and running the scan...
class   TCreateRoisDialog   :   public  TBaseDialog
{
public:
                        TCreateRoisDialog ( owl::TWindow* parent, owl::TResId resId );
                       ~TCreateRoisDialog ();


protected:
    owl::TEdit*         InputFile;
    owl::TCheckBox*     UseRoisMriFile;
    owl::TEdit*         RoisMriFile;
    owl::TEdit*         RoisLabelsFile;

    owl::TRadioButton*  Automatic;
    owl::TRadioButton*  Interactive;

    owl::TRadioButton*  AutoTalairach;
    owl::TRadioButton*  AutoAAL;
    owl::TRadioButton*  AutoOtherRois;
    owl::TEdit*         GenerateRoisFile;

    owl::TEdit*         RoiName;
    owl::TEdit*         Selection;

    owl::TStatic*       MessageInteractive;

    owl::TEdit*         NumRois;
    owl::TListBox*      RoisSummary;

    owl::TEdit*         BaseFileName;
    owl::TCheckBox*     OpenAuto;



    TBaseView*          BaseView;           // main view to process from
    TVolumeView*        MriView;            // optional view to process from
    bool                CanCloseBaseView;
    bool                CanCloseMriView;
                                            // Specialized versions of BaseView:
    TTracksView*        EegView;
    TElectrodesView*    XyzView;
    TSolutionPointsView*SpView;
    const TStrings*     Names;

    CreateRoisTypes     Processing;         // type of processing
    TRois*              Rois;               // current construction
    TSelection*         SelForRoi;

    TTimer              Timer;


    void                SetupWindow                 ();
    void                EvTimer                     ( owl::uint timerId );

    void                CleanUp                     ();
    void                CmSetProcessing             ();
    void                SetProcessing               ( bool docleanup );

    void                UpdateSelection             ( bool getselection );
    void                AddRoiSummary               ();

    bool                CanTalairach                ()  const;
    void                CmTalairachEnable           ( owl::TCommandEnabler &tce );


    void                CmAutomaticChanged          ();
    void                CmChangeInputFileEnable     ( owl::TCommandEnabler &tce );
    void                CmChangeRoisMriFileEnable   ( owl::TCommandEnabler &tce );
    void                CmRoisLabelsFileEnable      ( owl::TCommandEnabler &tce );
    void                CmInteractiveEnable         ( owl::TCommandEnabler &tce );
    void                CmAutomaticEnable           ( owl::TCommandEnabler &tce );
    void                CmGenerateRoisFileEnable    ( owl::TCommandEnabler &tce );
    void                CmUseRoisMriFile            ();
    void                CmSelectionChange           ();
    void                CmAddSelection              ();
    void                CmAddSelectionEnable        ( owl::TCommandEnabler &tce );
    void                CmRemoveSelection           ();
    void                CmRemoveSelectionEnable     ( owl::TCommandEnabler &tce );
    void                CmOk                        ();
    void                CmOkEnable                  ( owl::TCommandEnabler &tce );
    void                CmCancel                    ();
    void                CmBrowseInputFile           ();
    void                SetInputFile                ( const char* file );
    void                CmBrowseRoisMriFile         ();
    void                SetRoisMriFile              ( const char* file );
    void                CmBrowseTextFile            ( owlwparam w );
    void                SetTextFile                 ( owlwparam w, const char* file );
    void                EvDropFiles                 ( owl::TDropInfo drop );
    void                CmUpOneDirectory            ();
    void                CmBrowseRoiFile             ();

    DECLARE_RESPONSE_TABLE ( TCreateRoisDialog );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
