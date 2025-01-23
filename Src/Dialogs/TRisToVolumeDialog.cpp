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

#include    "TRisToVolumeDialog.h"
#include    "ESI.RisToVolume.h"         // RisToVolume

#include    "Strings.Grep.h"
#include    "Files.Utils.h"
#include    "Files.TVerboseFile.h"
#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"

#include    "TTracksDoc.h"
#include    "TVolumeDoc.h"
#include    "TSolutionPointsDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // declare this global variable
TRisToVolumeStructEx    RisToVolumeTransfer;


const char  RisToVolumePresetString[ NumRisToVolumePreset ][ 16 ] =
            {
            "Nifti files",
//          "Nifti files       / Rescaled values to 255      - for display",
            "Analyze files",
//          "Analyze files / Rescaled values to 255      - for display",
            };


const char  VolumeInterpolationPresetString[ NumVolumeInterpolationPreset ][ 128 ] =
            {
            "1 Nearest Neighbor    - Constant cubes, no interpolation",
            "4 Nearest Neighbors  - Identical to inverse display",
            "Linear                      - A classic",
//          "Trilinear smooth",
//          "Fast Quadratic Kernels- Oooh so smooth!",
            "Fast Cubic Kernels    - So smooth, NO maxima artifacts",
            };


const char  VolumeFileTypeString[ NumVolumeFileTypes ][ 32 ] =
            {
            "Nifti 2 (nii)",
            "Analyze 7.5 (hdr+img)",
            };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TRisToVolumeStruct::TRisToVolumeStruct ()
{
Presets.Clear ();
for ( int i = 0; i < NumRisToVolumePreset; i++ )
    Presets.AddString ( RisToVolumePresetString[ i ], i == RisToVolumePresetDefault );

ClearString     ( GreyMaskFile );
ClearString     ( SpFile       );

InterpolationPresets.Clear ();
for ( int i = 0; i < NumVolumeInterpolationPreset; i++ )
    InterpolationPresets.AddString ( VolumeInterpolationPresetString[ i ], i == VolumeInterpolationPresetDefault );

TimeAll             = BoolToCheck ( false );
TimeInterval        = BoolToCheck ( true  );
StringCopy      ( TimeMin,  "0" );
StringCopy      ( TimeMax,  "10000" );
StringCopy      ( StepTF,   "25" );

ClearString     ( BaseFileName );
TypeUnsignedByte    = BoolToCheck ( false );
TypeFloat           = BoolToCheck ( true  );

FileTypes.Clear ();
for ( int i = 0; i < NumVolumeFileTypes; i++ )
    FileTypes.AddString ( VolumeFileTypeString[ i ], i == VolumeTypeDefault );

OpenAuto            = BoolToCheck ( true  );
}


        TRisToVolumeStructEx::TRisToVolumeStructEx ()
{
                                        // Storing all checks across all dimensions
CompatRis       .Reset ();
CompatSp        .Reset ();

IsSpOK                  =
IsGreyMaskOK            =
IsSpAndGreyMaskOK       =
IsRisOK                 =
IsRisAndSpOK            = false;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TRisToVolumeDialog, TBaseDialog )

    EV_WM_DROPFILES,

    EV_COMMAND                  ( IDC_BROWSESPFILE,             CmBrowseSpFile ),
    EV_COMMAND                  ( IDC_BROWSEGREYMASKFILE,       CmBrowseGreyMaskFile ),

    EV_CBN_SELCHANGE            ( IDC_RISTOVOLPRESETS,          EvPresetsChange ),

    EV_COMMAND_ENABLE           ( IDC_TIMEMIN,                  CmTimeIntervalEnable ),
    EV_COMMAND_ENABLE           ( IDC_TIMEMAX,                  CmTimeIntervalEnable ),
    EV_COMMAND_ENABLE           ( IDC_STEPTF,                   CmTimeIntervalEnable ),

    EV_COMMAND_ENABLE           ( IDC_PROCESSCURRENT,           CmProcessCurrentEnable ),
    EV_COMMAND_ENABLE           ( IDC_PROCESSBATCH,             CmProcessBatchEnable ),

