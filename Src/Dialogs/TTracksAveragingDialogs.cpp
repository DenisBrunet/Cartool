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

#include    "TTracksAveragingDialogs.h"

#include    "CartoolTypes.h"            // SkippingEpochsType
#include    "TArray2.h"
#include    "Strings.Utils.h"
#include    "Strings.TFixedString.h"
#include    "Files.TGoF.h"
#include    "Files.TVerboseFile.h"
#include    "Files.SpreadSheet.h"
#include    "Files.Conversions.h"
#include    "Files.ReadFromHeader.h"
#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"

#include    "TTracksFilters.h"
#include    "TExportTracks.h"

#include    "TRisDoc.h"
#include    "TTracksView.h"
#include    "TPotentialsView.h"
#include    "TElectrodesView.h"
#include    "TLinkManyDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Global variable definition
TTracksAveragingStruct          TAvgTransfer;
TTracksAveragingControlStruct   TAvgCtrlTransfer;


const char  AvgPresets[ NumAvgPresets ][ 128 ] =
                    {
                    "Evoked Response / EEG / Surface",
                    "Evoked Response / EEG / Intra-Cranial",
                    "Evoked Response / Frequencies / Power",
                    "Evoked Response / Frequencies / FFT Approximation",
                    "Evoked Response / Inverse Solution / Scalar",
                    "",
                    "Grand Average of ERPs / EEG / Surface",
                    "Grand Average of ERPs / EEG / Intra-Cranial",
                    "Grand Average of ERPs / Frequencies / Power",
                    "Grand Average of ERPs / Frequencies / FFT Approximation",
                    "Grand Average of ERPs / Inverse Solution / Scalar",
                    };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TTracksAveragingFilesStruct::TTracksAveragingFilesStruct ()
{
UseEvents               = BoolToCheck ( false );
UseTriggers             = BoolToCheck ( true  );
UseMarkers              = BoolToCheck ( false );

ClearString ( XyzDocFile );
ClearString ( BaseFileName );
}


        TTracksAveragingParamsStruct::TTracksAveragingParamsStruct ()
{
Presets.Clear ();
for ( int i = 0; i < NumAvgPresets; i++ )
    Presets.AddString ( AvgPresets[ i ], i == AvgPresetErpEegSurface );

CheckEl                 = BoolToCheck ( true  );
StringCopy ( ElThreshold, "100" );
ClearString ( ElExcluded );

CheckAux                = BoolToCheck ( false );
StringCopy ( AuxThreshold, "100" );
ClearString ( AuxOverride );

TriggerFixedTF          = BoolToCheck ( false );
TriggerOffset           = BoolToCheck ( false );
TriggerReactionTime     = BoolToCheck ( false );
Trigger                 = BoolToCheck ( true  );
StringCopy ( TriggerOffsetValue,    "0" );
StringCopy ( TriggerOffsetValueMs,  "0 ms" );
StringCopy ( TriggerFixedTFValue,   "0" );
StringCopy ( TriggerFixedTFValueMs, "0 ms" );

StringCopy ( DurationPre,     "0" );
StringCopy ( DurationPreMs,   "0 ms" );
StringCopy ( DurationPost,    "0" );
StringCopy ( DurationPostMs,  "0 ms" );
StringCopy ( DurationTotal,   "0" );
StringCopy ( DurationTotalMs, "0 ms" );

PositiveData            = BoolToCheck ( false );
SignedData              = BoolToCheck ( true  );
NoRef                   = BoolToCheck ( true  );
AvgRef                  = BoolToCheck ( false );
AccountPolarity         = BoolToCheck ( true  );
IgnorePolarity          = BoolToCheck ( false );

SetFilters              = BoolToCheck ( false );
FilterData              = BoolToCheck ( false );
FilterDisplay           = BoolToCheck ( false );

BaselineCorr            = BoolToCheck ( false );
StringCopy ( BaselineCorrPre,    "0" );
StringCopy ( BaselineCorrPreMs,  "0 ms" );
StringCopy ( BaselineCorrPost,   "0" );
StringCopy ( BaselineCorrPostMs, "0 ms" );

SkipBadEpochs       = BoolToCheck ( false );
ClearString ( SkipMarkers );

AcceptAll               = BoolToCheck ( false );
}


        TTracksAveragingOutputsStruct::TTracksAveragingOutputsStruct ()
{
MergeTriggers           = BoolToCheck ( true  );
SplitTriggers           = BoolToCheck ( false );
SaveAverage             = BoolToCheck ( true  );
SaveSum                 = BoolToCheck ( false );
SaveSD                  = BoolToCheck ( false );
SaveSE                  = BoolToCheck ( false );
SaveEpochsMultiFiles    = BoolToCheck ( false );
SaveEpochsSingleFile    = BoolToCheck ( false );

FileTypes.Clear ();
for ( int i = 0; i < NumSavingEegFileTypes; i++ )
    FileTypes.AddString ( SavingEegFileTypePreset[ i ], i == 0 );
FileTypes.Select ( PresetFileTypeDefaultEEG );

SaveExcluded            = BoolToCheck ( true  );
SaveAux                 = BoolToCheck ( false );

OpenAuto                = BoolToCheck ( true  );
OriginMarker            = BoolToCheck ( true  );
}


        TTracksAveragingStruct::TTracksAveragingStruct ()
{
LastDialogId            = IDD_AVERAGE1;
AllFilesOk              = true;
SamplingFrequency       = 0;
MaxNumTF                = 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

DEFINE_RESPONSE_TABLE1 (TTracksAveragingFilesDialog, TTracksAveragingBaseDialog)
    EV_WM_DROPFILES,
    EV_COMMAND                  ( IDC_ADD2LIST,             CmAddToList ),
    EV_COMMAND                  ( IDC_REMFROMLIST,          CmRemoveFromList ),
    EV_COMMAND                  ( IDC_REPLFIELD,            CmReplaceField ),
    EV_COMMAND_ENABLE           ( IDC_REPLFIELD,            CmReplaceFieldEnable ),
    EV_COMMAND                  ( IDC_BROWSEEEG,            CmBrowseEeg ),
    EV_COMMAND                  ( IDC_BROWSETVA,            CmBrowseTva ),
    EV_COMMAND                  ( IDC_BROWSEXYZ,            CmBrowseXyz ),
    EV_COMMAND                  ( IDC_READPARAMS,           CmReadParams ),
    EV_COMMAND                  ( IDC_WRITEPARAMS,          CmWriteParams ),
    EV_COMMAND                  ( IDC_CLEARLISTS,           CmClearLists ),
    EV_COMMAND                  ( IDC_SORTLISTS,            CmSortLists ),
    EV_CBN_SELCHANGE            ( IDC_CBEEG,                EvCbnSelChange ),
    EV_CBN_SELCHANGE            ( IDC_CBTVA,                EvCbnSelChange ),
    EV_CBN_SELCHANGE            ( IDC_CBTRG,                EvCbnSelChange ),
    EV_CBN_SELCHANGE            ( IDC_CBSESSION,            EvCbnSelChange ),
    EV_CBN_SETFOCUS             ( IDC_CBEEG,                EvCbnSetFocus ),
    EV_CBN_SETFOCUS             ( IDC_CBTVA,                EvCbnSetFocus ),
    EV_CBN_SETFOCUS             ( IDC_CBTRG,                EvCbnSetFocus ),
    EV_CBN_SETFOCUS             ( IDC_CBSESSION,            EvCbnSetFocus ),
//  EV_EN_KILLFOCUS             ( IDC_EXYZFILE,             EvEnKillFocus ),
    EV_EN_CHANGE                ( IDC_EXYZFILE,             EvXyzFileChange ),
//  EV_COMMAND_ENABLE           ( IDC_TONEXTDIALOG,         CmToNextDialogEnable ),
    EV_COMMAND                  ( IDC_UPDIRECTORY,          CmUpOneDirectory ),
END_RESPONSE_TABLE;


        TTracksAveragingFilesDialog::TTracksAveragingFilesDialog ( TWindow* parent, int resId )
      : TTracksAveragingBaseDialog ( parent, resId )
{
StringCopy ( BatchFilesExt, AllTracksFilesExt " " AllDataExt );


ComboEeg        = new TComboBox ( this, IDC_CBEEG );
ComboSes        = new TComboBox ( this, IDC_CBSESSION );
ComboTva        = new TComboBox ( this, IDC_CBTVA );
ComboTrg        = new TComboBox ( this, IDC_CBTRG );

UseEvents       = new TCheckBox ( this, CM_EEGMRKEVENT );
UseTriggers     = new TCheckBox ( this, CM_EEGMRKTRIGGER );
UseMarkers      = new TCheckBox ( this, CM_EEGMRKMARKER );

XyzDocFile      = new TEdit     ( this, IDC_EXYZFILE, EditSizeText );
BaseFileName    = new TEdit     ( this, IDC_OUTFILEE, EditSizeText );


SetTransferBuffer ( dynamic_cast <TTracksAveragingFilesStruct*> ( &TAvgTransfer ) );


for ( int i = 0; i < 4; i++ )
    SelIndex[ i ] = -1;

ComboWithFocus = 0;
}


        TTracksAveragingFilesDialog::~TTracksAveragingFilesDialog ()
{
delete  ComboEeg;               delete  ComboSes;           delete  ComboTva;           delete  ComboTrg;
delete  UseEvents;              delete  UseTriggers;        delete  UseMarkers;
delete  XyzDocFile;             delete  BaseFileName;
}


//----------------------------------------------------------------------------
/*void    TTracksAveragingFilesDialog::CmToNextDialogEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

                                        // directly playing with the buffer does not seem to be a good idea, maybe its updated during the test?
TFileName           buff;
StringCopy ( buff, TAvgTransfer.BaseFileName );


tce.Enable ( ComboEeg->GetCount () > 0
          && ( CheckToBool ( UseEvents->GetCheck () ) || CheckToBool ( UseTriggers->GetCheck() ) || CheckToBool ( UseMarkers->GetCheck() ) )
          && IsAbsoluteFilename ( buff )
          && TAvgTransfer.AllFilesOk );
}
*/

//----------------------------------------------------------------------------
void    TTracksAveragingFilesDialog::EvDropFiles ( TDropInfo drop )
{
TGoF                tracksfiles     ( drop, BatchFilesExt           );
TGoF                tvafiles        ( drop, FILEEXT_TVA             );
TGoF                xyzfiles        ( drop, AllCoordinatesFilesExt  );
TGoF                spreadsheetfiles( drop, SpreadSheetFilesExt     );

char                buff[ 256 ];
StringCopy ( buff, BatchFilesExt, " " FILEEXT_TVA " " AllCoordinatesFilesExt " " SpreadSheetFilesExt );
TGoF                remainingfiles  ( drop, buff, 0, true );

drop.DragFinish ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) xyzfiles; i++ ) {

    XyzDocFile->SetText ( xyzfiles[ i ] );

    if ( ! CheckFilesCompatibility ( Interactive ) )
        XyzDocFile->SetText ( "" );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) spreadsheetfiles; i++ )
    ReadParams ( spreadsheetfiles[ i ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // try to insert eeg and tva at the same time
if ( (bool) tracksfiles && (int) tracksfiles == (int) tvafiles ) {

    for ( int i = 0; i < (int) tracksfiles; i++ ) {

        ComboEeg->Clear ();
        ComboEeg->SetText ( tracksfiles[ i ] );

        ComboTva->Clear ();
        ComboTva->SetText ( tvafiles   [ i ] );

        CmAddToList ();

        ComboEeg->SetEditSel ( 0, 0 );
        ComboTva->SetEditSel ( 0, 0 );
        }
    }

else if ( (bool) tracksfiles && ! (bool) tvafiles ) {

    for ( int i = 0; i < (int) tracksfiles; i++ ) {

        ComboEeg->Clear ();
        ComboEeg->SetText ( tracksfiles[ i ] );

        CmAddToList ();

        ComboEeg->SetEditSel ( 0, 0 );
        }
    }

else if ( (bool) tvafiles )

    if ( ComboEeg->GetCount () ) {

//      ShowMessage (   "Please provide as many TVA files as of EEG / Tracks files!" NewLine 
//                      "TVA and EEG Files ignored.", 
//                      "Averaging Files", ShowMessageWarning );

                                            // loop is not really used, only the last dropped will remain
        for ( int i = 0; i < (int) tvafiles; i++ ) {
                                            // overwrite over the last entry
            ComboTva->DeleteString ( 0 );
            ComboTva->InsertString ( (char *) tvafiles[ i ], 0 );
            ComboTva->SetEditSel   ( 0, 0 );
            }
        }
    else
        ShowMessage (   "Dropping a single TVA file will associate it with the last EEG entered," NewLine 
                        "but for now, it seems your EEG list looks quite empty...", 
                        "Averaging Files", ShowMessageWarning );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) remainingfiles )
    remainingfiles.Show ( IrrelevantErrorMessage );
}


//----------------------------------------------------------------------------
bool    TTracksAveragingFilesDialog::CheckFilesCompatibility ( ExecFlags execflags )
{
TAvgTransfer.AllFilesOk = true;


TFileName           buff;
XyzDocFile->GetText ( buff, EditSizeText );

bool                somexyz = StringIsNotEmpty ( buff );
bool                someeeg = ComboEeg->GetCount ();

                                        // nothing to test?
if ( ! somexyz && ! someeeg )
    return true;


int                 numelxyz;
int                 numeleeg1;
int                 numeleeg;
int                 numtf;
int                 maxtf           = INT_MAX;
//int               numauxeleeg;
double              dsf1;
double              dsf;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( somexyz && ! ReadFromHeader ( buff, ReadNumElectrodes, &numelxyz ) ) {
    if ( IsInteractive ( execflags ) ) {
        sprintf ( buff, "Please check the provided coordinates file!" );
        ShowMessage ( buff, "Averaging Files", ShowMessageWarning );
        }
    TAvgTransfer.AllFilesOk = false;
    return false;
    }


if ( ! someeeg )
    return true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < ComboEeg->GetCount (); i++ ) {

    ComboEeg->GetString ( buff, i );

                                        // well, not all eeg have this implemented, so put it out of test
    if ( ! ReadFromHeader ( buff, ReadNumTimeFrames,     &numtf    ) )
        numtf   = INT_MAX;


    if ( ! ReadFromHeader ( buff, ReadNumElectrodes,     &numeleeg )
      || ! ReadFromHeader ( buff, ReadSamplingFrequency, &dsf      ) ) {

        if ( IsInteractive ( execflags ) ) {
            sprintf ( buff, "Problem while reading EEG file!" );
            ShowMessage ( buff, "Averaging Files", ShowMessageWarning );
            }

        TAvgTransfer.AllFilesOk = false;

        return false;
        }

                                        // test eegs among themselves
    if ( i == 0 ) {
        numeleeg1   = numeleeg;
        dsf1        = dsf;
        }

    else if ( numeleeg1 != numeleeg ) {

        if ( IsInteractive ( execflags ) ) {

            sprintf     ( buff, "Number of electrodes does not match:" NewLine
                                Tab "EEG : %d" NewLine 
                                Tab "EEG : %d", 
                                numeleeg1, numeleeg );
            ShowMessage ( buff, "Averaging Files", ShowMessageWarning );
            }

        TAvgTransfer.AllFilesOk = false;

        return false;
        }
                                        // We choose to assume that a missing SF will be considered the same as the one we get
    else if ( dsf != 0 && dsf1 != 0 && fabs ( dsf1 - dsf ) / dsf > 1e-6 ) {

        if ( IsInteractive ( execflags ) ) {

            sprintf     ( buff, "Sampling frequencies do not match:" NewLine 
                                Tab "%4g Hz" NewLine 
                                Tab "%4g Hz", 
                                dsf1, dsf );
            ShowMessage ( buff, "Averaging Files", ShowMessageWarning );
            }

        TAvgTransfer.AllFilesOk = false;

        return false;
        }

                                        // catch the first valid frequency
    if ( dsf1 == 0 && dsf != 0 )
        dsf1 = dsf;

                                        // get the maximum possible numtf (that is, the min of all)
    maxtf   = min ( maxtf, numtf );


//  if ( ! ReadFromHeader ( buff, ReadNumAuxElectrodes, &numauxeleeg ) )  return;
//  if ( numelxyz != numeleeg - numauxeleeg ) {

                                        // test eeg and xyz
    if ( somexyz && numelxyz != numeleeg ) {

        if ( IsInteractive ( execflags ) ) {

//          sprintf     ( buff, "XYZ (%d electrodes) and EEG file (%d electrodes) do not match together.", numelxyz, numeleeg-numauxeleeg );
            sprintf     ( buff, "Number of electrodes does not match:" NewLine 
                                Tab "XYZ : %d" NewLine 
                                Tab "EEG : %d", 
                                numelxyz, numeleeg );
            ShowMessage ( buff, "Averaging Files", ShowMessageWarning );
            }

        TAvgTransfer.AllFilesOk = false;

        return false;
        }
    }


TAvgTransfer.SamplingFrequency  = dsf1;
TAvgTransfer.MaxNumTF           = maxtf < 0 || maxtf == INT_MAX ? 0 : maxtf;


return true;
}


//----------------------------------------------------------------------------
void    TTracksAveragingFilesDialog::CmAddToList ()
{
            // error if no file or no trigger !
//if ( ComboEeg->GetTextLen() == 0 || ComboTrg->GetTextLen() == 0 ) {
//    ShowMessage ( "You must provide at least:" NewLine "  an eeg file," NewLine "  a trigger code.", "Add file", ShowMessageWarning );

if ( ComboEeg->GetTextLen() == 0 ) {
    ShowMessage ( "You must provide a file in the edit field.", "Add file", ShowMessageWarning );
    return;
    }

char                mess    [ EditSizeText ];
char                fneeg   [ EditSizeText ];
char                session [ EditSizeValue ];
char                fntva   [ EditSizeText ];
char                trg     [ EditSizeText ];

                                        // transfer from edit to list
                                        // check that all files exist, first !
ComboEeg->GetText ( fneeg,   EditSizeText );
ComboSes->GetText ( session, EditSizeValue );
ComboTva->GetText ( fntva,   EditSizeText );
ComboTrg->GetText ( trg,     EditSizeText );


if ( ! IsExtensionAmong ( fneeg, AllTracksFilesExt " " AllDataExt ) ) {
    sprintf ( mess, "Wrong file extension: %s", StringContains ( fneeg, '.', StringContainsBackward ) );
    ShowMessage ( mess, "File", ShowMessageWarning );
    return;
    }


if ( ! CanOpenFile ( fneeg, CanOpenFileRead ) ) {
    sprintf     ( mess, "Can not find the file:" NewLine 
                        NewLine 
                        "%s" NewLine, 
                        fneeg );
    ShowMessage ( mess, "Input File", ShowMessageWarning );
    return;
    }


if ( StringIsNotEmpty ( fntva ) ) {
    if ( ! IsExtension ( fntva, FILEEXT_TVA ) ) {
        sprintf     ( mess, "Wrong file extension: %s", fntva );
        ShowMessage ( mess, "TVA File", ShowMessageWarning );
        return;
        }

    if ( ! CanOpenFile ( fntva, CanOpenFileRead ) ) {
        sprintf     ( mess, "Can not find the file:" NewLine 
                            NewLine 
                            "%s" NewLine, 
                            fneeg );
        ShowMessage ( mess, "Input File", ShowMessageWarning );
        return;
        }
    }


                                        // supply default values
if ( *trg == 0 )
    StringCopy ( trg, "*" );

//if ( *session == 0 )
//    StringCopy ( session, "*" );

                                        // now transfer to the lists
ComboEeg->InsertString ( fneeg,   0 );
ComboSes->InsertString ( session, 0 );
ComboTva->InsertString ( fntva,   0 );
ComboTrg->InsertString ( trg,     0 );

                                        // clear the eeg file edit
ComboEeg->Clear();
ComboTva->Clear();

ComboEeg->SetFocus();


if ( ! CheckFilesCompatibility ( Interactive ) ) {
    CmRemoveFromList ();
    TAvgTransfer.AllFilesOk = true;
    }
else {
    SetBaseFilename ();
    GuessTriggerType ();
    }
}


//----------------------------------------------------------------------------
void    TTracksAveragingFilesDialog::GuessTriggerType ()
{
TFileName       buff;
TGoF            gof;
bool            anytrigger;
bool            anymarker;


//UseEvents  ->SetCheck ( BoolToCheck ( false ) );
//UseTriggers->SetCheck ( BoolToCheck ( false ) );
//UseMarkers ->SetCheck ( BoolToCheck ( false ) );

                                        // build a temp TGoF
for ( int i = 0; i < ComboEeg->GetCount (); i++ ) {
    ComboEeg->GetString ( buff, i );
    gof.Add ( buff );
    }

                                        // so we can use these nifty functions
anytrigger  = gof.SomeExtensionsAre ( AllRawEegFilesExt );
anymarker   = gof.SomeExtensionsAre ( AllEegFreqRisFilesExt " " AllSdExt );

                                        // the less pushy, just turn on the flags
if ( anytrigger )   UseTriggers->SetCheck ( BoolToCheck ( true  ) );
if ( anymarker  )   UseMarkers ->SetCheck ( BoolToCheck ( true  ) );
}


//----------------------------------------------------------------------------
void    TTracksAveragingFilesDialog::CmClearLists ()
{
ComboEeg->ClearList ();
ComboSes->ClearList ();
ComboTva->ClearList ();
ComboTrg->ClearList ();


SetBaseFilename ();
}


void    TTracksAveragingFilesDialog::CmRemoveFromList ()
{
char        mess[EditSizeText];

if ( ComboEeg->GetString ( mess, 0 )  < 0 )
    ClearString ( mess );
ComboEeg->DeleteString ( 0 );
ComboEeg->SetText ( mess );

if ( ComboSes->GetString ( mess, 0 )  < 0 )
    ClearString ( mess );
ComboSes->DeleteString ( 0 );
ComboSes->SetText ( mess );

if ( ComboTva->GetString ( mess, 0 )  < 0 )
    ClearString ( mess );
ComboTva->DeleteString ( 0 );
ComboTva->SetText ( mess );

if ( ComboTrg->GetString ( mess, 0 )  < 0 )
    ClearString ( mess );
ComboTrg->DeleteString ( 0 );
ComboTrg->SetText ( mess );


SetBaseFilename ();
}


//----------------------------------------------------------------------------
void    TTracksAveragingFilesDialog::CmSortLists ()
{
int                 numeeg          = ComboEeg->GetCount();

if ( numeeg <= 1 )
    return;

TFileName           buffi;
TFileName           buffj;
int                 bestj;

                                        // THAT is a bad sort...
for ( int rep = 0; rep < numeeg; rep++ )

    for ( int i = 0; i < numeeg; i++ ) {

        ComboEeg->GetString ( buffi, i );

        bestj       = i;

        for ( int j = i + 1; j < numeeg; j++ ) {

            ComboEeg->GetString ( buffj, j );

            if ( _stricmp ( buffj, buffi ) > 0 )
                bestj   = j;
            }

        if ( bestj == i )
            continue;

                                        // now do the permutations, for all combos
        ComboEeg->GetString ( buffi, i );
        ComboEeg->GetString ( buffj, bestj );
        ComboEeg->DeleteString ( i );
        ComboEeg->InsertString ( buffj, i );
        ComboEeg->DeleteString ( bestj );
        ComboEeg->InsertString ( buffi, bestj );

        ComboSes->GetString ( buffi, i );
        ComboSes->GetString ( buffj, bestj );
        ComboSes->DeleteString ( i );
        ComboSes->InsertString ( buffj, i );
        ComboSes->DeleteString ( bestj );
        ComboSes->InsertString ( buffi, bestj );

        ComboTva->GetString ( buffi, i );
        ComboTva->GetString ( buffj, bestj );
        ComboTva->DeleteString ( i );
        ComboTva->InsertString ( buffj, i );
        ComboTva->DeleteString ( bestj );
        ComboTva->InsertString ( buffi, bestj );

        ComboTrg->GetString ( buffi, i );
        ComboTrg->GetString ( buffj, bestj );
        ComboTrg->DeleteString ( i );
        ComboTrg->InsertString ( buffj, i );
        ComboTrg->DeleteString ( bestj );
        ComboTrg->InsertString ( buffi, bestj );
        }

}


//----------------------------------------------------------------------------
                                        // generate a smart base name
void    TTracksAveragingFilesDialog::SetBaseFilename ()
{
if ( ComboEeg->GetCount() == 0 )
    return;

TransferData ( tdGetData );


char                buff     [ EditSizeText ];
TFileName           match;
char                triggers [ EditSizeText ];
int                 numeeg          = ComboEeg->GetCount();



TGoF                files ( numeeg );

for ( int i = 0; i < numeeg; i++ )

    ComboEeg->GetString ( files[ numeeg - i - 1 ], i );


files.GetCommonString ( match, true /*, true*/ );


if ( ! IsAbsoluteFilename ( match ) )
    return;


PrefixFilename ( match, "Avg " );


                                        // now scan all trigger names
TSplitStrings   triggerlist;
ClearString ( triggers );

//for ( int i = 0; i < ComboTrg->GetCount(); i++ ) {
for ( int i = ComboTrg->GetCount() - 1; i >= 0; i-- ) {

    ComboTrg->GetString ( buff, i );
                                        // cumulate all tokens
    triggerlist.Add ( buff, UniqueStrings );
    }

                                        // construct a compact string, also expanding any '*'
triggerlist.ToString ( triggers, CompactString );

                                        // compact triggers name
StringShrink ( triggers, triggers, 17 );
//StringShrink ( triggers, triggers, min ( (int) ( 4 + StringLength ( triggers ) / 2 ), 17 ) );

                                        // finally, compose the base name
StringCopy  ( buff, match, " ", triggers );

BaseFileName->SetText ( buff );

BaseFileName->ResetCaret;
}


void    TTracksAveragingFilesDialog::CmUpOneDirectory ()
{
RemoveLastDir ( TAvgTransfer.BaseFileName );

BaseFileName->SetText ( TAvgTransfer.BaseFileName );

BaseFileName->ResetCaret;
}


