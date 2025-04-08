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

#include    <iostream>
#include    <iomanip>
#include    <io.h>
#include    <wtypes.h>

#include    "Strings.Utils.h"
#include    "Strings.TStrings.h"
#include    "Strings.Grep.h"

#include    "TMicroStatesFitDialog.h"   // FitSubjectNameLong

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define             SpreadSheetFilesExt             FILEEXT_CSV " " FILEEXT_TXT
constexpr char*     AllSpreadSheetFilesFilter       = "CSV files|" FILEFILTER_CSV "|TXT files|" FILEFILTER_TXT;
constexpr char*     DefaultSpreadSheetSeparator     = ",";
constexpr int       SpreadSheetSeparatorListSize    = 5;
constexpr char*     SpreadSheetErrorMessage         = "It seems you dropped the wrong file, as some fields are missing!";


//----------------------------------------------------------------------------
                                        // Code all different types of .csv / .txt formats we can recognize
                                        // The meaning is what is the INPUT usage of the file
enum                CsvType
                    {
                    CsvUnknown              = 0x0000,

                    CsvStatFittingCartool   = 0x0001,   // results of fitting, Cartool format
                    CsvStatFittingR         = 0x0002,   // results of fitting, R format
                    CsvStatFiles            = 0x0004,   // stats on files
                    CsvStatMarkov           = 0x0008,   // Markov chains
                    CsvSegmentation         = 0x0010,   // input for segmentation
                    CsvFitting              = 0x0010,   // same requirements as for segmentation
                    CsvFileCalculator       = 0x0010,   // same requirements as for segmentation
                    CsvComputingRis         = 0x0010,   // same requirements as for segmentation
                    CsvTimeFreqEpochs       = 0x0020,   // Spindles analysis
                    CsvAveraging            = 0x0040,   // file averaging
                    CsvTypeMask             = 0x00FF,   // Only type

                    CsvLongNames            = 0x0100,
                    CsvShortNames           = 0x0200,
                    CsvNamesMask            = 0x0300,   // Only names
                    };

                                        // Mask relevant fields
inline  CsvType     GetCsvType              ( const CsvType& csvtype )      { return (CsvType) ( csvtype & CsvTypeMask  ); }
inline  CsvType     GetCsvNames             ( const CsvType& csvtype )      { return (CsvType) ( csvtype & CsvNamesMask ); }

inline  bool        IsCsvUnknownType        ( const CsvType& csvtype )      { return GetCsvType ( csvtype ) == CsvUnknown; }
inline  bool        IsCsvStatFittingCartool ( const CsvType& csvtype )      { return GetCsvType ( csvtype ) & CsvStatFittingCartool; }
inline  bool        IsCsvStatFittingR       ( const CsvType& csvtype )      { return GetCsvType ( csvtype ) & CsvStatFittingR; }
inline  bool        IsCsvStatFitting        ( const CsvType& csvtype )      { return GetCsvType ( csvtype ) & ( CsvStatFittingCartool | CsvStatFittingR ); }
inline  bool        IsCsvStatFiles          ( const CsvType& csvtype )      { return GetCsvType ( csvtype ) & CsvStatFiles; }
inline  bool        IsCsvStatMarkov         ( const CsvType& csvtype )      { return GetCsvType ( csvtype ) & CsvStatMarkov; }
inline  bool        IsCsvSegmentation       ( const CsvType& csvtype )      { return GetCsvType ( csvtype ) & CsvSegmentation; }
inline  bool        IsCsvFitting            ( const CsvType& csvtype )      { return GetCsvType ( csvtype ) & CsvFitting; }
inline  bool        IsCsvFileCalculator     ( const CsvType& csvtype )      { return GetCsvType ( csvtype ) & CsvFileCalculator; }
inline  bool        IsCsvComputingRis       ( const CsvType& csvtype )      { return GetCsvType ( csvtype ) & CsvComputingRis; }
inline  bool        IsCsvTimeFreqEpochs     ( const CsvType& csvtype )      { return GetCsvType ( csvtype ) & CsvTimeFreqEpochs; }
inline  bool        IsCsvAveraging          ( const CsvType& csvtype )      { return GetCsvType ( csvtype ) & CsvAveraging; }

