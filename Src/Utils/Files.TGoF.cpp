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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    <unordered_map>

#include    "Files.TGoF.h"

#include    "CartoolTypes.h"            // ResamplingType
#include    "Strings.Utils.h"
#include    "Strings.Grep.h"
#include    "TMarkers.h"
#include    "TArray1.h"
#include    "TArray2.h"
#include    "TSetArray2.h"
#include    "Files.TFindFile.h"
#include    "Files.ReadFromHeader.h"
#include    "Math.Random.h"
#include    "Dialogs.TSuperGauge.h"

#include    "TMaps.h"
#include    "TExportTracks.h"           // TExportTracks

#include    "TVolumeDoc.h"              // MriContentType
#include    "TFreqDoc.h"

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void    TracksCompatibleClass::Reset ()
{
NumTracks           =
NumAuxTracks        =
NumSolPoints        =
NumTF               =
NumFreqs            = CompatibilityIrrelevant;
SamplingFrequency   = CompatibilityIrrelevant;
}


void   TracksCompatibleClass::Show ( const char* title )    const
{
char                localtitle[ 256 ];

StringCopy  ( localtitle, StringIsEmpty ( title ) ? "FileName" : title );

char                buff[ 1024 ];

ClearString     ( buff );
StringAppend    ( buff, "NumTracks"     Tab Tab "= ",   NumTracks           == CompatibilityIrrelevant ? "Irrelevant" : NumTracks           == CompatibilityNotConsistent ? "Inconsistent" : IntegerToString ( NumTracks ),          NewLine );
StringAppend    ( buff, "NumAuxTracks"  Tab Tab "= ",   NumAuxTracks        == CompatibilityIrrelevant ? "Irrelevant" : NumAuxTracks        == CompatibilityNotConsistent ? "Inconsistent" : IntegerToString ( NumAuxTracks ),       NewLine );
StringAppend    ( buff, "NumSolPoints"  Tab Tab "= ",   NumSolPoints        == CompatibilityIrrelevant ? "Irrelevant" : NumSolPoints        == CompatibilityNotConsistent ? "Inconsistent" : IntegerToString ( NumSolPoints ),       NewLine );
StringAppend    ( buff, "NumTF"     Tab Tab Tab "= ",   NumTF               == CompatibilityIrrelevant ? "Irrelevant" : NumTF               == CompatibilityNotConsistent ? "Inconsistent" : IntegerToString ( NumTF ),              NewLine );
StringAppend    ( buff, "NumFreqs"      Tab Tab "= ",   NumFreqs            == CompatibilityIrrelevant ? "Irrelevant" : NumFreqs            == CompatibilityNotConsistent ? "Inconsistent" : IntegerToString ( NumFreqs ),           NewLine );
StringAppend    ( buff, "SamplingFrequency" Tab "= ",   SamplingFrequency   == CompatibilityIrrelevant ? "Irrelevant" : SamplingFrequency   == CompatibilityNotConsistent ? "Inconsistent" : FloatToString   ( SamplingFrequency )           );

ShowMessage ( buff, localtitle );
}


void    FreqsCompatibleClass::Reset ()
{
FreqType                    =
NumTracks                   =
NumTF                       =
NumFreqs                    = CompatibilityIrrelevant;
SamplingFrequency           =
OriginalSamplingFrequency   = CompatibilityIrrelevant;
}


void    ElectrodesCompatibleClass::Reset ()
{
NumElectrodes       = CompatibilityIrrelevant;
}


void    InverseCompatibleClass::Reset ()
{
NumElectrodes       =
NumSolPoints        = CompatibilityIrrelevant;
}


void    RoisCompatibleClass::Reset ()
{
NumDimensions       =
NumElectrodes       =
NumSolPoints        =
NumRois             = CompatibilityIrrelevant;
}