//----------------------------------------------------------------------------
void    TTracksAveragingFilesDialog::CmReadParams ()
{
ReadParams ();
}


void    TTracksAveragingFilesDialog::ReadParams ( char *filename )
{

TSpreadSheet        sf;

if ( ! sf.ReadFile ( filename ) )
    return;


CsvType             csvtype         = sf.GetType ();


if ( ! IsCsvAveraging ( csvtype ) ) {
    ShowMessage ( SpreadSheetErrorMessage, "Read list file", ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           attrfile;
char                attrsession  [ EditSizeText ];
char                attrtva      [ EditSizeText ];
char                attrtriggers [ EditSizeText ];
//char                buff [ 1024 ];


for ( int file = 0; file < sf.GetNumRecords (); file ++ ) {

    sf.GetRecord ( file, "file",     attrfile     );    // this one exists for sure
    sf.GetRecord ( file, "session",  attrsession  );
    sf.GetRecord ( file, "tva",      attrtva      );
                                                        // if trigger is empty, better to set one
    if ( ! sf.GetRecord ( file, "triggers", attrtriggers ) )
        StringCopy ( attrtriggers, "*" );

/*
    ComboEeg->Reset ();
    ComboSes->Reset ();
    ComboTva->Reset ();
    ComboTrg->Reset ();
*/
    ComboEeg->SetText ( attrfile     );
    ComboSes->SetText ( attrsession  );
    ComboTva->SetText ( attrtva      );
    ComboTrg->SetText ( attrtriggers );

    CmAddToList ();

    ComboEeg->SetEditSel ( 0, 0 );
    ComboSes->SetEditSel ( 0, 0 );
    ComboTva->SetEditSel ( 0, 0 );
    ComboTrg->SetEditSel ( 0, 0 );
    }
}


void    TTracksAveragingFilesDialog::CmWriteParams ()
{
if ( ComboEeg->GetCount () == 0 )
    return;


TSpreadSheet        sf;

if ( ! sf.InitFile () )
    return;

                                        // header line describes the attributes / fields
sf.WriteAttribute ( "file"     );
sf.WriteAttribute ( "session"  );
sf.WriteAttribute ( "tva"      );
sf.WriteAttribute ( "triggers" );
sf.WriteNextRecord ();

                                        // now write each line
char                buff [ MaxPathShort ];
int                 numeeg          = ComboEeg->GetCount();

for ( int i = numeeg - 1; i >= 0; i-- ) {

    ComboEeg->GetString ( buff, i );
    sf.WriteAttribute ( buff );

    ComboSes->GetString ( buff, i );
    sf.WriteAttribute ( buff );

    ComboTva->GetString ( buff, i );
    sf.WriteAttribute ( buff );

    ComboTrg->GetString ( buff, i );
    sf.WriteAttribute ( buff );

    sf.WriteNextRecord ();
    }


sf.WriteFinished ();
}


//----------------------------------------------------------------------------
void    TTracksAveragingFilesDialog::EvCbnSelChange ()
{
SelIndex[ 0 ] = ComboEeg->GetSelIndex ();
SelIndex[ 1 ] = ComboSes->GetSelIndex ();
SelIndex[ 2 ] = ComboTva->GetSelIndex ();
SelIndex[ 3 ] = ComboTrg->GetSelIndex ();
}


void    TTracksAveragingFilesDialog::CmReplaceFieldEnable ( TCommandEnabler &tce )
{
int         selindex;

if      ( ComboWithFocus == ComboEeg )  selindex    = SelIndex[ 0 ];
else if ( ComboWithFocus == ComboSes )  selindex    = SelIndex[ 1 ];
else if ( ComboWithFocus == ComboTva )  selindex    = SelIndex[ 2 ];
else                                    selindex    = SelIndex[ 3 ];

tce.Enable ( selindex >= 0 );
}


void    TTracksAveragingFilesDialog::EvCbnSetFocus ()
{
int     wid = CartoolApplication->GetWindowPtr(GetFocus())->GetParentO()->GetId();

if      ( wid == ComboEeg->GetId() )    ComboWithFocus = ComboEeg;
else if ( wid == ComboSes->GetId() )    ComboWithFocus = ComboSes;
else if ( wid == ComboTva->GetId() )    ComboWithFocus = ComboTva;
else                                    ComboWithFocus = ComboTrg;
}


void    TTracksAveragingFilesDialog::CmReplaceField ()
{
int         selindex;
char        buff [ EditSizeText ];
char        saved[ EditSizeText ];

if      ( ComboWithFocus == ComboEeg )      selindex    = SelIndex[ 0 ];
else if ( ComboWithFocus == ComboSes )      selindex    = SelIndex[ 1 ];
else if ( ComboWithFocus == ComboTva )      selindex    = SelIndex[ 2 ];
else                                        selindex    = SelIndex[ 3 ];

if ( selindex < 0 )
    return;


ComboWithFocus->GetText ( buff, EditSizeText );

ComboWithFocus->GetString ( saved, selindex );
ComboWithFocus->DeleteString ( selindex );
ComboWithFocus->InsertString ( buff, selindex );


if ( ! CheckFilesCompatibility ( Interactive ) ) {
    ComboWithFocus->DeleteString ( selindex );
    ComboWithFocus->InsertString ( saved, selindex );
    TAvgTransfer.AllFilesOk = true;
    }
else
    SetBaseFilename ();
}


//----------------------------------------------------------------------------
void    TTracksAveragingFilesDialog::CmBrowseXyz ()
{
static GetFileFromUser  getfile ( "Electrodes Coordinates File", AllCoordinatesFilesFilter, 1, GetFileRead );


if ( ! getfile.Execute ( TAvgTransfer.XyzDocFile ) )
    return;

TransferData ( tdSetData );


if ( ! CheckFilesCompatibility ( Interactive ) ) {
    ClearString ( TAvgTransfer.XyzDocFile );
    TransferData ( tdSetData );
    return;
    }

TransferData ( tdSetData );             // this will activate CmXyzChange (but not SetText ( .. ) )

XyzDocFile->ResetCaret;
}


void    TTracksAveragingFilesDialog::CmBrowseTva ()
{
static GetFileFromUser  getfile ( "Triggers Validation File", AllTvaFilesFilter, 1, GetFileRead );

                                        // retrieve the Edit field
TFileName           file;
ComboTva->GetText ( file, file.Size () );


if ( ! getfile.Execute ( file ) )
    return;


TransferData ( tdSetData );             // this will activate CmXyzChange (but not SetText ( .. ) )


ComboTva->SetText ( getfile );
ComboTva->SetEditSel ( 10000, 10000 );
}


void    TTracksAveragingFilesDialog::CmBrowseEeg ()
{
static GetFileFromUser  getfile ( "Averaging Files", AllErpEegFreqRisFilesFilter "|" AllOtherTracksFilter, 1, GetFileMulti );


if ( ! getfile.Execute () )
    return;


if ( (int) getfile > 1 ) {              // more than 1 file -> accept all

    for ( int i = 0; i < (int) getfile; i++ ) {

        ComboEeg->Clear ();
        ComboEeg->SetText ( getfile[ i ] );

        CmAddToList ();

        ComboEeg->SetEditSel ( 10000, 10000 );
        }

    ComboEeg->SetFocus ();
    }
else {                                  // only one -> just put in edit field
    ComboEeg->SetText ( getfile[ 0 ] );
    ComboEeg->SetEditSel ( 0, 0 );

    ComboTrg->SetFocus ();
    }
}

/*
void    TTracksAveragingFilesDialog::EvEnKillFocus ()
{
if ( ! CheckFilesCompatibility ( Interactive ) )
    XyzDocFile->SetFocus ();
}
*/

void    TTracksAveragingFilesDialog::EvXyzFileChange ()
{
CheckFilesCompatibility ( Silent );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

DEFINE_RESPONSE_TABLE1 (TTracksAveragingParamsDialog, TTracksAveragingBaseDialog)
    EV_WM_DROPFILES,

    EV_CBN_SELCHANGE            ( IDC_PRESETS,              EvPresetsChange ),

    EV_BN_CLICKED	            ( IDC_SETFILTERS,           EvFiltersChanged ),
    EV_COMMAND_ENABLE           ( IDC_SETFILTERS,           CmSetFiltersEnable ),
    EV_COMMAND_ENABLE           ( IDC_FILTERDATA,           CmFiltersEnable ),
    EV_COMMAND_ENABLE           ( IDC_FILTERDISPLAY,        CmFiltersEnable ),

    EV_COMMAND_ENABLE           ( IDC_NOREF,                CmAvgRefEnable ),
    EV_COMMAND_ENABLE           ( IDC_AVEREF,               CmAvgRefEnable ),

    EV_COMMAND_ENABLE           ( IDC_SOE_OFFSET,           CmTriggerOffsetEnable ),
    EV_COMMAND_ENABLE           ( IDC_SOE_OFFSET_MS,        CmTriggerOffsetEnable ),
    EV_COMMAND_ENABLE           ( IDC_SOE_1TF,              CmSO1TFEnable ),
    EV_COMMAND_ENABLE           ( IDC_SOE_1TF_MS,           CmSO1TFEnable ),

    EV_COMMAND_ENABLE           ( IDC_BASELINECORR,         CmBaseLineCorrectionEnable ),
    EV_COMMAND_ENABLE           ( IDC_BLCE_PRE,             CmBaseLineCorrectionParamsEnable ),
    EV_COMMAND_ENABLE           ( IDC_BLCE_POST,            CmBaseLineCorrectionParamsEnable ),
    EV_COMMAND_ENABLE           ( IDC_BLCE_PRE_MS,          CmBaseLineCorrectionParamsEnable ),
    EV_COMMAND_ENABLE           ( IDC_BLCE_POST_MS,         CmBaseLineCorrectionParamsEnable ),

    EV_COMMAND_ENABLE           ( IDC_SKIPMARKERS,          CmSkipBadEpochsListEnable ),

    EV_COMMAND_ENABLE           ( IDC_CHKE_AMPL,            CmCheckElEnable ),
//  EV_COMMAND_ENABLE           ( IDC_CHKE_EXCL,            CmCheckElEnable ),
    EV_COMMAND_ENABLE           ( IDC_AUXTHRESHOLD,         CmCheckAuxEnable ),
//  EV_COMMAND_ENABLE           ( IDC_AUXOVERRIDE,          CmCheckAuxEnable ),
    EV_EN_CHANGE                ( IDC_DE_PRE,               EvEditPrePostChanged ),
    EV_EN_CHANGE                ( IDC_DE_POST,              EvEditPrePostChanged ),
    EV_EN_CHANGE                ( IDC_SOE_OFFSET,           EvTFChanged ),
    EV_EN_CHANGE                ( IDC_SOE_1TF,              EvTFChanged ),
    EV_EN_CHANGE                ( IDC_BLCE_PRE,             EvTFChanged ),
    EV_EN_CHANGE                ( IDC_BLCE_POST,            EvTFChanged ),
    EV_BN_CLICKED               ( IDC_READPARAMS,           CmReadParams ),
//  EV_COMMAND_ENABLE           ( IDC_TONEXTDIALOG,         CmToNextDialogEnable ),
END_RESPONSE_TABLE;


        TTracksAveragingParamsDialog::TTracksAveragingParamsDialog ( TWindow* parent, int resId )
      : TTracksAveragingBaseDialog ( parent, resId )

{
StringCopy ( BatchFilesExt, FILEEXT_VRB );


Presets                 = new TComboBox ( this, IDC_PRESETS );

CheckEl                 = new TCheckBox ( this, IDC_CHECKEL );
ElThreshold             = new TEdit     ( this, IDC_CHKE_AMPL, EditSizeValue );
ElExcluded              = new TEdit     ( this, IDC_CHKE_EXCL, EditSizeText );
ElThreshold->SetValidator ( new TFilterValidator ( ValidatorSignedFloat ) );

CheckAux                = new TCheckBox ( this, IDC_CHECKAUX );
AuxThreshold            = new TEdit     ( this, IDC_AUXTHRESHOLD, EditSizeValue );
AuxOverride             = new TEdit     ( this, IDC_AUXOVERRIDE,  EditSizeText );
AuxThreshold->SetValidator ( new TFilterValidator ( ValidatorSignedFloat ) );

TriggerFixedTF          = new TRadioButton ( this, IDC_SOB_1TF );
Trigger                 = new TRadioButton ( this, IDC_SOB_TRIGGER );
TriggerOffset           = new TRadioButton ( this, IDC_SOB_OFFSET );
TriggerReactionTime     = new TRadioButton ( this, IDC_SOB_RTIME );
TriggerOffsetValue      = new TEdit     ( this, IDC_SOE_OFFSET, EditSizeValue );
TriggerOffsetValueMs    = new TEdit     ( this, IDC_SOE_OFFSET_MS, EditSizeValue );
TriggerFixedTFValue     = new TEdit     ( this, IDC_SOE_1TF, EditSizeValue );
TriggerFixedTFValueMs   = new TEdit     ( this, IDC_SOE_1TF_MS, EditSizeValue );

TriggerOffsetValue ->SetValidator ( new TFilterValidator ( ValidatorSignedInteger ) );
TriggerFixedTFValue->SetValidator ( new TFilterValidator ( ValidatorPositiveInteger ) );

DurationPre             = new TEdit     ( this, IDC_DE_PRE, EditSizeValue );
DurationPreMs           = new TEdit     ( this, IDC_DE_PRE_MS, EditSizeValue );
DurationPost            = new TEdit     ( this, IDC_DE_POST, EditSizeValue );
DurationPostMs          = new TEdit     ( this, IDC_DE_POST_MS, EditSizeValue );
DurationTotal           = new TEdit     ( this, IDC_DE_TOTAL, EditSizeValue );
DurationTotalMs         = new TEdit     ( this, IDC_DE_TOTAL_MS, EditSizeValue );

SignedData              = new TRadioButton ( this, IDC_SIGNEDDATA );
PositiveData            = new TRadioButton ( this, IDC_POSITIVEDATA );
NoRef                   = new TRadioButton ( this, IDC_NOREF );
AvgRef                  = new TRadioButton ( this, IDC_AVEREF );
AccountPolarity         = new TRadioButton ( this, IDC_ACCOUNTPOLARITY );
IgnorePolarity          = new TRadioButton ( this, IDC_IGNOREPOLARITY );

SetFilters              = new TCheckBox ( this, IDC_SETFILTERS );
FilterData              = new TCheckBox ( this, IDC_FILTERDATA );
FilterDisplay           = new TCheckBox ( this, IDC_FILTERDISPLAY );

BaselineCorr            = new TCheckBox ( this, IDC_BASELINECORR );
BaselineCorrPre         = new TEdit     ( this, IDC_BLCE_PRE, EditSizeValue );
BaselineCorrPreMs       = new TEdit     ( this, IDC_BLCE_PRE_MS, EditSizeValue );
BaselineCorrPost        = new TEdit     ( this, IDC_BLCE_POST, EditSizeValue );
BaselineCorrPostMs      = new TEdit     ( this, IDC_BLCE_POST_MS, EditSizeValue );

BaselineCorrPre ->SetValidator ( new TFilterValidator ( ValidatorSignedInteger ) );
BaselineCorrPost->SetValidator ( new TFilterValidator ( ValidatorSignedInteger ) );

SkipBadEpochs       = new TCheckBox     ( this, IDC_SKIPBADEPOCHS );
SkipMarkers         = new TEdit         ( this, IDC_SKIPMARKERS, EditSizeText );

AcceptAll               = new TCheckBox ( this, IDC_ACCEPTALL );


SetTransferBuffer ( dynamic_cast <TTracksAveragingParamsStruct*> ( &TAvgTransfer ) );


//if ( TAvgTransfer.MaxNumTF > 0 ) {
//
//    int                 durpre          = StringToInteger ( TAvgTransfer.DurationPre  );
//    int                 durpost         = StringToInteger ( TAvgTransfer.DurationPost );
//
//    Clipped ( durpre,  0, TAvgTransfer.MaxNumTF );
//    Clipped ( durpost, 0, TAvgTransfer.MaxNumTF );
//
//    IntegerToString ( TAvgTransfer.DurationPre,  durpre  );
//    IntegerToString ( TAvgTransfer.DurationPost, durpost );
//    }
}


        TTracksAveragingParamsDialog::~TTracksAveragingParamsDialog ()
{
delete  Presets;
delete  CheckEl;                delete  ElThreshold;            delete  ElExcluded;
delete  CheckAux;               delete  AuxThreshold;           delete  AuxOverride;
delete  TriggerFixedTF;         delete  Trigger;                delete  TriggerOffset;          delete  TriggerReactionTime;
delete  TriggerOffsetValue;     delete  TriggerOffsetValueMs;
delete  TriggerFixedTFValue;    delete  TriggerFixedTFValueMs;
delete  DurationPre;            delete  DurationPreMs;
delete  DurationPost;           delete  DurationPostMs;
delete  DurationTotal;          delete  DurationTotalMs;
delete  SignedData;             delete  PositiveData;
delete  NoRef;                  delete  AvgRef;
delete  AccountPolarity;        delete  IgnorePolarity;
delete  SetFilters;             delete  FilterData;             delete  FilterDisplay;
delete  BaselineCorr;
delete  BaselineCorrPre;        delete  BaselineCorrPreMs;      delete  BaselineCorrPost;       delete  BaselineCorrPostMs;
delete  SkipBadEpochs;          delete  SkipMarkers;
delete  AcceptAll;
}


void    TTracksAveragingParamsDialog::SetupWindow ()
{
TTracksAveragingBaseDialog::SetupWindow ();

EvTFChanged ();

                                        // "auto" mode, we force refreshing the automatic loading of boundaries - side-effect is that it can reset some options
int                 preseti         = Presets->GetSelIndex ();

if ( IsInsideLimits ( preseti, (int) AvgPresetGrandaverageErpSurface, (int) AvgPresetGrandaverageRisScal ) )
    EvPresetsChange ();
}


//----------------------------------------------------------------------------
void    TTracksAveragingParamsDialog::EvPresetsChange ()
{
if ( StringIsEmpty ( AvgPresets[ Presets->GetSelIndex () ] ) )
    return;

                                        // reset all options
CheckEl             ->SetCheck ( BoolToCheck ( false ) );
CheckAux            ->SetCheck ( BoolToCheck ( false ) );

TriggerFixedTF      ->SetCheck ( BoolToCheck ( false ) );
Trigger             ->SetCheck ( BoolToCheck ( false ) );
TriggerOffset       ->SetCheck ( BoolToCheck ( false ) );
TriggerReactionTime ->SetCheck ( BoolToCheck ( false ) );

PositiveData        ->SetCheck ( BoolToCheck ( false ) );
SignedData          ->SetCheck ( BoolToCheck ( false ) );
NoRef               ->SetCheck ( BoolToCheck ( false ) );
AvgRef              ->SetCheck ( BoolToCheck ( false ) );
AccountPolarity     ->SetCheck ( BoolToCheck ( false ) );
IgnorePolarity      ->SetCheck ( BoolToCheck ( false ) );

AcceptAll           ->SetCheck ( BoolToCheck ( false ) );

char                buff[ EditSizeText ];

                                        // then set only the right ones
switch ( Presets->GetSelIndex () ) {

    case    AvgPresetErpEegSurface :
    case    AvgPresetErpEegIntra   :
        CheckEl             ->SetCheck ( BoolToCheck ( true  ) );

        Trigger             ->SetCheck ( BoolToCheck ( true  ) );

        SignedData          ->SetCheck ( BoolToCheck ( true  ) );
        NoRef               ->SetCheck ( BoolToCheck ( true  ) );
        AccountPolarity     ->SetCheck ( BoolToCheck ( true  ) );
        break;

    case    AvgPresetErpFreqPower :
        CheckEl             ->SetCheck ( BoolToCheck ( true  ) );

        Trigger             ->SetCheck ( BoolToCheck ( true  ) );

        PositiveData        ->SetCheck ( BoolToCheck ( true  ) );
        NoRef               ->SetCheck ( BoolToCheck ( true  ) );
        AccountPolarity     ->SetCheck ( BoolToCheck ( true  ) );
        BaselineCorr        ->SetCheck ( BoolToCheck ( false ) );
        break;

    case    AvgPresetErpFreqFftapprox :
        CheckEl             ->SetCheck ( BoolToCheck ( true  ) );

        Trigger             ->SetCheck ( BoolToCheck ( true  ) );

        SignedData          ->SetCheck ( BoolToCheck ( true  ) );
        AvgRef              ->SetCheck ( BoolToCheck ( true  ) );
        IgnorePolarity      ->SetCheck ( BoolToCheck ( true  ) );
        BaselineCorr        ->SetCheck ( BoolToCheck ( false ) );
        break;

    case    AvgPresetErpRisScal :
        CheckEl             ->SetCheck ( BoolToCheck ( true  ) );

        Trigger             ->SetCheck ( BoolToCheck ( true  ) );

        PositiveData        ->SetCheck ( BoolToCheck ( true  ) );
        NoRef               ->SetCheck ( BoolToCheck ( true  ) );
        AccountPolarity     ->SetCheck ( BoolToCheck ( true  ) );
        BaselineCorr        ->SetCheck ( BoolToCheck ( false ) );
        break;


    case    AvgPresetGrandaverageErpSurface :
    case    AvgPresetGrandaverageErpIntra   :
        TriggerFixedTF      ->SetCheck ( BoolToCheck ( true  ) );
        TriggerFixedTFValue ->SetText ( "0" );

        DurationPre         ->SetText ( "0" );
        IntegerToString ( buff, TAvgTransfer.MaxNumTF > 0 ? TAvgTransfer.MaxNumTF : 0 );
        DurationPost        ->SetText ( buff );
        DurationTotal       ->SetText ( buff );

        SignedData          ->SetCheck ( BoolToCheck ( true  ) );
        NoRef               ->SetCheck ( BoolToCheck ( true  ) );
        AccountPolarity     ->SetCheck ( BoolToCheck ( true  ) );
        BaselineCorr        ->SetCheck ( BoolToCheck ( false ) );

        SkipBadEpochs       ->SetCheck ( BoolToCheck ( false ) );
        AcceptAll           ->SetCheck ( BoolToCheck ( true  ) );
        break;

    case    AvgPresetGrandaverageFreqPower :
        TriggerFixedTF      ->SetCheck ( BoolToCheck ( true  ) );
        TriggerFixedTFValue ->SetText ( "0" );

        DurationPre->SetText ( "0" );
        sprintf ( buff, "%0d", TAvgTransfer.MaxNumTF > 0 ? TAvgTransfer.MaxNumTF : 0 );
        DurationPost        ->SetText ( buff );
        DurationTotal       ->SetText ( buff );

        PositiveData        ->SetCheck ( BoolToCheck ( true  ) );
        NoRef               ->SetCheck ( BoolToCheck ( true  ) );
        AccountPolarity     ->SetCheck ( BoolToCheck ( true  ) );
        BaselineCorr        ->SetCheck ( BoolToCheck ( false ) );

        SkipBadEpochs       ->SetCheck ( BoolToCheck ( false ) );
        AcceptAll           ->SetCheck ( BoolToCheck ( true  ) );
        break;

    case    AvgPresetGrandaverageFreqFftapprox :
        TriggerFixedTF      ->SetCheck ( BoolToCheck ( true  ) );
        TriggerFixedTFValue ->SetText ( "0" );

        DurationPre         ->SetText ( "0" );
        sprintf ( buff, "%0d", TAvgTransfer.MaxNumTF > 0 ? TAvgTransfer.MaxNumTF : 0 );
        DurationPost        ->SetText ( buff );
        DurationTotal       ->SetText ( buff );

        SignedData          ->SetCheck ( BoolToCheck ( true  ) );
        AvgRef              ->SetCheck ( BoolToCheck ( true  ) );
        IgnorePolarity      ->SetCheck ( BoolToCheck ( true  ) );
        BaselineCorr        ->SetCheck ( BoolToCheck ( false ) );

        SkipBadEpochs       ->SetCheck ( BoolToCheck ( false ) );
        AcceptAll           ->SetCheck ( BoolToCheck ( true  ) );
        break;

    case    AvgPresetGrandaverageRisScal :
        TriggerFixedTF      ->SetCheck ( BoolToCheck ( true  ) );
        TriggerFixedTFValue ->SetText ( "0" );

        DurationPre         ->SetText ( "0" );
        sprintf ( buff, "%0d", TAvgTransfer.MaxNumTF > 0 ? TAvgTransfer.MaxNumTF : 0 );
        DurationPost        ->SetText ( buff );
        DurationTotal       ->SetText ( buff );

        PositiveData        ->SetCheck ( BoolToCheck ( true  ) );
        NoRef               ->SetCheck ( BoolToCheck ( true  ) );
        AccountPolarity     ->SetCheck ( BoolToCheck ( true  ) );
        BaselineCorr        ->SetCheck ( BoolToCheck ( false ) );

        SkipBadEpochs       ->SetCheck ( BoolToCheck ( false ) );
        AcceptAll           ->SetCheck ( BoolToCheck ( true  ) );
        break;
    }

}


TStringValue    TTracksAveragingParamsDialog::TFtoms ( int tf )
{
TStringValue    buff;

if ( TAvgTransfer.SamplingFrequency >= 1 )
    buff    = IntegerToString ( Round ( TimeFrameToMilliseconds ( tf, TAvgTransfer.SamplingFrequency ) ) ) + " ms";

return buff;
}


//----------------------------------------------------------------------------
/*void    TTracksAveragingParamsDialog::CmToNextDialogEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

char        buff[EditSizeValue];


if ( CheckToBool ( BaselineCorr->GetCheck() ) ) {
    int     pre     = GetInteger  ( BaselineCorrPre  );
    int     post    = GetInteger  ( BaselineCorrPost );

    if ( pre > post  )   { tce.Enable ( false ); return; }
    }


int     durtot  = GetInteger  ( DurationTotal );

tce.Enable ( durtot != 0 );
}
*/

//----------------------------------------------------------------------------
void    TTracksAveragingParamsDialog::EvDropFiles ( TDropInfo drop )
{
TGoF                paramfiles      ( drop, BatchFilesExt );

char                buff[ 256 ];
StringCopy ( buff, BatchFilesExt );
TGoF                remainingfiles  ( drop, buff, 0, true );

drop.DragFinish ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) paramfiles; i++ ) {

    ReadParams ( paramfiles[ i ] );

    ShowMessage ( "Parameters successfully retrieved from ." FILEEXT_VRB " file!", ToFileName ( paramfiles[ i ] ), ShowMessageNormal, this );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) remainingfiles )
    remainingfiles.Show ( IrrelevantErrorMessage );
}


