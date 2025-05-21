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

#include    "TEegEgiNsrDoc.h"

#include    "MemUtil.h"
#include    "Strings.Utils.h"
#include    "Strings.TFixedString.h"
#include    "Dialogs.Input.h"
#include    "TArray1.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline  bool    NsrDataFollow   ( const short& s )      { return s & 0x8000; }


void            SwapShort ( void *tos, int num = 1 )
{
for ( ; num > 0; num--, tos = (char *) tos + sizeof ( short ) ) {
    uchar   t               = *((uchar *) tos);
    *((uchar *) tos)        = *((uchar *) tos + 1);
    *((uchar *) tos + 1)    = t;
    }
}


void            SwapLong ( void *tos, int num = 1 )
{
for ( ; num > 0; num--, tos = (char *) tos + sizeof ( long ) ) {
    uchar   t               = *((uchar *) tos);
    *((uchar *) tos)        = *((uchar *) tos + 3);
    *((uchar *) tos + 3)    = t;
    t                       = *((uchar *) tos + 1);
    *((uchar *) tos + 1)    = *((uchar *) tos + 2);
    *((uchar *) tos + 2)    = t;
    }
}


uchar           ReadByte ( ifstream *ifs, long where = -1, ios::seekdir dir = ios::beg )
{
if ( where >= 0 || dir != ios::beg )
    ifs->seekg ( where, dir );

return  (uchar) ifs->get ();
}


short           ReadShort ( ifstream *ifs, long where = -1, ios::seekdir dir = ios::beg )
{
short   s;

if ( where >= 0 || dir != ios::beg )
    ifs->seekg ( where, dir );

ifs->read ( (char *) &s, sizeof ( s ) );

SwapShort ( &s );
return  s;
}


long            ReadLong ( ifstream *ifs, long where = -1, ios::seekdir dir = ios::beg )
{
long    l;

if ( where >= 0 || dir != ios::beg )
    ifs->seekg ( where, dir );

ifs->read ( (char *) &l, sizeof ( l ) );

SwapLong ( &l );
return  l;
}


float           ReadFloat ( ifstream *ifs, long where = -1, ios::seekdir dir = ios::beg )
{
float   f;

if ( where >= 0 || dir != ios::beg )
    ifs->seekg ( where, dir );

ifs->read ( (char *) &f, sizeof ( f ) );

SwapLong ( &f );
return  f;
}


char*           ReadString ( ifstream *ifs, char *str, long where = -1, ios::seekdir dir = ios::beg )
{
//static char str[ 256 ];

int lenstr = ReadByte ( ifs, where, dir );

if ( lenstr <= 0 ) {
    *str = 0;
    return str;
    }
else if ( lenstr >= 256 )
    lenstr = 255;

                                        // read string
ifs->read ( str, lenstr );
str[ lenstr ] = 0;                      // and set the end

                                        // remove any trailing spaces
for ( lenstr--; lenstr >= 0; lenstr-- )
    if ( str[ lenstr ] == ' ' )
        str[ lenstr ] = 0;
    else
        break;

return str;
}


//----------------------------------------------------------------------------
void    TEegEgiNsrRecord::Read ( ifstream *ifs, int type )
{
int                 size            = type ? 4 * ( type + 1 ) : 6;

ifs->read ( (char *) this, size * sizeof ( long ) );
SwapLong ( &RecordCode, size );
}


//----------------------------------------------------------------------------
        TEegEgiNsrTopHeader::TEegEgiNsrTopHeader ( ifstream *ifs )
{
ifs->seekg ( 0, ios::beg );
ifs->read ( (char *) this, sizeof ( *this ) );

SwapLong  ( &Version );
SwapLong  ( &ToSessionTime );
SwapLong  ( &ToMainHeader );
SwapLong  ( &NumMainRecords );
SwapLong  ( &Num );
SwapLong  ( &OneK );
}


        TEegEgiNsrMainHeaderFirst::TEegEgiNsrMainHeaderFirst ( ifstream *ifs, long address )
{
ifs->seekg ( address, ios::beg );
ifs->read ( (char *) this, sizeof ( *this ) );
SwapShort ( &RecordType );
SwapShort ( &NumRecords );
SwapShort ( &Num );
SwapShort ( &Num256 );
SwapLong  ( &Long1 );
SwapLong  ( &ToRecords );
}


        TEegEgiNsrMainHeader::TEegEgiNsrMainHeader ( ifstream *ifs )
{
TEegEgiNsrTopHeader   topheader ( ifs );

ToFile          = ifs;
Version         = (NsrVersion) topheader.Version;
Address         = topheader.ToMainHeader;
TotalRecords    = topheader.NumMainRecords;
Splitted        = Address != 256;
First1          = TEegEgiNsrMainHeaderFirst ( ifs, 256 );
First2          = TEegEgiNsrMainHeaderFirst ( ifs, Address );
NumSessions     = FindNumSessions();
NumElectrodes   = FindNumElectrodes();
AtomSize        = IsAtomFloat () ? sizeof ( float ) : sizeof ( short );
}


bool    TEegEgiNsrMainHeader::GetNextRecord ( int &type, bool reset )
{
static  long    currpos = 256 + sizeof ( TEegEgiNsrMainHeaderFirst );

if ( reset ) {
    currpos = 256 + sizeof ( TEegEgiNsrMainHeaderFirst );
    CurrentRecordIndex = 0;
    }
                                        // restored to current record
ToFile->seekg ( currpos );
                                        // read the type of record
uchar   c3[ 3 ];
ToFile->read ( (char*) c3, 3 );
type    = c3[ 2 ];
                                        // different records can be read
Record.Read ( ToFile, type );


CurrentRecordCode = type ? Record.RecordCode : 0;
CurrentRecordIndex++;

                                        // need to jump to other part of header?
if ( CurrentRecordIndex == First1.NumRecords && CurrentRecordIndex < TotalRecords ) {
    ToFile->seekg ( First2.ToRecords + 8 );
    }
currpos = ToFile->tellg();

return CurrentRecordIndex < TotalRecords;
}


bool    TEegEgiNsrMainHeader::GetRecord ( int type, long recordcode )
{
int     t;

for ( int i=0; i < TotalRecords; i++ ) {
    GetNextRecord ( t, !i );

//    DBGV2 ( t, CurrentRecordCode, "Record: type code" );

    if ( t == type && CurrentRecordCode == recordcode )
        return true;
    }
return false;
}


int     TEegEgiNsrMainHeader::GetNumData ( long startaddress )
{
bool                datafollow      = NsrDataFollow ( ReadShort ( ToFile, startaddress ) );
int                 n               = ReadShort  ( ToFile, startaddress + 6 );

if ( datafollow )
    return n;
else {
    int sum = 0;
    for ( int i=0; i < n; i++ )
        sum += GetNumData ( ReadLong ( ToFile, startaddress + 8 + i * sizeof ( long ) ) );
    return sum;
    }
}


