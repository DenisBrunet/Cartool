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

#include    <owl/pch.h>

#include    "TInterpolateTracksDialog.h"

#include    "Files.Extensions.h"
#include    "Math.Utils.h"
#include    "Strings.Grep.h"
#include    "Dialogs.Input.h"

#include    "TRisDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;
using namespace arma;

namespace crtl {

OptimizeOff

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // declare this global variable
TInterpolateTracksStructEx      InterpolateTracksTransfer;


const char  PredefLandmarksStrings[ NumLandmarksSystems ][ NumLandmarksInfo ][ 32 ] =
            {
                 // Front           Left        Top         Right       Rear            Description
                 //=========================================================================================
                {   "",             "",         "",         "",         "",             ""                  },
                {   "Fpz",          "T3",       "Cz",       "T4",       "Oz",           "10-10 System"      },
                {   "Fpz",          "T7",       "Cz",       "T8",       "O1 O2",        "10-10 System"      },
                {   "15 18",        "46",       "VREF",     "109",      "76",           "EGI Version 2"     },
                {   "15 18",        "46",       "REF",      "109",      "76",           "EGI Version 3"     },
                {   "E14 E15 E21",  "E39",      "REF",      "E115",     "E75",          "EGI Hydrocel 129"  },  // !For 128 electrodes, NOT 256!
                {   "E14 E15 E21",  "E39",      "Cz",       "E115",     "E75",          "EGI Hydrocel 129"  },  // !For 128 electrodes, NOT 256!
                {   "104",          "38",       "96",       "70",       "7",            "Neuroscan"         },
                {   "C17",          "D23",      "A1",       "B26",      "A23",          "Biosemi 128"       },
                {   "D8",           "E9",       "A1",       "B32",      "A23",          "Biosemi 192"       },
                {   "140",          "82",       "1",        "203",      "19",           "Biosemi 256"       },
            };


//----------------------------------------------------------------------------
                                        // init variables that do not belong to the dialog
        TInterpolateTracksStruct::TInterpolateTracksStruct ()
{
ClearString ( FromXyz );
ClearString ( FromBadElectrodes );

ClearString ( FromFront );
ClearString ( FromLeft  );
ClearString ( FromTop   );
ClearString ( FromRight );
ClearString ( FromRear  );

SameXyz        = BoolToCheck ( true  );
AnotherXyz     = BoolToCheck ( false );
ClearString ( ToXyz );

ClearString ( ToFront );
ClearString ( ToLeft  );
ClearString ( ToTop   );
ClearString ( ToRight );
ClearString ( ToRear  );

SurfaceSpline  = BoolToCheck ( false );
SphericalSpline= BoolToCheck ( false );
ThreeDSpline   = BoolToCheck ( true  );
CDSpherical    = BoolToCheck ( false );
IntegerToString ( Degree, DefaultInterpolationDegree );

FileTypes.Clear ();
for ( int i = 0; i < NumSavingEegFileTypes; i++ )
    InterpolateTracksTransfer.FileTypes.AddString ( SavingEegFileTypePreset[ i ], i == PresetFileTypeDefaultEEG );

CleanUp        = BoolToCheck ( true  );
OpenAuto       = BoolToCheck ( true  );
ClearString ( InfixFilename );
}


        TInterpolateTracksStructEx::TInterpolateTracksStructEx ()
{
FromXyzLinked           = false;
}


//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TInterpolateTracksDialog, TBaseDialog )

    EV_WM_DROPFILES,

    EV_EN_CHANGE                ( IDC_FROMXYZ,              CmFromXyzChange ),
    EV_COMMAND_AND_ID           ( IDC_BROWSEFROMXYZ,        CmBrowseXyzFileName ),
    EV_COMMAND_AND_ID           ( IDC_COPYTOLANDMARKS,      CmCopyLandmarks ),
    EV_COMMAND_ENABLE           ( IDC_FROMXYZ,              CmFromXyzEnable ),
    EV_COMMAND_ENABLE           ( IDC_BROWSEFROMXYZ,        CmFromXyzEnable ),

    EV_COMMAND_ENABLE           ( IDC_COPYTOLANDMARKS,      CmToXyzEnable ),

    EV_COMMAND_AND_ID           ( IDC_SAMEXYZ,              CmChoiceTo ),
    EV_COMMAND_AND_ID           ( IDC_ANOTHERXYZ,           CmChoiceTo ),
    EV_EN_CHANGE                ( IDC_TOXYZ,                CmToXyzChange ),
    EV_COMMAND_AND_ID           ( IDC_BROWSETOXYZ,          CmBrowseXyzFileName ),
    EV_COMMAND_AND_ID           ( IDC_COPYFROMLANDMARKS,    CmCopyLandmarks ),
    EV_COMMAND_ENABLE           ( IDC_BROWSETOXYZ,          CmToXyzEnable ),
    EV_COMMAND_ENABLE           ( IDC_TOXYZ,                CmToXyzEnable ),

