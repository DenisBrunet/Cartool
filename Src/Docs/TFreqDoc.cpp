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
void    TFreqDoc::InitLimits ( bool precise )
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


precise         = true; // precise && NumTimeFrames < EegMaxPointsDisplay;


for ( CurrentFrequency = downfreq.From; CurrentFrequency <= downfreq.To; CurrentFrequency += downfreq.Step ) {

    TTracksDoc::InitLimits ( precise );

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
void    TFreqDoc::GetTracks (   long                tf1,            long            tf2,
                                TArray2<float>&     buff,           int             tfoffset, 
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


/*                                      // force positive, otherwise trust the settings (data could still have 1 or 2 samples neg)
if ( IsPositive ( atomtype ) ) {
                                        // modify data now
    if ( loadbuffdiss )
        for ( el = 0; el < NumElectrodes; el++ )
            if ( ! AuxTracks[ el ] )    // a priori, auxs could be anything
                BuffDiss[ el ][ 0 ] = max ( BuffDiss[ el ][ 0 ], (float) 0 ); // fabs ( BuffDiss[ el ][ 0 ] );


    for ( el = 0; el < NumElectrodes; el++ )
        if ( ! AuxTracks[ el ] )        // a priori, auxs could be anything
            for ( tf = 0, tfo = tfoffset; tf < numtf; tf++, tfo++ )
                buff[ el ][ tfo ] = max ( buff[ el ][ tfo ], (float) 0 ); // fabs ( buff[ el ][ tfo ] );
    } // positive data
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int                 numtf           = tf2 - tf1 + 1;
int                 tf;
int                 tfo;
int                 tfo1;
double              numt;
double              sum;
double              sumsqr;
double              avg;
double              gfp;
double              avg2;
double              gfp2;
double              temp;
//int               eloffset        = GetFrequencyOffset ();
                                        // check if we need to load a separate 1 TF buffer for the first dissimilarity
bool                loadbuffdiss    = pseudotracks          // only if we compute the pseudos
                                   && tf1 > 0;              // previous absolute TF must exist


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Caller doesn't have to worry about the necessary margin, it will be adjusted here!
                                        // Note that the previous content will be lost, though this usually not a problem as when data is filtered, buffer is usually evaluated as a whole, and not a sub-part of it
                                        // buff can only grow, not shrink
buff.Resize (   max ( pseudotracks ? NumElectrodes + NumPseudoTracks 
                                   : NumElectrodes, 
                      buff.GetDim1 () ), 
                max ( numtf, 
                      buff.GetDim2 () ) );

                                        // BuffDiss dimensions could also be asserted here...

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // load buffer for the Dissimilarity of first time frame
if ( loadbuffdiss ) 
    ReadRawTracks ( tf1 - 1, tf1 - 1, BuffDiss );   // this is not optimal as it launches a separate read for 1 TF - also tf1 > 0 here
else
    BuffDiss.ResetMemory ();                        // just to be clean...


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read raw Eeg tracks from file
ReadRawTracks ( tf1, tf2, buff, tfoffset );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( pseudotracks ) {
                                        // rely on current, or force from user
//  bool                pseudonoavgref  = IsAbsolute ( atomtype );
    double              rescalevector   = IsVector ( atomtype ) ? 3 : 1;
    int                 el;

                                        // count only non-aux and non-bad electrodes
    numt = GetNumValidElectrodes ();
    if ( !numt )    numt = 1;

                                        // compute pseudos
    for ( tf = 0, tfo = tfoffset, tfo1=tfo-1; tf < numtf; tf++, tfo++, tfo1++ ) {

                                        // AVG is needed in either case: pseudos or average reference
                                        // don't use ReferenceTracks, as we can be in an overriding case
        for ( sum = 0, el = 0; el < NumElectrodes; el++ ) {
            if ( BadTracks[ el ] || AuxTracks[ el ] )   continue;
            sum     += buff ( el, tfo );
            }
        buff ( OffAvg, tfo )   = avg   = sum / numt;


//      if ( pseudonoavgref )
//          avg   = 0;                  // don't center on the average

                                        // compute GFP with average ref
        for ( sumsqr = 0, el = 0; el < NumElectrodes; el++ ) {
            if ( BadTracks[ el ] || AuxTracks[ el ] )   continue;
            temp    = buff ( el, tfo ) - avg;
            sumsqr  += temp * temp;
            }
        buff ( OffGfp, tfo )   = gfp = sqrt ( sumsqr / numt * rescalevector );

                                        // real GFP has been saved already, now avoid some 0 division
        if ( !gfp  )    gfp  = 1;

                                        // compute DIS
        if ( !tf )                      // first occurence of the loop?
            if ( !tf1 ) {               // TF 0?
                buff ( OffDis, tfo )   = 0;    // impossible to have the previous value
                }
            else {                      // use local 1 TF buffer, compute again AVG, GFP, then the DIS

                for ( sum = 0, el = 0; el < NumElectrodes; el++ ) {
                    if ( BadTracks[ el ] || AuxTracks[ el ] )   continue;
                    sum     += BuffDiss ( CurrentFrequency, el, 0 );
                    }

//              avg2  = pseudonoavgref ? 0 : sum / numt;
                avg2  = sum / numt;


                for ( sumsqr = 0, el = 0; el < NumElectrodes; el++ ) {
                    if ( BadTracks[ el ] || AuxTracks[ el ] )   continue;
                    temp    = BuffDiss ( CurrentFrequency, el, 0 ) - avg2;
                    sumsqr  += temp * temp;
                    }

                gfp2 = sqrt ( sumsqr / numt * rescalevector );

                if ( !gfp2 )    gfp2 = 1;


                for ( sumsqr = 0, el = 0; el < NumElectrodes; el++ ) {
                    if ( BadTracks[ el ] || AuxTracks[ el ] )   continue;
                    temp    = ( buff ( el, tfo  )                     - avg  ) / gfp
                            - ( BuffDiss ( CurrentFrequency, el, 0 )  - avg2 ) / gfp2;
                    sumsqr  += temp * temp;
                    }

                buff ( OffDis, tfo )   = sqrt ( sumsqr / numt * rescalevector );
                }
        else  {                         // NOT the first occurence in the loop

            for ( sumsqr = 0, el = 0; el < NumElectrodes; el++ ) {
                if ( BadTracks[ el ] || AuxTracks[ el ] )   continue;
                temp    = ( buff ( el, tfo  ) - avg  ) / gfp
                        - ( buff ( el, tfo1 ) - avg2 ) / gfp2;
                sumsqr  += temp * temp;
                }
            buff ( OffDis, tfo )   = sqrt ( sumsqr / numt * rescalevector );
            }

        gfp2 = gfp;                     // for next occurence
        avg2 = avg;                     // for next occurence
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

/*                                      // force positive, otherwise trust the settings (data could still have 1 or 2 samples neg)
if ( IsPositive ( atomtype ) ) {
                                        // modify data now
    if ( loadbuffdiss )
        for ( el = 0; el < NumElectrodes; el++ )
            if ( ! AuxTracks[ el ] )    // a priori, auxs could be anything
                BuffDiss ( el, 0 ) = max ( BuffDiss ( el, 0 ), (float) 0 ); // fabs ( BuffDiss ( el, 0 ) );


    for ( el = 0; el < NumElectrodes; el++ )
        if ( ! AuxTracks[ el ] )        // a priori, auxs could be anything
            for ( tf = 0, tfo = tfoffset; tf < numtf; tf++, tfo++ )
                buff ( el, tfo ) = max ( buff ( el, tfo ), (float) 0 ); // fabs ( buff ( el, tfo ) );
    } // positive data
*/


//                                        // To prevent average reference in case of positive data - Off now, always use the average ref
//bool                pseudonoavgref  = IsAbsolute ( atomtype );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int                 numtf           = tf2 - tf1 + 1;
int                 tf;
int                 tfo;
int                 tfo1;
double              numt;
double              sum;
double              sumsqr;
double              avg;
double              gfp;
double              avg2;
double              gfp2;
double              temp;
//int               eloffset        = GetFrequencyOffset ();
                                        // check if we need to load a separate 1 TF buffer for the first dissimilarity
bool                loadbuffdiss    = pseudotracks          // only if we compute the pseudos
                                   && tf1 > 0;              // previous absolute TF must exist


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Caller doesn't have to worry about the necessary margin, it will be adjusted here!
                                        // Note that the previous content will be lost, though this usually not a problem as when data is filtered, buffer is usually evaluated as a whole, and not a sub-part of it
                                        // buff can only grow, not shrink
buff.Resize (   max ( NumFrequencies,
                      buff.GetMaxPlanes () ), 
                max ( pseudotracks ? NumElectrodes + NumPseudoTracks 
                                   : NumElectrodes, 
                      buff.GetDim1 () ), 
                max ( numtf, 
                      buff.GetDim2 () ) );

                                        // BuffDiss dimensions could also be asserted here...

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // load buffer for the Dissimilarity of first time frame
if ( loadbuffdiss ) 
    ReadFrequencies ( tf1 - 1, tf1 - 1, BuffDiss ); // this is not optimal as it launches a separate read for 1 TF - also tf1 > 0 here
else
    BuffDiss.ResetMemory ();                        // just to be clean...


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read raw Eeg tracks from file
ReadFrequencies ( tf1, tf2, buff, tfoffset );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( pseudotracks ) {
                                        // rely on current, or force from user
    double              rescalevector   = IsVector ( atomtype ) ? 3 : 1;
    int                 el;

                                        // count only non-aux and non-bad electrodes
    numt = GetNumValidElectrodes ();
    if ( !numt )    numt = 1;

                                        // repeat independently for each frequency
    for ( int f = 0; f < NumFrequencies; f++ ) {
                                        // compute pseudos
        for ( tf = 0, tfo = tfoffset, tfo1 = tfo - 1; tf < numtf; tf++, tfo++, tfo1++ ) {

                                        // AVG is needed in either case: pseudos or average reference
                                        // don't use ReferenceTracks, as we can be in an overriding case
            for ( sum = 0, el = 0; el < NumElectrodes; el++ ) {
                if ( BadTracks[ el ] || AuxTracks[ el ] )   continue;
                sum     += buff ( f, el, tfo );
                }
            buff ( f, OffAvg, tfo ) = avg   = sum / numt;


//          if ( pseudonoavgref )
//              avg   = 0;                  // don't center on the average

                                            // compute GFP with average ref
            for ( sumsqr = 0, el = 0; el < NumElectrodes; el++ ) {
                if ( BadTracks[ el ] || AuxTracks[ el ] )   continue;
                temp    = buff ( f, el, tfo ) - avg;
                sumsqr  += temp * temp;
                }
            buff ( f, OffGfp, tfo ) = gfp = sqrt ( sumsqr / numt * rescalevector );

                                            // real GFP has been saved already, now avoid some 0 division
            if ( !gfp  )    gfp  = 1;

                                            // compute DIS
            if ( !tf )                      // first occurence of the loop?
                if ( !tf1 ) {               // TF 0?
                    buff ( f, OffDis, tfo ) = 0;    // impossible to have the previous value
                    }
                else {                      // use local 1 TF buffer, compute again AVG, GFP, then the DIS

                    for ( sum = 0, el = 0; el < NumElectrodes; el++ ) {
                        if ( BadTracks[ el ] || AuxTracks[ el ] )   continue;
                        sum     += BuffDiss ( f, el, 0 );
                        }

//                  avg2  = pseudonoavgref ? 0 : sum / numt;
                    avg2  = sum / numt;


                    for ( sumsqr = 0, el = 0; el < NumElectrodes; el++ ) {
                        if ( BadTracks[ el ] || AuxTracks[ el ] )   continue;
                        temp    = BuffDiss ( f, el, 0 ) - avg2;
                        sumsqr  += temp * temp;
                        }

                    gfp2 = sqrt ( sumsqr / numt * rescalevector );

                    if ( !gfp2 )    gfp2 = 1;


                    for ( sumsqr = 0, el = 0; el < NumElectrodes; el++ ) {
                        if ( BadTracks[ el ] || AuxTracks[ el ] )   continue;
                        temp    = ( buff     ( f, el, tfo ) - avg  ) / gfp
                                - ( BuffDiss ( f, el, 0 )   - avg2 ) / gfp2;
                        sumsqr  += temp * temp;
                        }

                    buff ( f, OffDis, tfo ) = sqrt ( sumsqr / numt * rescalevector );
                    }
            else  {                         // NOT the first occurence in the loop

                for ( sumsqr = 0, el = 0; el < NumElectrodes; el++ ) {
                    if ( BadTracks[ el ] || AuxTracks[ el ] )   continue;
                    temp    = ( buff ( f, el, tfo  ) - avg  ) / gfp
                            - ( buff ( f, el, tfo1 ) - avg2 ) / gfp2;
                    sumsqr  += temp * temp;
                    }

                buff ( f, OffDis, tfo ) = sqrt ( sumsqr / numt * rescalevector );
                }

            gfp2 = gfp;                     // for next occurence
            avg2 = avg;                     // for next occurence
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
