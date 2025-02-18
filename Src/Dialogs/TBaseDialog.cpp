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

#include    "MemUtil.h"

#include    "Files.ReadFromHeader.h"
#include    "Files.SpreadSheet.h"
#include    "Files.TSplitLinkManyFile.h"
#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"

#include    "TBaseDoc.h"
#include    "TFreqDoc.h"

#include    "TBaseView.h"
#include    "TTracksView.h"

#include    "TBaseDialog.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const char SavingEegFileTypePreset[ NumSavingEegFileTypes ][ 64 ] =
                                        {
                                        "Text    file ." FILEEXT_TXT "    (plain matrix)",
                                        "Text    file ." FILEEXT_EEGEP "    (plain matrix)",
                                        "Text    file ." FILEEXT_EEGEPH "  (header + plain matrix)",
                                        "Binary file ." FILEEXT_EEGSEF "   (Simple EEG Format)",
                                        "Binary file ." FILEEXT_EEGBV "  (BrainVision format)",
                                        "Binary file ." FILEEXT_EEGEDF "   (avoid this if possible)",
                                        "Binary file ." FILEEXT_RIS "    (Results of Inverse Solutions)",
                                        };

const char SavingEegFileExtPreset[ NumSavingEegFileTypes ][ 8 ] =
                                        {
                                        FILEEXT_TXT,
                                        FILEEXT_EEGEP,
                                        FILEEXT_EEGEPH,
                                        FILEEXT_EEGSEF,
                                        FILEEXT_EEGBV,
                                        FILEEXT_EEGEDF,
                                        FILEEXT_RIS,
                                        };


SavingEegFileTypes  ExtensionToSavingEegFileTypes   ( const char* ext )
{
for ( int i = 0; i < NumSavingEegFileTypes; i++ )
    if ( StringIs ( SavingEegFileExtPreset [ i ], ext ) )
        return (SavingEegFileTypes) i;

return  PresetFileTypeDefaultEEG;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TBaseDialog, TDialog )

    EV_COMMAND                  ( IDC_PROCESSCURRENT,       CmProcessCurrent ),
    EV_COMMAND                  ( IDC_PROCESSBATCH,         CmBatchProcess ),
    EV_COMMAND                  ( IDHELP,                   CmHelp ),

END_RESPONSE_TABLE;


        TBaseDialog::TBaseDialog ( TWindow* parent, TResId resId, TTracksDoc* doc )
      : TDialog ( parent, resId ), EEGDoc ( doc )
{
BatchProcessing     = true;
BatchFileIndex      = -1;

StringCopy ( BatchFilesExt, AllTracksFilesExt " " AllDataExt ); // default types of files is everything looking close to tracks

ResetFreq ();
}


void    TBaseDialog::ResetFreq ()
{
NumFreqs            = 0;
FreqIndex           = -1;
ClearString ( FreqName );
FreqType            = FreqUnknown;
}


//----------------------------------------------------------------------------
void    TBaseDialog::BatchProcessDropped ( TGoF &gof )
{
if ( ! (bool) gof || ! BatchProcessing )
    return;


BatchFileNames.Set ( gof );

BatchProcess ();

BatchFileNames.Reset ();
}


//----------------------------------------------------------------------------
void    TBaseDialog::BatchProcessLmFile ( char *lmfile )
{
if ( StringIsSpace ( lmfile ) || ! BatchProcessing )
    return;


TSplitLinkManyFile  lm ( lmfile );


if ( ! ( (bool) lm.leeg || (bool) lm.lris ) )
    return;

                                        // transfer list of filenames
BatchFileNames.Reset ();


for ( int gofi = 0; gofi < lm.leeg.NumFiles (); gofi++ )

    BatchFileNames.Add ( lm.leeg[ gofi ] );


for ( int gofi = 0; gofi < lm.lris.NumFiles (); gofi++ )

    BatchFileNames.Add ( lm.lris[ gofi ] );


BatchProcess ();

BatchFileNames.Reset ();
}


