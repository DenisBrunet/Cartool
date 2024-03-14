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

#include    <vector>
#include    <unordered_map>

#if defined (_DEBUG)
#include    "Dialogs.Input.h"
#endif

#include    "TArray1.h"
#include    "Math.Stats.h"

#include    "TBaseDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// NumSteps     : best is in the form ( 2 * PrimeNumber + 1 ), like 2 * 5 + 1 = 11, so that the zoom is centered on the best value
// NumSubSteps  : at least 2
// ZoomFactor   : something ( PrimeNumber - delta ) / PrimeNumber, like ( 5 - 2 ) / 5 = 0.6, to zoom with no overlaps between successive intervals (apart from center)

                                        // Single varying parameter
class  TGOParam
{
public:
                    TGOParam ();


    int             Type;               // type of parameter
    double          Min;                // range of real parameters
    double          Max;
    int             NumSteps;           // number of discrete sampling
    int             NumSubSteps;        // and sub-sampling, for a given sampled cell
    double          ZoomFactor;         // how to zoom in for the next optimization

    double          Value;              // current value to evaluate, or final result


    void            Reset           ();
    void            Set             ( int numsteps, int numsubsteps, double zoomfactor );

    double          GetStep         ()                  const   { return    ( Max - Min ) /   ( NumSteps - 1 );                 }
    double          GetSubStep      ()                  const   { return    ( Max - Min ) / ( ( NumSteps - 1 ) * NumSubSteps ); }    // GetStep () / NumSubSteps
    int             GetTotalSteps   ()                  const   { return    NumSteps * NumSubSteps;                             }

    double          GetRange        ()                  const   { return      Max - Min;        }
    double          GetMiddle       ()                  const   { return    ( Min + Max ) / 2;  }
                                        // convert an index to its corresponding value in the interval - use a multiplicative formula and not an incremental one to avoid cumulated errors
    double          GetValue        ( int index )       const   { return    Min          + index    * ( Max - Min ) /   ( NumSteps - 1 );                   } // index * GetStep ()
                                        // when subdividing, center the subsampling so shift the sub-origin backward
    double          GetSubMin       ()                  const   { return    Min          + ( 0.5 / NumSubSteps - 0.5 ) * GetStep ();                        } // proper offset to jump equally of NumSubSteps from Min to Max
    double          GetSubValue     ( int subindex )    const   { return    GetSubMin () + subindex * ( Max - Min ) / ( ( NumSteps - 1 ) * NumSubSteps );   } // subindex * GetSubStep ()


    void            ZoomIn          ( double newcenter );

};


//----------------------------------------------------------------------------
                                        // Group of varying parameters
class  TGOParams
{
public:
                    TGOParams ();
                    TGOParams ( int numdims );

                                        // fixed parameters that can be used in the evaluations - free access, but be careful that accessing a non-existing parameter will create it
    std::unordered_map<int, double>     FixedParams;


    virtual void    Reset           ();
    virtual void    Set             ( int numsteps, int numsubsteps, double zoomfactor );


    void            SetNumDims      ( int numdims );
    void            AddDim          ();
    void            AddDim          ( int type, double min, double max, int numsteps, int numsubsteps, double zoomfactor );
    int             GetNumDims      ()                  const   { return NumDims; };

    void            SetFixedParam   ( int type, double value );

                                        // ZoomIn each parameter
    void            ZoomIn          ( double *newcenters );


                    operator    int     ()              const   { return NumDims; }
                    operator    bool    ()              const   { return NumDims; }

    const TGOParam& operator    ()      ( int i )       const   { return Params[ i ]; }
    TGOParam&       operator    ()      ( int i )               { return Params[ i ]; }
    const TGOParam& operator    []      ( int i )       const   { return Params[ i ]; }
    TGOParam&       operator    []      ( int i )               { return Params[ i ]; }


protected:

    int                 NumDims;
    TArray1<TGOParam>   Params;     // variable parameters to be searched

};


//----------------------------------------------------------------------------
                                        // We will need to re-group some parameters arbitrarily at some points - a simple array seems to be doing just fine
                                        // It only stores POINTERS to parameters, so changing from this structure will change the original parameters
using   TBunchOfGOParam     = TArray1<TGOParam*>;


//----------------------------------------------------------------------------
                                        // Nelder-Mead simplex variables
                                        // A simplex replaces the exhaustive grid with just a few vertices, each having a set of n parameters
class  TSimplex
{
public:
                    TSimplex ();
                   ~TSimplex ();

    std::vector<TVector<double>>    Vertex;
    TVector<double>                 Value;
    TVector<double>                 Centroid;
    int                             BestVertex;


    bool            IsAllocated     ()  const   { return  Value.IsAllocated    (); }
    bool            IsNotAllocated  ()  const   { return  Value.IsNotAllocated (); }

    int             GetSimplexDim   ()  const   { return  Value.GetDim ();      }
    int             GetDim          ()  const   { return  GetSimplexDim () - 1; }

    void            Reset           ();
    void            Set             ( int simplexdim );         // init data structure
    void            Set             ( TBunchOfGOParam& params ); // + initialize vertex (but not values)

