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

#pragma once

namespace crtl {

//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
                                        // usually called from already parallelized code
template <class TypeD>
int     TVolume<TypeD>::GetNumSet ()    const
{
int                 num             = 0;

for ( int i = 0; i < LinearDim; i++ )
    if ( Array[ i ] != 0 )
        num++;

return  num;
}


template <class TypeD>
double  TVolume<TypeD>::GetPercentilePosition   ( double percentile, TVolume<TypeD> *mask )   const
{
if ( IsNotAllocated () )
    return  0;


THistogram          H   (   *this,
                            mask,
                            0,
                            0,
                            0,  3, 
                            (HistogramOptions) ( HistogramPDF | HistogramRaw | HistogramNormNone | HistogramLinear | HistogramIgnoreNulls )
                        );

return  H.GetPercentilePosition ( RealUnit, percentile );
}


//----------------------------------------------------------------------------
                                        // Do NOT parallelized - called from parallel blocks
template <class TypeD>
int     TVolume<TypeD>::GetNumNeighbors     ( int x, int y, int z, NeighborhoodType neighborhood )   const
{
                                        // for filters, we don't care for borders
if ( ! ( x > 0 && y > 0 && z > 0 && x < Dim1 - 1 && y < Dim2 - 1 && z < Dim3 - 1 ) )
    return  0;


int                 num             = 0;
int                 index;


index = IndexesToLinearIndex ( x - 1, y, z );

if ( neighborhood == Neighbors26 )  num  += (bool) Array[ index - Dim3 - 1 ];
if ( neighborhood != Neighbors6  )  num  += (bool) Array[ index - Dim3     ];
if ( neighborhood == Neighbors26 )  num  += (bool) Array[ index - Dim3 + 1 ];
if ( neighborhood != Neighbors6  )  num  += (bool) Array[ index        - 1 ];
                                    num  += (bool) Array[ index            ];
if ( neighborhood != Neighbors6  )  num  += (bool) Array[ index        + 1 ];
if ( neighborhood == Neighbors26 )  num  += (bool) Array[ index + Dim3 - 1 ];
if ( neighborhood != Neighbors6  )  num  += (bool) Array[ index + Dim3     ];
if ( neighborhood == Neighbors26 )  num  += (bool) Array[ index + Dim3 + 1 ];


index = IndexesToLinearIndex ( x, y, z );

if ( neighborhood != Neighbors6  )  num  += (bool) Array[ index - Dim3 - 1 ];
                                    num  += (bool) Array[ index - Dim3     ];
if ( neighborhood != Neighbors6  )  num  += (bool) Array[ index - Dim3 + 1 ];
                                    num  += (bool) Array[ index        - 1 ];
//                                  num  += (bool) Array[ index            ];   // don't count center
                                    num  += (bool) Array[ index        + 1 ];
if ( neighborhood != Neighbors6  )  num  += (bool) Array[ index + Dim3 - 1 ];
                                    num  += (bool) Array[ index + Dim3     ];
if ( neighborhood != Neighbors6  )  num  += (bool) Array[ index + Dim3 + 1 ];


index = IndexesToLinearIndex ( x + 1, y, z );

if ( neighborhood == Neighbors26 )  num  += (bool) Array[ index - Dim3 - 1 ];
if ( neighborhood != Neighbors6  )  num  += (bool) Array[ index - Dim3     ];
if ( neighborhood == Neighbors26 )  num  += (bool) Array[ index - Dim3 + 1 ];
if ( neighborhood != Neighbors6  )  num  += (bool) Array[ index        - 1 ];
                                    num  += (bool) Array[ index            ];
if ( neighborhood != Neighbors6  )  num  += (bool) Array[ index        + 1 ];
if ( neighborhood == Neighbors26 )  num  += (bool) Array[ index + Dim3 - 1 ];
if ( neighborhood != Neighbors6  )  num  += (bool) Array[ index + Dim3     ];
if ( neighborhood == Neighbors26 )  num  += (bool) Array[ index + Dim3 + 1 ];


return  num;
}

/*
void    Volume::StatNeighbors ( int x, int y, int z, int neighborhood, TEasyStats &stat )
{
//stat.Reset ();

                                        // for filters, we don't care for borders
if ( ! ( x > 0 && y > 0 && z > 0 && x < Dim1 - 1 && y < Dim2 - 1 && z < Dim3 - 1 ) )
    return;


int                 index;


index = IndexesToLinearIndex ( x - 1, y, z );

if ( neighborhood == Neighbors26 )  stat.Add ( Array[ index - Dim3 - 1 ] );
if ( neighborhood != Neighbors6  )  stat.Add ( Array[ index - Dim3     ] );
if ( neighborhood == Neighbors26 )  stat.Add ( Array[ index - Dim3 + 1 ] );
if ( neighborhood != Neighbors6  )  stat.Add ( Array[ index        - 1 ] );
                                    stat.Add ( Array[ index            ] );
if ( neighborhood != Neighbors6  )  stat.Add ( Array[ index        + 1 ] );
if ( neighborhood == Neighbors26 )  stat.Add ( Array[ index + Dim3 - 1 ] );
if ( neighborhood != Neighbors6  )  stat.Add ( Array[ index + Dim3     ] );
if ( neighborhood == Neighbors26 )  stat.Add ( Array[ index + Dim3 + 1 ] );


index = IndexesToLinearIndex ( x, y, z );

if ( neighborhood != Neighbors6  )  stat.Add ( Array[ index - Dim3 - 1 ] );
                                    stat.Add ( Array[ index - Dim3     ] );
if ( neighborhood != Neighbors6  )  stat.Add ( Array[ index - Dim3 + 1 ] );
                                    stat.Add ( Array[ index        - 1 ] );
                                    stat.Add ( Array[ index            ] ); // here, count center
                                    stat.Add ( Array[ index        + 1 ] );
if ( neighborhood != Neighbors6  )  stat.Add ( Array[ index + Dim3 - 1 ] );
                                    stat.Add ( Array[ index + Dim3     ] );
if ( neighborhood != Neighbors6  )  stat.Add ( Array[ index + Dim3 + 1 ] );


index = IndexesToLinearIndex ( x + 1, y, z );

if ( neighborhood == Neighbors26 )  stat.Add ( Array[ index - Dim3 - 1 ] );
if ( neighborhood != Neighbors6  )  stat.Add ( Array[ index - Dim3     ] );
if ( neighborhood == Neighbors26 )  stat.Add ( Array[ index - Dim3 + 1 ] );
if ( neighborhood != Neighbors6  )  stat.Add ( Array[ index        - 1 ] );
                                    stat.Add ( Array[ index            ] );
if ( neighborhood != Neighbors6  )  stat.Add ( Array[ index        + 1 ] );
if ( neighborhood == Neighbors26 )  stat.Add ( Array[ index + Dim3 - 1 ] );
if ( neighborhood != Neighbors6  )  stat.Add ( Array[ index + Dim3     ] );
if ( neighborhood == Neighbors26 )  stat.Add ( Array[ index + Dim3 + 1 ] );

stat.Sort ( true );
}
*/
/*
void    Volume::SetNeighbors ( int x, int y, int z, MriType v, int neighborhood )
{
                                        // for filters, we don't care for borders
if ( ! ( x > 0 && y > 0 && z > 0 && x < Dim1 - 1 && y < Dim2 - 1 && z < Dim3 - 1 ) )
    return;


int                 index;


index = IndexesToLinearIndex ( x - 1, y, z );

if ( neighborhood == Neighbors26 )  Array[ index - Dim3 - 1 ]   = v;
if ( neighborhood != Neighbors6  )  Array[ index - Dim3     ]   = v;
if ( neighborhood == Neighbors26 )  Array[ index - Dim3 + 1 ]   = v;
if ( neighborhood != Neighbors6  )  Array[ index        - 1 ]   = v;
                                    Array[ index            ]   = v;
if ( neighborhood != Neighbors6  )  Array[ index        + 1 ]   = v;
if ( neighborhood == Neighbors26 )  Array[ index + Dim3 - 1 ]   = v;
if ( neighborhood != Neighbors6  )  Array[ index + Dim3     ]   = v;
if ( neighborhood == Neighbors26 )  Array[ index + Dim3 + 1 ]   = v;


index = IndexesToLinearIndex ( x, y, z );

if ( neighborhood != Neighbors6  )  Array[ index - Dim3 - 1 ]   = v;
                                    Array[ index - Dim3     ]   = v;
if ( neighborhood != Neighbors6  )  Array[ index - Dim3 + 1 ]   = v;
                                    Array[ index        - 1 ]   = v;
//                                  Array[ index            ]   = v;   // don't set center!
                                    Array[ index        + 1 ]   = v;
if ( neighborhood != Neighbors6  )  Array[ index + Dim3 - 1 ]   = v;
                                    Array[ index + Dim3     ]   = v;
if ( neighborhood != Neighbors6  )  Array[ index + Dim3 + 1 ]   = v;


index = IndexesToLinearIndex ( x + 1, y, z );

if ( neighborhood == Neighbors26 )  Array[ index - Dim3 - 1 ]   = v;
if ( neighborhood != Neighbors6  )  Array[ index - Dim3     ]   = v;
if ( neighborhood == Neighbors26 )  Array[ index - Dim3 + 1 ]   = v;
if ( neighborhood != Neighbors6  )  Array[ index        - 1 ]   = v;
                                    Array[ index            ]   = v;
if ( neighborhood != Neighbors6  )  Array[ index        + 1 ]   = v;
if ( neighborhood == Neighbors26 )  Array[ index + Dim3 - 1 ]   = v;
if ( neighborhood != Neighbors6  )  Array[ index + Dim3     ]   = v;
if ( neighborhood == Neighbors26 )  Array[ index + Dim3 + 1 ]   = v;
}
*/

//----------------------------------------------------------------------------
                                        // This looks like a morphological operator, but it is not
template <class TypeD>
void    TVolume<TypeD>::FilterNeighbors ( FilterTypes filtertype, FctParams& params, bool showprogress )
{
int                 numneighbors    =              (int) params ( FilterParamNumNeighbors );
NeighborhoodType    neighborhood    = (NeighborhoodType) params ( FilterParamNeighborhood );


if ( numneighbors < 0 || numneighbors > Neighborhood[ neighborhood ].NumNeighbors )
    return;

                                        // we need a cloned, temp array
TVolume<TypeD>      temp ( *this );
TypeD               maxvalue        = GetMaxValue ();


TSuperGauge         Gauge ( FilterPresets[ filtertype ].Text, showprogress ? Dim1 - 2: 0 );


OmpParallelFor

for ( int x = 1; x < Dim1 - 1; x++ ) {

    Gauge.Next ();

    for ( int y = 1; y < Dim2 - 1; y++ )
    for ( int z = 1; z < Dim3 - 1; z++ ) {

        if ( filtertype == FilterTypeLessNeighbors ) {
            if (   temp ( x, y, z ) && temp.GetNumNeighbors ( x, y, z, neighborhood ) < numneighbors )
                (*this) ( x, y, z )  = 0;
            }
        else { // FilterTypeMoreNeighbors
            if ( ! temp ( x, y, z ) && temp.GetNumNeighbors ( x, y, z, neighborhood ) > numneighbors )
                (*this) ( x, y, z )  = maxvalue;
            }
        } // for y, z
    } // for x
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVolume<TypeD>::FilterRelax     ( FctParams& params, bool showprogress )
{
double              diameter        =       params ( FilterParamDiameter );
int                 repeat          = (int) params ( FilterParamNumRelax );


if ( diameter < 2 )                     // no other voxels involved other than center?
    return;

if ( repeat < 1 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // create Kernel mask, always of odd size
TVolume<uchar>      K  ( diameter, OddSize );
TPointInt           Ko ( K.GetDim1 () / 2, K.GetDim2 () / 2, K.GetDim3 () / 2 );

double              r2              = Square ( diameter / 2 ) + ( IsEven ( K.GetDim1 () ) ? 3 * Square ( 0.5 ) : 0 ) + SingleFloatEpsilon;
TPointDouble        kp;


                                        // compute the Kernel
for ( int xk = 0; xk < K.GetDim1 (); xk++ )
for ( int yk = 0; yk < K.GetDim2 (); yk++ )
for ( int zk = 0; zk < K.GetDim3 (); zk++ ) {

    kp.Set ( xk - Ko.X, yk - Ko.Y, zk - Ko.Z );
                                        // clip Kernel outside Euclidian norm!
    K ( xk, yk, zk )    = kp.Norm2 () <= r2;
    } // for xk, yk, zk


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // give more room: Kernel size + 1 neighbor test
TPointInt           Kshift ( K.GetDim1 () + 1, K.GetDim2 () + 1, K.GetDim3 () + 1 );
TPointDouble        Koi    ( K.GetDim1 () / 2, K.GetDim2 () / 2, K.GetDim3 () / 2 );
int                 Knumset2        = ( K.GetNumSet () - 1 ) / 2;
//int                 Knumsetmin      = ( K.GetNumSet () - 1 ) * 0.10;
//int                 Knumsetmax      = ( K.GetNumSet () - 1 ) * 0.40;

TypeD               maxvalue        = GetMaxValue ();

                                        // we need a cloned temp array, which includes a safety border
TVolume<TypeD>      temp ( Dim1 + Kshift.X * 2, Dim2 + Kshift.Y * 2, Dim3 + Kshift.Z * 2 );


TSuperGauge         Gauge ( FilterPresets[ FilterTypeRelax ].Text, showprogress ? repeat * Dim1 : 0 );


OmpParallelBegin

for ( int r = 0; r < repeat; r++ ) {

    temp.Insert ( *this, Kshift );

    OmpFor

    for ( int x = 0; x < Dim1; x++ ) {

        Gauge.Next ();


        for ( int y = 0; y < Dim2; y++ )
        for ( int z = 0; z < Dim3; z++ ) {
                                        // count Kernel, count neighbors only so discard the central voxel
            int     numneighbors    = -1;

            for ( int xki = 0, xk = x + Kshift.X - (int) Koi.X; xki < K.GetDim1 (); xki++, xk++ )
            for ( int yki = 0, yk = y + Kshift.Y - (int) Koi.Y; yki < K.GetDim2 (); yki++, yk++ )
            for ( int zki = 0, zk = z + Kshift.Z - (int) Koi.Z; zki < K.GetDim3 (); zki++, zk++ )

                if ( K ( xki, yki, zki ) && temp ( xk, yk, zk ) )
                    numneighbors++;

                                        // if full, remove if neighbors are rather empty; if empty, fill if neighbors are rather full
            if      ( GetValue ( x, y, z ) ) {
                if  ( numneighbors <= Knumset2 )    Array[ IndexesToLinearIndex ( x, y, z ) ] = 0;
                }
            else if ( numneighbors >= Knumset2 )    Array[ IndexesToLinearIndex ( x, y, z ) ] = maxvalue;

//                if      ( ! GetValue ( x, y, z ) && numneighbors > Knumsetmin && numneighbors < Knumsetmax )
//                    Array[ IndexesToLinearIndex ( x, y, z ) ] = maxvalue;
            } // for y, z
        } // for x
    } // for repeat

OmpParallelEnd
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVolume<TypeD>::FilterShapeFeature  ( FilterTypes filtertype, FctParams& params, bool showprogress )
{
double              diameter        =        params ( FilterParamDiameter );
double              belowcut        = Clip ( params ( FilterParamFullnessCut ), (double) 0, (double) 1 );


if ( diameter < 2 )                     // no other voxels involved other than center?
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // create Kernel mask, always of odd size
TVolume<uchar>      K  ( diameter, OddSize );
TPointInt           Ko ( K.GetDim1 () / 2, K.GetDim2 () / 2, K.GetDim3 () / 2 );

double              r2              = Square ( diameter / 2 ) + ( IsEven ( K.GetDim1 () ) ? 3 * Square ( 0.5 ) : 0 ) + SingleFloatEpsilon;
TPointDouble        kp;


                                        // compute the Kernel
for ( int xk = 0; xk < K.GetDim1 (); xk++ )
for ( int yk = 0; yk < K.GetDim2 (); yk++ )
for ( int zk = 0; zk < K.GetDim3 (); zk++ ) {

    kp.Set ( xk - Ko.X, yk - Ko.Y, zk - Ko.Z );
                                        // clip Kernel outside Euclidian norm!
    K ( xk, yk, zk )    = kp.Norm2 () <= r2;
    } // for xk, yk, zk


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // give more room: Kernel size + 1 neighbor test
TPointInt           Kshift ( K.GetDim1 () + 1, K.GetDim2 () + 1, K.GetDim3 () + 1 );
TPointDouble        Koi    ( K.GetDim1 () / 2, K.GetDim2 () / 2, K.GetDim3 () / 2 );
//TPointDouble        barycenter;
//TEasyStats          statx;
//TEasyStats          staty;
//TEasyStats          statz;

double              Knumset         = K.GetNumSet ();

belowcut    = belowcut * Knumset;

                                        // we need a cloned temp array, which includes a safety border
TVolume<TypeD>      temp ( Dim1 + Kshift.X * 2, Dim2 + Kshift.Y * 2, Dim3 + Kshift.Z * 2 );

temp.Insert ( *this, Kshift );


TSuperGauge         Gauge ( FilterPresets[ filtertype ].Text, showprogress ? Dim1 : 0 );


OmpParallelFor

for ( int x = 0; x < Dim1; x++ ) {

    Gauge.Next ();


    for ( int y = 0; y < Dim2; y++ )
    for ( int z = 0; z < Dim3; z++ ) {

        int                 numpoints       = 0;
        double              surface         = 0;
//      barycenter.Reset ();
//      statx.Reset (); staty.Reset (); statz.Reset ();
        double              v;


        for ( int xki = 0, xk = x + Kshift.X - (int) Koi.X; xki < K.GetDim1 (); xki++, xk++ )
        for ( int yki = 0, yk = y + Kshift.Y - (int) Koi.Y; yki < K.GetDim2 (); yki++, yk++ )
        for ( int zki = 0, zk = z + Kshift.Z - (int) Koi.Z; zki < K.GetDim3 (); zki++, zk++ )

            if ( K ( xki, yki, zki ) && temp ( xk, yk, zk ) ) {
                numpoints  ++;
                surface    += temp.GetNumNeighbors ( xk, yk, zk, Neighbors26 );
//              barycenter += TPointDouble ( xki, yki, zki );
//              statx.Add ( xki ); staty.Add ( yki ); statz.Add ( zki );
                }
                                    // normalize surface
        surface     = ( numpoints * Neighborhood[ Neighbors26 ].NumNeighbors - surface ) / Neighborhood[ Neighbors26 ].NumNeighbors;
//      barycenter /= numpoints;


        if      ( filtertype == FilterTypePercentFullness )     v   = (double) numpoints / Knumset * 100;
        else if ( filtertype == FilterTypeCutBelowFullness )    v   = numpoints < belowcut ? 0 : Array[ IndexesToLinearIndex ( x, y, z ) ];

        else if ( filtertype == FilterTypeSAV         )         v   = numpoints ? Power ( surface / numpoints, 1.5 ) * 100 : 0;   // relative
//      else if ( filtertype == FilterTypeSAV         )         v   = numpoints ? Power ( surface / Knumset,   1.5 ) * 100 : 0;   // absolute

//      else if ( filtertype == FilterTypeCompactness )         v   = numpoints ? Power ( numpoints / surface, 1.5 ) * 10 : 0;    // normalization?
        else if ( filtertype == FilterTypeCompactness )         v   = numpoints ? ( 1 - Power ( surface / numpoints, 1.5 ) ) * 100 : 0;   // simpler to invert the SAV

                                    // eccentricity of data from center of Kernel
//      else if ( filtertype == FilterTypeCompactness )         v   = numpoints ? ( barycenter - Koi ).Norm () / diameter * 200 + 0.5 : 0;
                              // fullness * eccentricity
//      else if ( filtertype == FilterTypeCompactness )         v   = numpoints ? (double) numpoints / Knumset  *  ( 1 - ( barycenter - Koi ).Norm () / diameter ) * 200 + 0.5 : 0;
                              // fullness + compactness
//      else if ( filtertype == FilterTypeCompactness )         v   = numpoints ? (double) numpoints / Knumset * 50  +  ( 1 - Power ( surface / numpoints, 1.5 ) ) * 50 + 0.5 : 0;
                              // fullness + compactness + eccentricity
//      else if ( filtertype == FilterTypeCompactness )         v   = numpoints ? (double) numpoints / Knumset  *  ( 1 - Power ( surface / numpoints, 1.5 ) )  *  ( ( barycenter - Koi ).Norm () / diameter )  * 200 + 0.5 : 0;

//      else if ( filtertype == FilterTypeCompactness )         v   = max ( statx.SD (), staty.SD (), statz.SD () ) / diameter * 100;
//      else if ( filtertype == FilterTypeCompactness )         v   = sqrt ( statx.Variance () + staty.Variance () + statz.Variance () ) / diameter * 100;


        Array[ IndexesToLinearIndex ( x, y, z ) ]   = (TypeD) v;
        } // for y, z
    } // for x
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVolume<TypeD>::FilterThinning  ( FctParams& params, bool showprogress )
{
int                 repeat          = (int) params ( FilterParamThinningRepeat );
bool                converge        = repeat <= 0;
int                 numfilters      = 6 + 12 + 8;   // cube faces, edges, corners

if ( converge )     repeat          = INT_MAX;
else                repeat         *= numfilters;

if ( repeat < 1 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // give more room: Kernel size + 1 neighbor test
TPointInt           Kshift ( 1, 1, 1 );
                                        // we need a cloned temp array, which includes a safety border
TVolume<TypeD>      temp ( Dim1 + Kshift.X * 2, Dim2 + Kshift.Y * 2, Dim3 + Kshift.Z * 2 );

int                 nomore          = 0;
int                 numset          = GetNumSet ();
int                 maxnumset       = numset;
int                 oldnumset;


TSuperGauge         Gauge ( FilterPresets[ FilterTypeThinning ].Text, showprogress ? ( converge ? 100 : repeat ) : 0 );


for ( int r = 0; r < repeat; r++ ) {

    temp.Insert ( *this, Kshift );
                                        // not super efficient, still at 50% CPU
    OmpParallelFor

    for ( int x = 1; x < Dim1; x++ )
    for ( int y = 1; y < Dim2; y++ )
    for ( int z = 1; z < Dim3; z++ ) {

                                // should be non-empty
        if ( ! temp.GetValue ( x, y, z ) )
            continue;

                                // shouldn't break neighborhood (basic cases only, missing corners connectivities)
        if ( ! temp.GetValue ( x    , y    , z + 1 ) && ! temp.GetValue ( x    , y    , z - 1 )
          || ! temp.GetValue ( x    , y + 1, z     ) && ! temp.GetValue ( x    , y - 1, z     )
          || ! temp.GetValue ( x + 1, y    , z     ) && ! temp.GetValue ( x - 1, y    , z     )

          || ! temp.GetValue ( x + 1, y + 1, z     ) && ! temp.GetValue ( x - 1, y - 1, z     )
          || ! temp.GetValue ( x + 1, y - 1, z     ) && ! temp.GetValue ( x - 1, y + 1, z     )
          || ! temp.GetValue ( x + 1, y    , z + 1 ) && ! temp.GetValue ( x - 1, y    , z - 1 )
          || ! temp.GetValue ( x + 1, y    , z - 1 ) && ! temp.GetValue ( x - 1, y    , z + 1 )
          || ! temp.GetValue ( x    , y + 1, z + 1 ) && ! temp.GetValue ( x    , y - 1, z - 1 )
          || ! temp.GetValue ( x    , y + 1, z - 1 ) && ! temp.GetValue ( x    , y - 1, z + 1 )

          || ! temp.GetValue ( x + 1, y + 1, z + 1 ) && ! temp.GetValue ( x - 1, y - 1, z - 1 )
          || ! temp.GetValue ( x + 1, y + 1, z - 1 ) && ! temp.GetValue ( x - 1, y - 1, z + 1 )
          || ! temp.GetValue ( x + 1, y - 1, z + 1 ) && ! temp.GetValue ( x - 1, y + 1, z - 1 )
          || ! temp.GetValue ( x + 1, y - 1, z - 1 ) && ! temp.GetValue ( x - 1, y + 1, z + 1 ) )

            continue;


        int                 c1;
        int                 c2;

        switch ( r % numfilters ) {

                                        // 1) Planes: look for 5 neighbors (one neighbor plane in 18 neighbors) on each side, counting for either 5/0 or 0/5
                                        // z bars
        case  0:
        c1      = (bool) temp.GetValue ( x, y + 1, z + 1 ) + (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z - 1 ) + (bool) temp.GetValue ( x + 1, y + 1, z ) + (bool) temp.GetValue ( x - 1, y + 1, z );
        c2      = (bool) temp.GetValue ( x, y - 1, z + 1 ) + (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z - 1 ) + (bool) temp.GetValue ( x + 1, y - 1, z ) + (bool) temp.GetValue ( x - 1, y - 1, z );
        if ( c1 == 5 && c2 == 0 || c1 == 0 && c2 == 5 )     Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case  1:
        c1      = (bool) temp.GetValue ( x + 1, y, z + 1 ) + (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y, z - 1 ) + (bool) temp.GetValue ( x + 1, y + 1, z ) + (bool) temp.GetValue ( x + 1, y - 1, z );
        c2      = (bool) temp.GetValue ( x - 1, y, z + 1 ) + (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y, z - 1 ) + (bool) temp.GetValue ( x - 1, y + 1, z ) + (bool) temp.GetValue ( x - 1, y - 1, z );
        if ( c1 == 5 && c2 == 0 || c1 == 0 && c2 == 5 )     Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;
                                // y bars
        case  2:
        c1      = (bool) temp.GetValue ( x, y + 1, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y - 1, z + 1 ) + (bool) temp.GetValue ( x + 1, y, z + 1 ) + (bool) temp.GetValue ( x - 1, y, z + 1 );
        c2      = (bool) temp.GetValue ( x, y + 1, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y - 1, z - 1 ) + (bool) temp.GetValue ( x + 1, y, z - 1 ) + (bool) temp.GetValue ( x - 1, y, z - 1 );
        if ( c1 == 5 && c2 == 0 || c1 == 0 && c2 == 5 )     Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case  3:
        c1      = (bool) temp.GetValue ( x + 1, y + 1, z ) + (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y - 1, z ) + (bool) temp.GetValue ( x + 1, y, z + 1 ) + (bool) temp.GetValue ( x + 1, y, z - 1 );
        c2      = (bool) temp.GetValue ( x - 1, y + 1, z ) + (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y - 1, z ) + (bool) temp.GetValue ( x - 1, y, z + 1 ) + (bool) temp.GetValue ( x - 1, y, z - 1 );
        if ( c1 == 5 && c2 == 0 || c1 == 0 && c2 == 5 )     Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;
                                // x bars
        case  4:
        c1      = (bool) temp.GetValue ( x + 1, y, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x - 1, y, z + 1 ) + (bool) temp.GetValue ( x, y + 1, z + 1 ) + (bool) temp.GetValue ( x, y - 1, z + 1 );
        c2      = (bool) temp.GetValue ( x + 1, y, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x - 1, y, z - 1 ) + (bool) temp.GetValue ( x, y + 1, z - 1 ) + (bool) temp.GetValue ( x, y - 1, z - 1 );
        if ( c1 == 5 && c2 == 0 || c1 == 0 && c2 == 5 )     Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case  5:
        c1      = (bool) temp.GetValue ( x + 1, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x - 1, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z + 1 ) + (bool) temp.GetValue ( x, y + 1, z - 1 );
        c2      = (bool) temp.GetValue ( x + 1, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x - 1, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z + 1 ) + (bool) temp.GetValue ( x, y - 1, z - 1 );
        if ( c1 == 5 && c2 == 0 || c1 == 0 && c2 == 5 )     Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;


                                        // 2) Edges
/*
                                        // Full edges - gives flat surfaces but not deep enough
        case  6:
        c1      = (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z );
        c2      = (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x - 1, y, z );
        z--;
        c1     += (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z );
        c2     += (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x - 1, y, z );
        z++;
//        z+=2;
//        c1     += (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z );
//        c2     += (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x - 1, y, z );
//        z--;
//        if ( c1 == 0 && c2 == 9 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        if ( c1 == 0 && c2 == 6 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case  7:
        c1      = (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 );
        c2      = (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x - 1, y, z );
        y--;
        c1     += (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 );
        c2     += (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x - 1, y, z );
        y++;
//        y+=2;
//        c1     += (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 );
//        c2     += (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x - 1, y, z );
//        y--;
//        if ( c1 == 0 && c2 == 9 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        if ( c1 == 0 && c2 == 6 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case  8:
        c1      = (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 );
        c2      = (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x, y - 1, z );
        x--;
        c1     += (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 );
        c2     += (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x, y - 1, z );
        x++;
//                x+=2;
//                c1     += (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 );
//                c2     += (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x, y - 1, z );
//                x--;
//                if ( c1 == 0 && c2 == 9 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        if ( c1 == 0 && c2 == 6 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;


        case  9:
        c1      = (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z );
        c2      = (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x - 1, y, z );
        z--;
        c1     += (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z );
        c2     += (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x - 1, y, z );
        z++;
//                z+=2;
//                c1     += (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z );
//                c2     += (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x - 1, y, z );
//                z--;
//                if ( c1 == 0 && c2 == 9 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        if ( c1 == 0 && c2 == 6 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 10:
        c1      = (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 );
        c2      = (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x - 1, y, z );
        y--;
        c1     += (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 );
        c2     += (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x - 1, y, z );
        y++;
//                y+=2;
//                c1     += (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 );
//                c2     += (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x - 1, y, z );
//                y--;
//                if ( c1 == 0 && c2 == 9 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        if ( c1 == 0 && c2 == 6 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 11:
        c1      = (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 );
        c2      = (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y, z ) +         (bool) temp.GetValue ( x, y - 1, z );
        x--;
        c1     += (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 );
        c2     += (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y, z ) +         (bool) temp.GetValue ( x, y - 1, z );
        x++;
//                x+=2;
//                c1     += (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 );
//                c2     += (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y, z ) +         (bool) temp.GetValue ( x, y - 1, z );
//                x--;
//                if ( c1 == 0 && c2 == 9 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        if ( c1 == 0 && c2 == 6 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;


        case 12:
        c1      = (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z );
        c2      = (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x + 1, y, z );
        z--;
        c1     += (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z );
        c2     += (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x + 1, y, z );
        z++;
//                z+=2;
//                c1     += (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z );
//                c2     += (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x + 1, y, z );
//                z--;
//                if ( c1 == 0 && c2 == 9 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        if ( c1 == 0 && c2 == 6 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 13:
        c1      = (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 );
        c2      = (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x + 1, y, z );
        y--;
        c1     += (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 );
        c2     += (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x + 1, y, z );
        y++;
//                y+=2;
//                c1     += (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 );
//                c2     += (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x + 1, y, z );
//                y--;
//                if ( c1 == 0 && c2 == 9 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        if ( c1 == 0 && c2 == 6 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 14:
        c1      = (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 );
        c2      = (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x, y + 1, z );
        x--;
        c1     += (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 );
        c2     += (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x, y + 1, z );
        x++;
//                x+=2;
//                c1     += (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 );
//                c2     += (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x, y + 1, z );
//                x--;
//                if ( c1 == 0 && c2 == 9 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        if ( c1 == 0 && c2 == 6 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;


        case 15:
        c1      = (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z );
        c2      = (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x + 1, y, z );
        z--;
        c1     += (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z );
        c2     += (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x + 1, y, z );
        z++;
//                z+=2;
//                c1     += (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z );
//                c2     += (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x + 1, y, z );
//                z--;
//                if ( c1 == 0 && c2 == 9 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        if ( c1 == 0 && c2 == 6 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 16:
        c1      = (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 );
        c2      = (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x + 1, y, z );
        y--;
        c1     += (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 );
        c2     += (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x + 1, y, z );
        y++;
//                y+=2;
//                c1     += (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 );
//                c2     += (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x + 1, y, z );
//                y--;
//                if ( c1 == 0 && c2 == 9 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        if ( c1 == 0 && c2 == 6 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 17:
        c1      = (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 );
        c2      = (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x, y + 1, z );
        x--;
        c1     += (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 );
        c2     += (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x, y + 1, z );
        x++;
//                x+=2;
//                c1     += (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 );
//                c2     += (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x, y + 1, z );
//                x--;
//                if ( c1 == 0 && c2 == 9 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        if ( c1 == 0 && c2 == 6 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;
*/
/*
                                        // Middle of the edge - not so good
        case  6:
        c1      = (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x + 1, y + 1, z + 1 ) + (bool) temp.GetValue ( x + 1, y + 1, z - 1 );
        c2      = (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x, y, z + 1 )          + (bool) temp.GetValue ( x, y, z - 1 );
        if ( c1 == 0 && c2 == 5 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case  7:
        c1      = (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x + 1, y + 1, z + 1 ) + (bool) temp.GetValue ( x + 1, y - 1, z + 1 );
        c2      = (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x, y + 1, z )          + (bool) temp.GetValue ( x, y - 1, z );
        if ( c1 == 0 && c2 == 5 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case  8:
        c1      = (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x + 1, y + 1, z + 1 ) + (bool) temp.GetValue ( x - 1, y + 1, z + 1 );
        c2      = (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x + 1, y, z )          + (bool) temp.GetValue ( x - 1, y, z );
        if ( c1 == 0 && c2 == 5 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;


        case  9:
        c1      = (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x + 1, y - 1, z + 1 ) + (bool) temp.GetValue ( x + 1, y - 1, z - 1 );
        c2      = (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x, y, z + 1 )         + (bool) temp.GetValue ( x, y, z - 1 );
        if ( c1 == 0 && c2 == 5 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 10:
        c1      = (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x + 1, y + 1, z - 1 ) + (bool) temp.GetValue ( x + 1, y - 1, z - 1 );
        c2      = (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x, y + 1, z )         + (bool) temp.GetValue ( x, y - 1, z );
        if ( c1 == 0 && c2 == 5 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 11:
        c1      = (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x + 1, y + 1, z - 1 ) + (bool) temp.GetValue ( x - 1, y + 1, z - 1 );
        c2      = (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y, z ) +         (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x + 1, y, z )         + (bool) temp.GetValue ( x - 1, y, z );
        if ( c1 == 0 && c2 == 5 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;


        case 12:
        c1      = (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x - 1, y + 1, z + 1 ) + (bool) temp.GetValue ( x - 1, y + 1, z - 1 );
        c2      = (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x, y, z + 1 )         + (bool) temp.GetValue ( x, y, z - 1 );
        if ( c1 == 0 && c2 == 5 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 13:
        c1      = (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x - 1, y + 1, z + 1 ) + (bool) temp.GetValue ( x - 1, y - 1, z + 1 );
        c2      = (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x, y + 1, z )         + (bool) temp.GetValue ( x, y - 1, z );
        if ( c1 == 0 && c2 == 5 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 14:
        c1      = (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x + 1, y - 1, z + 1 ) + (bool) temp.GetValue ( x - 1, y - 1, z + 1 );
        c2      = (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x + 1, y, z )         + (bool) temp.GetValue ( x - 1, y, z );
        if ( c1 == 0 && c2 == 5 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;


        case 15:
        c1      = (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x - 1, y - 1, z + 1 ) + (bool) temp.GetValue ( x - 1, y - 1, z - 1 );
        c2      = (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x, y, z + 1 )         + (bool) temp.GetValue ( x, y, z - 1 );
        if ( c1 == 0 && c2 == 5 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 16:
        c1      = (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x - 1, y + 1, z - 1 ) + (bool) temp.GetValue ( x - 1, y - 1, z - 1 );
        c2      = (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x, y + 1, z )         + (bool) temp.GetValue ( x, y - 1, z );
        if ( c1 == 0 && c2 == 5 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 17:
        c1      = (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x + 1, y - 1, z - 1 ) + (bool) temp.GetValue ( x - 1, y - 1, z - 1 );
        c2      = (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x + 1, y, z )         + (bool) temp.GetValue ( x - 1, y, z );
        if ( c1 == 0 && c2 == 5 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;
*/

                                        // Simpler middle of edge - quite efficient, maybe too much?
        case  6:
        c1      = (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z );
        c2      = (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y - 1, z );
        if ( c1 == 0 && c2 == 4 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case  7:
        c1      = (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 );
        c2      = (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y, z - 1 );
        if ( c1 == 0 && c2 == 4 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case  8:
        c1      = (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 );
        c2      = (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z - 1 );
        if ( c1 == 0 && c2 == 4 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;


        case  9:
        c1      = (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z );
        c2      = (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y + 1, z );
        if ( c1 == 0 && c2 == 4 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 10:
        c1      = (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 );
        c2      = (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y, z + 1 );
        if ( c1 == 0 && c2 == 4 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 11:
        c1      = (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 );
        c2      = (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y, z ) +         (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z + 1 );
        if ( c1 == 0 && c2 == 4 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;


        case 12:
        c1      = (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z );
        c2      = (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y - 1, z );
        if ( c1 == 0 && c2 == 4 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 13:
        c1      = (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 );
        c2      = (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y, z - 1 );
        if ( c1 == 0 && c2 == 4 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 14:
        c1      = (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z + 1 ) + (bool) temp.GetValue ( x, y, z + 1 );
        c2      = (bool) temp.GetValue ( x, y, z - 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z - 1 );
        if ( c1 == 0 && c2 == 4 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;


        case 15:
        c1      = (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z );
        c2      = (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y + 1, z );
        if ( c1 == 0 && c2 == 4 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 16:
        c1      = (bool) temp.GetValue ( x - 1, y, z ) + (bool) temp.GetValue ( x - 1, y, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 );
        c2      = (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x + 1, y, z ) + (bool) temp.GetValue ( x + 1, y, z + 1 );
        if ( c1 == 0 && c2 == 4 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 17:
        c1      = (bool) temp.GetValue ( x, y - 1, z ) + (bool) temp.GetValue ( x, y - 1, z - 1 ) + (bool) temp.GetValue ( x, y, z - 1 );
        c2      = (bool) temp.GetValue ( x, y, z + 1 ) + (bool) temp.GetValue ( x, y, z )         + (bool) temp.GetValue ( x, y + 1, z ) + (bool) temp.GetValue ( x, y + 1, z + 1 );
        if ( c1 == 0 && c2 == 4 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;



                                        // 3) Corners
        case 18:
        c1      = (bool) temp.GetValue ( x + 1, y + 1, z + 1 ) + (bool) temp.GetValue ( x + 1, y + 1, z ) + (bool) temp.GetValue ( x + 1, y, z + 1 ) + (bool) temp.GetValue ( x, y + 1, z + 1 );
        c2      = (bool) temp.GetValue ( x, y, z )             + (bool) temp.GetValue ( x, y, z - 1 )     + (bool) temp.GetValue ( x, y - 1, z )     + (bool) temp.GetValue ( x - 1, y, z );
        if ( c1 == 0 && c2 == 4 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 19:
        c1      = (bool) temp.GetValue ( x + 1, y + 1, z - 1 ) + (bool) temp.GetValue ( x + 1, y + 1, z ) + (bool) temp.GetValue ( x + 1, y, z - 1 ) + (bool) temp.GetValue ( x, y + 1, z - 1 );
        c2      = (bool) temp.GetValue ( x, y, z )             + (bool) temp.GetValue ( x, y, z + 1 )     + (bool) temp.GetValue ( x, y - 1, z )     + (bool) temp.GetValue ( x - 1, y, z );
        if ( c1 == 0 && c2 == 4 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 20:
        c1      = (bool) temp.GetValue ( x + 1, y - 1, z + 1 ) + (bool) temp.GetValue ( x + 1, y - 1, z ) + (bool) temp.GetValue ( x + 1, y, z + 1 ) + (bool) temp.GetValue ( x, y - 1, z + 1 );
        c2      = (bool) temp.GetValue ( x, y, z )             + (bool) temp.GetValue ( x, y, z - 1 )     + (bool) temp.GetValue ( x, y + 1, z )     + (bool) temp.GetValue ( x - 1, y, z );
        if ( c1 == 0 && c2 == 4 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 21:
        c1      = (bool) temp.GetValue ( x + 1, y - 1, z - 1 ) + (bool) temp.GetValue ( x + 1, y - 1, z ) + (bool) temp.GetValue ( x + 1, y, z - 1 ) + (bool) temp.GetValue ( x, y - 1, z - 1 );
        c2      = (bool) temp.GetValue ( x, y, z )             + (bool) temp.GetValue ( x, y, z + 1 )     + (bool) temp.GetValue ( x, y + 1, z )     + (bool) temp.GetValue ( x - 1, y, z );
        if ( c1 == 0 && c2 == 4 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 22:
        c1      = (bool) temp.GetValue ( x - 1, y + 1, z + 1 ) + (bool) temp.GetValue ( x - 1, y + 1, z ) + (bool) temp.GetValue ( x - 1, y, z + 1 ) + (bool) temp.GetValue ( x, y + 1, z + 1 );
        c2      = (bool) temp.GetValue ( x, y, z )             + (bool) temp.GetValue ( x, y, z - 1 )     + (bool) temp.GetValue ( x, y - 1, z )     + (bool) temp.GetValue ( x + 1, y, z );
        if ( c1 == 0 && c2 == 4 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 23:
        c1      = (bool) temp.GetValue ( x - 1, y + 1, z - 1 ) + (bool) temp.GetValue ( x - 1, y + 1, z ) + (bool) temp.GetValue ( x - 1, y, z - 1 ) + (bool) temp.GetValue ( x, y + 1, z - 1 );
        c2      = (bool) temp.GetValue ( x, y, z )             + (bool) temp.GetValue ( x, y, z + 1 )     + (bool) temp.GetValue ( x, y - 1, z )     + (bool) temp.GetValue ( x + 1, y, z );
        if ( c1 == 0 && c2 == 4 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 24:
        c1      = (bool) temp.GetValue ( x - 1, y - 1, z + 1 ) + (bool) temp.GetValue ( x - 1, y - 1, z ) + (bool) temp.GetValue ( x - 1, y, z + 1 ) + (bool) temp.GetValue ( x, y - 1, z + 1 );
        c2      = (bool) temp.GetValue ( x, y, z )             + (bool) temp.GetValue ( x, y, z - 1 )     + (bool) temp.GetValue ( x, y + 1, z )     + (bool) temp.GetValue ( x + 1, y, z );
        if ( c1 == 0 && c2 == 4 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;

        case 25:
        c1      = (bool) temp.GetValue ( x - 1, y - 1, z - 1 ) + (bool) temp.GetValue ( x - 1, y - 1, z ) + (bool) temp.GetValue ( x - 1, y, z - 1 ) + (bool) temp.GetValue ( x, y - 1, z - 1 );
        c2      = (bool) temp.GetValue ( x, y, z )             + (bool) temp.GetValue ( x, y, z + 1 )     + (bool) temp.GetValue ( x, y + 1, z )     + (bool) temp.GetValue ( x + 1, y, z );
        if ( c1 == 0 && c2 == 4 )       Array[ IndexesToLinearIndex ( x - 1, y - 1, z - 1 ) ]   = 0;
        break;


        } // switch

        } // for x, y, z


                                        // no more evolution?
    oldnumset   = numset;
    numset      = GetNumSet ();

    if ( numset == oldnumset )
        nomore++;                       // make sure each filter has no effect, so count through them
    else
        nomore  = 0;

    if ( nomore >= numfilters )
        break;

    if ( showprogress )
        if ( converge )     Gauge.SetValue ( SuperGaugeDefaultPart, Percentage ( ( maxnumset - numset ) * 1.15, maxnumset ) );
        else                Gauge.Next ();
    } // for repeat
}


//----------------------------------------------------------------------------
/*
template <class TypeD>
void    TVolume<TypeD>::FilterWaterfall ( int filtertype, FctParams& /*params* /, bool showprogress )
{
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We need a mask for the head only
TVolume<TypeD>      headmask ( *this );
FctParams           p;

p ( FilterParamToMaskThreshold )     = GetBackgroundValue ();
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
headmask.Filter ( FilterTypeToMask, p );


p ( FilterParamDiameter )     = 1;
headmask.Filter ( FilterTypeErode, p );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//TVolume<TypeD>      original    ( *this );
TVolume<TypeD>      edge        ( Dim1, Dim2, Dim3 );
TVolume<TypeD>      done        ( Dim1, Dim2, Dim3 );
TVolume<TypeD>      newdone     ( Dim1, Dim2, Dim3 );
//TVolume<TypeD>      height      ( Dim1, Dim2, Dim3 );


bool                ridges          = filtertype == FilterTypeWaterfallRidges;
double              maxlevel        = GetMaxValue ();
int                 firstl          = ridges ? 0 : maxlevel;
int                 incl            = ridges ? 1 : -1;
bool                edgegrown;
int                 l;
double              w;


if ( maxlevel == 0 )
    return;


TSuperGauge         Gauge ( FilterPresets[ filtertype ].Text, showprogress ? Dim1 : 0 );


                                        // grow from the 0 to max level
for ( l = firstl; l >= 0 && l <= maxlevel; l += incl ) {

                                        // value between 0..100
    w       = ridges ? (double) l / maxlevel * 100 : (double) ( maxlevel - l ) / maxlevel * 100;

    Gauge.SetValue ( SuperGaugeDefaultPart, Percentage ( w + 1, 100 ) );


                                        // ~region growing of current edge, @ current level
    do {
        edgegrown   = false;
        newdone.Insert ( done );


        for ( int x = 0; x < Dim1; x++ )
        for ( int y = 0; y < Dim2; y++ )
        for ( int z = 0; z < Dim3; z++ ) {

                                        // already done, or not from current level?
            if ( ! headmask ( x, y,z ) || done ( x, y, z ) || GetValue ( x, y, z ) != l )
                continue;

                                        // not an edge?
            if ( done.GetNumNeighbors ( x, y, z, Neighbors26 ) == 0 )
                continue;

                                        // grow edge!
            newdone ( x, y, z )     = true;     // don't update original voxels now, as we also test # of neighbors from it...
//            height  ( x, y, z )     = w;
            edgegrown               = true;

                                        // now, test for new bridges

                                        // voxels between 2 done
                                        // 6 neighbors
            if ( done.GetValueChecked ( x - 1, y, z ) ) {
                if      ( done.GetValueChecked ( x + 1, y, z ) ) edge ( x, y, z )    = w;
                else if ( done.GetValueChecked ( x + 2, y, z ) ) edge ( x, y, z )    = edge ( x + 1, y, z )  = w;
                }

            if ( done.GetValueChecked ( x, y - 1, z ) ) {
                if      ( done.GetValueChecked ( x, y + 1, z ) ) edge ( x, y, z )    = w;
                else if ( done.GetValueChecked ( x, y + 2, z ) ) edge ( x, y, z )    = edge ( x, y + 1, z )  = w;
                }

            if ( done.GetValueChecked ( x, y, z - 1 ) ) {
                if      ( done.GetValueChecked ( x, y, z + 1 ) ) edge ( x, y, z )    = w;
                else if ( done.GetValueChecked ( x, y, z + 2 ) ) edge ( x, y, z )    = edge ( x, y, z + 1 )  = w;
                }


                                        // 18 neighbors
            if ( done.GetValueChecked ( x - 1, y - 1, z ) ) {
                if      ( done.GetValueChecked ( x + 1, y + 1, z ) ) edge ( x, y, z )    = w;
                else if ( done.GetValueChecked ( x + 2, y + 2, z ) ) edge ( x, y, z )    = edge ( x + 1, y + 1, z )  = w;
                }

            if ( done.GetValueChecked ( x - 1, y + 1, z ) ) {
                if      ( done.GetValueChecked ( x + 1, y - 1, z ) ) edge ( x, y, z )    = w;
                else if ( done.GetValueChecked ( x + 2, y - 2, z ) ) edge ( x, y, z )    = edge ( x + 1, y - 1, z )  = w;
                }

            if ( done.GetValueChecked ( x - 1, y, z - 1 ) ) {
                if      ( done.GetValueChecked ( x + 1, y, z + 1 ) ) edge ( x, y, z )    = w;
                else if ( done.GetValueChecked ( x + 2, y, z + 2 ) ) edge ( x, y, z )    = edge ( x + 1, y, z + 1 )  = w;
                }

            if ( done.GetValueChecked ( x - 1, y, z + 1 ) ) {
                if      ( done.GetValueChecked ( x + 1, y, z - 1 ) ) edge ( x, y, z )    = w;
                else if ( done.GetValueChecked ( x + 2, y, z - 2 ) ) edge ( x, y, z )    = edge ( x + 1, y, z - 1 )  = w;
                }

            if ( done.GetValueChecked ( x, y - 1, z - 1 ) ) {
                if      ( done.GetValueChecked ( x, y + 1, z + 1 ) ) edge ( x, y, z )    = w;
                else if ( done.GetValueChecked ( x, y + 2, z + 2 ) ) edge ( x, y, z )    = edge ( x, y + 1, z + 1 )  = w;
                }

            if ( done.GetValueChecked ( x, y - 1, z + 1 ) ) {
                if      ( done.GetValueChecked ( x, y + 1, z - 1 ) ) edge ( x, y, z )    = w;
                else if ( done.GetValueChecked ( x, y + 2, z - 2 ) ) edge ( x, y, z )    = edge ( x, y + 1, z - 1 )  = w;
                }


                                        // 26 neighbors
            if ( done.GetValueChecked ( x - 1, y - 1, z - 1 ) ) {
                if      ( done.GetValueChecked ( x + 1, y + 1, z + 1 ) ) edge ( x, y, z )    = w;
                else if ( done.GetValueChecked ( x + 2, y + 2, z + 2 ) ) edge ( x, y, z )    = edge ( x + 1, y + 1, z + 1 )  = w;
                }

            if ( done.GetValueChecked ( x - 1, y - 1, z + 1 ) ) {
                if      ( done.GetValueChecked ( x + 1, y + 1, z - 1 ) ) edge ( x, y, z )    = w;
                else if ( done.GetValueChecked ( x + 2, y + 2, z - 2 ) ) edge ( x, y, z )    = edge ( x + 1, y + 1, z - 1 )  = w;
                }

            if ( done.GetValueChecked ( x - 1, y + 1, z - 1 ) ) {
                if      ( done.GetValueChecked ( x + 1, y - 1, z + 1 ) ) edge ( x, y, z )    = w;
                else if ( done.GetValueChecked ( x + 2, y - 2, z + 2 ) ) edge ( x, y, z )    = edge ( x + 1, y - 1, z + 1 )  = w;
                }

            if ( done.GetValueChecked ( x - 1, y + 1, z + 1 ) ) {
                if      ( done.GetValueChecked ( x + 1, y - 1, z - 1 ) ) edge ( x, y, z )    = w;
                else if ( done.GetValueChecked ( x + 2, y - 2, z - 2 ) ) edge ( x, y, z )    = edge ( x + 1, y - 1, z - 1 )  = w;
                }


/*                                        // conditionnally insert new edges
                                        // 6 neighbors
            #define     reldiff     0.05

            if ( done.GetValueChecked ( x - 1, y, z ) ) {
                if      ( done.GetValueChecked ( x + 1, y, z ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x + 1, y, z ) ) > reldiff ) edge ( x, y, z )    = w; }
                else if ( done.GetValueChecked ( x + 2, y, z ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x + 2, y, z ) ) > reldiff ) edge ( x, y, z )    = edge ( x + 1, y, z )  = w; }
                }

            if ( done.GetValueChecked ( x, y - 1, z ) ) {
                if      ( done.GetValueChecked ( x, y + 1, z ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x, y + 1, z ) ) > reldiff ) edge ( x, y, z )    = w; }
                else if ( done.GetValueChecked ( x, y + 2, z ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x, y + 2, z ) ) > reldiff ) edge ( x, y, z )    = edge ( x, y + 1, z )  = w; }
                }

            if ( done.GetValueChecked ( x, y, z - 1 ) ) {
                if      ( done.GetValueChecked ( x, y, z + 1 ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x, y, z + 1 ) ) > reldiff ) edge ( x, y, z )    = w; }
                else if ( done.GetValueChecked ( x, y, z + 2 ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x, y, z + 2 ) ) > reldiff ) edge ( x, y, z )    = edge ( x, y, z + 1 )  = w; }
                }


                                        // 18 neighbors
            if ( done.GetValueChecked ( x - 1, y - 1, z ) ) {
                if      ( done.GetValueChecked ( x + 1, y + 1, z ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x + 1, y + 1, z ) ) > reldiff )  edge ( x, y, z )    = w; }
                else if ( done.GetValueChecked ( x + 2, y + 2, z ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x + 2, y + 2, z ) ) > reldiff ) edge ( x, y, z )    = edge ( x + 1, y + 1, z )  = w; }
                }

            if ( done.GetValueChecked ( x - 1, y + 1, z ) ) {
                if      ( done.GetValueChecked ( x + 1, y - 1, z ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x + 1, y - 1, z ) ) > reldiff ) edge ( x, y, z )    = w; }
                else if ( done.GetValueChecked ( x + 2, y - 2, z ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x + 2, y - 2, z ) ) > reldiff ) edge ( x, y, z )    = edge ( x + 1, y - 1, z )  = w; }
                }

            if ( done.GetValueChecked ( x - 1, y, z - 1 ) ) {
                if      ( done.GetValueChecked ( x + 1, y, z + 1 ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x + 1, y, z + 1 ) ) > reldiff ) edge ( x, y, z )    = w; }
                else if ( done.GetValueChecked ( x + 2, y, z + 2 ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x + 2, y, z + 2 ) ) > reldiff ) edge ( x, y, z )    = edge ( x + 1, y, z + 1 )  = w; }
                }

            if ( done.GetValueChecked ( x - 1, y, z + 1 ) ) {
                if      ( done.GetValueChecked ( x + 1, y, z - 1 ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x + 1, y, z - 1 ) ) > reldiff ) edge ( x, y, z )    = w; }
                else if ( done.GetValueChecked ( x + 2, y, z - 2 ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x + 2, y, z - 2 ) ) > reldiff ) edge ( x, y, z )    = edge ( x + 1, y, z - 1 )  = w; }
                }

            if ( done.GetValueChecked ( x, y - 1, z - 1 ) ) {
                if      ( done.GetValueChecked ( x, y + 1, z + 1 ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x, y + 1, z + 1 ) ) > reldiff ) edge ( x, y, z )    = w; }
                else if ( done.GetValueChecked ( x, y + 2, z + 2 ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x, y + 2, z + 2 ) ) > reldiff ) edge ( x, y, z )    = edge ( x, y + 1, z + 1 )  = w; }
                }

            if ( done.GetValueChecked ( x, y - 1, z + 1 ) ) {
                if      ( done.GetValueChecked ( x, y + 1, z - 1 ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x, y + 1, z - 1 ) ) > reldiff ) edge ( x, y, z )    = w; }
                else if ( done.GetValueChecked ( x, y + 2, z - 2 ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x, y + 2, z - 2) ) > reldiff ) edge ( x, y, z )    = edge ( x, y + 1, z - 1 )  = w; }
                }


                                        // 26 neighbors
            if ( done.GetValueChecked ( x - 1, y - 1, z - 1 ) ) {
                if      ( done.GetValueChecked ( x + 1, y + 1, z + 1 ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x + 1, y + 1, z + 1 ) ) > reldiff ) edge ( x, y, z )    = w; }
                else if ( done.GetValueChecked ( x + 2, y + 2, z + 2 ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x + 2, y + 2, z + 2 ) ) > reldiff ) edge ( x, y, z )    = edge ( x + 1, y + 1, z + 1 )  = w; }
                }

            if ( done.GetValueChecked ( x - 1, y - 1, z + 1 ) ) {
                if      ( done.GetValueChecked ( x + 1, y + 1, z - 1 ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x + 1, y + 1, z - 1 ) ) > reldiff ) edge ( x, y, z )    = w; }
                else if ( done.GetValueChecked ( x + 2, y + 2, z - 2 ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x + 2, y + 2, z - 2 ) ) > reldiff ) edge ( x, y, z )    = edge ( x + 1, y + 1, z - 1 )  = w; }
                }

            if ( done.GetValueChecked ( x - 1, y + 1, z - 1 ) ) {
                if      ( done.GetValueChecked ( x + 1, y - 1, z + 1 ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x + 1, y - 1, z + 1 ) ) > reldiff ) edge ( x, y, z )    = w; }
                else if ( done.GetValueChecked ( x + 2, y - 2, z + 2 ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x + 2, y - 2, z + 2 ) ) > reldiff ) edge ( x, y, z )    = edge ( x + 1, y - 1, z + 1 )  = w; }
                }

            if ( done.GetValueChecked ( x - 1, y + 1, z + 1 ) ) {
                if      ( done.GetValueChecked ( x + 1, y - 1, z - 1 ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x + 1, y - 1, z - 1 ) ) > reldiff ) edge ( x, y, z )    = w; }
                else if ( done.GetValueChecked ( x + 2, y - 2, z - 2 ) ) { if ( RelativeDifference ( w, height.GetValueChecked ( x + 2, y - 2, z - 2 ) ) > reldiff ) edge ( x, y, z )    = edge ( x + 1, y - 1, z - 1 )  = w; }
                }
* /
            } // for x, y, z


        if ( edgegrown )
            done.Insert ( newdone );

                                        // debugging!
//        Insert ( edge );

        } while ( edgegrown ); // do grow level


                                        // edges have been grown, create new seeds which have no neighbors
    for ( int x = 0; x < Dim1; x++ )
    for ( int y = 0; y < Dim2; y++ )
    for ( int z = 0; z < Dim3; z++ )

        if ( headmask ( x, y,z ) && ! done ( x, y, z ) && GetValue ( x, y, z ) == l ) {
            done   ( x, y, z )  = true;
//            height ( x, y, z )  = w;
            }

    } // for l


//edge.ApplyMaskToData ( headmask );

                                        // copy resulting edges
Insert ( edge );
}
*/

template <class TypeD>
void    TVolume<TypeD>::FilterWaterfall ( FilterTypes filtertype, FctParams& /*params*/, bool showprogress )
{
                                        // We need a mask for the head only
TVolume<TypeD>      headmask ( *this );
FctParams           p;

p ( FilterParamToMaskThreshold )     = GetBackgroundValue ();
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
headmask.Filter ( FilterTypeToMask, p );


p ( FilterParamDiameter )     = 1;
headmask.Filter ( FilterTypeDilate, p );
//headmask.Filter ( FilterTypeErode, p );

                                        // we can restrict the processing to a smaller interval
TBoundingBox<int>   bounding ( headmask, false, 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//TVolume<TypeD>    original    ( *this );
TVolume<TypeD>      edge        ( Dim1, Dim2, Dim3 );
TVolume<uchar>      done        ( Dim1, Dim2, Dim3 );
TVolume<uchar>      newdone     ( Dim1, Dim2, Dim3 );
TVolume<int>        region      ( Dim1, Dim2, Dim3 );
TVolume<TypeD>      height      ( Dim1, Dim2, Dim3 );


bool                ridges          = filtertype == FilterTypeWaterfallRidges;
TypeD               maxlevel        = GetMaxValue ();
TypeD               minlevel        = GetMinValue ();
TypeD               firstl          = ridges ? minlevel : maxlevel;
//int                 incl            = ridges ? 1 : -1;  // or some delta?
TypeD               delta           = ( maxlevel - minlevel ) / 255;
TypeD               incl            = ridges ? delta : -delta;
bool                edgegrown;
TypeD               w;
int                 nextregioni     = 0;


if ( maxlevel == 0 )
    return;


TSuperGauge         Gauge ( FilterPresets[ filtertype ].Text, showprogress ? 100 : 0 );


                                        // grow from the 0 to max level
for ( TypeD l = firstl; IsInsideLimits ( l, minlevel, maxlevel ); l += incl ) {

                                        // increasing values
    w       = ridges ? l : maxlevel - l;

    Gauge.SetValue ( SuperGaugeDefaultPart, Percentage ( w + 1, maxlevel ) );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) ~region growing of current edge, @ current level
    do {
        edgegrown   = false;
        newdone     = done;
                                        // Not very efficient, still does a boost
        OmpParallelFor

        for ( int x = bounding.XMin (); x <= bounding.XMax (); x ++ )
        for ( int y = bounding.YMin (); y <= bounding.YMax (); y ++ )
        for ( int z = bounding.ZMin (); z <= bounding.ZMax (); z ++ ) {

                                        // already done, or not from current level?
//          if ( ! headmask ( x, y,z ) || done ( x, y, z ) || GetValue ( x, y, z ) != l )
            if ( ! headmask ( x, y,z ) || done ( x, y, z ) || fabs ( GetValue ( x, y, z ) - l ) > delta )   // ????? to be tested ?????
                continue;

                                        // not an edge?
            if ( done.GetNumNeighbors ( x, y, z, Neighbors6 ) == 0 )
//          if ( done.GetNumNeighbors ( x, y, z, Neighbors18 ) == 0 )
//          if ( done.GetNumNeighbors ( x, y, z, Neighbors26 ) == 0 )
                continue;

                                        // grow edge!
            newdone ( x, y, z )     = true;     // don't update original voxels now, as we also test # of neighbors from it...
            edgegrown               = true;     // global flag
            TypeD       newh        = w;

                                        // allocate new voxel to best neighborhood bidder
            auto    IsBestNeighbor  = [ & ] ( int X, int Y, int Z )    { if ( height ( X, Y, Z ) && height ( X, Y, Z ) < newh )  { newh = height ( X, Y, Z );  region ( x, y, z ) = region ( X, Y, Z ); } };

                                        // 6 neighbors
            IsBestNeighbor ( x + 1, y, z );
            IsBestNeighbor ( x - 1, y, z );
            IsBestNeighbor ( x, y + 1, z );
            IsBestNeighbor ( x, y - 1, z );
            IsBestNeighbor ( x, y, z + 1 );
            IsBestNeighbor ( x, y, z - 1 );
/*                                        // 18 neighbors
            IsBestNeighbor ( x + 1, y + 1, z );
            IsBestNeighbor ( x + 1, y - 1, z );
            IsBestNeighbor ( x - 1, y + 1, z );
            IsBestNeighbor ( x - 1, y - 1, z );

            IsBestNeighbor ( x + 1, y, z + 1 );
            IsBestNeighbor ( x + 1, y, z - 1 );
            IsBestNeighbor ( x - 1, y, z + 1 );
            IsBestNeighbor ( x - 1, y, z - 1 );

            IsBestNeighbor ( x, y + 1, z + 1 );
            IsBestNeighbor ( x, y + 1, z - 1 );
            IsBestNeighbor ( x, y - 1, z + 1 );
            IsBestNeighbor ( x, y - 1, z - 1 );
*/
/*                                        // 26 neighbors
            IsBestNeighbor ( x + 1, y + 1, z + 1 );
            IsBestNeighbor ( x + 1, y + 1, z - 1 );
            IsBestNeighbor ( x + 1, y - 1, z + 1 );
            IsBestNeighbor ( x + 1, y - 1, z - 1 );
            IsBestNeighbor ( x - 1, y + 1, z + 1 );
            IsBestNeighbor ( x - 1, y + 1, z - 1 );
            IsBestNeighbor ( x - 1, y - 1, z + 1 );
            IsBestNeighbor ( x - 1, y - 1, z - 1 );
*/
            height  ( x, y, z )     = newh;

            } // for x, y, z


        if ( edgegrown )
            done    = newdone;

        } while ( edgegrown ); // do grow level


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) edges have been grown, create new seeds which have no neighbors
    for ( int x = bounding.XMin (); x <= bounding.XMax (); x ++ )
    for ( int y = bounding.YMin (); y <= bounding.YMax (); y ++ )
    for ( int z = bounding.ZMin (); z <= bounding.ZMax (); z ++ )
                                        // each new voxel is a region, fusion will take place at next step
//      if ( headmask ( x, y,z ) && ! done ( x, y, z ) && GetValue ( x, y, z ) == l ) {
        if ( headmask ( x, y,z ) && ! done ( x, y, z ) && fabs ( GetValue ( x, y, z ) - l ) <= delta ) {
            region ( x, y, z )  = ++nextregioni;
            done   ( x, y, z )  = true;
            height ( x, y, z )  = w;
            }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) check for boundaries -> fusion or edges
//  double      reldiff     = ridges ? 0.05 : 0.30;
    double      reldiff     = ridges ? 0.99 : 0.30;
//    #define     absdiff     30
                                        // Not very efficient, still does a boost
    OmpParallelFor

    for ( int x = AtLeast ( 1, bounding.XMin () ); x <= NoMore ( Dim1 - 2, bounding.XMax () ); x ++ )
    for ( int y = AtLeast ( 1, bounding.YMin () ); y <= NoMore ( Dim2 - 2, bounding.YMax () ); y ++ )
    for ( int z = AtLeast ( 1, bounding.ZMin () ); z <= NoMore ( Dim3 - 2, bounding.ZMax () ); z ++ ) {

                                        // not done?
        if ( ! headmask ( x, y,z ) || ! done ( x, y, z ) )
////        if ( ! headmask ( x, y,z ) || ! done ( x, y, z ) || GetValue ( x, y, z ) != l )
//        if ( ! headmask ( x, y,z ) || ! done ( x, y, z ) || fabs ( GetValue ( x, y, z ) - l ) > delta )
            continue;

                                        // test neighborhood

//            if ( w - height ( x + 1, y, z ) < absdiff && height ( x, y, z ) < height ( x + 1, y, z ) ) {
//            if ( ( ( ridges && RelativeDifference ( w, height ( X, Y, Z ) ) > reldiff ) || ( ! ridges && RelativeDifference ( w, height ( X, Y, Z ) ) < reldiff ) )
//            if ( RelativeDifference ( w, height ( X, Y, Z ) ) < reldiff )


#define FusionOrEdge(X,Y,Z) \
        if ( region ( X, Y, Z ) && region ( x, y, z ) != region ( X, Y, Z ) ) { \
            if ( ( RelativeDifference ( w, height ( X, Y, Z ) ) < reldiff ) \
              && height ( x, y, z ) < height ( X, Y, Z ) ) { \
                int     toremap     = region ( X, Y, Z ); \
                int     newreg      = region ( x, y, z ); \
                TypeD   newh        = height ( x, y, z ); \
                edge ( x, y, z )    = 0; \
 \
                for ( int i = 0; i < LinearDim; i++ ) \
                    if ( region[ i ] == toremap ) { \
                        region[ i ] = newreg; \
                        height[ i ] = newh; \
                        } \
                } \
            else \
                edge ( x, y, z )    = w; \
            }
                                        // 6 neighbors
        FusionOrEdge ( x + 1, y, z );
        FusionOrEdge ( x - 1, y, z );
        FusionOrEdge ( x, y + 1, z );
        FusionOrEdge ( x, y - 1, z );
        FusionOrEdge ( x, y, z + 1 );
        FusionOrEdge ( x, y, z - 1 );
/*                                        // 18 neighbors
        FusionOrEdge ( x + 1, y + 1, z );
        FusionOrEdge ( x + 1, y - 1, z );
        FusionOrEdge ( x - 1, y + 1, z );
        FusionOrEdge ( x - 1, y - 1, z );

        FusionOrEdge ( x + 1, y, z + 1 );
        FusionOrEdge ( x + 1, y, z - 1 );
        FusionOrEdge ( x - 1, y, z + 1 );
        FusionOrEdge ( x - 1, y, z - 1 );

        FusionOrEdge ( x, y + 1, z + 1 );
        FusionOrEdge ( x, y + 1, z - 1 );
        FusionOrEdge ( x, y - 1, z + 1 );
        FusionOrEdge ( x, y - 1, z - 1 );
*/
/*                                        // 26 neighbors
        FusionOrEdge ( x + 1, y + 1, z + 1 );
        FusionOrEdge ( x + 1, y + 1, z - 1 );
        FusionOrEdge ( x + 1, y - 1, z + 1 );
        FusionOrEdge ( x + 1, y - 1, z - 1 );
        FusionOrEdge ( x - 1, y + 1, z + 1 );
        FusionOrEdge ( x - 1, y + 1, z - 1 );
        FusionOrEdge ( x - 1, y - 1, z + 1 );
        FusionOrEdge ( x - 1, y - 1, z - 1 );
*/
        } // for x, y, z


                                        // debugging!
//    Insert ( edge );
    } // for l


//edge.ApplyMaskToData ( headmask );

                                        // copy resulting edges
Insert ( edge );

                                        // show the regions !!!!!!!
//for ( int i = 0; i < LinearDim; i++ ) {
//    GetValue ( i ) = (uchar) ( region ( i ) ? region ( i ) % 253 + 1 : 0 );
////    if ( edge ( i ) )   GetValue ( i ) = (uchar) 255;
//    }

}


//----------------------------------------------------------------------------
                                        // this  Operation=  operand2
template <class TypeD>
void    TVolume<TypeD>::BinaryOp    (   TVolume<TypeD>&     operand2,   
                                        ArrayOperandType    typeoperand1,   ArrayOperandType        typeoperand2, 
                                        ArrayOperationType  typeoperation   
                                    )
{
                                        // light checking
if ( IsNotAllocated () || LinearDim != operand2.GetLinearDim () )
    return;

TypeD               minv            = Lowest  ( minv );
TypeD               maxv            = Highest ( maxv );

OmpParallelFor

for ( int i = 0; i < LinearDim; i++ )

    if      ( typeoperation == OperationAnd ) {
        if      ( typeoperand1 == OperandMask && typeoperand2 == OperandMask )      Array[ i ] = (TypeD) ( Array[ i ] && operand2[ i ] );
        else if ( typeoperand1 == OperandMask && typeoperand2 == OperandData )      Array[ i ] = (TypeD) ( Array[ i ] ? operand2[ i ] : 0 );
        else if ( typeoperand1 == OperandData && typeoperand2 == OperandMask )      Array[ i ] = (TypeD) ( operand2[ i ] ? Array[ i ] : 0 );
        else if ( typeoperand1 == OperandData && typeoperand2 == OperandData )      Array[ i ] = (TypeD) ( min ( Array[ i ], operand2[ i ] ) ); // not clear what to do?
        }

    else if ( typeoperation == OperationOr ) {
        if      ( typeoperand1 == OperandMask && typeoperand2 == OperandMask )      Array[ i ] = (TypeD) ( Array[ i ] || operand2[ i ] );
        else if ( typeoperand1 == OperandMask && typeoperand2 == OperandData )      Array[ i ] = (TypeD) ( operand2[ i ] ? operand2[ i ] : Array[ i ]    ); // priority to data value (operand 2)
        else if ( typeoperand1 == OperandData && typeoperand2 == OperandMask )      Array[ i ] = (TypeD) ( Array[ i ]    ? Array[ i ]    : operand2[ i ] ); // priority to data value (operand 1)
        else if ( typeoperand1 == OperandData && typeoperand2 == OperandData )      Array[ i ] = (TypeD) ( max ( Array[ i ], operand2[ i ] ) );             // max of value
        }

    else if ( typeoperation == OperationXor ) {
        if      ( typeoperand1 == OperandMask && typeoperand2 == OperandMask )      Array[ i ] = (TypeD) ( (bool) Array[ i ] ^ (bool) operand2[ i ] );
        else if ( typeoperand1 == OperandMask && typeoperand2 == OperandData )      Array[ i ] = (TypeD) ( Array[ i ] && ! operand2[ i ] ? true       : ! Array[ i ] && operand2[ i ] ? operand2[ i ] : 0 );
        else if ( typeoperand1 == OperandData && typeoperand2 == OperandMask )      Array[ i ] = (TypeD) ( Array[ i ] && ! operand2[ i ] ? Array[ i ] : ! Array[ i ] && operand2[ i ] ? true          : 0 );
        else if ( typeoperand1 == OperandData && typeoperand2 == OperandData )      Array[ i ] = (TypeD) ( Array[ i ] && ! operand2[ i ] ? Array[ i ] : ! Array[ i ] && operand2[ i ] ? operand2[ i ] : 0 );
        }
                                        // These operations make most sense either with both data operands, which boils down to arithemetic operations;
                                        // or both mask operands, which boils down to boolean operations. Mixed cases should be consisdered carefully.

                                        // Addition or OR
    else if ( typeoperation == OperationAdd ) {
        if      ( typeoperand1 == OperandMask && typeoperand2 == OperandMask )      Array[ i ] = (TypeD) ( Array[ i ] ||  operand2[ i ] );                      // taken from OR
        else if ( typeoperand1 == OperandMask && typeoperand2 == OperandData )      Array[ i ] = (TypeD) ( operand2[ i ] ? operand2[ i ] : Array[ i ]    );     // taken from OR
        else if ( typeoperand1 == OperandData && typeoperand2 == OperandMask )      Array[ i ] = (TypeD) ( Array[ i ]    ? Array[ i ]    : operand2[ i ] );     // taken from OR
        else if ( typeoperand1 == OperandData && typeoperand2 == OperandData )      Array[ i ] = Clip ( (TypeD) ( Array[ i ] + operand2[ i ] ), minv,   maxv ); // arithmetic +
        }
                                        // Subtraction or Clearing
    else if ( typeoperation == OperationSub ) {
        if      ( typeoperand1 == OperandMask && typeoperand2 == OperandMask )      Array[ i ] = (TypeD) ( Array[ i ] && ! operand2[ i ] );                     // AND NOT
        else if ( typeoperand1 == OperandMask && typeoperand2 == OperandData )      Array[ i ] = (TypeD) ( Array[ i ] && ! operand2[ i ] );                     // AND NOT
        else if ( typeoperand1 == OperandData && typeoperand2 == OperandMask )      Array[ i ] = (TypeD) ( ! operand2[ i ] ? Array[ i ] : 0 );                  // AND NOT
        else if ( typeoperand1 == OperandData && typeoperand2 == OperandData )      Array[ i ] = Clip ( (TypeD) ( Array[ i ] - operand2[ i ] ), minv,   maxv ); // arithmetic -
        }
                                        // Multiplication or AND
    else if ( typeoperation == OperationMultiply ) {
        if      ( typeoperand1 == OperandMask && typeoperand2 == OperandMask )      Array[ i ] = (TypeD) ( Array[ i ] && operand2[ i ] );                       // AND
        else if ( typeoperand1 == OperandMask && typeoperand2 == OperandData )      Array[ i ] = (TypeD) ( Array[ i ] ? operand2[ i ] : 0 );                    // AND
        else if ( typeoperand1 == OperandData && typeoperand2 == OperandMask )      Array[ i ] = (TypeD) ( operand2[ i ] ? Array[ i ] : 0 );                    // AND
        else if ( typeoperand1 == OperandData && typeoperand2 == OperandData )      Array[ i ] = Clip ( (TypeD) ( Array[ i ] * operand2[ i ] ), minv,   maxv ); // arithmetic *
        }
                                        // Division or XOR
    else if ( typeoperation == OperationDivide ) {
        if      ( typeoperand1 == OperandMask && typeoperand2 == OperandMask )      Array[ i ] = (TypeD) ( (bool) Array[ i ] ^ (bool) operand2[ i ] );          // XOR
        else if ( typeoperand1 == OperandMask && typeoperand2 == OperandData )      Array[ i ] = (TypeD) ( Array[ i ] && ! operand2[ i ] ? true       : ! Array[ i ] && operand2[ i ] ? operand2[ i ] : 0 );
        else if ( typeoperand1 == OperandData && typeoperand2 == OperandMask )      Array[ i ] = (TypeD) ( Array[ i ] && ! operand2[ i ] ? Array[ i ] : ! Array[ i ] && operand2[ i ] ? true          : 0 );
        else if ( typeoperand1 == OperandData && typeoperand2 == OperandData )      Array[ i ] = Clip ( (TypeD) ( operand2[ i ] ? Array[ i ] / operand2[ i ] : 0 ), minv,   maxv ); // arithmetic /
        }
}


/*                                        // loops if sizes differ
if ( LinearDim == op2.LinearDim )
    BinaryOp ( op2, OperandData, OperandMask, OperationAnd );

else {
    int     mindim1 = min ( Dim1, op2.Dim1 );
    int     mindim2 = min ( Dim2, op2.Dim2 );
    int     mindim3 = min ( Dim3, op2.Dim3 );

    for ( int d1 = 0; d1 < mindim1; d1++ )
    for ( int d2 = 0; d2 < mindim2; d2++ )
    for ( int d3 = 0, i = IndexesToLinearIndex ( d1, d2, d3 ); d3 < mindim3; d3++, i++ )
        Array[ i ]  = (TypeD) ( op2 ( d1, d2, d3 ) ? Array[ i ] : 0 );
    }
*/

//----------------------------------------------------------------------------

template <class TypeD>
void    TVolume<TypeD>::ApplyMaskToData ( TVolume<TypeD> &mask )
{
BinaryOp ( mask, OperandData, OperandMask, OperationAnd );
}


template <class TypeD>
void    TVolume<TypeD>::ApplyDataToMask ( TVolume<TypeD> &data )
{
BinaryOp ( data, OperandMask, OperandData, OperationAnd );
}


template <class TypeD>
void    TVolume<TypeD>::ClearMaskToData ( TVolume<TypeD> &mask )
{
BinaryOp ( mask, OperandData, OperandMask, OperationSub );
}


//----------------------------------------------------------------------------
                                        // threshold & make the interior filled
template <class TypeD>
void    TVolume<TypeD>::FilterToMask ( FctParams& params, bool showprogress )
{
TypeD               backthreshold   = (TypeD) params ( FilterParamToMaskThreshold );
TypeD               newvalue        = (TypeD) params ( FilterParamToMaskNewValue  );
bool                carveback       =         params ( FilterParamToMaskCarveBack );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need a cloned, temp array
TVolume<TypeD>      temp ( *this );

                                        // clear ourselves
ResetMemory ();


TSuperGauge         Gauge ( FilterPresets[ FilterTypeToMask ].Text, showprogress ? ( carveback ? 7 : 4 ) : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Fill the interior
OmpParallelBegin

Gauge.Next ();

OmpFor

for ( int x = 0; x < Dim1; x++ )
for ( int y = 0; y < Dim2; y++ ) {

    int                 zmin;
    int                 zmax;

    for ( zmin = 0; zmin < Dim3; zmin++ )
        if ( temp ( x, y, zmin ) >= backthreshold )
            break;

    if ( zmin == Dim3 )
        continue;

    for ( zmax = Dim3 - 1; zmax >= 0; zmax-- )
        if ( temp ( x, y, zmax ) >= backthreshold )
            break;

    for ( int z = zmin; z <= zmax; z++ )
        Array[ IndexesToLinearIndex ( x, y, z ) ]  = newvalue;
    } // for y


Gauge.Next ();

OmpFor

for ( int x = 0; x < Dim1; x++ )
for ( int z = 0; z < Dim3; z++ ) {

    int                 ymin;
    int                 ymax;

    for ( ymin = 0; ymin < Dim2; ymin++ )
        if ( temp ( x, ymin, z ) >= backthreshold )
            break;

    if ( ymin == Dim2 )
        continue;

    for ( ymax = Dim2 - 1; ymax >= 0; ymax-- )
        if ( temp ( x, ymax, z ) >= backthreshold )
            break;

    for ( int y = ymin; y <= ymax; y++ )
        Array[ IndexesToLinearIndex ( x, y, z ) ]  = newvalue;
    } // for z


Gauge.Next ();

OmpFor

for ( int y = 0; y < Dim2; y++ )
for ( int z = 0; z < Dim3; z++ ) {

    int                 xmin;
    int                 xmax;

    for ( xmin = 0; xmin < Dim1; xmin++ )
        if ( temp ( xmin, y, z ) >= backthreshold )
            break;

    if ( xmin == Dim1 )
        continue;

    for ( xmax = Dim1 - 1; xmax >= 0; xmax-- )
        if ( temp ( xmax, y, z ) >= backthreshold )
            break;

    for ( int x = xmin; x <= xmax; x++ )
        Array[ IndexesToLinearIndex ( x, y, z ) ]  = newvalue;
    } // for z

OmpParallelEnd

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! carveback ) {

    Gauge.Next ();
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Carve back the exterior, where "convex bridges" have been filled
OmpParallelBegin

Gauge.Next ();

OmpFor

for ( int x = 0; x < Dim1; x++ )
for ( int y = 0; y < Dim2; y++ ) {

    int                 zmin;
    int                 zmax;

    for ( zmin = 0; zmin < Dim3 && temp ( x, y, zmin ) < backthreshold; zmin++ )
        Array[ IndexesToLinearIndex ( x, y, zmin ) ]  = (TypeD) 0;

    if ( zmin == Dim3 )
        continue;

    for ( zmax = Dim3 - 1; zmax >= 0 && temp ( x, y, zmax ) < backthreshold; zmax-- )
        Array[ IndexesToLinearIndex ( x, y, zmax ) ]  = (TypeD) 0;
    } // for y


Gauge.Next ();

OmpFor

for ( int x = 0; x < Dim1; x++ )
for ( int z = 0; z < Dim3; z++ ) {

    int                 ymin;
    int                 ymax;

    for ( ymin = 0; ymin < Dim2 && temp ( x, ymin, z ) < backthreshold; ymin++ )
        Array[ IndexesToLinearIndex ( x, ymin, z ) ]  = (TypeD) 0;

    if ( ymin == Dim2 )
        continue;

    for ( ymax = Dim2 - 1; ymax >= 0 && temp ( x, ymax, z ) < backthreshold; ymax-- )
        Array[ IndexesToLinearIndex ( x, ymax, z ) ]  = (TypeD) 0;
    } // for z


Gauge.Next ();

OmpFor

for ( int y = 0; y < Dim2; y++ )
for ( int z = 0; z < Dim3; z++ ) {

    int                 xmin;
    int                 xmax;

    for ( xmin = 0; xmin < Dim1 && temp ( xmin, y, z ) < backthreshold; xmin++ )
        Array[ IndexesToLinearIndex ( xmin, y, z ) ]  = (TypeD) 0;

    if ( xmin == Dim1 )
        continue;

    for ( xmax = Dim1 - 1; xmax >= 0 && temp ( xmax, y, z ) < backthreshold; xmax-- )
        Array[ IndexesToLinearIndex ( xmax, y, z ) ]  = (TypeD) 0;
    } // for z

OmpParallelEnd

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Finish: Close the carved results, in case some lone surface holes produced some weird styletto punches
Gauge.Next ();


FctParams           p;
p ( FilterParamDiameter )     = 1;
Filter ( FilterTypeClose, p );
}


//----------------------------------------------------------------------------
                                        // Symmetrize a volume with a Max operator, which can elegantly applied to both real data and masks / ROIs
                                        // Center is currently assumed to be an exact voxel position, i.e. no interpolation is being used
                                        // Whole size is remaining unchanged
template <class TypeD>
void    TVolume<TypeD>::FilterSymmetrize ( FctParams& params )
{
int                 axis            = (int)      params ( FilterParamSymmetrizeAxis   );
int                 center          = Truncate ( params ( FilterParamSymmetrizeOrigin ) );


int                 dimaxis         = GetDim ( axis );
                                        // there needs to be at least 1 voxel of data worth symmetrizing!
if ( ! IsInsideLimits ( center, 1, dimaxis - 2 ) )
    return;

                                        // How far we are going to fold, as data outside volume is unknown / 0
int                 delta           = min ( center, dimaxis - 1 - center );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if      ( axis == 0 )

    OmpParallelFor

    for ( int y = 0; y < Dim2; y++ )
    for ( int z = 0; z < Dim3; z++ )
    for ( int d = 0, x1 = center - 1, x2 = center + 1; d < delta; d++, x1--, x2++ )
    
        (*this) ( x1, y, z )    = (*this) ( x2, y, z )    = (TypeD) max ( (*this) ( x1, y, z ), (*this) ( x2, y, z ) );

else if ( axis == 1 )

    OmpParallelFor

    for ( int x = 0; x < Dim1; x++ )
    for ( int z = 0; z < Dim3; z++ )
    for ( int d = 0, y1 = center - 1, y2 = center + 1; d < delta; d++, y1--, y2++ )
    
        (*this) ( x, y1, z )    = (*this) ( x, y2, z )    = (TypeD) max ( (*this) ( x, y1, z ), (*this) ( x, y2, z ) );

else if ( axis == 2 )

    OmpParallelFor

    for ( int x = 0; x < Dim1; x++ )
    for ( int y = 0; y < Dim2; y++ )
    for ( int d = 0, z1 = center - 1, z2 = center + 1; d < delta; d++, z1--, z2++ )
    
        (*this) ( x, y, z1 )    = (*this) ( x, y, z2 )    = (TypeD) max ( (*this) ( x, y, z1 ), (*this) ( x, y, z2 ) );

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
