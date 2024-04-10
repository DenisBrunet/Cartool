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

#include    "TFreqCartoolDoc.h"

#include    "TExportTracks.h"

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
InputStream         = 0;
SpectrumSize        = 0;
AtomSize            = 0;

Version             = 0;
DataOrg             = 0;
}


bool	TFreqCartoolDoc::Close ()
{
if ( InputStream != 0 )
    delete  InputStream, InputStream = 0;

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

    InputStream     = InStream ( ofRead | ofBinary );

    if ( ! InputStream )
        return  false;


    TFreqHeader     freqheader;

    InputStream->read ( (char *) (&freqheader), sizeof ( freqheader ) );

    if ( ! IsMagicNumber ( freqheader.Version, FREQBIN_MAGICNUMBER1 )       // obsolete format
      && ! IsMagicNumber ( freqheader.Version, FREQBIN_MAGICNUMBER2 ) ) {

        ShowMessage ( "Can not recognize this file (unknown magic number)!", "Open file", ShowMessageWarning );
        delete  InputStream; InputStream = 0;
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
                                        // Spectrum to hold the whole data remaining for 1 tracks, 1 TF
                                        // + dealing with the complex case, which can double the size
    AtomSize            = ( IsComplex ( AtomTypeUseOriginal ) ? 2 : 1 ) * sizeof ( float );
    SpectrumSize        = NumFrequencies * AtomSize;


    DateTime            = TDateTime ( freqheader.Year, freqheader.Month,  freqheader.Day,
                                      freqheader.Hour, freqheader.Minute, freqheader.Second, freqheader.Millisecond, 0 );

    DataOrg             = sizeof ( TFreqHeader )
                        + NumElectrodes * sizeof ( TFreqChannel )
                        + NumFrequencies * ( IsMagicNumber ( freqheader.Version, FREQBIN_MAGICNUMBER1 ) ? sizeof ( TFreqFrequency ) : sizeof ( TFreqFrequencyName ) );

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // space allocation + default names
    if ( ! SetArrays () ) {
        delete  InputStream; InputStream = 0;
        return false;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read electrode names
    char            buff[ 256 ];

    for ( int el = 0; el < NumElectrodes; el++ ) {

        InputStream->read ( buff, sizeof ( TFreqChannel ) );

        buff[ sizeof ( TFreqChannel ) ] = 0; // force EOS

        StringCopy ( ElectrodesNames[ el ], buff, ElectrodeNameSize - 1 );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read frequencies & create strings
    if ( IsMagicNumber ( freqheader.Version, FREQBIN_MAGICNUMBER1 ) ) {

        TFreqFrequency      freq;

        for ( int f=0; f < NumFrequencies; f++ ) {
            InputStream->read ( (char *) &freq, sizeof ( freq ) );

            sprintf ( buff, "%0.2lg Hz", freq.Frequency );

            StringCopy ( FrequenciesNames[ f ], buff );
            }
        }
    else { // FREQBIN_MAGICNUMBER2
        TFreqFrequencyName  freq;

        for ( int f=0; f < NumFrequencies; f++ ) {
            InputStream->read ( (char *) &freq, sizeof ( freq ) );

            StringCopy ( FrequenciesNames[ f ], freq.FrequencyName );
            }
        }

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
Spectrum.Resize ( SpectrumSize / sizeof ( float ) );


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
                                        // set file to first TF
InputStream->seekg ( DataOrg + NumElectrodes * SpectrumSize * tf1, ios::beg );


for ( long tfi = tf1, tf = tfoffset; tfi <= tf2;           tfi++, tf++ )
for ( int el   = 0;                  el  <  NumElectrodes; el++        ) {

    InputStream->read ( (char *) Spectrum.GetArray (), SpectrumSize );

    if ( ! IsComplex ( AtomTypeUseOriginal ) )
        buff ( el, tf )    = Spectrum[ CurrentFrequency ];
    else
        buff ( el, tf )    = sqrt ( Square ( Spectrum[ 2 * CurrentFrequency ] ) + Square ( Spectrum[ 2 * CurrentFrequency + 1 ] ) );
    }
}


//----------------------------------------------------------------------------
void    TFreqCartoolDoc::ReadFrequencies ( long tf1, long tf2, TSetArray2<float> &buff, int tfoffset )
{
                                        // set file to first TF
InputStream->seekg ( DataOrg + NumElectrodes * SpectrumSize * tf1, ios::beg );


for ( long tfi = tf1, tf = tfoffset; tfi <= tf2;           tfi++, tf++ )
for ( int el   = 0;                  el  <  NumElectrodes; el++        ) {

    InputStream->read ( (char *) Spectrum.GetArray (), SpectrumSize );


    float*              tof             = &buff ( 0, el, tf );


    if ( ! IsComplex ( AtomTypeUseOriginal ) )
                                                // !not allowed to move the pointer futher!
        for ( int f = 0; f < NumFrequencies; f++, tof += f < NumFrequencies ? buff.GetLinearDim () : 0 )

            *tof = Spectrum[ f ];

    else
                                                // !not allowed to move the pointer futher!
        for ( int f = 0, f2 = 0; f < NumFrequencies; f++, tof += f < NumFrequencies ? buff.GetLinearDim () : 0 )
                                        // convert real and imaginary to norm on the fly - Spectrum has the right size to hold the real & imaginary parts
            *tof = sqrt ( Square ( Spectrum[ f2++ ] ) + Square ( Spectrum[ f2++ ] ) );
    }
}


//----------------------------------------------------------------------------
void    TFreqCartoolDoc::ReadFrequencies ( long tf1, long tf2, TSetArray2<float> &realpart, TSetArray2<float> &imagpart, int tfoffset )
{
if ( ! IsComplex ( AtomTypeUseOriginal ) ) {
    imagpart.ResetMemory ();
    ReadFrequencies ( tf1, tf2, realpart, tfoffset );
    return;
    }


                                        // set file to first TF
InputStream->seekg ( DataOrg + NumElectrodes * SpectrumSize * tf1, ios::beg );


for ( long tfi = tf1, tf = tfoffset; tfi <= tf2;           tfi++, tf++ )
for ( int el   = 0;                  el  <  NumElectrodes; el++        ) {

    InputStream->read ( (char *) Spectrum.GetArray (), SpectrumSize );


    float*              tor             = &realpart ( 0, el, tf );
    float*              toi             = &imagpart ( 0, el, tf );

                                            // !not allowed to move the pointer futher!
    for ( int f = 0, f2 = 0; f < NumFrequencies; f++, tor += f < NumFrequencies ? realpart.GetLinearDim () : 0, toi += f < NumFrequencies ? imagpart.GetLinearDim () : 0 ) {
                                        // real and imaginary are interleaved (follow each others)
        *tor = Spectrum[ f2++ ];                     
        *toi = Spectrum[ f2++ ];                     
        }
    }
}


//----------------------------------------------------------------------------
double  TFreqCartoolDoc::GetFreqValue ( long el, long tf, long f )
{
InputStream->seekg ( DataOrg + ( ( NumElectrodes * tf + el ) * NumFrequencies + f ) * AtomSize, ios::beg );

if ( ! IsComplex ( AtomTypeUseOriginal ) ) {
    float               v;

    InputStream->read ( (char *) &v, sizeof ( float ) );

    return  v;
    }
else {
    float               v[ 2 ];

    InputStream->read ( (char *) &v, 2 * sizeof ( float ) );

    return  sqrt ( Square ( v[ 0 ] ) + Square ( v[ 1 ] ) );
    }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
