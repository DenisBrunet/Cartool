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

#include    "TFileCalculatorDialog.h"

#include    "Strings.Utils.h"
#include    "Files.Utils.h"
#include    "Files.SpreadSheet.h"
#include    "Files.ReadFromHeader.h"
#include    "Files.TSplitLinkManyFile.h"
#include    "Files.TGoF.h"
#include    "TInverseResults.h"

#include    "FileCalculator.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // declare this global variable
TFileCalculatorStruct       FileCalculatorTransfer;
                                        // declare this static / shared variable
TGoGoF                      TFileCalculatorDialog::GoGoF;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TFileCalculatorStruct::TFileCalculatorStruct ()
{
StringCopy ( NumGroups, "0" );
GroupsSummary.Clear ();

//TStringArray   &expression  = Expression.GetStrings ();
//expression[ 0 ] = "scalar ( group1 * group2 )";

ClearString ( BaseDir );
CompoundFilenames  = BoolToCheck (  true );
GenericFilenames   = BoolToCheck ( false );

FileTypes.Clear ();
for ( int i = 0; i < NumSavingEegFileTypes; i++ )
    FileTypes.AddString ( SavingEegFileTypePreset[ i ], i == 0 );
FileTypes.Select ( PresetFileTypeEph );

StringCopy ( Regularization, DefaultRegularization );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TFileCalculatorDialog, TBaseDialog )

    EV_WM_DROPFILES,

    EV_COMMAND                  ( IDC_UPDIRECTORY,          CmUpOneDirectory ),
    EV_COMMAND                  ( IDC_BROWSEBASEFILENAME,   CmBrowseBaseFileName ),
    EV_COMMAND_AND_ID           ( IDC_ADDGROUP,             CmAddGroup ),
    EV_COMMAND_AND_ID           ( IDC_USELASTGROUP,         CmAddGroup ),
    EV_COMMAND                  ( IDC_REMFROMLIST,          CmRemoveGroup ),
    EV_COMMAND                  ( IDC_CLEARLISTS,           CmClearGroups ),
    EV_COMMAND                  ( IDC_SORTLISTS,            CmSortGroups ),
    EV_COMMAND                  ( IDC_READPARAMS,           CmReadParams ),
    EV_COMMAND                  ( IDC_WRITEPARAMS,          CmWriteParams ),

    EV_COMMAND                  ( IDOK,                     CmOk ),
    EV_COMMAND_ENABLE           ( IDOK,                     CmOkEnable ),

END_RESPONSE_TABLE;


        TFileCalculatorDialog::TFileCalculatorDialog ( TWindow* parent, TResId resId )
      : TBaseDialog ( parent, resId )
{
StringCopy ( BatchFilesExt, AllTracksFilesExt " " AllDataExt " " AllInverseFilesExt );


NumGroups           = new TEdit ( this, IDC_NUMGROUPS, EditSizeValue );
GroupsSummary       = new TListBox ( this, IDC_GROUPSSUMMARY );

Expression          = new TComboBox ( this, IDC_EXPRESSION );

BaseDir             = new TEdit ( this, IDC_BASEFILENAME, EditSizeText );
CompoundFilenames   = new TRadioButton ( this, IDC_COMPOUNDFILENAMES );
GenericFilenames    = new TRadioButton ( this, IDC_GENERICFILENAMES );
FileTypes           = new TComboBox ( this, IDC_FILETYPES );

Regularization      = new TEdit ( this, IDC_REGULARIZATION, EditSizeText );


SetTransferBuffer ( &FileCalculatorTransfer );
}


        TFileCalculatorDialog::~TFileCalculatorDialog ()
{
delete  NumGroups;              delete  GroupsSummary;
delete  Expression;
delete  BaseDir;
delete  CompoundFilenames;      delete  GenericFilenames;
delete  FileTypes;
delete  Regularization;
}


