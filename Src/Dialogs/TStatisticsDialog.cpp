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

#include    "TStatisticsDialog.h"
#include    "Math.Statistics.h"

#include    "Math.Stats.h"
#include    "CartoolTypes.h"            // PolarityType
#include    "Strings.Utils.h"
#include    "Strings.Grep.h"
#include    "Files.Extensions.h"
#include    "Files.TVerboseFile.h"
#include    "Files.SpreadSheet.h"
#include    "Files.Conversions.h"
#include    "Files.ReadFromHeader.h"
#include    "Files.TSplitLinkManyFile.h"
#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"

#include    "TExportTracks.h"

#include    "TFreqDoc.h"
#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // declare this global variable
TStatStruct             StatTransfer;
                                        // declare this static / shared variable
TStatGoGoF              TStatisticsDialog::GoGoF;


const char  StatPresetsDataString[ NumStatPresets ][ 64 ] =
            {
            "EEG / Surface / Evoked Responses",
            "EEG / Surface / Spontaneous",
            "EEG / Intra-Cranial / Evoked Responses",
            "EEG / Intra-Cranial / Spontaneous",
            "",
            "Positive Data / Surface / Evoked Responses",
            "Positive Data / Surface / Spontaneous",
            "",
//          "Inverse Solution / Vectorial / Evoked Responses",
            "Inverse Solution / Scalar    / Evoked Responses",
//          "Inverse Solution / Vectorial / Spontaneous EEG",
            "Inverse Solution / Scalar    / Spontaneous EEG",
            "",
            "Frequency / Powers (absolute data)",
            "Frequency / FFT Approximation (signed data)",
            "",
            "Fitting Results",
            "Markov Chains Probabilities (positive data)",
            "",
            "Some Positive Data"
            };


const char  StatPresetsCorrectionString[ NumStatCorrPresets ][ 64 ] =
            {
            "No Correction",
            "Bonferroni Correction",
            "FDR - Optimally Thresholding p-Values",
//          "FDR - Correcting p-values to q-values",
            "FDR - Adjusting p-values to q-values",
            };


const char  StatTimeString      [ NumStatTimes ][ 64 ] =
            {
            "All values sequentially",
            "Mean of interval",
            "Median of interval",
            "Min of interval",
            "Max of interval",
            };

const char  StatTimeStringShort [ NumStatTimes ][ 32 ] =
            {
            "Sequentially",
            "Mean",
            "Median",
            "Min",
            "Max",
            };


const char  PairedTypeString[ NumPairedType ][ 16 ] =
            {
            "Unpaired",
            "Paired",
            };


const char  CorrectionTypeString[ NumCorrectionType ][ 64 ] =
            {
            "No Correction",
            "Bonferroni Correction",
            "FDR Correction, Thresholded p-values",
            "FDR Correction, Corrected q-values",
            "FDR Correction, Adjusted q-values",
            };


const char  OutputPTypeString[ NumOutputType ][ 16 ] =
            {
            "p",            // InfixP,          // using more "spacey" strings than the ones used for file names
            "1 - p",        // Infix1MinusP,
            "- log10 ( p )" // InfixLogP   
            };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TStatGoF::TStatGoF ()
{
Reset ();
}


void    TStatGoF::Reset ()
{
TGoF::Reset ();

TimeMin         = 0;
TimeMax         = 0;
EndOfFile       = false;
StatTime        = StatTimeDefault;

GroupIndex      = -1;
ClearString ( GroupName );
NumSamples      = 0;
}


//----------------------------------------------------------------------------
bool    TStatGoGoF::AllAreAverages ( int gofi1, int gofi2 ) const
{
for ( int gofi = gofi1; gofi <= gofi2 ; gofi++ )

    if ( ! (*this)[ gofi ].IsTimeAveraged () )

        return  false;

return  true;
}


bool    TStatGoGoF::IsSomeAverage ( int gofi1, int gofi2 )  const
{
for ( int gofi = gofi1; gofi <= gofi2 ; gofi++ )

    if ( (*this)[ gofi ].IsTimeAveraged () )

        return  true;

return  false;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TStatisticsFilesStruct::TStatisticsFilesStruct ()
{
SamplesNFiles               = BoolToCheck ( true  );
Samples1File                = BoolToCheck ( false );
SamplesCsvFile              = BoolToCheck ( false );

StringCopy  ( TimeMin, "0" );
ClearString ( TimeMax );
EndOfFile                   = BoolToCheck ( true  );

PresetsTime.Clear ();
for ( int i = 0; i < NumStatTimes; i++ )
    PresetsTime.AddString ( StatTimeString[ i ], i == StatTimeDefault );

StringCopy ( NumGroups, "0" );
GroupsSummary.Clear ();


//ClearString ( BaseFileName );
TFileName           cp;
cp.GetCurrentDir ();
StringAppend        ( cp, "\\Stat" );
StringCopy          ( BaseFileName, cp );


FileTypes.Clear ();
for ( int i = 0; i < NumSavingEegFileTypes; i++ )
    FileTypes.AddString ( SavingEegFileTypePreset[ i ], i == PresetFileTypeEp );


OutputP                     = BoolToCheck ( false );
Output1MinusP               = BoolToCheck ( true  );
OutputMinusLogP             = BoolToCheck ( false );

OpenAuto                    = BoolToCheck ( true  );
MoreOutputs                 = BoolToCheck ( false );
}


        TStatisticsParamsStruct::TStatisticsParamsStruct ()
{
ExportTracks                = BoolToCheck ( true  );
StringCopy ( Channels, "*" );
UseXyzDoc                   = BoolToCheck ( false );
ClearString ( XyzDocFile );

ExportRois                  = BoolToCheck ( false );
ClearString ( RoisDocFile );

UnpairedTest                = BoolToCheck ( false );
PairedTest                  = BoolToCheck ( true  );

TTest                       = BoolToCheck ( false );
Randomization               = BoolToCheck ( false );
TAnova                      = BoolToCheck ( false );


PresetsData.Clear ();
for ( int i = 0; i < NumStatPresets; i++ )
    PresetsData.AddString ( StatPresetsDataString[ i ], i == StatPresetDataDefault );

PositiveData                = BoolToCheck ( false );
SignedData                  = BoolToCheck ( true  );

AccountPolarity             = BoolToCheck ( true  );
IgnorePolarity              = BoolToCheck ( false );

NoRef                       = BoolToCheck ( true  );
AveRef                      = BoolToCheck ( false );

NormalizationGfp            = BoolToCheck ( false );
NormalizationGfpPaired      = BoolToCheck ( false );
NormalizationGfp1TF         = BoolToCheck ( false );
NormalizationNone           = BoolToCheck ( true  );

CheckMissingValues          = BoolToCheck ( false );
StringCopy ( CheckMissingValuesValue, "-1" );
StringCopy ( NumberOfRandomization, "5000" );


PresetsCorrection.Clear ();
for ( int i = 0; i < NumStatCorrPresets; i++ )
    PresetsCorrection.AddString ( StatPresetsCorrectionString[ i ], i == StatCorrDefault );

StringCopy ( FDRCorrectionValue, "5" );

ThresholdingPValues           = BoolToCheck ( false );
StringCopy ( ThresholdingPValuesValue, /*"1"*/ "5" );

MinSignificantDuration        = BoolToCheck ( false );
StringCopy ( MinSignificantDurationValue, "1" );
}


        TStatStruct::TStatStruct ()
{
LastDialogId                = IDD_STATISTICS1;
FileType                    = CM_STATISTICSTRACKS;
}


//----------------------------------------------------------------------------
int     TStatStruct::GetNumTF ( const TStatGoF& gof )   const
{
return  IsNFiles () ? gof.GetTimeDuration ()
                    : 1;
}

                                        // for a single group
int     TStatStruct::GetNumSamples ( const TStatGoF& gof )  const
{
return  IsNFiles  () ? gof.NumFiles ()
      : IsCsvFile () ? gof.NumSamples
      :                gof.GetTimeDuration ();
}

                                        // for a set of groups
int     TStatStruct::GetNumSamples ( const TStatGoGoF& gogof, int gofi1, int gofi2 )    const
{
int                 ns              = Highest<int> ();

for ( int gofi = gofi1; gofi <= gofi2 ; gofi++ )

    Mined ( ns, GetNumSamples ( gogof[ gofi ] ) );

return  ns;
}


int     TStatStruct::GetNumSequentialTF ( const TStatGoGoF& gogof, int gofi1, int gofi2 )   const
{
int                 numtf           = 1;

for ( int gofi = gofi1; gofi <= gofi2 ; gofi++ )

    if ( gogof[ gofi ].IsTimeSequential () ) 

        Maxed ( numtf, GetNumTF ( gogof[ gofi ] ) );

return numtf;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TStatisticsFilesDialog, TStatisticsDialog )

    EV_WM_DROPFILES,

    EV_COMMAND                  ( IDC_UPDIRECTORY,          CmUpOneDirectory ),
    EV_COMMAND                  ( IDC_BROWSEBASEFILENAME,   CmBrowseBaseFileName ),
    EV_COMMAND_AND_ID           ( IDC_ADDGROUP,             CmAddGroup ),
    EV_COMMAND_AND_ID           ( IDC_USELASTGROUP,         CmAddGroup ),
    EV_COMMAND                  ( IDC_REMFROMLIST,          CmRemoveGroup ),
    EV_COMMAND                  ( IDC_CLEARLISTS,           CmClearGroups ),
    EV_COMMAND                  ( IDC_SORTLISTS,            CmSortGroups ),
    EV_COMMAND                  ( IDC_READPARAMS,           CmReadSpreadSheet ),
    EV_COMMAND                  ( IDC_WRITEPARAMS,          CmWriteParams ),
    EV_COMMAND_AND_ID           ( IDC_SAMPLESNFILES,        CmSamplesParams ),
    EV_COMMAND_AND_ID           ( IDC_SAMPLES1FILE,         CmSamplesParams ),
    EV_COMMAND_AND_ID           ( IDC_SAMPLESCSVFILE,       CmSamplesParams ),
    EV_COMMAND_ENABLE           ( IDC_ADDGROUP,             CmAddGroupEnable ),
    EV_COMMAND_ENABLE           ( IDC_USELASTGROUP,         CmLastGroupEnable ),
    EV_COMMAND_ENABLE           ( IDC_TIMEMAX,              CmTimeMaxEnable ),
    EV_COMMAND_ENABLE           ( IDC_TIMEMIN,              CmSamplesNFilesEnable ),
    EV_COMMAND_ENABLE           ( IDC_TOENDOFFILE,          CmSamplesNFilesEnable ),
    EV_COMMAND_ENABLE           ( IDC_PRESETSTIME,          CmSamplesNFilesEnable ),
    EV_COMMAND_ENABLE           ( IDC_SORTLISTS,            CmSamplesNFilesEnable ),
    EV_COMMAND_ENABLE           ( IDC_WRITEPARAMS,          CmSamplesCsvFileEnable ),
    EV_COMMAND_ENABLE           ( IDOK,                     CmOkEnable ),
//  EV_COMMAND_ENABLE           ( IDC_TONEXTDIALOG,         CmToNextDialogEnable ),

END_RESPONSE_TABLE;


        TStatisticsFilesDialog::TStatisticsFilesDialog ( TWindow* parent, int resId, owlwparam w )
      : TStatisticsDialog ( parent, resId )
{
StringCopy ( BatchFilesExt, AllTracksFilesExt " " AllDataExt );


SamplesNFiles       = new TRadioButton ( this, IDC_SAMPLESNFILES );
Samples1File        = new TRadioButton ( this, IDC_SAMPLES1FILE );
SamplesCsvFile      = new TRadioButton ( this, IDC_SAMPLESCSVFILE );

TimeMin             = new TEdit ( this, IDC_TIMEMIN, EditSizeValue );
TimeMax             = new TEdit ( this, IDC_TIMEMAX, EditSizeValue );
EndOfFile           = new TCheckBox ( this, IDC_TOENDOFFILE );

PresetsTime         = new TComboBox ( this, IDC_PRESETSTIME );

NumGroups           = new TEdit ( this, IDC_NUMGROUPS, EditSizeValue );
GroupsSummary       = new TListBox ( this, IDC_GROUPSSUMMARY );

BaseFileName        = new TEdit ( this, IDC_INFIXFILENAME, EditSizeText );
FileTypes           = new TComboBox ( this, IDC_FILETYPES );
OutputP             = new TRadioButton ( this, IDC_OUTPUTP          );
Output1MinusP       = new TRadioButton ( this, IDC_OUTPUT1MINUSP    );
OutputMinusLogP     = new TRadioButton ( this, IDC_OUTPUTMINUSLOGP  );

OpenAuto            = new TCheckBox ( this, IDC_OPENAUTO );
MoreOutputs         = new TCheckBox ( this, IDC_MOREOUTPUTS );


SetTransferBuffer ( dynamic_cast <TStatisticsFilesStruct*> ( &StatTransfer ) );


if ( w ) {
                                        // saving caller file type
    StatTransfer.FileType    = w;
                                        // using file type for actual file extensions
    StatTransfer.FileTypes.Select ( w == CM_STATISTICSTRACKS ?  3
                                  : w == CM_STATISTICSRIS    ?  6
                                  : w == CM_STATISTICSFREQ   ?  3 
                                  :                             1 );
    }
}


        TStatisticsFilesDialog::~TStatisticsFilesDialog ()
{
if ( ! TimeMin )
    return;

delete  SamplesNFiles;          delete  Samples1File;           delete  SamplesCsvFile;
delete  TimeMin;                delete  TimeMax;                delete  EndOfFile;
delete  PresetsTime;
delete  NumGroups;              delete  GroupsSummary;
delete  BaseFileName;           delete  FileTypes;
delete  OutputP;                delete  Output1MinusP;          delete  OutputMinusLogP;
delete  OpenAuto;               delete  MoreOutputs;

TimeMin = 0;
}


//----------------------------------------------------------------------------
/*void    TStatisticsFilesDialog::CmToNextDialogEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

                                        // directly playing with the buffer does not seem to be a good idea, maybe its updated during the test?
TFileName           buff;
StringCopy ( buff, StatTransfer.BaseFileName );


if ( ! IsAbsoluteFilename ( buff ) ) {
    tce.Enable ( false );
    return;
    }


tce.Enable ( (int) GoGoF >= 2 );
}
*/

//----------------------------------------------------------------------------
void    TStatisticsFilesDialog::CmBrowseBaseFileName ()
{
static GetFileFromUser  getfile ( "Base File Name", AllFilesFilter, 1, GetFilePath );


if ( ! getfile.Execute ( StatTransfer.BaseFileName ) )
    return;


TransferData ( tdSetData );

BaseFileName->ResetCaret;
}


void    TStatisticsFilesDialog::CmSamplesParams ( owlwparam )
{
TransferData ( tdGetData );

SamplesParams ();

GoGoF.Reset ();
StringCopy ( StatTransfer.NumGroups, "0" );
StatTransfer.GroupsSummary.Clear ();

TransferData ( tdSetData );

//DBGV ( w, "" );
}


//----------------------------------------------------------------------------
void    TStatisticsFilesDialog::CmAddGroupEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

int                 tfmin           = StringToInteger ( StatTransfer.TimeMin );
int                 tfmax           = StringToInteger ( StatTransfer.TimeMax );
bool                tflast          = BoolToCheck     ( EndOfFile->GetCheck() );


if ( tfmin < 0 || ( ! tflast && ( tfmax < 0 || tfmax < tfmin ) ) ) {
    tce.Enable ( false );
    return;
    }

tce.Enable ( true );
}


void    TStatisticsFilesDialog::CmLastGroupEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

if ( GoGoF.IsEmpty () ) {
    tce.Enable ( false );
    return;
    }

if ( StatTransfer.IsCsvFile () ) {
    tce.Enable ( false );
    return;
    }

CmAddGroupEnable ( tce );
}


void    TStatisticsFilesDialog::CmTimeMaxEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( StatTransfer.IsNFiles ()  && ! CheckToBool ( StatTransfer.EndOfFile ) );
}


void    TStatisticsFilesDialog::CmSamplesNFilesEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( StatTransfer.IsNFiles () );
}


void    TStatisticsFilesDialog::CmSamplesCsvFileEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( ! StatTransfer.IsCsvFile () );
}


//----------------------------------------------------------------------------
void    TStatisticsFilesDialog::CmAddGroup ( owlwparam w )
{
TransferData ( tdGetData );


static GetFileFromUser  getfiles ( "", AllErpEegFreqRisFilesFilter, 1, GetFileMulti );
TStatGoF               *gof;


if ( w == IDC_ADDGROUP || GoGoF.IsEmpty () ) {

                                            // get eeg file names
    if ( ! getfiles.Execute () )
        return;


    if      ( StatTransfer.IsNFiles () && (int) getfiles < 2 ) {
        ShowMessage ( "Give at least 2 files to make a valid group!", "Adding Group of Files", ShowMessageWarning );
        return;
        }
    else if ( StatTransfer.Is1File () && (int) getfiles > 1 ) {
        ShowMessage ( "Give only 1 file with all the samples in it!", "Adding Group of Files", ShowMessageWarning );
        return;
        }
//    else if ( StatTransfer.IsCsvFile () && (int) getfiles > 1 ) { // doesn't care


                                        // redirect the file
    if ( StatTransfer.IsCsvFile () ) {
        for ( int i = 0; i < (int) getfiles; i++ )
            if ( IsExtensionAmong ( getfiles[ i ], SpreadSheetFilesExt ) )
                ReadSpreadSheet ( getfiles[ i ] );

        return;
        }

    gof         = new TStatGoF;

    gof->Set ( (const TGoF &) getfiles );
    }
else { // IDC_USELASTGROUP

    if ( StatTransfer.IsCsvFile () )
        return;

    gof         = new TStatGoF;

    gof->Set ( GoGoF.GetLast () );
    }

                                        // update slot
gof->TimeMin        = StringToInteger   ( StatTransfer.TimeMin      );
gof->TimeMax        = StringToInteger   ( StatTransfer.TimeMax      );
gof->EndOfFile      = CheckToBool       ( StatTransfer.EndOfFile    );
gof->StatTime       = (StatTimeType) StatTransfer.PresetsTime.GetSelIndex ();


if ( CheckGroups ( *gof ) ) {
    GoGoF.Add ( gof );
    AddGroupSummary ( GoGoF.NumGroups () - 1 );
    }
else
    delete  gof;
}