//----------------------------------------------------------------------------
void    TTracksAveragingParamsDialog::CmReadParams ()
{
static GetFileFromUser  getfile ( "Verbose File", AllVerboseFilesFilter, 1, GetFileRead );


if ( ! getfile.Execute () )
    return;


ReadParams ( getfile );
}


void    TTracksAveragingParamsDialog::ReadParams ( char *file )
{
char                buff[ EditSizeText ];
char                str [ 4 ][ 256 ];
char               *toc;


ifstream    is ( TFileName ( file, TFilenameExtendedPath ) );

do {
    is.getline ( buff, EditSizeText );

    if ( StringIsSpace ( buff ) )  continue;

    ClearString ( str[ 0 ] );
    ClearString ( str[ 1 ] );
    ClearString ( str[ 2 ] );
    ClearString ( str[ 3 ] );

    toc     = buff + VerboseFileDefaultWidth;


    if      ( StringStartsWith ( buff, "Duration Pre" ) ) {
        sscanf ( toc, "%s", TAvgTransfer.DurationPre );
        EvEditPrePostChanged ();
        }

    else if ( StringStartsWith ( buff, "Duration Post" ) ) {
        sscanf ( toc, "%s", TAvgTransfer.DurationPost );
        EvEditPrePostChanged ();
        }

    else if ( StringStartsWith ( buff, "Duration Total" ) ) {
        sscanf ( toc, "%s", TAvgTransfer.DurationTotal );
        EvEditPrePostChanged ();
        }

    else if ( StringStartsWith ( buff, "Epoch origin" ) ) {

        TAvgTransfer.TriggerFixedTF         = BoolToCheck ( false );
        TAvgTransfer.TriggerOffset          = BoolToCheck ( false );
        TAvgTransfer.TriggerReactionTime    = BoolToCheck ( false );
        TAvgTransfer.Trigger                = BoolToCheck ( false );

        sscanf ( toc, "%s %s %s %s", str[ 0 ], str[ 1 ], str[ 2 ], str[ 3 ] );

        if      ( StringIs ( str[ 0 ], "trigger" ) && StringIs ( str[ 2 ], "reaction" ) )
            TAvgTransfer.TriggerReactionTime= BoolToCheck ( true  );

        else if ( StringIs ( str[ 0 ], "trigger" ) && StringIs ( str[ 1 ], "+" ) ) {
            TAvgTransfer.TriggerOffset      = BoolToCheck ( true  );
            StringCopy ( TAvgTransfer.TriggerOffsetValue,    str[ 2 ] );
            }

        else if ( StringIs ( str[ 0 ], "trigger" ) )
            TAvgTransfer.Trigger            = BoolToCheck ( true  );

        else if ( StringIs ( str[ 0 ], "TF" ) ) {
            TAvgTransfer.TriggerFixedTF     = BoolToCheck ( true  );
            StringCopy ( TAvgTransfer.TriggerFixedTFValue,   str[ 1 ] );
            }
        } // trigger type


    else if ( StringStartsWith ( buff, "Electrodes coordinates" ) ) {
                                        // Use file name for both xyz display and filter
        if ( StringIsEmpty ( toc ) || StringIs ( toc, "none" ) ) {

            ClearString ( TAvgTransfer.XyzDocFile );                        // does not seem to update, because not the current dialog...
            ClearString ( TAvgTransfer.Filters.FiltersParam.XyzFile );
            }

        else {
            StringCopy  ( TAvgTransfer.XyzDocFile,                   toc ); // does not seem to update, because not the current dialog...
            StringCopy  ( TAvgTransfer.Filters.FiltersParam.XyzFile, toc );
            }
        }

    else if ( StringStartsWith ( buff, "Filters:" ) ) {
                                    // stupid trick to set the init flag to false, and prevent overwriting the textual params
        TTracksFiltersDialog    (   CartoolMainWindow,              IDD_TRACKSFILTERS, 
                                    TAvgTransfer.Filters.FiltersParam,  TAvgTransfer.SamplingFrequency ? TAvgTransfer.SamplingFrequency : 0 );

        TAvgTransfer.Filters.TextToParameters ( toc, TAvgTransfer.XyzDocFile, TAvgTransfer.SamplingFrequency );

        TAvgTransfer.SetFilters             = BoolToCheck ( TAvgTransfer.Filters.HasAnyFilter () );
        }

    else if ( StringStartsWith ( buff, "Filter data" ) )
        TAvgTransfer.FilterData             = StringToCheck ( toc );

    else if ( StringStartsWith ( buff, "Filter display" ) )
        TAvgTransfer.FilterDisplay          = StringToCheck ( toc );

    else if ( StringStartsWith ( buff, "Baseline correction" ) )
        TAvgTransfer.BaselineCorr           = StringToCheck ( toc );

    else if ( StringStartsWith ( buff, "Inferior limit" ) )
        sscanf ( toc, "%s", TAvgTransfer.BaselineCorrPre );

    else if ( StringStartsWith ( buff, "Superior limit" ) )
        sscanf ( toc, "%s", TAvgTransfer.BaselineCorrPost );

    else if ( StringStartsWith ( buff, "Regular tracks tested" ) )
        TAvgTransfer.CheckEl                = StringToCheck ( toc );

    else if ( StringStartsWith ( buff, "Regular tracks threshold" ) )
        StringCopy ( TAvgTransfer.ElThreshold, toc );

    else if ( StringStartsWith ( buff, "Channels excluded" ) ) {
        if ( StringToFalse ( toc ) )
            ClearString ( TAvgTransfer.ElExcluded );
        else
            StringCopy ( TAvgTransfer.ElExcluded, toc );
        }

    else if ( StringStartsWith ( buff, "Auxiliary tracks tested" ) )
        TAvgTransfer.CheckAux               = StringToCheck ( toc );

    else if ( StringStartsWith ( buff, "Auxiliary tracks threshold" ) )
        StringCopy ( TAvgTransfer.AuxThreshold, toc );

    else if ( StringStartsWith ( buff, "Auxiliary tracks override" ) ) {
        if ( StringToFalse ( toc ) )
            ClearString ( TAvgTransfer.AuxOverride );
        else
            StringCopy ( TAvgTransfer.AuxOverride, toc );
        }

    else if ( StringStartsWith ( buff, "Automatically accept all" ) )
        TAvgTransfer.AcceptAll              = StringToCheck ( toc );

    else if ( StringStartsWith ( buff, "Applying average reference" ) 
           || StringStartsWith ( buff, "Data reference" ) ) {
        TAvgTransfer.AvgRef                 = StringToCheck ( toc ) || StringIs ( toc, "Average Reference" );
        TAvgTransfer.NoRef                  = BoolToCheck ( ! CheckToBool ( TAvgTransfer.AvgRef ) );
        }
                                        // not supported anymore
//  else if ( StringStartsWith ( buff, "Tracks normalization by GFP" ) ) {
//      }

    else if ( StringStartsWith ( buff, "Ignoring map polarity" ) )
        TAvgTransfer.IgnorePolarity         = StringToCheck ( toc );

    else if ( StringStartsWith ( buff, "Output file type" ) ) {
        if      ( StringIs ( toc, FILEEXT_EEGEP  ) )    TAvgTransfer.FileTypes.Select ( PresetFileTypeEp  );
        else if ( StringIs ( toc, FILEEXT_EEGEPH ) )    TAvgTransfer.FileTypes.Select ( PresetFileTypeEph );
        else if ( StringIs ( toc, FILEEXT_TXT    ) )    TAvgTransfer.FileTypes.Select ( PresetFileTypeEp  );
        else if ( StringIs ( toc, FILEEXT_EEGSEF ) )    TAvgTransfer.FileTypes.Select ( PresetFileTypeSef );
        else if ( StringIs ( toc, FILEEXT_EEGBV  ) )    TAvgTransfer.FileTypes.Select ( PresetFileTypeBV  );
        else if ( StringIs ( toc, FILEEXT_RIS    ) )    TAvgTransfer.FileTypes.Select ( PresetFileTypeRis );
        }

    else if ( StringStartsWith ( buff, "Merge all triggers into a single file" ) )
        TAvgTransfer.MergeTriggers          = StringToCheck ( toc );

    else if ( StringStartsWith ( buff, "Split triggers into separate files" ) )
        TAvgTransfer.SplitTriggers          = StringToCheck ( toc );

    else if ( StringStartsWith ( buff, "Excluded  channels written" ) )
        TAvgTransfer.SaveExcluded           = StringToCheck ( toc );

    else if ( StringStartsWith ( buff, "Auxiliary channels written" ) )
        TAvgTransfer.SaveAux                = StringToCheck ( toc );
                                        // not supported anymore
//  else if ( StringStartsWith ( buff, "Reordering of the channels" ) )
//  else if ( StringStartsWith ( buff, "Use this order" ) )

    else if ( StringStartsWith ( buff, "Add marker to epoch origin" ) )
        TAvgTransfer.OriginMarker           = StringToCheck ( toc );

    else if ( StringStartsWith ( buff, "Save average" ) )
        TAvgTransfer.SaveAverage            = StringToCheck ( toc );

    else if ( StringStartsWith ( buff, "Save sum" ) )
        TAvgTransfer.SaveSum                = StringToCheck ( toc );

    else if ( StringStartsWith ( buff, "Save standard deviation" ) )
        TAvgTransfer.SaveSD                 = StringToCheck ( toc );

    else if ( StringStartsWith ( buff, "Save standard error" ) )
        TAvgTransfer.SaveSE                 = StringToCheck ( toc );

    else if ( StringStartsWith ( buff, "Save epochs in separate files" ) )
        TAvgTransfer.SaveEpochsMultiFiles   = StringToCheck ( toc );

    else if ( StringStartsWith ( buff, "Save epochs in 1 file" ) )
        TAvgTransfer.SaveEpochsSingleFile   = StringToCheck ( toc );

    } while ( !is.eof () );

                                        // transfer the stuff
TransferData ( tdSetData );

                                        // call all these resfreshing functions...
EvTFChanged ();
//EvFiltersChanged ();
}


//----------------------------------------------------------------------------
                                        // smart switches -> select filtering options automatically
void    TTracksAveragingParamsDialog::EvFiltersChanged ()
{
                                        // check limits
TransferData ( tdGetData );

if ( ! CheckToBool ( TAvgTransfer.SetFilters ) )
    return;

                                        // run dialog to ask the user
if ( TTracksFiltersDialog   (   CartoolMainWindow,              IDD_TRACKSFILTERS, 
                                TAvgTransfer.Filters.FiltersParam,  TAvgTransfer.SamplingFrequency ? TAvgTransfer.SamplingFrequency : 0 
                            ).Execute() == IDCANCEL ) {
    return;
    }

                                        // chew the results in a comprehensive way
TAvgTransfer.Filters.SetFromStruct ( TAvgTransfer.SamplingFrequency, Interactive );

                                        // reset button if dialog returns no filters
if ( TAvgTransfer.Filters.HasNoFilters () ) {
    TAvgTransfer.SetFilters     = BoolToCheck ( false );
    TransferData ( tdSetData );
    return;
    }

                                        // no freq from files? try the filter dialog
if ( TAvgTransfer.SamplingFrequency <= 0 ) {

    TAvgTransfer.SamplingFrequency = TAvgTransfer.Filters.SamplingFrequency;

    EvTFChanged ();                     // update conversion display
    }


if ( TAvgTransfer.SamplingFrequency > 0 && TAvgTransfer.Filters.HasAnyFilter () ) {
    if ( ! ( CheckToBool ( FilterData->GetCheck () ) || CheckToBool ( FilterDisplay->GetCheck () ) ) ) {
        FilterData   ->SetCheck ( BoolToCheck ( true  ) );
        FilterDisplay->SetCheck ( BoolToCheck ( true  ) );
        }
    }
else {
    FilterData   ->SetCheck ( BoolToCheck ( false ) );
    FilterDisplay->SetCheck ( BoolToCheck ( false ) );
    }


BaselineCorr->SetFocus ();
}


void    TTracksAveragingParamsDialog::CmSetFiltersEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
                                        // does not make any sense of filtering frequencies!
if ( Presets->GetSelIndex () == AvgPresetErpFreqPower
  || Presets->GetSelIndex () == AvgPresetErpFreqFftapprox
  || Presets->GetSelIndex () == AvgPresetGrandaverageFreqPower
  || Presets->GetSelIndex () == AvgPresetGrandaverageFreqFftapprox )
    tce.Enable ( false );
else
    tce.Enable ( StringToDouble ( ((TTracksAveragingParamsStruct*) GetTransferBuffer () )->DurationTotal ) > 0 );
}


void    TTracksAveragingParamsDialog::CmFiltersEnable ( TCommandEnabler &tce )
{
tce.Enable ( TAvgTransfer.SamplingFrequency > 0 && StringToDouble ( ((TTracksAveragingParamsStruct*) GetTransferBuffer () )->DurationTotal ) > 0
            && TAvgTransfer.Filters.HasAnyFilter ()
            && CheckToBool ( ((TTracksAveragingParamsStruct*) GetTransferBuffer () )->SetFilters ) );
}


void    TTracksAveragingParamsDialog::CmAvgRefEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );
                                        // does not make any sense of filtering frequencies!
tce.Enable ( Presets->GetSelIndex () == AvgPresetErpEegSurface
          || Presets->GetSelIndex () == AvgPresetGrandaverageErpSurface );
}


void    TTracksAveragingParamsDialog::CmSO1TFEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( TriggerFixedTF->GetCheck() ) );
}


void    TTracksAveragingParamsDialog::CmBaseLineCorrectionEnable ( TCommandEnabler &tce )
{
                                        // enabling baseline correction is quite restrictive
tce.Enable ( ! ( CheckToBool ( IgnorePolarity->GetCheck () ) || CheckToBool ( PositiveData->GetCheck () ) ) );
}


void    TTracksAveragingParamsDialog::CmBaseLineCorrectionParamsEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( BaselineCorr->GetCheck() ) && ! CheckToBool ( IgnorePolarity->GetCheck() ) );
}


void    TTracksAveragingParamsDialog::CmTriggerOffsetEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( TriggerOffset->GetCheck() ) );
}


void    TTracksAveragingParamsDialog::CmCheckElEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( CheckEl->GetCheck() ) );
}


void    TTracksAveragingParamsDialog::CmCheckAuxEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( CheckAux->GetCheck() ) );
}


void    TTracksAveragingParamsDialog::CmSkipBadEpochsListEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( SkipBadEpochs->GetCheck() ) );
}


void    TTracksAveragingParamsDialog::EvEditPrePostChanged ()
{
int                 val1            = GetInteger  ( DurationPre  );
int                 val2            = GetInteger  ( DurationPost );
int                 total           = val1 + val2;

//if ( CheckToBool ( BaselineCorr->GetCheck() ) ) {

    SetInteger  ( BaselineCorrPre,  -val1 );
//  SetInteger  ( BaselineCorrPost,  val2 );

    BaselineCorrPreMs ->SetText ( TFtoms ( -val1 ) );
//  BaselineCorrPostMs->SetText ( TFtoms (  val2 ) );
//  }

SetInteger  ( DurationTotal, total );
DurationTotalMs->SetText ( TFtoms ( total ) );

EvTFChanged ();
}


void    TTracksAveragingParamsDialog::EvTFChanged ()
{
TriggerFixedTFValueMs->SetText ( TFtoms ( GetInteger ( TriggerFixedTFValue ) ) );
TriggerOffsetValueMs ->SetText ( TFtoms ( GetInteger ( TriggerOffsetValue  ) ) );
DurationPreMs        ->SetText ( TFtoms ( GetInteger ( DurationPre         ) ) );
DurationPostMs       ->SetText ( TFtoms ( GetInteger ( DurationPost        ) ) );
DurationTotalMs      ->SetText ( TFtoms ( GetInteger ( DurationTotal       ) ) );
BaselineCorrPreMs    ->SetText ( TFtoms ( GetInteger ( BaselineCorrPre     ) ) );
BaselineCorrPostMs   ->SetText ( TFtoms ( GetInteger ( BaselineCorrPost    ) ) );
BaselineCorrPostMs   ->SetText ( TFtoms ( GetInteger ( BaselineCorrPost    ) ) );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

DEFINE_RESPONSE_TABLE1 (TTracksAveragingOutputsDialog, TTracksAveragingBaseDialog)

    EV_COMMAND_ENABLE           ( IDC_SPLITTRIGGERS,    CmSplitTriggersEnable ),

END_RESPONSE_TABLE;


        TTracksAveragingOutputsDialog::TTracksAveragingOutputsDialog ( TWindow* parent, int resId )
      : TTracksAveragingBaseDialog ( parent, resId )

{
StringCopy ( BatchFilesExt, FILEEXT_VRB );


MergeTriggers           = new TCheckBox ( this, IDC_MERGETRIGGERS );
SplitTriggers           = new TCheckBox ( this, IDC_SPLITTRIGGERS );
SaveAverage             = new TCheckBox ( this, IDC_SAVEAVG );
SaveSum                 = new TCheckBox ( this, IDC_SAVESUM );
SaveSD                  = new TCheckBox ( this, IDC_SAVESD );
SaveSE                  = new TCheckBox ( this, IDC_SAVESE );
SaveEpochsMultiFiles    = new TCheckBox ( this, IDC_SAVEEPOCHSMULTIFILES );
SaveEpochsSingleFile    = new TCheckBox ( this, IDC_SAVEEPOCHSSINGLEFILE );

FileTypes               = new TComboBox ( this, IDC_FILETYPES );

SaveExcluded            = new TCheckBox ( this, IDC_SAVEEXCLUDED );
SaveAux                 = new TCheckBox ( this, IDC_AUXCHANNELS );

OpenAuto                = new TCheckBox ( this, IDC_OPENAUTO );
OriginMarker            = new TCheckBox ( this, IDC_ORIGINMARKER );


SetTransferBuffer ( dynamic_cast <TTracksAveragingOutputsStruct*> ( &TAvgTransfer ) );

                                        // browse the files to guess what output type is best
//TAvgTransfer.FileTypeEph            = BoolToCheck ( true  );
}


        TTracksAveragingOutputsDialog::~TTracksAveragingOutputsDialog ()
{
delete  MergeTriggers;          delete  SplitTriggers;
delete  SaveAverage;            delete  SaveSum;
delete  SaveSD;                 delete  SaveSE;
delete  SaveEpochsMultiFiles;   delete  SaveEpochsSingleFile;
delete  FileTypes;
delete  SaveExcluded;           delete  SaveAux;
delete  OpenAuto;               delete  OriginMarker;
}


void    TTracksAveragingOutputsDialog::CmSplitTriggersEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! CheckToBool ( TAvgTransfer.TriggerFixedTF ) );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TTracksAveragingBaseDialog, TBaseDialog )
    EV_COMMAND                  ( IDOK,                     CmOk ),
    EV_COMMAND_ENABLE           ( IDOK,                     CmOkEnable ),
    EV_COMMAND_AND_ID           ( IDC_TOPREVDIALOG,         CmToDialog ),
    EV_COMMAND_AND_ID           ( IDC_TONEXTDIALOG,         CmToDialog ),
END_RESPONSE_TABLE;



        TTracksAveragingBaseDialog::TTracksAveragingBaseDialog (  TWindow* parent, int resId ) :
        TBaseDialog ( parent, resId )
{
}


        TTracksAveragingBaseDialog::~TTracksAveragingBaseDialog ()
{
}


//----------------------------------------------------------------------------
void    TTracksAveragingBaseDialog::CmToDialog ( owlwparam w )
{
uint                ResId           = PtrToUint ( Attr.Name );

                                        // goto new id
uint                todialogid      = Clip ( ResId + ( w == IDC_TOPREVDIALOG ? -1 : 1 ), (uint) IDD_AVERAGE1, (uint) IDD_AVERAGE3 );

                                        // avoid calling itself
if ( todialogid == ResId )
    return;


Destroy ();                             // remove the window, not the object

                                        // remember the last dialog
TAvgTransfer.LastDialogId   = todialogid;


if      ( todialogid == IDD_AVERAGE1 )  TTracksAveragingFilesDialog     ( CartoolMdiClient, IDD_AVERAGE1 ).Execute ();
else if ( todialogid == IDD_AVERAGE2 )  TTracksAveragingParamsDialog    ( CartoolMdiClient, IDD_AVERAGE2 ).Execute ();
else if ( todialogid == IDD_AVERAGE3 )  TTracksAveragingOutputsDialog   ( CartoolMdiClient, IDD_AVERAGE3 ).Execute ();
}


//----------------------------------------------------------------------------
void    TTracksAveragingBaseDialog::CmOkEnable ( TCommandEnabler& tce )
{
TransferData ( tdGetData );

if ( TAvgTransfer.GetNumFiles () == 0 || ! TAvgTransfer.AllFilesOk ) {
    tce.Enable ( false );
    return;
    }

if ( ! ( CheckToBool ( TAvgTransfer.UseEvents ) || CheckToBool ( TAvgTransfer.UseTriggers ) || CheckToBool ( TAvgTransfer.UseMarkers ) ) ) {
    tce.Enable ( false );
    return;
    }

                                        // directly playing with the buffer does not seem to be a good idea, maybe its updated during the test?
TFileName           buff;
StringCopy ( buff, TAvgTransfer.BaseFileName );

if ( ! IsAbsoluteFilename ( buff ) ) {
    tce.Enable ( false );
    return;
    }

if ( CheckToBool ( TAvgTransfer.BaselineCorr ) ) {
    if ( StringToDouble ( TAvgTransfer.BaselineCorrPre ) > StringToDouble ( TAvgTransfer.BaselineCorrPost ) ) {
        tce.Enable ( false );
        return; }
    }

if ( StringToDouble ( TAvgTransfer.DurationTotal ) <= 0 ) {
    tce.Enable ( false );
    return;
    }

if ( CheckToBool ( TAvgTransfer.SkipBadEpochs ) && StringIsEmpty ( TAvgTransfer.SkipMarkers ) ) {
    tce.Enable ( false );
    return;
    }


tce.Enable (    (    CheckToBool ( TAvgTransfer.SaveAverage   ) 
                  || CheckToBool ( TAvgTransfer.SaveSum       )
                  || CheckToBool ( TAvgTransfer.SaveSD        ) 
                  || CheckToBool ( TAvgTransfer.SaveSE        ) 
                )
             && (    CheckToBool ( TAvgTransfer.MergeTriggers ) 
                  || CheckToBool ( TAvgTransfer.SplitTriggers ) && ! CheckToBool ( TAvgTransfer.TriggerFixedTF ) 
                )
          ||         CheckToBool ( TAvgTransfer.SaveEpochsMultiFiles ) 
          ||         CheckToBool ( TAvgTransfer.SaveEpochsSingleFile ) 
           );

//tce.Enable ( true );
}


void    TTracksAveragingBaseDialog::CmOk ()
{
Destroy ();

                                        // transfer to a GoGoF of 4 groups, each group holding one type of the files parameters
TGoGoF              GoGoF;
                                        
GoGoF.Add ( new TGoF ( TAvgTransfer.ComboEeg.GetStrings (), TFilenameFlags ( TFilenameAbsolutePath | TFilenameExtendedPath ) ) );   // real file name - needs some checks
GoGoF.Add ( new TGoF ( TAvgTransfer.ComboSes.GetStrings (),                  TFilenameNoPreprocessing                        ) );   // just strings   - skip checks
GoGoF.Add ( new TGoF ( TAvgTransfer.ComboTva.GetStrings (), TFilenameFlags ( TFilenameAbsolutePath | TFilenameExtendedPath ) ) );   // real file name - needs some checks
GoGoF.Add ( new TGoF ( TAvgTransfer.ComboTrg.GetStrings (),                  TFilenameNoPreprocessing                        ) );   // just strings   - skip checks

                                        // first file introduced = first file to be processed
GoGoF.RevertOrder ();

                                        // does the handling of frequencies, if any
BatchProcessGroups ( &GoGoF, 4, 1, &TAvgTransfer );
}


