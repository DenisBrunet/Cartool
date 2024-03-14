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

#pragma once

#include    "TElectrodesDoc.h"
#include    "TSolutionPointsDoc.h"
#include    "Math.Stats.h"
#include    "Files.TOpenDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
                                        // Non-temporal filter
//----------------------------------------------------------------------------
                                        // Filters are roughly sorted by increasing smoothing effect
enum            SpatialFilterType
                {
                SpatialFilterNone,
                SpatialFilterOutlier,                   // Removing outliers locally only
                SpatialFilterOutliersGaussianMean,      // Detecting outliers globally, then Gaussian smoothing
                SpatialFilterOutliersWeightedMean,      // Detecting outliers globally, then inverse distance smoothing
                SpatialFilterInterseptileGaussianMean,  // Removing outliers and smoothing locally, with Gaussian distance
                SpatialFilterInterseptileWeightedMean,  // Removing outliers and smoothing locally, with inverse distance
                SpatialFilterMedian,
                SpatialFilterInterquartileMean,
                SpatialFilterMinMax,

                NumSpatialFilterTypes,
                SpatialFilterDefault            = SpatialFilterInterseptileWeightedMean,
                };


inline const char*  GetSpatialFilterName ( SpatialFilterType sf )
{
switch ( sf ) {
    case    SpatialFilterNone                       :   return  "No Spatial Filter";
    case    SpatialFilterOutlier                    :   return  "Spatial Outliers Interpolation";
    case    SpatialFilterOutliersGaussianMean       :   return  "Spatial Outliers Gaussian Smoothing";
    case    SpatialFilterOutliersWeightedMean       :   return  "Spatial Outliers Weighted Smoothing";
    case    SpatialFilterInterseptileGaussianMean   :   return  "Spatial Interseptile Gaussian Mean";
    case    SpatialFilterInterseptileWeightedMean   :   return  "Spatial Interseptile Weighted Mean";
    case    SpatialFilterMedian                     :   return  "Spatial Median";
    case    SpatialFilterInterquartileMean          :   return  "Spatial Interquartile Mean";
    case    SpatialFilterMinMax                     :   return  "Spatial Min-Max";
    default                                         :   return  "Unknown Spatial Filter";
    };
}


inline const char*  GetSpatialFilterShortName ( SpatialFilterType sf )
{
switch ( sf ) {
    case    SpatialFilterNone                       :   return  "NoSpatial";
    case    SpatialFilterOutlier                    :   return  "SpatialOutliers";
    case    SpatialFilterOutliersGaussianMean       :   return  "SpatialOutGauss";
    case    SpatialFilterOutliersWeightedMean       :   return  "SpatialOutWeight";
    case    SpatialFilterInterseptileGaussianMean   :   return  "SpatialIGM";
    case    SpatialFilterInterseptileWeightedMean   :   return  "SpatialIWM";
    case    SpatialFilterMedian                     :   return  "SpatialMedian";
    case    SpatialFilterInterquartileMean          :   return  "SpatialIQM";
    case    SpatialFilterMinMax                     :   return  "SpatialMinMax";
    default                                         :   return  "UnknownSpatial";
    };
}


//----------------------------------------------------------------------------
                                        // Predefined paired values for #neighbors and distance (so far)
enum            SpatialFilterRadius
                {
                SFDistance1,            // max normalized distance for up to first ring of neighbors
                SFDistance2,            // max normalized distance for up to second ring of neighbors
                SFDistance3,            // max normalized distance for up to second-third ring of neighbors

                DefaultSFDistance   = SFDistance1,
                };

                                        // made it a function instead of an array to avoid a cpp file
inline int      SpatialFilterMaxNeighbors ( SpatialFilterRadius sfr )
{
switch ( sfr ) {
    case    SFDistance1 : return     6;
    case    SFDistance2 : return    12;
    case    SFDistance3 : return    15;
    default             : return     0;     // compiler whining
    }
}


inline double   SpatialFilterMaxDistance ( SpatialFilterRadius sfr )
{
switch ( sfr ) {
    case    SFDistance1 : return    1.60;
    case    SFDistance2 : return    2.00;
    case    SFDistance3 : return    2.50;
    default             : return    0.00;   // compiler whining
    }
}