int     TEegEgiNsrMainHeader::GetData ( long startaddress, TEegEgiNsrTableCell *list, bool swap, int level )
{
bool				datafollow		= NsrDataFollow ( ReadShort ( ToFile, startaddress ) );
int					n				= ReadShort  ( ToFile, startaddress + 6 );
static  int			index;
                                        // first call?
if ( level == 0 )
    index = 0;

if ( datafollow ) {
    ToFile->seekg ( startaddress + 8 );
    for ( int i=0; i < n; i++ ) {       // here, we read 2 longs each time
        if ( swap ) {
            list[ index ].Id    = ReadLong ( ToFile );
            list[ index ].Data  = ReadLong ( ToFile );
            }
        else {
            list[ index ].Data  = ReadLong ( ToFile );
            list[ index ].Id    = ReadLong ( ToFile );
            }
        index++;
        }
    return index;
    }
else {
    for ( int i=0; i < n; i++ )
        GetData ( ReadLong ( ToFile, startaddress + 8 + i * sizeof ( long ) ), list, swap, level + 1 );
    return index;
    }
}


int     TEegEgiNsrMainHeader::GetStrings ( long startaddress, TEegEgiNsrTableCell *list, int level )
{
bool				datafollow		= NsrDataFollow ( ReadShort ( ToFile, startaddress ) );
int					n				= ReadShort  ( ToFile, startaddress + 6 );
//if ( n > 16 ) n = 16;
static  int			index;
                                        // first call?
if ( level == 0 )
    index = 0;

if ( datafollow ) {
    ToFile->seekg ( startaddress + 8 );
    for ( int i=0; i < n; i++ ) {       // here, we read 2 longs each time
        list[ index ].Id      = ReadLong ( ToFile );
        list[ index ].Address = ToFile->tellg ();
        index++;
                                        // jump to next string
        ToFile->seekg ( ReadByte ( ToFile ), ios::cur );
        }
    return index;
    }
else {
    for ( int i=0; i < n; i++ )
        GetStrings ( ReadLong ( ToFile, startaddress + 8 + i * sizeof ( long ) ), list, level + 1 );
    return index;
    }
}


//----------------------------------------------------------------------------
                                        // get the net's name
void    TEegEgiNsrMainHeader::GetNetName ( char *nname )
{
if ( !GetRecord ( 2, 27 ) )
    return;

long l1                 = ReadLong ( ToFile, Record.Address1 + 8 );
long l2                 = ReadLong ( ToFile, l1 + 14 );
long l3                 = l2 + 13 + ReadByte ( ToFile, l2 + 13 ) + 23;


ReadString ( ToFile, nname, l3 );
//DBGM ( nname, "net name" );


char   geosens [16] = "Geodesic Sensor";
char   gsn     [4]  = "GSN";
                                        // can be a wrong guess
if ( ! StringContains ( nname, geosens ) && ! StringContains ( nname, gsn ) ) {
    do {
        ToFile->read ( nname, 15 );
        ToFile->seekg ( - 15 + 1, ios::cur );
        } while ( ! StringStartsWith ( nname, geosens, 15 ) && ! StringStartsWith ( nname, gsn, 3 ) && ! ToFile->eof () );

    if ( ToFile->eof () ) {
//        ToFile        // reset this
        ClearString ( nname );
        }
    else {
        ReadString ( ToFile, nname, -2, ios::cur );
        }
    }

//DBGM ( nname, "Net name" );
}


int   TEegEgiNsrMainHeader::GetNumFilters ()
{
if ( !GetRecord ( 2, 28 ) )
    return  0;

long l1                 = ReadLong ( ToFile, Record.Address1 + 8 );
long l2                 = ReadLong ( ToFile, l1 + 14 );
long numfilters         = ReadLong ( ToFile, l2 + ( GetVersion () == NsrVersion1 ? 22 : 26 ) );

//DBGV2 ( l2 + 26, numfilters, "address  #filters" );

return  numfilters;
}


bool  TEegEgiNsrMainHeader::HasHighpassFilter ()
{
if ( !GetRecord ( 2, 28 ) )
    return  0;

long l1                 = ReadLong ( ToFile, Record.Address1 + 8 );
long l2                 = ReadLong ( ToFile, l1 + 14 );
long length             = ReadLong ( ToFile, l2 + 5 );

char filtersstring[ 1024 ];
                                        // get the whole block of filter stuff
ToFile->read ( filtersstring, length );
                                        // replace all null chars (from actual integer values)
for ( int i = 0; i < length; i++ )
    if ( filtersstring[ i ] == 0 )
        filtersstring[ i ] = '_';

filtersstring[ min ( length, 1023L ) ] = 0;
                                        // simply test this pseudo string for 'highpass'!
                                        // version 1 has different strings, though, just do a simpler test (maybe not working all the time)
return  (bool) StringContains ( (const char*) filtersstring, (const char*) "highpass" )
      || GetVersion () == NsrVersion1 && GetNumFilters () > 0 ;
}


bool    TEegEgiNsrMainHeader::IsAtomInteger ()
{
double  muvolts, numbits;

if ( ! GetScale ( muvolts, numbits ) )
    return  false;

return  numbits < 24;
}


bool    TEegEgiNsrMainHeader::IsAtomFloat ()
{
double  muvolts, numbits;

if ( ! GetScale ( muvolts, numbits ) )
    return  false;

return  numbits >= 24;
}


//----------------------------------------------------------------------------
                                        // can be smarter by sneaking in the file for 128L, 129L
int     TEegEgiNsrMainHeader::FindNumElectrodes ()
{
char                nname[ 128 ];
int                 nameel          = 0;

                                        // retrieve the Net name
GetNetName ( nname );

//if ( *nname )                           // try to extract the number of electrodes from it
//    if ( sscanf ( nname + strcspn ( nname, "0123456789" ), "%d", &nameel ) != 1 )
//        nameel = 0;

if ( *nname ) {                         // try to extract the number of electrodes from it
    char   *toc = StringContains ( nname, ' ', StringContainsBackward );

    if ( !toc ) {
        toc = nname;

        if ( sscanf ( toc + strcspn ( nname, "0123456789" ), "%d", &nameel ) != 1 )
            nameel = 0;

//        if ( JumpToChars ( nname, "0123456789" )
        }
    else {
        toc++;
        if ( sscanf ( toc, "%d", &nameel ) != 1 )
            nameel = 0;
        }
    }

return ( nameel ? nameel : 128 ) + ( Version == NsrVersion1 ? 0 : 1 );
}


void    TEegEgiNsrMainHeader::SetAuxElectrodes ( TSelection &aux )
{
if      ( Version == NsrVersion1 ) {
                                        // 125 126 127 128
    aux.Set ( aux.Size() - NumPseudoTracks - 5,
              aux.Size() - NumPseudoTracks - 2 );
    }
else if ( Version == NsrVersion2 ) {
                                        // 125 126 127 128
    aux.Set ( aux.Size() - NumPseudoTracks - 5,
              aux.Size() - NumPseudoTracks - 2 );

                                        // rather local to Geneva group
    aux.Set ( NSRV2_AUX1 - 1 );
    aux.Set ( NSRV2_AUX2 - 1 );
    }
else if ( Version == NsrVersion3 || Version == NsrVersion4 ) {    // no auxs
    }
}