bool    TStatisticsFilesDialog::CheckGroups ( const TGoF& gof ) const
{
int                 ng              = GoGoF.NumGroups ();
const char*         toc;
bool                checkprevious   = ng >= 2 && ! ( ng & 1 );

/*                                        // same # of files?
if ( StatTransfer.HasPaired () ) {

    for ( i = 0; i < ng; i++ )
        if ( GoGoF[ i ].NumFiles != numinfiles ) // NumSamples
            break;

    if ( ng && i != ng ) {
        ShowMessage ( "Not the same number of files with previous groups!", "Group of Files", ShowMessageWarning );
        return false;
        };
    }
*/
/*
int                 numinfiles      = gof.NumFiles;
                                        // same # of files?
                                        // test only with previous paired group
if ( StatTransfer.HasPaired () && checkprevious
  && GoGoF[ ng - 2 ].NumFiles != numinfiles ) {
    ShowMessage ( "Not the same number of files with previous group!", "Group of Files", ShowMessageWarning );
    return false;
    }
*/
                                        // same # of electrodes
                                        // within the new list of files?
int                 numel;
int                 numelref = 0;
TListIterator<char> iterator;

foreachin ( gof, iterator ) {

    toc = iterator ();

    if ( ! ReadFromHeader ( toc, ReadNumElectrodes, &numel ) ) {
        ShowMessage ( "Can not read the number of electrodes!", toc, ShowMessageWarning );
        return false;
        }

    if ( numelref == 0 )
        numelref = numel;
    else if ( numel != numelref ) {
        ShowMessage ( "Not the same number of electrodes across files!", toc, ShowMessageWarning );
        return false;
        }
    }
                                        // same # of electrodes
                                        // with all previous groups?
/*
for ( i = 0; i < ng; i++ ) {
    toc = GoGoF[ i ].FileNames[ 0 ];

    ReadFromHeader ( toc, ReadNumElectrodes, &numel );

    if ( numel != numelref ) {
        ShowMessage ( "Not the same number of electrodes across groups!", toc, ShowMessageWarning );
        return false;
        }
    }
*/

if ( checkprevious ) {

    toc = GoGoF[ ng - 2 ].GetFirst ();

    ReadFromHeader ( toc, ReadNumElectrodes, &numel );

    if ( numel != numelref ) {
        ShowMessage ( "Not the same number of electrodes with previous group!", toc, ShowMessageWarning );
        return false;
        }
    }

// needs to test all file extensions?

return true;
}


void    TStatisticsFilesDialog::AddGroupSummary ( int gofi )
{
if ( gofi < 0 || gofi >= (int) GoGoF )
    return;


const TStatGoF&     gof             = GoGoF[ gofi ];
char                text [ 256 ];
TFileName           filename;
char                group[ 256 ];

                                        // update dialog
StringCopy  ( filename, ToFileName ( gof.GetFirst () ) );


if ( gof.GroupIndex >= 0 ) {
    if ( StringIsEmpty ( gof.GroupName ) )      StringCopy  ( group, "Group ", IntegerToString ( gof.GroupIndex + 1 ) );
    else                                        StringCopy  ( group, gof.GroupName );
    }
else                                            ClearString ( group );


StringCopy  ( text, "Group #", IntegerToString ( gofi + 1 ), ":" );


if      ( StatTransfer.IsCsvFile () ) {

    StringAppend    ( text, " ", IntegerToString ( StatTransfer.GetNumSamples ( gof ) ), " Subject", StringPlural ( StatTransfer.GetNumSamples ( gof ) ) );
    StringAppend    ( text, " ( ", filename, " / ", group, " )" );
    }

else if ( StatTransfer.IsNFiles () ) {

    StringAppend    ( text, " Time Frames ", IntegerToString ( gof.TimeMin ) );
    StringAppend    ( text, " to ", gof.EndOfFile ? "End-of-File" : IntegerToString ( gof.TimeMax ) );
    StringAppend    ( text, ", ", StatTimeStringShort[ gof.StatTime ] ); 
    StringAppend    ( text, ", ", IntegerToString ( StatTransfer.GetNumSamples ( gof ) ), " Subject", StringPlural ( StatTransfer.GetNumSamples ( gof ) ) );
    StringAppend    ( text, " ( ", filename, " )" );
    }

else if ( StatTransfer.Is1File () ) {

    StringAppend    ( text, " ", gof.EndOfFile ? "n" : IntegerToString ( StatTransfer.GetNumSamples ( gof ) ), " Subject", StringPlural ( gof.EndOfFile || StatTransfer.GetNumSamples ( gof ) > 1 ) );
    StringAppend    ( text, " ( ", filename, " )" );
    }

else
    ClearString ( text );


GroupsSummary->InsertString ( text, 0 );

NumGroups->Clear ();
NumGroups->Insert ( IntegerToString ( GoGoF.NumGroups () ) );

GuessOutputFileExtension ();

SetBaseFilename ();
}


//----------------------------------------------------------------------------
                                        // generate a smart base name
