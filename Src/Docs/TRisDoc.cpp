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

#include    "TRisDoc.h"

#include    "MemUtil.h"

#include    "TList.h"
#include    "Dialogs.TSuperGauge.h"

#include    "Files.WriteInverseMatrix.h"
#include    "TExportTracks.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TRisDoc::TRisDoc (TDocument *parent)
      : TTracksDoc (parent)
{
NumTimeFrames       = 0;
NumElectrodes       = 0;
TotalElectrodes     = 0;
SamplingFrequency   = 0;
Reference           = ReferenceAsInFile;

NumTracks           = 0;
TotalTracks         = 0;
}


//----------------------------------------------------------------------------
bool	TRisDoc::CommitRis ( bool force )
{
if ( ! ( IsDirty () || force ) )
    return true;

//DBGM3 ( GetDocPath (), GetAtomTypeName ( AtomTypeUseOriginal ), GetAtomTypeName ( AtomTypeUseCurrent ), "TRisDoc::Commit" );

TFileName           safepath ( GetDocPath (), TFilenameExtendedPath );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         gauge;

gauge.Set ( "Saving Tracks" );

gauge.AddPart ( 0,  NumTimeFrames,  100 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


/*                                        // Trick: we can convert the .ris to a .is, so it can be multiplied with another "eeg", or weights (see revision 4227)
if ( IsExtension ( safepath, FILEEXT_IS ) ) {

                                        // time = list of templates = "electrodes"
    TStrings            TFNames;
    char                buff[ 256 ];

    for ( int tf = 0; tf < NumTimeFrames; tf++ ) {
        StringCopy  ( buff, "Templ", IntegerToString ( tf + 1 ) );
        TFNames.Add ( buff );
        }

                                        // electrodes = "solution points"


                                        // ISBIN_MAGICNUMBER3 type
    WriteInverseMatrixHeader    (   NumTimeFrames,  NumElectrodes,                      0,      ! IsVector ( AtomTypeUseOriginal ),
                                    TFNames,        TStrings ( ElectrodesNames ),       0,      0, 
                                    safepath 
                                );

                                            // append matrix body, while converting to float type
    ofstream            ofs ( safepath, ios::binary | ios::app );

                                            // explicit loop, as we have pseudo tracks in the Tracks array that we don't want to save...
//    for ( int el=0; el < NumTracks;     el++ )
//    for ( int tf=0; tf < NumTimeFrames; tf++ )
//        ofs.write ( (char *) &Tracks ( el , tf ), Tracks.AtomSize () );


    if ( IsVector ( AtomTypeUseOriginal ) )
        for ( int el=0; el < NumElectrodes; el++ )
        for ( int tf=0; tf < NumTimeFrames; tf++ ) {    // !this part not tested!
            ofs.write ( (char *) &Tracks ( 3 * el     , tf ), Tracks.AtomSize () );
            ofs.write ( (char *) &Tracks ( 3 * el + 1 , tf ), Tracks.AtomSize () );
            ofs.write ( (char *) &Tracks ( 3 * el + 2 , tf ), Tracks.AtomSize () );
            }
    else
        for ( int el=0; el < NumElectrodes; el++ )      // tested
        for ( int tf=0; tf < NumTimeFrames; tf++ )
            ofs.write ( (char *) &Tracks ( el , tf ), Tracks.AtomSize () );


    ofs.close ();
    }

else { // as other tracks / ris, can be used to convert type on the fly
*/

    TExportTracks     expfile;

    StringCopy ( expfile.Filename, safepath );

    expfile.SetAtomType ( OriginalAtomType );
    expfile.NumTracks           = NumElectrodes;
    expfile.NumTime             = NumTimeFrames;
    expfile.SamplingFrequency   = SamplingFrequency;


    for ( int tf=0; tf < NumTimeFrames; tf++ ) {

        gauge.Next ( 0 );

        for ( int el=0; el < NumElectrodes; el++ )

            if ( IsVector ( AtomTypeUseOriginal ) )
                expfile.Write ( TVector3Float ( Tracks ( 3 * el     , tf ),
                                                Tracks ( 3 * el + 1 , tf ), 
                                                Tracks ( 3 * el + 2 , tf ) ) );
            else
                expfile.Write ( Tracks ( el , tf ) );
        } // for tf


SetDirty ( false );

return  true;
}


//----------------------------------------------------------------------------
bool	TRisDoc::CanClose ()
{                                       
SetDirty ( false );

return TTracksDoc::CanClose ();
}


bool	TRisDoc::Close ()
{
return  TFileDocument::Close ();
}


//----------------------------------------------------------------------------
bool	TRisDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
ifstream        ifs ( TFileName ( file, TFilenameExtendedPath ), ios::binary );
if ( ifs.fail() ) return false;


TRisHeader  risheader;
ifs.read ( (char *)(&risheader), sizeof ( risheader ) );


switch ( what ) {

    case ReadMagicNumber:
        *((int *)answer) = *((int *)risheader.Magic);
        return  IsMagicNumber ( risheader.Magic, RISBIN_MAGICNUMBER1 );

    case ReadNumSolPoints:
    case ReadNumElectrodes :
        *((int *)answer) = risheader.NumSolutionPoints;
        return   true;

    case ReadNumAuxElectrodes:
        *((int *)answer) = 0;
        return   true;

    case ReadSamplingFrequency:
        *((double *)answer) = risheader.SamplingFrequency;
        return  true;

    case ReadNumTimeFrames :
        *((int *)answer) = risheader.NumTimeFrames;
        return   true;

    case ReadInverseScalar :
        *((bool *)answer) = risheader.IsInverseScalar;
        return   true;

    }


return false;
}


