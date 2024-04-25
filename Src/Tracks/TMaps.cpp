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

#include    <set>

#include    "TMaps.h"

#include    "CartoolTypes.h"            // ZScoreType PolarityType CentroidType CorrelateType
#include    "Math.Resampling.h"
#include    "TFilters.h"
#include    "TArray1.h"
#include    "TArray2.h"
#include    "TVector.h"
#include    "TVector.h"
#include    "Geometry.TPoint.h"
#include    "TList.h"
#include    "TFilters.h"
#include    "TMarkers.h"
#include    "Files.ReadFromHeader.h"
#include    "Files.PreProcessFiles.h"

#include    "TExportTracks.h"
#include    "TElectrodesDoc.h"
#include    "TInverseMatrixDoc.h"
#include    "TLeadField.h"
#include    "TRisDoc.h"

#include    "TMicroStates.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Used solely to speed up and replace Correlation, when data is already centered and normalized
                                        // hence the result is clipped in [-1..1]
double      Project ( const TMap&   map1,   const TMap&     map2,   PolarityType    polarity )
{
                                        // Force to remain within boundaries for safety
double              corr            = Clip ( map1.ScalarProduct ( map2 ), -1.0, 1.0 );

return  polarity == PolarityDirect ? corr : fabs ( corr );
}


//----------------------------------------------------------------------------