int     TEegEgiNsrMainHeader::GetNumAuxElectrodes ()
{
                                        // rather local to Geneva group
if      ( Version == NsrVersion1 )  return 4;
else if ( Version == NsrVersion2 )  return 6;
else if ( Version == NsrVersion3 )  return 0;
else if ( Version == NsrVersion4 )  return 0;
else                                return 0;
}


int     TEegEgiNsrMainHeader::GetSamplingFrequency ()
{
if ( GetRecord ( 2, 26 ) )
    return ReadShort ( ToFile, ReadLong ( ToFile, Record.Address1 + 8 ) + 14 );
else
    return 0;
}


int     TEegEgiNsrMainHeader::FindNumSessions ()
{
if ( GetRecord ( 2, 37 ) )
    return Record.L1;
else
    return 0;
}


bool    TEegEgiNsrMainHeader::GetDateTime ( TDateTime &dt, int session )
{
if ( !GetRecord ( 1, 22 ) )
    return false;

long l1                 = ReadLong ( ToFile, Record.Address1 + 8 );
long l2                 = ReadLong ( ToFile, l1 + 6 );
unsigned long timesec   = ReadLong ( ToFile, l2 + 11 ); // need unsigned, because it's a lot!
unsigned long timemsec  = ReadLong ( ToFile );
unsigned long deltams   = GetSessionOffset ( session );
//DBGV3 ( timesec, timemsec, deltams, "s ms deltams" );

unsigned long ms        = timemsec + deltams;
unsigned long s         = timesec  + ms / 1000;
//DBGV2 ( s, ms, "total  s ms" );

                                        // time in seconds since 1-1-1904 (valid till 2040)
TDate   macstart ( 1, 1904 );

dt = TDateTime ( macstart.Year(), macstart.Month(), macstart.DayOfMonth(),
                 0, s / 60, s % 60, ms % 1000, 0 );
//               0, 0, timesec, timemsec + deltams, 0 ); // old way: ms used to fit into ulong

return true;
}


//----------------------------------------------------------------------------
long    TEegEgiNsrMainHeader::GetSessionOffset ( int session )
{
long    deltams;

if ( !GetRecord ( 2, 37 ) )
    return false;

long    l3      = ReadLong ( ToFile, Record.Address1 + 8 + session * 8 );
deltams = ReadLong ( ToFile, l3 + 8 );

return deltams;
}

                                        // It is getting more hacky here...
bool    TEegEgiNsrMainHeader::GetDataBlocks ( std::vector<std::vector<long>>& db, long *tf, unsigned long &blocksize )
{
int                 numsessions     = GetNumSessions();
TArray1<int>        startingblock  ( numsessions + 1 );
TArray1<long>       tosessioninfos ( numsessions + 1 );
TArray1<int>        blockcount     ( numsessions + 1 );

                                        // session infos
if ( !GetRecord ( 2, 37 ) )
    return false;
                                        // get starting codes for each session
                                        // if more than 32 sessions, use GetData
ToFile->seekg ( Record.Address2 + 4 );

for ( int i=0; i < numsessions; i++ )
    startingblock[ i ] = ReadLong ( ToFile, 4, ios::cur );
startingblock[ numsessions ]  = LONG_MAX;

                                        // if more than 32 sessions, use GetData
ToFile->seekg ( Record.Address1 + 4 );

for ( int i=0; i < numsessions; i++ )
    tosessioninfos[ i ] = ReadLong ( ToFile, 4, ios::cur );

                                        // get length in TF
for ( int i=0; i < numsessions; i++ )
    tf[ i ] = ReadLong ( ToFile, tosessioninfos[ i ] + 16 );


                                        // data blocks infos
if ( !GetRecord ( 3, 25 ) )
    return false;

                                        // read ID + data address
int                 numblocks   = GetNumData ( Record.Address1 );
vector<TEegEgiNsrTableCell>     idblocks ( numblocks );
GetData ( Record.Address1, idblocks.data () );

                                        // count total # of blocks
for ( int i=0; i < numsessions; i++ )
    blockcount[ i ] = 0;

int                 j           = 0;
int                 k;
unsigned long       bs;
size_t              atomsize    = GetAtomSize ();
blocksize   = 0;

for ( int i=0; i < numblocks; i++ ) {
    if ( idblocks[ i ].Id  >= startingblock[ j + 1 ] )
        j++;

//    DBGV4 ( j, idblocks[ i ].Id, startingblock[ j ], startingblock[ j + 1 ], "session  current id  start id  end id" );
    blockcount[ j ]++;
                                        // get current block size
    bs          = ReadLong ( ToFile, idblocks[ i ].Address + 10 ) / GetNumElectrodesInFile() / atomsize;
                                        // keep the max, as some short sessions will not return the real actual size
    blocksize   = max ( blocksize, bs );
    }

                                        // allocate arrays of blocks
for ( int i=0; i < numsessions; i++ )
    db[ i ].resize ( blockcount[ i ] );

/*
if ( Version == NsrVersion1 )
    //blocksize = ReadLong ( ToFile, idblocks[0] + 10 ) / 256;
    blocksize = ReadLong ( ToFile, idblocks[ 0 ].Address + 10 ) / GetNumElectrodesInFile() / atomsize;
else
    blocksize = 1024;   // seems to be a constant
    //blocksize = ReadLong ( ToFile, idblocks[ 0 ].Address + 33 ); // version 3 only? returns 1024 for 257, 512 for 129 el
                                        // works for all versions, but not ok if 1 small session
blocksize = ReadLong ( ToFile, idblocks[ 10 ].Address + 10 ) / GetNumElectrodesInFile() / atomsize;
*/

j = k = 0;
for ( int i=0; i < numblocks; i++ ) {
    db[ j ][ k++ ] = ReadLong ( ToFile, idblocks[ i ].Address + 6 );
                                    // skip to next session?
    if ( k == blockcount[ j ] ) {
        k = 0;
        j++;
        }
    }

return true;
}


//----------------------------------------------------------------------------
bool    TEegEgiNsrMainHeader::IsFloatTable ()
{
                                        // Heuristic to discriminate between short and float tables
                                        // Looking at the smallest relative differences between 3 numbers, with either the short or the float versions
long                currpos     = ToFile->tellg();


double              short1          = ReadShort ( ToFile, 0, ios::cur );
double              short2          = ReadShort ( ToFile, 0, ios::cur );
double              short3          = ReadShort ( ToFile, 0, ios::cur );
ToFile->seekg ( currpos );

                                        // old way: abs Laplacian
//double              stepshort       = fabs ( short1 - 2 * short2 + short3 );
double              stepshort       = RelativeDifference ( short1, short2, short3 );


double              float1          = ReadFloat ( ToFile, 0, ios::cur );
double              float2          = ReadFloat ( ToFile, 0, ios::cur );
double              float3          = ReadFloat ( ToFile, 0, ios::cur );
ToFile->seekg ( currpos );

                                        // have to check that now - this is a no-no
if ( IsNotAProperNumber ( float1 )
  || IsNotAProperNumber ( float2 )
  || IsNotAProperNumber ( float3 ) )
    return  false;

                                        // old way: abs Laplacian
//double              stepfloat       = fabs ( float1 - 2 * float2 + float3 );
double              stepfloat       = RelativeDifference ( float1, float2, float3 );

//DBGV2 ( stepfloat, stepshort, "IsFloatTable: stepfloat < stepshort" );

return  stepfloat < stepshort;
}