    EV_COMMAND_ENABLE           ( IDC_TOFRONT,              CmToLandmarksEnable ),
    EV_COMMAND_ENABLE           ( IDC_TOLEFT,               CmToLandmarksEnable ),
    EV_COMMAND_ENABLE           ( IDC_TOTOP,                CmToLandmarksEnable ),
    EV_COMMAND_ENABLE           ( IDC_TORIGHT,              CmToLandmarksEnable ),
    EV_COMMAND_ENABLE           ( IDC_TOREAR,               CmToLandmarksEnable ),
    EV_COMMAND_ENABLE           ( IDC_TODESCRIPTION,        CmToLandmarksEnable ),
    EV_COMMAND_ENABLE           ( IDC_COPYFROMLANDMARKS,    CmToLandmarksEnable ),
    
    EV_COMMAND_AND_ID           ( IDC_FROMPREDEFLANDMARKS,  CmPredefLandmarks ),
    EV_COMMAND_AND_ID           ( IDC_TOPREDEFLANDMARKS,    CmPredefLandmarks ),
    EV_COMMAND_ENABLE           ( IDC_TOPREDEFLANDMARKS,    CmToLandmarksEnable ),

    EV_COMMAND_ENABLE           ( IDC_3DSPLINE,             CmSplineEnable ),
    EV_COMMAND_ENABLE           ( IDC_SPLINEDEGREE,         CmSplineDegreeEnable ),

    EV_COMMAND_ENABLE           ( IDC_PROCESSCURRENT,       CmProcessCurrentEnable ),
    EV_COMMAND_ENABLE           ( IDC_PROCESSBATCH,         CmProcessBatchEnable ),

END_RESPONSE_TABLE;