END_RESPONSE_TABLE;



        TRisToVolumeDialog::TRisToVolumeDialog ( TWindow* parent, TResId resId, TTracksDoc* doc )
      : TBaseDialog ( parent, resId, doc )
{
StringCopy ( BatchFilesExt, AllRisFilesExt );


Presets             = new TComboBox     ( this, IDC_RISTOVOLPRESETS );

GreyMaskFile        = new TEdit         ( this, IDC_GREYMASKFILE, EditSizeText );
SpFile              = new TEdit         ( this, IDC_SPFILE, EditSizeText );
InterpolationPresets= new TComboBox     ( this, IDC_INTERPOLATIONPRESETS );

TimeAll             = new TRadioButton  ( this, IDC_TIMEALL );
TimeInterval        = new TRadioButton  ( this, IDC_TIMEINTERVAL   );
TimeMin             = new TEdit         ( this, IDC_TIMEMIN, EditSizeValue );
TimeMax             = new TEdit         ( this, IDC_TIMEMAX, EditSizeValue );
StepTF              = new TEdit         ( this, IDC_STEPTF, EditSizeValue );

BaseFileName        = new TEdit         ( this, IDC_BASEFILENAME, EditSizeText );
TypeUnsignedByte    = new TRadioButton  ( this, IDC_UNSIGNEDBYTE   );
TypeFloat           = new TRadioButton  ( this, IDC_FLOAT   );
FileTypes           = new TComboBox     ( this, IDC_FILETYPES );
OpenAuto            = new TCheckBox     ( this, IDC_OPENAUTO );

                                        // are we entering batch?
BatchProcessing = doc == 0;


SetTransferBuffer ( dynamic_cast <TRisToVolumeStruct*> ( &RisToVolumeTransfer ) );


bool                oldav           = CartoolApplication->AnimateViews;
CartoolApplication->AnimateViews    = false;

if ( RisToVolumeTransfer.IsGreyMaskOK   )   GreyMaskDoc.Open ( RisToVolumeTransfer.GreyMaskFile, DefaultOpeningMode );
if ( RisToVolumeTransfer.IsSpOK         )   SPDoc      .Open ( RisToVolumeTransfer.SpFile,       DefaultOpeningMode );

CartoolApplication->AnimateViews    = oldav;

                                        // force setting auto-opening according to interactive or batch mode
//RisToVolumeTransfer.OpenAuto     = BoolToCheck ( ! BatchProcessing );
}


        TRisToVolumeDialog::~TRisToVolumeDialog ()
{
delete  Presets;
delete  GreyMaskFile;           delete  SpFile;
delete  InterpolationPresets;
delete  TimeAll;                delete  TimeInterval;         
delete  TimeMin;                delete  TimeMax;                delete  StepTF;
delete  BaseFileName;
delete  TypeUnsignedByte;       delete  TypeFloat;
delete  FileTypes;              delete  OpenAuto;
}


//----------------------------------------------------------------------------
void    TRisToVolumeDialog::EvPresetsChange ()
{
RisToVolumePreset   preset          = (RisToVolumePreset) Presets->GetSelIndex ();


//InterpolationPresets->SetSelIndex   ( VolumeInterpolationPresetDefault );
TypeFloat           ->SetCheck      ( BoolToCheck ( false ) );
TypeUnsignedByte    ->SetCheck      ( BoolToCheck ( false ) );


switch ( preset ) {

    case RisToVolumeNiftiFloat:
        TypeFloat       ->SetCheck ( BoolToCheck ( true  ) );
        FileTypes->SetSelIndex ( VolumeNifti2 );
        break;

//  case RisToVolumeNiftiByte:
//      TypeUnsignedByte->SetCheck ( BoolToCheck ( true  ) );
//      FileTypes->SetSelIndex ( VolumeNifti2 );
//      break;

    case RisToVolumeAnalyzeFloat:
        TypeFloat       ->SetCheck ( BoolToCheck ( true  ) );
        FileTypes->SetSelIndex ( VolumeAnalyze );
        break;

//  case RisToVolumeAnalyzeByte:
//      TypeUnsignedByte->SetCheck ( BoolToCheck ( true  ) );
//      FileTypes->SetSelIndex ( VolumeAnalyze );
//      break;

    }

                                        // update transfer buffer
TransferData ( tdGetData );
}