//----------------------------------------------------------------------------
bool    TEegEgiNsrMainHeader::GetZeros ( double *zeros, int session )
{
if ( !GetRecord ( 2, 32 ) )
    return false;

if ( Version == NsrVersion1 ) {
    long l1     = ReadLong ( ToFile, Record.Address1 + 8 );
    long l2     = ReadLong ( ToFile, l1 + 14 );
    long jump   = ReadLong ( ToFile, l2 + 22 );
                  ReadLong ( ToFile, jump + 13, ios::cur );

    //DBGV ( ToFile->tellg(), "Zero table @" );

    for ( int i=0; i < GetNumElectrodesInFile(); i++ ) {
        zeros[i] = ReadShort ( ToFile );

//      DBGV2 ( i + 1, zeros[i], "#  zeros" );
        }

    }
else { // NsrVersion2 || NsrVersion3 || Version == NsrVersion4

//  long l1     = ReadLong ( ToFile, Record.Address1 + 8 );

    long nsess  = ReadShort( ToFile, Record.Address1 + 6 );
    long l1     = ReadLong ( ToFile, Record.Address1 + 8 * ( session >= nsess || session < 0 ? nsess : session + 1 ) );
    long l2     = ReadLong ( ToFile, l1 + 14 );
    long jump   = ReadLong ( ToFile, l2 + 21 );

//  DBGV4 ( Record.Address1 + 8, l1 + 14, l2 + 21, ToFile->tellg() + jump, "Zeros:  Record.Address1+8  l1+14  l2+21  Gain table @" );

    ToFile->seekg ( jump, ios::cur );


    bool                typefloat       = IsFloatTable ();

//  DBGV3 ( session + 1, nsess, typefloat, "Zeros:  session  #sessions  typefloat" );


    for ( int i=0; i < GetNumElectrodesInFile() - 1; i++ ) {

        if ( typefloat )
            zeros[ i ] = ReadFloat ( ToFile );
        else
            zeros[ i ] = ReadShort ( ToFile );

//      DBGV3 ( typefloat, i + 1, zeros[ i ], "Zeros:  isfloat  #  zero" );
        }

    zeros[ GetNumElectrodesInFile() - 1 ] = 0;
    }

return true;
}


//----------------------------------------------------------------------------
bool    TEegEgiNsrMainHeader::GetGains ( double *gains, int session )
{
                                        // this test does not seem to be enough
                                        // for version 2
if ( !GetRecord ( 2, 31 ) )
    return false;

if ( Version == NsrVersion1 ) {
    long l1     = ReadLong ( ToFile, Record.Address1 + 8 );
    long l2     = ReadLong ( ToFile, l1 + 14 );
    long jump   = ReadLong ( ToFile, l2 + 22 );
                  ReadLong ( ToFile, jump + 13, ios::cur );

//    for ( int i=0; i < n; i++ ) {
    for ( int i=0; i < GetNumElectrodesInFile(); i++ ) {
        gains[i] = ReadShort ( ToFile );
        }
    }
else if ( Version == NsrVersion2 ) {
    long l1     = ReadLong ( ToFile, Record.Address1 + 8 );
    long l2     = ReadLong ( ToFile, l1 + 14 );
    long jump   = ReadLong ( ToFile, l2 + 21 );

    ToFile->seekg ( jump, ios::cur );

    for ( int i=0; i < GetNumElectrodesInFile(); i++ ) {
        gains[i] = ReadShort ( ToFile );
        }
    }
else { // Version == NsrVersion3  || Version == NsrVersion4

//  long l1     = ReadLong ( ToFile, Record.Address1 + 8 );

    long nsess  = ReadShort( ToFile, Record.Address1 + 6 );
    long l1     = ReadLong ( ToFile, Record.Address1 + 8 * ( session >= nsess || session < 0 ? nsess : session + 1 ) );
    long l2     = ReadLong ( ToFile, l1 + 14 );
    long jump   = ReadLong ( ToFile, l2 + 21 );

//  DBGV4 ( Record.Address1 + 8, l1 + 14, l2 + 21, ToFile->tellg() + jump, "Gains:  Record.Address1+8  l1+14  l2+21  Gain table @" );

    ToFile->seekg ( jump, ios::cur );


    bool                typefloat       = IsFloatTable ();

//  DBGV3 ( session + 1, nsess, typefloat, "Gains:  session  #sessions  typefloat" );

/*
    double  oldgain;
    TExportFile   expfile;
    StringCopy ( expfile.Filename, "E:\\Data\\EGI Old vs New Gains.ep" );
    expfile.NumTracks           = 3;
    expfile.NumTime             = GetNumElectrodesInFile();
*/

    for ( int i=0; i < GetNumElectrodesInFile(); i++ ) {

        if ( typefloat )
            gains[ i ] = ReadFloat ( ToFile );
        else
            gains[ i ] = ReadShort ( ToFile );

/*
        if ( typefloat ) {
            oldgain = ReadShort ( ToFile, -4, ios::cur );
            ReadShort ( ToFile );
            }
        else
            oldgain = ReadShort ( ToFile, -2, ios::cur );

        expfile.Write ( oldgain ); expfile.Write ( gains[ i ] ); expfile.Write ( oldgain / gains[ i ] );
*/

//      DBGV3    ( typefloat, i + 1, gains[ i ], "Gains:  isfloat  #  gain" );
        }

//  DBGV3    ( gains[ 0 ], gains[ 10 ], gains[ 20 ], "Gains" );
    }

return true;
}


bool    TEegEgiNsrMainHeader::GetBoardGain ( double &boardgain )
{
if ( !GetRecord ( 2, 29 ) )
    return false;

boardgain = ReadShort ( ToFile, Record.Address2 + 6 );

//DBGV ( boardgain, "boardgain" );

return true;
}


//----------------------------------------------------------------------------
                                        // microVolts value per number of bits (1000/12, 1600/16, 5000/16...)
bool    TEegEgiNsrMainHeader::GetScale ( double &muvolts, double &numbits )
{
if ( !GetRecord ( 2, 30 ) )
    return false;

long l1     = ReadLong ( ToFile, Record.Address1 + 8 );

if ( Version == NsrVersion4 ) {
    muvolts = ReadLong ( ToFile, l1 + 14 );
    numbits = ReadLong ( ToFile );
    }
else {
    muvolts = ReadShort ( ToFile, l1 + 14 );
    numbits = ReadShort ( ToFile );
    }

//DBGV4 ( Record.Address1 + 8, l1 + 14, muvolts, numbits, "Scale:  Record.Address1+8  l1+14  muvolts  numbits" );
//DBGV2 ( muvolts, numbits, "muvolts numbits" );

return true;
}


//----------------------------------------------------------------------------
                                        // We are past hacking here and entering voodoo territory
                                        // I'd recommend NOT to modify this code