//----------------------------------------------------------------------------
                                        // simple enum for sorting distances
enum            NeighborhoodInfo
                {
                NeighborhoodIndex,
                NeighborhoodDistance,

                NumNeighborhoodInfo,

                NeighborhoodValue       = NeighborhoodDistance + 1, // sometimes we need one more bucket
                NumNeighborhoodInfoExt,
                };

                                        // Minimum amount of neighbors for decent functionning
constexpr int   SpatialFilterMinNeighbors   = 2;


//----------------------------------------------------------------------------

template <class TypeD>
class   TFilterSpatial      : public TFilter<TypeD>
{
public:
                    TFilterSpatial ();
                    TFilterSpatial ( SpatialFilterType how, const char* file, int maxnumneigh, double maxdistneigh );   // Now XYZ or SP


    void            Reset   ();
    void            Set     ( SpatialFilterType how, const char* filexyz, int maxnumneigh, double maxnumdist );


    bool            IsAllocated     ()                              const   { return  NeighDist.IsAllocated    (); }
    bool            IsNotAllocated  ()                              const   { return  NeighDist.IsNotAllocated (); }
    bool            IsDimensionOK   ( const TVector<TypeD>& map  )  const   { return  map .GetDim1 ()       == GetNumElectrodes (); }   // exact dimensions
    bool            IsDimensionOK   ( const TArray2<TypeD>& data )  const   { return  data.GetDim1 ()       >= GetNumElectrodes (); }   // more liberal, array can hold pseuo-tracks
    inline  bool    IsDimensionOK   ( const TMaps&          maps )  const;                                                              // exact dimensions

    int             GetNumElectrodes()                              const   { return  NeighDist.GetDim1 (); }


//  void            Apply                               ( TypeD*          data, int numpts ) {}             // to shut-up compiler
    inline  bool    Apply                               ( TVector<TypeD>& map );                            // automatic redirection
    inline  void    Apply                               ( TArray2<TypeD> &data, int numtf, int tfoffset );
    inline  void    Apply                               ( TMaps&          maps, int nummaps = -1 );

//  inline  bool    SpatialFiltering                    ( TArray2<int>& neighindex, TVector<TypeD>& map );
    inline  bool    SpatialFilteringStat                ( TVector<TypeD>& map );
    inline  bool    SpatialFilteringInterpolate         ( TVector<TypeD>& map );
    inline  bool    SpatialFilteringInterpolateSmooth   ( TVector<TypeD>& map );
    inline  bool    SpatialFilteringInterseptile        ( TVector<TypeD>& map );


                        TFilterSpatial      ( const TFilterSpatial& op  );
    TFilterSpatial&     operator    =       ( const TFilterSpatial& op2 );


protected:

    SpatialFilterType   How;
    int                 MaxNumNeigh;
    double              MaxDistNeigh;
    TArray3<double>     NeighDist;
//  TFileName           File;           // ?storing file name could be useful to avoid reloading maybe?

};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------

template <class TypeD>
        TFilterSpatial<TypeD>::TFilterSpatial ()
      : TFilter<TypeD> ()
{
Reset ();
}


template <class TypeD>
        TFilterSpatial<TypeD>::TFilterSpatial ( SpatialFilterType how, const char* file, int maxnumneigh, double maxdistneigh )
{
Set ( how, file, maxnumneigh, maxdistneigh );
}


template <class TypeD>
void    TFilterSpatial<TypeD>::Reset ()
{
How                 = SpatialFilterNone;
MaxNumNeigh         = 0;
MaxDistNeigh        = 0;
NeighDist.DeallocateMemory ();
}


template <class TypeD>
            TFilterSpatial<TypeD>::TFilterSpatial ( const TFilterSpatial& op )
{
How                 = op.How;
MaxNumNeigh         = op.MaxNumNeigh;
MaxDistNeigh        = op.MaxDistNeigh;
NeighDist           = op.NeighDist;
}