//----------------------------------------------------------------------------
void    TFileCalculatorDialog::EvDropFiles ( TDropInfo drop )
{
TGoF                tracksfiles     ( drop, AllTracksFilesExt " " AllDataExt );
TGoF                isfiles         ( drop, AllInverseFilesExt  );
TGoF                lmfiles         ( drop, FILEEXT_LM          );
TGoF                spreadsheetfiles( drop, SpreadSheetFilesExt );
TGoF                remainingfiles  ( drop, AllTracksFilesExt " " AllDataExt " " AllInverseFilesExt " " FILEEXT_LM " " SpreadSheetFilesExt, 0, true );

drop.DragFinish ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) spreadsheetfiles; i++ )
    ReadParams ( spreadsheetfiles[ i ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) tracksfiles ) {

    for ( int i = 0; i < (int) tracksfiles; i++ )
        AddFileToGroup ( tracksfiles[ i ], i == 0 );


    if ( CheckGroups ( GoGoF.GetLast () ) )
        AddGroupSummary ( GoGoF.NumGroups () - 1 );
    else
        GoGoF.RemoveLastGroup ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) isfiles ) {

    for ( int i = 0; i < (int) isfiles; i++ )
        AddFileToGroup ( isfiles[ i ], i == 0 );


    if ( CheckGroups ( GoGoF.GetLast () ) )
        AddGroupSummary ( GoGoF.NumGroups () - 1 );
    else
        GoGoF.RemoveLastGroup ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) lmfiles ) {

    bool                first;

    for ( int i = 0; i < (int) lmfiles; i++ ) {

        TSplitLinkManyFile      lm ( lmfiles[ i ] );
        TListIterator<char>     iterator;

                                        // coming from .lm, process separately the eeg
        first = true;

        if ( (bool) lm.leeg ) {

            foreachin ( lm.leeg, iterator )

                if ( ! IsExtension ( iterator (), FILEEXT_FREQ ) ) {    // filters out frequency files
                    AddFileToGroup ( iterator (), first );

                    first = false;
                    }

            if ( CheckGroups ( GoGoF.GetLast () ) )
                AddGroupSummary ( GoGoF.NumGroups () - 1 );
            else
                GoGoF.RemoveLastGroup ();
            }

                                        // then process separately the ris
        first = true;

        if ( (bool) lm.lris ) {

            foreachin ( lm.lris, iterator ) {

                AddFileToGroup ( iterator (), first );

                first = false;
                }

            if ( CheckGroups ( GoGoF.GetLast () ) )
                AddGroupSummary ( GoGoF.NumGroups () - 1 );
            else
                GoGoF.RemoveLastGroup ();
            }
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) remainingfiles )
    remainingfiles.Show ( "Skipping non-relevant file:" );
}


//----------------------------------------------------------------------------
void    TFileCalculatorDialog::AddFileToGroup ( const char* filename, bool first )
{
if ( first )                            // add a new group
    GoGoF.Add ( new TGoF );


GoGoF.GetLast ().Add ( filename, MaxPathShort );
}


void    TFileCalculatorDialog::GuessOutputFileExtension ()
{
if ( GoGoF.IsEmpty () )
    return;

bool                allris  = GoGoF.AllExtensionsAre ( FILEEXT_RIS    );
bool                alleph  = GoGoF.AllExtensionsAre ( FILEEXT_EEGEPH );
bool                allep   = GoGoF.AllExtensionsAre ( FILEEXT_EEGEP  );
bool                allbv   = GoGoF.AllExtensionsAre ( FILEEXT_EEGBV  );
bool                allsef  = GoGoF.AllExtensionsAre ( FILEEXT_EEGSEF );
bool                someis  = GoGoF.SomeExtensionsAre ( AllInverseFilesExt );

//DBGV5 ( allep, alleph, allsef, allris, someis, "allep, alleph, allsef, allris, someis" );


TransferData ( tdGetData );

if      ( allris || someis )    FileCalculatorTransfer.FileTypes.Select ( PresetFileTypeRis );
else if ( alleph  )             FileCalculatorTransfer.FileTypes.Select ( PresetFileTypeEph );
else if ( allep   )             FileCalculatorTransfer.FileTypes.Select ( PresetFileTypeEp  );
else if ( allbv   )             FileCalculatorTransfer.FileTypes.Select ( PresetFileTypeBV  );
else if ( allsef  )             FileCalculatorTransfer.FileTypes.Select ( PresetFileTypeSef );
else                            FileCalculatorTransfer.FileTypes.Select ( PresetFileTypeDefaultEEG );

TransferData ( tdSetData );
}


//----------------------------------------------------------------------------
void    TFileCalculatorDialog::CmReadParams ()
{
ReadParams ();
}


