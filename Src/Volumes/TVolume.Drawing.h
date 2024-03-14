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

namespace crtl {

//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
                                        // Draw a line between the 2 points - no anti-aliasing
template <class TypeD>
void    TVolume<TypeD>::DrawLine ( TPointFloat fromp, TPointFloat top, TypeD v )
{
TPoints             linepoints;

                                        // get more points for more precision!
linepoints.SetLineOfPoints ( fromp, top, true, 2 );

                                        // rounding positions before truncation
linepoints     += 0.5;


for ( int i = 0; i < (int) linepoints; i++ )
    if ( WithinBoundary ( linepoints[ i ] ) )
          (*this) ( linepoints[ i ] )   = v;
}


//----------------------------------------------------------------------------
                                        // Anti-aliased drawing, can be used to other shapes
template <class TypeD>
void    TVolume<TypeD>::DrawCylinder    (   TPointFloat     fromp,  TPointFloat         top, 
                                            double          radius, double              feather,
                                            TypeD           v,      ArrayOperationType  typeoperation 
                                        )
{
if ( radius <= 0 )
    return;

Maxed ( feather, 0.0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get a bounding box around the object
TBoundingBox<int>   bounding ( max ( 0,        min ( Truncate ( fromp.X - radius - 1 ), Truncate ( top.X - radius - 1 ) ) ),
                               min ( Dim1 - 1, max ( Truncate ( fromp.X + radius + 2 ), Truncate ( top.X + radius + 2 ) ) ),
                               max ( 0,        min ( Truncate ( fromp.Y - radius - 1 ), Truncate ( top.Y - radius - 1 ) ) ),
                               min ( Dim2 - 1, max ( Truncate ( fromp.Y + radius + 2 ), Truncate ( top.Y + radius + 2 ) ) ),
                               max ( 0,        min ( Truncate ( fromp.Z - radius - 1 ), Truncate ( top.Z - radius - 1 ) ) ),
                               min ( Dim3 - 1, max ( Truncate ( fromp.Z + radius + 2 ), Truncate ( top.Z + radius + 2 ) ) ) );

                                        // compute transformation to cylinder
TPointFloat         O  ( fromp   );
TPointFloat         d1 ( top - O );
TPointFloat         d2;
TPointFloat         d3;
TPointFloat         p;
double              length;
double              p1;
double              p2;
double              p3;
double              ratio;

length      = d1.Norm ();
d1         /= length;

d1.GetOrthogonalVectors ( d2, d3 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do a subsampling
int                 numsubsampling  = 4;
double              substep         = 1.0 / numsubsampling;
int                 bucketsize      = Cube ( numsubsampling );
double              count;
double              r;
TypeD               value;
int                 xdi,    ydi,    zdi;


for ( int x = bounding.XMin (); x <= bounding.XMax (); x++ )
for ( int y = bounding.YMin (); y <= bounding.YMax (); y++ )
for ( int z = bounding.ZMin (); z <= bounding.ZMax (); z++ ) {

    count   = 0;
                                        // subsampling scan
    for ( xdi = 0, p.X = x - 0.5 + substep / 2 - O.X; xdi < numsubsampling; xdi++, p.X += substep )
    for ( ydi = 0, p.Y = y - 0.5 + substep / 2 - O.Y; ydi < numsubsampling; ydi++, p.Y += substep )
    for ( zdi = 0, p.Z = z - 0.5 + substep / 2 - O.Z; zdi < numsubsampling; zdi++, p.Z += substep ) {
                                        // project on the new base
        p1      = p.ScalarProduct ( d1 );
        p2      = p.ScalarProduct ( d2 );
        p3      = p.ScalarProduct ( d3 );
                                        // no rounding here!
        r       = sqrt ( Square ( p2 ) + Square ( p3 ) );
                                        // is inside cylinder?
        if ( p1 >= 0 && p1 <= length
          && r <= radius + feather )

//          count++;
                                        // accounts for antialiasing + feather transition
            count += r <= radius - feather ? 1 : ( radius - r + feather ) / ( 2 * feather );

        } // subsampled block


    if ( count == 0 )
        continue;

                                // final antialising ratio
    ratio   = (double) count / bucketsize;
                                // -> antialised value
    value   = (TypeD) ( ratio * v + ( 1 - ratio ) * Array[ IndexesToLinearIndex ( x, y, z ) ] );
//  value   = (TypeD) ( ratio * v );


    if      ( typeoperation == OperationMax )   Maxed ( Array[ IndexesToLinearIndex ( x, y, z ) ], value );
    else if ( typeoperation == OperationSet )   Array[ IndexesToLinearIndex ( x, y, z ) ]   = value;

    } // for x, y, z
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVolume<TypeD>::DrawSphere      (   TPointFloat     center, 
                                            double          radius,     double              feather,
                                            TypeD           v,          ArrayOperationType  typeoperation 
                                        )
{
if ( radius <= 0 )
    return;

Maxed ( feather, 0.0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get a bounding box around the object
TBoundingBox<int>   bounding ( max ( 0,        Truncate ( center.X - radius - 1 ) ),
                               min ( Dim1 - 1, Truncate ( center.X + radius + 2 ) ),
                               max ( 0,        Truncate ( center.Y - radius - 1 ) ),
                               min ( Dim2 - 1, Truncate ( center.Y + radius + 2 ) ),
                               max ( 0,        Truncate ( center.Z - radius - 1 ) ),
                               min ( Dim3 - 1, Truncate ( center.Z + radius + 2 ) ) );


//bool                singlevoxel     = radius <= 0.5;
//if ( singlevoxel ) feather  = 0;

                                        // do a subsampling
int                 numsubsampling  = 4;
double              substep         = 1.0 / numsubsampling;
int                 bucketsize      = Cube ( numsubsampling );


OmpParallelFor

for ( int x = bounding.XMin (); x <= bounding.XMax (); x++ )
for ( int y = bounding.YMin (); y <= bounding.YMax (); y++ )
for ( int z = bounding.ZMin (); z <= bounding.ZMax (); z++ ) {


    TPointFloat         p;
    int                 xdi,    ydi,    zdi;
    double              count   = 0;

    for ( xdi = 0, p.X = x - 0.5 + substep / 2 - center.X; xdi < numsubsampling; xdi++, p.X += substep )
    for ( ydi = 0, p.Y = y - 0.5 + substep / 2 - center.Y; ydi < numsubsampling; ydi++, p.Y += substep )
    for ( zdi = 0, p.Z = z - 0.5 + substep / 2 - center.Z; zdi < numsubsampling; zdi++, p.Z += substep ) {
                                        // no rounding here!
        double          r       = p.Norm ();
                                        // is inside sphere?
        if ( r <= radius + feather )

//          count++;
                                        // accounts for antialiasing + feather transition
            count += r <= radius - feather ? 1 : ( radius - r + feather ) / ( 2 * feather );

        } // subsampled block


    if ( count == 0 )
        continue;

                                // final antialising ratio
    double              ratio   = (double) count / bucketsize;
                                // -> antialised value
    TypeD               value   = (TypeD) ( ratio * v + ( 1 - ratio ) * Array[ IndexesToLinearIndex ( x, y, z ) ] );
//  TypeD               value   = (TypeD) ( ratio * v );


    if      ( typeoperation == OperationMax )   Maxed ( Array[ IndexesToLinearIndex ( x, y, z ) ], value );
    else if ( typeoperation == OperationSet )   Array[ IndexesToLinearIndex ( x, y, z ) ]   = value;

    } // for z
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVolume<TypeD>::BlurrSphere     (   TPointFloat     center, 
                                            double          radius
                                        )
{
if ( radius <= 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // copy volume & apply blurring globally
TVolume<TypeD>      blurred      ( *this );
TBoundingBox<int>   databounding ( *this, true );
FctParams           params;


double              maxgaussian     = databounding.Radius () / 10;  // fair enough maximum limit on the Gaussian

params ( FilterParamDiameter )     = Clip ( radius * 0.5, 3.47, maxgaussian );

//DBGV3 ( radius, maxgaussian, params ( FilterParamDiameter ), "radius, maxgaussian, FilterParamDiameter" );

blurred.Filter ( FilterTypeFastGaussian /*FilterTypeGaussian*/, params );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get a bounding box around the object
TBoundingBox<int>   bounding ( max ( 0,        Truncate ( center.X - radius - 1 ) ),
                               min ( Dim1 - 1, Truncate ( center.X + radius + 2 ) ),
                               max ( 0,        Truncate ( center.Y - radius - 1 ) ),
                               min ( Dim2 - 1, Truncate ( center.Y + radius + 2 ) ),
                               max ( 0,        Truncate ( center.Z - radius - 1 ) ),
                               min ( Dim3 - 1, Truncate ( center.Z + radius + 2 ) ) );



OmpParallelFor

for ( int x = bounding.XMin (); x <= bounding.XMax (); x++ )
for ( int y = bounding.YMin (); y <= bounding.YMax (); y++ )
for ( int z = bounding.ZMin (); z <= bounding.ZMax (); z++ ) {

    TPointFloat         p ( x, y, z );

    p -= center;
    p -= 0.5;
        
    double      ratio   = 1 - Clip ( p.Norm () / radius, 0.0, 1.0 );

//  ratio               = ratio > 0;                // on/off (should use some anti-aliasing)
//  ratio               = sqrt ( ratio );           // more blurring, but not smooth joints
    ratio               = Power ( Hanning ( ratio / 2 ), 0.25 );    // cosine + center boost


    double      value   =          ratio   * blurred[ IndexesToLinearIndex ( x, y, z ) ]
                           + ( 1 - ratio ) * Array  [ IndexesToLinearIndex ( x, y, z ) ];


    Array[ IndexesToLinearIndex ( x, y, z ) ]   = (TypeD) value;

    } // for x, y, z
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVolume<TypeD>::ErasePlane      (   TPointFloat     center,     TPointFloat normal,
                                            double          feather
                                        )
{
if ( normal.IsNull () )
    return;

Maxed ( feather, 0.0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scanning the whole volume
TBoundingBox<int>   bounding ( 0,       Dim1 - 1,
                               0,       Dim2 - 1,
                               0,       Dim3 - 1  );

TPointFloat         p;
double              ps;
                                        // do a subsampling
//int                 numsubsampling  = 4;
//double              substep         = 1.0 / numsubsampling;
//int                 bucketsize      = Cube ( numsubsampling );
//double              count;
//int                 xdi,    ydi,    zdi;
//double              ratio;


for ( int x = bounding.XMin (); x <= bounding.XMax (); x++ )
for ( int y = bounding.YMin (); y <= bounding.YMax (); y++ )
for ( int z = bounding.ZMin (); z <= bounding.ZMax (); z++ ) {


//  count   = 0;
//                                      // subsampling scan
//  for ( xdi = 0, p.X = x - center.X + substep / 2; xdi < numsubsampling; xdi++, p.X += substep )
//  for ( ydi = 0, p.Y = y - center.Y + substep / 2; ydi < numsubsampling; ydi++, p.Y += substep )
//  for ( zdi = 0, p.Z = z - center.Z + substep / 2; zdi < numsubsampling; zdi++, p.Z += substep ) {
//
//                                      // gradient points outward
//      ps  = p.ScalarProduct ( normal );
//
//      if      ( ps <= -feather )  count += 1;
//      else if ( ps <   feather )  count += ( feather - ps ) / ( 2 * feather );
//      } // subsampled block
//
//                                      // final antialising ratio
//  ratio   = (double) count / bucketsize;
//
//  if ( count == 0 )       Array[ IndexesToLinearIndex ( x, y, z ) ]   = 0;
//  else if ( ratio < 1.0 ) Array[ IndexesToLinearIndex ( x, y, z ) ]  *= ratio;


    p.Set ( x - center.X, y - center.Y, z - center.Z );

//  p  += 0.5; // ?needed ot not?
                                        // gradient points outward
    ps  = p.ScalarProduct ( normal );

    if      ( ps >   feather )  Array[ IndexesToLinearIndex ( x, y, z ) ]   = (TypeD) 0;
    else if ( ps >= -feather )  Array[ IndexesToLinearIndex ( x, y, z ) ]   = (TypeD) ( ( feather - ps ) / ( 2 * feather ) * Array[ IndexesToLinearIndex ( x, y, z ) ] );

    } // for x, y, z

}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVolume<TypeD>::DrawCube ( TPointFloat center, double radius, TypeD v, FilterTypes filtertype )
{
if ( radius <= 0 )
    return;

                                        // get a bounding box around the object
TBoundingBox<int>   bounding ( max ( 0,        Truncate ( center.X - radius - 1 ) ),
                               min ( Dim1 - 1, Truncate ( center.X + radius + 2 ) ),
                               max ( 0,        Truncate ( center.Y - radius - 1 ) ),
                               min ( Dim2 - 1, Truncate ( center.Y + radius + 2 ) ),
                               max ( 0,        Truncate ( center.Z - radius - 1 ) ),
                               min ( Dim3 - 1, Truncate ( center.Z + radius + 2 ) ) );


TPointFloat         p;

                                        // do a subsampling
int                 numsubsampling  = 4;
double              substep         = 1.0 / numsubsampling;
int                 bucketsize      = Cube ( numsubsampling );
int                 count;
TypeD               value;
int                 xdi,    ydi,    zdi;


for ( int x = bounding.XMin (); x <= bounding.XMax (); x++ )
for ( int y = bounding.YMin (); y <= bounding.YMax (); y++ )
for ( int z = bounding.ZMin (); z <= bounding.ZMax (); z++ ) {

    count   = 0;

    for ( xdi = 0, p.X = x - 0.5 + substep / 2 - center.X; xdi < numsubsampling; xdi++, p.X += substep )
    for ( ydi = 0, p.Y = y - 0.5 + substep / 2 - center.Y; ydi < numsubsampling; ydi++, p.Y += substep )
    for ( zdi = 0, p.Z = z - 0.5 + substep / 2 - center.Z; zdi < numsubsampling; zdi++, p.Z += substep ) {
                                // no rounding here!
        if ( p.X >= -radius && p.X <= radius
          && p.Y >= -radius && p.Y <= radius
          && p.Z >= -radius && p.Z <= radius )
            count++;
        } // subsampled block


    if ( count == 0 )
        continue;
                                        // this is the antialised value
//  value   = (TypeD) ( (double) count / bucketsize * v );
                                        // plug new value: use max, but could be a sum
//  Array[ IndexesToLinearIndex ( x, y, z ) ]   = max ( Array[ IndexesToLinearIndex ( x, y, z ) ], value );

                                        // new value
    if      ( filtertype == FilterTypeMedian )  value   = (TypeD) ( count >= bucketsize / 2 ? v : 0 );
    else if ( filtertype == FilterTypeMax   )   value   = (TypeD) ( count                   ? v : 0 );
    else if ( filtertype == FilterTypeMean  )   value   = (TypeD) ( (double) count / bucketsize * v );

                                        // merge value to existing data
    if      ( filtertype == FilterTypeMedian )  Array[ IndexesToLinearIndex ( x, y, z ) ]   = value ? value : Array[ IndexesToLinearIndex ( x, y, z ) ];
    else if ( filtertype == FilterTypeMax   )   Array[ IndexesToLinearIndex ( x, y, z ) ]   = value ? value : Array[ IndexesToLinearIndex ( x, y, z ) ];
    else if ( filtertype == FilterTypeMean  )   Array[ IndexesToLinearIndex ( x, y, z ) ]   = Array[ IndexesToLinearIndex ( x, y, z ) ] + value;

    } // for z
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
