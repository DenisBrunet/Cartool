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

#include    "GenerateData.h"

#include    "Math.Random.h"
#include    "Files.Utils.h"
#include    "Geometry.TPoints.h"
#include    "Dialogs.TSuperGauge.h"
#include    "TMaps.h"

#include    "TLeadField.h"

#include    "TMicroStates.h"
#include    "TMicroStatesSegDialog.h"   // SegPresetFilesEnum

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Loops through all parameters, calls MapsFromLeadField, then write results to files
void        GenerateData (  GenerateTypeFlag    what,
                            int                 nummapsmin,         int             nummapsmax,
                            double              correlationmin,     double          correlationmax,     double          correlationstep,
                            double              mapnoisemin,        double          mapnoisemax,        double          mapnoisestep,
                            int                 segdurationmin,     int             segdurationmax,
                            GenerateSegmentFlag typesegment,
                            bool                cyclicsegs,
                            bool                ignorepolarity,
                            bool                normalize,
                            TLeadField&         leadfield,          TTracks<float>& K,                  int             numsources,
                            TPoints             solp,
                            int                 numfiles,           int             fileduration,
                            const char*         basefilename,
                            bool                savemaps,           bool            savetemplatemaps,   bool            savetemplateris,
                            bool                runsegmentation,    int             numrandtrials
                            )
{
if ( what == GenerateEeg        && ! leadfield.IsOpen ()
  || what == GenerateRis        && solp.IsEmpty ()
  || what == GenerateUnknown                            )

    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // base directory & file names
TFileName           BaseDir;
TFileName           BaseFileName;
TFileName           fileoutprefix;
TFileName           buff;
TFileName           filename;
TFileName           mapsfile;
TFileName           seedsfile;
TFileName           templaterisfile;
char                ext[ 64 ];


StringCopy      ( BaseDir,          basefilename                            );
                                        // should be enough, or checking the whole path, too?
CreatePath      ( BaseDir,          false );
                                        // getting rid of any pre-existing data which match only and exactly our file names
StringCopy      ( fileoutprefix,    ToFileName ( basefilename ) );
                                        // compose path access and main prefix "full path\prefix\prefix"
StringCopy      ( BaseFileName,     BaseDir,     "\\",   fileoutprefix );


if      ( what == GenerateEeg )     StringCopy  ( ext,              FILEEXT_EEGSEF );
else /*if ( what == GenerateRis )*/ StringCopy  ( ext,              FILEEXT_RIS    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;
Gauge.Set       ( GenerateDataTitle, SuperGaugeLevelInter );

int                 nummaps         =              nummapsmax     - nummapsmin       + 1;
int                 numcorr         = Truncate ( ( correlationmax - correlationmin ) / correlationstep + 1 );
int                 numnoise        = Truncate ( ( mapnoisemax    - mapnoisemin    ) / mapnoisestep    + 1 );

Gauge.AddPart   ( 0, nummaps * numcorr * numnoise * numfiles );

                                        // Go through a dedicated class
int                 dim             = what == GenerateEeg ?   leadfield.GetNumElectrodes ()
                                  : /*what == GenerateRis ?*/ solp.GetNumPoints ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // store a new collection of maps - all sizes are known here
TMaps               seedmaps;
TMaps               seedris;
TMaps               data;

bool                varsegduration  = segdurationmin != segdurationmax;

TRandUniform        randunif;
//TRandCoin         randcoin;

double              powermin        = 1.0;  // don't use a too wide of a range, otherwise SD of global data will be weird
double              powermax        = 3.0;
                                        // constraining map polarity flipping
bool                alternatesign   = what == GenerateEeg && ignorepolarity && typesegment != SeedMaps;
TArray1<double>     mapsign ( nummapsmax );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // segmentation data
TMicroStates            segdlg;
TGoF				    gof1file;
TGoGoF                  gogof;
TFileName               segbasefilename;
TFileName               outputcommondir;
bool                    deletemapfiles  = false; // runsegmentation;

AnalysisType            analysis;
ModalityType            modality;
SamplingTimeType        time;
AtomType                datatype    = AtomTypeScalar;
PolarityType            polarity;
ReferenceType           dataref;

                                        // setting segmentation parameters
if ( runsegmentation ) {

    StringCopy  ( segbasefilename, basefilename, ".seg" );


    if      ( what == GenerateEeg )     analysis    = ignorepolarity ? AnalysisRestingStatesIndiv : AnalysisERP;
    else if ( what == GenerateRis )     analysis    = ignorepolarity ? AnalysisRestingStatesIndiv : AnalysisERP;

    if      ( what == GenerateEeg )     modality    = ModalityEEG;
    else if ( what == GenerateRis )     modality    = ModalityESI;
    else                                modality    = UnknownModality;

                                        time        = SamplingTimeWhole;

    if      ( what == GenerateEeg )     datatype    = AtomTypeScalar;
    else if ( what == GenerateRis )     datatype    = AtomTypePositive;

    if      ( what == GenerateEeg )     polarity    = ignorepolarity ? PolarityEvaluate : PolarityDirect;
    else if ( what == GenerateRis )     polarity    = PolarityDirect;

    if      ( what == GenerateEeg )     dataref     = ReferenceAverage;
    else if ( what == GenerateRis )     dataref     = ReferenceNone;
    } // runsegmentation


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                                        // Outer loop is through the output files, so we get all maps/correlations/noises for each iteration,
                                        // allowing some temporary exploration of the results
for ( int    filei   = 0;                filei   <  numfiles;        filei++ ) {

                                        // Loops through number of maps, correlations and noise levels
    for ( int    nummaps = nummapsmin;       nummaps <= nummapsmax;      nummaps++                ) {

                                        // we might have variable durations?
    int     duration    = typesegment == SeedMaps ? nummaps : fileduration;
                                        // resize - will have no cost for fixed duration
    data.Resize ( duration, dim );


    for ( double corr    = correlationmin;  corr    <= correlationmax;  corr  += correlationstep )
    for ( double noise   = mapnoisemin;     noise   <= mapnoisemax;     noise += mapnoisestep    ) {

        Gauge.Next ( 0 );

                                        // generate filename in advance to update title
        StringCopy      ( filename,     BaseFileName );
        StringAppend    ( filename,     ".", what == GenerateEeg ? "Maps" : "Ris" );
        StringAppend    ( filename,     " ",         IntegerToString ( buff, nummaps, NumIntegerDigits ( nummapsmax ) ) );
//      StringAppend    ( filename,     ".Sources ", IntegerToString ( buff, numsources ) );
        StringAppend    ( filename,     ".Corr ",    IntegerToString ( buff, Round ( corr  * 100 ), 2 ) );
        StringAppend    ( filename,     ".Noise ",   IntegerToString ( buff, Round ( noise * 100 ), 2 ) );
        StringAppend    ( filename,     normalize ? ".Norm" : ".RandPower" );
//      StringAppend    ( filename,     ".File ",    IntegerToString ( buff, filei + 1, 3 ) );
        StringAppend    ( filename,     ".File ",    StringRandom    ( buff, 4 ) );     // use a random name instead of numbering, so we can launch many maps/correlation/noise generation in parallel

        StringCopy      ( mapsfile,         filename,                   ".", ext            );
        StringCopy      ( seedsfile,        filename,     ".", "Seeds", ".", ext            );
        StringCopy      ( templaterisfile,  filename,                   ".", FILEEXT_RIS    );


        CartoolObjects.CartoolApplication->SetMainTitle        ( GenerateDataTitle, filename, Gauge );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Some thinking about ignorepolarity, which applies differently to root maps and data maps:
                                        // If we have  ignorepolarity, we want only positively correlated maps. We can later randomly flip the data to simulate alternating polarities
                                        // If we have !ignorepolarity, we can have either positively or negatively correlated maps, as for ERPs. Concatenation doesn't need flipping polarities, then.
                                        // HOWEVER, currently using only positively correlated maps even for "ERP" style evaluation.
                                        // Using negative correlation bias the distances between clusters and hence the computation of criteria.
                                        // This is not somethging we want when specifying a correlation!
                                        // Maybe to be more realistic for ERP case would be to randomly generate correlation within a RANGE [-corr..corr]

        if      ( what == GenerateEeg ) {
                                        // Returned maps are always centered and normalized
            if ( ! seedmaps.MapsFromLeadField ( nummaps, 
                                                corr, corr,
                                                false, // ! ignorepolarity,
                                                leadfield, K, numsources, 
                                                savetemplateris ? &seedris : 0 ) )
                return;
            }

        else if ( what == GenerateRis ) {

            if ( ! seedmaps.RisFromSolutionPoints ( nummaps, 
                                                    corr, corr,
                                                    solp,  numsources,
                                                    30, 0.20 ) )
                return;
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // setting time range of maps
        data.Reset ();


        if ( typesegment == SeedMaps ) {
                                        // just copy the root maps
            for ( int tfseg0 = 0; tfseg0 < duration; tfseg0++ )

                data[ tfseg0 ]   = seedmaps[ tfseg0 ];

            } // RootMaps

        else {
                                        // generate segments
            int                 mi          = -1;
            int                 segduration = 0;
            int                 border      = typesegment == LeakySegments ? ( segdurationmin + segdurationmax ) / 2: 0;    // as segments "leak", we need some before and after segments for proper look
            mapsign     = +1;           // default signs

                                                   // don't freak out, segduration is going to be (randomly) set before the end of the loop
            for ( int tf0 = -border; tf0 < duration + border; tf0 += segduration ) {

                //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // pick the next map
                if ( cyclicsegs || nummaps < 3 )
                                        // cycle through all maps, one at a time - forced if not enough maps for random generation (at least 3)
                    mi      = ++mi % nummaps;

                else {                  // randomize all maps appearance
                    int     newmi;
                                        // make sure we get a totally NEW random map
                    do newmi = randunif ( (UINT) nummaps ); while ( newmi == mi );

                    mi  = newmi;
                    }

                //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // could be constant if interval length is 0
                segduration = varsegduration ? randunif ( (UINT) segdurationmin, (UINT) segdurationmax )
                                             : segdurationmin;

//                if ( IsOdd ( mi ) ) continue; // !!!!! skipping 1 segment on 2 to see the weighting factors !!!!!

                //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // whole segment's power
                double              maxvalue    = normalize ?   1   : seedmaps[ mi ].GetAbsMaxValue ();
                double              power       = normalize ?   1   : randunif ( powermin, powermax ) / maxvalue;
//              double              sign        = FalseToPlus ( alternatesign && randcoin () ); // no randomize -> always +1; random sign otherwise
                double              sign        = mapsign[ mi ];


                if ( alternatesign )
                                        // alternating sign on each given segment's occurence, f.ex.: A B C -A -B -C A B C etc... or: A B C -A -C A C -B -A B -C etc...
                    mapsign[ mi ]  *= -1;


                //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                if      ( typesegment == ConstantSegments ) {
                                        // add new constant segment
                    for ( int tfseg0 = 0, tfseg = tf0; tfseg0 < segduration && tfseg < duration; tfseg0++, tfseg++ )
                                        // constant segment - skip power here?
                        data[ tfseg ]   = seedmaps[ mi ] * power * sign;
                    } // ConstantSegments


                else if ( typesegment == HanningSegments ) {
                                        // increasing then decreasing the same map
                    for ( int tfseg0 = 0, tfseg = tf0; tfseg0 < segduration && tfseg < duration; tfseg0++, tfseg++ ) {
                                        // increase then decrease, à la Hanning
                                        // there is no overlap between the maps
                                        // points are carefully chosen to never reach actual 0, and to be symmetrical
                        double  w   = Hanning ( ( tfseg0 + 0.5 ) / (double) segduration );

                        data[ tfseg ]   = seedmaps[ mi ] * ( w * power ) * sign;
                        }
                    } // HanningSegments


                else if ( typesegment == LeakySegments ) {
                                        // leaking current segment on each side, proportionally to the segment duration
                    int     overlap     = segduration / 2;

                                        // Overlapping Hannning - quite good with segments of different durations
                    for ( int tfseg0 = - overlap, tfseg = tf0 - overlap; tfseg0 < segduration + overlap; tfseg0++, tfseg++ )
                    
                        if ( IsInsideLimits ( tfseg, 0 , duration - 1 ) ) {

                            double  w   = Hanning ( ( tfseg0 + overlap + 0.5 ) / (double) ( segduration + 2 * overlap ) );

                            data[ tfseg ]  += seedmaps[ mi ] * ( w * power ) * sign;
                            }

/*                                        // Overlapping Sines - Very good for segments of constant durations
                    for ( int tfseg0 = - overlap, tfseg = tf0 - overlap; tfseg0 < segduration + overlap; tfseg0++, tfseg++ )
                    
                        if ( IsInsideLimits ( tfseg, 0 , duration - 1 ) ) {

                            double  w   = sin ( ( tfseg0 + overlap + 0.5 ) / (double) ( segduration + 2 * overlap ) * Pi );

                            data[ tfseg ]  += seedmaps[ mi ] * ( w * power ) * sign;
                            }
*/
                    } // LeakySegments

                } // for tf0

            } // ! RootMaps


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Noise
        double              sigmadata;

        if ( normalize )
                                        // exact SD: we have random data from normalized vectors
            sigmadata   = 1 / sqrt ( (double) dim );

        else {
                                        // Computing the real SD
                                        // It seems more reasonable to have a single SD for all data(?)
//            TEasyStats          stat;
//
//            for ( int mi = 0; mi < nummaps; mi++ )
//            for ( int j  = 0; j  < dim;     j++  )
//                stat.Add ( seedmaps[ mi ][ j ] );
//            sigmadata       = stat.SD ();
//            sigmadata       = sqrt ( stat.Sum2 ()  / stat.GetNumItems () ) / sqrt ( (double) dim );

                                        // estimated mean power
                                        // 3/2xPi is integral of sine on a sphere, so it gives a rough value of the norm of map
            sigmadata   = sqrt ( ( Square ( powermin * 1.5 * Pi ) + Square ( powermax * 1.5 * Pi ) ) / 2 / (double) dim );
            }

                                        // add noise on the final data
        data.AddGaussianNoise ( sigmadata, noise );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // noise can get the center & normalization a little off, so recompute it all
        if ( normalize )

            data.Normalize ( datatype, -1, what == GenerateEeg );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // flip data maps?
        if ( alternatesign )

            for ( int tfseg0 = 0; tfseg0 < duration; tfseg0++ )

//              if ( randcoin () )
                if ( IsOdd ( tfseg0 ) ) // flipping polarity on each time frame helps visually reminding date ignore polarities

                    data[ tfseg0 ].Invert ();
*/

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // saving maps
        if ( savemaps )             data.WriteFile      ( mapsfile );
                                        // saving root maps
        if ( savetemplatemaps )     seedmaps.WriteFile  ( seedsfile );
                                        // saving the root maps' sources only
        if ( savetemplateris )      seedris.WriteFile   ( templaterisfile, true );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( runsegmentation ) {
                                        // current file clustering - includes the "File ####" part
            StringCopy      ( segbasefilename, basefilename, ".Seg.", ToFileName ( filename ) );

                                        // common dir per #clusters / corr / noise
            StringCopy      ( outputcommondir, segbasefilename );
                                        // !remove the file number to have a common directory!
            RemoveExtension ( outputcommondir );

            StringAppend    ( outputcommondir, ".", InfixBestClustering );

                                        // file itself
            AddExtension    ( filename,     ext );

                                        // 1 file in this group
            gof1file.SetOnly ( filename );
                                        // copy group and add to list of groups
            gogof.Reset ();

            gogof.Add ( &gof1file, true, MaxPathShort );


            segdlg.Segmentation (   gogof[ 0 ], 
                                    gogof[ 0 ],
                                    NoDualData,         0,
                                    analysis,           modality,       time,

                                    EpochWholeTime,
                                    NoSkippingBadEpochs,0,
                                    NoGfpPeaksDetection,0,
                                    NoTimeResampling,   0,              0,
                                    SpatialFilterNone,  0,

                                    datatype,           polarity,       dataref,// same as the real clustering
                                    ClusteringTAAHC /*ClusteringKMeans*/,       // T-AAHC is NOT random, and should be faster(?)
                                    nummapsmin,         nummapsmax,             // important
                                    numrandtrials,      MeanCentroid,           // important
                                    false,              0.50,                   // cloud of maps should be consistent at that point

                                    false,
                                    false,              0.95,
                                    false,              0,              0,
                                    false,              0,
                                    MapOrderingContextual,  0,

                                    (MicroStatesOutFlags) ( WriteTemplatesFiles | CommonDirectory | DeleteIndivDirectories ),
                                    segbasefilename,    outputcommondir,
                                    false
                                );

                                        // clean-up after us, we don't actually need the files
            if ( deletemapfiles ) {
                DeleteFileExtended  ( filename );
                RemoveExtension     ( filename );
                }
            } // if runsegmentation

        } // for corr, noise
        } // for nummaps

    } // for filei


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // also get rid of our base directory - Don't, if results are generated in parallel with the same output directory
//if ( deletemapfiles )
//    NukeDirectory ( BaseDir );

CartoolObjects.CartoolApplication->WindowMaximize ();

CartoolObjects.CartoolApplication->SetMainTitle    ( GenerateDataTitle, BaseFileName, Gauge );

Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
