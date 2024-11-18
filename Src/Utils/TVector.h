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

#include    "CartoolTypes.h"            // FilterTypes TMap
#include    "TArray1.h"
#include    "Geometry.TPoint.h"
#include    "Strings.TSplitStrings.h"

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Forward declarations
template <class TypeD>  class       TArray2;
template <class TypeD>  class       TTracks;
template <class TypeD>  class       TVolume;
template <class TypeD>  class       TVector;
template <class TypeD>  class       TFilterRanking;
class                               TStrings;
class                               TSelection;
class                               TRandUniform;
class                               TMatrix44;
class                               TPoints;
class                               TMaps;
enum                                SpatialFilterType;
enum                                InterpolationType;


//----------------------------------------------------------------------------
                                        // Filters and many processing need to pass many parameters at once in a unified manner
                                        // Class can be upgraded later
class   FctParams :   public  TArray1<double>
{
public:             FctParams () : TArray1<double> ( MaxNumParams ) {}

    double&         p0          = Array[ 0 ];   // Some example of using references - comes in handy while debugging f.ex.
    double&         p1          = Array[ 1 ];
    double&         p2          = Array[ 2 ];
};


//----------------------------------------------------------------------------
                                        // A vector is an arithmetic array with some specific behaviors

                                        // forward declaration
class               TElectrodesDoc;
enum                PolarityType;


enum                RankingOptions
                    {
                    RankingIgnoreNulls      = 0x01,
                    RankingAccountNulls     = 0x02,

                    RankingMergeIdenticals  = 0x10,
                    RankingCountIdenticals  = 0x20,
                    
                    RankingOptionDefault    = RankingAccountNulls | RankingMergeIdenticals,
                    };

                                        // Percentage of range for an outlier
constexpr double    SpatialFilterExtraRange     = 0.10;
                                        // Minimum Z-Score value for a local outlier to be replaced
constexpr double    SpatialFilterZScoreMin      = 2.0;


//----------------------------------------------------------------------------

template <class TypeD>
class   TVector :   public  TArray1<TypeD>
{
public:

    using           TArray1::TArray1;   // TArray1 constructors
                    TVector             ( double diameter, ArraySizeType sizeconstraint );
                    TVector             ( char* stringofvalues );
                    TVector             ( const TVolume<unsigned char>& array3 );
                    TVector             ( const Volume& array3 );


    TRealIndex      Index1;             // Convert real position to index for dimension 1


    void            SetNonNull          ( const TVector<TypeD> &v );                            // create a TVector with only the non-null values - dimension can therefore varry

    bool            IsNonNull           ()  const;
    bool            IsNull              ()  const;

                                        // Basic access to data
    TypeD           GetMaxValue         ()  const;
    TypeD           GetMinValue         ()  const;
    TypeD           GetAbsMaxValue      ()  const;
    int             GetMaxPosition      ()  const;
    int             GetMinPosition      ()  const;
    int             GetNumMaxes         ()  const;
    TypeD           GetSumValues        ()  const;
    TypeD           GetSumAbsValues     ()  const;
    TypeD           GetMeanValue        ()  const;

    TypeD           GetValueChecked     ( int index )                               const;  // checks range & returns 0 if out of range
    TypeD           GetValueChecked     ( double v, InterpolationType interpolate ) const;  // !convert first with TRealIndex, then interpolated!

    void            GetColumn           ( TArray2<float>  &array2, long yoffset, bool invert = false );
    void            GetColumn           ( TArray2<double> &array2, long yoffset, bool invert = false );
    void            GetRow              ( TArray2<float>  &array2, long xoffset, bool invert = false );
//  void            GetRow              ( TypeD *array1, GetRowMode mode = GetRowNormal );
    void            GetRow              ( TMaps&           gomaps, int row );
    void            SetColumn           ( TArray2<float>  &array2, long yoffset )   const;
    void            SetColumn           ( TArray2<double> &array2, long yoffset )   const;
    void            SetRow              ( TMaps&           gomaps, int row )        const;
    void            SetRow              ( TArray2<float>  &array2, int xoffset )    const;

                                        // In case vector is multiplexed x,y,z components, provide a direct access to a 3D vector - Works only because TPointT is a simple triplet X,Y,Z too
    TPointT<TMapAtomType>& Get3DVector  ( int i )   const;

                                        // Massaging the data
    void            NormalizeArea       ()              { *this /= NonNull ( GetSumAbsValues () ); }
    void            NormalizeMax        ()              { *this /= NonNull ( GetMaxValue ()     ); }
    void            NormalizeMinMax     (); // rescale in [0..1]


    void            GetBestFitParameters    ( double& a, double& b )    const;  // equation: y = a * x + b
    void            Detrend                 ();

    void            Filter              ( FilterTypes filtertype, int size = 1 );   // FilterTypeMean, FilterTypeGaussian
    void            Filter              ( FilterTypes filtertype, FctParams& params, bool showprogress = false );


                                        // Vectorial functions    
    void            AtLeast             ( TypeD minv );
    double          Average             ()                                  const;
    double          Average             ( const TSelection& subset )        const;
    void            Absolute            ();
    double          AbsSum              ()                                  const;
    void            AverageReference    ();
    void            AddGaussianNoise    ( double          sigmadata, double percentsnr );       // sigmadata is the SD of all dimensions
    void            AddGaussianNoise    ( TVector<TypeD>& sigmadata, double percentsnr );       // sigmadata is the SD for each dimension
    double          Correlation         ( const TVector<TypeD> &v, bool centeraverage = true )                                                      const;  // Pearson Correlation
    double          Correlation         ( const TVector<TypeD> &v, PolarityType polarity, bool centeraverage = true )                               const;  // testing for polarity inversion, if requested and if needed
    double          CorrelationDipoles  ( const TVector<TypeD> &v, PolarityType polarity, TVector<TypeD>* signs = 0 )                               const;
    double          Correlation         ( const TVector<TypeD> &v, const TSelection& subset, bool centeraverage = true )                                  const;  // avoid parameter TSelection*, which could be cast to a bool!
    double          CorrelationLinearCircular       ( const TVector<TypeD>& mapcirc, bool centeraverage, TVector<TypeD>& mapcos, TVector<TypeD>& mapsin ) const;
    double          CorrelationLinearCircularRobust ( const TVector<TypeD>& mapcirc )                                                               const;
    double          CorrelationSpearman ( const TVector<TypeD> &v, bool centeraverage )                                                             const;  // Spearman Correlation
    double          CorrelationSpearman ( const TVector<TypeD> &v, PolarityType polarity, bool centeraverage )                                      const;
    double          CorrelationFisher   ( const TVector<TypeD> &v, bool centeraverage = true )                                                      const;  // Pearson Correlation -> Fisher
    double          CorrelationKendall  ( const TVector<TypeD> &v, bool centeraverage = true )                                                      const;  // Pearson Correlation -> Kendall Tau
    double          CrossCorrelation    ( const TVector<TypeD> &v, int T )                                                                          const;  // with v shifted by T
    double          CorrelationToP      ( const TVector<TypeD> &v, bool centeraverage = true )                                                      const;  // Pearson Correlation to Significance p value (how much that correlation is meaningful)
    double          CorrelationToZ      ( const TVector<TypeD> &v, bool centeraverage = true )                                                      const;  // Pearson Correlation to Z Normal value (Fisher transformation)
    void            Cumulate            ( const TVector<TypeD> &v, double weight );             // weighted sum, weight could be negative
    void            Cumulate            ( const TVector<TypeD> &v, bool invert );               // signed sum
    void            Cumulate            ( const TVector<TypeD> &v, PolarityType polarity );
    void            Clipped             ( TypeD minv, TypeD maxv );
    double          Difference          ( const TVector<TypeD> &v, bool invert = false )                                const;
    double          Dissimilarity       ( const TVector<TypeD> &v, bool centeraverage = true, bool vectorial = false )  const;
    double          Dissimilarity       ( const TVector<TypeD> &v, const TSelection& subset, bool centeraverage = true, bool vectorial = false )  const;
    double          Dissimilarity       ( const TVector<TypeD> &v, PolarityType polarity, bool centeraverage = true )   const;  // testing for polarity inversion, if requested and if needed
    double          ExpVar              ( const TVector<TypeD> &v, bool centeraverage = true )                          const;
    PolarityType    GetPolarity         ( const TVector<TypeD> &op2 )       const;                                              // returns either PolarityInvert or PolarityDirect
    double          GlobalFieldPower    ( bool centeraverage = true, bool vectorial = false )                       const;
    double          GlobalFieldPower    ( const TSelection& subset, bool centeraverage = true, bool vectorial = false )   const;
    void            Invert              ();
    bool            IsOppositeDirection ( const TVector<TypeD> &op2 )       const;
    double          MaxCrossCorrelation ( const TVector<TypeD> &v, int &T ) const;              // return the max and shift
    void            Maxed               ( const TVector<TypeD> &v );
    void            Mined               ( const TVector<TypeD> &v );
    void            NoMore              ( TypeD maxv );
    double          Norm                ( bool centeraverage = false )      const;
    double          Norm2               ( bool centeraverage = false )      const;
    void            Normalize           ( bool centeraverage = false );
    void            Normalize1          (); // make sum = 1 (norm 1)
    void            Random              ( double minv, double maxv, TRandUniform* randunif = 0 );       // all entries filled with random values in [min..max]
    void            RandomSeries        ( int numwished, int maxseries, TRandUniform* randunif = 0 );   // a series of n non-repeating indexes in [0..size)
    double          ScalarProduct       ( const TVector<TypeD> &v )         const;
    double          ScalarProduct       ( const TVector<TypeD> &v, PolarityType polarity )  const;
    void            Smooth              ();
    double          SquaredDifference   ( const TVector<TypeD> &v, bool invert = false )    const;
    double          SquaredDifference   ( const TVector<TypeD> &v, PolarityType polarity )  const;
    void            SquareRoot          ();
    double          Sum                 ()                                  const;
    double          Sum                 ( const TSelection& subset )        const;
    void            ToRank              ( RankingOptions options );
    void            ThresholdAbove      ( TypeD t );
    void            ThresholdBelow      ( TypeD t );
    void            ZScore              ( double center, double spreadleft, double spreadright );
    void            ZScore              ();
    void            ZScoreAsym          ();
    void            ZScorePositive      ( bool absolute );  // for positive data - should be OK but not tested yet
    void            ZScoreAbsolute      ();     // Z-Score + Abs


    void            Sort                ( SortDirection direction, int size = 0 );

    void            Apply               ( const TArray2<TypeD>& m );                        // consider TArray2 as a matrix, apply vector to matrix

    void            MapFromDipole       ( double centerx, double centery, double centerz, double precession, double nutation, bool normalized, bool centeraverage, const TPoints& points, const TMatrix44& standardorient );
    void            MapFromLeadField    ( TTracks<float>& K, int numsources, TPoints* solp, double spreadmax, double axisvariability, bool centeraverage, bool normalize, TMap* brainstate = 0 );
    void            RisFromSolutionPoints( const TPoints& solp, int numsources, double spreadmax, double axisvariability, bool normalize );
    void            FitDipole           ( TElectrodesDoc* xyzdoc, TMap* map, TVector3Float* dipole );
    void            GetScalarMap        ( TMap& scalarmap, bool isvectorial )   const;      // handling scalar and vectorial cases
    bool            IsLocalMax          ( const TArray2<int>& neighbi, int spi, double mincentervalue, double epsilonneighbor )     const;
    int             GetLocalMaxes       ( const TArray2<int>& neighbi, double mincentervalue, double epsilonneighbor, int maxmaxes, TArray1<int>& localmaxes )  const;


    void            ReadFile            ( char* file, TStrings*    tracknames = 0 );                    // ep sef txt bin
    void            WriteFile           ( char* file, char *trackname = 0, int numpoints = -1 ) const;  // ep txt bin sef ris
    void            Export              ( TStrings*    tracknames, bool transpose, char *file ) const;  // Use TExportTracks
    void            Show                ( const char* title = 0 )                               const;


    double          ToIndex             ( double realposition  )    const   { return  Index1.ToIndex        ( realposition  );  }
    double          ToReal              ( double indexposition )    const   { return  Index1.ToReal         ( indexposition );  }
    double          ToIndexWidth        ( double realwidth     )    const   { return  Index1.ToIndexWidth   ( realwidth     );  }
    double          ToRealWidth         ( double indexwidth    )    const   { return  Index1.ToRealWidth    ( indexwidth    );  }

    using  TArray1::GetValue;
    virtual TypeD&  GetValue            ( double rp )                       { return  Array[ (int) Clip ( (int) ToIndex ( rp ), 0, Dim1 - 1 ) ]; }  // indexed by real value, converted through TRealIndex


                    TVector<TypeD>      ( const TVector<TypeD>& op  ) : TArray1 ( op ), Index1 ( op.Index1 )    {}
    TVector<TypeD>& operator    =       ( const TVector<TypeD>& op2 ) { TArray1<TypeD>::operator= ( op2 ); Index1 = op2.Index1; return *this; }

    using  TArray1::operator    =;

    TVector         operator    -       ()                      const;              

    TVector         operator    +       ( double op2 )          const;
    TVector         operator    -       ( double op2 )          const;              
    TVector         operator    *       ( double op2 )          const;
    TVector         operator    /       ( double op2 )          const;

    TVector&        operator    +=      ( double op2 );              
    TVector&        operator    -=      ( double op2 );
    TVector&        operator    *=      ( double op2 );             
    TVector&        operator    /=      ( double op2 );             

    TVector         operator    +       ( const TVector& op2 )  const;
    TVector         operator    -       ( const TVector& op2 )  const;
    TVector         operator    *       ( const TVector& op2 )  const;
    TVector         operator    /       ( const TVector& op2 )  const;

    TVector&        operator    +=      ( const TVector& op2 );       
    TVector&        operator    -=      ( const TVector& op2 );       
    TVector&        operator    *=      ( const TVector& op2 );       
    TVector&        operator    /=      ( const TVector& op2 );       

