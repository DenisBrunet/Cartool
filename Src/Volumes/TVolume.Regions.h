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
template <class TypeD>
TVolume<TypeD>& TVolume<TypeD>::operator= ( const TVolumeRegions& op2 )
{
ResetMemory ();

if ( op2.IsEmpty () || IsNotAllocated () )
    return  *this;


for ( int i = 1; i <= op2.NumRegions () ; i++ )
                                        // cumulate all regions
    *this  |= op2[ i ];

return  *this;
}


template <class TypeD>
TVolume<TypeD>& TVolume<TypeD>::operator= ( const TVolumeRegion& op2 )
{
                                        // delegate to TVolumeRegion
op2.CopyTo ( *this, true );

return  *this;
}


template <class TypeD>
TVolume<TypeD>& TVolume<TypeD>::operator|= ( const TVolumeRegion& op2 )
{
                                        // delegate to TVolumeRegion
op2.CopyTo ( *this, false );

return  *this;
}


//----------------------------------------------------------------------------
                                        // Each level becomes a single region
template <class TypeD>
void    TVolume<TypeD>::LevelsToRegions ( TVolumeRegions& gor )
{
gor.Reset ();

                                        // account for the remaining voxels to process
TVolume<uchar>      voxelstodo ( Dim1, Dim2, Dim3 );

voxelstodo  = true;

                                        // all but the background, assumed to be 0 here - put it as a parameter, maybe?
OmpParallelFor

for ( int i = 0; i < LinearDim; i++ )
    if ( Array[ i ] == 0 )
        voxelstodo[ i ] = false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 index           = 1;
TypeD               l;
TVolumeRegion*      region;


do {

    l   = 0;
                                        // get next voxel
    for ( int i = 0; i < LinearDim; i++ )
        if ( voxelstodo[ i ] ) {
            l   = Array[ i ];
            break;
            }
                                        // exit here: no more voxels / only backaground
    if ( l == 0 )
        break;


    region  = new TVolumeRegion ( this, index );

    OmpParallelFor

    for ( int i = 0; i < LinearDim; i++ )
                                        // exact same level?
        if ( Array[ i ] == l ) {
//          OmpCriticalBegin (LevelsToRegions)

            region->Add ( LinearIndexToX ( i ), LinearIndexToY ( i ), LinearIndexToZ ( i ) );

            voxelstodo[ i ] = false;

//          OmpCriticalEnd
            }

                                        // update region
    region->Set ( true );
                                        // add new region
    gor.Add ( region );

    index++;

    } while ( true /*voxelstodo.GetNumSet ()*/ );

}


//----------------------------------------------------------------------------
                                        // Each geometrical cluster of same level becomes a region