//----------------------------------------------------------------------------
void    TRisToVolumeDialog::EvDropFiles ( TDropInfo drop )
{
TGoF                spfiles         ( drop, AllSolPointsFilesExt    );
TGoF                mrifiles        ( drop, AllMriFilesExt          );
TGoF                risfiles        ( drop, AllRisFilesExt          );
TGoF                remainingfiles  ( drop, AllSolPointsFilesExt " " AllMriFilesExt " " AllRisFilesExt, 0, true );

drop.DragFinish ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) mrifiles; i++ )

    SetGreyMaskFile ( mrifiles[ i ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) spfiles; i++ )

    SetSpFile ( spfiles[ i ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) risfiles )

    if ( BatchProcessing ) {
        
        CheckRis            ( risfiles );

        CheckRisAndSp       ( risfiles );

        BatchProcessDropped ( risfiles );
        }
    else                    
        ShowMessage ( "Batch 'Drag & Drop' is not enabled now,\nonly the current file can be processed.\n\nClose this file to have the Batch mode back...", "Export Tracks", ShowMessageWarning );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Let's be nice and try to load D&D Directories
                                        // For the moment, retrieving only the .ep files
for ( int i = 0; i < (int) remainingfiles; i++ ) {

    if ( ! IsDirectory ( remainingfiles[ i ] ) )
        continue;


    TGoF                    subgof;

    subgof.GrepFiles    ( remainingfiles[ i ], AllRisFilesGrep, GrepOptionDefaultFiles, true );

//  subgof.Show ( "found eeg" );

                                    // process if found some files, otherwise, complain
    if ( (bool) subgof ) {

        if ( BatchProcessing ) {
        
            CheckRis            ( subgof );

            CheckRisAndSp       ( subgof );

            BatchProcessDropped ( subgof );
            }
        else                    
            ShowMessage ( "Batch 'Drag & Drop' is not enabled now,\nonly the current file can be processed.\n\nClose this file to have the Batch mode back...", "Export Tracks", ShowMessageWarning );


        remainingfiles.RemoveRef ( remainingfiles[ i ] );
        i--;
        }
    }


if ( (bool) remainingfiles )
    remainingfiles.Show ( "Skipping non-relevant file:" );
}


//----------------------------------------------------------------------------
void    TRisToVolumeDialog::CmBrowseSpFile ()
{
SetSpFile ( 0 );
}


void    TRisToVolumeDialog::SetSpFile ( char *file )
{
static GetFileFromUser  getfile ( "Solution Points File:", AllSolPointsFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( RisToVolumeTransfer.SpFile ) )
        return;

    TransferData ( tdSetData );
    }
else {
    StringCopy ( RisToVolumeTransfer.SpFile, file );

    getfile.SetOnly ( file );
    }


TransferData ( tdSetData );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

RisToVolumeTransfer.CheckSp             ();

RisToVolumeTransfer.CheckSpAndGreyMask  ();


if ( RisToVolumeTransfer.IsSpOK )

    SPDoc.Open ( RisToVolumeTransfer.SpFile, DefaultOpeningMode );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! ( RisToVolumeTransfer.IsSpOK && SPDoc.IsOpen () ) ) {

    SPDoc.Close ();
    SpFile->SetText ( "" );

    RisToVolumeTransfer.IsSpOK              = false;
    RisToVolumeTransfer.IsSpAndGreyMaskOK   = false;
    RisToVolumeTransfer.IsRisAndSpOK        = false;
    }


GreyMaskFile->ResetCaret;
SpFile      ->ResetCaret;
}


//----------------------------------------------------------------------------
void    TRisToVolumeDialog::CmBrowseGreyMaskFile ()
{
SetGreyMaskFile ( 0 );
}


void    TRisToVolumeDialog::SetGreyMaskFile ( char *file )
{
static GetFileFromUser  getfile ( "Grey Mask MRI File:", AllMriFilesFilter, 1, GetFileRead );

TransferData ( tdGetData );


if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute ( RisToVolumeTransfer.GreyMaskFile ) )
        return;

    TransferData ( tdSetData );
    }
else {
    StringCopy ( RisToVolumeTransfer.GreyMaskFile, file );

    getfile.SetOnly ( file );
    }


TransferData ( tdSetData );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

RisToVolumeTransfer.CheckGreyMask       ();

RisToVolumeTransfer.CheckSpAndGreyMask  ();