        TInterpolateTracksDialog::TInterpolateTracksDialog ( TWindow* parent, TResId resId, TTracksDoc* doc )
      : TBaseDialog ( parent, resId, doc )
{
StringCopy ( BatchFilesExt, AllEegFilesExt );


FromXyz             = new TEdit         ( this, IDC_FROMXYZ, EditSizeText );
FromBadElectrodes   = new TEdit         ( this, IDC_REMOVINGELEC, EditSizeTextLong );

FromFront           = new TEdit         ( this, IDC_FROMFRONT, EditSizeValue );
FromLeft            = new TEdit         ( this, IDC_FROMLEFT,  EditSizeValue );
FromTop             = new TEdit         ( this, IDC_FROMTOP,   EditSizeValue );
FromRight           = new TEdit         ( this, IDC_FROMRIGHT, EditSizeValue );
FromRear            = new TEdit         ( this, IDC_FROMREAR,  EditSizeValue );
FromDescription     = new TStatic       ( this, IDC_FROMDESCRIPTION );

SameXyz             = new TRadioButton  ( this, IDC_SAMEXYZ );
AnotherXyz          = new TRadioButton  ( this, IDC_ANOTHERXYZ );
ToXyz               = new TEdit         ( this, IDC_TOXYZ, EditSizeText );

ToFront             = new TEdit         ( this, IDC_TOFRONT,   EditSizeValue );
ToLeft              = new TEdit         ( this, IDC_TOLEFT,    EditSizeValue );
ToTop               = new TEdit         ( this, IDC_TOTOP,     EditSizeValue );
ToRight             = new TEdit         ( this, IDC_TORIGHT,   EditSizeValue );
ToRear              = new TEdit         ( this, IDC_TOREAR,    EditSizeValue );
ToDescription       = new TStatic       ( this, IDC_TODESCRIPTION );

SurfaceSpline       = new TRadioButton  ( this, IDC_SURFACESPLINE );
SphericalSpline     = new TRadioButton  ( this, IDC_SPHERICALSPLINE );
ThreeDSpline        = new TRadioButton  ( this, IDC_3DSPLINE );
CDSpherical         = new TRadioButton  ( this, IDC_CDSPHERICAL );
Degree              = new TEdit         ( this, IDC_SPLINEDEGREE, EditSizeValue );

FileTypes           = new TComboBox     ( this, IDC_FILETYPES );
CleanUp             = new TCheckBox     ( this, IDC_CLEANUP );
OpenAuto            = new TCheckBox     ( this, IDC_OPENAUTO );
InfixFilename       = new TEdit         ( this, IDC_INFIXFILENAME, EditSizeText );

                                        // are we entering batch?
BatchProcessing     = doc == 0;


SetTransferBuffer ( dynamic_cast <TInterpolateTracksStruct*> ( &InterpolateTracksTransfer ) );

                                        // force RIS output selection if input is also RIS
if  ( EEGDoc ) {
    if      ( dynamic_cast<TRisDoc*> ( EEGDoc ) )

        InterpolateTracksTransfer.FileTypes.Select ( PresetFileTypeDefaultRIS );
                                        // force to default EEG format if input is not RIS and file output is RIS(?)
    else if ( InterpolateTracksTransfer.FileTypes.GetSelIndex () == PresetFileTypeDefaultRIS )

        InterpolateTracksTransfer.FileTypes.Select ( PresetFileTypeDefaultEEG );
    }

                                        // some override, on each call
if ( BatchProcessing ) {
    InterpolateTracksTransfer.OpenAuto          = BoolToCheck ( false );
    }

else {
    InterpolateTracksTransfer.OpenAuto          = BoolToCheck ( true  );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

FromPredef          = PredefClear;
ToPredef            = PredefClear;
FromGrid            = false;

                                        // Reset all file names, list points & names
IT.Reset ();
}


void    TInterpolateTracksDialog::SetupWindow ()
{
TDialog::SetupWindow ();

                                        // don't know why the transfer buffer is not properly sent to the dialog...
TransferData ( tdSetData );
}


        TInterpolateTracksDialog::~TInterpolateTracksDialog ()
{
delete  FromXyz;        delete  FromBadElectrodes;
delete  FromFront;      delete  FromLeft;           delete  FromTop;        delete  FromRight;      delete  FromRear;
delete  FromDescription;
delete  SameXyz;
delete  AnotherXyz;     delete  ToXyz;
delete  ToFront;        delete  ToLeft;             delete  ToTop;          delete  ToRight;        delete  ToRear;
delete  ToDescription;
delete  SurfaceSpline;  delete  SphericalSpline;    delete  ThreeDSpline;   delete  CDSpherical;
delete  Degree;
delete  FileTypes;
delete  CleanUp;        delete  OpenAuto;           delete  InfixFilename;
}


//----------------------------------------------------------------------------
void    TInterpolateTracksDialog::EvDropFiles ( TDropInfo drop )
{
TPointInt           where;
TGoF                tracksfiles     ( drop, BatchFilesExt                   );
TGoF                xyzfiles        ( drop, AllCoordinatesFilesExt, &where  );

char                buff[ 256 ];
StringCopy ( buff, BatchFilesExt, " " AllCoordinatesFilesExt );
TGoF                remainingfiles  ( drop, buff, 0, true );

drop.DragFinish ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                fromxyzlinked   = InterpolateTracksTransfer.FromXyzLinked;


if ( (bool) xyzfiles ) {

    int             currxyz         = 0;

    for ( int i = 0; i < (int) xyzfiles; i++ ) {

        if ( (int) xyzfiles == 1 )

            if ( where.Y < 227 ) {  // choose according to position
                if ( ! fromxyzlinked )
                    FromXyz->SetText ( xyzfiles[ i ] );
                else
                    ShowMessage ( "You can not change this coordinates file now,"  NewLine 
                                  "as it is already provided by the current link." NewLine NewLine 
                                  "Close the link file first if you wish to use another coordinates file.", InterpolationTitle, ShowMessageWarning );
                }
            else {
//              if ( ! CheckToBool ( SameXyz->GetCheck() ) )
//                  ToXyz->SetText ( xyzfiles[ i ] );

                SameXyz   ->SetCheck ( BoolToCheck ( false ) );
                AnotherXyz->SetCheck ( BoolToCheck ( true  ) );
                ToXyz     ->SetText  ( xyzfiles[ i ] );
                }

        else {                      // more than 1 xyz dropped
            currxyz++;
                                    // use the first 2 only
            if      ( currxyz == 1 )  { if ( ! fromxyzlinked )  FromXyz->SetText ( xyzfiles[ i ] ); }
            else if ( currxyz == 2 )                            ToXyz  ->SetText ( xyzfiles[ i ] );
            // else all ignored
            }

//        ExportTracksTransfer.UseXyzDoc  = BoolToCheck ( true  );
//        StringCopy ( ExportTracksTransfer.XyzDocFile, xyzfiles[ i ] );
//        TransferData ( tdSetData );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) tracksfiles && ! BatchProcessing )
    ShowMessage ( "Batch 'Drag & Drop' is not enabled now," NewLine 
                  "only the current file can be processed." NewLine NewLine 
                  "Close this file to have the Batch mode back...", InterpolationTitle, ShowMessageWarning );


if ( (bool) tracksfiles && BatchProcessing )
    BatchProcessDropped ( tracksfiles );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) remainingfiles )
    remainingfiles.Show ( "Skipping non-relevant file:" );
}


//----------------------------------------------------------------------------
void    TInterpolateTracksDialog::CmBrowseXyzFileName ( owlwparam w )
{
static GetFileFromUser  getfile ( "Electrodes Coordinates File", AllCoordinatesFilesFilter, 1, GetFileRead );


if ( ! getfile.Execute ( w == IDC_BROWSEFROMXYZ ? InterpolateTracksTransfer.FromXyz : InterpolateTracksTransfer.ToXyz ) )
    return;


TransferData ( tdSetData );             // this should activate CmXyzChange(?)

FromXyz->ResetCaret;
ToXyz  ->ResetCaret;

                                        // force calling the right Cm(From|To)XyzChange method
if      ( w == IDC_BROWSEFROMXYZ )  CmFromXyzChange ();
else if ( w == IDC_BROWSETOXYZ   )  CmToXyzChange   ();
}


void    TInterpolateTracksDialog::CmFromXyzEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( ! InterpolateTracksTransfer.FromXyzLinked );
}


void    TInterpolateTracksDialog::CmToXyzEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( AnotherXyz->GetCheck() ) );
}


void    TInterpolateTracksDialog::CmProcessCurrentEnable ( TCommandEnabler &tce )
{
if ( BatchProcessing ) {
    tce.Enable ( false );
    return;
    }

tce.Enable ( AreParametersOk () );
}


void    TInterpolateTracksDialog::CmProcessBatchEnable ( TCommandEnabler &tce )
{
if ( ! BatchProcessing ) {
    tce.Enable ( false );
    return;
    }

tce.Enable ( AreParametersOk () );
}


void    TInterpolateTracksDialog::CmToLandmarksEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( AnotherXyz->GetCheck() ) );
}


void    TInterpolateTracksDialog::CmSplineEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! FromGrid );
}

                                        // Spline's degree is currently not editable
