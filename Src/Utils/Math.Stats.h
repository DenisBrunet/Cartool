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


#include    <math.h>                    // math functions, constants
//#include    <complex.h>

#include    "System.h"
#include    "TArray1.h"
#include    "TArray2.h"
#include    "TArray3.h"

#include    "TMaps.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

                                        // Factor to make the MAD having the same spreading as a SD
constexpr double    MADToSigma          = 1.4825796886;
                                        // Factor to make the IQR having the same spreading as a SD
constexpr double    IQRToSigma          = 0.7412898443;

constexpr int       NumMaxModeRobustEstimates   = 4;


//----------------------------------------------------------------------------
                                        // Control how the internal Data field can be modified by a (robust) method
enum                StorageDataLife
                    {
                    PreserveData,                   // Operation should NOT alter Data content:   data will be needed later on
                    CanAlterData,                   // Operation CAN modify Data content at will: data not needed later on, therefor method can save some duplicate variables
                    };


enum                KernelDensityType
                    {
                    KernelDensitySilverman,         // AKA Rule of Thumb
                    KernelDensitySilvermanRobust,   // AKA Rule of Thumb, but robust
                    KernelDensityDiscrete,          // For enumerations that don't repeat
                    KernelDensityMultipleGaussians, // If we have a distribution made of multiple Gaussians

                    NumKernelDensityTypes,

                    KernelDensityDefault    = KernelDensitySilverman,
                    };


enum                DeSkewType
                    {
                    DeSkewNone,
                    DeSkewRight,
                    DeSkewLeft,
                    DeSkewAuto,
                    NumDeSkewTypes,
                    };


enum                TEasyStatsFunctionType
                    {
                    TEasyStatsFunctionNone,

                    TEasyStatsFunctionMaxMode,
                    TEasyStatsFunctionMaxModeHistogram,
                    TEasyStatsFunctionMaxModeHSM,
                    TEasyStatsFunctionMaxModeHRM,

                    TEasyStatsFunctionSD,
                    TEasyStatsFunctionInterQuartileRange,
                    TEasyStatsFunctionMAD,
                    TEasyStatsFunctionMADLeft,
                    TEasyStatsFunctionMADAsym,

                    NumTEasyStatsFunction,
                    };


enum                GaussianMixtureOptions
                    {
                    GaussianMixtureDefault,
                    GaussianMixtureFaster,
                    };


inline  char*       SkewnessToString ( int skewness )   { return    skewness == 1 ? "log" : skewness == -1 ? "exp" : ""; }

inline  double      LinToLog        ( double x, double w );


//----------------------------------------------------------------------------
                                        // To easily get stats from data
class   THistogram;


class   TEasyStats
{
public:
                    TEasyStats  ();
                    TEasyStats  ( int numdata )                                                     { Resize ( numdata );      }
                    TEasyStats  ( const TArray1<double> &array1, bool allocate )                    { Set ( array1, allocate ); }
                    TEasyStats  ( const TArray1<float>  &array1, bool allocate )                    { Set ( array1, allocate ); }
                    TEasyStats  ( const TArray2<int>    &array2, bool allocate )                    { Set ( array2, allocate ); }
                    TEasyStats  ( const TArray3<UCHAR>  &array3, bool ignorenulls, bool allocate )  { Set ( array3, ignorenulls, allocate ); }
                    TEasyStats  ( const Volume&         array3, const Volume* mask, bool ignorenulls, bool allocate, int numdownsamples = 0 )    { Set ( array3, mask, ignorenulls, allocate, numdownsamples ); }
                    TEasyStats  ( const TMaps&           maps,   bool allocate, int maxitems = 0 )  { Set ( maps,   allocate, maxitems ); }
//                 ~TEasyStats  ();


