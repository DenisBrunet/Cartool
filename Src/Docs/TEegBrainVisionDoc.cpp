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
#include    "Strings.TFixedString.h"

#include    "TArray1.h"
#include    "Strings.Grep.h"

#include    "TEegBrainVisionDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TEegBrainVisionDoc::TEegBrainVisionDoc ( TDocument *parent )
      : TTracksDoc ( parent )
{
Multiplexed         = true;
AscSkipLines        = 0;
AscSkipColumns      = 0;

DataType            = BVTypeNone;
BinTypeSize         = 0;
BuffSize            = 0;

Reference           = ReferenceAsInFile;
}


bool	TEegBrainVisionDoc::Close ()
{
InputStream.close ();

return  TFileDocument::Close ();
}


bool	TEegBrainVisionDoc::CanClose ()
{                                       // can not save this file
SetDirty ( false );

return TTracksDoc::CanClose();
}


//----------------------------------------------------------------------------
void    TEegBrainVisionDoc::ReadNativeMarkers ()
{
TFileName           filemrk;
                                        // open marker file
StringCopy          ( filemrk, GetDocPath () );
ReplaceExtension    ( filemrk, FILEEXT_BVMRK );

if ( ! CanOpenFile  ( filemrk ) )
    return;


ifstream            ifmrk ( TFileName ( filemrk, TFilenameExtendedPath ) );
TFileName           buff;


ifmrk.getline ( buff, buff.Size () );

if ( ! StringGrep ( buff, BrainVisionGrepHeaderVmrk, GrepOptionDefaultFiles ) ) {
    StringCopy  ( buff, "Marker file seems of improper type." );
    ShowMessage ( buff, filemrk, ShowMessageWarning );
    return;
    }

//double              Version     = StringToDouble ( buff + StringLength ( buff ) - 3 );
//DBGV ( version, buff );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

BrainVisionHeaderPart   part        = NoInfos;
char                bvmtype[ 32 ];
char                bvmdesc[ 32 ];
long                fromtf;
long                totf;
int                 dd;
int                 MM;
int                 yy;
int                 hh;
int                 mm;
int                 ss;
int                 uu;
int                 timesign;
//int                 mkindex;


do {
    ifmrk.getline ( buff, MaxPathShort );

                                        // comment
    if ( StringStartsWith ( buff, ";" ) )   continue;

                                        // beginning of a block?
    if      ( StringIs ( buff, "[Marker Infos]"  ) )  { part = MarkerInfos;     continue; }
    else if ( StringIsEmpty ( buff ) )                { part = NoInfos;         continue; } // empty line ends current part


    if ( part == NoInfos )              // not within a known part, continue
        continue;


    if ( part == MarkerInfos ) {
                                        // Mk???=Type,Description,Start,Length,Channel#,VeryLongDate

                                        // tokenize by commas, up to the 6 token
        for ( char *toc = StringContains ( buff, '=' ) + 1, toci = 1; toci <= 6; toci++, toc += StringSize ( toc ) ) {
                                        // cut a bit of the string
            if ( StringContains ( toc, ',' ) )
                *StringContains ( toc, ',' ) = 0;

            if      ( toci == 1 )   StringCopy ( bvmtype, toc );
            else if ( toci == 2 ) { StringCopy ( bvmdesc, toc );    ReplaceChars ( bvmdesc, "\"", "" ); }
            else if ( toci == 3 )   fromtf  =          StringToInteger ( toc ) - 1;
            else if ( toci == 4 )   totf    = fromtf + StringToInteger ( toc ) - 1;

            else if ( toci == 6 ) {
                                        // non-standard workaround for negative offsets!
                if ( *toc == '-' ) {
                    timesign    = -1;
                    toc++;              // skip sign now
                    }
                else 
                    timesign    = 1;

                sscanf ( toc, "%4d%2d%2d%2d%2d%2d%6d", &yy, &MM, &dd, &hh, &mm, &ss, &uu );
                }
            }


        if      ( StringIs ( bvmtype, "New Segment" ) ) {

            DateTime    = TDateTime ( yy, MM, dd, hh, mm, ss, 0, timesign * uu );
            InitDateTime ();

            AppendMarker ( TMarker ( fromtf, totf, (MarkerCode) StringToInteger ( bvmdesc ),
                                     StringIsNotEmpty ( bvmdesc ) ? bvmdesc : StringIsNotEmpty ( bvmtype ) ? bvmtype : "New Segment",
                                     MarkerTypeEvent   ) );
            continue;
            }
        else if ( StringIs ( bvmtype, "Trigger"  )
               || StringIs ( bvmtype, "Stimulus" ) ) {

            AppendMarker ( TMarker ( fromtf, totf, (MarkerCode) StringToInteger ( bvmdesc ),
                                     StringIsNotEmpty ( bvmdesc ) ? bvmdesc : StringIsNotEmpty ( bvmtype ) ? bvmtype : MarkerNameTrigger,
                                     MarkerTypeTrigger ) );
            continue;
            }
        else { // Response, Comment, SyncStatus, Time_0 (?) or "Time 0" (??), "DC Correction" (?)

            AppendMarker ( TMarker ( fromtf, totf, (MarkerCode) StringToInteger ( bvmdesc ),
                                     StringIsNotEmpty ( bvmdesc ) ? bvmdesc : StringIsNotEmpty ( bvmtype ) ? bvmtype : MarkerNameEvent,
                                     MarkerTypeEvent   ) );
            continue;
            }

        } // MarkerInfos

    } while ( ! ifmrk.eof() );


SortAndCleanMarkers ();
}