void    TInterpolateTracksDialog::CmSplineDegreeEnable ( TCommandEnabler &tce )
{
tce.Enable ( false );
}


bool    TInterpolateTracksDialog::AreParametersOk ()
{
TransferData ( tdGetData );


if ( StringIsEmpty ( InterpolateTracksTransfer.FromXyz ) )
    return false;

//if ( CheckToBool ( SameXyz->GetCheck() ) && StringIsEmpty ( InterpolateTracksTransfer.FromBadElectrodes ) ) {
//    tce.Enable ( false );
//    return;
//    }

if ( CheckToBool ( AnotherXyz->GetCheck() ) && StringIsEmpty ( InterpolateTracksTransfer.ToXyz ) )
    return false;

                                        // expects all landmarks to be filled in, while allowing for all to be empty for "as is" loading(?)
bool                fromlandmarksok     =  StringIsNotEmpty ( InterpolateTracksTransfer.FromFront ) 
                                        && StringIsNotEmpty ( InterpolateTracksTransfer.FromLeft  ) 
                                        && StringIsNotEmpty ( InterpolateTracksTransfer.FromTop   ) 
                                        && StringIsNotEmpty ( InterpolateTracksTransfer.FromRight ) 
                                        && StringIsNotEmpty ( InterpolateTracksTransfer.FromRear  );

if ( ! fromlandmarksok )
    return false;


if ( CheckToBool ( AnotherXyz->GetCheck() ) ) {

    bool                tolandmarksok       =  StringIsNotEmpty ( InterpolateTracksTransfer.ToFront ) 
                                            && StringIsNotEmpty ( InterpolateTracksTransfer.ToLeft  ) 
                                            && StringIsNotEmpty ( InterpolateTracksTransfer.ToTop   ) 
                                            && StringIsNotEmpty ( InterpolateTracksTransfer.ToRight ) 
                                            && StringIsNotEmpty ( InterpolateTracksTransfer.ToRear  );

    if ( ! tolandmarksok )
        return false;
    }


return true;
}


void    TInterpolateTracksDialog::CmChoiceTo ( owlwparam w )
{
bool                samexyz         = CheckToBool ( SameXyz->GetCheck() );


if ( w == IDC_SAMEXYZ && samexyz ) {
                                        // transfer the params to "To"
    CmCopyLandmarks ( IDC_COPYFROMLANDMARKS );
    }
else if ( w == IDC_ANOTHERXYZ )
                                        // restore the "to" orientation
    CmToXyzChange ();
}


void    TInterpolateTracksDialog::CmSetTargetSpace ()
{
if ( CheckToBool ( SameXyz->GetCheck () ) )
                                        // make a copy of "from" params
    CmChoiceTo ( IDC_SAMEXYZ );
}