    bool            operator    ==      ( TypeD   op2 )         const;              


protected:

    void            FilterLinear                ( FilterTypes filtertype, FctParams& params, bool showprogress = false );
    void            FilterStat                  ( FilterTypes filtertype, FctParams& params, bool showprogress = false );
    void            FilterFastGaussian          ( FilterTypes filtertype, FctParams& params, bool showprogress = false );
    void            FilterThreshold             ( FilterTypes filtertype, FctParams& params );
    void            FilterBinarize              (                         FctParams& params );

    void            SortAscending       ( int l, int r );
    void            SortDescending      ( int l, int r );
};


//----------------------------------------------------------------------------

enum            PredefinedArrayType
                {
                PredefinedArraySinus,
                PredefinedArrayCosinus,
                PredefinedArrayLanczos2,
                PredefinedArrayLanczos3,
                PredefinedArrayLog,
                PredefinedArrayPower,
                };

                                        // Arithmetic array that can be initialized as a static const
class   TPredefinedArray    : public TVector<double>       
{
public:
                    TPredefinedArray ( PredefinedArrayType type, double datarange, int storagesize, double param = 0 );

                                        // includes circular end
    int             Size;
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
template <class TypeD>
        TVector<TypeD>::TVector ( double diameter, ArraySizeType sizeconstraint )
{
Dim1            = 0;
Array           = 0;

int                 size            = DiameterToKernelSize ( diameter, sizeconstraint );

Resize ( size );
}


//----------------------------------------------------------------------------
template <class TypeD>
        TVector<TypeD>::TVector ( const TVolume<uchar>& array3 )
{
Resize ( array3.GetLinearDim () );

for ( int i = 0; i < array3.GetLinearDim (); i++ )
    Array[ i ]  = array3[ i ];
}


template <class TypeD>
        TVector<TypeD>::TVector ( const Volume& array3 )
{
Resize ( array3.GetLinearDim () );

for ( int i = 0; i < array3.GetLinearDim (); i++ )
    Array[ i ]  = array3[ i ];
}


//----------------------------------------------------------------------------
template <class TypeD>
        TVector<TypeD>::TVector ( char *stringofvalues )
{
Dim1            = 0;
Array           = 0;


if ( StringIsEmpty ( stringofvalues ) )
    return;


TSplitStrings       sv ( stringofvalues, NonUniqueStrings );
int                 numvalues       = 0;


for ( int i = 0; i < (int) sv; i++ )
    numvalues      += IsFloat ( sv[ i ] );


if ( numvalues == 0 )
    return;


Resize ( numvalues );

                                        // insert only the floats
for ( int i = 0, j = 0; i < (int) sv; i++ )
    if ( IsFloat ( sv[ i ] ) )
        GetValue ( j++ )    = StringToDouble ( sv[ i ] );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVector<TypeD>::SetNonNull ( const TVector<TypeD> &v )
{
int                 nonnull         = 0;

for ( int i = 0; i < v.GetDim (); i++ )
    if ( v[ i ] )
        nonnull++;


Resize ( nonnull );


if ( nonnull == 0 )
    return;

                                        // copy only the non-null parts
for ( int i = 0, j = 0; i < v.GetDim (); i++ )
    if ( v[ i ] )
        Array[ j++ ]    = v[ i ];
}


//----------------------------------------------------------------------------
template <class TypeD>
bool    TVector<TypeD>::IsNonNull ()    const
{
if ( IsNotAllocated () )
    return  false;
                                        // quick test without loop, for most of the cases
if ( Array[ 0 ] )
    return  true;

for ( int i = 0; i < Dim1; i++ )
    if ( Array[ i ] )
        return  true;

return  false;
}


template <class TypeD>
bool    TVector<TypeD>::IsNull ()   const
{
if ( IsNotAllocated () )
    return  false;
                                        // quick test without loop, for most of the cases
if ( Array[ 0 ] )
    return  false;

for ( int i = 0; i < Dim1; i++ )
    if ( Array[ i ] )
        return  false;

return  true;
}


//----------------------------------------------------------------------------
template <class TypeD>
TVector<TypeD> TVector<TypeD>::operator- ()     const
{
TVector<TypeD>  temp ( *this );

for ( int i = 0; i < Dim1; i++ )
    temp.Array[ i ] = (TypeD) - Array[ i ];

return  temp;
}


//----------------------------------------------------------------------------
                                        // double operand, returning copy
template <class TypeD>
TVector<TypeD> TVector<TypeD>::operator+ ( double op2 )    const
{
TVector<TypeD>  temp ( *this );

if ( op2 == 0 )
    return  temp;

for ( int i = 0; i < Dim1; i++ )
    temp.Array[ i ]    += op2;

return  temp;
}


template <class TypeD>
TVector<TypeD> TVector<TypeD>::operator- ( double op2 )    const
{
TVector<TypeD>  temp ( *this );

if ( op2 == 0 )
    return  temp;

for ( int i = 0; i < Dim1; i++ )
    temp.Array[ i ]    -= op2;

return  temp;
}


template <class TypeD>
TVector<TypeD> TVector<TypeD>::operator* ( double op2 )    const
{
TVector<TypeD>  temp ( *this );

if      ( op2 ==  1.0 )
    return  temp;
else if ( op2 == -1.0 ) {
    temp.Invert ();
    return  temp;
    }
else if ( op2 == 0.0 ) {
    temp.ResetMemory ();
    return  temp;
    }


for ( int i = 0; i < Dim1; i++ )
    temp.Array[ i ]    *= op2;

return  temp;
}


template <class TypeD>
TVector<TypeD> TVector<TypeD>::operator/ ( double op2 )    const
{
TVector<TypeD>  temp ( *this );

if      ( op2 ==  1.0 )
    return  temp;
else if ( op2 == -1.0 ) {
    temp.Invert ();
    return  temp;
    }


for ( int i = 0; i < Dim1; i++ )
    temp.Array[ i ]    /= op2;

return  temp;
}


//----------------------------------------------------------------------------
                                        // double operand, on itself
template <class TypeD>
TVector<TypeD>& TVector<TypeD>::operator+= ( double op2 )
{
if ( op2 == 0 )
    return  *this;

for ( int i = 0; i < Dim1; i++ )
    Array[ i ]     += op2;

return  *this;
}


template <class TypeD>
TVector<TypeD>& TVector<TypeD>::operator-= ( double op2 )
{
if ( op2 == 0 )
    return  *this;

for ( int i = 0; i < Dim1; i++ )
    Array[ i ]     -= op2;

return  *this;
}


template <class TypeD>
TVector<TypeD>& TVector<TypeD>::operator*= ( double op2 )
{
if      ( op2 ==  1.0 )
    return  *this;
else if ( op2 == -1.0 ) {
    Invert ();
    return  *this;
    }
else if ( op2 == 0.0 ) {
    ResetMemory ();
    return  *this;
    }


for ( int i = 0; i < Dim1; i++ )
    Array[ i ]     *= op2;

return  *this;
}


template <class TypeD>
TVector<TypeD>& TVector<TypeD>::operator/= ( double op2 )
{
                      // any good reason for that "safety", apart from not crashing?
if      ( op2 ==  1.0 || op2 == 0 )
    return  *this;
else if ( op2 == -1.0 ) {
    Invert ();
    return  *this;
    }


for ( int i = 0; i < Dim1; i++ )
    Array[ i ]     /= op2;

return  *this;
}


//----------------------------------------------------------------------------
                                        // term by term operations, returning copy
template <class TypeD>
TVector<TypeD> TVector<TypeD>::operator+ ( const TVector &op2 )    const
{
TVector<TypeD>  temp ( *this );

for ( int i = 0; i < Dim1; i++ )
    temp.Array[ i ]    += op2.Array[ i ];

return  temp;
}


template <class TypeD>
TVector<TypeD> TVector<TypeD>::operator- ( const TVector &op2 )    const
{
TVector<TypeD>  temp ( *this );

for ( int i = 0; i < Dim1; i++ )
    temp.Array[ i ]    -= op2.Array[ i ];

return  temp;
}


template <class TypeD>
TVector<TypeD> TVector<TypeD>::operator* ( const TVector &op2 )    const
{
TVector<TypeD>  temp ( *this );

for ( int i = 0; i < Dim1; i++ )
    temp.Array[ i ]    *= op2.Array[ i ];

return  temp;
}


template <class TypeD>
TVector<TypeD> TVector<TypeD>::operator/ ( const TVector &op2 )    const
{
TVector<TypeD>  temp ( *this );

for ( int i = 0; i < Dim1; i++ )
    temp.Array[ i ]    /= op2.Array[ i ];

return  temp;
}


//----------------------------------------------------------------------------
                                        // term by term operations, on itself
template <class TypeD>
TVector<TypeD>& TVector<TypeD>::operator+= ( const TVector &op2 )
{
for ( int i = 0; i < Dim1; i++ )
    Array[ i ]     += op2.Array[ i ];

return  *this;
}


template <class TypeD>
TVector<TypeD>& TVector<TypeD>::operator-= ( const TVector &op2 )
{
for ( int i = 0; i < Dim1; i++ )
    Array[ i ]     -= op2.Array[ i ];

return  *this;
}


template <class TypeD>
TVector<TypeD>& TVector<TypeD>::operator*= ( const TVector &op2 )
{
for ( int i = 0; i < Dim1; i++ )
    Array[ i ]     *= op2.Array[ i ];

return  *this;
}


template <class TypeD>
TVector<TypeD>& TVector<TypeD>::operator/= ( const TVector &op2 )
{
for ( int i = 0; i < Dim1; i++ )
    if ( op2.Array[ i ] )
        Array[ i ]     /= op2.Array[ i ];

return  *this;
}


//----------------------------------------------------------------------------
template <class TypeD>
bool    TVector<TypeD>::operator== ( TypeD op2 )    const
{
for ( int i = 0; i < Dim1; i++ )
    if ( Array[ i ] != op2 )
        return false;

return  true;
}


//----------------------------------------------------------------------------
                                        // If vector holds 3D vectors, this function will return a TVector3Float from an index
                                        // !index provided is in range [0..#3Dvectors)!
template <class TypeD>
TPointT<TMapAtomType>&  TVector<TypeD>::Get3DVector ( int i )   const  
{
return  *( (TPointT<TMapAtomType>*) &Array[ 3 * i ] );
}


//----------------------------------------------------------------------------
template <class TypeD>
TypeD   TVector<TypeD>::GetValueChecked ( int index )   const
{
return  WithinBoundary ( index ) ? Array[ index ] : (TypeD) 0;
}


template <class TypeD>
TypeD   TVector<TypeD>::GetValueChecked ( double x, InterpolationType interpolate ) const
{
                                        // convert to index, but still in float
x   = ToIndex ( x );


if      ( IsFlag ( interpolate, InterpolateTruncate ) )

    return  GetValueChecked ( Truncate ( x ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else if ( IsFlag ( interpolate, InterpolateNearestNeighbor ) )

    return  GetValueChecked ( Round    ( x ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else if ( IsFlag ( interpolate, InterpolateLinear ) ) {

    int                 xi              = Truncate ( x );
    double              fx              = x - xi;

                                        // beware of negative coordinates -> truncation is done in the wrong direction, so we need to compensate
    if ( fx < 0 )   {   xi--;   fx += 1; }


    TypeD               l               = ( 1 - fx ) * GetValueChecked ( xi ) + fx * GetValueChecked ( AtMost ( Dim1 - 1, xi + 1 ) );

                                        // linear interpolation never under- nor over-shoot
    return  l;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // a 4 points interpolation
else if ( IsFlag ( interpolate, InterpolateUniformCubicBSpline )
       || IsFlag ( interpolate, InterpolateCubicHermiteSpline  ) ) {

                                        // these interpolations have behave the same
    double              (*spline)   ( double, double, double, double, double )  = IsFlag ( interpolate, InterpolateUniformCubicBSpline ) ? UniformCubicBSpline
                                                                                                                                         : CubicHermiteSpline;

    int                 xi              = Truncate ( x );
    double              fx              = x - xi;

                                        // beware of negative coordinates -> truncation is done in the wrong direction, so we need to compensate
    if ( fx < 0 )   {   xi--;   fx += 1; }


    double              s               = spline (   GetValueChecked ( xi - 1 ),    GetValueChecked ( xi ),    GetValueChecked ( xi + 1 ),    GetValueChecked ( xi + 2 ), fx );

    CheckOvershooting ( interpolate, s );

    return  s;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // (2*2) = 4 points interpolation
else if ( IsFlag ( interpolate, InterpolateLanczos2 ) ) {

    int                 xi              = Truncate ( x );
    double              fx              = x - xi;
    double              lx[ Lanczos2Size2 ];     // Lanczos can be separated for each axis
//  double              t;
    double              v               = 0;
//  double              w               = 0;

                                        // beware of negative coordinates -> truncation is done in the wrong direction, so we need to compensate
    if ( fx < 0 )   {   xi--;   fx += 1; }

                                        // compute Lanczos factors @ current point
    for ( int i = 0; i < Lanczos2Size2; i++ ) {
//      t           = M_PI * ( fx - i + Lanczos2Size - 1 );
//      lx[ i ]     = t ? Lanczos2Size * sin ( t ) * sin ( t / Lanczos2Size ) / ( t * t ) : 1;
        lx[ i ]     = FastLanczos2 ( fx - i + Lanczos2Size - 1 );
        }

                                        // scan kernel
    for ( int xd0 = 0, xd = xi + xd0 - Lanczos2Size2 + 1; xd0 < Lanczos2Size2; xd0++, xd++ ) {

        v  += GetValueChecked ( xd )
            * lx[ xd0 ];

//      w  += lx[ xd0 ];
        }

    CheckOvershooting ( interpolate, v );

    return  v;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // (2*3) = 6 points interpolation
else if ( IsFlag ( interpolate, InterpolateLanczos3 ) ) {

    int                 xi              = Truncate ( x );
    double              fx              = x - xi;
    double              lx[ Lanczos3Size2 ];     // Lanczos can be separated for each axis
//  double              t;
    double              v               = 0;
//  double              w               = 0;

                                        // beware of negative coordinates -> truncation is done in the wrong direction, so we need to compensate
    if ( fx < 0 )   {   xi--;   fx += 1; }

                                        // compute Lanczos factors @ current point
    for ( int i = 0; i < Lanczos3Size2; i++ ) {
//      t           = M_PI * ( fx - i + Lanczos3Size - 1 );
//      lx[ i ]     = t ? Lanczos3Size * sin ( t ) * sin ( t / Lanczos3Size ) / ( t * t ) : 1;
        lx[ i ]     = FastLanczos3 ( fx - i + Lanczos3Size - 1 );
        }

                                        // scan kernel
    for ( int xd0 = 0, xd = xi + xd0 - Lanczos3Size + 1; xd0 < Lanczos3Size2; xd0++, xd++ ) {

        v  += GetValueChecked ( xd )
            * lx[ xd0 ];

//      w  += lx[ xd0 ];
        }


    CheckOvershooting ( interpolate, v );

    return  v;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // if everything else fails
return  0;
}


//----------------------------------------------------------------------------
template <class TypeD>
TypeD   TVector<TypeD>::GetMaxValue ()      const
{
if ( IsNotAllocated () )
    return  0;


TypeD               maxvalue        = Array[ 0 ];

for ( int i = 1; i < Dim1; i++ )

    crtl::Maxed ( maxvalue, Array[ i ] );


return  maxvalue;
}


template <class TypeD>
TypeD   TVector<TypeD>::GetMinValue ()      const
{
if ( IsNotAllocated () )
    return  0;


TypeD               minvalue        = Array[ 0 ];

for ( int i = 1; i < Dim1; i++ )

    crtl::Mined ( minvalue, Array[ i ] );


return  minvalue;
}


template <class TypeD>
TypeD   TVector<TypeD>::GetAbsMaxValue ()      const
{
if ( IsNotAllocated () )
    return  0;


TypeD               maxvalue        = (TypeD) fabs ( Array[ 0 ] );

for ( int i = 1; i < Dim1; i++ )

    crtl::Maxed ( maxvalue, (TypeD) fabs ( Array[ i ] ) );


return  maxvalue;
}


template <class TypeD>
int     TVector<TypeD>::GetMaxPosition ()   const
{
if ( IsNotAllocated () )
    return  -1;


TypeD               maxvalue        = Array[ 0 ];
int                 maxp            = 0;

for ( int i = 0; i < Dim1; i++ )
    if ( Array[ i ] > maxvalue ) {
        maxvalue    = Array[ i ];
        maxp        = i;
        }

return  maxp;
}


template <class TypeD>
int     TVector<TypeD>::GetMinPosition ()   const
{
if ( IsNotAllocated () )
    return  -1;


TypeD               minvalue        = Array[ 0 ];
int                 minp            = 0;

for ( int i = 0; i < Dim1; i++ )
    if ( Array[ i ] < minvalue ) {
        minvalue    = Array[ i ];
        minp        = i;
        }

return  minp;
}


template <class TypeD>
int     TVector<TypeD>::GetNumMaxes ()      const
{
if ( IsNotAllocated () )
    return  -1;


int                 n               = 0;


for ( int i = 1; i < Dim1 - 1; i++ )
    if ( Array[ i ] > Array[ i - 1 ]
      && Array[ i ] > Array[ i + 1 ] )

        n++;

                                        // border cases done separately
if ( Array[ 0        ] > Array[ 1        ] )    n++;
if ( Array[ Dim1 - 1 ] > Array[ Dim1 - 2 ] )    n++;


return  n;
}


//----------------------------------------------------------------------------
template <class TypeD>
TypeD   TVector<TypeD>::GetSumValues ()     const
{
if ( IsNotAllocated () )
    return  0;


double /*TypeD*/    sum             = 0;

for ( int i = 0; i < Dim1; i++ )
    sum    += Array[ i ];

return  (TypeD) sum;
}


template <class TypeD>
TypeD   TVector<TypeD>::GetSumAbsValues ()  const
{
if ( IsNotAllocated () )
    return  0;


double /*TypeD*/    sum             = 0;

for ( int i = 0; i < Dim1; i++ )
    sum    += fabs ( Array[ i ] );

return  (TypeD) sum;
}


template <class TypeD>
TypeD   TVector<TypeD>::GetMeanValue ()     const
{
if ( IsNotAllocated () )
    return  0;


double /*TypeD*/    sum             = 0;

for ( int i = 0; i < Dim1; i++ )
    sum    += Array[ i ];

return  (TypeD)  ( sum / Dim1 );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVector<TypeD>::Sort ( SortDirection direction, int size )
{
if ( IsNotAllocated () )
    return;

//if ( l < 0 || l >= GetDim1 () )         l       = 0;
//if ( r < 0 || r >= GetDim1 () )         r       = Dim1 - 1;

                                        // array might be partially filled
if ( direction == Ascending )   SortAscending  ( 0, IsInsideLimits ( size, 1, Dim1 ) ? size - 1 : Dim1 - 1 );
else                            SortDescending ( 0, IsInsideLimits ( size, 1, Dim1 ) ? size - 1 : Dim1 - 1 );
}


template <class TypeD>
void    TVector<TypeD>::SortAscending ( int l, int r )
{
if ( r <= l )   return;


int                 i               = l;
int                 j               = r;
TypeD               v               = GetValue ( ( l + r ) / 2 );


do {
    while ( GetValue ( i ) < v )    i++;

    while ( v < GetValue ( j ) )    j--;

    if ( i <= j ) {
                                        // permutate i and j
        Permutate ( Array[ i ], Array[ j ] );
        i++;    j--;
        }

    } while ( i <= j );


if ( l < j )    SortAscending ( l, j );
if ( i < r )    SortAscending ( i, r );
}


template <class TypeD>
void    TVector<TypeD>::SortDescending ( int l, int r )
{
if ( r <= l )   return;


int                 i               = l;
int                 j               = r;
TypeD               v               = GetValue ( ( l + r ) / 2 );


do {
    while ( GetValue ( i ) > v )    i++;

    while ( v > GetValue ( j ) )    j--;

    if ( i <= j ) {
                                        // permutate i and j
        Permutate ( Array[ i ], Array[ j ] );
        i++;    j--;
        }

    } while ( i <= j );


if ( l < j )    SortDescending ( l, j );
if ( i < r )    SortDescending ( i, r );
}


//----------------------------------------------------------------------------
                                        // Old filter, with size translated to # iterations
template <class TypeD>
void    TVector<TypeD>::Filter ( FilterTypes filtertype, int size )
{
if ( size <= 0 )
    return;

                                        // how many times we're going to repeat a 3x3 filter
int                 numfilters      = size / 2;

constexpr int       Xm          = 0;
constexpr int       X0          = 1;
constexpr int       Xp          = 2;
double              w[ 3 ];
double              sum;
double              sumw;
//double            sumw2;
int                 i1;

TVector<TypeD>      safecopy;



if      ( filtertype == FilterTypeGaussian ) {
    w[ Xm ] = 1;
    w[ X0 ] = 2;
    w[ Xp ] = 1;
    sumw    = 4;
//  sumw2   = 2;
    }
else if ( filtertype == FilterTypeMean ) {
    w[ Xm ] = 1;
    w[ X0 ] = 1;
    w[ Xp ] = 1;
    sumw    = 3;
//  sumw2   = 1;
    }
else
    return;



for ( int s = 0; s < numfilters; s++ ) {

    safecopy    = *this;

                                        // scan within safe limits
    for ( i1 = 1; i1 < Dim1 - 1; i1++ ) {

        sum   = (TypeD) (  w[ Xm ] * safecopy[ i1 - 1 ]
                         + w[ X0 ] * safecopy[ i1     ]
                         + w[ Xp ] * safecopy[ i1 + 1 ] );

                                // put result directly in my own array
//      Array[ i1 ]     = (TypeD) ( ( sum + sumw2 ) / sumw );
        Array[ i1 ]     = (TypeD) ( sum / sumw );
        }

                                        // edges, padding with 0's
    i1    = 0;
    sum   = (TypeD) (  w[ X0 ] * safecopy[ i1     ]
                     + w[ Xp ] * safecopy[ i1 + 1 ] );
    Array[ i1 ] = (TypeD) ( sum / sumw );

    i1    = Dim1 - 1;
    sum   = (TypeD) (  w[ Xm ] * safecopy[ i1 - 1 ]
                     + w[ X0 ] * safecopy[ i1     ] );
    Array[ i1 ] = (TypeD) ( sum / sumw );
    } // for s
}


//----------------------------------------------------------------------------
                                        // Correct filters
template <class TypeD>
void    TVector<TypeD>::Filter ( FilterTypes filtertype, FctParams& params, bool showprogress )
{
if      ( filtertype == FilterTypeGaussian
       || filtertype == FilterTypeHighPassLight
       || filtertype == FilterTypeLoG
       || filtertype == FilterTypeMean              )   FilterLinear                ( filtertype,               params, showprogress );

else if ( filtertype == FilterTypeFastGaussian      )   FilterFastGaussian          ( filtertype,               params, showprogress );

else if ( filtertype == FilterTypeThreshold
       || filtertype == FilterTypeThresholdAbove
       || filtertype == FilterTypeThresholdBelow    )   FilterThreshold             ( filtertype,               params               );
else if ( filtertype == FilterTypeBinarize          )   FilterBinarize              (                           params               );

else if ( filtertype == FilterTypeMin
       || filtertype == FilterTypeMax
       || filtertype == FilterTypeMinMax
       || filtertype == FilterTypeRange
       || filtertype == FilterTypeMedian
       || filtertype == FilterTypeInterquartileMean
//     || filtertype == FilterTypeMeanSub
//     || filtertype == FilterTypeMAD
       || filtertype == FilterTypeSD
       || filtertype == FilterTypeSDInv
       || filtertype == FilterTypeCoV
       || filtertype == FilterTypeLogSNR
//     || filtertype == FilterTypeMCoV
//     || filtertype == FilterTypeMADSDInv
//     || filtertype == FilterTypeModeQuant
//     || filtertype == FilterTypeEntropy
                                                    )  FilterStat          ( filtertype,               params, showprogress );
else
    ShowMessage ( "Filter not implemented for TVector<TypeD>!", FilterPresets[ filtertype ].Text, ShowMessageWarning );
}


//----------------------------------------------------------------------------
                                        // Border padding with edge value (was: 0)
template <class TypeD>
void    TVector<TypeD>::FilterThreshold ( FilterTypes filtertype, FctParams& params )
{

if      ( filtertype == FilterTypeThreshold ) {

    double              mint            = params ( FilterParamThresholdMin );
    double              maxt            = params ( FilterParamThresholdMax );

    for ( int i = 0; i < Dim1; i++ )

        if ( ! IsInsideLimits ( (double) Array[ i ], mint, maxt ) )

            Array[ i ]  = 0;
    }

else if ( filtertype == FilterTypeThresholdAbove ) {

    double              mint            = params ( FilterParamThreshold );

    for ( int i = 0; i < Dim1; i++ )

        if ( (double) Array[ i ] < mint )

            Array[ i ]  = 0;
    }

else if ( filtertype == FilterTypeThresholdBelow ) {

    double              maxt            = params ( FilterParamThreshold );

    for ( int i = 0; i < Dim1; i++ )

        if ( (double) Array[ i ] > maxt )

            Array[ i ]  = 0;
    }
}


template <class TypeD>
void    TVector<TypeD>::FilterBinarize ( FctParams& params )
{
double              bin             = params ( FilterParamBinarized );


for ( int i = 0; i < Dim1; i++ )

    if ( Array[ i ] )   Array[ i ]  = bin;
}


//----------------------------------------------------------------------------
                                        // Border padding with edge value (was: 0)
template <class TypeD>
void    TVector<TypeD>::FilterLinear ( FilterTypes filtertype, FctParams& params, bool showprogress )
{
double              diameter        = filtertype == FilterTypeHighPassLight ? 3 : params ( FilterParamDiameter );

if ( diameter < 2 )                     // no other voxels involved other than center?
    return;

                                        // Trick to have more precision on the Gaussian:
                                        // increase the diameter and the number of sigma by the same amount, here going from 2 sigma to 6.
double              resamplegaussian    = filtertype == FilterTypeGaussian
                                       || filtertype == FilterTypeLoG      ? 6 / 2 : 1;
                                        // go from 2 SD width to resamplegaussian
diameter    = params ( FilterParamDiameter ) * resamplegaussian;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // see:  http://en.wikipedia.org/wiki/Sampled_Gaussian_kernel#The_sampled_Gaussian_kernel

                                        // create Kernel mask, always of odd size
TVector<double>     Kf ( diameter, OddSize );
int                 KoX             = Kf.GetDim1 () / 2;

double              r               = diameter / 2 + ( IsEven ( Kf.GetDim1 () ) ? 0.5 : 0 ) + 1e-6;
int                 kpx;
double              v;

                                        // 2 sigma is the default
int                 gaussnumSD      = 2 * resamplegaussian;
TGaussian           gauss ( 0, ( diameter - 1 ) / 2, gaussnumSD );
double              sigma           = ( diameter - 1 ) / 2 / gaussnumSD;

                                        // compute the Kernel in floating point
for ( int xk = 0; xk < Kf.GetDim1 (); xk++ ) {

    kpx     = xk - KoX;

                                        // these filters size IS modified
    if      ( filtertype == FilterTypeGaussian      )       v   = gauss ( kpx );
    else if ( filtertype == FilterTypeLoG           )       v   = ( Square ( (double) kpx ) - Square ( sigma ) ) / Power ( sigma, 4 ) * gauss ( kpx );   // skipping the cst because we are normalizing after

                                        // these filters size IS NOT modified
    else if ( filtertype == FilterTypeHighPassLight )       v   = ( kpx ? -1 : 4 ) * gauss ( kpx ); // very rough, only for 3
    else /*if ( filtertype == FilterTypeMean        )*/     v   = 1;

                                // clip Kernel outside Euclidian norm!
    Kf ( xk )   = abs ( kpx ) <= r ? v : 0;
    }

                                        // normalize Kernel
Kf.NormalizeArea ();


//DBGV3 ( diameter, Kf.GetDim1 (), KoX, "diameter  Kernel: Dim1 Origin" );
//for ( int xk = 0; xk < Kf.GetDim1 (); xk++ )
//    DBGV2 ( xk, Kf ( xk ) * 1000, "x  K" );
//if ( filtertype == FilterTypeGaussian ) { Kf.WriteFile ( "E:\\Data\\Gaussian Kernel.sef" ); DBGM ( "Wrote Gaussian Kernel", ""); }
//if ( filtertype == FilterTypeLoG      ) { Kf.WriteFile ( "E:\\Data\\LoG Kernel.sef" ); DBGM ( "Wrote LoG Kernel", ""); }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need a cloned temp array, which includes a safety border
TVector<TypeD>      temp ( Dim1 + KoX * 2 );

temp.Insert ( *this, &KoX );

                                        // padding with border values
for ( int x = 0; x < KoX; x++ ) {
    temp ( x              ) = temp ( KoX            );
    temp ( Dim1 + KoX + x ) = temp ( Dim1 + KoX - 1 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;

if ( showprogress ) {
    Gauge.Set       ( FilterPresets[ filtertype ].Text );
    Gauge.AddPart   ( 0, Dim1, 100  );
    Gauge.CurrentPart   = 0;
    }


for ( int x = 0; x < Dim1; x++ ) {

    if ( showprogress )
        Gauge.Next ();

    v       = 0;

                                        // scan kernel
    for ( int xki = 0, xk = x; xki < Kf.GetDim1 (); xki++, xk++ )
                                        // always compute in double
        v      += Kf ( xki ) * temp ( xk );


    Array[ x ]  = (TypeD) v;
    }
}


//----------------------------------------------------------------------------
                                        // Fastest Gaussian filtering, with SKISPM:
// "An efficient algorithm for Gaussian blur using finite-state machines"
// Frederick M. Waltz and John W. V. Miller
// SPIE Conf. on Machine Vision Systems for Inspection and Metrology VII
// Originally published Boston, Nov. 1998

                                        // Size parameter is an odd, discrete width in voxels, like 3, 5, 7 etc..
                                        // We repeat elementary and fast 3x3 filters as many times as needed to reach the required width
                                        // Border padding is currently set to constant filling
                                        // !Be careful when using this, it behaves like a IIR, which can be instable at the edges!
template <class TypeD>
void    TVector<TypeD>::FilterFastGaussian ( FilterTypes filtertype, FctParams& params, bool showprogress )
{
int                 numrepeat;


if ( params ( FilterParamDiameter ) < 0 )
    numrepeat       = fabs ( params ( FilterParamDiameter ) );  // !if negative value, force use as the number of repetitions!
else
    numrepeat       = params ( FilterParamDiameter ) > 2 ? Round ( ( Power ( params ( FilterParamDiameter ) - 2, 1.755 ) - 1 ) / 2 ) : 0;    // ad-hoc formula after comparison with our plain Gaussian


if ( numrepeat < 1 )                    // not big enough
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need a cloned temp array, which includes a safety border + conversion to higher precision
int                 offset          = numrepeat;
int                 tdim1           = Dim1 + 2 * offset;
TVector<double/*TypeD*/>    temp ( tdim1 );

                                        // insert with origin shift + conversion
for ( int x = 0; x < Dim1; x++ )

    temp ( x + offset )  = Array[ x ];

                                        // edges filling:
for ( int x = 0; x < offset; x++ ) {
                                        // edges set to 0: comment this

                                        // cst value - this seems to give the best results for histograms
    temp ( x        )   = Array[ 0        ];
    temp ( Dim1 + x )   = Array[ Dim1 - 1 ];
                                        // mirroring
//  temp ( x        )   = Array[ LeftMirroring  ( 0,        x           ) ];
//  temp ( Dim1 + x )   = Array[ RightMirroring ( Dim1 - 1, x, Dim1 - 1 ) ];
                                        // trend
//  temp ( x        )   = ( Array[ 0        ] - Array[ 5            ] ) / 4 * ( x + 1 ) + Array[ 0        ];
//  temp ( Dim1 + x )   = ( Array[ Dim1 - 1 ] - Array[ Dim1 - 1 - 5 ] ) / 4 * ( x + 1 ) + Array[ Dim1 - 1 ];
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // allocate SKIPSM buffers
double /*TypeD*/    Sx0;                // 2 previous values
double /*TypeD*/    Sx1;
double /*TypeD*/    tmp1;
double /*TypeD*/    tmp2;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( FilterPresets[ filtertype ].Text, showprogress ? numrepeat *  ( tdim1 - 1 ) : 0 );


for ( int n = 0; n < numrepeat; n++ ) {

    Sx0     = 0;
    Sx1     = 0;

    for ( int x = 1; x < tdim1; x++ ) {

        Gauge.Next ();


        tmp1            = temp ( x );               // |1|

        tmp2            = Sx0 + tmp1;               // |1 1|
        Sx0             = tmp1;

        temp ( x - 1 )  = ( Sx1 + tmp2 ) / 4.0;

        Sx1             = tmp2;

        } // for x
    } // for numrepeat


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // copy back results
for ( int x = 0; x < Dim1; x++ )

    Array[ x ]  = (TypeD) temp ( x + offset );

}


//----------------------------------------------------------------------------
                                        // Anything stat-like filter
template <class TypeD>
void    TVector<TypeD>::FilterStat ( FilterTypes filtertype, FctParams& params, bool showprogress )
{
double              diameter        =         params ( FilterParamDiameter );
//double              histomaxbins    = Round ( params ( FilterParamNumModeQuant ) );
//int                 filterresult    =         params ( FilterParamResultType );


if ( diameter < 2 )                     // no other voxels involved other than center?
    return;

//if ( filtertype == FilterTypeModeQuant && histomaxbins < 2 )
//    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Log filters need some offset to avoid values <= 0
bool                logfilters      = filtertype == FilterTypeLogCoV
                                   || filtertype == FilterTypeLogMCoV
                                   || filtertype == FilterTypeLogMeanSub
                                   || filtertype == FilterTypeLogMeanDiv
                                   || filtertype == FilterTypeLogSNR;

double              logoffset       = logfilters ? params ( FilterParamLogOffset ) : 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute Kernel size, always odd

                                        // the part above 1 voxel
int                 radius          = Round ( diameter / 2 );
                                        // 1 -> 0; ]1..3] -> 1; ]3..5] -> 2 etc...
if ( Fraction ( diameter / 2 + 0.5 ) == 0 )
    radius--;

int                 dim             = 1 + 2 * radius;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need a cloned temp array, which includes a safety border
TVector<TypeD>      temp ( Dim1 + radius * 2 );

temp.Insert ( *this, &radius );

                                        // fill border with last known value
for ( int x = 0; x < radius; x++ ) {
    temp ( x )                  = temp ( radius );
    temp ( Dim1 + radius + x )  = temp ( Dim1 + radius - 1 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( FilterPresets[ filtertype ].Text, showprogress ? Dim1 : 0 );

                                        // we can save some resources for these filters
TEasyStats          stat ( filtertype == FilterTypeMin 
                        || filtertype == FilterTypeMax 
//                      || filtertype == FilterTypeSD       // these could be legally sped up, but longer version has supposedly a better precision
//                      || filtertype == FilterTypeSDInv 
//                      || filtertype == FilterTypeCoV 
//                      || filtertype == FilterTypeLogSNR 
                        || filtertype == FilterTypeMinMax ? 0 : dim );
double              v;
//double              avg;
//double              sd;

/*
int                 maxvalue        = max ( 1, (int) GetMaxValue () );

double              histodown       = filtertype == FilterTypeEntropy ? 1 : histomaxbins / maxvalue;
THistogram          histo ( maxvalue * histodown + 1 );
int                 histominfreq    = max ( 3, Round ( Power ( dim, 1 / 3.0 ) * 1.0 ) );

                                        // entropy is a count of grey levels: can not exceed histo size, and # of insertion
double              entropyrescale  = UCHAR_MAX / (double) max ( UCHAR_MAX, min ( maxvalue + 1, dim ) );
*/
                                        // rescale to max char
//double              covrescale      = LONG_MAX / (double) sqrt ( dim );
//double              covrescale      = 1 / (double) sqrt ( dim );
//double              covrescale      = LONG_MAX / Power ( dim, 0.25 );



for ( int x = 0; x < Dim1; x++ ) {

    Gauge.Next ();


    stat .Reset ();
//    histo.Reset ();

                                        // scan kernel
    for ( int xki = 0, xk = x; xki < dim; xki++, xk++ ) {

        stat .Add ( temp ( xk ), ThreadSafetyIgnore );
//        histo.Add ( temp ( xk ) * histodown + 0.5 );
        }

    stat.Sort ( true );


    if      ( filtertype == FilterTypeMin               )   v   = stat.Min ();
    else if ( filtertype == FilterTypeMax               )   v   = stat.Max ();
    else if ( filtertype == FilterTypeRange             )   v   = stat.Range ();
    else if ( filtertype == FilterTypeMedian            )   v   = stat.Median ();
    else if ( filtertype == FilterTypeInterquartileMean )   v   = stat.InterQuartileMean ();
    else if ( filtertype == FilterTypeSD                )   v   = stat.SD ();
    else if ( filtertype == FilterTypeSDInv             )   v   = ( v = stat.SD () ) != 0 ? crtl::NoMore ( 1e10, 1 / v ) : 0; // capping the max(?)
                                        // give more resolution to lower values (Gamma in option?)
    else if ( filtertype == FilterTypeCoV               )   v   = stat.CoefficientOfVariation (); //  * covrescale;
//  else if ( filtertype == FilterTypeSNR               )   v   = stat.SNR ();
    else if ( filtertype == FilterTypeLogSNR            )   v   = Log10 ( logoffset + stat.SNR () );    // much better with a log

                                        // Sharp Window: take the one closest, the min or the max
    else if ( filtertype == FilterTypeMinMax            )   v   = stat.MinMax ( GetValue ( x ) );


    Array[ x ]  = (TypeD) v;

    }
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVector<TypeD>::GetBestFitParameters ( double& a, double& b )   const
{
a       = b     = 0;

if ( Dim1 < 2 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              xm              = (double) ( Dim1 - 1 ) / 2;
double              ym              = GetMeanValue ();


double              sum1            = 0;
double              sum2            = 0;

for ( int i = 0; i < Dim1; i++ ) {
    sum1   += ( i - xm ) * ( Array[ i ] - ym );
    sum2   += Square ( i - xm );
    }

a   = sum1 / NonNull ( sum2 );

b   = ym - a * xm;
}


template <class TypeD>
void    TVector<TypeD>::Detrend ()
{
if ( Dim1 < 2 )
    return;


double              a;
double              b;

                                        // Get trend with a least square best fitting line
GetBestFitParameters ( a, b );

                                        // Subtract the trend
                                        // If trend also modulates the variance (high values -> high variance) then divide instead of subtract
for ( int i = 0; i < Dim1; i++ )
    Array[ i ]     -= a * i + b;
}


//----------------------------------------------------------------------------
template <class TypeD>
void TVector<TypeD>::GetColumn ( TArray2<float> &array2, long yoffset, bool invert )
{
                                        // gracefully resize myself
if ( Dim1 == 0 )
    Resize ( array2.GetDim1 () );

                                        // set secure limits for both variables
int                 indim1          = min ( Dim1, array2.GetDim1 () );


if ( invert )                           
    for ( int i = 0; i < indim1; i++ )
        Array[ i ]  = - array2 ( i, yoffset );
else
    for ( int i = 0; i < indim1; i++ )
        Array[ i ]  =   array2 ( i, yoffset );


/*float              *tof             = &array2 ( 0, yoffset );
TypeD              *tod             = Array;


if ( invert )                           // optimized transfer
    for ( int e = Dim1 - 1; e >= 0; --e, tof += e >= 0 ? array2.GetDim2 () : 0, tod++ )
        *tod = - *tof;
else
    for ( int e = Dim1 - 1; e >= 0; --e, tof += e >= 0 ? array2.GetDim2 () : 0, tod++ )
        *tod =   *tof;*/
}


template <class TypeD>
void TVector<TypeD>::GetColumn ( TArray2<double> &array2, long yoffset, bool invert )
{
                                        // gracefully resize myself
if ( Dim1 == 0 )
    Resize ( array2.GetDim1 () );


int                 indim1          = min ( Dim1, array2.GetDim1 () );


if ( invert )                           
    for ( int i = 0; i < indim1; i++ )
        Array[ i ]  = - array2 ( i, yoffset );
else
    for ( int i = 0; i < indim1; i++ )
        Array[ i ]  =   array2 ( i, yoffset );


/*double             *tof             = &array2 ( 0, yoffset );
TypeD              *tod             = Array;


if ( invert )                           // optimized transfer
    for ( int e = Dim1 - 1; e >= 0; --e, tof += e >= 0 ? array2.GetDim2 () : 0, tod++ )
        *tod = - *tof;
else
    for ( int e = Dim1 - 1; e >= 0; --e, tof += e >= 0 ? array2.GetDim2 () : 0, tod++ )
        *tod =   *tof;*/
}


template <class TypeD>
void TVector<TypeD>::SetColumn ( TArray2<float> &array2, long yoffset )     const
{
int                 indim1          = min ( Dim1, array2.GetDim1 () );


for ( int i = 0; i < indim1; i++ )
    array2 ( i, yoffset )   = Array[ i ];


/*float              *tof             = &array2 ( 0, yoffset );
TypeD              *tod             = Array;


for ( int e = Dim1 - 1; e >= 0; --e, tof += e >= 0 ? array2.GetDim2 () : 0, tod++ )
    *tof = *tod;*/
}


template <class TypeD>
void TVector<TypeD>::SetColumn ( TArray2<double> &array2, long yoffset )    const
{
int                 indim1          = min ( Dim1, array2.GetDim1 () );


for ( int i = 0; i < indim1; i++ )
    array2 ( i, yoffset )   = Array[ i ];


/*float              *tof             = &array2 ( 0, yoffset );
TypeD              *tod             = Array;


for ( int e = Dim1 - 1; e >= 0; --e, tof += e >= 0 ? array2.GetDim2 () : 0, tod++ )
    *tof = *tod;*/
}


//----------------------------------------------------------------------------
template <class TypeD>
void TVector<TypeD>::GetRow ( TArray2<float> &array2, long xoffset, bool invert )
{
                                        // gracefully resize myself
if ( Dim1 == 0 )
    Resize ( array2.GetDim2 () );


int                 indim1          = min ( Dim1, array2.GetDim2 () );


if ( invert )                           
    for ( int i = 0; i < indim1; i++ )
        Array[ i ] -= array2 ( xoffset, i );
else
    for ( int i = 0; i < indim1; i++ )
        Array[ i ]  = array2 ( xoffset, i );


/*float              *tof             = &array2 ( xoffset, 0 );
TypeD              *tod             = Array;


if ( invert )                           // optimized transfer
    for ( int e = Dim1 - 1; e >= 0; --e, tof += e >= 0 ? 1 : 0, tod++ )
        *tod = - *tof;
else
    for ( int e = Dim1 - 1; e >= 0; --e, tof += e >= 0 ? 1 : 0, tod++ )
        *tod =   *tof;
*/
}


//----------------------------------------------------------------------------
                                        // Conveniently extracting a temporal structure from a set of maps
template <class TypeD>
void TVector<TypeD>::GetRow ( TMaps &gomaps, int row )
{
int                 NumTimeFrames   = gomaps.GetNumMaps ();

                                        // gracefully resize myself
if ( Dim1 == 0 )
    Resize ( NumTimeFrames );


for ( int tf = 0; tf < NumTimeFrames; tf++ )

    Array[ tf ]     = gomaps ( tf, row );
}


template <class TypeD>
void TVector<TypeD>::SetRow ( TMaps &gomaps, int row )  const
{
int                 NumTimeFrames   = min ( gomaps.GetNumMaps (), Dim1 );


for ( int tf = 0; tf < NumTimeFrames; tf++ )

    gomaps ( tf, row )  = Array[ tf ];
}


template <class TypeD>
void TVector<TypeD>::SetRow ( TArray2<float>  &array2, int xoffset ) const
{
int                 indim2          = min ( Dim1, array2.GetDim2 () );


for ( int i = 0; i < indim2; i++ )
    array2 ( xoffset, i )   = Array[ i ];
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVector<TypeD>::ReadFile ( char* file, TStrings* tracknames )
{
if ( StringIsEmpty ( file ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0) Get file ready
bool                epformat        = IsExtensionAmong ( file, FILEEXT_TXT" "FILEEXT_EEGEP );
bool                sefformat       = IsExtensionAmong ( file, FILEEXT_EEGSEF );
bool                binformat       = IsExtensionAmong ( file, "bin" );
bool                FileBin         = sefformat || binformat;

                                        // should be a known format!
#if defined(CHECKASSERT)
bool                formatok        = epformat || sefformat || binformat;
assert ( formatok );
#endif


if ( ! CanOpenFile ( file, CanOpenFileRead ) )
    return;


ifstream            ifs ( TFileName ( file, TFilenameExtendedPath ), FileBin ? ios::binary | ios::in : ios::in );

if ( ifs.fail () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Get dimensions
char                buff[ 256 ];
ulong               dim1;
TSefHeader          sefheader;


if      ( epformat ) {
//  dim1            = CountLines   ( file );
                                        // this will swallow any textual formatting: either everything on 1 line, or 1 per line, empty lines, different # per lines etc..
    dim1            = CountTokens  ( file );

    if ( dim1 <= 0 )
        return;
    }

else if ( sefformat ) {

    ifs.read ( (char *)(&sefheader), sizeof ( sefheader ) );

    if ( ! IsMagicNumber ( sefheader.Version, SEFBIN_MAGICNUMBER1 ) )
        return;
                                        // transposed               not transposed
    dim1            = max ( sefheader.NumElectrodes, sefheader.NumTimeFrames );


    if ( tracknames ) {

        tracknames->Reset ();
                                        // read electrode names
        char            buff[ 256 ];

        for ( ulong i = 0; i < dim1 /*LinearDim*/; i++ ) {

            ifs.read ( buff, sizeof ( TSefChannelName ) );

            buff[ sizeof ( TSefChannelName ) ] = 0; // force EOS

            tracknames->Add ( buff );
            }
        } // if tracknames

    else                                // skip electrode names
        ifs.seekg ( dim1 * sizeof ( TSefChannelName ), ios::cur );

    }

else if ( binformat ) {
                                        // binary file is a dump, we can recover the dimension with the file and atom sizes
    dim1            = GetFileSize ( file ) / AtomSize ();
    }

                                        // allocate
Resize ( dim1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Read data
ResetMemory ();

float               vf;
//double              vd;


if      ( epformat ) {

    for ( int x = 0; x < Dim1; x++ )

        GetValue ( x )  = StringToDouble ( GetToken ( &ifs, buff ) );

    } // epformat

else if ( sefformat ) {
                                        // read float & convert
    for ( int i = 0; i < Dim1 /*LinearDim*/; i++ ) {
        ifs.read ( (char *) &vf, sizeof ( vf ) );
        Array[ i ]  = vf;
        }
    } // sefformat

else if ( binformat ) {
                                        // memory dump
//  for ( int i = 0; i < Dim1 /*LinearDim*/; i++ )
//      ifs.read ( (char *) &Array[ i ], AtomSize () );

    ifs.read ( (char *) Array, MemorySize () );

//      ifs.read ( (char *) &vd, sizeof ( vd ) );   // read another format
//      Array[ j ]      = vd;                       // from the saved one
    } // binformat

}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVector<TypeD>::WriteFile ( char *file, char *trackname, int numpoints )    const
{
if ( /*IsNotAllocated () ||*/ StringIsEmpty ( file ) )
    return;


numpoints   = numpoints < 0 ? Dim1 : crtl::NoMore ( (int) Dim1, numpoints );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                epformat        = IsExtensionAmong ( file, FILEEXT_TXT " " FILEEXT_EEGEP );
bool                sefformat       = IsExtensionAmong ( file, FILEEXT_EEGSEF );
bool                risformat       = IsExtensionAmong ( file, FILEEXT_RIS );
bool                binformat       = IsExtensionAmong ( file, "bin" );
bool                FileBin         = ! epformat /*sefformat || binformat*/;

                                        // should be a known format!
#if defined(CHECKASSERT)
bool                formatok        = epformat || sefformat || risformat || binformat;
assert ( formatok );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( sefformat || risformat ) {

    TExportTracks       expfile;

    StringCopy   ( expfile.Filename,    file );

    expfile.SetAtomType ( AtomTypeScalar );
    expfile.NumTracks           = risformat ? numpoints : 1;
    expfile.NumTime             = risformat ? 1         : numpoints;
    expfile.SamplingFrequency   = risformat ? 0         : Index1.IndexRatio;                    // will "rescale" to actual variable values
    if ( ! risformat )
        expfile.DateTime        = TDateTime ( 0, 0, 0, 0, 0, 0, Index1.IndexMin * 1000, 0 );    // will set the offset correctly

    if ( StringIsNotEmpty ( trackname ) ) {
        expfile.ElectrodesNames.Set ( expfile.NumTracks, ElectrodeNameSize );
        StringCopy ( expfile.ElectrodesNames[ 0 ], trackname, ElectrodeNameSize - 1 );
        }

    expfile.Begin   ();

//  expfile.Write   ( *this );

    for ( int i = 0; i < numpoints /*LinearDim*/; i++ )
        expfile.Write ( (float) Array[ i ] );

    expfile.End     ();

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


ofstream            ofs ( TFileName ( file, TFilenameExtendedPath ), FileBin ? ios::out | ios::binary : ios::out );

if ( ofs.fail () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( binformat ) {
//    ofs.write ( (char *) Array, MemorySize () );

    for ( int i = 0; i < numpoints /*LinearDim*/; i++ )
        ofs.write ( (char *) &Array[ i ], AtomSize () );

    } // binformat

else if ( epformat ) {
    ofs << StreamFormatScientific;

    for ( int i = 0; i < numpoints; i++ )
        ofs << StreamFormatFloat64 << Array[ i ] << "\n";
//      ofs << StreamFormatFloat32 << Array[ i ] << "\n";
    } // epformat
}


//----------------------------------------------------------------------------
                                        // Use TExportTracks
template <class TypeD>
void    TVector<TypeD>::Export ( TStrings* tracknames, bool transpose, char* file )  const
{
if ( /*IsNotAllocated () ||*/ StringIsEmpty ( file ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // should be a known format!
#if defined(CHECKASSERT)
bool                formatok        = GetExportType ( file, false ) != ExportTracksUnknown;
assert ( formatok );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TExportTracks       exptracks;

                                        // track names may have been given, test for the same size as the data
if ( transpose ) {
                                        // "vertical" save: good, each track keeps its original electrode name
    if ( tracknames && tracknames->NumStrings () == Dim1 ) {

        exptracks.ElectrodesNames.Set ( Dim1,   ElectrodeNameSize + 1 );

        for ( int i = 0; i < Dim1; i++ )
            StringCopy ( exptracks.ElectrodesNames[ i ], (*tracknames)[ i ], ElectrodeNameSize );
        }
    }

else {
                                        // "horizontal" save: not good, we lose the electrode names
    if ( tracknames && tracknames->NumStrings () == 1 ) {

        exptracks.ElectrodesNames.Set ( 1,   ElectrodeNameSize + 1 );

        StringCopy ( exptracks.ElectrodesNames[ 0 ], (*tracknames)[ 0 ], ElectrodeNameSize );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // transpose gives "vertical" results
StringCopy            ( exptracks.Filename, file );
exptracks.SetAtomType ( AtomTypeScalar );
exptracks.NumTracks           = transpose ? Dim1 : 1;
exptracks.NumTime             = transpose ? 1    : Dim1;
exptracks.SamplingFrequency   = 0;

                                        // this will work, transposed or not
for ( int i = 0; i < Dim1; i++ )
    exptracks.Write ( (float) Array[ i ] );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVector<TypeD>::Show ( const char *title ) const
{
if ( IsNotAllocated () ) {
    ShowMessage ( "- Empty Vector -", StringIsEmpty ( title ) ? "String List" : title );
    return;
    }


char                buff      [ 32 ];
char                localtitle[ 256 ];


for ( int i = 0; i < Dim1; i++ ) {

    StringCopy  ( localtitle, StringIsEmpty ( title ) ? "String List" : title, " / Item#", FloatToString ( buff, i + 1 ) );

    ShowMessage ( FloatToString ( buff, Array[ i ] ), localtitle );
    }

}


//----------------------------------------------------------------------------
template <class TypeD>
double  TVector<TypeD>::Sum ()  const
{
double              sum             = 0;

for ( int i = 0; i < Dim1; i++ )
    sum    += Array[ i ];

return      sum;
}


template <class TypeD>
double  TVector<TypeD>::Sum ( const TSelection& subset )    const
{
double              sum             = 0;

for ( TIteratorSelectedForward seli ( subset ); (bool) seli; ++seli )

    if ( seli() < Dim1 )

        sum    += Array[ seli() ];

return      sum;
}


template <class TypeD>
double  TVector<TypeD>::Average ()  const
{
return      Sum () / NonNull ( Dim1 );
}


template <class TypeD>
double  TVector<TypeD>::Average ( const TSelection& subset )    const
{
return      Sum ( subset ) / NonNull ( subset.NumSet () );
}


template <class TypeD>
double  TVector<TypeD>::AbsSum ()   const
{
double              sum             = 0;

for ( int i = 0; i < Dim1; i++ )
    sum    += fabs ( Array[ i ] );

return      sum;
}


template <class TypeD>
void    TVector<TypeD>::AverageReference ()
{
double              avg             = Average ();

for ( int i = 0; i < Dim1; i++ )
    Array[ i ]  -= avg;
}


template <class TypeD>
double  TVector<TypeD>::Norm2 ( bool centeraverage )    const
{
double              avg             = centeraverage ? Average () : 0;
double              sum             = 0;


if ( centeraverage )
    for ( int i = 0; i < Dim1; i++ )
        sum    += Square ( (double) Array[ i ] - avg );
else
    for ( int i = 0; i < Dim1; i++ )
        sum    += Square ( (double) Array[ i ]       );


return  sum;
}


template <class TypeD>
double  TVector<TypeD>::Norm ( bool centeraverage ) const
{
return  sqrt ( Norm2 ( centeraverage ) );
}


template <class TypeD>
void    TVector<TypeD>::SquareRoot ()
{
for ( int i = 0; i < Dim1; i++ )
    Array[ i ]  = sqrt ( Array[ i ] );
}


//----------------------------------------------------------------------------
                                        // v could be of another dimension, but not tested
template <class TypeD>
double  TVector<TypeD>::CrossCorrelation ( const TVector<TypeD> &v, int T )   const
{
double              sum             = 0;

for ( int i = 0, j = T; i < Dim1; i++, j++ )

    sum    += Array[ i ] * ( j < 0 || j >= v.Dim1 ? 0 : v.Array[ j ] );

return      sum;
}


//----------------------------------------------------------------------------
template <class TypeD>
double  TVector<TypeD>::Correlation ( const TVector<TypeD> &v, bool centeraverage )   const
{
double              avg1            = centeraverage ?   Average () : 0;
double              avg2            = centeraverage ? v.Average () : 0;
double              sum             = 0;
double              norm1           = 0;
double              norm2           = 0;
double              t1;
double              t2;


for ( int i=0; i < Dim1; i++ ) {

    t1      =   Array[ i ] - avg1;
    t2      = v.Array[ i ] - avg2;

    sum    += t1 * t2;
    norm1  += t1 * t1;
    norm2  += t2 * t2;
    }


return  sum == 0 || norm1 == 0 || norm2 == 0 ? 0 : Clip ( sum / sqrt ( norm1 * norm2 ), -1.0, 1.0 );
}


//----------------------------------------------------------------------------
                                        // Wrapper around regular Correlation, then testing for polarities
template <class TypeD>
double  TVector<TypeD>::Correlation ( const TVector<TypeD> &v, PolarityType polarity, bool centeraverage )    const
{
double              corr            = Correlation ( v, centeraverage );

return  polarity == PolarityDirect ? corr : fabs ( corr );
}


//----------------------------------------------------------------------------
                                        // Special correlation, when vector is composed of series of 3D vectors
                                        // No average reference in this case
                                        // Can optionally return a vector of the 3D vectors signs
template <class TypeD>
double  TVector<TypeD>::CorrelationDipoles  ( const TVector<TypeD> &v, PolarityType polarity, TVector<TypeD>* signs )  const
{
double              sum             = 0;
double              norm1           = 0;
double              norm2           = 0;

OmpParallelForSum ( sum, norm1, norm2 )

for ( int spi = 0; spi < Dim1 / 3; spi++ ) {
                                        // per dipole polarity check
    bool        opposite    = polarity == PolarityEvaluate  ? Get3DVector ( spi ).IsOppositeDirection ( v.Get3DVector ( spi ) )
                            : polarity == PolarityInvert    ? true
                            :                                 false;

    double      t1x     =   Array[ 3 * spi     ];
    double      t1y     =   Array[ 3 * spi + 1 ];
    double      t1z     =   Array[ 3 * spi + 2 ];
    double      t2x     = v.Array[ 3 * spi     ];
    double      t2y     = v.Array[ 3 * spi + 1 ];
    double      t2z     = v.Array[ 3 * spi + 2 ];

    if ( opposite ) {
        sum    -= t1x * t2x;
        sum    -= t1y * t2y;
        sum    -= t1z * t2z;
                                        // !signs dimension!
        if ( signs )
            (*signs)[ spi ] = -1;
        }
    else {
        sum    += t1x * t2x;
        sum    += t1y * t2y;
        sum    += t1z * t2z;
                                        // !signs dimension!
        if ( signs )
            (*signs)[ spi ] =  1;
        }

    norm1  += t1x * t1x + t1y * t1y + t1z * t1z;
    norm2  += t2x * t2x + t2y * t2y + t2z * t2z;
    }


return  sum == 0 || norm1 == 0 || norm2 == 0 ? 0 : Clip ( sum / sqrt ( norm1 * norm2 ), -1.0, 1.0 );
}


//----------------------------------------------------------------------------
template <class TypeD>
double  TVector<TypeD>::Correlation ( const TVector<TypeD> &v, const TSelection& subset, bool centeraverage )   const
{
double              avg1            = centeraverage ?   Average ( subset ) : 0;
double              avg2            = centeraverage ? v.Average ( subset ) : 0;
double              sum             = 0;
double              norm1           = 0;
double              norm2           = 0;
double              t1;
double              t2;


for ( TIteratorSelectedForward seli ( subset ); (bool) seli; ++seli )

    if ( seli() < Dim1 ) {

        t1      =   Array[ seli() ] - avg1;
        t2      = v.Array[ seli() ] - avg2;

        sum    += t1 * t2;
        norm1  += t1 * t1;
        norm2  += t2 * t2;
        }


return  sum == 0 || norm1 == 0 || norm2 == 0 ? 0 : Clip ( sum / sqrt ( norm1 * norm2 ), -1.0, 1.0 );
}


//----------------------------------------------------------------------------
                                        // Spearman Correlation is Pearson on ranked data
template <class TypeD>
double  TVector<TypeD>::CorrelationSpearman ( const TVector<TypeD> &v, bool centeraverage )   const
{
TFilterRanking<TypeD>   filterrank;

                                        // !we can totally use these temp maps, saving allocations!
TVector<TypeD>      map1 ( *this );
TVector<TypeD>      map2 ( v     );

                                        // ignoring nulls?
filterrank.Apply ( map1, RankingOptions ( RankingAccountNulls | RankingMergeIdenticals ) );
filterrank.Apply ( map2, RankingOptions ( RankingAccountNulls | RankingMergeIdenticals ) );


return  map1.Correlation ( map2, centeraverage );
}


//----------------------------------------------------------------------------
                                        // Wrapper around regular CorrelationSpearman, then testing for polarities
template <class TypeD>
double  TVector<TypeD>::CorrelationSpearman ( const TVector<TypeD> &v, PolarityType polarity, bool centeraverage )    const
{
double              corr            = CorrelationSpearman ( v, centeraverage );

return  polarity == PolarityDirect ? corr : fabs ( corr );
}


//----------------------------------------------------------------------------
                                        // Converts from Pearson to Kendall - NOT the pairwise formula
                                        // See: Lindskog, McNeil, Schmock "Kendall's Tau for Elliptical Distributions", 2001
template <class TypeD>
double  TVector<TypeD>::CorrelationKendall ( const TVector<TypeD> &v, bool centeraverage )    const
{
return  PearsonToKendall ( Correlation ( v, centeraverage ) );
}


//----------------------------------------------------------------------------
                                        // Converts from Pearson to Fisher - currently returning in the range [-3..3]
template <class TypeD>
double  TVector<TypeD>::CorrelationFisher ( const TVector<TypeD> &v, bool centeraverage )    const
{
return  PearsonToFisher ( Correlation ( v, centeraverage ) );
}


//----------------------------------------------------------------------------
                                        // Mardia (1976) / Johnson Wehrly (1977) Linear-Circular Correlation
                                        // 'this' holds the linear vector, mapcirc is the circular one
                                        // mapcos and mapsin are temp variables, courtesy of caller, to avoid reallocation on each call
                                        // results in [0..1]
template <class TypeD>
double  TVector<TypeD>::CorrelationLinearCircular ( const TVector<TypeD>& mapcirc, bool centeraverage, TVector<TypeD>& mapcos, TVector<TypeD>& mapsin )   const
{
                                        // just in case...
mapcos.Resize ( Dim1 );
mapsin.Resize ( Dim1 );

                                        // we need the cosine and sine versions of the circular data
for ( int i = 0; i < Dim1; i++ ) {
    mapcos[ i ]     = cos ( mapcirc[ i ] );
    mapsin[ i ]     = sin ( mapcirc[ i ] );
    }
                                        // ?shouldn't cos and sin be centered anyway?

                                        // compute these separate correlations
double              rcx             =        Correlation ( mapcos, centeraverage );
double              rsx             =        Correlation ( mapsin, centeraverage );
double              rcs             = mapcos.Correlation ( mapsin, centeraverage ); // ?centered?

                                        // then merge to get the linear-circular we wanted
double              circcorr        = sqrt (   ( Square ( rcx ) + Square ( rsx ) - 2 * rcx * rsx * rcs )
                                             / NonNull ( 1 - Square ( rcs ) )                            );

return  Clip ( circcorr, 0.0, 1.0 );
}


//----------------------------------------------------------------------------
                                        // Mardia (1976) Non-Parametric Linear-Circular Correlation
                                        // 'this' holds the linear vector, mapcirc is the circular one
                                        // results in [0..1]
template <class TypeD>
double  TVector<TypeD>::CorrelationLinearCircularRobust ( const TVector<TypeD>& mapcirc )  const
{
TFilterRanking<TypeD>   filterrank;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Circular vector: convert to circular ranks
TVector<TypeD>      mapc ( mapcirc );

                                        // first to linear rank
filterrank.Apply ( mapc, RankingOptions ( RankingAccountNulls | RankingMergeIdenticals ) );

                                        // then convert to circular rank
mapc   *= TwoPi; // / (double) Dim1;    // !already divided!


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Correlation between the linear and circular ranks
double              Tc              = 0;
double              Ts              = 0;

for ( int i = 1; i <= Dim1; i++ ) {
                                        // !article doesn't make use of the linearly-ranked vector!
    Tc     += i * cos ( mapc[ i ] );
    Ts     += i * sin ( mapc[ i ] );
    }

                                        // 3) Final formula, with scaling factors
double              pin             = Pi / (double) Dim1;

double              an              = IsEven ( Dim1 ) ? 1 / ( 1 + 5 * Square ( Cotangent ( pin ) ) + 4 * Power ( Cotangent ( pin ), 4 ) )
                                                      : ( 2 * Power ( sin ( pin ), 4 ) ) / Cube ( 1 + cos ( pin ) );

                                        // Dn in [0..1], invariant in translation for linear variable, invariant in rotation for circular variable
double              Dn              = an * ( Square ( Tc ) + Square ( Ts ) );

                                        // Converges to Chi Square with 2 degrees of freedom
                                        // Does NOT scale in [0..1]
//double              Un              = 24 * ( Square ( Tc ) + Square ( Ts ) ) / ( Square ( Dim1 ) * ( Dim1 + 1 ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  Clip ( Dn, 0.0, 1.0 );
}


//----------------------------------------------------------------------------
                                        // How meaningful is the correlation
template <class TypeD>
double  TVector<TypeD>::CorrelationToP ( const TVector<TypeD> &v, bool centeraverage )    const
{
if ( Dim1 < 3 )
    return  1;


double              rho             = Correlation ( v, centeraverage );

if ( fabs ( rho ) == 1 )
    return  0;


double              dof             = Dim1 - 2;
double              t               = rho / sqrt ( ( 1 - Square ( rho ) ) / dof );
double              p               = Student_TwoTails_t_to_p ( t, dof );   // correct dof?


return  p;
}

                                        // Correlation to Normal Z
template <class TypeD>
double  TVector<TypeD>::CorrelationToZ ( const TVector<TypeD> &v, bool centeraverage )    const
{
if ( Dim1 < 2 )
    return  0;


double              rho             = Correlation ( v, centeraverage ) * 0.99;

if ( rho == 1 )
    return   1e30; //  DBL_MAX;

if ( rho == -1 )
    return  -1e30; // -DBL_MAX;


//double              z               = atanh ( rho );
double              z               = log ( ( 1 + rho ) / ( 1 - rho ) ) / 2;


return  z;
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVector<TypeD>::ToRank ( RankingOptions options )
{
TFilterRanking<TypeD>   filterrank;

filterrank.Apply ( *this, options );
}


//----------------------------------------------------------------------------
template <class TypeD>
double  TVector<TypeD>::Dissimilarity ( const TVector<TypeD> &v, bool centeraverage, bool vectorial )     const
{
double              avg1            = centeraverage ?   Average () : 0;
double              avg2            = centeraverage ? v.Average () : 0;
double              gfp1            = NonNull (   GlobalFieldPower ( centeraverage, vectorial ) );
double              gfp2            = NonNull ( v.GlobalFieldPower ( centeraverage, vectorial ) );
double              sum             = 0;


for ( int i = 0; i < Dim1; i++ )
                                        // Euclidean distance between standardized vectors
    sum    += Square ( (   Array[ i ] - avg1 ) / gfp1 
                     - ( v.Array[ i ] - avg2 ) / gfp2 );


return  Clip ( sqrt ( sum / Dim1 * ( vectorial ? 3 : 1 ) ), 0.0, 2.0 );
}


template <class TypeD>
double  TVector<TypeD>::Dissimilarity ( const TVector<TypeD> &v, const TSelection& subset, bool centeraverage, bool vectorial )     const
{
double              avg1            = centeraverage ?   Average ( subset ) : 0;
double              avg2            = centeraverage ? v.Average ( subset ) : 0;
double              gfp1            = NonNull (   GlobalFieldPower ( subset, centeraverage, vectorial ) );
double              gfp2            = NonNull ( v.GlobalFieldPower ( subset, centeraverage, vectorial ) );
double              sum             = 0;


for ( TIteratorSelectedForward seli ( subset ); (bool) seli; ++seli )

    if ( seli() < Dim1 )
                                        // Euclidean distance between standardized vectors
        sum    += Square ( (   Array[ seli() ] - avg1 ) / gfp1 
                         - ( v.Array[ seli() ] - avg2 ) / gfp2 );


return  Clip ( sqrt ( sum / Dim1 * ( vectorial ? 3 : 1 ) ), 0.0, 2.0 );
}

                                        // Wrapper around regular Correlation, then testing for polarities
template <class TypeD>
double  TVector<TypeD>::Dissimilarity ( const TVector<TypeD> &v, PolarityType polarity, bool centeraverage )  const
{
double              corr            = Correlation ( v, polarity, centeraverage );

return  Clip ( CorrelationToDifference ( corr ), 0.0, 2.0 );
}


//----------------------------------------------------------------------------
template <class TypeD>
double  TVector<TypeD>::ExpVar ( const TVector<TypeD> &v, bool centeraverage )    const
{
return      Square ( Correlation ( v, centeraverage ) );
}


//----------------------------------------------------------------------------
template <class TypeD>
double  TVector<TypeD>::GlobalFieldPower ( bool centeraverage, bool vectorial )     const
{
double              avg             = centeraverage ? Average () : 0;
double              sum             = 0;


for ( int i = 0; i < Dim1; i++ )

    sum    += Square ( Array[ i ] - avg );

                                        // vectorial: array dimension is 3 times the number of vectors, so correct for that
return      sqrt ( sum / Dim1 * ( vectorial ? 3 : 1 ) );
}


template <class TypeD>
double  TVector<TypeD>::GlobalFieldPower ( const TSelection& subset, bool centeraverage, bool vectorial )     const
{
double              avg             = centeraverage ? Average ( subset ) : 0;
double              sum             = 0;
int                 num             = 0;


for ( TIteratorSelectedForward seli ( subset ); (bool) seli; ++seli )

    if ( seli() < Dim1 ) {

        sum    += Square ( Array[ seli() ] - avg );
        num++;
        }

return  sum == 0 || num == 0 ? 0 : sqrt ( sum / num * ( vectorial ? 3 : 1 ) );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVector<TypeD>::Invert ()
{
//if ( !Array )
//    return;

for ( int i = 0; i < Dim1; i++ )

    Array[ i ]   = (TypeD) - Array[ i ];
}


template <class TypeD>
double  TVector<TypeD>::MaxCrossCorrelation ( const TVector<TypeD> &v, int &T )   const
{
double              cc;
double              maxcc           = CrossCorrelation ( v, 0 );

T = 0;

for ( int t = -Dim1; t < v.Dim1; t++ ) {
    cc = CrossCorrelation ( v, t );
    if ( cc > maxcc ) {
        maxcc = cc;
        T = t;
        }
    }

return      maxcc;
}


template <class TypeD>
void    TVector<TypeD>::Maxed ( const TVector<TypeD> &v )
{
for ( int i = 0; i < Dim1; i++ )
    crtl::Maxed ( Array[ i ], v[ i ] );
}


template <class TypeD>
void    TVector<TypeD>::Mined ( const TVector<TypeD> &v )
{
for ( int i = 0; i < Dim1; i++ )
    crtl::Mined ( Array[ i ], v[ i ] );
}


template <class TypeD>
void    TVector<TypeD>::Clipped ( TypeD minv, TypeD maxv )
{
for ( int i = 0; i < Dim1; i++ )
    crtl::Clipped ( Array[ i ], minv, maxv );
}


template <class TypeD>
void    TVector<TypeD>::AtLeast ( TypeD minv )
{
for ( int i = 0; i < Dim1; i++ )
    crtl::Maxed ( Array[ i ], minv );
}


template <class TypeD>
void    TVector<TypeD>::NoMore ( TypeD maxv )
{
for ( int i = 0; i < Dim1; i++ )
    crtl::Mined ( Array[ i ], maxv );
}


template <class TypeD>
void    TVector<TypeD>::Normalize ( bool centeraverage )
{
if ( centeraverage )
    AverageReference ();


double              n               = Norm ();

if ( n == 0 ) return;

for ( int i = 0; i < Dim1; i++ )
    Array[ i ]  /= n;
}


template <class TypeD>
void    TVector<TypeD>::Normalize1 ()
{
double              s               = AbsSum ();

if ( s == 0 ) return;

for ( int i = 0; i < Dim1; i++ )
    Array[ i ]  /= s;
}


template <class TypeD>
void    TVector<TypeD>::ThresholdAbove ( TypeD t )
{
for ( int i = 0; i < Dim1; i++ )
    if ( Array[ i ] < t )
        Array[ i ]  = 0;
}


template <class TypeD>
void    TVector<TypeD>::ThresholdBelow ( TypeD t )
{
for ( int i = 0; i < Dim1; i++ )
    if ( Array[ i ] > t )
        Array[ i ]  = 0;
}


template <class TypeD>
void    TVector<TypeD>::Absolute ()
{
for ( int i = 0; i < Dim1; i++ )
    Array[ i ]  = fabs ( Array[ i ] );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVector<TypeD>::ZScore ( double center, double spreadleft, double spreadright )
{
for ( int i = 0; i < Dim1; i++ )
    Array[ i ]  = crtl::ZScoreAsym ( Array[ i ], center, spreadleft, spreadright );
}


template <class TypeD>
void    TVector<TypeD>::ZScore ()
{
TEasyStats          stat ( *this, true );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Robust estimate of max mode
TEasyStats          statcenter ( NumMaxModeRobustEstimates );

stat.MaxModeRobust ( statcenter );

double              center          = statcenter.Median ( false );

//statcenter.Show ( "TVector::ZScore  Center" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Bunch of estimates for  spreading
//double            s0              = stat.SD ();                   //
//double            s1              = stat.InterQuartileRange ();   // Robust, doesn't need a center
double              s3              = stat.Sn ( 1000 );             // Robust, doesn't need a center
double              s4              = stat.Qn ( 1000 );             // Robust, doesn't need a center - smaller values than Sn
double              s2              = stat.MAD ( CanAlterData );    // Robust, doesn't need a center

double              spread          = ( s2 + s3 + s4 ) / 3;

//DBGV6 ( s0, s1, s2, s3, s4, spread, "TVector::ZScore:  SD IQR MAD Sn Qn -> spread" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < Dim1; i++ )

    Array[ i ]  = crtl::ZScore ( Array[ i ], center, spread );
}


template <class TypeD>
void    TVector<TypeD>::ZScoreAsym ()
{
TEasyStats          stat ( *this, true );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Robust estimate of max mode
TEasyStats          statcenter ( NumMaxModeRobustEstimates );

stat.MaxModeRobust ( statcenter );

double              center          = statcenter.Median ( false );

//statcenter.Show ( "TVector::ZScoreAsym  Center" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Bunch of estimates for (asymmetrical) spreading
double              sdleft;
double              sdright;
double              madleft;
double              madright;


stat.SDAsym  ( center, sdleft, sdright );

stat.MADAsym ( center, madleft, madright );


double              spreadleft      = (     sdleft
                                      + 2 * madleft
                                      ) / 3;

double              spreadright     = (     sdright
                                      + 2 * madright
                                      ) / 3;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < Dim1; i++ )

    Array[ i ]  = crtl::ZScoreAsym ( Array[ i ], center, spreadleft, spreadright );
}

                                        // Z-Score done on the positive part only, resetting the negative part
                                        // Optionally can do the absolute beforehand
template <class TypeD>
void    TVector<TypeD>::ZScorePositive ( bool absolute )
{
TEasyStats          stat ( Dim1 );

for ( int i = 0; i < Dim1; i++ )

    if ( absolute || Array[ i ] > 0 )

        stat.Add ( fabs ( Array[ i ] ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Bunch of estimates for (asymmetrical) spreading
double              sdleft;
double              sdright;
double              madleft;
double              madright;


stat.SDAsym  ( 0, sdleft, sdright );

stat.MADAsym ( 0, madleft, madright );


double              spreadright     = (     sdright
                                      + 2 * madright
                                      ) / 3;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < Dim1; i++ )

    Array[ i ]  = absolute || Array[ i ] > 0 ? crtl::ZScore ( fabs ( Array[ i ] ), 0, spreadright ) 
                                             : 0;
}


template <class TypeD>
void    TVector<TypeD>::ZScoreAbsolute ()
{
ZScore ();

Absolute ();
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVector<TypeD>::Random  ( 
                                double          minv, 
                                double          maxv,
                                TRandUniform*   randunif
                                )
{
TRandUniform*       torandunif      = randunif ? randunif : new TRandUniform;


for ( int i = 0; i < Dim1; i++ )

    Array[ i ]   = (TypeD) (*torandunif) ( minv, maxv );


if ( ! randunif )
    delete torandunif;
}


//----------------------------------------------------------------------------
                                        // Non-repeating series of n indexes across max
                                        // Knuth-Fisher-Yates shuffling algorithm, https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle
                                        // Returns the n first entries with the random, non-repeating indexes in [0..size)
template <class TypeD>
void    TVector<TypeD>::RandomSeries    (
                                        int             numwished,
                                        int             maxseries,
                                        TRandUniform*   randunif
                                        )
{
                                        // safety allocation
Resize ( crtl::AtLeast ( Dim1, maxseries ), ResizeNoReset );

ResetMemory ();

                                        // safety check
crtl::Clipped ( numwished, 0, maxseries );
                                        // handling special case
if ( numwished == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // optional parameter
TRandUniform*       torandunif      = randunif ? randunif : new TRandUniform;

                                        // edge case for 1 sample is straightforward
if ( numwished == 1 ) {
    Array[ 0 ]  = (*torandunif) ( (UINT) maxseries );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // initialize with the range of all possible indexes [0..maxseries) - trailing with 0's
for ( int i = 0; i < maxseries; i++ )
    Array[ i ]  = i;

                                        // do the increasing index variant, so we can stop after the first n are done
for ( int i = 0; i <= maxseries - 2 && i < numwished; i++ ) {
                                        // r in [i..maxseries)
    int         r       = i + (*torandunif) ( (UINT) ( maxseries - i ) );
                                        // spare a useless permutation
    if ( r != i )
        Permutate ( Array[ i ], Array[ r ] );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! randunif )
    delete torandunif;
}


//----------------------------------------------------------------------------
                                        // in case the SD is the same across all dimensions
template <class TypeD>
void    TVector<TypeD>::AddGaussianNoise ( double sigmadata, double percentsnr )
{
if ( sigmadata <= 0 || percentsnr <= 0 )
    return;


static TRandNormal  randnormal;

                                        //  SNR   %Variance
                                        //   0      100%
                                        //   0.5     90%
                                        //   1       80%
                                        //   1.5     70%
                                        //   2.2     60%
                                        //   3       50%
                                        //   4       40%
                                        //   5.2     30%
                                        //   7       20%
                                        //  10       10%
                                        //  13        5%
                                        //  20        1%

                                        // Going from Variance to SD will need a sqrt - but it makes the noise not linear
double              sigmanoise      = sigmadata * percentsnr; // sqrt ( percentsnr );

randnormal.Set ( 0, sigmanoise );

for ( int i = 0; i < Dim1; i++ )

    Array[ i ] += (TypeD) randnormal ();
}

                                        // in case each dimension has its own SD (not what we really expect)
template <class TypeD>
void    TVector<TypeD>::AddGaussianNoise ( TVector<TypeD>& sigmadata, double percentsnr )
{
if ( sigmadata.IsNotAllocated () || percentsnr <= 0 )
    return;


static TRandNormal  randnormal;

double              sigmanoise;


for ( int i = 0; i < Dim1; i++ ) {

    if ( sigmadata[ i ] <= 0 )
        continue;

    sigmanoise  = sigmadata[ i ] * percentsnr; // sqrt ( percentsnr );
                                        // !or simply rescale a distribution (0,1)!
    randnormal.Set ( 0, sigmanoise );

    Array[ i ] += (TypeD) randnormal ();
    }
}

                                        // by averaging
template <class TypeD>
void    TVector<TypeD>::Smooth ()
{
if ( ! Array || Dim1 < 3 )
    return;

TVector<TypeD>      temp ( *this );

Array[0]        = (TypeD) ( ( temp[0]        + temp[1]        ) / 2 );
Array[Dim1 - 1] = (TypeD) ( ( temp[Dim1 - 1] + temp[Dim1 - 2] ) / 2 );

for ( int i = 1; i < Dim1 - 1; i++ )
    Array[i]     = (TypeD) ( ( temp[i] + temp[i - 1] + temp[i + 1] ) / 3 );
}

                                        // scalar product. No tests on dimensions
template <class TypeD>
double  TVector<TypeD>::ScalarProduct ( const TVector<TypeD> &v )     const
{
double              sum             = 0;

for ( int i = 0; i < Dim1; i++ )
    sum    += Array[ i ] * v.Array[ i ];

return  sum;
}

                                        // Wrapper around regular ScalarProduct, then testing for polarities
template <class TypeD>
double  TVector<TypeD>::ScalarProduct ( const TVector<TypeD> &v, PolarityType polarity )    const
{
double              scalprod        = ScalarProduct ( v );

return  polarity == PolarityDirect ? scalprod : fabs ( scalprod );
}

                                        // the scalar product always boils down to the the angle in the plan formed by the 2 vectors
template <class TypeD>
bool    TVector<TypeD>::IsOppositeDirection ( const TVector<TypeD> &op2 )   const
{
return  ScalarProduct ( op2 ) < 0;
}


template <class TypeD>
PolarityType    TVector<TypeD>::GetPolarity ( const TVector<TypeD> &op2 )   const
{
return  IsOppositeDirection ( op2 ) ? PolarityInvert : PolarityDirect;
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVector<TypeD>::MapFromDipole   (   double          centerx,        double  centery,    double  centerz,
                                            double          precession,     double  nutation,   
                                            bool            normalized,     bool    centeraverage,
                                            const TPoints&  points,
                                            const TMatrix44&    standardorient
                                        )
{
                                        // needs +90 degrees to be in front: X points to the right, rotate to have it point forward
TMatrix44           euler ( precession + 90, nutation, 0 );

TMatrix44           transform ( euler * standardorient );

transform.Invert ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TVector3Float       dipole ( 0, 0, 1 );
                                        // transform is only for the direction of dipole
transform.Apply ( dipole );
                                        // shouldn't be needed if transform has no scaling
//dipole.Normalize ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Resize ( (int) points );

TVector3Float       toel;
double              d;

                                        // value is projection on Z axis
for ( int i = 0; i < Dim1; i++ )
                                        // direction only
//    Array[ i ]  = points[ i ].ScalarProduct ( dipole );
                                        // direction only, with translation
//    Array[ i ]  = ( points[ i ] - TPointFloat ( centerx, centery, centerz ) ).ScalarProduct ( dipole );
    {
    toel        = points[ i ] - TPointFloat ( centerx, centery, centerz );

    d           = toel.Norm ();
                                               // normalize ScalarProduct & divide by distance²
    Array[ i ]  = toel.ScalarProduct ( dipole ) / NonNull ( Cube ( d ) );
    }
    

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( normalized )

    Normalize ( centeraverage );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVector<TypeD>::MapFromLeadField (  TTracks<float>&     K,              int         numsources,     
                                            TPoints*            solp,           double      spreadmax,      double  axisvariability,
                                            bool                centeraverage,  bool        normalize,
                                            TMap*               brainstate )
{
int                 numel           = K.GetDim1 ();
int                 numsolp3        = K.GetDim2 ();
int                 numsolp         = numsolp3 / 3;


TMap                spvalues        ( numsolp3 );
TArray1<bool>       sourceindexes   ( numsolp  );
int                 positioni;
TVector3Float       sourcedir;
double              sourcepower;
TVector3Double      sourcespread;
TVector3Double      delta;
double              gaussian;
bool                dospread        = solp != 0 && spreadmax > 0;
static TRandUniform     randunif;   // this one is static, because reloading at the millisecond can repeat the seame seed in case of multiple close calls
static TRandSpherical   randsph;


//Resize ( numel );                     // !not working when called with a TGoMap[ i ]!
sourceindexes   = false;

                                        // sum up many sources
for ( int sourcei = 0; sourcei < numsources; sourcei++ ) {
                                        // get a random position index
                                        // also check for non-duplicate positions
    do      positioni   = randunif ( (UINT) numsolp );
    while ( sourceindexes[ positioni ] );

                                        // remember this position for the next points
    sourceindexes[ positioni ]  = true;

                                        // get a random direction
    sourcedir       = randsph ();

                                        // intensity scaling:
//  sourcepower     = 1;                            // none
    sourcepower     = 1 / (double) ( sourcei + 1 ); // gives a bit more of a realistic vibe
//  sourcepower     = 1 - ( (double) sourcei / numsources ) * 0.25; // less damping


    if ( dospread ) {
                                        // spreading is proportional power: strong <-> wide, weak <-> narrow
//      sourcespread    = sourcepower * spreadmax;
                                        // add a bit of variability on each axis - take care to NOT go beyond current max spreading
        sourcespread.X  = sourcepower * spreadmax * ( axisvariability != 0.0 ? randunif ( 1 - axisvariability, 1.00 ) : 1 );
        sourcespread.Y  = sourcepower * spreadmax * ( axisvariability != 0.0 ? randunif ( 1 - axisvariability, 1.00 ) : 1 );
        sourcespread.Z  = sourcepower * spreadmax * ( axisvariability != 0.0 ? randunif ( 1 - axisvariability, 1.00 ) : 1 );

                                        // spread current position with some Gaussian
        for ( int spi = 0; spi < numsolp; spi++ ) {

                                        // how far from the center of blob?
    //      d   = ( *solp[ spi ] - *solp[ positioni ] ).Norm ();
    //                                  // !all contributions cumulate!
    //      Array[ spi ]   += Gaussian ( d, 0, sourcespread, sourcepower );


            delta           = (*solp)[ spi ] - (*solp)[ positioni ];

            delta          /= sourcespread; // !could be improved by normalizing by SP spacing!

                                        // centering is done, we have a distance vector, and scaling has been done per each axis
            gaussian        = Gaussian ( delta.Norm (), 0, 1, sourcepower );

            spvalues[ 3 * spi + 0 ]    += gaussian * sourcedir.X;
            spvalues[ 3 * spi + 1 ]    += gaussian * sourcedir.Y;
            spvalues[ 3 * spi + 2 ]    += gaussian * sourcedir.Z;
            }
        } // dospread

    else {
                                        // simply cumulate the bare dipole to global brain state
        spvalues[ 3 * positioni + 0 ]  += sourcepower * sourcedir.X;
        spvalues[ 3 * positioni + 1 ]  += sourcepower * sourcedir.Y;
        spvalues[ 3 * positioni + 2 ]  += sourcepower * sourcedir.Z;
        }

    } // for sourcei


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // apply lead field
TArray1<TMapAtomType>      map ( K * spvalues );

                                        // transfer results - it looks like we are missing some assignation operators here...
for ( int i = 0; i < numel; i++ )
    Array[ i ]  = map[ i ];

                                        // post-processing
if ( centeraverage )
    AverageReference ();


if ( normalize )
    Normalize ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( brainstate ) {
    for ( int i = 0; i < numsolp3; i++ )
        (*brainstate)[ i ]  = spvalues[ i ];
    }
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVector<TypeD>::RisFromSolutionPoints   (   const TPoints&      solp,       int         numsources,
                                                    double              spreadmax,  double      axisvariability,
                                                    bool                normalize
                                                )
{
int                 numsolp         = solp.GetNumPoints ();

TArray1<bool>       sourceindexes ( numsolp );
int                 positioni;
double              sourcepower;
//double              sourcespread;
//double              d;
TVector3Double      sourcespread;
TVector3Double      delta;
static TRandUniform randunif;   // this one is static, because reloading at the millisecond can repeat the seame seed in case of multiple close calls


crtl::Clipped ( axisvariability, 0.0, 1.0 );


//Resize ( numel );                     // !not working when called with a TGoMap[ i ]!
sourceindexes   = 0;

                                        // sum up many sources
for ( int sourcei = 0; sourcei < numsources; sourcei++ ) {
                                        // get a random position index
                                        // also check for non-duplicate positions
    do      positioni   = randunif ( (UINT) numsolp );
    while ( sourceindexes[ positioni ] );

                                        // remember this position for the next points
    sourceindexes[ positioni ]  = true;

                                        // intensity scaling:
//  sourcepower     = 1;                                            // none
//  sourcepower     = 1 / (double) ( sourcei + 1 );                 // gives a bit more of a realistic vibe
    sourcepower     = 1 - ( (double) sourcei / numsources ) * 0.25; // less damping

                                        // make the spreading also proportional to the current power
//  sourcespread    = sourcepower * spreadmax;
                                        // add a bit of variability on each axis - take care to NOT go beyond current max spreading
    sourcespread.X  = sourcepower * spreadmax * ( axisvariability != 0.0 ? randunif ( 1 - axisvariability, 1.00 ) : 1 );
    sourcespread.Y  = sourcepower * spreadmax * ( axisvariability != 0.0 ? randunif ( 1 - axisvariability, 1.00 ) : 1 );
    sourcespread.Z  = sourcepower * spreadmax * ( axisvariability != 0.0 ? randunif ( 1 - axisvariability, 1.00 ) : 1 );

                                        // spread current position with some Gaussian
    for ( int spi = 0; spi < numsolp; spi++ ) {

                                        // how far from the center of blob?
//      d   = ( *solp[ spi ] - *solp[ positioni ] ).Norm ();
//                                      // !all contributions cumulate!
//      Array[ spi ]   += Gaussian ( d, 0, sourcespread, sourcepower );


        delta           = solp[ spi ] - solp[ positioni ];

        delta          /= sourcespread;
                                        // !all contributions cumulate!
                                        // centering is done, we have a distance vector, and scaling has been done per each axis
        Array[ spi ]   += Gaussian ( delta.Norm (), 0, 1, sourcepower );
        }

    } // for sourcei


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // post-processing
if ( normalize )
    Normalize ();

}


//----------------------------------------------------------------------------
                                        // !NOT for vectorial inverse!
template <class TypeD>
bool    TVector<TypeD>::IsLocalMax  (   const TArray2<int>&     neighbi,    
                                        int             spi,    
                                        double          mincentervalue,         // min absolute value to be a local max
                                        double          epsilonneighbor         // relative (to center) threshold that all neighbors have to be above
                                    )   const
{
if ( neighbi.IsNotAllocated () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numneigh        = neighbi ( spi, 0 );
                                        // any neighbors?
if ( numneigh == 0 )

    return  false;

                                        // actual center value
double              vcenter         = Array[ spi ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // mincentervalue > 0 used to look at values above a background threshold
                                        // mincentervalue = 0 to ignore and use all data
if ( mincentervalue > 0 && vcenter < mincentervalue )

    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // epsilonneighbor > 0 used to enforce stronger neighborhood
                                        // epsilonneighbor < 0 used to relax neighborhood constraints
                                        // epsilonneighbor = 0 to ignore
if ( epsilonneighbor )

    vcenter    *= 1 - epsilonneighbor;  // center lower  -> neighbor needs to discriminate more
//  vcenter    -= epsilonneighbor;      // center lower  -> neighbor needs to discriminate more

                                        // scanning every neighbor from that solution points
for ( int ni = 1; ni <= numneigh; ni++ )
                                        // this will stop at the first neighbor above center
    if ( Array[ neighbi ( spi, ni ) ] > vcenter )
        
        return  false;

return  true;
}


//----------------------------------------------------------------------------
                                        // Sum of all differences of neighbors toward center
                                        // A bit similar to a Laplacian, except signs don't cancel each other
/*template <class TypeD>
double  TVector<TypeD>::SumDeltasCenter     (   const TArray2<int>&     neighbi,    
                                                int             spi
                                            )   const
{
int                 numneigh        = neighbi ( spi, 0 );
                                        // any neighbors?
if ( numneigh == 0 )

    return  0;

                                        // actual center value
double              vcenter         = Array[ spi ];
double              lapl            = 0;

                                        // scanning every neighbor from that solution points
for ( int ni = 1; ni <= numneigh; ni++ )
                                        // "inverted" subtraction, so we have positive answers for local maxes
    lapl   += vcenter - Array[ neighbi ( spi, ni ) ];

lapl   /= numneigh;


return  lapl;
}
*/

//----------------------------------------------------------------------------
                                        // !NOT for vectorial inverse!
template <class TypeD>
int     TVector<TypeD>::GetLocalMaxes   (   const TArray2<int>&     neighbi,
                                            double                  mincentervalue,     double                  epsilonneighbor,
                                            int                     maxmaxes,
                                            TArray1<int>&           localmaxes
                                        )   const
{
if ( neighbi.IsNotAllocated () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                        // Look only at local max inverse
//for ( int spi = 0; spi < Dim1; spi++ )
//
//    if ( IsLocalMax ( neighbi, spi, mincentervalue, epsilonneighbor ) ) {
//
//        localmaxes[ numlocalmaxes++ ]   = spi;
//                                        // we can optionally stop at a specified number of local maxes
//        if ( numlocalmaxes == maxmaxes )
//            break;
//        }
//
//
//return  numlocalmaxes;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Build a list of inverse local maxes - can't be more than half points
TArray2<float>      lmxs ( Dim1, 2 );
int                 numlocalmaxes   = 0;

                                        
for ( int spi = 0; spi < Dim1; spi++ )
                                        // gather all local maxes
    if ( IsLocalMax ( neighbi, spi, mincentervalue, epsilonneighbor ) ) {

        lmxs ( numlocalmaxes, 0 )   = spi;

        lmxs ( numlocalmaxes, 1 )   = Array[ spi ]; // center value as criterion

        numlocalmaxes++;
        }

                                        // sort them by criterion
lmxs.SortRows ( 1, Descending, numlocalmaxes );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // optionally clipping the number of returned local maxes
if ( maxmaxes > 0 )

    crtl::Mined ( numlocalmaxes, maxmaxes );


localmaxes.Resize ( numlocalmaxes );

                                        // copy the first local maxes
for ( int lmi = 0; lmi < numlocalmaxes; lmi++ )

    localmaxes ( lmi )  = lmxs ( lmi, 0 );


return  numlocalmaxes;
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVector<TypeD>::GetScalarMap ( TMap& scalarmap, bool isvectorial )    const
{
if ( scalarmap.IsNotAllocated () )

    scalarmap.Resize ( isvectorial ? Dim1 / 3 : Dim1 );


if ( isvectorial )
                                        // !making sure we don't leak past Dim1!
    for ( int spi3 = 2, spi = 0; spi3 < Dim1; spi3 += 3, spi++ )
                                        // transfer from vectorial to scalar, data is multiplexed as: x, y, z, x, y, z...
        scalarmap ( spi )   = NormVector3 ( &Array [ spi3 - 2 ] );
else

    scalarmap   = *this;
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVector<TypeD>::Apply ( const TArray2<TypeD>& m )
{
if ( m.GetDim1 () != Dim1 || m.GetDim2 () != Dim1 )
    return;


TVector<TypeD>      vo ( *this );
double              s;


for ( int i1 = 0; i1 < Dim1; i1++ ) {

    s           = 0;

    for ( int i2 = 0; i2 < Dim1; i2++ )

        s      += m ( i1, i2 ) * vo[ i2 ];

    Array[ i1 ] = (TypeD) s;
    }

}


//----------------------------------------------------------------------------
template <class TypeD>
double  TVector<TypeD>::Difference ( const TVector<TypeD> &v, bool invert )   const
{
double              norm            = 0;


if ( invert )
    for ( int i = 0; i < Dim1; i++ )   
        norm   += Square ( Array[ i ] + v.Array[ i ] );
else
    for ( int i = 0; i < Dim1; i++ )   
        norm   += Square ( Array[ i ] - v.Array[ i ] );


return  sqrt ( norm );
}


template <class TypeD>
double  TVector<TypeD>::SquaredDifference ( const TVector<TypeD> &v, bool invert )    const
{
double              norm            = 0;


if ( invert )
    for ( int i = 0; i < Dim1; i++ )   
        norm   += Square ( Array[ i ] + v.Array[ i ] );
else
    for ( int i = 0; i < Dim1; i++ )   
        norm   += Square ( Array[ i ] - v.Array[ i ] );


return  norm;
}


template <class TypeD>
double  TVector<TypeD>::SquaredDifference ( const TVector<TypeD> &v, PolarityType polarity )  const
{
if ( polarity == PolarityEvaluate )
    polarity    = IsOppositeDirection ( v ) ? PolarityInvert : PolarityDirect;


return  SquaredDifference ( v, PolarityInvertToBool ( polarity ) );
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TVector<TypeD>::Cumulate ( const TVector<TypeD>& v, double weight )
{
for ( int i = 0; i < Dim1; i++ )
    Array[ i ]  += weight * v.Array[ i ];
}


template <class TypeD>
void    TVector<TypeD>::Cumulate ( const TVector<TypeD>& v, bool invert )
{
if ( invert )   (*this)    -= v;
else            (*this)    += v;
}


template <class TypeD>
void    TVector<TypeD>::Cumulate ( const TVector<TypeD>& v, PolarityType polarity )
{
                                        // test direction against itself - still OK if it is 0
if ( polarity == PolarityEvaluate && IsOppositeDirection ( v ) )    (*this)    -= v;
else                                                                (*this)    += v;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