void    TStatisticsFilesDialog::SetBaseFilename ()
{
if ( GoGoF.IsEmpty () )
    return;

                                        // compose with only up to the first 2 groups
int                 ng              = min ( StatTransfer.IsCsvFile () ? 1 : 2, GoGoF.NumGroups () );
const TGoF*         gof;
TFileName           bigmatch;
//TFileName           buff;
TFileName           match;


for ( int g = 0; g < ng; g++ ) {

    gof     = &GoGoF[ g ];


    gof->GetCommonString ( match, ! g );


    if ( g ) {
        if ( ! StringContains ( bigmatch, match ) )
            StringAppend ( bigmatch, " ", match );
        }
    else
        StringCopy ( bigmatch, match );
    }


if ( ! IsAbsoluteFilename ( bigmatch ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // add any roi name?
if ( StatTransfer.IsRois () && *StatTransfer.RoisDocFile ) {

    TFileName       roiname;
    TFileName       match;

    StringCopy ( roiname, StatTransfer.RoisDocFile );
    GetFilename ( roiname );

    StringShrink ( roiname, match, min ( (int) ( 4 + StringLength ( roiname ) / 2 ), 17 ) );

    if ( ! StringContains ( bigmatch, match ) )
        StringAppend ( bigmatch, " ", match );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // finally, compose the base name
PrefixFilename ( bigmatch, "Stat " );

                                        // stats from segmentation have two extensions in a row
if ( StatTransfer.IsCsvFile () && IsExtension ( bigmatch, FILEEXT_SEG ) )

    RemoveExtension ( bigmatch );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // See if we better move one directory up
if ( GoGoF.NumGroups () >= 2 )
                                        // move by pairs of TGoF
    for ( int gofi = 0; gofi < TruncateTo ( GoGoF.NumGroups (), 2 ); gofi+=2 ) {

        TFileName               dir1;
        TFileName               dir2;
                                        // grab first file of each group
        StringCopy      ( dir1, GoGoF[ gofi     ][ 0 ] );
        StringCopy      ( dir2, GoGoF[ gofi + 1 ][ 0 ] );
                                        // get the last directory part only
        RemoveFilename  ( dir1, false );
        RemoveFilename  ( dir2, false );
        RemoveDir       ( dir1 );
        RemoveDir       ( dir2 );

                                        // files come from different sub-directories?
        if ( dir1 != dir2 /* || StatTransfer.IsCsvFile () */ ) {

            RemoveLastDir   ( bigmatch );
                                        // one group is enough
            break;
            }
        } // for GoGoF


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//bigmatch.Show ( "bigmatch" );


BaseFileName->SetText ( bigmatch );

BaseFileName->ResetCaret;
}


void    TStatisticsFilesDialog::CmUpOneDirectory ()
{
RemoveLastDir ( StatTransfer.BaseFileName );

BaseFileName->SetText ( StatTransfer.BaseFileName );

BaseFileName->ResetCaret;
}


//----------------------------------------------------------------------------
void    TStatisticsFilesDialog::CmRemoveGroup ()
{
if ( GoGoF.IsEmpty () )
    return;


if ( StatTransfer.IsCsvFile () ) {

    const char*     toref       = GoGoF.GetLast().GetFirst ();
    int             count       = GoGoF.GetLast().GroupIndex;

    for ( ; GoGoF.NumGroups () > 0 && count >= 0; count-- )

        if ( StringIs ( GoGoF.GetLast().GetFirst (), toref ) ) {
            GoGoF.RemoveLastGroup ();
            GroupsSummary->DeleteString ( 0 );
            }
        else
            break;
    }
else {
    GoGoF.RemoveLastGroup ();
    GroupsSummary->DeleteString ( 0 );
    }


TransferData ( tdGetData );
IntegerToString ( StatTransfer.NumGroups, GoGoF.NumGroups () );
TransferData ( tdSetData );

SetBaseFilename ();

//SetXyzFileIsOk ( true );
}


void    TStatisticsFilesDialog::CmClearGroups ()
{
GoGoF.Reset ();

GroupsSummary->ClearList ();

TransferData ( tdGetData );
IntegerToString ( StatTransfer.NumGroups, GoGoF.NumGroups () );
StringCopy ( StatTransfer.Channels, "*" );
TransferData ( tdSetData );

SetBaseFilename ();
}


void    TStatisticsFilesDialog::CmSortGroups ()
{
if ( StatTransfer.IsCsvFile () )
    return;


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
void    TStatisticsFilesDialog::EvDropFiles ( TDropInfo drop )
{
TGoF                tracksfiles     ( drop, BatchFilesExt       );
TGoF                lmfiles         ( drop, FILEEXT_LM          );
TGoF                spreadsheetfiles( drop, SpreadSheetFilesExt );
TGoF                roifiles        ( drop, AllRoisFilesExt     );

char                buff[ 256 ];
StringCopy ( buff, BatchFilesExt, " " FILEEXT_LM " " SpreadSheetFilesExt " " AllRoisFilesExt );
TGoF                remainingfiles  ( drop, buff, 0, true );

drop.DragFinish ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) spreadsheetfiles; i++ )
    ReadSpreadSheet ( spreadsheetfiles[ i ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // correct some kind of bug, when Dialog get focus back, it "presses" the button
#define     StatDialogPreventButtonPress    { CmSamplesParams ( 0 ); SamplesCsvFile->SetFocus (); }


if ( (bool) tracksfiles ) {
                                        // the drop shifts the file type to 1 file, if files have already been dropped
    if ( ! (bool) GoGoF && (int) tracksfiles == 1 && ! StatTransfer.Is1File () ) {
        StatTransfer.SamplesNFiles  = BoolToCheck ( false );
        StatTransfer.Samples1File   = BoolToCheck ( true  );
        StatTransfer.SamplesCsvFile = BoolToCheck ( false );

        TransferData ( tdSetData );

        StatDialogPreventButtonPress;
        }


    bool                first;

    first   = true;

    for ( int i = 0; i < (int) tracksfiles; i++ ) {

        AddFileToGroup ( tracksfiles[ i ], first );


        if ( StatTransfer.Is1File () ) {    // each file is a group

            if ( CheckGroups ( GoGoF.GetLast () ) )
                AddGroupSummary ( GoGoF.NumGroups () - 1 );
            else
                GoGoF.RemoveLastGroup ();
            }
        else
            first = false;
        }


    if ( StatTransfer.IsNFiles () )
        if ( (int) tracksfiles <= 1 ) {
            GoGoF.RemoveLastGroup ();
            ShowMessage ( "Give at least 2 files!", "Statistics - Files", ShowMessageWarning );
            }
        else
            AddGroupSummary ( GoGoF.NumGroups () - 1 );
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

                    if ( StatTransfer.Is1File () ) {    // each file is a group
                        if ( CheckGroups ( GoGoF.GetLast () ) )
                            AddGroupSummary ( GoGoF.NumGroups () - 1 );
                        else
                            GoGoF.RemoveLastGroup ();
                        }
                    else
                        first = false;
                    }


            if ( StatTransfer.IsNFiles () && ! first ) { // security check, in case of only frequency files
                if ( CheckGroups ( GoGoF.GetLast () ) )
                    AddGroupSummary ( GoGoF.NumGroups () - 1 );
                else
                    GoGoF.RemoveLastGroup ();
                }
            }

                                        // then process separately the ris
        first = true;

        if ( (bool) lm.lris ) {

            foreachin ( lm.lris, iterator ) {

                AddFileToGroup ( iterator (), first );

                if ( StatTransfer.Is1File () ) {    // each file is a group
                    if ( CheckGroups ( GoGoF.GetLast () ) )
                        AddGroupSummary ( GoGoF.NumGroups () - 1 );
                    else
                        GoGoF.RemoveLastGroup ();
                    }
                else
                    first = false;
                }


            if ( StatTransfer.IsNFiles () ) {
                if ( CheckGroups ( GoGoF.GetLast () ) )
                    AddGroupSummary ( GoGoF.NumGroups () - 1 );
                else
                    GoGoF.RemoveLastGroup ();
                }
            }
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) roifiles ) {

    if ( ! CheckToBool ( StatTransfer.ExportRois ) ) {
        StatTransfer.ExportTracks   = BoolToCheck ( false );
        StatTransfer.ExportRois     = BoolToCheck ( true  );
        }

    for ( int i = 0; i < (int) roifiles; i++ )
        StringCopy ( StatTransfer.RoisDocFile, roifiles[ i ] );

                                        // do any switch at drop time
    GuessOutputFileExtension ();

    SetBaseFilename ();
    } // if numroi


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Let's be nice and try to load D&D Directories
                                        // For the moment, retrieving only the .ep files
for ( int i = 0; i < (int) remainingfiles; i++ ) {

    if ( ! IsDirectory ( remainingfiles[ i ] ) )
        continue;


    TGoF                    subgof;

    subgof.GrepFiles    ( remainingfiles[ i ], AllTracksFilesGrep, GrepOptionDefaultFiles, true );

//  subgof.Show ( "found eeg" );

                                    // process if found some files, otherwise, complain
    bool                first       = true;

    if ( (bool) subgof ) {

        for ( int sgi = 0; sgi < (int) subgof; sgi++ ) {

            AddFileToGroup ( subgof[ sgi ], first );


            if ( StatTransfer.Is1File () ) {    // each file is a group
                if ( CheckGroups    ( GoGoF.GetLast () ) )
                    AddGroupSummary ( GoGoF.NumGroups () - 1 );
                else
                    GoGoF.RemoveLastGroup ();
                }
            else
                first = false;
            }


        if ( StatTransfer.IsNFiles () )
            if ( (int) subgof <= 1 ) {
                GoGoF.RemoveLastGroup ();
                ShowMessage ( "Give at least 2 files!", "Statistics - Files", ShowMessageWarning );
                }
            else
                AddGroupSummary ( GoGoF.NumGroups () - 1 );


        remainingfiles.RemoveRef ( remainingfiles[ i ] );
        i--;
        }
    }


if ( (bool) remainingfiles )
    remainingfiles.Show ( "Skipping non-relevant file:" );
}


//----------------------------------------------------------------------------
void    TStatisticsFilesDialog::AddFileToGroup ( const char* filename, bool first )
{
if ( first ) {                          // add a new group
    TStatGoF    *gof    = new TStatGoF;

    GoGoF.Add ( gof );

    gof->TimeMin        = StringToInteger   ( StatTransfer.TimeMin      );
    gof->TimeMax        = StringToInteger   ( StatTransfer.TimeMax      );
    gof->EndOfFile      = CheckToBool       ( StatTransfer.EndOfFile    );
    gof->StatTime       = (StatTimeType) StatTransfer.PresetsTime.GetSelIndex ();
    }


GoGoF.GetLast ().Add ( filename, MaxPathShort );
}


void    TStatisticsFilesDialog::GuessOutputFileExtension ()
{
if ( GoGoF.IsEmpty () )
    return;

bool                allris  = GoGoF.AllExtensionsAre ( FILEEXT_RIS    );
bool                alleph  = GoGoF.AllExtensionsAre ( FILEEXT_EEGEPH );
bool                allep   = GoGoF.AllExtensionsAre ( FILEEXT_EEGEP  );
bool                allbv   = GoGoF.AllExtensionsAre ( FILEEXT_EEGBV  );
bool                allsef  = GoGoF.AllExtensionsAre ( FILEEXT_EEGSEF );
bool                allfreq = GoGoF.AllExtensionsAre ( FILEEXT_FREQ   );

//DBGV4 ( allep, alleph, allris, allfreq, "allep, alleph, allris, allfreq" );


TransferData ( tdGetData );

if      ( StatTransfer.IsRois () )  StatTransfer.FileTypes.Select ( PresetFileTypeDefaultEEG );
else if ( allris  )                 StatTransfer.FileTypes.Select ( PresetFileTypeRis );
else if ( alleph  )                 StatTransfer.FileTypes.Select ( PresetFileTypeEph );
else if ( allep   )                 StatTransfer.FileTypes.Select ( PresetFileTypeEp  );
else if ( allbv   )                 StatTransfer.FileTypes.Select ( PresetFileTypeBV  );
else if ( allsef  )                 StatTransfer.FileTypes.Select ( PresetFileTypeSef );
else if ( allfreq )                 StatTransfer.FileTypes.Select ( PresetFileTypeSef );

TransferData ( tdSetData );
}


void    TStatisticsFilesDialog::CmReadSpreadSheet ()
{
ReadSpreadSheet ();
}


//----------------------------------------------------------------------------
void    TStatisticsFilesDialog::ReadSpreadSheet ( char *filename )
{
TFileName           realfilename;

if ( StringIsNotEmpty ( filename ) )     // could be null, which we don't really like...
    StringCopy ( realfilename, filename );
else
    ClearString ( realfilename );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSpreadSheet        sf;

if ( ! sf.ReadFile ( realfilename ) )
    return;


CsvType             csvtype         = sf.GetType ();


if ( IsCsvUnknownType ( csvtype ) /*|| IsCsvStatFittingR ( csvtype )*/ )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           attr;
TFileName           buff;
TStatGoF           *gof;


if      ( IsCsvStatFittingCartool ( csvtype ) 
       || IsCsvStatMarkov         ( csvtype ) ) {
                                        // update the file type on the dialog
    if ( ! StatTransfer.IsCsvFile () ) {
        StatTransfer.SamplesNFiles  = BoolToCheck ( false );
        StatTransfer.Samples1File   = BoolToCheck ( false );
        StatTransfer.SamplesCsvFile = BoolToCheck ( true  );

        TransferData ( tdSetData );

        StatDialogPreventButtonPress;
        }


    int                 csvnumgroups;
    TArray1<int>        csvgroupindex;
    int                 csvnumvarspergroup;
    TStrings            csvvarnames;
    int                 csvmaxmap;
    TSelection          csvmapsel;
    TStrings            csvmapnames;
    int                 csvnumfitvars;
    TStrings            csvfitvarnames;


    sf.GetFittingInfo ( csvnumgroups,       csvgroupindex,  0, 
                        csvnumvarspergroup, csvvarnames, 
                        csvmaxmap,          csvmapsel,      csvmapnames, 
                        csvnumfitvars,      csvfitvarnames );
    

    if ( csvnumgroups == 0 ) {
        ShowMessage ( "Can not retrieve the number of groups!", realfilename, ShowMessageWarning );
        return;
        }

                                        // now feed with the groups, though coming from the same file
    for ( int i = 0; i < csvnumgroups; i++ ) {

        gof                 = new TStatGoF;
        GoGoF.Add ( gof );

        gof->NumSamples     = sf.GetNumRecords ();
        gof->GroupIndex     = i;        // this index is local to current file
                                                                                              // here we can show a global index
        StringCopy  ( gof->GroupName, IsLongNames ( csvtype ) ? FitGroupNameLong : FitGroupNameShort, IntegerToString ( GoGoF.NumGroups () /*csvgroupindex ( i )*/ ) );

        gof->TimeMin        = 0;
        gof->TimeMax        = 0;
        gof->EndOfFile      = false;
        gof->StatTime       = StatTimeDefault;

                                            // transfer the filenames...
        gof->Add ( realfilename, MaxPathShort );

                                            // now check for completely missing values at the bottom
                                            // in this case, decrement the number of samples

        bool            emptyline;

        for ( int s = gof->NumSamples - 1; s >= 0; s-- ) {

            emptyline   = true;

            for ( int e = 0; e < csvnumvarspergroup; e++ ) {

                sf.GetRecord ( s, 1 + ( gof->GroupIndex * csvnumvarspergroup ) + e, attr );

                if ( StringToDouble ( attr ) != -1 ) {
                    emptyline = false;
                    break;
                    }
                }

            if ( emptyline )
                gof->NumSamples--;
            }


        AddGroupSummary ( GoGoF.NumGroups () - 1 );
        } // for group

    } // if IsCsvStatFittingCartool || IsCsvStatMarkov


else if ( IsCsvStatFiles ( csvtype )  ) {

    sf.GetRecord ( 0, "numfiles",    attr );

                                        // try to smartly shifts the file type according to the drop
    if ( StatTransfer.IsCsvFile ()
      || StatTransfer.IsNFiles () && StringIs    ( attr, "1" )
      || StatTransfer.Is1File ()  && StringIsNot ( attr, "1" ) ) {


        if ( StringIs ( attr, "1" ) ) {
            StatTransfer.SamplesNFiles  = BoolToCheck ( false );
            StatTransfer.Samples1File   = BoolToCheck ( true  );
            }
        else {
            StatTransfer.Samples1File   = BoolToCheck ( false );
            StatTransfer.SamplesNFiles  = BoolToCheck ( true  );
            }

        StatTransfer.SamplesCsvFile = BoolToCheck ( false );

        TransferData ( tdSetData );

        StatDialogPreventButtonPress;
        }


    for ( int file = 0; file < sf.GetNumRecords (); file ++ ) {

        gof                 = new TStatGoF;
        GoGoF.Add ( gof );


        sf.GetRecord ( file, "numfiles",    attr );
        int     numfiles    = StringToInteger ( attr );

        sf.GetRecord ( file, "timemin",    attr );
        gof->TimeMin        = StringToInteger ( attr );

        sf.GetRecord ( file, "timemax",    attr );
        if ( StringIs ( attr, "Eof" ) ) {
            gof->TimeMax        = 0;
            gof->EndOfFile      = true;
            }
        else {
            gof->TimeMax        = StringToInteger ( attr );
            gof->EndOfFile      = false;
            }


        sf.GetRecord ( file, "timemode",    attr );

                                        // legacy strings (pre-2016)
        if ( StringIs ( attr, "Average" ) )
            gof->StatTime       = StatTimeMean;

        else if ( StringIs ( attr, "Sequential" ) )
            gof->StatTime       = StatTimeSequential;

        else                            // new legal strings (post 2016)
            for ( int i = 0; i < NumStatTimes; i++ )
                if ( StringIs ( attr, StatTimeStringShort[ i ] ) ) {
                    gof->StatTime       = (StatTimeType) i;
                    break;
                    }

                                            // transfer the filenames...
        for ( int i = 0; i < numfiles; i++ ) {
            sprintf ( buff, "file%0d", i + 1 );
            sf.GetRecord ( file, buff, attr );

            gof->Add ( attr, MaxPathShort );
            } // for file


        AddGroupSummary ( GoGoF.NumGroups () - 1 );
        } // for record

    } // if IsCsvStatFiles


else { // all other cases, like IsCsvStatFittingR
    ShowMessage ( SpreadSheetErrorMessage, realfilename, ShowMessageWarning );
    return;
    }
}


void    TStatisticsFilesDialog::CmWriteParams ()
{
if ( GoGoF.NumGroups () < 2 )
    return;


int                 i;
TStatGoF           *gof;
TSpreadSheet        sf;


if ( ! sf.WriteFile () )
    return;

                                        // header line describes the attributes / fields
sf.WriteAttribute ( "numfiles" );
sf.WriteAttribute ( "timemin" );
sf.WriteAttribute ( "timemax" );
sf.WriteAttribute ( "timemode" );

                                        // we need to give the largest amount of files
int             minnumfiles = 0;

TListIterator<TGoF>     gofiterator;

foreachin ( GoGoF, gofiterator )
    minnumfiles = max ( minnumfiles, gofiterator ()->NumFiles () );


for ( i = 0; i < minnumfiles; i++ )
    sf.WriteAttribute ( "file", i + 1 );

sf.WriteNextRecord ();

                                        // now write each line
foreachin ( GoGoF, gofiterator ) {

    gof                 = dynamic_cast< TStatGoF * > ( gofiterator () );
                                        // write parameters
    sf.WriteAttribute ( gof->NumFiles () );
    sf.WriteAttribute ( (int) gof->TimeMin );

    if ( gof->EndOfFile )
        sf.WriteAttribute ( "Eof" );
    else
        sf.WriteAttribute ( (int) gof->TimeMax );

    sf.WriteAttribute ( StatTimeStringShort[ gof->StatTime ] );

                                        // write all files
    TListIterator<char>     iterator;

    foreachin ( *gof, iterator )
        sf.WriteAttribute ( iterator () );

                                        // complete line with empty attributes
    for ( i = gof->NumFiles (); i < minnumfiles; i++ )
        sf.WriteAttribute ( "" );

    sf.WriteNextRecord ();
    }


sf.WriteFinished ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TStatisticsParamsDialog, TStatisticsDialog )

    EV_WM_DROPFILES,

    EV_COMMAND                  ( IDC_BROWSEXYZDOC,             CmBrowseXyzDoc ),
    EV_COMMAND_ENABLE           ( IDC_CHANNELS,                 CmTracksEnable ),
    EV_COMMAND_ENABLE           ( IDC_USEXYZDOC,                CmChannelsEnable ),
    EV_COMMAND_ENABLE           ( IDC_XYZDOCFILE,               CmXyzDocFileEnable ),
    EV_COMMAND_ENABLE           ( IDC_BROWSEXYZDOC,             CmXyzDocFileEnable ),

    EV_COMMAND                  ( IDC_ROIS,                     CmRoiChange ),
    EV_COMMAND                  ( IDC_BROWSEROISFILE,           CmBrowseRoiDoc ),
    EV_COMMAND_ENABLE           ( IDC_ROIS,                     CmCsvDisable ),
    EV_COMMAND_ENABLE           ( IDC_ROISFILE,                 CmRoisFileEnable ),
    EV_COMMAND_ENABLE           ( IDC_BROWSEROISFILE,           CmRoisFileEnable ),

    EV_COMMAND                  ( IDC_UNPAIREDTEST,             CmCheckTestParams ),
    EV_COMMAND                  ( IDC_PAIREDTEST,               CmCheckTestParams ),
    EV_COMMAND                  ( IDC_TTEST,                    CmCheckTestParams ),
    EV_COMMAND                  ( IDC_RANDOMIZATION,            CmCheckTestParams ),
    EV_COMMAND                  ( IDC_TANOVA,                   CmCheckTestParams ),

    EV_CBN_SELCHANGE            ( IDC_PRESETS,                  EvPresetsChange ),

    EV_COMMAND                  ( IDC_AVEREF,                   CmCheckTestParams ),
    EV_COMMAND                  ( IDC_POSITIVEDATA,             CmCheckTestParams ),
//    EV_COMMAND_ENABLE           ( IDC_ACCOUNTPOLARITY,          CmPolarityEnable ),
//    EV_COMMAND_ENABLE           ( IDC_IGNOREPOLARITY,           CmPolarityEnable ),

    EV_COMMAND_ENABLE           ( IDC_TANOVA,                   CmChannelsEnable ),
    EV_COMMAND_ENABLE           ( IDC_CHECKMISSINGVALUES,       CmCheckMissingValuesEnable ),
    EV_COMMAND_ENABLE           ( IDC_RESCALE_GFP,              CmSamplesNFilesEnable ),
    EV_COMMAND_ENABLE           ( IDC_RESCALE_GFP_PAIRED,       CmSamplesNFilesEnable ),
    EV_COMMAND_ENABLE           ( IDC_RESCALE_GFP_1TF,          CmChannelsEnable ),

    EV_COMMAND_ENABLE           ( IDC_CHECKMISSINGVALUESVALUE,  CmMissingValuesEnable ),
    EV_COMMAND_ENABLE           ( IDC_NUMBEROFRANDOMIZATIONS,      CmRandomizationEnable ),

    EV_COMMAND_ENABLE           ( IDC_FDRCORRECTIONVALUE,       CmFDRCorrectionThresholdingEnable ),
    EV_COMMAND_ENABLE           ( IDC_THRESHOLDINGPVALUES,      CmThresholdingPValuesEnable ),
    EV_COMMAND_ENABLE           ( IDC_THRESHOLDINGPVALUESVALUE, CmThresholdingPValuesValueEnable ),
    EV_COMMAND_ENABLE           ( IDC_MINSIGNIFICANTDURATION,   CmMinSignificantDurationEnable ),
    EV_COMMAND_ENABLE           ( IDC_MINSIGNIFICANTDURATIONVALUE,   CmMinSignificantDurationValueEnable ),

//  EV_COMMAND_ENABLE           ( IDC_PAIREDTTEST,              CmPairedEnable ),
//  EV_COMMAND_ENABLE           ( IDC_MEANDIFFERENCES,          CmPairedEnable ),
//  EV_COMMAND_ENABLE           ( IDC_PAIREDTANOVA,             CmPairedEnable ),

    EV_COMMAND_ENABLE           ( IDOK,                         CmOkEnable ),

END_RESPONSE_TABLE;


        TStatisticsParamsDialog::TStatisticsParamsDialog (  TWindow* parent, int resId )
      : TStatisticsDialog ( parent, resId )
{
ClearString ( BatchFilesExt );          // not used here


ExportTracks            = new TRadioButton ( this, IDC_TRACKS );
Channels                = new TEdit ( this, IDC_CHANNELS, EditSizeTextLong );
UseXyzDoc               = new TCheckBox ( this, IDC_USEXYZDOC );
XyzDocFile              = new TEdit ( this, IDC_XYZDOCFILE, EditSizeText );

ExportRois              = new TRadioButton ( this, IDC_ROIS );
RoisDocFile             = new TEdit ( this, IDC_ROISFILE, EditSizeText );

UnpairedTest            = new TRadioButton ( this, IDC_UNPAIREDTEST );
PairedTest              = new TRadioButton ( this, IDC_PAIREDTEST );

TTest                   = new TCheckBox ( this, IDC_TTEST );
Randomization           = new TCheckBox ( this, IDC_RANDOMIZATION );
TAnova                  = new TCheckBox ( this, IDC_TANOVA );

PresetsData             = new TComboBox ( this, IDC_PRESETS );

SignedData              = new TRadioButton ( this, IDC_SIGNEDDATA );
PositiveData            = new TRadioButton ( this, IDC_POSITIVEDATA );

AccountPolarity         = new TRadioButton ( this, IDC_ACCOUNTPOLARITY );
IgnorePolarity          = new TRadioButton ( this, IDC_IGNOREPOLARITY );

NoRef                   = new TRadioButton ( this, IDC_NOREF );
AveRef                  = new TRadioButton ( this, IDC_AVEREF );

NormalizationNone       = new TRadioButton ( this, IDC_RESCALE_NONE );
NormalizationGfp        = new TRadioButton ( this, IDC_RESCALE_GFP );
NormalizationGfpPaired  = new TRadioButton ( this, IDC_RESCALE_GFP_PAIRED );
NormalizationGfp1TF     = new TRadioButton ( this, IDC_RESCALE_GFP_1TF );

CheckMissingValues      = new TCheckBox ( this, IDC_CHECKMISSINGVALUES );
CheckMissingValuesValue = new TEdit ( this, IDC_CHECKMISSINGVALUESVALUE, EditSizeValue );
NumberOfRandomization   = new TEdit ( this, IDC_NUMBEROFRANDOMIZATIONS, EditSizeValue );

PresetsCorrection       = new TComboBox ( this, IDC_PRESETSCORRECTION );

FDRCorrectionValue      = new TEdit ( this, IDC_FDRCORRECTIONVALUE, EditSizeValue );

ThresholdingPValues         = new TCheckBox ( this, IDC_THRESHOLDINGPVALUES );
ThresholdingPValuesValue    = new TEdit ( this, IDC_THRESHOLDINGPVALUESVALUE, EditSizeValue );

MinSignificantDuration      = new TCheckBox ( this, IDC_MINSIGNIFICANTDURATION );
MinSignificantDurationValue = new TEdit ( this, IDC_MINSIGNIFICANTDURATIONVALUE, EditSizeValue );


SetTransferBuffer ( dynamic_cast <TStatisticsParamsStruct*> ( &StatTransfer ) );


static bool     init    = true;

if ( init )
    init    = false;
else
    SamplesParams ();                   // check / reset some variables
}


void    TStatisticsParamsDialog::SetupWindow ()
{
TStatisticsDialog::SetupWindow ();


TransferData ( tdSetData );

                                        // FileType has to be set from dialog's caller as a strong hint to the context for statistics: tracks, RIS or frequencies
if      ( StatTransfer.IsCsvFile () )                           PresetsData->SetSelIndex ( StatPresetFitting            );

else if ( StatTransfer.Is1File   () ) {

    if      ( StatTransfer.FileType == CM_STATISTICSTRACKS )    PresetsData->SetSelIndex ( StatPresetEegSurfaceSpont    );
    else if ( StatTransfer.FileType == CM_STATISTICSRIS    )    PresetsData->SetSelIndex ( StatPresetRisScalSpont       );
    else                                                        PresetsData->SetSelIndex ( StatPresetEegSurfaceSpont    );
    }
else { // if ( StatTransfer.IsNFiles  () ) {

    if      ( StatTransfer.FileType == CM_STATISTICSTRACKS )    PresetsData->SetSelIndex ( StatPresetEegSurfaceErp      );
    else if ( StatTransfer.FileType == CM_STATISTICSRIS    )    PresetsData->SetSelIndex ( StatPresetRisScalErp         );
    else if ( StatTransfer.FileType == CM_STATISTICSFREQ   )    PresetsData->SetSelIndex ( StatPresetFFTPower           );
    else                                                        PresetsData->SetSelIndex ( StatPresetEegSurfaceErp      );
    }

                                        // !process preset to set/reset the proper options!
EvPresetsChange ();

TransferData ( tdGetData );
}


        TStatisticsParamsDialog::~TStatisticsParamsDialog ()
{
if ( ! Channels )
    return;

delete  ExportTracks;
delete  Channels;               delete  UseXyzDoc;              delete  XyzDocFile;
delete  ExportRois;             delete  RoisDocFile;
delete  UnpairedTest;           delete  PairedTest;
delete  TTest;                  delete  Randomization;          delete  TAnova;
delete  PresetsData;
delete  SignedData;             delete  PositiveData;
delete  AccountPolarity;        delete  IgnorePolarity;
delete  NoRef;                  delete  AveRef;
delete  NormalizationNone;      delete  NormalizationGfp;       delete  NormalizationGfpPaired; delete  NormalizationGfp1TF;
delete  CheckMissingValues;     delete  CheckMissingValuesValue;
delete  NumberOfRandomization;
delete  PresetsCorrection;
delete  FDRCorrectionValue;
delete  ThresholdingPValues;    delete  ThresholdingPValuesValue; 
delete  MinSignificantDuration; delete  MinSignificantDurationValue;

Channels = 0;
}


//----------------------------------------------------------------------------
void    TStatisticsParamsDialog::CmBrowseXyzDoc ()
{
static GetFileFromUser  getfile ( "Electrodes Coordinates File", AllCoordinatesFilesFilter, 1, GetFileRead );


if ( ! getfile.Execute ( StatTransfer.XyzDocFile ) )
    return;


TransferData ( tdSetData );             // this will activate CmXyzChange (but not SetText ( .. ) )

XyzDocFile->ResetCaret;
}


void    TStatisticsParamsDialog::CmTracksEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( StatTransfer.IsChannels () || StatTransfer.IsCsvFile () );
}


void    TStatisticsParamsDialog::CmChannelsEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( StatTransfer.IsChannels () );
}


void    TStatisticsParamsDialog::CmXyzDocFileEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( StatTransfer.IsChannels () && CheckToBool ( StatTransfer.UseXyzDoc ) );
}


//----------------------------------------------------------------------------
void    TStatisticsParamsDialog::CmRoiChange ()
{
TransferData ( tdGetData );

SamplesParams ();

TransferData ( tdSetData );
}


void    TStatisticsParamsDialog::CmBrowseRoiDoc ()
{
static GetFileFromUser  getfile ( "ROIs File", AllRoisFilesFilter, 1, GetFileRead );


if ( ! getfile.Execute ( StatTransfer.RoisDocFile ) )
    return;


TransferData ( tdSetData );             // this will activate CmRoiChange (but not SetText ( .. ) )

RoisDocFile->ResetCaret;
}


void    TStatisticsParamsDialog::CmCsvDisable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( ! StatTransfer.IsCsvFile () );
}


void    TStatisticsParamsDialog::CmRoisFileEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( StatTransfer.IsRois () );
}


//----------------------------------------------------------------------------
void    TStatisticsParamsDialog::EvDropFiles ( TDropInfo drop )
{
TGoF                xyzfiles        ( drop, AllCoordinatesFilesExt  );
TGoF                roifiles        ( drop, AllRoisFilesExt         );
TGoF                remainingfiles  ( drop, AllCoordinatesFilesExt " " AllRoisFilesExt, 0, true );

drop.DragFinish ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) xyzfiles; i++ ) {

    UseXyzDoc ->SetCheck ( BoolToCheck ( true  ) );
    XyzDocFile->SetText  ( xyzfiles[ i ] );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) roifiles ) {

    if ( ! CheckToBool ( StatTransfer.ExportRois ) ) {
        StatTransfer.ExportTracks   = BoolToCheck ( false );
        StatTransfer.ExportRois     = BoolToCheck ( true  );
        }

    for ( int i = 0; i < (int) roifiles; i++ ) {

        StringCopy ( StatTransfer.RoisDocFile, roifiles[ i ] );
        TransferData ( tdSetData );     // this will activate any change detection function
        }
    } // if numroi


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (bool) remainingfiles )
    remainingfiles.Show ( "Skipping non-relevant file:" );
}


