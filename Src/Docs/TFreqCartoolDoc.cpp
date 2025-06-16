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

#include    "TFreqCartoolDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TFreqCartoolDoc::TFreqCartoolDoc ( TDocument *parent )
      : TFreqDoc ( parent )
{
AtomSize            = 0;
IsAtomScalar        = true;

Version             = 0;
}


bool	TFreqCartoolDoc::Close ()
{
return  TFileDocument::Close ();
}


bool	TFreqCartoolDoc::CanClose ()
{                                       // can not save this file
SetDirty ( false );

return  TTracksDoc::CanClose ();
}


bool	TFreqCartoolDoc::Commit	( bool force )
{
if ( ! ( IsDirty () || force ) )
    return true;

SetDirty ( false );

return  true;
}


//----------------------------------------------------------------------------
bool    TFreqCartoolDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
ifstream        ifs ( TFileName ( file, TFilenameExtendedPath ), ios::binary );
if ( ifs.fail() ) return false;


TFreqHeader     freqheader;

ifs.read ( (char *) &freqheader,  sizeof (freqheader) );

switch ( what ) {

    case ReadNumElectrodes :
        *((int *) answer)   = freqheader.NumChannels;
        return  true ;

    case ReadNumAuxElectrodes :
        *((int *) answer)   = 0;
        return  true ;

    case ReadNumTimeFrames :
        *((int *) answer)   = freqheader.NumBlocks;
        return  true ;

    case ReadNumFrequencies :
        *((int *) answer)   = freqheader.NumFrequencies;
        return  true;

    case ReadSamplingFrequency :
        *((double *) answer)= freqheader.BlockFrequency;
        return  true;

    case ReadOriginalSamplingFrequency :
        *((double *) answer)= freqheader.SamplingFrequency;
        return  true;

    case ReadFrequencyType :
        *((int *) answer)   = StringToFreqType ( freqheader.Type );
        return  true ;
    }


return false;
}


