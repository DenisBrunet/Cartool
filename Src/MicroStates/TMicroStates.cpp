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

#include    "TMicroStates.h"

#include    "Math.Utils.h"
#include    "TInterval.h"
#include    "TArray1.h"
#include    "TArray2.h"
#include    "Files.ReadFromHeader.h"
#include    "Files.TOpenDoc.h"
#include    "TExportTracks.h"

#include    "TTracksDoc.h"
#include    "TRisDoc.h"
#include    "TVolumeDoc.h"
#include    "TInverseMatrixDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const char  ClusteringString[ NumClusteringType ][ 128 ] =
            {
            "Unknown clustering",
            "k-means",
            "T-AAHC (Topographic Atomize & Agglomerate Hierarchical Clustering)",
            };


const char  MapOrderingString[ NumMapOrering ][ 64 ] =
            {
            "No reordering",
            "Reordering temporally",
            "Reordering topographically",
            "Reordering by solution points",
            "Reordering anatomically",
            "Reordering from templates",
            "Reordering according to data",
            };


const char  SegmentCriteriaVariablesNames[ segnumvar ][ 32 ] = 
            {
            TrackNameClust,
            TrackNameMaps,
            TrackNameGEV,

            TrackNameWforKL,
            "WCD",
            "WCD2",
            "BCD2",
            "WPD",
            "BPD",
            "APD",
            "WPD2",
            "WPdD2",

            TrackNameCalinskiHarabasz,
            TrackNameCalinskiHarabasz PostfixCriterionDerivative2,
            TrackNameCalinskiHarabasz PostfixCriterionRobust PostfixCriterionDerivative2,
            TrackNameCIndex,
            TrackNameCIndex PostfixCriterionDerivative2,
            TrackNameCIndex PostfixCriterionRobust PostfixCriterionDerivative2,
            TrackNameCrossValidation,
            TrackNameCrossValidation PostfixCriterionDerivative2,
            TrackNameCrossValidation PostfixCriterionRobust PostfixCriterionDerivative2,
            TrackNameDaviesBouldin,
            TrackNameDaviesBouldin PostfixCriterionDerivative2,
            TrackNameDaviesBouldin PostfixCriterionRobust PostfixCriterionDerivative2,
            TrackNameDunn,
            TrackNameDunn PostfixCriterionDerivative2,
            TrackNameDunn PostfixCriterionRobust PostfixCriterionDerivative2,
            TrackNameDunnRobust,
            TrackNameDunnRobust PostfixCriterionDerivative2,
            TrackNameDunnRobust PostfixCriterionRobust PostfixCriterionDerivative2,
            TrackNameFreyVanGroenewoud,
            TrackNameFreyVanGroenewoud PostfixCriterionDerivative2,
            TrackNameFreyVanGroenewoud PostfixCriterionRobust PostfixCriterionDerivative2,
            TrackNameGamma,
            TrackNameGamma PostfixCriterionDerivative2,
            TrackNameGamma PostfixCriterionRobust PostfixCriterionDerivative2,
            TrackNameGPlus,
            TrackNameHartigan,
            TrackNameHartigan PostfixCriterionDerivative2,
            TrackNameHartigan PostfixCriterionRobust PostfixCriterionDerivative2,
            TrackNameKrzanowskiLai,
            TrackNameKrzanowskiLaiC,
            TrackNameKrzanowskiLai PostfixCriterionRobust,
            TrackNameKrzanowskiLaiC PostfixCriterionRobust,
            TrackNameMarriott,
            TrackNameMcClain,
            TrackNameMcClain PostfixCriterionDerivative2,
            TrackNameMcClain PostfixCriterionRobust PostfixCriterionDerivative2,
            TrackNamePointBiserial,
            TrackNamePointBiserial PostfixCriterionDerivative2,
            TrackNamePointBiserial PostfixCriterionRobust PostfixCriterionDerivative2,
            TrackNameRatkowski,
            TrackNameRatkowski PostfixCriterionDerivative2,
            TrackNameRatkowski PostfixCriterionRobust PostfixCriterionDerivative2,
            TrackNameSilhouettes,
            TrackNameSilhouettes PostfixCriterionDerivative2,
            TrackNameSilhouettes PostfixCriterionRobust PostfixCriterionDerivative2,
            TrackNameTau,
            TrackNameTraceW,

            TrackNameMeanRanks,
            TrackNameArgMaxHisto,
//          TrackNameArgMax,
//          "Merged3",

            TrackNameMetaCriterion,
            };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TMicroStates::TMicroStates ()
{
NumElectrodes           = 0;
NumRows                 = 0;
TotalElectrodes         = 0;
NumTimeFrames           = 0;
SamplingFrequency       = 0;
MaxNumTF                = 0;

NumFiles                = 0;

CriterionDownSampling   = 0;
}


        TMicroStates::~TMicroStates ()
{
//DeallocateVariables ();
}