//----------------------------------------------------------------------------
void    TTracksAveragingBaseDialog::ProcessGroups ( TGoGoF *gogof, int gofi1, int gofi2, void *usetransfer )
{
                                        // create control dialog
TTracksAveragingControlDialog   ControlDlg ( CartoolMdiClient, IDD_AVERAGECONTROL );

                                        // open it modeless
ControlDlg.Create ();

                                        // update the controls with current frequency
ControlDlg.NumFreqs     = NumFreqs;
ControlDlg.FreqIndex    = FreqIndex;

                                        // do the averaging
ControlDlg.ProcessGroups ( gogof, gofi1, gofi2, usetransfer );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TTracksAveragingControlStruct::TTracksAveragingControlStruct ()
{
ClearString ( CurrentFile         );

ClearString ( TriggerCode         );
ClearString ( TriggerPos          );
ClearString ( TriggerIndex        );

ClearString ( AcceptNumTF         );
ClearString ( RejectNumTF         );

ClearString ( NumTriggersAccepted );
ClearString ( NumTriggersRejected );

ClearString ( TrackName           );
ClearString ( TrackMin            );
ClearString ( TrackMax            );
ClearString ( Threshold           );

ClearString ( AuxName             );
ClearString ( AuxMin              );
ClearString ( AuxMax              );
ClearString ( AuxThreshold        );

TrackZoom          = BoolToCheck ( false );
ClearString ( TracksToExclude     );
ClearString ( TracksExcluded      );
}


//----------------------------------------------------------------------------

DEFINE_RESPONSE_TABLE1 (TTracksAveragingControlDialog, TBaseDialog)
    EV_WM_TIMER,
    EV_COMMAND_AND_ID           ( IDC_ACCTRIGG,     CmButtonPressed ),
    EV_COMMAND_AND_ID           ( IDC_ACCNEXT,      CmButtonPressed ),
    EV_COMMAND_AND_ID           ( IDC_REJTRIGG,     CmButtonPressed ),
    EV_COMMAND_AND_ID           ( IDC_REJNEXT,      CmButtonPressed ),
    EV_COMMAND_AND_ID           ( IDC_ACCCOND,      CmButtonPressed ),
    EV_COMMAND_AND_ID           ( IDC_EXCLCHANNEL,  CmButtonPressed ),
    EV_COMMAND_AND_ID           ( IDC_CHANNELZOOM,  CmButtonPressed ),
    EV_COMMAND_AND_ID           ( IDC_RESTORECURSOR,CmButtonPressed ),
    EV_COMMAND_AND_ID           ( IDC_RESTORETRACKS,CmButtonPressed ),
    EV_COMMAND_AND_ID           ( IDCANCEL,         CmButtonPressed ),
    EV_COMMAND_AND_ID           ( IDC_UNDOLAST,     CmButtonPressed ),
    EV_COMMAND_AND_ID           ( IDC_RESTARTCURRENT,CmButtonPressed ),
    EV_COMMAND_AND_ID           ( IDC_ACCEPTUNTILBAD,CmButtonPressed ),
    EV_COMMAND_AND_ID           ( IDC_REJECTUNTILOK,CmButtonPressed ),
    EV_COMMAND                  ( IDOK,             CmMyOk ),
    EV_COMMAND_ENABLE           ( IDC_AUXNAMES,     CmAuxThresholdEnable ),
    EV_COMMAND_ENABLE           ( IDC_AUXMIN,       CmAuxThresholdEnable ),
    EV_COMMAND_ENABLE           ( IDC_AUXMAX,       CmAuxThresholdEnable ),
    EV_COMMAND_ENABLE           ( IDC_AUXTHRESHOLD, CmAuxThresholdEnable ),
    EV_COMMAND_ENABLE           ( IDC_ACCEPTUNTILBAD, CmAcceptUntilBadEnable ),
    EV_COMMAND_ENABLE           ( IDC_REJECTUNTILOK,  CmRejectUntilOkEnable ),
END_RESPONSE_TABLE;


        TTracksAveragingControlDialog::TTracksAveragingControlDialog ( TWindow* parent, TResId resId /*, TTracksAveragingControlStruct& transfer*/ )
      : TBaseDialog ( parent, resId )
{
ButtonPressed           = 0;
canClose                = false;
EegView                 = 0;
EpochState              = true;


CurrentFile             = new TEdit ( this, IDC_CURRFILE, EditSizeText );

TriggerCode             = new TEdit ( this, IDC_TRIGGCODE, EditSizeText );
TriggerPos              = new TEdit ( this, IDC_TRIGGPOS, EditSizeValue );
TriggerIndex            = new TEdit ( this, IDC_CURRTRIGG, EditSizeValue );

AcceptNumTF             = new TEdit ( this, IDC_ANE, EditSizeValue );
RejectNumTF             = new TEdit ( this, IDC_RNE, EditSizeValue );

NumTriggersAccepted     = new TEdit ( this, IDC_NUMTRIGGACC, EditSizeValue );
NumTriggersRejected     = new TEdit ( this, IDC_NUMTRIGGREJ, EditSizeValue );

//SetCaption
TracksBox               = new TGroupBox ( this, IDC_GROUPTRACKS );

TrackName               = new TEdit ( this, IDC_CURRCHANNEL, EditSizeText );
TrackMin                = new TEdit ( this, IDC_CHANNMIN, EditSizeValue );
TrackMax                = new TEdit ( this, IDC_CHANNMAX, EditSizeValue );
Threshold               = new TEdit ( this, IDC_AMPLWINDOW, EditSizeValue );

AuxName                 = new TEdit ( this, IDC_AUXNAMES, EditSizeText );
AuxMin                  = new TEdit ( this, IDC_AUXMIN, EditSizeValue );
AuxMax                  = new TEdit ( this, IDC_AUXMAX, EditSizeValue );
AuxThreshold            = new TEdit ( this, IDC_AUXTHRESHOLD, EditSizeValue );

TrackZoom               = new TCheckBox ( this, IDC_CHANNELZOOM );
TracksToExclude         = new TEdit ( this, IDC_TOEXCLUDE, EditSizeText ); // this edit can be either filled by the eeg view selection, or manually modified by the user, depending on current state
TracksExcluded          = new TEdit ( this, IDC_EXCLCHANNLIST, EditSizeText );


SetTransferBuffer ( &TAvgCtrlTransfer );
}


        TTracksAveragingControlDialog::~TTracksAveragingControlDialog ()
{
delete  CurrentFile;
delete  TriggerCode;                delete  TriggerPos;             delete  TriggerIndex;
delete  AcceptNumTF;                delete  RejectNumTF;
delete  NumTriggersAccepted;        delete  NumTriggersRejected;
delete  TracksBox;
delete  TrackName;                  delete  TrackMin;               delete  TrackMax;               delete  Threshold;
delete  AuxName;                    delete  AuxMin;                 delete  AuxMax;                 delete  AuxThreshold;
delete  TrackZoom;                  delete  TracksToExclude;        delete  TracksExcluded;
//delete  gauge;
}


void    TTracksAveragingControlDialog::SetupWindow ()
{
TBaseDialog::SetupWindow();

canClose    = false;

Timer       = TTimer ( GetHandle () );
}


void    TTracksAveragingControlDialog::EvTimer ( uint timerId )
{
switch ( timerId ) {
    case TimerRefresh :
                                        // we don't increment nor reset the timer, so it keeps flashing forever
                                        // refresh only if focus is on eeg
        if ( GetFocus () == EegView->GetHandle () && GetIsModal () && EegView ) {

            EegView->GetHighlighted ( &TempSel );

            EegView->GetEEGDoc()->ClearPseudo ( TempSel );

            if ( ! CheckToBool ( TAvgTransfer.CheckAux ) )
                TempSel -= AuxTracks;       // and clear auxiliaries

            TempSel.ToText ( TAvgCtrlTransfer.TracksToExclude, &ElectrodesNames, AuxiliaryTracksNames );

            TransferData ( tdSetData );
            }

        break;
    }
}


void    TTracksAveragingControlDialog::CmButtonPressed ( owlwparam wparam )
{
                                        // once a button pressed, resume the processing where it stopped
EndModal ();

ButtonPressed   = wparam;               // remember the button

canClose        = wparam == IDCANCEL;
}

                                        // prevent exiting
void    TTracksAveragingControlDialog::CmMyOk ()
{
}


void    TTracksAveragingControlDialog::CmAuxThresholdEnable ( TCommandEnabler &tce )
{
tce.Enable ( CheckToBool ( TAvgTransfer.CheckAux ) );
}


void    TTracksAveragingControlDialog::CmAcceptUntilBadEnable ( TCommandEnabler &tce )
{
tce.Enable ( EpochState );
}


void    TTracksAveragingControlDialog::CmRejectUntilOkEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! EpochState );
}