inline  bool        IsUndefinedNames        ( const CsvType& csvtype )      { return GetCsvNames ( csvtype ) == CsvUnknown; }
inline  bool        IsLongNames             ( const CsvType& csvtype )      { return GetCsvNames ( csvtype ) & CsvLongNames; }
inline  bool        IsShortNames            ( const CsvType& csvtype )      { return GetCsvNames ( csvtype ) & CsvShortNames; }


//class             ShortString             { char  String[ 8 ]; };
inline const char*  GetLocaleListSeparator  ( char* csvseparator );


//----------------------------------------------------------------------------
                                        // read a file and split it into records
class   TSpreadSheet
{
public:

    inline              TSpreadSheet ();
    inline             ~TSpreadSheet ();


    inline bool         IsAllocated     ()      const               { return NumRecords != 0; }


    inline void         Reset           ();


    inline bool         ReadFile        ( const char* file );

    inline bool         WriteFile       ( char* file = 0, char* forcedelimiter = 0 );
    inline void         WriteAttribute  ( const char* sattr, int iattr = -1 );
    inline void         WriteAttribute  ( int         iattr );
    inline void         WriteAttribute  ( float       fattr );
    inline void         WriteNextRecord ();
    inline void         WriteFinished   ();


    inline int          GetNumRecords ()        const               { return NumRecords; }
    inline int          GetNumAttributes ()     const               { return NumAttributes; }

    inline CsvType      GetType ()              const;
    inline void         GetFittingInfo ( int& numgroups, TArray1<int>& groupindex, char* CsvFirstGroup, int& numvarspergroup, TStrings&    varnames, int& maxmap, TSelection& mapsel, TStrings&    mapnames, int& numfitvars, TStrings&    fitvarnames )    const;


    inline const char*  GetHeader (             int   attribute )   const           { return Header[ attribute ]; }
    inline const char*  GetRecord ( int record, int   attribute )   const           { return Tokens[ IndexesToLinearIndex ( record, attribute, NumAttributes ) ]; }

    inline bool         GetRecord ( int record, const char* attribute, char*  answer )  const;
    inline bool         GetRecord ( int record, const char* attribute, int   &answer )  const;
    inline bool         GetRecord ( int record, const char* attribute, float &answer )  const;
    inline bool         GetRecord ( int record, int         attribute, char*  answer )  const;
    inline bool         GetRecord ( int record, int         attribute, int   &answer )  const;
    inline bool         GetRecord ( int record, int         attribute, float &answer )  const;

    inline int          GetAttributeIndex   ( const char* attribute )   const;
    inline const char*  GetAttribute        ( int index       )         const   { return Header ( index ); }
    inline bool         HasAttribute        ( const char* attribute )   const   { return GetAttributeIndex ( attribute ) != -1; }


    inline const char*  GetLocaleListSeparator ();


                    operator bool   ()                  const       { return NumRecords; }

    const char*     operator ()     ( int i1, int i2 )  const       { return Tokens[ IndexesToLinearIndex ( i1, i2, NumAttributes ) ]; }


protected:

    TStrings        Header;
    TStrings        Tokens;
    int             NumRecords;
    int             NumAttributes;

    char            LocaleListSeparator[ SpreadSheetSeparatorListSize ];    // depends on the user settings...

    std::ofstream*  OutStream;          // in write mode
    bool            OutNewRecord;
    char            OutListSeparator[ SpreadSheetSeparatorListSize ];
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
        TSpreadSheet::TSpreadSheet ()
{
OutStream           = 0;

Reset ();

GetLocaleListSeparator ();
}