//----------------------------------------------------------------------------
bool	TFreqCartoolDoc::Open	( int /*mode*/, const char *path )
{
if ( path )
    SetDocPath ( path );

SetDirty ( false );


if ( GetDocPath () ) {

    FileStream.Open ( GetDocPath(), FileStreamRead );


    TFreqHeader     freqheader;

    FileStream.Read ( &freqheader, sizeof ( freqheader ) );

    if ( ! IsMagicNumber ( freqheader.Version, FREQBIN_MAGICNUMBER1 )       // obsolete format
      && ! IsMagicNumber ( freqheader.Version, FREQBIN_MAGICNUMBER2 ) ) {

        ShowMessage ( "Can not recognize this file (unknown magic number)!", "Open file", ShowMessageWarning );

        FileStream.Close ();

        return false;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Version     = freqheader.Version;

                                        // try to detect frequency analysis type
    FreqType    = StringToFreqType ( freqheader.Type );
                                        // which we can use to determine the atom type
    SetAtomType     ( IsFreqTypeComplex () ? AtomTypeComplex : UnknownAtomType ); // AtomTypeScalar;  // let InitLimits guess the content


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fill product info
    StringCopy ( CompanyName, CartoolRegistryCompany );
    StringCopy ( ProductName, FILEEXT_FREQ );
    Version             = freqheader.Version;
    Subversion          = freqheader.NumChannels;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    NumElectrodes       = freqheader.NumChannels;
    TotalElectrodes     = NumElectrodes + NumPseudoTracks;
    SamplingFrequency   = freqheader.BlockFrequency;
    NumTimeFrames       = freqheader.NumBlocks;
    NumFrequencies      = freqheader.NumFrequencies;
    OriginalSamplingFrequency   = freqheader.SamplingFrequency;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    IsAtomScalar        = ! IsComplex ( AtomTypeUseOriginal );
    AtomSize            = IsAtomScalar ? 1 : 2;


    DateTime            = TDateTime ( freqheader.Year, freqheader.Month,  freqheader.Day,
                                      freqheader.Hour, freqheader.Minute, freqheader.Second, freqheader.Millisecond, 0 );

    DataOrg             = sizeof ( TFreqHeader )
                        + NumElectrodes * sizeof ( TFreqChannel )
                        + NumFrequencies * ( IsMagicNumber ( freqheader.Version, FREQBIN_MAGICNUMBER1 ) ? sizeof ( TFreqFrequency ) : sizeof ( TFreqFrequencyName ) );

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // space allocation + default names
    if ( ! SetArrays () ) {

        FileStream.Close ();

        return false;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read electrode names
    char            buff[ 256 ];

    for ( int el = 0; el < NumElectrodes; el++ ) {

        FileStream.Read ( buff, sizeof ( TFreqChannel ) );

        buff[ sizeof ( TFreqChannel ) ] = EOS;  // force End Of String, i.e. 0

        StringCopy ( ElectrodesNames[ el ], buff, ElectrodeNameSize - 1 );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read frequencies & create strings
    if ( IsMagicNumber ( freqheader.Version, FREQBIN_MAGICNUMBER1 ) ) {

        TFreqFrequency      freq;

        for ( int f=0; f < NumFrequencies; f++ ) {

            FileStream.Read ( &freq, sizeof ( freq ) );

            sprintf ( buff, "%0.2lg Hz", freq.Frequency );

            StringCopy ( FrequenciesNames[ f ], buff );
            }
        }
    else { // FREQBIN_MAGICNUMBER2

        TFreqFrequencyName  freq;

        for ( int f=0; f < NumFrequencies; f++ ) {

            FileStream.Read ( &freq, sizeof ( freq ) );

            StringCopy ( FrequenciesNames[ f ], freq.FrequencyName );
            }
        }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Read all data at once, and reorganize it to improve efficiency
                                        // File organization is:    time        x electrodes              x frequencies x complex/real
                                        // Tracks is:               frequencies x ( 1 or 2 * electrodes ) x time
                                        // Note that real values are stored as multiplexed electrodes
    FileStream.SeekBegin ( DataOrg );

    TArray3<float>      onetf ( NumElectrodes, NumFrequencies, AtomSize );

    for ( int tf = 0; tf < NumTimeFrames;  tf++ ) {

        FileStream.Read ( onetf.GetArray (), onetf.MemorySize () );

        if ( IsAtomScalar ) {
            OmpParallelFor

            for ( int e = 0; e < NumElectrodes;  e++ )
            for ( int f = 0; f < NumFrequencies; f++ )

                Tracks ( f, e,         tf ) = onetf ( e, f, 0 );
            }
        else {
            OmpParallelFor

            for ( int e = 0; e < NumElectrodes;  e++ )
            for ( int f = 0; f < NumFrequencies; f++ ) {

                Tracks ( f, 2 * e,     tf ) = onetf ( e, f, 0 );
                Tracks ( f, 2 * e + 1, tf ) = onetf ( e, f, 1 );
                }
            }

        } // read tf


    FileStream.Close ();
    }

else {
    return false;
    }

return true;
}


//----------------------------------------------------------------------------
bool    TFreqCartoolDoc::SetArrays ()
{
OffGfp              = NumElectrodes + PseudoTrackOffsetGfp;
OffDis              = NumElectrodes + PseudoTrackOffsetDis;
OffAvg              = NumElectrodes + PseudoTrackOffsetAvg;

                                        // do all allocations stuff
Tracks.Resize ( NumFrequencies, AtomSize * NumElectrodes, NumTimeFrames );  // !Multiplexing real and imaginary parts as successive electrodes!


FrequenciesNames.Set ( NumFrequencies,  MaxCharFrequencyName );


ElectrodesNames .Set ( TotalElectrodes, ElectrodeNameSize );

StringCopy ( ElectrodesNames[ OffGfp ], TrackNameGFP );
StringCopy ( ElectrodesNames[ OffDis ], TrackNameDIS );
StringCopy ( ElectrodesNames[ OffAvg ], TrackNameAVG );


BadTracks   = TSelection ( TotalElectrodes, OrderSorted );
BadTracks.Reset();
AuxTracks   = TSelection ( TotalElectrodes, OrderSorted );
AuxTracks.Reset();


return true;
}


//----------------------------------------------------------------------------
                                        // read all electrodes of current selected frequency only
void    TFreqCartoolDoc::ReadRawTracks ( long tf1, long tf2, TArray2<float> &buff, int tfoffset )
{
for ( int  el  = 0;                  el  <  NumElectrodes; el++        )
for ( long tfi = tf1, tf = tfoffset; tfi <= tf2;           tfi++, tf++ )

    if ( IsAtomScalar )     buff ( el, tf )    =                 Tracks ( CurrentFrequency, el,         tfi );
                                        // compute norm on the fly
    else                    buff ( el, tf )    = sqrt ( Square ( Tracks ( CurrentFrequency, 2 * el,     tfi ) ) 
                                                      + Square ( Tracks ( CurrentFrequency, 2 * el + 1, tfi ) ) );
}


//----------------------------------------------------------------------------
void    TFreqCartoolDoc::ReadFrequencies ( long tf1, long tf2, TSetArray2<float> &buff, int tfoffset )
{
for ( int  f   = 0;                  f   <  NumFrequencies; f++         )
for ( int  el  = 0;                  el  <  NumElectrodes;  el++        )
for ( long tfi = tf1, tf = tfoffset; tfi <= tf2;            tfi++, tf++ )

    if ( IsAtomScalar )     buff ( f, el, tf )  =                 Tracks ( f, el,         tfi );
                                        // compute norm on the fly
    else                    buff ( f, el, tf )  = sqrt ( Square ( Tracks ( f, 2 * el,     tfi ) ) 
                                                       + Square ( Tracks ( f, 2 * el + 1, tfi ) ) );
}


//----------------------------------------------------------------------------
void    TFreqCartoolDoc::ReadFrequencies ( long tf1, long tf2, TSetArray2<float> &realpart, TSetArray2<float> &imagpart, int tfoffset )
{
if ( IsAtomScalar ) {

    imagpart.ResetMemory ();
    ReadFrequencies ( tf1, tf2, realpart, tfoffset );
    return;
    }

//else if ( IsAtomComplex )

for ( int  f   = 0;                  f   <  NumFrequencies; f++         )
for ( int  el  = 0;                  el  <  NumElectrodes;  el++        )
for ( long tfi = tf1, tf = tfoffset; tfi <= tf2;            tfi++, tf++ ) {

    realpart ( f, el, tf )  = Tracks ( f, 2 * el,     tfi );
    imagpart ( f, el, tf )  = Tracks ( f, 2 * el + 1, tfi );
    }
}


//----------------------------------------------------------------------------
double  TFreqCartoolDoc::GetFreqValue ( long el, long tf, long f )
{
return  IsAtomScalar ?                 Tracks ( f, el,     tf ) 
                     : sqrt ( Square ( Tracks ( f, 2 * el, tf ) ) + Square ( Tracks ( f, 2 * el + 1, tf ) ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