if ( RisToVolumeTransfer.IsGreyMaskOK )

    GreyMaskDoc.Open ( RisToVolumeTransfer.GreyMaskFile, DefaultOpeningMode );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! ( RisToVolumeTransfer.IsGreyMaskOK && GreyMaskDoc.IsOpen () ) ) {

    GreyMaskDoc.Close ();
    GreyMaskFile->SetText ( "" );

    RisToVolumeTransfer.IsGreyMaskOK        = false;
    RisToVolumeTransfer.IsSpAndGreyMaskOK   = false;
    }


GreyMaskFile->ResetCaret;
SpFile      ->ResetCaret;
}


//----------------------------------------------------------------------------
                                        // Testing EEG / Freqs across themselves - A lot of testing ahead
void    TRisToVolumeDialog::CheckRis ( const TGoF& gofris )    const
{

RisToVolumeTransfer.IsRisOK         = false;

RisToVolumeTransfer.CompatRis.Reset ();

if ( gofris.IsEmpty () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check first the last group's own coherence, as tracks files
gofris.AllTracksAreCompatible ( RisToVolumeTransfer.CompatRis );


if      ( RisToVolumeTransfer.CompatRis.NumTracks == CompatibilityNotConsistent ) {
    ShowMessage ( "Files don't seem to have the same number of tracks!\nCheck again your input files...", RisToVolumeTitle, ShowMessageWarning );
    return;
    }
else if ( RisToVolumeTransfer.CompatRis.NumTracks == CompatibilityIrrelevant ) {
    ShowMessage ( "Files don't seem to have any tracks at all!\nCheck again your input files...", RisToVolumeTitle, ShowMessageWarning );
    return;
    }

                                        // this test is quite weak, as ReadFromHeader does a lousy job at retrieving the aux tracks (it needs the strings, so opening the whole file)
if      ( RisToVolumeTransfer.CompatRis.NumAuxTracks > 0 ) {
    ShowMessage ( "It is not allowed to have RIS tracks with auxiliary tracks!\nCheck again your input files...", RisToVolumeTitle, ShowMessageWarning );
    return;
    }


//if      ( RisToVolumeTransfer.CompatRis.SamplingFrequency == CompatibilityNotConsistent ) {
////  ShowMessage ( "Files don't seem to have the same sampling frequencies!\nCheck again your input files...", RisToVolumeTitle, ShowMessageWarning );
//    if ( ! GetAnswerFromUser ( "Files don't seem to have the same sampling frequencies!\nDo you want to proceed anyway (not recommended)?", RisToVolumeTitle ) ) {
//        return;
//        }
//    }


if      ( RisToVolumeTransfer.CompatRis.NumTF == CompatibilityIrrelevant ) {
    ShowMessage ( "Files don't seem to have any samples or time at all!\nCheck again your input files...", RisToVolumeTitle, ShowMessageWarning );
    return;
    }
//else if ( RisToVolumeTransfer.CompatRis.NumTF == CompatibilityNotConsistent ) {
//                                    // we need all files from a group to have the same length!
////  ShowMessage ( "Files don't seem to have the same number of samples/time range!\nProcessing will proceed anyway...", RisToVolumeTitle, ShowMessageWarning );
//    ShowMessage ( "Files don't seem to have the same number of samples/time range!\nCheck again your input files...", RisToVolumeTitle, ShowMessageWarning );
//    return;
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Group is coherent regarding file types? (doing this first, maybe?)
TracksGroupClass        tg;

if ( gofris.AnyTracksGroup ( tg ) ) {

    if      (   tg.noneofthese  ) { ShowMessage ( "Oops, files don't really look like tracks,\nare you trying to fool me?!\nCheck again your input files...", RisToVolumeTitle, ShowMessageWarning );   return; }
    else if ( ! tg.allris       ) { ShowMessage ( "Files don't seem to be consistently of the RIS type!\nCheck again your input files...", RisToVolumeTitle, ShowMessageWarning );                      return; }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We did it!
RisToVolumeTransfer.IsRisOK             = true;

}


//----------------------------------------------------------------------------
void    TRisToVolumeDialog::CheckRisAndSp ( TGoF& gofris ) const
{
if ( ! RisToVolumeTransfer.IsRisOK  ) {

    gofris.Reset ();

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

RisToVolumeTransfer.CheckRisAndSp  ();


                                        // in case user dropped EEG files which are incompatible, clear-up matrix?
if ( RisToVolumeTransfer.IsRisOK && ! RisToVolumeTransfer.IsRisAndSpOK ) {

    RisToVolumeTransfer.IsRisAndSpOK    = false;

    gofris.Reset ();
    }

}


//----------------------------------------------------------------------------
                                        // Testing SP file(s) by itself/themselves
void    TRisToVolumeStructEx::CheckSp ()
{
CompatSp.Reset ();
IsSpOK              = false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Make a local copy of the file name, and preemptively clearing up the dialog's one
TFileName           spfile;

StringCopy  ( spfile, SpFile );

if ( StringIsEmpty ( spfile ) )
    return;

ClearString ( SpFile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // getting number of tracks is also possible with xyz file
TGoF                spgof;

spgof.Add ( spfile );


if ( ! spgof.AllExtensionsAre ( AllSolPointsFilesExt ) ) {
    ShowMessage ( "Please provide some legal Solution Points data!", RisToVolumeTitle, ShowMessageWarning );
    return;
    }


spgof.AllInverseAreCompatible ( CompatSp );


if      ( CompatSp.NumSolPoints == CompatibilityNotConsistent ) {
    ShowMessage ( "Solution Points don't seem to have the same number of electrodes!", RisToVolumeTitle, ShowMessageWarning );
    return;
    }
else if ( CompatSp.NumSolPoints == CompatibilityIrrelevant ) {
    ShowMessage ( "Solution Points don't seem to have any coordinates at all!", RisToVolumeTitle, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here solution points are OK by themselves
IsSpOK              = true;
                                        // restore file name, now it that it has been checked
StringCopy  (  SpFile, spfile );
}


//----------------------------------------------------------------------------
                                        // Testing Grey Mask file(s) by itself/themselves
void    TRisToVolumeStructEx::CheckGreyMask ()
{
IsGreyMaskOK        = false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Make a local copy of the file name, and preemptively clearing up the dialog's one
TFileName           greymaskfile;

StringCopy  ( greymaskfile, GreyMaskFile );

if ( StringIsEmpty ( greymaskfile ) )
    return;

ClearString ( SpFile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 
TGoF                gof;

gof.Add ( greymaskfile );


if ( ! gof.AllExtensionsAre ( AllMriFilesExt ) ) {
    ShowMessage ( "Please provide some legal MRI data!", RisToVolumeTitle, ShowMessageWarning );
    return;
    }


TOpenDoc< TVolumeDoc >      mridoc ( greymaskfile, OpenDocHidden /*OpenDocVisible*/ );


if ( ! mridoc.IsOpen () ) {
    ShowMessage ( "Can not open this MRI!", RisToVolumeTitle, ShowMessageWarning );
    return;
    }


if ( ! mridoc->IsMask () ) {
    ShowMessage ( "Are you sure you really provided a Grey Mask?\nDialog will resume, but check your input again...", RisToVolumeTitle, ShowMessageWarning );
//    return;
    }


//mridoc.Close ( CloseDocLetOpen );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here grey masks are OK by themselves
IsGreyMaskOK        = true;
                                        // restore file name, now it that it has been checked
StringCopy  ( GreyMaskFile, greymaskfile );
}


//----------------------------------------------------------------------------
                                        // Testing the joint between SP and Grey Mask
void    TRisToVolumeStructEx::CheckSpAndGreyMask ()
{

IsSpAndGreyMaskOK   = false;


if ( ! ( IsSpOK && IsGreyMaskOK ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// Here could somehow check the strict / reasonable inclusion of the SPs into the Grey Mask


IsSpAndGreyMaskOK   = true;
}


//----------------------------------------------------------------------------
                                        // Testing the joint between RIS and SP
void    TRisToVolumeStructEx::CheckRisAndSp ()
{

IsRisAndSpOK        = false;


if ( ! ( IsRisOK && IsSpOK ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( CompatRis.NumSolPoints != CompatSp.NumSolPoints ) {
    ShowMessage ( "RIS file(s) doesn't seem to have the same number of Solution Points as the Solution Points file!", RisToVolumeTitle, ShowMessageWarning );
    return;
    }


IsRisAndSpOK        = true;
}


//----------------------------------------------------------------------------
void    TRisToVolumeDialog::CmTimeIntervalEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( CheckToBool ( RisToVolumeTransfer.TimeInterval ) );
}


//----------------------------------------------------------------------------
bool    TRisToVolumeDialog::CmProcessEnable ()
{
TransferData ( tdGetData );


if ( ! ( SPDoc.IsOpen () && GreyMaskDoc.IsOpen () ) )

    return  false;


int                 timemin         = StringToInteger ( RisToVolumeTransfer.TimeMin );
int                 timemax         = StringToInteger ( RisToVolumeTransfer.TimeMax );
int                 steptf          = StringToInteger ( RisToVolumeTransfer.StepTF  );

if ( CheckToBool ( RisToVolumeTransfer.TimeInterval )
  && ! ( IsInsideLimits ( timemin, timemax, (int) 0, (int) INT_MAX )
      && steptf  >= 1 ) )

    return  false;


return  true;
}


void    TRisToVolumeDialog::CmProcessCurrentEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

if ( BatchProcessing ) {
    tce.Enable ( false );
    return;
    }

tce.Enable ( CmProcessEnable () );
}


void    TRisToVolumeDialog::CmProcessBatchEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

if ( ! BatchProcessing ) {
    tce.Enable ( false );
    return;
    }

tce.Enable ( CmProcessEnable () );
}


//----------------------------------------------------------------------------
void    TRisToVolumeDialog::ProcessCurrent ( void* /*usetransfer*/, const char* /*moreinfix*/ )
{
if ( ! ( EEGDoc && SPDoc.IsOpen () && GreyMaskDoc.IsOpen () ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Create gauge before the interpolation, so that we can see all progress bars opening in the right sequence
TSuperGauge         Gauge;

Gauge.Set           ( RisToVolumeTitle, SuperGaugeLevelInter );
                                        // we don't know how many TF to actually save - but we give it 100% of progress bar
Gauge.AddPart       ( 0, 100 );

Gauge.CurrentPart   = 0;


if ( IsBatchFirstCall () && (int) BatchFileNames > 10 )
                                        // batch can be long, hide Cartool until we are done
    WindowMinimize ( CartoolMainWindow );


CartoolApplication->SetMainTitle ( RisToVolumeTitle, EEGDoc->GetDocPath (), Gauge );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Volume parameters
VolumeInterpolationPreset   interpol    = (VolumeInterpolationPreset) RisToVolumeTransfer.InterpolationPresets.GetSelIndex ();

                                        // Some volume interpolation needs the SP interpolation
SPInterpolationType         spinterpol  = interpol == VolumeInterpolation1NN        ?   SPInterpolation1NN
                                        : interpol == VolumeInterpolation4NN        ?   SPInterpolation4NN
                                        :                                               SPInterpolationNone;

                                        // Initialize the requested SP interpolation
if ( ! SPDoc->BuildInterpolation ( spinterpol, GreyMaskDoc ) ) {

    ShowMessage ( "Error while trying to compute the Solution Points Interpolation", RisToVolumeTitle, ShowMessageWarning );
    return;
    }
    

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Time parameters
bool                timeall         = CheckToBool ( RisToVolumeTransfer.TimeAll );

int                 timemin         = StringToInteger ( RisToVolumeTransfer.TimeMin );
int                 timemax         = StringToInteger ( RisToVolumeTransfer.TimeMax );
int                 lasttimeframes  = EEGDoc->GetNumTimeFrames () - 1;
int                 steptf          = AtLeast ( 1, StringToInteger ( RisToVolumeTransfer.StepTF ) );

                                        // make sure the given limits are within current time range
Clipped ( timemin, timemax, 0, lasttimeframes );

                                        // extract an appropriate interval from user wishes
int                 fromtf          = timeall   ?   0               : timemin;
int                 totf            = timeall   ?   lasttimeframes  : timemax;
                    steptf          = timeall   ?   1               : steptf;

                                        // special case where step is actually bigger than interval provided
if ( steptf > totf - fromtf + 1 )
                                        // the adjust the step from this interval
    steptf  = totf - fromtf + 1;

else
                                        // step is smaller than interval - truncate the interval to have exact number of steps
    totf    = fromtf + TruncateTo ( totf - fromtf + 1, steptf ) - 1;


FilterTypes         merging         = FilterTypeMedian;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Output parameters

AtomFormatType      atomformat      = CheckToBool ( RisToVolumeTransfer.TypeUnsignedByte )  ?   AtomFormatByte
                                    : CheckToBool ( RisToVolumeTransfer.TypeFloat        )  ?   AtomFormatFloat
                                    :                                                           RisToVolumeDefaultAtomFormat;

VolumeFileType      filetype        = (VolumeFileType) RisToVolumeTransfer.FileTypes.GetSelIndex ();

char                fileext[ 32 ];

if      ( filetype == VolumeNifti2          )   StringCopy  ( fileext, FILEEXT_MRINII       );
else if ( filetype == VolumeAnalyze         )   StringCopy  ( fileext, FILEEXT_MRIAVW_HDR   );
//elseif( filetype == VolumeField           )   StringCopy  ( fileext, FILEEXT_MRIAVS       );
//elseif( filetype == VolumeBrainVoyager    )   StringCopy  ( fileext, FILEEXT_MRIVMR       );
else                                            StringCopy  ( fileext, DefaultMriExt        );

bool                openauto        = CheckToBool ( RisToVolumeTransfer.OpenAuto );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           BaseDir;
TFileName           BaseFileName;
TFileName           fileprefix;
TFileName           VerboseFile;
TFileName           CentroidsFile;
//char                buff[ 256 ];

                                        // use first file directory as the base directory
StringCopy      ( BaseDir,                  EEGDoc->GetDocPath () );
RemoveFilename  ( BaseDir );


StringCopy      ( fileprefix, RisToVolumeTransfer.BaseFileName );

if ( StringIsNotEmpty ( fileprefix ) )
    StringAppend ( fileprefix, "." );


StringCopy      ( BaseFileName,             BaseDir,                "\\",               fileprefix );

StringCopy      ( VerboseFile,              BaseFileName,           "RIS To Volume",    "." FILEEXT_VRB );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TVerboseFile    verbose ( VerboseFile, VerboseFileDefaultWidth );

verbose.PutTitle ( "Results of Inverse Solution To Volumes" );



verbose.NextTopic ( "Input Files:" );
{
verbose.Put ( "Solution Points file:", SPDoc->GetDocPath () );
verbose.Put ( "MRI Grey Mask file:", GreyMaskDoc->GetDocPath () );
verbose.Put ( "RIS file:", EEGDoc->GetDocPath () );
}


verbose.NextTopic ( "Volume Parameters:" );
{
verbose.Put ( "Using interpolation:", VolumeInterpolationPresetString[ interpol ] );
//verbose.Put ( "Using interpolation:", SPInterpolationTypeNames[ spinterpol ] );
}


verbose.NextTopic ( "Time Parameters:" );
{
verbose.Put ( "Saving time range:", timeall ? "All Data" : "Time Interval" );
verbose.Put ( "From     [TF]:", fromtf );
verbose.Put ( "To       [TF]:", totf );
verbose.Put ( "By steps [TF]:", steptf );
verbose.Put ( "Averaging time intervals with:", FilterPresets[ merging ].Ext /*Text*/ );
}


verbose.NextTopic ( "Output Files:" );
{
verbose.Put ( "Output data type:", AtomFormatTypePresets[ atomformat ].Text );
verbose.Put ( "Rescaling output data:", atomformat == AtomFormatByte ? "Global max data to 255" : "None, writing original values" );
verbose.Put ( "Output format:", VolumeFileTypeString[ filetype ] );

verbose.NextLine ();
verbose.Put ( "Verbose file (this):", VerboseFile );
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                                        // Changing priority
SetProcessPriority ( BatchProcessingPriority );

TGoF                volgof;


RisToVolume (   EEGDoc->GetDocPath (),
                SPDoc,                  interpol, 
                GreyMaskDoc, 
                fromtf,                 totf,           steptf,
                merging,
                atomformat,             
                fileprefix,             fileext,
                volgof,
                &Gauge
            );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // complimentary opening the file for the user
if ( openauto && ( ! BatchProcessing || (int) BatchFileNames <= 10 ) )
    volgof.OpenFiles ();

                                        // final verbose output files
{
verbose.NextLine ();


verbose.Put ( "Number of volumes files:", volgof.NumFiles () );

for ( int fi = 0; fi < volgof.NumFiles (); fi++ )
    verbose.Put ( "", volgof[ fi ] );


verbose.NextLine ();
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SetProcessPriority ();

UpdateApplication;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( IsBatchLastCall () ) {

    WindowMaximize ( CartoolMainWindow );

    Gauge.FinishParts ();
    CartoolApplication->SetMainTitle ( RisToVolumeTitle, fileprefix, Gauge );

    Gauge.HappyEnd ();
    }


//if ( ! BatchProcessing )
//    Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