void    TMicroStates::DeallocateVariables ()
{
Data        .DeallocateMemory ();
Gfp         .DeallocateMemory ();
Dis         .DeallocateMemory ();
Norm        .DeallocateMemory ();
NumTF       .DeallocateMemory ();
OffsetTF    .DeallocateMemory ();
//AbsTFToRelTF.DeallocateMemory ();
IntervalTF.resize ( 0 );
}


//----------------------------------------------------------------------------
                                        // Setting variables without reading actual data, for group(s) gofi1 to gofi2, with optional file range filei1 to filei2:
                                        //
                                        //  OriginalAtomType
                                        //  NumFiles
                                        //  NumElectrodes
                                        //  TotalElectrodes
                                        //  NumRows
                                        //
                                        //  NumTimeFrames
                                        //  MaxNumTF
                                        //  NumTF       [ NumFiles     ]
                                        //  OffsetTF    [ NumFiles + 1 ]
                                        //  IntervalTF  [ NumFiles     ]
                                        //
                                        //  Data        [ NumTimeFrames ] allocated, but not read / set
                                        //  Gfp         [ NumTimeFrames ] allocated, but not read / set
                                        //  Dis         [ NumTimeFrames ] allocated, but not read / set
                                        //  Norm        [ NumTimeFrames ] allocated, but not read / set