//----------------------------------------------------------------------------
void    TBaseDialog::BatchProcessSplitFile ( char *splitfile )
{
if ( StringIsSpace ( splitfile ) || ! BatchProcessing )
    return;


TSpreadSheet        sf;


if ( ! sf.ReadFile ( splitfile ) )
    return;

                                        // does split file has files?
if ( ! ( sf.HasAttribute ( "file" ) || sf.HasAttribute ( "file1" ) ) )
    return;


char            attr [ MaxPathShort ];
char            buff [ MaxPathShort ];
int             numfiles    = 0;
int             f;

                                        // cumulate total number of files
for ( int r = 0; r < sf.GetNumRecords (); r++ ) {
    if      ( sf.GetRecord ( r, "numfiles", f ) )
        numfiles    += f;
    else if ( sf.GetRecord ( r, "file", attr ) )
        numfiles++;
    }


if ( numfiles == 0 )
    return;


BatchFileNames.Reset ();


for ( int r = 0; r < sf.GetNumRecords (); r++ ) {
                                        // get numfiles for the current record
    if ( ! sf.GetRecord ( r, "numfiles", numfiles ) ) {
        sf.GetRecord ( r, "file", attr );
        BatchFileNames.Add ( attr );
        }
    else
                                        // copy each file name
        for ( int i = 0; i < numfiles; i++ ) {
            sprintf ( buff, "file%0d", i + 1 );
            sf.GetRecord ( r, buff, attr );
            BatchFileNames.Add ( attr );
            }
    }


BatchProcess ();

BatchFileNames.Reset ();
}


//----------------------------------------------------------------------------
                                        // Called from button pressed
void    TBaseDialog::CmProcessCurrent ()
{
if ( EEGDoc )                           // by safety - dialog shouldn't actually enable a ProcessCurrent button without any current EEGDoc
    CmProcessCurrent ( EEGDoc->GetDocPath () );
else
    Destroy ();                         // kill it with fire
}


void    TBaseDialog::CmProcessCurrent ( const char* file )
{
TransferData ( tdGetData );


if ( ! BatchProcessing ) {

//  WindowMinimize ( this );

    Destroy ();

    CartoolMdiClient->RefreshWindows ();

    UpdateApplication;
    }


BatchFileNames.SetOnly ( file );        // setting only the current file name

BatchProcess ();

BatchFileNames.Reset ();
}


//----------------------------------------------------------------------------
                                        // Called from button pressed
                                        // Batch default type is tracks
void    TBaseDialog::CmBatchProcess ()
{
CmBatchProcess ( AllErpEegFreqRisFilesFilter );
}


void    TBaseDialog::CmBatchProcess ( const char* filesfilter )
{
TransferData ( tdGetData );


static GetFileFromUser  getfiles ( "Batch Files:", filesfilter, 1, GetFileMulti );

                                        // ask files to process
if ( ! getfiles.Execute () ) {
    this->SetFocus ();
    return;
    }


BatchFileNames.Set ( (const TGoF &) getfiles );   // setting multiple files

BatchProcess ();

BatchFileNames.Reset ();
}


//----------------------------------------------------------------------------
                                        // Default just calls ProcessCurrent - The advantage is for a derived class to override this for specific needs, like frequency loops
void    TBaseDialog::BatchProcessCurrent ()
{
ProcessCurrent ( 0, 0 );
}


