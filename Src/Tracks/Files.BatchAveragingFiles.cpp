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

#include    "Files.BatchAveragingFiles.h"

#include    "Math.Stats.h"
#include    "Strings.Utils.h"
#include    "Files.TGoF.h"
#include    "Dialogs.TSuperGauge.h"
#include    "TTracks.h"

#include    "TExportTracks.h"
#include    "TMaps.h"

#include    "TMicroStates.h"
#include    "TRisDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // !Doesn't test for files consistencies, this should be done by the caller!
void    BatchAveragingScalar    (   const TGoF& gof,
                                    char*       meanfile,       char*       sdfile,         char*       snrfile,
                                    char*       medianfile,     char*       madfile,
                                    bool        openresults,    bool        showgauge 
                                )
{
ClearString ( meanfile   );
ClearString ( sdfile     );
ClearString ( snrfile    );
ClearString ( medianfile );
ClearString ( madfile    );

if ( gof.IsEmpty () )
    return;

                                        // !Type(s) of output is controlled by the pointers being null or not!
if ( ! ( meanfile || sdfile || snrfile || medianfile || madfile ) )
    return;
                                        
                                        // This can be improved to allow computing and saving only SD f.ex.
if ( sdfile  && !   meanfile                // sd needs mean
  || snrfile && ! ( meanfile && sdfile )    // snr needs mean and sd
    )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           commondir;
TFileName           commonstart;
TFileName           commonend;


gof.GetCommonParts  (   commondir,  commonstart,    commonend,  0   );

                                        // this could happen if files have nothing in common at their beginning
if ( StringIsEmpty ( commonstart ) )

    StringCopy  ( commonstart, DefaultFileName );



TFileName           filemean;
TFileName           filesd;
TFileName           filesnr;
TFileName           filemedian;
TFileName           filemad;
TFileName           fileext;


StringCopy          ( fileext,      IsExtensionAmong ( gof[ 0 ], AllRisFilesExt ) ? FILEEXT_RIS : FILEEXT_EEGSEF );

StringCopy          ( filemean,     commondir, "\\", commonstart, commonend, "." InfixMean   " ", IntegerToString ( (int) gof ), ".", fileext );
StringCopy          ( filesd,       commondir, "\\", commonstart, commonend, "." InfixSD     " ", IntegerToString ( (int) gof ), ".", fileext );
StringCopy          ( filesnr,      commondir, "\\", commonstart, commonend, "." InfixSNR    " ", IntegerToString ( (int) gof ), ".", fileext );
StringCopy          ( filemedian,   commondir, "\\", commonstart, commonend, "." InfixMedian " ", IntegerToString ( (int) gof ), ".", fileext );
StringCopy          ( filemad,      commondir, "\\", commonstart, commonend, "." InfixMad    " ", IntegerToString ( (int) gof ), ".", fileext );


CheckNoOverwrite    ( filemean   );
CheckNoOverwrite    ( filesd     );
CheckNoOverwrite    ( filesnr    );
CheckNoOverwrite    ( filemedian );
CheckNoOverwrite    ( filemad    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;

if ( showgauge ) {
    Gauge.Set           ( BatchAveragingTitle );
    Gauge.AddPart       ( 0, (int) gof );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TOpenDoc<TTracksDoc>    eegdoc;

if ( ! eegdoc.Open ( gof[ 0 ], OpenDocHidden ) )
    return;

int                 numtracks           = eegdoc->GetNumElectrodes ();
int                 numtf               = eegdoc->GetNumTimeFrames ();
int                 lineardim           = numtracks * numtf;

                                        
bool                nonrobust       = meanfile   || sdfile || snrfile;
bool                robust          = medianfile || madfile;
TTracks<float>      eegbuff;
TTracks<float>      sum;
TTracks<float>      sum2;
TTracks<float>      snr;
TGoEasyStats        stat;


                    eegbuff .Resize ( numtracks, numtf );
                                        // there is some potential duplicate allocations, but it is kepts as is for the sake of easier computation
if ( nonrobust ) {
                    sum     .Resize ( numtracks, numtf );
    if ( sdfile )   sum2    .Resize ( numtracks, numtf );
    if ( snrfile )  snr     .Resize ( numtracks, numtf );
    }

if ( robust )                           // this will be a lot...
                    stat    .Resize ( lineardim, (int) gof );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // using a TExportTracks object offers more options, as of right now
TExportTracks       expfile;

expfile.NumTracks           =  eegdoc->GetNumElectrodes     ();
expfile.NumAuxTracks        =  eegdoc->GetNumAuxElectrodes  ();
expfile.NumTime             =  eegdoc->GetNumTimeFrames     ();
expfile.SamplingFrequency   =  eegdoc->GetSamplingFrequency ();
expfile.DateTime            =  eegdoc->DateTime;
expfile.AuxTracks           =  eegdoc->GetAuxTracks ();
expfile.ElectrodesNames     = *eegdoc->GetElectrodesNames ();
expfile.SetAtomType ( AtomTypeScalar );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int eegi = 0; eegi < (int) gof; eegi++ ) {

    if ( Gauge.IsAlive () )
        Gauge.Next ( 0 );


    if ( ! eegdoc.Open ( gof[ eegi ], OpenDocHidden ) )
        continue;

        
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // try to recover the sampling frequency on each file
    if ( expfile.SamplingFrequency == 0 && eegdoc->GetSamplingFrequency () > 0 )

        expfile.SamplingFrequency   = eegdoc->GetSamplingFrequency ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get whole data set
    eegdoc->ReadRawTracks ( 0, numtf - 1, eegbuff );


    if ( robust )

        OmpParallelFor

        for ( int e  = 0; e  < numtracks; e++  )
        for ( int tf = 0; tf < numtf;     tf++ )

            stat[ e * numtf + tf ].Add ( eegbuff ( e, tf ) );


    if ( nonrobust ) {

        OmpParallelFor

        for ( int i = 0; i < lineardim; i++ ) {

            sum.GetValue ( i )     += eegbuff.GetValue ( i );

            if ( sdfile )
                sum2.GetValue ( i )    += Square ( eegbuff.GetValue ( i ) );
            }
        }


    eegdoc.Close ();
    } // for eegi


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( nonrobust ) {

    int                 numfiles        = (int) gof;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute SD
    if ( sdfile ) {

        OmpParallelFor

        for ( int i = 0; i < lineardim; i++ )

            sum2.GetValue ( i )     = sqrt ( fabsl ( ( sum2.GetValue ( i ) - Square ( sum.GetValue ( i ) ) / numfiles ) 
                                                     / NonNull ( numfiles - 1 )                                         ) );
        }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute Mean
    sum    /= numfiles;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute SNR, as Mean / SD (not the variance formula)
    if ( snrfile ) {

        OmpParallelFor

        for ( int i = 0; i < lineardim; i++ )
                                   // sqrt to "rescale" to the original data distribution
            snr .GetValue ( i )     = sqrt ( fabs ( sum.GetValue ( i ) ) / NonNull ( sum2.GetValue ( i ) ) );
        }

    } // if nonrobust


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Save Mean
if ( meanfile ) {

    StringCopy      ( expfile.Filename, filemean );

                                        // !If file exists and user does update the output file, then all the subsequent files will also be updated!
    if ( CanOpenFile ( expfile.Filename, CanOpenFileWriteAndAsk ) ) {

        expfile.Write   ( sum, Transposed );

        expfile.End ();


        StringCopy ( meanfile, expfile.Filename );
                                            // complimentary opening the resulting file
        if ( openresults )
            expfile.Filename.Open ();
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Save Median
if ( medianfile ) {

    StringCopy      ( expfile.Filename, filemedian );


    if ( CanOpenFile ( expfile.Filename, CanOpenFileWriteAndAsk ) ) {

        for ( int e  = 0; e  < numtracks; e++  )
        for ( int tf = 0; tf < numtf;     tf++ )

            expfile.Write ( stat[ e * numtf + tf ].Median (), tf, e );

        expfile.End ();


        StringCopy ( medianfile, expfile.Filename );
                                            // complimentary opening the resulting file
        if ( openresults )
            expfile.Filename.Open ();
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // save SD
                                        // For RIS: the SDm of the Mean   is SDm = SD / sqrt ( #files ) - the resulting Mean is therefore not N(1,1/3) but N(1,1/(3*sqrt(#))
                                        //          the SDm of the Median is SDm = SD * sqrt ( Pi / ( 2 ( #files + 2 ) ) )
if ( sdfile ) {

    StringCopy      ( expfile.Filename, filesd );


    if ( CanOpenFile ( expfile.Filename, CanOpenFileWriteAndAsk ) ) {

        expfile.Write   ( sum2, Transposed );

        expfile.End ();


        StringCopy ( sdfile, expfile.Filename );
                                        // complimentary opening the resulting file(?)
        if ( openresults && ! meanfile )
              expfile.Filename.Open ();
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // save SNR
if ( snrfile ) {

    StringCopy      ( expfile.Filename, filesnr );


    if ( CanOpenFile ( expfile.Filename, CanOpenFileWriteAndAsk ) ) {

        expfile.Write   ( snr, Transposed );

        expfile.End ();


        StringCopy ( snrfile, expfile.Filename );
                                        // complimentary opening the resulting file(?)
        if ( openresults && ! meanfile )
              expfile.Filename.Open ();
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // save MAD
if ( madfile ) {

    StringCopy      ( expfile.Filename, filemad );


    if ( CanOpenFile ( expfile.Filename, CanOpenFileWriteAndAsk ) ) {

        for ( int e  = 0; e  < numtracks; e++  )
        for ( int tf = 0; tf < numtf;     tf++ )
                                               // !destroys data!
            expfile.Write ( stat[ e * numtf + tf ].MAD ( CanAlterData ), tf, e );

        expfile.End ();


        StringCopy ( madfile, expfile.Filename );
                                        // complimentary opening the resulting file(?)
        if ( openresults && ! medianfile )
            expfile.Filename.Open ();
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//if ( Gauge.IsAlive () )
//    Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
                                        // Dedicated average for clustering criteria:
                                        // It does NOT average the criteria from individual trials
                                        // but produces (normalized) histograms of the arg max'es of each criterion
void    BatchAveragingErrorData (   const TGoF& gof, 
                                    char*       meanfile,
                                    bool        openresults,    bool        showgauge 
                                )
{
ClearString ( meanfile   );

if ( gof.IsEmpty () )
    return;

if ( ! ( meanfile ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;

if ( showgauge ) {
    Gauge.Set           ( "Error.Data Averaging" );
    Gauge.AddPart       ( 0, 3 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // reading everything at once, there shouldn't be that much
if ( Gauge.IsAlive () )
    Gauge.Next ( 0 );


TStrings            tracksnames;
TGoMaps             maps ( &gof, AtomTypeScalar, ReferenceAsInFile, &tracksnames );
                                        // until all strings are in TStrings, convert them
TStrings            critnames ( tracksnames );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           commondir;
TFileName           commonstart;
TFileName           commonend;


gof.GetCommonParts  (   commondir,  commonstart,    commonend,  0   );

                                        // this could happen if files have nothing in common at their beginning
if ( StringIsEmpty ( commonstart ) )

    StringCopy  ( commonstart, DefaultFileName );


TFileName           filemedianed;
//TFileName           filemediansef;
                                        // removing the .error part
RemoveExtension ( commonend );


StringCopy      ( filemedianed,     commondir, "\\", commonstart, commonend, "." InfixMean   " ", IntegerToString ( (int) gof ), ".", FILEEXT_ERRORDATA );
//StringCopy    ( filemediansef,    commondir, "\\", commonstart, commonend, "." InfixMean   " ", IntegerToString ( (int) gof ), ".", FILEEXT_EEGSEF    );


CheckNoOverwrite ( filemedianed  );
//CheckNoOverwrite ( filemediansef );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numfiles        = gof.NumFiles ();
int                 numtracks       = maps.GetDimension ();
int                 numtf           = maps.GetNumMaps   ();


TSelection          critselavg      ( numtracks, OrderSorted );     // tracks to be averaged
TSelection          critselargmax   ( numtracks, OrderSorted );     // criteria from which to pick only the arg max'es
TSelection          critselmeancrit ( numtracks, OrderSorted );     // criteria to be used for the average mean criterion
TSelection          critselmetacrit ( numtracks, OrderSorted );     // criteria to be used for the average meta-criterion
TSelection          critselavgrank  ( numtracks, OrderSorted );     // criteria to be finally ranked
TSelection          critselavgnorm  ( numtracks, OrderSorted );     // criteria to be finally normalized


int                 firstcriti      = 3;                // critnames.GetIndex ( TrackNameGamma              );

//int                 histoargmax     = critnames.GetIndex ( TrackNameArgMaxHisto        ) ? 1 : 0;   // old files had 3 special tracks

int                 ranki           = critnames.GetIndex ( TrackNameMeanRanks          );           // numtracks - 2 - histoargmax;

int                 lastcriti       = ranki - 1;                                                    // numtracks - 3 - histoargmax;  // critnames.GetIndex ( TrackNameMeanRanks          ) - 1;

int                 metacriti       = numtracks - 1;                                                // critnames.GetIndex ( TrackNameMetaCriterion           );


                                        // Tracks to average
critselavg      .Set ( 0,            firstcriti - 1 );

                                        // Tracks to cumulate only arg max'es
critselargmax   .Set ( firstcriti,   lastcriti );

                                        // tracks used for the mean rank
                                        // !not using all tracks - same reason as below for the meta-criterion!
//critselmeancrit .Set ( firstcriti,   lastcriti );
critselmeancrit .Set ( ranki );

                                        // using only the arg max of each individual of meta-criterion
                                        // !cumulating all arg maxes of all individual criteria does not make sense, as they are inter-related at a single clustering level, and the arg of them taken!
                                        // !otherwise, cumulating all of them will bias the average toward the criteria with the highest bias, like the KL, which respond a lot on the first slots. Which will show up in the average!
critselmetacrit .Set ( metacriti );

                                        // Tracks to be finally ranked / normalized
                                        // Ranking equalizes the curve, so it emphasizes small variations, at the cost of losing the original "weights"
//critselavgrank  .Set ( ranki );
//critselavgrank  .Set ( metacriti );

                                        // Normalization retains the original shape of the curve
critselavgnorm  .Set ( firstcriti,   lastcriti );
critselavgnorm  .Set ( ranki );
critselavgnorm  .Set ( metacriti );


                                        // to revert to regular averaging for all tracks, uncomment this:
//critselavg .Set ();
//critselmeancrit.Reset ();
//critselmetacrit .Reset ();
//critselequ.Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Fill stats
if ( Gauge.IsAlive () )
    Gauge.Next ( 0 );


TMaps               errdat ( numtracks, numtf );    // !unusual reversed order: we want to access all data for a given criterion!
TMaps               avg    ( numtf, numtracks );


for ( int eegi = 0; eegi < numfiles; eegi++ ) {

                                        // regular average is straightforward
    avg    += maps[ eegi ];


    for ( int e  = 0; e  < numtracks; e++  )
    for ( int tf = 0; tf < numtf;     tf++ ) {

                                        // looking only for arg maxes (including multiple maxes) - the only thing that has a real significance
        if ( maps[ eegi ]( tf, e ) < 1 - SingleFloatEpsilon )
            continue;

                                        // individually for each criterion
        if ( critselargmax  [ e ] )     errdat ( e,           tf )++;

                                        // for the the mean rank - use ony the individual rank, as it is already consolidated
        if ( critselmeancrit[ e ] )     errdat ( ranki,       tf )++;

                                        // for the average meta-criterion - 
        if ( critselmetacrit[ e ] )     errdat ( metacriti,   tf )++;

        }

    } // for eegi


avg    /= numfiles;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Extracts stats
if ( Gauge.IsAlive () )
    Gauge.Next ( 0 );

                                        // Tracks that are just averaged
for ( int e  = 0, si = 0; e  < numtracks; e++        )
for ( int tf = 0;         tf < numtf;     tf++, si++ ) {

                                        // tracks
    if ( critselavg[ e ] )
                                        // using all data + has some smoothing factor in it
        errdat ( e, tf )    = avg ( tf, e );

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // rank most of the final tracks, except number of clusters and GEV
for ( TIteratorSelectedForward criti ( critselavgrank ); (bool) criti; ++criti )

    errdat[ criti() ].ToRank ( RankingIgnoreNulls );  // rank it like a criterion - also can help seeing subtle differences in secondary peaks


for ( TIteratorSelectedForward criti ( critselavgnorm ); (bool) criti; ++criti )

    errdat[ criti() ].NormalizeMax ();        // we are not dealing with a criterion, but an average, we might be interested to see the shape of it, sp simply setting the max to 1


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Save to files

                                        // doesn't work for .data files
//errdat.WriteFile  ( filemediansef,    false, 0, &tracksnames, 0, ReadGoMapsToNumMaps, ReadGoMapsToDimension );


TExportTracks     expdata;

StringCopy          ( expdata.Filename, filemedianed );

expdata.NumFiles            = 1;
expdata.NumTime             = numtf;
expdata.NumTracks           = numtracks;
expdata.VariableNames       = critnames;

                                        // var is correctly set before reqminclusters, in case reqminclusters is not 1
for ( int ncl = 0; ncl < numtf; ncl++ )
for ( int v = 0; v < numtracks; v++ )
    expdata.Write ( (float) errdat ( v, ncl ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Complimentary adding a marker to the optimal position in error file
double              maxmetacrit         = -1;
int                 argmaxmetacrit;
TMarkers            argmaxmetacritrmk;


for ( int tf = 0;         tf < numtf;     tf++ )

    if ( errdat ( metacriti, tf ) > maxmetacrit ) {

        maxmetacrit     = errdat ( metacriti, tf );
        argmaxmetacrit  = tf;
        }


if ( maxmetacrit > 0 ) {

    argmaxmetacritrmk.AppendMarker ( TMarker ( argmaxmetacrit, argmaxmetacrit, MarkerDefaultCode, InfixBestClustering, MarkerTypeMarker ) );

    AddExtension    ( expdata.Filename, FILEEXT_MRK );

    argmaxmetacritrmk.WriteFile ( expdata.Filename );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // complimentary opening the resulting file
if ( openresults ) {
//  filemediansef.Open ();
    filemedianed .Open ();
    }


if ( meanfile ) {
//  outputfiles->Add ( filemediansef );
    StringCopy  ( meanfile, filemedianed  );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( Gauge.IsAlive () )
    Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
                                        // sphmean and sphsd are spherical stats, done on normalized dipoles, to extract main direction and its deviation / consistency
void    BatchAveragingVectorial (   const TGoF& gof, 
                                    char*       vmeanfile,      char*       nmeanfile,      char*       snrfile,
                                    char*       sphmeanfile,    char*       sphsdfile,      char*       sphsnrfile,
                                    bool        openresults,    bool        showgauge 
                                )
{
ClearString ( vmeanfile     );
ClearString ( nmeanfile     );
ClearString ( snrfile       );
ClearString ( sphmeanfile   );
ClearString ( sphsdfile     );
ClearString ( sphsnrfile    );


if ( gof.IsEmpty () )
    return;

                                        // !Type(s) of output is controlled by the pointers being null or not!
if ( ! ( vmeanfile || nmeanfile || snrfile || sphmeanfile || sphsdfile || sphsnrfile ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TOpenDoc<TRisDoc>   risdoc;

if ( ! risdoc.Open ( gof[ 0 ], OpenDocHidden ) )
    return;

int                 numtracks           = risdoc->GetNumElectrodes ();
int                 numtf               = risdoc->GetNumTimeFrames ();
double              samplingfrequency   = risdoc->GetSamplingFrequency ();

risdoc.Close ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // to keep the code as general as possible
ReferenceType       ref             = ReferenceAsInFile;
AtomType            datatype        = AtomTypeVector;

CheckReference ( ref, datatype );


bool                regstats        = vmeanfile   || nmeanfile || snrfile;
bool                sphstats        = sphmeanfile || sphsdfile || snrfile || sphsnrfile;
TMaps               risbuff;
TMaps               sum;
TMaps               sumn;

                    risbuff .Resize ( numtf, numtracks * ( datatype == AtomTypeVector ? 3 : 1 ) );

if ( regstats )     sum     .Resize ( risbuff.GetNumMaps (), risbuff.GetDimension () );
                                        // for circular stats
if ( sphstats )     sumn    .Resize ( risbuff.GetNumMaps (), risbuff.GetDimension () );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           commondir;
TFileName           commonstart;
TFileName           commonend;


gof.GetCommonParts  (   commondir,  commonstart,    commonend,  0   );

                                        // this could happen if files have nothing in common at their beginning
if ( StringIsEmpty ( commonstart ) )

    StringCopy  ( commonstart, DefaultFileName );


TFileName           filevmean;
TFileName           filenmean;
TFileName           filesnr;
TFileName           filesphmean;
TFileName           filesphsd;
TFileName           filesphsnr;
TFileName           fileext;


StringCopy          ( fileext,      IsExtensionAmong ( gof[ 0 ], AllRisFilesExt ) ? FILEEXT_RIS : FILEEXT_EEGSEF );

StringCopy          ( filevmean,    commondir, "\\", commonstart, commonend, "." InfixVectorial " "           InfixMean " " , IntegerToString ( (int) gof ), ".", fileext );
StringCopy          ( filenmean,    commondir, "\\", commonstart, commonend, "." InfixScalar    " "           InfixMean " " , IntegerToString ( (int) gof ), ".", fileext );    // being more explicit on the scalar part
StringCopy          ( filesnr,      commondir, "\\", commonstart, commonend, "." InfixVectorial " "           InfixSNR  " " , IntegerToString ( (int) gof ), ".", fileext );
StringCopy          ( filesphmean,  commondir, "\\", commonstart, commonend, "." InfixVectorial " Spherical " InfixMean " " , IntegerToString ( (int) gof ), ".", fileext );
StringCopy          ( filesphsd,    commondir, "\\", commonstart, commonend, "." InfixVectorial " Spherical " InfixSD   " " , IntegerToString ( (int) gof ), ".", fileext );
StringCopy          ( filesphsnr,   commondir, "\\", commonstart, commonend, "." InfixVectorial " Spherical " InfixSNR  " " , IntegerToString ( (int) gof ), ".", fileext );


CheckNoOverwrite    ( filevmean     );
CheckNoOverwrite    ( filenmean     );
CheckNoOverwrite    ( filesnr       );
CheckNoOverwrite    ( filesphmean   );
CheckNoOverwrite    ( filesphsd     );
CheckNoOverwrite    ( filesphsnr    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;

if ( showgauge ) {
    Gauge.Set           ( BatchAveragingTitle );
    Gauge.AddPart       ( 0, (int) gof );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int risi = 0; risi < (int) gof; risi++ ) {

    if ( Gauge.IsAlive () )
        Gauge.Next ( 0 );

        
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get whole data set
    risbuff.ReadFile ( gof[ risi ], 0, datatype, ref );

                                        // try to recover the sampling frequency on each file
    if ( samplingfrequency == 0 && risbuff.GetSamplingFrequency () > 0 )

        samplingfrequency   = risbuff.GetSamplingFrequency ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( regstats )

        OmpParallelFor

        for ( int nc = 0; nc < sum.GetNumMaps (); nc++ )

            sum[ nc ]  += risbuff[ nc ];



    if ( sphstats ) {
                                        // we need to normalize each 3D vector
        risbuff.NormalizeSolutionPoints ( datatype );

                                        // summing up the 3D normalized vectors
        OmpParallelFor

        for ( int nc = 0; nc < sum.GetNumMaps (); nc++ )

            sumn[ nc ] += risbuff[ nc ];
        } // if sphstats

    } // for risi


if ( regstats )
    sum    /= (int) gof;

if ( sphstats )
    sumn   /= (int) gof;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // the real vectorial mean, saved as vectorial
if ( vmeanfile ) {

    sum.WriteFile ( filevmean, datatype == AtomTypeVector, samplingfrequency );

                                        // complimentary opening the resulting file
    if ( openresults )
        filevmean.Open ();

    StringCopy ( vmeanfile, filevmean );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // complimentary output: the VECTORIAL mean, but saved as NORM
if ( nmeanfile ) {

    sum.WriteFileScalar ( filenmean, samplingfrequency );

                                        // complimentary opening the resulting file
    if ( openresults )
        filenmean.Open ();

    StringCopy ( nmeanfile, filenmean );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // the spherical mean R, vectorial
if ( sphmeanfile ) {

    sumn.WriteFile ( filesphmean, datatype == AtomTypeVector, samplingfrequency );

                                        // complimentary opening the resulting file
    if ( openresults && ! ( vmeanfile || nmeanfile ) )
        filesphmean.Open ();

    StringCopy ( sphmeanfile, filesphmean );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                dosphsd         = sphsdfile || snrfile || sphsnrfile;

TMaps               sphsd;              // spherical SD


if ( dosphsd ) {
                                        // variance / SD output is scalar
    sphsd.Resize ( numtf, numtracks );


    OmpParallelFor

    for ( int tf = 0; tf  < numtf; tf++ )
    for ( int spi= 0; spi < numtracks; spi++ ) {
                                        // norm of the vectorial sum of spherical vectors
        double      R       = NormVector3 ( &sumn[ tf ][ 3 * spi ] );
//                                      // this is one of the many formula - results in [0..1]
//      sphsd[ tf ][ spi ]  = RtoSphericalSD         ( R );
                                        // this is one of the many formula - results in [0..inf)
        sphsd[ tf ][ spi ]  = RtoSphericalGaussianSD ( R );
        }
    }


if ( sphsdfile ) {

    sphsd.WriteFile ( filesphsd, false, samplingfrequency );

                                        // complimentary opening the resulting file
    if ( openresults && ! ( vmeanfile || nmeanfile ) )
        filesphsd.Open ();

    StringCopy ( sphsdfile, filesphsd );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Vectorial Mean / Spherical SD
if ( snrfile ) {
                                        // SNR output is scalar
    TMaps               snr ( numtf, numtracks );


    OmpParallelFor

    for ( int tf = 0; tf  < numtf; tf++ )
    for ( int spi= 0; spi < numtracks; spi++ ) {

        double      sd      = sphsd[ tf ][ spi ];
                                    // sqrt to "rescale" to the original data distribution
        snr[ tf ][ spi ]    = sd > 0 ? sqrt ( NormVector3 ( &sum[ tf ][ 3 * spi ] ) / sd ) : BigSingleFloat;
        }


    snr.WriteFile ( filesnr, false, samplingfrequency );

                                        // complimentary opening the resulting file
    if ( openresults && ! ( vmeanfile || nmeanfile ) )
        filesnr.Open ();

    StringCopy ( snrfile, filesnr );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Spherical Mean / Spherical SD
if ( sphsnrfile ) {
                                        // SNR output is scalar
    TMaps               snr ( numtf, numtracks );


    OmpParallelFor

    for ( int tf = 0; tf  < numtf; tf++ )
    for ( int spi= 0; spi < numtracks; spi++ ) {

        double      sd      = sphsd[ tf ][ spi ];
                                    // sqrt to "rescale" to the original data distribution
        snr[ tf ][ spi ]    = sd > 0 ? sqrt ( NormVector3 ( &sumn[ tf ][ 3 * spi ] ) / sd ) : BigSingleFloat;
        }


    snr.WriteFile ( filesphsnr, false, samplingfrequency );

                                        // complimentary opening the resulting file
    if ( openresults && ! ( vmeanfile || nmeanfile ) )
        filesphsnr.Open ();

    StringCopy ( sphsnrfile, filesphsnr );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//if ( Gauge.IsAlive () )
//    Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
                                        // This will perform n randomized subsets of size m, vectorially averaged, then the pool of n files is classicaly averaged
void    BatchPoolAveragingVectorial (   const TGoF& gof, 
                                        char*       vmeanfile,      char*       nmeanfile,      char*       snrfile,
                                        char*       sphmeanfile,    char*       sphsdfile,      char*       sphsnrfile,
                                        int         numlocalavg,    int         numrepeatavg,
                                        bool        openresults,    bool        showgauge 
                                    )
{
ClearString ( vmeanfile     );
ClearString ( nmeanfile     );
ClearString ( snrfile       );
ClearString ( sphmeanfile   );
ClearString ( sphsdfile     );
ClearString ( sphsnrfile    );


if ( gof.IsEmpty () )
    return;

                                        // !Type(s) of output is controlled by the pointers being null or not!
if ( ! ( vmeanfile || nmeanfile || snrfile || sphmeanfile || sphsdfile || sphsnrfile ) )
    return;

if ( numlocalavg <= 0 || numrepeatavg <= 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TOpenDoc<TRisDoc>   risdoc;

if ( ! risdoc.Open ( gof[ 0 ], OpenDocHidden ) )
    return;

int                 numtracks           = risdoc->GetNumElectrodes ();
int                 numtf               = risdoc->GetNumTimeFrames ();
double              samplingfrequency   = risdoc->GetSamplingFrequency ();

risdoc.Close ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // to keep the code as general as possible
ReferenceType       ref             = ReferenceAsInFile;
AtomType            datatype        = AtomTypeVector;

CheckReference ( ref, datatype );


TMaps               risbuff;
TMaps               sum;

risbuff .Resize ( numtf, numtracks * ( datatype == AtomTypeVector ? 3 : 1 ) );

sum     .Resize ( risbuff.GetNumMaps (), risbuff.GetDimension () );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           commondir;
TFileName           commonstart;
TFileName           commonend;


gof.GetCommonParts  (   commondir,  commonstart,    commonend,  0   );

                                        // this could happen if files have nothing in common at their beginning
if ( StringIsEmpty ( commonstart ) )

    StringCopy  ( commonstart, DefaultFileName );


TFileName           filevmean;
TFileName           fileext;
TGoF                gofvmean;


StringCopy          ( fileext,      IsExtensionAmong ( gof[ 0 ], AllRisFilesExt ) ? FILEEXT_RIS : FILEEXT_EEGSEF );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;

if ( showgauge ) {
    Gauge.Set           ( BatchAveragingTitle, SuperGaugeLevelInter );
    Gauge.AddPart       ( 0, numrepeatavg * numlocalavg + 2 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numfiles        = (int) gof;
TVector<int>        randindex ( numfiles );
TRandUniform        randunif;


for ( int repi = 0; repi < numrepeatavg; repi++ ) {

    randindex.RandomSeries ( numlocalavg, numfiles, &randunif );
    sum .Reset ();


    for ( int randi = 0; randi < numlocalavg; randi++ ) {

        if ( Gauge.IsAlive () )
            Gauge.Next ( 0 );

                                        // get whole data set
        risbuff.ReadFile ( gof[ randindex[ randi ] ], 0, datatype, ref );

                                        // try to recover the sampling frequency on each file
        if ( samplingfrequency == 0 && risbuff.GetSamplingFrequency () > 0 )

            samplingfrequency   = risbuff.GetSamplingFrequency ();


        sum    += risbuff;
        } // for randi


    sum    /= (int) gof;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    StringCopy          ( filevmean,    commondir, "\\", commonstart, commonend, "." InfixVectorial " Pooled " InfixMean " " , IntegerToString ( repi + 1, NumIntegerDigits ( numrepeatavg ) ), ".", fileext );

    CheckNoOverwrite    ( filevmean     );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    sum.WriteFile ( filevmean, datatype == AtomTypeVector, samplingfrequency );

    gofvmean.Add ( filevmean );

    } // for repi


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( Gauge.IsAlive () )
    Gauge.Next ( 0 );

                                        // Now, doing the requested averages but on the pooled averages
BatchAveragingVectorial (   gofvmean,
                            vmeanfile,      nmeanfile,      snrfile,
                            sphmeanfile,    sphsdfile,      sphsnrfile,
                            openresults,    showgauge 
                        );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( Gauge.IsAlive () )
    Gauge.Next ( 0 );

                                        // We don't need these guys anymore
gofvmean.DeleteFiles ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//if ( Gauge.IsAlive () )
//    Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
void    BatchAveragingFreq      (   const TGoF&             gof,
                                    FrequencyAnalysisType   freqtype,       PolarityType    fftapproxpolarity,
                                    char*                   meanfile,       char*           sdfile, 
                                    bool                    openresults,    bool            showgauge 
                                )
{
int                 numfiles        = (int) gof;
bool                poleval         = IsFreqTypeFFTApproximation ( freqtype ) && fftapproxpolarity == PolarityEvaluate;

ClearString ( meanfile   );
ClearString ( sdfile     );

if ( gof.IsEmpty () )
    return;

if ( sdfile && numfiles <= 1 )
    sdfile  = false;

if ( ! ( meanfile || sdfile ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           commondir;
TFileName           commonstart;
TFileName           commonend;


gof.GetCommonParts  (   commondir,  commonstart,    commonend,  0   );

                                        // this could happen if files have nothing in common at their beginning
if ( StringIsEmpty ( commonstart ) )

    StringCopy  ( commonstart, DefaultFileName );


TFileName           filemean;
TFileName           filesd;


StringCopy          ( filemean,     commondir, "\\", commonstart, commonend, "." InfixMean   " ", IntegerToString ( numfiles ), ".", FILEEXT_FREQ );
StringCopy          ( filesd,       commondir, "\\", commonstart, commonend, "." InfixSD     " ", IntegerToString ( numfiles ), ".", FILEEXT_FREQ );


CheckNoOverwrite    ( filemean );
CheckNoOverwrite    ( filesd   );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;

if ( showgauge ) {
    Gauge.Set           ( BatchAveragingTitle );
    Gauge.AddPart       ( 0, 3 + 3 * numfiles + ( poleval ? 3 : 0 ) + ( meanfile ? 2 : 0 ) + ( sdfile ? 3 : 0 ) );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Retrieve some info from first file
Gauge.Next ( 0 );

TOpenDoc<TFreqDoc>  freqdoc;

if ( ! freqdoc.Open ( gof[ 0 ], OpenDocHidden ) )
    return;

                                        // Init time
int                 numfreqs        = freqdoc->GetNumFrequencies ();
int                 numtracks       = freqdoc->GetNumElectrodes  ();
int                 numtf           = freqdoc->GetNumTimeFrames  ();


TSetArray2<float>   freqbuff    ( numfreqs, numtracks, numtf );
TSetArray2<float>   sum         ( numfreqs, numtracks, numtf );
TSetArray2<float>   sum2        ( sdfile ? numfreqs : 0, sdfile ? numtracks : 0, sdfile ? numtf : 0 );


vector<TGoMaps>     freqmaps;       // yessir, we are going to load all this data in memory
TGoMaps             summaps;
TMap                map;

if ( poleval ) {
    freqmaps.resize ( numfiles );
    summaps .Resize (          numfreqs, numtf, numtracks );
    map     .Resize (                           numtracks );

    for ( int filei = 0; filei < numfiles; filei++ )
        freqmaps[ filei ].Resize  ( numfreqs, numtf, numtracks );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Init export tracks
TExportTracks       expfile;

expfile.SetAtomType ( AtomTypeScalar );

expfile.NumTracks           =  freqdoc->GetNumElectrodes     ();
expfile.NumAuxTracks        =  freqdoc->GetNumAuxElectrodes  ();
expfile.NumTime             =  freqdoc->GetNumTimeFrames     ();
expfile.SamplingFrequency   =  freqdoc->GetOriginalSamplingFrequency ();
expfile.DateTime            =  freqdoc->DateTime;
expfile.AuxTracks           =  freqdoc->GetAuxTracks ();
expfile.ElectrodesNames     = *freqdoc->GetElectrodesNames ();

expfile.NumFrequencies      =  freqdoc->GetNumFrequencies    ();
expfile.BlockFrequency      =  freqdoc->GetSamplingFrequency ();
StringCopy ( expfile.FreqTypeName, freqdoc->GetFreqTypeName () );
expfile.FrequencyNames      = *freqdoc->GetFrequenciesNames ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int filei = 0; filei < numfiles; filei++ ) {

    Gauge.Next ( 0 );

    if ( ! freqdoc.Open ( gof[ filei ], OpenDocHidden ) )
        continue;

        
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // try our chance on every file
    if ( expfile.SamplingFrequency == 0 && freqdoc->GetOriginalSamplingFrequency () > 0 )
        expfile.SamplingFrequency   = freqdoc->GetOriginalSamplingFrequency ();

    if ( expfile.BlockFrequency == 0 && freqdoc->GetSamplingFrequency () > 0 )
        expfile.BlockFrequency      = freqdoc->GetSamplingFrequency ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get the whole bunch of data
    Gauge.Next ( 0 );

    freqdoc->ReadFrequencies ( 0, numtf - 1, freqbuff );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Gauge.Next ( 0 );

    if ( poleval ) {
                                        // load all files in memory as maps
        for ( int fi = 0; fi < numfreqs; fi++ ) {
                                        // shift "TArray2" frequency - NOT thread safe
            freqbuff.SetCurrentPlane ( fi );

            OmpParallelFor

            for ( int tfi = 0; tfi < numtf; tfi++ ) {

                freqmaps[ filei ] ( fi, tfi ).GetColumn ( freqbuff, tfi );

//              freqmaps[ filei ] ( fi, tfi ).AverageReference (); // ?
                }
            } // for freq
        } // if poleval

    else {
                                        // simple case, regular sum
                                        // !Already using OmpParallelFor!
        sum        += freqbuff;

        if ( sdfile )
            sum2       += ( freqbuff *= freqbuff );
        }


    freqdoc.Close ();
    } // for filei


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( poleval ) {

    Gauge.Next ( 0 );
                                        // other files, need to test and sum
//  OmpParallelBegin

    TArray1<TMap*>      allmaps ( numfiles );

//  OmpFor

    for ( int fi  = 0; fi  < numfreqs; fi ++ )
    for ( int tfi = 0; tfi < numtf;    tfi++ ) {
            
        allmaps.ResetMemory ();

                                        // retrieve collection of pointers to maps, across all files - !we don't actually copy maps!
        for ( int filei = 0; filei < numfiles; filei++ )
            allmaps[ filei ]    = &freqmaps[ filei ] ( fi, tfi );
                
                                        // ComputeCentroid is now parallelized, so keeping the outer loop sequential?
                                        // optimal centroid computation                   PolarityEvaluate
        summaps ( fi, tfi ) += ComputeCentroid ( allmaps, MeanCentroid, AtomTypeScalar, fftapproxpolarity );
        }

//  OmpParallelEnd


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Gauge.Next ( 0 );
                                        // realign first TF to previous freq
    for ( int fi  = 1; fi  < numfreqs; fi ++ )

        if ( summaps ( fi, 0 ).IsOppositeDirection ( summaps ( fi - 1, 0 ) ) )

            summaps ( fi, 0 ).Invert ();

                                        // realign next TF to first (previous) TF
    OmpParallelFor

    for ( int fi  = 0; fi  < numfreqs; fi ++ )
    for ( int tfi = 1; tfi < numtf;    tfi++ )

        if ( summaps ( fi, tfi ).IsOppositeDirection ( summaps ( fi, 0 /*tfi - 1*/ ) ) )

            summaps ( fi, tfi ).Invert ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // transfer results
    Gauge.Next ( 0 );

    OmpParallelFor

    for ( int fi  = 0; fi  < numfreqs;  fi ++ )
    for ( int tfi = 0; tfi < numtf;     tfi++ )
    for ( int ei  = 0; ei  < numtracks; ei ++ ) {

        sum ( fi, ei, tfi ) = summaps ( fi, tfi, ei );

        if ( sdfile )   sum2( fi, ei, tfi ) = Square ( summaps ( fi, tfi, ei ) );
        }


    freqmaps.clear ();
    } // if poleval


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute SD - there seems to be some ambiguity on the - operator
if ( sdfile ) {

    Gauge.Next ( 0 );
                                        // do everything in the loop
    OmpParallelFor

    for ( int i = 0; i < sum2.GetTotalLinearDim (); i++ )

        sum2.GetValue ( i )     = sqrt ( abs (   ( sum2.GetValue ( i ) - Square ( sum.GetValue ( i ) ) / numfiles ) 
                                               / ( numfiles - 1 )                                                   ) );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute Mean
Gauge.Next ( 0 );

sum    /= numfiles;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // save a lot of time with some handy buffer
TArray3<float>  results (   expfile.NumTime,                                                        // !already transposed for direct file output!
                            expfile.NumTracks, 
                            expfile.NumFrequencies 
                        * ( expfile.GetAtomType ( AtomTypeUseCurrent ) == AtomTypeComplex ? 2 : 1 ) // multiplex the real / imaginary parts manually
                        );

                                        // Save Mean
if ( meanfile ) {

    Gauge.Next ( 0 );

    OmpParallelFor

    for ( int tfi = 0; tfi < numtf;     tfi++ )
    for ( int fi  = 0; fi  < numfreqs;  fi ++ )
    for ( int ei  = 0; ei  < numtracks; ei ++ )
                                        // copy all data into big buffer
        results ( tfi, ei, fi ) = sum ( fi, ei, tfi );


    Gauge.Next ( 0 );

    StringCopy      ( expfile.Filename, filemean );
                                        // write all data in one shot
    expfile.Write   ( results, NotTransposed );

    expfile.End     ();


    StringCopy ( meanfile, expfile.Filename );
                                        // complimentary opening the resulting file
    if ( openresults )
        expfile.Filename.Open ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Save SD
if ( sdfile ) {

    Gauge.Next ( 0 );

    OmpParallelFor

    for ( int tfi = 0; tfi < numtf;     tfi++ )
    for ( int fi  = 0; fi  < numfreqs;  fi ++ )
    for ( int ei  = 0; ei  < numtracks; ei ++ )
                                        // !sum2 is now holding the actual sd!
        results ( tfi, ei, fi ) = sum2 ( fi, ei, tfi );


    Gauge.Next ( 0 );

    StringCopy      ( expfile.Filename, filesd );
                                        // write all data in one shot
    expfile.Write   ( results, NotTransposed );

    expfile.End     ();


    StringCopy ( sdfile, expfile.Filename );
                                        // complimentary opening the resulting file
    if ( openresults && ! meanfile )
        expfile.Filename.Open ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Finished ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