    bool            IsEmpty         ()              const       { return    NumItems == 0; }            // Tells how many data has been currently Add'ed, independently to them being internally stored or not.
    bool            IsNotEmpty      ()              const       { return    NumItems != 0; }
    bool            IsAllocated     ()              const       { return    Data.IsAllocated    (); }   // Tells if object is ABLE to internally store Add'ed data (hence allowing non-linear stats). It does not tell how much data is currently in.
    bool            IsNotAllocated  ()              const       { return    Data.IsNotAllocated (); }
//  bool            IsSorted        ()                          { return    Sorted; }


    void            Reset       ();                 // clear variables and reset array if it exists
    void            Resize      ( int numitems );   // could be 0 for deallocation - resets content all the time
    void            Set         ( const TArray1<double> &array1, bool allocate );
    void            Set         ( const TArray1<float>  &array1, bool allocate );
    void            Set         ( const TArray2<int>    &array2, bool allocate );
    void            Set         ( const TArray2<float>  &array2, bool allocate, int maxitems = 0 );
    void            Set         ( const TArray3<UCHAR>  &array3, bool ignorenulls, bool allocate );
    void            Set         ( const Volume&          array3, const Volume* mask, bool ignorenulls, bool allocate, int numdownsamples = 0 );
    void            Set         ( const TArray3<double>& array3, const Volume* mask, bool ignorenulls, bool allocate );
    void            Set         ( const TMap            *maps,   bool allocate );  // use this for TMap variable
    void            Set         ( const TMaps&           maps,   bool allocate, int maxitems = 0 );


    void            Resample    ( TEasyStats& substats,  double percentage, TVector<int>& randindex, TRandUniform* randunif = 0 )   const;                 // random subsampling
    void            Resample    ( TEasyStats& substats,  int    samplesize, TVector<int>& randindex, TRandUniform* randunif = 0 )   const;                 // random subsampling
    double          Randomize   ( TEasyStatsFunctionType how, int numresampling, int samplesize, TArray1<double>* params = 0 ); // randomized function, returning the mean of all sampling
    void            KeepWithin  ( double minvalue, double maxvalue );
    void            RemoveNulls ();                 // !Sorts data!

    int             GetNumItems ()                  const       { return    NumItems; }
    int             MaxSize     ()                  const       { return    Data.MaxSize (); }
    size_t          MemorySize  ()                  const       { return    Data.MemorySize (); }


    void            Add         ( double  v,                        ThreadSafety safety = ThreadSafetyCare );   // the work-horse for adding values - now explicitly asking for thread-safety
    void            Add         ( std::complex<float> c,            ThreadSafety safety = ThreadSafetyCare );
    void            Add         ( const TArray1<float>  &array1,    ThreadSafety safety = ThreadSafetyCare );
    void            AddAngle    ( double  a,                        ThreadSafety safety = ThreadSafetyCare );   // angles should have a special treatment

    void            Sort        ( bool force = false );                             // !be careful for Omp/Parallel code!

    int             IsSkewed    ();                                                 // returns 0 is Normal, +1 if positive Skew, -1 if negative skew
    int             DeSkew      ( DeSkewType how );                                 // Test for skewness, and either log or exp the data accordingly
    double          GetLinLogFactor  ( double minll, double maxll );                // estimate log bias to have a Normal distribution
    void            ToLog       ();                                                 // If distribution is not Normal
    void            ToAntiLog   ();           
    void            ToExp       ();    
    void            ToAbsolute  ();    

    bool            IsAngular   ();
    bool            IsInteger   ();