    void            ToParams        ( int vertexi, TBunchOfGOParam& params ) const;  // transfer a given vertex to parameters
    void            BestToParams    ( TBunchOfGOParam& params )              const;  // transfer best vertex to parameters

    double          GetBestValue    ()                      const   { return  Value ( BestVertex ); }
    double          GetRadius       ()                      const;
    void            GetStatsValues  ( TEasyStats& stats )   const;

    void            Show            ( const char *title, int index )    const;


    TSimplex                                ( const TSimplex& op  );
    TSimplex&               operator    =   ( const TSimplex& op2 );

    TVector<double>&        operator    []  ( int i )               { return Vertex[ i ]; }
    const TVector<double>&  operator    []  ( int i )       const   { return Vertex[ i ]; }

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // various way to navigate the groups / dimensions
enum                GOMethod 
                    {

                    MethodBoxScan           = 0x10,     // Brute force, full box scan
                    MethodCrossHairScan     = 0x20,     // Brute force, crosshair scan
                    MethodNelderMead        = 0x40,     // Smarter & faster scan
                    MethodMask              = MethodBoxScan | MethodCrossHairScan | MethodNelderMead,

                    ExploreGlobal           = 0x01,     // Using all paramaters at once
                    ExploreCyclical         = 0x02,     // Going through all groups cyclically
                    ExploreWeakestGroup     = 0x04,     // Only on the weakest group's whole parameters
                    ExploreWeakestDimension = 0x08,     // Only on the weakest dimension / single parameter
                    ExploreMask             = ExploreGlobal | ExploreCyclical | ExploreWeakestGroup | ExploreWeakestDimension,

                                                        // Current implementations:
                    GlobalBoxScan                   = MethodBoxScan         | ExploreGlobal,
                    CyclicalBoxScan                 = MethodBoxScan         | ExploreCyclical,
                    WeakestGroupBoxScan             = MethodBoxScan         | ExploreWeakestGroup,
                    WeakestDimensionBoxScan         = MethodBoxScan         | ExploreWeakestDimension,

                    GlobalCrossHairScan             = MethodCrossHairScan   | ExploreGlobal,
                    CyclicalCrossHairScan           = MethodCrossHairScan   | ExploreCyclical,
                    WeakestGroupCrossHairScan       = MethodCrossHairScan   | ExploreWeakestGroup,
                    WeakestDimensionCrossHairScan   = MethodCrossHairScan   | ExploreWeakestDimension,  // == WeakestDimensionBoxScan
                                                
                    GlobalNelderMead                = MethodNelderMead      | ExploreGlobal,
                    CyclicalNelderMead              = MethodNelderMead      | ExploreCyclical,
                    WeakestGroupNelderMead          = MethodNelderMead      | ExploreWeakestGroup,
                    };

inline bool         IsMethodScanBox         ( GOMethod m )  { return IsFlag ( m, MethodBoxScan          ); }
inline bool         IsMethodCrossHairScan   ( GOMethod m )  { return IsFlag ( m, MethodCrossHairScan    ); }
inline bool         IsMethodNelderMead      ( GOMethod m )  { return IsFlag ( m, MethodNelderMead       ); }
const char*         GetGOMethodName         ( GOMethod m );
const char*         GetGOMethodExtension    ( GOMethod m );

                                        // "Standard" relative error convergence
constexpr double    GODefaultConvergence        = 1e-6;
                                        // value to be returned when evaluation fails - NOT DBL_MAX so it can still do some arithmetics on it
constexpr double    GOMaxEvaluation             = 1e30;

constexpr double    GOMaxCoeffRelDiff           = 1e-6;
constexpr int       GOMaxIterations             = 2000;

                                        // less than 2 steps does not make any sense
constexpr int       NumStepsMin                 =   2;
constexpr int       NumStepsMax                 = 100;
                                        // sub-steps should be >= 1
constexpr int       NumSubStepsMin              =   1;
constexpr int       NumSubStepsMax              = 100;
                                        // zoom factor should be < 1.0
constexpr double    NumZoomFactorMin            = 1e-6;
constexpr double    NumZoomFactorMax            = 0.999;

                                        // Most common steps that have been used so far (using defines because of the 3 parameters)
                                        //      #Steps          #SubSteps   Zooming Factor (!related to the number of steps!)
#define             GOStepsFaster               2 * 2 + 1,      1,          3.0 / 4.0
#define             GOStepsDefault              2 * 2 + 1,      2,          3.0 / 4.0
#define             GOStepsPrecise              2 * 5 + 1,      1,          4.0 / 5.0


//----------------------------------------------------------------------------
                                        // Class that holds the parameters and runs the optimization search

                                        // !TODO: have a single way to access the parameters, whatever their type fixed or searched

class  TGlobalOptimize
{
public:
                    TGlobalOptimize ();
                    TGlobalOptimize ( int numsteps, int numsubsteps, double zoomfactor );
    virtual        ~TGlobalOptimize ();



    bool            IsSet           ()                  const   { return GetTotalDims () > 0; }
    bool            IsNotSet        ()                  const   { return ! IsSet (); }