//----------------------------------------------------------------------------
bool	TEegBrainVisionDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
                                        // open descriptor file
TFileName           buff;

StringCopy          ( buff, file );
ReplaceExtension    ( buff, FILEEXT_BVHDR );

ifstream            ifhdr ( TFileName ( buff, TFilenameExtendedPath ) );

if ( ifhdr.fail () )
    return false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // All the possible options start to be painful, maybe open the file directly...
char                s[64];
int                 NumElectrodes       = 0;
int                 NumAuxElectrodes    = 0;
long                NumTimeFrames       = 0;
BrainVisionDataType DataType;
int                 BinTypeSize;
double              samplint        = 0;
char                dataformat[ 32 ]    = { "" };
char                bintype   [ 32 ]    = { "" };


do {
    ifhdr.getline ( buff, buff.Size () );

    if ( StringIsEmpty ( buff ) )
        continue;

    if ( StringStartsWith ( buff, "NumberOfChannels=" ) ) {
        sscanf ( buff, "NumberOfChannels=%d", &NumElectrodes );
        }

    else if ( StringStartsWith ( buff, "Ch" ) ) {
        sscanf ( buff, "Ch%*d=%s,", s );
        if ( IsElectrodeNameAux ( s ) )
            NumAuxElectrodes++;
        }

    else if ( StringStartsWith ( buff, "DataPoints=" ) ) {
        sscanf ( buff, "DataPoints=%d", &NumTimeFrames );
        }

    else if ( StringStartsWith ( buff, "SamplingInterval=" ) ) {
        sscanf ( buff + StringLength ( "SamplingInterval=" ), "%lf", &samplint );
        }

    else if ( StringStartsWith ( buff, "DataFormat=" ) ) {
        sscanf ( buff, "DataFormat=%s", dataformat );
//      if ( ! IsStringAmong ( dataformat, BrainVisionBinaryString " " BrainVisionAsciiString ) )
        }

    else if ( StringStartsWith ( buff, "BinaryFormat=" ) ) {
        sscanf ( buff, "BinaryFormat=%s", bintype );
//        if ( ! IsStringAmong ( bintype, BrainVisionFloat32String " " BrainVisionInt16String " " BrainVisionUint16String ) )
        }

    } while ( ! ifhdr.eof() );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // f..king format that can ommit the number of TF!