void    TMicroStates::AllocateVariables     (   const TGoGoF&   gogof,          int     gofi1,      int             gofi2,
                                                int             filei1,         int     filei2,
                                                AtomType        datatype 
                                            )
{
//DeallocateVariables ();               // not really needed as all fields will rescale gracefully anyway


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//OriginalAtomType    = gogof.AllAre ( gofi1, gofi2, FILEEXT_RIS, AtomTypeVector ) ? AtomTypeVector : AtomTypeScalar;
                                        // it shouldn't happen, but just in case data type changed across groups of files
TDataFormat::Reset ();

SetAtomType ( datatype );               // trust / override by the caller


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // # files, with optional files sub-range
NumFiles    = gogof.NumFiles ( gofi1, gofi2, filei1, filei2 );

                                        // now set default file indice if needed
if ( filei1 == -1 || filei2 == -1 ) {
    filei1  = 0;
    filei2  = gogof.GetMaxFiles ( gofi1, gofi2 ) - 1;
    }

                                        // some more checking
Clipped ( filei1, filei2, 0, gogof.GetMaxFiles ( gofi1, gofi2 ) - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // # electrodes & rows
                                        // the number of electrodes should be the same across files of the same group, but just in case take the first selected file
ReadFromHeader ( gogof[ gofi1 ][ filei1 ], ReadNumElectrodes, &NumElectrodes );


NumRows             = NumElectrodes * ( IsVector ( AtomTypeUseOriginal ) ? 3 : 1 );  // This is used only for our storage arrays, vectors have 3 times more rows. NumElectrodes remains the same for all the formulas, though
TotalElectrodes     = NumRows + NumPseudoTracks;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // # time frames
NumTimeFrames       = 0;
NumTF.Resize ( NumFiles );
MaxNumTF            = 0;


for ( int f = 0, absg = gofi1; absg <= gofi2; absg++ ) {

                                        // limit the files to current group max - not used for the moment, and there should be way for the caller to know how the data are finally laid out
    for ( int fi = filei1; fi <= min ( filei2, gogof[ absg ].NumFiles () - 1 ); fi++ ) {

        ReadFromHeader ( gogof[ absg ][ fi ], ReadNumTimeFrames, &NumTF[ f ] );

        NumTimeFrames  += NumTF[ f ];

        Maxed ( MaxNumTF, NumTF[ f ] );

        f++;
        } // for fi

    } // for absg


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // offset in all TFs for a each file
OffsetTF.Resize ( NumFiles + 1 );

OffsetTF[ 0 ]       = 0;

for ( int f = 1; f <= NumFiles; f++ )
    OffsetTF[ f ]   = OffsetTF[ f - 1 ] + NumTF[ f - 1 ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // interval of TFs for each file
IntervalTF.resize ( NumFiles );

for ( int f = 0; f < NumFiles; f++ )
    IntervalTF[ f ] = TInterval ( OffsetTF[ f ], OffsetTF[ f + 1 ] - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // create more buffers
AbsTFToRelTF.Resize ( NumTimeFrames );


long                tforg           = 0;
int                 curreeg         = 0;


for ( long tf = 0; tf < NumTimeFrames; tf++ ) {
    if ( tf - tforg == NumTF[ curreeg ] ) {
        tforg  += NumTF[ curreeg ];
        curreeg++;
        }

    AbsTFToRelTF[ tf ]  = tf - tforg;
    }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // allocate all structures, loading actual data is done later on
Data.Resize ( NumTimeFrames, NumRows );
Gfp .Resize ( NumTimeFrames );
Dis .Resize ( NumTimeFrames );
Norm.Resize ( NumTimeFrames );
}


//----------------------------------------------------------------------------
                                        // Simplified version of AllocateVariables above
void    TMicroStates::AllocateVariables     (   const TGoF&     gof, 
                                                AtomType        datatype
                                            )
{
AllocateVariables   (   gof, 0, 0,  // converts to TGoGoF automatically
                        -1, -1, 
                        datatype
                    );
}


//----------------------------------------------------------------------------
                                        // Simplified version of AllocateVariables above
void    TMicroStates::AllocateVariables     (   const TGoGoF&   gogof,          int     gofi1,      int             gofi2,
                                                int             filei,          // index for single subject
                                                AtomType        datatype
                                            )
{
AllocateVariables   (   gogof, gofi1, gofi2,
                        filei, filei,
                        datatype
                    );
}


//----------------------------------------------------------------------------
                                        // Fill data array from a list of files
                                        // !Average Reference does not apply for inverse!
                                        // Output:
                                        //  Data        [ NumTimeFrames ] read with given parameters
void    TMicroStates::ReadData  (   const TGoGoF&   gogof,          int             gofi1,      int         gofi2,
                                    int             filei1,         int             filei2,
                                    AtomType        datatype,       ReferenceType   dataref,
                                    bool            showprogress 
                                )
{
                                        // now set default file indice if needed
if ( filei1 == -1 || filei2 == -1 ) {
    filei1  = 0;
    filei2  = gogof.GetMaxFiles ( gofi1, gofi2 ) - 1;
    }

                                        // some more checking
Clipped ( filei1, filei2, 0, gogof.GetMaxFiles ( gofi1, gofi2 ) - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

AllocateVariables   (   gogof,  gofi1,  gofi2,
                        filei1, filei2,
                        datatype
                    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read data in
TOpenDoc< TTracksDoc >  EEGDoc;
TOpenDoc< TRisDoc >     RISDoc;

                                        // !NOT resizing here, as we don't know yet the filters' margin's size actually needed!
TTracks<float>          EegBuff;
TArray1<TVector3Float>  InvBuff;



for ( int f = 0, absg = gofi1; absg <= gofi2; absg++ ) {

                                        // limit the files to current group max - not used for the moment, and there should be way for the caller to know how the data are finally laid out
    for ( int fi = filei1; fi <= min ( filei2, gogof[ absg ].NumFiles () - 1 ); fi++ ) {


        if ( showprogress )
            Gauge.Next ();              // using currently set part


        if ( ! IsVector ( datatype ) ) {

            EEGDoc.Open ( gogof[ absg ][ fi ], OpenDocHidden );
                                        // resize the biggest size, as to avoid re-allocations if possible
            EegBuff.Resize ( TotalElectrodes, MaxNumTF /*NumTF[ f ]*/ ); 

            if ( SamplingFrequency == 0 )   SamplingFrequency   = EEGDoc->GetSamplingFrequency ();

                                        // get all data at once
            EEGDoc->GetTracks   (   0,          NumTF[ f ] - 1, 
                                    EegBuff,    0, 
                                    datatype,
                                    NoPseudoTracks, 
                                    dataref
                                );

                                        // read data with requested reference & type, correctly offseted
            for ( int tf0 = 0, tf = OffsetTF[ f ] + tf0; tf0 < NumTF[ f ]; tf0++, tf++ )

                Data[ tf ].GetColumn ( EegBuff, tf0 );


            EEGDoc.Close ( CloseDocRestoreAsBefore );
            } // ! vector

        else { // IsVector

            RISDoc.Open ( gogof[ absg ][ fi ], OpenDocHidden );

            InvBuff.Resize ( NumElectrodes );

            SamplingFrequency   = SamplingFrequency == 0 ? RISDoc->GetSamplingFrequency () : SamplingFrequency;


            for ( int tf0 = 0, tf = OffsetTF[ f ] + tf0; tf0 < NumTF[ f ]; tf0++, tf++ ) {
                                        // get a single TF, and we know the data are 3D vectors
                RISDoc->GetInvSol ( 0, tf0, tf0, InvBuff, 0, 0 );

                                        // spread x, y, z by rows
                for ( int e1 = 0, e3 = 0; e1 < NumElectrodes; e1++, e3 += 3 ) {
                    Data[ tf ][ e3     ]    = InvBuff[ e1 ].X;
                    Data[ tf ][ e3 + 1 ]    = InvBuff[ e1 ].Y;
                    Data[ tf ][ e3 + 2 ]    = InvBuff[ e1 ].Z;
                    }

                }


            RISDoc.Close ( CloseDocRestoreAsBefore );
            } // IsVector

        f++;
        } // for fi

    } // for absg


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Complimentary building the array of pointers to TMap's
Data.GetIndexes ( ToData );
}


//----------------------------------------------------------------------------
                                        // Simplified version of ReadData above
void    TMicroStates::ReadData  (   const TGoF&     gof, 
                                    AtomType        datatype,       ReferenceType   dataref,
                                    bool            showprogress 
                                )
{
ReadData    (   gof, 0, 0,  // converts to TGoGoF automatically
                -1, -1,
                datatype,   dataref,
                showprogress 
            );
}


//----------------------------------------------------------------------------
                                        // Simplified version of ReadData above
void    TMicroStates::ReadData  (   const TGoGoF&   gogof,          int             gofi1,      int         gofi2,
                                    int             filei,
                                    AtomType        datatype,       ReferenceType   dataref,
                                    bool            showprogress 
                                )
{
ReadData    (   gogof, gofi1, gofi2,
                filei, filei,
                datatype,   dataref,
                showprogress 
            );
}


//----------------------------------------------------------------------------
                                        // Getting maps ready for internal use, basically centered and normalized by with some other fancy options, too
                                        // !dataref being use to save the optional  Norm, Gfp and Dis  arrays, and is useless in all other cases!
                                        // !processingref is the final ref applied, used for processing!
void    TMicroStates::PreprocessMaps(   TMaps&          maps,
                                        bool            esizscored,     bool            esizscoreshift1,
                                        AtomType        datatype,       PolarityType    polarity,       ReferenceType   dataref,
                                        bool            ranking,
                                        ReferenceType   processingref,
                                        bool            normalizing,
                                        bool            computeandsavenorm
                                    )
{
//#define     TrackingPreprocData

#if defined(TrackingPreprocData)
TFileName           basefilename ( "E:\\Data\\TMicroStates.PreprocessMaps" );
TFileName           _file;
StringCopy      ( _file, basefilename, ".1.Original.ris" );
maps.WriteFile  ( _file );
#endif

                                        // Optional Inverse preprocessing
if ( esizscored ) {
                                        // Getting rid of any existing biased ZScore, usually shifting background values from 1 back to 0
                                        // If data are ranked later, this has basically no impact for ERP case - For RS it does anyway due to the Absolute folding
                                        // However, it changes the global curves Norm and GFP, which has some small effect on the Smoothing and GEV
                                        // Removing the bias is more optimal, as the background is now 0

                                        // Center data back to 0 + NOT folding values below 0 - low values are not oscillations but ERP convergence
//  maps.ZPositiveToZSigned     ();
                                        // Same, BUT average of vectorial Z-Score'd are NOT centered around 1, so this correction will not be applied, which is correct
    maps.ZPositiveToZSignedAuto ();

    if ( esizscoreshift1 )
                                        // re-shift to 1
        maps    += 1;
    else
                                        // After the removing the ZScore bias, fold Z values below 0 - oscillating dipoles close to 0 ARE meaningful
        maps.Absolute ();

    #if defined(TrackingPreprocData)
    StringCopy      ( _file, basefilename, esizscoreshift1 ? ".2.ZScorePO.ris" : ".2.ZScorePA.ris" );
    maps.WriteFile  ( _file );
    #endif
    } // esizscored


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optional, should only be used when called with Data
if ( computeandsavenorm ) {
                                        // Init these arrays before any normalization below
    maps.ComputeNorm            ( Norm, dataref );

    maps.ComputeGFP             ( Gfp,  ReferenceAverage /*dataref*/, datatype );

    maps.ComputeDissimilarity   ( Dis,  polarity, dataref );
                                        // clearing-up dissimilarity array at files junctions
    ClearDisJunctions ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ranking ) {               

    maps.ToRank ( datatype, RankingOptions ( RankingAccountNulls | RankingMergeIdenticals ) );

    #if defined(TrackingPreprocData)
    StringCopy      ( _file, basefilename, ".3.Rank.ris" );
    maps.WriteFile  ( _file );
    #endif
    } // ranking


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // processing reference might be different than data reference
maps.SetReference ( processingref, datatype );

#if defined(TrackingPreprocData)
StringCopy      ( _file, basefilename, ".4.Reference.ris" );
maps.WriteFile  ( _file );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // normalizing
if ( normalizing ) {

    maps.Normalize ( datatype );

    #if defined(TrackingPreprocData)
    StringCopy      ( _file, basefilename, ".5.Normalized.ris" );
    maps.WriteFile  ( _file );
    #endif
    }

}


//----------------------------------------------------------------------------
                                        // From labeling, comput the GEV per cluster
void    TMicroStates::ComputeGevPerCluster  (   int                 nclusters,  
                                                const TMaps&        maps,  
                                                const TLabeling&    labels,  
                                                PolarityType        polarity,
                                                TVector<double>&    gevperclusterrel,
                                                TVector<double>&    gevperclusterabs
                                            )   const
{
gevperclusterrel.Resize ( nclusters + 1 );

for ( long tf = 0; tf < NumTimeFrames; tf++ )

    if ( labels.IsDefined ( tf ) ) {
                                                // both Norm and Data accounted here
        double          gc              = Square ( Norm[ tf ] * Project ( maps[ labels[ tf ] ], Data[ tf ], labels.GetPolarity ( tf ) ) );

        gevperclusterrel[ labels[ tf ] ] += gc;  // per cluster
        gevperclusterrel[ nclusters    ] += gc;  // total

//      gevperclusterrel[ labels[ tf ] ] += Square ( projection[ tf ] );
        }

gevperclusterrel   /= NonNull ( gevperclusterrel[ nclusters ] );    // normalize by the total numerator

                                                // compute the absolute distribution of GEV
double              gev             = ComputeGEV ( maps, labels, polarity, 0, NumTimeFrames - 1 );

gevperclusterabs    = gevperclusterrel * gev;
}


//----------------------------------------------------------------------------

void    TMicroStates::WriteSegFile  (   int                 nclusters,  
                                        const TMaps&        maps,  
                                        const TLabeling&    labels,  
                                        PolarityType        polarity,
                                        const char*         filename 
                                    )   const
{
if ( StringIsEmpty ( filename ) || NumFiles == 0 || MaxNumTF == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              gfpscale        = NonNull ( Gfp.GetMeanValue () );  // synth file are text files, magnetometer values can be very small and don't fit into the fixed space allocated


TVector<double>     gevperclusterrel;
TVector<double>     gevperclusterabs;

ComputeGevPerCluster    (   nclusters,  
                            maps,  
                            labels,  
                            polarity,
                            gevperclusterrel,
                            gevperclusterabs
                        );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TExportTracks       expseg;

StringCopy ( expseg.Filename, filename );

expseg.NumClusters        = nclusters;
expseg.NumFiles           = NumFiles;
expseg.NumTime            = MaxNumTF;
expseg.NumTracks          = NumSegVariables;
expseg.VariableNames.Set ( SegFile_VariableNames_Signed, " " Tab );


for ( long tf = 0; tf < MaxNumTF; tf++ ) {
                                        // segment output
    for ( int fi = 0; fi < NumFiles; fi++ ) {

        long        tf2     = OffsetTF[ fi ] + tf;

        if ( tf < NumTF[ fi ] ) {

            double      corr    = maps[ labels[ tf2 ] ].Correlation ( Data[ tf2 ], labels.GetPolarity ( tf2 ), false /*processingref == ReferenceAverage*/ );

            expseg.Write ( (float) ( Gfp[ tf2 ] / gfpscale ) );
            expseg.Write ( (float)   labels.GetSign ( tf2 ) );
            expseg.Write ( (float) ( labels.IsUndefined ( tf2 ) ? 0 :                   labels[ tf2 ] + 1 ) );
            expseg.Write ( (float) ( labels.IsUndefined ( tf2 ) ? 0 : gevperclusterabs[ labels[ tf2 ] ]   ) );
            expseg.Write ( (float)   corr );
            }
        else {                      // pad current file with 0's
            for ( int vari = 0; vari < NumSegVariables; vari++ )
                expseg.Write ( (float) 0 );
            }
        }
    }
}


//----------------------------------------------------------------------------
void    TMicroStates::TimeRangeToDataRange ( int filei, long fromtf, long totf, long &tfmin, long &tfmax )
{
                                        // time window
IntervalTF[ filei ].SetMinMax ( IntervalTF[ filei ].GetLimitMin () + fromtf, 
                                IntervalTF[ filei ].GetLimitMin () + totf    );
                                        // returning proper data indexes
tfmin   = IntervalTF[ filei ].GetMin ();
tfmax   = IntervalTF[ filei ].GetMax ();
}


//----------------------------------------------------------------------------
                                        // Update dissimilarity at files' junction - not really used, as it is dealt when writing files
void    TMicroStates::ClearDisJunctions ()
{
for ( int i = 0; i < NumFiles; i++ ) {
                                        // this one is enough for the regular Dis(n,n-1) formula
    Dis[ OffsetTF[ i     ]     ]    = 0;
                                        // needed with improved formula (Dis(n,n-1)+Dis(n,n+1))/2
    Dis[ OffsetTF[ i + 1 ] - 1 ]    = 0;
    }
}


//----------------------------------------------------------------------------
                                        // Compute SigmaMu2, the residual variance - exact formula, including scaling constants
double  TMicroStates::ComputeSigmaMu2   (   int                 numelectrodes,
                                            const TMaps&        maps,           
                                            const TLabeling&    labels,
                                            PolarityType        polarity,
                                            long                tfmin,          long            tfmax 
                                        )
{
double              sigma2          = 0;
int                 numtf           = 0;
                                        // Note that the parallel sum can introduce epsilon differences due to the sub-sums order
OmpParallelForSum ( sigma2, numtf )

for ( long tf = tfmin; tf <= tfmax; tf++ ) {

    if ( labels.IsUndefined ( tf ) )
        continue;

                                                                // maps are already centered and normalized
    sigma2     += Square ( Norm[ tf ] ) * ( 1 - Square ( Project ( maps[ labels[ tf ] ], Data[ tf ], polarity ) ) );

    numtf++;
    }


return  sigma2 / (double) NonNull ( numtf * ( numelectrodes - 1 ) );
}


//----------------------------------------------------------------------------
                                        // Compute SigmaD2, the sum of square of norms
double  TMicroStates::ComputeSigmaD2    (   int                 numelectrodes,
                                            const TLabeling&    labels,
                                            long                tfmin,          long            tfmax 
                                        )
{
double              sigma2          = 0;
int                 numtf           = 0;
                                        // Note that the parallel sum can introduce epsilon differences due to the sub-sums order
OmpParallelForSum ( sigma2, numtf )

for ( long tf = tfmin; tf <= tfmax; tf++ ) {

    if ( labels.IsUndefined ( tf ) )
        continue;


    sigma2     += Square ( Norm[ tf ] );

    numtf++;
    }


return  sigma2 / (double) NonNull ( numtf * ( numelectrodes - 1 ) );
}


//----------------------------------------------------------------------------
                                        // Compute Global Explain Variance (R2 in article)
                                        // Exact formula, but extra computation costs
/*
double  TMicroStates::ComputeGEV    (   const TMaps&        maps,       
                                        const TLabeling&    labels,
                                        PolarityType        polarity,
                                        long                tfmin,      long            tfmax 
                                    )
{
double              gev;

                                 // !we don't actually care for NumElectrodes, as a the ratio will cancel it up!
double              sigmamu2        = ComputeSigmaMu2 ( /*NumElectrodes* / 2, maps, labels, polarity, tfmin, tfmax );     // residual variance

double              sigmad2         = ComputeSigmaD2  ( /*NumElectrodes* / 2,       labels,           tfmin, tfmax );     // scaling to square of data


                                        // This is equivalent to Sum Square ( Norm * Project ( Data ) ) / Sum Squares data
                                        // which means we can split the GEV into contributions per segment f.ex.
gev     = sigmad2 > 0   ? 1 - sigmamu2 / sigmad2
                        : 0;            // 0 means no labeling, and nothing gets explained here so return 0, not 1

                                        // nicely clip the results in [0..1] by safety
return  Clip ( gev, 0.0, 1.0 );
}
*/

                                        // Compute Global Explain Variance
                                        // Simplified formula, more direct without computing Sigma2 (Version 4489)
double  TMicroStates::ComputeGEV    (   const TMaps&        maps,       
                                        const TLabeling&    labels,
                                        PolarityType        polarity,
                                        long                tfmin,      long            tfmax 
                                    )   const
{
double              sigma2          = 0;
double              sigmad          = 0;


OmpParallelForSum ( sigma2, sigmad )

for ( long tf = tfmin; tf <= tfmax; tf++ ) {

    if ( labels.IsUndefined ( tf ) )
        continue;
    
                                        // not the full sigma2 formula here, but simplified to: Sum sigma2 / Sum sigmad
    sigma2     += Square ( Norm[ tf ] * maps[ labels[ tf ] ].Correlation ( Data[ tf ], polarity, false ) );
    sigmad     += Square ( Norm[ tf ] );
    }

                                        // GEV actually boils down to this
return  sigma2 / NonNull ( sigmad );
}


//----------------------------------------------------------------------------
                                        // !Spreading formula should maybe take into account the spherical aspect of the data, using angular statistics!
void    TMicroStates::ComputeClustersDispersion (   int                 nummaps,
                                                    TMaps&              maps,       TLabeling&          labels,
                                                    PolarityType        polarity,
                                                    TArray1<double>&    dispersion,
                                                    bool                precise
                                                )
{
enum        DispersionEnum
            {
//          DispersionCentroidMode,
//          DispersionCentroidSDLeft,
//          DispersionCentroidSDRight,
            DispersionCentroidSD,
            DispersionCentroidMAD,

            DispersionPooledMAD,

            DispersionMean,

            NumDispersionEnum
            };


TArray2<double>     alldisp     ( precise ? NumDispersionEnum : 0, precise ? nummaps : 0 );
TEasyStats          statcluster ( precise ? 1024 : 0 );

dispersion.Resize ( nummaps );


for ( int nc = 0; nc < nummaps; nc++ ) {

    statcluster.Reset ();

                                        // simplest & fastest
    ComputeW    ( nc, maps, labels, polarity, (WFlag) ( WFromCentroid        | WDistance         ), &statcluster, 0, 0, 0 );


    if ( precise ) {

//      alldisp ( DispersionCentroidMode , nc )     = statcluster.MaxModeHistogram ();
//      
//                                      // robust centroid + left-right SD
//      statcluster.SDAsym ( alldisp ( DispersionCentroidMode ,      nc ),
//                           alldisp ( DispersionCentroidSDLeft ,    nc ),
//                           alldisp ( DispersionCentroidSDRight ,   nc )  );
        
                                        // do the SD ourselves, the "mean" (centroid) has already been "subtracted" (distance to centroid)
        alldisp ( DispersionCentroidSD   , nc )     = sqrt ( statcluster.Sum2 ()  / NonNull ( statcluster.GetNumItems () - 1 ) );
        
                                        // this is equivalent to a MAD, we can rescale the results to an equivalent sigma
        alldisp ( DispersionCentroidMAD  , nc )     = statcluster.Median () * sqrt ( MADToSigma );
        
        
                                        // now go for pooled estimators
        statcluster.Reset ();
        
        ComputeW    ( nc, maps, labels, polarity, (WFlag) ( WPooled              | WDistance         ), &statcluster, 0, 0, 0 );
        
                                        // the most robust, looks pretty much exactly as the mean of the 3 estimates
        alldisp ( DispersionPooledMAD    , nc )     = statcluster.Median () / sqrt ( MADToSigma );
        
        
        alldisp ( DispersionMean         , nc )     = ( alldisp ( DispersionCentroidSD    , nc  )
                                                      + alldisp ( DispersionCentroidMAD   , nc  )
                                                      + alldisp ( DispersionPooledMAD     , nc  ) ) / 3;
        
        dispersion ( nc )   = alldisp ( DispersionMean, nc );
        } // precise

    else // faster
                                        // only do the parametric SD
        dispersion ( nc )   = NonNull ( sqrt ( statcluster.Sum2 ()  / NonNull ( statcluster.GetNumItems () - 1 ) ) );

    }

}


//----------------------------------------------------------------------------
void    TMicroStates::RejectBadEpochs   (    
                                        int             filei, 
                                        long            fromtf,         long            totf,
                                        const char*     eegfile,
                                        SkippingEpochsType  badepochs,  const char*     listbadepochs,  double      badepochstolerance,
                                        TLabeling&      labels 
                                        )
{
if ( StringIsEmpty ( eegfile ) || badepochs == NoSkippingBadEpochs )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TMarkers            rejectmarkers;


if      ( badepochs == SkippingBadEpochsAuto )
                                        // Search for bad epochs
    rejectmarkers.BadEpochsToMarkers ( 0, eegfile, badepochstolerance, MarkerNameAutoBadEpoch );


else if ( badepochs == SkippingBadEpochsList ) {
                                        // open EEG, letting Cartool figure out which marker file is the right one, then copy it!
    TOpenDoc< TTracksDoc >  EEGDoc ( eegfile, OpenDocHidden );


    TMarkers            eegmarkers ( *EEGDoc );

                                        // transfer & filter at the same time
    rejectmarkers.InsertMarkers ( eegmarkers, listbadepochs );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//long                tfmin;
//long                tfmax;
//TimeRangeToDataRange ( filei, fromtf, totf, tfmin, tfmax );
//LabelType           l;

long                fromclipped;
long                toclipped;

long                tfabstorel      = OffsetTF[ filei ];


                                        // loop through the reject list, and clear the labeling if appropriate
for ( int i = 0; i < (int) rejectmarkers; i++ ) {

    if ( rejectmarkers[ i ]->IsNotOverlappingInterval ( fromtf, totf ) )
        continue;                       // no overlap

                                        // within absolute current file limits
    fromclipped     = AtLeast ( fromtf, rejectmarkers[ i ]->From );
    toclipped       = NoMore  ( totf,   rejectmarkers[ i ]->To   );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Deal first with the special cases of overlapping segments at both edges (this because we need the labels info at the internal side of the edge)
                                        // Off for the moment, as it tends to overkill big segments which happen to just touch the deletion markers...
/*
                                        // get left labels
    l   = labels[ fromclipped ];

    if ( l != UndefinedLabel )
                                        // delete any overlapping segment on the left
        for ( long tfl = tfabstorel + fromclipped - 1; tfl >= tfmin; tfl-- )

            if ( labels[ tfl ] == l )
                labels.Reset ( tfl );    // continue deleting same segment
            else
                break;                  // we're done with left segment


                                        // get right labels
    l   = labels[ toclipped ];

    if ( l != UndefinedLabel )

        for ( long tfl = tfabstorel + toclipped + 1; tfl <= tfmax; tfl++ )

            if ( labels[ tfl ] == l )
                labels.Reset ( tfl );    // continue deleting same segment
            else
                break;                  // we're done with left segment
*/

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // erase from parts of markers inside the given epoch
    for ( long tf = fromclipped; tf <= toclipped; tf++ )
                                        // labels can be a concatenation of multiple conditions, so there is a shift per file
        labels.Reset ( tfabstorel + tf );

    }

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
