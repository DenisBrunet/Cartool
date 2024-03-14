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

#include    "TVector.h"
#include    "TArray2.h"
#include    "TVolume.h"
#include    "TFilters.Base.h"
#include    "TMaps.h"

namespace crtl {

//----------------------------------------------------------------------------
                                        // Non-temporal filter
//----------------------------------------------------------------------------

template <class TypeD>
class   TFilterRanking  : public TFilter<TypeD>
{
public:

                    TFilterRanking ()       {}


//  inline  void    Apply                   ( TypeD*          data, int numpts ) {}                                 // to shut-up compiler
    inline  void    Apply                   ( TVector<TypeD>& map, RankingOptions options );                        // !Normalized result!
    inline  void    Apply                   ( TArray2<TypeD>& data, int numel, int numtf, int tfoffset = 0 );
    inline  void    Apply                   ( TMaps&          maps, RankingOptions options, int nummaps = -1 );
    inline  void    Apply                   ( TVolume<TypeD>& vol, double threshold = 0 );


                    TFilterRanking          ( const TFilterRanking& op  );
    TFilterRanking& operator    =           ( const TFilterRanking& op2 );

};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------

template <class TypeD>
            TFilterRanking<TypeD>::TFilterRanking ( const TFilterRanking& /*op*/ )
{
                                        // not copying temp variables
}


template <class TypeD>
TFilterRanking<TypeD>& TFilterRanking<TypeD>::operator= ( const TFilterRanking& op2 )
{
if ( &op2 == this )
    return  *this;

                                        // not copying temp variables

return  *this;
}


//----------------------------------------------------------------------------
                                        // On a single vector / map (code moved from previous TVector::ToRank)
                                        // !RankingAccountNulls option has to be used only for positive data, in which case caller wish to skip the null values!
                                        // !Result is already normalized to range [epsilon..1], with possible 0's in case of RankingIgnoreNulls!
template <class TypeD>
void    TFilterRanking<TypeD>::Apply ( TVector<TypeD>& v, RankingOptions options )
{
int                 dim             = v.GetDim1 ();

if ( dim == 0 )
    return;

                                        // very special case, we don't rank a fully null map
if ( v.IsNull () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // loading and sorting
TArray2<double>     sortdata ( dim, SortDataNum );  // !type should be big enough to hold all the indexes without overflowing / aliasing!


for ( int i = 0; i < dim; i++ ) {
                                        // copying ALL data + setting indexes
    sortdata ( i, SortDataIndex )   = i;
    sortdata ( i, SortDataValue )   = v[ i ];
    }

                                        // sort by values
sortdata.SortRows ( SortDataValue, Ascending );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // ranking
int                 rank            = 1;
int                 maxrank         = 0;

for ( int i = 0; i < dim; i++ ) {

    if ( IsFlag ( options, RankingIgnoreNulls ) && sortdata ( i, SortDataValue ) == 0 ) {
                                        // reset rank
        v[ (int) sortdata ( i, SortDataIndex ) ]    = (TypeD) 0;

        continue;
        }


    v[ (int) sortdata ( i, SortDataIndex ) ]    = (TypeD) rank;

                                        // keeping track of max rank, as options could allow it to stall
    maxrank     = rank;

                                        // bumping next rank
    if ( IsFlag ( options, RankingCountIdenticals ) // allows for repeated values - indexes will be random, though
      || IsFlag ( options, RankingMergeIdenticals ) // only for strictly different values, otherwise keep it as a plateau value
         && i < dim - 1 
         && sortdata ( i, SortDataValue ) != sortdata ( i + 1, SortDataValue ) )

        rank++;
    }

                                        // normalize to max rank [epsilon..1]
if ( maxrank > 0 )
    v  /= (double) maxrank;
}


//----------------------------------------------------------------------------
                                        // Wrapping calls for a big TArray2 variable
template <class TypeD>
void    TFilterRanking<TypeD>::Apply ( TArray2<TypeD>& data, int numel, int numtf, int tfoffset )
{
if ( data.IsNotAllocated () )
    return;

          // !numel can be < than data Dim1, in case we want to skip pseudo-tracks!
Mined       ( numel, data.GetDim1 () );


OmpParallelBegin

TVector<TypeD>      temp ( numel );

OmpFor

for ( int tf0 = 0, tf = tfoffset; tf0 < numtf; tf0++, tf++ ) {

    for ( int i = 0; i < numel; i++ )
        temp[ i ]       = data ( i, tf );

                                        // Rank data - ignoring null values or not? or ignoring only for positive data?
    Apply ( temp, RankingOptions ( RankingAccountNulls | RankingMergeIdenticals ) );


    for ( int i = 0; i < numel; i++ )
        data ( i, tf )  = temp[ i ];
    }

OmpParallelEnd
}


//----------------------------------------------------------------------------
                                        // Wrapping calls for a TMaps variable
template <>
void    TFilterRanking<TMapAtomType>::Apply ( TMaps& maps, RankingOptions options, int nummaps )
{
if ( maps.IsNotAllocated () )
    return;

if ( maps.CheckNumMaps ( nummaps ) <= 0 )
    return;


OmpParallelFor

for ( int mi = 0; mi < nummaps; mi++ )

    Apply ( maps[ mi ], options );
}



//----------------------------------------------------------------------------
                                        // Wrapping calls for a volume variable
                                        // Threshold could 0, but null values are going to be ignored anyway
                                        // !Handles positive, negative or mixed case data - each positive and negative part is ranked separately!
template <>
void    TFilterRanking<MriType>::Apply ( Volume& vol, double threshold )
{
if ( vol.IsNotAllocated () )
    return;

threshold   = fabs ( threshold );


bool                somepos;
bool                someneg;

vol.HasPositiveNegative ( somepos, someneg );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // loading and sorting
int                 dim             = vol.GetLinearDim ();
TArray2<double>     sortdata ( dim, SortDataNum );  // !type should be big enough to hold all the indexes without overflowing / aliasing!


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do the positive part - if any
                                        // ranking done separately, returning positive normalized values
int                 numdata;


if ( somepos ) {
                                        // fill in the array with only the appropriate data
    numdata     = 0;

    for ( int i = 0; i < dim; i++ )
                                        // strict test, we are going to ignore nulls anyway
        if ( vol[ i ] > threshold ) {
                                        // copying only relevant data + setting indexes
            sortdata ( numdata, SortDataIndex )     = i;
            sortdata ( numdata, SortDataValue )     = vol[ i ];
            numdata++;
            }

        else if ( vol[ i ] > 0 )        // !clearing data in (0..threshold]!

            vol[ i ]    = (MriType) 0;

                                        // sort by values
    sortdata.SortRows ( SortDataValue, Ascending, numdata );

                                        // "plain" ranking - in case of plateau, ranks will be assigned "randomly"
    for ( int ni = 0; ni < numdata; ni++ )

        vol[ (int) sortdata ( ni, SortDataIndex ) ]     = (MriType) ( (double) ( ni + 1 ) / (double) numdata );

/*                                      // In case we want to assign the same rank to plateau - it doesn't behave nicely with MRIs if they have had any sort of quantization in their lifespan
                                        // rank / clear data
    long        rank        = 1;

    for ( int ni = 0; ni < numdata; ni++ ) {

        vol[ (int) sortdata ( ni, SortDataIndex ) ]     = (MriType) rank;
                                            // allow to bump to next rank only for strictly different values, otherwise keep it as a plateau value
        if ( ni < numdata - 1 && sortdata ( ni, SortDataValue ) != sortdata ( ni + 1, SortDataValue ) )
            rank++;
        }

                                        // normalize rank
    if ( rank > 1 )

        for ( int ni = 0; ni < numdata; ni++ )

            vol[ (int) sortdata ( ni, SortDataIndex ) ]   /= rank;
*/
    } // some positive data


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do the negative part - if any
                                        // ranking done separately, returning negative normalized values
if ( someneg ) {
                                        // inverting threshold to avoid testing - threshold
    threshold   = - threshold;
                                        // fill in the array with only the appropriate data
    numdata     = 0;

    for ( int i = 0; i < dim; i++ )
                                        // strict test, we are going to ignore nulls anyway
        if ( vol[ i ] < threshold ) {
                                        // copying only relevant data + setting indexes
            sortdata ( numdata, SortDataIndex )     = i;
            sortdata ( numdata, SortDataValue )     = - vol[ i ];   // !converted to positive data!
            numdata++;
            }

        else if ( vol[ i ] < 0 )        // !clearing data in [-threshold..0)!

            vol[ i ]    = (MriType) 0;

                                        // sort by values (!positive!)
    sortdata.SortRows ( SortDataValue, Ascending, numdata );

                                        // "plain" ranking - in case of plateau, ranks will be assigned "randomly"
    for ( int ni = 0; ni < numdata; ni++ )
                                        // ouputting negative normalized rank
        vol[ (int) sortdata ( ni, SortDataIndex ) ]     = - (MriType) ( (double) ( ni + 1 ) / (double) numdata );

/*                                      // In case we want to assign the same rank to plateau - it doesn't behave nicely with MRIs if they have had any sort of quantization in their lifespan
                                        // rank / clear data
    long        rank        = 1;

    for ( int ni = 0; ni < numdata; ni++ ) {
                                        // !negative rank!
        vol[ (int) sortdata ( ni, SortDataIndex ) ]     = (MriType) - rank;
                                            // allow to bump to next rank only for strictly different values, otherwise keep it as a plateau value
        if ( ni < numdata - 1 && sortdata ( ni, SortDataValue ) != sortdata ( ni + 1, SortDataValue ) )
            rank++;
        }

                                        // normalize rank
    if ( rank > 1 )

        for ( int ni = 0; ni < numdata; ni++ )
                                        // ouputting negative normalized rank
            vol[ (int) sortdata ( ni, SortDataIndex ) ]   /= rank;
*/
    } // some negative data

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
