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


#include    "Math.Utils.h"
#include    "TArray1.h"
#include    "TArray2.h"
#include    "TArray3.h"
#include    "TVector.h"
#include    "CartoolTypes.h"

namespace crtl {

//----------------------------------------------------------------------------

enum            HistogramOptions
                {
                NoHistogramOptions      = 0x000000,
                                        // Main type of histogram output:
                HistogramPDF            = 0x000001,
                HistogramCDF            = 0x000002,         // !will force HistogramNormMax anyway!
                HistogramTypeMask       = HistogramPDF  | HistogramCDF,

                HistogramRaw            = 0x000010,         // buckets raw count, no smoothing
                HistogramSmooth         = 0x000020,         // filtered results, usually used together with subsampling
                HistogramSmoothMask     = HistogramRaw  | HistogramSmooth,

                HistogramCount          = 0x000100,
                HistogramNormNone       = HistogramCount,   // equivalent, just to make it clearer what caller wants
                HistogramNormMax        = 0x000200,
                HistogramNormArea       = 0x000400,
                HistogramNormMask       = HistogramCount  | HistogramNormNone | HistogramNormMax | HistogramNormArea,

                HistogramLinear         = 0x001000,         // curve results linear
                HistogramLog            = 0x002000,         // curve results log - currently can be used as CDF ( log data )
                HistogramIgnoreNulls    = 0x004000,         // a good idea to ignore nulls in case of a big background that will bias the global histogram
                HistogramAbsolute       = 0x008000,         // using both abs ( negative ) and positive data as a single dataset
                HistogramIgnorePositive = 0x010000,         // we may need to focus on only the negative part
                HistogramIgnoreNegative = 0x020000,         // we may need to focus on only the positive part
                HistogramDiscrete       = 0x040000,         // discrete values, like ROIs / labels
                HistogramContinuous     = 0x080000,         // continuous values
                HistogramCompMask       = HistogramLinear | HistogramLog | HistogramIgnoreNulls | HistogramAbsolute | HistogramIgnorePositive | HistogramIgnoreNegative | HistogramDiscrete | HistogramContinuous,

                HistogramOptionsMask    = HistogramTypeMask | HistogramSmoothMask | HistogramNormMask | HistogramCompMask,

                HistogramDefaultOptions = HistogramPDF | HistogramSmooth | HistogramNormNone | HistogramLinear,
                HistogramCDFOptions     = HistogramCDF | HistogramRaw    | HistogramNormMax  | HistogramLinear | HistogramIgnoreNulls,
                };

                                        // Specify if positions are specified as bin / index or real value
enum            HistoUnit
                {
                BinUnit,
                RealUnit,
                };

                                        // Initialization default values
//constexpr double  HistoDefaultKernelDensity   = 0;
//constexpr double  HistoDefaultMinData         = 0;
//constexpr double  HistoDefaultMaxData         = 0;
                                        // 3 -> 99% of the Gaussian
constexpr double    HistoMarginFactor           = 3;
constexpr double    HistoKernelSub              = 3;

constexpr double    HistoMinSize                = 3.0;

                                        // MRI background parameters
constexpr int       BackgroundMaxSamples        = 1000000;
constexpr double    MaskBackgroundValue         = 1.0;
constexpr double    RankBackgroundValue         = 0.04;
constexpr double    RISBackgroundValue          = 1e-6;
constexpr double    ErrorBackgroundValue        = 10;
constexpr double    RoundBackgroundValue        = 10000;


//----------------------------------------------------------------------------

class   TEasyStats;


class   THistogram  :   public  TVector<double>
{
public:
                    THistogram  ();

                    THistogram  ( const THistogram &op  );
                                        // Resized & empty histogram
                    THistogram  ( int numbins );
                                        // General purpose constructor, from some TEasyStats object
                    THistogram  (   TEasyStats&             stats, 
                                    double                  kerneldensity,      double          mindata,            double          maxdata,
                                    double                  marginfactor,       double          kernelsubsampling,
                                    HistogramOptions        options,
                                    double*                 usercurvesize   = 0
                                );
                                        // Constructor from whole volume
                    THistogram  (   const Volume&           data,
                                    const Volume*           mask,
                                    int                     numdownsamples,
                                    double                  kerneldensity,
                                    double                  marginfactor,       double          kernelsubsampling, 
                                    HistogramOptions        options
                                );
                                        // Constructor from clipped volume
                    THistogram  (   const Volume&           data,
                                    const Volume*           mask,
                                    int                     li1,                int             ls1, 
                                    int                     li2,                int             ls2, 
                                    int                     li3,                int             ls3, 
                                    double                  kerneldensity,
                                    double                  marginfactor,       double          kernelsubsampling, 
                                    HistogramOptions        options
                                );


    void            Reset   ();
    void            Set     ( int numbins );

                                        // Conversion bin index <-> real values, with optional flag
    double          ToReal              ( HistoUnit unit, double b )        const   { return  unit == RealUnit ? Index1.ToReal      ( b ) : b; }
    double          ToRealWidth         ( HistoUnit unit, double b )        const   { return  unit == RealUnit ? Index1.ToRealWidth ( b ) : b; }
    int             ToBin               ( HistoUnit unit, double v )        const   { return  Clip ( Round ( unit == RealUnit ? Index1.ToIndex ( v ) : v ), 0, Dim1 - 1 ) ; }   // !Bins are centered, needs to round!
//  int             ToBin               ( HistoUnit unit, double v )        const   { return  Clip ( Truncate ( unit == RealUnit ? Index1.ToIndex ( v ) : v ), 0, Dim1 - 1 ) ; }             // !Bins NOT centered, NO needs to round!
    double          Step                ()                                  const   { return  1 / NonNull ( Index1.IndexRatio ); }  // data step between 2 bins = KernelSize / SubSampling
    bool            IsStepInteger       ()                                  const   { return  IsInteger ( RoundTo ( Step () * KernelSubsampling, SingleFloatEpsilon ) ); }  // case where data is integer, we also should end up with integer steps
    double&         GetValue            ( double v )                                { return  Array[ ToBin ( RealUnit, v ) ]; }     // TVector virtual function
    double          GetMarginFactor     ()                                  const   { return  MarginFactor; }
    double          GetKernelSubsampling()                                  const   { return  KernelSubsampling; }