void    TEegEgiNsrMainHeader::GetTriggers ( vector<TMarker>& tl )
{
tl.resize ( 0 );

int                 numtrig         = 0;
bool                newtriggers     = false;


if ( ! GetRecord ( 6, 36 ) ) {          // usual list
    if ( ! GetRecord ( 6, 34 ) )        // but it can be this list, too
        return;

    newtriggers     = true;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // create a temp list of markers
int                 maxnumtrig      = Record.L1;
vector<TMarker>     tlm ( maxnumtrig );

                                        // read ID + Time
                                        // this list is sorted both by ID and time
int                 numidtime       = GetNumData ( Record.Address2 );
vector<TEegEgiNsrTableCell> idtime ( numidtime );
GetData ( Record.Address2, idtime.data (), true );


for ( int i = 0; i < numidtime; i++ ) {
    tlm[ i ].To     = idtime[ i ].Id;   // store ID
    tlm[ i ].From   = idtime[ i ].Data; // time in ms, NOT in TF
    }

                                        // read trigger strings adresses
int                 numidstr        = GetNumData ( Record.Address1 );
vector<TEegEgiNsrTableCell>     idstr ( numidstr );
GetData ( Record.Address1, idstr.data () );

                                        // real strings array, lexicographically sorted
//vector<TEegEgiNsrTableCell>     idstr2 ( numidstr );
//GetStrings ( Record.Address6, idstr2.data () );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char                buff[ 256 ];
int                 lenstr;
bool                TriggerUSB;
bool                TriggerMultiport;
bool                TriggerDIN;

                                        // find out the type of trigger: USB or 4 DINs?
                                        // simply try to read first trigger "USB name"
ReadString ( ToFile, buff, idstr[ 0 ].Address + 128 );
//DBGM ( buff, "trigger type" );

TriggerUSB      = StringIsNotEmpty ( buff ); // if no string here, it should be a digital input (DIN1, DIN2...)


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // see first block
//long    sessionstring = ReadLong ( ToFile, ReadLong ( ToFile, ReadLong ( ToFile, Record.Address1 + 8 ) + 8 ) + 14 ) + 17;
//ReadString ( ToFile, buff, sessionstring ); // after that string, you can find useful information as Session info, subject stuff...
ReadString ( ToFile, buff, idstr[ 0 ].Address + 49 );

//DBGM ( buff, "trigger info" );        // can be: Net Amps Digital Inputs / Net Amps² USB / Multi-Port ECI


StringToLowercase ( buff );
TriggerMultiport    = StringStartsWith ( buff, "Multi-Port", 10 );

if ( TriggerMultiport )
    TriggerUSB          = false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TriggerDIN      = ! ( TriggerMultiport || TriggerUSB );

if ( newtriggers ) {
    TriggerMultiport    = true;
    TriggerUSB          = false;
    TriggerDIN          = false;
    }

//DBGV3 ( TriggerMultiport, TriggerUSB, TriggerDIN, "Triggers: MultiPort USB DIN") ;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // create final list
if ( TriggerMultiport ) {

    int                 i,  j,  n;
    short               cellnum;
    MarkerType          markertype;
    ushort              dintype;

    numtrig             = numidtime;
    tl.resize ( numtrig );
    char                eventstring[ 256 ];

                                        // process the addresses to jump above strings
    for ( i = 0; i < numtrig; i++ ) {
        if ( newtriggers ) {
            long l1     = ReadLong ( ToFile, idstr[ i ].Address + 14 );
            ReadString ( ToFile, buff, l1 + 17 );   // skip that string
            }
        else
            ReadString ( ToFile, buff, idstr[ i ].Address + 49 );

        idstr[ i ].Address = ToFile->tellg ();
        }

                                        // then copy
    for ( i = 0; i < numtrig; i++ ) {

                                        // scan to match string Id and trigger Id, because some triggers can be send back from the "past"
                                        // do a "jerky loop", starting from center, +1 -1, +2 -2, etc... + positive modulus
        for ( j = i, n = 1; n <= numtrig; n++, j = ( i + ( n & 1 ? - ( n >> 1 ) : n >> 1 ) + numtrig ) % numtrig )
            if ( idtime[ j ].Id == idstr[ i ].Id )
                break;


        ToFile->seekg ( idstr[ i ].Address );

        lenstr  = 4;                    // read the 4 chars of event Code
        ToFile->read ( eventstring, lenstr );
        eventstring[lenstr] = 0;

        StringCleanup ( eventstring );


//        DBGM ( eventstring, "trigger type" );


        if ( StringContains ( (const char*) EgiEventCodes, (const char*) eventstring ) )
            markertype  = MarkerTypeEvent;
        else
            markertype  = MarkerTypeTrigger;


        if ( markertype == MarkerTypeTrigger && ! newtriggers ) {
//            DBGV ( idstr[ i ].Address, "trigger address" );
/*
            int     j;
            for ( j=0; j < numtrig; j++ )
                if ( idstr2[ j ].Id == idstr[ i ].Id )
                    break;

            if ( j < numtrig )
                ReadString ( ToFile, buff, idstr2[ j ].Address );
            else
                StringCopy ( buff, "Id string not found" );
*/
                                        // for special coding purpose only
            #define     DINOFFSET       0xFFF0

                                        // retrieve only the cell number that has been used, let the string be empty
            if ( StringStartsWith ( eventstring, "DIN" ) ) // case where DINs have been mixed with Multi-Port
                cellnum     = (ushort) ( DINOFFSET + eventstring[ 3 ] - '1' ); // add a special code
            else
                cellnum     = ReadShort ( ToFile, 57, ios::cur );

            tlm[ j ].Code   = (ushort) cellnum;
//          IntegerToString ( buff, cellnum );
            ClearString ( buff );
            }
        else {
                                        // ok for non-triggers events, retrieve the string associated with this cell
//            ReadString ( ToFile, buff );

//            if ( StringIs ( eventstring, "DIN" ) )    // override is ~ok, but it is 0 there...
//                ReadString ( ToFile, buff, idstr[ i ].Address + 65 );
//            else
                ReadString ( ToFile, buff );

                                        // here we find the cell number that has been used
            cellnum         = ReadShort ( ToFile, 56, ios::cur );
            tlm[ j ].Code   = (ushort) cellnum;
//          IntegerToString ( buff, cellnum );
//          StringCopy ( buff, eventstring );
            }

                                        // set trigger string from the cell number
//        if ( *buff == 0 )
//            sprintf ( buff, "%s %0u", eventstring, tlm[ j ].Code );
                                        // set trigger string as the 4 chars event string
//        StringCopy ( buff, eventstring );

        tl[ i ] = TMarker ( tlm[ j ].From, tlm[ j ].From, tlm[ j ].Code, buff, markertype );
        }

                                        // re-scan the triggers, and complete the empty strings, using the trigger codes
    for ( i = 0; i < numtrig; i++ ) {
        if ( ! tl[ i ].Name[ 0 ] ) // && tl[ i ].Type == MarkerTypeTrigger )
            if ( tl[ i ].Code >= DINOFFSET  ) {  // DIN case?
                                        // convert this DIN to int
                dintype     = (ushort) ( 1 << ( tl[ i ].Code - DINOFFSET ) );

                                        // try to do a binary OR to cumulate following DINs
                                        // we assume that no EVENTS are mixed between DINs of same position...
                int     toshift = 0;
                for ( j = i + 1; j < numtrig; j++ ) {
                    if ( StringIsNotEmpty ( tl[ j ].Name ) || tl[ j ].From > tl[ i ].From )
                        break;

                    if ( tl[ i ].From == tl[ j ].From ) {
                        dintype        |= (ushort) ( 1 << ( tl[ j ].Code - DINOFFSET ) );
                        tl [ j ].Code   = 0; // set this trigger to null
                        ClearString ( tl[ j ].Name );
                        toshift++;
                        break;
                        }
                    } // for j

                if ( toshift > 0 ) {    // some triggers to zap?

                    if ( i < numtrig - 1 && numtrig - ( i + 1 + toshift ) > 0 )

                        std::move ( tl.begin () + ( i + 1 + toshift ), tl.end (), tl.begin () + ( i + 1 ) );
                                        // reduce numtrig by amount of zapped triggers
                    numtrig -= toshift;
                    }

//              sprintf ( buff, "DIN%0d", tl[ i ].Code - DINOFFSET );
                                        // set to the cumulated DIN number
                IntegerToString ( tl[ i ].Name, dintype );
                } // if DIN
            else
                for ( j = i - 1; j >= 0; j-- )  // search backward for the same code, with non-empty string
                    if ( tl[ j ].Code == tl[ i ].Code && tl[ j ].Name[ 0 ] ) {
                        StringCopy ( tl[ i ].Name, tl[ j ].Name );
                        break;
                        }
        }

                                        // re-scan the triggers, prefixing events with their Code
    for ( i=0; i < numtrig; i++ )

        if ( StringIsEmpty ( tl[ i ].Name ) )
            StringCopy ( tl[ i ].Name, "0" );

        else { // if ( tl[ i ].Type == MarkerTypeEvent ) {

            ToFile->seekg ( idstr[ i ].Address );

            lenstr  = 4;                // read the 4 chars of event coding
            ToFile->read ( eventstring, lenstr );
            eventstring[lenstr] = 0;

            StringCleanup ( eventstring );

//            DBGM ( eventstring, "trigger type" );


            if ( StringIsNot ( eventstring, "DIN" ) ) {

                if ( ! StringStartsWith ( eventstring, "DIN" ) ) {
                    StringAppend ( eventstring, ":", tl[ i ].Name );
                    StringCopy   ( tl[ i ].Name, eventstring );
                    }
//              else
//                  eventstring[ 3 ] = 0;           // remove 4th letter, which is garbage, prepend only "DIN"
//                  ClearString ( eventstring );    // don't prepend triggers with "DIN"
                }
            else                        // not sure if this case still holds?
                StringCopy ( tl[ i ].Name, eventstring );   // DIN case not ok right now...

            }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else if ( TriggerUSB ) {                // the simplest way

    numtrig             = numidtime;
    tl.resize ( numtrig );

                                        // then copy
    for ( int i = 0; i < numtrig; i++ ) {

        ReadString ( ToFile, buff, idstr[ i ].Address + 128 );

        tl[ i ]     = TMarker ( tlm[ i ].From, tlm[ i ].From, tlm[ i ].Code, buff, MarkerTypeTrigger );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else if ( TriggerDIN ) {                // trigger DINs, the long way

    int                 i,  j;
                                        // read ID + din
                                        // not sorted
    int                 numiddin        = GetNumData ( Record.Address4 );
    vector<TEegEgiNsrTableCell>     iddin ( numiddin );
    GetData ( Record.Address4, iddin.data (), true );

                                        // add din info to temp list
    long                din;
    int                 shift;
    bool                decimalcoded    = false;
    bool                binarycoded     = false;
    bool                expandedcoded   = false;

                                        // scan the DIN's and see the names
    buff[ 4 ] = 0;



/*                                      // counting overlapping triggers
    int                 numoverlapping  = 0;
                                        // overlapping means merging
    for ( i = 0; i < numidtime; i++ )
        for ( j = i + 1; j < numidtime; j++ ) {

            if ( tlm[ j ].From > tlm[ i ].From ) break;

            if ( tlm[ j ].From == tlm[ i ].From ) {
                numoverlapping++;
                break;
                }
            }

    DBGV ( numoverlapping, "num overlapping DINs" );
*/


                                        // scan DIN names
    for ( int i = 0; i < numidtime; i++ ) {
                                        // DIN names (not the trigger name)
        din     = iddin[ i ].Data;
        SwapLong ( &din );
        memcpy ( buff, &din, 4 );
//        DBGM ( buff, "DIN name" );

        binarycoded    |= IsStringAmong ( buff, "DIN3 DIN5 DIN6 DIN7" );    // can not be produced by decimal codes
        decimalcoded   |= IsStringAmong ( buff, "DI16 DI32 DI64 D128" );    // these numbers can not be binary codes
        expandedcoded  |= IsStringAmong ( buff, "D100 D101 D102 D103 D104 D105 D106 D107 D108 D109 D110 D111 D112 D113 D114 D115 D116 D117 D118 D119 D120 D121 D122 D123 D124 D125 D126 D127" );   // these codes when 255 channels selected

        if ( decimalcoded || binarycoded || expandedcoded )
            break;
        }

                                        // scan again trigger names, in case wrong detection
    if ( binarycoded ) {
        int                 dinv;

        for ( int i = 0; i < numidtime; i++ )
            for ( int j = 0; j < numiddin; j++ )

                if ( iddin[ j ].Id == idtime[ i ].Id ) {

                    din     = iddin[ j ].Data;
                    SwapLong ( &din );
                    memcpy ( buff, &din, 4 );
                    buff[ 4 ] = 0;
//                    DBGM ( buff, "Trigger name" );


                    KeepChars ( buff, "0123456789" );

                    dinv    = StringToInteger ( buff );
    //                DBGV ( dinv, buff );

                    if ( dinv > 7 ) {
                        binarycoded     = false;
                        break;
                        }
                    }
        }

                                        // if neither case, force one
    if ( ! ( decimalcoded || binarycoded || expandedcoded ) )
        decimalcoded    = true;

//    DBGV3 ( decimalcoded, binarycoded, expandedcoded, "decimalcoded  binarycoded  expandedcoded" );


    buff[ 4 ] = 0;

    for ( int i = 0; i < numidtime; i++ )
        for ( int j = 0; j < numiddin; j++ )

            if ( iddin[ j ].Id == idtime[ i ].Id ) {
                                        // look at trigger name (not DIN name)
                din     = iddin[ j ].Data;
                SwapLong ( &din );
                memcpy ( buff, &din, 4 );
//                DBGM ( buff, "Trigger name" );

                if ( decimalcoded || expandedcoded ) {
                                        // extract value from string: "DIN3", "DI23" "D123"...
                    KeepChars ( buff, "0123456789" );

                    tlm[ i ].Code   = (ushort) StringToInteger ( buff );
                    }
                else if ( binarycoded ) {

                    shift = buff[ 3 ] - '1';
                                        // up to 8 bits are allowed
                    if ( shift >= 0 && shift <= 7 )
                        tlm[ i ].Code   = (ushort) ( 1 << shift );  // convert last char to a bit position
                    else
                        tlm[ i ].Code   = (ushort) ( 1 << 8 );
                    }
                else // shouldn't happen
                    tlm[ i ].Code   = (ushort) ( 1 << 8 );

                break;
                }

                                        // now cumulate dins at same time
    for ( i = 0; i < maxnumtrig; i++ ) {
        if ( ! tlm[ i ].Code ) continue;

        for ( j = i + 1; j < maxnumtrig; j++ ) {
            if ( tlm[ j ].From > tlm[ i ].From ) break;

            if ( tlm[ j ].From == tlm[ i ].From ) {
//              DBGV4 ( i, j, tlm[i].Code, tlm[j].Code, "i j codei codej" );
                tlm[ i ].Code  |= tlm[ j ].Code;
                tlm[ j ].Code   = 0;    // mark this trigger to null
                }
            }
        }

                                        // count remaining non-null triggers
    j = 0;
    for ( i = 0; i < maxnumtrig; i++ )
        if ( tlm[ i ].Code )    j++;
                                        // create the final list
    numtrig = j;
    tl.resize ( numtrig );
                                        // then copy
    j = 0;
    char    tname[20];
    for ( i = 0; i < maxnumtrig; i++ )
        if ( tlm[ i ].Code ) {
            IntegerToString ( tname, tlm[ i ].Code );
            tl[ j ] = TMarker ( tlm[ i ].From, tlm[ i ].From, tlm[ i ].Code, tname, MarkerTypeTrigger );
            j++;
            }

    } // TriggerDIN

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Tadaa... everything is clean again
        TEegEgiNsrDoc::TEegEgiNsrDoc ( TDocument *parent )
      : TTracksDoc ( parent )
{
InputStream         = 0;

Version             = 0;
AtomSize            = 0;

TFPerBlock          = 0;
Block               = 0;
Offset              = 0;

NumElectrodesInFile = 0;
Reference           = ReferenceAsInFile;
}


bool	TEegEgiNsrDoc::Close ()
{
if ( InputStream ) {
    delete  InputStream;
    InputStream = 0;
    }

return  TFileDocument::Close ();
}


bool	TEegEgiNsrDoc::CanClose ()
{                                       // can not save this file
SetDirty ( false );

return TTracksDoc::CanClose ();
}


//----------------------------------------------------------------------------
void    TEegEgiNsrDoc::ReadNativeMarkers ()
{
ifstream        ifs ( TFileName ( GetDocPath (), TFilenameExtendedPath ), ios::binary );

TEegEgiNsrMainHeader  mainheader ( &ifs );
                                        // read the markers
                                        // ! the list is given in [ms], so convert it
vector<TMarker>     markers;
                    mainheader.GetTriggers ( markers );
long                offsetms        = mainheader.GetSessionOffset ( CurrSequence );

if ( markers.size () ) {
    long                from;
    long                to;

    for ( int i = 0; i < markers.size (); i++ ) {
                                                             // round to millisecond
        from    = Truncate ( MillisecondsToTimeFrame ( markers[ i ].From - offsetms + 0.5, SamplingFrequency ) );
        to      = Truncate ( MillisecondsToTimeFrame ( markers[ i ].To   - offsetms + 0.5, SamplingFrequency ) );

        if ( IsInsideLimits ( from, to, (long) 0, NumTimeFrames - 1 ) )
            AppendMarker ( TMarker ( from, to, markers[i].Code, markers[i].Name, markers[i].Type ) );
        }
    }


SortAndCleanMarkers ();
}


//----------------------------------------------------------------------------
bool	TEegEgiNsrDoc::Open	( int /*mode*/, const char* path )
{
if ( path )
    SetDocPath ( path );

SetDirty ( false );


if ( GetDocPath () ) {

    char                buff[ 256 ];
    ifstream            ifs ( TFileName ( GetDocPath (), TFilenameExtendedPath ), ios::binary );

                                        // get everything from this
    TEegEgiNsrMainHeader  mainheader ( &ifs );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fill product info
    StringCopy ( CompanyName, "Electrical Geodesics" );
    StringCopy ( ProductName, FILEEXT_EEGNSR );
    Version             = mainheader.GetVersion ();
    Subversion          = mainheader.GetNumElectrodes ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    NumElectrodesInFile = mainheader.GetNumElectrodesInFile (); // used to access data in file
    NumElectrodes       = mainheader.GetNumElectrodes ();
    TotalElectrodes     = NumElectrodes + NumPseudoTracks;
    SamplingFrequency   = mainheader.GetSamplingFrequency();
    AtomSize            = mainheader.GetAtomSize ();


    NumSequences        = mainheader.GetNumSessions ();

    if ( NumSequences == 0 ) {
        StringCopy  ( buff, "There is no session in this file," NewLine "can not open file!" );
        ShowMessage ( buff, GetDocPath(), ShowMessageWarning );
        return false;
        }
//  else if ( HasMultipleSessions () ) {
//      StringCopy  ( buff, "There are ", IntegerToString ( NumSequences )," sessions in this file," NewLine "only one can be used at a time." );
//      ShowMessage ( buff, GetDocPath(), ShowMessageWarning );
//      }


    DataBlocks.resize ( NumSequences );
    TimeFrames.resize ( NumSequences );

    if ( ! mainheader.GetDataBlocks ( DataBlocks, TimeFrames.data (), TFPerBlock ) ) {
        StringCopy  ( buff, "An error occured while reading the data." );
        ShowMessage ( buff, GetDocPath(), ShowMessageWarning );
        return false;
        }

                                        // using the first sequence at opening time, or the next one if it appears to be too short
    CurrSequence        = 0;

    if ( HasMultipleSessions ()
      && TimeFrames[ 0 ] < TimeFrames[ 1 ] / 2 )
        CurrSequence        = 1;


    mainheader.GetDateTime ( DateTime, CurrSequence );
//  DBGV3 ( DateTime.GetHour (), DateTime.GetMinute (), DateTime.GetSecond (), "" );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    int             BuffSize            = mainheader.GetAtomSize () * NumElectrodesInFile;
    FileBuff.resize ( BuffSize );

                                        // length of current session, only
    NumTimeFrames       = TimeFrames[ CurrSequence ];
    if ( NumTimeFrames <= 0 ) {
        StringCopy  ( buff, "An error occured while reading session length," NewLine "found ", IntegerToString ( NumTimeFrames ), " TF." );
        ShowMessage ( buff, GetDocPath(), ShowMessageWarning );
        return false;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Gains.resize ( NumElectrodesInFile );
    Zeros.resize ( NumElectrodesInFile );

    if ( ! SetGainsZeros ( mainheader, CurrSequence ) )
        return false;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    InputStream         = InStream ( ofRead | ofBinary );
    if ( !InputStream ) return false;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // space allocation + default names
    if ( ! SetArrays () ) {
        delete  InputStream; InputStream = 0;
        return false;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // ask the registry, or scan electrode names
    InitAuxiliaries ();

//  if ( ! (bool) AuxTracks )
//      mainheader.SetAuxElectrodes ( AuxTracks );

    UpdateTitle ();

                                        // warn user about the additional line?
//  if ( Version == NsrVersion1 ) {
//      StringCopy  ( buff, "An additional 0 line has been added to NetStation1 file" NewLine
//                          NewLine
//                          Tab "Original number of electrodes= ", IntegerToString ( NumElectrodes - 1 ), NewLine 
//                          NewLine,
//                          Tab "New number of electrodes      = ", IntegerToString ( NumElectrodes )
//                  );
//      ShowMessage ( buff, ToFileName ( GetDocPath () ), ShowMessageWarning );
//      }
    }
else {                                  // can not create
    return false;
    }


return true;
}


//----------------------------------------------------------------------------
bool    TEegEgiNsrDoc::SetGainsZeros ( TEegEgiNsrMainHeader &mainheader, int session )
{
bool                gg              = mainheader.GetGains ( Gains.data (), session );
bool                gz              = mainheader.GetZeros ( Zeros.data (), session );


if ( ! gz || ! gg ) {

    char    buff[ 256 ];

    if ( ! ( gg && mainheader.IsAtomFloat () ) ) {

        sprintf ( buff, "This file does not seem to be calibrated properly:" NewLine 
                        Tab "Zeroes: %s" NewLine 
                        Tab "Gains:  %s" NewLine 
                        "Do you wish to open it anyway?", 
                        gz ? "found" : "not found", gg ? "found" : "not found" );

        if ( ! GetAnswerFromUser ( buff, GetDocPath() ) )
            return false;
            }
                                    // provide default values if missing
    if ( ! gz )
        for ( int i=0; i < NumElectrodesInFile; i++ )
            Zeros[i]    = 0;

    if ( ! gg )
        for ( int i=0; i < NumElectrodesInFile; i++ )
            Gains[i]    = Version == NsrVersion1 ?  1850.0 : // better than 400 / 0.24
                          Version == NsrVersion2 ? 16480.0 : // ad-hoc value, maybe 16384?
                          Version == NsrVersion3 ?  1850.0 :
                          Version == NsrVersion4 ?  1850.0 :
                                                    1850.0;
    }


double  regain = Version == NsrVersion1 ?   400 :
                 Version == NsrVersion2 ?   400 : // 1345
                 Version == NsrVersion3 ?   400 :
                 Version == NsrVersion4 ?   400 :
                                            400;

                                        // high gains (200'000 rather than 5'000) means the constant is 5000, not 400
double  avggains    = 0;
for ( int i=0; i < NumElectrodesInFile; i++ )
    avggains    += Gains[ i ];
avggains   /= NumElectrodesInFile;

if ( Version >= NsrVersion4 && avggains > 32768 )
    regain  = 5000;


/*
double  muvolts, numbits;
if ( ! mainheader.GetScale ( muvolts, numbits ) ) {
    muvolts = 1000;
    numbits = 12;
    }

regain = muvolts / pow ( 2, numbits );
DBGV3 ( muvolts, numbits, regain, "muvolts  numbits  regain" );
*/

double  boardgain;
if ( ! mainheader.GetBoardGain ( boardgain ) || boardgain <= 0 )
    boardgain = 1;

regain *= boardgain;                // correct? in our cases, boardgain is always 1


//DBGV3 ( Version, regain, boardgain, "Version, regain, boardgain" );


for ( int i=0; i < NumElectrodesInFile; i++ )
    Gains[ i ] = Gains[ i ] ? regain / Gains[ i ] : 1.0;


return  true;
}


//----------------------------------------------------------------------------
bool    TEegEgiNsrDoc::SetArrays ()
{
OffGfp              = NumElectrodes + PseudoTrackOffsetGfp;
OffDis              = NumElectrodes + PseudoTrackOffsetDis;
OffAvg              = NumElectrodes + PseudoTrackOffsetAvg;

                                        // do all allocations stuff
Tracks.resize ( TotalElectrodes );


ElectrodesNames.Set ( TotalElectrodes, ElectrodeNameSize );

for ( int i = 1; i <= NumElectrodes; i++ )
    IntegerToString ( ElectrodesNames[ i - 1 ], i );
                                        // overriding reference name
StringCopy ( ElectrodesNames[ NumElectrodes - 1 ], Version == NsrVersion3 || Version == NsrVersion4 ? "REF" : "VREF" );

StringCopy ( ElectrodesNames[ OffGfp ], TrackNameGFP );
StringCopy ( ElectrodesNames[ OffDis ], TrackNameDIS );
StringCopy ( ElectrodesNames[ OffAvg ], TrackNameAVG );


return true;
}


//----------------------------------------------------------------------------
void    TEegEgiNsrDoc::ReadRawTracks ( long tf1, long tf2, TArray2<float> &buff, int tfoffset )
{
                                        // Update Block + Offset
auto                TfToBlockOffset = [ this ] ( const long& tfi )
{
Block   = DataBlocks[ CurrSequence ][ tfi / TFPerBlock ];
Offset  = ( tfi  % TFPerBlock ) * AtomSize * NumElectrodesInFile;
};

long                oldBlock = -1;


for ( long tfi = tf1, tf = 0; tfi <= tf2; tfi++, tf++ ) {

    TfToBlockOffset ( tfi );

    if ( Block != oldBlock ) {          // read 1 new block
        InputStream->seekg ( Block + Offset );
        oldBlock    = Block;
        }


	InputStream->read ( (char*) FileBuff.data (),  FileBuff.size () );


    if ( AtomSize == 4 ) {              // float version

        SwapLong ( FileBuff.data (), NumElectrodesInFile );

        float*      toFBF   = (float *) FileBuff.data ();

        for ( int el = 0; el < NumElectrodesInFile; el++, toFBF++ )
            Tracks[ el ]    = ( *toFBF - Zeros[ el ] ) * Gains[ el ];
        }
    else {                              // integer version

        SwapShort ( FileBuff.data (), NumElectrodesInFile );

        short*      toFBS   = (short *) FileBuff.data ();

        for ( int el = 0; el < NumElectrodesInFile; el++, toFBS++ )
            Tracks[ el ]    = ( *toFBS - Zeros[ el ] ) * Gains[ el ];
        }


    if ( Version == NsrVersion1 )
        Tracks[ NumElectrodes - 1 ] = 0;    // we added this track, so set it to 0


    for ( int el = 0; el < NumElectrodes; el++ )
        buff ( el, tfoffset + tf ) = Tracks[ el ];
    }
}


//----------------------------------------------------------------------------
bool    TEegEgiNsrDoc::UpdateSession ( int newsession )
{
ifstream            ifs ( TFileName ( GetDocPath (), TFilenameExtendedPath ), ios::binary );
                                        // get everything from this
TEegEgiNsrMainHeader  mainheader ( &ifs );


NumTimeFrames       = TimeFrames[ newsession ];

if ( NumTimeFrames <= 0 ) {
    char                buff[256];
    StringCopy  ( buff, "An error occured while reading session length," NewLine "found ", IntegerToString ( NumTimeFrames ), " TF." );
    ShowMessage ( buff, GetDocPath(), ShowMessageWarning );

    NumTimeFrames   = TimeFrames[ CurrSequence ];
    return  false;
    }

                                        // update new starting date & time
mainheader.GetDateTime ( DateTime, newsession );

if ( ! SetGainsZeros ( mainheader, newsession ) )
    return  false;

return  true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
