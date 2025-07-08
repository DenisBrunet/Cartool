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

#include    "ComputeCentroidFiles.h"

#include    "Strings.Utils.h"
#include    "Files.TGoF.h"
#include    "TFilters.h"
#include    "CartoolTypes.h"            // AtomType PolarityType CentroidType

#include    "TMaps.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Processing used for centroids / templates
void    ProcessResults  (   TMaps&      data,           AtomType        datatype,       ReferenceType   reference,
                            bool        ranking, 
                            bool        thresholding,   double          threshold, 
                            bool        normalize 
                        )
{
if ( data.IsNotAllocated () )
    return;
                                        // used for merging ESI templates with different distributions, which impacts the min-max
                                        // handles both vectorial and scalar data
                                        // MUST account for all data
if ( ranking )              		data.ToRank  ( datatype, RankingOptions ( RankingAccountNulls | RankingCountIdenticals ), -1 );
                                        // handling both vectorial and scalar data
if ( thresholding )         		data.Thresholding ( threshold, datatype );

if ( reference != ReferenceNone )	data.SetReference ( reference, datatype );

if ( normalize )            		data.Normalize ( datatype );
};


void    ProcessResults  (   TGoF&       gof,            AtomType        datatype,       ReferenceType   reference,
                            bool        ranking, 
                            bool        thresholding,   double          threshold, 
                            bool        normalize 
                        )
{
if ( gof.IsEmpty () )
    return;

TMaps               data;

for ( int fi = 0; fi < (int) gof; fi++ ) {

    data.ReadFile  ( gof[ fi ], 0, datatype, ReferenceAsInFile );

    ProcessResults  (   data,           datatype,   reference,
                        ranking, 
                        thresholding,   threshold, 
                        normalize
                    );
                                        // !overwriting file!
    data.WriteFile ( gof[ fi ], IsVector ( datatype )  );
        
                                        // ..or saving to another file?
//  TFileName           newfile         = gof[ fi ];
//  if ( ranking )      PostfixFilename ( newfile, ".Rank" );
//  if ( thresholding ) PostfixFilename ( newfile, ".Clip" );
//  if ( normalize )    PostfixFilename ( newfile, ".Norm" );
//  data.WriteFile ( newfile, IsVector ( datatype )  );
    }
};


//----------------------------------------------------------------------------
                                        // Computing + returning centroid maps