    double          GetFirstPosition        ( HistoUnit unit )              const;
    double          GetLastPosition         ( HistoUnit unit )              const;
    double          GetExtent               ( HistoUnit unit )              const;
    double          GetRelativeExtent       ()                              const;
    double          GetMaxPosition          ( HistoUnit unit )              const;
    double          GetMinPosition          ( HistoUnit unit )              const;
    double          GetMaxStat              ()                              const;  // the Max Value from the original data, estimated from the last converted bin

    double          GetMiddlePosition       ( HistoUnit unit )              const;
    double          GetRelativePosition     ( HistoUnit unit, double from ) const;

    double          GetTotalArea            ( HistoUnit unit, double from = 0, double to = 0 )  const;
    int             GetNumNonEmptyBins      ( HistoUnit unit )              const;

    double          GetRelativeValueMax     ( int b )                       const;
    double          GetRelativeValueArea    ( int b )                       const;

    int             GetNumModes             ()                              const;
    double          GetModePosition         ( HistoUnit unit, int mi )      const;  // index from 1 to GetNumModes
    double          GetMaximumModePosition  ( HistoUnit unit )              const;
    int             GetMaximumModeIndex     ()                              const;
    double          GetModeValue            ( int m )                       const;
    double          GetModesMiddlePosition  ( HistoUnit unit, int mi1, int mi2 )        const;
    double          GetPercentilePosition   ( HistoUnit unit, double p )                const;
    int             GetNumZeroPlateaux      ( HistoUnit unit, double from, double to )  const;
    void            GetValleys              ( int minscale, int maxscale, int scalestep );      // scale search for best valleys


    double          ComputeBackground       ( const Volume& data );
    void            ComputeZScoreFactors    ( int maxbinspertf, double& center, double& spreadleft, double& spreadright )   const;  // extract the center and spreading of histogram
    void            ClearZero               ();
    void            EqualizeCDF             ( TVector<double>&  topack );   // caller should take care to ignore nulls at histogram construction
    void            Erode                   ( int nbins );
    void            Erode                   ( double percentmax );
    double          HowSymetric             ( int &T )                      const;
    void            Pack                    ( TArray1<int> &topack, TArray1<int> &tounpack, double minreject = 0 );   // pack empty cells, returning 2 LUTs, one to unpack and one to pack
    void            Smooth                  ( int num = 1 );
    void            ToCDF                   ( HistogramOptions options  = HistogramNormMax );
    void            Unpack                  ( TArray1<int> &tounpack );         // reverse the packing


    void            ComputeHistogramSize (  double      kerneldensity,  double      mindata,            double      maxdata,
                                            double      marginfactor,   double      kernelsubsampling,
                                            double&     curvesize,      double&     curvemin,           double&     curveratio 
                                         );

    void            ComputeHistogram    (   TEasyStats&             stats, 
                                            double                  kerneldensity,  double      mindata,            double      maxdata,    // kerneldensity, mindata and maxdata can be 0 for automatic scaling
                                            double                  marginfactor,   double      kernelsubsampling,  
                                            HistogramOptions        options,
                                            double*                 usercurvesize   = 0
                                         );

    void            ComputeHistogram    (   const Volume&           data,
                                            const Volume*           mask,
                                            int                     numdownsamples,
                                            double                  kerneldensity,
                                            double                  marginfactor,   double      kernelsubsampling,
                                            HistogramOptions        options
                                        );

//  void            ComputeHistogram    (   const Volume&           data,
//                                          double                  percentage,     TVector<int>&   randindex,
//                                          double                  kerneldensity,
//                                          double                  marginfactor,   double          kernelsubsampling,  
//                                          HistogramOptions        options
//                                      );

    void            ComputeHistogram    (   const Volume&           data,
                                            const Volume*           mask,
                                            int                     li1,            int             ls1, 
                                            int                     li2,            int             ls2, 
                                            int                     li3,            int             ls3, 
                                            double                  kerneldensity,
                                            double                  marginfactor,   double          kernelsubsampling,  
                                            HistogramOptions        options
                                        );


    void            WriteFileVerbose    ( char *file, bool logplot )                                const;


    THistogram&     operator    =       ( const THistogram &op2 );
    THistogram&     operator    =       ( const TVector<float>  &op2 );
    THistogram&     operator    =       ( const TVector<double> &op2 );

//  double          operator    ()      ( double v, InterpolationType interpolate ) const   { return GetValueChecked ( v, interpolate ); }
//  double&         operator    []      ( double v )        { return GetValue ( v ); }  //!converts value to index with specific histogram formula!
//  double&         operator    ()      ( double v )        { return GetValue ( v ); }  //!converts value to index with specific histogram formula!

    THistogram&     operator    =       ( double op2 );


protected:

    double          MarginFactor;           // set in ComputeHistogramSize
    double          KernelSubsampling;

    int             Descend                 ( int b )                   const;
    int             Ascend                  ( int b )                   const;
    int             GetNextInflexionPosition( int from, bool forward )  const;  // when no min between to positions exist, look for an inflexion
    int             GetNextLCornerPosition  ( int from )                const;  // high deceleration

    double          ComputeBackground       ( HistoUnit unit )          const;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}