if ( NumTimeFrames == 0 ) {

        if      ( StringIs ( dataformat, BrainVisionBinaryString ) ) {

            if      ( StringIs ( bintype, BrainVisionFloat32String ) ) {
                DataType    = BVTypeFloat32;
                BinTypeSize = sizeof ( float );
                }
            else if ( StringIs ( bintype, BrainVisionInt16String   ) ) {
                DataType    = BVTypeInt16;
                BinTypeSize = sizeof ( short );
                }
            else if ( StringIs ( bintype, BrainVisionUint16String  ) ) {
                DataType    = BVTypeUint16;
                BinTypeSize = sizeof ( ushort );
                }
            } // binary

        else if ( StringIs ( dataformat, BrainVisionAsciiString ) ) {
            DataType        = BVTypeAscii;
            BinTypeSize     = sizeof ( float );
            } // ASCII

        else {
            DataType        = BVTypeNone;
            BinTypeSize     = 0;
            }

                                        // finally, we can recover the number of time frames
    if ( BinTypeSize > 0 && NumElectrodes > 0 )
        NumTimeFrames   = GetFileSize ( file ) / ( BinTypeSize * NumElectrodes );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

switch ( what ) {

    case ReadNumElectrodes :

        *((int *) answer)   = NumElectrodes;
        return  NumElectrodes > 0;

    case ReadNumAuxElectrodes :

        *((int *) answer)   = NumAuxElectrodes;
        return  true;

    case ReadNumTimeFrames :

        *((int *) answer)   = NumTimeFrames;
        return  NumTimeFrames > 0;

    case ReadSamplingFrequency :

        *((double *) answer) = MicrosecondsToFrequency ( samplint );
        return  true;
    }


return false;
}


//----------------------------------------------------------------------------