        TSpreadSheet::~TSpreadSheet ()
{
Reset ();
}


void    TSpreadSheet::Reset ()
{
NumRecords          = 0;
NumAttributes       = 0;
Header.Reset ();
Tokens.Reset ();

WriteFinished ();
}


//----------------------------------------------------------------------------
const char* GetLocaleListSeparator ( char* csvseparator )
{
if ( csvseparator == 0 )
    return  0;

if ( ! GetLocaleInfo ( LOCALE_USER_DEFAULT, LOCALE_SLIST, csvseparator, SpreadSheetSeparatorListSize ) )
//  ClearString ( LocaleListSeparator );
    StringCopy ( csvseparator, DefaultSpreadSheetSeparator );

return  csvseparator;
}


const char* TSpreadSheet::GetLocaleListSeparator ()
{
return  crtl::GetLocaleListSeparator ( LocaleListSeparator );
}


//----------------------------------------------------------------------------
CsvType     TSpreadSheet::GetType ()    const
{
CsvType             csvtype         = CsvUnknown;


if ( ! IsAllocated () )
    return  csvtype;

                                        // guess the type of file dropped
bool                isstatfittingcartool    =   StringIs   ( GetHeader ( 0 ), FitSubjectNameLong )                                                                  // Cartool / R / Markov
                                           && ( StringGrep ( GetHeader ( 1 ), FitGroupNameLong  "[0-9]+_" FitMapNameLong  "[0-9]+_.+", GrepOptionDefault )   // Cartool only
                                             || StringGrep ( GetHeader ( 1 ), FitGroupNameShort "[0-9]+_" FitMapNameShort "[0-9]+_.+", GrepOptionDefault ) );// Cartool only


bool                isstatfittingR          =   StringIs   ( GetHeader ( 0 ), FitSubjectNameLong )  // Cartool / R / Markov
                                           &&   StringIs   ( GetHeader ( 1 ), FitGroupNameLong   )  // R only
                                           &&   StringIs   ( GetHeader ( 2 ), FitMapNameLong     ); // R only


bool                isstatmarkov            =   StringIs   ( GetHeader ( 0 ), FitSubjectNameLong )                                                                                               // Cartool / R / Markov
                                           && ( StringGrep ( GetHeader ( 1 ), FitGroupNameLong  "[0-9]+_" FitFromMapNameLong  "[0-9]+_" FitToMapNameLong  "[0-9]+", GrepOptionDefault )   // Markov only
                                             || StringGrep ( GetHeader ( 1 ), FitGroupNameShort "[0-9]+_" FitFromMapNameShort "[0-9]+_" FitToMapNameShort "[0-9]+", GrepOptionDefault ) );// Markov only


                                        // Next 2 share "numfiles"
bool                isstatfiles         =   HasAttribute ( "numfiles" )                                         
                                       &&   HasAttribute ( "timemin"  )                                         // Only in stat files
                                       &&   HasAttribute ( "timemax"  )
                                       &&   HasAttribute ( "timemode" )
                                       &&   HasAttribute ( "file1"    );


bool                issegmentation      =   HasAttribute ( "numfiles" )                                         
                                       && ! HasAttribute ( "timemin"  )                                         // NOT like stat files
                                       &&   StringGrep ( GetHeader ( 1 ), "file[0-9]+", GrepOptionDefault );                   


bool                istimefreqepochs    =   HasAttribute ( "Track Name" )                                         
                                       &&   HasAttribute ( "TF From" )
                                       &&   HasAttribute ( "TF To" )
                                       &&   HasAttribute ( "FreqCentr" );


bool                isaveraging         =   HasAttribute ( "file" )                                         
                                       &&   HasAttribute ( "session" )
                                       &&   HasAttribute ( "tva" )
                                       &&   HasAttribute ( "triggers" );


csvtype     = isstatfittingcartool  ? CsvStatFittingCartool
            : isstatfittingR        ? CsvStatFittingR
            : isstatmarkov          ? CsvStatMarkov
            : isstatfiles           ? CsvStatFiles
            : issegmentation        ? CsvSegmentation
            : istimefreqepochs      ? CsvTimeFreqEpochs
            : isaveraging           ? CsvAveraging
            :                         CsvUnknown;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Long / Short variable names?

                                        // Extra flags for the fitting variables
if ( isstatfittingcartool   )   csvtype     = (CsvType) ( csvtype | ( StringGrep   ( GetHeader ( 1 ), FitGroupNameLong "[0-9]+_" FitMapNameLong "[0-9]+_.+", GrepOptionDefault )                             ? CsvLongNames : CsvShortNames ) );
                                        // The short case shouldn't really occur, still testing
if ( isstatfittingR         )   csvtype     = (CsvType) ( csvtype | ( StringLength ( GetHeader ( 3 ) ) > 1                                                                                                          ? CsvLongNames : CsvShortNames ) );

if ( isstatmarkov           )   csvtype     = (CsvType) ( csvtype | ( StringGrep   ( GetHeader ( 1 ), FitGroupNameLong "[0-9]+_" FitFromMapNameLong "[0-9]+_" FitToMapNameLong "[0-9]+", GrepOptionDefault ) ? CsvLongNames : CsvShortNames ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//DBGV3 ( GetCsvType ( csvtype ), IsLongNames( csvtype ), IsShortNames( csvtype ), "csv Type, LongNames, ShortNames" );

return  csvtype;
}


//----------------------------------------------------------------------------
void        TSpreadSheet::GetFittingInfo ( int&     numgroups,          TArray1<int>&   groupindex,     char*           CsvFirstGroup,
                                           int&     numvarspergroup,    TStrings&       varnames,           
                                           int&     maxmap,             TSelection&     mapsel,         TStrings&       mapnames,
                                           int&     numfitvars,         TStrings&       fitvarnames )   const
{
numgroups       = 0;
groupindex  .DeallocateMemory ();
ClearString ( CsvFirstGroup );
numvarspergroup = 0;
varnames    .Reset ();         
maxmap          = 0;
mapsel      .Reset ();
mapnames    .Reset ();       
numfitvars      = 0;
fitvarnames .Reset ();    


CsvType             csvtype         = GetType ();

if ( ! ( IsCsvStatFittingCartool ( csvtype ) || IsCsvStatMarkov ( csvtype ) ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Retrieve group indexes (starting with "Groups") and number of groups

TSelection          selgroupindex ( 100, OrderArbitrary ); // max 100 groups!
int                 numnotgroup     = 0;
char                attr            [ 256 ];
char                csvfirstgroup   [ 32 ];
int                 gi;


ClearString ( csvfirstgroup );
                                        
                                        // !Groups can have any arbitrary indexes, so we need to retrieve all of them!
                                        // Also count all the non-group attributes to get a proper count of groups/variables
for ( int i = 0; i < GetNumAttributes(); i++ ) {

    StringCopy ( attr, GetAttribute ( i ) );

    if      ( StringStartsWith ( attr, FitGroupNameLong  ) ) {

        sscanf ( attr, FitGroupNameLong  "%d_", &gi );
                                        // store real indexes, in the scanned order
        selgroupindex.Set ( gi );

        if ( StringIsEmpty ( csvfirstgroup ) ) sprintf ( csvfirstgroup, FitGroupNameLong  "%d_", gi );
        }
        
    else if ( StringStartsWith ( attr, FitGroupNameShort ) ) {  

        sscanf ( attr, FitGroupNameShort "%d_", &gi );
                                        // store real indexes, in the scanned order
        selgroupindex.Set ( gi );

        if ( StringIsEmpty ( csvfirstgroup ) ) sprintf ( csvfirstgroup, FitGroupNameShort "%d_", gi );
        }

    else 
        numnotgroup++;
        
    }


numgroups       = (int) selgroupindex;

                                        // optional parameter
if ( CsvFirstGroup )
    StringCopy ( CsvFirstGroup, csvfirstgroup );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // transfer the resulting group indexes
groupindex.Resize ( numgroups );

for ( int i = 0; i < numgroups; i++ )
    groupindex[ i ]     = selgroupindex.GetValue ( i );

                                        // remove 'Subject' attribute, all the remaining is in the form of Group*_Var*
numvarspergroup  = ( GetNumAttributes () - numnotgroup ) / NonNull ( numgroups );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Extract and copy the variables names (anything past "Groups")
varnames.Reset ();


char                varname         [ 256 ];

                                    // then re-scan and copy them
for ( int i = 0; i < GetNumAttributes(); i++ ) {

    StringCopy ( attr, GetAttribute ( i ) );

                                    // Scanning first encountered group is enough!
    if ( ! StringStartsWith ( attr, csvfirstgroup ) )
        continue;

                                    // !case sensitive!
    if      ( IsLongNames  ( csvtype ) )    sscanf ( attr, FitGroupNameLong  "%*d_%s", varname );
    else if ( IsShortNames ( csvtype ) )    sscanf ( attr, FitGroupNameShort "%*d_%s", varname );


    varnames.Add ( varname );
    }


//selgroupindex.Show ( "group indexes" );
//varnames.Show ( "VarNames extracted" );
//DBGV3 ( (int) selgroupindex, numvarspergroup, (int) varnames, "#groups, numvarspergroup, numvarnames" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3.1) Retrieve number of maps, and map names in the form "MapXX" or "MXX"
char                mapname         [ 256 ];
char                fitvarname      [ 256 ];
//int                 mapi;


//maxmapi     = 0;

for ( int i = 0; i < numvarspergroup; i++ ) {
                                        // Get maps names in the form "MapXX" or "MXX" for all cases
    StringCopy ( mapname, varnames[ i ] );

    if      ( IsCsvStatMarkov         ( csvtype ) && IsLongNames  ( csvtype ) )     StringReplace   ( mapname, FitFromMapNameLong , FitMapNameLong  );
    else if ( IsCsvStatMarkov         ( csvtype ) && IsShortNames ( csvtype ) )     StringReplace   ( mapname, FitFromMapNameShort, FitMapNameShort );

    ClipToChars ( mapname, "_" );

    mapnames.AddNoDuplicates ( mapname );
    }

                                        // COUNT of maps, NOT max index
maxmap      = (int) mapnames;


//mapnames.Show ( "Map names" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3.2) Now select the maps (2nd layer of variable)

                                        // map numbering starts from 1
mapsel      = TSelection ( maxmap + 1, OrderSorted );

                                        // select all maps associated to mapnames variable
mapsel.Set ( 1, maxmap );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3.3) Save the 2nd layer of variable names

for ( int i = 0; i < numvarspergroup; i++ ) {

    StringCopy ( fitvarname, varnames[ i ] );

    char*       toc     = JumpToChars ( fitvarname, "_" );

                                        // fitvarnames is unprocessed
    if ( StringIsNotEmpty ( toc ) )
        fitvarnames.AddNoDuplicates ( toc + 1 );
    }


numfitvars  = numvarspergroup / NonNull ( (int) mapsel );


//DBGV5 ( (int) mapsel, maxmap, (int) mapnames, numfitvars, (int) fitvarnames, "#mapsel, maxmap, #mapnames, numfitvars #fitvarnames" );
//mapnames.Show ( "map names" );
//fitvarnames.Show ( "fitvarnames" );
}


//----------------------------------------------------------------------------
bool    TSpreadSheet::WriteFile ( char* file, char* forcedelimiter )
{
if ( OutStream ) {
    ShowMessage ( "Already in write mode!", "Writing list file", ShowMessageWarning );
    return false;
    }


static GetFileFromUser  getfile ( "", AllSpreadSheetFilesFilter, 1, GetFileWrite );

                                        // no file name provided, open a dialog
if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute () )
        return  false;
    }
else
    getfile.SetOnly ( file );


                                        // is it a legal extension?
if ( ! IsExtensionAmong ( (char *) getfile, SpreadSheetFilesExt ) ) {
    ShowMessage ( "Wrong file extension!", "Writing list file", ShowMessageWarning );
    return false;
    }

                                        // get delimiter
if ( StringIsNotEmpty ( forcedelimiter ) )                  StringCopy ( OutListSeparator, forcedelimiter, SpreadSheetSeparatorListSize - 1 );
else {
    if      ( IsExtension ( (char *) getfile, FILEEXT_CSV ) )   StringCopy ( OutListSeparator, LocaleListSeparator, SpreadSheetSeparatorListSize - 1 );
    else if ( IsExtension ( (char *) getfile, FILEEXT_TXT ) )   StringCopy ( OutListSeparator, Tab,                 SpreadSheetSeparatorListSize - 1 );
    else                                                        StringCopy ( OutListSeparator, " ",                 SpreadSheetSeparatorListSize - 1 );
    }


OutStream           = new ofstream ( TFileName ( (const char* ) getfile, TFilenameExtendedPath ) );
OutNewRecord        = true;

return  ! OutStream->fail ();
}


//----------------------------------------------------------------------------
                                        // compose the attribute with any of the parameters
void    TSpreadSheet::WriteAttribute ( const char* sattr, int iattr )
{
if ( ! OutNewRecord )
    *OutStream << OutListSeparator;

*OutStream << sattr;

if ( iattr >= 0 )
    *OutStream << iattr;

OutNewRecord        = false;
}


void    TSpreadSheet::WriteAttribute ( int iattr )
{
if ( ! OutNewRecord )
    *OutStream << OutListSeparator;

*OutStream << iattr;

OutNewRecord        = false;
}


void    TSpreadSheet::WriteAttribute ( float fattr )
{
if ( ! OutNewRecord )
    *OutStream << OutListSeparator;

*OutStream << fattr;

OutNewRecord        = false;
}


void    TSpreadSheet::WriteNextRecord ()
{
OutNewRecord        = true;
*OutStream << fastendl;
}


void    TSpreadSheet::WriteFinished ()
{
if ( ! OutStream )
    return;

//OutStream->flush ();

delete  OutStream;
OutStream = 0;
}


//----------------------------------------------------------------------------
bool    TSpreadSheet::ReadFile ( const char* file )
{
Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static GetFileFromUser  getfile ( "", AllSpreadSheetFilesFilter, 1, GetFileRead );
TFileName               filecsv;

                                        // no file name provided, open a dialog
if ( StringIsEmpty ( file ) ) {

    if ( ! getfile.Execute () )
        return  false;

    filecsv.Set ( (const char* ) getfile, TFilenameExtendedPath );
    }
else
    filecsv.Set ( (const char* ) file,    TFilenameExtendedPath );


                                        // is it a legal extension?
if ( ! IsExtensionAmong ( filecsv, SpreadSheetFilesExt ) ) {
    ShowMessage ( "Wrong file extension!", "Reading list file", ShowMessageWarning );
    return false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Retrieve actual delimiter
constexpr int       bufflinesize    = MegaByte;
TArray1<char>       buffline ( bufflinesize );

const char*         toext           = ToExtension ( filecsv );
const char*         delimiter;
char                delimiterdouble     [ 8 ];
char                delimiterdoublespace[ 8 ];


ifstream            is ( filecsv );


if      ( StringIs ( toext, FILEEXT_CSV ) ) {   // actually, this case is tricky, it can either be , or ;

    is.getline ( buffline, bufflinesize, '\n' );

    if      ( StringContains ( buffline, ';' ) )    delimiter = ";";
    else if ( StringContains ( buffline, ',' ) )    delimiter = ",";
    else                                            delimiter = LocaleListSeparator;
    }
else if     ( StringIs       ( toext, "txt"  ) )    delimiter = Tab;
else                                                delimiter = " ";

                                        // like ",,"
StringCopy  ( delimiterdouble,      delimiter, delimiter );
                                        // like ", ,"
StringCopy  ( delimiterdoublespace, delimiter, " ", delimiter );

                                        // reset to beginning
is.seekg ( 0, ios::beg );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scan file & fill structure
TSplitStrings       split;

NumRecords      = 0;
NumAttributes   = 0;


do {
                                        // get a whole (big) line at once
    is.getline ( buffline, bufflinesize, '\n' );

                                        // skipping empty lines
    if ( StringIsSpace ( buffline ) )
        continue;

                                        // duplicated commas should create as many empty tokens, so to bypass TSplitStrings we need to separate consecutive delimiters
    StringReplace ( buffline, delimiterdouble, delimiterdoublespace );

                                        // split to delimiters
                                        // should also take care of quoted text with "
    split.Set ( buffline, NonUniqueStrings, delimiter );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // looking for header, the first non-empty line
    if ( NumAttributes == 0 ) {

        NumAttributes   = split.GetNumTokens ();

        for ( int attri = 0; attri < (int) split; attri++ )

            Header.Add ( split[ attri ] );

        continue;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here we have actual data
    for ( int attri = 0; attri < NoMore ( NumAttributes, (int) split ); attri++ ) {
                                        // removing spaces on each end, also getting rid of artificial space we introduced
        StringCleanup   ( split[ attri ] );
                                        // add string to the end of list
        Tokens.Add      ( split[ attri ] );
                                        // or simply
//      Tokens.Add      ( StringIs ( split[ attri ], " " ) ? "" : split[ attri ] );
        }

                                        // in case a line is not complete, fill it with empty tokens to remain consistent with the 2D tiling
    for ( int attri = (int) split; attri < NumAttributes; attri++ )

        Tokens.Add ( "" );

                                        // count non empty lines
    NumRecords++;

    } while ( ! is.eof() );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

is.close ();


if ( ! IsAllocated () )
    Reset ();

return  IsAllocated ();
}


//----------------------------------------------------------------------------
int     TSpreadSheet::GetAttributeIndex ( const char* attribute )     const
{
for ( int attri = 0; attri < NumAttributes; attri++ )

    if ( StringIs ( Header ( attri ), attribute ) )

        return  attri;

return -1;
}


//----------------------------------------------------------------------------
bool    TSpreadSheet::GetRecord ( int record, int attribute, char* answer ) const
{
ClearString ( answer );

if ( ! (    IsInsideLimits ( record,    0, NumRecords    - 1 ) 
         && IsInsideLimits ( attribute, 0, NumAttributes - 1 ) ) )
    return false;

                                        // Ok, return the stuff, not forgetting first line contains the attributes decription
StringCopy ( answer, GetRecord ( record, attribute ) );

return  true;
}


bool    TSpreadSheet::GetRecord ( int record, int attribute, int &answer )  const
{
if ( ! (    IsInsideLimits ( record,    0, NumRecords    - 1 ) 
         && IsInsideLimits ( attribute, 0, NumAttributes - 1 ) ) )
    return false;

                                        // Ok, return the stuff, not forgetting first line contains the attributes decription
answer  = StringToInteger ( GetRecord ( record, attribute ) );

return  true;
}


bool    TSpreadSheet::GetRecord ( int record, int attribute, float &answer )    const
{
if ( ! (    IsInsideLimits ( record,    0, NumRecords    - 1 ) 
         && IsInsideLimits ( attribute, 0, NumAttributes - 1 ) ) )
    return false;

                                        // Ok, return the stuff, not forgetting first line contains the attributes decription
answer  = StringToFloat ( GetRecord ( record, attribute ) );

return  true;
}


bool    TSpreadSheet::GetRecord ( int record, const char* attribute, char* answer )   const
{
                                        // find the attribute index
int     attr = GetAttributeIndex ( attribute );

if ( attr == -1 )
    return false;

return  GetRecord ( record, attr, answer );
}


bool    TSpreadSheet::GetRecord ( int record, const char* attribute, int &answer )    const
{
                                        // find the attribute index
int     attr = GetAttributeIndex ( attribute );

if ( attr == -1 )
    return false;

return  GetRecord ( record, attr, answer );
}


bool    TSpreadSheet::GetRecord ( int record, const char* attribute, float &answer )  const
{
                                        // find the attribute index
int     attr = GetAttributeIndex ( attribute );

if ( attr == -1 )
    return false;

return  GetRecord ( record, attr, answer );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
