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

#include    "TMicroStates.h"

#include    "TVolumeDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Init array
void    TMicroStates::PreProcOrdering   (   int             nclusters,  
                                            TArray2<int>&   ordering    
                                        )   const
{
ordering.Resize ( nclusters, numscanvar );

                                        // init ordering indexes
for ( int nc = 0; nc < nclusters; nc++ ) {
                                        // no touchy, please!
    ordering ( nc, scanindex            )  = nc;
    ordering ( nc, scanbackindex        )  = nc;
                                               // free to use
    ordering ( nc, scanmintf            )  = Highest<long> ();
    ordering ( nc, scanmeantf           )  = 0;
    ordering ( nc, scannumtf            )  = 0;
    ordering ( nc, scanfromtemplates    )  = -1;
    ordering ( nc, scanorient           )  = 0;
    }
}

                                        // Resolve back index - the one actually used in the end
void    TMicroStates::PostProcOrdering  (   TArray2<int>&   ordering    
                                        )   const
{
                                        // find backward indexes
for ( int nc1 = 0; nc1 < ordering.GetDim1 (); nc1++ )
for ( int nc2 = 0; nc2 < ordering.GetDim1 (); nc2++ )

    if ( ordering ( nc2, scanindex ) == nc1 )

        ordering ( nc1, scanbackindex ) = nc2;
}


//----------------------------------------------------------------------------
void    TMicroStates::GetTemporalOrdering   (   int                 nclusters,  
                                                const TLabeling&    labels,  
                                                TArray2<int>&       ordering    
                                            )   const
{
PreProcOrdering ( nclusters, ordering );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int                 curreeg         = 0;

                                        // scan all beginnings of segments, relatively to each file
for ( long tf = 0, tf0 = 0; tf < NumTimeFrames; tf++, tf0++ ) {

    if ( tf0 == NumTF[ curreeg ] ) {
        tf0     = 0;
        curreeg++;
        }

    if ( labels.IsUndefined ( tf ) )
        continue;

                                        // to sort by first appearance
    ordering ( labels[ tf ], scanmintf )    = min ( (int) tf0, (int) ordering ( labels[ tf ], scanmintf ) );
                                        // to sort by mean position
    ordering ( labels[ tf ], scanmeantf )  += tf0;
    ordering ( labels[ tf ], scannumtf  )  ++;
    }


                                        // finalize mean position
for ( int nc = 0; nc < nclusters; nc++ )
    ordering ( nc, scanmeantf ) = ordering ( nc, scannumtf ) ? Round ( ordering ( nc, scanmeantf ) / (double) NonNull ( ordering ( nc, scannumtf ) ) ) 
                                                             : Highest<int> ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // sort by increasing appearance
ordering.SortRows ( /*scanmintf*/ scanmeantf, Ascending );

                                        // find backward indexes
PostProcOrdering ( ordering );
}


//----------------------------------------------------------------------------
                                        // Maps and Templates Maps CAN have different number of maps
                                        // Each input map will simply be allocated to the closest template one
                                        //  - If there are more maps, this will sequentially group them together, like "1 1 1 2 2 3 4 4"
                                        //  - If there are less maps, it will simply follow the templates sequence, skipping the non-matching ones, like "1 3 4"
void    TMicroStates::GetOrderingFromTemplates  (   int                     nclusters,  
                                                    const TMaps&            maps,
                                                    const TMaps&            templatesmaps,
                                                    PolarityType            polarity,
                                                    TArray2<int>&           ordering    
                                                )   const
{
Mined ( nclusters, maps.GetNumMaps () );    // by safety
int                 numtemplates    = templatesmaps.GetNumMaps ();

if ( nclusters == 0 || numtemplates == 0 
  || maps.GetDimension () != templatesmaps.GetDimension () )

    return;


PreProcOrdering ( nclusters, ordering );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int nc = 0; nc < nclusters; nc++ ) {
    
    double      bestcorr    = Lowest ( bestcorr );
    
    for ( int tmi = 0; tmi < numtemplates; tmi++ ) {

        double      corr    = Project ( maps[ nc ], templatesmaps[ tmi ], polarity );

        if ( corr >= bestcorr ) {
            bestcorr                            = corr;
            ordering ( nc, scanfromtemplates )  = tmi;  // store index of best match
            }
        } // for template
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // sort by increasing orientation criterion
ordering.SortRows ( scanfromtemplates, Ascending );

                                        // find backward indexes
PostProcOrdering ( ordering );
}