    virtual void    Reset           ();
    virtual void    Set             ( int numsteps, int numsubsteps, double zoomfactor );


    int             GetNumGroups    ()                  const   { return NumGroups; }
    void            AddGroup        ();
    void            AddGroup        ( int numsteps, int numsubsteps, double zoomfactor );

    void            AddDim          ( int group, int type, double min, double max );
    void            AddDim          ( int type, double min, double max );   // to current group
    int             GetTotalDims    ()                  const;

    void            SetFixedParam   ( int group, int type, double value );
    void            SetFixedParam   ( int type, double value );             // to current group

                                        // successive zoom into the parameters range, a non-greedy evaluation
    virtual void    GetSolution     ( GOMethod method, int how, double requestedprecision, double outliersprecision, const char *title, TEasyStats *stat = 0 );
                                        // function to be called by the optimization
    virtual double  Evaluate        ( TEasyStats *stat = 0 )   = 0;
                                        // ZoomIn each group, each parameter
    void            ZoomIn          ( double *newcenters );
                                        // for the derived class, to monitor each GetSolution step
    virtual void    ShowProgress    ()                      const   {}


    bool            HasValue        ( int type )            const   { return (bool) ToValues.count ( type ); /*ToValues.contains ( type )*/ }
    void            SetValue        ( int type, double value )      { if ( HasValue ( type ) )  *ToValues[ type ] = value; }      // force set AN EXISTING PARAMETER
#if defined (_DEBUG)
                                        // missing type will crash, better give some warning
    double          GetValue        ( int type )            const   { if ( HasValue ( type ) ) return  *ToValues.at ( type );   /*assert ( HasValue ( type ) );*/ DBGV ( type, "TGlobalOptimize::GetValue: undefined type" );    return 0; }
    double          GetMinValue     ( int type )            const   { if ( HasValue ( type ) ) return  *ToMin   .at ( type );   /*assert ( HasValue ( type ) );*/ DBGV ( type, "TGlobalOptimize::GetMinValue: undefined type" ); return 0; }
    double          GetMaxValue     ( int type )            const   { if ( HasValue ( type ) ) return  *ToMax   .at ( type );   /*assert ( HasValue ( type ) );*/ DBGV ( type, "TGlobalOptimize::GetMaxValue: undefined type" ); return 0; }
#else
                                        // or test HasValue at each call, which could be sub-optimal?
    double          GetValue        ( int type )            const   { return  *ToValues.at ( type );   }    // retrieve existing parameter, WITHOUT TESTING FOR EXISTENCE, as this is the only way for 'const'
    double          GetMinValue     ( int type )            const   { return  *ToMin   .at ( type );   }    // retrieve existing parameter, WITHOUT TESTING FOR EXISTENCE, as this is the only way for 'const'
    double          GetMaxValue     ( int type )            const   { return  *ToMax   .at ( type );   }    // retrieve existing parameter, WITHOUT TESTING FOR EXISTENCE, as this is the only way for 'const'
#endif

    void            WriteParameters ( const char* file )    const;
    void            ReadParameters  ( const char* file );


                        operator    int     ()              const   { return NumGroups; }
                        operator    bool    ()              const   { return NumGroups; }
    const TGOParams&    operator    []  ( int i )           const   { return Groups[ i ]; }
    TGOParams&          operator    []  ( int i )                   { return Groups[ i ]; }


protected:

    int                     NumGroups;
    std::vector<TGOParams>  Groups;

    int             How;                // main parameter to be used by derived classes - could be anything: unused, enum, bitwise flags
    double          CurrentPrecision;   // current precision during optimization
    double          OutliersPrecision;  // a special threshold for any smart outliers rejection
    double          RequestedPrecision; // storing caller requested precision
    int             Iteration;          // global iteration variable

                                        // Box-Scan method
    double          BoxScan                 ( TArray1<TGOParam*>& params, double* bestvalues, TEasyStats& stats );

    double          CrossHairScan           ( TArray1<TGOParam*>& params, double* bestvalues, TEasyStats& stats );
                                        // Nelder-Mead method
    void            InitSimplex             ( TArray1<TGOParam*>& params, TSimplex& simplex );
    void            EvaluateSimplexValues   ( TArray1<TGOParam*>& params, TSimplex& simplex );
    double          NelderMead              ( TArray1<TGOParam*>& params, TSimplex& simplex, TEasyStats& stats, bool forceevaluate = false );

    int             GetGroupOffset          ( int group );


private:
                                        // default values for new groups / dimensions, if the latter don't provide any
    int             NumSteps;           
    int             NumSubSteps;
    double          ZoomFactor;
                                        // quick access to any parameter, which could be distributed across any groups / dimensions
                                        // to be used by derived classes when computing their models
    std::unordered_map<int, double*>    ToValues;
    std::unordered_map<int, double*>    ToMin;
    std::unordered_map<int, double*>    ToMax;

    void            ResetToValue    ();
    void            SetToValue      ();
    void            SetToValue      ( TGOParam& param );

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