//----------------------------------------------------------------------------
void    TInterpolateTracksDialog::CmFromXyzChange ()
{
char                fromxyzfile[ EditSizeText ];

FromXyz->GetText ( fromxyzfile, EditSizeText );

TOpenDoc<TElectrodesDoc>    DocFromXyz ( fromxyzfile, OpenDocHidden );

if ( DocFromXyz.IsNotOpen () )
    return;
                                        // !Grid processing is currently disabled for the whole interpolation!
                                        // switch to surface if a grid
FromGrid    = false; // DocFromXyz->IsGrid ();

if ( FromGrid ) {
    SphericalSpline->SetCheck ( BoolToCheck ( false ) );
    CDSpherical    ->SetCheck ( BoolToCheck ( false ) );
    ThreeDSpline   ->SetCheck ( BoolToCheck ( false ) );
    SurfaceSpline  ->SetCheck ( BoolToCheck ( true  ) );
    }

FromFront       ->SetText ( "" );
FromLeft        ->SetText ( "" );
FromTop         ->SetText ( "" );
FromRight       ->SetText ( "" );
FromRear        ->SetText ( "" );
FromDescription ->SetText ( "" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // test if electrodes names exist

                                        // 10-10 system like
if      ( ( DocFromXyz->GetElectrodeName ( "T3" )  || DocFromXyz->GetElectrodeName ( "T7" ) )
       && ( DocFromXyz->GetElectrodeName ( "T4" )  || DocFromXyz->GetElectrodeName ( "T8" ) )
       && ( DocFromXyz->GetElectrodeName ( "Fpz" ) || DocFromXyz->GetElectrodeName ( "Fp1" ) && DocFromXyz->GetElectrodeName ( "Fp2" ) )
       && ( DocFromXyz->GetElectrodeName ( "Oz" )  || DocFromXyz->GetElectrodeName ( "O1" )  && DocFromXyz->GetElectrodeName ( "O2" )  )
       &&   DocFromXyz->GetElectrodeName ( "Cz" ) ) {

    if      ( DocFromXyz->GetElectrodeName ( "T3" ) )   FromLeft->SetText ( DocFromXyz->GetElectrodeName ( "T3" ) );
    else if ( DocFromXyz->GetElectrodeName ( "T7" ) )   FromLeft->SetText ( DocFromXyz->GetElectrodeName ( "T7" ) );

    if      ( DocFromXyz->GetElectrodeName ( "T4" ) )   FromRight->SetText ( DocFromXyz->GetElectrodeName ( "T4" ) );
    else if ( DocFromXyz->GetElectrodeName ( "T8" ) )   FromRight->SetText ( DocFromXyz->GetElectrodeName ( "T8" ) );

    if      ( DocFromXyz->GetElectrodeName ( "Fpz" ) )  FromFront->SetText ( DocFromXyz->GetElectrodeName ( "Fpz" ) );
    else if ( DocFromXyz->GetElectrodeName ( "Fp1" ) )  FromFront->SetText ( "Fp1 Fp2" );

    if      ( DocFromXyz->GetElectrodeName ( "Oz" ) )   FromRear->SetText ( DocFromXyz->GetElectrodeName ( "Oz" ) );
    else if ( DocFromXyz->GetElectrodeName ( "O1" ) )   FromRear->SetText ( "O1 O2" );

    FromTop->SetText ( DocFromXyz->GetElectrodeName ( "Cz" ) );

    FromPredef  = PredefClear;

    FromDescription ->SetText ( LandmarksDescrAutoDetect );
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if (   DocFromXyz->GetElectrodeName ( "46"  )
       &&   DocFromXyz->GetElectrodeName ( "109" )
       && ( DocFromXyz->GetElectrodeName ( "15"  )  && DocFromXyz->GetElectrodeName ( "18" )  )
       &&   DocFromXyz->GetElectrodeName ( "76"  )
       && ( DocFromXyz->GetElectrodeName ( "VREF" ) || DocFromXyz->GetElectrodeName ( "REF" ) ) ) {

                                        // EGI?
    if ( StringIs ( DocFromXyz->GetElectrodeName ( /*0*/ ), "1" )   // compiler whining about overloaded function
      && StringIs ( DocFromXyz->GetElectrodeName ( 1 ), "2" )
      && StringIs ( DocFromXyz->GetElectrodeName ( 2 ), "3" ) )
        if ( DocFromXyz->GetElectrodeName ( "VREF" ) )
            FromPredef  = PredefEGI2;
        else
            FromPredef  = PredefEGI3;
    else                                // Neuroscan
        FromPredef      = PredefNS;


    FromFront       ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkFront         ] );
    FromLeft        ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkLeft          ] );
    FromTop         ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkTop           ] );
    FromRight       ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkRight         ] );
    FromRear        ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkRear          ] );
    FromDescription ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkDescription   ] );
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if (   DocFromXyz->GetElectrodeName ( "E39"  )
       &&   DocFromXyz->GetElectrodeName ( "E115" )
       && ( DocFromXyz->GetElectrodeName ( "E14"  ) && DocFromXyz->GetElectrodeName ( "E15" ) && DocFromXyz->GetElectrodeName ( "E21" ) )
       &&   DocFromXyz->GetElectrodeName ( "E75"  ) ) { // or E75 E71 E76?

    if ( DocFromXyz->GetElectrodeName ( "Cz" ) )
        FromPredef = PredefEGIHydrocel2;
    else
        FromPredef = PredefEGIHydrocel1;

    
    FromFront       ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkFront         ] );
    FromLeft        ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkLeft          ] );
    FromTop         ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkTop           ] );
    FromRight       ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkRight         ] );
    FromRear        ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkRear          ] );
    FromDescription ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkDescription   ] );
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Biosemi 256
else if (   DocFromXyz->GetElectrodeName ( "140" )
       &&   DocFromXyz->GetElectrodeName ( "82"  )
       &&   DocFromXyz->GetElectrodeName ( "1"   )
       &&   DocFromXyz->GetElectrodeName ( "203" )
       &&   DocFromXyz->GetElectrodeName ( "19"  ) ) {

    FromPredef = PredefBiosemi256;

    FromFront       ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkFront         ] );
    FromLeft        ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkLeft          ] );
    FromTop         ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkTop           ] );
    FromRight       ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkRight         ] );
    FromRear        ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkRear          ] );
    FromDescription ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkDescription   ] );
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Biosemi 192 tested before Biosemi 128
else if (   DocFromXyz->GetElectrodeName ( "D8"   )
       &&   DocFromXyz->GetElectrodeName ( "E9"   )
       &&   DocFromXyz->GetElectrodeName ( "A1"   )
       &&   DocFromXyz->GetElectrodeName ( "B32"  )
       &&   DocFromXyz->GetElectrodeName ( "A23"  ) ) {

    FromPredef = PredefBiosemi192;

    FromFront       ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkFront         ] );
    FromLeft        ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkLeft          ] );
    FromTop         ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkTop           ] );
    FromRight       ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkRight         ] );
    FromRear        ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkRear          ] );
    FromDescription ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkDescription   ] );
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Biosemi 128
else if (   DocFromXyz->GetElectrodeName ( "C17"  )
       &&   DocFromXyz->GetElectrodeName ( "D23"  )
       &&   DocFromXyz->GetElectrodeName ( "A1"   )
       &&   DocFromXyz->GetElectrodeName ( "B23"  )
       &&   DocFromXyz->GetElectrodeName ( "A23"  ) ) {

    FromPredef = PredefBiosemi128;

    FromFront       ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkFront         ] );
    FromLeft        ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkLeft          ] );
    FromTop         ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkTop           ] );
    FromRight       ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkRight         ] );
    FromRear        ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkRear          ] );
    FromDescription ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkDescription   ] );
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TInterpolateTracksStructEx*     transfer   = (TInterpolateTracksStructEx *) GetTransferBuffer ();
                                        // ProcessCurrent may have given us some selection, so use it now that we now the XYZ