//----------------------------------------------------------------------------
void    TStatisticsParamsDialog::EvPresetsChange ()
{
if ( StringIsEmpty ( StatPresetsDataString[ PresetsData->GetSelIndex () ] ) )
    return;

                                        // reset all options
PositiveData    ->SetCheck ( BoolToCheck ( false ) );
SignedData      ->SetCheck ( BoolToCheck ( false ) );
//VectorData      ->SetCheck ( BoolToCheck ( false ) );
AccountPolarity ->SetCheck ( BoolToCheck ( false ) );
IgnorePolarity  ->SetCheck ( BoolToCheck ( false ) );
NoRef           ->SetCheck ( BoolToCheck ( false ) );
AveRef          ->SetCheck ( BoolToCheck ( false ) );


                                        // then set only the right ones
switch ( PresetsData->GetSelIndex() ) {

    case    StatPresetEegSurfaceErp :
        SignedData      ->SetCheck ( BoolToCheck ( true  ) );
        AccountPolarity ->SetCheck ( BoolToCheck ( true  ) );
        AveRef          ->SetCheck ( BoolToCheck ( true  ) );
        break;

    case    StatPresetEegSurfaceSpont :
        SignedData      ->SetCheck ( BoolToCheck ( true  ) );
        IgnorePolarity  ->SetCheck ( BoolToCheck ( true  ) );
        AveRef          ->SetCheck ( BoolToCheck ( true  ) );
        break;

    case    StatPresetEegIntraErp :
        SignedData      ->SetCheck ( BoolToCheck ( true  ) );
        AccountPolarity ->SetCheck ( BoolToCheck ( true  ) );
        NoRef           ->SetCheck ( BoolToCheck ( true  ) );
        break;

    case    StatPresetEegIntraSpont :
        SignedData      ->SetCheck ( BoolToCheck ( true  ) );
        IgnorePolarity  ->SetCheck ( BoolToCheck ( true  ) );
        NoRef           ->SetCheck ( BoolToCheck ( true  ) );
        break;


    case    StatPresetMegSurfaceErp :
        PositiveData    ->SetCheck ( BoolToCheck ( true  ) );
        AccountPolarity ->SetCheck ( BoolToCheck ( true  ) );
        NoRef           ->SetCheck ( BoolToCheck ( true  ) );
        break;

    case    StatPresetMegSurfaceSpont :
        PositiveData    ->SetCheck ( BoolToCheck ( true  ) );
        AccountPolarity ->SetCheck ( BoolToCheck ( true  ) );
        NoRef           ->SetCheck ( BoolToCheck ( true  ) );
        break;


//  case    StatPresetRisVectErp :
//      VectorData      ->SetCheck ( BoolToCheck ( true  ) );
//      AccountPolarity ->SetCheck ( BoolToCheck ( true  ) );
//      NoRef           ->SetCheck ( BoolToCheck ( true  ) );
//      break;

    case    StatPresetRisScalErp :
        PositiveData    ->SetCheck ( BoolToCheck ( true  ) );
        AccountPolarity ->SetCheck ( BoolToCheck ( true  ) );
        NoRef           ->SetCheck ( BoolToCheck ( true  ) );
        break;

//  case    StatPresetRisVectSpont :
//      VectorData      ->SetCheck ( BoolToCheck ( true  ) );
//      AccountPolarity ->SetCheck ( BoolToCheck ( true  ) );
////    IgnorePolarity  ->SetCheck ( BoolToCheck ( true  ) );  // this, but at individual vectors level
//      NoRef           ->SetCheck ( BoolToCheck ( true  ) );
//      break;

    case    StatPresetRisScalSpont :
        PositiveData    ->SetCheck ( BoolToCheck ( true  ) );
        AccountPolarity ->SetCheck ( BoolToCheck ( true  ) );
        NoRef           ->SetCheck ( BoolToCheck ( true  ) );
        break;


    case    StatPresetFFTPower :
        PositiveData    ->SetCheck ( BoolToCheck ( true  ) );
        AccountPolarity ->SetCheck ( BoolToCheck ( true  ) );
        NoRef           ->SetCheck ( BoolToCheck ( true  ) );
        break;

    case    StatPresetFFTApprox :
        SignedData      ->SetCheck ( BoolToCheck ( true  ) );
        IgnorePolarity  ->SetCheck ( BoolToCheck ( true  ) );
        AveRef          ->SetCheck ( BoolToCheck ( true  ) );
        break;


    case    StatPresetFitting :
        SignedData      ->SetCheck ( BoolToCheck ( true  ) );
        AccountPolarity ->SetCheck ( BoolToCheck ( true  ) );
        NoRef           ->SetCheck ( BoolToCheck ( true  ) );
        break;


    case    StatPresetMarkov :
        PositiveData    ->SetCheck ( BoolToCheck ( true  ) );
        AccountPolarity ->SetCheck ( BoolToCheck ( true  ) );
        NoRef           ->SetCheck ( BoolToCheck ( true  ) );

        NormalizationNone     ->SetCheck ( BoolToCheck ( true  ) );
        NormalizationGfp      ->SetCheck ( BoolToCheck ( false ) );
        NormalizationGfpPaired->SetCheck ( BoolToCheck ( false ) );
        NormalizationGfp1TF   ->SetCheck ( BoolToCheck ( false ) );
        break;


    case    StatPresetPos :
        PositiveData    ->SetCheck ( BoolToCheck ( true  ) );
        AccountPolarity ->SetCheck ( BoolToCheck ( true  ) );
        NoRef           ->SetCheck ( BoolToCheck ( true  ) );
        break;
    }
}


void    TStatisticsParamsDialog::CmMinSignificantDurationEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( StatTransfer.IsNFiles ()
         && (    CheckToBool ( ThresholdingPValues->GetCheck() )                                        // user thresholded the p-values
              || StatTransfer.PresetsCorrection.GetSelIndex () == StatCorrPresetThresholdingPValues )   // or this FDR correction, which also clip the p-values
           );
}


void    TStatisticsParamsDialog::CmMinSignificantDurationValueEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( StatTransfer.IsNFiles ()
         && (    CheckToBool ( ThresholdingPValues->GetCheck() )
              || StatTransfer.PresetsCorrection.GetSelIndex () == StatCorrPresetThresholdingPValues ) 
         && CheckToBool ( MinSignificantDuration->GetCheck() ) );
}

/*
void    TStatisticsParamsDialog::CmUnpairedEnable ( TCommandEnabler &tce )
{
//TransferData ( tdGetData );

//tce.Enable ( ( StatTransfer.HasUnpaired () && !StatTransfer.HasPaired () )
tce.Enable ( StatTransfer.IsNFiles () );
}
*/

void    TStatisticsParamsDialog::CmSamplesNFilesEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( StatTransfer.IsNFiles () );
}


void    TStatisticsParamsDialog::CmCheckTestParams ()
{
TransferData ( tdGetData );

/*
if ( ! StatTransfer.HasTwoSample () ) {
    NormalizationNone     ->SetCheck ( BoolToCheck ( true  ) );
    NormalizationGfp      ->SetCheck ( BoolToCheck ( false ) );
    NormalizationGfpPaired->SetCheck ( BoolToCheck ( false ) );
    NormalizationGfp1TF   ->SetCheck ( BoolToCheck ( false ) );
    }
*/
if ( StatTransfer.HasPaired () && CheckToBool ( StatTransfer.NormalizationGfp ) ) {
    NormalizationGfp      ->SetCheck ( BoolToCheck ( false ) );
    NormalizationGfpPaired->SetCheck ( BoolToCheck ( true  ) );
    }
else if ( ! StatTransfer.HasPaired () && CheckToBool ( StatTransfer.NormalizationGfpPaired ) ) {
    NormalizationGfp      ->SetCheck ( BoolToCheck ( true  ) );
    NormalizationGfpPaired->SetCheck ( BoolToCheck ( false ) );
    }
/*
if ( ! StatTransfer.HasParametric () && CheckToBool ( StatTransfer.BonferroniParametric ) ) {
    BonferroniParametric->SetCheck ( BoolToCheck ( false ) );
    }

if ( ! StatTransfer.HasNonParametric () && CheckToBool ( StatTransfer.BonferroniNonParametric ) ) {
    BonferroniNonParametric->SetCheck ( BoolToCheck ( false ) );
    }
*/
/*
if ( CheckToBool ( StatTransfer.AveRef ) && CheckToBool ( StatTransfer.PositiveData ) ) {
    AveRef->SetCheck ( BoolToCheck ( false ) );
    NoRef ->SetCheck ( BoolToCheck ( true  )   );
    }
*/
}

/*
void    TStatisticsParamsDialog::CmPolarityEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( ! StatTransfer.IsCsvFile () && StatTransfer.HasTAnova () && CheckToBool ( StatTransfer.SignedData ) );
}
*/


void    TStatisticsParamsDialog::CmFDRCorrectionThresholdingEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( StatTransfer.PresetsCorrection.GetSelIndex () == StatCorrPresetThresholdingPValues );
}


void    TStatisticsParamsDialog::CmThresholdingPValuesEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( StatTransfer.PresetsCorrection.GetSelIndex () != StatCorrPresetThresholdingPValues );
}


void    TStatisticsParamsDialog::CmThresholdingPValuesValueEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( StatTransfer.PresetsCorrection.GetSelIndex () != StatCorrPresetThresholdingPValues 
          && CheckToBool ( StatTransfer.ThresholdingPValues ) );
}


void    TStatisticsParamsDialog::CmRandomizationEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( StatTransfer.UseRandomization () );
}


void    TStatisticsParamsDialog::CmCheckMissingValuesEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( StatTransfer.IsCsvFile () );
}


void    TStatisticsParamsDialog::CmMissingValuesEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

tce.Enable ( CheckToBool ( StatTransfer.CheckMissingValues ) );
}

                                        // could be used to check # samples is different in groups
//void    TStatisticsParamsDialog::CmPairedEnable ( TCommandEnabler &tce )
//{
//tce.Enable ( true );
//}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 ( TStatisticsDialog, TBaseDialog )

    EV_COMMAND                  ( IDOK,                     CmOk ),

    EV_COMMAND_AND_ID           ( IDC_TOPREVDIALOG,         CmToDialog ),
    EV_COMMAND_AND_ID           ( IDC_TONEXTDIALOG,         CmToDialog ),

END_RESPONSE_TABLE;


//----------------------------------------------------------------------------
void    TStatisticsDialog::CmToDialog ( owlwparam w )
{
uint                ResId           = PtrToUint ( Attr.Name );

                                        // goto new id
uint                todialogid      = Clip ( ResId + ( w == IDC_TOPREVDIALOG ? -1 : 1 ), (uint) IDD_STATISTICS1, (uint) IDD_STATISTICS2 );

                                        // avoid calling itself
if ( todialogid == ResId )
    return;


Destroy ();                             // remove the window, not the object

                                        // remember the last dialog
StatTransfer.LastDialogId   = todialogid;


if      ( todialogid == IDD_STATISTICS1 )   TStatisticsFilesDialog  ( CartoolMdiClient, IDD_STATISTICS1 ).Execute ();
else if ( todialogid == IDD_STATISTICS2 )   TStatisticsParamsDialog ( CartoolMdiClient, IDD_STATISTICS2 ).Execute ();
}


//----------------------------------------------------------------------------
void    TStatisticsDialog::SamplesParams ()
{
if ( StatTransfer.Is1File () || StatTransfer.IsCsvFile () ) {

    if ( StatTransfer.Is1File ()   && ( CheckToBool ( StatTransfer.NormalizationGfp       )
                                     || CheckToBool ( StatTransfer.NormalizationGfpPaired ) )
      || StatTransfer.IsCsvFile () &&  !CheckToBool ( StatTransfer.NormalizationNone      ) ) {

        StatTransfer.NormalizationGfp       = BoolToCheck ( false );
        StatTransfer.NormalizationGfpPaired = BoolToCheck ( false );
        StatTransfer.NormalizationGfp1TF    = BoolToCheck ( false );
        StatTransfer.NormalizationNone      = BoolToCheck ( true  );
        }

    if ( StatTransfer.IsCsvFile () ) {
        StatTransfer.UseXyzDoc              = BoolToCheck ( false );
        StatTransfer.AveRef                 = BoolToCheck ( false );
        StatTransfer.NoRef                  = BoolToCheck ( true  );

        StatTransfer.TAnova                 = BoolToCheck ( false );
        }

    StatTransfer.PresetsTime.Select ( StatTimeSequential );
    }


if ( StatTransfer.IsCsvFile () ) {
    StatTransfer.ExportTracks           = BoolToCheck ( true  );
    StatTransfer.ExportRois             = BoolToCheck ( false );

    StatTransfer.CheckMissingValues     = BoolToCheck ( true  );
    }
else {
    StatTransfer.CheckMissingValues     = BoolToCheck ( false );
    }


if ( StatTransfer.IsRois () ) {

    if ( CheckToBool ( StatTransfer.NormalizationGfp1TF ) ) {
        StatTransfer.NormalizationGfp       = BoolToCheck ( false );
        StatTransfer.NormalizationGfpPaired = BoolToCheck ( false );
        StatTransfer.NormalizationGfp1TF    = BoolToCheck ( false );
        StatTransfer.NormalizationNone      = BoolToCheck ( true  );
        }
    }
}


//----------------------------------------------------------------------------
void    TStatisticsDialog::CmOkEnable ( TCommandEnabler &tce )
{
TransferData ( tdGetData );

int     numgroups   = GoGoF.NumGroups ();
bool    twosample   = StatTransfer.HasTwoSample ();

                                        // directly playing with the buffer does not seem to be a good idea, maybe its updated during the test?
TFileName           buff;
StringCopy ( buff, StatTransfer.BaseFileName );

if ( ! IsAbsoluteFilename ( buff ) ) {
    tce.Enable ( false );
    return;
    }


if ( ( StatTransfer.IsChannels () || StatTransfer.IsCsvFile () ) && StringIsEmpty ( StatTransfer.Channels ) ) {
    tce.Enable ( false );
    return;
    }


StringCopy ( buff, StatTransfer.XyzDocFile );

if ( StatTransfer.IsChannels () && CheckToBool ( StatTransfer.UseXyzDoc ) && ! IsAbsoluteFilename ( buff ) ) {
    tce.Enable ( false );
    return;
    }


StringCopy ( buff, StatTransfer.RoisDocFile );

if ( StatTransfer.IsRois () && ! IsAbsoluteFilename ( buff ) ) {
    tce.Enable ( false );
    return;
    }


if ( StringToInteger ( StatTransfer.NumberOfRandomization ) <= 0 ) {
    tce.Enable ( false );
    return;
    }


tce.Enable ( numgroups >= 2
          && ( twosample && !( numgroups & 1 ) ) );
}


//----------------------------------------------------------------------------
void    TStatisticsDialog::CmOk ()
{
Destroy ();


if ( GoGoF.NumGroups () / 2 > MaxFilesToOpen )
    StatTransfer.OpenAuto       = BoolToCheck ( false );


BatchProcessGroups ( &GoGoF, 2, 2, &StatTransfer, ! CheckToBool ( StatTransfer.OpenAuto ) || ! StatTransfer.IsNFiles () );
}


//----------------------------------------------------------------------------
                                        // Check & update durations / end of files with a range of group of files