void    TFileCalculatorDialog::ReadParams ( char *filename )
{
TFileName           realfilename;

if ( StringIsNotEmpty ( filename ) )     // could be null, which we don't really like...
    StringCopy ( realfilename, filename );
else
    ClearString ( realfilename );


TSpreadSheet        sf;

if ( ! sf.ReadFile ( realfilename ) )
    return;


CsvType             csvtype         = sf.GetType ();

                                        // We can deal with statistics type, too...
if ( ! ( IsCsvFileCalculator ( csvtype ) || IsCsvStatFiles ( csvtype ) ) ) {
    ShowMessage ( SpreadSheetErrorMessage, "Read list file", ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                attr [ MaxPathShort ];
char                buff [ MaxPathShort ];
TGoF*               gof;


for ( int file = 0; file < sf.GetNumRecords (); file ++ ) {

    gof     = new TGoF;
    GoGoF.Add ( gof );


    sf.GetRecord ( file, "numfiles",    attr );
    int     numfiles    = StringToInteger ( attr );

                                        // transfer the filenames...
    for ( int i = 0; i < numfiles; i++ ) {
        sprintf ( buff, "file%0d", i + 1 );
        sf.GetRecord ( file, buff, attr );

        gof->Add ( attr );
        } // for file


    AddGroupSummary ( GoGoF.NumGroups () - 1 );
    } // for record
}


void    TFileCalculatorDialog::CmWriteParams ()
{
if ( GoGoF.IsEmpty () )
    return;


TSpreadSheet        sf;


if ( ! sf.WriteFile () )
    return;

                                        // header line describes the attributes / fields
sf.WriteAttribute ( "numfiles" );

                                        // we need to give the largest amount of files
int             minnumfiles = 0;

TListIterator<TGoF>     gofiterator;

foreachin ( GoGoF, gofiterator )
    minnumfiles = max ( minnumfiles, gofiterator ()->NumFiles () );


for ( int i = 0; i < minnumfiles; i++ )
    sf.WriteAttribute ( "file", i + 1 );

sf.WriteNextRecord ();

                                        // now write each line
foreachin ( GoGoF, gofiterator ) {
                                        // write parameters
    sf.WriteAttribute ( gofiterator ()->NumFiles () );

                                        // write all files
    TListIterator<char>     iterator;

    foreachin ( *gofiterator (), iterator )
        sf.WriteAttribute ( iterator () );

                                        // complete line with empty attributes
    for ( int i = gofiterator ()->NumFiles (); i < minnumfiles; i++ )
        sf.WriteAttribute ( "" );

    sf.WriteNextRecord ();
    }


sf.WriteFinished ();
}


//----------------------------------------------------------------------------
void    TFileCalculatorDialog::CmAddGroup ( owlwparam w )
{
TransferData ( tdGetData );


static GetFileFromUser  getfiles ( "", AllErpEegRisFilesFilter, 1, GetFileMulti );
TGoF               *gof         = new TGoF;


if ( w == IDC_ADDGROUP || GoGoF.IsEmpty () ) {
                                            // get eeg file names
    if ( ! getfiles.Execute () )
        return;

    gof->Set ( (const TGoF &) getfiles );
    }
else { // IDC_USELASTGROUP

    gof->Set ( GoGoF.GetLast () );
    }


if ( CheckGroups ( *gof ) ) {
    GoGoF.Add ( gof, false, MaxPathShort );
    AddGroupSummary ( GoGoF.NumGroups () - 1 );
    }
else
    delete  gof;
}


void    TFileCalculatorDialog::CmRemoveGroup ()
{
if ( GoGoF.IsEmpty () )
    return;


GoGoF.RemoveLastGroup ();
GroupsSummary->DeleteString ( 0 );


TransferData ( tdGetData );
IntegerToString ( FileCalculatorTransfer.NumGroups, GoGoF.NumGroups () );
TransferData ( tdSetData );

SetBaseFilename ();
}


void    TFileCalculatorDialog::CmClearGroups ()
{
GoGoF.Reset ();

GroupsSummary->ClearList ();

TransferData ( tdGetData );
IntegerToString ( FileCalculatorTransfer.NumGroups, GoGoF.NumGroups () );
TransferData ( tdSetData );

SetBaseFilename ();
}


void    TFileCalculatorDialog::CmSortGroups ()
{
if ( GoGoF.IsEmpty () )
    return;


GroupsSummary->ClearList ();


TListIterator<TGoF>     gofiterator;

foreachin ( GoGoF, gofiterator ) {

    gofiterator ()->Sort ();

    AddGroupSummary ( (int) gofiterator );
    }
}


//----------------------------------------------------------------------------
bool    TFileCalculatorDialog::CheckGroups ( const TGoF& gof )
{
const char*         toc;
int                 numscanned      = gof.NumFiles ();
//int                 numscanned      = min ( gof.NumFiles (), 100 );    // poll only this amount of files, assume it's enough!


                                        // only for tracks
if ( IsExtensionAmong ( gof[ 0 ], AllTracksFilesExt " " AllDataExt ) ) {

/*
                                        // all groups with tracks have the same # of files?
    for ( i = 0; i < ng; i++ )
        if ( IsExtensionAmong ( GoGoF[ i ][ 0 ], AllTracksFilesExt" "AllDataExt ) && GoGoF[ i ].NumFiles () != numinfiles )
            break;

    if ( ng && i != ng ) {
        ShowMessage ( "Not the same number of files with previous groups!", "Group of Files", ShowMessageWarning );
        return false;
        };
*/

                                        // same duration within group?
    int                 numtf;
    int                 numtfref = 0;

    for ( int i = 0; i < numscanned; i++ ) {

        toc = gof[ Round ( ( i / (double) NonNull ( numscanned - 1 ) ) * ( gof.NumFiles () - 1 ) ) ];

        if ( ! ReadFromHeader ( toc, ReadNumTimeFrames, &numtf ) ) {
            ShowMessage ( "Can not read the number of Time Frames!", toc, ShowMessageWarning );
            return false;
            }

        if ( numtfref == 0 )
            numtfref = numtf;
        else if ( numtf != numtfref ) {
            ShowMessage ( "Not the same number of Time Frames within group!", toc, ShowMessageWarning );
            return false;
            }
        }

/*
                                        // same duration across group?
    for ( int i = 0; i < ng; i++ ) {
        toc = GoGoF[ i ][ 0 ];

        if ( ! IsExtensionAmong ( toc, AllTracksFilesExt" "AllDataExt ) )
            continue;

        if ( ! ReadFromHeader ( toc, ReadNumTimeFrames, &numtf ) ) {
            ShowMessage ( "Can not read the number of Time Frames!", toc, ShowMessageWarning );
            return false;
            }

        if ( numtf != numtfref ) {
            ShowMessage ( "Not the same number of Time Frames across groups!", toc, ShowMessageWarning );
            return false;
            }
        }
*/
    } // only tracks


                                        // for all files
                                        // same # of electrodes within group?
int                 numel;
int                 numelref = 0;

for ( int i = 0; i < numscanned; i++ ) {

    toc = gof[ Round ( ( i / (double) NonNull ( numscanned - 1 ) ) * ( gof.NumFiles () - 1 ) ) ];

    if ( ! ReadFromHeader ( toc, ReadNumElectrodes, &numel ) ) {
        ShowMessage ( "Can not read the number of electrodes!", toc, ShowMessageWarning );
        return false;
        }

    if ( numelref == 0 )
        numelref = numel;
    else if ( numel != numelref ) {
        ShowMessage ( "Not the same number of electrodes within group!", toc, ShowMessageWarning );
        return false;
        }
    }


                                        // only solution points
if ( IsExtensionAmong ( gof[ 0 ], AllInverseFilesExt ) ) {
                                        // same # of solution points across groups?
    int                 numsp;
    int                 numspref = 0;


    for ( int i = 0; i < numscanned; i++ ) {

        toc = gof[ Round ( ( i / (double) NonNull ( numscanned - 1 ) ) * ( gof.NumFiles () - 1 ) ) ];

        if ( ! ReadFromHeader ( toc, ReadNumSolPoints, &numsp ) ) {
            ShowMessage ( "Can not read the number of solution points!", toc, ShowMessageWarning );
            return false;
            }

        if ( numspref == 0 )
            numspref = numsp;
        else if ( numsp != numspref ) {
            ShowMessage ( "Not the same number of solution points within group!", toc, ShowMessageWarning );
            return false;
            }
        }
    }


return true;
}


void    TFileCalculatorDialog::AddGroupSummary ( int gofi )
{
if ( gofi < 0 || gofi >= (int) GoGoF )
    return;


TGoF&               gof             = GoGoF[ gofi ];
char                buff [ MaxPathShort ];
char                buff2[ MaxPathShort ];

                                        // update dialog
if ( gof.NumFiles () > 1 ) {
    StringCopy ( buff2, ToFileName ( gof.GetFirst () ) );

    char                buff3[ MaxPathShort ];
    StringCopy ( buff3, ToFileName ( gof.GetLast () ) );

    StringAppend ( buff2, gof.NumFiles () == 2 ? ", " : " .. ", buff3 );
    }
else {
    StringCopy ( buff2, ToFileName ( gof.GetFirst () ) );
    }


sprintf ( buff, "Group %2d: %0d File%s ( %s )",
          gofi + 1,
          gof.NumFiles (),
          StringPlural ( gof.NumFiles (), true ),
          buff2 );


GroupsSummary->InsertString ( buff, 0 );

IntegerToString ( buff, GoGoF.NumGroups () );
NumGroups->Clear ();
NumGroups->Insert ( buff );

GuessOutputFileExtension ();

SetBaseFilename ();
}


//----------------------------------------------------------------------------
void    TFileCalculatorDialog::CmBrowseBaseFileName ()
{
static GetFileFromUser  getfile ( "Base File Name", AllFilesFilter, 1, GetFilePath );


if ( ! getfile.Execute ( FileCalculatorTransfer.BaseDir ) )
    return;


TransferData ( tdSetData );

BaseDir->ResetCaret;
}

                                        // generate a smart base name
void    TFileCalculatorDialog::SetBaseFilename ()
{
if ( GoGoF.IsEmpty () )
    return;

                                        // compose with only up to the first 2 groups
char                bigmatch    [ MaxPathShort ];


TListIterator<TGoF>     gofiterator;

foreachin ( GoGoF, gofiterator ) {

    char        match    [ MaxPathShort ];

    gofiterator ()->GetCommonString ( match, ! (int) gofiterator );


    if ( (int) gofiterator ) {
        if ( ! StringContains ( bigmatch, match ) )
            StringAppend ( bigmatch, " ", match );
        }
    else
        StringCopy ( bigmatch, match );
    }


if ( ! IsAbsoluteFilename ( bigmatch ) )
    return;


                                        // finally, compose the base name
PrefixFilename ( bigmatch, "Calc " );

//DBGM ( bigmatch, "BaseDir" );

BaseDir->SetText ( bigmatch );

BaseDir->ResetCaret;
}


void    TFileCalculatorDialog::CmUpOneDirectory ()
{
RemoveLastDir ( FileCalculatorTransfer.BaseDir );

BaseDir->SetText ( FileCalculatorTransfer.BaseDir );

BaseDir->ResetCaret;
}


//----------------------------------------------------------------------------
void    TFileCalculatorDialog::CmOkEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

                                        // directly playing with the buffer does not seem to be a good idea, maybe its updated during the test?
char                buff[ MaxPathShort ];
StringCopy ( buff, FileCalculatorTransfer.BaseDir );


if ( ! IsAbsoluteFilename ( buff ) ) {
    tce.Enable ( false );
    return;
    }


tce.Enable ( (bool) GoGoF /*&& ! StringIsEmpty ( exp )*/ );
}


//----------------------------------------------------------------------------
void    TFileCalculatorDialog::CmOk ()
{
Destroy ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // retrieve the expression
char                expr[ EditSizeTextLong ];
                                        // retrieving the expression
Expression->GetText ( expr, EditSizeTextLong );

                                        // check if expression is already stored in the combo box
char                buff[ EditSizeTextLong ];
int                 numcombo        = Expression->GetCount() - 1;

for ( ; numcombo >= 0; numcombo-- ) {

    Expression->GetString ( buff, numcombo );

    if ( StringIs ( buff, expr ) )
        break;
    }
                                        // this is a new expression, push it into the combo box for user's convenience
if ( numcombo == -1 )
    Expression->InsertString ( expr, 0 );

                                        // send to transfer buffer
TransferData ( tdGetData );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 presetsindex        =               FileCalculatorTransfer.FileTypes.GetSelIndex ();
bool                compoundfilenames   = CheckToBool ( FileCalculatorTransfer.CompoundFilenames );


FileCalculator  (   expr,
                    GoGoF,
                    FileCalculatorTransfer.Regularization,
                    FileCalculatorTransfer.BaseDir,         SavingEegFileExtPreset[ presetsindex ],     compoundfilenames
                );

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