if ( ! BatchProcessing && transfer->BadElectrodesSelection.IsAllocated () ) {

    char        buff[ EditSizeTextLong ];

    transfer->BadElectrodesSelection.ToText ( buff, DocFromXyz->GetElectrodesNames (), AuxiliaryTracksNames );

    FromBadElectrodes->SetText ( buff );
    StringCopy  ( transfer->FromBadElectrodes, buff );  // also directly updating the transfer buffer, for re-opening dialog from tracks with selection
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // close from xyz
DocFromXyz.Close ();
                                        // spread new data, if meaningful
CmSetTargetSpace ();
}


//----------------------------------------------------------------------------
void    TInterpolateTracksDialog::CmToXyzChange ()
{
char                toxyzfile[ EditSizeText ];

ToXyz->GetText ( toxyzfile, EditSizeText );

TOpenDoc<TElectrodesDoc>    DocToXyz ( toxyzfile, OpenDocHidden );

if ( DocToXyz.IsNotOpen () )
    return;


ToFront         ->SetText ( "" );
ToLeft          ->SetText ( "" );
ToTop           ->SetText ( "" );
ToRight         ->SetText ( "" );
ToRear          ->SetText ( "" );
ToDescription   ->SetText ( "" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // test if electrodes names exist

                                        // 10-10 system like
if      ( ( DocToXyz->GetElectrodeName ( "T3" )  || DocToXyz->GetElectrodeName ( "T7" ) )
       && ( DocToXyz->GetElectrodeName ( "T4" )  || DocToXyz->GetElectrodeName ( "T8" ) )
       && ( DocToXyz->GetElectrodeName ( "Fpz" ) || DocToXyz->GetElectrodeName ( "Fp1" ) && DocToXyz->GetElectrodeName ( "Fp2" ) )
       && ( DocToXyz->GetElectrodeName ( "Oz" )  || DocToXyz->GetElectrodeName ( "O1" )  && DocToXyz->GetElectrodeName ( "O2" )  )
       &&   DocToXyz->GetElectrodeName ( "Cz" ) ) {

    if      ( DocToXyz->GetElectrodeName ( "T3" ) )     ToLeft->SetText ( DocToXyz->GetElectrodeName ( "T3" ) );
    else if ( DocToXyz->GetElectrodeName ( "T7" ) )     ToLeft->SetText ( DocToXyz->GetElectrodeName ( "T7" ) );

    if      ( DocToXyz->GetElectrodeName ( "T4" ) )     ToRight->SetText ( DocToXyz->GetElectrodeName ( "T4" ) );
    else if ( DocToXyz->GetElectrodeName ( "T8" ) )     ToRight->SetText ( DocToXyz->GetElectrodeName ( "T8" ) );

    if      ( DocToXyz->GetElectrodeName ( "Fpz" ) )    ToFront->SetText ( DocToXyz->GetElectrodeName ( "Fpz" ) );
    else if ( DocToXyz->GetElectrodeName ( "Fp1" ) )    ToFront->SetText ( "Fp1 Fp2" );

    if      ( DocToXyz->GetElectrodeName ( "Oz" ) )     ToRear->SetText ( DocToXyz->GetElectrodeName ( "Oz" ) );
    else if ( DocToXyz->GetElectrodeName ( "O1" ) )     ToRear->SetText ( "O1 O2" );

    ToTop->SetText ( DocToXyz->GetElectrodeName ( "Cz" ) );

    ToPredef = PredefClear;

    ToDescription ->SetText ( LandmarksDescrAutoDetect );
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if (   DocToXyz->GetElectrodeName ( "46"  )
       &&   DocToXyz->GetElectrodeName ( "109" )
       && ( DocToXyz->GetElectrodeName ( "15"  )  && DocToXyz->GetElectrodeName ( "18" )  )
       &&   DocToXyz->GetElectrodeName ( "76"  )
       && ( DocToXyz->GetElectrodeName ( "VREF" ) || DocToXyz->GetElectrodeName ( "REF" ) ) ) {

                                        // EGI?
    if ( StringIs ( DocToXyz->GetElectrodeName ( /*0*/ ), "1" )   // compiler whining about overloaded function
      && StringIs ( DocToXyz->GetElectrodeName ( 1 ), "2" )
      && StringIs ( DocToXyz->GetElectrodeName ( 2 ), "3" ) )
        if ( DocToXyz->GetElectrodeName ( "VREF" ) )
            ToPredef    = PredefEGI2;
        else
            ToPredef    = PredefEGI3;
    else                                // Neuroscan
        ToPredef        = PredefNS;


    ToFront         ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkFront         ] );
    ToLeft          ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkLeft          ] );
    ToTop           ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkTop           ] );
    ToRight         ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkRight         ] );
    ToRear          ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkRear          ] );
    ToDescription   ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkDescription   ] );
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if (   DocToXyz->GetElectrodeName ( "E39"  )
       &&   DocToXyz->GetElectrodeName ( "E115" )
       && ( DocToXyz->GetElectrodeName ( "E14"  ) && DocToXyz->GetElectrodeName ( "E15" ) && DocToXyz->GetElectrodeName ( "E21" ) )
       &&   DocToXyz->GetElectrodeName ( "E75"  ) ) { // or E75 E71 E76?

    if ( DocToXyz->GetElectrodeName ( "Cz" ) )
        ToPredef = PredefEGIHydrocel2;
    else
        ToPredef = PredefEGIHydrocel1;

    
    ToFront         ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkFront         ] );
    ToLeft          ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkLeft          ] );
    ToTop           ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkTop           ] );
    ToRight         ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkRight         ] );
    ToRear          ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkRear          ] );
    ToDescription   ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkDescription   ] );
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Biosemi 256
else if (   DocToXyz->GetElectrodeName ( "140" )
       &&   DocToXyz->GetElectrodeName ( "82"  )
       &&   DocToXyz->GetElectrodeName ( "1"   )
       &&   DocToXyz->GetElectrodeName ( "203" )
       &&   DocToXyz->GetElectrodeName ( "19"  ) ) {

    ToPredef = PredefBiosemi256;

    ToFront         ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkFront         ] );
    ToLeft          ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkLeft          ] );
    ToTop           ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkTop           ] );
    ToRight         ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkRight         ] );
    ToRear          ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkRear          ] );
    ToDescription   ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkDescription   ] );
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Biosemi 192 tested before Biosemi 128
else if (   DocToXyz->GetElectrodeName ( "D8"   )
       &&   DocToXyz->GetElectrodeName ( "E9"   )
       &&   DocToXyz->GetElectrodeName ( "A1"   )
       &&   DocToXyz->GetElectrodeName ( "B32"  )
       &&   DocToXyz->GetElectrodeName ( "A23"  ) ) {

    ToPredef = PredefBiosemi192;

    ToFront         ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkFront         ] );
    ToLeft          ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkLeft          ] );
    ToTop           ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkTop           ] );
    ToRight         ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkRight         ] );
    ToRear          ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkRear          ] );
    ToDescription   ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkDescription   ] );
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Biosemi 128
else if (   DocToXyz->GetElectrodeName ( "C17"  )
       &&   DocToXyz->GetElectrodeName ( "D23"  )
       &&   DocToXyz->GetElectrodeName ( "A1"   )
       &&   DocToXyz->GetElectrodeName ( "B23"  )
       &&   DocToXyz->GetElectrodeName ( "A23"  ) ) {

    ToPredef = PredefBiosemi128;

    ToFront         ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkFront         ] );
    ToLeft          ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkLeft          ] );
    ToTop           ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkTop           ] );
    ToRight         ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkRight         ] );
    ToRear          ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkRear          ] );
    ToDescription   ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkDescription   ] );
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // close to xyz
DocToXyz.Close ();
}