void    CheckGroupsDurations    (   TStatGoGoF&     gogof,      int         gofi1,      int     gofi2, 
                                    bool            paired,     bool        unpaired,
                                    bool            isnfiles,   bool        iscsvfile 
                                )
{
TStatGoF*           gof;
int                 timemax;
int                 goftimemax;
int                 mintimemax;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // NOT a good idea to direclty modify the GoGoF
                                        // because when running multiple times paired & unpaired test will change the #samples!

                                        // strict checking will modify number of samples
if ( paired ) {

    int             maxsamples  = INT_MAX;

                                        // check # of samples in each group
    for ( gofi = gofi1; gofi <= gofi2 ; gofi++ )
        maxsamples  = min ( StatTransfer.GetNumSamples ( gogof[ gofi ] ), maxsamples );


    if ( isnfiles ) {
                                        // adjust # files by removing excess files from groups
        for ( gofi = gofi1; gofi <= gofi2 ; gofi++ ) {
            gof = &gogof[ gofi ];

            gof->FileNames.RemoveLast ( gof->NumFiles - maxsamples );
            gof->NumFiles   = maxsamples;
            }
        }
    else if ( transfer.Is1File () ) {
        for ( gofi = gofi1; gofi <= gofi2 ; gofi++ ) {
            gof = &gogof[ gofi ];
            gof->TimeMax    = gof->TimeMin + maxsamples - 1;
            }
        }
    else if ( transfer.IsCsvFile () ) {
        for ( gofi = gofi1; gofi <= gofi2 ; gofi++ ) {
            gof = &gogof[ gofi ];
            gof->NumSamples = maxsamples;
            }
        }
    } // paired
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

mintimemax  = Highest<int> ();

                                        // resolve TF limits + boundary checking
for ( int gofi = gofi1; gofi <= gofi2 ; gofi++ ) {

    gof         = &gogof[ gofi ];

    goftimemax  = gof->EndOfFile ? Highest<int> () : gof->TimeMax;

                                        // scan all files in group
    if ( ! iscsvfile ) {

        TListIterator<char>     iterator;

        foreachin ( *gof, iterator ) {
            ReadFromHeader ( iterator (), ReadNumTimeFrames, &timemax );
            timemax--;

            Mined ( goftimemax, timemax );
            }
        }

                                        // update
    gof->TimeMax    = goftimemax;
                                        // in any case..
    if ( gof->TimeMax < gof->TimeMin )
        gof->TimeMin = gof->TimeMax;

    gof->EndOfFile  = false;            // clear this flag

                                        // store min interval amongst all groups, only for the sequential case
    if ( gof->IsTimeSequential ()  )
        Mined ( mintimemax, gof->GetTimeDuration () );
    }

                                        // set interval min common to all groups, still allowing origin shifts
for ( int gofi = gofi1; gofi <= gofi2 ; gofi++ ) {
    gof         = &gogof[ gofi ];

                                        // override TimeMax
    if ( gof->IsTimeSequential () && ( paired || unpaired && isnfiles ) )
        gof->TimeMax = gof->TimeMin + mintimemax - 1;

                                        // in any case..
    if ( gof->TimeMax < gof->TimeMin )
        gof->TimeMin = gof->TimeMax;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // clearing the average flag
for ( int gofi = gofi1; gofi <= gofi2 ; gofi++ ) {

    gof         = &gogof[ gofi ];
                                        // samples in single file (== 1 TF), or only 1 TF
    if ( ! isnfiles || gof->TimeMax == gof->TimeMin ) 
        gof->StatTime       = StatTimeSequential;
    }

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Processing 2 groups at a time
                                        // TODO: extract the computation only code as a separate function, with this method just deciphering the dialog's values and calling said function
void    TStatisticsDialog::ProcessGroups ( TGoGoF* /*gogof*/, int gofi1, int gofi2, void* usetransfer )
{
                                        // useless for the moment, we always use our local GoGoF
//TStatGoGoF     &GoGoF       = * dynamic_cast< TStatGoGoF * > ( gogof );

TStatStruct  &transfer    = usetransfer ? *(TStatStruct *) usetransfer : StatTransfer;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check and chew parameters
int                 numgof          = gofi2 - gofi1 + 1;

if ( numgof != 2 )
    return;

const TStatGoF&     gof1            = GoGoF[ gofi1 ];
const TStatGoF&     gof2            = GoGoF[ gofi2 ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check the less strict way first
bool                paired          = transfer.HasPaired    (); // these 2 are now exclusive, it doesn't really make sense to have both at the same time...
bool                unpaired        = transfer.HasUnpaired  ();
bool                recheck         = paired;
bool                isnfiles        = transfer.IsNFiles     ();
bool                isonefile       = transfer.Is1File      ();
bool                iscsvfile       = transfer.IsCsvFile    ();
bool                ischannels      = transfer.IsChannels   ();
bool                isrois          = transfer.IsRois       ();


CheckGroupsDurations    (   GoGoF,      gofi1,  gofi2, 
                            false,      unpaired,
                            isnfiles,   iscsvfile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                pttest          = paired   && CheckToBool ( transfer.TTest );
bool                uttest          = unpaired && CheckToBool ( transfer.TTest );

bool                prand           = paired   && CheckToBool ( transfer.Randomization );
bool                urand           = unpaired && CheckToBool ( transfer.Randomization );

bool                ptanova         = paired   && CheckToBool ( transfer.TAnova ) && ischannels;
bool                utanova         = unpaired && CheckToBool ( transfer.TAnova ) && ischannels;
bool                tanova          = ptanova || utanova;

TestTypesEnum       processings [ NumStatisticalTests ];
TestTypesEnum       processing;
int                 numprocessings  = 0;

                                        // add the unpaired tests first, because tests on data are weaker - then when the paired tests comes in, stricter tests will be run again
if ( uttest  )      processings[ numprocessings++ ]   = UnpairedTTest;
if ( urand   )      processings[ numprocessings++ ]   = UnpairedRandomization;
if ( utanova )      processings[ numprocessings++ ]   = UnpairedTAnova;
if ( pttest  )      processings[ numprocessings++ ]   = PairedTTest;
if ( prand   )      processings[ numprocessings++ ]   = PairedRandomization;
if ( ptanova )      processings[ numprocessings++ ]   = PairedTAnova;


if ( numprocessings == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TOpenDoc< TElectrodesDoc >  XYZDoc;

                                        // see if a coordinates file has been provided
if ( transfer.UseXyzDoc
  && CanOpenFile ( transfer.XyzDocFile ) ) {
                                        // will automatically close the document in case of premature exit    
    XYZDoc.Open ( transfer.XyzDocFile, OpenDocHidden );

    UpdateApplication;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // list of channels -> TSelection
int                 numel           = 0;
TSelection          elsel;
double              samplfreq;
char                buff[ EditSizeTextLong ];
int                 filetype;
char                ext    [ 32 ];
char                moreext[ 32 ];
int                 gfpoff;
int                 disoff;
int                 avgoff;
TRois*              rois            = 0;

TStrings            VarNames;
TStrings            TAnovaName;
bool                outputvarnames  = false;
TFileName           attr;
//int                 nummaps         = 0;        // used for csvfile

TAnovaName.Add ( InfixTAnova );


CsvType             csvtype;
int                 csvnumgroups;
TArray1<int>        csvgroupindex;
char                CsvFirstGroup[ 32 ];
int                 csvnumvarspergroup;
TStrings            csvvarnames;
int                 csvmaxmap       = 0;
//TSelection          csvmapsel;
TSelection          csvinmapsel;
TStrings            csvmapnames;
int                 csvnumfitvars;
TStrings            csvfitvarnames;
TSelection          csvoutmapsel;
int                 csvoutnummaps   = 0;        // used for csvfile


ClearString ( CsvFirstGroup );

                                        // Flag is set only for CsvStatFittingCartool || CsvStatMarkov
if ( iscsvfile ) {
                                        // Retrieve file & variables structures
    TSpreadSheet        sf;
                                        // open only the first file
                                        // - all files should contain the same set of variables
                                        // - files could have different # of groups stored into them, though
    sf.ReadFile ( gof1[ 0 ] );


    csvtype     = sf.GetType ();


//  if ( ! ( IsCsvStatFittingCartool ( csvtype ) 
//        || IsCsvStatMarkov         ( csvtype ) ) )
//      return;


    sf.GetFittingInfo ( csvnumgroups,       csvgroupindex,  CsvFirstGroup, 
                        csvnumvarspergroup, csvvarnames,
                        csvmaxmap,          csvinmapsel,    csvmapnames, 
                        csvnumfitvars,      csvfitvarnames );
    

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Copy all variable names

                                        // for consistency, add pseudo tracks names
    numel           = csvnumvarspergroup + NumPseudoTracks;
    samplfreq       = 0;
    gfpoff          = csvnumvarspergroup + PseudoTrackOffsetGfp;
    disoff          = csvnumvarspergroup + PseudoTrackOffsetDis;
    avgoff          = csvnumvarspergroup + PseudoTrackOffsetAvg;

                                        // copy to VarNames
    for ( int i = 0; i < csvnumvarspergroup; i++ )
        VarNames.Add ( csvvarnames ( i ) );


    VarNames.Add ( TrackNameGFP );
    VarNames.Add ( TrackNameDIS );
    VarNames.Add ( TrackNameAVG );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Can retrieve some OUTPUT maps, set by user (like 'Map8 Map9 Map10')?
    StringCopy ( buff, transfer.Channels );
                                        // map numbering starts from 1
    csvoutmapsel    = TSelection ( csvmaxmap + 1, OrderSorted );


    if ( IsCsvStatMarkov ( csvtype )                            // Markov
      || StringIsEmpty ( buff ) || StringIs ( buff, "*" ) )     // nothing specified, or '*'
                                                                // -> use all data
        csvoutmapsel    = csvinmapsel;

    else {
                                        // entry field not empty, scan it
        csvoutmapsel.Reset ();

        StringToLowercase ( buff );
        TSplitStrings       token ( buff, UniqueStrings, " \t,;" );

        int                 mapi;

        for ( int toki = 0; toki < (int) token; toki++ ) {
                                        // !case INsensitive!
            if ( IsLongNames ( csvtype ) ) {
                if ( StringStartsWith ( token[ toki ], "map" ) ) {
                    sscanf ( token[ toki ], "map%d", &mapi );
                    csvoutmapsel.Set ( mapi );
//                    DBGV ( mapi, "Map Index to Test" );
                    }
                }
            else {
                if ( StringStartsWith ( token[ toki ], "m" ) ) {
                    sscanf ( token[ toki ], "m%d", &mapi );
                    csvoutmapsel.Set ( mapi );
//                    DBGV ( mapi, "Map Index to Test" );
                    }
                }

            } // for token
        }

                                        // user input doesn't match anything map-like? -> use all input maps
    if ( ! (bool) csvoutmapsel )
        csvoutmapsel    = csvinmapsel;

    csvoutnummaps   = (int) csvoutmapsel;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Can retrieve specific OUTPUT variables, set by user (like 'GEV' 'NumTF' or simply 'TF')?
                                        //(BTW this intersects with the previous scan above, we try to handle it as nicely as possible)

    StringCopy ( buff, transfer.Channels );
    elsel           = TSelection ( numel, OrderSorted );  // stick to an ordered set, as we don't want something like map15 map14 map16
    elsel.Reset ();


    if ( IsCsvStatMarkov ( csvtype )                            // Markov
      || StringIsEmpty ( buff ) || StringIs ( buff, "*" ) )     // nothing specified, or '*'
                                                                // -> use all data
        elsel.Set ( 0, csvnumvarspergroup - 1 );

    else {
                                        // entry field not empty, scan it
//      elsel.Set    ( buff, &VarNames, numel );

        TSplitStrings       token ( buff, UniqueStrings, " \t,;" );
        TFileName           vnbuff;

                                        // break string in tokens
        for ( int toki = 0; toki < (int) token; toki++ ) {

            StringCopy ( vnbuff, token[ toki ] );
            StringToLowercase ( vnbuff );
//          DBGM ( vnbuff, "Found root name" );

                                        // now generate all cases
            for ( int i = 0, j = 0; i < sf.GetNumAttributes() && j < csvnumvarspergroup; i++ ) {

                StringCopy ( attr, sf.GetAttribute ( i ) );
                                        // !case INsensitive!
                if ( ! StringStartsWith ( attr, CsvFirstGroup ) )
                    continue;

                StringToLowercase ( attr );

                if ( StringContains ( attr, vnbuff ) ) {
                    elsel.Set ( j );    // select anything that contains the substring, f.ex. "Map1", "Tf", "Corr"...
//                    DBGM ( attr, "found variable" );
                    }

                j++;
                } // for attribute
            } // for token
        } // StringIsNotEmpty buff


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we can use the VarNames for output if all variables are used!
    outputvarnames  = (int) elsel == csvnumvarspergroup; //  IsCsvStatMarkov ( csvtype )

                                        // does not mean anything here
    elsel.Reset ( gfpoff );
    elsel.Reset ( disoff );
    elsel.Reset ( avgoff );


    if ( (int) elsel == 0 )
        ShowMessage ( "No variables were found with the names provided!", "Opening File", ShowMessageWarning );

    } // if iscsvfile


else { // ! iscsvfile
                                        // open first eeg
    TOpenDoc< TTracksDoc >      EEGDoc ( gof1[ 0 ], OpenDocHidden );

    numel           = EEGDoc->GetTotalElectrodes ();
    samplfreq       = EEGDoc->GetSamplingFrequency ();
    gfpoff          = EEGDoc->GetGfpIndex ();
    disoff          = EEGDoc->GetDisIndex ();
    avgoff          = EEGDoc->GetAvgIndex ();


    if ( isrois ) {
        rois        = new TRois ( transfer.RoisDocFile );

        if ( rois->GetDimension () != EEGDoc->GetNumElectrodes () ) {

            delete  rois;
            rois    = 0;

            ShowMessage ( "Dimension of the ROIs doesn't match the dimension of the input files!" NewLine "Aborting the processing now...", "ROIs File", ShowMessageWarning );
            return;
            }
        }


    if ( rois ) {                       // isrois + correct rois
        elsel           = TSelection ( rois->GetNumRois (), OrderArbitrary );

        VarNames        = *rois->GetRoiNames ();

        elsel.Set ();
        }
    else {
        elsel           = TSelection ( numel, OrderArbitrary );


        //StringCopy ( buff, EEGDoc->GetTitle () );
        //StringCopy ( ext, GetExtension ( buff ) );


        VarNames        = *( XYZDoc.IsOpen () ? XYZDoc->GetElectrodesNames () 
                                              : EEGDoc->GetElectrodesNames () );


        StringCopy ( buff, transfer.Channels );
        elsel.Reset ();
                                            // bypass the '*' of TSelection
                                            // for only regular electrodes to be selected
        if ( transfer.PresetsData.GetSelIndex () == StatPresetMarkov    // IsMarkov ( csvtype )
          || StringIsEmpty ( buff )                                     // to be practical, use everything if empty
          || tanova                                                     // always override for TAnova
          || StringIs ( buff, "*" )                                     // or * wildchar
            )
            elsel.Set ( 0, EEGDoc->GetNumElectrodes() - 1 );
        else                                // get the right list of electrodes names
            elsel.Set ( buff, &VarNames );

                                            // it is not recommended to stat on the dis...
        elsel.Reset ( EEGDoc->GetDisIndex () );

                                            // also do some checking if using TAnova
        if ( tanova )
            EEGDoc->ClearPseudo ( elsel );


    //  elsel.ToText ( transfer.Channels, &VarNames, EEGDoc->GetTotalElectrodes(), AuxiliaryTracksNames );
        }

                                        // we can use the VarNames for output if all variables are used!
    outputvarnames  = rois != 0 || (int) elsel == EEGDoc->GetNumElectrodes(); //  IsCsvStatMarkov ( csvtype )


    EEGDoc.Close ();
    } // ! iscsvfile


                                        // aborting now?
if ( (int) elsel == 0 ) {
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( "Reading", 100 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Correction & p-values post-processing
int                 presetcorri         = transfer.PresetsCorrection.GetSelIndex ();

CorrectionType      multipletestscorrection;

int                 numvars             = (int) elsel;

int                 bonferronicorrectionvalue   = NonNull ( numvars );
double              fdrcorrectionvalue          = Clip ( StringToDouble ( transfer.FDRCorrectionValue ) / 100, 0.0, 1.0 );


bool                thresholdingpvalues = CheckToBool ( transfer.ThresholdingPValues ) 
                                       && presetcorri != StatCorrPresetThresholdingPValues;     // NO p-value thresholding with this FDR correction: results are already clipped AND the results can be legally above the threshold

double              pvaluesthreshold    = thresholdingpvalues   ? Clip ( StringToDouble ( transfer.ThresholdingPValuesValue ) / 100, 0.0, 1.0 ) 
                                                                : 1.0;  // by safety, assign threshold to max possible value

int                 significantduration = AtLeast ( 1, StringToInteger ( transfer.MinSignificantDurationValue ) );

bool                clippingpduration   = CheckToBool ( transfer.MinSignificantDuration )
                                       && isnfiles
                                       && ( thresholdingpvalues || presetcorri == StatCorrPresetThresholdingPValues )
                                       && significantduration > 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                openauto                = CheckToBool ( transfer.OpenAuto    )
                                            && ( ! IsFreqLoop () || IsLastFreq () );
bool                moreoutputs             = CheckToBool ( transfer.MoreOutputs );


bool                alltimeavg              = gof1.IsTimeAveraged () && gof2.IsTimeAveraged ();
bool                sometimeavg             = gof1.IsTimeAveraged () || gof2.IsTimeAveraged ();


int                 inputnumtf[ 3 ];
                    inputnumtf[ 0 ]         = isnfiles  ?   gof1.GetTimeDuration () 
                                                        :   1;
                    inputnumtf[ 1 ]         = isnfiles  ?   gof2.GetTimeDuration () 
                                                        :   1;
                    inputnumtf[ 2 ]         = max ( inputnumtf[ 0 ], inputnumtf[ 1 ] );             // maxinputnumtf

int                 outputnumtf[ 3 ];    
                    outputnumtf[ 0 ]        = gof1.IsTimeSequential ()  ?   inputnumtf[ 0 ] 
                                                                        :   1;
                    outputnumtf[ 1 ]        = gof2.IsTimeSequential ()  ?   inputnumtf[ 1 ] 
                                                                        :   1;
                    outputnumtf[ 2 ]        = max ( outputnumtf[ 0 ], outputnumtf[ 1 ] );           // jointnumtf = max # of output TF


int                 numsamples[ 3 ];
                    numsamples[ 0 ]         = isnfiles  ?   gof1.NumFiles ()
                                            : iscsvfile ?   gof1.NumSamples
                                            :               gof1.GetTimeDuration ();
                    numsamples[ 1 ]         = isnfiles  ?   gof2.NumFiles ()
                                            : iscsvfile ?   gof2.NumSamples
                                            :               gof2.GetTimeDuration ();
                    numsamples[ 2 ]         = min ( numsamples[ 0 ], numsamples[ 1 ] );             // jointnumsamples = min # of samples

bool                equalsamples            = numsamples[ 0 ] == numsamples[ 1 ];


bool                groupissequential[ 2 ];
                    groupissequential[ 0 ]  = gof1.IsTimeSequential ();
                    groupissequential[ 1 ]  = gof2.IsTimeSequential ();

int                 numrand                 = StringToInteger ( transfer.NumberOfRandomization );

bool                checkmissingvalues  = CheckToBool ( transfer.CheckMissingValues );
double              missingvalue        = StringToDouble ( transfer.CheckMissingValuesValue );

//bool                polaritymatters = transfer.HasTAnova () && ! iscsvfile;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We have to link datatype and reference: average ref <-> scalar, so that the norm of data corresponds to the GFP, not the RMS
AtomType         	datatype;
if      ( CheckToBool ( transfer.PositiveData   ) )         datatype    = AtomTypePositive;
else if ( CheckToBool ( transfer.SignedData     ) )         datatype    = AtomTypeScalar;
//else if ( CheckToBool ( transfer.VectorData     ) )         datatype    = AtomTypeVector;
else                                                        datatype    = AtomTypeScalar;

                                        // downgrade vectorial type to scalar if not all vectorial data
bool                allris      = gof1.AllExtensionsAre ( AllRisFilesExt, UnknownAtomType ) && gof2.AllExtensionsAre ( AllRisFilesExt, UnknownAtomType );
bool                allrisv     = gof1.AllExtensionsAre ( AllRisFilesExt, AtomTypeVector  ) && gof2.AllExtensionsAre ( AllRisFilesExt, AtomTypeVector  );
//if ( datatype == AtomTypeVector && ! allrisv )            datatype    = AtomTypePositive;

                                                                              // not Positive, not Vectorial
bool                ignorepolarity  = CheckToBool ( transfer.IgnorePolarity ) 
                                   && ! ( datatype == AtomTypePositive || datatype == AtomTypeVector );

PolarityType        polarity        = ignorepolarity ? PolarityEvaluate : PolarityDirect;


ReferenceType       dataref;
if      ( CheckToBool ( transfer.AveRef ) )                 dataref     = ReferenceAverage;
else /*if ( CheckToBool ( transfer.NoRef ) )*/              dataref     = ReferenceAsInFile;


CheckReference ( dataref, datatype );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Processing reference for TAnova           not actually needed, as dataref should be avg ref already, but still
ReferenceType       processingref   = GetProcessingRef ( allris ? ProcessingReferenceESI : ProcessingReferenceEEG );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 normalize;
if      ( CheckToBool ( transfer.NormalizationGfp       ) )     normalize   = IDC_RESCALE_GFP;
else if ( CheckToBool ( transfer.NormalizationGfpPaired ) )     normalize   = IDC_RESCALE_GFP_PAIRED;
else if ( CheckToBool ( transfer.NormalizationGfp1TF    ) )     normalize   = IDC_RESCALE_GFP_1TF;
else                                                            normalize   = IDC_RESCALE_NONE;

                                        // if not same amount of files, remove the paired GFP normalization
if ( normalize == IDC_RESCALE_GFP_PAIRED 
  && ! equalsamples )
                                        // downgrade to single file GFP normalization
        normalize   = IDC_RESCALE_GFP;

                                        // force no normalization
if ( ( normalize == IDC_RESCALE_GFP || normalize == IDC_RESCALE_GFP_PAIRED ) 
   && ! isnfiles )

    normalize   = IDC_RESCALE_NONE;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // there are 2 groups of files
TArray3<float>*     Data;
TArray3<float>*     Results;
TTracks<float>      eegb;
TTracks<float>      sampleddiss;


Data    = new TArray3<float>  [ 2 ];
Results = new TArray3<float>  [ 3 ];    // results for each group, plus joint results


int                 writenumtf      = alltimeavg ? 1 : inputnumtf[ 2 ];

                                        // do we need any average?
bool                anyaverage      = isonefile || inputnumtf[ 2 ] == 1 ? false         // anyway, make it foolproof
                                                                        : sometimeavg;

                                        // allocate a buffer for each group of files
                                        // each could have a different # samples dimension

for ( int gofi0 = 0; gofi0 < 2; gofi0++ )

    Data[ gofi0 ].Resize (  inputnumtf[ 2 ] + ( anyaverage ? 1 : 0 ),   // 1 more for averages, only if needed
                            numvars  + 1,                               // 1 more for GFP
                            numsamples[ gofi0 ]                );       // # of samples can be different for each group

                                        // one result per group, plus one joint results
for ( int gofi0 = 0; gofi0 < 3; gofi0++ )

    Results[ gofi0 ].Resize ( inputnumtf[ 2 ],
                              numvars + 1,                      // one more for temp computation
                              NumTestVariables );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // Prototype code to check for inverse results consistency across neighbors
bool            dospicheck  = false;
int             spimincount;
TArray2<int>    SpiCheck;
#define         MaxSpiCheck         26


if ( dospicheck ) { //filetype == IDC_FILETYPE_RIS ) {

    GetFileFromUser getfile ( "", AllSolPointsFilesFilter, 1, GetFilePath );

    if ( ! getfile.Execute () )
        dospicheck = false;
    else
        StringCopy ( buff, getfile );

    if ( dospicheck && ! CartoolDocManager->CanOpenFile ( buff, params?) )
        dospicheck = false;


    TFileName           SpiFile;
    StringCopy ( SpiFile, buff );

    TSolutionPointsDoc*     SpiDoc = dynamic_cast< TSolutionPointsDoc* > ( CartoolDocManager->OpenDoc ( SpiFile, dtOpenOptionsNoView ) );


    ClearString ( buff );
    if ( ! GetValueFromUser ( "Give the min number of neighbors: ", "Topological Checking", buff, this )
    if ( ! GetInputFromUser ( "File already exist, give a new path, or overwite it:", getfile, buff, "" ) )
        dospicheck = false;
    else
        spimincount = StringToInteger ( buff );

    if ( spimincount <= 0 || spimincount > MaxSpiCheck )
        dospicheck = false;

                                                       // increase a little the limit
                                                       // regular    : only a few % is enough, to include boundary & rounding errors
                                                       // non-regular: multiply by  sqrt ( 3 )  as the furthest distance to a corner box
    double          mindist = SpiDoc->GetMinDistance () * sqrt ( 3 ); // IsGridAligned ()
    int             numsp   = SpiDoc->GetNumSolPoints ();
    TPointFloat    *points  = SpiDoc->GetPoints ();
    int             sp1, sp2;
    TPointDouble   *tosp1, *tosp2;
    double          dx, dy, dz;
    int             count1, count2;

                                        // 26 neighbors + 1 counter
    SpiCheck.Resize ( numsp, MaxSpiCheck + 1 );


    for ( sp1 = 0, tosp1 = points; sp1 < numsp; sp1++, tosp1++ ) {

        Gauge.SetValue ( SuperGaugeDefaultPart, Percentage ( numsp - sp1, numsp ) );

                                        // neighborhood is reciprocal
        for ( sp2 = sp1 + 1, tosp2 = points + sp2; sp2 < numsp; sp2++, tosp2++ ) {

            dx  = abs ( tosp2->x - tosp1->x );
            dy  = abs ( tosp2->y - tosp1->y );
            dz  = abs ( tosp2->z - tosp1->z );

                                        // "8" neighbors
            if ( dx <= mindist && dy <= mindist && dz <= mindist ) {
                                        // increase counter
                SpiCheck ( sp1 , 0 )++;
                SpiCheck ( sp2 , 0 )++;

                count1 = SpiCheck ( sp1 , 0 );
                count2 = SpiCheck ( sp2 , 0 );
                                        // store indexes
                if ( count1 <= MaxSpiCheck )    SpiCheck ( sp1 , count1 ) = sp2;
                if ( count2 <= MaxSpiCheck )    SpiCheck ( sp2 , count2 ) = sp1;
                }

            } // for sp2

        } // for sp1


	CartoolDocManager->CloseDoc ( SpiDoc );
    }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // open files & populate arrays
for ( int gofi0 = 0; gofi0 < 2; gofi0++ ) {

    const TStatGoF*     gof         = gofi0 == 0 ? &gof1 : &gof2;

    Data[ gofi0 ].ResetMemory ();


    for ( int filei = 0; filei < gof->NumFiles (); filei++ ) {

        if ( ! isonefile )              // gauge per file
            Gauge.SetValue ( SuperGaugeDefaultPart, Percentage ( gofi0 * gof->NumFiles () + filei + 1 + 2, 2 * gof->NumFiles () + 2 ) );


        if ( iscsvfile ) {

            TSpreadSheet        sf;

            sf.ReadFile ( (*gof)[ filei ] );

                                        // set the index of first variable
            int             vari    = gof->GroupIndex * csvnumvarspergroup;

            for ( int i = 0; i < sf.GetNumAttributes(); i++ ) {

                StringCopy ( attr, sf.GetAttribute ( i ) );

                if ( IsLongNames  ( csvtype ) && StringStartsWith ( attr, FitGroupNameLong  )
                  || IsShortNames ( csvtype ) && StringStartsWith ( attr, FitGroupNameShort ) ) {
                    vari += i;
                    break;
                    }
                }

                                        // for each subject / line
            for ( int s = 0; s < gof->NumSamples; s++ ) {

                for ( TIteratorSelectedForward ei ( elsel ); (bool) ei; ++ei ) {

                    sf.GetRecord ( s, vari + ei(), attr );

                    Data[ gofi0 ] ( 0, ei.GetIndex (), s )  = StringToDouble ( attr );
                    }

                                        // set a dummy GFP
                Data[ gofi0 ] ( 0, numvars, s ) = 1;
                } // for s
            } // if iscsvfile

        else { // tracks

            TOpenDoc<TTracksDoc>    EEGDoc ( (*gof)[ filei ], OpenDocHidden );

                                        // switching frequency?
            if ( IsFreqLoop () ) {

                TFreqDoc*   FreqDoc     = dynamic_cast<TFreqDoc*> ( (TTracksDoc*) EEGDoc );

                FreqDoc->SetCurrentFrequency ( FreqIndex );

                StringCopy ( FreqName, FreqDoc->GetFrequencyName ( FreqIndex ) );

                FreqType    = FreqDoc->GetFreqType ();

                                        // above "99.99 Hz", the "Hz" is truncated to "H", like "100.00 H"
                                        // correct this as we need the correct file names for the ending merge
//              if ( StringEndsWith ( FreqName, " H" ) )
//                  StringAppend ( FreqName,      "z" );
                }

                                        // get all data, with type, ref, and rois
            EEGDoc->GetTracks   (   gof->TimeMin,   gof->TimeMax, 
                                    eegb,           0, 
                                    datatype, 
                                    ComputePseudoTracks, 
                                    dataref,        0,
                                    rois 
                                );


            for ( int tf = gof->TimeMin, tf0 = 0; tf <= gof->TimeMax; tf++, tf0++ ) {

                if ( isonefile )        // gauge per TF
                    Gauge.SetValue ( SuperGaugeDefaultPart, Percentage ( gofi0 * gof->NumFiles () + filei + (double) tf0 / gof->GetTimeDuration (), 2 * gof->NumFiles () ) );

                                        // dispatch to my buffers
                for ( TIteratorSelectedForward ei ( elsel ); (bool) ei; ++ei ) {

                    if ( isnfiles )
                        Data[ gofi0 ] ( tf0, ei.GetIndex (), filei   )  = rois ? eegb ( rois->GetRoi ( ei() )->Selection.FirstSelected (), tf0 ) 
                                                                               : eegb ( ei()                                             , tf0 );
                    else
                        Data[ gofi0 ] ( 0,   ei.GetIndex (), tf0     )  = rois ? eegb ( rois->GetRoi ( ei() )->Selection.FirstSelected (), tf0 ) 
                                                                               : eegb ( ei()                                             , tf0 );
                    }

                                        // store GFP
                                        // !GFP here is always computed with an AvgRef, even for positive data - It shouldn't really matter as it is used for scaling!
                if ( isnfiles )
                    Data[ gofi0 ] ( tf0, numvars, filei )   = eegb ( gfpoff , tf0 );
                else
                    Data[ gofi0 ] ( 0,   numvars, tf0   )   = eegb ( gfpoff , tf0 );

                } // for tf


            EEGDoc.Close ();
            } // else tracks

        } // for file

    } // for gof

                                        // in case the buffer grew big
eegb.DeallocateMemory ();

Gauge.Finished ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here we could test for skewed data, and un-skew them(?)
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // level normalizing
if ( normalize != IDC_RESCALE_NONE ) {

    long double     rescalefactor;


    for ( int gofi0 = 0; gofi0 < 2; gofi0++ ) {

        if      ( normalize == IDC_RESCALE_GFP_1TF ) {

            for ( int s = 0; s < numsamples[ gofi0 ]; s++ ) {

                for ( int tf0 = 0; tf0 < inputnumtf[ gofi0 ]; tf0++ ) {

                    rescalefactor = NonNull ( Data[ gofi0 ] ( tf0, numvars, s ) );

                                        // normalize only regular electrodes / sp
                    for ( TIteratorSelectedForward ei ( elsel ); (bool) ei; ++ei )

                        Data[ gofi0 ] ( tf0, ei.GetIndex (), s )  /= rescalefactor;
                    } // for tf
                } // for samples
            } // if IDC_RESCALE_GFP_1TF

                                        // apply a stricter / simpler normalization
        else if ( normalize == IDC_RESCALE_GFP ) {

            for ( int s = 0; s < numsamples[ gofi0 ]; s++ ) {

                rescalefactor   = 0;

                for ( int tf0 = 0; tf0 < inputnumtf[ gofi0 ]; tf0++ )

                    rescalefactor += Data[ gofi0 ] ( tf0, numvars, s );

                rescalefactor   = NonNull ( rescalefactor / inputnumtf[ gofi0 ] );


                for ( int tf0 = 0; tf0 < inputnumtf[ gofi0 ]; tf0++ )

                    for ( TIteratorSelectedForward ei ( elsel ); (bool) ei; ++ei )
                                        // be cautious to what we are normalizing
                        if      ( ei() == disoff )  continue;
                        else if ( ei() == gfpoff )  Data[ gofi0 ] ( tf0, ei.GetIndex (), s )  /= abs ( rescalefactor );
                        else                        Data[ gofi0 ] ( tf0, ei.GetIndex (), s )  /=       rescalefactor;
                } // for samples
            } // if IDC_RESCALE_GFP
                                        // if paired is selected, apply a smarter normalization
                                        // by using the 2 conditions together to get the norm
                                        // so that the ratio between the 2 conditions is not changed!
        else if ( normalize == IDC_RESCALE_GFP_PAIRED ) {

            for ( int s = 0; s < numsamples[ gofi0 ]; s++ ) {
                                        // even if paired, one can be an average, therefore with different # of TF
                rescalefactor   = 0;

                for ( int tf0 = 0; tf0 < inputnumtf[ 0 ]; tf0++ )

                    rescalefactor += Data[ gofi0     ] ( tf0, numvars, s );


                for ( int tf0 = 0; tf0 < inputnumtf[ 1 ]; tf0++ )

                    rescalefactor += Data[ gofi0 + 1 ] ( tf0, numvars, s );

                rescalefactor   = NonNull ( rescalefactor / ( inputnumtf[ 0 ] + inputnumtf[ 1 ] ) );


                for ( int tf0 = 0; tf0 < inputnumtf[ 0 ]; tf0++ )

                    for ( TIteratorSelectedForward ei ( elsel ); (bool) ei; ++ei )

                        if      ( ei() == disoff )  continue;
                        else if ( ei() == gfpoff )  Data[ gofi0     ] ( tf0, ei.GetIndex (), s )  /= abs ( rescalefactor );
                        else                        Data[ gofi0     ] ( tf0, ei.GetIndex (), s )  /=       rescalefactor;


                for ( int tf0 = 0; tf0 < inputnumtf[ 1 ]; tf0++ )

                    for ( TIteratorSelectedForward ei ( elsel ); (bool) ei; ++ei )

                        if      ( ei() == disoff )  continue;
                        else if ( ei() == gfpoff )  Data[ gofi0 + 1 ] ( tf0, ei.GetIndex (), s )  /= abs ( rescalefactor );
                        else                        Data[ gofi0 + 1 ] ( tf0, ei.GetIndex (), s )  /=       rescalefactor;

                } // for samples

            gofi0++;    // skip next group
            } // if IDC_RESCALE_GFP_PAIRED

        } // for gof
    } // normalizing


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute any needed TF average
for ( int gofi0 = 0; gofi0 < 2; gofi0++ ) {

    const TStatGoF*     gof         = gofi0 == 0 ? &gof1 : &gof2;

    if ( ! gof->IsTimeAveraged () )
        continue;


    int                 avgindex        = GetTimeAverageIndex ( Data[ gofi0 ] );
    TEasyStats          stat ( inputnumtf[ gofi0 ] );


    for ( int v = 0; v < numvars;             v++ )
    for ( int s = 0; s < numsamples[ gofi0 ]; s++ ) {

        stat.Reset ();

        for ( int tf0 = 0; tf0 < inputnumtf[ gofi0 ]; tf0++ )

            stat.Add ( Data[ gofi0 ] ( tf0, v, s ) );

                                        // store the average in the last TF
                                        // we now have a bunch of different ways to convert n time frames to 1
        Data[ gofi0 ] ( avgindex, v, s )    = gof->StatTime == StatTimeMean     ?   stat.Mean   ()
                                            : gof->StatTime == StatTimeMedian   ?   stat.Median ()
                                            : gof->StatTime == StatTimeMin      ?   stat.Min    ()
                                            : gof->StatTime == StatTimeMax      ?   stat.Max    ()
                                            :                                       stat.Mean   ();
        } // for variable, sample

    } // for gof


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute a global maxvalue
double              maxvalue        = 1;

for ( int gofi0 = 0; gofi0 < 2; gofi0++ ) {

    for ( int tf0 = 0; tf0 < inputnumtf[ gofi0 ]; tf0++ )
    for ( int v   = 0; v   < numvars;             v++   )
    for ( int s   = 0; s   < numsamples[ gofi0 ]; s++   )

        Maxed ( maxvalue, abs ( (double) Data[ gofi0 ] ( tf0, v, s ) ) );
    } // for gof


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // choose file type
int                 presetsindex    = transfer.FileTypes.GetSelIndex ();

if      ( presetsindex == PresetFileTypeEph
       && samplfreq > 0                      )  filetype = IDC_FILETYPE_EPH;
else if ( presetsindex == PresetFileTypeEp  )   filetype = IDC_FILETYPE_EP;
else if ( presetsindex == PresetFileTypeTxt )   filetype = IDC_FILETYPE_TXT;
else if ( presetsindex == PresetFileTypeSef )   filetype = IDC_FILETYPE_SEF;
else if ( presetsindex == PresetFileTypeBV  )   filetype = IDC_FILETYPE_BV;
else if ( presetsindex == PresetFileTypeEdf )   filetype = IDC_FILETYPE_EDF;
else if ( presetsindex == PresetFileTypeRis )   filetype = IDC_FILETYPE_RIS;
else                                            filetype = IDC_FILETYPE_BV;


StringCopy ( ext,       SavingEegFileExtPreset[ presetsindex ] );
StringCopy ( moreext,   FILEEXT_EEGSEF );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           BaseFileName;
TFileName           GroupBaseFileName;
TFileName           VerbFile;
TFileName           TFile;
TFileName           DFile;
TFileName           PFile;
TFileName           TDataFile;
TFileName           DDataFile;
TFileName           PDataFile;
TFileName           PValueName;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // choose p value type
OutputPType         outputp;

if      ( CheckToBool ( transfer.OutputP         ) )  { outputp    = OutputP;           StringCopy ( PValueName, InfixP       ); }
else if ( CheckToBool ( transfer.Output1MinusP   ) )  { outputp    = Output1MinusP;     StringCopy ( PValueName, Infix1MinusP ); }
else if ( CheckToBool ( transfer.OutputMinusLogP ) )  { outputp    = OutputMinusLogP;   StringCopy ( PValueName, InfixLogP    ); }
else                                                  { outputp    = Output1MinusP;     StringCopy ( PValueName, Infix1MinusP ); }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // repeat for each processings
for ( int proci = 0; proci < numprocessings; proci++ ) {

    processing  = processings[ proci ];

                                        // redo the stricter checking now
    if ( recheck && IsPaired ( processing ) ) {

        CheckGroupsDurations    (   GoGoF,      gofi1,  gofi2, 
                                    paired,     unpaired,   // real check now
                                    isnfiles,   iscsvfile );

        recheck = false;
        }

                                        // set the correction according to current processing
    multipletestscorrection = presetcorri == StatCorrPresetNone                 ?   CorrectionNone
                            : presetcorri == StatCorrPresetBonferroni           ?   CorrectionBonferroni
                            : presetcorri == StatCorrPresetThresholdingPValues  ?   CorrectionFDRThresholdedP
//                          : presetcorri == StatCorrPresetCorrecting           ?   CorrectionFDRCorrectedP
                            : presetcorri == StatCorrPresetAdjusting            ?   CorrectionFDRAdjustedP
                            :                                                       CorrectionNone;
        
                                        // there is no need of correction for TAnova, as there is only 1 test
    if ( IsTAnova ( processing ) )
        multipletestscorrection = CorrectionNone;

                                            // no t and p values for fitting results, though we want some results for 1 TF tracks...
    bool                exporttfile     = ! iscsvfile /*( writenumtf > 1 )*/ && IsTTest         ( processing ); // t values exist
    bool                exportdfile     = ! iscsvfile /*( writenumtf > 1 )*/ && IsRandomization ( processing ); // t values don't exist, use the average of differences instead
    bool                exportpfile     = ! iscsvfile /*( writenumtf > 1 )*/;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // filename generation
    int                 numpairsofgroups= GoGoF.NumGroups () / 2;

    StringCopy      ( BaseFileName, transfer.BaseFileName );

    if ( numpairsofgroups >= 2 )    StringAppend    ( BaseFileName, ".", IntegerToString ( gofi1 / 2 + 1, NumIntegerDigits ( numpairsofgroups ) ) );


    StringCopy      ( buff, ToFileName ( BaseFileName ) );

//                                      // add current test as directory - remove this to have it all in the same common directory
//  if      ( processing == UnpairedTTest           )    StringAppend ( BaseFileName, "\\"   InfixUnpaired  "."  InfixTTest  );
//  else if ( processing == UnpairedRandomization   )    StringAppend ( BaseFileName, "\\"   InfixUnpaired  "."  InfixRand   );
//  else if ( processing == UnpairedTAnova          )    StringAppend ( BaseFileName, "\\"   InfixUnpaired  "."  InfixTAnova );
//  else if ( processing == PairedTTest             )    StringAppend ( BaseFileName, "\\"   InfixPaired    "."  InfixTTest  );
//  else if ( processing == PairedRandomization     )    StringAppend ( BaseFileName, "\\"   InfixPaired    "."  InfixRand   );
//  else if ( processing == PairedTAnova            )    StringAppend ( BaseFileName, "\\"   InfixPaired    "."  InfixTAnova );


                                        // destroy all previous directory content
    if ( IsFirstFreq () )
        NukeDirectory ( BaseFileName );


    if ( ! CreatePath ( BaseFileName, false ) ) {
        ShowMessage ( "Error with file path!", "Creating Directory", ShowMessageWarning );
//      continue;
        return;
        }

                                        // add the base name (just file part) plus the test name
    StringAppend    ( BaseFileName, "\\", buff );


    if      ( processing == UnpairedTTest           )    StringAppend ( BaseFileName, "."  InfixUnpaired  "."  InfixTTest  );
    else if ( processing == UnpairedRandomization   )    StringAppend ( BaseFileName, "."  InfixUnpaired  "."  InfixRand   );
    else if ( processing == UnpairedTAnova          )    StringAppend ( BaseFileName, "."  InfixUnpaired  "."  InfixTAnova );
    else if ( processing == PairedTTest             )    StringAppend ( BaseFileName, "."  InfixPaired    "."  InfixTTest  );
    else if ( processing == PairedRandomization     )    StringAppend ( BaseFileName, "."  InfixPaired    "."  InfixRand   );
    else if ( processing == PairedTAnova            )    StringAppend ( BaseFileName, "."  InfixPaired    "."  InfixTAnova );

                                        // save the global base file name, without frequency
    StringCopy      ( GroupBaseFileName, BaseFileName );

                                        // add only to the file name (not the directory!) the frequency part
    if ( IsFreqLoop () )
        StringAppend ( BaseFileName, ".", FreqName );

                                        // delete any previous files, with my prefix only!
    StringCopy      ( buff,          BaseFileName,       ".*" );
    DeleteFiles     ( buff );


    StringCopy      ( VerbFile,      BaseFileName,       ".",                    FILEEXT_VRB     );
    StringCopy      ( TFile,         BaseFileName,       ".", InfixT,     ".",   ext             );
    StringCopy      ( DFile,         BaseFileName,       ".", InfixDelta, ".",   ext             ); // can duplicate the intermediate files Mean.Delta or Delta.Mean
    StringCopy      ( PFile,         BaseFileName,       ".", PValueName, ".",   ext             );
    StringCopy      ( TDataFile,     BaseFileName,       ".", InfixT,     ".",   FILEEXT_DATA    );
    StringCopy      ( DDataFile,     BaseFileName,       ".", InfixDelta, ".",   FILEEXT_DATA    );
    StringCopy      ( PDataFile,     BaseFileName,       ".", PValueName, ".",   FILEEXT_DATA    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // verbose file init
    TVerboseFile    verbose ( VerbFile, VerboseFileDefaultWidth );

    verbose.PutTitle ( "Statistics" );


    verbose.NextTopic ( "Parameters:" );
    {
    if      ( processing == UnpairedTTest           )    verbose.Put ( "Test used:", "Unpaired t-test" );
    else if ( processing == UnpairedRandomization   )    verbose.Put ( "Test used:", "Unpaired Randomization / Difference of Means" );
    else if ( processing == UnpairedTAnova          )    verbose.Put ( "Test used:", "Unpaired TAnova" );
    else if ( processing == PairedTTest             )    verbose.Put ( "Test used:", "Paired t-test" );
    else if ( processing == PairedRandomization     )    verbose.Put ( "Test used:", "Paired Randomization / Mean of Differences" );
    else if ( processing == PairedTAnova            )    verbose.Put ( "Test used:", "Paired TAnova" );

    if ( IsTAnova ( processing ) ) {
        verbose.Put ( "TAnova test:", "One-tailed" );
        verbose.Put ( "TAnova centroids computation:", "Mean" );
        }
    else
        verbose.Put ( "", "Two-tailed" );


    verbose.NextLine ();
    verbose.Put ( "Testing:", rois ? "ROIs" : iscsvfile ? "Variables" : "Tracks" );


    if ( rois ) {
        verbose.Put ( "ROIs from file:", transfer.RoisDocFile );
        verbose.Put ( "Number of variables:", rois->GetNumRois () );
        verbose.Put ( "Variables to test:", rois->RoiNamesToText ( buff ) );
        }
    else {
        verbose.Put ( "Number of variables:", (int) elsel );
        verbose.Put ( "Variables to test:", iscsvfile ? transfer.Channels : elsel.ToText ( buff, &VarNames, AuxiliaryTracksNames ) );
        }

//  verbose.Put ( "Variables to test:", elsel.ToText ( buff, &VarNames, AuxiliaryTracksNames ) );

    if ( IsFreqLoop () ) {
        verbose.NextLine ();
        verbose.Put ( "Frequency:", FreqName );
        }


    if ( iscsvfile && IsCsvStatFitting ( csvtype ) ) {
        verbose.NextLine ();
        verbose.Put ( "Excluding missing values:", checkmissingvalues );
        if ( checkmissingvalues )
            verbose.Put ( "Values excluded:", missingvalue );
        }


    verbose.NextLine ();
    if      ( datatype == AtomTypeScalar   )    verbose.Put ( "Data type:", "Signed Data" );
    else if ( datatype == AtomTypePositive )    verbose.Put ( "Data type:", allrisv ? "Positive Data (Norm of Vectors)" : "Positive Data" );
//  else if ( datatype == AtomTypeVector   )    verbose.Put ( "Data type:", "3D Vectorial Data" );

    verbose.Put ( "Data reference:", ReferenceNames[ dataref ] );

    if ( IsTAnova ( processing ) ) {
        verbose.Put ( "Correlation reference:", ReferenceNames[ processingref ] );
        verbose.Put ( "Maps / Templates Polarity:", datatype == AtomTypePositive ? "Not relevant" : ignorepolarity ? "Ignore" : "Account" );
        }


    verbose.NextLine ();
    if      ( normalize == IDC_RESCALE_NONE         )   verbose.Put ( "Data level normalization:", "None" );
    else if ( normalize == IDC_RESCALE_GFP          )   verbose.Put ( "Data level normalization:", "Mean GFP, independently on each condition" );
    else if ( normalize == IDC_RESCALE_GFP_PAIRED   )   verbose.Put ( "Data level normalization:", "Mean GFP, joining paired conditions together" );
    else if ( normalize == IDC_RESCALE_GFP_1TF      )   verbose.Put ( "Data level normalization:", "GFP of each single sample" );


    verbose.NextLine ();
    verbose.Put ( "Multiple-tests correction:", CorrectionTypeString[ multipletestscorrection ] );

    if ( multipletestscorrection == CorrectionBonferroni ) {
        verbose.Put ( "Bonferroni with number of tests:", bonferronicorrectionvalue );
        }
    else if ( multipletestscorrection == CorrectionFDRThresholdedP ) {
        verbose.Put ( "FDR discovery rate [%]:", fdrcorrectionvalue * 100, 2 );
        }

    if ( IsCorrectionFDR ( multipletestscorrection ) )
        verbose.Put ( "FDR output:", IsCorrectionFDROutputQ ( multipletestscorrection ) ? "q-values" : "p-values" );

    verbose.NextLine ();
    verbose.Put ( "Thresholding p-values:", thresholdingpvalues );
    if ( thresholdingpvalues )
        verbose.Put ( "Keeping p-values below [%]:", pvaluesthreshold * 100, 2 );

    verbose.Put ( "Minimum Significant Duration:", clippingpduration );
    if ( clippingpduration ) 
        verbose.Put ( "p-value minimum duration [TF]:", significantduration );

    verbose.Put ( "p-values output formula:", OutputPTypeString[ outputp ] );

    if ( ! IsTAnova ( processing ) ) {
        verbose.NextLine ();

        if ( iscsvfile )    verbose.Put ( "Subtracting groups:", gof1.GroupName, " - ", gof2.GroupName );
        else                verbose.Put ( "Subtracting groups:", "First Group - Second Group" );
        }
    }


    verbose.NextTopic ( "Input Files:" );
    {
    if      ( isnfiles  )   verbose.Put ( "Data format:", "Data span across a set of files" );
    else if ( isonefile )   verbose.Put ( "Data format:", "Data are consecutive in 1 file" );
    else if ( iscsvfile )   verbose.Put ( "Data format:", "Data are contained in a .csv file" );

    verbose.NextLine ();

    for ( int gofi0 = 0; gofi0 < 2; gofi0++ ) {

        const TStatGoF*     gof         = gofi0 == 0 ? &gof1 : &gof2;

        if ( gofi0 )
            verbose.NextLine ();

        verbose.Put ( "Group #        :", gofi0 + 1 );

        if ( isnfiles ) {
            verbose.Put ( "From Time Frame:", (int) gof->TimeMin );
            verbose.Put ( "To   Time Frame:", (int) gof->TimeMax );
            verbose.Put ( "Using value(s) :", StatTimeString[ gof->StatTime ] );
            }

        verbose.Put ( "Number of data / subjects:", IsPaired ( processing ) ? numsamples[ 2 ] : numsamples[ gofi0 ] );

        for ( int filei = 0; filei < gof->NumFiles (); filei++ ) {

            if ( IsPaired ( processing ) && filei >= numsamples[ 2 ] )
                break;

            verbose.Put ( filei ? "" : isnfiles ? "Files          :" : "File           :", (*gof)[ filei ] );
            }

        if ( iscsvfile ) {
            verbose.Put ( "Group relative index within file:", gof->GroupIndex + 1 );
            verbose.Put ( "Group name:", gof->GroupName );
            }
        }
    }


    verbose.NextTopic ( "Output Files:" );
    {
    verbose.Put ( "Verbose file (this):", VerbFile );

    if ( exporttfile )
        verbose.Put ( "t file:", GroupBaseFileName, "." InfixT ".", IsFreqLoop () ? FILEEXT_FREQ : ext );

    if ( exportdfile )
        verbose.Put ( "Average difference file:", GroupBaseFileName, "." InfixDelta ".", IsFreqLoop () ? FILEEXT_FREQ : ext );

    if ( exportpfile ) {
        StringCopy ( buff, GroupBaseFileName, ".", PValueName, ".",   IsFreqLoop () ? FILEEXT_FREQ : ext );
        verbose.Put ( "p file:",            buff );
        }

    if ( iscsvfile ) {
        if ( IsTTest         ( processing ) )   verbose.Put ( "t values transposed in a Data file:", TDataFile );
        if ( IsRandomization ( processing ) )   verbose.Put ( "Delta values transposed in a Data file:", DDataFile );
        verbose.Put ( "p values transposed in a Data file:", PDataFile );
        }

    verbose.NextLine ();
    verbose.Put ( "Saving intermediate results:", moreoutputs );


    if ( moreoutputs /*&& writenumtf > 1*/ && HasMoreOutputs ( processing ) ) {
        verbose.NextLine ();

        if      ( processing == PairedTTest ) {
            verbose.Put ( "Mean           of Delta:",   GroupBaseFileName, "." InfixDelta "." InfixMean ".", IsFreqLoop () ? FILEEXT_FREQ : moreext );
            verbose.Put ( "Standard Error of Delta:",   GroupBaseFileName, "." InfixDelta "." InfixSE   ".", IsFreqLoop () ? FILEEXT_FREQ : moreext );
            }
        else if ( processing == UnpairedTTest ) {
            verbose.Put ( "Delta of Mean:",             GroupBaseFileName, "." InfixDelta "." InfixMean ".", IsFreqLoop () ? FILEEXT_FREQ : moreext );
            verbose.Put ( "Joint Standard Error:",      GroupBaseFileName, "." InfixJoint "." InfixSE   ".", IsFreqLoop () ? FILEEXT_FREQ : moreext );
            }

        for ( int gofi0 = 0; gofi0 < 2; gofi0++ ) {
            verbose.NextLine ();
            verbose.Put ( "Group #:", gofi0 + 1 );

            sprintf ( buff, "%s." InfixGroup "%0d." InfixSubjects ".%s",    (char*) GroupBaseFileName, gofi0 + 1, IsFreqLoop () ? FILEEXT_FREQ : filetype == IDC_FILETYPE_RIS ? FILEEXT_RIS : FILEEXT_EEGSEF );
            verbose.Put ( "All subjects  :", buff );

            sprintf ( buff, "%s." InfixGroup "%0d." InfixMean ".%s",        (char*) GroupBaseFileName, gofi0 + 1, IsFreqLoop () ? FILEEXT_FREQ : moreext );
            verbose.Put ( "Mean          :", buff );

            sprintf ( buff, "%s." InfixGroup "%0d." InfixSE ".%s",          (char*) GroupBaseFileName, gofi0 + 1, IsFreqLoop () ? FILEEXT_FREQ : moreext );
            verbose.Put ( "Standard Error:", buff );
            }
        }


    if ( moreoutputs && IsTAnova ( processing ) ) {
        verbose.NextLine ();
        verbose.Put ( "Dissimilarity of Mean Maps:",    GroupBaseFileName, "." InfixDis        ".", IsFreqLoop () ? FILEEXT_FREQ : moreext );
        verbose.Put ( "Sampled Dissimilarities:",       GroupBaseFileName, "." InfixSampledDis ".", IsFreqLoop () ? FILEEXT_FREQ : moreext );
        }
    }


    verbose.NextTopic ( "Processing Summary:" );
    {
    (ofstream&) verbose << "Before the Statistics, data are processed according to this sequence (& depending on user's choice):\n\n";
    (ofstream&) verbose << "    * Reading data from file(s)\n";
    (ofstream&) verbose << "    * Applying new reference\n";
    (ofstream&) verbose << "    * Normalizing data levels\n";
    (ofstream&) verbose << "    * Computing ROIs\n";
    }


    verbose.NextLine ( 2 );
    //verbose.NextBlock ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // clear results array
    for ( int gofi0 = 0; gofi0 < 3 ; gofi0++ )

        Results[ gofi0 ].ResetMemory ();

                                        // run all unpaired tests first
                                        // then the paired, after the parameters have been reprocessed
                                        // !we should get rid of any transfer and gof parameters, so to have explicit parameters all along - see TAnova!
    if      ( processing == UnpairedTTest )     
        
        Run_t_test              (   Data[ 0 ],          Data[ 1 ],
                                    gof1.StatTime,      gof2.StatTime, 
                                    outputnumtf[ 0 ],   outputnumtf[ 1 ],   outputnumtf[ 2 ],
                                    numsamples [ 0 ],   numsamples [ 1 ],   numsamples [ 2 ],
                                    checkmissingvalues, missingvalue,
                                    numvars,
                                    TestUnpaired,
                                    Results[ 0 ],       Results[ 1 ],       Results[ 2 ]
                                );

    else if ( processing == UnpairedRandomization )      

        Run_Randomization_test  (   Data[ 0 ],          Data[ 1 ],
                                    gof1.StatTime,      gof2.StatTime, 
                                    outputnumtf[ 0 ],   outputnumtf[ 1 ],   outputnumtf[ 2 ],
                                    numsamples [ 0 ],   numsamples [ 1 ],   numsamples [ 2 ],
                                    checkmissingvalues, missingvalue,
                                    numvars,
                                    TestUnpaired,
                                    numrand,
                                    Results[ 0 ],       Results[ 1 ],       Results[ 2 ]
                                );

    else if ( processing == UnpairedTAnova )

        Run_TAnova              (   Data[ 0 ],          Data[ 1 ],
                                    gof1.StatTime,      gof2.StatTime, 
                                    outputnumtf[ 2 ],
                                    numsamples [ 0 ],   numsamples [ 1 ],   numsamples [ 2 ],
                                    numvars,
                                    TestUnpaired,
                                    numrand,
                                    processingref,      polarity,
                                    Results[ 2 ],       moreoutputs ? &sampleddiss : 0
                                );

    else if ( processing == PairedTTest )       
        
        Run_t_test              (   Data[ 0 ],          Data[ 1 ],
                                    gof1.StatTime,      gof2.StatTime, 
                                    outputnumtf[ 0 ],   outputnumtf[ 1 ],   outputnumtf[ 2 ],
                                    numsamples [ 0 ],   numsamples [ 1 ],   numsamples [ 2 ],
                                    checkmissingvalues, missingvalue,
                                    numvars,
                                    TestPaired,
                                    Results[ 0 ],       Results[ 1 ],       Results[ 2 ]
                                );
        
    else if ( processing == PairedRandomization )        
        
        Run_Randomization_test  (   Data[ 0 ],          Data[ 1 ],
                                    gof1.StatTime,      gof2.StatTime, 
                                    outputnumtf[ 0 ],   outputnumtf[ 1 ],   outputnumtf[ 2 ],
                                    numsamples [ 0 ],   numsamples [ 1 ],   numsamples [ 2 ],
                                    checkmissingvalues, missingvalue,
                                    numvars,
                                    TestPaired,
                                    numrand,
                                    Results[ 0 ],       Results[ 1 ],       Results[ 2 ]
                                );

    else if ( processing == PairedTAnova )

        Run_TAnova              (   Data[ 0 ],          Data[ 1 ],
                                    gof1.StatTime,      gof2.StatTime, 
                                    outputnumtf[ 2 ],
                                    numsamples [ 0 ],   numsamples [ 1 ],   numsamples [ 2 ],
                                    numvars,
                                    TestPaired,
                                    numrand,
                                    processingref,      polarity,
                                    Results[ 2 ],       moreoutputs ? &sampleddiss : 0
                                );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Correct p-values
    Correct_p_values    (   Results[ 2 ],   numvars,    inputnumtf[ 2 ], 
                            multipletestscorrection, 
                            bonferronicorrectionvalue, 
                            fdrcorrectionvalue 
                        );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scan min duration of p
    if ( clippingpduration )

        CheckDuration_p_values  (   Results[ 2 ], numvars, inputnumtf[ 2 ], 
                                    true,                    // force thresholding p-values
                                    pvaluesthreshold,
                                    significantduration 
                                );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // export t / delta / p
    TExportTracks*      expt        = exporttfile ? new TExportTracks : 0;
    TExportTracks*      expd        = exportdfile ? new TExportTracks : 0;
    TExportTracks*      expp        = exportpfile ? new TExportTracks : 0;


    int                 writenumvars    = IsTAnova ( processing ) ? 1 : numvars;
    double              t;
    double              avg;
    double              p;


    if ( expt ) {

        StringCopy ( expt->Filename, TFile );

        expt->NumTracks             = writenumvars;
        expt->NumTime               = writenumtf;
        expt->SamplingFrequency     = samplfreq;
        expt->MaxValue              = 2 * maxvalue;

        if      ( IsTAnova ( processing ) ) expt->ElectrodesNames   = TAnovaName;
        else if ( outputvarnames )          expt->ElectrodesNames   = VarNames;

        expt->Begin ();
        }


    if ( expd ) {

        StringCopy ( expd->Filename, DFile );

        expd->NumTracks             = writenumvars;
        expd->NumTime               = writenumtf;
        expd->SamplingFrequency     = samplfreq;
        expd->MaxValue              = 2 * maxvalue;

        if      ( IsTAnova ( processing ) ) expd->ElectrodesNames   = TAnovaName;
        else if ( outputvarnames )          expd->ElectrodesNames   = VarNames;

        expd->Begin ();
        }


    if ( expp ) {

        StringCopy ( expp->Filename, PFile );

        expp->NumTracks             = writenumvars;
        expp->NumTime               = writenumtf;
        expp->SamplingFrequency     = samplfreq;
        expp->MaxValue              = 1.0;

        if      ( IsTAnova ( processing ) ) expp->ElectrodesNames   = TAnovaName;
        else if ( outputvarnames )          expp->ElectrodesNames   = VarNames;

        expp->Begin ();
        }


    if ( expt && expt->IsFileTextual () )   (ofstream&) (*expt)    << StreamFormatLeft;
    if ( expd && expd->IsFileTextual () )   (ofstream&) (*expd)    << StreamFormatLeft;
    if ( expp && expp->IsFileTextual () )   (ofstream&) (*expp)    << StreamFormatLeft;


    for ( int tf0 = 0; tf0 < writenumtf;   tf0++ )
    for ( int v   = 0; v   < writenumvars; v++   ) {

        t       =                   Results[ 2 ] ( tf0, v, Test_t_value  );
        avg     =                   Results[ 2 ] ( tf0, v, TestMean      );
        p       = Format_p_value (  Results[ 2 ] ( tf0, v, Test_p_value  ), outputp, thresholdingpvalues, pvaluesthreshold );


        if ( expt )     expt->Write ( t   );
        if ( expd )     expd->Write ( avg );
        if ( expp )     expp->Write ( p   );
        } // for variable, tf


    if ( expt )     delete expt;
    if ( expd )     delete expd;
    if ( expp )     delete expp;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // output transposed (put within verbose)?
    if ( writenumtf == 1 && csvoutnummaps > 0 ) {

        TExportTracks*      exptdata        = iscsvfile && IsTTest         ( processing )   ? new TExportTracks : 0;
        TExportTracks*      expddata        = iscsvfile && IsRandomization ( processing )   ? new TExportTracks : 0;
        TExportTracks*      exppdata        = iscsvfile                                     ? new TExportTracks : 0;
        int                 numvars         = writenumvars / NonNull ( csvoutnummaps ); // number of output variables (without maps and groups) - All variables for Markov

                                        // init error.data files
        if ( exppdata ) {

            StringCopy ( exppdata->Filename, PDataFile );

            exppdata->NumFiles          = 1;
                                        // hopefully, we don't have to change that for Markov case
            exppdata->NumTime           = csvmaxmap; // csvoutnummaps;  // output for all input maps, including the non-selected ones, so the templates start from 1 on display
            exppdata->NumTracks         = numvars;

//          DBGV5 ( csvoutnummaps, csvmaxmap, writenumvars, exppdata->NumTracks, csvnumvarspergroup, "csvoutnummaps, csvmaxmap, writenumvars, exppdata->NumTracks, csvnumvarspergroup" );

                                        // variable names for .data format
            TStringGrep         grepfit     ( "^(" FitMapNameLong     "|" FitMapNameShort     ")[0-9]+_", GrepOptionDefault );  // we want to skip this part
            TStringGrep         grepmarkov  ( "^(" FitFromMapNameLong "|" FitFromMapNameShort ")[0-9]+",  GrepOptionDefault );  // we want to keep this part
            TStrings            matched;


            for ( int v = 0; v < exppdata->NumTracks; v++ ) {
                                        // retrieve LAST part as variable names
                if      ( IsCsvStatFittingCartool ( csvtype )
                       && grepfit.Matched ( VarNames[ elsel.GetValue ( v ) ], &matched ) )

                        exppdata->VariableNames.Add ( VarNames[ elsel.GetValue ( v ) ] + StringLength ( matched ( 0 ) ) );
                                        // retrieve FIRST part as variable names
                else if ( IsCsvStatMarkov ( csvtype )
                       && grepmarkov.Matched ( VarNames[ elsel.GetValue ( v * csvmaxmap ) ], &matched ) )

                        exppdata->VariableNames.Add ( matched ( 0 ) );

                } // VariableNames

                                        // same with t data
            if ( exptdata ) {
                *exptdata   = *exppdata;
                StringCopy ( exptdata->Filename, TDataFile );
                }

            if ( expddata ) {
                *expddata   = *exppdata;
                StringCopy ( expddata->Filename, DDataFile );
                }

            }


        verbose.NextBlock ();
        verbose.NextLine ();

        (ofstream&) verbose << StreamFormatFixed << StreamFormatLeft;


        (ofstream&) verbose                                      << setw ( 30 ) << "Variable(s)";
        (ofstream&) verbose                                      << setw ( 20 ) << PValueName;

        if ( IsTTest ( processing ) )
            (ofstream&) verbose                                  << setw ( 20 ) << "t";

        if ( IsRandomization ( processing ) )
            (ofstream&) verbose                                  << setw ( 20 ) << "Delta";

        if ( moreoutputs && HasMoreOutputs ( processing ) ) {
            (ofstream&) verbose                                  << setw ( 20 ) << "# of Data Used";

            if ( IsTTest ( processing ) )
                (ofstream&) verbose                              << setw ( 25 ) << ( "Degrees of Freedom" );

            if      ( processing == PairedTTest ) {
                (ofstream&) verbose                              << setw ( 20 ) << InfixMean " of " InfixDelta;
                (ofstream&) verbose                              << setw ( 20 ) << InfixSE   " of " InfixDelta;
                }
            else if ( processing == UnpairedTTest ) {
                (ofstream&) verbose                              << setw ( 20 ) << InfixDelta " of " InfixMean;
                (ofstream&) verbose                              << setw ( 20 ) << InfixJoint " " InfixSE;
                }

            (ofstream&) verbose                                  << setw ( 20 ) << "Group1 " InfixMean;
            (ofstream&) verbose                                  << setw ( 20 ) << "Group1 " InfixSE;

            (ofstream&) verbose                                  << setw ( 20 ) << "Group2 " InfixMean;
            (ofstream&) verbose                                  << setw ( 20 ) << "Group2 " InfixSE;
            } // moreoutputs


        (ofstream&) verbose << "\n";
        verbose.NextLine ();


                                        // loop through all maps x variables
//      for ( int v = 0; v < writenumvars; v++ ) {
                                        // loop through all input maps (!selection is 1 based!)
        for ( int mapi = 1, v = -1; mapi <= csvmaxmap; mapi++ )
        for ( int vari = 0; vari < numvars; vari++ ) {

                                        // not selected by user?
            if ( ! csvoutmapsel[ mapi ] ) {
                                        // fill anyway with null data
                if ( exptdata )     exptdata->Write ( (float) 0 );
                if ( expddata )     expddata->Write ( (float) 0 );
                if ( exppdata )     exppdata->Write ( (float) 0 );
                                        // note: nothing outputted for the verbose, the variables name are indicative enough, f.ex. if first var. is Map3_GEV
                continue;
                } // ! csvoutmapsel

                                        // here data exist, increment to next actual data
            if      ( IsCsvStatFittingCartool ( csvtype ) )    v++;
            else if ( IsCsvStatMarkov         ( csvtype ) )    v   = vari * csvmaxmap + mapi - 1;  // we need to transpose the data i<->j


            t       =                   Results[ 2 ] ( 0, v, Test_t_value    );
            avg     =                   Results[ 2 ] ( 0, v, TestMean        );
            p       = Format_p_value (  Results[ 2 ] ( 0, v, Test_p_value    ), outputp, thresholdingpvalues, pvaluesthreshold );


            if ( exptdata )     exptdata->Write ( (float) t   );
            if ( expddata )     expddata->Write ( (float) avg );
            if ( exppdata )     exppdata->Write ( (float) p   );


            (ofstream&) verbose                                  << setw ( 30 ) << VarNames[ elsel.GetValue ( v ) ];
            (ofstream&) verbose << setprecision ( p ? 9 : 1 )    << setw ( 20 ) << p;

            if ( IsTTest ( processing ) )
                (ofstream&) verbose << setprecision ( 9 )        << setw ( 20 ) << t;

            if ( IsRandomization ( processing ) )
                (ofstream&) verbose << setprecision ( 9 )        << setw ( 20 ) << avg;


            if ( moreoutputs && HasMoreOutputs ( processing ) ) {

                (ofstream&) verbose                              << setw ( 20 ) << (int) Results[ 2 ] ( 0, v, TestNumSamples );

                if ( IsTTest ( processing ) )
                    (ofstream&) verbose                          << setw ( 25 ) << (int) Results[ 2 ] ( 0, v, TestDoF );

                if      ( processing == PairedTTest ) {
                    (ofstream&) verbose << setprecision ( 9 )    << setw ( 20 ) << (double) Results[ 2 ] ( 0, v, TestMean );
                    (ofstream&) verbose << setprecision ( 9 )    << setw ( 20 ) << (double) Results[ 2 ] ( 0, v, TestStandardError );
                    }
                else if ( processing == UnpairedTTest ) {
                    (ofstream&) verbose << setprecision ( 9 )    << setw ( 20 ) << (double) Results[ 2 ] ( 0, v, TestMean );
                    (ofstream&) verbose << setprecision ( 9 )    << setw ( 20 ) << (double) Results[ 2 ] ( 0, v, TestStandardError );
                    }

                (ofstream&) verbose << setprecision ( 9 )        << setw ( 20 ) << (double) Results[ 0 ] ( 0, v, TestMean );
                (ofstream&) verbose << setprecision ( 9 )        << setw ( 20 ) << (double) Results[ 0 ] ( 0, v, TestStandardError );

                (ofstream&) verbose << setprecision ( 9 )        << setw ( 20 ) << (double) Results[ 1 ] ( 0, v, TestMean );
                (ofstream&) verbose << setprecision ( 9 )        << setw ( 20 ) << (double) Results[ 1 ] ( 0, v, TestStandardError );
                } // moreoutputs


            (ofstream&) verbose << "\n";
            } // for variable


        if ( exptdata )     delete exptdata;
        if ( expddata )     delete expddata;
        if ( exppdata )     delete exppdata;
        } // if writenumtf == 1


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // more outputs in more files
    TExportTracks*      expmore = new TExportTracks;


    expmore->NumTracks          = writenumvars;
    expmore->NumAuxTracks       = 0;
    expmore->NumTime            = writenumtf;
    expmore->SamplingFrequency  = samplfreq;
    expmore->MaxValue           = 2 * maxvalue;

    if ( outputvarnames )
        expmore->ElectrodesNames= VarNames;



    if ( moreoutputs && HasMoreOutputs ( processing ) /*&& writenumtf > 1*/ ) {

        TSuperGauge         Gauge ( "Saving to disk", 4 );

                                        // joint mean, SD, SE
        Gauge.Next ();

        if ( IsTTest ( processing ) ) {
            sprintf ( expmore->Filename,    "%s." InfixDelta "." InfixMean ".%s",       (char*) BaseFileName, moreext );
            for ( int tf0 = 0; tf0 < writenumtf; tf0++ )
                for ( int v = 0; v < writenumvars; v++ )
                    expmore->Write ( (double) Results[ 2 ] ( tf0, v, TestMean ) );
            }


        Gauge.Next ();

        if ( IsTTest ( processing ) ) {
            sprintf ( expmore->Filename,    "%s." InfixDelta "." InfixSE ".%s",         (char*) BaseFileName, moreext );
            for ( int tf0 = 0; tf0 < writenumtf; tf0++ )
                for ( int v = 0; v < writenumvars; v++ )
                    expmore->Write ( (double) Results[ 2 ] ( tf0, v, TestStandardError ) );
            }


                                        // then by group: mean, SD, SE
        Gauge.Next ();

        for ( int gofi0 = 0; gofi0 < 2; gofi0++ ) {

            sprintf ( expmore->Filename,    "%s." InfixGroup "%0d." InfixMean ".%s",    (char*) BaseFileName, gofi0 + 1, moreext );
            for ( int tf0 = 0; tf0 < writenumtf; tf0++ )
                for ( int v = 0; v < writenumvars; v++ )
                    expmore->Write ( (double) Results[ gofi0 ] ( tf0, v, TestMean ) );

            sprintf ( expmore->Filename,    "%s." InfixGroup "%0d." InfixSE ".%s",      (char*) BaseFileName, gofi0 + 1, moreext );
            for ( int tf0 = 0; tf0 < writenumtf; tf0++ )
                for ( int v = 0; v < writenumvars; v++ )                // we kept the variance per group
                    expmore->Write ( (double) Results[ gofi0 ] ( tf0, v, TestStandardError ) );
            }


                                        // write all subjects in one file per group
//      expmore->AuxTracks          = 0;

        Gauge.Next ();

        for ( int gofi0 = 0; gofi0 < 2; gofi0++ ) {

            const TStatGoF*     gof         = gofi0 == 0 ? &gof1 : &gof2;


            if ( writenumvars == 1 ) {  // write all subjects as "electrode"

                int     numfiles            = IsPaired ( processing ) ? numsamples[ 2 ] : gof->NumFiles ();
                expmore->NumTracks          = numfiles;

                                        // provide with all the file names
                TStrings        FileNames;

                for ( int s = 0; s < numfiles; s++ ) {
                    FileNames.Add   ( ToFileName ( (*gof)[ s ] ) );
                    RemoveExtension ( FileNames[ s ] );
                    }
                                        // the names will be compacted if needed by the export itself
                expmore->ElectrodesNames    = FileNames;


                sprintf ( expmore->Filename,    "%s." InfixGroup "%0d." InfixSubjects ".%s", (char*) BaseFileName, gofi0 + 1, filetype == IDC_FILETYPE_RIS ? FILEEXT_RIS : FILEEXT_EEGSEF );
                expmore->Type               = filetype == IDC_FILETYPE_RIS ? ExportTracksRis : ExportTracksSef /*ExportTracksDefault*/;

                                        // write each files
                for ( int tf0 = 0; tf0 < writenumtf; tf0++ )
                    for ( int s = 0; s < numfiles; s++ )
                        expmore->Write ( (float) Data[ gofi0 ] ( tf0, 0, s ) );

                }

            else {                      // write each subject one "after" the other

                expmore->SelTracks          = elsel;
                int     numfiles            = IsPaired ( processing ) ? numsamples[ 2 ] : gof->NumFiles ();
                expmore->NumTime            = writenumtf * numfiles;


                sprintf ( expmore->Filename,    "%s." InfixGroup "%0d." InfixSubjects ".%s", (char*) BaseFileName, gofi0 + 1, filetype == IDC_FILETYPE_RIS ? FILEEXT_RIS : FILEEXT_EEGSEF );
                expmore->Type               = filetype == IDC_FILETYPE_RIS ? ExportTracksRis : ExportTracksSef /*ExportTracksDefault*/;

                                        // write each files
                for ( int s = 0; s < numfiles; s++ )
                    for ( int tf0 = 0; tf0 < writenumtf; tf0++ )
                        for ( int v = 0; v < writenumvars; v++ )
                            expmore->Write ( (float) Data[ gofi0 ] ( tf0, v, s ) );


                StringCopy ( buff, expmore->Filename, ".", FILEEXT_MRK );

                ofstream ofmrk ( TFileName ( buff, TFilenameExtendedPath ) );

                WriteMarkerHeader ( ofmrk );

                for ( int s = 0; s < numfiles; s++ ) {

                    WriteMarker (   ofmrk, 
                                    s * writenumtf,
                                    StringCopy  ( buff, "File", IntegerToString ( s + 1 ) ) 
                                );

                    WriteMarker (   ofmrk, 
                                    s * writenumtf,
                                    GetFilename ( StringCopy ( buff, (*gof)[ s ] ) ) 
                                );
                    } // for file

                } // writenumvars > 1

            } // for gof

        } // more outputs



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( moreoutputs && IsTAnova ( processing ) ) {
                                        // dissimilarity of mean
        sprintf ( expmore->Filename,    "%s." InfixDis ".%s",   (char*) BaseFileName, moreext );

        for ( int tf0 = 0; tf0 < writenumtf; tf0++ )
            for ( int v = 0; v < writenumvars; v++ )
                expmore->Write ( (double) Results[ 2 ] ( tf0, v, Test_Dissimilarity ) );


        if ( sampleddiss.IsAllocated () ) {
            TFileName               filesdis;

            StringCopy      ( filesdis,     BaseFileName, ".", InfixSampledDis );
            AddExtension    ( filesdis,     moreext );

            sampleddiss.WriteFile ( filesdis );

                                        // output a normalized diss?
            for ( int tf0 = 0; tf0 < sampleddiss.GetDim1 (); tf0++ ) {

                for ( int s   = 1; s   < sampleddiss.GetDim2 (); s++   )
                    sampleddiss ( tf0, s )     /= NonNull ( sampleddiss ( tf0, 0 ) );

                sampleddiss ( tf0, 0 )  = 1;
                }

            PostfixFilename ( filesdis,     "Ratio" );

            sampleddiss.WriteFile ( filesdis );
            }

        } // more tanova outputs



    delete  expmore;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // last freq? do the big merge(s)
    if ( IsFreqLoop () && IsLastFreq () ) {

        TGoF            filenames;
        TFileName       dir;
        TFileName       templ;

                                        // get directory
        StringCopy ( dir, BaseFileName );
        RemoveFilename ( dir );


                                        // merge p
        sprintf ( (char*) templ, "%s\\*." InfixP        ".%s", (char*) dir, ext );
        MergeTracksToFreqFiles ( templ, FreqType );

                                        // merge 1-p
        sprintf ( (char*) templ, "%s\\*." Infix1MinusP  ".%s", (char*) dir, ext );
        MergeTracksToFreqFiles ( templ, FreqType );

                                        // merge logp
        sprintf ( (char*) templ, "%s\\*." InfixLogP     ".%s", (char*) dir, ext );
        MergeTracksToFreqFiles ( templ, FreqType );

                                        // merge t
        sprintf ( (char*) templ, "%s\\*." InfixT        ".%s", (char*) dir, ext );
        MergeTracksToFreqFiles ( templ, FreqType );

                                        // merge delta mean
        sprintf ( (char*) templ, "%s\\*." InfixDelta "." InfixMean  ".%s", (char*) dir, moreext );
        MergeTracksToFreqFiles ( templ, FreqType );

                                        // merge delta SE
        sprintf ( (char*) templ, "%s\\*." InfixDelta "." InfixSE    ".%s", (char*) dir, moreext );
        MergeTracksToFreqFiles ( templ, FreqType );

                                        // merge joint SE
        sprintf ( (char*) templ, "%s\\*." InfixJoint "." InfixSE    ".%s", (char*) dir, moreext );
        MergeTracksToFreqFiles ( templ, FreqType );

                                        // merge DIS
        sprintf ( (char*) templ, "%s\\*." InfixDis ".%s", (char*) dir, moreext );
        MergeTracksToFreqFiles ( templ, FreqType );


                                        // now, per group
        for ( int gofi0 = 0; gofi0 < 2; gofi0++ ) {

            sprintf ( (char*) templ, "%s\\*." InfixGroup "%0d." InfixSubjects   ".%s", (char*) dir, gofi0 + 1, filetype == IDC_FILETYPE_RIS ? FILEEXT_RIS : FILEEXT_EEGSEF );
            MergeTracksToFreqFiles ( templ, FreqType );


            sprintf ( (char*) templ, "%s\\*." InfixGroup "%0d." InfixMean       ".%s", (char*) dir, gofi0 + 1, moreext );
            MergeTracksToFreqFiles ( templ, FreqType );


            sprintf ( (char*) templ, "%s\\*." InfixGroup "%0d." InfixSE         ".%s", (char*) dir, gofi0 + 1, moreext );
            MergeTracksToFreqFiles ( templ, FreqType );
            }
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // complimentary opening the file for the user
    if ( openauto ) {

        if ( writenumtf > 1 || IsTAnova ( processing ) ) {

            if ( IsFreqLoop () ) {
                                        // final file names
                StringCopy ( TFile,             GroupBaseFileName,  "."  InfixT      ".",   FILEEXT_FREQ );
                StringCopy ( PFile,             GroupBaseFileName,  ".", PValueName, ".",   FILEEXT_FREQ );
                }

            if ( exporttfile )  TFile.Open ();
            if ( exportdfile )  DFile.Open ();
            if ( exportpfile )  PFile.Open ();
            } // writenumtf > 1

        if ( iscsvfile ) {
            TDataFile.Open ();
            DDataFile.Open ();
            PDataFile.Open ();
            }
        }

    } // for numprocessings



XYZDoc.Close ();


if ( rois )
    delete  rois;


delete[]    Data;
delete[]    Results;

UpdateApplication;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
