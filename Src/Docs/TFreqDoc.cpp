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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "TFreqDoc.h"

#include    "Math.Resampling.h"
#include    "TVector.h"
#include    "TArray2.h"
#include    "TArray3.h"
#include    "TSetArray2.h"

#include    "TRois.h"
#include    "TExportTracks.h"

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const char  FrequencyAnalysisNames[ NumFrequencyAnalysisTypes ][ MaxCharFreqType ] = {
                    "Unknown",

                    "FFT Norm",
                    "FFT Norm2",
                    "FFT Complex",
                    "FFT Phase",

                    "FFT Approximation",

                    "S Transform",
                    "S Transform Norm",
                    "S Transform Norm2",
                    "S Transform Complex",
                    "S Transform Phase",

                    "Butterworth Bank",

                    "Frequency RIS",
                    };


//----------------------------------------------------------------------------
                                        // reverse from a string to a FrequencyAnalysisType, as .freq files store only a string, not the enum itself
FrequencyAnalysisType    StringToFreqType    ( const char* freqtypestring ) 
{
if ( StringIsEmpty ( freqtypestring ) )
    return  FreqUnknown;

for ( int freqi = FreqUnknown; freqi < NumFrequencyAnalysisTypes; freqi++ )
    if ( StringIs ( FrequencyAnalysisNames[ freqi ], freqtypestring ) )
        return  (FrequencyAnalysisType) freqi;

return  FreqUnknown;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TFreqDoc::TFreqDoc ( TDocument *parent )
      : TTracksDoc ( parent )
{
ContentType                 = ContentTypeFreq;

NumFrequencies              = 0;
OriginalSamplingFrequency   = 0;
FreqType                    = FreqUnknown;
Reference                   = ReferenceAsInFile;
CurrentFrequency            = 0;
}


bool	TFreqDoc::InitDoc ()
{
if ( ! IsOpen () )
    return  TTracksDoc::InitDoc ();

return  true;
}


//----------------------------------------------------------------------------
                                        // do the job in different frequencies
void    TFreqDoc::InitLimits ( InitType how )
{
ResetLimits ();


int                 bestfreq            = CurrentFrequency;
double              currMinValue        = 0;
double              currMaxValue        = 0;
double              currAbsMaxValue     = 0;
long                currAbsMaxTF        = 0;
double              currMaxGfp          = 0;
long                currMaxGfpTF        = 0;
TDownsampling       downfreq ( 0, NumFrequencies / 2, 17 ); // focusing on the lower frequencies, which have the highest power
//bool              alwayspos           = true;


if ( NumTimeFrames > EegMaxPointsDisplay )
    how     = InitFast;


for ( CurrentFrequency = downfreq.From; CurrentFrequency <= downfreq.To; CurrentFrequency += downfreq.Step ) {

    TTracksDoc::InitLimits ( how );

//  alwayspos   = alwayspos && IsPositive ( AtomTypeUseOriginal );

    Mined ( currMinValue, MinValue );
    Maxed ( currMaxValue, MaxValue );

    if ( AbsMaxValue > currAbsMaxValue ) {
        currAbsMaxValue = AbsMaxValue;
        currAbsMaxTF    = AbsMaxTF;
        bestfreq        = CurrentFrequency;
        }

    if ( MaxGfp > currMaxGfp ) {
        currMaxGfp      = MaxGfp;
        currMaxGfpTF    = MaxGfpTF;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // keep max of the max
MinValue        = currMinValue;
MaxValue        = currMaxValue;
AbsMaxValue     = currAbsMaxValue;
AbsMaxTF        = currAbsMaxTF;
MaxGfp          = currMaxGfp;
MaxGfpTF        = currMaxGfpTF;

                                        // restore CurrentFrequency
CurrentFrequency    = bestfreq;


//if ( IsUnknownType ( AtomTypeUseOriginal ) )
//    SetAtomType ( alwayspos ? AtomTypePositive : AtomTypeScalar );
}


//----------------------------------------------------------------------------
void    TFreqDoc::SetCurrentFrequency ( int f, bool notifyviews )
{
CurrentFrequency    = IsInsideLimits ( f, 0, NumFrequencies - 1 ) ? f : 0;

if ( notifyviews )
    NotifyDocViews ( vnReloadData, EV_VN_RELOADDATA_FREQ );
}


//----------------------------------------------------------------------------
                                        // A simplified version of TTracksDoc::GetTracks
                                        // It returns tracks from the CURRENT selected frequency
void    TFreqDoc::GetTracks (   long                tf1,            long                tf2,
                                TArray2<float>&     buff,           int                 tfoffset, 
                                AtomType            atomtype,
                                PseudoTracksType    pseudotracks,
                                ReferenceType     /*reference*/,    const TSelection* /*referencetracks*/,
                                const TRois*        rois      
                            )
{
                                        // Checking parameters consistency

                                        // should resolve to a meaningful type
atomtype    = GetAtomType ( atomtype );


if ( pseudotracks == ComputePseudoTracks && ! ( buff.GetDim1 () >= NumElectrodes + NumPseudoTracks ) )

    pseudotracks    = NoPseudoTracks;

                                        // When data is positive, compute the RMS instead of the GFP by skipping the re-centering
bool                gfpnotcentered      = IsAbsolute ( atomtype );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numtf           = tf2 - tf1 + 1;

                                        // Adjust buffer to either original or expanded limits
                                        // Note that the previous content could be lost, though this usually is not a problem, as for filtered data, the buffer is usually evaluated as a whole
                                        // If this is still a problem, make sure the buffer is allocated large enough beforehand
buff.Resize (   AtLeast ( buff.GetDim1 (), NumElectrodes + ( pseudotracks ? NumPseudoTracks : 0 ) ),
                AtLeast ( buff.GetDim2 (), (int) ( tfoffset + numtf )                             )  );


                                        // BuffDiss dimensions could also be asserted here...

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We have to care for an optional 1 TF buffer used for dissimilarity computation
if ( pseudotracks ) {
    
    if      ( tf1 == 0 )                // original or modified tf1 - Dissimilarity can not be computed 

        BuffDiss.ResetMemory ();        // by safety

    else if ( tf1 > 0 )                 // read an additional time point @ tf1-1 as there is addition margin from any filter here

        ReadRawTracks ( tf1 - 1, tf1 - 1, BuffDiss );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read raw Eeg tracks from file
ReadRawTracks ( tf1, tf2, buff, tfoffset );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // copied from TTracksDoc::GetTracks
if ( pseudotracks ) {
                                        // count only non-aux and non-bad electrodes
    double              numt            = NonNull ( GetNumValidElectrodes () );
    double              rescalevector   = IsVector ( atomtype ) ? 3 : 1;
    double              avgbefore       = 0;
    double              gfpbefore       = 1;


    auto    ComputeAvg  = [ this, &numt ] ( TArray2<float>& buff, int tfo ) {

        double      sum     = 0;
                                        // don't use ReferenceTracks, as we can be in an overriding case
        for ( int el = 0; el < NumElectrodes; el++ ) 
            if ( ValidTracks[ el ] )
                sum    += buff ( el, tfo );
                
        return  sum / numt;
        };

    auto    ComputeGfp  = [ this, &numt, &rescalevector ] ( TArray2<float>& buff, int tfo, double avg ) {

        double      sumsqr  = 0;

        for ( int el = 0; el < NumElectrodes; el++ ) 
            if ( ValidTracks[ el ] )
                sumsqr += Square ( buff ( el, tfo ) - avg );
            
        return  sqrt ( sumsqr / numt * rescalevector );
        };

    auto    ComputeDis  = [ this, &numt, &rescalevector ] ( TArray2<float>& buff1, int tfo1, double avg1, double gfp1, TArray2<float>& buff2, int tfo2, double avg2, double gfp2 ) {

        double      sumsqr  = 0;

        for ( int el = 0; el < NumElectrodes; el++ ) 
            if ( ValidTracks[ el ] )
                sumsqr += Square (   ( buff1 ( el, tfo1 ) - avg1 ) / gfp1
                                   - ( buff2 ( el, tfo2 ) - avg2 ) / gfp2 );
            
        return  sqrt ( sumsqr / numt * rescalevector );
        };


    for ( int tf = 0, tfo = tfoffset; tf < numtf; tf++, tfo++ ) {

        double          avg     = ComputeAvg ( buff, tfo );

        buff ( OffAvg, tfo )    = avg;  // saving real average


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( gfpnotcentered )
            avg     = 0;                // resetting avg? GFP becomes RMS

        double          gfp     = ComputeGfp ( buff, tfo, avg );

        buff ( OffGfp, tfo )    = gfp;  // saving real GFP / RMS


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        NonNulled ( gfp );              // just to prevent division by zero


        if ( tf == 0 ) {                // first occurence in the loop, which means we have to handle some missing data

            if ( tf1 == 0 ) {           // first actual tf: dissimilarity can not be computed, as there is no previous data to compare from

                buff ( OffDis, tfo )   = 0;
                }
            else {                      // tf1 > 0 - previous data exist and is to be found in our local 1 TF buffer

                double      avgbuffdiss     = gfpnotcentered ? 0 : ComputeAvg ( BuffDiss, 0 );

                double      gfpbuffdiss     = NonNull ( ComputeGfp ( BuffDiss, 0, avgbuffdiss ) );

                buff ( OffDis, tfo )        = ComputeDis    (   buff,       tfo,    avg,            gfp,
                                                                BuffDiss,   0,      avgbuffdiss,    gfpbuffdiss );
                } // else tf1 > 0

            } // tf == 0

        else {                          // tf > 0 - NOT the first occurence in the loop, and avgbefore and gfpbefore exist

            buff ( OffDis, tfo )    = ComputeDis    (   buff,   tfo,        avg,        gfp,
                                                        buff,   tfo - 1,    avgbefore,  gfpbefore );
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // store current values for the next tf
        avgbefore   = avg;
        gfpbefore   = gfp;
        } // for tf

    } // if pseudotracks


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // optional ROIing
if ( rois )
    rois->Average ( buff, tfoffset, tfoffset + numtf - 1, FilterTypeMean );
}


//----------------------------------------------------------------------------
                                        // Returning ALL TRACKS, ALL FREQUENCIES, for the SELECTED TIME RANGE
void    TFreqDoc::GetFrequencies    (   long                tf1,            long            tf2,
                                        TSetArray2<float>&  buff,           int             tfoffset, 
                                        AtomType            atomtype,
                                        PseudoTracksType    pseudotracks,
                                        ReferenceType       /*reference*/,  TSelection*     /*referencetracks*/,
                                        TRois*              rois      
                                    )

{
                                        // Checking parameters consistency

                                        // should resolve to a meaningful type
atomtype    = GetAtomType ( atomtype );


if ( pseudotracks == ComputePseudoTracks && ! ( buff.GetDim1 () >= NumElectrodes + NumPseudoTracks ) )

    pseudotracks    = NoPseudoTracks;

                                        // When data is positive, compute the RMS instead of the GFP by skipping the re-centering
bool                gfpnotcentered      = IsAbsolute ( atomtype );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numtf           = tf2 - tf1 + 1;

                                        // Adjust buffer to either original or expanded limits
                                        // Note that the previous content could be lost, though this usually is not a problem, as for filtered data, the buffer is usually evaluated as a whole
                                        // If this is still a problem, make sure the buffer is allocated large enough beforehand
buff.Resize (   AtLeast ( buff.GetMaxPlanes (), NumFrequencies                                         ),
                AtLeast ( buff.GetDim1      (), NumElectrodes + ( pseudotracks ? NumPseudoTracks : 0 ) ),
                AtLeast ( buff.GetDim2      (), (int) ( tfoffset + numtf )                             )  );

                                        // BuffDiss dimensions could also be asserted here...

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We have to care for an optional 1 TF buffer used for dissimilarity computation
if ( pseudotracks ) {
    
    if      ( tf1 == 0 )                // original or modified tf1 - Dissimilarity can not be computed 

        BuffDiss.ResetMemory ();        // by safety

    else if ( tf1 > 0 )                 // read an additional time point @ tf1-1 as there is addition margin from any filter here

        ReadFrequencies ( tf1 - 1, tf1 - 1, BuffDiss );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read raw Eeg tracks from file
ReadFrequencies ( tf1, tf2, buff, tfoffset );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // transposed from TTracksDoc::GetTracks
if ( pseudotracks ) {
                                        // count only non-aux and non-bad electrodes
    double              numt            = NonNull ( GetNumValidElectrodes () );
    double              rescalevector   = IsVector ( atomtype ) ? 3 : 1;

                                        // repeat independently for each frequency
    OmpParallelFor

    for ( int f = 0; f < NumFrequencies; f++ ) {

        double              avgbefore       = 0;
        double              gfpbefore       = 1;


        auto    ComputeAvg  = [ this, &numt, &f ] ( TSetArray2<float>& buff, int tfo ) {

            double      sum     = 0;
                                            // don't use ReferenceTracks, as we can be in an overriding case
            for ( int el = 0; el < NumElectrodes; el++ ) 
                if ( ValidTracks[ el ] )
                    sum    += buff ( f, el, tfo );
                
            return  sum / numt;
            };

        auto    ComputeGfp  = [ this, &numt, &f, &rescalevector ] ( TSetArray2<float>& buff, int tfo, double avg ) {

            double      sumsqr  = 0;

            for ( int el = 0; el < NumElectrodes; el++ ) 
                if ( ValidTracks[ el ] )
                    sumsqr += Square ( buff ( f, el, tfo ) - avg );
            
            return  sqrt ( sumsqr / numt * rescalevector );
            };

        auto    ComputeDis  = [ this, &numt, &f, &rescalevector ] ( TSetArray2<float>& buff1, int tfo1, double avg1, double gfp1, TSetArray2<float>& buff2, int tfo2, double avg2, double gfp2 ) {

            double      sumsqr  = 0;

            for ( int el = 0; el < NumElectrodes; el++ ) 
                if ( ValidTracks[ el ] )
                    sumsqr += Square (   ( buff1 ( f, el, tfo1 ) - avg1 ) / gfp1
                                       - ( buff2 ( f, el, tfo2 ) - avg2 ) / gfp2 );
            
            return  sqrt ( sumsqr / numt * rescalevector );
            };


        for ( int tf = 0, tfo = tfoffset; tf < numtf; tf++, tfo++ ) {

            double          avg     = ComputeAvg ( buff, tfo );

            buff ( f, OffAvg, tfo ) = avg;  // saving real average


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

            if ( gfpnotcentered )
                avg     = 0;                // resetting avg? GFP becomes RMS

            double          gfp     = ComputeGfp ( buff, tfo, avg );

            buff ( f, OffGfp, tfo ) = gfp;  // saving real GFP / RMS


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

            NonNulled ( gfp );              // just to prevent division by zero


            if ( tf == 0 ) {                // first occurence in the loop, which means we have to handle some missing data

                if ( tf1 == 0 ) {           // first actual tf: dissimilarity can not be computed, as there is no previous data to compare from

                    buff ( f, OffDis, tfo ) = 0;
                    }
                else {                      // tf1 > 0 - previous data exist and is to be found in our local 1 TF buffer

                    double      avgbuffdiss     = gfpnotcentered ? 0 : ComputeAvg ( BuffDiss, 0 );

                    double      gfpbuffdiss     = NonNull ( ComputeGfp ( BuffDiss, 0, avgbuffdiss ) );

                    buff ( f, OffDis, tfo )     = ComputeDis    (   buff,       tfo,    avg,            gfp,
                                                                    BuffDiss,   0,      avgbuffdiss,    gfpbuffdiss );
                    } // else tf1 > 0

                } // tf == 0

            else {                          // tf > 0 - NOT the first occurence in the loop, and avgbefore and gfpbefore exist

                buff ( f, OffDis, tfo ) = ComputeDis    (   buff,   tfo,        avg,        gfp,
                                                            buff,   tfo - 1,    avgbefore,  gfpbefore );
                }


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                            // store current values for the next tf
            avgbefore   = avg;
            gfpbefore   = gfp;
            } // for tf

        } // for f

    } // if pseudotracks


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // optional ROIing
if ( rois ) {

    int     oldcp   = buff.GetCurrentPlane ();

    for ( int f=0; f < NumFrequencies; f++ ) {

        buff.SetCurrentPlane ( f );

        rois->Average ( buff, tfoffset, tfoffset + numtf - 1, FilterTypeMean );
        }

    buff.SetCurrentPlane ( oldcp );

    // what to do with tracks outside any ROI?
    } // if rois
}


//----------------------------------------------------------------------------
                                        // FFT Approximation loses the maps polarities in the process
                                        // this will re-order the sequence of maps as to keep some temporal consistency
                                        // It is only there for the aesthetic, though
void    TFreqDoc::SortFFTApproximation ()
{
                 // maybe this is too much? in case we remerged tracks and lost the original type...
if ( ! ( IsOpen () && IsFreqTypeFFTApproximation () ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 savedfreq       = GetCurrentFrequency ();
TArray3<float>      results  (  NumTimeFrames,                                                  // !transposed for direct file output!
                                NumElectrodes, 
                                NumFrequencies * ( IsComplex ( AtomTypeUseOriginal ) ? 2 : 1 )  // multiplex the real / imaginary parts manually
                             );
TArray2<float>      tracks      ( NumElectrodes, 1 );
TVector<float>      vref        ( NumElectrodes );
TVector<float>      v           ( NumElectrodes );
TSelection          revertfreq  ( NumFrequencies, OrderSorted );
revertfreq.Reset ();


TExportTracks       expfile;
                                        // clone this document parameters
expfile.CloneParameters ( 0, (char *) GetDocPath () );

StringCopy      ( expfile.Filename, (char *) GetDocPath () );
RemoveExtension ( expfile.Filename );
AddExtension    ( expfile.Filename, "Sorted." FILEEXT_FREQ );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0) Getting first TF / first frequency vector as cornerstone
SetCurrentFrequency ( 0 );

GetTracks   (   0,                  0, 
                tracks,             0, 
                AtomTypeUseCurrent, 
                NoPseudoTracks, 
                ReferenceAsInFile 
            );

vref.GetColumn ( tracks, 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Order all first TF of all freqs, onto freq 0
for ( int f = 0; f < NumFrequencies; f++ ) {

    SetCurrentFrequency ( f );

    GetTracks   (   0,                  0, 
                    tracks,             0, 
                    AtomTypeUseCurrent, 
                    NoPseudoTracks, 
                    ReferenceAsInFile 
                );

    v.GetColumn ( tracks, 0 );

//    DBGV2 ( f, v.Correlation ( vref ), "Freq#   Corr Fi F0" );

    if ( v.Correlation ( vref ) < 0 ) {
        revertfreq.Set ( f );           // remember this for the other TFs
        v.Invert ();
        }

    for ( int el = 0; el < NumElectrodes; el++ )
        results ( 0, el, f )    = v[ el ];

//  vref    = v;
    } // for freq


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) For each frequency, order the remaining TFs according to the first TF
for ( int f = 0; f < NumFrequencies; f++ ) {
                                        // get first TF current freq
    SetCurrentFrequency ( f );
                                        // load all the data at once, for that frequency
    GetTracks   (   0,                  NumTimeFrames - 1,
                    tracks,             0, 
                    AtomTypeUseCurrent, 
                    NoPseudoTracks, 
                    ReferenceAsInFile 
                );

                                        // get first TF reference
    vref.GetColumn ( tracks, 0 );
                                        // revert ref again
    if ( revertfreq[ f ] )
        vref.Invert ();


    for ( long tf = 1; tf < NumTimeFrames; tf++ ) {

        v.GetColumn ( tracks, tf );

        if ( v.Correlation ( vref ) < 0 )
            v.Invert ();

        for ( int el = 0; el < NumElectrodes; el++ )
            results ( tf, el, f )   = v[ el ];
                                        // update reference to try to have some smooth evolution
        vref    = v;
        } // for tf
    } // for freq


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Dump the big block all at once
expfile.Write ( results, NotTransposed );

expfile.End ();

                                        // overwrite current file - !OK as document is not in memory for the moment!
DeleteFileExtended  ( GetDocPath () );
CopyFileExtended    ( expfile.Filename, GetDocPath () );
DeleteFileExtended  ( expfile.Filename );


SetCurrentFrequency ( savedfreq );

                                        // refresh all
NotifyDocViews ( vnReloadData, EV_VN_RELOADDATA_EEG );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