//----------------------------------------------------------------------------
void    TInterpolateTracksDialog::CmPredefLandmarks ( owlwparam w )
{
if      ( w == IDC_FROMPREDEFLANDMARKS ) {

    FromPredef  = NextState ( FromPredef, NumLandmarksSystems, (bool) VkShift () );

    FromFront       ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkFront         ] );
    FromLeft        ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkLeft          ] );
    FromTop         ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkTop           ] );
    FromRight       ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkRight         ] );
    FromRear        ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkRear          ] );
    FromDescription ->SetText ( PredefLandmarksStrings[ FromPredef ][ LandmarkDescription   ] );
    }

else if ( w == IDC_TOPREDEFLANDMARKS ) {

    ToPredef    = NextState ( ToPredef, NumLandmarksSystems, (bool) VkShift () );

    ToFront         ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkFront         ] );
    ToLeft          ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkLeft          ] );
    ToTop           ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkTop           ] );
    ToRight         ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkRight         ] );
    ToRear          ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkRear          ] );
    ToDescription   ->SetText ( PredefLandmarksStrings[ ToPredef ][ LandmarkDescription   ] );
    }
}


void    TInterpolateTracksDialog::CmCopyLandmarks ( owlwparam w )
{
char                buff[ EditSizeValue ];

if      ( w == IDC_COPYTOLANDMARKS ) {
    ToLeft          ->GetText ( buff, EditSizeValue );  FromLeft        ->SetText ( buff );
    ToRight         ->GetText ( buff, EditSizeValue );  FromRight       ->SetText ( buff );
    ToFront         ->GetText ( buff, EditSizeValue );  FromFront       ->SetText ( buff );
    ToRear          ->GetText ( buff, EditSizeValue );  FromRear        ->SetText ( buff );
    ToTop           ->GetText ( buff, EditSizeValue );  FromTop         ->SetText ( buff );
    ToDescription   ->GetText ( buff, EditSizeValue );  FromDescription ->SetText ( buff );
    }

else if ( w == IDC_COPYFROMLANDMARKS ) {
    FromLeft        ->GetText ( buff, EditSizeValue );  ToLeft          ->SetText ( buff );
    FromRight       ->GetText ( buff, EditSizeValue );  ToRight         ->SetText ( buff );
    FromFront       ->GetText ( buff, EditSizeValue );  ToFront         ->SetText ( buff );
    FromRear        ->GetText ( buff, EditSizeValue );  ToRear          ->SetText ( buff );
    FromTop         ->GetText ( buff, EditSizeValue );  ToTop           ->SetText ( buff );
    FromDescription ->GetText ( buff, EditSizeValue );  ToDescription   ->SetText ( buff );
    }
}


//----------------------------------------------------------------------------