void    TracksGroupClass::Reset ()
{
alleeg              =
allfreq             =
allris              =
allrisv             =
//allsegdata        =
alldata             =
noneofthese         = false;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // "Flatten" a TGoGoF
        TGoF::TGoF ( const TGoGoF& gogof, int gofi1, int gofi2 )
{
if ( gogof.IsEmpty () )
    return;


if ( ! gogof.CheckGroupIndexes ( gofi1, gofi2 ) )
    return;

                                        // cumulate all files
for ( int gofi = gofi1; gofi <= gofi2; gofi++ )

    Add ( gogof[ gofi ] );
}

                                        // "Flatten" a TGoGoF
        TGoF::TGoF ( const TGoGoF& gogof, const TGoF& gof )
{
                                        // first construct from the gogof
for ( int gofi = 0; gofi < gogof.NumGroups (); gofi++ )

    Add ( gogof[ gofi ] );

                                        // then add the given group
Add ( gof );
}


//----------------------------------------------------------------------------
TGoF& TGoF::operator= ( const TGoF &op2 )
{
if ( &op2 == this )
    return  *this;

Set ( (TStrings &) op2 );

return  *this;
}


//----------------------------------------------------------------------------
int     TGoF::NumFiles ()   const
{
                                        // all range of files?
return  NumStrings ();
}


int     TGoF::NumFiles ( int filei1, int filei2 )   const
{
                                        // all range of files?
if ( filei1 == -1 || filei2 == -1 )
    return  NumFiles ();

                                        // look for subsets
CheckOrder ( filei1, filei2 );
                                        // check for non-overlap
if ( filei2 < 0              )  return  0;

if ( filei1 >= NumStrings () )  return  0;

                                        // here we have some overlap

                                        // clip both ends to be within legal range
Clipped ( filei1, filei2, 0, NumStrings () - 1 );

return  filei2 - filei1 + 1;
}


//----------------------------------------------------------------------------
                                        // Overridden functions, called from either TGoF or TStrings:
                                        //  - Adding some margin to the reserved size, in case of extension replacement
                                        //  - Making sure we have at least 2K space reserved
                                        //  - Making sure we have absolute path files
                                        //  - Checking file names bigger than old 256 MAX_PATH have the extended prefix
void    TGoF::Add ( const char* file )
{
Add ( file, 0 );
}


void    TGoF::Add ( const char* file, long length )
{
int                 stringlength    = max ( length, StringLength ( file ) + 64, (long) MaxPathShort ); 

TStrings::Add ( file, StringFixedSize, stringlength ); 
}


void    TGoF::Add ( const TGoF& gof )                             
{
TStrings::Add ( gof );               // will call back our TGoF::Add above
}


//----------------------------------------------------------------------------
void    TGoF::CheckFileNames  ( TFilenameFlags flags )
{
if ( flags == TFilenameNoPreprocessing || IsEmpty () )
    return;


TFileName       filename;

for ( int i = 0; i < (int) Strings; i++ ) {

    if ( StringIsEmpty ( Strings[ i ] ) )
        continue;

                                        // copy AND update file name at the same time
    filename.Set ( Strings[ i ], flags );

                                        // any effective change?
    if ( StringIs ( filename, Strings[ i ] ) )
        continue;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // allocate a new string
    int             stringsize      = max ( filename.Length () + 64, (long) MaxPathShort ); 
    char*           toc             = new char [ stringsize ];

    ClearString ( toc, stringsize );
    StringCopy  ( toc, filename   );

                                        // delete old string, without releasing the list atom, and make it point to the new memory string instead
    delete[]    Strings.GetAtom ( i )->ToData;
    Strings.GetAtom ( i )->ToData = toc;
    }
}


//----------------------------------------------------------------------------
                                        // Overriding TStringsList::Sort with a special lexico-numerical criterion (epochs and stuff)

                                        // This specialized method will interpret the first numerical infix as NUMBERS and NOT TEXT
                                        // This matters in case the infixes were not 0-padded, and look like <xyz>.1 <xyz>.2 <xyz>.3 ... <xyz>.10 <xyz>.11 ...
                                        // Sorting these strings as characters will re-order them as         <xyz>.1 <xyz>.10 <xyz>.11 ... <xyz>.2 <xyz>.20 <xyz>.21 ... 
                                        // which is not intuitive for the user, nor what Windows File Explorer is showing.
                                        // If all infixes were 0-padded, than everything is working fine already.
                                        // Current limitation is: files are expected to be in the syntax  <some common chars><some digits><not digits>
                                        // Maybe the sort can split files by directories, in case multiple paths were used?
void    TGoF::Sort ()
{
if ( NumFiles () <= 1 )
    return;


int                 numstrings      = NumFiles ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we are going to work on the file names only
//TGoF                str ( *this );
//
//str.RemoveDir ();
//
//str.RemoveExtension ();

                                        // working on whole file paths, so directory part will be accounted for in the common prefix
TGoF                str;

for ( int si = 0; si < numstrings; si++ )
                                        // copy whole path + adding some extra room
    str.Add ( Strings[ si ], StringLength ( Strings[ si ] ) + 16 );

//str.Show ( "before sort" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here at least 2 strings
int                 minlength       = str.GetMinStringLength ();

                                        // some emtpy strings?
if ( minlength == 0 ) {
                                        // call default Sort
    TStrings::Sort ();

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scan up to the minlength characters
int                 commonstartlength   = 0;
int                 i;

                                        // scan each letter forward
for ( int l = 0; l < minlength; l++ ) {
                                        // scan each string
    for ( i = 1; i < numstrings; i++ )

        if ( str[ i ][ l ] != str[ 0 ][ l ] )
            break;                      // fail


    if ( i == numstrings )  commonstartlength++;    // all files matched current char
    else                    break;                  // stop now
    }

                                        // ho-ho, all strings "look the same" (modulo their common length)
if ( commonstartlength == minlength ) {
                                        // call default Sort
    TStrings::Sort ();

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we are going to count any digits after the common start, for all strings
enum                digitposenum 
                    {
                    DigitFrom,
                    DigitTo,
                    NumDigits,
                    numdigitposenum
                    };

TArray2<int>        digitindex ( numstrings, numdigitposenum );   // from, to, #
int                 numstringdigits = 0;
int                 maxdigitlength  = 0;

digitindex.ResetMemory ();

                                        // getting each string digits interval
for ( int si = 0; si < numstrings; si++ ) {
                                        // scan each letter forward
    for ( int l = commonstartlength; l < StringLength ( str[ si ] ); l++ )

        if ( isdigit ( str[ si ][ l ] ) ) {
                                        // starting digit position            
            if ( l == commonstartlength ) {
                digitindex ( si, DigitFrom )    = l;
                numstringdigits++;
                }
                                        // ending digit position & count    
            digitindex ( si, DigitTo   )    = l;
            digitindex ( si, NumDigits )++;

            Maxed ( maxdigitlength, digitindex ( si, NumDigits ) );
            }
        else
            break;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do all strings begin with digit(s)?
bool                allhavedigits       = numstringdigits == numstrings;
bool                diffdigitslength    = false;


if ( allhavedigits ) {

    for ( int si = 0; si < numstrings; si++ )

        if ( digitindex ( si, NumDigits ) < maxdigitlength ) {
            diffdigitslength    = true;
            break;
            }
    }
else {
                                        // call default Sort
    TStrings::Sort ();

    return;
    }
        

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do we really need to work on any digit substring?
if ( diffdigitslength ) {

    for ( int si = 0; si < numstrings; si++ ) {
        
        if ( digitindex ( si, NumDigits ) < maxdigitlength ) {
                                        // need to modify this string
            int     numinsert   = maxdigitlength - digitindex ( si, NumDigits );
                                        // !we allocated some extra space, so we can move without problems!
            MoveVirtualMemory   (   &str[ si ][ digitindex ( si, DigitFrom ) ] + numinsert, 
                                    &str[ si ][ digitindex ( si, DigitFrom ) ], 
                                    StringSize ( str[ si ] ) - digitindex ( si, DigitFrom ) // also move the \0
                                );
                                        // fill with 0's
            for ( int zi = digitindex ( si, DigitFrom ), zi0 = 0; zi0 < numinsert; zi++, zi0++ )
                str[ si ][ zi ] = '0';
            }
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // store current processed strings' order
    std::unordered_map< char*, int >    stringindex;

    for ( int si = 0; si < numstrings; si++ )

        stringindex[ str[ si ] ]    = si;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // finally sort the upgraded string list
    str.TStrings::Sort ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // copy whole original file paths
    TStrings            oldgof ( *this );

    Reset ();

    for ( int si = 0; si < numstrings; si++ )
                                        // re-insert back according to sorted index, computed from the expanded digit strings
        Add ( oldgof[ stringindex[ str[ si ] ] ] );

    } // if diffdigitslength

else
                                        // digits with all equal length
    TStrings::Sort ();
}


//----------------------------------------------------------------------------
        TGoF::TGoF ( const owl::TDropInfo& drop, const char* filesext, TPointInt* where, bool doesnotbelong )
{
Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( where != 0 ) {
    owl::TPoint     droppedposition;
    bool            droppedinside       = drop.DragQueryPoint ( droppedposition );  // inside Client area

    if ( droppedinside )    where->Set   ( droppedposition.X (), droppedposition.Y (), 0 );
    else                    where->Reset ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           file;
TGoF                siblingfiles;
int                 dropcount       = drop.DragQueryFileCount ();

for ( int i = 0; i < dropcount; i++ ) {

//  drop.DragQueryFileNameLen ( i );

    drop.DragQueryFile ( i, file, file.Size () );

                                        // do that all the time, by safety (used to be in the Add method)
    file.CheckFileName ( TFilenameFlags ( TFilenameAbsolutePath | TFilenameExtendedPath ) );

                                        // see if user dropped a directory, or EEG buddy files
    FilenameToSiblings ( (char *) file, siblingfiles );


    for ( int si = 0; si < (int) siblingfiles; si++ )
                                        // doesnotbelong, if set, allows to test the non-belonging
        if (   StringIsEmpty ( filesext ) 
          || ( IsExtensionAmong ( siblingfiles[ si ], filesext ) ^ doesnotbelong ) )
                                        // check for duplicates, due to sibling effect for MRIs f.ex.
            AddNoDuplicates ( siblingfiles[ si ] );     // there should be enough room to allow the extension to be modified
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Microsoft Windows has the extremely silly habit to shuffle the order of the dropped files
                                        // Give it some consistent behavior by sorting them
Sort ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // It can be convenient to rearrange the MRIs so to have the Grey / Brain / Head ordering right
                                        // This will help and D&D into .lm files
TGoF                sortedmrifiles;

                                        // insertion order matters here
for ( int i = 0; i < (int) Strings; i++ )

    if ( IsGrey             ( GuessTypeFromFilename ( Strings[ i ] ) ) ) {
        
        sortedmrifiles.Add  ( Strings[ i ] );
        RemoveRef           ( Strings[ i ] );
        break;
        }


for ( int i = 0; i < (int) Strings; i++ )

    if ( IsWhite            ( GuessTypeFromFilename ( Strings[ i ] ) ) ) {
        
        sortedmrifiles.Add  ( Strings[ i ] );
        RemoveRef           ( Strings[ i ] );
        break;
        }


for ( int i = 0; i < (int) Strings; i++ )

    if ( IsRoiMask          ( GuessTypeFromFilename ( Strings[ i ] ) ) ) {
        
        sortedmrifiles.Add  ( Strings[ i ] );
        RemoveRef           ( Strings[ i ] );
        break;
        }


for ( int i = 0; i < (int) Strings; i++ )

    if ( IsBrain            ( GuessTypeFromFilename ( Strings[ i ] ) ) ) {
        
        sortedmrifiles.Add  ( Strings[ i ] );
        RemoveRef           ( Strings[ i ] );
        break;
        }


for ( int i = 0; i < (int) Strings; i++ )

    if ( IsFullHead         ( GuessTypeFromFilename ( Strings[ i ] ) ) ) {
        
        sortedmrifiles.Add  ( Strings[ i ] );
        RemoveRef           ( Strings[ i ] );
        break;
        }

                                        // finally add all remaining files
sortedmrifiles.Add ( *this );

                                        // copy
*this   = sortedmrifiles;
}


//----------------------------------------------------------------------------
void    TGoF::CopyFilesTo ( const char* newdir, CopyToFlags flags, const char* buddyexts )
{
if ( IsEmpty () || StringIsEmpty ( newdir ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TFileName               filename;
TFileName               ext;
TFileName               oldfilename;
TFileName               newfilename;

                                        // given buddy extensions
TSplitStrings           exts ( buddyexts, UniqueStrings );
                                        // known buddies, on request
if ( flags & CopyAllKnownBuddies )
    exts.Add ( AllKnownBuddyExt, UniqueStrings );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // create full path to new dir
CreatePath ( newdir, false );


for ( int i = 0; i < (int) Strings; i++ ) {

    if ( (bool) exts ) {
                                        // in case of associated extensions, we need to process the original file a little
        filename.Set    ( Strings[ i ], TFilenameNoPreprocessing );

        crtl::GetExtension      ( ext, filename );
        crtl::RemoveExtension   ( filename );

        if ( exts.Contains ( FILEEXT_MRK ) ) {
                                        // add one more extension  <originalext>.mrk
                                        // it allows the GoF to be of different extensions, although this is not beset practice
            AddExtension ( ext, FILEEXT_MRK );
            exts.Add     ( ext, UniqueStrings );
            }
        }

                                        // set the main files
    oldfilename.Set ( Strings[ i ], TFilenameNoPreprocessing );

    StringCopy ( newfilename, oldfilename );
    ReplaceDir ( newfilename, newdir );


    if ( flags & CopyAndUpdateFileNames )
        StringCopy ( Strings[ i ], newfilename );


    oldfilename.SetExtendedPath ();
    newfilename.SetExtendedPath ();   // not before copying to caller

                                        // loop through exts + no ext change case
    for ( int i = -1; i < (int) exts; i++ ) {
                                        // working on buddy extensions?
        if ( i >= 0 ) {

            StringCopy ( oldfilename, filename, ".", exts[ i ] );

            StringCopy ( newfilename, oldfilename );
            ReplaceDir ( newfilename, newdir );
            }

                                        // file exists?
        if ( ! CanOpenFile ( oldfilename ) )
            continue;


//      DBGM3 ( oldfilename.FileName, newfilename.FileName, ( flags & CopyAndDeleteOriginals ) ? "Move" : "Copy", "TGoF::CopyFilesTo:  from -> to, Moving/Copying" );
                                        
        if ( flags & CopyAndDeleteOriginals )   MoveFileExtended    ( oldfilename, newfilename );   // copy & delete original == moving
        else                                    CopyFileExtended    ( oldfilename, newfilename );   // make a separate copy

        } // for exts

    } // for strings


//if ( flags & CopyAndDeleteOriginals )            // assume all files are in the same directory, and only these files
//    NukeDirectory ( Strings[ 0 ] );
}


void    TGoGoF::CopyFilesTo ( const char* newdir, CopyToFlags flags, const char* buddyexts )
{
//TGoF ( *this ).CopyFilesTo ( newdir, flags, buddyexts );  // does not work on a copy, we might need to update the GoGoF


if ( IsEmpty () || StringIsEmpty ( newdir ) )
    return;


for ( int gofi = 0; gofi < NumGroups (); gofi++ )

    Group[ gofi ]->CopyFilesTo ( newdir, flags, buddyexts );
}


//----------------------------------------------------------------------------
void    TGoF::CopyFilesTo ( const TGoF& newlist, CopyToFlags flags, const char* buddyexts )     const
{
if ( IsEmpty () || ! (bool) newlist )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TFileName               filename;
TFileName               ext;
TFileName               oldfilename;
TFileName               newfilename;

                                        // given buddy extensions
TSplitStrings           exts ( buddyexts, UniqueStrings );
                                        // known buddies, on request
if ( flags & CopyAllKnownBuddies )
    exts.Add ( AllKnownBuddyExt, UniqueStrings );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // create full path to new dir
CreatePath ( newlist[ 0 ], true );


for ( int i = 0; i < min ( (int) Strings, (int) newlist ); i++ ) {

    if ( (bool) exts ) {
                                        // in case of associated extensions, we need to process the original file a little
        filename.Set    ( Strings[ i ], TFilenameNoPreprocessing );

        GetExtension      ( ext, filename );
        crtl::RemoveExtension ( filename );

        if ( exts.Contains ( FILEEXT_MRK ) ) {
                                        // add one more extension  <originalext>.mrk
            AddExtension ( ext, FILEEXT_MRK );
            exts.Add     ( ext, UniqueStrings );
            }
        }


    oldfilename.Set ( Strings[ i ], TFilenameExtendedPath );
    newfilename.Set ( newlist[ i ], TFilenameExtendedPath );

                                        // loop throuh exts + no ext change case
    for ( int i = -1; i < (int) exts; i++ ) {
                                        // working on buddy extensions?
        if ( i >= 0 ) {

            StringCopy      ( oldfilename, filename, ".", exts[ i ] );

            StringCopy        ( newfilename, newlist[ i ] );  // !assumes newlist has the same extension as Strings!
            crtl::RemoveExtension ( newfilename );
            AddExtension      ( newfilename, exts[ i ] );
            }

                                        // file exists?
        if ( ! CanOpenFile ( oldfilename ) )
            continue;


//        DBGM2 ( oldfilename.FileName, newfilename.FileName, "copying: from to" );

        if ( flags & CopyAndDeleteOriginals )   MoveFileExtended    ( oldfilename, newfilename );   // copy & delete original == moving
        else                                    CopyFileExtended    ( oldfilename, newfilename );   // make a separate copy

        } // for exts

    } // for strings


//if ( flags & CopyAndDeleteOriginals )                  // assume all files are in the same directory, and only these files
//    NukeDirectory ( Strings[ 0 ] );
}


//----------------------------------------------------------------------------
                                        // Will delete all matching files  "path\file.*", i.e. marker files, assocated headers & files etc...
void    TGoF::DeleteFiles ( const char* buddyexts )    const
{
for ( int i = 0; i < (int) Strings; i++ )
                                        // Will take care of all extension cases
    crtl::DeleteFileExtended ( Strings[ i ], buddyexts );
}


void    TGoGoF::DeleteFiles ( const char* buddyexts )   const
{
for ( int gofi = 0; gofi < NumGroups (); gofi++ )

    Group[ gofi ]->DeleteFiles ( buddyexts );
}


//----------------------------------------------------------------------------
void    TGoF::NukeDirectories ( bool confirm )  const
{
if ( IsEmpty () )
    return;


TGoF                    tonuke ( *this );
                                        // keeping the directory parts only
tonuke.RemoveFilename ();
                                        // removing duplicate directories - files are often in a single directory!
tonuke.RemoveDuplicates ();


for ( int i = 0; i < (int) tonuke; i++ ) {

    //DBGM ( tonuke[ i ], "Nuking" );

    crtl::NukeDirectory ( tonuke[ i ], confirm );
    } // for strings
}


void    TGoGoF::NukeDirectories ( bool confirm )  const
{
for ( int gofi = 0; gofi < NumGroups (); gofi++ )

    Group[ gofi ]->NukeDirectories ( confirm );
}


//----------------------------------------------------------------------------
void    TGoF::OpenFiles ()  const
{
for ( int i = 0; i < (int) Strings; i++ )

    Cartool.CartoolDocManager->OpenDoc ( Strings[ i ], dtOpenOptions );
}


//----------------------------------------------------------------------------
bool    TGoF::CanOpenFiles ( CanOpenFlags flags )    const
{
if ( IsEmpty () )
    return  false;  // or true? caller had better to decide before we end up here...


for ( int i = 0; i < (int) Strings; i++ )

    if ( ! CanOpenFile ( Strings[ i ], flags ) )

        return  false;


return  true;
}


//----------------------------------------------------------------------------
void    TGoF::RemoveDir ()
{
for ( int i = 0; i < (int) Strings; i++ )
    crtl::RemoveDir ( Strings[ i ] );
}


void    TGoF::RemoveFilename ()
{
for ( int i = 0; i < (int) Strings; i++ )
    crtl::RemoveFilename ( Strings[ i ], false );
}


void    TGoF::RemoveExtension ()
{
for ( int i = 0; i < (int) Strings; i++ )
    crtl::RemoveExtension ( Strings[ i ] );
}


TGoF    TGoF::GetPaths ()   const
{
TGoF                paths;
TFileName           dir;

for ( int i = 0; i < (int) Strings; i++ ) {

    dir     = Strings[ i ];

    crtl::RemoveFilename    ( dir, false );

    paths.AddNoDuplicates   ( dir );
    }

return  paths;
}


//----------------------------------------------------------------------------
void    TGoF::SetTempFileNames ( int numfiles, const char* ext )
{
Reset ();

if ( numfiles <= 0 )
    return;


TFileName           file;

for ( int i = 0; i < numfiles; i++ ) {

    file.SetTempFileName ( ext );

    Add ( file );
    }
}


//----------------------------------------------------------------------------
                                        // optional parameter to search only for directories
int     TGoF::FindFiles ( const char* templ, bool searchfiles )
{
Reset ();

if ( StringIsEmpty ( templ ) )
    return  0;


TFileName           dir;
TFileName           file;

                                        // the stupid function strips out the directory part
StringCopy       ( dir, templ );
crtl::RemoveFilename ( dir );


TFindFile           findfile;


while ( findfile.SearchFile ( templ ) ) {

                                        // we can either search for files or directories (exclusively)
    if (  searchfiles && ! findfile.IsFile      ()
     || ! searchfiles && ! findfile.IsDirectory () )
        continue;
                                        // we have a real file here

    StringCopy ( file, dir, "\\", findfile.GetPathChunk () );
                                        // insert complete path
    Add ( file, MaxPathShort );

//  DBGM ( file, "FindFiles: File Added" );
    }

                                        
Sort ();
//TStrings::Sort ();                   // do we really need the advanced TGoF::Sort if used for copying stuff?


return  NumFiles ();
}


//----------------------------------------------------------------------------
                                        // Use the Grep syntax for more powerful searches
int     TGoF::GrepFiles ( const char* path, const char* regexp, GrepOption options, bool searchfiles )
{
Reset ();

if ( StringIsEmpty ( path ) || StringIsEmpty ( regexp ) )
    return  0;


TFileName           pathext;
TGoF                allfiles;

                                        // read whole directory
StringCopy      ( pathext, path );
StringAppend    ( pathext, "\\*" );

                                        // get all files
allfiles.FindFiles  ( pathext, searchfiles );

                                        // prepare the grep
TStringGrep         greppy ( regexp, options );

                                        // run grep on retrieved files
for ( int i = 0; i < (int) allfiles; i++ )
    if ( greppy.Matched ( allfiles[ i ] ) )
        Add ( allfiles[ i ], MaxPathShort );


//Show ( "GrepFiles: grepped files" );


return  NumFiles ();
}


//----------------------------------------------------------------------------
                                        // ! prefilename and postfilename are Grep syntax string - File name parts like "." or "\" have to neutralized when constructing the string !
void    TGoF::GrepGoF  ( const TGoF& gof, const char* prefilename, const char* postfilename, const char* newexts, bool allresults )
{
Reset ();

if ( gof.IsEmpty () || StringIsEmpty ( newexts ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TFileName           path;
TFileName           file;
TFileName           grepnew;
char                grepnewexts[ 256 ];
TGoF                gofnew;

                                        // convert parameter to Grep syntax
ExtensionsToGrep    ( grepnewexts, newexts );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) gof; i++ ) {

                                        // take the path from each input file, in case it is heterogeneous
    StringCopy          ( path, gof[ i ] );
    crtl::RemoveFilename( path );

    StringCopy          ( file, gof[ i ] );
    GetFilename         ( file );
    StringGrepNeutral   ( file );

                                        // searching directly into file name part only
    StringCopy          ( grepnew, prefilename, file, postfilename, grepnewexts );

                                        // get all potential files first
    if ( gofnew.GrepFiles ( path, grepnew, GrepOptionDefaultFiles ) )
                                        // cumulate all found files
        Add ( gofnew );

    else {
                                        // if only 1 fails, all fail
//      Reset ();
//      return;
                                        // continue the search - user has to test the number of returned files
                                        // also, this wil disrupt the groups sync, which could be problematic
        ;
                                        // or insert a null string to remain synced with the input list(?)
//      Add ( "" );
        }
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // if caller asked for only 1 result, return the most recent one(?)
if ( ! allresults ) {

//  Show ( "All files" );

    KeepLatestFile ();

//  Show ( "Latest file" );
    }

}


//----------------------------------------------------------------------------
void    TGoF::KeepLatestFile () 
{
                                        // well, there must be more than 1 file...
if ( NumFiles () <= 1 )
    return;


TFindFile       findfile;
FILETIME        LastWriteTime;
FILETIME        BestLastWriteTime;
int             latesti         = -1;


for ( int i = 0; i < (int) Strings; i++ ) {

                                        // by safety, check file opens correctly
    if ( ! findfile.SearchFirstFile ( Strings[ i ] ) )
        continue;

                                        // actually TFindFile already knows everything about that file
    LastWriteTime   = findfile.GetLastWriteTime ();


    if ( latesti == -1 ) {
        BestLastWriteTime   = LastWriteTime;
        latesti             = i;
        }
                                    // select the file with latest write time
    else if (    LastWriteTime.dwHighDateTime >  BestLastWriteTime.dwHighDateTime
           ||    LastWriteTime.dwHighDateTime == BestLastWriteTime.dwHighDateTime
              && LastWriteTime.dwLowDateTime  >  BestLastWriteTime.dwLowDateTime  ) {

        BestLastWriteTime   = LastWriteTime;
        latesti             = i;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // take some care to keep the latest file name
TFileName       latestfile ( Strings[ latesti ], TFilenameExtendedPath );

SetOnly ( latestfile );
}


//----------------------------------------------------------------------------
void    TGoF::ReplaceExtension ( const char* newext )
{

if ( IsEmpty () )
    return;


for ( int i = 0; i < (int) Strings; i++ )

    crtl::ReplaceExtension ( Strings[ i ], newext );

                                        // New extension might leak beyond the magic Windows size
CheckFileNames ( TFilenameExtendedPath );
}


//----------------------------------------------------------------------------
                                        // Here, the input gof is the one with the biggest strings
                                        // Things are therefore more tricky, as it is not possible to grep shorter strings
                                        // Solution is to get all possible candidates first, the Grep every of these candidates to the input GoF
                                        // Works across multiple directories

/*
                                        Method needs to go back and forth between the EEGs and the RIS:

                                        We want:
                                            shorteeg        <-      longris

                                        What we do:
                                            allshorteegs    <-      longris

                                            foreach oneshorteeg in allshorteegs

                                                oneshorteeg ->  alllongris

                                                foreach onelongris in alllongris
                                                    if onelongri == longris
                                                        store longest oneshorteeg
*/


void    TGoF::RevertGrepGoF  ( const TGoF& gof, const char* prefilename, const char* postfilename, const char* newexts )
{
Reset ();

if ( gof.IsEmpty () || StringIsEmpty ( newexts ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // extract all existing paths in case files span across multiple directories
TGoF                paths           = gof.GetPaths ();


char                grepnewexts[ KiloByte ];
                                        // convert parameter to Grep syntax
ExtensionsToGrep    ( grepnewexts, newexts );

                                        // Retrieve all files with the given extensions
TFileName           grepshort;

StringCopy          ( grepshort, ".+", grepnewexts );


TGoF                gofshortcandidate;
                                        // cumulate all possible files across all possible directories first
for ( int pathi = 0; pathi < (int) paths; pathi++ ) {
    
    TGoF            gofsc;
    
    gofsc.GrepFiles  ( paths[ pathi ], grepshort, GrepOptionDefaultFiles );
    
    gofshortcandidate.Add ( gofsc );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Loop through all the potential (shorter) targets, and test for input gof
TGoF                gof1short;
TGoF                gofnlong;
TGoF                gofshortbest ( (int) gof );
int                 gofindex;
//TGoF              gofin ( gof );      // make a copy
char                oldext[ 256 ];

                                        // assume all input (long) gof are the same...
GetExtension ( oldext, gof[ 0 ] );


for ( int i = 0; i < (int) gofshortcandidate; i++ ) {

                                        // transfer current single short file to a gof
    gof1short.SetOnly ( gofshortcandidate[ i ] );

                                        // search back to all files, with the maximum number of results
    gofnlong.GrepGoF ( gof1short, prefilename, postfilename, oldext, true );

                                        // got nothing back?
    if ( gofnlong.IsEmpty () )
        continue;
                                        // there can be more than 1 long file name (like diffrerent preprocessed EEG leading to a RIS file)
                                        // it is also expected that no 2 short file names will match 1 long name
    for ( int j = 0; j < (int) gofnlong; j++ ) {

//      DBGM3 ( ToFileName ( gof[ 0 ] ), ToFileName ( gof1short[ 0 ] ), ToFileName ( gofnlong[ j ] ), "target -> back eeg -> back ris" );

                                        // are we cycling back to one of the input long file?
        gofindex    = gof.GetIndex ( gofnlong[ j ] );

        if ( gofindex == -1 )   continue;


        if ( StringLength ( gofshortcandidate[ i ] ) > StringLength ( gofshortbest[ gofindex ] ) )
                                        // keep the longest candidate
            StringCopy ( gofshortbest[ gofindex ], gofshortcandidate[ i ] );

        }

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // copy only the non-empty (i.e. the one that matched something) files
for ( int i = 0; i < (int) gofshortbest; i++ )

    if ( StringIsNotEmpty ( gofshortbest[ i ] ) )

        Add ( gofshortbest[ i ] );

                                        // OK only if the same number of files
//if ( NumFiles () != (int) gof )
//
//    Reset ();
}


//----------------------------------------------------------------------------
void    TGoGoF::GrepGoF (   const TGoGoF& gogof, const char* prefilename, const char* postfilename, const char* newexts, bool allresults )
{
Reset ();

if ( gogof.IsEmpty () || StringIsEmpty ( newexts ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Set ( gogof.NumGroups () );

                                        // Grep on each gof
for ( int absg = 0; absg < gogof.NumGroups (); absg++ ) {

    Group[ absg ]->GrepGoF ( gogof[ absg ], prefilename, postfilename, newexts, allresults );

                                        // note: allow an empty group, if the input group was also empty, results remain coherent
//  if ( Group[ absg ]->NumFiles () != gogof[ absg ].NumFiles () ) {
//                                      // one failing gof is enough to fail all gof's
//      Reset ();
//      return;
//      }
    }
}


//----------------------------------------------------------------------------
void    TGoGoF::RevertGrepGoF  ( const TGoGoF& gogof, const char* prefilename, const char* postfilename, const char* newexts )
{
Reset ();

if ( gogof.IsEmpty () || StringIsEmpty ( newexts ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Set ( gogof.NumGroups () );

                                        // Grep on each gof
for ( int absg = 0; absg < gogof.NumGroups (); absg++ ) {

    Group[ absg ]->RevertGrepGoF ( gogof[ absg ], prefilename, postfilename, newexts );

                                        // note: allow an empty group, if the input group was also empty, results remain coherent
//  if ( Group[ absg ]->NumFiles () != gogof[ absg ].NumFiles () ) {
//                                      // one failing gof is enough to fail all gof's
//      Reset ();
//      return;
//      }
    }
}


//----------------------------------------------------------------------------
bool    TGoF::AllExtensionsAre ( const char* exts, int atomtype ) const
{
if ( IsEmpty () )
    return  false;


bool                isscalar;


for ( int i = 0; i < (int) Strings; i++ ) {

    if ( ! IsExtensionAmong ( Strings[ i ], exts ) )
        return  false;

                                        // optionally testing the data type
    if ( atomtype != UnknownAtomType ) {
        if ( ! ReadFromHeader ( Strings[ i ], ReadInverseScalar, &isscalar ) )
            return  false;

        if ( atomtype == AtomTypeScalar && ! isscalar
          || atomtype == AtomTypeVector &&   isscalar )
            return  false;
        }
    }


return  true;
}


bool    TGoF::SomeExtensionsAre ( const char* exts /*, int atomtype*/ )   const
{
if ( IsEmpty () )
    return  false;


//bool                    isscalar;


for ( int i = 0; i < (int) Strings; i++ ) {

    if ( IsExtensionAmong ( Strings[ i ], exts ) )
        return  true;

/*                                        // optionally testing the data type
    if ( atomtype != UnknownAtomType ) {
        if ( ! ReadFromHeader ( iterator (), ReadInverseScalar, &isscalar ) )
            continue;

        if ( atomtype == AtomTypeScalar &&   isscalar
          || atomtype == AtomTypeVector && ! isscalar )
            return  true;
        }
*/
    }

return  false;
}


bool    TGoGoF::AllExtensionsAre ( const char* exts, int atomtype, int gofi1, int gofi2 ) const
{
return  TGoF ( *this, gofi1, gofi2 ).AllExtensionsAre ( exts, atomtype );
}


bool    TGoGoF::SomeExtensionsAre ( const char* exts /*, int atomtype*/, int gofi1, int gofi2 )   const
{
return  TGoF ( *this, gofi1, gofi2 ).SomeExtensionsAre ( exts );
}


//----------------------------------------------------------------------------
bool    TGoF::AllFreqsGroup ()  const
{
if ( IsEmpty () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Test files extension
TracksGroupClass        tg;

if ( ! ( AnyTracksGroup ( tg ) 
      && tg.allfreq ) )

    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Test tracks dimensions
TracksCompatibleClass   tc;

AllTracksAreCompatible ( tc );

                                        // most important is to test for the number of frequencies
                                        // variables are tested to be explicitly valid AND to have meaningful values
return  tc.NumTracks >= CompatibilityConsistent && tc.NumTracks > 0
     && tc.NumTF     >= CompatibilityConsistent && tc.NumTF     > 0
     && tc.NumFreqs  >= CompatibilityConsistent && tc.NumFreqs  > 0;
}


bool    TGoGoF::AllFreqsGroup ( int gofi1, int gofi2 )  const
{
return  TGoF ( *this, gofi1, gofi2 ).AllFreqsGroup ();
}


//----------------------------------------------------------------------------
bool    TGoF::AllExtensionsIdentical ( const char* errormessage ) const
{
if ( IsEmpty () )
    return  false;

                                        // all the same extensions? (more restrictive than file types, where EEGs can spread among different file types)
const char*         toext0          = ToExtension ( Strings[ 0 ] );


for ( int i = 1; i < (int) Strings; i++ )

    if ( ! IsExtension ( Strings[ i ], toext0 ) ) {

        if ( StringIsNotEmpty ( errormessage ) )
            ShowMessage ( errormessage, "Group of Files", ShowMessageWarning );

        return  false;
        }

return  true;
}


//----------------------------------------------------------------------------
                                        // Look for a consistent group of files
                                        // Retuns true if 1 group is consistent
bool    TGoF::AnyTracksGroup ( TracksGroupClass& tg )   const
{
tg.Reset ();

if ( IsEmpty () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // count each groups of files
int                 eegcount        = 0;
int                 freqcount       = 0;
int                 riscount        = 0;
int                 risvcount       = 0;
//int               segdatacount    = 0;
int                 datacount       = 0;


for ( int i = 0; i < (int) Strings; i++ ) {

                                        // take benefit from extension groups being exclusive
    if      ( IsExtensionAmong ( Strings[ i ], AllEegFilesExt  ) )      eegcount    ++;
    else if ( IsExtensionAmong ( Strings[ i ], AllFreqFilesExt ) )      freqcount   ++;
//  else if ( IsExtensionAmong ( Strings[ i ], AllSegDataExt   ) )      segdatacount++;
    else if ( IsExtensionAmong ( Strings[ i ], AllDataExt      ) )      datacount   ++;
    else if ( IsExtensionAmong ( Strings[ i ], AllRisFilesExt  ) ) {

        riscount++;

                                        // made some further investigations for vectorial type
        bool                    isscalar;

        if ( ReadFromHeader ( Strings[ i ], ReadInverseScalar, &isscalar )
             && ! isscalar )
                                        // looking for AtomTypeVector
            risvcount++;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

tg.alleeg           = eegcount     == NumFiles ();
tg.allfreq          = freqcount    == NumFiles ();
tg.allris           = riscount     == NumFiles ();
tg.allrisv          = risvcount    == NumFiles ();  // this is a specialized version of ris
//tg.allsegdata     = segdatacount == NumFiles ();
tg.alldata          = datacount    == NumFiles ();
tg.noneofthese      = eegcount + freqcount + riscount/* + segdatacount*/ + datacount == 0;


return  tg.alleeg || tg.allfreq || tg.allris /* || tg.allrisv */ /*|| tg.allsegdata*/ || tg.alldata;
}


bool    TGoGoF::AnyTracksGroup ( TracksGroupClass& tg, int gofi1, int gofi2 )   const
{
return  TGoF ( *this, gofi1, gofi2 ).AnyTracksGroup ( tg );
}


//----------------------------------------------------------------------------
                                        // Testing for the consistency of a group of files, for 1 given parameter
                                        // Legal types are int and double, the types of values handled by ReadFromHeader function
                                        // Function is not part of TGoF (as it should) so we can templatize it
                                        // A returned value can be:
                                        // - any value >= 0             if all files successfully returned the same results (could technically even be 0)
                                        // - CompatibilityIrrelevant    if all files missed the given parameter
                                        // - CompatibilityNotConsistent if consistency whatsoever failed (existing vs non existing, and/or different values)
template <class TypeD>    
void        CheckParameterCompatibility ( const TGoF& gof, TypeD& value, ReadFromHeaderType parameter )
{
value   = CompatibilityIrrelevant;

if ( gof.IsEmpty () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TypeD               retrievedvalue;

                                        // Scan for number of tracks
for ( int i = 0; i < (int) gof; i++ ) {

                                            // test if retrieve is failing, due to request being irrelevant
    if ( ! ReadFromHeader ( TFileName ( gof[ i ] ), parameter, &retrievedvalue ) )
                                            // overwrite value with special irrelevant flag
        retrievedvalue  = CompatibilityIrrelevant;

//  DBGV3 ( parameter, i, retrievedvalue, "Parameter #File -> value");

    if      ( i == 0 )                      // first value?

        value           = retrievedvalue;   // intialize with either the retrieved value, or irrelevant flag - then continue

                                            // already initialized with either a value or irrelevant flag
    else if ( retrievedvalue != value ) {   // now comparing with previous value: 
                                            // - should be successful AND same value all along
                                            // - or, should be irrelevant all along - if sometimes relevant and sometimes not, rather flag it as inconsistent
        value           = CompatibilityNotConsistent;
        break;                              // enough for breaking
        }

    } // for Strings

}


//----------------------------------------------------------------------------
                                        // Testing for the consistency of a group of files as tracks, for all meaningful parameters
                                        // For returned types see  CheckCompatibilityT
void    TGoF::AllTracksAreCompatible ( TracksCompatibleClass& tc )  const
{
tc.Reset ();
                                        // Scanning for each feature sequentially, which means a lot of files opening/closing, but it seems to be quite fast anyway
CheckParameterCompatibility ( *this,    tc.NumTracks,                   ReadNumElectrodes     );    // both for tracks & xyz files!
CheckParameterCompatibility ( *this,    tc.NumAuxTracks,                ReadNumAuxElectrodes  );    // will work correctly only for files with header info: MFF, sef, d, ep/eph
CheckParameterCompatibility ( *this,    tc.NumSolPoints,                ReadNumSolPoints      );
CheckParameterCompatibility ( *this,    tc.NumTF,                       ReadNumTimeFrames     );
CheckParameterCompatibility ( *this,    tc.NumFreqs,                    ReadNumFrequencies    );
CheckParameterCompatibility ( *this,    tc.SamplingFrequency,           ReadSamplingFrequency );

//tc.Show ( "TGoF::AllTracksAreCompatible" );


/*                                        // Opening the whole documents in sequence, so that all variables are exposed at once - Does not seem to be more efficient due to the whole document processing
for ( int i = 0; i < NumFiles (); i++ ) {

    TOpenDoc<TTracksDoc>    EegDoc;
    TracksCompatibleClass   retrieved;  // inits all to  CompatibilityIrrelevant


    if ( EegDoc.Open ( (*this)[ i ], OpenDocHidden ) ) {
                                        // Doc is open, we can retrieve things
        retrieved.NumTracks         = EegDoc->GetNumElectrodes ();
        retrieved.NumAuxTracks      = EegDoc->GetNumAuxElectrodes ();
        retrieved.NumTF             = EegDoc->GetNumTimeFrames ();
        retrieved.SamplingFrequency = EegDoc->GetSamplingFrequency ();

                                        // look for any derived class
        TFreqDoc*       FreqDoc     = dynamic_cast<TFreqDoc*> ( EegDoc.GetDoc () );
        if ( FreqDoc )  
            retrieved.NumFreqs      = FreqDoc->GetNumFrequencies ();

        TRisDoc*        RisDoc      = dynamic_cast<TRisDoc*>      ( EegDoc.GetDoc () );
        if ( RisDoc )  
            retrieved.NumSolPoints  = tc.NumTracks;
        }
    else
        retrieved.Reset ();                 // An un-openable document is considered irrelevant


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( i == 0 )                           // first file / first value?

        tc      = retrieved;                // intialize with either the retrieved values, either correct or irrelevant, then continue

    else {                                  // already initialized with either a value or irrelevant flag

                                            // now comparing current retrieved values with previous values
                                            // - should be successful AND same value all along
                                            // - or, should be irrelevant all along - if sometimes relevant and sometimes not, rather flag it as inconsistent
        if ( retrieved.NumTracks            != tc.NumTracks         )   tc.NumTracks            = CompatibilityNotConsistent;
        if ( retrieved.NumAuxTracks         != tc.NumAuxTracks      )   tc.NumAuxTracks         = CompatibilityNotConsistent;
        if ( retrieved.NumTF                != tc.NumTF             )   tc.NumTF                = CompatibilityNotConsistent;
        if ( retrieved.SamplingFrequency    != tc.SamplingFrequency )   tc.SamplingFrequency    = CompatibilityNotConsistent;
        if ( retrieved.NumFreqs             != tc.NumFreqs          )   tc.NumFreqs             = CompatibilityNotConsistent;
        if ( retrieved.NumSolPoints         != tc.NumSolPoints      )   tc.NumSolPoints         = CompatibilityNotConsistent;
        } // not first file


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    EegDoc.Close ();
    } // for file


//tc.Show ( "TGoF::AllTracksAreCompatible" );
*/
}


void    TGoGoF::AllTracksAreCompatible ( TracksCompatibleClass& tc, int gofi1, int gofi2 )  const
{
TGoF ( *this, gofi1, gofi2 ).AllTracksAreCompatible ( tc );
}


//----------------------------------------------------------------------------
                                        // Testing for the consistency of a group of files as frequencies, for all meaningful parameters
void    TGoF::AllFreqsAreCompatible   ( FreqsCompatibleClass& fc )  const
{
fc.Reset ();

CheckParameterCompatibility ( *this,    fc.NumTracks,                  ReadNumElectrodes              );
CheckParameterCompatibility ( *this,    fc.NumTF,                      ReadNumTimeFrames              );
CheckParameterCompatibility ( *this,    fc.NumFreqs,                   ReadNumFrequencies             );
CheckParameterCompatibility ( *this,    fc.SamplingFrequency,          ReadSamplingFrequency          );
CheckParameterCompatibility ( *this,    fc.OriginalSamplingFrequency,  ReadOriginalSamplingFrequency  );
CheckParameterCompatibility ( *this,    fc.FreqType,                   ReadFrequencyType              );    // NOT a FrequencyAnalysisType, because it can assign a CompatibilityFlags to variable
}


void    TGoGoF::AllFreqsAreCompatible ( FreqsCompatibleClass& fc, int gofi1, int gofi2 )    const
{
TGoF ( *this, gofi1, gofi2 ).AllFreqsAreCompatible ( fc );
}


//----------------------------------------------------------------------------
bool    TGoGoF::AllSplitGroupsAreCompatible ()   const
{
if ( NumGroups () == 0 )
    return  false;


if ( NumGroups () == 1 )
    return  true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TGoGoF              splitgogof;
TGoF                splitnames;     // should be TStrings, but GetCommonParts is currently part of TGoF
TGoF                splitnames0;
int                 numsplit;
int                 numsplit0;
TFileName           commonstart;
TFileName           commonend;


for ( int gofi = 0; gofi < NumGroups (); gofi++ ) {

                                  // Default to split by our epoch name 
    Group[ gofi ]->SplitGoFByNames ( "." InfixEpoch, splitgogof, &splitnames );

    numsplit    = (int) splitgogof;

                                        // removing the common parts from a given group, which should convey the subject's name/ID
                                        // !function is currently tailored for file names, so any trailing "." will be ignored in the common end!
    splitnames.GetCommonParts  (    0,  commonstart,    commonend,  0   );

    for ( int i = 0; i < (int) splitnames; i++ )

        StringClip  ( splitnames[ i ], commonstart.Length (), StringLength ( splitnames[ i ] ) - 1 - commonend.Length () );


//  splitgogof.Show ( "Splitting per epochs: files" );
//  splitnames.Show ( "Splitting per epochs: names" );


    if ( gofi == 0 ) {
        splitnames0 = splitnames;
        numsplit0   = numsplit;
        continue;
        }

                                        // first, we can test the number of groups
    if ( numsplit != numsplit0 )
        return  false;

                                        // we can now test a one-to-one correspondance in the names
                                        // !Off for the moment - users tend to mess up names!
//  if ( splitnames != splitnames0 )
//      return  false;
    }


return  true;
}


//----------------------------------------------------------------------------
                                        // Testing for the consistency of a group of files as inverse, for all meaningful parameters
void    TGoF::AllElectrodesAreCompatible ( ElectrodesCompatibleClass& ec )  const
{
ec.Reset ();

CheckParameterCompatibility ( *this,    ec.NumElectrodes,              ReadNumElectrodes              );
}


//void    TGoGoF::AllElectrodesAreCompatible ( ElectrodesCompatibleClass& ec, int gofi1, int gofi2 )
//{
//TGoF ( *this, gofi1, gofi2 ).AllElectrodesAreCompatible ( ec );
//}


//----------------------------------------------------------------------------
                                        // Testing for the consistency of a group of files as inverse, for all meaningful parameters
                                        // Optimizations seem to mess up with this method...
void    TGoF::AllInverseAreCompatible ( InverseCompatibleClass& ic )    const
{
ic.Reset ();

CheckParameterCompatibility ( *this,    ic.NumElectrodes,              ReadNumElectrodes              );
CheckParameterCompatibility ( *this,    ic.NumSolPoints,               ReadNumSolPoints               );
}


//void    TGoGoF::AllInverseAreCompatible ( InverseCompatibleClass& ic, int gofi1, int gofi2 )
//{
//TGoF ( *this, gofi1, gofi2 ).AllInverseAreCompatible ( ic );
//}


//----------------------------------------------------------------------------
                                        // Testing for the consistency of a group of files as inverse, for all meaningful parameters
void    TGoF::AllRoisAreCompatible ( RoisCompatibleClass& rc )    const
{
rc.Reset ();

CheckParameterCompatibility ( *this,    rc.NumElectrodes,               ReadNumElectrodes              );
rc.NumSolPoints     = rc.NumElectrodes;
rc.NumDimensions    = rc.NumElectrodes;
                                        // !will retrieve _consistent_ number of ROIs!
CheckParameterCompatibility ( *this,    rc.NumRois,                     ReadNumRois                    );
}


//void    TGoGoF::AllRoisAreCompatible ( RoisCompatibleClass& rc, int gofi1, int gofi2 )
//{
//TGoF ( *this, gofi1, gofi2 ).AllRoisAreCompatible ( rc );
//}


//----------------------------------------------------------------------------
                                        // Note that this will call ReadFromHeader functions
                                        // and that it will use the first session at hand!
                                        // So cases with multiple sessions are problematic...
int     TGoF::GetSumNumTF ( bool verbose )  const
{
int                 numtf;
int                 sumnumtf        = 0;


for ( int i = 0; i < (int) Strings; i++ ) {

    if ( ReadFromHeader ( Strings[ i ], ReadNumTimeFrames, &numtf ) )
        sumnumtf   += numtf;
    else if ( verbose )
        ShowMessage ( "Error while retrieving duration of file!", Strings[ i ], ShowMessageWarning );
    } // for fi


//DBGV3 ( gofi1, gofi2, maxnumtf, "fromgroup to group: #MaxTF" );

return  sumnumtf;
}


                                        // Same as above, but across multiple GoF
int     TGoGoF::GetSumNumTF ( int gofi1 , int gofi2, bool verbose ) const
{
if ( NumGroups () == 0 )
    return  0;


if ( ! CheckGroupIndexes ( gofi1, gofi2 ) )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int                 sumnumtf        = 0;


for ( int absg = gofi1; absg <= gofi2; absg++ )

    sumnumtf   += Group[ absg ]->GetSumNumTF ( verbose );


return  sumnumtf;
}


//----------------------------------------------------------------------------
                                        // Note that this will call ReadFromHeader functions
                                        // and that it will use the first session at hand!
                                        // So cases with multiple sessions are problematic...
int     TGoF::GetMaxNumTF ( bool verbose )  const
{
int                 numtf;
int                 maxnumtf        = 0;


for ( int i = 0; i < (int) Strings; i++ ) {

    if ( ReadFromHeader ( Strings[ i ], ReadNumTimeFrames, &numtf ) )
        Maxed ( maxnumtf, numtf );
    else if ( verbose )
        ShowMessage ( "Error while retrieving duration of file!", Strings[ i ], ShowMessageWarning );
    } // for fi


//DBGV3 ( gofi1, gofi2, maxnumtf, "fromgroup to group: #MaxTF" );

return  maxnumtf;
}

                                        // Same as above, but across multiple GoF
int     TGoGoF::GetMaxNumTF ( int gofi1 , int gofi2, bool verbose ) const
{
if ( NumGroups () == 0 )
    return  0;


if ( ! CheckGroupIndexes ( gofi1, gofi2 ) )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int                 maxnumtf        = 0;


for ( int absg = gofi1; absg <= gofi2; absg++ )

    Maxed ( maxnumtf, Group[ absg ]->GetMaxNumTF ( verbose ) );


return  maxnumtf;
}


//----------------------------------------------------------------------------

int     TGoF::GetMeanFileSize () const
{
if ( NumFiles () == 0 )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t              meansize        = 0;


for ( int i = 0; i < (int) Strings; i++ )

    meansize   += GetFileSize ( Strings[ i ] );

meansize   /= (int) Strings;


return  meansize;
}


//----------------------------------------------------------------------------
int     TGoF::GetNumElectrodes ()   const
{
                                        // get the sampling frequency from the first non-failing file
int                 numel               = 0;


for ( int fi = 0; fi < (int) Strings; fi++ )

    if ( ReadFromHeader ( Strings[ fi ], ReadNumElectrodes, &numel ) )
        break;


return  numel;
}


//----------------------------------------------------------------------------
double  TGoF::GetSamplingFrequency ()   const
{
                                        // get the sampling frequency from the first non-failing file
double              samplingfrequency   = 0;


for ( int fi = 0; fi < (int) Strings; fi++ )

    if ( ReadFromHeader ( Strings[ fi ], ReadSamplingFrequency, &samplingfrequency ) )
        break;


return  samplingfrequency;
}


double  TGoGoF::GetSamplingFrequency () const
{
                                        // get the sampling frequency from the first non-failing file
double              samplingfrequency   = 0;


for ( int gogofi = 0; gogofi < (int) Group; gogofi++ ) {

    samplingfrequency   = Group[ gogofi ]->GetSamplingFrequency ();

    if ( samplingfrequency > 0 )
        break;
    }


return  samplingfrequency;
}


//----------------------------------------------------------------------------
bool    TGoGoF::AllStringsGrep ( const char* regexp, GrepOption options, int gofi1, int gofi2 )   const
{
return  TGoF ( *this, gofi1, gofi2 ).AllStringsGrep ( regexp, options );
}


//----------------------------------------------------------------------------
                                        // Takes a range of GoF, with identical number of files in each group
                                        // and reduce the names to the most meaningful part, ditching post-processing stuff
bool    TGoGoF::SimplifyFilenames   (   int         gofi1,      int         gofi2, 
                                        TGoF&       gof
                                    )   const
{
gof.Reset ();

if ( IsEmpty () )
    return  false;

if ( ! CheckGroupIndexes ( gofi1, gofi2 ) )
    return  false;

if ( ! AreGroupsEqual ( gofi1, gofi2 ) )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numgroups           = gofi2- gofi1 + 1;
int                 maxfilespergroup    = GetMaxFilesPerGroup ( gofi1, gofi2 );
TFileName           buff;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // backup names if anything goes bad
TGoF                backup;

for ( int subji = 0; subji < maxfilespergroup; subji++ ) {
                                        // just an index to current file
    StringCopy      ( buff,     InfixSubject, IntegerToString ( subji + 1, NumIntegerDigits ( maxfilespergroup ) ) );
                                        // original file name?
//  StringCopy      ( buff, (*Group[ 0 ])[ subji ] );
//  GetFilename     ( buff );

    backup.Add ( buff );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // some (relative) TGoGoF
TGoGoF              gogof ( numgroups );

                                        // loop all local groups and files
for ( int relg = 0, absg = gofi1; relg < numgroups; relg++, absg++ )

for ( int subji = 0; subji < maxfilespergroup; subji++ ) {
                                        // short file name
    StringCopy      ( buff, (*Group[ absg ])[ subji ] );

    GetFilename     ( buff );

    gogof[ relg ].Add ( buff );
    } // for relg, subj

//gogof[ 0 ].Show ( "Original" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here working on gogof
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Simplifying WITHIN subject, keeping the common part
if ( numgroups > 1 ) {

    TGoF                subj;
    TFileName           commonstart;

    for ( int subji = 0; subji < maxfilespergroup; subji++ ) {

        subj.Reset ();

        for ( int relg = 0; relg < numgroups; relg++ )

            subj.Add ( gogof[ relg ][ subji ] );

                                        // let's grab all the common beginning for one subject across multiple conditions
        subj.GetCommonParts  (  0,  commonstart,    0,  0   );

                                        // this could happen if files have nothing in common at their beginning - it shouldn't in our case
        gof.Add ( StringIsEmpty ( commonstart ) ? backup[ subji ] : commonstart );
        } // for subj

//    gof.Show ( "Simpl1" );
    }

else

    for ( int subji = 0; subji < maxfilespergroup; subji++ )

        gof.Add ( gogof[ 0 ][ subji ] );


gogof.Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here working on gof only
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Simplifying across subjects WITHIN A GROUP, keeping the differences
TFileName           commonstart;
TFileName           commonend;
TStrings            diffs;
    
                                        // let's grab what differs across all subjects
gof.GetCommonParts  ( 0, commonstart, commonend,  &diffs  );

                                        // by safety removing trailing spaces
for ( int subji = 0; subji < maxfilespergroup; subji++ )

    StringCleanup ( diffs[ subji ] );


//commonstart.Show ( "commonstart"); commonend.Show ( "commonend" ); diffs.Show ( "diffs" );


for ( int subji = 0; subji < maxfilespergroup; subji++ )

    StringCopy ( gof[ subji ], commonstart, diffs[ subji ] ); // ?using commonstart or not?

//gof.Show ( "Simpl2" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Simplifying across subjects WITHIN A GROUP, removing any duplicated sub-strings

                                        // extract all tokens that can be found in all files
char                filetokensseparators[ 256 ] = " \t_-.,;:/+";
TSplitStrings       splitunique;
TSplitStrings       splitnonunique;


for ( int subji = 0; subji < maxfilespergroup; subji++ ) {

    splitunique   .Add ( gof[ subji ], UniqueStrings,    filetokensseparators );

    splitnonunique.Add ( gof[ subji ], NonUniqueStrings, filetokensseparators );
    }

//splitunique.Show ();


                                        // compute histogram on all (duplicated) tokens
unordered_map< const char*, int >   histosplit;

for ( int i = 0; i < splitnonunique.GetNumTokens (); i++ )
for ( int j = 0; j < splitunique   .GetNumTokens (); j++ )

    if ( StringIs ( splitunique[ j ], splitnonunique[ i ] ) ) {
                                        // !we have to use the non-unique strings for the histogram!
        ++histosplit[ splitunique[ j ] ];

        break;
        }

                                    // for debugging
//for ( auto mapi = histosplit.begin (); mapi != histosplit.end (); mapi++ )
//    if ( mapi->second == 1 )
//        DBGV ( mapi->second, mapi->first );



char                strgrep[ 1024 ];
TStringGrep         grepy;
TStrings            grepresults;

StringGrepNeutral   ( filetokensseparators );

/*                                        // deleting is working, but we still have a trail of bullshit chars
                                        // cleaning-up repeated tokens
for ( auto mapi = histosplit.begin (); mapi != histosplit.end (); mapi++ )
                                        // repeating tokens are suspicious
    if ( mapi->second > 1 ) {

//      DBGV ( mapi->second, mapi->first );

        StringCopy      ( strgrep, "(" );
        StringAppend    ( strgrep, "^", mapi->first, "[", filetokensseparators, "]+" ); // beginning token
        StringAppend    ( strgrep, "|" );                                               // XOR
        StringAppend    ( strgrep, "[", filetokensseparators, "]+", mapi->first, "$" ); // ending token
        StringAppend    ( strgrep, ")" );

        grepy.Set ( strgrep, GrepOptionDefaultFiles );


        for ( int subji = 0; subji < maxfilespergroup; subji++ ) {
                                        // replace string with nothing - !note that any separator will remain, though!
            //StringReplace   ( gof[ subji ], mapi->first, "" or "-" );

            if ( grepy.Matched ( gof[ subji ], &grepresults ) ) {

//              grepresults.Show ( gof[ subji ] );
                                        // delete first & single occurence
                char*       toc     = StringContains ( gof[ subji ], grepresults[ 0 ] );

                MoveVirtualMemory ( toc, toc + StringLength ( buff ), StringSize ( gof[ subji ] ) - StringLength ( buff ) );
                // or use StringClip  ( toc, 0, length );
                }
            }
        }
*/
                                        // cleaning-up clip PAST any non-repeating, supposedly meaningful, token
                                        // anything BEFORE will remain, though, even if repeating, as the prefix might convey some meaning in itself
for ( auto mapi = histosplit.begin (); mapi != histosplit.end (); mapi++ )
                                        // focus on keys
    if ( mapi->second == 1 ) {

//      DBGV ( mapi->second, mapi->first );

        StringCopy      ( strgrep, "(^|[", filetokensseparators, "]+)" );   // optional beginning separator
        StringAppend    ( strgrep, "(",    mapi->first,            ")" );   // our token!
        StringAppend    ( strgrep, "($|[", filetokensseparators, "]+)" );   // optional ending separator

        grepy.Set ( strgrep, GrepOptionDefaultFiles );


        for ( int subji = 0; subji < maxfilespergroup; subji++ ) {

            if ( grepy.Matched ( gof[ subji ], &grepresults ) >= 4 )
                                        // cut string past current item
                StringClip  ( const_cast<char*> ( grepy.GetMatchStart ( 3 ) ), 0 );
            }
        }

//gof.Show ( "Simpl3" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Final check for empty strings, reverting to default strings
for ( int relg = 0; relg < numgroups; relg++ )
for ( int subji = 0; subji < maxfilespergroup; subji++ )

    if ( StringIsEmpty ( gof[ subji ] ) )

        StringCopy ( gof[ subji ], backup[ subji ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//gof.Show ( "Final" );

return  true;
}


//----------------------------------------------------------------------------
                                        // resample a group of files into another resampled group of files
enum            FilesProjectedEnum
                {
                FileIndex,
                TFIndex,
                NumFilesProjectedEnum
                };


void    TGoF::ResampleFiles (   ResamplingType  resampling,     int         numresamples,   int         resamplingsize,
                                const TGoF*     gofalt,
                                TGoGoF&         resgogof,       TGoGoF*     resgogofalt,
                                ExecFlags       execflags
                            )
{
resgogof.Reset ();
if ( resgogofalt )  resgogofalt->Reset ();


if ( IsEmpty () 
  || resampling     == NoTimeResampling 
  || numresamples   <= 0 
  || resamplingsize <= 0 )

    return;

                                        // testing the number of files - could also test the number of time frames
if ( gofalt && gofalt->NumFiles () != NumFiles () )
    gofalt  = 0;


if ( resgogofalt && ! gofalt )
    resgogofalt = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Project all time frames continuously
int                 numfiles        = NumFiles    ();
int                 totalnumtf      = GetSumNumTF ();
int                 numel           = GetNumElectrodes ();
int                 numsp           = gofalt ? gofalt->GetNumElectrodes () : 0; // works for both electrodes or solution points
TArray2<int>        mappedtf        ( totalnumtf, NumFilesProjectedEnum );      // map all files into a linear time line


int                 numtf;
int                 tfc             = 0;

for ( int fi = 0; fi < numfiles; fi++ ) {

    if ( ! ReadFromHeader ( Strings[ fi ], ReadNumTimeFrames, &numtf ) )
                                        // shouldn't happen - just ignoring file should be fine
        continue;

                                        // fill projection
    for ( int tf0 = 0; tf0 < numtf; tf0++, tfc++ ) {

        mappedtf ( tfc, FileIndex   ) = fi;     // file index
        mappedtf ( tfc, TFIndex     ) = tf0;    // relative TF index
        }

    } // for fi


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // File names
TFileName           basefilename;
TFileName           filenameout;
TFileName           filenameoutalt;
char                ext     [ 64 ];
char                extalt  [ 64 ];
char                buff[ 256 ];
bool                allris              = AllExtensionsAre ( AllRisFilesExt );


//StringCopy      ( basefilename, Strings[ 0 ] );
//::RemoveFilename( basefilename, true );
//StringAppend    ( basefilename, PostfixResampled, ".", StringRandom ( buff, 4 ), "." );
                                        // Re-use first file part - maybe in case of single input file?
StringCopy        ( basefilename, Strings[ 0 ] );
crtl::RemoveExtension ( basefilename );
StringAppend      ( basefilename, ".", PostfixResampled, "." );


StringCopy      ( ext,          allris ? FILEEXT_RIS : FILEEXT_EEGSEF );
StringCopy      ( extalt,     ! allris ? FILEEXT_RIS : FILEEXT_EEGSEF );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Do the multiple-files resampling
//#define           MaxRatioToRandomize     0.75
//int               resampsize          = NoMore ( Round ( totalnumtf * MaxRatioToRandomize ), resamplingsize );    // !can override caller resampling size: no more than 75% of data!
int                 resampsize          = NoMore ( totalnumtf, resamplingsize );    // trust caller - currently only from segmentation, which is throughly checked
TVector<int>        randindex;
TMarkers*           writingepochslist   = new TMarkers[ numfiles ];     // we need as many markers list as input file
int                 randfile;
int                 randtf0;
TMaps               Data;
TMaps               DataAlt;
TMaps               ConcatData      ( resampsize, numel );
TMaps               ConcatDataAlt   ( resampsize, numsp );
int                 tfconcat;
//TMaps               ESI;
TStrings            tracksnames;
TStrings            tracksnamesalt;
TRandUniform        randunif;


for ( int rsi = 0; rsi < numresamples; rsi++ ) {


    for ( int fi = 0; fi < numfiles; fi++ )

        writingepochslist[ fi ].ResetMarkers ();


    randindex.RandomSeries ( resampsize, totalnumtf, &randunif );


    for ( int i = 0; i < resampsize; i++ ) {
                                        // unfold projection
        randfile    = mappedtf ( randindex[ i ], FileIndex );
        randtf0     = mappedtf ( randindex[ i ], TFIndex   );

                        // assign to this file's list,         and TF within that file
        writingepochslist[ randfile ].AppendMarker ( TMarker ( randtf0, randtf0, 0, "RandMarker" /*TagNameBlock*/, MarkerTypeTemp ) );

        } // for each sample

                                        // let's be consistent and writing in the same order as the input
    for ( int fi = 0; fi < numfiles; fi++ ) {

        writingepochslist[ fi ].SortAndCleanMarkers ();
                                        // does it help?        
//      writingepochslist[ fi ].CompactConsecutiveMarkers ( false, 1000 /*MAXLONG*/ ) ) {
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    tfconcat    = 0;


    for ( int fi = 0; fi < numfiles; fi++ ) {

        if ( writingepochslist[ fi ].IsEmpty () )
            continue;

                                        // !reading only the interesting part! -> multiple reads, less memory instead of "1" big read, more memory
        Data.ReadFile   ( Strings[ fi ], 0, AtomTypeScalar /*datatype*/, ReferenceAsInFile, writingepochslist[ fi ], tracksnames.IsEmpty () ? &tracksnames : 0 );

        if ( gofalt )
            DataAlt.ReadFile    ( (*gofalt)[ fi ], 0, AtomTypeScalar /*datatype*/, ReferenceAsInFile, writingepochslist[ fi ], tracksnamesalt.IsEmpty () ? &tracksnamesalt : 0 );

                                        // Concatenate these maps
        for ( int tf0 = 0; tf0 < Data.GetNumMaps (); tf0++, tfconcat++ ) {

            ConcatData      [ tfconcat ]    = Data      [ tf0 ];

            if ( gofalt )
                ConcatDataAlt   [ tfconcat ]    = DataAlt   [ tf0 ];
            }

        } // for file


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set only 1 output resampled file
    StringCopy          ( filenameout,  basefilename, IntegerToString ( buff, rsi + 1, NumIntegerDigits ( numresamples ) ) );
    AddExtension        ( filenameout,  ext );

    if ( IsNoOverwrite ( execflags ) )
        CheckNoOverwrite    ( filenameout );

                                        // we have a new TGoF at each resampling - each TGoF then has the same number of files as the input TGoF
    resgogof.Add ( new TGoF );
                                        // copying resulting file name
    resgogof[ rsi ].Add ( filenameout );

                                        // making the resampled file a shadow of the main file - even if original files were not
    if ( gofalt ) {
        StringCopy          ( filenameoutalt,   filenameout );
        crtl::ReplaceExtension  ( filenameoutalt,   extalt );
        }

    if ( resgogofalt ) {
        resgogofalt->Add ( new TGoF );

        (*resgogofalt)[ rsi ].Add ( filenameoutalt );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // write single file
    ConcatData.WriteFile    (   filenameout, 
                                false, 
                                0, 
                                &tracksnames
                            );

    if ( gofalt )
        ConcatDataAlt.WriteFile (   filenameoutalt, 
                                    false, 
                                    0, 
                                    &tracksnamesalt
                                );

    } // for numresamples


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

delete[]    writingepochslist;
}


//----------------------------------------------------------------------------
                                        // Complex data will generate 2 files, one real and one imaginary
                                        // Real (FFT Approximation) data will have the spectrum as the squared input
                                        // ?TODO: if file is Complex format, read complex & save norm^2 ?
void    TGoF::SplitFreqFiles    (   SplitFreqFlags  how, 
                                    TGoGoF*         gogofout, 
                                    ExecFlags       execflags   )   const
{
if ( gogofout )
    gogofout->Reset ();


if ( IsEmpty () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge     Gauge;

if ( IsInteractive ( execflags ) ) {

    if      ( how == SplitFreqByFrequency ) {
        Gauge.Set       ( "Split FREQ to Frequencies" );
        Gauge.AddPart   ( 0, NumFiles () + 1 );
        }
    else if ( how == SplitFreqByElectrode ) {
        Gauge.Set       ( "Split FREQ to Electrodes" );
        Gauge.AddPart   ( 0, NumFiles () + 1 );
        }
    else if ( how == SplitFreqByTime ) {
        Gauge.Set       ( "Split FREQ to Spectrum" );
        Gauge.AddPart   ( 0, NumFiles () + 1 );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           filemrkin;
TFileName           filemrkout;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < NumFiles (); i++ ) {

    TOpenDoc<TFreqDoc>  FreqDoc;

                                        // add a TGoF to follow the exact same structure as input
    if ( gogofout )
        gogofout->Add ( new TGoF );


    if ( ! FreqDoc.Open ( (*this)[ i ], OpenDocHidden ) )
        continue;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    int             numel           = FreqDoc->GetNumElectrodes  ();
    long            numtf           = FreqDoc->GetNumTimeFrames  ();
    int             numfreqs        = FreqDoc->GetNumFrequencies ();

    bool            isatomcomplex   = FreqDoc->IsComplex ( AtomTypeUseOriginal );
    bool            isatomscalar    = ! isatomcomplex;
    int             atomsize        = isatomscalar ? 1 : 2;

    FrequencyAnalysisType freqtype  = FreqDoc->GetFreqType ();
    const char*     fileext         = freqtype == FreqESI ? FILEEXT_RIS : FILEEXT_EEGSEF;   // could be useful


    TSetArray2<float>   realpart    ( numfreqs, numel, numtf );
    TSetArray2<float>   imagpart    ( isatomcomplex ? numfreqs : 0, isatomcomplex ? numel : 0, isatomcomplex ? numtf : 0 );
                                        // there might be more than 1 part, and we want to use the same loop
    int             numparts        = atomsize;

    int             tfwidth         = NumIntegerDigits ( numtf );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // assume all files have the same number of frequencies...
    if ( i == 0 && IsInteractive ( execflags ) ) {

        if      ( how == SplitFreqByFrequency )     Gauge.SetRange ( 0, NumFiles () * numfreqs * numparts );
        else if ( how == SplitFreqByElectrode )     Gauge.SetRange ( 0, NumFiles () * numel    * numparts );
        else if ( how == SplitFreqByTime      )     Gauge.SetRange ( 0, NumFiles () * numtf    * numparts );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    TExportTracks   exptracks;


    if      ( how == SplitFreqByFrequency ) {

        exptracks.SetAtomType ( AtomTypeScalar );
        exptracks.NumTracks         = numel;
        exptracks.NumTime           = numtf;
        exptracks.SamplingFrequency = FreqDoc->GetSamplingFrequency ();
        exptracks.ElectrodesNames   = *FreqDoc->GetElectrodesNames ();
        exptracks.DateTime          = FreqDoc->DateTime;
        }

    else if ( how == SplitFreqByElectrode ) {

        exptracks.SetAtomType ( AtomTypeScalar );
        exptracks.NumTracks         = numfreqs;
        exptracks.NumTime           = numtf;
        exptracks.SamplingFrequency = FreqDoc->GetSamplingFrequency ();
        exptracks.ElectrodesNames   = *FreqDoc->GetFrequenciesNames ();
        exptracks.DateTime          = FreqDoc->DateTime;
        }

    else if ( how == SplitFreqByTime ) {

        exptracks.SetAtomType ( AtomTypeScalar );
        exptracks.NumTracks         = numel;
        exptracks.NumTime           = numfreqs;
        exptracks.ElectrodesNames   = *FreqDoc->GetElectrodesNames ();
                                        // set an offset from the first frequency name
        double          firstfreq   = StringToDouble ( FreqDoc->GetFrequencyName ( 0 ) );
        exptracks.DateTime          = TDateTime ( 0, 0, 0, 0, 0, 0, 0, firstfreq * 1e6 );
                                        // estimate the "sampling frequency", i.e. the saved frequency steps (if linear)
                                        // because the saved frequency steps doesn't match the original sampling frequency
        if ( FreqDoc->GetNumFrequencies () >= 2 ) {
            double          lastfreq    = StringToDouble ( FreqDoc->GetFrequencyName ( FreqDoc->GetNumFrequencies () - 1 ) );
                                        // this will not match exactly, but is still helpful
            exptracks.SamplingFrequency = ( lastfreq - firstfreq ) / (double) ( FreqDoc->GetNumFrequencies () - 1 );
            }
        }
                                        // Speed up things with an appropriate transposed buffer
    TArray2<float>      buffer      ( exptracks.NumTime, exptracks.NumTracks );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    StringCopy   ( filemrkin, (*this)[ i ] );
    AddExtension ( filemrkin, FILEEXT_MRK );
    filemrkin.SetExtendedPath ();


    if ( ! CanOpenFile ( filemrkin ) )
        ClearString ( filemrkin );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // in case frequency file is big, read by blocks
    if ( numparts == 2 )    FreqDoc->ReadFrequencies ( 0, numtf - 1, realpart, imagpart );
    else                    FreqDoc->ReadFrequencies ( 0, numtf - 1, realpart           );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if      ( how == SplitFreqByFrequency ) {

        for ( int freq = 0; freq < numfreqs; freq++ )
                                        // then split to 1 or 2 files per freq
        for ( int part = 0; part < numparts; part++ ) {

            Gauge.Next ( 0 );


            exptracks.Filename  = (*this)[ i ];

            exptracks.Filename.RemoveExtension ();

            exptracks.Filename += "." + TStringValue ( FreqDoc->GetFrequencyName ( freq ) );
                                        // check for buggy "Hz"
    //      if ( StringEndsWith ( exptracks.Filename, " H" ) )
    //          exptracks.Filename += "z";

            if ( isatomcomplex )
                exptracks.Filename += "." + TStringValue ( part == 0 ? InfixReal : InfixImag );

            exptracks.Filename.AddExtension ( fileext );
                                        // 2 consecutive equal freq names?
    //      if ( freq && StringIs ( FreqDoc->GetFrequencyName ( freq ), FreqDoc->GetFrequencyName ( freq - 1 ) ) )
    //          ReplaceExtension    ( OutFileName, FILEEXT_EEGSEF );

            if ( IsNoOverwrite ( execflags ) )
                exptracks.Filename.CheckNoOverwrite ();


            if ( gogofout )
                (*gogofout)[ i ].Add ( exptracks.Filename );

                                        // any marker file?
            if ( StringIsNotEmpty ( filemrkin ) ) {
                filemrkout  = exptracks.Filename;
                filemrkout.AddExtension ( FILEEXT_MRK );
                CopyFileExtended        ( filemrkin,  filemrkout );
                }


            for ( long tf0 = 0; tf0 < numtf; tf0++ )
            for ( int  e   = 0; e   < numel;   e++ )
                                        // pick the right buffer
                buffer ( tf0, e )   = part == 0 ? realpart ( freq, e, tf0 ) : imagpart ( freq, e, tf0 );
    

            exptracks.Write ( buffer, NotTransposed );
            } // for freq & part

        } // if SplitFreqByFrequency


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    else if  ( how == SplitFreqByElectrode ) {

        for ( int  e   = 0; e   < numel;    e++   )
                                        // then split to 1 or 2 files per freq
        for ( int part = 0; part < numparts; part++ ) {

            Gauge.Next ( 0 );


            exptracks.Filename  = (*this)[ i ];

            exptracks.Filename.RemoveExtension ();

            exptracks.Filename += "." + TStringValue ( FreqDoc->GetElectrodeName ( e ) );
                                        // check for buggy "Hz"
    //      if ( StringEndsWith ( exptracks.Filename, " H" ) )
    //          exptracks.Filename += "z";

            if ( isatomcomplex )
                exptracks.Filename += "." + TStringValue ( part == 0 ? InfixReal : InfixImag );

            exptracks.Filename.AddExtension ( /*fileext*/ FILEEXT_EEGSEF );

            if ( IsNoOverwrite ( execflags ) )
                exptracks.Filename.CheckNoOverwrite ();


            if ( gogofout )
                (*gogofout)[ i ].Add ( exptracks.Filename );

                                        // any marker file?
            if ( StringIsNotEmpty ( filemrkin ) ) {
                filemrkout  = exptracks.Filename;
                filemrkout.AddExtension ( FILEEXT_MRK );
                CopyFileExtended        ( filemrkin,  filemrkout );
                }


            for ( long tf0 = 0; tf0 < numtf;      tf0++ )
            for ( int freq = 0; freq < numfreqs; freq++ )
                                        // !frequency will appear "inverted", low frequencies on top, but coherent with frequency names!
                                        // pick the right buffer
                buffer ( tf0, freq )    = part == 0 ? realpart ( freq, e, tf0 ) : imagpart ( freq, e, tf0 );


            exptracks.Write ( buffer, NotTransposed );
            } // for freq & part

        } // if SplitFreqByElectrode


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    else if  ( how == SplitFreqByTime ) { // i.e. Spectrum

        for ( long tf0 = 0; tf0 < numtf; tf0++ )
                                        // then split to 1 or 2 files per freq
        for ( int part = 0; part < numparts; part++ ) {

            Gauge.Next ( 0 );


            exptracks.Filename  = (*this)[ i ];

            exptracks.Filename.RemoveExtension ();

            exptracks.Filename += "." InfixSpectrum " " + IntegerToString ( tf0 + 1, tfwidth );

            if ( isatomcomplex )
                exptracks.Filename += "." + TStringValue ( part == 0 ? InfixReal : InfixImag );

            exptracks.Filename.AddExtension ( /*fileext*/ FILEEXT_EEGSEF );

            if ( IsNoOverwrite ( execflags ) )
                exptracks.Filename.CheckNoOverwrite ();


            if ( gogofout )
                (*gogofout)[ i ].Add ( exptracks.Filename );

                                        // any marker file?
            if ( StringIsNotEmpty ( filemrkin ) ) {
                filemrkout  = exptracks.Filename;
                filemrkout.AddExtension ( FILEEXT_MRK );
                CopyFileExtended        ( filemrkin,  filemrkout );
                }


            for ( int freq = 0; freq < numfreqs; freq++ )
            for ( int e    = 0; e    < numel;    e++    ) {

                double      v;

                if      ( FreqDoc->IsFreqTypeFFTApproximation () )  v   = Square ( realpart ( freq, e, tf0 ) ); // we could also output fabs(fftapprox)
                else if ( part == 0 )                               v   = realpart ( freq, e, tf0 );
                else                                                v   = imagpart ( freq, e, tf0 );

                buffer ( freq, e )  = v;
                }


            exptracks.Write ( buffer, NotTransposed );
            } // for freq & part

        } // if SplitFreqByTime


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    FreqDoc.Close ();
    } // for file


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//if ( IsInteractive ( execflags ) )  Gauge.HappyEnd ();    // takes too much time when called from other processing
}


//----------------------------------------------------------------------------
                                        // Try to group this big GoF into smaller GoF
int     TGoF::SplitGoFByNames   (   const char*     splitwith, 
                                    TGoGoF&         gogofout,   TStrings*       groupnames 
                                )   const
{
gogofout.Reset ();

if ( groupnames )
    groupnames->Reset ();

if ( IsEmpty () )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Splitting per directory
TGoF                dironly ( *this );
                                        // keeping only the directory part
dironly.RemoveFilename ();


TStrings            dirgroup;           // list of unique directories


gogofout.Reset ();

for ( int gofi = 0; gofi < (int) dironly; gofi++ ) {

    bool        found       = false;

    for ( int dirgroupi = 0; dirgroupi < (int) dirgroup; dirgroupi++ )

        if ( StringIs ( dironly[ gofi ], dirgroup[ dirgroupi ] ) ) {
                                        // found it, but store the real file name!
            gogofout[ dirgroupi ].Add ( (*this)[ gofi ] );
            found   = true;
            break;
            }

    if ( found )    continue;

                                        // store only the directory part
    dirgroup     .Add ( dironly[ gofi ] );
                                        // new GoF
    gogofout     .Add ( new TGoF );
                                        // found it, but store the real file name!
    gogofout[ (int) gogofout - 1 ].Add ( (*this)[ gofi ] );
    }
        

//gogofout.Show ( "Splitting per directory" );

                                        // assume we did the job here if splitting per directory has more than 1 family
if ( gogofout.NumGroups () > 1 ) {

    if ( groupnames )
        groupnames->Set ( dirgroup );

    return  gogofout.NumGroups ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Split per beginning of file name, before a specified magic string
TGoF                filenameonly ( *this );
                                        // keeping only the filename part - extension shouldn't matter here
filenameonly.RemoveDir ();


char*               tocut;

for ( int gofi = 0; gofi < (int) filenameonly; gofi++ ) {
           // you saw nothing...
    tocut   = const_cast<char*> ( StringContains  ( filenameonly[ gofi ], splitwith ) );
                                        // hopefully always the case...
    if ( tocut )
        *tocut  = EOS;
    }


TStrings            namegroup;          // list of unique beginning


gogofout.Reset ();

for ( int gofi = 0; gofi < (int) filenameonly; gofi++ ) {

    bool        found       = false;

    for ( int namegroupi = 0; namegroupi < (int) namegroup; namegroupi++ )

        if ( StringIs ( filenameonly[ gofi ], namegroup[ namegroupi ] ) ) {
                                        // found it, but store the real file name!
            gogofout[ namegroupi ].Add ( (*this)[ gofi ] );
            found   = true;
            break;
            }

    if ( found )    continue;

                                        // store only the directory part
    namegroup    .Add ( filenameonly[ gofi ] );
                                        // new GoF
    gogofout     .Add ( new TGoF );
                                        // found it, but store the real file name!
    gogofout[ (int) gogofout - 1 ].Add ( (*this)[ gofi ] );
    }
        
                                        // returns with what we have at that point
if ( groupnames )
    groupnames->Set ( namegroup );

return  gogofout.NumGroups ();
}


//----------------------------------------------------------------------------
                                        // Splitting a set of files into smaller epoch files
                                        // Epochs can have any durations
                                        // Will always produce new temp files, so that caller can delete them at will
                                        // !Only scalar for the moment!
void    TGoF::SplitEpochsFiles  (   const char*     greppedwith,
                                    int             maxepochs,
                                    const char*     outputdir,
                                    const char*     prefix,
                                    TGoF&           gofout,
                                    ExecFlags       execflags
                                )   const
{
gofout.Reset ();

if ( IsEmpty () )
    return;

if ( maxepochs <= 0 )
    maxepochs   = Highest ( maxepochs );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Writing all epochs of all files into a single directory - this helps cleaning-up later on
TFileName           tempdir;
TFileName           subdir;


GetTempFileName     ( tempdir );

tempdir = (TFileName) prefix + "SplitEpochs." + tempdir;

tempdir.RemoveExtension ();

subdir  = outputdir;

AppendDir           ( subdir, false, tempdir );
                                    // works also for directory
if ( IsNoOverwrite ( execflags ) )
    subdir.CheckNoOverwrite ();

CreatePath          ( subdir, false );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           mrkfilename;
TMarkers            onetaglist;
TMarker             oneepoch;
TStringGrep         grepepoch ( InfixEpochConcatGrep, GrepOptionDefault );  // !handling file names ourselves here!
TStrings            matched;


for ( int i = 0; i < (int) Strings; i++ ) {

                                        // Read data now
//  TStrings            tracksnames;
                                        // these are temp files, we don't care for names
    TMaps               maps ( Strings[ i ], 0, AtomTypeScalar, ReferenceAsInFile/*, &tracksnames*/ );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Reading markers
    StringCopy          ( mrkfilename, Strings[ i ], "." FILEEXT_MRK );
                                        // we have no session issues here, so we can load the marker file directly
    TMarkers            eegmarkers ( mrkfilename );

                                        // Keeping markers according to caller's request
    eegmarkers.KeepMarkers ( greppedwith );

                                        // !Removing markers with identical position!
    eegmarkers.SortAndCleanMarkers ( false );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check what remains after filtering the markers
    if ( eegmarkers.IsEmpty () ) {
                                        // Oops, there is nothing - what do?

                                        // Option 1: duplicating the file
        TFileName           copyfilename    = Strings[ i ];

        copyfilename.ReplaceDir ( subdir );
                                        // when not relocating, we need to avoid overwriting
//      crtl::RemoveExtension   ( copyfilename );
//
//      StringAppend        ( copyfilename,  ".Copy" );
//
//      AddExtension        ( copyfilename,  FILEEXT_EEGSEF );

        copyfilename.ReplaceExtension ( FILEEXT_EEGSEF );

        if ( IsNoOverwrite ( execflags ) )
            copyfilename.CheckNoOverwrite();


        maps.WriteFile      ( copyfilename, false, maps.GetSamplingFrequency () );


        gofout.Add          ( copyfilename );

        continue;

                                        // Option 2: fail everything and return
//      gofout.DeleteFiles ( true );
//
//      gofout.Reset ();
//
//      return;
        } // empty eegmarkers


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // For automatic, cyclical markers
//    TMarkers          markers;
//    int               epochlength     = eegmarkers[ 1 ]->From - eegmarkers[ 0 ]->From;
//    markers.GeneratePeriodicEpochs ( 0, maps.GetNumMaps () - 1, epochlength, MarkerNameAutoEpochs );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Markers exist here
    oneepoch.Type   = MarkerTypeTemp;
    StringCopy  ( oneepoch.Name, MarkerNameEpoch );


    for ( int markeri = 0; markeri < eegmarkers.GetNumMarkers () && markeri < maxepochs; markeri++ ) {

                                        // starting from marker beginning
        oneepoch.From   = eegmarkers[ markeri     ]->From;

                                        // ending depends on current marker
        oneepoch.To     = eegmarkers[ markeri     ]->IsExtended ()  ? eegmarkers[ markeri     ]->To         // if extended, use marker's span
                        : markeri < eegmarkers.GetNumMarkers () - 1 ? eegmarkers[ markeri + 1 ]->From - 1   // if not extended, use next marker's beginning
                        :                                             maps.GetNumMaps () - 1;

        onetaglist.ResetMarkers ();

        onetaglist.AppendMarker    ( oneepoch );

                                        // Epoch file name
        TFileName           onefilename     = Strings[ i ];

        onefilename.ReplaceDir  ( subdir );
                                        // replace any existing epochs name
        if ( grepepoch.Matched   ( ToFileName ( onefilename ), &matched ) )
            StringReplace   ( ToFileName ( onefilename ), matched ( 0 ), "." );

                                        // cooking a new epoch name
        TFileName           epochname   = "." InfixEpoch " " + IntegerToString ( markeri + 1, NumIntegerDigits ( eegmarkers.GetNumMarkers () ) );

        PostfixFilename     ( onefilename,  epochname );

        if ( IsNoOverwrite ( execflags ) )
            onefilename.CheckNoOverwrite ();


        maps.WriteFileEpochs( onefilename, false, maps.GetSamplingFrequency (), onetaglist/*, &tracksnames*/ );

                                        // this should be a TGoGoF, but the caller will manage a single group
        gofout.Add          ( onefilename );
        } // for markeri

    } // for file

}


//----------------------------------------------------------------------------
                                        // Split a list of files into directory, common parts at beginning and ending of the file names, and the remaining differences
                                        // All of them being optionals
                                        // Case insensitive comparison
void    TGoF::GetCommonParts    (   char*           commondir,
                                    char*           commonstart,
                                    char*           commonend,
                                    TStrings*       diffs
                                )   const
{
ClearString     ( commondir     );
ClearString     ( commonstart   );
ClearString     ( commonend     );
if ( diffs )    diffs->Reset ();


if ( ! ( commondir || commonstart || commonend || diffs ) )
    return;


if ( IsEmpty () )
    return;

                                        // working with a copy
TGoF                str ( *this );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Use the first directory - we can not rely on all files being in the same directory
if ( commondir )

    StringCopy      ( commondir, TFileName ( str[ 0 ], (TFilenameFlags) ( TFilenameExtendedPath | TFilenameToDirectory ) ) );

                                        // now we can remove the directory parts
str.RemoveDir ();

                                        // and the extension parts
str.RemoveExtension ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // only 1 file?
if ( (int) str == 1 ) {
                                        // this will do - the others remain empty
    StringCopy      ( commonstart, str[ 0 ] );

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here at least 2 strings
int                 numstrings      = (int) str;
int                 minlength       = str.GetMinStringLength ();

                                        // limit case: some string is empty
if ( minlength == 0 )
                                        // end of the story
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do the scan for minlength characters
int                 commonstartlength   = 0;
int                 i;

                                        // scan each letter forward
for ( int l = 0; l < minlength; l++ ) {
                                        // scan each string
    for ( i = 1; i < numstrings; i++ )

//      if ( str[ i ][ l ] != str[ 0 ][ l ] )
        if ( toupper ( str[ i ][ l ] ) != toupper ( str[ 0 ][ l ] ) )
            break;                      // fail


    if ( i == numstrings )  commonstartlength++;    // all files matched current char
    else                    break;                  // stop now
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scan each letter backward - !LastChar offsets start from 1!
int                 commonendlength     = 0;


for ( int l = 1; l <= minlength; l++ ) {
                                        // scan each string
    for ( i = 1; i < numstrings; i++ )

//      if ( *LastChar ( str[ i ], l ) != *LastChar ( str[ 0 ], l ) )
        if ( toupper ( *LastChar ( str[ i ], l ) ) != toupper ( *LastChar ( str[ 0 ], l ) ) )
            break;                      // fail


    if ( i == numstrings )  commonendlength++;      // all files matched current char
    else                    break;                  // stop now
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // there they are:
if ( commonstart && commonstartlength > 0 )     StringCopy ( commonstart,               str[ 0 ],                       commonstartlength );

if ( commonend   && commonendlength   > 0 )     StringCopy ( commonend,     StringEnd ( str[ 0 ] ) - commonendlength,   commonendlength   );

                                        // if all names were identical, we would have this
if ( StringIs ( commonstart, commonend ) )
                                        // but there could only be one, sorry for you
    ClearString ( commonend );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // extra: returning the difference parts
if ( ! diffs )
    return;

                                        // extract difference parts, string by string
TFileName           part;
                     

for ( int i = 0; i < numstrings; i++ ) {
                                        // copy beginning, starting after the common part
    StringCopy  ( part, str[ i ] + StringLength ( commonstart ) );
                                        // clip ending
    StringClip  ( part, StringLength ( part ) - StringLength ( commonend ) );
                                        // storing all differences
    diffs->Add  ( part );
    }
}


//----------------------------------------------------------------------------
                                        // returns the constant part of a set of filenames
                                        // includes the directory
bool    TGoF::GetCommonString   (   char*   base,       bool    includedir,     bool    includedifference   )   const
{
ClearString ( base );

if ( IsEmpty () )
    return false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TFileName           buff;
TFileName           directory;
TFileName           match;
TFileName           sub;
long                minlength;
bool                found       = false;
bool                foundone;
int                 numstrings  = NumStrings ();
char*               toc;

                                        // work with a copy
TStrings            string ( *this );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // move one directory up in case of MFF "files" (which are in fact directories)
                                        // this tests only the string, not the actual file, but there are very little chances a string will have this syntax
for ( int i = 0; i < numstrings ; i++ ) {

    StringCopy ( buff, string[ i ] );

//  StringToLowercase ( buff );

    if ( IsGeodesicsMFFPath ( buff ) ) {

        crtl::RemoveFilename ( string[ i ] );

        if ( IsExtension ( string[ i ], FILEEXT_EEGMFFDIR ) )
            crtl::RemoveExtension ( string[ i ] );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // choose one file to extract the directory part
if ( includedir ) {
    StringCopy          ( directory, string[ 0 ] );
    crtl::RemoveFilename    ( directory );
    StringAppend        ( directory, "\\" );
    }
else
    ClearString ( directory );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // purge names from directory and extension, numbers and separators
for ( int i = 0; i < numstrings; i++ ) {
    GetFilename  ( string[ i ] );
    ReplaceChars ( string[ i ], "$?!&@()[]{}-.,;_0123456789", "                          " );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here we filter out all the infixes garbage

                                        // only 1 file, purge it by keeping the first tokens
if ( numstrings == 1 ) {

    TSplitStrings   splitnames ( string[ 0 ], UniqueStrings, " _\t\n,;:" );
                                        // purge out insignificant words
    #define     InsignificantWords      "From Into To The In Export"
    splitnames.Remove ( InsignificantWords );
    splitnames.CompactSpaces ();


    ClearString ( string[ 0 ] );

    for ( int tok = 0; tok < 2 && tok < splitnames.GetNumTokens (); tok++ ) {

        StringCamelCase ( splitnames[ tok ] );

        StringAppend ( string[ 0 ], splitnames[ tok ], " " );
        }
    } // if numstrings == 1

                                        // set a limit to the number of files for heavy scrutinization
else if ( numstrings < 50 ) {

    TSplitStrings  *splitnames  = new TSplitStrings [ numstrings ];


                                        // split into tokens
    for ( int i = 0; i < numstrings; i++ ) {
        splitnames[ i ].Set ( string[ i ], UniqueStrings, " _\t\n,;:" );
                                        // purge out insignificant words
        splitnames[ i ].Remove ( InsignificantWords );
        splitnames[ i ].CompactSpaces ();
        }

                                        // remove any 2 common parts, in whatever order
                                        // but still keeping the first token(s), as it's usually meaningful
    for ( int i = 0; i < numstrings - 1; i++ )
        for ( int itok = 2; itok < splitnames[ i ].GetNumTokens (); itok++ ) {

            bool    found   = false;

            for ( int j = i + 1; j < numstrings; j++ )
                for ( int jtok = 2; jtok < splitnames[ j ].GetNumTokens (); jtok++ )

                    if ( *splitnames[ i ][ itok ] && StringIs ( splitnames[ i ][ itok ], splitnames[ j ][ jtok ] ) ) {
                        ClearString ( splitnames[ j ][ jtok ] );
                        found   = true;
                        }

            if ( found ) {              // also remove the first occurence
//              DBGM ( splitnames[ i ][ itok ], "deleting token" );
                ClearString ( splitnames[ i ][ itok ] );
                }
            }

                                        // reconstruct strings with the remains
    for ( int i = 0; i < numstrings; i++ ) {

        splitnames[ i ].CompactSpaces ();

        ClearString ( string[ i ] );

        for ( int tok = 0; tok < splitnames[ i ].GetNumTokens (); tok++ ) {

            StringCamelCase ( splitnames[ i ][ tok ] );

            StringAppend ( string[ i ], tok ? " " : "", splitnames[ i ][ tok ] );
            }

//      DBGM ( string[ i ], "after" );
        }


    delete[] splitnames;
    } // if numstrings


else {                                  // too much files, go for a faster cleansing
    includedifference   = false;
                                        // simply truncate to the first part
    for ( int i = 0; i < numstrings; i++ ) {
        if ( StringContains ( string[ i ], ' ' ) )

            *StringContains ( string[ i ], ' ' ) = EOS;

//      DBGM ( string[ i ], "after" );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // removing duplicates?
for ( int i = 0; i < numstrings - 1; i++ )
    for ( int j = i + 1; j < numstrings; j++ )
        if ( StringIs ( string[ i ], string[ j ] ) )
            strset ( string[ j ], ' ' );
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
                                        // simplest search for the longer common part
                                        // get the longest strings first
for ( int i = 0; i < numstrings; i++ )
    if ( StringLength ( string[ i ] ) < minlength ) {
        StringCopy ( match, string[ i ] );
        minlength = StringLength ( match );
        }

                                        // search from longest to shortest substrings
for ( int l = minlength; l >= 1 && !found ; l-- ) {
                                        // scan all possible positions
    for ( int p = 0; p + l <= minlength ; p++ ) {
                                        // correctly set current substring
        StringCopy ( sub, match + p, l );
                                        // now scan all files
        for ( i = 0; i < numstrings; i++ ) {

            if ( ! StringContains ( string[ i ], sub ) )
                break;                  // if one file does not match, break
            }

                                        // all names have fitted?
        if ( i == numstrings ) {
            found       = true;
            StringCopy ( match, sub );
            minlength   = StringLength ( match );
            break;
            }
        }
    }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compounds the match with all bits of common stuff
match.Clear (); // whole content


do {
    minlength   = Highest ( minlength );
    foundone    = false;

                                        // get the shortest string first
    for ( int i = 0; i < numstrings; i++ )

        if ( StringLength ( string[ i ] ) < minlength ) {

            StringCopy ( buff, string[ i ] );
            minlength = StringLength ( buff );
            }

                                        // if at least one of the string is finished, stop
    if ( minlength == 0 )
        break;

/*                                            // add all common substrings, but from longest to shortest (random) order
                                            // search from longest to shortest substrings
    for ( long l = minlength; l >= 1 && ! foundone ; l-- ) {
                                            // scan all possible positions
        for ( long p = 0; p + l <= minlength ; p++ ) {
                                            // correctly set current substring
            StringCopy ( sub, buff + p, l );

                                            // now scan all files
            for ( i = 0; i < numstrings; i++ )
                if ( ! StringContains ( string[ i ], sub ) )
                    break;                  // if one file does not match, break

                                            // all names have fitted?
            if ( i != numstrings )
                continue;


            foundone    = true;
            StringAppend ( match, sub );
            DBGM ( sub, match );

            for ( i = 0; i < numstrings; i++ ) {
                toc     = StringContains ( string[ i ], sub );
                MoveVirtualMemory ( toc, toc + StringLength ( sub ), StringLength ( string[ i ] ) - ( toc - string[ i ] ) );
                }

            break;
            }
*/

                                        // scan all possible positions
    int                 i;

    for ( long p = 0; p < minlength && ! foundone ; p++ ) {
                                        // search from longest to shortest substrings
        for ( long l = minlength - p; l >= 1 /*&& p + l <= minlength*/ ; l-- ) {
                                        // correctly set current substring
            StringCopy ( sub, (const char*) buff + p, l );

                                        // now scan all files
            for ( i = 0; i < numstrings; i++ )
                if ( ! StringContains ( string[ i ], sub ) )
                    break;              // if one file does not match, break

                                        // all names have fitted?
            if ( i != numstrings )
                continue;


            foundone    = true;

                                        // remove from beginning up to the end of each sub
            for ( i = 0; i < numstrings; i++ ) {
                toc     = StringContains ( string[ i ], sub );

                                        // copy the difference part before the substring
                if ( includedifference ) {
//                  StringCamelCase ( string[ i ] );

                    StringCopy ( StringEnd ( match ), string[ i ], toc - string[ i ] );
//                  DBGM3 ( sub, StringEnd ( match ) - toc + string[ i ], match, "substring  difference  match" );
                    }

                                        // truncate string
                MoveVirtualMemory ( string[ i ], toc + StringLength ( sub ), StringLength ( string[ i ] ) - ( toc + l - string[ i ] ) + 1 );
                }


//          StringCamelCase ( sub );    // done in first part
            StringAppend ( match, sub );
//          DBGM2 ( sub, match, "sub  match" );


            break;
            }
        }

    found   |= foundone;


    if ( ! foundone && includedifference )
                                        // add the trailing parts
        for ( int i = 0; i < numstrings; i++ ) {
            toc     = StringEnd ( match );
            StringAppend ( match, string[ i ] );
            *toc    = (char) toupper ( *toc );
            }

    } while ( foundone );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // not enough substring?
if ( /*! found ||*/ StringLength ( match ) == 0 ) {

    StringCopy  ( match, Strings[ 0 ] );
    GetFilename ( match );
    }

                                        // don't allow the filename to begin with some kind of spaces...
while ( match.IsNotEmpty () &&
   (    StringStartsWith ( match, " " )
     || StringStartsWith ( match, "." )
     || StringStartsWith ( match, "_" ) ) ) {

    StringCopy ( buff, (const char*) match + 1, (int) StringLength ( match ) );
    StringCopy ( match, buff );
    }

                                        // don't allow the filename to end with some kind of spaces...
while ( match.IsNotEmpty () &&
   (    StringEndsWith ( match, " " )
     || StringEndsWith ( match, "." )
     || StringEndsWith ( match, "_" ) ) ) {

    match[ (int) ( match.Length () - 1 ) ] = EOS;
    }

                                        // remove all spaces
StringNoSpace ( match );

                                        // check again it's not empty
if      ( match.IsEmpty () ) {
    StringCopy  ( match, Strings[ 0 ] );
    GetFilename ( match );
    }
else if ( match.Length () > 20 )        // or too long
    match[ 20 ] = EOS;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // finally, compose the base name
StringCopy ( base, directory, match );
                                        // gracefully checking these
SetCorrectPathCase  ( base );

SetExtendedPath     ( base );


return  found && StringLength ( match ) > 0;
}


//----------------------------------------------------------------------------
                                        // compute the sub-strings range that will discriminate all files
void    TGoF::GetFilenamesSubRange ( int& fromchars, int& tochars )     const
{
if ( IsEmpty () ) {
    fromchars   = 0;
    tochars     = 0;
    return;
    }

if ( NumFiles () == 1 ) {
    fromchars   = 0;
    tochars     = AtLeast ( (long) 0, StringLength ( Strings[ 0 ] ) - 1 );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // estimate the best file names part to discriminate against all files
TGoF                filenames       = *this;

filenames.RemoveDir ();
                                        // get the constant parts from the file name parts only
TFileName           commonstart;

filenames.GetCommonParts    (   0,  commonstart,    0,  0   );


long                from            = StringLength ( commonstart );         // could be 0
long                to;
long                maxto           = filenames.GetMaxStringLength () - 1;  // max of all strings


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TGoF                clipped;
TFileName           buff;


for ( to = from; to <= maxto; to++ ) {

    clipped.Reset ();

    for ( int fi = 0; fi < (int) filenames; fi++ ) {
                                                // check locally for each file name's length
        StringCopy ( buff, filenames[ fi ] + from, min ( to, (long) StringLength ( filenames[ fi ] ) - 1 ) - from + 1 );
        clipped.AddNoDuplicates ( buff );
        }
                                    // no duplicates with the current from..to range means we can discriminate all files
    if ( (int) clipped == (int) filenames )
        break;
    }

                                    // problem if we passed maxto, that means there are not enough length for discrimination
to  = NoMore ( maxto, to );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                    // try to complete backward any partial numbering, for a more intuitive result
//  if ( isdigit ( commonstart[ from ] ) )
    while ( from > 0 )
        if ( isdigit ( commonstart[ from - 1 ] ) )  from--;
        else                                        break;


int                 minto = filenames.GetMinStringLength() - 1;  // can't go beyond the shortest file name


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                    // try completing any partial numbering, forward
//  if ( isdigit ( filenames[ 0 ][ to ] ) )
//      while ( to < minto /*maxto*/ )
//          if ( isdigit ( filenames[ 0 ][ to + 1 ] ) ) to++;   // should loop through all files...
//          else                                        break;

bool                anydigit;

while ( to < minto /*maxto*/ ) {

    anydigit    = false;

    for ( int fi = 0; fi < (int) filenames && ! anydigit; fi++ )
        anydigit   |= (bool) isdigit ( filenames[ fi ][ to + 1 ] );

    if ( anydigit )     to++;
    else                break;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

fromchars   = from;
tochars     = to;
}


void    TGoGoF::GetFilenamesSubRange ( int& fromchars, int& tochars )   const
{
return  TGoF ( *this ).GetFilenamesSubRange ( fromchars, tochars );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TGoGoF::TGoGoF ()
{
Reset ();
}

                                        // create a set of empty groups
        TGoGoF::TGoGoF ( int numgofs )
{
Set ( numgofs );
}


        TGoGoF::TGoGoF ( const TGoF& gof )
{
Set ( gof );
}


void    TGoGoF::Set ( int numgofs )
{
Reset ();

for ( int gofi = 0; gofi < numgofs; gofi++ )
    Add ( new TGoF );
}


void    TGoGoF::Set ( const TGoF& gof )
{
Reset ();

Add ( &gof, true );
}


        TGoGoF::~TGoGoF ()
{
Reset ();
}


void    TGoGoF::Reset ()
{
                                        // clean-up each group
for ( int gofi = 0; gofi < (int) Group; gofi++ )
    Group[ gofi ]->Reset ();

                                        // delete content & structure
Group.Reset ( Deallocate );
}


        TGoGoF::TGoGoF ( const TGoGoF &op )
{
for ( int gofi = 0; gofi < (int) op; gofi++ )
    Add ( &op[ gofi ], true );
}


TGoGoF& TGoGoF::operator= ( const TGoGoF &op2 )
{
if ( &op2 == this )
    return  *this;

Reset ();

for ( int gofi = 0; gofi < (int) op2; gofi++ )
    Add ( &op2[ gofi ], true );

return  *this;
}


TGoGoF& TGoGoF::operator= ( const TGoF &op2 )
{
Set ( op2 );

return  *this;
}


//----------------------------------------------------------------------------
bool    TGoGoF::CheckGroupIndexes ( int& gofi1, int& gofi2 )    const
{
if ( NumGroups () == 0 ) {
    gofi1   = gofi2   = -1;
    return  false;
    }

                                        // if one of the 2 index is -1, set to max range
if ( gofi1 == -1 || gofi2 == -1 ) {
    gofi1   = 0;
    gofi2   = NumGroups () - 1;
    }

                                        // final test, (#groups - 1) is >= 0
Clipped ( gofi1, gofi2, 0, NumGroups () - 1 );

return  true;
}


//----------------------------------------------------------------------------
void    TGoGoF::Add ( const TGoF *gof, bool copy, long length )
{
if ( copy ) {
                                        // allocate a new TGoF and copy content
    TGoF*       newgof      = new TGoF ();

    for ( int gofi = 0; gofi < gof->NumFiles (); gofi++ )

        newgof->Add ( (*gof)[ gofi ], length );

    Group.Append ( newgof );
    }
else
                                        // caller transferred the ownership
    Group.Append ( const_cast<TGoF*> ( gof ) );
}


void    TGoGoF::Add ( const TGoGoF& gogof, long length )
{
for ( int gofi = 0; gofi < (int) gogof; gofi++ )

    Add ( &gogof[ gofi ], true, length );
}


//----------------------------------------------------------------------------
bool    TGoGoF::RemoveLastGroup ()
{
if ( IsEmpty () )
    return  false;

Group.RemoveLast ( Deallocate );

return true;
}


void    TGoGoF::RevertOrder ()
{
if ( IsEmpty () )
    return;


TListIterator<TGoF>     iterator;

foreachin ( Group, iterator )
    iterator ()->RevertOrder ();
}

                                        // max number of files in a range of group
int     TGoGoF::GetMaxFilesPerGroup ( int gofi1, int gofi2 )    const
{
if ( NumGroups () == 0 )
    return  0;


if ( ! CheckGroupIndexes ( gofi1, gofi2 ) )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 maxfilespergroup    = 0;

for ( int gofi = gofi1; gofi <= gofi2; gofi++ )
    Maxed ( maxfilespergroup, Group[ gofi ]->NumFiles () );


return  maxfilespergroup;
}


bool    TGoGoF::AreGroupsEqual ( int gofi1, int gofi2 ) const
{
if ( NumGroups () == 0 )
    return  false;


if ( ! CheckGroupIndexes ( gofi1, gofi2 ) )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int gofi = gofi1; gofi < gofi2; gofi++ )
    if ( Group[ gofi ]->NumFiles () != Group[ gofi + 1 ]->NumFiles () )
        return  false;


return  true;
}

                                        // all range of files?
int     TGoGoF::NumFiles () const
{
if ( NumGroups () == 0 )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                     numfiles        = 0;


for ( int gofi = 0; gofi < (int) Group; gofi++ )

    numfiles += Group[ gofi ]->NumFiles ();


return  numfiles;
}


int     TGoGoF::NumFiles ( int gofi1, int gofi2, int filei1, int filei2 )   const
{
if ( NumGroups () == 0 )
    return  0;


if ( ! CheckGroupIndexes ( gofi1, gofi2 ) )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                     gofi;
int                     numfiles        = 0;


for ( gofi = gofi1; gofi <= gofi2; gofi++ )

    numfiles += Group[ gofi ]->NumFiles ( filei1, filei2 );


return  numfiles;
}


void    TGoGoF::Show ( int gofi1, int gofi2, const char* title )  const
{
if ( IsEmpty () ) {
    ShowMessage ( "- Empty -", StringIsEmpty ( title ) ? "Group of Group of Files" : title );
    return;
    }


Clipped ( gofi1, gofi2, 0, NumGroups () - 1 );


char                buff      [ 32 ];
char                localtitle[ 256 ];

for ( int gofi = gofi1; gofi <= gofi2; gofi++ ) {

    StringCopy  ( localtitle, StringIsEmpty ( title ) ? "Group of Group of Files" : title, " / Group#", IntegerToString ( buff, gofi + 1 ) );

    Group[ gofi ]->Show ( localtitle );
    }
}


//----------------------------------------------------------------------------
int     TGoGoF::GetMaxGroupsWithinSubjects ()   const
{
if ( IsEmpty () )
    return  0;


int                 maxgroupswithinsubjects     = 1;


int                 numgroups           = NumGroups ();
int                 divisor;
TArray1<int>        fpg ( numgroups );  // Files Per Group

                                        // store files per group
for ( int gogofi = 0; gogofi < numgroups; gogofi++ ) {
    fpg ( gogofi )  = Group[ gogofi ]->NumFiles ();
//  DBGV3 ( numgroups, gogofi, fpg ( gogofi ), "numgroups, gogofi -> fpg" );
    }


                                        // loop through all divisors of numgoups, from bigger to smaller
for ( divisor = numgroups; divisor >= 2; divisor-- ) {
                                        // not a divisor of #groups?
    if ( numgroups % divisor != 0 )
        continue;

                                        // get number of ( group of groups )
    int     numgog      = numgroups / divisor;
    bool    gogok       = true;

                                        // loop through all ( group of groups )
    for ( int gogi = 0; gogi < numgog && gogok; gogi++ )
    for ( int gofi = gogi * divisor, gofi2 = gofi + divisor - 1; gofi < gofi2; gofi++ )
                                        // not consistent within a group of groups?
                                        // we could also run some checking on the # of time frames for each subject across all potential groups
        if ( fpg ( gofi ) != fpg ( gofi + 1 ) ) {
            gogok   = false;
            break;
            }

                                        // this is our best divisor
    if ( gogok ) {
        maxgroupswithinsubjects = divisor;
        break;
        }

    } // for divisor

//DBGV3 ( numgroups, divisor, maxgroupswithinsubjects, "numgroups divisor -> MaxGroupsWithinSubjects" );

return  maxgroupswithinsubjects;
}


//----------------------------------------------------------------------------
bool    TGoGoF::ContainsAny ( const TGoGoF& gogof ) const
{
if ( IsEmpty () || gogof.IsEmpty () )
    return  false;


TGoF                flatgogof       = gogof;

for ( int gofi = 0; gofi < NumGroups (); gofi++ )
    if ( Group[ gofi ]->ContainsAny ( flatgogof ) )
        return  true;

return  false;
}


//----------------------------------------------------------------------------
                                        // Converts <c> GoF's containing the <c> conditions for <n> subjects
                                        // into     <n> GoF's containing 1 subject <c> files
void    TGoGoF::ConditionsToSubjects ( int gofi1, int gofi2, TGoGoF& outgogof )     const
{
outgogof.Reset ();

if ( ! IsInsideLimits ( gofi1, gofi2, 0, NumGroups () - 1 ) )
    return;


CheckOrder ( gofi1, gofi2 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 maxfiles        = GetMaxFilesPerGroup ( gofi1, gofi2 );


for ( int fi = 0; fi < maxfiles; fi++ ) {
                                        // add 1 group for each line
    outgogof.Add ( new TGoF, false );
                                        // each group will contribute
    for ( int gofi = gofi1; gofi <= gofi2; gofi++ )
                                        // groups could have different number of files, though it usually doesn't make sense/shouldn't happen (haha)
        if ( fi < Group[ gofi ]->NumFiles () )

            outgogof[ fi ].Add ( (*Group[ gofi ])[ fi ], MaxPathShort );

    } // for fi

}


//----------------------------------------------------------------------------
void    TGoGoF::SubjectsToConditions ( TGoGoF& outgogof )   const
{
outgogof.Reset ();

if ( IsEmpty () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 maxgroups       = GetMaxFilesPerGroup ();   // max number of files in GoFs == max number of groups


for ( int gi = 0; gi < maxgroups; gi++ )
                                        // add 1 group for each group of files
    outgogof.Add ( new TGoF, false );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 maxfiles        = NumGroups   ();

                                        // subjects are spread across groups
for ( int fi = 0; fi < maxfiles; fi++ ) {
                                        // while each GoF contains all groups
    for ( int gofi = 0; gofi < Group[ fi ]->NumFiles (); gofi++ )

        outgogof[ gofi ].Add ( (*Group[ fi ])[ gofi ], MaxPathShort );

    } // for fi

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