//----------------------------------------------------------------------------
bool	TRisDoc::Open	( int, const char* path )
{
if ( path )
    SetDocPath ( path );

SetDirty ( false );


if ( GetDocPath () ) {

    TInStream*          is              = InStream ( ofRead | ofBinary );

    if ( !is )  return false;


    TRisHeader          risheader;

    is->read ( (char *) (&risheader), sizeof ( risheader ) );

    if ( ! IsMagicNumber ( risheader.Magic, RISBIN_MAGICNUMBER1 ) ) {

        ShowMessage ( "Can not recognize this file (unknown magic number)!", "Open file", ShowMessageWarning );
        delete is;
        return false;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    NumElectrodes       = risheader.NumSolutionPoints;
    NumTimeFrames       = risheader.NumTimeFrames;
    SamplingFrequency   = risheader.SamplingFrequency;
    SetAtomType ( risheader.IsInverseScalar ? UnknownAtomType : AtomTypeVector );   // in case of scalar, it could be positive or signed data (technically angular, too), so let the InitLimits finish the job
    

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fill product info
    StringCopy ( CompanyName, CartoolRegistryCompany );
    StringCopy ( ProductName, FILEEXT_RIS );
    Version             = 1;
    Subversion          = NumElectrodes;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // NumTracks is the actual number of lines in the buffer
    NumTracks           = ( IsVector ( AtomTypeUseOriginal ) ? 3 : 1 ) * NumElectrodes;


    if ( ! SetArrays () )
        return false;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read in the values
    if ( IsVector ( AtomTypeUseOriginal ) )

        for ( int tf=0; tf < NumTimeFrames; tf++ )
        for ( int el=0; el < NumElectrodes; el++ ) {

            is->read ( (char *) &Tracks ( 3 * el     , tf ), Tracks.AtomSize () );
            is->read ( (char *) &Tracks ( 3 * el + 1 , tf ), Tracks.AtomSize () );
            is->read ( (char *) &Tracks ( 3 * el + 2 , tf ), Tracks.AtomSize () );
            }
    else

        for ( int tf=0; tf < NumTimeFrames; tf++ )
        for ( int el=0; el < NumElectrodes; el++ )

            is->read ( (char *) &Tracks ( el , tf ), Tracks.AtomSize () );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check for auxiliaries
    InitAuxiliaries ();


    delete      is;
    }
else {                                  // can not create a RIS file here
    return false;
    }


return true;
}


//----------------------------------------------------------------------------
bool    TRisDoc::SetArrays ()
{
                                        // add the computed tracks to list of tracks
TotalElectrodes = NumElectrodes + NumPseudoTracks;
TotalTracks     = NumTracks     + NumPseudoTracks;

//OffGfp          = NumTracks     + PseudoTrackOffsetGfp;
//OffDis          = NumTracks     + PseudoTrackOffsetDis;
//OffAvg          = NumTracks     + PseudoTrackOffsetAvg;
                                        // ! use these indexes, which are wrong according to our own buffer
                                        // in vectorial case, but appears right to external calls, like f.ex. GetTracks
OffGfp          = NumElectrodes + PseudoTrackOffsetGfp;
OffDis          = NumElectrodes + PseudoTrackOffsetDis;
OffAvg          = NumElectrodes + PseudoTrackOffsetAvg;

                                        // do all allocations stuff
Tracks.Resize ( TotalTracks, NumTimeFrames );


ElectrodesNames.Set ( TotalElectrodes, ElectrodeNameSize );

for ( int i = 1; i <= NumElectrodes; i++ )
    StringCopy  ( ElectrodesNames[ i - 1 ], "sp", IntegerToString ( i ) );
                                        // always this place
StringCopy ( ElectrodesNames[ NumElectrodes + PseudoTrackOffsetGfp ], TrackNameGFP );
StringCopy ( ElectrodesNames[ NumElectrodes + PseudoTrackOffsetDis ], TrackNameDIS );
StringCopy ( ElectrodesNames[ NumElectrodes + PseudoTrackOffsetAvg ], TrackNameAVG );


BadTracks   = TSelection ( TotalElectrodes, OrderSorted );
BadTracks.Reset();
AuxTracks   = TSelection ( TotalElectrodes, OrderSorted );
AuxTracks.Reset();


return true;
}


//----------------------------------------------------------------------------
void    TRisDoc::ReadRawTracks ( long tf1, long tf2, TArray2<float> &buff, int tfoffset )
{
if ( IsVector ( AtomTypeUseOriginal ) ) {

    float*              toTx;
    float*              toTy;
    float*              toTz;
    float*              b;

    for ( int t = 0, t3 = 0; t < NumElectrodes; t++ ) {

        toTx    = Tracks[ t3++ ] + tf1;
        toTy    = Tracks[ t3++ ] + tf1;
        toTz    = Tracks[ t3++ ] + tf1;
        b       = buff  [ t    ] + tfoffset;

        for ( long tf = tf1; tf <= tf2; tf++, toTx++, toTy++, toTz++, b++ )

            *b  = NormVector3 ( *toTx, *toTy, *toTz );
        }
    }
else { // scalar or positive

    size_t              numtf           = tf2 - tf1 + 1;

    for ( int t = 0; t < NumElectrodes; t++ )

        CopyVirtualMemory ( buff[ t ] + tfoffset, Tracks[ t ] + tf1, numtf * Tracks.AtomSize () );
    }
}


//----------------------------------------------------------------------------
// AveragingBefore  is of no use here, as we don't have access to the eeg anymore

void    TRisDoc::GetInvSol ( int /*reg*/, long tf1, long tf2, TArray1< float > &inv, TTracksView* /*eegview*/, TRois *rois )    const
{
int                 numtf           = tf2 - tf1 + 1;

//Clipped ( tf1, tf2, (long) 0, (long) NumTimeFrames - 1 );


if ( IsVector ( AtomTypeUseOriginal ) ) {

    const float*        toTx;
    const float*        toTy;
    const float*        toTz;
    double              sum;

    for ( int sp = 0, sp3 = 0; sp < NumElectrodes; sp++ ) {

        sum     = 0;
        toTx    = Tracks[ sp3++ ] + tf1;
        toTy    = Tracks[ sp3++ ] + tf1;
        toTz    = Tracks[ sp3++ ] + tf1;

        for ( long tf = tf1; tf <= tf2; tf++, toTx++, toTy++, toTz++ )

            sum += NormVector3 ( *toTx, *toTy, *toTz );

        inv[ sp ]   = sum / numtf ;
        }
    }
else { // scalar or positive

    const float*        toT;
    double              sum;

    for ( int sp = 0; sp < NumElectrodes; sp++ ) {

        sum     = 0;
        toT     = Tracks[ sp ] + tf1;

        for ( long tf = tf1; tf <= tf2; tf++, toT++ )

            sum += *toT;

        inv[ sp ]   = sum / numtf ;
        }
    }

                                        // optional ROIing
if ( rois )
    rois->Average ( inv, FilterTypeMean );
}


//----------------------------------------------------------------------------
void    TRisDoc::GetInvSol ( int /*reg*/, long tf1, long tf2, TArray1< TVector3Float > &inv, TTracksView* /*eegview*/, TRois *rois )    const
{
int                 numtf           = tf2 - tf1 + 1;

//Clipped ( tf1, tf2, (long) 0, (long) NumTimeFrames - 1 );


if ( IsVector ( AtomTypeUseOriginal ) ) {

    const float*        toTx;
    const float*        toTy;
    const float*        toTz;
    double              sumx;
    double              sumy;
    double              sumz;

    for ( int sp = 0, sp3 = 0; sp < NumElectrodes; sp++ ) {

        sumx    = sumy  = sumz  = 0;
        toTx    = Tracks[ sp3++ ] + tf1;
        toTy    = Tracks[ sp3++ ] + tf1;
        toTz    = Tracks[ sp3++ ] + tf1;

        for ( long tf = tf1; tf <= tf2; tf++, toTx++, toTy++, toTz++ ) {
                                        // !there should be some polarity option & testing here!
            sumx    += *toTx;
            sumy    += *toTy;
            sumz    += *toTz;
            }

        inv[ sp ].X     = sumx / numtf ;
        inv[ sp ].Y     = sumy / numtf ;
        inv[ sp ].Z     = sumz / numtf ;
        }
    }

else { // scalar or positive

    const float*        toT;
    double              sum;

    for ( int sp = 0; sp < NumElectrodes; sp++ ) {

        sum     = 0;
        toT     = Tracks[ sp ] + tf1;

        for ( long tf = tf1; tf <= tf2; tf++, toT++ )

            sum += *toT;

        inv[ sp ].X     = sum / numtf;
        inv[ sp ].Y     = 0;
        inv[ sp ].Z     = 0;
        }
    }

                                        // optional ROIing
if ( rois )
    rois->Average ( inv, FilterTypeMean );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