template <class TypeD>
TFilterSpatial<TypeD>& TFilterSpatial<TypeD>::operator= ( const TFilterSpatial& op2 )
{
if ( &op2 == this )
    return  *this;


How                 = op2.How;
MaxNumNeigh         = op2.MaxNumNeigh;
MaxDistNeigh        = op2.MaxDistNeigh;
NeighDist           = op2.NeighDist;


return  *this;
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TFilterSpatial<TypeD>::Set ( SpatialFilterType how, const char* file, int maxnumneigh, double maxdistneigh )
{
Reset ();

if ( how == SpatialFilterNone || StringIsEmpty ( file ) )
    return;

                                        // Opening only once, retrieving neighborhood info, then closing
TOpenDoc< TElectrodesDoc     >  XYZDoc ( file, OpenDocHidden );
TOpenDoc< TSolutionPointsDoc >  SPDoc  ( file, OpenDocHidden );

if ( XYZDoc.IsNotOpen () && SPDoc.IsNotOpen () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

How                 = how;
MaxDistNeigh        = maxdistneigh;


if      ( XYZDoc ) {

    MaxNumNeigh         = Clip ( maxnumneigh, SpatialFilterMinNeighbors, XYZDoc->GetNumElectrodes () - 1 ); // MaxNumNeigh < Dim1, or stated otherwise max number of neighbors is (#electrodes - 1)
                                        // sorted array of distance between all electrodes
    XYZDoc->GetNeighborhoodDistances ( NeighDist, MaxNumNeigh );
    }

else if ( SPDoc ) {

    MaxNumNeigh         = Clip ( maxnumneigh, SpatialFilterMinNeighbors, SPDoc->GetNumSolPoints () - 1 );   // MaxNumNeigh < Dim1, or stated otherwise max number of neighbors is (#sp - 1)
                                        // sorted array of distance between all electrodes
    SPDoc->GetNeighborhoodDistances ( NeighDist, MaxNumNeigh );
    }

/*
TArray2<double>         slice1 ( NeighDist.GetDim1 (), NeighDist.GetDim2 () );
TArray2<double>         slice2 ( NeighDist.GetDim1 (), NeighDist.GetDim2 () );
NeighDist.ExtractFromZ ( 0, slice1 );
NeighDist.ExtractFromZ ( 1, slice2 );
TFileName               _file1 ( "E:\\Data\\NeighDist.Index.sef" );
TFileName               _file2 ( "E:\\Data\\NeighDist.Distance.sef" );
CheckNoOverwrite ( _file1 );
CheckNoOverwrite ( _file2 );
slice1.WriteFile ( _file1 );
slice2.WriteFile ( _file2 );
*/
                                        // If it still doesn't work, revoke the spatial filter
//if ( NeighDist.IsNotAllocated () )
//    Reset ();
}


//----------------------------------------------------------------------------
                                        // Global dispatcher to the right function - although the specialized functions are directly callable, too
template <class TypeD>
bool    TFilterSpatial<TypeD>::Apply ( TVector<TypeD>& map )
{
                                        // !Subtle difference here compared to the specialized functions: no filtering returns true here, while the wrong filter type to the specialized functions will return false!
if ( How == SpatialFilterNone )
    return  true;

                                        // !not tested as it is usually deep in nested loops!
//if ( ! IsDimensionOK ( map ) )
//    return  false;


if      ( How == SpatialFilterInterseptileWeightedMean
       || How == SpatialFilterInterseptileGaussianMean  )   return  SpatialFilteringInterseptile        ( map );

else if ( How == SpatialFilterOutlier                   )   return  SpatialFilteringInterpolate         ( map );

else if ( How == SpatialFilterOutliersGaussianMean      )   return  SpatialFilteringInterpolateSmooth   ( map );
else if ( How == SpatialFilterOutliersWeightedMean      )   return  SpatialFilteringInterpolateSmooth   ( map );

else if ( How == SpatialFilterMedian
       || How == SpatialFilterInterquartileMean
       || How == SpatialFilterMinMax                    )   return  SpatialFilteringStat                ( map );


return  true;
}


//----------------------------------------------------------------------------
                                        // Wrapping calls for a big TArray2 variable
template <class TypeD>
void    TFilterSpatial<TypeD>::Apply ( TArray2<TypeD>& data, int numtf, int tfoffset )
{
if ( ! IsDimensionOK ( data )
  || How == SpatialFilterNone )
    return;


int                 numel           = GetNumElectrodes ();
TVector<TypeD>      temp ( numel );


for ( int tf0 = 0, tf = tfoffset; tf0 < numtf; tf0++, tf++ ) {

    for ( int i = 0; i < numel; i++ )
        temp[ i ]    = data ( i, tf );


    Apply ( temp );


    for ( int i = 0; i < numel; i++ )
        data ( i, tf )  = temp[ i ];
    }
}


//----------------------------------------------------------------------------

bool    TFilterSpatial<TMapAtomType>::IsDimensionOK ( const TMaps& maps )   const   
{
return  maps.GetDimension ()  == GetNumElectrodes ();
}   


//----------------------------------------------------------------------------

void    TFilterSpatial<TMapAtomType>::Apply ( TMaps& maps, int nummaps )
{
if ( ! IsDimensionOK ( maps )
  || How == SpatialFilterNone )
    return;

if ( maps.CheckNumMaps ( nummaps ) <= 0 )
    return;


for ( int mi = 0; mi < nummaps; mi++ )

    Apply ( maps[ mi ] );
}


//----------------------------------------------------------------------------
/*                                    // Input: neighbors indexes from Delaunay triangulation
template <class TypeD>
bool    TFilterSpatial<TypeD>::SpatialFiltering ( TArray2<int>& neighindex, TVector<TypeD>& map )
{
if ( How == SpatialFilterNone )
    return  true;

if ( map.IsNotAllocated () || neighindex.IsNotAllocated () || neighindex.GetDim1 () != map.GetDim1 () )
    return  false;

int                 Dim1            = map.GetDim1 ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                statstorage     = How == SpatialFilterMedian 
                                   || How == SpatialFilterInterquartileMean
                                   || How == SpatialFilterMinMax;
                                        // neighindex is sized according to max number of neighbors + 1 (for count of neighbors)
TEasyStats          stat ( statstorage ? neighindex.GetDim2 () - 1 : 0 );

TVector<TypeD>      temp ( map );


for ( int i = 0; i < Dim1; i++ ) {

    stat.Reset ();


    if ( How == SpatialFilterMedian )
        stat.Add ( temp[ i ] );   // add the central part for a Median

                                        // stat of all neighbors, without central value
    for ( int ni = 1; ni <= neighindex ( i, 0 ); ni++ )
        stat.Add ( temp[ neighindex ( i, ni ) ] );

                                        // mean of neighbors replace the center
    if      ( How == SpatialFilterSmooth             )   map[ i ]   = stat.Mean ();
    else if ( How == SpatialFilterMedian             )   map[ i ]   = stat.Median ( false );
    else if ( How == SpatialFilterInterquartileMean  )   map[ i ]   = stat.InterQuartileMean ();
    else if ( How == SpatialFilterMinMax             )   map[ i ]   = stat.MinMax ( stat[ 0 ] );    // !stat not sorted yet, so first element is the central one (distance == 0)!

    } // for Dim1


return  true;
}
*/

//----------------------------------------------------------------------------
                                        // Simpler statistical spatial filter
template <class TypeD>
bool    TFilterSpatial<TypeD>::SpatialFilteringStat ( TVector<TypeD>& map )
{
if ( ! (    How == SpatialFilterMedian
         || How == SpatialFilterInterquartileMean
         || How == SpatialFilterMinMax ) )
    return  false;


//if ( ! IsDimensionOK ( map ) )
//    return  false;

int                 Dim1            = map.GetDim1 ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // + center
TEasyStats          stat ( MaxNumNeigh + 1 /*NeighDist.GetDim2 ()*/ );

TVector<TypeD>      temp ( map );


for ( int i = 0; i < Dim1; i++ ) {


    stat.Reset ();
                                        // stat of all neighbors below a given radius - central value included (older version had center excluded)
                                        // stops when either radius is further, or number of neghbors is reached
    for ( int j = 0; j <= MaxNumNeigh; j++ ) {

        if ( NeighDist ( i, j, NeighborhoodDistance ) > MaxDistNeigh )
            break;                      // distances are sorted, we can stop here


        stat.Add ( temp[ NeighDist ( i, j, NeighborhoodIndex ) ] );
        } // for MaxNumNeigh


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Formula is pretty much straightforward, the drawback being that the new value doesn't incoporate any distance weighting in the formula
    if      ( How == SpatialFilterMedian             )  map[ i ]    = stat.Median ( false );
//  else if ( How == SpatialFilterMean               )  map[ i ]    = stat.Mean ();
    else if ( How == SpatialFilterInterquartileMean  )  map[ i ]    = stat.InterQuartileMean ();
    else if ( How == SpatialFilterMinMax             )  map[ i ]    = stat.MinMax ( stat[ 0 ] );    // !stat not sorted yet, so first element is the central one (distance == 0)!

    } // for Dim1


return  true;
}


//----------------------------------------------------------------------------
                                        // Interpolation of bad electrodes only
template <class TypeD>
bool    TFilterSpatial<TypeD>::SpatialFilteringInterpolate ( TVector<TypeD>& map )
{
if ( How != SpatialFilterOutlier )
    return  false;


//if ( ! IsDimensionOK ( map ) )
//    return  false;

int                 Dim1            = map.GetDim1 ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TEasyStats          stat ( MaxNumNeigh /*NeighDist.GetDim2 ()*/ );
bool                badcenter;
double              delta;
double              reasonmin;
double              reasonmax;
                                        // for distance and value sort
TArray2<double>     neigh ( Dim1, /*NumNeighborhoodInfo*/ NumNeighborhoodInfoExt );


int                 maxneigh;
double              w;
double              sumw;
double              sumv;

TVector<TypeD>      temp ( map );


for ( int i = 0; i < Dim1; i++ ) {


    stat.Reset ();
                                        // stat of all neighbors below a given radius - central value excluded
                                        // stops when either radius is further, or number of neghbors is reached
    for ( int j = 1; j <= MaxNumNeigh; j++ ) {

        if ( NeighDist ( i, j, NeighborhoodDistance ) > MaxDistNeigh )
            break;                      // distances are sorted, we can stop here


        stat.Add ( temp[ NeighDist ( i, j, NeighborhoodIndex ) ] );
        } // for MaxNumNeigh


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Is central point an outlier?
    delta       = stat.Range () * SpatialFilterExtraRange;
    reasonmin   = stat.Min   () - delta;
    reasonmax   = stat.Max   () + delta;

                                        // If any of these 2 tests fail, interpolate the central point
    badcenter   = ! IsInsideLimits ( (double) temp[ i ], reasonmin, reasonmax )         // if central point is not within a reasonable margin from its neighbors
               ||   fabs ( stat.ZScoreRobust ( temp[ i ] ) ) > SpatialFilterZScoreMin;  // or Z-Score is not satisfying - Note that the robust version indeed works better


    if ( ! badcenter )
        continue;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    neigh       = DBL_MAX;              // sorting will be ascending
    maxneigh    = 0;

                                        // stat of all neighbors, including central value, below radius
                                        // stops when either radius is further, or number of neighbors is reached
    for ( int j = 0; j <= MaxNumNeigh; j++ ) {

                                        // !we need to have at least 3 data points to be able to remove 2 outliers, even if they are outside the limit!
        if ( NeighDist ( i, j, NeighborhoodDistance ) > MaxDistNeigh && maxneigh > SpatialFilterMinNeighbors )
            break;                      // distances are sorted, we can stop here

                                        // store values and distances
//      neigh ( maxneigh, NeighborhoodIndex     )   =       NeighDist ( i, j, NeighborhoodIndex    );   // not actually needed

        neigh ( maxneigh, NeighborhoodValue     )   = temp[ NeighDist ( i, j, NeighborhoodIndex    ) ];

        neigh ( maxneigh, NeighborhoodDistance  )   =       NeighDist ( i, j, NeighborhoodDistance );

        maxneigh++;
        } // for MaxNumNeigh


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // sort by value
    neigh.SortRows ( NeighborhoodValue, Ascending, maxneigh );

                                        // exclude the 2 outliers (min and max) which is a bit more powerful than just removing the single central value
                                        // then do a distance-weighted mean
    sumv    = 0;
    sumw    = 0;

    for ( int j = 1; j < maxneigh - 1; j++ ) {
                                        // weighted sum by distance
                                        // - in case central value is used, its weight will be 1
                                        // - note that some distances could be lower than 1, due to the average distance used for normalization, so the weight will be more than 1 on these points
//      w       = 1 / NonNull ( neigh ( j, NeighborhoodDistance ) );
        w       = Gaussian ( GaussianSigmaToWidth ( neigh ( j, NeighborhoodDistance ) ), 0, 1, 1 );
        sumv   += w *                               neigh ( j, NeighborhoodValue    );
        sumw   += w;
        }


    map[ i ]    = sumv / NonNull ( sumw );
    } // for Dim1


return  true;
}


//----------------------------------------------------------------------------
                                        // Interpolation of bad electrodes + smoothing
template <class TypeD>
bool    TFilterSpatial<TypeD>::SpatialFilteringInterpolateSmooth ( TVector<TypeD>& map )
{
if ( ! (    How == SpatialFilterOutliersGaussianMean
         || How == SpatialFilterOutliersWeightedMean ) )
    return  false;


//if ( ! IsDimensionOK ( map ) )
//    return  false;

int                 Dim1            = map.GetDim1 ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Identify the bads electrodes first
TVector<TypeD>      bads ( Dim1 );

TEasyStats          stat ( MaxNumNeigh /*NeighDist.GetDim2 ()*/ );
double              delta;
double              reasonmin;
double              reasonmax;

TVector<TypeD>      temp ( map );


for ( int i = 0; i < Dim1; i++ ) {


    stat.Reset ();
                                        // stat of all neighbors below a given radius - central value excluded
                                        // stops when either radius is further, or number of neghbors is reached
    for ( int j = 1; j <= MaxNumNeigh; j++ ) {

        if ( NeighDist ( i, j, NeighborhoodDistance ) > MaxDistNeigh )
            break;                      // distances are sorted, we can stop here


        stat.Add ( temp[ NeighDist ( i, j, NeighborhoodIndex ) ] );
        } // for MaxNumNeigh


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Is central point an outlier?
    delta       = stat.Range () * SpatialFilterExtraRange;
    reasonmin   = stat.Min   () - delta;
    reasonmax   = stat.Max   () + delta;

                                        // If any of these 2 tests fail, assume this electrode is bad
    bads[ i ]   = ! IsInsideLimits ( (double) temp[ i ], reasonmin, reasonmax )         // if central point is not within a reasonable margin from its neighbors
               ||   fabs ( stat.ZScoreRobust ( temp[ i ] ) ) > SpatialFilterZScoreMin;  // or Z-Score is not satisfying - Note that the robust version indeed works better

    } // for Dim1


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Smoothing while ignoring the bads
                                        // for distance and value sort
double              w;
double              sumw;
double              sumv;

for ( int i = 0; i < Dim1; i++ ) {
                                        // use all data, which are supposed to be "not bad"
    sumv    = 0;
    sumw    = 0;
                                        // stat of all neighbors, including central value, below radius
                                        // stops when either radius is further, or number of neighbors is reached
    for ( int j = 0; j <= MaxNumNeigh; j++ ) {

                                        // skip the baddies - central or neighbor values
        if ( bads[ NeighDist ( i, j, NeighborhoodIndex ) ] )
            continue;

                                        // stop when too far off
        if ( NeighDist ( i, j, NeighborhoodDistance ) > MaxDistNeigh )
            break;                      // distances are sorted, we can stop here

                                        // weighted sum by distance
                                        // - in case central value is used, its weight will be 1
                                        // - note that some distances could be lower than 1, due to the average distance used for normalization, so the weight will be more than 1 on these points
        if ( How == SpatialFilterOutliersGaussianMean )
            w       = Gaussian ( GaussianSigmaToWidth ( NeighDist ( i, j, NeighborhoodDistance ) ), 0, 1, 1 );
        else
            w       = 1 / NonNull (      NeighDist ( i, j, NeighborhoodDistance ) );


        sumv   += w *              temp[ NeighDist ( i, j, NeighborhoodIndex    ) ];
        sumw   += w;
        }

                                        // check there remain (enough?) values in...
    if ( sumw /*maxneigh*/ > 0 )
        map[ i ]    = sumv / NonNull ( sumw );
    } // for Dim1


return  true;
}


//----------------------------------------------------------------------------
                                        // Interseptile family of filters

                                        // At the border of the net, there are fewer neighbors, therefore the Median gets a little too much biased by the central value.
                                        // So a Median can tend to preserve and/or spread a potential local extrema "inside" the net.
                                        // By using the Interseptile Mean, we use all data, but reject the outliers (be them on central or as neighbors),
                                        // then use a mean to have a new value, instead of a Median which returns only one among the data.
                                        // This combines the robustness of the Median, and the smoothing of the Mean.
                                        // Globally, 1 pass is not as effective as the Median. We can chose to apply 2 passes, which has the side effect benefit of stabilizing an extrema that could have switch to a neighbor when in 1 pass.
template <class TypeD>
bool    TFilterSpatial<TypeD>::SpatialFilteringInterseptile ( TVector<TypeD>& map )
{
if ( ! (    How == SpatialFilterInterseptileWeightedMean
         || How == SpatialFilterInterseptileGaussianMean ) )
    return  false;


//if ( ! IsDimensionOK ( map ) )
//    return  false;

int                 Dim1            = map.GetDim1 ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // for distance and value sort
TArray2<double>     neigh ( Dim1, /*NumNeighborhoodInfo*/ NumNeighborhoodInfoExt );


int                 maxneigh;
double              w;
double              sumw;
double              sumv;

TVector<TypeD>      temp ( map );


for ( int i = 0; i < Dim1; i++ ) {


    neigh       = DBL_MAX;            // sorting will be ascending
    maxneigh    = 0;

                                        // stat of all neighbors, including central value, below radius
                                        // stops when either radius is further, or number of neighbors is reached
    for ( int j = 0; j <= MaxNumNeigh; j++ ) {

                                        // !we need to have at least 3 data points to be able to remove 2 outliers, even if they are outside the limit!
        if ( NeighDist ( i, j, NeighborhoodDistance ) > MaxDistNeigh && maxneigh > SpatialFilterMinNeighbors )
            break;                      // distances are sorted, we can stop here

                                        // store values and distances
//      neigh ( maxneigh, NeighborhoodIndex     )   =       NeighDist ( i, j, NeighborhoodIndex    );   // not actually needed

        neigh ( maxneigh, NeighborhoodValue     )   = temp[ NeighDist ( i, j, NeighborhoodIndex    ) ];

        neigh ( maxneigh, NeighborhoodDistance  )   =       NeighDist ( i, j, NeighborhoodDistance );

        maxneigh++;
        } // for MaxNumNeigh


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // sort by value
    neigh.SortRows ( NeighborhoodValue, Ascending, maxneigh );


    sumv    = 0;
    sumw    = 0;

                                        // exclude the 2 outliers (min and max), then do a distance-weighted mean
    for ( int j = 1; j < maxneigh - 1; j++ ) {

        if ( How == SpatialFilterInterseptileGaussianMean )
                                        // does less filtering than inverse distance
            w   = Gaussian ( GaussianSigmaToWidth ( neigh ( j, NeighborhoodDistance ) ), 0, 1, 1 );

        else // SpatialFilterInterseptileWeightedMean, SpatialFilterOutlier
                                        // weighted sum by distance
                                        // - in case central value is used, its weight will be 1
                                        // - note that some distances could be lower than 1, due to the average distance used for normalization, so the weight will be more than 1 on these points
            w   = 1 / NonNull ( neigh ( j, NeighborhoodDistance ) );


        sumv   += w *           neigh ( j, NeighborhoodValue    );
        sumw   += w;
        }


    map[ i ]    = sumv / NonNull ( sumw );
    } // for Dim1


return  true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
