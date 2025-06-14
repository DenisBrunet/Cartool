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

#include    <stdio.h>

#include    "Files.Conversions.h"
#include    "Files.Extensions.h"

#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"
#include    "Strings.Grep.h"
#include    "Files.Utils.h"
#include    "TArray2.h"

#include    "TExportTracks.h"
#include    "TMaps.h"

#include    "TCartoolMdiClient.h"

#include    "TBaseDoc.h"
#include    "TFreqCartoolDoc.h"         // TFreqFrequencyName
#include    "TRisDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Warning: this code is highly dependent of HOW the verbose is written
                                        // f.ex. spaces and new lines, items in a lines, etc...
void    FileConversionVrbToTva ( const char* file )
{
if ( StringIsEmpty ( file ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ifstream                is  ( TFileName ( file, TFilenameExtendedPath ) );
ofstream                os;
TFileName               filetva;
char                    code[ KiloByte ];
constexpr int           RWSIZE      = 2 * KiloByte;
char                    rw[ RWSIZE ];

TStringGrep             grepfile ( "File *: *(.+)$",     GrepOptionDefault );   // accounting for spaces in file names
TStringGrep             grepline ( "trigger (.+)( :.+)", GrepOptionDefault );   // accounting for spaces in trigger names


do {
                                    // skip until file name
    do      is.getline ( rw, RWSIZE );
    while ( ! ( StringContains ( (const char*) rw, "Processing File" ) || is.eof () /*|| StringContains ( (const char*) rw, "USER BREAK" )*/ ) );

    if ( is.eof () )               // no more inside
        break;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    do      is.getline ( rw, RWSIZE );
    while ( ! ( grepfile.Matched ( rw ) || is.eof () ) );


    if ( is.eof () )               // no more inside
        break;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // has been canceled, removing
    if ( StringContains ( rw, "USER BREAK" ) ) {
        remove ( name );            // name already exist here
        ShowMessage ( "Found \"user break\", file conversion aborted.", name, ShowMessageWarning );
        continue;                   // continue the EPV loop
        }
*/

                                    // convert EEG file name to TVA file name
    StringCopy ( filetva, grepfile.GetMatchStart ( 1 ) );

    filetva.ReplaceExtension ( FILEEXT_VRB "." FILEEXT_TVA );

    if ( ! CanOpenFile ( filetva, CanOpenFileWriteAndAsk ) )
        continue;

    //filetva.Show ( "filetva" );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    os.open ( TFileName ( filetva, TFilenameExtendedPath ) );

                                    // write my header
    os << TVATXT_MAGICNUMBER1 << fastendl;

                                    // skip until triggers list
    do is.getline ( rw, RWSIZE );
    while ( ! StringContains ( (const char*) rw, "Epoch" ) );


    do {

        if ( StringContains ( (const char*) rw, "Restarting" ) ) {
                                    // re-create the tva from scratch
            os.close ();

            os.open ( TFileName ( filetva, TFilenameExtendedPath ) );

            os << TVATXT_MAGICNUMBER1 << fastendl;

            do is.getline ( rw, RWSIZE );
            while ( ! StringContains ( (const char*) rw, "Epoch" ) );
            }


        if ( ! StringIsSpace ( rw ) ) {

            if ( grepline.Matched ( rw ) == 3 )

                StringCopy ( code, grepline.GetMatchStart ( 1 ), grepline.GetMatchStart ( 2 ) - grepline.GetMatchStart ( 1 ) );
            else 
                StringCopy ( code, "?" );

            //DBGM ( code, "grep code" );


            os  << ( (    StringContains ( (const char*) rw, "REJECT" ) 
                       || StringContains ( (const char*) rw, "WRONG"  ) ) ? "0" 
                                                                          : "1" )
                << Tab "0"
                << Tab << code 
                << fastendl;
            }

        is.getline ( rw, RWSIZE );

        } while (  StringIsSpace ( rw ) 
                || StringContains ( (const char*) rw, "Epoch" ) 
                || StringContains ( (const char*) rw, "Restarting" ) );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                    // has been canceled, removing
    if ( StringContains ( (const char*) rw, "USER BREAK" ) ) {

        os.close ();

        remove ( filetva );         // filetva already exist here

        ShowMessage ( "Found \"user break\", file conversion aborted.", filetva, ShowMessageWarning );

        continue;                   // continue the EPV loop
        }


    os.close ();

    } while ( ! is.eof () );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
                                        // Not finished due to .mat content being zipped
void    SplitMatFiles ( const char* file )
{
if ( StringIsEmpty ( file ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TMatlabMat          mat;
TMemory             mem;

mat.Open ( file );

do {
    if ( ! mat.GetNextMarker ( mem ) )     break;

                                    // in case of compressed, just dump to file
    if ( mem.IsMemoryAllocated () ) {

        TFileName       filezip ( file );
                                    // !this doesn't "work", as it lacks header and stuff, this is more like a compressed stream!
        ReplaceExtension    ( filezip, "zip" );

        CheckNoOverwrite    ( filezip );

        ofstream        ofs ( filezip );

        ofs.write ( (char*) mem.GetMemoryAddress (), mem.GetMemorySize () );

        ofs.close ();
        }


    } while ( mat.IsOpen () );
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // convert the template to a TGoF, then call the main function MergeTracksToFreqFiles
void    MergeTracksToFreqFiles ( const char* filestemplate, FrequencyAnalysisType freqtype, char *returnfreqfile, bool showgauge )
{
if ( StringIsEmpty ( filestemplate ) )
    return;


TGoF                gof;

                                        // get the files matching the template
if ( ! gof.FindFiles ( filestemplate ) )
    return;

                                        // do the merge
MergeTracksToFreqFiles ( gof, freqtype, returnfreqfile, showgauge );

                                        // clean-up
DeleteFiles ( filestemplate );

                                        // check for remaining .mrk files
TFileName           filestemplatemrk;

StringCopy   ( filestemplatemrk, filestemplate );
AddExtension ( filestemplatemrk, FILEEXT_MRK );

DeleteFiles  ( filestemplatemrk );
}


//----------------------------------------------------------------------------

void    MergeTracksToFreqFiles ( const TGoF& gof, FrequencyAnalysisType freqtype, char *returnfreqfile, bool showgauge )
{
if ( gof.IsEmpty () )
    return;


if ( returnfreqfile )
    ClearString ( returnfreqfile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

constexpr char*     BatchTracks2Freq            = "Merging Frequencies";

                                        // Check file extensions
if ( ! gof.AllExtensionsAre ( AllEegRisFilesExt ) ) {
    ShowMessage (   "Files type not allowed for this operation!" NewLine 
                    "Check again your input files...", 
                    BatchTracks2Freq, ShowMessageWarning );
    return;
    }

                                        // Check whole group dimensions
TracksCompatibleClass   tc;


                                        // Checks for the most important dimensions (note that numsolpoints is NumTracks for .ris)
gof.AllTracksAreCompatible ( tc );

//DBGV6 ( NumTracks, numauxtracks, numsolpoints, numtf, numfreq, samplingfrequency, "NumTracks, numauxtracks, numsolpoints, numtf, numfreq, samplingfrequency" );


if      ( tc.NumTracks == CompatibilityNotConsistent ) {
    ShowMessage (   "Files don't seem to have the same number of electrodes/tracks!" NewLine 
                    "Check again your input files...", 
                    BatchTracks2Freq, ShowMessageWarning );
    return;
    }
else if ( tc.NumTracks == CompatibilityIrrelevant ) {
    ShowMessage (   "Files don't seem to have any electrodes/tracks at all!" NewLine 
                    "Check again your input files...", 
                    BatchTracks2Freq, ShowMessageWarning );
    return;
    }


//if      ( tc.NumAuxTracks > 0 ) {
//    ShowMessage ( "Files shouldn't have auxiliary channels!" NewLine 
//                  "Check again your input files...", 
//                  BatchTracks2Freq, ShowMessageWarning );
//    return;
//    }
                                        // this test is quite weak, as ReadFromHeader does a lousy job at retrieving the aux tracks (it needs the strings, so opening the whole file)
if      ( tc.NumAuxTracks == CompatibilityNotConsistent ) {
    ShowMessage (   "Files don't seem to have the same number of auxiliary tracks!" NewLine 
                    "Check again your input files...", 
                    BatchTracks2Freq, ShowMessageWarning );
    return;
    }


if      ( tc.NumTF == CompatibilityNotConsistent ) {
    ShowMessage (   "Files don't seem to have the same number of samples/time range!" NewLine 
                    "Check again your input files...", 
                    BatchTracks2Freq, ShowMessageWarning );
    return;
    }
else if ( tc.NumTF == CompatibilityIrrelevant ) {
    ShowMessage (   "Files don't seem to have any samples or time at all!" NewLine 
                    "Check again your input files...", 
                    BatchTracks2Freq, ShowMessageWarning );
    return;
    }


if      ( tc.SamplingFrequency == CompatibilityNotConsistent ) {
    ShowMessage (   "Files don't seem to have the same sampling frequencies!" NewLine 
                    "Check again your input files...", 
                    BatchTracks2Freq, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do some checking + retrieve some info
int                 numel           = tc.NumTracks;
int                 numfreqs        = (int) gof;

                                        // we can fill the gaps here
if ( freqtype == FreqUnknown && gof.AllExtensionsAre ( AllRisFilesExt ) )
    freqtype    = FreqESI;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           commondir;
TFileName           commonstart;
TFileName           commonend;


gof.GetCommonParts   (   commondir,  commonstart,    commonend,  0   );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // In case of very few frequencies, the common start might have retained the highest digits
                                        // Although not perfect, this helps a bit
while ( isdigit ( *LastChar ( commonstart ) ) )

    *LastChar ( commonstart )   = EOS;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Test if we have a case of .<special infix>.ext

TFileName           grepinfixstring;


ListToGrep      ( grepinfixstring, SpecialFilesInfix );
                                        // usually following the "XXHz."
StringPrepend   ( grepinfixstring, "Hz\\." );
                                        // and maybe followed by a "."
StringAppend    ( grepinfixstring, "\\.?" );


TStringGrep         grepinfix ( grepinfixstring, GrepOptionDefault );

TStrings            infix;

grepinfix.Matched ( commonend, &infix );
                                        // We don't need this part, it was just for retrieval...
StringReplace       ( infix ( 0 ), "Hz.", "" );

//infix.Show ( "infix" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // if not a special infix, try to erode it
//if ( infix ( 0 ).IsEmpty () ) {
                                        // get rid of common trailing digits, like for "100.00 Hz" and "110.00 Hz", to reach the (potential) final " Hz"
    for ( int i = 0; i < StringLength ( commonend ); i++ ) {
                                        // not a digit and not a '.'?
        if ( ! ( isdigit ( *commonend ) || *commonend == '.' ) )
            break;                      // we look only for zeroes

        StringCopy ( commonend, commonend + 1 );
        i--;
        }
//  }

                                        // skip the "Hz"
if      ( StringStartsWith ( commonend, " Hz." ) )  StringCopy ( commonend, commonend + 4 );
else if ( StringStartsWith ( commonend, "Hz."  ) )  StringCopy ( commonend, commonend + 3 );
else if ( StringStartsWith ( commonend, " Hz"  ) )  StringCopy ( commonend, commonend + 3 );
else if ( StringStartsWith ( commonend, "Hz"   ) )  StringCopy ( commonend, commonend + 2 );

//DBGM ( (const char*) commonend, "common end / final" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TStringGrep         grepHz ( "\\.[0-9]+\\.?[0-9]*Hz\\.", GrepOptionDefaultFiles );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // retrieve frequency names
                                        // we need to read all files, extract frequency info, and sort it
TFreqFrequencyName  freqname;
TArray2<double>     freqindex ( numfreqs, 2 );
TStrings            freqnames;
TFileName           OutFileName;
TStrings            matched;


freqindex.ResetMemory ();
OutFileName.Clear ();


                                        // try to extract a (frequency) value from each file
for ( int freqi = 0; freqi < numfreqs; freqi++ ) {

    grepHz.Matched ( gof[ freqi ], &matched );

    StringClip ( matched ( 0 ), 1, StringLength ( matched ( 0 ) ) - 2 );

                                        // store the converted value for later sorting
    freqindex ( freqi , 0 ) = StringToDouble ( matched ( 0 ) );    // could add the second value, after the dot, in case two freqs begins with the same value
    freqindex ( freqi , 1 ) = freqi;

                                        // store the frequency name
    StringCopy      ( freqname.FrequencyName, matched ( 0 ), sizeof ( freqname ) - 1 );

    freqnames.Add   ( freqname.FrequencyName );

//    DBGV ( freqindex ( freqi , 0 ), freqname.FrequencyName );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // count failed conversions
int                 nzeroes     = 0;

for ( int freq = 0; freq < numfreqs; freq++ )
    nzeroes += freqindex ( freq , 0 ) == 0;

                                        // this could happen if files have nothing in common at their beginning
if ( StringIsEmpty ( commonstart ) )

    StringCopy  ( commonstart, DefaultFileName );

                                        // 0 or 1 zero is OK (0 Hz), more than 1 means the extraction went wrong
if ( nzeroes > 1 ) {

    freqnames.Reset ();

                                        // overwrite with stupidly ordered values
    for ( int freqi = 0; freqi < numfreqs; freqi++ ) {

        freqindex ( freqi , 0 ) = freqi + 1;
        freqindex ( freqi , 1 ) = freqi;

                                        // store pseudo frequency name
        sprintf ( freqname.FrequencyName, "Freq%0d", freqi + 1 );

        freqname.FrequencyName[ sizeof ( freqname.FrequencyName ) - 1 ] = 0;

        freqnames.Add ( freqname.FrequencyName );
//      DBGV ( freqindex ( freqi , 0 ), freqname.FrequencyName );
        }

                                        // main output file
    StringCopy ( OutFileName, commondir, "\\", commonstart );

    if ( ! StringEndsWith ( OutFileName, "." ) )    StringAppend ( OutFileName, "." );

    StringAppend ( OutFileName, "Merged" );

    if ( ! StringStartsWith ( commonend, "." ) )    StringAppend ( OutFileName, "." );

    StringAppend ( OutFileName, commonend );

    if ( StringIsNotEmpty ( infix ( 0 ) ) )
        StringAppend ( OutFileName, ".", infix ( 0 ) );

    ReplaceExtension ( OutFileName, FILEEXT_FREQ );
//    DBGM ( OutFileName, "OutFileName Not OK" );
    }

else {
                                        // main output file
    StringCopy ( OutFileName, commondir, "\\", commonstart );

    if ( ! StringEndsWith ( OutFileName, "." ) )    StringAppend ( OutFileName, "." );

    StringAppend ( OutFileName, commonend );

    if ( StringIsNotEmpty ( infix ( 0 ) ) )
        StringAppend ( OutFileName, ".", infix ( 0 ) );

    ReplaceExtension ( OutFileName, FILEEXT_FREQ );
//    DBGM ( OutFileName, "OutFileName OK" );
    }

                                        // if files were .epsd or .epse, re-introduce the extension, as to differentiate from the data themselves
if      ( StringEndsWith ( commonend, FILEEXT_EEGEPSD ) )   PostfixFilename ( OutFileName, "." /*FILEEXT_EEGEPSD*/ InfixSD );
else if ( StringEndsWith ( commonend, FILEEXT_EEGEPSE ) )   PostfixFilename ( OutFileName, "." /*FILEEXT_EEGEPSE*/ InfixSE );
else if ( StringEndsWith ( commonend, FILEEXT_RIS     ) )   PostfixFilename ( OutFileName, "." FILEEXT_RIS     );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here, all files are compatibles
OutFileName.CheckExtendedPath ();

                                        // don't overwrite an existing file, which might be the original we split from on the first hand!
//if ( CanOpenFile ( OutFileName ) ) {
//    RemoveExtension ( OutFileName );
//    StringAppend    ( OutFileName, "." "Merged", "." FILEEXT_FREQ );
//    }

CheckNoOverwrite    ( OutFileName );

                                        // check we can write that one
if ( ! CanOpenFile ( OutFileName, CanOpenFileWrite ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need some more details, open first file
TOpenDoc<TTracksDoc>    EegDoc;

EegDoc.Open ( gof.GetFirst (), OpenDocHidden );


TExportTracks       expfile;


StringCopy ( expfile.Filename, OutFileName );

expfile.SetAtomType ( AtomTypeScalar );

ClearVirtualMemory ( expfile.FreqTypeName, sizeof ( expfile.FreqTypeName ) /*MaxCharFreqType*/ );
StringCopy         ( expfile.FreqTypeName, FrequencyAnalysisNames[ freqtype ], MaxCharFreqType - 1 );

expfile.NumTracks           = numel;
expfile.NumFrequencies      = numfreqs;
expfile.NumTime             = tc.NumTF;
expfile.SamplingFrequency   = 0;
expfile.BlockFrequency      = EegDoc->GetSamplingFrequency ();
expfile.DateTime            = EegDoc->DateTime;

expfile.ElectrodesNames     = *EegDoc->GetElectrodesNames ();

expfile.FrequencyNames.Set ( numfreqs, sizeof ( TFreqFrequencyName ) );

                                        // finished with EEG file
EegDoc.Close ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // sort the freq values
freqindex.SortRows ( 0, Ascending );

                                        // now, can really write the sorted frequency names
for ( int freq = 0; freq < numfreqs; freq++ ) {

//    DBGV2 ( freqindex ( freq , 1 ) + 1, freqindex ( freq , 0 ), freqnames[ (int) freqindex ( freq , 1 ) ] );

    ClearVirtualMemory ( expfile.FrequencyNames[ freq ], sizeof ( TFreqFrequencyName ) );

    StringCopy ( expfile.FrequencyNames[ freq ], freqnames[ (int) freqindex ( freq , 1 ) ], sizeof ( TFreqFrequencyName ) - 1 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge     Gauge;

if ( showgauge ) {
    Gauge.Set       ( BatchTracks2Freq );
    Gauge.AddPart   ( 0, numfreqs );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Reading everything at once
TGoMaps         maps    ( &gof, AtomTypeScalar, ReferenceAsInFile );

TArray3<float>  results (   expfile.NumTime,                                                        // !already transposed for direct file output!
                            expfile.NumTracks, 
                            expfile.NumFrequencies 
                        * ( expfile.GetAtomType ( AtomTypeUseCurrent ) == AtomTypeComplex ? 2 : 1 ) // multiplex the real / imaginary parts manually
                        );

                                        // actually merge the data
for ( int freq = 0; freq < numfreqs; freq++ ) {

    if ( Gauge.IsAlive () )
        Gauge.Next ( 0 );

    for ( int tf = 0; tf < tc.NumTF; tf++ )
    for ( int e  = 0; e  < numel;    e++  )
                                        // copy all data into big buffer
        results ( tf, e, freq ) = maps[ freqindex ( freq , 1 ) ] ( tf, e );

    } // for file / freq

                                        // write all data in one shot
expfile.Write ( results, NotTransposed );


expfile.End ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // any marker file?
TFileName           filemrk;

for ( int i = 0; i < (int) gof; i++ ) {

    filemrk     = gof[ i ];
    filemrk.AddExtension ( FILEEXT_MRK );
                                        // copy the first one and stop - would be better to merge all marker files(?)
    if ( filemrk.CanOpenFile () ) {
        OutFileName.AddExtension    ( FILEEXT_MRK );
        CopyFileExtended            ( filemrk,     OutFileName );
        OutFileName.RemoveExtension ();
        break;
        }
    }


if ( returnfreqfile )
    StringCopy ( returnfreqfile, OutFileName );

//CartoolDocManager->OpenDoc ( OutFileName, dtOpenOptions );

if ( Gauge.IsAlive () )
    Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    SortFFTApproximation ( const char* file )
{
if ( StringIsEmpty ( file ) || ! CanOpenFile ( file, CanOpenFileRead ) )
    return;


TOpenDoc<TFreqDoc>  freqdoc ( file, OpenDocHidden );

if ( freqdoc.IsOpen () )
    freqdoc->SortFFTApproximation ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    RisToCloudVectors   (
                            const char*         risfile,
                            int                 tfmin,          int             tfmax,
                            const TSelection*   spselin,        const char*     splist,
                            bool                spontaneous,    bool            normalize,
                            TGoF&               outgof,
                            bool                showprogress
                            )

{
outgof.Reset ();

                                        // Parameters checking
if ( StringIsEmpty ( risfile ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TOpenDoc<TRisDoc>   risdoc ( risfile, OpenDocHidden );

                                        // this works only on vectorial RIS
if ( ! risdoc->IsVector ( AtomTypeUseOriginal ) ) {
    risdoc.Close ( CloseDocRestoreAsBefore );
    return;
    }


int                 numsp           = risdoc->GetNumElectrodes ();
int                 numtimeframes   = risdoc->GetNumTimeFrames ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // setting default time interval?
if ( tfmin == -1 )  tfmin           = 0;
if ( tfmax == -1 )  tfmax           = numtimeframes - 1;


Clipped ( tfmin, tfmax, 0, numtimeframes - 1 );

//int                 numtfsaved      = tfmax - tfmin + 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setting the SP selection
TSelection          spsel;

                                        // If selection is given and is of right size
if ( spselin )                          spsel   = *spselin;

else {                                  // Otherwise, create our own selection
                                        spsel   = TSelection ( numsp, OrderSorted );
                                        // Caller might have specified an optional list..
    if ( StringIsNotEmpty ( splist ) )  spsel.Set ( splist, risdoc->GetElectrodesNames () );
                                        // ..or not
    else                                spsel.Set ();
    }


int                 numspsaved      = spsel.NumSet ();

if ( numspsaved == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;


if ( showprogress ) {
    Gauge.Set           ( RisToPointsTitle, SuperGaugeLevelDefault );

    Gauge.AddPart       ( 0, tfmax - tfmin + 1, 33 );
    Gauge.AddPart       ( 1, spsel.NumSet (),   33 );
    Gauge.AddPart       ( 2, spsel.NumSet (),   34 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We are going to store all the needed points before writing to file - hoping for enough memory
TPoints*                points      = new TPoints [ numspsaved ];
TStrings                pointsnames;
TFileName               filenamerissp;
char                    buff[ 256 ];
TArray1<TVector3Float>  InvBuff ( numsp );
TVector3Float           v;

                                        // Scanning only once!
for ( int tfi = tfmin; tfi <= tfmax; tfi++ ) {

    if ( Gauge.IsAlive () ) Gauge.Next ( 0, SuperGaugeUpdateTitle );
    else                    UpdateApplication;


    risdoc->GetInvSol ( 0, tfi, tfi, InvBuff, 0, 0 );

                                        // Dispatch to our list of points
    for ( TIteratorSelectedForward spi ( spsel ); (bool) spi; ++spi ) {

        v   = InvBuff[ spi() ];

        if ( normalize )
            v.Normalize ();

        points[ spi.GetIndex () ].Add ( v );
        }


    StringCopy      ( buff, "TF", IntegerToString ( tfi, NumIntegerDigits ( numtimeframes ) ) );

    pointsnames.Add ( buff );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Computing best axis for polarity testing
TPoints             allaxis;


for ( TIteratorSelectedForward spi ( spsel ); (bool) spi; ++spi ) {

    if ( Gauge.IsAlive () ) Gauge.Next ( 1, SuperGaugeUpdateTitle );
    else                    UpdateApplication;


    v   = ComputeCloudFolding ( points[ spi.GetIndex () ],   ReferenceNumSamplesSP,    ReferenceNumSamplesSP2 );


    if ( spontaneous )
                                        // orientation is all relative, we can do a rough re-alignment, pointing the main direction to either +X, +Y or +Z
        if ( fabs ( v.X ) > fabs ( v.Y ) && fabs ( v.X ) > fabs ( v.Z ) && v.IsOppositeDirection ( TVector3Float ( 1, 0, 0 ) )
          || fabs ( v.Y ) > fabs ( v.X ) && fabs ( v.Y ) > fabs ( v.Z ) && v.IsOppositeDirection ( TVector3Float ( 0, 1, 0 ) )
          || fabs ( v.Z ) > fabs ( v.X ) && fabs ( v.Z ) > fabs ( v.Y ) && v.IsOppositeDirection ( TVector3Float ( 0, 0, 1 ) ) )

            v.Invert ();


    allaxis.Add (  v );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Write files
TPoints             oneaxis;


for ( TIteratorSelectedForward spi ( spsel ); (bool) spi; ++spi ) {

    if ( Gauge.IsAlive () ) Gauge.Next ( 2, SuperGaugeUpdateTitle );
    else                    UpdateApplication;


    StringCopy          ( filenamerissp, risdoc->GetDocPath () );
    RemoveExtension     ( filenamerissp );

    StringAppend        ( filenamerissp, ".SP", IntegerToString ( spi() + 1, NumIntegerDigits ( numsp ) ) );

    if ( normalize )
        StringAppend    ( filenamerissp, ".Norm" );

    AddExtension        ( filenamerissp, FILEEXT_SPIRR );
    CheckNoOverwrite    ( filenamerissp );


    if ( spontaneous ) {                // To emphasize the cloud main direction, we can duplicate each vector on its opposite side
                                        // OK only for spontaneous data
                                        // Note that we double the number of output points by doing so
        TPoints         copypoints ( points[ spi.GetIndex () ] );

        copypoints.Invert ();

        points[ spi.GetIndex () ]  += copypoints;
        }


    points[ spi.GetIndex () ].WriteFile ( filenamerissp, &pointsnames );

    outgof.Add          ( filenamerissp );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute best axis for polarity
    oneaxis.Reset ();

    oneaxis.Add ( allaxis[ spi.GetIndex () ] );

    if ( spontaneous )
        oneaxis.Add ( -allaxis[ spi.GetIndex () ] );

    PostfixFilename     ( filenamerissp,    ".Axis" );

    oneaxis.WriteFile ( filenamerissp );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

delete[]    points;

risdoc.Close ( CloseDocRestoreAsBefore );

//if ( Gauge.IsAlive () )   Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