void    ComputeCentroidFiles    (   const TGoF&         gof,                ComputeCentroidEnum     layout,
                                    CentroidType        centroidflag,
                                    AtomType            datatype,
                                    PolarityType        polarity,
                                    ReferenceType       processingref,
//                                  bool                toabszscore,
                                    bool                ranking,
                                    bool                thresholding,       double                  threshold,
                                    bool                normalize,
                                    SpatialFilterType   spatialfilter,      const char*             xyzfile,
                                    TMaps&              mapscentroid,
                                    bool                showprogress
                                )
{
mapscentroid.DeallocateMemory ();

if ( gof.IsEmpty () )
    return;
    

if ( spatialfilter != SpatialFilterNone && StringIsEmpty ( xyzfile ) )
    return;

                                        // "vectorial" ranking is allowed
//if ( ranking && ! ( IsScalar ( datatype ) || IsPositive ( datatype ) ) )
//    ranking     = false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;

if ( showprogress ) {
    Gauge.Set       ( CentroidNames[ centroidflag ] /*CentroidTitle*/ );
    Gauge.AddPart   ( 0, (int) gof );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do some checking + retrieve some info
TMaps               mapsin;


for ( int i = 0; i < (int) gof; i++ ) {

    if ( Gauge.IsAlive () )     Gauge.Next ( 0 );


    if ( layout == AllFilesOneCentroid )        mapsin.ReadFiles ( gof,         datatype, ReferenceNone );   // reading & concatenating all files at once
    else /*OneFileOneCentroid*/                 mapsin.ReadFile  ( gof[ i ], 0, datatype, ReferenceNone );   // reading a single file


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Spatial Filter is better without average reference, as the latter can spoil the whole data
    if ( spatialfilter != SpatialFilterNone )   mapsin.FilterSpatial ( SpatialFilterDefault, xyzfile );


//  if ( toabszscore ) {
//  //  mapsin.ZPositiveToZSigned     ();
//      mapsin.ZPositiveToZSignedAuto ();
//
//      if ( polarity == PolarityEvaluate ) // spontaneous data?
//          mapsin.Absolute ();
//      }

    ProcessResults  (   mapsin,         datatype,   processingref,
                        ranking, 
                        thresholding,   threshold, 
                        normalize 
                    );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute 1 centroid, add to current structure - note that centroid has not been normalized
    mapscentroid.Add ( mapsin.ComputeCentroid ( centroidflag, datatype, polarity ) );

                                        // done all files at once
    if ( layout == AllFilesOneCentroid )    break;
    } // for i


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Postprocess templates similarly to the intermediate options
ProcessResults  (   mapscentroid,   datatype,   processingref,
                    ranking, 
                    thresholding,   threshold, 
                    normalize 
                );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // For the aesthetic, reorder the polarities
if ( polarity != PolarityDirect )

    mapscentroid.AlignSuccessivePolarities ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( Gauge.IsAlive () )     Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
                                        // Computing + auto saving to file
void    ComputeCentroidFiles    (   const TGoF&         gof,                ComputeCentroidEnum     layout,
                                    CentroidType        centroidflag,
                                    AtomType            datatype,
                                    PolarityType        polarity,
                                    ReferenceType       processingref,
//                                  bool                toabszscore,
                                    bool                ranking,
                                    bool                thresholding,       double                  threshold,
                                    bool                normalize,
                                    SpatialFilterType   spatialfilter,      const char*             xyzfile,
                                    char*               centrfile,
                                    bool                showprogress
                                )
{
if ( gof.IsEmpty () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TMaps               mapscentroid;


ComputeCentroidFiles    (   gof,                layout,
                            centroidflag,
                            datatype,
                            polarity,
                            processingref,
//                          toabszscore,
                            ranking,
                            thresholding,       threshold,
                            normalize,
                            spatialfilter,      xyzfile,
                            mapscentroid,
                            showprogress
                        );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get common part (including full path)
TFileName           commondir;
TFileName           commonstart;
TStrings            diffs;


gof.GetCommonParts    (   commondir,  commonstart,    0,  &diffs  );

StringCleanup   ( commonstart );

                                        // this could happen if files have nothing in common at their beginning
if ( StringIsEmpty ( commonstart ) )

    StringCopy  ( commonstart, DefaultFileName );

                                        // Concatenate diffs
TFileName           alldiffs;

for ( int i = 0;     i < (int) diffs; i++ ) {

    StringAppend ( alldiffs, " ", diffs[ i ] );

    if ( StringLength ( alldiffs ) > NoMore ( WindowsMaxComponentLength, MaxPathShort / 2 ) )
        break;                          // stop the slaughter!
    }

                                        // if too long, try to produce a shorter version!
if ( StringLength ( alldiffs ) > 50 )
                                        // like an interval
    StringCopy ( alldiffs, " ", diffs[ 0 ], " .. ", diffs[ (int) diffs - 1 ] );

                                        // clean-up
StringCleanup   ( alldiffs );
ReplaceChars    ( alldiffs, "()[]", "" );

//DBGM ( alldiffs, "alldiffs" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Final file name
TFileName           filename;

                                        // first part
filename    = commondir + "\\" + commonstart;

if ( *LastChar ( commonstart ) != '.' )     filename   += ".";
if ( IsVector ( datatype ) )                filename   += TFileName ( InfixVectorial ) + " ";

filename   += TFileName ( CentroidNames[ centroidflag ] ) + StringPlural ( (int) gof );

//DBGM ( (char*) filename, "filename begin" );


                                        // then try to add all diffs if not too long...
if ( StringIsNotEmpty ( alldiffs )
  && StringLength ( filename ) + StringLength ( alldiffs ) + 4 <= NoMore ( WindowsMaxComponentLength, MaxPathShort )  )
                                        // we're good here
    StringAppend ( filename, ".", alldiffs );
                                        // else: nothing


filename.AddExtension   ( IsExtensionAmong ( gof[ 0 ], AllRisFilesExt ) ? FILEEXT_RIS : FILEEXT_EEGSEF );

filename.CheckNoOverwrite    ();

//DBGM ( (char*) filename, "filename" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Actual save
mapscentroid.WriteFile ( filename, IsVector ( datatype ) );

                                        // complimentary opening the resulting file
filename.Open ();

                                        // returned to caller
StringCopy  ( centrfile, filename );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