//----------------------------------------------------------------------------
void    TMicroStates::GetTopographicalOrdering  (   int                     nclusters,  
                                                    const TMaps&            maps,
                                                    const TElectrodesDoc*   xyzdoc,
                                                    TArray2<int>&           ordering    
                                                )   const
{
PreProcOrdering ( nclusters, ordering );

                                        // or can think of a fall back sort in case of no xyz?
if ( xyzdoc == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//TMaps                   synthmaps   ( nclusters, NumElectrodes );
//TArray1<TVector3Float>  dipoles     ( nclusters );
double                  azimut;
double                  vertical;

TPoints             Points          = xyzdoc->GetPoints ( DisplaySpaceProjected );
TGoEasyStats        stats ( 2 );
int                 maxsp;
int                 minsp;


for ( int nc = 0; nc < nclusters; nc++ ) {
                                        // get indexes of min and max SP    
    minsp           = maps[ nc ].GetMinPosition ();
    maxsp           = maps[ nc ].GetMaxPosition ();


    azimut          = RadiansToDegrees ( ArcTangent ( ( Points[ maxsp ].Y - Points[ minsp ].Y ), Points[ maxsp ].X - Points[ minsp ].X ) ) - 90;

    azimut          = NormalizeAngle ( azimut, 180 );

    azimut          = fabs ( ( azimut - 90 ) / 90 );
                                        // increasing: forward facing | to lateral facing _
//  ordering ( nc, scanorient ) = fabs ( ( azimut - 90 ) / 90 ) * 10000;



    stats.Reset ();

    for ( int e1 = 0; e1 < NumElectrodes; e1++ ) 

        if ( maps[ nc ][ e1 ] > 0 )
            stats ( 1 ).Add ( ( Points[ e1 ] - Points[ maxsp ] ).Norm () );
        else
            stats ( 0 ).Add ( ( Points[ e1 ] - Points[ minsp ] ).Norm () );


//  vertical        = stats ( 0 ).Variance () + stats ( 1 ).Variance ();                // not bad
    vertical        = RelativeDifference ( stats ( 0 ).SD   (), stats ( 1 ).SD   () )   // good
                    + RelativeDifference ( stats ( 0 ).Mean (), stats ( 1 ).Mean () );  // also good
                                        // increasing: from half moon to full moon
//  ordering ( nc, scanorient ) = vertical * 10000;


                                        // mixing: half moon to full moon, then azimut
//  ordering ( nc, scanorient ) = vertical * azimut * 1000;             // better
//  ordering ( nc, scanorient ) = ( vertical + azimut ) * 10000;        // good
    ordering ( nc, scanorient ) = ( vertical + azimut * 10 ) * 10000;   // gooder


/*                                      // old way, through dipole fitting, then looking at the vectorial components of the said dipole
    maps[ nc ].FitDipole ( xyzdoc, 0 /*&synthmaps[ nc ]* /, &dipoles[ nc ] );
//  maps[ nc ].FitDipole ( xyzdoc, &maps[ nc ], &dipoles[ nc ] );   // !will overwrite the maps with the synthetic ones - for debugging!

                                        // look only at half the directions
    azimut          = RadiansToDegrees ( ArcTangent ( dipoles[ nc ].Y, dipoles[ nc ].X ) );
                                        // only superior half circle
    azimut          = NormalizeAngle ( azimut, 180 );
                                        // this only to have symmetric left-right maps have the same angle
    if ( azimut > 90 )  azimut  = 180 - azimut;


                                        // look only at half the directions
    vertical        = RadiansToDegrees ( acos ( dipoles[ nc ].Z ) );
                                        // pointing upward or downward is equivalent here, so we need to symmetrize around 90 degrees
    if ( vertical > 90 )    vertical    = 180 - vertical;
                                        // vertical   : 0=through vertex, 90=perpendicular to vertex
                                        // sort: from horizontal to vertical, then from frontal to ears orientation
    ordering ( nc, scanorient ) = ( ( vertical / 90 ) * 3 + ( 1 - azimut / 90 ) ) * 10000;

//    DBGV5 ( nclusters, nc + 1, dipoles[ nc ].X, dipoles[ nc ].Y, dipoles[ nc ].Z, "ncluster  map  xyz" );
//    DBGV6 ( nclusters, nc + 1, precession, nutation, azimut, vertical, "ncluster map:  prec nut -> azimut vertical" );
*/
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // sort by increasing orientation criterion
ordering.SortRows ( scanorient, Ascending );

                                        // find backward indexes
PostProcOrdering ( ordering );
}


//----------------------------------------------------------------------------
                                        // if spdoc is null, use the index of SPs
                                        // else use anatomical ordering, using spdoc, with the help of optional mridoc
void    TMicroStates::GetAnatomicalOrdering (   int                         nclusters,  
                                                const TMaps&                maps,
                                                const TSolutionPointsDoc*   spdoc,
                                                const TVolumeDoc*           mridoc,
                                                TArray2<int>&               ordering    
                                            )   const
{
PreProcOrdering ( nclusters, ordering );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( spdoc == 0 ) {

    for ( int nc = 0; nc < nclusters; nc++ ) {
                                            // as a fall-back, assume SPs are ordered with some meaning, so using the index of SP
        ordering ( nc, scanorient ) = maps[ nc ].GetMaxPosition ();
        }
    }

else { // ! spdoc

    TPoints             Points          = spdoc ->GetPoints ( DisplaySpace3D );
  //TPointFloat         Center;
    const TBoundingBox<double>* spbound = spdoc->GetBounding ();
    TPointFloat         spnorm ( spbound->GetExtent ( 0 ), spbound->GetExtent ( 1 ), spbound->GetExtent ( 2 ) );
    TMatrix44           StandardOrient;
    int                 maxsp;


    if ( mridoc )                       // trust better the MRI than the SPs
        StandardOrient  = mridoc->GetStandardOrientation ( LocalToRAS );
    else
        StandardOrient  = spdoc ->GetStandardOrientation ( LocalToRAS );

                                        // scale normalize
    Points     /= spnorm;
                                        // reorient
    StandardOrient.Apply ( Points );


    for ( int nc = 0; nc < nclusters; nc++ ) {
                                            // get max SP
        maxsp   = maps[ nc ].GetMaxPosition ();
                                            // sort: from bottom to top, then from posterior to anterior
                                            // Left-Right symmetry needs more tricks...
        ordering ( nc, scanorient ) = ( 10 * Points[ maxsp ].Z + Points[ maxsp ].Y ) * 10000;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // sort by increasing orientation criterion
ordering.SortRows ( scanorient, Ascending );

                                        // find backward indexes
PostProcOrdering ( ordering );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