void    TInterpolateTracksDialog::ProcessCurrent ( void* usetransfer, const char* /*moreinfix*/ )
{
                                        // this shouldn't happen
if ( EEGDoc == 0 )
    return;

                                        // check we have all the info we need
if ( ! IT.IsOpen () )
    return;


TInterpolateTracksStructEx*     transfer    = usetransfer ? (TInterpolateTracksStructEx*) usetransfer : &InterpolateTracksTransfer;

if ( transfer == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                hasfrombadelectrodes    = ! StringIsSpace ( transfer->FromBadElectrodes );
TSelection          fromgoodsel ( EEGDoc->GetTotalElectrodes(), OrderSorted );

                                        // select all
fromgoodsel.Set ();
                                        // but without pseudo-electrodes
EEGDoc->ClearPseudo ( fromgoodsel );

if ( hasfrombadelectrodes )
                                        // remove specified electrodes
    fromgoodsel.Reset ( transfer->FromBadElectrodes, &IT.GetFromOrigPointsNames () );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           buff;
int                 fromnumpoints   = IT.GetFromPoints ().GetNumPoints ();

                                        // same number of FINAL electrodes
if ( (int) fromgoodsel != fromnumpoints )

    StringCopy  ( buff, "Number of electrodes does not match:"                      NewLine
                        "    ", IntegerToString ( (int) fromgoodsel ),  "  in EEG"  NewLine 
                        "    ", IntegerToString ( fromnumpoints ),      "  in XYZ"  NewLine 
                        "Skipping this file!" );


if ( EEGDoc->GetNumAuxElectrodes() != 0 )

    StringCopy  ( buff, IntegerToString ( EEGDoc->GetNumAuxElectrodes() ), " auxiliaries have been found" NewLine 
                                                                           "Skipping this file!" );


if ( StringIsNotEmpty ( buff ) ) {

    ShowMessage ( buff, EEGDoc->GetTitle () /*eegfile*/, ShowMessageWarning );

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Output file type
SavingEegFileTypes  filetype            = (SavingEegFileTypes) transfer->FileTypes.GetSelIndex ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // silencing in these cases
bool                silent              = NumBatchFiles () > 1;


if ( IsBatchFirstCall () && BatchFileNames.NumFiles () > 1 )

    CartoolApplication->WindowMinimize ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           neweegfile;


IT.InterpolateTracks    (   EEGDoc,
                            transfer->InfixFilename,    SavingEegFileExtPreset[ filetype ],     neweegfile,
                            silent
                        );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                openauto            = CheckToBool ( transfer->OpenAuto );

                                        // complimentary opening the file for the user
if ( openauto )

    neweegfile.Open ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( IsBatchLastCall () && BatchFileNames.NumFiles () > 1 ) {

    CartoolApplication->WindowRestore ();

    CartoolApplication->SetMainTitle ( "Done - " InterpolationTitle, neweegfile );
    }

UpdateApplication;
}


//----------------------------------------------------------------------------
void    TInterpolateTracksDialog::BatchProcess ()
{
                                        // Changing priority
SetProcessPriority ( BatchProcessingPriority );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TransferData ( tdGetData );

TInterpolateTracksStructEx*     transfer   = &InterpolateTracksTransfer;


TracksInterpolationType         interpolationtype   = CheckToBool ( transfer->SurfaceSpline   ) ?   Interpolation2DSurfaceSpline
                                                    : CheckToBool ( transfer->SphericalSpline ) ?   InterpolationSphericalSpline
                                                    : CheckToBool ( transfer->CDSpherical     ) ?   InterpolationSphericalCurrentDensitySpline
                                                    : CheckToBool ( transfer->ThreeDSpline    ) ?   Interpolation3DSpline
                                                    :                                               UnknownInterpolation;
                                                    // currently not editable, and set to DefaultInterpolationDegree
int                             degree              = Clip ( StringToInteger ( transfer->Degree ), MinInterpolationDegree, MaxInterpolationDegree );

InterpolationTargetOptions      targetspace         = CheckToBool ( transfer->SameXyz    )      ?   BackToOriginalElectrodes
                                                    : CheckToBool ( transfer->AnotherXyz )      ?   ToOtherElectrodes
                                                    :                                               BackToOriginalElectrodes;

bool                            cleanupfiles        = CheckToBool ( transfer->CleanUp );

int                             numinfiles          = BatchFileNames.NumFiles ();

bool                            silent              = NumBatchFiles () > 1;

if ( numinfiles > MaxFilesToOpen )
    transfer->OpenAuto  = BoolToCheck ( false );

                                        // Init points, transform & matrices - better to do it only once for a batch of files, for efficiency reasons
IT.Set  (   interpolationtype,      degree,
            targetspace,

            transfer->FromXyz,
            FiducialNormalization,      // always when called from the dialog
            transfer->FromFront,    transfer->FromLeft,     transfer->FromTop,  transfer->FromRight,    transfer->FromRear, 
            transfer->FromBadElectrodes,
            
            transfer->ToXyz,
            FiducialNormalization,      // always when called from the dialog
            transfer->ToFront,      transfer->ToLeft,       transfer->ToTop,    transfer->ToRight,      transfer->ToRear, 

            BatchFileNames[ 0 ],        // temp files in the same directory as first EEG file
            silent
        );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TBaseDialog::BatchProcess ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( cleanupfiles )
                                        // removing all temp files, if requested
    IT.FilesCleanUp ();

                                        // Reset all file names, list points & names
IT.Reset ();


SetProcessPriority ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OptimizeOn

}