template <class TypeD>
void    TVolume<TypeD>::LevelsClustersToRegions ( TVolumeRegions& gor, int minvoxels, int maxvoxels, NeighborhoodType neighborhood, bool showprogress )
{
gor.Reset ();

                                        // account for the remaining voxels to process
TVolume<uchar>      voxelstodo ( Dim1, Dim2, Dim3 );
                                        // binarize
OmpParallelFor

for ( int i = 0; i < LinearDim; i++ )
    voxelstodo[ i ] = (bool) Array[ i ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 index           = 1;
TypeD               l;
TVolumeRegion*      region;

TVolume<uchar>      edge ( Dim1, Dim2, Dim3 );
int                 edgecount;
//int                 x,  y,  z;
int                 j;
int                 numvoxelstotal  = voxelstodo.GetNumSet ();
int                 numvoxelstodo   = numvoxelstotal;
TBoundingBox<int>   bremain;
TBoundingBox<int>   bedge;


TSuperGauge         Gauge ( FilterPresets[ FilterTypeClustersToRegions ].Text, showprogress ? numvoxelstotal : 0 );

Gauge.SetValue ( SuperGaugeDefaultPart, 0 );


do {

    l           = 0;
    edge.ResetMemory ();
    edgecount   = 0;

                                        // global box of remaining voxels
    bremain     = TBoundingBox<int> ( voxelstodo, false, 0.1 );


    for ( int x = bremain.XMin (); x <= bremain.XMax () && ! edgecount; x++ )
    for ( int y = bremain.YMin (); y <= bremain.YMax () && ! edgecount; y++ )
    for ( int z = bremain.ZMin (); z <= bremain.ZMax (); z++ )

        if ( voxelstodo ( x, y, z ) ) {

            edge       ( x, y, z )  = true; // edge seed
            edgecount               = 1;
            bedge.Set ( x, x, y, y, z, z );

            voxelstodo ( x, y, z )  = false;
            numvoxelstodo--;

            l                       = GetValue ( x, y, z );
            break;
            }

    if ( edgecount == 0 )               // no more voxels?
        break;                          // finito


    region          = new TVolumeRegion ( this, index );


    do {                                // progress the edge, if the same level, connected, and not already done

        for ( int x = bedge.XMin (); x <= bedge.XMax (); x++ )
        for ( int y = bedge.YMin (); y <= bedge.YMax (); y++ )
        for ( int z = bedge.ZMin (); z <= bedge.ZMax (); z++ ) {

            if ( ! edge ( x, y, z ) )
                continue;

                                        // reset current edge
            region->Add ( x, y, z );
            edge        ( x, y, z ) = false;
            edgecount--;

                                        // progress the edge, doing as much as possible in straight directions
                                        // 6 neighbors
            {
            for ( int x2 = x - 1; x2 >= bremain.XMin (); x2-- ) {
                j   = IndexesToLinearIndex ( x2, y, z );
                if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                else break;
                }

            for ( int x2 = x + 1; x2 <= bremain.XMax (); x2++ ) {
                j   = IndexesToLinearIndex ( x2, y, z );
                if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                else break;
                }

            for ( int y2 = y - 1; y2 >= bremain.YMin (); y2-- ) {
                j   = IndexesToLinearIndex ( x, y2, z );
                if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                else break;
                }

            for ( int y2 = y + 1; y2 <= bremain.YMax (); y2++ ) {
                j   = IndexesToLinearIndex ( x, y2, z );
                if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                else break;
                }

            for ( int z2 = z - 1; z2 >= bremain.ZMin (); z2-- ) {
                j   = IndexesToLinearIndex ( x, y, z2 );
                if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                else break;
                }

            for ( int z2 = z + 1; z2 <= bremain.ZMax (); z2++ ) {
                j   = IndexesToLinearIndex ( x, y, z2 );
                if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                else break;
                }
            }


                                        // 18 neighbors
            if ( neighborhood >= Neighbors18 ) {

                for ( int x2 = x - 1, y2 = y - 1; x2 >= bremain.XMin () && y2 >= bremain.YMin (); x2--, y2-- ) {
                    j   = IndexesToLinearIndex ( x2, y2, z );
                    if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x - 1, y2 = y + 1; x2 >= bremain.XMin () && y2 <= bremain.YMax (); x2--, y2++ ) {
                    j   = IndexesToLinearIndex ( x2, y2, z );
                    if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x + 1, y2 = y - 1; x2 <= bremain.XMax () && y2 >= bremain.YMin (); x2++, y2-- ) {
                    j   = IndexesToLinearIndex ( x2, y2, z );
                    if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x + 1, y2 = y + 1; x2 <= bremain.XMax () && y2 <= bremain.YMax (); x2++, y2++ ) {
                    j   = IndexesToLinearIndex ( x2, y2, z );
                    if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }


                for ( int x2 = x - 1, z2 = z - 1; x2 >= bremain.XMin () && z2 >= bremain.ZMin (); x2--, z2-- ) {
                    j   = IndexesToLinearIndex ( x2, y, z2 );
                    if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x - 1, z2 = z + 1; x2 >= bremain.XMin () && z2 <= bremain.ZMax (); x2--, z2++ ) {
                    j   = IndexesToLinearIndex ( x2, y, z2 );
                    if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x + 1, z2 = z - 1; x2 <= bremain.XMax () && z2 >= bremain.ZMin (); x2++, z2-- ) {
                    j   = IndexesToLinearIndex ( x2, y, z2 );
                    if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x + 1, z2 = z + 1; x2 <= bremain.XMax () && z2 <= bremain.ZMax (); x2++, z2++ ) {
                    j   = IndexesToLinearIndex ( x2, y, z2 );
                    if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }


                for ( int y2 = y - 1, z2 = z - 1; y2 >= bremain.YMin () && z2 >= bremain.ZMin (); y2--, z2-- ) {
                    j   = IndexesToLinearIndex ( x, y2, z2 );
                    if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int y2 = y - 1, z2 = z + 1; y2 >= bremain.YMin () && z2 <= bremain.ZMax (); y2--, z2++ ) {
                    j   = IndexesToLinearIndex ( x, y2, z2 );
                    if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int y2 = y + 1, z2 = z - 1; y2 <= bremain.YMax () && z2 >= bremain.ZMin (); y2++, z2-- ) {
                    j   = IndexesToLinearIndex ( x, y2, z2 );
                    if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int y2 = y + 1, z2 = z + 1; y2 <= bremain.YMax () && z2 <= bremain.ZMax (); y2++, z2++ ) {
                    j   = IndexesToLinearIndex ( x, y2, z2 );
                    if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                } // Neighbors18


                                        // 26 neighbors
            if ( neighborhood >= Neighbors26 ) {

                for ( int x2 = x - 1, y2 = y - 1, z2 = z - 1; x2 >= bremain.XMin () && y2 >= bremain.YMin () && z2 >= bremain.ZMin (); x2--, y2--, z2-- ) {
                    j   = IndexesToLinearIndex ( x2, y2, z2 );
                    if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x - 1, y2 = y - 1, z2 = z + 1; x2 >= bremain.XMin () && y2 >= bremain.YMin () && z2 <= bremain.ZMax (); x2--, y2--, z2++ ) {
                    j   = IndexesToLinearIndex ( x2, y2, z2 );
                    if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x - 1, y2 = y + 1, z2 = z - 1; x2 >= bremain.XMin () && y2 <= bremain.YMax () && z2 >= bremain.ZMin (); x2--, y2++, z2-- ) {
                    j   = IndexesToLinearIndex ( x2, y2, z2 );
                    if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x - 1, y2 = y + 1, z2 = z + 1; x2 >= bremain.XMin () && y2 <= bremain.YMax () && z2 <= bremain.ZMax (); x2--, y2++, z2++ ) {
                    j   = IndexesToLinearIndex ( x2, y2, z2 );
                    if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x + 1, y2 = y - 1, z2 = z - 1; x2 <= bremain.XMax () && y2 >= bremain.YMin () && z2 >= bremain.ZMin (); x2++, y2--, z2-- ) {
                    j   = IndexesToLinearIndex ( x2, y2, z2 );
                    if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x + 1, y2 = y - 1, z2 = z + 1; x2 <= bremain.XMax () && y2 >= bremain.YMin () && z2 <= bremain.ZMax (); x2++, y2--, z2++ ) {
                    j   = IndexesToLinearIndex ( x2, y2, z2 );
                    if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x + 1, y2 = y + 1, z2 = z - 1; x2 <= bremain.XMax () && y2 <= bremain.YMax () && z2 >= bremain.ZMin (); x2++, y2++, z2-- ) {
                    j   = IndexesToLinearIndex ( x2, y2, z2 );
                    if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x + 1, y2 = y + 1, z2 = z + 1; x2 <= bremain.XMax () && y2 <= bremain.YMax () && z2 <= bremain.ZMax (); x2++, y2++, z2++ ) {
                    j   = IndexesToLinearIndex ( x2, y2, z2 );
                    if ( Array[ j ] == l && voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                } // Neighbors26


                                        // inflate edge only, using current point
//            bedge.Set ( max ( bremain.XMin (), min ( bedge.XMin (), x - 1 ) ), min ( bremain.XMax (), max ( bedge.XMax (), x + 1 ) ),
//                        max ( bremain.YMin (), min ( bedge.YMin (), y - 1 ) ), min ( bremain.YMax (), max ( bedge.YMax (), y + 1 ) ),
//                        max ( bremain.ZMin (), min ( bedge.ZMin (), z - 1 ) ), min ( bremain.ZMax (), max ( bedge.ZMax (), z + 1 ) ) );

            } // process edge


                                        // exact box, handles inflation and deletion
//        bedge       = TBoundingBox<int> ( edge, 0.1 );
                                        // inflate edge only with global increment
        bedge.Set ( max ( bremain.XMin (), bedge.XMin () - 1 ), min ( bremain.XMax (), bedge.XMax () + 1 ),
                    max ( bremain.YMin (), bedge.YMin () - 1 ), min ( bremain.YMax (), bedge.YMax () + 1 ),
                    max ( bremain.ZMin (), bedge.ZMin () - 1 ), min ( bremain.ZMax (), bedge.ZMax () + 1 ) );


        Gauge.SetValue ( SuperGaugeDefaultPart, numvoxelstotal - numvoxelstodo );

        } while ( edgecount );

                                        // update region
    region->Set ( true );

                                        // enough points?
    int         rnp     = region->GetNumPoints ();

    if ( rnp >= minvoxels && rnp <= maxvoxels ) {

        gor.Add ( region );

        index++;
        }
    else
        delete  region;


//  Gauge.SetValue ( SuperGaugeDefaultPart, numvoxelstotal - numvoxelstodo );

    } while ( numvoxelstodo );

                                        // By decreasing # of points in regions
gor.Sort ( SortRegionsCount );

}


//----------------------------------------------------------------------------
/*                                      // First version of ClustersToRegions
                                        // Each geometrical cluster of ANY level becomes a region
template <class TypeD>
void    TVolume<TypeD>::ClustersToRegions ( TVolumeRegions& gor, int minvoxels, int maxvoxels, int neighborhood, bool showprogress )
{
gor.Reset ();

                                        // account for the remaining voxels to process
TVolume<uchar>      voxelstodo ( Dim1, Dim2, Dim3 );
                                        // binarize
for ( int i = 0; i < LinearDim; i++ )
    voxelstodo[ i ] = (bool) Array[ i ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 index           = 1;
TVolumeRegion*      region;

TVolume<uchar>      edge ( Dim1, Dim2, Dim3 );
int                 edgecount;
//int                 x,  y,  z;
int                 j;
int                 numvoxelstotal  = voxelstodo.GetNumSet ();
int                 numvoxelstodo   = numvoxelstotal;
TBoundingBox<int>   bremain;
TBoundingBox<int>   bedge;


TSuperGauge         Gauge ( FilterPresets[ FilterTypeClustersToRegions ].Text, showprogress ? numvoxelstotal : 0 );

Gauge.SetValue ( SuperGaugeDefaultPart, 0 );


do {
    edge.ResetMemory ();
    edgecount   = 0;

                                        // global box of remaining voxels
    bremain     = TBoundingBox<int> ( voxelstodo, 0.1 );


    for ( int x = bremain.XMin (); x <= bremain.XMax () && ! edgecount; x++ )
    for ( int y = bremain.YMin (); y <= bremain.YMax () && ! edgecount; y++ )
    for ( int z = bremain.ZMin (); z <= bremain.ZMax (); z++ )

    if ( voxelstodo ( x, y, z ) ) {
            edge       ( x, y, z )  = true; // edge seed
            edgecount               = 1;
            bedge.Set ( x, x, y, y, z, z );

            voxelstodo ( x, y, z )  = false;
            numvoxelstodo--;
            break;
            }

    if ( edgecount == 0 )               // no more voxels?
        break;                          // finito


    region          = new TVolumeRegion ( this, index );

//    DBGV ( index, "next index to cluster" );


    do {                                // progress the edge, if the same level, connected, and not already done

        for ( int x = bedge.XMin (); x <= bedge.XMax (); x++ )
        for ( int y = bedge.YMin (); y <= bedge.YMax (); y++ )
        for ( int z = bedge.ZMin (); z <= bedge.ZMax (); z++ ) {

//            bedge.Show ( "edge" );

            if ( ! edge ( x, y, z ) )
                continue;

                                        // reset current edge
            region->Add ( x, y, z );
            edge        ( x, y, z ) = false;
            edgecount--;

                                        // progress the edge, doing as much as possible in straight directions
                                        // 6 neighbors
            for ( int x2 = x - 1; x2 >= bremain.XMin (); x2-- ) {
                j   = IndexesToLinearIndex ( x2, y, z );
                if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                else break;
                }

            for ( int x2 = x + 1; x2 <= bremain.XMax (); x2++ ) {
                j   = IndexesToLinearIndex ( x2, y, z );
                if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                else break;
                }

            for ( int y2 = y - 1; y2 >= bremain.YMin (); y2-- ) {
                j   = IndexesToLinearIndex ( x, y2, z );
                if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                else break;
                }

            for ( int y2 = y + 1; y2 <= bremain.YMax (); y2++ ) {
                j   = IndexesToLinearIndex ( x, y2, z );
                if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                else break;
                }

            for ( int z2 = z - 1; z2 >= bremain.ZMin (); z2-- ) {
                j   = IndexesToLinearIndex ( x, y, z2 );
                if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                else break;
                }

            for ( int z2 = z + 1; z2 <= bremain.ZMax (); z2++ ) {
                j   = IndexesToLinearIndex ( x, y, z2 );
                if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                else break;
                }


                                        // 18 neighbors
            if ( neighborhood >= Neighbors18 ) {

                for ( int x2 = x - 1, y2 = y - 1; x2 >= bremain.XMin () && y2 >= bremain.YMin (); x2--, y2-- ) {
                    j   = IndexesToLinearIndex ( x2, y2, z );
                    if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x - 1, y2 = y + 1; x2 >= bremain.XMin () && y2 <= bremain.YMax (); x2--, y2++ ) {
                    j   = IndexesToLinearIndex ( x2, y2, z );
                    if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x + 1, y2 = y - 1; x2 <= bremain.XMax () && y2 >= bremain.YMin (); x2++, y2-- ) {
                    j   = IndexesToLinearIndex ( x2, y2, z );
                    if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x + 1, y2 = y + 1; x2 <= bremain.XMax () && y2 <= bremain.YMax (); x2++, y2++ ) {
                    j   = IndexesToLinearIndex ( x2, y2, z );
                    if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }


                for ( int x2 = x - 1, z2 = z - 1; x2 >= bremain.XMin () && z2 >= bremain.ZMin (); x2--, z2-- ) {
                    j   = IndexesToLinearIndex ( x2, y, z2 );
                    if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x - 1, z2 = z + 1; x2 >= bremain.XMin () && z2 <= bremain.ZMax (); x2--, z2++ ) {
                    j   = IndexesToLinearIndex ( x2, y, z2 );
                    if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x + 1, z2 = z - 1; x2 <= bremain.XMax () && z2 >= bremain.ZMin (); x2++, z2-- ) {
                    j   = IndexesToLinearIndex ( x2, y, z2 );
                    if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x + 1, z2 = z + 1; x2 <= bremain.XMax () && z2 <= bremain.ZMax (); x2++, z2++ ) {
                    j   = IndexesToLinearIndex ( x2, y, z2 );
                    if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }


                for ( int y2 = y - 1, z2 = z - 1; y2 >= bremain.YMin () && z2 >= bremain.ZMin (); y2--, z2-- ) {
                    j   = IndexesToLinearIndex ( x, y2, z2 );
                    if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int y2 = y - 1, z2 = z + 1; y2 >= bremain.YMin () && z2 <= bremain.ZMax (); y2--, z2++ ) {
                    j   = IndexesToLinearIndex ( x, y2, z2 );
                    if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int y2 = y + 1, z2 = z - 1; y2 <= bremain.YMax () && z2 >= bremain.ZMin (); y2++, z2-- ) {
                    j   = IndexesToLinearIndex ( x, y2, z2 );
                    if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int y2 = y + 1, z2 = z + 1; y2 <= bremain.YMax () && z2 <= bremain.ZMax (); y2++, z2++ ) {
                    j   = IndexesToLinearIndex ( x, y2, z2 );
                    if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                } // Neighbors18


                                        // 26 neighbors
            if ( neighborhood >= Neighbors26 ) {

                for ( int x2 = x - 1, y2 = y - 1, z2 = z - 1; x2 >= bremain.XMin () && y2 >= bremain.YMin () && z2 >= bremain.ZMin (); x2--, y2--, z2-- ) {
                    j   = IndexesToLinearIndex ( x2, y2, z2 );
                    if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x - 1, y2 = y - 1, z2 = z + 1; x2 >= bremain.XMin () && y2 >= bremain.YMin () && z2 <= bremain.ZMax (); x2--, y2--, z2++ ) {
                    j   = IndexesToLinearIndex ( x2, y2, z2 );
                    if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x - 1, y2 = y + 1, z2 = z - 1; x2 >= bremain.XMin () && y2 <= bremain.YMax () && z2 >= bremain.ZMin (); x2--, y2++, z2-- ) {
                    j   = IndexesToLinearIndex ( x2, y2, z2 );
                    if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x - 1, y2 = y + 1, z2 = z + 1; x2 >= bremain.XMin () && y2 <= bremain.YMax () && z2 <= bremain.ZMax (); x2--, y2++, z2++ ) {
                    j   = IndexesToLinearIndex ( x2, y2, z2 );
                    if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x + 1, y2 = y - 1, z2 = z - 1; x2 <= bremain.XMax () && y2 >= bremain.YMin () && z2 >= bremain.ZMin (); x2++, y2--, z2-- ) {
                    j   = IndexesToLinearIndex ( x2, y2, z2 );
                    if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x + 1, y2 = y - 1, z2 = z + 1; x2 <= bremain.XMax () && y2 >= bremain.YMin () && z2 <= bremain.ZMax (); x2++, y2--, z2++ ) {
                    j   = IndexesToLinearIndex ( x2, y2, z2 );
                    if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x + 1, y2 = y + 1, z2 = z - 1; x2 <= bremain.XMax () && y2 <= bremain.YMax () && z2 >= bremain.ZMin (); x2++, y2++, z2-- ) {
                    j   = IndexesToLinearIndex ( x2, y2, z2 );
                    if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                for ( int x2 = x + 1, y2 = y + 1, z2 = z + 1; x2 <= bremain.XMax () && y2 <= bremain.YMax () && z2 <= bremain.ZMax (); x2++, y2++, z2++ ) {
                    j   = IndexesToLinearIndex ( x2, y2, z2 );
                    if ( voxelstodo[ j ] ) { edge[ j ] = true; edgecount++; voxelstodo[ j ] = false; numvoxelstodo--; }
                    else break;
                    }

                } // Neighbors26


                                        // inflate edge only, using current point
//            bedge.Set ( max ( bremain.XMin (), min ( bedge.XMin (), x - 1 ) ), min ( bremain.XMax (), max ( bedge.XMax (), x + 1 ) ),
//                        max ( bremain.YMin (), min ( bedge.YMin (), y - 1 ) ), min ( bremain.YMax (), max ( bedge.YMax (), y + 1 ) ),
//                        max ( bremain.ZMin (), min ( bedge.ZMin (), z - 1 ) ), min ( bremain.ZMax (), max ( bedge.ZMax (), z + 1 ) ) );

            } // process edge


                                        // exact box, handles inlfation and deletion
//        bedge       = TBoundingBox<int> ( edge, 0.1 );
                                        // inflate edge only with global increment
        bedge.Set ( max ( bremain.XMin (), bedge.XMin () - 1 ), min ( bremain.XMax (), bedge.XMax () + 1 ),
                    max ( bremain.YMin (), bedge.YMin () - 1 ), min ( bremain.YMax (), bedge.YMax () + 1 ),
                    max ( bremain.ZMin (), bedge.ZMin () - 1 ), min ( bremain.ZMax (), bedge.ZMax () + 1 ) );


        Gauge.SetValue ( SuperGaugeDefaultPart, numvoxelstotal - numvoxelstodo );

        } while ( edgecount );

                                        // update region
    region->Set ();
//    region->Show ( "region grown" );

                                        // enough points?
    int         rnp     = region->GetNumPoints ();

    if ( rnp >= minvoxels && rnp <= maxvoxels ) {
        gor.Add ( region );
        index++;
        }
    else
        delete  region;


//  Gauge.SetValue ( SuperGaugeDefaultPart, numvoxelstotal - numvoxelstodo );

    } while ( numvoxelstodo );

                                        // By decreasing # of points in regions
gor.Sort ( SortRegionsCount );

}
*/

//----------------------------------------------------------------------------
                                        // Second version of ClustersToRegions
                                        // Each geometrical cluster of ANY level becomes a region
template <class TypeD>
void    TVolume<TypeD>::ClustersToRegions ( TVolumeRegions& gor, int minvoxels, int maxvoxels, NeighborhoodType neighborhood, bool showprogress )
{
gor.Reset ();

                                        // coding content to do
enum        VoxelCodeEnum
            {
            VoxelDone       = 0,
            VoxelToDo       = 1,
            VoxelRegion     = 2,
            VoxelSeed       = 3,
            };
                                        // account for the remaining voxels to process - keeping MriType for the TBoundingBox constructor
TVolume<uchar>      voxelcode ( Dim1, Dim2, Dim3 );

OmpParallelFor

for ( int i = 0; i < LinearDim; i++ )
    voxelcode[ i ]  = (uchar) ( Array[ i ] ? VoxelToDo : VoxelDone );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 index           = 1;
TVolumeRegion*      region;

//int                 x,  y,  z;
int                 numvoxelstotal  = voxelcode.GetNumSet ();
int                 numvoxelstodo   = numvoxelstotal;
TBoundingBox<int>   bremain;
TBoundingBox<int>   bseed;
bool                expanded;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( FilterPresets[ FilterTypeClustersToRegions ].Text, showprogress ? numvoxelstotal : 0 );

Gauge.SetValue ( SuperGaugeDefaultPart, 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


do {

    Gauge.SetValue ( SuperGaugeDefaultPart, numvoxelstotal - numvoxelstodo );

                                        // recompute remaining voxels' box at each iteration
    bremain.Set ( voxelcode, false, 0.1 );

                                        // get next voxel seed
    for ( int x = bremain.XMin (); x <= bremain.XMax (); x++ )
    for ( int y = bremain.YMin (); y <= bremain.YMax (); y++ )
    for ( int z = bremain.ZMin (); z <= bremain.ZMax (); z++ )

        if ( voxelcode ( x, y, z ) == VoxelToDo ) {
                                        // single voxel seed
            voxelcode ( x, y, z )   = VoxelSeed;

            bseed  .Set ( x, x, y, y, z, z );

            goto seedexists;
            }

    break;                              // no seed -> exit the main loop

    seedexists:


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // expand seed / region
    int                 numvoxels       = 0;


    do {
//      Gauge.Blink ();


        expanded = false;

        OmpParallelFor

        for ( int x = bseed.XMin (); x <= bseed.XMax (); x++ )
        for ( int y = bseed.YMin (); y <= bseed.YMax (); y++ )
        for ( int z = bseed.ZMin (); z <= bseed.ZMax (); z++ ) {

                                        // expand only from a part of the current region
            if ( voxelcode ( x, y, z ) != VoxelSeed )
                continue;

                                        // continue current expansion if not already done, adding only voxels not yet marked by the current region
#define         regiongrowinginnerloop(J) \
                if      ( voxelcode[ J ] == VoxelToDo )   { voxelcode[ J ] = VoxelSeed; expanded = true; } \
                else if ( voxelcode[ J ] == VoxelDone )     break;
//              else VoxelSeed || VoxelRegion               continue;   // to progress the scan


/*                                        // continue current expansion if not already done,
#define         regiongrowinginnerloop(J) \
                if      ( voxelcode[ J ] == VoxelToDo || voxelcode[ J ] == VoxelSeed )    { voxelcode[ J ] = VoxelSeed; expanded = true; } \
                else if ( voxelcode[ J ] == VoxelDone )                                     break;
//              else VoxelRegion                                                            continue;   // to progress the scan*/


/*                                        // continue current expansion if not already done, marking untouched voxels and re-seeding previous seeds, too
#define         regiongrowinginnerloop(J) \
                if ( voxelcode[ J ] == VoxelToDo || voxelcode[ J ] == VoxelSeed ) { voxelcode[ J ] = VoxelSeed; expanded = true; } \
                else                                                                break;*/


/*#define         regiongrowinginnerloop(J) \
                if ( voxelcode[ J ] == VoxelToDo )    { voxelcode[ J ] = VoxelSeed; expanded = true; } \
                else                                    break;*/


                                        // 6 neighbors
//          if ( neighborhood >= Neighbors6 ) {
                {
                for ( int x2 = x - 1; x2 >= bremain.XMin (); x2-- ) {
                    int     j   = IndexesToLinearIndex ( x2, y, z );
                    regiongrowinginnerloop ( j );
                    }

                for ( int x2 = x + 1; x2 <= bremain.XMax (); x2++ ) {
                    int     j   = IndexesToLinearIndex ( x2, y, z );
                    regiongrowinginnerloop ( j );
                    }

                for ( int y2 = y - 1; y2 >= bremain.YMin (); y2-- ) {
                    int     j   = IndexesToLinearIndex ( x, y2, z );
                    regiongrowinginnerloop ( j );
                    }

                for ( int y2 = y + 1; y2 <= bremain.YMax (); y2++ ) {
                    int     j   = IndexesToLinearIndex ( x, y2, z );
                    regiongrowinginnerloop ( j );
                    }

                for ( int z2 = z - 1; z2 >= bremain.ZMin (); z2-- ) {
                    int     j   = IndexesToLinearIndex ( x, y, z2 );
                    regiongrowinginnerloop ( j );
                    }

                for ( int z2 = z + 1; z2 <= bremain.ZMax (); z2++ ) {
                    int     j   = IndexesToLinearIndex ( x, y, z2 );
                    regiongrowinginnerloop ( j );
                    }

                } // Neighbors6

                                        // 18 neighbors
            if ( neighborhood >= Neighbors18 ) {

                for ( int x2 = x - 1, y2 = y - 1; x2 >= bremain.XMin () && y2 >= bremain.YMin (); x2--, y2-- ) {
                    int     j   = IndexesToLinearIndex ( x2, y2, z );
                    regiongrowinginnerloop ( j );
                    }

                for ( int x2 = x - 1, y2 = y + 1; x2 >= bremain.XMin () && y2 <= bremain.YMax (); x2--, y2++ ) {
                    int     j   = IndexesToLinearIndex ( x2, y2, z );
                    regiongrowinginnerloop ( j );
                    }

                for ( int x2 = x + 1, y2 = y - 1; x2 <= bremain.XMax () && y2 >= bremain.YMin (); x2++, y2-- ) {
                    int     j   = IndexesToLinearIndex ( x2, y2, z );
                    regiongrowinginnerloop ( j );
                    }

                for ( int x2 = x + 1, y2 = y + 1; x2 <= bremain.XMax () && y2 <= bremain.YMax (); x2++, y2++ ) {
                    int     j   = IndexesToLinearIndex ( x2, y2, z );
                    regiongrowinginnerloop ( j );
                    }


                for ( int x2 = x - 1, z2 = z - 1; x2 >= bremain.XMin () && z2 >= bremain.ZMin (); x2--, z2-- ) {
                    int     j   = IndexesToLinearIndex ( x2, y, z2 );
                    regiongrowinginnerloop ( j );
                    }

                for ( int x2 = x - 1, z2 = z + 1; x2 >= bremain.XMin () && z2 <= bremain.ZMax (); x2--, z2++ ) {
                    int     j   = IndexesToLinearIndex ( x2, y, z2 );
                    regiongrowinginnerloop ( j );
                    }

                for ( int x2 = x + 1, z2 = z - 1; x2 <= bremain.XMax () && z2 >= bremain.ZMin (); x2++, z2-- ) {
                    int     j   = IndexesToLinearIndex ( x2, y, z2 );
                    regiongrowinginnerloop ( j );
                    }

                for ( int x2 = x + 1, z2 = z + 1; x2 <= bremain.XMax () && z2 <= bremain.ZMax (); x2++, z2++ ) {
                    int     j   = IndexesToLinearIndex ( x2, y, z2 );
                    regiongrowinginnerloop ( j );
                    }


                for ( int y2 = y - 1, z2 = z - 1; y2 >= bremain.YMin () && z2 >= bremain.ZMin (); y2--, z2-- ) {
                    int     j   = IndexesToLinearIndex ( x, y2, z2 );
                    regiongrowinginnerloop ( j );
                    }

                for ( int y2 = y - 1, z2 = z + 1; y2 >= bremain.YMin () && z2 <= bremain.ZMax (); y2--, z2++ ) {
                    int     j   = IndexesToLinearIndex ( x, y2, z2 );
                    regiongrowinginnerloop ( j );
                    }

                for ( int y2 = y + 1, z2 = z - 1; y2 <= bremain.YMax () && z2 >= bremain.ZMin (); y2++, z2-- ) {
                    int     j   = IndexesToLinearIndex ( x, y2, z2 );
                    regiongrowinginnerloop ( j );
                    }

                for ( int y2 = y + 1, z2 = z + 1; y2 <= bremain.YMax () && z2 <= bremain.ZMax (); y2++, z2++ ) {
                    int     j   = IndexesToLinearIndex ( x, y2, z2 );
                    regiongrowinginnerloop ( j );
                    }

                } // Neighbors18

                                        // 26 neighbors
            if ( neighborhood >= Neighbors26 ) {

                for ( int x2 = x - 1, y2 = y - 1, z2 = z - 1; x2 >= bremain.XMin () && y2 >= bremain.YMin () && z2 >= bremain.ZMin (); x2--, y2--, z2-- ) {
                    int     j   = IndexesToLinearIndex ( x2, y2, z2 );
                    regiongrowinginnerloop ( j );
                    }

                for ( int x2 = x - 1, y2 = y - 1, z2 = z + 1; x2 >= bremain.XMin () && y2 >= bremain.YMin () && z2 <= bremain.ZMax (); x2--, y2--, z2++ ) {
                    int     j   = IndexesToLinearIndex ( x2, y2, z2 );
                    regiongrowinginnerloop ( j );
                    }

                for ( int x2 = x - 1, y2 = y + 1, z2 = z - 1; x2 >= bremain.XMin () && y2 <= bremain.YMax () && z2 >= bremain.ZMin (); x2--, y2++, z2-- ) {
                    int     j   = IndexesToLinearIndex ( x2, y2, z2 );
                    regiongrowinginnerloop ( j );
                    }

                for ( int x2 = x - 1, y2 = y + 1, z2 = z + 1; x2 >= bremain.XMin () && y2 <= bremain.YMax () && z2 <= bremain.ZMax (); x2--, y2++, z2++ ) {
                    int     j   = IndexesToLinearIndex ( x2, y2, z2 );
                    regiongrowinginnerloop ( j );
                    }

                for ( int x2 = x + 1, y2 = y - 1, z2 = z - 1; x2 <= bremain.XMax () && y2 >= bremain.YMin () && z2 >= bremain.ZMin (); x2++, y2--, z2-- ) {
                    int     j   = IndexesToLinearIndex ( x2, y2, z2 );
                    regiongrowinginnerloop ( j );
                    }

                for ( int x2 = x + 1, y2 = y - 1, z2 = z + 1; x2 <= bremain.XMax () && y2 >= bremain.YMin () && z2 <= bremain.ZMax (); x2++, y2--, z2++ ) {
                    int     j   = IndexesToLinearIndex ( x2, y2, z2 );
                    regiongrowinginnerloop ( j );
                    }

                for ( int x2 = x + 1, y2 = y + 1, z2 = z - 1; x2 <= bremain.XMax () && y2 <= bremain.YMax () && z2 >= bremain.ZMin (); x2++, y2++, z2-- ) {
                    int     j   = IndexesToLinearIndex ( x2, y2, z2 );
                    regiongrowinginnerloop ( j );
                    }

                for ( int x2 = x + 1, y2 = y + 1, z2 = z + 1; x2 <= bremain.XMax () && y2 <= bremain.YMax () && z2 <= bremain.ZMax (); x2++, y2++, z2++ ) {
                    int     j   = IndexesToLinearIndex ( x2, y2, z2 );
                    regiongrowinginnerloop ( j );
                    }

                } // Neighbors26


            voxelcode ( x, y, z )  = VoxelRegion;
                                        // one more voxel
            OmpAtomic
            numvoxels++;
            } // for x, y, z
                                        // force edge box to be the whole remaining data - seems faster (!?)
        bseed       = bremain;

        } while ( expanded );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Region has fully grown here
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // update remaining nunmber of voxels
    numvoxelstodo  -= numvoxels;

                                        // we might create a region with the right size straight away?
    region          = IsInsideLimits ( numvoxels, minvoxels, maxvoxels )    ?   new TVolumeRegion ( this, index )  :   0;


    OmpParallelFor

    for ( int x = bremain.XMin (); x <= bremain.XMax (); x++ )
    for ( int y = bremain.YMin (); y <= bremain.YMax (); y++ )
    for ( int z = bremain.ZMin (); z <= bremain.ZMax (); z++ ) {

        if ( voxelcode ( x, y, z ) == VoxelRegion ) {
//          OmpCriticalBegin (ClustersToRegions)
                                        // if region OK
            if ( region )   (*region) ( x, y, z )   = 1;
                                        // reset in all cases
            voxelcode ( x, y, z ) = VoxelDone;

//          OmpCriticalEnd
            }
        }


    if ( region ) {
                                        // update region: compacting memory & computing stats
        region->Set ( true );

//      if ( VkQuery () )  region->Show ( "region scanned" );

        gor.Add ( region );

        index++;
        }

    } while ( numvoxelstodo >= minvoxels );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // By decreasing # of points in regions
gor.Sort ( SortRegionsCount );

}


//----------------------------------------------------------------------------
                                        // encapsulates ClustersToRegions
template <class TypeD>
bool    TVolume<TypeD>::ClustersToRegions ( FctParams& params, NeighborhoodType neighborhood, bool showprogress )
{
                                        // set global parameters
int                 minvoxels       = params ( FilterParamRegionMinSize );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TVolumeRegions      gor;


ClustersToRegions ( gor, minvoxels, INT_MAX, neighborhood, showprogress );

//gor.Show ( "ClustersToRegions" );


if ( (int) gor ) {
    *this           = gor;
    return  true;
    }
else {
    ResetMemory ();
    return  false;
    }
}


//----------------------------------------------------------------------------
                                        // encapsulates LevelsClustersToRegions / ClustersToRegions when we just want the single "best" region
template <class TypeD>
bool    TVolume<TypeD>::KeepRegion      (   VolumeRegionsSort   sortcriterion, 
                                            int                 minvoxels,      int             maxvoxels, 
                                            NeighborhoodType    neighborhood,   double          probregion2, 
                                            bool                showprogress 
                                        )
{
TVolumeRegions      gor;

                                        // grab the remaining big bit
//LevelsClustersToRegions ( gor, minvoxels, maxvoxels, neighborhood, showprogress );
ClustersToRegions       ( gor, minvoxels, maxvoxels, neighborhood, showprogress );


gor.Sort ( sortcriterion );


if ( (int) gor ) {

    if ( (int) gor > 1 ) {

        gor[ 1 ].Index  = 1;
        *this           = gor[ 1 ];
                                        // we may have more than 1 region, take the second one if very similar
                                        // can also compare 1/2 / 2/3 + 1/3; 1/2 + 1/3
        if ( probregion2 && gor[ 1 ].ComparedTo ( gor[ 2 ] ) > probregion2 ) {
            gor[ 2 ].Index  = 1;
            *this          |= gor[ 2 ];
            }
        }
    else {
        gor[ 1 ].Index  = 1;
        *this           = gor[ 1 ];
        }

    return  true;
    }
else {
    ResetMemory ();

    return  false;
    }
}


//----------------------------------------------------------------------------
                                        // grow from a single region
template <class TypeD>
void    TVolume<TypeD>::RegionGrowing   (   TVolumeRegion&          region,
                                            RegionGrowingFlags      how,
                                            FctParams&              params,
                                            const Volume*           mask, 
                                            bool                    showprogress  
                                        )
{
if ( region.IsEmpty () || IsNotAllocated () )
    return;

                                        // convert region to volume
Volume              regionvol    ( Dim1, Dim2, Dim3 );
                                        // value set to boolean, in case of filters with real values
//int                 index           = region.Index;
//region.Index    = true;
regionvol       = region;
//region.Index    = index;


RegionGrowing ( regionvol, how, params, neighborhood, mask, showprogress  );

                                        // convert volume back to region
region  = regionvol;
}

                                        // grow from a volume (single region for now)
template <class TypeD>
void    TVolume<TypeD>::RegionGrowing   (   Volume&                 regionvol,
                                            RegionGrowingFlags      how,
                                            FctParams&              params,
                                            const Volume*           mask, 
                                            bool                    showprogress  
                                        )
{
if ( regionvol.IsNotAllocated () || IsNotAllocated () )
    return;


double              precision       = 1e-6;

NeighborhoodType    neighborhood    = (NeighborhoodType) params ( RegionGrowingNeighborhood );

double              centertolerance = fabs ( params ( RegionGrowingTolerance ) );   // 0.5: compact;    0.75:about half Normal area;    1.17741:Normal probability>0.5

double              lessneighthan   = Clip ( params ( RegionGrowingLessNeighborsThan ), 0.0, 1.0 );

double              statsradius     = AtLeast ( 1.735 /*1.0*/, fabs ( params ( RegionGrowingLocalStatsWidth ) ) / 2.0 );    // default min to 26 neighbors
double              statsradius2    = Square ( statsradius );
                                        // 0 -> ignore & use default; 1 -> original mask; 2+ -> number of max layers
int                 maxiterations   = params ( RegionGrowingMaxIterations ) > 0 ? Clip ( (int) params ( RegionGrowingMaxIterations ), 1, RegionGrowingMaxIterationsDefault ) 
                                                                                : RegionGrowingMaxIterationsDefault;

//DBGV2 ( params ( RegionGrowingLocalStatsWidth ), statsradius, "Local Stats: Width -> Radius" );

//bool              growregion      = IsFlag ( how, GrowRegion );
bool                moveregion      = IsFlag ( how, MoveRegion );

bool                globalstats     = IsFlag ( how, GlobalStats );
bool                localstats      = IsFlag ( how, LocalStats  );
bool                statsmask       = IsFlag ( how, StatsMask   );
bool                statsring       = IsFlag ( how, StatsRing   );

if ( globalstats + localstats != 1 ) {
    globalstats     = true;
    localstats      = false;
    }

if ( statsmask + statsring != 1 ) {
    statsmask       = true;
    statsring       = false;
    }
                                        // simpler test or leaking test?
bool                testneighbor    = /*IsFlag ( how, TestNeighbor             ) */ ! IsFlag ( how, TestCenterToNeighbor        );
                                        // by security, no flags will default to abs for both, the most logical formula
bool                regiontestabs   = /*IsFlag ( how, RegionDistanceTestAbs    ) */ ! IsFlag ( how, RegionDistanceTestSigned    );
bool                nonregiontestabs= /*IsFlag ( how, NonRegionDistanceTestAbs ) */ ! IsFlag ( how, NonRegionDistanceTestSigned );
//DBGV3 ( testneighbor, regiontestabs, nonregiontestabs, "testneighbor, regiontestabs, nonregiontestabs" );

bool                lessneighbors   = IsFlag ( how, RemoveLessNeighbors );
bool                median          = IsFlag ( how, RemoveMedian        );
//DBGV2 ( lessneighbors, median, "lessneighbors, median" );



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // convert region to volume
Volume              regionvolnew ( Dim1, Dim2, Dim3 );  // we can avoid this volume by using 2 different binary values, like 0x1 and 0x2


FctParams           p;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( FilterPresets[ FilterTypeRegionGrowing ].Text, showprogress ? 100 : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // Gradient trial: control expansion according to gradient
TVolume<TypeD>      temp ( *this );
TArray3< TVector3Float >    gradvect ( Dim1, Dim2, Dim3 );


p ( FilterParamDiameter   ) = params ( RegionGrowingLocalStatsWidth );
p ( FilterParamResultType ) = FilterResultPositive;

temp.FilterPartDeriv ( FilterTypeGradient, p, &gradvect );

//double              maxgradient     = temp.GetMaxValue ();

temp.DeallocateMemory ();

                                        // normalize all gradients
for ( int li = 0; li < gradvect.GetLinearDim (); li++ )
    gradvect ( li ).Normalize ();
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// 
//double            dampfactor      = 1;

double              regionavgglob       = 0;
double              regionsdglob        = 0;
double              nonregionavgglob    = 0;
double              nonregionsdglob     = 0;

int                 iteration;
int                 numset;
int                 oldnumset       = 0;
double              convergence;
double              convergenceold  = 0;
bool                smartneighborhood   = neighborhood == SmartNeighbors;
bool                bigneighborhood;

TEasyStats          globalregionstat;
TEasyStats          globalnonregionstat;
//TEasyStats          statlocal;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // seed is already 1
iteration       = 1;

                                        // binarize mask to 1, so we can track "ring growth"
OmpParallelFor

for ( int i = 0; i < LinearDim; i++ )
    regionvol[ i ]  = (MriType) ( ! ( mask && ! mask->GetValue ( i ) ) && regionvol[ i ] ? iteration : 0 );

                                        // first growth index will therefore be 2
iteration++;

                                        // test here is for consistency only, this would simply return the input mask
if ( iteration > maxiterations )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

do {
                                        // processing can be quite long, refresh the app once in a while
    UpdateApplication;

                                        // damping factor
//  dampfactor    = (double) ( 1 + 3 ) / ( iteration - 1 + 3 );   // 0.5 after 5 iterations


//  if ( localstats )
//      statlocal.Reset ();

                                        // alternate the actual neighborhood
    if ( smartneighborhood ) {
        bigneighborhood = IsOdd ( iteration );
        neighborhood    = bigneighborhood ? Neighbors26 : Neighbors6;
        }

    #define     IsVoxelMask(V)  (bool) (V)
    #define     IsVoxelRing(V)  ( (V) == iteration - 1 )


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Compute global stats here
    if ( globalstats ) {

        globalregionstat   .Reset ();
        globalnonregionstat.Reset ();


        for ( int i = 0; i < LinearDim; i++ ) {

            if ( mask && ! mask->GetValue ( i ) )
                continue;


            if ( statsmask && IsVoxelMask ( regionvol[ i ] )
              || statsring && IsVoxelRing ( regionvol[ i ] ) )

                globalregionstat.Add ( GetValue ( i ) );


            else if ( ! IsVoxelMask ( regionvol[ i ] ) ) {  // not inside global mask

                int                 x,  y,  z;
                LinearIndexToXYZ ( i, x, y, z );

                if ( regionvol.GetNumNeighbors ( x, y, z, neighborhood ) > 0 )

                    globalnonregionstat.Add ( GetValue ( i ) );
                }

//            else if ( GetValue ( i ) != 0 && ! IsVoxelMask ( regionvol[ i ] ) )
            }


        regionavgglob       = globalregionstat   .Average ();
        regionsdglob        = globalregionstat   .SD      ();
        nonregionavgglob    = globalnonregionstat.Average ();
        nonregionsdglob     = globalnonregionstat.SD      ();
        } // globalstats


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Growing region
                                        // make a copy of current region
    regionvolnew.Insert ( regionvol );


    OmpParallelBegin

    double              regionavg;
    double              regionsd;
    double              nonregionavg;
    double              nonregionsd;
    TEasyStats          localregionstat;
    TEasyStats          localnonregionstat;

    OmpFor

    for ( int i = 0; i < LinearDim; i++ ) {

        if ( mask && ! mask->GetValue ( i ) )
            continue;

        if ( ! ( statsmask && IsVoxelMask ( regionvol[ i ] )
              || statsring && IsVoxelRing ( regionvol[ i ] ) ) )
            continue;


        int                 x;
        int                 y;
        int                 z;
        LinearIndexToXYZ ( i, x, y, z );

        double              centervalue     = GetValue ( i );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do local stats around current voxel
        if ( localstats ) {

            int                 xmin            = AtLeast ( 0,          x - (int) statsradius );
            int                 xmax            = NoMore  ( Dim1 - 1,   x + (int) statsradius );
            int                 ymin            = AtLeast ( 0,          y - (int) statsradius );
            int                 ymax            = NoMore  ( Dim2 - 1,   y + (int) statsradius );
            int                 zmin            = AtLeast ( 0,          z - (int) statsradius );
            int                 zmax            = NoMore  ( Dim3 - 1,   z + (int) statsradius );


            localregionstat   .Reset ();
            localnonregionstat.Reset ();

                                        // cubic local neighborhood
            for ( int xk = xmin; xk <= xmax; xk++ )
            for ( int yk = ymin; yk <= ymax; yk++ )
            for ( int zk = zmin; zk <= zmax; zk++ ) {

                int             in          = IndexesToLinearIndex ( xk, yk, zk );

                if ( mask && ! mask->GetValue ( in ) )
                    continue;

                                        // inside spherical neighborhood?
                if ( Square ( (double) ( xk - x ) ) + Square ( (double) ( yk - y ) ) + Square ( (double) ( zk - z ) ) > statsradius2 )
                    continue;


                if ( statsmask && IsVoxelMask ( regionvol[ in ] )
                  || statsring && IsVoxelRing ( regionvol[ in ] ) )

                    localregionstat.Add ( GetValue ( in ), ThreadSafetyIgnore );

                else if ( /*GetValue ( in ) != 0 && or Mask */ ! IsVoxelMask ( regionvol[ in ] ) ) // always global mask
                                        // border of the ring
//                  if ( regionvol.GetNumNeighbors ( xk, yk, zk, neighborhood ) > 0 )
                                        // we don't have enough samples in the ring-not-in-mask, so use everything not the mask
                        localnonregionstat.Add ( GetValue ( in ), ThreadSafetyIgnore );

                } // for xk, yk, zk


            regionavg       = localregionstat   .Average ();
            regionsd        = localregionstat   .SD      ();
            nonregionavg    = localnonregionstat.Average ();
            nonregionsd     = localnonregionstat.SD      ();
                                        // we need a global average for convergence purpose
//          statlocal.Add ( regionavg, ? );

//          dampfactor    = localregionstat.GetNumItems () / 27.0;   // less samples means less reliability
            } // localstats
        else {
            regionavg       = regionavgglob;
            regionsd        = regionsdglob;
            nonregionavg    = nonregionavgglob;
            nonregionsd     = nonregionsdglob;
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Use some defines for easier tuning of criterion

        #define     DistCenterToRegion      ZScore ( centervalue, regionavg,    regionsd,    regiontestabs    )
        #define     DistNeighborToRegion    ZScore ( v,           regionavg,    regionsd,    regiontestabs    )
        #define     DistNeighborToNonRegion ZScore ( v,           nonregionavg, nonregionsd, nonregiontestabs )

                                        // test if neighbor is closer to region or non-region
        #define     TestNeighborDist            ( DistNeighborToRegion < DistNeighborToNonRegion )

                                        // Test if central voxel is close enough to the region stats
        #define     TestCenterDist              ( fabs ( DistCenterToRegion ) < centertolerance )

                                        // test if central voxel could leak to neighbor (central closer to region than neighbor to non-region) (signed-signed or signed-abs for best results)
        #define     TestCenterToNeighborDist    ( DistCenterToRegion < DistNeighborToNonRegion )

                                        // test if further from non-region - Needs TestCenterDist && TestCenterToNeighborDist - quite conservative white-part
//      #define     TestNeighborNotRegionDist   ( fabs ( DistNeighborToNonRegion ) > 2 )

                                        // gradient trial, not very conclusive
//      #define     TestGradient                ( gradvect[ i ] * gradvect[ j ] > -0.707 )

                                        // simple neighbor test
//      #define     TestVoxel                 (MriType) ( TestNeighborDist ? iteration : 0 )
                                        // center leaking to neighbors
//      #define     TestVoxel                   (MriType) ( TestCenterToNeighborDist && TestCenterDist ? iteration : 0 )
                                        // test is defined by caller
        #define     TestVoxel                   (MriType) ( ( testneighbor ? TestNeighborDist : TestCenterDist && TestCenterToNeighborDist ) ? iteration : 0 )


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Moving: an already set voxel can be revoked if it does not fit to region, which will change the region stats & move the region spreading
//        if ( moveregion /*&& regionvol[ i ]*/ ) { v = centervalue; if ( ! TestVoxel )    regionvolnew[ i ]  = 0; }


                                        // Test if central voxel is close enough to the region stats
                                        // if not, skip processing - gives thicker results
        if ( ! TestCenterDist ) {
                                        // Moving: an already set voxel can be revoked if it does not fit to region, which will change the region stats & move the region spreading
            if ( moveregion )
                regionvolnew[ i ]   = 0;

            continue;
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // update neighbors
                                        // 6 neighbors

//OmpCritical ( regionvolnew[ j ] = t );    // access does not seem to need this
//OmpFlush ( regionvolnew );                // after update - maybe slow?
//Maxed ( regionvolnew[ j ], t );           // does the same job
                                        // current x,y,z will access and update neighbors, concurrently, so some care has to be taken when updating regionvolnew
                                        // !TestVoxel macro uses value 'v' - at least it is local!
                                        // OmpCritical ( regionvolnew[ j ]   = t );  does not seem necessary, though, in the unlikely event 2 threads are overwriting, mask will be set anyway
#define UpdateNeighbor(X,Y,Z) { \
int         j       = IndexesToLinearIndex ( X, Y, Z ); \
if ( ! ( ( mask && mask->GetValue ( j ) == 0 ) || regionvol[ j ] || regionvolnew[ j ] ) ) { \
    TypeD       v       = GetValue ( j );       \
    TypeD       t       = TestVoxel;            \
    if ( t != 0 )   regionvolnew[ j ]   = t;    \
    } \
}

//      if ( neighborhood >= Neighbors6 ) {
            {
            if ( x > 0        )                                     UpdateNeighbor ( x - 1, y, z );
            if ( x < Dim1 - 1 )                                     UpdateNeighbor ( x + 1, y, z );
            if ( y > 0        )                                     UpdateNeighbor ( x, y - 1, z );
            if ( y < Dim2 - 1 )                                     UpdateNeighbor ( x, y + 1, z );
            if ( z > 0        )                                     UpdateNeighbor ( x, y, z - 1 );
            if ( z < Dim3 - 1 )                                     UpdateNeighbor ( x, y, z + 1 );
            } // Neighbors6

                                        // 18 neighbors
        if ( neighborhood >= Neighbors18 ) {
            x--; y--;           if ( WithinBoundary ( x, y, z ) )   UpdateNeighbor ( x, y, z );
            x+=2;               if ( WithinBoundary ( x, y, z ) )   UpdateNeighbor ( x, y, z );
            y+=2;               if ( WithinBoundary ( x, y, z ) )   UpdateNeighbor ( x, y, z );
            x-=2;               if ( WithinBoundary ( x, y, z ) )   UpdateNeighbor ( x, y, z );
            x++; y--;

            x--; z--;           if ( WithinBoundary ( x, y, z ) )   UpdateNeighbor ( x, y, z );
            x+=2;               if ( WithinBoundary ( x, y, z ) )   UpdateNeighbor ( x, y, z );
            z+=2;               if ( WithinBoundary ( x, y, z ) )   UpdateNeighbor ( x, y, z );
            x-=2;               if ( WithinBoundary ( x, y, z ) )   UpdateNeighbor ( x, y, z );
            x++; z--;

            y--; z--;           if ( WithinBoundary ( x, y, z ) )   UpdateNeighbor ( x, y, z );
            y+=2;               if ( WithinBoundary ( x, y, z ) )   UpdateNeighbor ( x, y, z );
            z+=2;               if ( WithinBoundary ( x, y, z ) )   UpdateNeighbor ( x, y, z );
            y-=2;               if ( WithinBoundary ( x, y, z ) )   UpdateNeighbor ( x, y, z );
            y++; z--;
            } // Neighbors18

                                        // 26 neighbors
        if ( neighborhood >= Neighbors26 ) {
            x--; y--; z--;      if ( WithinBoundary ( x, y, z ) )   UpdateNeighbor ( x, y, z );
            z+=2;               if ( WithinBoundary ( x, y, z ) )   UpdateNeighbor ( x, y, z );
            y+=2; z-=2;         if ( WithinBoundary ( x, y, z ) )   UpdateNeighbor ( x, y, z );
            z+=2;               if ( WithinBoundary ( x, y, z ) )   UpdateNeighbor ( x, y, z );
            x+=2; y-=2; z-=2;   if ( WithinBoundary ( x, y, z ) )   UpdateNeighbor ( x, y, z );
            z+=2;               if ( WithinBoundary ( x, y, z ) )   UpdateNeighbor ( x, y, z );
            y+=2; z-=2;         if ( WithinBoundary ( x, y, z ) )   UpdateNeighbor ( x, y, z );
            z+=2;               if ( WithinBoundary ( x, y, z ) )   UpdateNeighbor ( x, y, z );
//          x--; y--; z--;
            } // Neighbors26

        } // for each voxel mask

    OmpParallelEnd


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//    statloop.Show ( "stats loop" );
                                        // update new region
    regionvol.Insert ( regionvolnew );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3.1) Filtering current progress
                                        // Median can add and delete current voxels
    if ( median ) {
//  if ( median && ( ( iteration - 1 ) % 2 == 0 ) ) { // filtering at each iteration can be too drastic?

//      p ( FilterParamDiameter   ) = 2 * statsradius;
//      p ( FilterParamResultType ) = FilterResultSigned;
//      regionvol.Filter ( FilterTypeMedian, p, true );

/*
        int                 medianthresh        = Neighborhood[ neighborhood ].NumNeighbors / 2;
                                        // faster boolean median, for a small neighborhood (< 3.47)
                                        // but we need a temp copy
        regionvolnew.Insert ( regionvol );

        for ( int i = 0; i < LinearDim; i++ ) {

//          if ( mask && ! mask->GetValue ( i ) ) {
//              regionvol[ i ]  = 0;
//              continue;
//              }

            if ( regionvolnew[ i ] )    // prevent deletion of existing voxel
                continue;

            LinearIndexToXYZ ( i, x, y, z );
                                                                                                                              // keeps coherence with growth ring
//          regionvol[ i ]  = (MriType) ( regionvolnew.GetNumNeighbors ( x, y, z, neighborhood ) + (bool) regionvolnew[ i ] > medianthresh ? iteration : 0 );
            regionvol[ i ]  = regionvolnew.GetNumNeighbors ( x, y, z, neighborhood ) + (bool) regionvolnew[ i ] > medianthresh;
            } // for i
*/

                                        // !not needed right now, as we just copied the results above!
//      regionvolnew.Insert ( regionvol );


        for ( int i = 0; i < LinearDim; i++ ) {
                                        // skip non-mask voxels, also reset any out-of-mask voxels, just in case
            if ( mask && ! mask->GetValue ( i ) ) {
                regionvol[ i ]  = 0;
                continue;
                }

                                        // prevent deletion of existing voxel
            if ( regionvol[ i ] && ! moveregion )
                continue;


            int                 x,  y,  z;
            LinearIndexToXYZ ( i, x, y, z );


            int                 medianthresh    = 0;    // number max of neighbors, based on current neighborhood & mask
            int                 numneigh        = 0;    // number of acutal neighbors, also based on current neighborhood & mask
            int                 j;

    //      if ( neighborhood >= Neighbors6 ) {
                {
                if ( x > 0 ) {
                    j   = IndexesToLinearIndex ( x - 1, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }

                if ( x < Dim1 - 1 ) {
                    j   = IndexesToLinearIndex ( x + 1, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }

                if ( y > 0 ) {
                    j   = IndexesToLinearIndex ( x, y - 1, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }

                if ( y < Dim2 - 1 ) {
                    j   = IndexesToLinearIndex ( x, y + 1, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }

                if ( z > 0 ) {
                    j   = IndexesToLinearIndex ( x, y, z - 1 );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }

                if ( z < Dim3 - 1 ) {
                    j   = IndexesToLinearIndex ( x, y, z + 1 );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
                } // Neighbors6

                                        // 18 neighbors
            if ( neighborhood >= Neighbors18 ) {
                x--; y--;
                if ( WithinBoundary ( x, y, z ) ) {
                    j   = IndexesToLinearIndex ( x, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
                x+=2;
                if ( WithinBoundary ( x, y, z ) ) {
                    j   = IndexesToLinearIndex ( x, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
                y+=2;
                if ( WithinBoundary ( x, y, z ) ) {
                    j   = IndexesToLinearIndex ( x, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
                x-=2;
                if ( WithinBoundary ( x, y, z ) ) {
                    j   = IndexesToLinearIndex ( x, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
                x++; y--;


                x--; z--;
                if ( WithinBoundary ( x, y, z ) ) {
                    j   = IndexesToLinearIndex ( x, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
                x+=2;
                if ( WithinBoundary ( x, y, z ) ) {
                    j   = IndexesToLinearIndex ( x, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
                z+=2;
                if ( WithinBoundary ( x, y, z ) ) {
                    j   = IndexesToLinearIndex ( x, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
                x-=2;
                if ( WithinBoundary ( x, y, z ) ) {
                    j   = IndexesToLinearIndex ( x, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
                x++; z--;


                y--; z--;
                if ( WithinBoundary ( x, y, z ) ) {
                    j   = IndexesToLinearIndex ( x, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
                y+=2;
                if ( WithinBoundary ( x, y, z ) ) {
                    j   = IndexesToLinearIndex ( x, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
                z+=2;
                if ( WithinBoundary ( x, y, z ) ) {
                    j   = IndexesToLinearIndex ( x, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
                y-=2;
                if ( WithinBoundary ( x, y, z ) ) {
                    j   = IndexesToLinearIndex ( x, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
                y++; z--;
                } // Neighbors18

                                        // 26 neighbors
            if ( neighborhood >= Neighbors26 ) {
                x--; y--; z--;
                if ( WithinBoundary ( x, y, z ) ) {
                    j   = IndexesToLinearIndex ( x, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
                z+=2;
                if ( WithinBoundary ( x, y, z ) ) {
                    j   = IndexesToLinearIndex ( x, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
                y+=2; z-=2;
                if ( WithinBoundary ( x, y, z ) ) {
                    j   = IndexesToLinearIndex ( x, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
                z+=2;
                if ( WithinBoundary ( x, y, z ) ) {
                    j   = IndexesToLinearIndex ( x, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
                x+=2; y-=2; z-=2;
                if ( WithinBoundary ( x, y, z ) ) {
                    j   = IndexesToLinearIndex ( x, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
                z+=2;
                if ( WithinBoundary ( x, y, z ) ) {
                    j   = IndexesToLinearIndex ( x, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
                y+=2; z-=2;
                if ( WithinBoundary ( x, y, z ) ) {
                    j   = IndexesToLinearIndex ( x, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
                z+=2;
                if ( WithinBoundary ( x, y, z ) ) {
                    j   = IndexesToLinearIndex ( x, y, z );
                    if ( ! mask || mask->GetValue ( j ) )   {   medianthresh++; if ( regionvolnew[ j ] )  numneigh++; }
                    }
    //          x--; y--; z--;
                } // Neighbors26


                                        // compute locally constrained median, without the central voxel
            if ( numneigh > medianthresh / 2 ) {
                if ( ! moveregion )                         // region is null, set only
                    regionvol[ i ]  = (MriType) iteration;
                }
            else
                if (   moveregion )                         // if test is false, reset only
                    regionvol[ i ]  = (MriType) 0;
            } // for i

        } // if median


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3.2) Filtering current progress
                                        // Less drastic than Median: removing voxels with less neighbors than parameter, to prevent small leaks to happen
    if ( lessneighbors ) {
        p ( FilterParamNumNeighbors )     = Neighborhood[ neighborhood ].NumNeighbors * lessneighthan;
        p ( FilterParamNeighborhood )     = neighborhood;
        regionvol.Filter ( FilterTypeLessNeighbors, p );
        } // if lessneighbors


//    p ( FilterParamDiameter )     = 2;
//    regionvol.Filter ( FilterTypeClose, p );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 4) Test convergence

                                        // convergence by number of points set
    numset          = regionvol.GetNumSet ();
    convergence     = RelativeDifference ( numset, oldnumset );


    Gauge.SetValue ( SuperGaugeDefaultPart, Percentage ( 1 - convergence, 1 ) );

                                        // we're done precision-wise
    if ( convergence < precision )
        break;

                                        // oops, error is increasing!
    if ( iteration > 2
      && ! ( smartneighborhood && bigneighborhood )
      && convergence > convergenceold               ) {

                                        // rewind to previous iteration
                                        // center to neighbor test seems to add way too much voxels, while central testing looks more OK (?)
        if ( ! testneighbor )
            for ( int i = 0; i < LinearDim; i++ )
                if ( regionvol[ i ] == iteration )  regionvol[ i ]  = 0;

        break;
        }


    oldnumset       = numset;
//  regionavgold    = regionavg;
    convergenceold  = convergence;

    iteration++;

                                        // reached the max number of iterations?
    if ( iteration > maxiterations )
        break;

    } while ( true );

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