double      Project ( const TMaps&  maps1,  const TMaps&    maps2,  PolarityType    polarity )
{
int                 maxnumsamples       = 1000;             // in each cluster
int                 step                = AtLeast ( 1, (int) min ( (int) maps1 / maxnumsamples, (int) maps2 / maxnumsamples ) );
int                 numsamples1         = ( (int) maps1 + step - 1 ) / step;
int                 numsamples2         = ( (int) maps2 + step - 1 ) / step;

                                        // limit the ranges
TEasyStats          stat ( numsamples1 * numsamples2 );

for ( int mapi1 = 0; mapi1 < (int) maps1; mapi1 += step )
for ( int mapi2 = 0; mapi2 < (int) maps2; mapi2 += step )

    stat.Add ( Project ( maps1[ mapi1 ], maps2[ mapi2 ], polarity ), ThreadSafetyIgnore );


                                        // using a median allows to handle all possible distributions (non-Normal) - also more robust
return  stat.Median ( false );
                                        // Mode makes it clearer by not accounting for any skewness, which would bias the Median, and worse for the Mean
//return  stat.MaxModeHistogram ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TMaps::TMaps ()
{
Maps                = 0;
NumMaps             = 0;
Dimension           = 0;
SamplingFrequency   = 0;

#if defined(TMapsOptimized)
ToMapsArray         = 0;
#endif

DeallocateMemory ();
}


        TMaps::TMaps ( int nummaps, int dim )                
{
Maps                = 0;
NumMaps             = 0;
Dimension           = 0;
SamplingFrequency   = 0;

#if defined(TMapsOptimized)
ToMapsArray         = 0;
#endif

Resize ( nummaps, dim );
}


        TMaps::TMaps ( const char* filename, AtomType datatype, ReferenceType reference, TStrings* tracksnames )
{
Maps                = 0;
NumMaps             = 0;
Dimension           = 0;
SamplingFrequency   = 0;

#if defined(TMapsOptimized)
ToMapsArray         = 0;
#endif

ReadFile ( filename, datatype, reference, tracksnames );
}

                                        // Concatenate all TGoMaps into a single, big TMaps
        TMaps::TMaps ( const TGoMaps* gogomaps )
{
Maps                = 0;
NumMaps             = 0;
Dimension           = 0;
SamplingFrequency   = 0;

#if defined(TMapsOptimized)
ToMapsArray         = 0;
#endif

Set ( gogomaps );
}


        TMaps::TMaps ( const TGoF& gof, AtomType datatype, ReferenceType reference, TStrings* tracksnames )
{
Maps                = 0;
NumMaps             = 0;
Dimension           = 0;
SamplingFrequency   = 0;

#if defined(TMapsOptimized)
ToMapsArray         = 0;
#endif

ReadFiles ( gof, datatype, reference, tracksnames );
}


//----------------------------------------------------------------------------
                                        // copy while downsampling, and optionally skipping some tracks
        TMaps::TMaps ( const TMaps &op, int downsampling, const TSelection* ignoretracks )
{
Maps                = 0;
NumMaps             = 0;
Dimension           = 0;
SamplingFrequency   = 0;

#if defined(TMapsOptimized)
ToMapsArray         = 0;
#endif


Maxed ( downsampling, 1 );


int                 nummaps         = Truncate ( op.NumMaps / downsampling );
int                 dimension       = op.Dimension - ( ignoretracks ? ignoretracks->NumSet () : 0 );

                                        // new size, requesting n complete input samples for 1 output sample
Resize ( nummaps, dimension );


SamplingFrequency   = op.SamplingFrequency / downsampling;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 maxnummapsin    = NumMaps * downsampling;

                                        // cumulating maps
if ( dimension == op.Dimension )

    for ( int mi = 0; mi < maxnummapsin; mi++ )

        Maps[ mi / downsampling ]  += op[ mi ];

else                                // skipping some tracks

    for ( int mi = 0; mi < maxnummapsin; mi++ )
    for ( int el0 = 0, el = 0; el < op.Dimension; el++ )

        if ( ignoretracks->IsSelected ( el ) )

            Maps[ mi / downsampling ][ el0++ ] += op[ mi ][ el ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                                        // average
(*this) /= downsampling;
}


//----------------------------------------------------------------------------
        TMaps::~TMaps ()
{
DeallocateMemory ();
}


void    TMaps::DeallocateMemory ()
{
#if defined(TMapsOptimized)

if ( ToMapsArray ) {
    FreeVirtualMemory ( ToMapsArray );
    ToMapsArray     = 0;
    }

if ( Maps ) {
    FreeVirtualMemory ( Maps );
    Maps            = 0;
    }

#else

if ( Maps ) {

    for ( int nc = 0; nc < NumMaps; nc++ )
        Maps[ nc ].DeallocateMemory ();

    delete[]    Maps;
    Maps        = 0;
    }

#endif


NumMaps             = 0;
Dimension           = 0;
//SamplingFrequency   = 0;              // keeping this through Resize
}


//----------------------------------------------------------------------------
bool    TMaps::SomeNullMaps ( int nummaps )   const
{
if ( IsNotAllocated () )
    return  false;

if ( CheckNumMaps ( nummaps ) <= 0 )
    return  false;


for ( int nc = 0; nc < nummaps; nc++ )

    if ( Maps[ nc ].IsNull () )

        return  true;


return  false;
}


//----------------------------------------------------------------------------
void    TMaps::Resize ( int nummaps, int dim )
{

if ( dim <= 0 )                         // this is definitely an error, get out of here
    return;

                                        // nothing changed?
if ( nummaps == NumMaps && dim == Dimension ) {
    Reset ();
    return;
    }


DeallocateMemory ();

                                        // !We set the dimension here, even if the number of maps is 0, so that it is legal to have an empty set of maps, and retrieve its dimension!
Dimension       = dim;

                                        // anything to allocate?
if ( nummaps <= 0 )                     
    return;


NumMaps         = nummaps;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if defined(TMapsOptimized)
                                        // allocate space for the TMap objects themselves
                                        // !There are many things forbiden, like resizing a specific TMap, but this was not the primary intent of a TMaps!
Maps        = (TMap*)         GetVirtualMemory ( NumMaps * sizeof ( TMap ) );

                                        // allocate space for the content of the vectors (we could have allocated with the above block, too)
ToMapsArray = (TMapAtomType*) GetVirtualMemory ( DataMemorySize ( NumMaps ) );


                                        // forcefully overwrite the content of our TMaps
                                        // !Not done here: setting MemoryBlock / MemorySize / InVirtualMemory - Either make TMemory a friend, or adjust the few functions that yould access TMemory (currently the latter)!
for ( int nc = 0; nc < NumMaps; nc++ ) {
    Maps[ nc ].Dim1         = Dimension;
    Maps[ nc ].Index1.Reset ();
    Maps[ nc ].Array        = ToMapsArray + nc * Dimension; // get us a slice of the allocated cake
    }

#else
                                        // Boils down to TArray1 new[] ("this" objects are consecutive in memory, but each object will allocate its storage for data by itself, meaning in non-consecutive parts)
Maps            = new TMap [ NumMaps ];


for ( int nc = 0; nc < NumMaps; nc++ )
                                        // regular way, each map gets its own allocation: lots of calls for small amount of memory, each will land in the C Heap if nothing is done against that
    Maps[ nc ].Resize ( Dimension );

#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Reset ();
}


//----------------------------------------------------------------------------
                                        // copy constructor
        TMaps::TMaps ( const TMaps &op )
{
Maps                = 0;
NumMaps             = 0;
Dimension           = 0;
SamplingFrequency   = 0;

#if defined(TMapsOptimized)
ToMapsArray         = 0;
#endif

                                        // allocate
Resize ( op.NumMaps, op.Dimension );

                                        // (already done in Resize)
NumMaps             = op.NumMaps;
Dimension           = op.Dimension;
SamplingFrequency   = op.SamplingFrequency;


#if defined(TMapsOptimized)
                                        // we allocated a contiguous block of memory, take advantage of that
CopyVirtualMemory ( ToMapsArray, op.ToMapsArray, DataMemorySize ( NumMaps ) );

#else

for ( int mi = 0; mi < NumMaps ; mi++ )
    Maps[ mi ]  = op.Maps[ mi ];

#endif
}

                                        // assignation operator
TMaps&    TMaps::operator= ( const TMaps &op2 )
{
if ( &op2 == this )
    return  *this;

                                        // allocate
if ( ! ( NumMaps == op2.NumMaps && Dimension == op2.Dimension ) )
    Resize ( op2.NumMaps, op2.Dimension );

                                        // (already done in Resize)
NumMaps             = op2.NumMaps;
Dimension           = op2.Dimension;
SamplingFrequency   = op2.SamplingFrequency;


#if defined(TMapsOptimized)
                                        // we allocated a contiguous block of memory, take advantage of that
CopyVirtualMemory ( ToMapsArray, op2.ToMapsArray, DataMemorySize ( NumMaps ) );

#else

for ( int mi = 0; mi < NumMaps; mi++ )
    Maps[ mi ]  = op2.Maps[ mi ];

#endif


return  *this;
}


//----------------------------------------------------------------------------
TMaps&    TMaps::operator+= ( const TMaps& op2 )
{
                                        // Checked it was not already part of a parallel loop
OmpParallelFor

for ( int nc = 0; nc < min ( NumMaps, op2.NumMaps ); nc++ )
    Maps[ nc ]     += op2.Maps[ nc ];

return  *this;
}


TMaps&    TMaps::operator*= ( const TMaps& op2 )
{
                                        // Checked it was not already part of a parallel loop
OmpParallelFor

for ( int nc = 0; nc < min ( NumMaps, op2.NumMaps ); nc++ )
    Maps[ nc ]     *= op2.Maps[ nc ];

return  *this;
}


TMaps&    TMaps::operator*= ( double op2 )
{
if ( op2 == 1.0 )
    return *this;

                                        // Checked it was not already part of a parallel loop
OmpParallelFor

for ( int nc = 0; nc < NumMaps; nc++ )
    Maps[ nc ]     *= op2;

return  *this;
}


TMaps&    TMaps::operator/= ( double op2 )
{
if ( op2 == 0.0 || op2 == 1.0 )
    return *this;

                                        // Checked it was not already part of a parallel loop
OmpParallelFor

for ( int nc = 0; nc < NumMaps; nc++ )
    Maps[ nc ]     /= op2;

return  *this;
}


TMaps&    TMaps::operator+= ( double op2 )
{
if ( op2 == 0.0 )
    return *this;

                                        // Checked it was not already part of a parallel loop
OmpParallelFor

for ( int nc = 0; nc < NumMaps; nc++ )
    Maps[ nc ]     += op2;

return  *this;
}


TMaps&    TMaps::operator-= ( double op2 )
{
if ( op2 == 0.0 )
    return *this;

                                        // Checked it was not already part of a parallel loop
OmpParallelFor

for ( int nc = 0; nc < NumMaps; nc++ )
    Maps[ nc ]     -= op2;

return  *this;
}


TMaps&    TMaps::operator= ( double op2 )
{
OmpParallelFor

for ( int nc = 0; nc < NumMaps; nc++ )
    Maps[ nc ]      = op2;

return  *this;
}


//----------------------------------------------------------------------------
                                        // !this will allocate space if needed!
void    TMaps::CopyFrom ( TMaps& othermaps, int nummaps )
{
                                        // !0 nummaps is allowed!
othermaps.CheckNumMaps ( nummaps );

                                        // be extra nice: if receiving object is not big enough, (re)allocate it
if ( nummaps > NumMaps )
    Resize ( nummaps, othermaps.Dimension );


SamplingFrequency   = othermaps.SamplingFrequency;


#if defined(TMapsOptimized)
                                        // we allocated a contiguous block of memory, take advantage of that
CopyVirtualMemory ( ToMapsArray, othermaps.ToMapsArray, DataMemorySize ( nummaps ) );

#else

//OmpParallelFor

for ( int mi = 0; mi < nummaps; mi++ )

    Maps[ mi ]  = othermaps[ mi ];

#endif
}
        
                                        // can optionally restrict the range
void    TMaps::Reset ( int nummaps )
{
                                        // !0 nummaps is allowed!
CheckNumMaps ( nummaps );


#if defined(TMapsOptimized)
                                        // we allocated a contiguous block of memory, take advantage of that
ClearVirtualMemory ( ToMapsArray, DataMemorySize ( nummaps ) );

#else

//OmpParallelFor

for ( int mi = 0; mi < nummaps; mi++ )

    Maps[ mi ].ResetMemory ();

#endif
}

                                        // Concatenate all TGoMaps into a single, big TMaps
void    TMaps::Set ( const TGoMaps* gogomaps )
{
DeallocateMemory ();


if ( gogomaps == 0 )
    return;


const TGoMaps&      ggm ( *gogomaps );


Resize ( ggm.GetTotalNumMaps (), ggm.GetDimension () );


SamplingFrequency   = ggm.GetSamplingFrequency ();


int             mabs            = 0;

for ( int gofi = 0; gofi < ggm.NumGroups (); gofi++ )
for ( int mrel  = 0; mrel  < ggm[ gofi ].GetNumMaps (); mrel++, mabs++  )
for ( int el    = 0; el    < Dimension;                 el++    )

    Maps[ mabs ][ el ]  = ggm[ gofi ][ mrel ][ el ];
}


//----------------------------------------------------------------------------
void    TMaps::GetIndexes ( TArray1<TMap *> &indexes, const TSelection* tfok )    const
{
indexes.Resize ( tfok ? tfok->NumSet () : NumMaps );


if ( tfok ) {

    for ( int nc = 0, i0 = 0; nc < NumMaps; nc++ )

//      if ( tfok->IsSelected ( nc ) )
        if ( (*tfok)[ nc ] )    // faster

            indexes[ i0++ ] = &Maps[ nc ];
    }

else {
    
    OmpParallelFor

    for ( int nc = 0; nc < NumMaps; nc++ )

        indexes[ nc ]   = &Maps[ nc ];
    }
}


TArray1<TMap *> TMaps::GetIndexes ()  const
{
TArray1<TMap *>     allmaps; 

GetIndexes ( allmaps ); 

return  allmaps; 
}


//----------------------------------------------------------------------------
void    TMaps::ComputeNorm ( TArray1<double> &norm, ReferenceType reference )
{
norm.Resize ( NumMaps );

OmpParallelFor
                                        // for all time frame
for ( int nc = 0; nc < NumMaps; nc++ )

    norm[ nc ]   = Maps[ nc ].Norm ( reference == ReferenceAverage );
}

                                        // ?Shouldn't reference be always ReferenceAverage?
void    TMaps::ComputeGFP ( TArray1<double> &gfp, ReferenceType reference, AtomType datatype )  const
{
gfp.Resize ( NumMaps );

OmpParallelFor
                                        // for all time frame
for ( int nc = 0; nc < NumMaps; nc++ )

    gfp[ nc ]   = Maps[ nc ].GlobalFieldPower ( reference == ReferenceAverage, IsVector ( datatype ) );
}

                                        // Assume a whole continuity in the data (which might not always be true, in case of concatenated files)
void    TMaps::ComputeDissimilarity ( TArray1<double> &dis, PolarityType polarity, ReferenceType reference )    const
{
                                        // also resets memory
dis.Resize ( NumMaps );

//OmpParallelFor
//
//                                        // historic method with a shift to the right, starting from time frame 1
//for ( int nc = 1; nc < NumMaps; nc++ )
//
//    dis[ nc ]   = Maps[ nc ].Dissimilarity ( Maps[ nc - 1 ], reference == ReferenceAverage, IsVector ( datatype ) );


OmpParallelFor
                                        // correct method, centered
for ( int nc = 1; nc < NumMaps - 1; nc++ )

    dis[ nc ]   = (  Maps[ nc ].Dissimilarity ( Maps[ nc - 1 ], polarity, reference == ReferenceAverage )
                   + Maps[ nc ].Dissimilarity ( Maps[ nc + 1 ], polarity, reference == ReferenceAverage ) ) / 2;
}


//----------------------------------------------------------------------------
                                        // Vectorial case will just merge the 3 dimensions altogether, as sqrt (N1^2 + N2^2 + N3^2 + ...) = sqrt (X1^2+Y1^2+Z1^2 + X2^2+Y2^2+Z2^2 + X3^2+Y3^2+Z3^2 + ...)
void    TMaps::Normalize ( AtomType /*datatype*/, int nummaps, bool centeraverage )
{
if ( CheckNumMaps ( nummaps ) <= 0 )
    return;


OmpParallelFor

for ( int mi = 0; mi < nummaps; mi++ )

    Maps[ mi ].Normalize ( centeraverage );
}
                                        // !Only for vectorial data! Maybe merge with Normalize above with a parameter specifying which way?
                                        // Normalizing each solution point 3D vector individually
void    TMaps::NormalizeSolutionPoints ( AtomType datatype, int nummaps )
{
if ( CheckNumMaps ( nummaps ) <= 0 )
    return;

if ( ! IsVector ( datatype ) )
    return;


//OmpParallelFor

for ( int mi = 0; mi < nummaps; mi++ )
for ( int e3 = 0; e3 < Dimension; e3 += 3 ) {

    double          vn          = NormVector3 ( &Maps[ mi ][ e3 ] );

    if ( vn ) {
                                // normalizing 3D vector
        Maps[ mi ][ e3     ]   /= vn;
        Maps[ mi ][ e3 + 1 ]   /= vn;
        Maps[ mi ][ e3 + 2 ]   /= vn;
        }
    } // for mi, e3
}


//----------------------------------------------------------------------------
                                        // Unique function, called by both TMaps and TGoMaps
double  ComputeGfpNormalizationFactor ( TArray1<TMap *>& allmaps, AtomType datatype )
{
if ( allmaps.IsNotAllocated () )
    return  1;


TEasyStats          stat ( (int) allmaps );
double              gfp;
double              rescalefactor;

                                        // sum all GFPs within specified time range
for ( int nc = 0; nc < (int) allmaps; nc++ ) {

    gfp     = allmaps[ nc ]->GlobalFieldPower ( true, IsVector ( datatype ) );
                                        
    if ( gfp > 0 )
        stat.Add ( gfp, ThreadSafetyIgnore );
//      stat.Add ( logl ( gfp ), ThreadSafetyIgnore );// GFP is right skewed, so log it - also, ignore null GFPs...
    }


if ( stat.IsEmpty () )
    return  1;

                                        // aim at the mode, with the original data scaling
rescalefactor   = NonNull ( min ( stat.MaxModeHistogram (), stat.Median () ) );

                                        // invert the log now
//rescalefactor   = NonNull ( exp ( stat.Mean () ) );
//rescalefactor   = NonNull ( expl ( stat.Median ( false ) ) );


//TFileName           _file;
//StringCopy      ( _file, "E:\\Data\\GfpNormalization.Histo.sef" );
//stat.WriteFileHistogram ( _file );
//stat.Show ( "Stats GfpNormalization" );
//DBGV ( rescalefactor, "ComputeGfpNormalizationFactor" );

                                        // Centering in the log space might not be enough, as the distributions show different spreading
                                        // that means normalizing by the spread (dividing), hence doing a Gamma correction on all electrodes (data^1/spread)
return  rescalefactor;
}


//----------------------------------------------------------------------------
double  TMaps::ComputeGfpNormalization ( AtomType datatype, double& gfpnorm )
{
gfpnorm     = ComputeGfpNormalizationFactor ( GetIndexes (), datatype );

return  gfpnorm;
}


double  TGoMaps::ComputeGfpNormalization ( AtomType datatype, double& gfpnorm )
{
gfpnorm     = ComputeGfpNormalizationFactor ( GetIndexes (), datatype );

return  gfpnorm;
}


void    TMaps::ApplyGfpNormalization ( double gfpnorm )
{
(*this)    /= gfpnorm;
}


void    TGoMaps::ApplyGfpNormalization ( double gfpnorm )
{
(*this)    /= gfpnorm;
}


//----------------------------------------------------------------------------
                                        // Make a kind of winner-takes-all across a set of templates:
                                        // a given solution point is given only to 1 template, the one with the max value
                                        // all the others templates get this SP reset
                                        // !This is OK only on positive maps!
                                        // Results are not normalized
void    TMaps::Orthogonalize ( int nummaps )
{
                                        // there needs to be at least 2 maps to make things orthogonal
if ( CheckNumMaps ( nummaps ) < 2 )
    return;


int                 maxi;
TMapAtomType        maxv;

                                        // we need normalized maps to make all maps comparable between each others
Normalize ( AtomTypeScalar, nummaps, false );


for ( int dim = 0; dim < Dimension; dim++ ) {

    maxi    = 0;
    maxv    = Maps[ maxi ][ dim ];

                                        // get index of greatest dimension
    for ( int mi = 0; mi < nummaps; mi++ )

        if ( Maps[ mi ][ dim ] > maxv ) {
            maxv    = Maps[ mi ][ dim ];
            maxi    = mi;
            }

                                        // keep only the max, zero's all the others
    for ( int mi = 0; mi < nummaps; mi++ )

        if ( mi != maxi )
            Maps[ mi ][ dim ]   = 0;
    }
}


//----------------------------------------------------------------------------
                                        // New version works on scalar values AND 3D vectorial values
                                        // For the latter case, the ranking is done on the norms, direction is not modified
void    TMaps::ToRank ( AtomType datatype, RankingOptions options, int nummaps )
{
if ( CheckNumMaps ( nummaps ) <= 0 )
    return;


TFilterRanking<TMapAtomType>    filterrank;

                                        // special case
if ( datatype == AtomTypeVector ) {


    int                 DimensionSP     = Dimension / 3;

    OmpParallelBegin

    TMap                mapn ( DimensionSP );
    TMap                mapr ( DimensionSP );

    OmpFor

    for ( int mi = 0; mi < nummaps; mi++ ) {

        Maps[ mi ].GetScalarMap ( mapn, true );


        mapr    = mapn;
                                        // ranking the norms
        filterrank.Apply ( mapr, options );

                                        // now we can rescale vectorially
        for ( int dim = 0, dim3 = 0; dim < DimensionSP; dim++, dim3+=3 )
                                        // nothing to rescale if norm is null
            if ( mapn[ dim ] ) {
                                        // rescaling factor norm to rank
                double      r       = mapr[ dim ] / mapn[ dim ];
                                        // applied to all 3 components
                Maps[ mi ][ dim3     ] *= r;
                Maps[ mi ][ dim3 + 1 ] *= r;
                Maps[ mi ][ dim3 + 2 ] *= r;
                }
        }

    OmpParallelEnd
    } // vectors

else                                    // regular case
                                        // this one is parallelized already
    filterrank.Apply ( *this, options, nummaps );
}


//----------------------------------------------------------------------------
                                      // General case, using a TFilterThreshold object
void    TMaps::Thresholding ( double threshold, AtomType datatype, int nummaps )
{
if ( CheckNumMaps ( nummaps ) <= 0 )
    return;


TFilterThreshold<TMapAtomType>  filterthreshold ( FilterClipBelow, threshold );

                                        // special case
if ( datatype == AtomTypeVector ) {

    int                 DimensionSP     = Dimension / 3;

    OmpParallelBegin

    TMap                mapn ( DimensionSP );

    OmpFor

    for ( int mi = 0; mi < nummaps; mi++ ) {

        Maps[ mi ].GetScalarMap ( mapn, true );

                                        // thresholding the norms
        filterthreshold.Apply ( mapn );

                                        // rescan vectors
        for ( int dim = 0, dim3 = 0; dim < DimensionSP; dim++, dim3+=3 )
                                        // if new norm is 0..
            if ( mapn[ dim ] == 0 ) {
                                        // ..then reset all 3 components
                Maps[ mi ][ dim3     ]  = 0;
                Maps[ mi ][ dim3 + 1 ]  = 0;
                Maps[ mi ][ dim3 + 2 ]  = 0;
                }
        }

    OmpParallelEnd
    } // vectors

else                                    // regular case
                                        // this one is parallelized already
    filterthreshold.Apply ( *this, nummaps );
}


//----------------------------------------------------------------------------
void    TMaps::OrthogonalizeRanks ( RankingOptions options, int nummaps )
{
                                        // there needs to be at least 2 maps to make things orthogonal
if ( CheckNumMaps ( nummaps ) < 2 )
    return;

                                        // copy & rank the maps
TMaps             mapsrank ( *this );

mapsrank.ToRank         ( AtomTypeScalar, options, nummaps );

mapsrank.Orthogonalize  ( nummaps );

                                        // use remaining ranks as a mask to the original maps
for ( int mi  = 0; mi  < nummaps;   mi++  )
for ( int dim = 0; dim < Dimension; dim++ )

    if ( mapsrank[ mi ][ dim ] == 0 )
        Maps[ mi ][ dim ]   = 0;


//                                        // convert to ranks, and ranks become the new template
//ToRank          ( nummaps, options );
//                                        // regular orthogonalize
//Orthogonalize   ( nummaps );
}


//----------------------------------------------------------------------------
                                        // Will work on all cases
void    TMaps::AverageReference ( AtomType datatype )
{
if ( IsVector ( datatype ) ) {
                                        // do a separate mean for each X,Y,Z component
    TGoEasyStats        ref ( 3 );

    OmpParallelFor
                                        // simple parallelization on each component
    for ( int compi = 0; compi < 3; compi++ ) {

        for ( int nc = 0; nc < NumMaps; nc++ ) {

            ref ( compi ).Reset ();

                                            // compute reference per dimension
            for ( int e3 = compi; e3 < Dimension; e3 += 3 )

                ref ( compi ).Add ( Maps[ nc ][ e3 ], ThreadSafetyIgnore );


            double      avgrefxyz       = ref ( compi ).Mean ();

                                            // each component is average reference, and as a consequence, all components together too
            for ( int e3 = compi; e3 < Dimension; e3 += 3 )

                Maps[ nc ][ e3 ]   -= avgrefxyz;

            } // for maps
        } // for xyz
    }

else {
    OmpParallelFor

    for ( int nc = 0; nc < NumMaps; nc++ ) 

        Maps[ nc ].AverageReference ();

    }

}


//----------------------------------------------------------------------------
                                        // works with all types of data
void    TMaps::TimeCentering ( bool robust )
{
TGoEasyStats        ref ( Dimension, robust ? NumMaps : 0 );
double              center;

                                        // work on each track / vectorial component independently
OmpParallelFor

for ( int e = 0; e < Dimension; e++ ) {

    for ( int nc = 0; nc < NumMaps; nc++ )

        ref[ e ].Add ( Maps[ nc ][ e ], ThreadSafetyIgnore );


    center      = robust ? ref[ e ].Median () : ref[ e ].Mean ();
        

    for ( int nc = 0; nc < NumMaps; nc++ )

        Maps[ nc ][ e ]    -= center;

    } // for NumRows

}


//----------------------------------------------------------------------------
void    TMaps::ThresholdAbove ( TMapAtomType t )
{
OmpParallelFor

for ( int nc = 0; nc < NumMaps; nc++ )

    Maps[ nc ].ThresholdAbove ( t );
}


//----------------------------------------------------------------------------
void    TMaps::Mean ( int frommap, int tomap, TMap& avgmap )  const
{
avgmap.ResetMemory ();

if ( ! IsAllocated () )
    return;


avgmap.Resize ( Dimension );


crtl::Clipped ( frommap, tomap, 0, NumMaps - 1 );

if ( frommap == tomap ) {
    avgmap  = Maps[ frommap ];
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 nummaps         = tomap - frommap + 1;

for ( int nc = frommap; nc <= tomap; nc++ )

    avgmap     += Maps[ nc ];

avgmap     /= nummaps;
}


//----------------------------------------------------------------------------
void    TMaps::Median ( int frommap, int tomap, TMap& avgmap )    const
{
avgmap.ResetMemory ();

if ( ! IsAllocated () )
    return;


avgmap.Resize ( Dimension );


crtl::Clipped ( frommap, tomap, 0, NumMaps - 1 );

                                        // nothing to do?
if ( frommap == tomap ) {
    avgmap  = Maps[ frommap ];
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 nummaps         = tomap - frommap + 1;
TEasyStats          stats   ( nummaps );


for ( int dim = 0; dim < Dimension; dim++ ) {

    stats.Reset ();

    for ( int nc = frommap; nc <= tomap; nc++ )

        stats.Add ( Maps[ nc ][ dim ], ThreadSafetyIgnore );

    avgmap[ dim ]   = stats.Median ();
    }

}


//----------------------------------------------------------------------------
void    TMaps::SD ( int frommap, int tomap, TMap& sdmap )     const
{
sdmap.ResetMemory ();

if ( ! IsAllocated () )
    return;


sdmap.Resize ( Dimension );


crtl::Clipped ( frommap, tomap, 0, NumMaps - 1 );

if ( frommap == tomap ) {
    sdmap  = Maps[ frommap ];
    sdmap.ResetMemory ();
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TEasyStats          stats;


for ( int dim = 0; dim < Dimension; dim++ ) {

    stats.Reset ();

    for ( int nc = frommap; nc <= tomap; nc++ )

        stats.Add ( Maps[ nc ][ dim ], ThreadSafetyIgnore );

    sdmap[ dim ]   = stats.SD ();
    }

}


//----------------------------------------------------------------------------
void    TMaps::GetNullElectrodes ( TSelection& sel )  const
{
sel.Reset ();

if ( IsNotAllocated () )
    return;


sel     = TSelection ( Dimension, OrderSorted );

sel.Set ();

                                        // a few maps are just enough(?)
int                 step            = 1; // crtl::AtLeast ( 1, NumMaps / 10 );


for ( int dim = 0; dim < Dimension; dim++ )

    for ( int nc = 0;  nc  < NumMaps;   nc += step ) 

        if ( Maps[ nc ][ dim ] != 0.0 )  {

            sel.Reset ( dim );
            break;
            }

}


//----------------------------------------------------------------------------
                                        // Centralized re-referencing, OK on Vectorial
void    TMaps::SetReference ( ReferenceType ref, AtomType datatype /*, TSelection *refsel*/ )
{
if ( ref == ReferenceNone || ref == ReferenceUsingCurrent )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if      ( ref == ReferenceAverage     )   AverageReference     ( datatype );
                                        // !TODO!
else if ( ref == ReferenceSingleTrack 
       || ref == ReferenceMultipleTracks ) {
    ShowMessage ( "Single / Multi tracks referencing not yet implemented!", "TMaps::SetReference", ShowMessageWarning ); 
    exit ( 1 ); 
    }

else if ( ref == UnknownReference     ) {
    ShowMessage ( "Unkown reference!", "TMaps::SetReference", ShowMessageWarning ); 
    exit ( 1 ); 
    }

}


void    TGoMaps::SetReference ( ReferenceType ref, AtomType datatype /*, TSelection *refsel*/ )
{
for ( int gofi = 0; gofi < (int) Group; gofi++ )

    Group[ gofi ]->SetReference ( ref, datatype );
}


//----------------------------------------------------------------------------
                                        // Takes 2 groups of maps, then call the right correlation function
                                        // Results are in (this)
void    TMaps::Correlate    (   TMaps&          maps1,      TMaps&          maps2,      CorrelateType   how,
                                PolarityType    polarity,   ReferenceType   reference,  
                                int             numrand,    TMaps*          pvalues,
                                char*           infix       
                            )

{
if ( maps1.IsNotAllocated () 
  || maps2.IsNotAllocated () 
  || maps1.GetDimension () != maps2.GetDimension () ) {

    DeallocateMemory ();
    ClearString ( infix );
    return;
    }

                                        // data is in maps1, and we want to keep the same time dimension
Resize ( maps1.GetNumMaps (), maps2.GetNumMaps () );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do we have to guess which correlation to use?
if ( how == CorrelateTypeAuto ) {
    TEasyStats          stat;
                                        // test 1st set of maps
    stat.Set ( maps1, false );

    bool                ismaps1circular     = stat.IsAngular ();
                                        // test 2d set of maps
    stat.Set ( maps2, false );

    bool                ismaps2circular     = stat.IsAngular ();

                                        // deduce the correlation type - opting for parametric versions for the moment
    how     =   ismaps1circular &&   ismaps2circular ? CorrelateTypeCircularCircular
            : ! ismaps1circular &&   ismaps2circular ? CorrelateTypeLinearCircular
            :   ismaps1circular && ! ismaps2circular ? CorrelateTypeCircularLinear
                                                     : CorrelateTypeLinearLinear;
    }


//if ( how == CorrelateTypeLinearLinear   )   how = CorrelateTypeLinearLinearRobust;
//if ( how == CorrelateTypeLinearCircular )   how = CorrelateTypeLinearCircularRobust;
//if ( how == CorrelateTypeCircularLinear )   how = CorrelateTypeCircularLinearRobust;
//DBGM ( CorrelateTypeNames[ how ], "TMaps::Correlate New type" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! IsInsideLimits ( how, CorrelateTypeLinearLinear, CorrelateTypeCircularCircular ) ) {
    Reset (); 
    ShowMessage ( "Unknown correlation type!", "Correlation", ShowMessageWarning ); 
    return; 
    }


StringCopy ( infix, CorrelateTypeNames[ how ] );


if ( how == CorrelateTypeCircularCircular ) {
    Reset (); 
    ShowMessage ( "Circular-Circular correlation is not yet implemented!", "Correlation", ShowMessageWarning ); 
    return; 
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // temp variables - allocated once by the called functions, then reused on each call
TVector<float>      map1;
TVector<float>      map2;
double              corr;


for ( int nc1 = 0; nc1 < maps1.GetNumMaps (); nc1++ )
for ( int nc2 = 0; nc2 < maps2.GetNumMaps (); nc2++ ) {

    corr    = how == CorrelateTypeLinearLinear          ? maps1[ nc1 ].Correlation                      ( maps2[ nc2 ], polarity, reference == ReferenceAverage               )
            : how == CorrelateTypeLinearLinearRobust    ? maps1[ nc1 ].CorrelationSpearman              ( maps2[ nc2 ], polarity, reference == ReferenceAverage               )
            : how == CorrelateTypeLinearCircular        ? maps1[ nc1 ].CorrelationLinearCircular        ( maps2[ nc2 ],           reference == ReferenceAverage, map1, map2   )
            : how == CorrelateTypeLinearCircularRobust  ? maps1[ nc1 ].CorrelationLinearCircularRobust  ( maps2[ nc2 ]                                                  )
            : how == CorrelateTypeCircularLinear        ? maps2[ nc2 ].CorrelationLinearCircular        ( maps1[ nc1 ],           reference == ReferenceAverage, map1, map2   )
            : how == CorrelateTypeCircularLinearRobust  ? maps2[ nc2 ].CorrelationLinearCircularRobust  ( maps1[ nc1 ]                                                  )
            : how == CorrelateTypeCircularCircular      ? 0
                                                        : 0;

    Maps[ nc1 ][ nc2 ] = corr; // PearsonToFisher ( corr )
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optionally doing some randomization
if ( ! ( numrand > 0 && pvalues != 0 ) )
    return;


pvalues->Resize ( NumMaps, Dimension );

                                        // 1000 for p=5%, 5000 for p=1%
int                 dim1            = maps1.GetDimension ();
TMap                shuffle1    ( dim1 );
TVector<int>        count       ( maps2.GetNumMaps () );
TRandUniform        randunif;
int                 offset;
double              p;
double              maxp            = 1.0; // 0.01;


for ( int nc1 = 0; nc1 < maps1.GetNumMaps (); nc1++ ) {

    count.ResetMemory ();


    for ( int ri = 0; ri < numrand; ri++ ) {

        UpdateApplication;

                                        // shuffling only maps1 - also, make sure we do have an offset
        do  offset  = randunif ( (UINT) dim1 );  while ( offset == 0 );

                                        // copy with offset
        shuffle1.Insert ( maps1[ nc1 ], offset );

                                        // then run all maps2 against the current shuffled maps1
        for ( int nc2 = 0; nc2 < maps2.GetNumMaps (); nc2++ ) {

            corr    = how == CorrelateTypeLinearLinear          ? shuffle1    .Correlation                      ( maps2[ nc2 ], polarity, reference == ReferenceAverage               )
                    : how == CorrelateTypeLinearLinearRobust    ? shuffle1    .CorrelationSpearman              ( maps2[ nc2 ], polarity, reference == ReferenceAverage               )
                    : how == CorrelateTypeLinearCircular        ? shuffle1    .CorrelationLinearCircular        ( maps2[ nc2 ],           reference == ReferenceAverage, map1, map2   )
                    : how == CorrelateTypeLinearCircularRobust  ? shuffle1    .CorrelationLinearCircularRobust  ( maps2[ nc2 ]                                                  )
                    : how == CorrelateTypeCircularLinear        ? maps2[ nc2 ].CorrelationLinearCircular        ( shuffle1,               reference == ReferenceAverage, map1, map2   )
                    : how == CorrelateTypeCircularLinearRobust  ? maps2[ nc2 ].CorrelationLinearCircularRobust  ( shuffle1                                                      )
                    : how == CorrelateTypeCircularCircular      ? 0
                                                                : 0;

                                        // ignore polarity -> 2-tailed distribution test
            if ( polarity == PolarityEvaluate )
                                        // not necessary, this should have been done in any of the Correlation function above - but still
                corr    = fabs ( corr );


            if ( corr >= Maps[ nc1 ][ nc2 ] )

                count ( nc2 )++;

            } // for maps2

        } // for ri

                                        // now replace correlation with (1-p) value estimated from the randomization
    for ( int nc2 = 0; nc2 < maps2.GetNumMaps (); nc2++ ) {

        p                       = (double) count ( nc2 ) / numrand;

        (*pvalues) ( nc1, nc2 ) = p <= maxp ? Clip ( 1.0 - p, 0.0, 1.0 ) : 0;
        }

    } // for maps1

}


//----------------------------------------------------------------------------
                                        // min across all maps
TMapAtomType    TMaps::GetMinValue () const
{
if ( IsNotAllocated () )
    return  0;


TMapAtomType    minvalue        = Maps[ 0 ].GetMinValue ();

for ( int nc = 1; nc < NumMaps; nc++ )

    crtl::Mined ( minvalue, Maps[ nc ].GetMinValue () );


return  minvalue;
}


TMapAtomType    TMaps::GetMaxValue () const
{
if ( IsNotAllocated () )
    return  0;


TMapAtomType    maxvalue        = Maps[ 0 ].GetMaxValue ();

for ( int nc = 1; nc < NumMaps; nc++ )

    crtl::Maxed ( maxvalue, Maps[ nc ].GetMaxValue () );


return  maxvalue;
}


TMapAtomType    TMaps::GetAbsMaxValue () const
{
if ( IsNotAllocated () )
    return  0;


TMapAtomType    maxvalue        = Maps[ 0 ].GetAbsMaxValue ();

for ( int nc = 1; nc < NumMaps; nc++ )

    crtl::Maxed ( maxvalue, Maps[ nc ].GetAbsMaxValue () );


return  maxvalue;
}


//----------------------------------------------------------------------------
void    TMaps::Random   ( double    minv,   double      maxv )
{
if ( IsNotAllocated () )
    return;


TRandUniform        randunif;

for ( int nc = 0; nc < NumMaps; nc++ )

    Maps[ nc ].Random ( minv, maxv, &randunif );
}


//----------------------------------------------------------------------------
void    TMaps::Add ( const TMap& map )
{
                                        // save old maps
TMaps               oldmaps ( *this );
int                 oldnummaps      = NumMaps;

                                        // allocate new size
Resize ( oldnummaps + 1, map.GetDim () );


#if defined(TMapsOptimized)

if ( oldnummaps > 0 )
                                        // we allocated a contiguous block of memory, take advantage of that
    CopyVirtualMemory ( ToMapsArray, oldmaps.ToMapsArray, DataMemorySize ( oldnummaps ) );


int                 offsetinbytes   = oldnummaps > 0 ? DataMemorySize ( oldnummaps ) : 0;
                                        // !attention to pointer arithmetic!
CopyVirtualMemory ( ToMapsArray + offsetinbytes / sizeof ( *ToMapsArray ), map.GetArray (), DataMemorySize ( 1 ) );

#else
                                        // then add new map
if ( oldnummaps > 0 ) {
                                        // copy back old maps
    for ( int mi = 0; mi < oldnummaps; mi++ )
        Maps[ mi ]  = oldmaps[ mi ];
    }

                                        // then add new map
Maps[ oldnummaps ]  = map;

#endif
}


//----------------------------------------------------------------------------
                                        // Compute the labeling, with optional limit in correlation
                                        // Called from the templates side
void    TMaps::CentroidsToLabeling  (   const TMaps&        data,
                                        long                tfmin,      long            tfmax,
                                        int                 nclusters,
                                        const TSelection*   mapsel,     TLabeling&      labels,
                                        PolarityType        polarity,   double          limitcorr
                                    )   const
{
//if ( IsNotAllocated () )
//    return;
                                        // pointer to specialized function, according to polarity
bool    (*keepbestlabeling) ( const TMap&, const TMap&, double&, double )   = polarity == PolarityEvaluate ? &KeepBestLabelingEvaluate : &KeepBestLabelingDirect;

                                        // reset only these TFs (important in case of partial labeling)
labels.Reset ( tfmin, tfmax );


if ( mapsel == 0 )

    OmpParallelFor
                                        // time can be restricted to an epoch (limits are not tested here)
    for ( long tf = tfmin; tf <= tfmax; tf++ ) {

        UpdateApplication;

        double          maxcorr     = Lowest ( maxcorr );
                                        // most of the time (segmentation) we use all maps
        for ( int nc = 0; nc < nclusters; nc++ )

            if ( (*keepbestlabeling) (   Maps[ nc ], data[ tf ],  maxcorr, limitcorr ) )

                labels.SetLabel ( tf, nc );

        } // for tf

else

    OmpParallelFor
                                        // time can be restricted to an epoch (limits are not tested here)
    for ( long tf = tfmin; tf <= tfmax; tf++ ) {

        UpdateApplication;

        double          maxcorr     = Lowest ( maxcorr );
                                        // maps can be restricted to a given subset (fitting)
//      for ( TIteratorSelectedForward nci ( *mapsel ); (bool) nci; ++nci ) // with thread-safe iterator
        for ( int nc = 0; nc < nclusters; nc++ )

            if ( (*mapsel)[ nc ] )
                                        
                if ( (*keepbestlabeling) (   Maps[ nc ], data[ tf ],  maxcorr, limitcorr ) )

                    labels.SetLabel ( tf, nc );

        } // for tf


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Update all polarity flags
labels.UpdatePolarities ( data, tfmin, tfmax, *this, polarity );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Compute a robust estimate of the cluster's main direction
                                        // Returns a map, either Scalar / Norm or Vectorial, to be used for polarity tests
                                        // Actual norm of reference map is usually of no importance, as only the sign of correlation is usefull
TMap        ComputeReferenceMap         (   
                                        const TArray1<TMap*>&   allmaps,
                                        AtomType            datatype,
                                        TLabeling*          labels,     int             l
                                        )
{
if ( IsVector ( datatype ) )
                                        // Best orientation per SP
    return  ComputeCloudsFolding      ( allmaps,                   ReferenceNumSamplesSP, labels, l );

else
                                        // Global Medoid will do the job pretty well while being way faster than Eigenvector
    return  ComputeMedoidCentroid     ( allmaps, datatype, PolarityEvaluate, ReferenceNumSamplesMap, labels, l );
                                        // Officially the best solution
//  return  ComputeEigenvectorCentroid ( allmaps, refmap, polarity );
}


//----------------------------------------------------------------------------
                                        // Compute the centroid for a given labels value, or whole data set
                                        // !Does NOT normalize anymore, caller has to do that explicitly!
TMap    ComputeCentroid     (   const TArray1<TMap*>&   allmaps,
                                CentroidType            centroid,
                                AtomType                datatype,       PolarityType            polarity,
                                int                     maxsamples,
                                TLabeling*              labels,         int                     l,      const TMap*             ref
                            )
{
TMap                map;

if ( allmaps.IsNotAllocated () )
    return  map;    // null map

int                 NumMaps         = allmaps.GetDim ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Special cases
if      ( NumMaps == 0 )                return  map;            // null map
                                        
else if ( NumMaps == 1 )                return  *allmaps[ 0 ];  // return single existing map
                                        
else { // NumMaps >= 2
                                        // These functions will NOT normalize the resulting map
                                        // Some functions will may need to estimate the best referential maps in case of evaluating polarities
    switch ( centroid ) {

        case    MeanCentroid:           return  ComputeMeanCentroid         (   allmaps, datatype,   polarity,               labels,  l       );
        case    WeightedMeanCentroid:   return  ComputeWeightedMeanCentroid (   allmaps, datatype,   polarity,               labels,  l,  ref );
        case    MedianCentroid:         return  ComputeMedianCentroid       (   allmaps, datatype,   polarity,               labels,  l       );
        case    MedoidCentroid:         return  ComputeMedoidCentroid       (   allmaps, datatype,   polarity,   maxsamples, labels,  l       );
        case    EigenVectorCentroid:    return  ComputeEigenvectorCentroid  (   allmaps, datatype,   polarity,   maxsamples, labels,  l       );
        case    MaxCentroid:            return  ComputeMaxCentroid          (   allmaps, datatype,   polarity,               labels,  l       );
        default:                        return  map;            // null map
        }
    }
}

                                        // Slight overhead to extract the indexes
TMap    TMaps::ComputeCentroid  (   CentroidType        centroid,
                                    AtomType            datatype,       PolarityType        polarity,
                                    int                 maxsamples,
                                    TLabeling*          labels,         int                 l,      const TMap*             ref
                                )   const
{
TArray1<TMap *>     allmaps;

GetIndexes ( allmaps );

return  crtl::ComputeCentroid ( allmaps, centroid, datatype, polarity, maxsamples, labels, l, ref );
}


//----------------------------------------------------------------------------
                                        // Average of the maps
                                        // Parallel version is pretty much the same as Median, so maybe merge the 2 at some point?
TMap    ComputeMeanCentroid             (   const TArray1<TMap*>&    allmaps,
                                            AtomType            datatype,       PolarityType        polarity,
                                            TLabeling*          labels,         int                 l
                                        )
{
TMap                map;

if ( allmaps.IsNotAllocated () )
    return  map;


int                 Dimension       = allmaps[ 0 ]->GetDim ();
int                 DimensionSP     = Dimension / 3;
int                 NumMaps         = allmaps     . GetDim ();

map.Resize ( Dimension );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get some reference map, both for Scalar or Vectorial data
TMap                refmap;


if ( polarity == PolarityEvaluate && ! labels )

    refmap  = ComputeReferenceMap ( allmaps, datatype, labels, l );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Parallel loop is on Dimension, so we can access stats without locks
TGoEasyStats        stats   ( Dimension );
                                        // No polarity checks and vectorial can also step by 1
int                 stepdim         = polarity == PolarityEvaluate && IsVector ( datatype ) ? 3 : 1;


OmpParallelFor

for ( int dimi = 0; dimi < Dimension; dimi += stepdim ) {

    UpdateApplication;


    for ( int mi = 0; mi < NumMaps; mi++ ) {

                                        // labeling provided?
        if ( labels && (*labels)[ mi ] != l )
            continue;
                                        // getting a handy reference to current map
        const TMap&         mapi            = *allmaps[ mi ];
                                        // skipping null maps - these might come from empty clusters files which contain a null map
        if ( mapi.IsNull () )
            continue;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Evaluate some polarity?
        if ( polarity == PolarityEvaluate ) {

            if ( IsVector ( datatype ) ) {
                                        // !POLARITY EVALUATED INDEPENDENTLY FOR EACH SOLUTION POINT!
                                        // !labels case not handled, segmentation does not work on the SP level!
                int     spi     = dimi / 3;
                                        // per vector polarity check
                bool    invert  = mapi.Get3DVector ( spi ).IsOppositeDirection ( refmap.Get3DVector ( spi ) );

                                        // sum-up each components
                if ( invert ) {
                    stats[ dimi     ].Add ( - mapi[ dimi     ], ThreadSafetyIgnore );
                    stats[ dimi + 1 ].Add ( - mapi[ dimi + 1 ], ThreadSafetyIgnore );
                    stats[ dimi + 2 ].Add ( - mapi[ dimi + 2 ], ThreadSafetyIgnore );
                    }
                else {
                    stats[ dimi     ].Add (   mapi[ dimi     ], ThreadSafetyIgnore );
                    stats[ dimi + 1 ].Add (   mapi[ dimi + 1 ], ThreadSafetyIgnore );
                    stats[ dimi + 2 ].Add (   mapi[ dimi + 2 ], ThreadSafetyIgnore );
                    }

                } // IsVector

            else {                      // Scalar data

                bool    invert  = labels ? labels->IsInverted ( mi )                // labeling provides the polarity
                                         : mapi.IsOppositeDirection ( refmap );     // global evaluation of polarity to refmap

                if ( invert )
                    stats[ dimi ].Add ( - mapi[ dimi ], ThreadSafetyIgnore );
                else
                    stats[ dimi ].Add (   mapi[ dimi ], ThreadSafetyIgnore );

                } // Norm

            } // PolarityEvaluate

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        else
                                        // no polarity evaluation, use data as is for both Scalar and Vectorial
            stats[ dimi ].Add (   mapi[ dimi ], ThreadSafetyIgnore );

        } // for mi

    } // for dimi


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

OmpParallelFor

for ( int i = 0; i < Dimension; i++  )

    map[ i ]    = stats[ i ].Mean ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  map;
}


//----------------------------------------------------------------------------
                                        // Max of each component, across all maps
TMap    ComputeMaxCentroid              (
                                        const TArray1<TMap*>&    allmaps,
                                        AtomType            datatype,       PolarityType        polarity,
                                        TLabeling*          labels,         int                 l
                                        )
{
TMap                map;

if ( allmaps.IsNotAllocated () )
    return  map;


int                 Dimension       = allmaps[ 0 ]->GetDim ();
int                 DimensionSP     = Dimension / 3;
int                 NumMaps         = allmaps     . GetDim ();

map.Resize ( Dimension );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get some reference map, both for Scalar or Vectorial data
TMap                refmap;


if ( polarity == PolarityEvaluate && ! labels && ! IsVector ( datatype ) )

    refmap  = ComputeReferenceMap ( allmaps, datatype, labels, l );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                invert;
TMap                m;

                               // !vectorial data is testing the norm!
map     = IsVector ( datatype ) ? 0 : -FLT_MAX;


for ( int mi = 0; mi < NumMaps; mi++ ) {

    UpdateApplication;

                                        // labeling provided?
    if ( labels && (*labels)[ mi ] != l )
        continue;
                                        // getting a handy reference to current map
    const TMap&         mapi            = *allmaps[ mi ];
                                        // skipping null maps - these might come from empty clusters files which contain a null map
    if ( mapi.IsNull () )
        continue;

                                        // Evaluate some polarity
    if ( polarity == PolarityEvaluate ) {

        if ( IsVector ( datatype ) ) {
                                        // testing for max of norm per SP - direction / polarity does not matter
            for ( int spi = 0; spi < DimensionSP; spi++ ) {

                if ( mapi.Get3DVector ( spi ).Norm2 () > map.Get3DVector ( spi ).Norm2 () )

                    map.Get3DVector ( spi ) = mapi.Get3DVector ( spi );

                } // for SP

            } // IsVector
                                        
        else {                          // Scalar data

            m       = mapi;

            invert  = labels ? labels->IsInverted ( mi )                // labeling provides the polarity
                             : mapi.IsOppositeDirection ( refmap );     // global evaluation of polarity to refmap

            if ( invert )   m.Invert ();
                                        // or ignore polarity -> Absolute?
//          m.Absolute ();

            map.Maxed ( m );
            } // Norm

        } // PolarityEvaluate

    else {                              // no polarity evaluation, do a direct max

        if ( IsVector ( datatype ) ) {
                                        // testing for max of norm per SP - direction / polarity does not matter
            for ( int spi = 0; spi < DimensionSP; spi++ ) {

                if ( mapi.Get3DVector ( spi ).Norm2 () > map.Get3DVector ( spi ).Norm2 () )

                    map.Get3DVector ( spi ) = mapi.Get3DVector ( spi );

                } // for SP

            } // IsVector

        else
            map.Maxed ( mapi );
        }

    }
                                        // is there a way to rescale the max of all dimensions?

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  map;
}


//----------------------------------------------------------------------------
                                        // Median of each track
TMap    ComputeMedianCentroid           (
                                        const TArray1<TMap*>&    allmaps,
                                        AtomType            datatype,       PolarityType        polarity,
                                        TLabeling*          labels,         int                 l
                                        )
{
TMap                map;

if ( allmaps.IsNotAllocated () )
    return  map;


int                 Dimension       = allmaps[ 0 ]->GetDim ();
int                 DimensionSP     = Dimension / 3;
int                 NumMaps         = allmaps     . GetDim ();

map.Resize ( Dimension );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get some reference map, both for Scalar or Vectorial data
TMap                refmap;


if ( polarity == PolarityEvaluate && ! labels )

    refmap  = ComputeReferenceMap ( allmaps, datatype, labels, l );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Parallel loop is on Dimension, so we can access stats without locks
TGoEasyStats        stats   ( Dimension, NumMaps );
                                        // No polarity checks and vectorial can also step by 1
int                 stepdim         = polarity == PolarityEvaluate && IsVector ( datatype ) ? 3 : 1;


OmpParallelFor

for ( int dimi = 0; dimi < Dimension; dimi += stepdim ) {

    UpdateApplication;


    for ( int mi = 0; mi < NumMaps; mi++ ) {

                                        // labeling provided?
        if ( labels && (*labels)[ mi ] != l )
            continue;
                                        // getting a handy reference to current map
        const TMap&         mapi            = *allmaps[ mi ];
                                        // skipping null maps - these might come from empty clusters files which contain a null map
        if ( mapi.IsNull () )
            continue;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Evaluate some polarity?
        if ( polarity == PolarityEvaluate ) {

            if ( IsVector ( datatype ) ) {
                                        // !POLARITY EVALUATED INDEPENDENTLY FOR EACH SOLUTION POINT!
                                        // !labels case not handled, segmentation does not work on the SP level!
                int     spi     = dimi / 3;
                                        // per vector polarity check
                bool    invert  = mapi.Get3DVector ( spi ).IsOppositeDirection ( refmap.Get3DVector ( spi ) );

                                        // stats on each components
                if ( invert ) {
                    stats[ dimi     ].Add ( - mapi[ dimi     ], ThreadSafetyIgnore );
                    stats[ dimi + 1 ].Add ( - mapi[ dimi + 1 ], ThreadSafetyIgnore );
                    stats[ dimi + 2 ].Add ( - mapi[ dimi + 2 ], ThreadSafetyIgnore );
                    }
                else {
                    stats[ dimi     ].Add (   mapi[ dimi     ], ThreadSafetyIgnore );
                    stats[ dimi + 1 ].Add (   mapi[ dimi + 1 ], ThreadSafetyIgnore );
                    stats[ dimi + 2 ].Add (   mapi[ dimi + 2 ], ThreadSafetyIgnore );
                    }

                } // IsVector

            else {                      // Scalar data

                bool    invert  = labels ? labels->IsInverted ( mi )                // labeling provides the polarity
                                         : mapi.IsOppositeDirection ( refmap );     // global evaluation of polarity to refmap

                if ( invert )
                    stats[ dimi ].Add ( - mapi[ dimi ], ThreadSafetyIgnore );
                else
                    stats[ dimi ].Add (   mapi[ dimi ], ThreadSafetyIgnore );

                } // Norm

            } // PolarityEvaluate

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        else
                                        // no polarity evaluation, use data as is for both Scalar and Vectorial
            stats[ dimi ].Add (   mapi[ dimi ], ThreadSafetyIgnore );

        } // for mi

    } // for dimi


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

OmpParallelFor

for ( int i = 0; i < Dimension; i++  )

    map[ i ]    = stats[ i ].Median ( false );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  map;
}


//----------------------------------------------------------------------------
                                        // Most topographically central map of the whole cluster, the one that maximizes the sum of correlations
TMap    ComputeMedoidCentroid           (
                                        const TArray1<TMap*>&    allmaps,
                                        AtomType            datatype,       PolarityType        polarity,
                                        int                 maxsamples,
                                        TLabeling*          labels,         int                 l
                                        )
{
TMap                map;

if ( allmaps.IsNotAllocated () )
    return  map;


int                 Dimension       = allmaps[ 0 ]->GetDim ();
int                 DimensionSP     = Dimension / 3;
int                 NumMaps         = allmaps     . GetDim ();

map.Resize ( Dimension );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get the right number of maps
int                 nummaps         = labels ? labels->GetSizeOfClusters ( l ) : NumMaps;


if ( nummaps == 0 )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // In case of 2 maps, use an average
//if ( nummaps <= 2 )
//
//    return  ComputeMeanCentroid ( polarity );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              maxcorr         = Lowest ( maxcorr );
int                 bestindex       = 0;
int                 step            = AtLeast ( 1, nummaps / maxsamples );
int                 offset2         = AtLeast ( 1, step / 2 );


for ( int mi1 = 0, mri1 = 0; mi1 < NumMaps; mi1++ ) {

    UpdateApplication;

                                        // skip wrong labels, if any
    if ( labels && (*labels)[ mi1 ] != l )
        continue;
                                        // getting a handy reference to current map
    const TMap&         mapi1           = *allmaps[ mi1 ];
                                        // skipping null maps - these might come from empty clusters files which contain a null map
    if ( mapi1.IsNull () )
        continue;
                                        // from the right labels, downsample the remaining maps
    if ( mri1++ % step )
        continue;


    double      sumcorr     = 0;

    for ( int mi2 = offset2, mri2 = 0; mi2 < NumMaps; mi2++ ) {

        if ( mi2 == mi1 )
            continue;

        if ( labels && (*labels)[ mi2 ] != l )
            continue;
                                        // getting a handy reference to current map
        const TMap&         mapi2           = *allmaps[ mi2 ];
                                        // skipping null maps - these might come from empty clusters files which contain a null map
        if ( mapi2.IsNull () )
            continue;

        if ( mri2++ % step )
            continue;

                                        // Evaluate some polarity
        if ( polarity == PolarityEvaluate ) {

            if ( IsVector ( datatype ) ) {
                                        // polarity evaluated per solution point
                                        // labels polarity is irrelevant here
                for ( int spi = 0; spi < DimensionSP; spi++ )
                                        // sum of Absolute of SP-by-SP projection
                    sumcorr    += fabs ( mapi2.Get3DVector ( spi ).ScalarProduct ( mapi1.Get3DVector ( spi ) ) );

                } // IsVector
                                        
            else {                      // Scalar data
                                        // We don't make use of labeling polarity, because we test maps 2 by 2
//              sumcorr    += Project ( mapi2, mapi1, polarity );       // only for normalized data
                sumcorr    += fabs ( mapi2.ScalarProduct ( mapi1 ) );   // other cases
                }

            } // PolarityEvaluate

        else
                                        // no polarity evaluation, do a direct sum for both Scalar and Vectorial
//          sumcorr    += Project ( mapi2, mapi1, polarity );   // only for normalized data
            sumcorr    += mapi2.ScalarProduct ( mapi1 );        // other cases

        } // for mi2

                                        // In case data was not normalized, do it here as we need to project on a normalized map1
    sumcorr    /= NonNull ( mapi1.Norm () );


    if ( sumcorr > maxcorr ) { 
        maxcorr     = sumcorr;
        bestindex   = mi1;
        }

    } // for mi1


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  *allmaps[ bestindex ];
}


//----------------------------------------------------------------------------
                                        // Returns a new map with, for each SP, the dipole which direction maximizes the projection of all data
                                        // Similar to  ComputeMedoidCentroid  but at a SP level
                                        // Works with and without labeling
                                        // !Assumes data is Vectorial and that we do look after orientation!
/*
                                        // Normalized cloud optimal direction
                                        // Min Average Orthogonal Residual from normalized cloud
TMap    ComputeCloudsFolding    (   const TArray1<TMap*>&    allmaps,
                                    int                 maxsamples,
                                    TLabeling*          labels,     int         l       
                                )
{
TMap                map;

if ( allmaps.IsNotAllocated () )
    return  map;

int                 Dimension3      = allmaps[ 0 ]->GetDim ();
int                 Dimension       = Dimension3 / 3;
int                 NumMaps         = allmaps     . GetDim ();

                                        // init as null vectors
map.Resize ( Dimension3 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get the right number of maps
int                 nummaps         = labels ? labels->GetSizeOfClusters ( l, l ) : NumMaps;


if ( nummaps == 0 )
    return  map;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TMap                sumdist     ( Dimension );
TMap                mindist     ( Dimension );
TArray1<int>        bestindex   ( Dimension );
TMap                numsum      ( Dimension );
int                 step            = AtLeast ( 1, nummaps / maxsamples );
int                 offset2         = AtLeast ( 1, step / 2 );


mindist     = Highest ( mindist );


for ( int mi1 = 0, mri1 = 0; mi1 < NumMaps; mi1++ ) {

    UpdateApplication;

                                        // skip wrong labels, if any
    if ( labels && (*labels)[ mi1 ] != l )
        continue;
                                        // skipping null maps - these might come from empty clusters files which contain a null map
    if ( allmaps[ mi1 ]->IsNull () )
        continue;
                                        // from the right labels, downsample the remaining maps
    if ( mri1++ % step )
        continue;


    TVector3Float*      v1      = &allmaps[ mi1 ]->Get3DVector ( 0 );

    sumdist = 0;
    numsum  = 0;

    for ( int mi2 = offset2, mri2 = 0; mi2 < NumMaps; mi2++ ) {

        if ( mi2 == mi1 )
            continue;

        if ( labels && (*labels)[ mi2 ] != l )
            continue;
                                        // skipping null maps - these might come from empty clusters files which contain a null map
        if ( allmaps[ mi2 ]->IsNull () )
            continue;

        if ( mri2++ % step )
            continue;


        TVector3Float*      v2      = &allmaps[ mi2 ]->Get3DVector ( 0 );

        for ( int i0 = 0; i0 < Dimension; i0++ ) {

            if ( v1[ i0 ].IsNull () || v2[ i0 ].IsNull () )
                continue;
                                        // optimal direction: min avg of squared angular error
//          sumdist[ i0 ]  += Square ( acos ( fabs ( v2[ i0 ].Cosine ( v1[ i0 ] ) ) ) );
                                        // optimal direction: min avg of squared error after projection (sine)
//            sumdist[ i0 ]  -= Square ( v2[ i0 ].Cosine ( v1[ i0 ] ) );
                                        // same results, faster
            sumdist[ i0 ]  -= fabs ( v2[ i0 ].Cosine ( v1[ i0 ] ) );

            numsum [ i0 ]++;
            }

        }

    sumdist    /= numsum;

                                        // test for each SP if this is the best orientation
    for ( int i0 = 0; i0 < Dimension; i0++ )

        if ( sumdist    [ i0 ] < mindist[ i0 ] 
          && v1[ i0 ].IsNotNull ()             ) {  // null vector is not desirable for testing polarity...
            mindist     [ i0 ]  = sumdist[ i0 ];
            bestindex   [ i0 ]  = mi1;
            }

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // copy the dipoles with highest spread / preferred direction
for ( int i0 = 0; i0 < Dimension; i0++ ) {
                                        // !we are patching with SP vectors from different data point!
    map.Get3DVector ( i0 )  = allmaps[ bestindex[ i0 ] ]->Get3DVector ( i0 );

                                        // rough re-alignment, pointing the main direction to either +X, +Y or +Z
    TVector3Float&  v   = map.Get3DVector ( i0 );

    if ( fabs ( v.X ) > fabs ( v.Y ) && fabs ( v.X ) > fabs ( v.Z ) && v.IsOppositeDirection ( TVector3Float ( 1, 0, 0 ) )
      || fabs ( v.Y ) > fabs ( v.X ) && fabs ( v.Y ) > fabs ( v.Z ) && v.IsOppositeDirection ( TVector3Float ( 0, 1, 0 ) )
      || fabs ( v.Z ) > fabs ( v.X ) && fabs ( v.Z ) > fabs ( v.Y ) && v.IsOppositeDirection ( TVector3Float ( 0, 0, 1 ) ) )

            v.Invert ();

                                        // for display use, does not change the sign of directional test
//  v.Normalize ();
    }


return  map;
}
*/


                                        // Computes the best axis to test for polarity inversion, where we can fold the cloud of points
TVector3Float       ComputeCloudFolding     (   const TPoints&      points,
                                                int                 numcandidates,
                                                int                 numtests
                                            )
{
TDownsampling       downcand    ( (int) points, numcandidates );
TDownsampling       downtest    ( (int) points, numtests      );
TEasyStats          blob;
double              crit;
double              mincrit         = Highest ( mincrit );
int                 bestindex       = 0;
TVector3Float       v1;


for ( int mi1 = downcand.From; mi1 <= downcand.To; mi1 += downcand.Step ) {

                                        // Copy & normalize candidate vector
    v1  = points[ mi1 ];
                                        // this can happen, we can not test any polarity with this!
    if ( v1.IsNull () )
        continue;

    v1.Normalize ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute stats of projected vectors
    blob.Reset ();

    for ( int mi2 = downtest.From; mi2 <= downtest.To; mi2 += downtest.Step )

        if ( ! ( mi1 == mi2 || points[ mi2 ].IsNull () ) )
                                        // even for non-spontaneous data - we are looking for a main direction(?)
            blob.Add ( fabs ( points[ mi2 ].ScalarProduct ( v1 ) ), ThreadSafetyIgnore );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Best separation criterion: high mean + low SD
//  crit    = blob.SD () - blob.Mean ();
    crit    = blob.CoV ();

                                        // Keep vectors which minimize criterion
    if ( crit < mincrit ) {
        mincrit     = crit;
        bestindex   = mi1;
        }

    } // for sample


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we have our champion!
v1  = points[ bestindex ];

                                        // better for the eyes, but doesn't matter in terms of polarity testing
v1.Normalize ();


return  v1;
}

                                        // Non-normalized cloud topology
                                        // Min SDCloud & Max AvgCloud
TMap    ComputeCloudsFolding    (   const TArray1<TMap*>&    allmaps,
                                    int                 maxsamples,
                                    TLabeling*          labels,     int         l       
                                )
{
TMap                map;

if ( allmaps.IsNotAllocated () )
    return  map;

int                 Dimension       = allmaps[ 0 ]->GetDim ();
int                 DimensionSP     = Dimension / 3;
int                 NumMaps         = allmaps     . GetDim ();

                                        // init as null vectors
map.Resize ( Dimension );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get the right number of maps
int                 nummaps         = labels ? labels->GetSizeOfClusters ( l ) : NumMaps;


if ( nummaps == 0 )

    return  map;

                                        // it needs a minimum amount of data point to be a cloud...
if ( nummaps < maxsamples /*ReferenceNumSamplesSP*/ ) {

    map.ResetMemory ();
                                        // just a backup strategy
    for ( int tfi = 0; tfi < NumMaps; tfi++ )
    
        if ( labels && (*labels)[ tfi ] != l )
                                        // hoping for the best with an averaging - very low number of maps should come from ERP case, and we can expect vectors to be somehow consistent...
            map    += *(allmaps[ tfi ]);

    map    /= nummaps;

    return  map;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TPoints             cloud;
TVector3Float       v;

                                        // doing 1 SP at a time
for ( int spi = 0; spi < DimensionSP; spi++ ) {

    cloud.Reset ();

                                        // populate the cloud with the appropriate points
    for ( int tfi = 0; tfi < NumMaps; tfi++ ) {
    
        if ( labels && (*labels)[ tfi ] != l )
            continue;

        v   = allmaps[ tfi ]->Get3DVector ( spi );

        if ( v.IsNotNull () )
            cloud.Add ( v );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // getting best folding axis
    v   = ComputeCloudFolding ( cloud,   maxsamples,    ReferenceNumSamplesSP2 );

                                        // orientation is all relative, we can do a rough re-alignment, pointing the main direction to either +X, +Y or +Z
    if ( fabs ( v.X ) > fabs ( v.Y ) && fabs ( v.X ) > fabs ( v.Z ) && v.IsOppositeDirection ( TVector3Float ( 1, 0, 0 ) )
      || fabs ( v.Y ) > fabs ( v.X ) && fabs ( v.Y ) > fabs ( v.Z ) && v.IsOppositeDirection ( TVector3Float ( 0, 1, 0 ) )
      || fabs ( v.Z ) > fabs ( v.X ) && fabs ( v.Z ) > fabs ( v.Y ) && v.IsOppositeDirection ( TVector3Float ( 0, 0, 1 ) ) )

        v.Invert ();


    map.Get3DVector ( spi ) = v;

    } // for spi


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  map;
}

/*
                                        // Non-normalized cloud centroid
                                        // Avg IterativeCentroids
TMap    ComputeCloudsFolding    (   const TArray1<TMap*>&    allmaps,
                                    int                 /*maxsamples* /,
                                    TLabeling*          labels,     int         l       
                                )
{
TMap                map;

if ( allmaps.IsNotAllocated () )
    return  map;

int                 Dimension3      = allmaps[ 0 ]->GetDim ();
int                 Dimension       = Dimension3 / 3;
int                 NumMaps         = allmaps     . GetDim ();

                                        // init as null vectors
map.Resize ( Dimension3 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get the right number of maps
int                 nummaps         = labels ? labels->GetSizeOfClusters ( l, l ) : NumMaps;


if ( nummaps == 0 )
    return  map;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TMap                numsum      ( Dimension );
TRandUniform        randunif;
int                 numseeds        = 31;
int                 numconv         = 25;
int                 numavg          = 200;
TPoints             axis        ( Dimension );
TPoints             centroid    ( Dimension );
TPoints             meancentroid( Dimension );


int                 mi1;
TVector3Float*      v1;
TVector3Float*      v2;


for ( int seedi = 0; seedi < numseeds; seedi++ ) {

    do mi1 = randunif ( (uint) NumMaps ); while ( labels && (*labels)[ mi1 ] != l );

    v1  = &allmaps[ mi1 ]->Get3DVector ( 0 );

                                        // init axis as random point
    for ( int i0 = 0; i0 < Dimension; i0++ )
        axis[ i0 ]  = v1[ i0 ];


    for ( int convi = 0; convi < numconv; convi++ ) {

        centroid.ResetMemory ();
        numsum  = 0;


        for ( int si2 = 0, mi2; si2 < numavg; ) {

            mi2     = randunif ( (uint) NumMaps );

            if ( labels && (*labels)[ mi2 ] != l )
                continue;

            si2++;


            v2  = &allmaps[ mi2 ]->Get3DVector ( 0 );

            for ( int i0 = 0; i0 < Dimension; i0++ ) {

                if ( v2[ i0 ].IsNull () )
                    continue;


                centroid[ i0 ].Cumulate ( v2[ i0 ], v2[ i0 ].IsOppositeDirection ( axis[ i0 ] ) );

                numsum [ i0 ]++;
                }

            }

        centroid   /= numsum;
                                        // new axis is updated centroid
        axis        = centroid;
        } // for convergence


    for ( int i0 = 0; i0 < Dimension; i0++ )
                                                   // OK even if meancentroid is null
        meancentroid[ i0 ].Cumulate ( centroid[ i0 ], meancentroid[ i0 ].IsOppositeDirection ( centroid[ i0 ] ) );

    } // for seedi

                                        // average of all seeds final convergence centroids
meancentroid   /= numseeds;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // copy the dipoles with highest spread / preferred direction
for ( int i0 = 0; i0 < Dimension; i0++ ) {

    map.Get3DVector ( i0 )  = meancentroid[ i0 ];

                                        // rough re-alignment, pointing the main direction to either +X, +Y or +Z
    TVector3Float&  v   = map.Get3DVector ( i0 );

    if ( fabs ( v.X ) > fabs ( v.Y ) && fabs ( v.X ) > fabs ( v.Z ) && v.IsOppositeDirection ( TVector3Float ( 1, 0, 0 ) )
      || fabs ( v.Y ) > fabs ( v.X ) && fabs ( v.Y ) > fabs ( v.Z ) && v.IsOppositeDirection ( TVector3Float ( 0, 1, 0 ) )
      || fabs ( v.Z ) > fabs ( v.X ) && fabs ( v.Z ) > fabs ( v.Y ) && v.IsOppositeDirection ( TVector3Float ( 0, 0, 1 ) ) )

            v.Invert ();

                                        // for display use, does not change the sign of directional test
//  v.Normalize ();
    }


return  map;
}
*/

//----------------------------------------------------------------------------
                                        // 2 stages computation:
                                        //  - Get a good centroid: either optionally provided, or using the Medoid
                                        //  - Compute the weight from the correlation to this centroid

                                        // Ranked weights version:
TMap    ComputeWeightedMeanCentroid     (   const TArray1<TMap*>&    allmaps,
                                            AtomType            datatype,       PolarityType        polarity,
                                            TLabeling*          labels,         int                 l,      const TMap*         ref
                                        )
{
TMap                map;

if ( allmaps.IsNotAllocated () )
    return  map;


int                 Dimension       = allmaps[ 0 ]->GetDim ();
int                 DimensionSP     = Dimension / 3;
int                 NumMaps         = allmaps     . GetDim ();

map.Resize ( Dimension );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get some reference map: per solution point for vectorial data, or the Medoid for scalar data
TMap                refmap;

if      ( ref != 0 && ref->IsNonNull () )
                                        // reference could be provided by caller, like a known template map - saves quite some computation
    refmap  = *ref;

else if ( IsVector ( datatype ) )
                                        // Best orientation per SP
    refmap  = ComputeCloudsFolding      ( allmaps, ReferenceNumSamplesSP, labels, l );

else
                                        // Global Medoid
    refmap  = ComputeMedoidCentroid     ( allmaps, datatype, polarity, MedoidNumSamples, labels, l );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // storing weight per maps
enum                WeightsEnum { 
                    WeightIndex, 
                    MapIndex, 
                    NumWeightsDim, 
                    SignIndex       = NumWeightsDim
                    };
                                                      // we might need to additionally store signs, either per map or per dipole
TArray2<float>      weights ( NumMaps, NumWeightsDim + ( polarity == PolarityEvaluate ? ( IsVector ( datatype ) ? DimensionSP 
                                                                                                                : 1           ) 
                                                                                      : 0                                       ) );
int                 numweigths      = 0;

                                        // we might also need to store per-dipole signs
TMap                signs ( polarity == PolarityEvaluate && IsVector ( datatype ) ? DimensionSP : 0 );


for ( int mi = 0; mi < NumMaps; mi++ ) {

    UpdateApplication;

                                        // labeling provided?
    if ( labels && (*labels)[ mi ] != l )
        continue;
                                        // getting a handy reference to current map
    const TMap&         mapi            = *allmaps[ mi ];
                                        // skipping null maps - these might come from empty clusters files which contain a null map
    if ( mapi.IsNull () )
        continue;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Evaluate some polarity?
    if ( polarity == PolarityEvaluate ) {

        if ( IsVector ( datatype ) ) {
                                        // !POLARITY EVALUATED INDEPENDENTLY FOR EACH SOLUTION POINT!
                                        // !labels case not handled, segmentation does not work on the SP level!
            double      corr    = refmap.CorrelationDipoles  ( mapi, PolarityEvaluate, &signs );

            if ( corr <= 0 )
                continue;

            weights ( numweigths, WeightIndex   )   = corr;
            weights ( numweigths, MapIndex      )   = mi;
                                        // copy the per-dipole signs factors, which saves computing again dipoles opposite directions
            for ( int spi = 0; spi < DimensionSP; spi++ )
                weights ( numweigths, SignIndex + spi ) = signs ( spi );
            } // IsVector

        else {                          // Scalar data

            PolarityType    pol     = labels ? labels->GetPolarity ( mi )       // labeling provides the polarity
                                             : PolarityEvaluate;                // will locally evaluate polarity to refmap

            double      corr    = refmap.Correlation ( mapi, pol );

            if ( corr <= 0 )
                continue;

            weights ( numweigths, WeightIndex   )   = corr;
            weights ( numweigths, MapIndex      )   = mi;
            weights ( numweigths, SignIndex     )   = labels ? PolarityToSign ( labels->GetPolarity ( mi ) )            // store global map sign
                                                             : TrueToMinus    ( refmap.IsOppositeDirection ( mapi ) );
            } // Scalar

        } // PolarityEvaluate

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    else {                              // Direct polarities

        if ( IsVector ( datatype ) ) {
                                        // formula is actually a tiny bit different than the scalar correlation - also, we don't need to store the signs
            double      corr    = refmap.CorrelationDipoles  ( mapi, polarity );

            if ( corr <= 0 )
                continue;

            weights ( numweigths, WeightIndex   )   = corr;
            weights ( numweigths, MapIndex      )   = mi;
            } // IsVector

        else {                          // Scalar data
                                        // no polarity evaluation, use data as is for both Scalar and Vectorial
            double      corr    = refmap.Correlation ( mapi, polarity );

            if ( corr <= 0 )
                continue;

            weights ( numweigths, WeightIndex   )   = corr;
            weights ( numweigths, MapIndex      )   = mi;
            } // Scalar
        }

                                        // We have 1 more map in
    numweigths++;
    } // for mi


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Safety check
if ( numweigths == 0 )
    return  refmap;

                                        // We are interested in the top-most correlated maps to the referential one
weights.SortRows ( WeightIndex, Descending, numweigths );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//constexpr double  toppercentilecut= 0.25;     // Using only the top Quartile of data closest to the centroid
constexpr double    toppercentilecut= 1.0;      // Using ALL data positively correlated to the centroid, but weighted by proximity
constexpr int       minmaps         = 1;
constexpr int       maxmaps         = Highest ( maxmaps );  // Limiting number of closest maps(?)
int                 nummaps         = Clip ( Round ( numweigths * toppercentilecut ), minmaps, maxmaps );
double              sumw            = 0;


for ( int wi = 0; wi < nummaps; wi++ ) {

  //double              w           = PearsonToKendall ( weights ( wi, WeightIndex ) ); // de-skewed distance weight
    double              w           = nummaps - wi;                                     // rank weight - no need to normalize..
                        sumw       += w;                                                // ..as we cumulate the total sum of weights for normalization anyway
    int                 mi          = weights ( wi, MapIndex    );
    const TMap&         mapi        = *allmaps[ mi ];


    if ( polarity == PolarityEvaluate ) {

        if ( IsVector ( datatype ) ) {

            for ( int spi = 0; spi < DimensionSP; spi++ ) {
                                        // sign is evaluated per-dipole 
                double      sign    = weights ( wi, SignIndex + spi );

                map  [ 3 * spi     ]   += sign * w * mapi[ 3 * spi     ];
                map  [ 3 * spi + 1 ]   += sign * w * mapi[ 3 * spi + 1 ];
                map  [ 3 * spi + 2 ]   += sign * w * mapi[ 3 * spi + 2 ];
                } // for spi
            } // IsVector

        else {                          // Scalar data
            double      sign    = weights ( wi, SignIndex );

            map.Cumulate ( mapi, sign * w );      
            } // Scalar

        } // PolarityEvaluate

    else                                // Direct polarities - for both vectorial and scalar cases

        map.Cumulate ( mapi, w );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // rescale by total sum of weights                                        
map    /= sumw;

return  map;
}

/*                                      // Top correlation version
TMap    ComputeWeightedMeanCentroid     (   const TArray1<TMap*>&    allmaps,
                                            AtomType            datatype,       PolarityType        polarity,
                                            TLabeling*          labels,         int                 l,      const TMap*         ref
                                        )
{
TMap                map;

if ( allmaps.IsNotAllocated () )
    return  map;


int                 Dimension       = allmaps[ 0 ]->GetDim ();
int                 DimensionSP     = Dimension / 3;
int                 NumMaps         = allmaps     . GetDim ();

map.Resize ( Dimension );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get some reference map: per solution point for vectorial data, or the Medoid for scalar data
TMap                refmap;

if      ( ref != 0 )
                                        // reference could be provided by caller, like a known template map - saves quite some computation
    refmap  = *ref;

else if ( IsVector ( datatype ) )
                                        // Best orientation per SP
    refmap  = ComputeCloudsFolding      ( allmaps, ReferenceNumSamplesSP, labels, l );

else
                                        // Global Medoid
    refmap  = ComputeMedoidCentroid     ( allmaps, datatype, polarity, MedoidNumSamples, labels, l );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Taking the maps that correlated the most to the template
constexpr double    limittau        = 0.75;// 0.5;

auto    CorrelationToWeight = [ limittau ] ( double corr )
{
              // De-skewing Correlation with Kendall Tau formula
                                          // then rescaling to [0..1]
return  Clip ( ( PearsonToKendall ( corr ) - limittau ) / ( 1 - limittau ), 0.0, 1.0 );
};


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we might also need to store per-dipole signs
TMap                dipolesign ( polarity == PolarityEvaluate && IsVector ( datatype ) ? DimensionSP : 0 );
double              sumw            = 0;


for ( int mi = 0; mi < NumMaps; mi++ ) {

    UpdateApplication;

                                        // labeling provided?
    if ( labels && (*labels)[ mi ] != l )
        continue;
                                        // getting a handy reference to current map
    const TMap&         mapi            = *allmaps[ mi ];
                                        // skipping null maps - these might come from empty clusters files which contain a null map
    if ( mapi.IsNull () )
        continue;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Evaluate some polarity?
    if ( polarity == PolarityEvaluate ) {

        if ( IsVector ( datatype ) ) {
                                        // !POLARITY EVALUATED INDEPENDENTLY FOR EACH SOLUTION POINT!
                                        // !labels case not handled, segmentation does not work on the SP level!
            double      w       = CorrelationToWeight ( refmap.CorrelationDipoles  ( mapi, PolarityEvaluate, &dipolesign ) );

            if ( w <= 0 )
                continue;


//          OmpParallelFor
            for ( int spi = 0; spi < DimensionSP; spi++ ) {
                                        // sign is evaluated per-dipole 
                double      sign    = dipolesign ( spi );

                map  [ 3 * spi     ]   += sign * w * mapi[ 3 * spi     ];
                map  [ 3 * spi + 1 ]   += sign * w * mapi[ 3 * spi + 1 ];
                map  [ 3 * spi + 2 ]   += sign * w * mapi[ 3 * spi + 2 ];
                } // for spi

            sumw   += w;
            } // IsVector

        else {                          // Scalar data

            PolarityType    pol     = labels ? labels->GetPolarity ( mi )       // labeling provides the polarity
                                             : PolarityEvaluate;                // will locally evaluate polarity to refmap

            double      w       = CorrelationToWeight ( refmap.Correlation ( mapi, pol ) );

            if ( w <= 0 )
                continue;

            map.Cumulate ( mapi, w );                       
            sumw   += w;
            } // Scalar

        } // PolarityEvaluate

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    else {                              // Direct polarities

        if ( IsVector ( datatype ) ) {
                                        // formula is actually a tiny bit different than the scalar correlation - also, we don't need to store the signs
            double      w       = CorrelationToWeight ( refmap.CorrelationDipoles  ( mapi, polarity ) );

            if ( w <= 0 )
                continue;

            map.Cumulate ( mapi, w );                       
            sumw   += w;
            } // IsVector

        else {                          // Scalar data
                                        // no polarity evaluation, use data as is for both Scalar and Vectorial
            double      w       = CorrelationToWeight ( refmap.Correlation ( mapi, polarity ) );

            if ( w <= 0 )
                continue;

            map.Cumulate ( mapi, w );                       
            sumw   += w;
            } // Scalar
        }

    } // for mi


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // rescale by total sum of weights                                        
if ( sumw != 0 )
    map    /= sumw;
else
    map     = refmap;   // by safety

return  map;
}
*/

//----------------------------------------------------------------------------
                                        // Computing the centroid "from scratch" is more difficult than within the clustering:
                                        // during the latter, we constructed the cluster from an existing template, so testing
                                        // each member of the cluster for the polarity against the template does the job to ignore polarity.
                                        // If no template is available, there is no canonical map to be tested against.
                                        // In this case, we need to get the best eigenvector instead.
/*void    TMaps::ComputeEigenvectorCentroid (   TMap&           map, 
                                                PolarityType    polarity,
                                                TLabeling*      labels,     int         l       
                                            )
{
//map.ResetMemory ();
map     = 0;

                                        // get the right number of maps
int                 nummaps         = labels ? labels->GetSizeOfClusters ( l, l ) : NumMaps;


if ( nummaps == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Currently we need to convert the data to a TArray2...
                                        // !order for faster transfer!
TTracks<float>      eegb    ( nummaps, Dimension );
int                 belongtocluster = 0;

                                        // transfer data
for ( int mi = 0, mri = 0; mi < NumMaps;   mi++ )

    if ( ! labels || (*labels)[ mi ] == l ) {

        for ( int e  = 0; e  < Dimension; e ++ )

            eegb ( mri, e ) = Maps[ mi ][ e ];

        mri++;

        belongtocluster = mi;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Matrix              V;
TVector<float>      eigenvalues;

                                        // Note about centering the data:
                                        // - PCA "requires" to have the data centered in time, per electrode
                                        //   It does indeed give a better 1 & 2 components in case of templates
                                        // - In case of templates, data are centered across electrodes, not time
                                        //   If we do the PCA without the centering, the 1 & 2 components "look" nice and preserved the norm = 1 and center
                                        //   but it looks like the discrimination is less good than with the center across time
bool                timecentering   = false; // ! pcatemplates;   // true;
bool                variablesindim1 = false;

                                        // returns the Eigenvectors, Eigenvalues, and the transformed data in original buffer
PCA ( eegb, variablesindim1, timecentering, -1, -1, V, eigenvalues /*, &gauge* / );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get the first eigenvector
for ( int e  = 0; e  < Dimension; e ++ )

    map ( e )   = V[ 0 ][ e ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // By default, eigenvalues don't care for orientation, so we are good for ignore polarity
if ( polarity == PolarityDirect ) {
                                        // But in case polarity matters, re-orient the map with any map from the cluster
    if ( map.IsOppositeDirection ( Maps[ belongtocluster ] ) )

        map.Invert ();
    }
}
*/

                                        // Do an optimized computation to extract ONLY the biggest eigenvector (and eigenvalue BTW)
                                        // NOT working on vectorial data
TMap    ComputeEigenvectorCentroid      (
                                        const TArray1<TMap*>&    allmaps,
                                        AtomType            datatype,       PolarityType        polarity,
                                        int                 maxsamples,
                                        TLabeling*          labels,         int                 l
                                        )
{
TMap                map;

if ( allmaps.IsNotAllocated () )
    return  map;


int                 Dimension       = allmaps[ 0 ]->GetDim ();
int                 NumMaps         = allmaps     . GetDim ();

map.Resize ( Dimension );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get the right number of maps
int                 nummaps         = labels ? labels->GetSizeOfClusters ( l ) : NumMaps;


if ( nummaps == 0 )
    return  map;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Build covariance matrix (double version doesn't seem any better)
TTracks<float>      covariance ( Dimension, Dimension );
LabelType           belongtocluster = UndefinedLabel - 1;
int                 step            = AtLeast ( 1, Truncate ( nummaps / NonNull ( maxsamples ) ) );


for ( int mi = 0, mri = 0; mi < NumMaps; mi++ ) {

    UpdateApplication;

                                        // skip wrong labels, if any
    if ( labels && (*labels)[ mi ] != l )
        continue;
                                        // getting a handy reference to current map
    const TMap&         mapi            = *allmaps[ mi ];
                                        // skipping null maps - these might come from empty clusters files which contain a null map
    if ( mapi.IsNull () )
        continue;
                                        // from the right labels, downsample the remaining maps
    if ( mri++ % step )
        continue;

                                        // remember 1 data point from cluster
    belongtocluster = mi;


    if ( IsVector ( datatype ) )
                                        // !don't mix X, Y and Z data!
        for ( int i = 0; i < Dimension; i+=3  )
        for ( int j = 0; j <= i;        j+=3  ) {

            covariance ( j    , i     ) = covariance ( i    , j     )  += mapi[ i     ] * mapi[ j     ];
            covariance ( j + 1, i + 1 ) = covariance ( i + 1, j + 1 )  += mapi[ i + 1 ] * mapi[ j + 1 ];
            covariance ( j + 2, i + 2 ) = covariance ( i + 2, j + 2 )  += mapi[ i + 2 ] * mapi[ j + 2 ];
            }

    else // regular scalar

        for ( int i = 0; i < Dimension; i++  )
        for ( int j = 0; j <= i;        j++  )

            covariance ( j, i ) = covariance ( i, j )  += mapi[ i ] * mapi[ j ];
    }


covariance     /= NonNull ( nummaps - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Note about centering the data:
                                        // - PCA "requires" to have the data centered in time, per electrode
                                        //   It does indeed give a better 1 & 2 components in case of templates
                                        // - In case of templates, data are centered across electrodes, not time
                                        //   If we do the PCA without the centering, the 1 & 2 components "look" nice and preserved the norm = 1 and center
                                        //   but it looks like the discrimination is less good than with the center across time
//bool                timecentering   = false; // ! pcatemplates;   // true;


TVector<float>      z ( Dimension );
TVector<float>      v ( Dimension );

                                        // initialize estimate with random values [-1..1]
v.Random ( -1.0, 1.0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Convergence
double              lambda          = 0;
double              lambdaold;
double              error;
#define             EigenvaluesMaxIterations    50
#define             EigenvaluesConvergence      1e-6


for ( int iter = 0; iter < EigenvaluesMaxIterations; iter++ ) {
                                        // apply matrix to current estimate, again and again
    v.Apply ( covariance );

    z       = v;

    v.Normalize ();


    lambdaold   = lambda;

    lambda      = v.ScalarProduct ( z );

    error       = RelativeDifference ( lambda, lambdaold );

//  if ( VkQuery () )  DBGV4 ( iter + 1, nummaps, lambda, error * 1e6, "iter, nummaps, lambda, error" );

    if ( error < EigenvaluesConvergence )
        break;

    } // for iter


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Copy the first eigenvector
                                        // it doesn't seem to be possible to avoid returning a normalized map
map     = v;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // By default, eigenvalues don't care for orientation, so we are good for ignore polarity (for once it is practical!)
                                        // But in case where polarity matters, re-orient the map with any map from the cluster
if ( polarity == PolarityDirect && map.IsOppositeDirection ( *allmaps[ belongtocluster ] ) )

    map.Invert ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  map;
}


//----------------------------------------------------------------------------
                                        // Labeling -> Maps
                                        // Works only on scalar data, not vectorial
                                        // !Can update negmap when converging to new centroid!
void    TMaps::LabelingToCentroids  (   
                                    const TMaps&        data,           const TArray1<TMap *>*  todata, // actually optional, used for speeding things up
                                    int                 nclusters,  
                                    TLabeling&          labels,
                                    PolarityType        polarity,       CentroidType            centroid,       bool                ranking,
                                    bool                updatepolarity 
                                    )
{
                                        // nothing to do?
if ( nclusters <= 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // be nice, if caller has not allocated the buffer, do it now?
//if ( IsNotAllocated () )
//    Resize ( nclusters, data.GetDimension () );  


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Loop through each cluster, then compute the centroids on each subset of data
                                        // note however that we compute the centroids on all data

                                        // ComputeCentroid is parallelized
if ( todata )

    for ( int nc = 0; nc < nclusters; nc++ )

        Maps[ nc ]  = crtl::ComputeCentroid (   *todata,
                                                centroid, 
                                                AtomTypeScalar,     polarity, 
                                                LabelingNumSamples,
                                                &labels,            nc,         &Maps[ nc ]     // we have the template to be used as centroid
                                            );
else {
                                        // slower as it needs to compute the pointers to maps - still handy if called only a few times...
    TArray1<TMap*>      allmaps;

    data.GetIndexes ( allmaps );


    for ( int nc = 0; nc < nclusters; nc++ )

        Maps[ nc ]  = crtl::ComputeCentroid (   allmaps,
                                                centroid, 
                                                AtomTypeScalar,     polarity, 
                                                LabelingNumSamples, 
                                                &labels,            nc,         &Maps[ nc ]     // we have the template to be used as centroid
                                            );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // average of ranked data is not ranked, so lets re-rank results
if ( ranking )      ToRank ( AtomTypeScalar, RankingOptions ( RankingAccountNulls | RankingMergeIdenticals ) );

                                        // final results are normalized
Normalize ( AtomTypeScalar, nclusters, false );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We have new maps, make the polarity coherent although the labeling remains the same
if ( updatepolarity )

    labels.UpdatePolarities ( data, 0, labels.GetNumData () - 1, *this, polarity );
}


//----------------------------------------------------------------------------
                                        // Returned indexes:  index1 < index2
                                        // Using real correlation
double  TMaps::GetClosestPair   (   int             nclusters, 
                                    PolarityType    polarity,
                                    LabelType&      index1,     LabelType&      index2
                                )
{
double              maxcorr     = Lowest ( maxcorr );

index1      = index2    = -1;

                                        // search for the closest pair of maps
OmpParallelFor

for ( int nc1 = 0;       nc1 < nclusters; nc1++ )
for ( int nc2 = nc1 + 1; nc2 < nclusters; nc2++ ) {
                                        // polarity flag is given as is, should be PolarityEvaluate or PolarityDirect
    double      corr        = Maps[ nc1 ].Correlation ( Maps[ nc2 ], polarity, false );

    if ( corr > maxcorr ) {

        OmpCriticalBegin (TMapsGetClosestPair)

        maxcorr = corr;
        index1  = nc1;
        index2  = nc2;

        OmpCriticalEnd
        }
    }


return  maxcorr;
}


//----------------------------------------------------------------------------
                                        // Wrapper to TTracksFilters
bool    TMaps::FilterSpatial ( SpatialFilterType filtertype, const char *xyzfile )
{
if ( filtertype == SpatialFilterNone )
    return  true;

if ( StringIsEmpty ( xyzfile ) )
    return  false;


TFilterSpatial<TMapAtomType>    filter  (   filtertype,
                                            xyzfile, 
                                            SpatialFilterMaxNeighbors ( DefaultSFDistance ), 
                                            SpatialFilterMaxDistance  ( DefaultSFDistance ) 
                                        );

filter.Apply ( *this );


return  true;
}

                                        // Currently used for Butterworth & Envelope filters
void    TMaps::FilterTime ( FilterTypes filtertype, FctParams& params, bool showprogress )
{
TVector<double>     line;
TFilter<double>*    filter          = 0;


//if ( VkQuery () )  DBGV ( filtertype, FilterPresets[ filtertype ].Text );

if      ( filtertype == FilterTypeLowPass               )   filter  = new TFilterButterworthLowPass <double> ( params[ FilterParamOrder ], FilterNonCausal, SamplingFrequency, params[ FilterParamFreqCut ] );
else if ( filtertype == FilterTypeHighPass              )   filter  = new TFilterButterworthHighPass<double> ( params[ FilterParamOrder ], FilterNonCausal, SamplingFrequency, params[ FilterParamFreqCut ] );
else if ( filtertype == FilterTypeBandPass              )   filter  = new TFilterButterworthBandPass<double> ( params[ FilterParamOrder ], FilterNonCausal, SamplingFrequency, params[ FilterParamFreqCutMin ], params[ FilterParamFreqCutMax ] );
else if ( filtertype == FilterTypeBandStop              )   filter  = new TFilterButterworthBandStop<double> ( params[ FilterParamOrder ], FilterNonCausal, SamplingFrequency, params[ FilterParamFreqCutMin ], params[ FilterParamFreqCutMax ] );
else if ( filtertype == FilterTypeEnvelopeSlidingWindow )   filter  = new TFilterEnvelope<double> ( FilterTypeEnvelopeSlidingWindow, SamplingFrequency, params[ FilterParamEnvelopeWidth ] );
else if ( filtertype == FilterTypeEnvelopeGapBridging   )   filter  = new TFilterEnvelope<double> ( FilterTypeEnvelopeGapBridging,   SamplingFrequency, params[ FilterParamEnvelopeWidth ] );
else if ( filtertype == FilterTypeEnvelopeAnalytic      )   filter  = new TFilterEnvelope<double> ( FilterTypeEnvelopeAnalytic,      SamplingFrequency, params[ FilterParamEnvelopeWidth ] );

                                        // data is not ordered for time filters, load every line at a time to perform the filtering...
for ( int i = 0; i < GetDimension (); i++ ) {

    line.GetRow ( *this, i );

    if ( filter )   filter->Apply ( line.GetArray (), line.GetDim () );
    else            line.Filter ( filtertype, params, showprogress );

    line.SetRow ( *this, i );
    }


if ( filter )
    delete  filter;
}


//----------------------------------------------------------------------------
                                        // Returns the regularization actually being used
RegularizationType  TMaps::ComputeESI   (   const TInverseMatrixDoc*    ISDoc, 
                                            RegularizationType          regularization,     bool        vectorial, 
                                            TMaps&                      ESI 
                                        )   const
{
//ESI.Reset ();
//ESI.DeallocateMemory ();

if ( ISDoc == 0 )
    return  regularization;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // this is legal
ESI.SetSamplingFrequency ( SamplingFrequency );

                                        // allocate ESI results - !results type, not inverse type!
int                 numsp           = ISDoc->GetNumSolPoints ();

ESI.Resize ( NumMaps, numsp * ( vectorial ? 3 : 1 ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Note that  RegularizationAutoLocal  will not change regularization
if ( regularization == RegularizationAutoGlobal ) {
                                        // this is usually called from a resampled of the original dataset
                                        // but it still might be too big, so we restrict even further down to 1000 samples
    TDownsampling       downtf ( NumMaps, DownsamplingTargetSizeReg );

//  DBGV ( downtf.NumDownData, "Global Regularization" );

    regularization  = (RegularizationType) ISDoc->GetGlobalRegularization ( this, 0, downtf.From, downtf.To, downtf.Step );

//  DBGV ( regularization, "Global Regularization" );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numel           = ISDoc->GetNumElectrodes();
//TArray1<TVector3Float>  vinv ( vectorial ? numsp : 0     );

                                        // multiply and save in big buffer - also converts to scalar at the same time
for ( int nc = 0; nc < NumMaps; nc++ )
                                        // dispatch according to results                proper way, using fast transfer
//  if ( vectorial ) {  ISDoc->MultiplyMatrix ( regularization, Maps[ nc ], vinv );     ESI[ nc ].CopyMemoryFrom ( vinv.GetMemoryAddress () ); }
//  else                ISDoc->MultiplyMatrix ( regularization, Maps[ nc ], ESI[ nc ] );

                                        // dispatch according to results    a bit hacky but skips any transfer, as we do have interlaced X,Y,Z structure - also MultiplyMatrix only access the data array
    if ( vectorial )    ISDoc->MultiplyMatrix ( regularization, Maps[ nc ], *(TArray1<TVector3Float>*) &ESI[ nc ] );
    else                ISDoc->MultiplyMatrix ( regularization, Maps[ nc ], ESI[ nc ] );

                                        // the one that has been used
return  regularization;
}


//----------------------------------------------------------------------------
bool    TMaps::Covariance3DVectorial(   AMatrix33&          Cov,   const TSelection*   tfok
                                    )   const
{
if ( IsNotAllocated () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numdim          = Dimension;
int                 numsamples      = NumMaps;

                                        // only working on 3D vectorial covariance
if ( numdim != 3 )
    return  false;
                                        // not enough data?
//if ( dim2 < dim1 )
//    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we don't need to center, as we deal with vector components, which are all relative to 0 already
/*
TEasyStats              ref;
TArray1<TMapAtomType>   center ( numdim );


for ( int i  = 0; i  < numdim; i++  ) {

    ref.Reset ();

    for ( int tf = 0; tf < numsamples; tf++ )

        ref.Add ( (*allmaps[ tf ])[ i ], ThreadSafetyIgnore );

    center[ i ] = ref.Mean ();
    }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//Cov.resize ( numdim, numdim );
Cov.AReset ();

                                        // fill covariance matrix
for ( int i  = 0; i  < numdim; i++  )
for ( int j  = 0; j  <= i;     j++  )

    for ( int tf = 0; tf < numsamples; tf++ )

//      Cov ( i, j )   += ( Maps[ tf ][ i ] - center[ i ] ) * ( Maps[ tf ][ j ] - center[ j ] );
        Cov ( i, j )   += Maps[ tf ][ i ] * Maps[ tf ][ j ];


Cov    /= NonNull ( numsamples - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  true;
}


//----------------------------------------------------------------------------

void    TMaps::Multiply ( const AMatrix& T, TMaps& results )    const
{
int                 numrows         = T.n_rows;
int                 numcols         = T.n_cols;


if ( numcols != Dimension ) {
    results.DeallocateMemory ();
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

results.Resize ( NumMaps, numrows );
results.SetSamplingFrequency ( SamplingFrequency );

                                        // explicit summation - faster as it skips all the memory transfers
long double         s;
TMap*               map;


for ( int tf = 0; tf < NumMaps; tf++ ) {

    map     = &Maps[ tf ];

    for ( int i  = 0; i  < numrows; i++  ) {

        s   = 0;
                                        // transfer data
        for ( int j  = 0; j  < Dimension; j++  )

            s   += T ( i, j ) * (*map)[ j ];

        results ( tf, i )   = s;

        } // for i

    } // for tf

}


//----------------------------------------------------------------------------
                                        // Standardizing tracks the usual way
void    TMaps::ComputeZScoreSigned  (   ZScoreType  how,    TArray2<float>&     zscorevalues )  const
{
if ( IsNotAllocated () )
    return;


if ( ! IsZScoreSigned ( how ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                scanmaxes       = how & ZScoreMaxData;
int                 dimension       = Dimension;
TEasyStats          stat ( scanmaxes ? NumMaps / 2 : NumMaps );
double              center;
double              sd;


zscorevalues.Resize ( NumZValuesCalibration, dimension );

                                        // ZScoreSigned_CenterScale is the only case here

for ( int e = 0; e < dimension; e++ ) {

    UpdateApplication;


    stat.Reset ();

    if ( scanmaxes ) {

        double              e0;
        double              ep;
        double              en;

        for ( int nc = 1; nc < NumMaps - 1; nc++ ) {

            e0  = Maps[ nc     ][ e ];
            ep  = Maps[ nc - 1 ][ e ];
            en  = Maps[ nc + 1 ][ e ];
                                        // stats on local, positive or negative, maxes only
            if ( e0 > 0 && e0 > ep && e0 > en
              || e0 < 0 && e0 < ep && e0 < en )

                stat.Add ( e0, ThreadSafetyIgnore );
            }
        } // scanmaxes
    else

        for ( int nc = 0; nc < NumMaps; nc++ )

            stat.Add ( Maps[ nc ][ e ], ThreadSafetyIgnore );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // extract variables for transformation
                                        
//  center      = stat.Mean ();                             // regular Z-Score
    center      = min ( stat.MaxModeHSM (), stat.Median () );                   // HSM better for 1 mode (Older HSM was pretty good if data are not skewed)
//  center      = stat.Randomize ( TEasyStatsFunctionMaxModeHSM, 100, 0.10 );   // not improving things


//  sd          = stat.SD   ();
//  sd          = stat.InterQuartileRange ();
//  sd          = ( stat.Qn ( 500 ) + stat.MAD () + stat.InterQuartileRange () ) / 3;
    sd          = ( stat.MAD () + stat.InterQuartileRange () ) / 2;


    zscorevalues ( NumZValuesCenter , e )  = center;
    zscorevalues ( NumZValuesSpread , e )  = NonNull ( sd );
        
    } // for dimension

}


void    TMaps::ApplyZScoreSigned    (   ZScoreType  how,    const TArray2<float>&   zscorevalues    )
{
if ( IsNotAllocated () )
    return;


if ( ! IsZScoreSigned ( how ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 dimension       = Dimension;

if ( ! ( zscorevalues.GetDim1 () == NumZValuesCalibration && zscorevalues.GetDim2 () == dimension ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // ZScoreSigned_CenterScale is the only case here
OmpParallelFor

for ( int nc = 0; nc < NumMaps; nc++ )
for ( int e = 0; e < dimension; e++ ) {

    UpdateApplication;


    double          center      = zscorevalues ( NumZValuesCenter , e );
    double          sd          = zscorevalues ( NumZValuesSpread , e );

                                        // apply transformation
    Maps[ nc ][ e ]     = crtl::ZScore ( Maps[ nc ][ e ], center, sd );

    } // for dimension / TF

}


//----------------------------------------------------------------------------
                                        // Standardizing the norm of 3D vectors (scalar, positive data)
void    TMaps::ComputeZScorePositive    (   ZScoreType  how,    TArray2<float>&     zscorevalues    )   const
{
if ( IsNotAllocated () )
    return;


if ( ! IsZScorePositive ( how ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                scanmaxes       = how & ZScoreMaxData;
int                 dimensionsp     = Dimension;
TEasyStats          stat    ( scanmaxes ? NumMaps / 2 : NumMaps );
double              norm;
TArray1<double>     normal  ( NumMaps );
double              center;
double              sd;
TRandUniform        randunif;

TResampling         resampling;
#if defined (_DEBUG)
int                 numresampling       = 1;
#else
int                 numresampling       = TMapsNumResampling;
#endif
TEasyStats          statcenter ( NumMaxModeRobustEstimates * numresampling );   // using 4 estimators for the center
TEasyStats          statsd     ( 1 * numresampling );   // using only 1 estimator for the spreading
TEasyStats          substats;
TVector<int>        randindex;
                                        // caller can be specific about the dimensions, it could be 6 in case of vectorial ris from complex data
                                        // otherwise assumes dimensions are 3 (though better if specified by the caller)
double              (*vectornormtonormal) ( double )    = how & ZScoreDimension6 ? Vector6NormToNormal : Vector3NormToNormal;


zscorevalues.Resize ( NumZValuesCalibration, dimensionsp );

                                        // work on the norm of vector
for ( int e = 0; e < dimensionsp; e++ ) {

    UpdateApplication;

                                        // scan ALL data, we need the norm and normal values on everything
    for ( int nc = 0; nc < NumMaps; nc++ ) {
                                        // data is already the norm
        norm            = Maps[ nc ][ e ];
                                        // Deskew the data, converting norm to Normal
        normal ( nc )   = vectornormtonormal ( norm );
        } // for map


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    stat.Reset ();
                                        // stats however can be restricted
    if ( scanmaxes ) {

        double              e0;
        double              ep;
        double              en;

        for ( int nc = 1; nc < NumMaps - 1; nc++ ) {

            e0  = normal ( nc     );
            ep  = normal ( nc - 1 );
            en  = normal ( nc + 1 );
                                        // stats on local max only
            if ( e0 > ep && e0 > en )

                stat.Add ( e0, ThreadSafetyIgnore );
            }
        } // scanmaxes
    else

        stat.Set ( normal, true );      // allocate if using Robust stats


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // extract variables for transformation

    if ( how & ZScorePositive_NocenterScale ) {
                                        // This is the only special case: Z-score centered on 0, computed from Sum of squares
        center      = 0;
        sd          = sqrt ( stat.Sum2 () / NonNull ( stat.GetNumItems () - 1 ) );
        }
    else {
                                        // most of the cases
                                        
//      center      = stat.Mean ();                 // regular Z-Score
//      center      = min ( stat.MaxModeHRM (), stat.MaxModeHistogram () );

                                        // getting the optimal resampling size - could be done per electrode
        if ( e == 0 || scanmaxes ) {

            resampling.SetNumData       ( stat.GetNumItems () );

            resampling.SetNumResampling ( numresampling );

            resampling.GetSampleSize    ( TMapsResamplingCoverage, TMapsMinSampleSize, Round ( resampling.NumData * TMapsMaxSampleSizeRatio ) );
            }


        statcenter.Reset ();
                                        // resampling & using for different stats at the same time
        for ( int i = 0; i < numresampling; i++ ) {

            stat.Resample ( substats, resampling.SampleSize, randindex, &randunif );
                                        // cumulate estimates
            substats.MaxModeRobust ( statcenter );
            }

        center      = statcenter.Median ( false );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//      sd          = stat.SD   ();                 // regular Z-Score
//      sd          = stat.InterQuartileRange ();   // quite OK
//      sd          = center / 3;                   // spreading is proportional to center
//      double  halfiqr     = ( stat.Quantile ( 0.50 ) - stat.Quantile ( 0.25 ) ) * IQRToSigma * 2; // instead of using the full IQR, use the lower half
//      double  halfiqmode  = ( center - stat.Quantile ( 0.25 ) ) * IQRToSigma * 2;                 // same, but middle point is the mode


        statsd .Reset ();
        double              madleft;
//      double              madright;
                                        // resampling & using for different stats at the same time
        for ( int i = 0; i < numresampling; i++ ) {

            stat.Resample ( substats, resampling.SampleSize, randindex, &randunif );

                                        // optimal on the left part of center - the background activity should be only here
            substats.MADLeft    ( center, madleft );
            statsd.Add          ( madleft, ThreadSafetyIgnore );

                                        // optimal on both the left and right parts of center - in between value, takes into account both background and high activities
//          statsd.Add          ( substats.InterQuartileRange () );

                                        // optimal on the right part of center - takes the higher values into account, i.e. can minimize constant high activities
//          substats.MADRight   ( center, madright );
//          statsd.Add          ( madright );
            }

        sd          = statsd.Median ( false );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // re-compute with a restricted dataset
        stat.Reset ();

        for ( int nc = 0; nc < NumMaps; nc++ )
                                        // put only data reasonably close to center - including on the right
            if ( normal ( nc ) < 2 * center )

                stat.Add ( normal ( nc ) );



        statsd .Reset ();
                                        // resampling & using for different stats at the same time
        for ( int i = 0; i < numresampling; i++ ) {

            stat.Resample ( substats, resampling.SampleSize, randindex, &randunif );
                                        // symmetrical estimates
            double      s1      = substats.SD   ();
            double      s2      = substats.MAD  ( center );
            double      s3      = substats.Sn   ( 500 );    // costly

            statsd.Add          ( s1 );
            statsd.Add          ( s2 );
            statsd.Add          ( s3 );
            }
        sd          = statsd .Median ( false );
*/
        }


    zscorevalues ( NumZValuesCenter , e )  = center;
    zscorevalues ( NumZValuesSpread , e )  = NonNull ( sd );

    } // for dimension

}


void    TMaps::ApplyZScorePositive  (   ZScoreType  how,    const TArray2<float>&   zscorevalues    )
{
if ( IsNotAllocated () )
    return;


if ( ! IsZScorePositive ( how ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 dimensionsp     = Dimension;

if ( ! ( zscorevalues.GetDim1 () == NumZValuesCalibration && zscorevalues.GetDim2 () == dimensionsp ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // work on the norm of vector

                                        // caller can be specific about the dimensions, it could be 6 in case of vectorial ris from complex data
                                        // otherwise assumes dimensions are 3 (though better if specified by the caller)
double              (*vectornormtonormal) ( double )    = how & ZScoreDimension6 ? Vector6NormToNormal : Vector3NormToNormal;


OmpParallelFor

for ( int nc = 0; nc < NumMaps; nc++ )
for ( int e = 0; e < dimensionsp; e++ ) {

    UpdateApplication;


    double      center      = zscorevalues ( NumZValuesCenter , e );
    double      sd          = zscorevalues ( NumZValuesSpread , e );

                                        // data is already the norm
    double      norm        = Maps[ nc ][ e ];
                                        // Deskew the data, converting norm to Normal
    double      normal      = vectornormtonormal ( norm );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if      ( how & ZScorePositive_CenterScale )
                                    // data is Normal, i.e. signed
        Maps[ nc ][ e ]     = crtl::ZScore ( normal, center, sd );


//  else if ( how & ZScorePositive_CenterScaleInvert )
//                                      // data is Normal, i.e. signed
//      Maps[ nc ][ e ]     = - crtl::ZScore ( normal, center, sd );


    else if ( how & ZScorePositive_CenterScaleAbs )
                                        // data is positive
        Maps[ nc ][ e ]     = fabs ( crtl::ZScore ( normal, center, sd ) );            // assumes activated <=> deactivated, looking for outlier regions


    else if ( how & ZScorePositive_CenterScalePlus )
                                        // data is positive
        Maps[ nc ][ e ]     = crtl::AtLeast ( 0.0, crtl::ZScore ( normal, center, sd ) );    // keeps only significant (above noise) responses


    else {
            
        if ( norm == 0 )
            continue;

        double          n       = 0;
        double          r;
            
        if      ( how & ZScorePositive_CenterScaleOffset ) {
                                        // compute a new norm
            n   = crtl::ZScore ( normal, center, sd );
                                        // shift n SD to the "right", keep max at 1, and prevent negative norm
            ZSignedToZPositive ( n );
            }

        else if ( how & ZScorePositive_CenterScaleInvertOffset ) {
                                        // compute a new norm
            n   = - crtl::ZScore ( normal, center, sd );
                                        // shift n SD to the "right", keep max at 1, and prevent negative norm
            ZSignedToZPositive ( n );
            }

        else if ( how & ZScorePositive_NocenterScale ) {
                                        // new norm computed from a Z-Score equivalent, without center
            n   = normal / sd;
            }

                                        // back to skewed distribution? - divide by sqrt(3) to center the mode back to 1
//      n   = NormalToVector3Norm ( n ); //  / sqrt ( 3.0 );
                                        // compute final ratio, from norm to z-scored norm
        r   = n / NonNull ( norm );

        Maps[ nc ][ e ]    *= r;
        }

    } // for dimension / TF
}


//----------------------------------------------------------------------------
                                        // Standardizing the norm of 3D vectorial data
void    TMaps::ComputeZScoreVectorial   (   ZScoreType  how,    TArray2<float>&   zscorevalues ) const
{
if ( IsNotAllocated () )
    return;


if ( ! IsZScoreVectorial ( how ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if      ( how & ZScoreVectorial_CenterScaleByComponent ) {
                                        // simply apply regular Z-Score to X,Y,Z components independently
    ComputeZScoreSigned ( SetZScore ( ZScoreSigned_CenterScale, how ), zscorevalues );

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                scanmaxes       = how & ZScoreMaxData;
int                 dimensionsp     = Dimension / 3;
TEasyStats          stat    ( scanmaxes ? NumMaps / 2 : NumMaps );
double              norm;
TArray1<double>     normal ( NumMaps );
double              center;
double              sd;
TRandUniform        randunif;

TResampling         resampling;
#if defined (_DEBUG)
int                 numresampling       = 1;
#else
int                 numresampling       = TMapsNumResampling;
#endif
TEasyStats          statcenter ( NumMaxModeRobustEstimates * numresampling );
TEasyStats          statsd     ( 1 * numresampling );
TEasyStats          substats;
TVector<int>        randindex;


//if ( how & ( ZScoreVectorial_CenterVectors_CenterScale 
//           | ZScoreVectorial_CenterVectors_Scale       ) )
                                        // Center to mean vector, first step of Vectorial Z-Score
    //TimeCentering ();                 // !to be done!


zscorevalues.Resize ( NumZValuesCalibration, dimensionsp );

                                        // work on the norm of vector
for ( int e1 = 0, e3 = 0; e1 < dimensionsp; e1++, e3 += 3 ) {

    UpdateApplication;

                                        // Once centered, we look for the spreading, but norm is skewed, so un-skew first
    for ( int nc = 0; nc < NumMaps; nc++ ) {
                                        // recover & Deskew the norms of the vectors
        norm            = sqrt ( Square ( Maps[ nc ][ e3     ] )      
                               + Square ( Maps[ nc ][ e3 + 1 ] )
                               + Square ( Maps[ nc ][ e3 + 2 ] ) );

        normal ( nc )   = Vector3NormToNormal ( norm );
        } // for map


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    stat.Reset ();
                                        // stats however can be restricted
    if ( scanmaxes ) {

        double              e0;
        double              ep;
        double              en;

        for ( int nc = 1; nc < NumMaps - 1; nc++ ) {

            e0  = normal ( nc     );
            ep  = normal ( nc - 1 );
            en  = normal ( nc + 1 );
                                        // stats on local max only
            if ( e0 > ep && e0 > en )

                stat.Add ( e0, ThreadSafetyIgnore );
            }
        } // scanmaxes
    else

        stat.Set ( normal, true );      // allocate if using Robust stats


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    center  = 0;
    sd      = 0;

                                        // extract variables for transformation
    if      ( how & ZScoreVectorial_CenterVectors_CenterScale ) {
                                        // regular Z-Score
//      center      = stat.Mean ();
//      center      = stat.Median ();               // seems the best in robust stats
//      center      = min ( stat.MaxModeHistogram (), stat.Median () );
//      sd          = stat.SD   ();
//      sd          = stat.InterQuartileRange ();

                                        // getting the optimal resampling size - could be done per electrode
        if ( e1 == 0 || scanmaxes ) {

            resampling.SetNumData       ( stat.GetNumItems () );

            resampling.SetNumResampling ( numresampling );

            resampling.GetSampleSize    ( TMapsResamplingCoverage, TMapsMinSampleSize, Round ( resampling.NumData * TMapsMaxSampleSizeRatio ) );
            }


        statcenter.Reset ();
                                        // resampling & using for different stats at the same time
        for ( int i = 0; i < numresampling; i++ ) {

            stat.Resample ( substats, resampling.SampleSize, randindex, &randunif );
                                        // cumulate estimates
            substats.MaxModeRobust ( statcenter );
            }

        center      = statcenter.Median ( false );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        statsd .Reset ();
        double              madleft;
//      double              madright;
                                        // resampling & using for different stats at the same time
        for ( int i = 0; i < numresampling; i++ ) {

            stat.Resample ( substats, resampling.SampleSize, randindex, &randunif );

                                        // optimal on the left part of center - the background activity should be only here
            substats.MADLeft    ( center, madleft );
            statsd.Add          ( madleft, ThreadSafetyIgnore );

                                        // optimal on both the left and right parts of center - in between value, takes into account both background and high activities
//          statsd.Add          ( substats.InterQuartileRange () );

                                        // optimal on the right part of center - takes the higher values into account, i.e. can minimize constant high activities
//          substats.MADRight   ( center, madright );
//          statsd.Add          ( madright );
            }

        sd          = statsd .Median ( false );

        }
    else if ( how & ZScoreVectorial_CenterVectors_Scale ) {
                                        // Z-score centered on 0, computed from Sum of squares
//      center      = 0;
        sd          = sqrt ( stat.Sum2 () / NonNull ( stat.GetNumItems () - 1 ) );
        }


    zscorevalues ( NumZValuesCenter , e1 )     = center;
    zscorevalues ( NumZValuesSpread , e1 )     = NonNull ( sd );

    } // for dimension

}


//----------------------------------------------------------------------------

void    TMaps::ApplyZScoreVectorial (   ZScoreType  how,    const TArray2<float>&   zscorevalues    )
{
if ( IsNotAllocated () )
    return;


if ( ! IsZScoreVectorial ( how ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if      ( how & ZScoreVectorial_CenterScaleByComponent ) {
                                        // simply apply regular Z-Score to X,Y,Z components independently
    ApplyZScoreSigned ( SetZScore ( ZScoreSigned_CenterScale, how ), zscorevalues );

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 dimensionsp     = Dimension / 3;

if ( ! ( zscorevalues.GetDim1 () == NumZValuesCalibration && zscorevalues.GetDim2 () == dimensionsp ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // work on the norm of vector

//if ( how & ( ZScoreVectorial_CenterVectors_CenterScale 
//           | ZScoreVectorial_CenterVectors_Scale       ) )
                                        // Center to mean vector, first step of Vectorial Z-Score
    //TimeCentering ();                 // !to be done!


OmpParallelFor

for ( int nc = 0; nc < NumMaps; nc++ )
for ( int e1 = 0, e3 = 0; e1 < dimensionsp; e1++, e3 += 3 ) {

    UpdateApplication;


    double      center      = zscorevalues ( NumZValuesCenter , e1 );
    double      sd          = zscorevalues ( NumZValuesSpread , e1 );

                                        // recover & Deskew the norms of the vectors
    double      norm        = sqrt ( Square ( Maps[ nc ][ e3     ] )      
                                   + Square ( Maps[ nc ][ e3 + 1 ] )
                                   + Square ( Maps[ nc ][ e3 + 2 ] ) );

    if ( norm == 0 )
        continue;


    double      normal      = Vector3NormToNormal ( norm );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    double      n       = 0;
    double      r;

    if      ( how & ZScoreVectorial_CenterVectors_CenterScale ) {
                                        // compute a new norm
        n   = crtl::ZScore ( normal, center, sd );
                                        // shift n SD to the "right", keep max at 1, and prevent negative norm
        ZSignedToZPositive ( n );
        }
    else if ( how & ZScoreVectorial_CenterVectors_Scale ) {
                                        // new norm computed from a Z-Score equivalent, without center
        n   = normal / sd;
        }

                                        // Re-skew the data, as we are working on vectors'components later(?)
                                        // Actually: don't - It is better to keep the norm of the final vector equivalent to the Z-Score on positive data
//  n   = NormalToVector3Norm ( n );

                                        // compute final ratio, from norm to z-scored norm
    r   = n / NonNull ( norm );

                                        // scale whole vector, keeping original direction
    Maps[ nc ][ e3     ]   *= r;
    Maps[ nc ][ e3 + 1 ]   *= r;
    Maps[ nc ][ e3 + 2 ]   *= r;

    } // for dimension / TF

}


//----------------------------------------------------------------------------
void    TMaps::ComputeZScore ( ZScoreType how, TArray2<float>& zscorevalues )   const
{
if ( ! IsZScore ( how ) )
    return;

//DBGM ( "Start", "ComputeZScore" );

if      ( IsZScoreVectorial ( how ) )   ComputeZScoreVectorial ( how, zscorevalues );
else if ( IsZScorePositive  ( how ) )   ComputeZScorePositive  ( how, zscorevalues );
else if ( IsZScoreSigned    ( how ) )   ComputeZScoreSigned    ( how, zscorevalues );

//DBGM ( "Finished", "ComputeZScore" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
                                        // Running tests for size
std::set<int>       resampling  = { 50, 100, 500, 1000, 2500, 5000, 7500, 10000, 15000 };

for ( auto seti = resampling.begin (); seti != resampling.end (); seti++ ) {

    int                 targetsize      = *seti;

    if ( targetsize > NumMaps )
        continue;


    TMaps               submaps  ( targetsize, Dimension );

    for ( int mi = 0; mi < targetsize; mi++ )
                                        // linear pick
        submaps [ mi ]  = Maps[ (int) ( (double) mi / ( targetsize - 1 ) * ( NumMaps - 1 ) ) ];


//    THistogram (  )


    TArray2<float>  zscorevalues;       // !local!

    submaps.ComputeZScorePositive  ( how, zscorevalues );


    TFileName           _file;
    StringCopy  ( _file, "E:\\Data\\ZScoreFactors.", IntegerToString ( targetsize, 4 ), ".sef" );

    zscorevalues.WriteFile ( _file );
    }
exit ( 0 );
*/
}


//----------------------------------------------------------------------------
void    TMaps::ApplyZScore ( ZScoreType how, const TArray2<float>& zscorevalues )
{
if ( ! IsZScore ( how ) )
    return;

if      ( IsZScoreVectorial ( how ) )   ApplyZScoreVectorial ( how, zscorevalues );
else if ( IsZScorePositive  ( how ) )   ApplyZScorePositive  ( how, zscorevalues );
else if ( IsZScoreSigned    ( how ) )   ApplyZScoreSigned    ( how, zscorevalues );
}


//----------------------------------------------------------------------------
                                        // Conveniently wrapping the two processing into 1
void    TMaps::ZScore ( ZScoreType how, TArray2<float>* tozscorevalues )
{
TArray2<float>      zscorevalues;

ComputeZScore      ( how, zscorevalues );

ApplyZScore        ( how, zscorevalues );

if ( tozscorevalues )
    *tozscorevalues = zscorevalues;
}


//----------------------------------------------------------------------------
                                        // Test if the data has actually been shifted or not
bool    TMaps::IsZScorePositiveShifted ()   const
{
                                        // do a rough scan of the data
TEasyStats          stat ( *this, true, 500000 );


bool                iszpositive     = stat.Average () >  0.05                   // No Z-Score means very small values, like 1e-3..1e-4
                                   && stat.Min ()     >= 0                      // Z-Score but without shift is negative
                                   && stat.SNR ()     >  ( 1.1 + 2.5 ) / 2      // !Strongest criterion! SD is closer to Mean for non-Z-Score, while Z-Score shifted have a SD closer to 1/3 of Mean
                                   && stat.Average () >  ( 0.6 + 1.1 ) / 2;     // Z-Score Non-shifted values are closer to 0.6, shifted closer to 1.1

//char                buff[ 256 ];
//stat.Show ( StringCopy ( buff, "IsZScorePositiveShifted: ", BoolToString ( iszpositive ) ) );

return  iszpositive;
}


//----------------------------------------------------------------------------
                                        // Converts ZScorePO to regular ZScore
                                        // !Background SHOULD be 1 - otherwise use ZPositiveToZSignedAuto!
void    TMaps::ZPositiveToZSigned ()
{
                                        // Take some caution - scan the data first to evaluate if this is a proper case for correction
if ( ! IsZScorePositiveShifted () )
    return;

OmpParallelFor

for ( int nc  = 0; nc  < NumMaps;   nc++  )
for ( int dim = 0; dim < Dimension; dim++ )

    Maps[ nc ][ dim ]   = crtl::ZPositiveToZSigned ( Maps[ nc ][ dim ] );
}

                                        // Converts to ZScore, but estimate the actual max mode
                                        // Averaging vectorial Z-Scored data will NOT give Z-Score distributed data, due to the vectorial cancellation
                                        // so we need to estimate the new max mode. But we only need this to be done with all tracks together.
void    TMaps::ZPositiveToZSignedAuto ()
{
                                        // global recentering, across all SPs, all time points, to recover the actual max mode after averaging
TEasyStats          stats ( *this, true, 500000 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Robust estimate of max mode
TEasyStats          statcenter ( NumMaxModeRobustEstimates );

stats.MaxModeRobust ( statcenter );

//stats     .Show ( "ZPositiveToZSignedAuto Data" );
//statcenter.Show ( "ZPositiveToZSignedAuto Center" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Robust estimate for center
double              center          = statcenter.Median ( false );
                                        // center will also be used as scaling
double              spread          = NonNull ( center );

OmpParallelFor

for ( int nc  = 0; nc  < NumMaps;   nc++  )
for ( int dim = 0; dim < Dimension; dim++ )
                                                                   // Our Z-Scoring is using 3 SD between 0 and 1
    Maps[ nc ][ dim ]   = ( Maps[ nc ][ dim ] - center ) / spread; // * MinSDToKeep;
}


void    TMaps::ZPositiveAuto ()
{
                                        // Automatic re-centering, signed results
ZPositiveToZSignedAuto ();
                                        // re-shift to 1
(*this)    += 1;

//AtLeast ( 0 );
}


//----------------------------------------------------------------------------
void    TMaps::Absolute ()
{
OmpParallelFor

for ( int nc = 0; nc < NumMaps; nc++ )

    Maps[ nc ].Absolute ();
}


//----------------------------------------------------------------------------
void    TMaps::Clipped  ( TMapAtomType minv, TMapAtomType maxv )
{
OmpParallelFor

for ( int nc = 0; nc < NumMaps; nc++ )

    Maps[ nc ].Clipped ( minv, maxv );
}


void    TMaps::AtLeast ( TMapAtomType minv )
{
OmpParallelFor

for ( int nc = 0; nc < NumMaps; nc++ )

    Maps[ nc ].AtLeast ( minv );
}


void    TMaps::NoMore  ( TMapAtomType maxv )
{
OmpParallelFor

for ( int nc = 0; nc < NumMaps; nc++ )

    Maps[ nc ].NoMore ( maxv );
}


//----------------------------------------------------------------------------
void    TMaps::AlignSuccessivePolarities ()
{
if ( NumMaps <= 1 )
    return;


for ( int nc = 1; nc < NumMaps; nc++ )

    if ( Maps[ nc ].IsOppositeDirection ( Maps[ nc - 1 ] ) )

         Maps[ nc ].Invert ();
}


//----------------------------------------------------------------------------
                                        // Method always generate maps with correlation >= 0
                                        // The ignorepolarity flag, if set, just post-process the maps and randomly invert some of them
                                        // so we have some +corr and -corr correlation values.
                                        // correlationmin and correlationmax are given >= 0 for the moment
bool    TMaps::MapsFromLeadField    ( int           nummaps,            
                                      double        correlationmin,     double          correlationmax,
                                      bool          ignorepolarity,
                                      TLeadField&   leadfield,          TTracks<float>& K,              int         numsources, 
                                      TMaps*        sourcemaps )
{
if ( ! ( leadfield.IsOpen () && K.IsAllocated () ) )
    return  false;

                                        // Go through a dedicated class
int                 numel           = leadfield.GetNumElectrodes     ();
int                 numsolp         = leadfield.GetNumSolutionPoints ();    // well, should check for scalar type (no orientation) when implemented
int                 numsolp3        = 3 * numsolp;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // store a new collection of maps - all sizes are known here
Resize ( nummaps, numel );

if ( sourcemaps )
    sourcemaps->Resize ( nummaps, numsolp3 );


TEasyStats          stat;


for ( int mi = 0; mi < nummaps; mi++ ) {
                                        // get any random maps first
    Maps[ mi ].MapFromLeadField ( K, numsources, 0, 0, 0,
                                  true, true, sourcemaps ? &(*sourcemaps)[ mi ] : 0 );


    stat.Set    ( &Maps[ mi ], false );
                                        // outliers are not looking nice, re-do this map?
    if ( stat.ZScore ( stat.AbsoluteMax () ) > 2 )
        mi--;
    }

//WriteFile ( "E:\\Data\\MapsFromLeadField.1.Random.sef" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // now adjust correlation levels
TMap                mapi      ( numel    );
TMap                mapj      ( numel    );
TMap                sourcemap ( numsolp3 );
TMap                sourcemapi( numsolp3 );
TMap                sourcemapj( numsolp3 );
double              wi;
double              wj;
static TRandUniform randunif;           // this one is static, because reloading at the millisecond can repeat the seame seed in case of multiple close calls
static TRandCoin    randcoin;           // this one is static, because reloading at the millisecond can repeat the seame seed in case of multiple close calls
double              corr;

                                        // repeat process as many times as there are maps
for ( int repeati = 0; repeati < nummaps; repeati++ )

for ( int mi = 0; mi < nummaps; mi++ ) {
                                        // here, mapi is already centered and normalized
    mapi    = Maps[ mi ];

    if ( sourcemaps )
        sourcemapi   = (*sourcemaps)[ mi ];


    for ( int mj = mi + 1; mj < nummaps; mj++ ) {
                                        // here, mapj is already centered and normalized
        mapj        = Maps[ mj ];
                                        // remove contribution of mapi to mapj, so mapj is orthogonal to mapi
        mapj       -= mapi * mapi.ScalarProduct ( mapj );
                                        // force average reference, shouldn't be needed (?)
        mapj.Normalize ( true );        


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute weights to re-combine the now 2 orthogonal vectors mapi and mapj to attain the requested correlation
                                        // !Formula doesn't seem to work with negative correlation - prevent theses cases for the moment!

                                        // get some correlation level, either fixed or random
        corr        = correlationmin == correlationmax ? correlationmin
                                                       : randunif ( correlationmin, correlationmax ); 
                                        // the correlation is exactly the scalar product / projection on mapj to mapi (both are normalized)
                                        // we could also use wi = -corr here to have a negative correlation, wj would be the same due to the squaring
                                        // which would produce a different map as if we just invert the whole final map (-wi and -wj in that case)
        wi          =                     corr;
                                        // trigonometry to get the other projected part - this formula will force the results to be positively correlated
        wj          = sqrt ( 1 - Square ( corr ) ); 
                                        // combine to get the requested correlation
        mapj        = ( mapj * wj ) + ( mapi * wi );
                                        // not really needed, just for safety
        mapj.Normalize ( true );        

                                        // and... that's it
        Maps[ mj ] = mapj;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if ( sourcemaps ) {
            sourcemapj   = (*sourcemaps)[ mj ];
                                        // does this make any sense?
            sourcemapj  -= sourcemapi * mapi.ScalarProduct ( mapj );

            (*sourcemaps)[ mj ] = sourcemapj;
            }
        }

    }

//WriteFile ( "E:\\Data\\MapsFromLeadField.2.Orthogonal.sef" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // post-process the resulting maps, so we can have either +corr or -corr
if ( ignorepolarity )

    for ( int mi = 0; mi < nummaps; mi++ )

        if ( randcoin () )
                                        // so, polarity doesn't matter? lets flip some maps randomly...
            Maps[ mi ].Invert ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  true;
}


//----------------------------------------------------------------------------
bool    TMaps::RisFromSolutionPoints    (   int         nummaps, 
                                            double      correlationmin, double  correlationmax, 
                                            TPoints     solp,           int     numsources,
                                            double      spreadmax,      double  axisvariability
                                        )
{
if ( solp.IsEmpty () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numsolp         = solp.GetNumPoints ();
                                        // store a new collection of maps - all sizes are known here
Resize ( nummaps, numsolp );


for ( int mi = 0; mi < nummaps; mi++ )
                                        // get any random maps first
    Maps[ mi ].RisFromSolutionPoints ( solp, numsources, spreadmax, axisvariability, true );


//WriteFile ( "E:\\Data\\RisFromSolutionPoints.1.Random.ris" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // now adjust correlation levels
TMap                mapi      ( numsolp );
TMap                mapio     ( numsolp );
TMap                mapj      ( numsolp );
TMap                mapjo     ( numsolp );
double              wi;
double              wj;
static TRandUniform randunif;           // this one is static, because reloading at the millisecond can repeat the seame seed in case of multiple close calls
double              corr;

                                        // repeat process as many times as there are maps
for ( int repeati = 0; repeati < nummaps; repeati++ )

for ( int mi = 0; mi < nummaps; mi++ ) {
                                        // here, mapi is already normalized
    mapi    = Maps[ mi ];


    for ( int mj = mi + 1; mj < nummaps; mj++ ) {
                                        // here, mapj is already normalized
        mapj        = Maps[ mj ];
                                        // remove contribution of mapi to mapj, so mapj is orthogonal to mapi
                                        // problem here is that mapj has now negative parts, so the final mix will likely have them too, for low correlation
        mapj       -= mapi * mapi.ScalarProduct ( mapj );
                                        // force average reference, shouldn't be needed (?)
        mapj.Normalize ( false );        


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute weights to re-combine the now 2 orthogonal vectors mapi and mapj to attain the requested correlation
                                        // !Formula doesn't seem to work with negative correlation - prevent theses cases for the moment!

                                        // get some correlation level, either fixed or random
        corr        = correlationmin == correlationmax ? correlationmin
                                                       : randunif ( correlationmin, correlationmax ); 
                                        // the correlation is exactly the scalar product / projection on mapj to mapi (both are normalized)
                                        // we could also use wi = -corr here to have a negative correlation, wj would be the same due to the squaring
                                        // which would produce a different map as if we just invert the whole final map (-wi and -wj in that case)
        wi          =                     corr;
                                        // trigonometry to get the other projected part - this formula will force the results to be positively correlated
        wj          = sqrt ( 1 - Square ( corr ) ); 
                                        // combine to get the requested correlation
        mapj        = ( mapj * wj ) + ( mapi * wi );
                                        // not really needed, just for safety
        mapj.Normalize ( false );        

                                        // and... that's it
        Maps[ mj ] = mapj;

        }

    }

                                        // Make sure we have only positive data in the end, even if this interferes with the correlation
AtLeast ( 0 );

//WriteFile ( "E:\\Data\\RisFromSolutionPoints.2.Orthogonal.ris" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  true;
}


//----------------------------------------------------------------------------
                                        // Evaluate the SD from these maps
double  TMaps::EstimateSigmaData ()
{
TEasyStats          stat;

for ( int nc  = 0; nc  < NumMaps;   nc++  )
for ( int dim = 0; dim < Dimension; dim++ )

    stat.Add ( Maps[ nc ][ dim ], ThreadSafetyIgnore );

                                        // ?maybe if data is all > 0, use a non-centered SD?
return  stat.SD ();

/*
                                        // Vectorial estimate of the noise
                                        // Not sure if helpful / correct, as then applying noise to each x/y/z components will need to convert this vectorial SD to component SD
double  allsd = stat.SD ();

stat.Reset ();

for ( int nc  = 0; nc  < NumMaps;   nc++  )
for ( int dim = 0; dim < Dimension; dim+=3 )
                                        // directly summing the squared values
    stat.Add ( Square ( Maps[ nc ][ dim ] ) + Square ( Maps[ nc ][ dim + 1 ] ) + Square ( Maps[ nc ][ dim + 2 ] ), ThreadSafetyIgnore );

                                        // shortcut to SD: Sum is actually Sum2 here, and the spreading is computed from 0
double              sdvect          = sqrt ( stat.Sum () / NonNull ( stat.GetNumItems () - 1 ) );


DBGV4 ( NumMaps, Dimension, allsd, sdvect, "NumMaps, Dimension, allsd, avgsd" );

return  sdvect;
*/
}


void    TMaps::AddGaussianNoise ( double  sigmadata,  double    percentsnr )
{
if ( percentsnr <= 0 )
    return;


if ( sigmadata <= 0 ) {

    sigmadata   = EstimateSigmaData ();
                                        // no data, force to 1
    if ( sigmadata == 0 )
        sigmadata   = 1;
    }


for ( int mi = 0; mi < NumMaps; mi++ )

    Maps[ mi ].AddGaussianNoise ( sigmadata, percentsnr );
}

                                        // Range of noise
void    TMaps::AddGaussianNoise ( double  sigmadata,  double  percentsnr1,  double  percentsnr2 )
{
if ( sigmadata <= 0 ) {

    sigmadata   = EstimateSigmaData ();
                                        // no data, force to 1
    if ( sigmadata == 0 )
        sigmadata   = 1;
    }


TRandUniform        randunif;
double              noise;

for ( int mi = 0; mi < NumMaps; mi++ ) {
                                        // percentage of noise uniformly distributed
    noise   = randunif ( percentsnr1, percentsnr2 );

    Maps[ mi ].AddGaussianNoise ( sigmadata, noise );
    }
}


//----------------------------------------------------------------------------
                                        // Allocate and load maps, optionally forcing scalar/vectorial, average reference
                                        // Optional dimorders tells how to load the data, from a virtual EEG / FREQ point of view (definitely not the file structure!)
                                        // The reference order is  EEG[ el ][ tf ]  or  FREQ[ el ][ tf ][ f ]
                                        // Default is to load a time series of Maps
void    TMaps::ReadFile     (   const char*         filename,
                                AtomType            datatype,       ReferenceType       reference,
                                TStrings*           tracksnames,    TStrings*           freqsnames,
                                int                 dim1goes,       int                 dim2goes,       int             dim3goes,
                                int                 dimmargin 
                            )
{
                                        // retrieve file size, in time frames and electrodes
int                 numtf;
ReadFromHeader ( filename, ReadNumTimeFrames,         &numtf );


int                 numel;
ReadFromHeader ( filename, ReadNumElectrodes,         &numel );


int                 numfreq         = -1;
ReadFromHeader ( filename, ReadNumFrequencies,        &numfreq );


SamplingFrequency   = -1;
ReadFromHeader ( filename, ReadSamplingFrequency,     &SamplingFrequency );
Maxed ( SamplingFrequency, 0.0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set the maps' dimension  according to the order parameters
int                 dimension       = dim1goes == ReadGoMapsToDimension ? numel 
                                    : dim2goes == ReadGoMapsToDimension ? numtf 
                                    : dim3goes == ReadGoMapsToDimension ? numfreq 
                                                                        : numel;    // shouldn't happen

                                        // we can optionally clip the dimension of the map - usually time but it could be tracks
Maxed ( dimmargin, 0 );

                                        // check margin is legal, i.e. some data remain after clipping
if ( dimmargin > 0 && 2 * dimmargin >= dimension )
    dimmargin   = 0;

                                        // here margin is OK
if ( dimmargin > 0 )
    dimension  -= 2 * dimmargin;        // margin applied to dimension

                                        // so that any correlation / distance between maps will actually use all 3 components
int                 numrows         = dimension * ( datatype == AtomTypeVector ? 3 : 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set the number of maps according to the order parameters
int                 nummaps         = dim1goes == ReadGoMapsToNumMaps   ? numel 
                                    : dim2goes == ReadGoMapsToNumMaps   ? numtf 
                                    : dim3goes == ReadGoMapsToNumMaps   ? numfreq 
                                                                        : numtf;    // shouldn't happen

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we got the final sizes
Resize ( nummaps, numrows );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( numfreq > 0 ) {
                                        // frequency file case, caller has to specify where to send 2 dimensions + a fixed index for the remaining dimension
    TOpenDoc<TFreqDoc>  FreqDoc ( filename, OpenDocHidden );

    if ( tracksnames )
        *tracksnames    = *FreqDoc->GetElectrodesNames ();

    if ( freqsnames )
        *freqsnames     = *FreqDoc->GetFrequenciesNames ();


    int                 el;
    int                 tf;
    int                 f;
                                            // loop from destination
    for ( int mi = 0; mi < NumMaps;   mi++ )
    for ( int mj = 0; mj < Dimension; mj++ ) {

        el  = dim1goes == ReadGoMapsToNumMaps   ? mi
            : dim1goes == ReadGoMapsToDimension ? mj + dimmargin
                                                : dim1goes; // constant index

        tf  = dim2goes == ReadGoMapsToNumMaps   ? mi
            : dim2goes == ReadGoMapsToDimension ? mj + dimmargin
                                                : dim2goes; // constant index

        f   = dim3goes == ReadGoMapsToNumMaps   ? mi
            : dim3goes == ReadGoMapsToDimension ? mj + dimmargin
                                                : dim3goes; // constant index

        Maps[ mi ][ mj ]    = FreqDoc->GetFreqValue ( el, tf, f );
        }

    }

else if ( datatype != AtomTypeVector ) {
                                        // open as an EEG
    TOpenDoc< TTracksDoc >  EEGDoc ( filename, OpenDocHidden );

    if ( tracksnames )
        *tracksnames    = *EEGDoc->GetElectrodesNames ();

    if ( freqsnames )
        freqsnames->Reset ();


    TTracks<float>      EegBuff  ( numel, numtf );

                                        // read all data at once
    EEGDoc->GetTracks   (   0,          numtf - 1, 
                            EegBuff,    0,
                            datatype,   
                            NoPseudoTracks, 
                            reference
                        );


    int                 mi;
    int                 mj;
                                        // loop from source
    for ( int tf = 0; tf < numtf; tf++ )
    for ( int el = 0; el < numel; el++ ) {

        mi  = dim1goes == ReadGoMapsToNumMaps   ? el : tf;
        mj  = dim1goes == ReadGoMapsToDimension ? el : tf;


        if ( dimmargin > 0 ) {
            mj     -= dimmargin;
            if ( ! IsInsideLimits ( mj, 0, Dimension - 1 ) )
                continue;
            }


        Maps[ mi ][ mj ]    = EegBuff ( el , tf );
        }

//                                        // loop from destination
//    for ( int mi = 0; mi < NumMaps;   mi++ )
//    for ( int mj = 0; mj < Dimension; mj++ ) {
//
//        el  = dim1goes == ReadGoMapsToNumMaps   ? mi : mj;
//        tf  = dim2goes == ReadGoMapsToNumMaps   ? mi : mj;
//
//        Maps[ mi ][ mj ]    = EegBuff ( el , tf );
//        }
    }


else if ( datatype == AtomTypeVector ) {

    TOpenDoc< TRisDoc >     RISDoc ( filename, OpenDocHidden );

    if ( tracksnames )
        *tracksnames    = *RISDoc->GetElectrodesNames ();

    if ( freqsnames )
        freqsnames->Reset ();


    TArray1<TVector3Float>  InvBuff ( numel );


    int                 mi;
    int                 mj;
                                        // loop from source, reading sequentially 1 TF at a time
    for ( int tf = 0; tf < numtf; tf++ ) {
                                        // get a single TF, and we know the data are 3D vectors
        RISDoc->GetInvSol ( 0, tf, tf, InvBuff, 0, 0 );

                                        // spread x, y, z inside the same map
        for ( int el = 0; el < numel; el++ ) {

            mi  =       dim1goes == ReadGoMapsToNumMaps   ? el : tf;
            mj  = 3 * ( dim1goes == ReadGoMapsToDimension ? el : tf );  // always store the X,Y,Z components, for all types of layout


            if ( dimmargin > 0 ) {
                mj     -= 3 * dimmargin;
                if ( ! IsInsideLimits ( mj, 0, Dimension - 3 ) )
                    continue;
                }


            Maps[ mi ][ mj     ]    = InvBuff[ el ].X;
            Maps[ mi ][ mj + 1 ]    = InvBuff[ el ].Y;
            Maps[ mi ][ mj + 2 ]    = InvBuff[ el ].Z;
            } // for el
        } // for tf
    }
}


//----------------------------------------------------------------------------
                                        // Simplified to read only from a list of TF
                                        // Designed to be working on both scalar and vectorial data
void    TMaps::ReadFile     (   const char*         filename,
                                AtomType            datatype,       ReferenceType   reference,
                                const TMarkers&     keeplist,
                                TStrings*           tracksnames
                            )
{
                                        // retrieve file size, in time frames and electrodes
int                 numtf;
ReadFromHeader ( filename, ReadNumTimeFrames,         &numtf );


int                 numel;
ReadFromHeader ( filename, ReadNumElectrodes,         &numel );


SamplingFrequency   = -1;
ReadFromHeader ( filename, ReadSamplingFrequency,     &SamplingFrequency );
Maxed ( SamplingFrequency, 0.0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // so that any correlation / distance between maps will actually use all 3 components
int                 numrows         = numel * ( datatype == AtomTypeVector ? 3 : 1 );

                                        // set the number of maps according to the order parameters
int                 nummaps         = keeplist.GetMarkersTotalLength (); // numtf;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we got the final sizes
Resize ( nummaps, numrows );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const TMarker*      tomarker;


if      ( datatype != AtomTypeVector ) {
                                        // open as an EEG
    TOpenDoc< TTracksDoc >  EEGDoc ( filename, OpenDocHidden );


    if ( tracksnames )
        *tracksnames    = *EEGDoc->GetElectrodesNames ();


    TTracks<float>      EegBuff  ( keeplist.GetLongestMarker (), numtf );

                                        // !Overlapping markers are NOT tested, so it can technically save un-ordered and/or overlapping epochs!
    for ( int ki = 0, tf = 0; ki < (int) keeplist; ki++ ) {

        UpdateApplication;

        tomarker   = keeplist[ ki ];

                                        // read only for that marker
        EEGDoc->GetTracks   (   tomarker->From,    tomarker->To, 
                                EegBuff,        0, 
                                datatype, 
                                NoPseudoTracks, 
                                reference
                            );

                                        // copy the marker length only
        for ( ULONG tf0 = 0; tf0 < tomarker->Length (); tf0++, tf++ )

            for ( int el  = 0; el < numel; el++ )

                Maps[ tf ][ el ]    = EegBuff ( el , tf0 );

        } // for keeplist

    } // ! AtomTypeVector


else if ( datatype == AtomTypeVector ) {

    TOpenDoc< TRisDoc >     RISDoc ( filename, OpenDocHidden );


    if ( tracksnames )
        *tracksnames    = *RISDoc->GetElectrodesNames ();


    TArray1<TVector3Float>  InvBuff ( numel );

                                        // !Overlapping markers are NOT tested, so it can technically save un-ordered and/or overlapping epochs!
    for ( int ki = 0, tf = 0; ki < (int) keeplist; ki++ ) {

        UpdateApplication;

        tomarker   = keeplist[ ki ];

                                        // copy the marker length only
        for ( int tfm = tomarker->From; tfm <= tomarker->To; tfm++ ) {
                                        // get a single TF, and we know the data are 3D vectors
            RISDoc->GetInvSol ( 0, tfm, tfm, InvBuff, 0, 0 );


            for ( int el  = 0, el3 = 0; el < numel; el++ ) {

                Maps[ tf ][ el3++ ] = InvBuff[ el ].X;
                Maps[ tf ][ el3++ ] = InvBuff[ el ].Y;
                Maps[ tf ][ el3++ ] = InvBuff[ el ].Z;
                }

            }

        } // for keeplist

    } // AtomTypeVector

}


//----------------------------------------------------------------------------
                                        // Simplified to read multiple files and concatenate them into 1 TMaps all at once
void    TMaps::ReadFiles    (   const TGoF&         gof,
                                AtomType            datatype,       ReferenceType   reference,
                                TStrings*           tracksnames
                            )
{
                                        // TGoF already does this handy computations for us
int                 numtf           = gof.GetSumNumTF ();
int                 numel           = gof.GetNumElectrodes ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // so that any correlation / distance between maps will actually use all 3 components
int                 numrows         = numel * ( datatype == AtomTypeVector ? 3 : 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set the number of maps according to the order parameters
int                 nummaps         = numtf;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we got the final sizes
Resize ( nummaps, numrows );

SamplingFrequency   = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 mabs            = 0;


for ( int gofi = 0; gofi < gof.NumFiles (); gofi++ ) {

                                        // reading 1 file - that's the most duplicated data that will be encountered
    TMaps           onetmaps ( gof[ gofi ], datatype, reference, tracksnames && tracksnames->IsEmpty () ? tracksnames : 0 );


    if ( SamplingFrequency == 0 && onetmaps.GetSamplingFrequency () > 0 )
        SamplingFrequency   = onetmaps.GetSamplingFrequency ();

                                        // copying
    for ( int mrel  = 0; mrel  < onetmaps.GetNumMaps (); mrel++, mabs++ )

#if defined(TMapsOptimized)
    for ( int el    = 0; el    < Dimension;              el++           )
        Maps[ mabs ][ el ]  = onetmaps ( mrel, el );
#else
        Maps[ mabs ]    = onetmaps[ mrel ];
#endif

    }

}


//----------------------------------------------------------------------------
                                        // Save as scalar, or vectorial optionally
                                        // Re-orders dimensions in any combination, though it will add some complexities and will be slower
                                        // !filename / type must be of type .freq in case one wants to use the third dimension - caller responsability!
void    TMaps::WriteFileReordered   (   const char*             filename, 
                                        bool                    vectorial,      double              samplingfrequency, 
                                        const TStrings*         tracksnames,    const TStrings*     freqsnames,
                                        int                     dim1gets,       int                 dim2gets,       int         dim3gets,   
                                        int                     dimsize,        bool                updatingcall
                                    )   const

{
if ( IsNotAllocated () || StringIsEmpty ( filename ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set output file dimensions according to dispatch parameters
int                 numtf           = dim2gets == ReadGoMapsToNumMaps   ? NumMaps
                                    : dim2gets == ReadGoMapsToDimension ? Dimension
                                                                        : crtl::AtLeast ( 0, dimsize );  // additional size provided as option

int                 numel           = dim1gets == ReadGoMapsToNumMaps   ? NumMaps 
                                    : dim1gets == ReadGoMapsToDimension ? Dimension 
                                                                        : crtl::AtLeast ( 0, dimsize );  // additional size provided as option

int                 numfreq         = dim3gets == ReadGoMapsToNumMaps   ? NumMaps 
                                    : dim3gets == ReadGoMapsToDimension ? Dimension 
                                                                        : crtl::AtLeast ( 0, dimsize );  // additional size provided as option


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TExportTracks       expfile;

StringCopy   ( expfile.Filename,    filename );

bool                isfrequency     = IsExtensionAmong ( filename, AllFreqFilesExt );

                                        // !vectorial case needs more testing to get the # of tracks right!
if      ( isfrequency ) {
    expfile.SetAtomType ( AtomTypeScalar );
    expfile.NumTracks           = numel;

    StringCopy         ( expfile.FreqTypeName, FrequencyAnalysisNames[ FreqUnknown ], MaxCharFreqType - 1 );

/*
StringCopy ( expfile.Filename, OutFileName );

expfile.SetAtomType ( AtomTypeScalar );

ClearVirtualMemory ( expfile.FreqTypeName, sizeof ( expfile.FreqTypeName ) /*MaxCharFreqType* / );
StringCopy         ( expfile.FreqTypeName, FrequencyAnalysisNames[ freqtype ], MaxCharFreqType - 1 );

expfile.NumTracks           = numel;
expfile.NumFrequencies      = numfreqs;
expfile.NumTime             = numtf;
expfile.SamplingFrequency   = 0;
expfile.BlockFrequency      = EegDoc->GetSamplingFrequency ();
expfile.DateTime            = EegDoc->DateTime;

expfile.ElectrodesNames     = *EegDoc->GetElectrodesNames ();

expfile.FrequencyNames.Resize ( numfreqs, sizeof ( TFreqFrequencyName ) );
*/

    }
else if ( vectorial ) {
    expfile.SetAtomType ( AtomTypeVector );
    expfile.NumTracks           = numel / 3;    // actual number of tracks
    }
else {
    expfile.SetAtomType ( AtomTypeScalar );
    expfile.NumTracks           = numel;
    }


expfile.NumTime             = numtf;
expfile.NumFrequencies      = numfreq;          // 0 means no frequencies
expfile.SamplingFrequency   = samplingfrequency > 0 ? samplingfrequency : SamplingFrequency;


if ( tracksnames && tracksnames->NumStrings () >= expfile.NumTracks )
    expfile.ElectrodesNames = *tracksnames;

//expfile.ElectrodesNames.Resize ( expfile.NumTracks, ElectrodeNameSize );
//for ( int i = 0; i < NumMaps; i++ )
//    sprintf ( expfile.ElectrodesNames[ i ], "Map" );

if ( freqsnames && freqsnames->NumStrings () >= expfile.NumFrequencies )
    expfile.FrequencyNames  = *freqsnames;


int                 el;
int                 tf;
int                 f;

                                        // don't reset freq file after the first call (freq = 0)
expfile.Begin   ( isfrequency && updatingcall );

                                        // loop from destination
for ( int mi = 0; mi < NumMaps;   mi++ )
for ( int mj = 0; mj < Dimension; mj++ ) {

    el  = dim1gets == ReadGoMapsToNumMaps   ? mi
        : dim1gets == ReadGoMapsToDimension ? mj 
                                            : dim1gets; // constant index

    tf  = dim2gets == ReadGoMapsToNumMaps   ? mi
        : dim2gets == ReadGoMapsToDimension ? mj 
                                            : dim2gets; // constant index

    if ( isfrequency ) {

        f   = dim3gets == ReadGoMapsToNumMaps   ? mi
            : dim3gets == ReadGoMapsToDimension ? mj 
                                                : dim3gets; // constant index

        expfile.Write ( Maps[ mi ][ mj ], tf, el, f );
        }

    else if ( vectorial ) {

        expfile.Write ( TVector3Float ( Maps[ mi ][ mj     ],
                                        Maps[ mi ][ mj + 1 ],
                                        Maps[ mi ][ mj + 2 ] ), tf, el / 3 );
        mj += 2; // +=3 with loop
        }

    else // scalar
        expfile.Write ( Maps[ mi ][ mj ], tf, el );
    }


expfile.End     ();
}


//----------------------------------------------------------------------------
                                        // Save as scalar, or vectorial optionally
                                        // Go-to function for most of the cases
void    TMaps::WriteFile    (   const char*             filename, 
                                bool                    vectorial,
                                double                  samplingfrequency, 
                                const TStrings*         tracksnames 
                            )   const

{
if ( IsNotAllocated () || StringIsEmpty ( filename ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TExportTracks       expfile;

StringCopy   ( expfile.Filename,    filename );


if ( vectorial ) {
    expfile.SetAtomType ( AtomTypeVector );
    expfile.NumTracks           = Dimension / 3;    // actual number of tracks
    }
else {
    expfile.SetAtomType ( AtomTypeScalar );
    expfile.NumTracks           = Dimension;
    }


expfile.NumTime             = NumMaps;
expfile.SamplingFrequency   = samplingfrequency > 0 ? samplingfrequency : SamplingFrequency;


if ( tracksnames && tracksnames->NumStrings () >= expfile.NumTracks )
    expfile.ElectrodesNames = *tracksnames;


expfile.Begin   ();


for ( int tf = 0; tf < NumMaps; tf++ ) {

    UpdateApplication;

    if ( vectorial )

        for ( int e = 0; e < expfile.NumTracks; e++ )

            expfile.Write ( TVector3Float ( Maps[ tf ][ 3 * e     ],
                                            Maps[ tf ][ 3 * e + 1 ],
                                            Maps[ tf ][ 3 * e + 2 ] ) );
    else // Scalar
            expfile.Write ( Maps[ tf ] );
    }


expfile.End     ();
}


//----------------------------------------------------------------------------
                                        // Force writing vectorial maps as scalar - very special case
void    TMaps::WriteFileScalar  (   const char*             filename, 
                                    double                  samplingfrequency, 
                                    const TStrings*         tracksnames 
                                )   const

{
if ( IsNotAllocated () || StringIsEmpty ( filename ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TExportTracks       expfile;

StringCopy   ( expfile.Filename,    filename );


int                 numtracks   = Dimension / 3;


expfile.SetAtomType ( AtomTypeScalar );
expfile.NumTracks           = numtracks;
expfile.NumTime             = NumMaps;
expfile.SamplingFrequency   = samplingfrequency > 0 ? samplingfrequency : SamplingFrequency;


if ( tracksnames && tracksnames->NumStrings () >= expfile.NumTracks )
    expfile.ElectrodesNames = *tracksnames;


expfile.Begin   ();


for ( int tf = 0; tf < NumMaps; tf++ ) {

    UpdateApplication;

    for ( int e = 0; e < expfile.NumTracks; e++ )

        expfile.Write ( NormVector3 ( &Maps[ tf ][ 3 * e ] ) );
    }


expfile.End     ();
}


//----------------------------------------------------------------------------
                                        // Write maps only at given markers / epochs
                                        // !Caller should call with consistent parameters: vectorial data -> vectorial output; scalar data -> scalar output - we don't convert anymore here!
void    TMaps::WriteFileEpochs  (   const char*             filename, 
                                    bool                    vectorial,
                                    double                  samplingfrequency, 
                                    const TMarkers&         keeplist,
                                    const TStrings*         tracksnames 
                                )   const
{
if ( /*IsNotAllocated () ||*/ StringIsEmpty ( filename ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TExportTracks       expfile;

StringCopy   ( expfile.Filename,    filename );


if ( vectorial ) {
    expfile.SetAtomType ( AtomTypeVector );
    expfile.NumTracks           = Dimension / 3;    // actual number of tracks
    }
else {
    expfile.SetAtomType ( AtomTypeScalar );
    expfile.NumTracks           = Dimension;
    }


expfile.NumTime             = keeplist.GetMarkersTotalLength ();
expfile.SamplingFrequency   = samplingfrequency > 0 ? samplingfrequency : SamplingFrequency;

if ( tracksnames && tracksnames->NumStrings () >= expfile.NumTracks )
    expfile.ElectrodesNames   = *tracksnames;

//expfile.ElectrodesNames.Set ( expfile.NumTracks, ElectrodeNameSize );
//for ( int i = 0; i < NumMaps; i++ )
//    sprintf ( expfile.ElectrodesNames[ i ], "Map" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const TMarker*      tomarker;
bool                usebuff         = true; // ! ( expfile.NumTime == NumMaps && (int) keeplist == 1 ); // skip duplicating data?
TArray2<float>      buff            ( usebuff ? expfile.NumTime : 0, usebuff ? Dimension : 0 );
long                tf0             = 0;


expfile.Begin   ();
                                        // n intervals
                                        // Overlapping is not tested here, so it can technically save non-ordered and/or overlapping epochs
for ( int ki = 0; ki < (int) keeplist; ki++ ) {

    tomarker   = keeplist[ ki ];

                                        // skip markers not inside limit
//  if ( ! tomarker->IsInsideLimits ( TimeMin, TimeMax ) )
//      continue;


//                                        // transfer big block to memory - only if consecutive - not finished, not tested
//#if defined(TMapsOptimized)
//    CopyVirtualMemory ( buff[ tf0 ], Maps[ tomarker->From ], ( tomarker->To - tomarker->From + 1 ) * Maps[ 0 ].MemorySize () );
//    
//    tf0    += tomarker->To - tomarker->From + 1;
//#else


    for ( long tf = tomarker->From; tf <= tomarker->To; tf++, tf0++ ) {

        UpdateApplication;

        if ( usebuff )
            CopyVirtualMemory ( buff[ tf0 ], Maps[ tf ], Maps[ tf ].MemorySize () );
        else
            expfile.Write ( Maps[ tf ] );
        } // for tf

    } // for keeplist


if ( usebuff )
    expfile.Write   ( buff, NotTransposed );

expfile.End     ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TGoMaps::TGoMaps ()
{
}


        TGoMaps::TGoMaps ( int numgroups, int nummaps, int dim )                
{
Resize ( numgroups, nummaps, dim );
}


        TGoMaps::TGoMaps ( const TGoF* gof, AtomType datatype, ReferenceType reference, TStrings*    tracksnames )
{
ReadFiles ( *gof, datatype, reference, tracksnames );
}


        TGoMaps::~TGoMaps ()
{
DeallocateMemory ();
}


void    TGoMaps::DeallocateMemory ()
{
if ( IsNotAllocated () )
    return;

                                        // clean-up each group
for ( int gofi = 0; gofi < (int) Group; gofi++ )
    Group[ gofi ]->DeallocateMemory ();

                                        // delete content & structure
Group.Reset ( true );
}


void    TGoMaps::Resize ( int numgroups, int nummaps, int dim )
{
if ( dim <= 0 )                         // this is definitely an error, get out of here
    return;

                                        // nothing changed?
if ( numgroups == NumGroups () && nummaps == GetNumMaps () && dim == GetDimension () ) {
    Reset ();
    return;
    }


DeallocateMemory ();


for ( int gofi = 0; gofi < numgroups; gofi++ )

    Add ( new TMaps ( nummaps, dim ), false );


Reset ();
}


void    TGoMaps::Reset ()
{
                                        // clean-up each group
for ( int gofi = 0; gofi < (int) Group; gofi++ )

    Group[ gofi ]->Reset ();
}


        TGoMaps::TGoMaps ( const TGoMaps &op )
{
for ( int gofi = 0; gofi < op.NumGroups (); gofi++ )

    Add ( &op[ gofi ] );
}


TGoMaps& TGoMaps::operator= ( const TGoMaps &op2 )
{
if ( &op2 == this )
    return  *this;


DeallocateMemory ();


for ( int gofi = 0; gofi < op2.NumGroups (); gofi++ )

    Add ( &op2[ gofi ] );


return  *this;
}


//----------------------------------------------------------------------------
TGoMaps&  TGoMaps::operator/= ( double op2 )
{
if ( op2 == 0 || op2 == 1.0 )
    return *this;


TArray1<TMap *>     allmaps;

GetIndexes ( allmaps );

for ( int nc = 0; nc < (int) allmaps; nc++ )
    (*allmaps[ nc ])   /= op2;

return  *this;
}


//----------------------------------------------------------------------------
int     TGoMaps::GetMaxNumMaps () const
{
int                 maxnummaps      = 0;


for ( int gofi = 0; gofi < (int) Group; gofi++ )

    Maxed ( maxnummaps, Group[ gofi ]->GetNumMaps () );


return  maxnummaps;
}


int     TGoMaps::GetTotalNumMaps ()   const
{
int                 nummaps         = 0;


for ( int gofi = 0; gofi < (int) Group; gofi++ )

    nummaps    += Group[ gofi ]->GetNumMaps ();


return  nummaps;
}


void    TGoMaps::GetIndexes ( TArray1<TMap *> &indexes )  const
{
indexes.Resize ( GetTotalNumMaps () );


for ( int gofi = 0, i0 = 0; gofi < (int) Group; gofi++ ) {

    const TMaps&    Maps ( (*Group[ gofi ]) );

    for ( int nc = 0; nc < Maps.GetNumMaps (); nc++, i0++ )

        indexes[ i0 ]   = &Maps[ nc ];
    } // for gofi

}


TArray1<TMap *> TGoMaps::GetIndexes ()    const
{
TArray1<TMap *>     allmaps; 

GetIndexes ( allmaps ); 

return  allmaps; 
}

                                        // Retrieve 'g' maps, 1 for each group at position 'colindex'
void    TGoMaps::GetColumnIndexes ( TArray1<TMap *> &indexes, int colindex )  const
{
indexes.Resize ( (int) Group );

for ( int gofi = 0; gofi < (int) Group; gofi++ )

    indexes[ gofi ]     = &(*Group[ gofi ])[ colindex ];
}


//----------------------------------------------------------------------------
                                        // Returns the first non-null sampling frequency, if any
double  TGoMaps::GetSamplingFrequency ()  const
{
for ( int gofi = 0; gofi < (int) Group; gofi++ )

    if ( Group[ gofi ]->GetSamplingFrequency () > 0 )

        return  Group[ gofi ]->GetSamplingFrequency ();

return  0;
}


//----------------------------------------------------------------------------
void    TGoMaps::Add ( const TMaps* maps, bool copy )
{
if ( copy ) {
                                        // allocate and copy
    TMaps*            newgom      = new TMaps ( *maps );

    Group.Append ( newgom );
    }
else
    Group.Append ( maps );
}


void    TGoMaps::Remove ( const TMaps* maps )
{
Group.Remove ( maps );
}


//----------------------------------------------------------------------------
                                        // It might be more correct, in case of fancy normalization, to put all the data/conditions together
void    TGoMaps::Normalize ( AtomType datatype, bool centeraverage )
{
for ( int gofi = 0; gofi < (int) Group; gofi++ )

    Group[ gofi ]->Normalize ( datatype, -1, centeraverage );
}


//----------------------------------------------------------------------------
                                        // Centroids across all GoF
TMaps   TGoMaps::ComputeCentroids ( CentroidType centroid, AtomType datatype, PolarityType polarity ) const
{
TMaps               avgmaps;

if ( IsNotAllocated () )
    return  avgmaps;
                                        // !It could be a good idea at that point to check that all TMaps have the same number of maps!
avgmaps.Resize ( GetNumMaps (), GetDimension () );


                                        // get maps pointers across all GoF
TArray1<TMap *>     allmaps;


for ( int nc = 0; nc < GetNumMaps (); nc++ ) {

    GetColumnIndexes ( allmaps, nc );

    avgmaps[ nc ]   = crtl::ComputeCentroid ( allmaps, centroid, datatype, polarity );
    }


return  avgmaps;
}


//----------------------------------------------------------------------------
bool    TGoMaps::FilterSpatial ( SpatialFilterType filtertype, const char *xyzfile )
{
if ( filtertype == SpatialFilterNone )
    return  true;

if ( StringIsEmpty ( xyzfile ) )
    return  false;


TFilterSpatial<TMapAtomType>    filter  (   filtertype, 
                                            xyzfile, 
                                            SpatialFilterMaxNeighbors ( DefaultSFDistance ), 
                                            SpatialFilterMaxDistance  ( DefaultSFDistance ) 
                                        );

for ( int gofi = 0; gofi < (int) Group; gofi++ )

    filter.Apply ( *Group[ gofi ] );


return  true;
}


void    TGoMaps::FilterTime ( FilterTypes filtertype, FctParams& params, bool showprogress )
{
for ( int gofi = 0; gofi < (int) Group; gofi++ )

    Group[ gofi ]->FilterTime ( filtertype, params, showprogress );
}


//----------------------------------------------------------------------------
                                        // Allocate and load maps
                                        // optionally forcing scalar/vectorial, average reference and normalization
void    TGoMaps::ReadFiles  (   const TGoF&     gof,
                                AtomType        datatype,   ReferenceType   reference,  TStrings*       tracksnames 
                            )
{
DeallocateMemory ();

                                        // Read and create a TMaps for each file
for ( int gofi = 0; gofi < gof.NumFiles (); gofi++ ) {

    TMaps*            newgom      = new TMaps ( gof[ gofi ], datatype, reference, tracksnames && tracksnames->IsEmpty () ? tracksnames : 0 );

//    DBGV4 ( gofi + 1, gof->NumFiles (), newgom->GetNumMaps (), newgom->GetDimension (), (*gof)[ gofi ] );

    Add ( newgom, false );
    }

}


void    TGoMaps::WriteFiles (   const TGoF&     gof,
                                bool vectorial, double samplingfrequency, const TStrings*    tracksnames )  const
{

for ( int gofi = 0; gofi < gof.NumFiles (); gofi++ )

    Group[ gofi ]->WriteFile ( gof[ gofi ], vectorial, samplingfrequency, tracksnames );

}

                                        // write all maps in a single file
void    TGoMaps::WriteFile  (   const char*     file,
                                bool vectorial, double samplingfrequency, const TStrings*    tracksnames )  const
{
                                        // this makes a copy, a smarter way would be to simply generate a TMaps* and copy only the pointers..
TMaps               concatmaps ( this );

concatmaps.WriteFile ( file, vectorial, samplingfrequency, tracksnames );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TGoGoMaps::TGoGoMaps ()
{
}


        TGoGoMaps::TGoGoMaps ( int numgroups1, int numgroups2, int nummaps, int dim )                
{
for ( int gofi1 = 0; gofi1 < numgroups1; gofi1++ )

    Add ( new TGoMaps ( numgroups2, nummaps, dim ), false );
}


        TGoGoMaps::~TGoGoMaps ()
{
DeallocateMemory ();
}


void    TGoGoMaps::DeallocateMemory ()
{
if ( IsNotAllocated () )
    return;

                                        // clean-up each group
for ( int gofi = 0; gofi < (int) Group; gofi++ )
    Group[ gofi ]->DeallocateMemory ();

                                        // delete content & structure
Group.Reset ( true );
}


void    TGoGoMaps::Reset ()
{
                                        // clean-up each group
for ( int gofi = 0; gofi < (int) Group; gofi++ )
    Group[ gofi ]->Reset ();
}


        TGoGoMaps::TGoGoMaps ( const TGoGoMaps &op )
{
for ( int gofi = 0; gofi < op.NumGroups (); gofi++ )

    Add ( &op[ gofi ] );
}


TGoGoMaps& TGoGoMaps::operator= ( const TGoGoMaps &op2 )
{
if ( &op2 == this )
    return  *this;


DeallocateMemory ();


for ( int gofi = 0; gofi < op2.NumGroups (); gofi++ )

    Add ( &op2[ gofi ] );


return  *this;
}


//----------------------------------------------------------------------------
void    TGoGoMaps::Add ( const TGoMaps* gomaps, bool copy )
{
if ( copy ) {
                                        // allocate and copy
    TGoMaps*            newgom      = new TGoMaps ( *gomaps );

    Group.Append ( newgom );
    }
else
    Group.Append ( gomaps );
}


void    TGoGoMaps::Remove ( const TGoMaps* gomaps )
{
Group.Remove ( gomaps );
}


//----------------------------------------------------------------------------
int     TGoGoMaps::GetTotalNumMaps ()   const
{
int                 nummaps         = 0;


for ( int gogofi = 0; gogofi < (int) Group; gogofi++ )

    nummaps    += Group[ gogofi ]->GetTotalNumMaps ();


return  nummaps;
}


//----------------------------------------------------------------------------
                                        // Allocate and load maps
                                        // optionally forcing scalar/vectorial, average reference and normalization
void    TGoGoMaps::ReadFiles (    const TGoGoF*   gogof,
                                    AtomType        datatype,   ReferenceType   reference )
{
                                        // Read and create a TMaps for each file
for ( int gogofi = 0; gogofi < gogof->NumGroups (); gogofi++ ) {

    TGoMaps*            newgogom    = new TGoMaps ( &(*gogof)[ gogofi ], datatype, reference );

    Add ( newgogom, false );
    }

}


//----------------------------------------------------------------------------
                                        // It might be more correct, in case of fancy normalization, to put all the data/conditions together
void    TGoGoMaps::Normalize ( AtomType datatype, bool centeraverage )
{
for ( int gogofi = 0; gogofi < (int) Group; gogofi++ )

    Group[ gogofi ]->Normalize ( datatype, centeraverage );
}


void    TGoGoMaps::SetReference ( ReferenceType ref, AtomType datatype /*, TSelection *refsel*/ )
{
for ( int gogofi = 0; gogofi < (int) Group; gogofi++ )

    Group[ gogofi ]->SetReference ( ref, datatype );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