                                                // These methods do NOT need to store data: fast execution and very small memory print
    double          AbsoluteMax             ()                   { return  max ( fabs ( Max () ), fabs ( Min () ) ); }
    double          Average                 ();
    double          CoefficientOfVariation  ()                   { return  CoV (); }
    double          CoV                     (); // SD / Mean
    double          ConsistentMean          ( double penalty = 1 );     // Mean * ( 1 - SD )
    double          Max                     ();
    double          Mean                    ()                   { return  Average (); }
    double          Min                     ();
    double          MinMax                  ( double v );
    double          Normalize               ( double v );   // results in [0..1]
    double          Range                   ()                   { return  Max () - Min (); }
    double          RMS                     (); // Root Mean Square, kind of SD without mean subtraction
    double          SD                      ()                   { return  sqrt ( Variance () ); }
    double          SD                      ( double center )    { return  sqrt ( Variance ( center ) ); }
    double          SignalToNoiseRatio      (); // Mean / SD
    double          SNR                     ()                   { return  SignalToNoiseRatio (); }
    double          Sum                     ();
    double          Sum2                    ();
    double          Variance                ();
    double          Variance                ( double center );
    double          ZScore                  ( double v );
                                                // These methods DO need to store all data points
                                                // To enable that, pass a non-null allocation size upon creation, or call Resize
                                                // Data order is NOT guaranteed AFTER calling any of these methods, as they will very likely call Sort
    double          ConsistentMedian        ( double penalty = 1 );     // Median * ( 1 - IQR )
    double          FirstMode               ( double noisethreshold = 0.50 );   // Give some threshold to ignore low values
    double          InterQuartileRange      ();
    void            InterQuartileRangeAsym  ( double center, double &iqrleft, double &iqrright );           // same results as MADAsym
    double          InterQuartileMean       ()                   { return  TruncatedMean ( 0.25, 0.75 ); }  // remove first and last 25% data, then do the Mean
    double          Kurtosis                ();
    void            KurtosisAsym            ( double center, double &kurtleft, double &kurtright );
    double          LastMode                ( double noisethreshold = 0.50 );   // Give some threshold to ignore low values
    double          Median                  ( bool strictvalue = true );        // can optionally return an interpolated value
    double          MAD                     ( StorageDataLife datalife = PreserveData );      // Median of Absolute Deviation
    double          MAD                     ( double center, StorageDataLife datalife = PreserveData );
    void            MADAsym                 ( double center, double &madleft, double &madright );
    void            MADLeft                 ( double center, double &madleft );
    void            MADRight                ( double center, double &madright );
    double          MaxModeHistogram        ( THistogram* h = 0 );      // #1 estimate for all cases- Compute the histogram, then simply look for max position
    double          MaxModeHRM              ();                         // #2 estimate for 2 modes  - Done by Half Range Mode   ;HRM & HSM will asymptotically behave the same past ~128 samples
    double          MaxModeHSM              ();                         // #2 estimate for 1 mode   - Done by Half Sample Mode  ;HRM & HSM will asymptotically behave the same past ~128 samples
    void            MaxModeRobust           ( TEasyStats& statcenter );
    double          Qn                      ( int maxitems );           // Rousseeuw and Croux Qn for Robust Standard Deviation
    double          Quantile                ( double p );               // can return an interpolated value
    double          RobustCoV               ();                         // CoV with Median and MAD: MAD / Median
    double          RobustKurtosis          ();                         // Using Median and MAD
    double          RobustKurtosisCS        ();                         // Crow Siddiqui estimate
    double          RobustKurtosisHogg      ();                         // better for heavy tails
    double          RobustKurtosisOctiles   ();
    double          RobustSNR               ();                         // CoV with Median and MAD: MAD / Median
    double          RobustSkewnessPearson   ();                         // Pearson, non-parametric, less sensitive to outliers
    double          RobustSkewnessQuantiles ( double alpha );           // non-parametric, robust, general formula
    double          RobustSkewnessQuartiles ();                         // non-parametric, robust
    double          RobustSkewnessMAD       ( double center );          // non-parametric, robust - Cartool made
    void            SDAsym                  ( double center, double &sdleft,  double &sdright  );
    double          Sn                      ( int maxitems );           // Rousseeuw and Croux Sn for Robust Standard Deviation
    double          SkewnessPearson         ();                         // adjusted Fisher-Pearson, parametric, sensitive to outliers
    double          SkewnessPearson         ( double center );          // adjusted Fisher-Pearson, parametric, sensitive to outliers
    void            SkewnessPearsonAsym     ( double center, double &skewleft, double &skewright );
    double          TruncatedMean           ( double qfrom, double qto );// mean without some extreme values
    void            VarianceAsym            ( double center, double &varleft, double &varright );
    double          ZScoreRobust            ( double v );