bool    TEegBrainVisionDoc::Open ( int /*mode*/, const char* path )
{
if ( path )
    SetDocPath ( path );

SetDirty ( false );


if ( GetDocPath () ) {

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // open descriptor file
    TFileName           filehdr;
    char                buff[ KiloByte ];


    StringCopy          ( filehdr, GetDocPath () );
    ReplaceExtension    ( filehdr, FILEEXT_BVHDR );

    if ( ! CanOpenFile ( filehdr ) ) {
        StringCopy  ( buff, "Can not find header file" NewLine, filehdr );
        ShowMessage ( buff, "Opening error", ShowMessageWarning );
        return  false;
        }


    ifstream            ifhdr ( TFileName ( filehdr, TFilenameExtendedPath ) );

    ifhdr.getline ( buff, KiloByte );

                                        // some files have a bug and no space between Brain and Vision...
    if ( ! StringGrep ( buff, BrainVisionGrepHeaderVhdr, GrepOptionDefaultFiles ) ) {
        StringCopy  ( buff, "Header file seems of improper type." );
        ShowMessage ( buff, filehdr, ShowMessageWarning );
        return false;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fill product info
    StringCopy ( CompanyName, "Brain Products" );
    StringCopy ( ProductName, FILEEXT_EEGBV );
                                        // extract version
    Version     = StringToDouble ( buff + StringLength ( buff ) - 3 );
//    DBGV ( Version, buff + StringLength ( buff ) - 3 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // there seems to be the possibility of multiple sessions?
                                        // as in the vmrk associated file exist a marker "New Segment"
    BrainVisionHeaderPart   part            = NoInfos;
    char                dataformat[ 32 ];
    char                dataorient[ 32 ];
    char                bintype   [ 32 ]    = { "" };
    int                 numchannels;
    int                 numtf               = 0;
    double              samplint;
    int                 numfieldsneeded     = 0;
    bool                alreadyarrays       = false;
    int                 chindex;
    int                 asclines            = 0;
    int                 asccols             = 0;
    char                ascdec[16];//       = { "." };


    do {
        ifhdr.getline ( buff, KiloByte );

                                        // skip empty lines
        if ( StringIsEmpty ( buff ) )
            continue;

                                        // skip comment
        if ( StringStartsWith ( buff, ";" ) )   continue;

                                        // beginning of a block?
        if      ( StringIs ( buff, "[Common Infos]"  ) )    { part = CommonInfos;     continue; }
        else if ( StringIs ( buff, "[Binary Infos]"  ) )    { part = BinaryInfos;     continue; }
        else if ( StringIs ( buff, "[ASCII Infos]"   ) )    { part = AsciiInfos;      continue; }
        else if ( StringIs ( buff, "[Channel Infos]" ) )    { part = ChannelInfos;    continue; }
        else if ( StringIs ( buff, "[Coordinates]"   ) )    { part = Coordinates;     continue; }
        else if ( StringIs ( buff, "[Comment]"       ) )    { part = Comment;         continue; }
        else if ( StringStartsWith ( buff, "[" ) )          { part = NoInfos;         continue; } // unknown case
        else if ( StringIsEmpty    ( buff ) )               { part = NoInfos;         continue; } // empty line ends current part


        if ( part == NoInfos || part == Comment )
            continue;                   // not within a known part, continue


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // explore each block separately
        if      ( part == CommonInfos ) {

            if ( StringStartsWith ( buff, "DataFormat=" ) ) {
                sscanf ( buff, "DataFormat=%s", dataformat );
                if ( ! IsStringAmong ( dataformat, BrainVisionBinaryString " " BrainVisionAsciiString ) ) {
                    ShowMessage ( "Unrecognized file format!" NewLine "Data format is neither " BrainVisionBinaryString " nor " BrainVisionAsciiString ".", "Open File", ShowMessageWarning );
                    numfieldsneeded = -100;
                    break;
                    }
                continue;
                }

            if ( StringStartsWith ( buff, "DataOrientation=" ) ) {
                numfieldsneeded++;
                sscanf ( buff, "DataOrientation=%s", dataorient );
                if ( ! IsStringAmong ( dataorient, BrainVisionVectorizedString " " BrainVisionMultiplexedString ) ) {
                    ShowMessage ( "Unrecognized file format!" NewLine "Data orientation is neither " BrainVisionVectorizedString " nor " BrainVisionMultiplexedString ".", "Open File", ShowMessageWarning );
                    numfieldsneeded = -100;
                    break;
                    }
                continue;
                }

            if ( StringStartsWith ( buff, "NumberOfChannels=" ) ) {
                numfieldsneeded++;
                sscanf ( buff, "NumberOfChannels=%d", &numchannels );
                continue;
                }

            if ( StringStartsWith ( buff, "DataPoints=" ) ) {
//              numfieldsneeded++;
                sscanf ( buff, "DataPoints=%d", &numtf );
                continue;
                }

            if ( StringStartsWith ( buff, "SamplingInterval=" ) ) {
                numfieldsneeded++;     // some files are wrongly spelled  "Samplinginterval"
                sscanf ( buff + StringLength ( "SamplingInterval=" ), "%lf", &samplint );
//              sscanf ( buff, "SamplingInterval=%lf", &samplint );
                continue;
                }
            } // CommonInfos

            
        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        else if ( part == BinaryInfos ) {

            if ( StringStartsWith ( buff, "BinaryFormat=" ) ) {
                numfieldsneeded++;
                sscanf ( buff, "BinaryFormat=%s", bintype );
                if ( ! IsStringAmong ( bintype, BrainVisionFloat32String " " BrainVisionInt16String " " BrainVisionUint16String ) ) {
                    ShowMessage ( "Unrecognized file format!" NewLine "Binary format is neither " BrainVisionFloat32String ", " BrainVisionInt16String " nor " BrainVisionUint16String ".", "Open File", ShowMessageWarning );
                    numfieldsneeded = -100;
                    break;
                    }
                continue;
                }
            } // BinaryInfos


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        else if ( part == AsciiInfos ) {

            if ( StringStartsWith ( buff, "DecimalSymbol=" ) ) {
                numfieldsneeded++;
                sscanf ( buff, "DecimalSymbol=%s", ascdec );
                if ( StringIsNot ( ascdec, "," ) && StringIsNot ( ascdec, "." ) ) {
                    ShowMessage ( "Unrecognized file format!" NewLine "Decimal symbol is neither "","" nor "".""", "Open File", ShowMessageWarning );
                    numfieldsneeded = -100;
                    break;
                    }
                continue;
                }

            if ( StringStartsWith ( buff, "SkipLines=" ) ) {
                sscanf ( buff, "SkipLines=%d", &asclines );
                continue;
                }

            if ( StringStartsWith ( buff, "SkipColumns=" ) ) {
                sscanf ( buff, "SkipColumns=%d", &asccols );
                continue;
                }

            } // AsciiInfos


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        else if ( part == ChannelInfos ) {
                                        // ok, so now do the allocation once
            if ( ! alreadyarrays ) {
                alreadyarrays   = true;
                                        // did we collected enough infos?
                if ( numfieldsneeded < 4 )
                    return false;


                SamplingFrequency   = MicrosecondsToFrequency ( samplint );
                NumElectrodes       = numchannels;
                TotalElectrodes     = NumElectrodes + NumPseudoTracks;


                if      ( StringIs ( dataformat, BrainVisionBinaryString ) ) {

                    if      ( StringIs ( bintype, BrainVisionFloat32String ) ) {
                        DataType    = BVTypeFloat32;
                        BinTypeSize = sizeof ( float );
                        }
                    else if ( StringIs ( bintype, BrainVisionInt16String   ) ) {
                        DataType    = BVTypeInt16;
                        BinTypeSize = sizeof ( short );
                        }
                    else if ( StringIs ( bintype, BrainVisionUint16String  ) ) {
                        DataType    = BVTypeUint16;
                        BinTypeSize = sizeof ( ushort );
                        }
                    } // binary

                else if ( StringIs ( dataformat, BrainVisionAsciiString ) ) {
                    DataType        = BVTypeAscii;
                    BinTypeSize     = sizeof ( float );

                    AscSkipLines    = asclines;
                    AscSkipColumns  = asccols;
                    } // ASCII

                else {
                    DataType        = BVTypeNone;
                    BinTypeSize     = 0;
                    }


                if ( numtf )
                    NumTimeFrames       = numtf;
                else                    // f..king format that can ommit the number of TF!
                    NumTimeFrames       = GetFileSize ( (char *) GetDocPath () ) / ( BinTypeSize * NumElectrodes );



                Multiplexed         = StringIs ( dataorient, BrainVisionMultiplexedString );

                if ( IsFlag ( DataType, BVTypeBinMask ) ) {

                    if ( Multiplexed )

                        BuffSize    = (LONGLONG) BinTypeSize * NumElectrodes;
                    else
//                      BuffSize    = (LONGLONG) BinTypeSize * NumTimeFrames;
                                        // load all data in memory, for access efficiency
                        BuffSize    = (LONGLONG) BinTypeSize * NumTimeFrames * NumElectrodes;
                    }
                else // BVTypeAscMask - load all data in memory, as random access is not possible

                    BuffSize        = (LONGLONG) BinTypeSize * NumElectrodes * NumTimeFrames;


                Tracks.Resize ( BuffSize        );
                Gain  .Resize ( TotalElectrodes );
                Gain                = 1;    // set default value


                if ( ! SetArrays () )
                    return false;

                } // ! alreadyarrays

                                        // Ch<Channel number>=<Name>,<Reference channel name>,<Resolution in microvolts>,<Future extensions..>
                                        // Ch1=Fp1,,0.5,µV

                                        // clean-up
            StringNoSpace ( buff );

                                        // not a channel stuff?
            if ( ! StringStartsWith ( buff, "Ch" ) )
                continue;

                                        // get channel index
            if ( sscanf ( buff, "Ch%d=", &chindex ) == 0 )
                continue;

            chindex--;

            if ( chindex < 0 || chindex >= NumElectrodes )
                continue;


            bool            hascomma;
                                        // tokenize by commas, up to the 3d token (allows spaces in the names)
            for ( char *toc = StringContains ( buff, '=' ) + 1, toci = 1; toci <= 6; toci++, toc += StringSize ( toc ) ) {
                                        // cut the string to next comma, if any
                if ( StringContains ( toc, ',' ) ) {
                    *StringContains ( toc, ',' ) = 0;
                    hascomma    = true;
                    }
                else
                    hascomma    = false;


                if      ( toci == 1 ) {
                    StringCopy    ( ElectrodesNames[ chindex ], toc );
                                        // special case, coding for the comma
                    StringReplace ( ElectrodesNames[ chindex ], "\\1", "," );
                    }
                else if ( toci == 3 ) {
                    Gain[ chindex ] = StringIsEmpty ( toc ) ? 1 : StringToDouble ( toc );
                    //DBGV ( Gain[ chindex ], toc );
                    }

                                        // read channel name, stop if no comma!
                if ( ! hascomma )
                    break;
                }
            } // ChannelInfos


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        else if ( part == Coordinates ) {
            continue;                   // don't care right now
            } // Coordinates

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        } while ( ! ifhdr.eof () );


    ifhdr.close ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( numfieldsneeded < 4 )          // not enough mandatory fields
        return false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    // DateTime can be found through the trigger file


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // finish filling product info
    Subversion          = NumElectrodes;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // open EEG file
    InputStream.open ( GetDocPath (), ofRead | ( IsFlag ( DataType, BVTypeBinMask ) ? ofBinary : 0 ) );

    if ( InputStream.fail () )
        return false;

                                        // check for auxilliary
    InitAuxiliaries ();

                                        // swallow the whole bunch of data:
                                        // ASCII or not Multiplexed (which is quite inefficient to read)
    if      ( IsFlag ( DataType, BVTypeAscMask ) ) {

                                        // skip required lines (kind of header, I guess)
        for ( int skipl = 0; skipl < AscSkipLines; skipl++ )
            InputStream.ignore ( 1000000, '\n' );

                                        // use the same loops for both multiplexed and vectorized
        float*              toT         = (float *) Tracks.GetArray ();
        int                 numlines    = Multiplexed ? NumTimeFrames : NumElectrodes;
        int                 numcols     = Multiplexed ? NumElectrodes : NumTimeFrames;


        for ( int line = 0; line < numlines; line++ ) {

                                        // skip required columns
            for ( int skipc = 0; skipc < AscSkipColumns; skipc++ )

                GetToken ( &InputStream, buff );

                                        // now the real data
            for ( int col = 0; col < numcols; col++, toT++ ) {

                GetToken ( &InputStream, buff );

                if ( *ascdec != '.' )
                    StringReplace ( buff, ascdec, "." );

                *toT = StringToFloat ( buff );
                }
            }

                                        // shut down the stream, everything is in memory now
        InputStream.close ();

        } // BVTypeAscMask

    else if ( IsFlag ( DataType, BVTypeBinMask ) && ! Multiplexed ) {
                                        // load everything in our array, which is precisely demultiplexed
        InputStream.read ( Tracks.GetArray (), BuffSize );
                                        // shut down the stream, everything is in memory now
        InputStream.close ();

        } // BVTypeBinMask && Vectorized

    }
else {
    return false;
    }


return true;
}


//----------------------------------------------------------------------------
bool    TEegBrainVisionDoc::SetArrays ()
{
OffGfp          = NumElectrodes + PseudoTrackOffsetGfp;
OffDis          = NumElectrodes + PseudoTrackOffsetDis;
OffAvg          = NumElectrodes + PseudoTrackOffsetAvg;

                                        // do all allocations stuff
ElectrodesNames.Set ( TotalElectrodes, ElectrodeNameSize );

for ( int i = 1; i <= NumElectrodes; i++ )
    StringCopy  ( ElectrodesNames[ i - 1 ], "e", IntegerToString ( i ) );

StringCopy ( ElectrodesNames[ OffGfp ], TrackNameGFP );
StringCopy ( ElectrodesNames[ OffDis ], TrackNameDIS );
StringCopy ( ElectrodesNames[ OffAvg ], TrackNameAVG );


return true;
}


//----------------------------------------------------------------------------
void    TEegBrainVisionDoc::ReadRawTracks ( long tf1, long tf2, TArray2<float> &buff, int tfoffset )
{
if ( Multiplexed ) {
                                        // set file to first TF
    if ( InputStream.is_open () )
        InputStream.seekg ( BuffSize * tf1, ios::beg );


    if      ( DataType == BVTypeFloat32 ) {

        float*          toTf;

        for ( long tfi = tf1, tf = tfoffset; tfi <= tf2; tfi++, tf++ ) {

            InputStream.read  ( Tracks.GetArray (), BuffSize );
            toTf    = (float *) Tracks.GetArray ();

            for ( int el = 0; el < NumElectrodes; el++, toTf++ )
                buff ( el, tf ) = Gain[ el ] * *toTf;
            }
        }
    else if ( DataType == BVTypeInt16 ) {

        short*          toTs;

        for ( long tfi = tf1, tf = tfoffset; tfi <= tf2; tfi++, tf++ ) {

            InputStream.read  ( Tracks.GetArray (), BuffSize );
            toTs    = (short *) Tracks.GetArray ();

            for ( int el = 0; el < NumElectrodes; el++, toTs++ )
                buff ( el, tf ) = Gain[ el ] * *toTs;
            }
        }
    else if ( DataType == BVTypeUint16 ) {

        ushort*         toTus;

        for ( long tfi = tf1, tf = tfoffset; tfi <= tf2; tfi++, tf++ ) {

            InputStream.read   ( Tracks.GetArray (), BuffSize );
            toTus   = (ushort *) Tracks.GetArray ();

            for ( int el = 0; el < NumElectrodes; el++, toTus++ )
                buff ( el, tf ) = Gain[ el ] * *toTus;
            }
        }
    else if ( DataType == BVTypeAscii ) {

        float*          toTf = ((float *) Tracks.GetArray ()) + tf1 * NumElectrodes;

        for ( long tfi = tf1, tf = tfoffset; tfi <= tf2; tfi++, tf++ ) {
            for ( int el = 0; el < NumElectrodes; el++, toTf++ )
                buff ( el, tf ) = Gain[ el ] * *toTf;
            }
        }
    } // Multiplexed

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else { // ! Multiplexed

    if      ( DataType == BVTypeFloat32 ) {

        float*          toTf;

        for ( int el = 0; el < NumElectrodes; el++ ) {

            toTf    = ((float *) Tracks.GetArray ()) + el * NumTimeFrames + tf1;

            for ( long tfi = tf1, tf = tfoffset; tfi <= tf2; tfi++, tf++, toTf++ )
                buff ( el, tf ) = Gain[ el ] * *toTf;
            }
        }
    else if ( DataType == BVTypeInt16 ) {

        short*          toTs;

        for ( int el = 0; el < NumElectrodes; el++ ) {

            toTs    = ((short *) Tracks.GetArray ()) + el * NumTimeFrames + tf1;

            for ( long tfi = tf1, tf = tfoffset; tfi <= tf2; tfi++, tf++, toTs++ )
                buff ( el, tf ) = Gain[ el ] * *toTs;
            }
        }
    else if ( DataType == BVTypeUint16 ) {

        ushort*         toTus;

        for ( int el = 0; el < NumElectrodes; el++ ) {

            toTus   = ((ushort *) Tracks.GetArray ()) + el * NumTimeFrames + tf1;

            for ( long tfi = tf1, tf = tfoffset; tfi <= tf2; tfi++, tf++, toTus++ )
                buff ( el, tf ) = Gain[ el ] * *toTus;
            }

        }
    else if ( DataType == BVTypeAscii ) {

        float*          toTf;

        for ( int el = 0; el < NumElectrodes; el++ ) {

            toTf    = ((float *) Tracks.GetArray ()) + el * NumTimeFrames + tf1;

            for ( long tfi = tf1, tf = tfoffset; tfi <= tf2; tfi++, tf++, toTf++ )
                buff ( el, tf ) = Gain[ el ] * *toTf;
            }
        }
    } // ! Multiplexed
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