//----------------------------------------------------------------------------
void    TTracksAveragingControlDialog::ProcessGroups ( TGoGoF* gogof, int gofi1, int gofi2, void* usetransfer )
{

TransferData ( tdGetData );

TTracksAveragingStruct&         transfer        = usetransfer ? *(TTracksAveragingStruct *) usetransfer : TAvgTransfer;
TTracksAveragingControlStruct&  transfercontrol = TAvgCtrlTransfer;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need 4 "groups", holding the 4 lists of parameters
if ( gofi2 - gofi1 + 1 != 4 )
    return;

TGoF&               gofeeg          = (*gogof)[ gofi1     ];
TGoF&               gofses          = (*gogof)[ gofi1 + 1 ];
TGoF&               goftva          = (*gogof)[ gofi1 + 2 ];
TGoF&               goftrg          = (*gogof)[ gofi1 + 3 ];

char                buff   [ 64 * KiloByte ];
TFileName           eegfile;
char                seslist[ EditSizeValue ];
TFileName           tvafile;
char                trglist[ EditSizeText ];

                                        // retrieve the filters - ready for use
TTracksFilters<float>&  Filters     = transfer.Filters;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // open first file to retrieve some infos
TOpenDoc<TTracksDoc>    EegDoc;
                                        // no need to show anything at that stage
EegDoc.Open ( gofeeg[ 0 ], OpenDocHidden );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                usefilters          = Filters.HasAnyFilter () && CheckToBool ( transfer.SetFilters    );
bool                filterdata          = usefilters              && CheckToBool ( transfer.FilterData    );
bool                filterdisplay       = usefilters              && CheckToBool ( transfer.FilterDisplay );
bool                anyfilter           = filterdata || filterdisplay;
double              SamplingFrequency   = transfer.SamplingFrequency;   // retrieve the global sampling frequency

bool                CheckEl             = CheckToBool    ( transfer.CheckEl );
double              ElThreshold         = StringToDouble ( transfer.ElThreshold );
bool                CheckAux            = CheckToBool    ( transfer.CheckAux );
double              AuxThreshold        = StringToDouble ( transfer.AuxThreshold );


enum                EpochOriginEnum
                    {
                    EpochOriginFixedTF,
                    EpochOriginTrigger,
                    EpochOriginTriggerOffset,
                    EpochOriginTriggerReactionTime
                    };

EpochOriginEnum     triggerOrigin;
double              triggerOffset       = 0;
int                 trigger1TF          = 0;

if      ( CheckToBool ( transfer.TriggerFixedTF      ) )    triggerOrigin   = EpochOriginFixedTF;
else if ( CheckToBool ( transfer.Trigger             ) )    triggerOrigin   = EpochOriginTrigger;
else if ( CheckToBool ( transfer.TriggerOffset       ) )    triggerOrigin   = EpochOriginTriggerOffset;
else if ( CheckToBool ( transfer.TriggerReactionTime ) )    triggerOrigin   = EpochOriginTriggerReactionTime;
else                                                        triggerOrigin   = EpochOriginTrigger;

if ( triggerOrigin == EpochOriginFixedTF )
    trigger1TF      = StringToInteger ( transfer.TriggerFixedTFValue );
if ( triggerOrigin == EpochOriginTriggerOffset )
    triggerOffset   = StringToDouble ( transfer.TriggerOffsetValue );


MarkerType          markertype          = MarkerTypeUnknown;

if ( CheckToBool ( transfer.UseEvents   ) )     SetFlags ( markertype, MarkerTypeEvent   );
if ( CheckToBool ( transfer.UseTriggers ) )     SetFlags ( markertype, MarkerTypeTrigger );
if ( CheckToBool ( transfer.UseMarkers  ) )     SetFlags ( markertype, MarkerTypeMarker  );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 durationPre         = StringToInteger ( transfer.DurationPre  );
int                 durationPost        = StringToInteger ( transfer.DurationPost );

                                        // check margins, usually for auto-averaging
if ( triggerOrigin == EpochOriginFixedTF ) {
    Clipped ( trigger1TF,   (int) 0, (int) EegDoc->GetNumTimeFrames () - 1          );

    Clipped ( durationPre,  (int) 0, (int) trigger1TF                               );
    Clipped ( durationPost, (int) 1, (int) EegDoc->GetNumTimeFrames () - trigger1TF );
    }


bool                baseLineCorr        = CheckToBool ( transfer.BaselineCorr );
int                 baseLineCorrPre     = baseLineCorr ? StringToInteger ( transfer.BaselineCorrPre  ) : 0;
int                 baseLineCorrPost    = baseLineCorr ? StringToInteger ( transfer.BaselineCorrPost ) : 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Data type & reference

                                        // We have to link datatype and reference: average ref <-> scalar, so that the norm of data corresponds to the GFP, not the RMS
AtomType         	datatype;
if      ( CheckToBool ( transfer.PositiveData   ) )         datatype    = AtomTypePositive;
else if ( CheckToBool ( transfer.SignedData     ) )         datatype    = AtomTypeScalar;
//else if ( CheckToBool ( transfer.VectorData     ) )       datatype    = AtomTypeVector;
else                                                        datatype    = AtomTypeScalar;

                                        // downgrade vectorial type to scalar if not all data are vectorial
bool                allrisv     = gofeeg.AllExtensionsAre ( FILEEXT_RIS, AtomTypeVector );
//if ( IsVector ( datatype ) && ! allrisv )                 datatype    = AtomTypePositive;


bool                ignorepolarity  = CheckToBool ( transfer.IgnorePolarity ) && ! IsAbsolute ( datatype );


ReferenceType       dataref;
if      ( CheckToBool ( transfer.AvgRef ) )                 dataref     = ReferenceAverage;
else /*if ( CheckToBool ( transfer.NoRef ) )*/              dataref     = ReferenceAsInFile;


CheckReference ( dataref, datatype );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SkippingEpochsType  badepochs       = CheckToBool ( transfer.SkipBadEpochs     ) ? SkippingBadEpochsList
                                    :                                              NoSkippingBadEpochs;


char                listbadepochs [ EditSizeText ];
ClearString ( listbadepochs );

if ( badepochs == SkippingBadEpochsList ) {

    StringCopy      ( listbadepochs,    transfer.SkipMarkers );
    StringCleanup   ( listbadepochs );

    if ( StringIsEmpty ( listbadepochs ) )
        badepochs   = NoSkippingBadEpochs;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                acceptall           = CheckToBool ( transfer.AcceptAll );
                                        // not sure it makes sense to baseline correct for FFT Approx, which is already computed with average ref...
                                        // otherwise, browse the code part, and revert polarities BEFORE doing the baseline correction
if ( baseLineCorrPost == baseLineCorrPre || ignorepolarity )
                    baseLineCorr        = false;

bool                MergeTriggers       = CheckToBool ( transfer.MergeTriggers );
bool                SplitTriggers       = CheckToBool ( transfer.SplitTriggers ) && triggerOrigin != EpochOriginFixedTF;
bool                SaveAverage         = CheckToBool ( transfer.SaveAverage );
bool                SaveSum             = CheckToBool ( transfer.SaveSum );
bool                SaveSD              = CheckToBool ( transfer.SaveSD );
bool                SaveSE              = CheckToBool ( transfer.SaveSE );
bool                SaveEpochsMultiFiles= CheckToBool ( transfer.SaveEpochsMultiFiles );
bool                SaveEpochsSingleFile= CheckToBool ( transfer.SaveEpochsSingleFile );


bool                usexyz              = StringIsNotEmpty ( transfer.XyzDocFile );
bool                saveexcl            = CheckToBool ( transfer.SaveExcluded );
bool                saveaux             = CheckToBool ( transfer.SaveAux );
                                        // there is a problem if averaging more than 1 file
                                        // excluded channels are not saved, and the epochs are concatenated:
                                        // epochs from second file can have different # of channels
if ( SaveEpochsSingleFile && ! saveexcl && (int) gofeeg > 1 )
//                  saveexcl                = true;
                    SaveEpochsSingleFile    = false;    // cancel epochs in single file seems the less harmful

bool                SaveEpochs          = SaveEpochsMultiFiles || SaveEpochsSingleFile;

bool                openauto            = CheckToBool ( transfer.OpenAuto ) && ( ! IsFreqLoop () || IsLastFreq () );

bool                writeoriginmrk      = CheckToBool ( transfer.OriginMarker );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           BaseDir;
TFileName           BaseFileName;
TFileName           GroupBaseFileName;
TFileName           EpochsDir;
TFileName           TriggerFileName;
TFileName           fileoutprefix;
TFileName           fileoutext;
TFileName           fileoutavg;
TFileName           fileoutsum;
TFileName           fileoutsd;
TFileName           fileoutse;
TFileName           fileoutepochs;
TFileName           fileout1epoch;
TFileName           fileoutvrb;
TFileName           fileoutxyz;
TFileName           fileouttva;
TFileName           fileout1epochmrk;
TFileName           fileoutsplit;
TFileName           fileoutsplitmrk;
TFileName           fileoutmrk;
TFileName           filefiltered;
TFileName           file;


TSplitStrings       alltriggers;
TSplitStrings       triggerlist;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // choose file type
//int                 filetype;
int                 presetsindex    = transfer.FileTypes.GetSelIndex ();


//if      ( SamplingFrequency <= 0        // override all to EP if no sampling frequency found
//       || presetsindex == PresetFileTypeEp   )  filetype = IDC_FILETYPE_EP;
//else if ( presetsindex == PresetFileTypeEph )   filetype = IDC_FILETYPE_EPH;
//else if ( presetsindex == PresetFileTypeTxt )   filetype = IDC_FILETYPE_TXT;
//else if ( presetsindex == PresetFileTypeSef )   filetype = IDC_FILETYPE_SEF;
//else if ( presetsindex == PresetFileTypeBV  )   filetype = IDC_FILETYPE_BV;
//else if ( presetsindex == PresetFileTypeEdf )   filetype = IDC_FILETYPE_EDF;
//else if ( presetsindex == PresetFileTypeRis )   filetype = IDC_FILETYPE_RIS;
//else                                            filetype = IDC_FILETYPE_BV;

if ( SamplingFrequency <= 0 )           // override all to EP if no sampling frequency found
    presetsindex    = PresetFileTypeEp;

if ( IsFreqLoop () )
//  filetype        = IDC_FILETYPE_SEF; // force binary & faster files
    presetsindex    = PresetFileTypeSef;// force binary & faster files


StringCopy ( fileoutext, SavingEegFileExtPreset[ presetsindex ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

StringCopy  ( BaseDir,                  transfer.BaseFileName );

                                        // extract string "prefix"
StringCopy  ( fileoutprefix,            ToFileName ( BaseDir ) );

                                        // compose path access and main prefix "full path\prefix\prefix"
StringCopy  ( BaseFileName,             BaseDir,    "\\",   fileoutprefix );

                                        // save the global base file name, without frequency
StringCopy  ( GroupBaseFileName, BaseFileName );

                                        // add only to the file name (not the directory!) the frequency part
if ( IsFreqLoop () ) {

    TFreqDoc*       FreqDoc     = dynamic_cast< TFreqDoc * > ( CartoolDocManager->OpenDoc ( gofeeg.GetFirst (), dtOpenOptionsNoView ) );

    if ( FreqDoc ) {
        StringCopy ( FreqName, FreqDoc->GetFrequencyName ( FreqIndex ) );

        StringAppend    ( BaseFileName, ".", FreqName );

        FreqType    = FreqDoc->GetFreqType ();
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // create main output directory "full path\prefix\"
//NukeDirectory   ( BaseDir );    // DON'T! if the user wrongly specify a high directory, everything will be deleted! (and don't search in the Recycle Bin)

CreatePath ( BaseDir, false );


                                        // delete any previous files, with my prefix only!
StringCopy      ( buff,         BaseFileName,   ".*" );
DeleteFiles     ( buff );

                                        // clean-up from previous computation
StringCopy      ( buff,         BaseDir,        "\\" InfixEpochs );
NukeDirectory   ( buff );

                                        // clean-up all directories starting with..
if ( ! IsFreqLoop () || IsFirstFreq () ) {
    StringCopy      ( buff,         BaseDir,    "\\" InfixTrigger " *" );
    NukeDirectory   ( buff );
    }

                                        // create directory for any sort of epochs
if ( SaveEpochs ) {
    StringCopy  ( EpochsDir,        BaseDir,    "\\" InfixEpochs );

    if ( IsFreqLoop () )
        StringAppend    ( EpochsDir, " ", FreqName );

    CreatePath ( EpochsDir, false );
    }
else
   ClearString ( EpochsDir );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // generate as much filenames as possible
StringCopy  ( fileoutavg,       BaseFileName,                                                   ".",    fileoutext );
StringCopy  ( fileoutsum,       BaseFileName,           "." InfixSum " (#epochs)",              ".",    fileoutext );
StringCopy  ( fileout1epoch,    EpochsDir,              "\\", fileoutprefix,    "." InfixEpochs ".",  /*FILEEXT_EEGSEF*/ fileoutext );
StringCopy  ( fileout1epochmrk, fileout1epoch,                                                  ".",    FILEEXT_MRK );
StringCopy  ( fileoutsplit,     BaseFileName,           "." InfixSplit,                         ".",    FILEEXT_RIS );
StringCopy  ( fileoutsplitmrk,  fileoutsplit,                                                   ".",    FILEEXT_MRK );

StringCopy  ( fileoutvrb,       fileoutavg,                                                     ".",    FILEEXT_VRB );
StringCopy  ( fileoutxyz,       BaseFileName,           "." InfixExcl,                          ".",    usexyz ? ToExtension ( transfer.XyzDocFile ) : FILEEXT_XYZ );

StringCopy  ( fileoutsd,        BaseFileName,           "." InfixSD,                            ".",    fileoutext );
StringCopy  ( fileoutse,        BaseFileName,           "." InfixSE,                            ".",    fileoutext );


                                        // can create all requested files?
if ( SaveAverage          && MergeTriggers  && ! CanOpenFile ( fileoutavg,          CanOpenFileWriteAndAsk )
  || SaveSum              && MergeTriggers  && ! CanOpenFile ( fileoutsum,          CanOpenFileWrite       )
//|| SaveEpochsMultiFiles                   && ! CanOpenFile ( fileoutepochs,       CanOpenFileWriteAndAsk ) 
  || SaveEpochsSingleFile                   && ! CanOpenFile ( fileout1epoch,       CanOpenFileWrite       )
  || SaveSD               && MergeTriggers  && ! CanOpenFile ( fileoutsd,           CanOpenFileWrite       )
  || SaveSE               && MergeTriggers  && ! CanOpenFile ( fileoutse,           CanOpenFileWrite       )
  ||                                           ! CanOpenFile ( fileoutvrb,          CanOpenFileWrite       )
  || usexyz                                 && ! CanOpenFile ( fileoutxyz,          CanOpenFileWrite       )
  || usexyz                                 && ! CanOpenFile ( transfer.XyzDocFile, CanOpenFileReadAndAsk  ) )

    TBaseDialog::CmCancel ();


                                        // test the output filename is not one of the file to process
bool                foutok;

do {
    foutok = true;

    for ( int eegi = 0; eegi < (int) gofeeg; eegi++ )  {

        if ( StringIs ( gofeeg[ eegi ], fileoutavg ) || StringIs ( gofeeg[ eegi ], fileoutsum ) ) {
            foutok = false;
            break;
            }
        }

    if ( ! foutok )
        CanOpenFile ( fileoutavg, CanOpenFileWriteAndAsk );

    } while ( !foutok );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TExportTracks       exp1epoch;
ofstream            of1epochmrk;

TExportTracks       expsplit;
ofstream            ofsplitmrk;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // main variables
int                 numElectrodes;
long                numTimeFrames;
int                 totalChannels;
int                 filtermargin;
int                 totalnumtriggacc    = 0;
int                 totalnumtriggrej    = 0;
int                 totalnumtrigg       = 0;
int                 currchannel;
double              samplingfrequency;
bool                closexyz;


TElectrodesDoc*     XyzDoc;
TDynamicLinkManyDoc*LmDoc;
TElectrodesView*    XyzView;
//TTracksView*      EegView;            // EegView is global, so it can be accessed from EvTimer for tracks selection updates
TPotentialsView*    PotentialsView;


//ReadFromHeader ( gofeeg[ 0 ], ReadSamplingFrequency,    &samplingfrequency );
                                        // We have this file open already
samplingfrequency   = EegDoc->GetSamplingFrequency ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // !ignoring the 0 TF!
int                 dataLength      = durationPre + durationPost;
int                 dataTotal       = dataLength;


if ( baseLineCorrPre  > 0 )     baseLineCorrPre--;
if ( baseLineCorrPost < 0 )     baseLineCorrPost++;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // is the baseline further left than the pre-stimulus?
                                        // !baseline is negative (relative) to the left, while duration pre is positive (absolute) to the left!
int                 deltaleftbaseline   = baseLineCorr                              ? - baseLineCorrPre - durationPre : 0;

                                        // bad epochs inflation
int                 deltabadepochs      = badepochs == SkippingBadEpochsList        ? Round ( MillisecondsToTimeFrame ( BadEpochExtraMarginMilliseconds, samplingfrequency ) ) : 0;

                                        // in case of high-pass, we add an extra bit of testing (on each side)
double              deltafilterms       = filterdata && Filters.ButterworthHigh > 0 ? FrequencyToMilliseconds ( Filters.ButterworthHigh ) * HighPassAdditionalMargin : 0;
int                 deltafiltertf       = filterdata && deltafilterms > 0           ? Round ( MillisecondsToTimeFrame ( deltafilterms, samplingfrequency ) )         : 0;

                                        // max of all possible extensions
int                 deltaleft           = max ( 0, deltaleftbaseline, /*deltabadepochs,*/ deltafiltertf );  // deltabadepochs is not used to read data, only for interval testing


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // buffer offset to get the first TF of data
int                 offsetData          = deltaleft;
                                        // expanding to the left
                    dataTotal          += deltaleft;


                                        // buffer offset to the first TF of baseline
int                 offsetBLCMin        = ( durationPre + offsetData ) + baseLineCorrPre;

                                        // buffer offset to testing part
int                 offsetTestMin       =                 offsetData   - deltafiltertf;
                                        // extend testing to the baseline - a bad baseline is also bad epoch...
                    offsetTestMin       = min ( offsetTestMin, offsetBLCMin );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // is the baseline further right than the post-stimulus (weird case in real life, but still)?
int                 deltarightbaseline  = baseLineCorr                              ? baseLineCorrPost - durationPost + 1 : 0;

                                        // max of all possible extensions
int                 deltaright          = max ( 0, deltarightbaseline, /*deltabadepochs,*/ deltafiltertf ); // deltabadepochs is not used to read data, only for interval testing


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // expanding on the right
                    dataTotal          += deltaright;


                                        // buffer offset to the last  TF of baseline
int                 offsetBLCMax        = ( durationPre + offsetData ) + baseLineCorrPost;

                                        // buffer offset to testing part
int                 offsetTestMax       = ( durationPre + offsetData ) + durationPost + deltafiltertf - 1;
                                        // extend testing to the baseline - a bad baseline is also bad epoch...
                    offsetTestMax       = max ( offsetTestMax, offsetBLCMax );

                                        // finally, we have the range of the baseline
int                 numBLCTF            = offsetBLCMax - offsetBLCMin + 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // the whole pre-trigger interval to be read, including pre-stimulus and baseline
int                 pretrigger          = durationPre + offsetData;
                                        // the whole post-trigger interval to be read, including post-stimulus and baseline
int                 posttrigger         = ( dataTotal - 1 ) - pretrigger;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // could also increase borders for Envelope filter
numElectrodes   = EegDoc->GetNumElectrodes ();
totalChannels   = EegDoc->GetTotalElectrodes ();
filtermargin    = 0; // Filters.GetSafeMargin ();   // in case of filtering, file will fully pre-processed at once and saved to a temp file - we don't need margins anymore


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // choose ordered as there are many operations between these TSelection
                                        // which would give unpredictable results would they be unordered
TSelection          CheckingExcluded ( totalChannels, OrderSorted );
TSelection          DisplayTracks    ( totalChannels, OrderSorted );
TSelection          FocusOn          ( totalChannels, OrderSorted );
TSelection          OutputTracks;
TSelection          CorrelationSel;

AuxTracks           = TSelection     ( totalChannels, OrderSorted );
TempSel             = TSelection     ( totalChannels, OrderSorted );

if ( ignorepolarity )                   // we MUST be very cautious to handle only clean & meaningful tracks to compute the Correlation
    CorrelationSel  = TSelection     ( totalChannels, OrderSorted );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // allocate calculus buffers, plus extra space for filters
TArray2<float>      DataEeg     ( totalChannels,    dataTotal + 2 * filtermargin );
TArray2<double>     Sum         ( numElectrodes,    dataTotal + 2 * filtermargin );
TArray2<double>     SumSqr      ( numElectrodes,    dataTotal + 2 * filtermargin );
TArray2<double>     TotalSum    ( numElectrodes,    dataTotal + 2 * filtermargin );
TArray2<double>     TotalSumSqr ( numElectrodes,    dataTotal + 2 * filtermargin );
TMaps               PolarityRef;
TMap                Map;


                                        // clear the total buffers
TotalSum   .ResetMemory ();
TotalSumSqr.ResetMemory ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // main gauge bar
TGauge*             gauge           = 0;


IntegerToString ( transfercontrol.AcceptNumTF, SamplingFrequency > 0 ? (int) ( 2 * SamplingFrequency ) : 1000 );
StringCopy      ( transfercontrol.RejectNumTF, transfercontrol.AcceptNumTF );
//transfercontrol.TrackZoom   = BoolToCheck ( false );

if ( StringIsNotEmpty ( transfer.ElExcluded ) )
    StringCopy  ( transfercontrol.TracksExcluded, transfer.ElExcluded, " " );
else
    ClearString ( transfercontrol.TracksExcluded );


TransferData ( tdSetData );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

auto    CookTvaFileName = [] ( const char* basefilename, const char* eegfilename, const char* session, const char* triggerlist )
{
TFileName           tvafile;

StringCopy      ( tvafile,      basefilename );
StringAppend    ( tvafile, ".", TFileName ( eegfilename ).GetFilename () );

if ( StringToInteger ( session ) > 0 )
    StringAppend( tvafile, ".", session );

char            buff[ KiloByte ];
StringAppend    ( tvafile, ".", TSplitStrings ( triggerlist, UniqueStrings ).ToString ( buff, CompactString ) );
AddExtension    ( tvafile, FILEEXT_TVA );

                                        // We don't want to overwrite any existing file
CheckNoOverwrite( tvafile );

return  tvafile;
};


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // verbose file init
TVerboseFile    verbose ( fileoutvrb, VerboseFileDefaultWidth );

verbose.PutTitle ( "Averaging Files" );


verbose.NextTopic ( "Input File(s):" );
{
verbose.Put ( "Number of input file(s):", (int) gofeeg );

for ( int eegi = 0; eegi < (int) gofeeg; eegi++ )  {

    verbose.NextLine ();
    verbose.Put ( "File #    :", eegi + 1 );
    verbose.Put ( "File      :", gofeeg[ eegi ] );
    verbose.Put ( "Session   :", StringToInteger ( gofses[ eegi ] ) > 0 ? gofses[ eegi ] : "None, using whole file" );
    verbose.Put ( "TVA file  :", StringIsEmpty ( goftva[ eegi ] ) ? "None" : goftva[ eegi ] );

    triggerlist.Set ( goftrg[ eegi ], UniqueStrings );
    verbose.Put ( "Trigger(s):", triggerlist.ToString ( buff, ExpandedString ) );

    if ( IsFreqLoop () ) {
        verbose.NextLine ();
        verbose.Put ( "Frequency :", FreqName );
        }
    }

verbose.NextLine ();
verbose.Put ( "Electrodes coordinates file:", usexyz                                            ? transfer.XyzDocFile 
                                            : StringIsNotEmpty ( Filters.FiltersParam.XyzFile ) ? Filters.FiltersParam.XyzFile 
                                            :                                                     "None" );
}


verbose.NextTopic ( "Output Files:" );
{
//verbose.Put ( "Output file type:", IsFreqLoop () ? FILEEXT_FREQ : fileoutext );
verbose.Put ( "Output file type:", fileoutext );
verbose.NextLine ();

verbose.Put ( "Merge all triggers into a single file:", MergeTriggers );
verbose.Put ( "Split triggers into separate files:", SplitTriggers );
verbose.NextLine ();

verbose.Put ( "Excluded  channels written to file:", saveexcl );
verbose.Put ( "Auxiliary channels written to file:", saveaux );

verbose.Put ( "Add marker to epoch origin:", writeoriginmrk );
verbose.NextLine ();


verbose.Put ( "Save average:", SaveAverage );
verbose.Put ( "Save sum:", SaveSum );
verbose.Put ( "Save standard deviation:", SaveSD );
verbose.Put ( "Save standard error:",     SaveSE );
verbose.Put ( "Save epochs in separate files:", SaveEpochsMultiFiles );
verbose.Put ( "Save epochs in 1 file:", SaveEpochsSingleFile );
verbose.NextLine ();


if ( MergeTriggers && SaveAverage )
    verbose.Put ( "Merged file    - Average:", fileoutavg );
if ( MergeTriggers && SaveSum )
    verbose.Put ( "Merged file    - Sum:",     fileoutsum );

if ( MergeTriggers && SaveSD )
    verbose.Put ( "Merged file    - Standard Deviation:", fileoutsd );
if ( MergeTriggers && SaveSE )
    verbose.Put ( "Merged file    - Standard Error:",     fileoutse );

if ( MergeTriggers )
    verbose.NextLine ();


if ( SplitTriggers ) {
                                        // sub-directory for split triggers - at that point, we don't know the fill path names
    TriggerFileName     = BaseDir + "\\" InfixTrigger " *" "\\";

    if ( IsFreqLoop () )    TriggerFileName    += fileoutprefix + "." + FreqName + "." InfixTrigger " *";
    else                    TriggerFileName    += fileoutprefix                  + "." InfixTrigger " *";


    if ( SaveAverage ) {
        file    = TriggerFileName + "." + fileoutext;

        verbose.Put ( "Split  file(s) - Average:", file );
        }

    if ( SaveSum ) {
        if ( IsFreqLoop () )    file    = TriggerFileName + "." + InfixSum                + "." + fileoutext;
        else                    file    = TriggerFileName + "." + InfixSum + " (#epochs)" + "." + fileoutext;

        verbose.Put ( "Split  file(s) - Sum:", file );
        }

    if ( SaveSD ) {
        file    = TriggerFileName + "." InfixSD "." + fileoutext;

        verbose.Put ( "Split  file(s) - Standard Deviation:", file );
        }

    if ( SaveSE ) {
        file    = TriggerFileName + "." InfixSE "." + fileoutext;

        verbose.Put ( "Split  file(s) - Standard Error:", file );
        }

    verbose.NextLine ();
    } // SplitTriggers


if ( SaveEpochsMultiFiles ) {
    sprintf ( buff, "%s\\%s." InfixEpoch " *.%s", (char*) EpochsDir, (char*) fileoutprefix, /*FILEEXT_EEGSEF*/ (char*) fileoutext );
    verbose.Put ( "Epochs outputed in separate files:", buff );
    }
if ( SaveEpochsSingleFile )
    verbose.Put ( "Epochs outputed into a single file:", fileout1epoch );

if ( SaveEpochs )
    verbose.NextLine ();


if ( usexyz )
    verbose.Put ( "New electrodes coordinates file:", fileoutxyz );
verbose.Put ( "Verbose file (this):", fileoutvrb );
verbose.NextLine ();

if ( ! acceptall ) {

    for ( int eegi = 0; eegi < (int) gofeeg; eegi++ )  {
                                        // Will not return the proper, non-overwriting file name, because files don't really exist here...
        fileouttva  = CookTvaFileName ( BaseFileName, gofeeg[ eegi ], gofses[ eegi ], goftrg[ eegi ] );

        verbose.Put ( eegi == 0 ? "Trigger validation file(s):" : "", fileouttva );
        }
    }
}


verbose.NextTopic ( "Channel Checking:" );
{
verbose.Put ( "Regular tracks tested:", CheckEl );
if ( CheckEl )
    verbose.Put ( "Regular tracks threshold:", ElThreshold, 2 );

verbose.Put ( "Channels excluded:", StringIsEmpty ( transfer.ElExcluded ) ? "None, all channels are tested" : transfer.ElExcluded );


verbose.NextLine ();
verbose.Put ( "Auxiliary tracks tested:", CheckAux );
if ( CheckAux )
    verbose.Put ( "Auxiliary tracks threshold:", AuxThreshold, 2 );


verbose.Put ( "Auxiliary tracks override:", StringIsEmpty ( transfer.AuxOverride ) ? "None, keeping original auxiliary tracks" : transfer.AuxOverride );


if ( deltafilterms > 0 ) {
    verbose.NextLine ();
    verbose.Put ( "Additional margin from high-pass:", deltafilterms, 2, "[ms]" );
    }
}


verbose.NextTopic ( "Time Interval:" );
{
if      ( triggerOrigin == EpochOriginFixedTF               )   verbose.Put ( "Epoch origin is:", "TF ", trigger1TF );
else if ( triggerOrigin == EpochOriginTrigger               )   verbose.Put ( "Epoch origin is:", "Trigger" );
else if ( triggerOrigin == EpochOriginTriggerOffset         )   verbose.Put ( "Epoch origin is:", "Trigger + ", triggerOffset, " [TF]" );
else if ( triggerOrigin == EpochOriginTriggerReactionTime   )   verbose.Put ( "Epoch origin is:", "Trigger + Reaction Time" );

verbose.Put ( "Duration Pre   in [TF]:", durationPre );
verbose.Put ( "Duration Post  in [TF]:", durationPost );
verbose.Put ( "Duration Total in [TF]:", transfer.DurationTotal );
//verbose.Put ( "Duration Total in [TF]:", durationPre + durationPost );

if      ( triggerOrigin != EpochOriginFixedTF ) {

    ClearString ( buff );

    if ( IsFlag ( markertype, MarkerTypeEvent   ) )     StringAppend ( buff, "Events",   " " );
    if ( IsFlag ( markertype, MarkerTypeTrigger ) )     StringAppend ( buff, "Triggers", " " );
    if ( IsFlag ( markertype, MarkerTypeMarker  ) )     StringAppend ( buff, "Markers",  " " );

    verbose.Put ( "Type of triggers:", buff );
    }
}


verbose.NextTopic ( "Data Type:" );
{
if      ( datatype == AtomTypeScalar   )    verbose.Put ( "Data type:", "Signed Data" );
else if ( datatype == AtomTypePositive )    verbose.Put ( "Data type:", allrisv ? "Positive Data (Norm of Vectors)" : "Positive Data" );
//else if ( datatype == AtomTypeVector   )    verbose.Put ( "Data type:", "3D Vectorial Data" );

verbose.Put ( "Data reference:", dataref == ReferenceAverage ? "Average Reference" : "None" );

verbose.Put ( "Maps / Templates Polarity:", ( datatype == AtomTypePositive || datatype == AtomTypeVector ) ? "Not relevant" : ignorepolarity ? "Ignore" : "Account" );
}


verbose.NextTopic ( "Filtering:" );
{
if ( filterdata ) {
    verbose.Put ( "Filters:", Filters.ParametersToText ( buff ) );
//  *ofv << "Filters are applied:                    before summation" fastendl;
    verbose.Put ( "Filter data (before summation):", filterdata );
    verbose.Put ( "Filter display:", filterdisplay );
    }
else
    verbose.Put ( "Filters:", false );
}


verbose.NextTopic ( "Baseline Correction:" );
{
verbose.Put ( "Baseline correction:", baseLineCorr );
if ( baseLineCorr ) {
    verbose.Put ( "Inferior limit (from origin) in [TF]:", baseLineCorrPre  );
    verbose.Put ( "Superior limit (from origin) in [TF]:", baseLineCorrPost );
    }
}


verbose.NextTopic ( "Accepting / Rejecting Epochs:" );
{
verbose.Put ( "Excluding Bad Epochs:", SkippingEpochsNames[ badepochs ] );

if ( badepochs == SkippingBadEpochsList ) {
    verbose.Put ( "Skipping markers:", listbadepochs );
    verbose.Put ( "Additional margin to bad epochs:", BadEpochExtraMarginMilliseconds, 2, "[ms]" );
    }

verbose.NextLine ();
verbose.Put ( "Automatically accept all epochs/files:", acceptall );
}


verbose.NextTopic ( "Processing Summary:" );
{
(ofstream&) verbose << "Data are processed according to this sequence (& depending on user's choice):"                      fastendl;
(ofstream&) verbose <<                                                                                                      fastendl;
(ofstream&) verbose << "    * Reading data from file"                                                                       fastendl;
(ofstream&) verbose << "    * Filtering"                                                                                    fastendl;
(ofstream&) verbose << "    * Applying average reference"                                                                   fastendl;
(ofstream&) verbose << "    * Applying baseline correction"                                                                 fastendl;
(ofstream&) verbose << "    * Testing data against threshold, and getting user feedback for epoch acceptation or rejection" fastendl;
(ofstream&) verbose << "    * Rescaling data levels"                                                                        fastendl;
(ofstream&) verbose << "    * Optionally reversing polarity of maps (for spontaneous EEG)"                                  fastendl;
(ofstream&) verbose << "    * Saving current epoch to file"                                                                 fastendl;
(ofstream&) verbose << "    * Cumulating data into current sum"                                                             fastendl;
(ofstream&) verbose << "    * Dividing the whole sum by the number of accepted epochs"                                      fastendl;
}


verbose.Flush ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // close this eeg - actually, don't, we are fine with the hidden doc now
//EegDoc.Close ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // open coordinates
if ( usexyz ) {

//  closexyz    = ! CartoolDocManager->IsOpen ( transfer.XyzDocFile ); // if already open, do not close it at the end
    closexyz    = true;

    XyzDoc  = dynamic_cast< TElectrodesDoc* > ( CartoolDocManager->OpenDoc ( transfer.XyzDocFile, dtOpenOptions ) );

    if ( ! XyzDoc ) {
        verbose.NextLine ();
        verbose.NextBlock ();
        verbose.Put ( "Error! Can not open file:", transfer.XyzDocFile );
        }

//  XyzDoc->AddLink ( (TDocument *) this ); // not correct but should work
    XyzDoc->PreventClosing ();

    XyzView = dynamic_cast< TElectrodesView* > ( XyzDoc->GetViewList() );

    if ( XyzView == 0 ) {

        XyzView     = new TElectrodesView ( *XyzDoc );

        CartoolDocManager->PostEvent ( dnCreate, *XyzView );
        }

//  XyzDoc->GetViewList()->GetParentO()->SetFocus();
    XyzView->GetParentO()->SetFocus();
    CartoolMdiClient->CmDocWinAction ( CM_DOCWIN_MIN );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

enum            {   AutoNone        = 0,
                    AutoAcceptCond,
                    AutoAcceptUntilBad,
                    AutoAcceptAll,
                    AutoRejectUntilOk,
                };

int                 automode        = acceptall ? AutoAcceptAll : AutoNone;
bool                firsteeg;
bool                lasteeg;

                                        // process each eeg
for ( int eegi = 0; eegi < (int) gofeeg; eegi++ )  {

    firsteeg    = eegi == 0;
    lasteeg     = eegi == (int) gofeeg - 1;

                                        // extract current files names
    StringCopy  ( eegfile, gofeeg[ eegi ] );
    StringCopy  ( seslist, gofses[ eegi ] );
    StringCopy  ( tvafile, goftva[ eegi ] );
    StringCopy  ( trglist, goftrg[ eegi ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Open hidden doc - will be OK for the already opened first file, too
    EegDoc.Open ( eegfile, OpenDocHidden );

                                        // troubles opening?
    if ( ! EegDoc.IsOpen () ) {

        verbose.NextLine ();
        verbose.NextBlock ();
        verbose.Put ( "Error! Can not open file:", eegfile );
        continue;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // if specified, try to set it or ask for correct range
                                        // else, returns if no sessions, or try to ask for session
                                        // results are 0 for nothing, or 1..n
    int             session     = StringToInteger ( seslist );

    if ( session > 0 ) {

        EegDoc.Unlock ();
                                        // changing session means closing and re-opening
        EegDoc->GoToSession ( session );

        EegDoc.Lock ();
        }

    UpdateApplication;
                                        // retrieving current value anyway
    session     = EegDoc->GetCurrentSession ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // switching frequency?
    if ( IsFreqLoop () ) {

        TFreqDoc*       FreqDoc     = dynamic_cast< TFreqDoc * > ( EegDoc.GetDoc () );

        if ( FreqDoc )
            FreqDoc->SetCurrentFrequency ( FreqIndex, true );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Filtering is done once and for all, then saved into a temp file, so we don't have to deal with filters anymore
    if ( anyfilter ) {

                                        // try to set a sampling frequency, if needed
        if ( EegDoc->GetSamplingFrequency() <= 0 && SamplingFrequency > 0 )

            EegDoc->SetSamplingFrequency ( SamplingFrequency );


        EegDoc->SetFilters      ( &Filters );

        EegDoc->ActivateFilters ( true );
//                                        // we could also process the ref here, instead of through GetTracks - !tricky: we now have 2 versions of file, which can be differently hard-reference!
//      EegDoc->SetReferenceType ( dataref );

                                        // New temp file name - including session #
        StringCopy          ( filefiltered, BaseFileName );
        if ( session > 0 )
            StringAppend    ( filefiltered, "." "Session", IntegerToString ( session ) );
        StringAppend        ( filefiltered, "." "Filtered" );
        AddExtension        ( filefiltered, FILEEXT_EEGBV );    // !using a file type that handles native triggers, so the saved file will totally mimick the original one!


        EegDoc->SetDocPath  ( filefiltered );
                                        // Saving to file - From TTracksDoc, this will use the current atom type, reference and filters
        EegDoc->Commit      ( true );

        EegDoc.Close        ();

        EegDoc.Open         ( filterdisplay ? filefiltered : gofeeg[ eegi ], OpenDocHidden /*OpenDocVisibleLocked*/ );

//      dataref             = ReferenceAsInFile;  // if already processed
        }

                                        // !revoking any existing filter: either off, or already a filtered temp file!
    EegDoc->DeactivateFilters ( true );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Open view
    EegView     = dynamic_cast<TTracksView*> ( EegDoc->GetViewList () );

                                        // in case we opened the EEG in hidden mode, this is time to step forward and create a view!
    if ( EegView == 0 ) {

        EegView     = new TTracksView ( *EegDoc );

        CartoolDocManager->PostEvent ( dnCreate, *EegView );
        }


    if ( automode == AutoAcceptAll )

        EegView->WindowHide ();

    UpdateApplication;

                                        // now, we know the window's Id
    EegView->LinkedViewId   = EegView->GetViewId ();

    FocusOn      .SentTo    = 0;
    FocusOn      .SentFrom  = EegView->GetViewId ();

    DisplayTracks.SentTo    = 0;
    DisplayTracks.SentFrom  = EegView->GetViewId ();

                                        // now, we lock everything
    EegDoc.Lock ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set correct reference
//  EegDoc->SetReferenceType ( dataref );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    samplingfrequency       = EegDoc->GetSamplingFrequency ();
    numTimeFrames           = EegDoc->GetNumTimeFrames ();


    if ( ElectrodesNames.IsEmpty () )
        ElectrodesNames     =  *EegDoc->GetElectrodesNames ();

                                        // load aux or override?
    if ( StringIsNotEmpty ( transfer.AuxOverride ) ) {

        if ( firsteeg ) {               // a priori, do it once only
            AuxTracks.Reset ();
            AuxTracks.Set ( transfer.AuxOverride, &ElectrodesNames );

//          AuxTracks.Reset ( numElectrodes, EegDoc->GetTotalElectrodes() - 1 );  // let allow checking pseudos? useful trick?
            }

        EegDoc->SetAuxTracks ( &AuxTracks );
        }
    else
        AuxTracks               = EegDoc->GetAuxTracks ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//  EegView->ShowNow ();

    UpdateApplication;

    EegView->SetMarkerType ( CombineFlags ( markertype, MarkerTypeTemp ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // split triggers list into structure
    triggerlist.Set ( trglist, UniqueStrings );

                                        // retrieve marker names
    TStrings            markernames;

    EegDoc->GetMarkerNames ( markernames, markertype );

                                        // also expand the list to real names, in case of '*'
    triggerlist.ExpandWildchars ( markernames, UniqueStrings );

                                        // keep only existing triggers!
    triggerlist.FilterWith ( markernames, Silent );

                                        // cumulate all real triggers
    alltriggers.Add ( triggerlist, UniqueStrings );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // cursor to update the eeg view
    TTFCursor           doctfcursor ( EegDoc, 0, numTimeFrames - 1 );
    doctfcursor.SentTo    = EegView->GetViewId ();
    doctfcursor.SentFrom  = EegView->GetViewId ();

                                        // first set of excluded channels, do it only 1 time
    if ( firsteeg ) {
                                        // put in non-aux, non-pseudos, that are to be not tested
        CheckingExcluded.Reset ();

        CheckingExcluded.Set ( transfer.ElExcluded, &ElectrodesNames );

        EegDoc->ClearPseudo  ( CheckingExcluded );

        if ( ! CheckAux )
            CheckingExcluded -= AuxTracks;  // and clear auxiliaries

                                        // nicely reformat the string
        CheckingExcluded.ToText ( transfercontrol.TracksExcluded, &ElectrodesNames, AuxiliaryTracksNames );

        EegDoc->SetBadTracks ( &CheckingExcluded );


        if ( ignorepolarity ) {         // exclude everything suspected of being polluted
            CorrelationSel.Set ();
            EegDoc->ClearPseudo ( CorrelationSel );
            CorrelationSel  -= CheckingExcluded;
            CorrelationSel  -= AuxTracks;
            }
        }

                                        // show selected tracks
    FocusOn.Reset();

    DisplayTracks.Set();
                                        // always remove the pseudos
    EegDoc->ClearPseudo ( DisplayTracks );

    DisplayTracks  += AuxTracks;        // but in case the auxs were among the pseudos...

    DisplayTracks  -= CheckingExcluded; // remove excluded


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // first setup for electrodes
    if ( usexyz && firsteeg && (bool) CheckingExcluded ) {
        TSelection      tempsel ( ~DisplayTracks );

        EegDoc->ClearPseudo ( tempsel );

        XyzDoc->SetBadElectrodes ( tempsel );
        }

    EegView->VnNewSelection ( &DisplayTracks );

                                        // create the potential map ?
    if ( usexyz ) {
                                        // now, create a link and a view
        LmDoc   = new TDynamicLinkManyDoc ( EegDoc, XyzDoc );

        LmDoc->LastEegViewId  = EegView->GetViewId ();

        PotentialsView      = new TPotentialsView ( *EegDoc, 0, LmDoc );

        CartoolDocManager->PostEvent ( dnCreate, *PotentialsView );

                                        //  the 2 views accept the same messages
        EegView->SetFriendView ( PotentialsView );
                                        // setup appearance
                                        // show big electrodes
        PotentialsView->EvCommand ( IDB_SHOWELEC,      0, 0 );
        PotentialsView->EvCommand ( IDB_SHOWELEC,      0, 0 );

        PotentialsView->EvCommand ( IDB_FLATVIEW,      0, 0 );
        PotentialsView->EvCommand ( IDB_FIXEDSCALE,    0, 0 );
                                        // lowering contrast makes the view clearer with raw data
        PotentialsView->EvCommand ( IDB_SPDECCONTRAST, 0, 0 );
        PotentialsView->EvCommand ( IDB_SPDECCONTRAST, 0, 0 );
        PotentialsView->EvCommand ( IDB_SPDECCONTRAST, 0, 0 );

                                        // prevent closing the potential map !!
//      LmDoc->AddLink ( EegDoc );
        LmDoc->PreventClosing ();
        } // if usexyz


    FocusOn      .SentTo    = EegView->FriendshipId;
    DisplayTracks.SentTo    = EegView->FriendshipId;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    int                 ctdlgw              = GetWindowWidth  ( this );
    int                 ctdlgh              = GetWindowHeight ( this );
    int                 ctdlgx              = GetWindowLeft   ( this );
//  int                 ctdlgy              = GetWindowTop    ( this );

                                        // The progress bar, not being part of the dialog, needs to be "attached" to it manually
                                        // We need extra-care to compute its exact position in pixels, as the dialog measurements are all in points
    int                 gaugex              = CartoolApplication->PointsToPixels ( 2 );                 // distance to left (and right) border
    int                 gaugew              = ctdlgw - (   Windows10OffsetW                             // dialog shadow shift
                                                         + 2 * gaugex                                   // centered left and right
                                                         + 1                                            // it still looks 1 pixel too wide
                                                       );
    int                 gaugeh              = CartoolApplication->PointsToPixels ( 20 );
    int                 gaugey              = ctdlgh - (   ( Windows10OffsetH - Windows10OffsetY )      // dialog shadow shift
                                                         + gaugeh                                       // gauge actual height
                                                         + CartoolApplication->PointsToPixels ( 25 )    // gauge top position in dialog
                                                       );

    gauge   = new TGauge    (   this, 
                                "Browsing  %02d%%", 
                                IDC_PROGRESS,
                                gaugex,
                                gaugey,
                                gaugew,
                                gaugeh,
                                true 
                            );

    gauge->Create       ();
    gauge->SetRange     ( 0, 100 );
    gauge->UpdateWindow ();

                                        // resize and move the xyz window
    int                 vw              = ctdlgx + ctdlgw;
    int                 vh              = vw;
    int                 vx              = 0;
    int                 vy              = CartoolMdiClient->GetClientRect ().Height () - vh;

    if ( usexyz && automode != AutoAcceptAll ) {

        if ( PotentialsView->IsWindowMinimized () )
            PotentialsView->WindowRestore ();

        PotentialsView->WindowSetPosition ( vx, vy, vw, vh );

        PotentialsView->GetParentO()->SetFocus();   // to realize the palette

        PotentialsView->Invalidate();
        }

                                        // resize and move the eeg window
    vw  = CartoolMdiClient->GetClientRect().Width() - ( ctdlgx + ctdlgw );
    vh  = CartoolMdiClient->GetClientRect().Height();
    vx  = ctdlgx + ctdlgw;
    vy  = 0;

    if ( automode != AutoAcceptAll ) {

        if ( EegView->IsWindowMinimized () )
            EegView->WindowRestore ();

        EegView->WindowSetPosition ( vx, vy, vw, vh );

        EegView->WindowRestore ();
                                        // redraw NOW
        EegView->ShowNow();
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // init the control dialog
    int                 currtrigg;
    int                 savedcurrtrigg;
    int                 numtriggers         = triggerOrigin == EpochOriginFixedTF ? 1 : EegDoc->GetNumMarkers ( /*markertype*/ );   // count all markers
    int                 numtriggacc         = 0;
    int                 numtriggrej         = 0;

    TransferData ( tdGetData );

                                        // fill in constant data
    StringCopy  ( transfercontrol.CurrentFile, ToFileName ( filterdisplay ? filefiltered : eegfile ) );

    FloatToString ( transfercontrol.Threshold, ElThreshold );

    if ( CheckAux )
        FloatToString ( transfercontrol.AuxThreshold, AuxThreshold );
    else {                              // reset display
        ClearString ( transfercontrol.AuxName      );
        ClearString ( transfercontrol.AuxMin       );
        ClearString ( transfercontrol.AuxMax       );
        ClearString ( transfercontrol.AuxThreshold );
        }

                                        // clean-up from any previous file
    ClearString ( transfercontrol.TriggerCode          );
    ClearString ( transfercontrol.TriggerPos           );
    ClearString ( transfercontrol.TriggerIndex         );
    ClearString ( transfercontrol.NumTriggersAccepted  );
    ClearString ( transfercontrol.NumTriggersRejected  );

    TransferData ( tdSetData );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    verbose.NextLine ();
    verbose.NextBlock ();

    verbose.NextTopic ( "Processing File:" );

    verbose.NextLine ();
    verbose.Put ( "File #:", eegi + 1 );
    verbose.Put ( "File  :", eegfile );
    verbose.NextLine ();

    verbose.Put ( "Session used:", session > 0 ? seslist : "None, using whole file" );
    verbose.Put ( "Trigger(s) used:", triggerlist.ToString ( buff, ExpandedString ) );

//  verbose.Put ( "Total number of triggers and markers:", EegDoc->GetNumMarkers ( AllMarkerTypes ) );


    if      ( triggerOrigin != EpochOriginFixedTF ) {

        ClearString ( buff );

        if ( IsFlag ( markertype, MarkerTypeEvent   ) )     StringAppend ( buff, "Events",   " " );
        if ( IsFlag ( markertype, MarkerTypeTrigger ) )     StringAppend ( buff, "Triggers", " " );
        if ( IsFlag ( markertype, MarkerTypeMarker  ) )     StringAppend ( buff, "Markers",  " " );

        verbose.Put ( "Type of triggers:", buff );

        verbose.Put ( "Number of triggers from selected type:", EegDoc->GetNumMarkers ( markertype ) );
        }

    verbose.Put ( "Number of time frames:", (int)   EegDoc->GetNumTimeFrames    () );
    verbose.Put ( "Number of channels:",            EegDoc->GetNumElectrodes    () );
    verbose.Put ( "Number of auxiliary channels:",  EegDoc->GetNumAuxElectrodes () );
    verbose.Put ( "Sampling frequency [Hz]:",       EegDoc->GetSamplingFrequency() );
    if ( IsFreqLoop () )
        verbose.Put ( "Frequency:", FreqName );


    verbose.NextLine ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    verbose.NextLine ();

    if ( samplingfrequency == 0 && Filters.HasAnyFilter () )
        (ofstream&) verbose << "!!! Filtering can not be applied on this file, due to unknown sampling frequency !!!" fastendl fastendl;

    verbose.Flush ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // reset automode, except if grand averaging
//  if ( triggerOrigin != EpochOriginFixedTF )
//      automode    = AutoNone;
    automode        = acceptall ? AutoAcceptAll : triggerOrigin != EpochOriginFixedTF ? AutoNone : automode;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    TMarker             marker;
    TMarker             markerepoch;
    TMarkers            markersfiltered ( *EegDoc );   // copying ALL markers from EEG
    int                 markerindex;
    const MarkersList&  markers         = EegDoc->GetMarkersList ();

    long                tf0;
    long                tf1;
    long                tf2;
    double              baseline;
    float               channmin;
    float               channmax;
    float               regallmin;
    float               regallmax;
    float               auxallmin;
    float               auxallmax;

    int                 tvaok;
    double              tvartms;
    char                tvaname [ MarkerNameMaxLength ];
    bool                tvacartool;
    bool                tvaalreadyread      = false;
    bool                accepttrigger       = automode == AutoAcceptAll;
    bool                rejecttrigger       = false;
    long                arlimit             = 0;
    int                 restartcomputation  = 0;
    bool                choiceloopexit;
    bool                canopen;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    ofstream            oftva;

    if ( ! acceptall ) {
                                        // can open file and is different from input tva?
        do {
                                        // Will return the proper, non-overwriting file name
            fileouttva  = CookTvaFileName ( BaseFileName, eegfile, seslist, trglist );


            if ( triggerOrigin == EpochOriginFixedTF || automode == AutoAcceptAll )
                                        // overwrite silently
                canopen = CanOpenFile ( fileouttva, CanOpenFileWrite );
            else
                                        // ask for new one
                canopen = CanOpenFile ( fileouttva, CanOpenFileWriteAndAsk );

            if ( ! canopen )
                ShowMessage ( "Warning: no output TVA file will be written.", eegfile, ShowMessageWarning );

            else if ( StringIs ( fileouttva, tvafile ) )
                ShowMessage (   "Can not overwrite this file as it is currently used for output!" NewLine 
                                "Choose another file name please.", 
                                fileouttva, ShowMessageWarning );

            } while ( canopen && StringIs ( fileouttva, tvafile ) );


        if ( canopen ) {
            verbose.Put ( "Output TVA file used:", fileouttva );

            oftva.open ( TFileName ( fileouttva, TFilenameExtendedPath ) );

            oftva << TVATXT_MAGICNUMBER1 << fastendl;
            }
        else
            (ofstream&) verbose << "Error! No output TVA file!" fastendl fastendl;

        } // ! acceptall


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optionally opening trigger validation file
    ifstream*           iftva           = 0;


    if ( StringIsNotEmpty ( tvafile ) ) {

        if ( ! CanOpenFile ( tvafile, CanOpenFileReadAndAsk ) ) {
            (ofstream&) verbose << "Error! Can not open file " << tvafile << ", all triggers will be considered as OK!" fastendl fastendl;

            iftva   = 0;
            }
        else {
            iftva   = new ifstream ( TFileName ( tvafile, TFilenameExtendedPath ) );
            iftva->getline ( buff, 129 );

            tvacartool = StringStartsWith ( buff, TVATXT_MAGICNUMBER1 );

            if ( ! tvacartool )         // reset the first line
                iftva->seekg ( 0, ios::beg );

            verbose.Put ( "Using trigger validation file:", tvafile );
            verbose.NextLine ();
            }
        }
    else

        if ( triggerOrigin == EpochOriginTriggerReactionTime )
            (ofstream&) verbose << "Error! Can not compute the epoch origin based on reaction time, tva file is missing!" fastendl
                                << "       trigger will be used as the origin" fastendl fastendl;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optionally creating the bad epochs marker list
    TMarkers            badepochslist;

    if ( badepochs == SkippingBadEpochsList ) {
                                        // !using the original marker list, markersfiltered has already been modified!
        badepochslist.InsertMarkers ( *EegDoc, listbadepochs );

//      badepochslist.Show ( "badepochslist" );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // some local codes for handling markers cases
    enum                TriggerValidatedEnum
                        {
                        TriggerIgnore       = 0x0000,

                        TriggerInList       = 0x0001,

                        TriggerBadEpoch     = 0x0010,
                        TriggerTvaNotOk     = 0x0020,

                        TriggerAccepted     = 0x0100,
                        };


    marker.Set ( -1 );
    tvartms     = 0;

                                        // special case: force this only 1 TF
    if ( triggerOrigin == EpochOriginFixedTF ) {

        marker.From    = trigger1TF;
        StringCopy ( marker.Name, triggerlist[ 0 ] ); // to be sure it is accepted

        marker.Code    = (MarkerCode) TriggerInList;
        marker.To      = 0;                // store the RT

        *(markersfiltered[ 0 ])    = marker;
        }

    else { // real triggers
                                        // reset all codes
        for ( markerindex = 0; markerindex < markersfiltered.GetNumMarkers (); markerindex++ )

            markersfiltered[ markerindex ]->Code  = (MarkerCode) TriggerIgnore;

                                        // scan ALL triggers, put them in array, and flag them as inlist / ok
        for ( int i = 0; i < markers.Num (); i++ ) {
                                        // get next marker
            marker = *(markers[ i ]);


            marker.Code    = (MarkerCode) TriggerIgnore;


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check if trigger is of the right kind
            if ( ! IsFlag ( marker.Type, markertype ) ) {

                *(markersfiltered[ i ])    = marker;

                continue;
                }

            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // !From here, it is important to keep the TVA list in sync with markersfiltered / markers, 'continue' is forbidden!
            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                                        // see if trigger is in the list to be averaged
            if ( triggerlist.Contains ( marker.Name ) )

                SetFlags ( marker.Code, (MarkerCode) TriggerInList );


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // now, test if trigger is valid
            if ( iftva ) {
                                        // read a tva line, being careful if not an expected marker!
                if ( ! iftva->eof ()
                  && ( ! tvacartool                                             // always read, tva is an exhaustive list
                    || ! tvaalreadyread && IsFlag ( marker.Code, (MarkerCode) TriggerInList ) ) ) {   // read only if a selected marker

                    iftva->getline ( buff, 256 );

                    if ( StringIsEmpty ( buff ) ) {

                        (ofstream&) verbose << "Error! Early End-Of-File encountered in input TVA file " << tvafile << "!" fastendl fastendl;
                        tvaok       = true;
                        tvartms     = 0;
                        StringCopy ( tvaname, "?" );
                        }
                    else {
                                        // read only these 2
                        sscanf ( buff, "%d %lf", &tvaok, &tvartms );
                                        // then skip tvaok and tvartms, to have access to a trigger name which could contain some spaces(!)
                        SkipFirstWords ( buff, tvaname, 2 );
                        }
                    }


                if ( tvacartool ) {     // cartool tva is a subset of the full list of markers

                    if ( IsFlag ( marker.Code, (MarkerCode) TriggerInList ) && StringIsNot ( tvaname, marker.Name ) ) {

                        tvaok           = true;
                        tvartms         = 0;

                        if ( ! tvaalreadyread ) {

                            (ofstream&) verbose << "Error! Mismatch between markers: TVA marker is " << tvaname << ", while EEG marker is " << marker.Name << ", at TF " << marker.From << ". Skipping the remaining TVA file." fastendl fastendl;
                            sprintf     ( buff, "Error in TVA file, see the verbose for more details," NewLine 
                                                "but essentially the trigger names don't match." );
                            ShowMessage ( buff, "Averaging Error", ShowMessageWarning );
                            }

                        tvaalreadyread  = true;     // stop reading after the first miss
                        }
                    else
                        tvaalreadyread  = false;
                    } // if tvacartool

                else {                  // ! tvacartool, always read even if not in the tested list

                    if ( StringIsNot ( tvaname, marker.Name ) ) {

                        (ofstream&) verbose << "Error! Mismatch between markers: TVA marker is " << tvaname << ", while EEG marker is " << marker.Name << ", at TF " << marker.From << ". Skipping this trigger." fastendl fastendl;
                        tvaok       = false;
                        tvartms     = 0;
                        }
                    } // if ! tvacartool

                                        // store if trigger is NOT ok
                if ( ! ( IsFlag ( marker.Code, (MarkerCode) TriggerInList ) && tvaok ) )
                    SetFlags ( marker.Code, (MarkerCode) TriggerTvaNotOk );

                } // iftva

            else                        // no tva is always OK
                tvartms     = 0;

                                        // transform trigger position into epoch origin
            if      ( triggerOrigin == EpochOriginTriggerOffset         )   marker.From += triggerOffset;
            else if ( triggerOrigin == EpochOriginTriggerReactionTime   )   marker.From += Round ( MillisecondsToTimeFrame ( tvartms, samplingfrequency ) );

                                        // !silly, but this is the only place to store the Reaction Time (converted to int)!
            marker.To  = tvartms * 1000;


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // trigger is in list, but may overlap with rejection markers
            if ( badepochs == SkippingBadEpochsList ) {
                                        // use the actual epoch interval, without baseline margin but with additional margin to bad epochs
                tf0     = marker.From;
                tf1     = tf0 - ( durationPre  + deltabadepochs );
                tf2     = tf0 + ( durationPost + deltabadepochs );


                if ( badepochslist.IsOverlapping ( tf1, tf2 ) )

                    SetFlags ( marker.Code, (MarkerCode) TriggerBadEpoch );

                } // SkippingBadEpochsList


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // store updated marker
            *(markersfiltered[ i ])    = marker;

            } // for markers

        } // real triggers

                                        // close tva in
    if ( iftva != 0 )   delete  iftva;
//  iftva = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    markerepoch.Code       = (MarkerCode) 123;
    markerepoch.Type       = MarkerTypeTemp; // !these markers shoud never be written to file!

                                        // pre-loop to create TVA marked epochs
    for ( currtrigg = 0; currtrigg < markersfiltered.GetNumMarkers (); currtrigg++ ) {
                                        // get the next marker from the preprocessed list
        marker             = *(markersfiltered[ currtrigg ]);

                                        // is trigger not ok from tva?
        if ( ! IsFlag ( marker.Code, (MarkerCode) TriggerInList ) )

            continue;


        markerepoch.From   =
        markerepoch.To     = marker.From;

                                        // note that a marker can show as Bad Epoch AND Tva Not OK
        if ( IsFlag ( marker.Code, (MarkerCode) TriggerBadEpoch ) ) {

            StringCopy ( markerepoch.Name, MessageEpochBadEpoch );

            EegDoc->AppendMarker ( markerepoch );
            }

        if ( IsFlag ( marker.Code, (MarkerCode) TriggerTvaNotOk ) ) {

            StringCopy ( markerepoch.Name, MessageEpochBadTva );

            EegDoc->AppendMarker ( markerepoch );
            }
        }

    EegDoc->SortMarkers ();

                                        // Update view with the temp pre-labelled markers
    EegDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    auto    SetCursor   = [ &doctfcursor, &PotentialsView, &tf0, &durationPre, &durationPost, this, usexyz ] ()
    {
    doctfcursor.SetPos ( tf0 - durationPre, tf0 + durationPost );

    EegView->VnNewTFCursor ( &doctfcursor );

    if ( usexyz )
        PotentialsView->VnNewTFCursor ( &doctfcursor );
    };


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    SetFocus ();
//  EpochState      = true;

                                        // MAIN LOOP: process all triggers
                                        // Note this loop can restart itself
    for ( currtrigg = 0; currtrigg < markersfiltered.GetNumMarkers (); currtrigg++ ) {

                                        // force reset the Accepted flag, as we may have restarted the whole process
        ResetFlags ( markersfiltered[ currtrigg ]->Code, (MarkerCode) TriggerAccepted );

                                        // get the next marker from the preprocessed list
        marker     = *(markersfiltered[ currtrigg ]);


        SetGaugeRel ( *gauge, marker.From, numTimeFrames );

                                        // not concerned with this trigger
        if ( ! IsFlag ( marker.Code, (MarkerCode) TriggerInList ) )
            continue;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // update control dialog
        TransferData ( tdGetData );
        IntegerToString ( transfercontrol.TriggerIndex,         currtrigg + 1 );
        IntegerToString ( transfercontrol.NumTriggersAccepted,  numtriggacc   );
        IntegerToString ( transfercontrol.NumTriggersRejected,  numtriggrej   );
        TransferData ( tdSetData );

                                        // give all the focus to the dialog
//      WindowRestore ();
//      EnableWindow ( true );
//      SetActiveWindow ();
//      BringWindowToTop ();
//      SetFocus ();


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set the time origin
        tf0                 = marker.From;
        markerepoch.From    = tf0;
        markerepoch.To      = tf0;

                                        // test early reject that will bypass reading anything
        if ( rejecttrigger && marker.From <= arlimit ) {

            numtriggrej++;

            StringCopy ( markerepoch.Name, MessageEpochBadUser );
            EegDoc->InsertMarker ( markerepoch );

//          if ( automode == AutoNone )
//              EegDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );

            continue;
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        rejecttrigger   = false;        // no more to reject


        sprintf ( transfercontrol.TriggerPos,           "%0d", marker.From );
        sprintf ( transfercontrol.TriggerCode,          "%s",  marker.Name );
        TransferData ( tdSetData );


                                        // is trigger not ok?
        if      (    IsFlag ( marker.Code, (MarkerCode) TriggerBadEpoch )
                  || IsFlag ( marker.Code, (MarkerCode) TriggerTvaNotOk ) ) {

            numtriggrej++;

            continue;
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        SetGaugeRel ( *gauge, tf0, numTimeFrames );

                                        // set the time limits
        tf1     = tf0 - pretrigger;
        tf2     = tf0 + posttrigger;

                                        // out of range ?
        if ( ! IsInsideLimits ( tf1, tf2, (long) 0, numTimeFrames - 1 ) ) {

//          *ofv << " NOT ACCESSIBLE";
//          *ofv << fastendl fastendl "Error:  trying to access beyong file limits, skipping trigger " << ( currtrigg + 1 ) << " at position " << marker.From << " TF" fastendl;
//          if ( oftva.is_open () )     // update output tva
//              oftva << "0" Tab "0" Tab << marker.Name << fastendl;

            numtriggrej++;

            StringCopy          ( markerepoch.Name, MessageEpochBadAuto );
            EegDoc->InsertMarker( markerepoch );
            EegDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );

            if ( ! ( automode == AutoAcceptAll || IsFreqLoop () ) ) {
                sprintf     ( buff, "Trying to access file at positions  %0d  to  %0d," NewLine 
                                    "while file limits are  %0d  to  %0d." NewLine 
                                    "Trigger rejected.", 
                                    tf1, tf2, 0, numTimeFrames - 1 );
                ShowMessage ( buff, "Averaging Error", ShowMessageWarning );
                }

            EegView->Invalidate ();

            continue;
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // test early accept that will bypass channel checking
        if ( accepttrigger && ( marker.From <= arlimit || automode == AutoAcceptAll ) ) {
                                        // only here we label as Accepted
            SetFlags ( markersfiltered[ currtrigg ]->Code, (MarkerCode) TriggerAccepted );

            numtriggacc++;

            StringCopy          ( markerepoch.Name, MessageEpochOkUser );
            EegDoc->InsertMarker( markerepoch );

//          if ( automode == AutoNone )
//              EegDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );

            continue;
            } // if accepttrigger

        accepttrigger   = false;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read original data, filtered and referenced
        EegDoc->GetTracks   (   tf1,        tf2, 
                                DataEeg,    0, 
                                datatype, 
                                ComputePseudoTracks, 
                                dataref
                            );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        regallmin = auxallmin =  FLT_MAX;
        regallmax = auxallmax = -FLT_MAX;

        FocusOn.Reset ();

                                        // scan all tracks
//      for ( currchannel = 0; currchannel < numElectrodes; currchannel++ ) {
        for ( currchannel = 0; currchannel < totalChannels; currchannel++ ) {

            if ( ! CheckAux && ( AuxTracks[ currchannel ] || currchannel >= numElectrodes ) )
                continue;

                                        // computes min & max
            channmax    = channmin  = DataEeg ( currchannel, offsetTestMin );

                                        // this possibly includes more than the epoch
            for ( int t = offsetTestMin; t <= offsetTestMax; t++ ) {

                Mined ( channmin, DataEeg ( currchannel, t ) );
                Maxed ( channmax, DataEeg ( currchannel, t ) );
                }


                                        // no baseline on auxs
            if ( baseLineCorr && ! ( AuxTracks[ currchannel ] || currchannel >= numElectrodes ) ) {
                                        // compute baseline
                baseline    = 0;
                                        
                for ( int t = offsetBLCMin; t <= offsetBLCMax; t++ )
                    baseline   += DataEeg ( currchannel, t );

                baseline    /= NonNull ( numBLCTF );

                                        // apply baseline
                for ( int t = 0; t < dataTotal; t++ )
                    DataEeg ( currchannel, t ) -= baseline;
                } 


                                        // test here after the baseline has been applied to all tracks
            if ( CheckingExcluded[ currchannel ] )
                continue;               // no checking

                                        // compute the right global min / max
            if ( AuxTracks[ currchannel ] ) {
                Mined ( auxallmin, channmin );
                Maxed ( auxallmax, channmax );
                }
            else {
                Mined ( regallmin, channmin );
                Maxed ( regallmax, channmax );
                }

                                        // test if channel not ok
            if ( ( CheckEl  && ! AuxTracks[ currchannel ] && ( channmin < - ElThreshold  || channmax > ElThreshold  ) )
              || ( CheckAux &&   AuxTracks[ currchannel ] && ( channmin < - AuxThreshold || channmax > AuxThreshold ) ) )
                FocusOn.Set ( currchannel );

            } // for currchannel


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( automode != AutoNone ) {

            arlimit         = marker.From;
                                        // reject test, if either threshold fails
            if ( ( CheckEl  && ( regallmin < - ElThreshold  || regallmax > ElThreshold  ) )
              || ( CheckAux && ( auxallmin < - AuxThreshold || auxallmax > AuxThreshold ) ) ) {

                accepttrigger   = false;
                rejecttrigger   = true;
                }
            else {
                accepttrigger   = true;
                rejecttrigger   = false;
                }

                                        // need to break the automode?
            if ( automode == AutoRejectUntilOk && accepttrigger ) {
                automode        = AutoNone;
                accepttrigger   = false;
                rejecttrigger   = false;
                }

            if ( automode == AutoAcceptUntilBad && rejecttrigger ) {
                automode        = AutoNone;
                accepttrigger   = false;
                rejecttrigger   = false;
                }
            } //  !AutoNone

                                        // update display
        if ( automode == AutoNone ) {

            FloatToString ( transfercontrol.TrackMin, regallmin, 2 );
            FloatToString ( transfercontrol.TrackMax, regallmax, 2 );

            if ( CheckAux ) {
                FloatToString ( transfercontrol.AuxMin, auxallmin, 2 );
                FloatToString ( transfercontrol.AuxMax, auxallmax, 2 );
                }


            EpochState  = ! (bool) FocusOn;

                                        // some tracks to see?
            if ( EpochState ) {

                TracksBox->SetCaption ( "TRACKS  OK" );

                StringCopy ( transfercontrol.TrackName, "ALL" );

                if ( CheckAux )
                    StringCopy ( transfercontrol.AuxName, "ALL" );

                ClearString ( transfercontrol.TracksToExclude );


                StringCopy ( markerepoch.Name, MessageEpochSeemsOk );
                EegDoc->InsertMarker ( markerepoch );
                EegDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );
                }
            else {

                TracksBox->SetCaption ( "TRACKS" );

                if ( (bool) ( FocusOn & ~AuxTracks ) )
                    ( FocusOn & ~AuxTracks ).ToText ( transfercontrol.TrackName, &ElectrodesNames, AuxiliaryTracksNames );
                else
                    StringCopy ( transfercontrol.TrackName, "ALL" );

                if ( (bool) ( FocusOn &  AuxTracks ) )
                    ( FocusOn &  AuxTracks ).ToText ( transfercontrol.AuxName,   &ElectrodesNames, AuxiliaryTracksNames );
                else
                    if ( CheckAux )
                        StringCopy ( transfercontrol.AuxName, "ALL" );

                FocusOn.ToText ( transfercontrol.TracksToExclude, &ElectrodesNames, AuxiliaryTracksNames );


                StringCopy ( markerepoch.Name, MessageEpochSeemsBad );
                EegDoc->InsertMarker ( markerepoch );
                EegDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );
                }

            TransferData ( tdSetData );

                                        // remove this test to have an animated view
            if ( ! ( accepttrigger && marker.From <= arlimit ) ) {
                                        // update display
                SetCursor ();

                if ( CheckToBool ( transfercontrol.TrackZoom ) && !EpochState )
                    EegView->VnNewSelection ( &FocusOn       );
                else
                    EegView->VnNewSelection ( &DisplayTracks );

                EegView->VnNewHighlighted ( &FocusOn );

                if ( usexyz )
                    PotentialsView->VnNewHighlighted ( &FocusOn );
                }
            } // AutoNone
        else
            EpochState  = true;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // if in accept mode, don't enter the dialog
        if ( ! ( automode != AutoNone || ( accepttrigger && marker.From <= arlimit ) ) )

          do {

            choiceloopexit  = true;
                                        // trick:convert dialog to modal through Application
                                        // the dialog will break the loop by itself

                                        // set the timer to refresh the dialog with whatever is selected in the eeg view
            Timer.Start ( TimerRefresh, 250, 1 );

                                        // set dialog to modal, forbidding it to lose focus
            BeginModal ();

            TransferData ( tdGetData );

                                        // no more need to refresh
            Timer.Stop ();


            if ( ! CheckAux ) {         // account for user desires to see or not the aux, when they are not tested
                DisplayTracks  -= AuxTracks & ~EegView->SelTracks;
                DisplayTracks  += AuxTracks &  EegView->SelTracks;
                }


            switch ( ButtonPressed ) {  // test the button code

                case IDC_EXCLCHANNEL :

                    CheckingExcluded.Set ( transfercontrol.TracksToExclude, &ElectrodesNames );  // add new selection

                    EegDoc->ClearPseudo  ( CheckingExcluded );                          // but clear pseudos

                    if ( ! CheckAux )
                        CheckingExcluded -= AuxTracks;  // and clear auxiliaries

                    DisplayTracks   -= CheckingExcluded;

                    choiceloopexit      = true; // make sure to exit
                    restartcomputation  = 1;    // and restart, as at least 1 electrode is removed

                    CheckingExcluded.ToText ( transfercontrol.TracksExcluded, &ElectrodesNames, AuxiliaryTracksNames );
                    TransferData ( tdSetData );

                    EegDoc->SetBadTracks ( &CheckingExcluded );


                    if ( ignorepolarity ) {     // exclude everything suspected of being polluted
                        CorrelationSel.Set ();
                        EegDoc->ClearPseudo ( CorrelationSel );
                        CorrelationSel  -= CheckingExcluded;
                        CorrelationSel  -= AuxTracks;
                        }

                                        // do only once a new window
                    if ( usexyz && restartcomputation ) {
                        TSelection      tempsel ( ~DisplayTracks );

                        EegDoc->ClearPseudo ( tempsel );

                        XyzDoc->SetBadElectrodes ( tempsel );
                        }

                    break;

                case IDC_REJTRIGG :
                    accepttrigger   = false;
                    rejecttrigger   = true;
                    arlimit         = marker.From;
                    break;

                case IDC_REJNEXT :
                    accepttrigger   = false;
                    rejecttrigger   = true;
                    arlimit         = marker.From + StringToInteger ( transfercontrol.RejectNumTF );
                    break;

                case IDC_ACCTRIGG :
                    accepttrigger   = true;
                    rejecttrigger   = false;
                    arlimit         = marker.From;
                    break;

                case IDC_ACCNEXT :
                    accepttrigger   = true;
                    rejecttrigger   = false;
                    arlimit         = marker.From + StringToInteger ( transfercontrol.AcceptNumTF );
                    break;

                case IDC_ACCEPTUNTILBAD :
                    automode        = AutoAcceptUntilBad;
                    accepttrigger   = true;
                    rejecttrigger   = false;
                    arlimit         = marker.From;
                    break;

                case IDC_REJECTUNTILOK :
                    automode        = AutoRejectUntilOk;
                    accepttrigger   = false;
                    rejecttrigger   = true;
                    arlimit         = marker.From;
                    break;

                case IDC_ACCCOND :
                    automode        = AutoAcceptCond;
                    arlimit         = marker.From;
                    break;

                case IDC_CHANNELZOOM :
                    choiceloopexit  = false; // need a real choice

                    if ( CheckToBool ( transfercontrol.TrackZoom ) && ! EpochState )
                        EegView->VnNewSelection ( &FocusOn       );
                    else
                        EegView->VnNewSelection ( &DisplayTracks );
                    break;

                case IDC_RESTORECURSOR :
                    SetCursor ();
                    choiceloopexit  = false;
                    break;

                case IDC_RESTORETRACKS :
                                        // restore it anyway
                    if ( !CheckAux ) {
                        DisplayTracks  -= AuxTracks & ~EegView->SelTracks;
                        DisplayTracks  += AuxTracks &  EegView->SelTracks;
                        }

                                        // choose which to display
                    if ( CheckToBool ( transfercontrol.TrackZoom ) && ! EpochState )
                        EegView->VnNewSelection ( &FocusOn       );
                    else
                        EegView->VnNewSelection ( &DisplayTracks );

                                        // finally refresh the highlighted tracks
                    if ( !EpochState ) {
                        EegView->VnNewHighlighted ( &FocusOn );

                        if ( usexyz )
                            PotentialsView->VnNewHighlighted ( &FocusOn );

                        FocusOn.ToText ( transfercontrol.TracksToExclude, &ElectrodesNames, AuxiliaryTracksNames );
                        TransferData ( tdSetData );
                        }

                    choiceloopexit  = false;
                    break;

                case IDC_UNDOLAST :
                                        // nothing to undo?
                    if ( numtriggrej + numtriggacc == 0 ) {
                        choiceloopexit  = false;
                        break;
                        }
                                        // save current trigger index
                    savedcurrtrigg  = currtrigg;

                                        // scan backward the triggers
                    for ( currtrigg-- ; currtrigg >= 0; currtrigg-- ) {
                                        // ignore these
                        if ( ! IsFlag ( markersfiltered[ currtrigg ]->Code, (MarkerCode) TriggerInList   )
                          ||   IsFlag ( markersfiltered[ currtrigg ]->Code, (MarkerCode) TriggerBadEpoch )
                          ||   IsFlag ( markersfiltered[ currtrigg ]->Code, (MarkerCode) TriggerTvaNotOk ) )

                            continue;   // to revert

                                        // we have the last dealt trigger
                        if ( IsFlag ( markersfiltered[ currtrigg ]->Code, (MarkerCode) TriggerAccepted ) )
                            numtriggacc--;  // undo the number of accepted
                        else
                            numtriggrej--;  // or undo the number of rejected

                                        // clear the Accepted flag
                        ResetFlags ( markersfiltered[ currtrigg ]->Code, (MarkerCode) TriggerAccepted );

                                        // clear trigger choice display
                        EegDoc->RemoveMarkers ( markersfiltered[ currtrigg ]->From, markersfiltered[ currtrigg ]->From, MarkerTypeTemp );
                        EegDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );
                        break;
                        }

                                        // can not find a trigger to select?
                    if ( currtrigg < 0 ) {
                        currtrigg       = savedcurrtrigg;
                        choiceloopexit  = false;
                        break;
                        }

                    accepttrigger   = false;
                    rejecttrigger   = false;
                    marker.To          = -1;
                    marker.From        = -1;
                    currtrigg--;        // will be ++ -> back to currtrigg

                    restartcomputation  = 0;
                    continue;           // all counters are reset, so it will restart

                case IDC_RESTARTCURRENT :
                    if ( ! GetAnswerFromUser (  "Are you sure you want to restart" NewLine 
                                                "to the beginning of the current file?", 
                                                "Restart", CartoolMainWindow ) ) {
                        choiceloopexit  = false;
                        break;
                        }

                    choiceloopexit      = true; // make sure to exit
                    restartcomputation  = 2;    // and restart
                    break;

                case IDCANCEL :
                    if ( ! GetAnswerFromUser (  "Are you sure you want to cancel" 
                                                NewLine 
                                                "the whole averaging process?", 
                                                "Cancel All", CartoolMainWindow ) ) {
                        choiceloopexit  = false;
                        break;
                        }

                    (ofstream&) verbose << fastendl fastendl "!!!!! USER BREAK, ALL RESULTS WILL BE LOST !!!!!" fastendl fastendl;

                                        // we have to delete all local variables (see the end of the loop)
                    gauge->Destroy();
                    delete gauge;
                    gauge   = 0;

                    canClose = true;              // allow to close, now
                    CmCancel ();                  // modeless quit

                    if ( usexyz ) {

//                      LmDoc->RemoveLink ( EegDoc );
                        LmDoc->AllowClosing ();

                        CartoolDocManager->CloseView ( PotentialsView );

                        delete  PotentialsView;
                        delete  LmDoc;
                        }

                    if ( oftva.is_open () )
                        oftva.close ();

                    markersfiltered.ResetMarkers ();

                    EegDoc->SetDirty ( false );

                    EegDoc.Close ();

                    goto    deallocate;

                default:
                    choiceloopexit  = false;
                } // switch control

            } while ( ! choiceloopexit );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // update display
        if ( automode != AutoAcceptCond ) {
                                        // clear trigger choice display
            EegDoc->RemoveMarkers ( markerepoch.From, markerepoch.To, MarkerTypeTemp );

            if ( automode == AutoNone )
                EegDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( restartcomputation ) {

//          if ( restartcomputation == 1 ) // excluded channels
//              ShowMessage ( "Restarting the whole computation for this file!", "Channel exclusion" );

                                        // clean-up previous answers
            EegDoc->RemoveMarkers ( MarkerTypeTemp );

            if ( automode == AutoNone )
                EegDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );

            EegView->VnNewSelection ( &DisplayTracks );

            accepttrigger   = false;
            rejecttrigger   = false;
            numtriggrej     = 0;
            numtriggacc     = 0;
            currtrigg       = -1;       // will be ++ -> back to 0
            marker.To       = -1;
            marker.From     = -1;

            restartcomputation  = 0;
            continue;                   // all counters are reseted, so it will restart
            } // if restartcomputation


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( automode == AutoAcceptCond ) {
                                        // choose what to do with current epoch
            if ( ( CheckEl  && ( regallmin < - ElThreshold  || regallmax > ElThreshold  ) )
              || ( CheckAux && ( auxallmin < - AuxThreshold || auxallmax > AuxThreshold ) ) ) {
                accepttrigger   = false;
                rejecttrigger   = true;
                }
            else {
                accepttrigger   = true;
                rejecttrigger   = false;
                }
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( rejecttrigger && marker.From <= arlimit ) {
            numtriggrej++;

            StringCopy ( markerepoch.Name, MessageEpochBadUser );
            EegDoc->InsertMarker ( markerepoch );

            if ( automode == AutoNone )
                EegDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );

            continue;                   // we know already, skip next stop
            }


        if ( accepttrigger && marker.From <= arlimit ) {
                                        // only here we label as Accepted
            SetFlags ( markersfiltered[ currtrigg ]->Code, (MarkerCode) TriggerAccepted );

            numtriggacc++;

            StringCopy ( markerepoch.Name, MessageEpochOkUser );
            EegDoc->InsertMarker ( markerepoch );

            if ( automode == AutoNone )
                EegDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );

            continue;
            } // if accepttrigger

        accepttrigger   = false;

        } // for currtrig


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    // test if the list has been modified?
    verbose.Put ( "Channels excluded from amplitude test:", transfercontrol.TracksExcluded );

    (ofstream&) verbose << StreamFormatFixed;
    verbose.NextLine ();

    verbose.Flush ();

                                        // clear current epoch buffers
    Sum   .ResetMemory ();
    SumSqr.ResetMemory ();
                                        // index starts from 1, or simply follow the previous eeg
    int                 epochindex      = totalnumtriggacc;

    OutputTracks    = DisplayTracks;

    if ( saveaux )      OutputTracks   += AuxTracks;
    else                OutputTracks   -= AuxTracks;

    if ( saveexcl )     OutputTracks   += CheckingExcluded;
    else                OutputTracks   -= CheckingExcluded;

                                        // now we know
    int                 NumElectrodes   = OutputTracks.NumSet();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // initialize the output objects
    if ( SaveEpochsSingleFile && ! exp1epoch.IsOpen () && CanOpenFile ( fileout1epoch, CanOpenFileWrite ) ) {

        exp1epoch.Filename              = fileout1epoch;
        exp1epoch.SetAtomType ( AtomTypeScalar );
        exp1epoch.NumTracks             = NumElectrodes;
        exp1epoch.NumTime               = INT_MAX;      // still not known here
        exp1epoch.SamplingFrequency     = SamplingFrequency;
        exp1epoch.SelTracks             = OutputTracks;
        exp1epoch.ElectrodesNames       = ElectrodesNames;


        exp1epoch.Begin ( true );          // write dummy header

                                            // open marker file
        of1epochmrk.open ( TFileName ( fileout1epochmrk, TFilenameExtendedPath ) );

        WriteMarkerHeader ( of1epochmrk );
        }


    if ( SplitTriggers && ! expsplit.IsOpen () && CanOpenFile ( fileoutsplit, CanOpenFileWrite ) ) {

        expsplit.Filename               = fileoutsplit;
        expsplit.SetAtomType ( AtomTypeScalar );
        expsplit.NumTracks              = NumElectrodes;
        expsplit.NumTime                = INT_MAX;
        expsplit.SamplingFrequency      = SamplingFrequency;
        expsplit.SelTracks              = OutputTracks;
        expsplit.ElectrodesNames        = ElectrodesNames;

    /*
        expsplit.NumAuxTracks           = 0;
        expsplit.SelTracks              = 0;
        expsplit.AuxTracks              = 0;
    */

        expsplit.Begin ( true );        // write dummy header

                                        // open marker file
        ofsplitmrk.open ( TFileName ( fileoutsplitmrk, TFilenameExtendedPath ) );

        WriteMarkerHeader ( ofsplitmrk );
        }


    numtriggacc = 0;
    numtriggrej = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // needs to switch between filtered and non-filtered data (in either direction)?
    if ( filterdisplay != filterdata ) {
                                        // this will close the view, which we don't need anymore anyway
        EegDoc.Close        ();

        EegView     = 0;

        EegDoc.Open         ( filterdata ? filefiltered : gofeeg[ eegi ], OpenDocHidden );

//      EegDoc.Lock ();
                                        // !revoking any existing filter: either off, or already a filtered temp file!
        EegDoc->DeactivateFilters ( true );

                                        // Updating remote control
        StringCopy  ( transfercontrol.CurrentFile, ToFileName ( filterdata ? filefiltered : eegfile ) );

        TransferData ( tdSetData );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scan for polarities to build a reference template, to which the sums will be checked against
                                        // the choice of the template appear to be is quite sensitive for ERP averages
                                        // and less for Grand Means
    if ( ignorepolarity && PolarityRef.IsNotAllocated () ) {

        gauge->SetCaption ( "Checking Polarity  %02d%%" );

                                        // allocate only once for all epochs and all files
        PolarityRef.Resize ( dataTotal, numElectrodes );
        Map        .Resize            ( numElectrodes );

                                        // then fill the vectors
        for ( currtrigg = 0; currtrigg < markersfiltered.GetNumMarkers (); currtrigg++ ) {

            marker     = *(markersfiltered[ currtrigg ]);

            if ( ! IsFlag ( marker.Code, (MarkerCode) TriggerInList   )
              ||   IsFlag ( marker.Code, (MarkerCode) TriggerBadEpoch )
              ||   IsFlag ( marker.Code, (MarkerCode) TriggerTvaNotOk )
              || ! IsFlag ( marker.Code, (MarkerCode) TriggerAccepted ) )

                continue;

                                        // set the time limits
            tf0     = marker.From;
            tf1     = tf0 - pretrigger;
            tf2     = tf0 + posttrigger;


            SetGaugeRel ( *gauge, tf0, numTimeFrames );

                                        // read original data, filtered and referenced
            EegDoc->GetTracks   (   tf1,        tf2, 
                                    DataEeg,    0, 
                                    datatype, 
                                    NoPseudoTracks, 
                                    dataref
                                );

                                        // first epoch / file sets an initial template
            if ( currtrigg == 0 )

                for ( int t = 0; t < dataTotal; t++ ) {

                    PolarityRef[ t ].GetColumn ( DataEeg, t );

//                  PolarityRef[ t ].Normalize ();  // should be done only on CorrelationSel!
                    PolarityRef[ t ]   /= NonNull ( PolarityRef[ t ].GlobalFieldPower ( CorrelationSel, dataref == ReferenceAverage, IsVector ( datatype ) ) );
                    }
            else                        // if other epochs, iteratively update the initial template

                for ( int t = 0; t < dataTotal; t++ ) {

                    Map.GetColumn ( DataEeg, t );

//                  Map.Normalize ();
                    Map    /= Map.GlobalFieldPower ( CorrelationSel, dataref == ReferenceAverage, IsVector ( datatype ) );

                    if ( Map.Correlation ( PolarityRef[ t ], CorrelationSel, dataref == ReferenceAverage ) < 0 )
                        PolarityRef[ t ]   -= Map;
                    else
                        PolarityRef[ t ]   += Map;
                    } // for t

            } // for currtrigg

                                        // just to clean-up the templates
        for ( int t = 0; t < dataTotal; t++ )
//          PolarityRef[ t ].Normalize ();
            PolarityRef[ t ] /= NonNull ( PolarityRef[ t ].GlobalFieldPower ( CorrelationSel, dataref == ReferenceAverage, IsVector ( datatype ) ) );

        } // if ignorepolarity


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    gauge->SetCaption ( "Computing  %02d%%" );

                                        // now, rescan the data to actually compute the sums
    for ( currtrigg = 0; currtrigg < markersfiltered.GetNumMarkers (); currtrigg++ ) {

        marker     = *(markersfiltered[ currtrigg ]);

                                        // skip all non-accepted triggers
        if ( ! IsFlag ( marker.Code, (MarkerCode) TriggerInList ) )

            continue;

                                        // trigger in list
        (ofstream&) verbose  << fastendl;
        (ofstream&) verbose  << "Epoch #"    << setw ( 4 )   << ( currtrigg + 1 )   << ","      << Tab;
        (ofstream&) verbose  << "origin at " << setw ( 6 )   << marker.From         << " TF,"   << Tab;
        (ofstream&) verbose  << "trigger "                   << marker.Name         << " :"     << Tab;

                                        // not a good trigger?
        if (      IsFlag ( marker.Code, (MarkerCode) TriggerBadEpoch ) 
             ||   IsFlag ( marker.Code, (MarkerCode) TriggerTvaNotOk ) 
             || ! IsFlag ( marker.Code, (MarkerCode) TriggerAccepted ) ) {

            if      (   IsFlag ( marker.Code, (MarkerCode) TriggerBadEpoch ) )    (ofstream&) verbose << MessageEpochBadEpoch;
            else if (   IsFlag ( marker.Code, (MarkerCode) TriggerTvaNotOk ) )    (ofstream&) verbose << MessageEpochBadTva;
            else if ( ! IsFlag ( marker.Code, (MarkerCode) TriggerAccepted ) )    (ofstream&) verbose << MessageEpochBadUser;


            if ( oftva.is_open () )     // update output tva
//              oftva << "0" Tab "0" Tab << marker.Name << fastendl;
                oftva << "0" Tab << ( marker.To / 1000.0 ) << Tab << marker.Name << fastendl;

            numtriggrej++;

            continue;
            }

                                        // here, trigger is accepted
        epochindex++;
        numtriggacc++;

        (ofstream&) verbose << MessageEpochOkUser;

        if ( oftva.is_open () )         // update output tva
//          oftva << "1" Tab "0" Tab << marker.Name << fastendl;
            oftva << "1" Tab << ( marker.To / 1000.0 ) << Tab << marker.Name << fastendl;


                                        // set the time limits
        tf0     = marker.From;
        tf1     = tf0 - pretrigger;
        tf2     = tf0 + posttrigger;


                                        // update control dialog, just for the beauty of it
        TransferData ( tdGetData );
        sprintf ( transfercontrol.TriggerIndex,         "%0d", currtrigg + 1 );
        sprintf ( transfercontrol.NumTriggersAccepted,  "%0d", numtriggacc   );
        sprintf ( transfercontrol.NumTriggersRejected,  "%0d", numtriggrej   );
        sprintf ( transfercontrol.TriggerPos,           "%0d", marker.From      );
        sprintf ( transfercontrol.TriggerCode,          "%s",  marker.Name      );
        TransferData ( tdSetData );


        SetGaugeRel ( *gauge, tf0, numTimeFrames );

                                        // read original data, filtered and referenced
        EegDoc->GetTracks   (   tf1,        tf2, 
                                DataEeg,    0, 
                                datatype, 
                                NoPseudoTracks, 
                                dataref
                            );


                                        // compute & subtract baseline, store result in buffer
        if ( baseLineCorr )

            for ( currchannel = 0; currchannel < numElectrodes; currchannel++ ) {

                if ( AuxTracks[ currchannel ] )
                    continue;

                baseline    = 0;
                                        
                for ( int t = offsetBLCMin; t <= offsetBLCMax; t++ )
                    baseline   += DataEeg ( currchannel, t );

                baseline    /= NonNull ( numBLCTF );

                                        // apply baseline
                for ( int t = 0; t < dataTotal; t++ )
                    DataEeg ( currchannel, t ) -= baseline;
                }


                                        // check for polarity, at each TF independently
        if ( ignorepolarity ) {

            for ( int t = 0; t < dataTotal; t++ ) {

                Map.GetColumn ( DataEeg, t );

                if ( Map.Correlation ( PolarityRef[ t ], CorrelationSel, dataref == ReferenceAverage ) < 0 )

                    for ( currchannel = 0; currchannel < numElectrodes; currchannel++ )

                        if ( ! AuxTracks[ currchannel ] )

                            DataEeg ( currchannel , t ) = - DataEeg ( currchannel , t );
                } // for t

            } // if ignorepolarity


                                        // add to the current sums
        for ( currchannel = 0; currchannel < numElectrodes; currchannel++ ) {

            for ( int t = 0; t < dataTotal; t++ ) {

                Sum    ( currchannel, t )  +=          DataEeg ( currchannel, t );
                SumSqr ( currchannel, t )  += Square ( DataEeg ( currchannel, t ) );
                } // for t
            } // for currchannel



        if ( SaveEpochs ) {             // write epoch

            if ( SaveEpochsMultiFiles ) {

                TExportTracks   expepoch;


                StringCopy      ( fileoutepochs, EpochsDir, "\\", fileoutprefix, "." InfixEpoch " ", IntegerToString ( epochindex, NumIntegerDigits ( numtriggers ) ) );
                AddExtension    ( fileoutepochs, /*FILEEXT_EEGSEF*/ fileoutext );


                if ( CanOpenFile ( fileoutepochs, CanOpenFileWrite ) ) {

                    StringCopy ( expepoch.Filename, fileoutepochs );

                    expepoch.SetAtomType ( AtomTypeScalar );
                    expepoch.NumTracks              = NumElectrodes;
                    expepoch.NumTime                = dataLength;
                    expepoch.SamplingFrequency      = samplingfrequency;
                    expepoch.SelTracks              = OutputTracks;
                    expepoch.ElectrodesNames        = ElectrodesNames;


                    for ( int t = 0; t < dataLength; t++)
                    for ( TIteratorSelectedForward seli ( OutputTracks ); (bool) seli; ++seli )

                        expepoch.Write ( DataEeg[ seli() ][ offsetData + t ] );

                                        // re-using the export file name
                    AddExtension    ( expepoch.Filename, FILEEXT_MRK );

                    ofstream            ofmultiepochmrk ( expepoch.Filename );

                    WriteMarkerHeader ( ofmultiepochmrk );

                    WriteMarker (   ofmultiepochmrk, 
                                    durationPre,        // set at trigger position
                                    marker.Name 
                                );

                    } // if CanOpen

                } // SaveEpochsMultiFiles


            if ( SaveEpochsSingleFile && exp1epoch.IsOpen () ) {

                for ( int t = 0; t < dataLength; t++)
                for ( TIteratorSelectedForward seli ( OutputTracks ); (bool) seli; ++seli )

                    exp1epoch.Write ( DataEeg[ seli() ][ offsetData + t ] );

                                        // update marker file
                WriteMarker (   of1epochmrk, 
                                ( epochindex - 1 ) * dataLength,
                                  epochindex       * dataLength - 1,            // extend to the whole epoch duration
                                StringCopy  ( buff, "epoch", IntegerToString ( /*currtrigg + 1*/ epochindex, NumIntegerDigits ( numtriggers ) ) )
                            );

                                        
                WriteMarker (   of1epochmrk, 
                                ( epochindex - 1 ) * dataLength + durationPre,  // set at trigger position
                                marker.Name 
                            );


                } // SaveEpochsSingleFile

            } // save epochs


                                        // write all epochs for further processings
        if ( SplitTriggers && expsplit.IsOpen () ) {

            for ( int t = 0; t < dataLength; t++)
            for ( TIteratorSelectedForward seli ( OutputTracks ); (bool) seli; ++seli )

                expsplit.Write ( DataEeg[ seli() ][ offsetData + t ] );


            WriteMarker (   ofsplitmrk, 
                            ( epochindex - 1 ) * dataLength,
                            marker.Name 
                        );
            } // SplitTriggers

        } // for currtrigg

    verbose.Flush ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // close the pot map view & the link
    if ( usexyz ) {

//      LmDoc->RemoveLink ( EegDoc );
        LmDoc->AllowClosing ();

        CartoolDocManager->CloseView ( PotentialsView );

        delete  PotentialsView;
        delete  LmDoc;
        }


//  int     oldtna      = totalnumtriggacc;
    totalnumtriggacc    += numtriggacc;
    totalnumtriggrej    += numtriggrej;
    totalnumtrigg       += numtriggacc + numtriggrej;


                                        // add to the grand sums
    for ( currchannel = 0; currchannel < numElectrodes; currchannel++ )
    for ( int t = 0; t < dataTotal; t++ ) {

//      if ( !saveaux && AuxTracks[ currchannel ] )
//          continue;

        TotalSum    ( currchannel, t ) += Sum    ( currchannel, t );
        TotalSumSqr ( currchannel, t ) += SumSqr ( currchannel, t );
        }


    int     numtriggar  = numtriggacc + numtriggrej;


    (ofstream&) verbose << StreamFormatFixed << StreamFormatLeft;

    verbose.NextLine ( 2 );
    verbose.NextTopic ( "Summary:" );
    verbose.NextLine ();

    verbose.Put ( "File #:", eegi + 1 );
    verbose.Put ( "File  :", eegfile );
    verbose.NextLine ();

    verbose.Put ( "Trigger(s) used:", triggerlist.ToString ( buff, ExpandedString ) );
    verbose.Put ( "Number of triggers scanned :", numtriggar, 4 );
    if ( numtriggar == 0 )  numtriggar = 1;

    sprintf ( buff, "%4d / %5.1f%%", numtriggacc, 100.0 * numtriggacc / numtriggar );
    verbose.Put ( "Number of triggers accepted:", buff );
    sprintf ( buff, "%4d / %5.1f%%", numtriggrej, 100.0 * numtriggrej / numtriggar );
    verbose.Put ( "Number of triggers rejected:", buff );

//    if ( SaveEpochsMultiFiles && oldtna != totalnumtriggacc )
//        verbose.Put ( "Epoch files:                            " << BaseFileName << ".[" << ( oldtna + 1 ) << ".." << totalnumtriggacc << "]." << fileoutext << fastendl;

    verbose.Flush ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    gauge->Destroy();
    delete gauge;
    gauge   = 0;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    EegDoc->RemoveMarkers ( MarkerTypeTemp );

                                        // close the current document - be smart, don't close before re-opening!
    if (    lasteeg 
         || StringIsNot ( eegfile, gofeeg[ eegi + 1 ] ) 
         || anyfilter                                   ) {

        EegDoc->SetDirty ( false );

        EegDoc.Close ();

        EegView     = 0;
        }


    if ( anyfilter )
                                         // delete associated files, too
        DeleteFileExtended  ( filefiltered, TracksBuddyExt );


    if ( oftva.is_open () )
        oftva.close ();


    markersfiltered.ResetMarkers ();

                                        // make everything up to date
    UpdateApplication;

    } // for gofeeg


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ofsplitmrk .is_open () )   ofsplitmrk .close ();
if ( of1epochmrk.is_open () )   of1epochmrk.close ();

canClose = true;                        // allow to close, now

TBaseDialog::CmOk();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // output selection is basically DisplayTracks
OutputTracks    = DisplayTracks;

                                        // add or remove according to user's wishes
if ( saveaux )      OutputTracks += AuxTracks;
else                OutputTracks -= AuxTracks;

if ( saveexcl )     OutputTracks += CheckingExcluded;
else                OutputTracks -= CheckingExcluded;

                                        // then we have
int     NumElectrodes   = OutputTracks.NumSet();
long    NumTimeFrames   = dataLength;


                                        // re-order in sequence
if ( ignorepolarity ) {

    for ( int t = 0; t < dataTotal; t++ )
        PolarityRef[ t ].GetColumn ( TotalSum, t );


    for ( int t = 1; t < dataTotal; t++ ) {

        if ( PolarityRef[ t ].Correlation ( PolarityRef[ t - 1 ], CorrelationSel, dataref == ReferenceAverage ) < 0 )

            for ( currchannel = 0; currchannel < numElectrodes; currchannel++ )

                if ( ! AuxTracks[ currchannel ] ) {
                    TotalSum    ( currchannel , t ) = - TotalSum ( currchannel , t );
                    PolarityRef ( t , currchannel ) = - PolarityRef ( t , currchannel );
                    }

        } // for t
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // finally compute the mean & standard deviation
double              numtrg          = totalnumtriggacc ? totalnumtriggacc : 1;
double              avg;
double              sd;
double              se;

                                        // we can update this filename
if ( IsFreqLoop () )
    sprintf ( fileoutsum,    "%s.%s.%s",       (char*) BaseFileName, InfixSum,                  (char*) fileoutext );
else
    sprintf ( fileoutsum,    "%s.%s (%0d).%s", (char*) BaseFileName, InfixSum,    (int) numtrg, (char*) fileoutext );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // write the average or the full sums
if ( MergeTriggers && ( SaveAverage || SaveSum ) ) {

    TExportTracks    *expavg      = 0;
    TExportTracks    *expsum      = 0;


    if ( SaveAverage ) {
        expavg      = new TExportTracks;

        expavg->Filename            = fileoutavg;
        expavg->SetAtomType ( AtomTypeScalar );
        expavg->NumTracks           = NumElectrodes;
        expavg->NumTime             = NumTimeFrames;
        expavg->SamplingFrequency   = SamplingFrequency;
        expavg->SelTracks           = OutputTracks;
        expavg->ElectrodesNames     = ElectrodesNames;


        fileoutmrk  = fileoutavg + "." + FILEEXT_MRK;

        if ( writeoriginmrk && durationPre > 0 ) {
            ofstream        ofmrk ( TFileName ( fileoutmrk, TFilenameExtendedPath ) );

            WriteMarkerHeader ( ofmrk );

            WriteMarker (   ofmrk, 
                            durationPre,        // set at trigger position
                            MarkerOrigin
                        );

            }
        else if ( CanOpenFile ( fileoutmrk ) )
            DeleteFileExtended ( fileoutmrk );
        }


    if ( SaveSum ) {
        expsum      = new TExportTracks;

        expsum->Filename            = fileoutsum;
        expsum->SetAtomType ( AtomTypeScalar );
        expsum->NumTracks           = NumElectrodes;
        expsum->NumTime             = NumTimeFrames;
        expsum->SamplingFrequency   = SamplingFrequency;
        expsum->SelTracks           = OutputTracks;
        expsum->ElectrodesNames     = ElectrodesNames;


        fileoutmrk  = fileoutsum + "." + FILEEXT_MRK;

        if ( writeoriginmrk && durationPre > 0 ) {
            ofstream        ofmrk ( TFileName ( fileoutmrk, TFilenameExtendedPath ) );

            WriteMarkerHeader ( ofmrk );

            WriteMarker (   ofmrk, 
                            durationPre,        // set at trigger position
                            MarkerOrigin
                        );
            }
        else if ( CanOpenFile ( fileoutmrk ) )
            DeleteFileExtended ( fileoutmrk );
        }


    for ( int t = 0; t < NumTimeFrames; t++ )
    for ( TIteratorSelectedForward seli ( OutputTracks ); (bool) seli; ++seli ) {

        if ( SaveAverage )  expavg   ->Write ( (float) ( TotalSum   [ seli() ][ offsetData + t ] / numtrg ) );
        if ( SaveSum     )  expsum   ->Write ( (float)   TotalSum   [ seli() ][ offsetData + t ]            );

        } // for channel


    if ( expavg    )    delete  expavg;
    if ( expsum    )    delete  expsum;
    } // if MergeTriggers


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // save the SD or SE to file
if ( MergeTriggers && ( SaveSD || SaveSE ) ) {

    TExportTracks*      expsd       = 0;
    TExportTracks*      expse       = 0;


    if ( SaveSD ) {
        expsd   = new TExportTracks;

        expsd->Filename             = fileoutsd;
        expsd->SetAtomType ( AtomTypeScalar );
        expsd->NumTracks            = NumElectrodes;
        expsd->NumTime              = NumTimeFrames;
        expsd->SamplingFrequency    = SamplingFrequency;
        expsd->SelTracks            = OutputTracks;
        expsd->ElectrodesNames      = ElectrodesNames;
        }


    if ( SaveSE ) {
        expse   = new TExportTracks;

        expse->Filename             = fileoutse;
        expse->SetAtomType ( AtomTypeScalar );
        expse->NumTracks            = NumElectrodes;
        expse->NumTime              = NumTimeFrames;
        expse->SamplingFrequency    = SamplingFrequency;
        expse->SelTracks            = OutputTracks;
        expse->ElectrodesNames      = ElectrodesNames;
        }


//  double      numtrg = sqrt ( totalnumtriggacc ? totalnumtriggacc : 1 );


    for ( int t = 0; t < NumTimeFrames; t++ )
    for ( TIteratorSelectedForward seli ( OutputTracks ); (bool) seli; ++seli ) {

        sd      = sqrt ( fabsl ( (            TotalSumSqr ( seli(), offsetData + t ) 
                                   - Square ( TotalSum    ( seli(), offsetData + t ) ) / numtrg ) 
                                 / NonNull ( numtrg - 1 ) 
                               ) 
                       );

        se      = sd / sqrt ( numtrg );


        if ( SaveSD )   expsd->Write ( (float) sd );
        if ( SaveSE )   expse->Write ( (float) se );
        }


    if ( expsd )    delete  expsd;
    if ( expse )    delete  expse;
    } // SD or SE

                                            // at this point, it is safe to delete files that have been computed earlier, to avoid mixing data from different computations
if ( ! SaveSD && CanOpenFile ( fileoutsd ) )
    DeleteFileExtended ( fileoutsd );

if ( ! SaveSE && CanOpenFile ( fileoutse ) )
    DeleteFileExtended ( fileoutse );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // final output for new .xyz, accounting for all electrodes choices
if ( usexyz )
    XyzDoc->ExtractToFile ( fileoutxyz, OutputTracks, false );


/*                                        // can create a link right now, and open it ?
if ( usexyz ) {
    char    fileoutlm[129];
    sprintf ( fileoutlm,  "%s.%s", BaseFileName, FILEEXT_LM );
    ofstream       *ofg = new ofstream ( fileoutlm );

    *ofg << fileoutxyz << fastendl;
    *ofg << fileoutavg << fastendl;

    delete  ofg;

    CartoolDocManager->OpenDoc ( fileoutlm, dtOpenOptions );
    }
*/


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( SaveEpochsSingleFile && exp1epoch.IsOpen () && totalnumtriggacc > 0 ) {

                                        // set again all variables + the real time length
    exp1epoch.NumTracks             = NumElectrodes;
    exp1epoch.NumTime               = totalnumtriggacc * dataLength;
    exp1epoch.SamplingFrequency     = SamplingFrequency;

    exp1epoch.WriteHeader ( true );    // overwrite the header

    exp1epoch.End ();                  // that's it
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                dosplittriggers = SplitTriggers && expsplit.IsOpen () && totalnumtriggacc > 0 && (bool) alltriggers;


if ( dosplittriggers ) {

                                        // set again all variables + the real time length
    expsplit.NumTracks              = NumElectrodes;
    expsplit.NumTime                = totalnumtriggacc * dataLength;
    expsplit.SamplingFrequency      = SamplingFrequency;

    expsplit.WriteHeader ( true );     // overwrite the header

    expsplit.End ();                   // that's it


                                        // rock around the triggs'
    verbose.NextLine ();
    verbose.NextBlock ();
    verbose.NextTopic ( "Splitting Triggers" );
    verbose.NextLine ();


                                        // open split file directly (can be too big to be swallowed by OpenDoc )
    ifstream        ifs ( TFileName ( fileoutsplit, TFilenameExtendedPath ), ios::binary );

                                        // open marker file directly, too
    TMarkers            docmarkers ( fileoutsplitmrk );
    char                trigname [ MarkerNameMaxLength ];
    const MarkersList&  markers         = docmarkers.GetMarkersList ();


    TSuperGauge         Gauge ( "Splitting Triggers", alltriggers.GetNumTokens (), SuperGaugeLevelInter );

                                        // browse each trigger separately
    for ( int trig = 0; trig < alltriggers.GetNumTokens (); trig++ ) {

        Gauge.Next ();


        StringCopy ( trigname, alltriggers[ trig ] );

                                        // reset buffers
        DataEeg.ResetMemory ();
        Sum    .ResetMemory ();
        SumSqr .ResetMemory ();
        int     numsplittrg = 0;

                                        // scan each time all triggers
        for ( int i = 0; i < markers.Num (); i++ ) {

                                        // correct trigger?
            if ( StringIsNot ( markers[ i ]->Name, trigname ) )
                continue;

            numsplittrg++;

                                        // get next epoch "by hand"
            ifs.seekg ( sizeof ( TRisHeader ) + markers[ i ]->From * NumElectrodes * DataEeg.AtomSize (), ios::beg );

            for ( int t = 0; t < NumTimeFrames; t++ )
            for ( int c = 0; c < NumElectrodes; c++ )
                ifs.read ( (char *) &DataEeg ( c , t ), DataEeg.AtomSize () );


                                        // add to the current sums
            for ( int c = 0; c < NumElectrodes; c++ )
            for ( int t = 0; t < NumTimeFrames; t++ ) {

                Sum    ( c, t )    +=          DataEeg ( c, t );
                SumSqr ( c, t )    += Square ( DataEeg ( c, t ) );
                }

            } // for markers


                                        // re-order in sequence
        if ( ignorepolarity ) {

            for ( int t = 0; t < dataTotal; t++ )
                PolarityRef[ t ].GetColumn ( Sum, t );


            for ( int t = 1; t < dataTotal; t++ ) {

                if ( PolarityRef[ t ].Correlation ( PolarityRef[ t - 1 ], CorrelationSel, dataref == ReferenceAverage ) < 0 )

                    for ( currchannel = 0; currchannel < numElectrodes; currchannel++ )

                        if ( ! AuxTracks[ currchannel ] ) {
                            Sum         ( currchannel , t ) = - Sum ( currchannel , t );
                            PolarityRef ( t , currchannel ) = - PolarityRef ( t , currchannel );
                            }

                } // for t
            }


                                        // fill up some more verbose
        if ( trig )
            verbose.NextLine ( 3 );

        verbose.Put ( "Trigger:", trigname );
        verbose.NextLine ();
        verbose.Put ( "Total number of triggers:", numsplittrg );

                                        // sub-directory for split triggers
        TriggerFileName     = BaseDir + "\\" InfixTrigger " " + trigname + "\\";
        CreatePath  ( TriggerFileName, false );

                                        // set the base name for the current trigger
        if ( IsFreqLoop () )    TriggerFileName    += fileoutprefix + "." + FreqName + "." InfixTrigger " " + trigname;
        else                    TriggerFileName    += fileoutprefix                  + "." InfixTrigger " " + trigname;


        if ( SaveAverage ) {
            file    = TriggerFileName + "." + fileoutext;

            verbose.Put ( "Split  file    - Average:",              file );
            }
        if ( SaveSum ) {
            if ( IsFreqLoop () )    file    = TriggerFileName + "." + InfixSum                                                + "." + fileoutext;
            else                    file    = TriggerFileName + "." + InfixSum + " (" + (const char*) IntegerToString ( numsplittrg ) + ")" + "." + fileoutext;

            verbose.Put ( "Split  file    - Sum:",                  file );
            }
        if ( SaveSD ) {
            file    = TriggerFileName + "." InfixSD "." + fileoutext;

            verbose.Put ( "Split  file    - Standard Deviation:",   file );
            }
        if ( SaveSE ) {
            file    = TriggerFileName + "." InfixSE "." + fileoutext;

            verbose.Put ( "Split  file    - Standard Error:",       file );
            }

                                        // no epochs remain for this trigger?
        if ( numsplittrg == 0 )
            continue;

                                        // output files - a copy from above (except for the file names, buffers & array indexes)
        TExportTracks*      expavg      = 0;
        TExportTracks*      expsum      = 0;
        TExportTracks*      expsd       = 0;
        TExportTracks*      expse       = 0;
        

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( SaveAverage ) {

            expavg      = new TExportTracks;

            expavg->Filename            = TriggerFileName + "." + fileoutext;
            expavg->SetAtomType ( AtomTypeScalar );
            expavg->NumTracks           = NumElectrodes;
            expavg->NumTime             = NumTimeFrames;
            expavg->SamplingFrequency   = SamplingFrequency;
            expavg->SelTracks           = OutputTracks;
            expavg->ElectrodesNames     = ElectrodesNames;


            StringCopy      ( fileoutmrk, expavg->Filename );
            AddExtension    ( fileoutmrk, FILEEXT_MRK );

            if ( writeoriginmrk && durationPre > 0 ) {
                ofstream        ofmrk ( TFileName ( fileoutmrk, TFilenameExtendedPath ) );

                WriteMarkerHeader ( ofmrk );

                WriteMarker (   ofmrk, 
                                durationPre,        // set at trigger position
                                MarkerOrigin
                            );

                WriteMarker (   ofmrk, 
                                durationPre,        // set at trigger position
                                trigname
                            );
                }
            else if ( CanOpenFile ( fileoutmrk ) )
                DeleteFileExtended ( fileoutmrk );
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( SaveSum ) {

            expsum      = new TExportTracks;

            if ( IsFreqLoop () )    expsum->Filename    = TriggerFileName + "." + InfixSum                                                + "." + fileoutext;
            else                    expsum->Filename    = TriggerFileName + "." + InfixSum + " (" + (const char*) IntegerToString ( numsplittrg ) + ")" + "." + fileoutext;

            expsum->SetAtomType ( AtomTypeScalar );
            expsum->NumTracks           = NumElectrodes;
            expsum->NumTime             = NumTimeFrames;
            expsum->SamplingFrequency   = SamplingFrequency;
            expsum->SelTracks           = OutputTracks;
            expsum->ElectrodesNames     = ElectrodesNames;


            StringCopy      ( fileoutmrk, expsum->Filename );
            AddExtension    ( fileoutmrk, FILEEXT_MRK );

            if ( writeoriginmrk && durationPre > 0 ) {
                ofstream        ofmrk ( TFileName ( fileoutmrk, TFilenameExtendedPath ) );

                WriteMarkerHeader ( ofmrk );

                WriteMarker (   ofmrk, 
                                durationPre,        // set at trigger position
                                MarkerOrigin
                            );

                WriteMarker (   ofmrk, 
                                durationPre,        // set at trigger position
                                trigname
                            );
                }
            else if ( CanOpenFile ( fileoutmrk ) )
                DeleteFileExtended ( fileoutmrk );
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( SaveSD ) {

            expsd   = new TExportTracks;

            expsd->Filename             = TriggerFileName + "." InfixSD "." + fileoutext;
            expsd->SetAtomType ( AtomTypeScalar );
            expsd->NumTracks            = NumElectrodes;
            expsd->NumTime              = NumTimeFrames;
            expsd->SamplingFrequency    = SamplingFrequency;
            expsd->SelTracks            = OutputTracks;
            expsd->ElectrodesNames      = ElectrodesNames;
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( SaveSE ) {

            expse   = new TExportTracks;

            expse->Filename             = TriggerFileName + "." InfixSE "." + fileoutext;
            expse->SetAtomType ( AtomTypeScalar );
            expse->NumTracks            = NumElectrodes;
            expse->NumTime              = NumTimeFrames;
            expse->SamplingFrequency    = SamplingFrequency;
            expse->SelTracks            = OutputTracks;
            expse->ElectrodesNames      = ElectrodesNames;
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        for ( int t = 0; t < NumTimeFrames; t++ )
        for ( int c = 0; c < NumElectrodes; c++ ) {

            avg     = Sum ( c, t ) / numsplittrg;

            sd      = sqrt ( abs ( (            SumSqr ( c, t ) 
                                     - Square ( Sum    ( c, t ) ) / numsplittrg ) 
                                   / NonNull ( numsplittrg - 1 ) 
                                 ) 
                           );

            se      = sd / sqrt ( numsplittrg );


            if ( expavg )   expavg->Write ( (float) avg           );
            if ( expsum )   expsum->Write ( (float) Sum ( c, t )  );
            if ( expsd  )   expsd ->Write ( (float) sd            );
            if ( expse  )   expse ->Write ( (float) se            );
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( expavg )   delete  expavg;
        if ( expsum )   delete  expsum;
        if ( expsd  )   delete  expsd;
        if ( expse  )   delete  expse;

        } // for alltriggers

    } // dosplittriggers


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

numtrg      = totalnumtrigg ? totalnumtrigg : 1;


verbose.NextLine ();
verbose.NextBlock ();
verbose.NextTopic ( "End of Computation:" );
verbose.NextLine ();


verbose.Put ( "Total number of triggers scanned :", totalnumtrigg, 4 );
sprintf ( buff, "%4d / %5.1f%%", totalnumtriggacc, 100.0 * totalnumtriggacc / numtrg );
verbose.Put ( "Total number of triggers accepted:", buff );
sprintf ( buff, "%4d / %5.1f%%", totalnumtriggrej, 100.0 * totalnumtriggrej / numtrg );
verbose.Put ( "Total number of triggers rejected:", buff );


//if ( SaveEpochsMultiFiles && totalnumtriggacc > 0 )
//    verbose.Put ( "Epoch files:                        " << BaseFileName << ".[" << 1 << ".." << totalnumtriggacc << "]." << fileoutext << fastendl fastendl;

verbose.NextLine ();
verbose.NextBlock ();


                                        // add a cookie
switch ( rand () % 10 ) {
    case 0 : (ofstream&) verbose << Tab "Congratulation if you have read all this file (haven't you?)!"               fastendl; break;
    case 1 : (ofstream&) verbose << Tab "Thank you Cartool!"                                                          fastendl; break;
    case 2 : (ofstream&) verbose << Tab "That's all folks!"                                                           fastendl; break;
    case 3 : (ofstream&) verbose << Tab "Et voila!"                                                                   fastendl; break;
    case 4 : (ofstream&) verbose << Tab "Have you really understood what I've done?"                                  fastendl; break;
    case 5 : (ofstream&) verbose << Tab "Honestly, it was a real pleasure to compute this for you!"                   fastendl; break;
    case 6 : (ofstream&) verbose << Tab "Shouldn't I have been paid for this, how computers are treated nowadays.."   fastendl; break;
    case 7 : (ofstream&) verbose << Tab "Wow! that's already done?!"                                                  fastendl; break;
    case 8 : (ofstream&) verbose << Tab ";-)"                                                                         fastendl; break;
    case 9 : (ofstream&) verbose << Tab "This message will destruct itself in 10 seconds..."                          fastendl; break;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // last freq? do the big merge(s)
if ( IsFreqLoop () && IsLastFreq () ) {

    TGoF            filenames;
    TFileName       templ   ;
    TFileName       freqfile;


    if ( SaveSD ) {
        templ   = BaseDir + "\\*." InfixSD "." + fileoutext;

        MergeTracksToFreqFiles ( templ, FreqType );
        }

    if ( SaveSE ) {
        templ   = BaseDir + "\\*." InfixSE "." + fileoutext;

        MergeTracksToFreqFiles ( templ, FreqType );
        }

    if ( SaveSum ) {
        templ   = BaseDir + "\\*." InfixSum "." + fileoutext;

        MergeTracksToFreqFiles ( templ, FreqType, fileoutsum );
                                        // re-order results
        if ( ignorepolarity )
            SortFFTApproximation ( fileoutsum );
        }

    if ( SaveAverage ) {
        templ   = BaseDir + "\\*." + fileoutext;

        MergeTracksToFreqFiles ( templ, FreqType, fileoutavg );
                                        // re-order results
        if ( ignorepolarity )
            SortFFTApproximation ( fileoutavg );
        }


    if ( dosplittriggers ) {
                                        // browse each trigger separately
        for ( int trig = 0; trig < alltriggers.GetNumTokens (); trig++ ) {

                                        // sub-directory for split triggers
            file    = BaseDir + "\\" InfixTrigger " " + alltriggers[ trig ];

            if ( SaveSD ) {
                templ   = file + "\\*." InfixSD "." + fileoutext;

                MergeTracksToFreqFiles ( templ, FreqType );
                }

            if ( SaveSE ) {
                templ   = file + "\\*." InfixSE "." + fileoutext;

                MergeTracksToFreqFiles ( templ, FreqType );
                }

            if ( SaveSum ) {
                templ   = file + "\\*." InfixSum "." + fileoutext;

                MergeTracksToFreqFiles ( templ, FreqType, freqfile );
                                        // re-order results
                if ( ignorepolarity )
                    SortFFTApproximation ( freqfile );
                }

            if ( SaveAverage ) {
                templ   = file + "\\*." + fileoutext;

                MergeTracksToFreqFiles ( templ, FreqType, freqfile );
                                        // re-order results
                if ( ignorepolarity )
                    SortFFTApproximation ( freqfile );
                }
            }

        } // dosplittriggers

    } // IsLastFreq


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // successfully finished
                                        // complimentary opening the file for the user
if ( openauto ) {

    if ( MergeTriggers ) {

        if      ( SaveAverage )        // filename has been updated in case of IsFreqLoop

            fileoutavg.Open ();

        else if ( SaveSum )

            fileoutsum.Open ();
        }


    if ( dosplittriggers ) {

        char            trigname [ MarkerNameMaxLength ];
        TFileName       trigfile;


        for ( int trig = 0; trig < alltriggers.GetNumTokens (); trig++ ) {

            StringCopy ( trigname, alltriggers[ trig ] );

            if ( SaveAverage ) {

                sprintf ( trigfile,  "%s\\" InfixTrigger " %s\\%s." InfixTrigger " %s.%s", (char*) BaseDir, trigname, (char*) fileoutprefix, trigname, IsFreqLoop () ? FILEEXT_FREQ : (char*) fileoutext );

                trigfile.Open ();
                }
            else if ( SaveSum && MergeTriggers ) {
                                        // we have to retrieve the correct filename...
                if ( IsFreqLoop () )
                    sprintf ( trigfile,  "%s\\" InfixTrigger " %s\\%s." InfixTrigger " %s.%s.%s", (char*) BaseDir, trigname, (char*) fileoutprefix, trigname, InfixSum, FILEEXT_FREQ );
                else
                    sprintf ( trigfile,  "%s\\" InfixTrigger " %s\\%s." InfixTrigger " %s.%s *.%s", (char*) BaseDir, trigname, (char*) fileoutprefix, trigname, InfixSum, (char*) fileoutext );

                GetFirstFile ( trigfile );

                trigfile.Open ();
                }
            } // for trig
        } // dosplittriggers

                                        // open epochs only if 1 file, and if average was not computed
    if ( SaveEpochsSingleFile && ! IsFreqLoop ()
      && ! ( SaveAverage || SaveSum ) )

        fileout1epoch.Open ();


//  UpdateApplication;
    }

else if ( ! IsFreqLoop () || IsLastFreq () ) {

    TSuperGauge         Gauge ( "Averaging Files" );
    Gauge.HappyEnd ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // clean up everything allocated
deallocate:

/*
if ( InterpolateDlg ) {
    // also close the docs
    delete InterpolateDlg;
    }
*/

if ( usexyz ) {
//  XyzDoc->RemoveLink ( (TDocument *) this );     // remove the xyz doc
    XyzDoc->AllowClosing ();

//  XyzDoc->Revert ();

    if ( closexyz ) {
        CartoolDocManager->CloseDoc ( XyzDoc );
        XyzDoc      = 0;
        }
    }

                                        // close and delete
DeleteFileExtended ( fileoutsplit    );
DeleteFileExtended ( fileoutsplitmrk );


//canClose = true;                        // allow to close, now
//TBaseDialog::CmOk();
}

}