    int             PairsToNumItems         ( int numpairs = -1 );      // converting pairs of comparisons to the corresponding number of items
    int             NumItemsToPairs         ( int numitems      );      // converting number of items to pairs

    double          GaussianKernelDensity   ( KernelDensityType kdetype = KernelDensityDefault, int numgaussians = 1 );

    double          GetGaussianMixture      ( int numgaussian, double requestedprecision, GaussianMixtureOptions options, TTracks<double>& mixg, TTracks<double>* gaussmix = 0 /*, const char *title, TEasyStats *stat = 0*/ );
//  double          GetAsymGaussianMixture  ( int numgaussian, double requestedprecision, GaussianMixtureOptions options, TTracks<double>& mixg, TTracks<double>* gaussmix = 0 /*, const char *title, TEasyStats *stat = 0*/ );


    void            Show                ( const char *title = 0 );
    void            ShowData            ( const char *title = 0 )   const;
    void            WriteFileData       ( char *file );                 // calls TVector::WriteFile
    void            WriteFileVerbose    ( char *file );
    void            WriteFileHistogram  ( char *file, bool smooth = true );


                    TEasyStats          ( const TEasyStats &op  );
    TEasyStats&     operator        =   ( const TEasyStats &op2 );


                    operator        int                     ()  const   { return    NumItems; }
                    operator        const TVector<float>&   ()  const   { return    Data;     }
                    operator              TVector<float>&   ()          { return    Data;     }

    float           operator        []  ( int i )   const               { return    Data[ i ]; }    // !NO checks on index boundaries!
    float&          operator        []  ( int i )                       { return    Data[ i ]; }


private:

    TVector<float>  Data;               // unallocated, or allocated to max size
    int             NumItems;           // actual # of data
    bool            Sorted;             // remember if data has been sorted or not

                                        // for faster & simpler stats, we need only these fields:
    double          CacheSum;
    double          CacheSum2;
    double          CacheMin;
    double          CacheMax;


    void           _Add         ( double  v );  // non thread-safe method which does the actual work of Add
};


//----------------------------------------------------------------------------
                                        // We often need a set of statistics
enum        PolarityType;


class   TGoEasyStats
{
public:
                    TGoEasyStats ();
                    TGoEasyStats ( int numstats, int numdata = 0 );
                   ~TGoEasyStats ();


    bool            IsEmpty         ()                  { return    NumStats == 0; }    // Number of TEasyStats allocated
    bool            IsNotEmpty      ()                  { return    NumStats != 0; }
    int             GetNumStats     ()                  { return    NumStats; }


    void            Reset           ();                                 // TEasyStats::Reset for all TEasyStats
    void            Resize          ( int numstats, int numdata = 0 );  // could be 0 for deallocation - resets content all the time


    void            Cumulate        ( const TVector<float>& v, PolarityType polarity, const TVector<float>& refv ); // similar to TVector::Cumulate, if we want robust estimators


    TArray1<int>    DeSkew          ( DeSkewType how );                 // same as IsSkewed, but has to return an array of skewness


    void            Median          ( TVector<float>& v,  bool strictvalue = true );
    double          Max             ();
    double          Min             ();
    double          Quantile        ( double p );               // can return an interpolated value


    double          GaussianKernelDensity   ( KernelDensityType kdetype = KernelDensityDefault );


    void            Show            ( char *title = 0 );


                    TGoEasyStats                    ( const TGoEasyStats &op  );
    TGoEasyStats&   operator    =                   ( const TGoEasyStats &op2 );


    TEasyStats     &operator    ()                  ( int index )   { return Stats[ index ]; }
    TEasyStats     &operator    []                  ( int index )   { return Stats[ index ]; }

                    operator    int                  ()             { return NumStats; }
                    operator    bool                 ()             { return NumStats; }

protected:
    TEasyStats*         Stats;
//  TList<TEasyStats>   Stats;
    int                 NumStats;

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}