//----------------------------------------------------------------------------
void    TBaseDialog::BatchProcess ()
{
Destroy ();

                                        // no file(s) in list?
if ( ! HasBatchFiles () )
    return;

                                        // processing current, but doc pointer is null?
if ( ! BatchProcessing )
    if ( EEGDoc == 0 )
        return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numinfiles      = BatchFileNames.NumFiles ();
                                        // position 0:real processing; 1:frequencies; 2:(this) file level
TSuperGauge         Gauge ( "File", numinfiles > 1 ? numinfiles : 0, SuperGaugeLevelBatch );


TTracksViewState    tracksviewstate;
int                 numsessions;
int                 currsession;

                                        // loop through the eeg files
for ( BatchFileIndex = 0; BatchFileIndex < numinfiles ; BatchFileIndex++ ) {

    Gauge.Next ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( BatchProcessing ) {
                                        // open the doc myself
        EEGDoc      = dynamic_cast<TTracksDoc*> ( CartoolDocManager->OpenDoc ( BatchFileNames[ BatchFileIndex ], dtOpenOptionsNoView ) );

        if ( ! EEGDoc )                 // oops, not gonna like it
            continue;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we have to loop through sessions, too!
    numsessions     = EEGDoc->GetNumSessions () > 0 ? EEGDoc->GetNumSessions () : 1;
    currsession     = ! BatchProcessing ? EEGDoc->GetCurrentSession () : 0;

                                        // save some of the display states, if view exists & there are sessions
    if ( numsessions > 1 && ! BatchProcessing ) {

        TTracksView        *tracksview      = dynamic_cast< TTracksView * > ( EEGDoc->GetViewList () );

        if ( tracksview )
            tracksviewstate.SaveState ( tracksview );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // process all sessions, even if only current file open - this could be an option to just process the current process in that case
    for ( int s = 1; s <= numsessions; s++ ) {

        Gauge.Actualize ();

        if ( numsessions > 1 )
            EEGDoc->GoToSession ( s );

        BatchProcessCurrent ();         // calls ProcessCurrent, or an overloaded function
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // restore current session
    if ( numsessions > 1 && ! BatchProcessing ) {

        Gauge.Actualize ();

        EEGDoc->GoToSession ( currsession );
                                        // also try to restore current view to its original appearance
        TTracksView        *tracksview      = dynamic_cast< TTracksView * > ( EEGDoc->GetViewList () );

        if ( tracksview )               // restore some of the display states
            tracksviewstate.RestoreState ( tracksview );
        }

                                        // then close eeg
    if ( BatchProcessing && EEGDoc ) {
                                        // just to shut-up everybody complaining about "modified file"
//      EEGDoc->DeactivateFilters ();
//      EEGDoc->SetDirty          ( false );

        if ( EEGDoc->CanClose ( true ) )
            CartoolDocManager->CloseDoc ( EEGDoc );
        }

    }


Gauge.Finished ();

                                        // reset reference to current doc
EEGDoc      = 0;
}


//----------------------------------------------------------------------------
void    TBaseDialog::BatchProcessGroups ( TGoGoF *gogof, int bygroupsof, int testinggroups, void *usetransfer, bool endadvertised )
{
int                 numgroups       = gogof->NumGroups ();

TSuperGauge         GaugeG ( "Group of Files", numgroups / bygroupsof, SuperGaugeLevelBatch );


for ( int g = 0; g < numgroups; g += bygroupsof ) {

    GaugeG.Next ();


    if ( gogof->AllFreqsGroup ( g, g + testinggroups - 1 ) ) {

        ReadFromHeader ( (*gogof)[ g ].GetFirst (), ReadNumFrequencies, &NumFreqs );


        TSuperGauge         GaugeF ( "Frequency", NumFreqs, SuperGaugeLevelInter );

                                        // run an outer loop by frequency
        for ( FreqIndex = 0; FreqIndex < NumFreqs; FreqIndex++ ) {

            GaugeF.Next ();
                                        // !it does not actually set a frequency, but just this field for information!
            ProcessGroups ( gogof, g, g + bygroupsof - 1, usetransfer );
            } // for FreqIndex


        ResetFreq ();
        }
    else // no freqs
        ProcessGroups     ( gogof, g, g + bygroupsof - 1, usetransfer );
    }


if ( endadvertised )    GaugeG.HappyEnd ();
else                    GaugeG.Finished ();
}


//----------------------------------------------------------------------------
void    TBaseDialog::BeginModal ()
{
if ( GetIsModal () )                    // already in modal mode?
    return;

SetIsModal ( true );                    // just for safety

CartoolApplication->BeginModal ( 0, MB_APPLMODAL );
}


void    TBaseDialog::EndModal ()
{
if ( ! GetIsModal () )                  // not in modal mode?
    return;

CartoolApplication->EndModal ( 0 );

SetIsModal ( false );                   // just for safety
}


//----------------------------------------------------------------------------
                                        // Called from a dialog - dispatch Help according to caller resource's Id
void    TBaseDialog::CmHelp ()
{
uint                ResId           = PtrToUint ( Attr.Name );


TFileName           helpaddress     = GiHubCartoolRefGuide;

if      ( ResId == IDD_STATISTICS1              )   helpaddress    += "/computing-statistics.html"               "#dialog-files";
else if ( ResId == IDD_STATISTICS2              )   helpaddress    += "/computing-statistics.html"               "#dialog-parameters";
else if ( ResId == IDD_AVERAGE1                 )   helpaddress    += "/computing-erps.html"                     "#dialog-files";
else if ( ResId == IDD_AVERAGE2                 )   helpaddress    += "/computing-erps.html"                     "#dialog-parameters";
else if ( ResId == IDD_AVERAGE3                 )   helpaddress    += "/computing-erps.html"                     "#dialog-output";
else if ( ResId == IDD_AVERAGECONTROL           )   helpaddress    += "/computing-erps.html"                     "#dialog-control";
else if ( ResId == IDD_TRACKSFILTERS            )   helpaddress    += "/eeg-display.html"                        "#filters";
else if ( ResId == IDD_EXPORTTRACKS             )   helpaddress    += "/reprocess-tracks.html";
else if ( ResId == IDD_SEGMENT1                 )   helpaddress    += "/microstates-segmentation.html"           "#segmentation-files-dialog";
else if ( ResId == IDD_SEGMENT2                 )   helpaddress    += "/microstates-segmentation.html"           "#segmentation-parameters-dialog";
else if ( ResId == IDD_FITTING1                 )   helpaddress    += "/microstates-back-fitting-templates.html" "#fitting-files-dialog";
else if ( ResId == IDD_FITTING2                 )   helpaddress    += "/microstates-back-fitting-templates.html" "#fitting-parameters-dialog";
else if ( ResId == IDD_FITTING3                 )   helpaddress    += "/microstates-back-fitting-templates.html" "#fitting-results-dialog";
else if ( ResId == IDD_INTERPOLATE              )   helpaddress    += "/tracks-interpolation.html"               "#dialog";
else if ( ResId == IDD_SCANMARKERS              )   helpaddress    += "/eeg-display.html"                        "#scanning-triggers";
else if ( ResId == IDD_FREQANALYSIS             )   helpaddress    += "/frequency-analysis.html";
else if ( ResId == IDD_CREATEROIS               )   helpaddress    += "/creating-rois.html";
else if ( ResId == IDD_CALCULATOR               )   helpaddress    += "/file-calculator.html";
else if ( ResId == IDD_INVERSEMATRICES          )   helpaddress    += "/computing-inverse-solution-matrices.html";
else if ( ResId == IDD_RISCOMPUTATION           )   helpaddress    += "/computing-ris.html";
else if ( ResId == IDD_MRIPREPROCESSING         )   helpaddress    += "/mris-preprocessing.html";
else if ( ResId == IDD_RISTOVOLUME              )   helpaddress    += "/ris-to-volumes.html";
else if ( ResId == IDD_COREGISTRATION           )   helpaddress    += "/coregistering-electrodes-to-mri.html";
//else                                              ;               // all other cases: land on the main page


ShellExecute    ( NULL, "open", helpaddress, NULL, NULL, SW_SHOWNORMAL );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
