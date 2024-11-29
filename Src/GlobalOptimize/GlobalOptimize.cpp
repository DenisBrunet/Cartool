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

#if defined(CHECKASSERT)
#include    <assert.h>
#endif

#include    "MemUtil.h"
#include    "Math.Random.h"
#include    "TArray1.h"
#include    "TArrayM.h"
#include    "TVector.h"
#include    "Files.TVerboseFile.h"
#include    "Dialogs.TSuperGauge.h"

#include    "TMaps.h"
#include    "TExportTracks.h"

#include    "GlobalOptimize.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const char*     GetGOMethodName ( GOMethod m )
{
if      ( m == GlobalBoxScan                    )   return  "Global Box-Scan";
else if ( m == CyclicalBoxScan                  )   return  "Cyclical Box-Scan";
else if ( m == WeakestGroupBoxScan              )   return  "Weakest Group Box-Scan";
else if ( m == WeakestDimensionBoxScan          )   return  "Weakest Dimension Box-Scan";

else if ( m == GlobalCrossHairScan              )   return  "Global Crosshair-Scan";
else if ( m == CyclicalCrossHairScan            )   return  "Cyclical Crosshair-Scan";
else if ( m == WeakestGroupCrossHairScan        )   return  "Weakest Group Crosshair-Scan";
else if ( m == WeakestDimensionCrossHairScan    )   return  "Weakest Dimension Crosshair-Scan";

else if ( m == GlobalNelderMead                 )   return  "Global Nelder-Mead";
else if ( m == CyclicalNelderMead               )   return  "Cyclical Nelder-Mead";
else if ( m == WeakestGroupNelderMead           )   return  "Weakest Group Nelder-Mead";

else                                                return  IsMethodScanBox       ( m ) ?   "Box-Scan Method" 
                                                          : IsMethodCrossHairScan ( m ) ?   "Crosshair Scan Method" 
                                                          : IsMethodNelderMead    ( m ) ?   "Nelder-Mead Method" 
                                                          :                                 "Unknown Method";
}


const char*     GetGOMethodExtension ( GOMethod m )
{
if      ( m == GlobalBoxScan                    )   return  "GBS";
else if ( m == CyclicalBoxScan                  )   return  "CBS";
else if ( m == WeakestGroupBoxScan              )   return  "WGBS";
else if ( m == WeakestDimensionBoxScan          )   return  "WDBS";

else if ( m == GlobalCrossHairScan              )   return  "GXS";
else if ( m == CyclicalCrossHairScan            )   return  "CXS";
else if ( m == WeakestGroupCrossHairScan        )   return  "WGXS";
else if ( m == WeakestDimensionCrossHairScan    )   return  "WDXS";

else if ( m == GlobalNelderMead                 )   return  "GNM";
else if ( m == CyclicalNelderMead               )   return  "CNM";
else if ( m == WeakestGroupNelderMead           )   return  "WGNM";

else                                                return  IsMethodScanBox       ( m ) ?   "BS" 
                                                          : IsMethodCrossHairScan ( m ) ?   "XS" 
                                                          : IsMethodNelderMead    ( m ) ?   "NM" 
                                                          :                                 "";
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TGOParam::TGOParam ()
{
Reset ();
}


void    TGOParam::Reset ()
{
Type                = -1;

Set ( 1, 1, 0.5 );
}


void    TGOParam::Set ( int numsteps, int numsubsteps, double zoomfactor )
{
Min                 = 0;
Max                 = 0;
NumSteps            = Clip ( numsteps,      NumStepsMin,        NumStepsMax      );
NumSubSteps         = Clip ( numsubsteps,   NumSubStepsMin,     NumSubStepsMax   );
ZoomFactor          = Clip ( zoomfactor,    NumZoomFactorMin,   NumZoomFactorMax );

Value               = 0;
}


void    TGOParam::ZoomIn ( double newcenter )
{
Value               = newcenter;

double  newdelta    = ZoomFactor * GetRange () / 2;
                                        // !limits are allowed to drift!
Min                 = Value - newdelta;
Max                 = Value + newdelta;

/*                                      // !limits are not allowed to drift! - if clipping occurs, keep the interval symetric!
if      ( Value - newdelta < Min )
    Max             = 2 * Value - Min;  // force the Max to be symetrical to the (untouched) Min
else if ( Value + newdelta > Max )
    Min             = 2 * Value - Max;  // force the Min to be symetrical to the (untouched) Max
else {
    Min             = Value - newdelta;
    Max             = Value + newdelta;
    }
*/
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TGOParams::TGOParams ()
{
NumDims             = 0;
Reset ();
}


        TGOParams::TGOParams ( int numdims )
{
NumDims             = 0;
Reset ();

SetNumDims ( numdims );
}


void    TGOParams::Reset ()
{
if ( NumDims )                          // some existing parameters?
    for ( int i = 0; i < NumDims; i++ )
        Params[ i ].Reset ();


FixedParams.clear ();

NumDims             = 0;
}


void    TGOParams::SetNumDims ( int numdims )
{
NumDims             = numdims;
                                        // We can do this as TGOParam is basically a struct without any internal allocation
Params.Resize ( NumDims );

for ( int i = 0; i < NumDims; i++ )
    Params[ i ].Reset ();
}


void    TGOParams::AddDim ()
{
NumDims++;
                                        // We can do this as TGOParam is basically a struct without any internal allocation
Params.Resize ( NumDims, (MemoryAllocationType) ( MemoryAuto | ResizeKeepMemory ) );

Params[ NumDims - 1 ].Reset ();
}


void    TGOParams::AddDim ( int type, double min, double max, int numsteps, int numsubsteps, double zoomfactor )
{
AddDim ();

Params[ NumDims - 1 ].Set ( numsteps, numsubsteps, zoomfactor );
Params[ NumDims - 1 ].Type      = type;
Params[ NumDims - 1 ].Min       = min;
Params[ NumDims - 1 ].Max       = max;
}


void    TGOParams::SetFixedParam ( int type, double value )
{
                                        // create / update pair
FixedParams[ type ]  = value;
}


void    TGOParams::Set ( int numsteps, int numsubsteps, double zoomfactor )
{
for ( int i = 0; i < NumDims; i++ )
    Params[ i ].Set ( numsteps, numsubsteps, zoomfactor );
}


void    TGOParams::ZoomIn ( double *newcenters )
{
for ( int i = 0; i < NumDims; i++ )
    Params[ i ].ZoomIn ( newcenters[ i ] );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TSimplex::TSimplex()
{
Reset ();
}


        TSimplex::~TSimplex()
{
Reset ();
}


        TSimplex::TSimplex ( const TSimplex& op )
{
Vertex      = op.Vertex;
Value       = op.Value;
Centroid    = op.Centroid;
BestVertex  = op.BestVertex;
}


TSimplex&   TSimplex::operator= ( const TSimplex& op2 )
{
if ( &op2 == this )
    return  *this;

Vertex      = op2.Vertex;
Value       = op2.Value;
Centroid    = op2.Centroid;
BestVertex  = op2.BestVertex;

return  *this;
}


//----------------------------------------------------------------------------
void    TSimplex::Reset ()
{
Vertex.resize ( 0 );
Value   .DeallocateMemory ();
Centroid.DeallocateMemory ();
BestVertex  = -1;
}


//----------------------------------------------------------------------------
                                        // Allocating data structure
void    TSimplex::Set ( int simplexdim )
{
Reset ();

int                 numdims         = simplexdim - 1;   // simplex dimension = dimensions + 1

Vertex.resize ( simplexdim );

for ( int v = 0; v < simplexdim; v++ )
    Vertex[ v ].Resize ( numdims );

Value   .Resize ( simplexdim );
Centroid.Resize ( numdims    );
}

                                        // Allocating data structure + computing vertices (but NOT values)
void    TSimplex::Set ( TBunchOfGOParam& params )
{
Reset ();

if ( params.IsNotAllocated () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Allocating variables
int                 numdims         = (int) params;
int                 simplexdim      = numdims + 1;


Set ( simplexdim );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setting initial values
for ( int v = 0; v < simplexdim; v++ ) {

    for ( int p = 0; p < numdims; p++ ) {
    
        if ( v == 0 )
                                        // Center vertex
            Vertex[ v ][ p ]   = params[ p ]->GetMiddle ();

        else
                                        // All other vertex are summit of each dimension in a row
            Vertex[ v ][ p ]   = p == v - 1 ? params[ p ]->Max : Vertex[ 0 ][ p ];
        } // numdims
    }
}

                                        // Transferring vertex (parameters) to searched Values
void    TSimplex::ToParams ( int vertexi, TBunchOfGOParam& params )  const
{
for ( int p = 0; p < (int) params; p++ )
                                    // setting parameter value
    params[ p ]->Value  = Vertex[ vertexi ][ p ];
//  params[ p ]->Value  = Clip ( Vertex[ vertexi ][ p ], params[ p ]->Min, params[ p ]->Max );
}


void    TSimplex::BestToParams ( TBunchOfGOParam& params )  const
{
ToParams ( BestVertex, params );
}


//----------------------------------------------------------------------------
double  TSimplex::GetRadius ()     const
{
if ( IsNotAllocated () )
    return  0;


int                 simplexdim      = Vertex.size ();
int                 numdims         = simplexdim - 1;   // simplex dimension = dimensions + 1
double              simplexradius   = 0;


for ( int v = 0; v < simplexdim; v++ )
for ( int p = 0; p < numdims; p++ )

    simplexradius  += Square ( Vertex[ v ][ p ] - Centroid[ p ] );

simplexradius   = sqrt ( simplexradius / simplexdim );

return  simplexradius;
}


void    TSimplex::GetStatsValues ( TEasyStats& stats )     const
{
                                        // thread-safe in case we parallelize code
stats.Set ( Value, false );
}


//----------------------------------------------------------------------------
void    TSimplex::Show ( const char *title, int index )    const
{
char                localtitle[ 256 ];
char                buff      [ 32 ];

StringCopy  ( localtitle, title, IntegerToString ( buff, index ) );


if ( IsNotAllocated () ) {
    ShowMessage ( "- empty -", localtitle );
    return;
    }


int                 simplexdim      = Vertex.size ();
int                 numdims         = simplexdim - 1;   // simplex dimension = dimensions + 1
char                info      [ 256 ];

for ( int v = 0; v < simplexdim; v++ ) {

    StringCopy  ( info, "Vertex#", IntegerToString ( buff, v + 1 ), NewLine NewLine  );

    for ( int p = 0; p < numdims; p++ )
        StringAppend    ( info, Tab "Parameter#", IntegerToString ( buff, p + 1 ), Tab "= ", FloatToString ( Vertex[ v ][ p ], 6 ), NewLine );

    ShowMessage ( info, localtitle );
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TGlobalOptimize::TGlobalOptimize ()
{
NumGroups           = 0;
Reset ();
}


        TGlobalOptimize::TGlobalOptimize ( int numsteps, int numsubsteps, double zoomfactor )
{
NumGroups           = 0;
Reset ();

                                        // remember these for later calls to AddDim
NumSteps            = Clip ( numsteps,      NumStepsMin,        NumStepsMax      );
NumSubSteps         = Clip ( numsubsteps,   NumSubStepsMin,     NumSubStepsMax   );
ZoomFactor          = Clip ( zoomfactor,    NumZoomFactorMin,   NumZoomFactorMax );
}


        TGlobalOptimize::~TGlobalOptimize ()
{
Reset ();
}

                                        // Delete all allocated objects & reset all the other fields
void    TGlobalOptimize::Reset ()
{
Groups.clear ();

ResetToValue ();


NumGroups           = 0;
How                 = -1;
CurrentPrecision    = 1;
OutliersPrecision   = 0;
RequestedPrecision  = 0;

NumSteps            = 0;
NumSubSteps         = 0;
ZoomFactor          = 0;
}


//----------------------------------------------------------------------------
void    TGlobalOptimize::AddGroup ()
{
NumGroups++;

Groups.push_back ( *new TGOParams );
}

                                        // in case the caller wants to override the default settings
void    TGlobalOptimize::AddGroup ( int numsteps, int numsubsteps, double zoomfactor )
{
AddGroup ();
                                        // remember these for later calls to AddDim
NumSteps            = Clip ( numsteps,      NumStepsMin,        NumStepsMax      );
NumSubSteps         = Clip ( numsubsteps,   NumSubStepsMin,     NumSubStepsMax   );
ZoomFactor          = Clip ( zoomfactor,    NumZoomFactorMin,   NumZoomFactorMax );
}


//----------------------------------------------------------------------------
void    TGlobalOptimize::AddDim ( int group, int type, double min, double max )
{
if ( ! IsInsideLimits ( group, 0, NumGroups - 1 ) )
    return;
                                        // add to last group, using saved parameters
Groups[ group ].AddDim ( type, min, max, NumSteps, NumSubSteps, ZoomFactor );
                                        // it does not hurt to update this right now
SetToValue ( Groups[ group ][ Groups[ group ].GetNumDims () - 1 ] );
}


void    TGlobalOptimize::AddDim ( int type, double min, double max )
{
AddDim ( NumGroups - 1, type, min, max );
}


void    TGlobalOptimize::SetFixedParam ( int group, int type, double value )
{
if ( ! IsInsideLimits ( group, 0, NumGroups - 1 ) )
    return;
                                        // set to last group
Groups[ group ].SetFixedParam ( type, value );
}


void    TGlobalOptimize::SetFixedParam ( int type, double value )
{
SetFixedParam ( NumGroups - 1, type, value );
}


int     TGlobalOptimize::GetTotalDims ()    const
{
int                 dims            = 0;

for ( int g = 0; g < NumGroups; g++ )
    dims   += Groups[ g ].GetNumDims ();

return  dims;
}


//----------------------------------------------------------------------------
void    TGlobalOptimize::Set ( int numsteps, int numsubsteps, double zoomfactor )
{
                                        // remember these for later calls to AddDim
NumSteps            = Clip ( numsteps,      NumStepsMin,        NumStepsMax      );
NumSubSteps         = Clip ( numsubsteps,   NumSubStepsMin,     NumSubStepsMax   );
ZoomFactor          = Clip ( zoomfactor,    NumZoomFactorMin,   NumZoomFactorMax );


for ( int g = 0; g < NumGroups; g++ )
    Groups[ g ].Set ( numsteps, numsubsteps, zoomfactor );
}

                                        // Offset of group's parameters when linearly concatenated
int     TGlobalOptimize::GetGroupOffset ( int group )
{
int                 offset          = 0;

for ( int g = 0; g < group; g++ )
    offset += Groups[ g ].GetNumDims ();

return  offset;
}


void    TGlobalOptimize::ZoomIn ( double *newcenters )
{
for ( int g = 0, paramoffset = 0; g < NumGroups; paramoffset += Groups[ g ].GetNumDims (), g++ )
    Groups[ g ].ZoomIn ( newcenters + paramoffset );
}


//----------------------------------------------------------------------------
                                        // ToValue & buddies are just pointers
void    TGlobalOptimize::ResetToValue ()
{
ToValues.clear ();
ToMin   .clear ();
ToMax   .clear ();
}


void    TGlobalOptimize::SetToValue ( TGOParam& param )
{
                                        // pair of <parameter_type, pointer_to_data>
                                        // will create the association, or update it if it alread exists
ToValues[ param.Type ]  = &param.Value;
ToMin   [ param.Type ]  = &param.Min;
ToMax   [ param.Type ]  = &param.Max;
}


void    TGlobalOptimize::SetToValue ()
{
ResetToValue ();

                                        // browse through all groups/dimensions, and store to quick access ToValue
for ( int g   = 0; g   < NumGroups; g++ )
for ( int dim = 0; dim < Groups[ g ].GetNumDims (); dim++ )

    SetToValue ( Groups[ g ][ dim ] );
}


//----------------------------------------------------------------------------
                                        // Currently just outputing all parameters, following actual group/dim sequence
void    TGlobalOptimize::WriteParameters ( const char* file )   const
{
if ( StringIsEmpty ( file ) )
    return;


ofstream            ofs ( file );

ofs << StreamFormatGeneral;


for ( int g   = 0; g   < NumGroups; g++ )
for ( int dim = 0; dim < Groups[ g ].GetNumDims (); dim++ )

    ofs << StreamFormatInt32 <<  Groups[ g ][ dim ].Type << "\t" << StreamFormatFloat64 << Groups[ g ][ dim ].Value << "\n";


ofs.close ();
}
                                        // Any missing parameter will be added to the model
                                        // !Additional parameters will be put arbitrarily in the last group, so if groups/parameters arangement matter, it'd better be done before!
void    TGlobalOptimize::ReadParameters ( const char* file )
{
if ( StringIsEmpty ( file ) )
    return;


ifstream            ifs ( file );
char                buff[ 1024 ];
int                 type;
double              value;

                                        // making sure pointers to existing parameters exist
SetToValue ();


do {
    type    = StringToInteger   ( GetToken ( &ifs, buff ) );
    value   = StringToDouble    ( GetToken ( &ifs, buff ) );

                                        // parameter exists?
    if ( HasValue ( type ) )
    
        SetValue ( type, value );
    
    else {                              // parameter does NOT exist
                                        // forcefully create a parameter and/or a group - we don't really care where it is put, as indeed only its value will be reused
        if ( NumGroups == 0 )
            AddGroup ();

        int             g           = NumGroups - 1;
                                        // we already know it is missing, just add one
        Groups[ g ].AddDim ();

        int             dim         = Groups[ g ].GetNumDims () - 1;


        TGOParam&       param       = Groups[ g ][ dim ];
                                        // setting parameter with values
                        param.Type  = type;
                        param.Value = value;
                        param.Min   = value;    // not really needed, just by consistency
                        param.Max   = value;    // idem

        SetToValue ( param );
        }

    } while ( ifs.good () );


ifs.close ();
}


//----------------------------------------------------------------------------
                                        // Process a single group, fully scanning the whole defined box of parameters
double  TGlobalOptimize::BoxScan    (   TBunchOfGOParam&    params, 
                                        double*             bestvalues, 
                                        TEasyStats&         stats 
                                    )
{
stats.Reset ();

int                 numparams   = (int) params;


TArray1<int>        indexes    ( numparams );
                                        // allocate a grid to explore all the space
for ( int p = 0; p < numparams; p++ )

    indexes[ p ]    = params[ p ]->NumSteps;

TMultiArray<double> results ( indexes );

//results.Reset ();                     // done at creation


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numsubbuckets   = 1;        // amount of data collected into each results bin
TArray1<int>        subindexes ( numparams );

for ( int p = 0; p < numparams; p++ ) {

    subindexes[ p ] = params[ p ]->GetTotalSteps ();

    numsubbuckets  *= params[ p ]->NumSubSteps;
    }

TMultiArray<double> subgrid ( subindexes );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // This method looks at all dimensions at the same time
                                        // searching for the min in the n-dimensional space at once

                                        // linear loop across sub-grid
for ( int subli = 0; subli < subgrid.GetLinearDim (); subli++ ) {

    subgrid.LinearToIndexes ( subli, subindexes );


    for ( int p = 0; p < numparams; p++ )
                                        // setting current parameter' value
        params[ p ]->Value  = params[ p ]->GetSubValue ( subindexes[ p ] );


    double          result      = Evaluate ();

                                        // downsample to grid
    for ( int p = 0; p < numparams; p++ )

        indexes[ p ]    = subindexes[ p ] / params[ p ]->NumSubSteps;
                                        // cumulating within results bucket
    results ( indexes )    += result;

                                        // stats on all available results
    stats.Add ( result, ThreadSafetyIgnore );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // search for local min
double              minresult       = DBL_MAX;

for ( int li = 0; li < results.GetLinearDim (); li++ ) {

                                        // !rescaling to allow proper comparisons across groups with different subsampling!
    results[ li ]  /= numsubbuckets;

                                        // stat on smoothed results
//  stats.Add ( results[ li ], ThreadSafetyIgnore );


    if ( results[ li ] <= minresult ) {

        minresult    = results[ li ];

        results.LinearToIndexes ( li, indexes );

        for ( int p = 0; p < numparams; p++ ) {

            bestvalues[ p ]     = params[ p ]->GetValue ( indexes[ p ] );
                                        // Setting parameters(?)
//          params[ p ]->Value  = bestvalues[ p ];
            }
        }
    }

                                        // smallest result evaluated
return  minresult;
}


//----------------------------------------------------------------------------
                                        // Process a single group, scanning the whole axis of parameters
double  TGlobalOptimize::CrossHairScan  (   TBunchOfGOParam&    params, 
                                            double*             bestvalues, 
                                            TEasyStats&         stats 
                                        )
{
stats.Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numparams   = (int) params;
TArray1<int>        minindexes ( numparams );
TArray1<double>     minvalues  ( numparams );

minindexes  = -1;
minvalues   = DBL_MAX;

                                        // Each axis is evaluated independently of the others, hence being crosshair scan
for ( int p = 0; p < numparams; p++ ) {
    
    double          savedvalue      = params[ p ]->Value;
    double          result          = 0;
    double          value           = 0;


    for ( int substepi = 0; substepi < params[ p ]->GetTotalSteps (); substepi++ ) {
                                        // setting current parameter' value
        params[ p ]->Value  = params[ p ]->GetSubValue ( substepi );
                                        // cumulate current at grid resolution
        result      = Evaluate ();

        value      += result;

                                        // stats on all available results
        stats.Add ( result, ThreadSafetyIgnore );

                                        // right amount of subsampled data?
        if ( ( substepi + 1 ) % params[ p ]->NumSubSteps != 0 )
            continue;

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        value  /= params[ p ]->NumSubSteps;
                                        // stat on smoothed results
//      stats.Add ( value, ThreadSafetyIgnore );


        if ( value < minvalues[ p ] ) {

            minvalues [ p ] = value;
            minindexes[ p ] = substepi / params[ p ]->NumSubSteps;
            }
                                        // reset
        value   = 0;
        } // for sub-grid

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // restore original parameter's value
    params[ p ]->Value  = savedvalue;
    } // for parameter


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Each parameter is done independently
for ( int p = 0; p < numparams; p++ ) {

    bestvalues[ p ]     = params[ p ]->GetValue ( minindexes[ p ] );
                                        // Setting parameters(?)
    params[ p ]->Value  = bestvalues[ p ];
    }

                                        // Value of estimated min location
double              minresult       = Evaluate ();
                                        // !There is a subtle difference here with BoxScan: it returns only a single point evaluation, whereas BoxScan returns an average of evaluations!
return  minresult;
}


//----------------------------------------------------------------------------
                                        // Create simplex & evaluate all of its vertices
void    TGlobalOptimize::InitSimplex ( TBunchOfGOParam& params, TSimplex& simplex )
{
simplex.Reset ();

if ( params.IsNotAllocated () )
    return;

                                        // Allocating & setting vertices
simplex.Set ( params );
                                        // Computing values for each vertex
EvaluateSimplexValues ( params, simplex );
}

                                        // Just evaluating simplex
void    TGlobalOptimize::EvaluateSimplexValues ( TBunchOfGOParam& params, TSimplex& simplex )
{
                                        // Computing values for each vertex
for ( int v = 0; v < simplex.GetSimplexDim (); v++ ) {
                                        // Set parameters from simplex
    simplex.ToParams ( v, params );
                                        // Compute & store value for this vertex
    simplex.Value ( v ) = Evaluate ();
    }
}


//----------------------------------------------------------------------------
                                        // Nelder Mead simplex (http://www.scholarpedia.org/article/Nelder-Mead_algorithm)
                                        // !This needs at least 2 parameters!
                                        // Parameters stay strictly within the specified range, so make some more room if needed
double  TGlobalOptimize::NelderMead (   TBunchOfGOParam&    params, 
                                        TSimplex&           simplex, 
                                        TEasyStats&         stats, 
                                        bool                forceevaluate 
                                    )
{
stats.Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Simplex initialization
if ( simplex.IsNotAllocated () )
                                        // Init & evaluate
    InitSimplex ( params, simplex );

else if ( forceevaluate )
                                        // Caller requested force evaluation - simplex was restored and values are not up-to-date
    EvaluateSimplexValues ( params, simplex );


int                 numdims         = simplex.GetDim ();
int                 simplexdim      = simplex.GetSimplexDim ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Get worst, second to worst and best indexes
                                        // ! This needs at least 3 parameters !
double              worstv1             = -DBL_MAX;
double              worstv2             = -DBL_MAX;
double              bestv               =  DBL_MAX;
int                 worsti1             = -1;
//int               worsti2             = -1;
                    simplex.BestVertex  = -1;


for ( int v = 0; v < simplexdim; v++ ) {
                                        // two worst indexes, i.e. highest values
    if      ( simplex.Value ( v ) > worstv1 ) {
      /*worsti2     = worsti1;*/    worstv2     = worstv1;
        worsti1     = v;            worstv1     = simplex.Value ( v );
        }
    else if ( simplex.Value ( v ) > worstv2 ) {
      /*worsti2     = v;*/          worstv2     = simplex.Value ( v );
        }
                                        // in another loop, testing for BestVertex != worsti1 ?
    if      ( simplex.Value ( v ) < bestv ) {
        simplex.BestVertex  = v;    bestv       = simplex.Value ( v );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Compute centroid of all vertices, except the worst one
simplex.Centroid.ResetMemory ();

for ( int v = 0; v < simplexdim; v++ )
    if ( v != worsti1 )
        simplex.Centroid    += simplex[ v ];

simplex.Centroid    /= ( simplexdim - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Constants for simplex transformation

                                        // Historical values
//#define             NelderMeadReflect       1.0
//#define             NelderMeadExpand        2.0
//#define             NelderMeadContract      0.5
//#define             NelderMeadShrink        0.5

                                        // Fan & Zahara
//#define             NelderMeadReflect       1.50
//#define             NelderMeadExpand        2.75
//#define             NelderMeadContract      0.75
//#define             NelderMeadShrink        0.50

                                        // Wang & Shoup
//#define             NelderMeadReflect       1.29
//#define             NelderMeadExpand        2.29
//#define             NelderMeadContract      0.464
//#define             NelderMeadShrink        0.571

                                        // Cartool: max ( F&Z, W&S )
#define             NelderMeadReflect       1.50
#define             NelderMeadExpand        2.75
#define             NelderMeadContract      0.75
#define             NelderMeadShrink        0.60


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Copy the best simplex to parameters
auto                BestSimplexToParams = [ & ] ()
{
                                        // restoring parameters from best vertex
simplex.BestToParams    ( params );
                                        // this is usually done after the whole convergence has been reached, to evaluate the final results
simplex.GetStatsValues  ( stats );
                                        // be nice and also return the best value
return  simplex.GetBestValue ();
};

                                        // Handy transfer from a vector to parameters
auto                VectorToParams      = [ &params ] ( const TVector<double>& v )
{
for ( int p = 0; p < (int) params; p++ )
                                        // setting parameter value
    params[ p ]->Value  = v[ p ];
//  params[ p ]->Value  = Clip ( v[ p ], params[ p ]->Min, params[ p ]->Max );
};


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 5.1) Reflect
TVector<double>     SimplexReflect  = simplex.Centroid   * ( 1 + NelderMeadReflect ) 
                                    - simplex[ worsti1 ] *       NelderMeadReflect;
                                        // copy to parameters
VectorToParams  ( SimplexReflect );

                                        // evaluate @ vertex position
double              reflectv        = Evaluate ();

                                        // better than second worst, but not better than best?
if ( reflectv < worstv2 && reflectv >= bestv ) {
                                        // replace worst with it
    simplex       [ worsti1 ]   = SimplexReflect;
    simplex.Value ( worsti1 )   = reflectv;

    return  BestSimplexToParams ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 5.2) Expand
if ( reflectv < bestv ) {

    TVector<double>     SimplexExpand   = simplex.Centroid   * ( 1 + NelderMeadExpand ) 
                                        - simplex[ worsti1 ] *       NelderMeadExpand;
                                        // copy to parameters
    VectorToParams  ( SimplexExpand );

                                        // evaluate @ vertex position
    double              expandv         = Evaluate ();

                                        // pick the best one from expand or reflect
    if ( expandv < reflectv ) {
        simplex       [ worsti1 ]   = SimplexExpand;
        simplex.Value ( worsti1 )   = expandv;
        }
    else {
        simplex       [ worsti1 ]   = SimplexReflect;
        simplex.Value ( worsti1 )   = reflectv;
        }

    return  BestSimplexToParams ();
    } // SimplexExpand


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 5.3) Contract

                                        // contract to either side of the centroid
TVector<double>     SimplexContract = reflectv < worstv1 ?  simplex.Centroid * ( 1 + NelderMeadContract ) - simplex[ worsti1 ] * NelderMeadContract
                                                         :  simplex.Centroid * ( 1 - NelderMeadContract ) + simplex[ worsti1 ] * NelderMeadContract;
                                        // copy to parameters
VectorToParams  ( SimplexContract );

                                        // evaluate @ vertex position
double              contractv       = Evaluate ();

                                        // better than these two?
if ( contractv < min ( reflectv, worstv1 ) ) {
                                        // replace worst with it
    simplex       [ worsti1 ]   = SimplexContract;
    simplex.Value ( worsti1 )   = contractv;

    return  BestSimplexToParams ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 5.4) Shrink

                                        // shrink all but the best vertex
for ( int v = 0; v < simplexdim; v++ ) {

    if ( v == simplex.BestVertex )
        continue;


    simplex[ v ]    = simplex[ simplex.BestVertex ] * ( 1 - NelderMeadShrink ) 
                    + simplex[ v                  ] *       NelderMeadShrink;
                                        // copy to parameters
    VectorToParams  ( simplex[ v ] );

    simplex.Value ( v )  = Evaluate ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // need to find the best value
bestv           =  DBL_MAX;

for ( int v = 0; v < simplexdim; v++ )

    if ( simplex.Value ( v ) < bestv ) {
        simplex.BestVertex  = v;    bestv       = simplex.Value ( v );
        }


return  BestSimplexToParams ();
}


//----------------------------------------------------------------------------
                                        //      Notes on the various methods implemented here:
                                        // We can not know in advance which method will perform best for some specifi, real-case scenario.
                                        // Each method has to be tried and results assessed to see if the problem at hand is being solved.
                                        // There will hardly be a single method that will work on all cases. Also speed differs widely across different methods.

void    TGlobalOptimize::GetSolution    (   GOMethod    method,             int             how, 
                                            double      requestedprecision, double          outliersprecision, 
                                            const char* title,
                                            TEasyStats* stat 
                                        )
{
                                        // store current processing (not resetted until another call)
How                 = how;

if ( IsNotSet () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // concatenate / flatten all parameters, which is more handy - also we might have even more parameters hierarchy at some point
TBunchOfGOParam     allparams;

                                        // Flatten all parameters
auto    GetAllParameters            = [ & ] ( TBunchOfGOParam& params ) {

    params.Resize ( GetTotalDims () );

    for ( int g   = 0, p = 0; g   < NumGroups; g++ )
    for ( int dim = 0;        dim < Groups[ g ].GetNumDims (); dim++, p++ )

        params[ p ]    = &Groups[ g ][ dim ];
    };

                                        // Same but only for 1 group
auto    GetGroupParameters          = [ & ] ( int group, TBunchOfGOParam& params ) {

    params.Resize ( Groups[ group ].GetNumDims () );

    for ( int dim = 0, p = 0; dim < Groups[ group ].GetNumDims (); dim++, p++ )

        params[ p ]    = &Groups[ group ][ dim ];
    };

                                        // Same but only for 1 group
auto    GetDimensionParameter       = [ & ] ( int p, TBunchOfGOParam& params ) {

    params.Resize ( 1 );
                                    // setting single parameter
    params[ 0 ]     = allparams[ p ];
    };

                                        // Flatten all parameters into a single structure
GetAllParameters ( allparams );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numparams       = (int) allparams;
TArray1<double>     savedvalues ( numparams );
TArray1<double>     bestvalues  ( numparams );
TArray1<double>     evals       ( numparams );
TArray1<double>     evalsds     ( numparams );
double              eval            = 0;
double              evalsd          = DBL_MAX;
TEasyStats          stats;
double              paramsradius        = 0;
double              valuesradius        = 0;
double              paramsradiusinit    = 0;
double              valuesradiusinit    = 0;

                                        // saving all parameters' values
auto    SetAllParametersValues      = [ & ] () {

    for ( int p = 0; p < numparams; p++ )
                                        // setting default Value
        allparams[ p ]->Value   = allparams[ p ]->GetMiddle ();
    };

                                        // saving all parameters' values
auto    SaveAllParametersValues     = [ & ] () {

    for ( int p = 0; p < numparams; p++ )
                                        // saving
        savedvalues[ p ]        = allparams[ p ]->Value;
    };

                                        // restoring whole group, paying attention to the concatenated offset
auto    RestoreGroupParameterValues = [ & ] ( int g ) {

    for ( int dim = 0, paramoffset = GetGroupOffset ( g ); dim < Groups[ g ].GetNumDims (); dim++, paramoffset++ )

        allparams[ paramoffset ]->Value   = savedvalues[ paramoffset ];
    };

                                        // restoring a given parameter' value
auto    RestoreDimensionParameterValue  = [ & ] ( int p ) {

    allparams[ p ]->Value   = savedvalues[ p ];
    };


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

CurrentPrecision    = 1;
OutliersPrecision   = outliersprecision;    // set to 0 for not using it
RequestedPrecision  = requestedprecision;   // just storing - could be of interest for derived classes

                                        // automatic outliers precision threshold
//OutliersPrecision   = Power ( 10, Log10 ( requestedprecision ) / 2 );
//DBGV2 ( requestedprecision, OutliersPrecision, "Precision  OutliersPrecision" );
//DBGM ( GetGOMethodName ( method ), "TGlobalOptimize::GetSolution" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//#define     DebugGlobalOptimizeConvergence

#if defined (DebugGlobalOptimizeConvergence)
TSuperGauge         GaugeLoop ( "Iteration", GOMaxIterations, SuperGaugeLevelBatch, SuperGaugeLinearCount );

TFileName           _file;
StringCopy          ( _file, "E:\\Data\\GlobalOptimize.", GetGOMethodName ( method ), ".Convergence.txt" );
CheckNoOverwrite    ( _file );
TVerboseFile        ooo ( _file, 40 );
char                buff[ 256 ];

ooo.NextTopic ( GetGOMethodName ( method ) );

ooo.ResetTable ();

ooo.TableColNames.Add ( "Iteration" );
ooo.TableColNames.Add ( "Eval" );
ooo.TableColNames.Add ( "EvalSD" );
ooo.TableColNames.Add ( "Group" );
ooo.TableColNames.Add ( "PRadius" );
ooo.TableColNames.Add ( "VRadius" );
ooo.TableColNames.Add ( "IterPrecision" );
ooo.TableColNames.Add ( "Precision" );

for ( int p = 0; p < numparams; p++ )
    ooo.TableColNames.Add ( StringCopy ( buff, "p", IntegerToString ( p + 1 ) ) );

ooo.BeginTable ( 12 );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              currgauge       = 0;
bool                showprogress    = StringIsNotEmpty ( title );
TRandUniform        randunif;

TSuperGauge         Gauge ( StringIsEmpty ( title ) ? "Global Optimization" : title, showprogress ? 100 : 0 );

ShowProgress ();                        // up to the derived class


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set array of pointers to values
SetToValue ();

                                        // setting all initial values
SetAllParametersValues ();

                                        
unordered_map<int,TSimplex>     simplex;        // storing all needed simplexes
unordered_map<int,TSimplex>     tempsimplex;    // saving simplexes might be necessary
TBunchOfGOParam                 params;         // used to select only some required parameters
int                 worstgroup      = -1;
int                 worstp          = -1;

                                        // Loop until some hard-limit number of iterations
for ( Iteration = 0; Iteration < GOMaxIterations; Iteration++ ) {

    switch ( method ) {

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Looking at all groups & dimensions at once, browsing systematically all parameters
      case GlobalBoxScan:
      case GlobalCrossHairScan:

        eval        = method == GlobalBoxScan ? BoxScan       ( allparams, &bestvalues[ 0 ], stats )
                                              : CrossHairScan ( allparams, &bestvalues[ 0 ], stats );

        evalsd      = stats.SD ();
                                        // zoom in to new center
        ZoomIn ( &bestvalues[ 0 ] );

        break;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Looking at all groups & dimensions at once, browsing with the Simplex method
      case GlobalNelderMead:
                                        // there is a single simplex, consequently, it has always up-to-date values
        eval        = NelderMead ( allparams, simplex[ 0 ], stats );

        evalsd      = stats.SD ();

        break;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Cycling through all groups, one at a time, browsing systematically all parameters
      case CyclicalBoxScan:
      case CyclicalCrossHairScan:

        eval        = 0;
        evalsd      = 0;


        for ( int g = 0, paramoffset = 0; g < NumGroups; paramoffset += Groups[ g ].GetNumDims (), g++ ) {
                                        // evaluate current group, with correct access within the bestvalues array
            GetGroupParameters ( g, params );

            eval       += method == CyclicalBoxScan ? BoxScan       ( params, &bestvalues[ paramoffset ], stats )
                                                    : CrossHairScan ( params, &bestvalues[ paramoffset ], stats );
            evalsd     += stats.Variance ();
                                        // zoom in to new center
            Groups[ g ].ZoomIn ( &bestvalues[ paramoffset ] );
            } // for group

                                        // compute the mean of all evals, which is better than just the only last one
        eval       /= NumGroups;
        evalsd      = sqrt ( evalsd ) / NumGroups;

        break;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Cycling through all groups, one at a time, browsing with the Simplex method
      case CyclicalNelderMead:

        eval        = 0;
        evalsd      = 0;


        for ( int g = 0; g < NumGroups; g++ ) {
                                        // evaluate current group, with correct access within the bestvalues array
            GetGroupParameters ( g, params );
                                        // starts from current parameters state, then update it - so no need to force re-evaluate values
            eval           += NelderMead ( params, simplex[ g ], stats );
            evalsd         += stats.Variance ();
            } // for group

                                        // compute the mean of all evals, which is better than just the only last one
        eval       /= NumGroups;
        evalsd      = sqrt ( evalsd ) / NumGroups;

        break;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Focusing on the worst group, browsing systematically all parameters
      case WeakestGroupBoxScan:
      case WeakestGroupCrossHairScan:

        SaveAllParametersValues ();

                                        // process each group independently
        for ( int g = 0, paramoffset = 0; g < NumGroups; paramoffset += Groups[ g ].GetNumDims (), g++ ) {
                                        // evaluate current group, with correct access within the bestvalues array
            GetGroupParameters ( g, params );

            evals   [ g ]   =  method == WeakestGroupBoxScan ? BoxScan       ( params, &bestvalues[ paramoffset ], stats )
                                                             : CrossHairScan ( params, &bestvalues[ paramoffset ], stats );
            evalsds [ g ]   = stats.SD ();

            RestoreGroupParameterValues ( g );
            }

                                        // search for max SD (worst group)
        eval            = 0;
        evalsd          = -DBL_MAX;
        worstgroup      = -1;

        for ( int g = 0; g < NumGroups; g++ )

            if ( evalsds[ g ] > evalsd ) {

                worstgroup  = g;
                eval        = evals     [ g ];
                evalsd      = evalsds   [ g ];
                }

                                        // zoom only into the worst group, this will slowly by slowly levelize all groups
        Groups[ worstgroup ].ZoomIn ( &bestvalues[ GetGroupOffset ( worstgroup ) ] );

        break;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Focusing on the worst group, browsing systematically all parameters
                                        // Theoretically, it requires about 3 times more calls to Evaluate, though some tests have shown a 10x slowdown
      case WeakestGroupNelderMead:

        SaveAllParametersValues ();


        if ( Iteration == 0 ) {
                                        // simplexes have not been allocated yet - let's create them all
            for ( int g = 0; g < NumGroups; g++ ) {

                GetGroupParameters ( g, params );
                                        // force init simplex now, with all the other parameters correctly centered
                InitSimplex ( params, simplex[ g ] );

//              simplex[ g ].Show ( "Init Simplex#", g + 1 );
                                        // restore these parameters to center
                RestoreGroupParameterValues ( g );
                }
            }

                                        // process each group independently
        for ( int g = 0; g < NumGroups; g++ ) {
                                        // evaluate current group
            GetGroupParameters ( g, params );
                                        // we also need to back-track on the simplex itelsef, not only the values
            TSimplex        savedsimplex    = simplex[ g ];

                                        // !force re-evaluate this simplex values, as other parameters have been modified from another simplex!
            evals   [ g ]   = NelderMead ( params, simplex[ g ], stats, Iteration > 0 && g != worstgroup );  // worstgroup is up-to-date, no need to re-evaluate it
            evalsds [ g ]   = stats.SD ();

                                        // save processed simplex state for later
            tempsimplex[ g ]= simplex[ g ];

                                        // restoring both parameters AND simplex state
            RestoreGroupParameterValues ( g );

            simplex[ g ]    = savedsimplex;
            }

                                        // search for max SD (worst group)
        eval            = 0;
        evalsd          = -DBL_MAX;
        worstgroup      = -1;

        for ( int g = 0; g < NumGroups; g++ )

            if ( evalsds[ g ] > evalsd ) {

                worstgroup  = g;
                eval        = evals     [ g ];
                evalsd      = evalsds   [ g ];
                }

                                        // apply only this simplex (could be saved earlier and copied here, too)
        GetGroupParameters ( worstgroup, params );
                                        // restoring both vertices AND values
        simplex[ worstgroup ]   = tempsimplex[ worstgroup ];
                                        // & propagate its parameters
        simplex[ worstgroup ].BestToParams ( params );

//      simplex[ worstgroup ].Show ( "Worst-post Simplex#", worstgroup + 1 );

        break;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Focusing on the worst dimension, browsing systematically all parameters
      case WeakestDimensionBoxScan:
      case WeakestDimensionCrossHairScan:

        SaveAllParametersValues ();

                                        // process each dimension independently
        for ( int p = 0; p < numparams; p++ ) {
                                        // setting single parameter
            GetDimensionParameter ( p, params );
                                        // evaluate current dimension, with correct access within the bestvalues array
//          evals   [ p ]   = method == WeakestDimensionBoxScan ? BoxScan       ( params, &bestvalues[ p ], stats )     // one dimension at a time, the two are equivalent, we can use the fastest one all the time
//                                                              : CrossHairScan ( params, &bestvalues[ p ], stats );
            evals   [ p ]   = CrossHairScan ( params, &bestvalues[ p ], stats );
            evalsds [ p ]   = stats.SD ();
                                        // restore original values, for a correct evaluation of the other groups
            RestoreDimensionParameterValue ( p );
            } // for dim

                                        // search for max SD (worst dimension)
        eval            = 0;
        evalsd          = -DBL_MAX;
        worstp          = -1;

        for ( int p = 0; p < numparams; p++ )

            if ( evalsds[ p ] > evalsd ) {

                worstp      = p;
                eval        = evals     [ p ];
                evalsd      = evalsds   [ p ];
                }

                                        // zoom only into the worst dimension, this will slowly by slowly levelize all groups
        allparams[ worstp ]->ZoomIn ( bestvalues[ worstp ] );

        break;

        } // switch method


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // does nothing here, but derived classes might want to show something
    ShowProgress ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // radius on the searched parameter space
    if      ( IsMethodScanBox       ( method ) 
           || IsMethodCrossHairScan ( method ) ) {

        paramsradius    = 0;

        for ( int p = 0; p < numparams; p++ )
            paramsradius += Square ( allparams[ p ]->GetRange () / 2 );

        paramsradius   = sqrt ( paramsradius );
        }

    else if ( IsMethodNelderMead ( method ) ) {

        paramsradius    = 0;

        for ( auto s = simplex.begin (); s != simplex.end (); ++s )
            paramsradius += Square ( s->second.GetRadius () );

        paramsradius   = sqrt ( paramsradius );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // radius on the evaluation space - using sqrt to get a high bound
    valuesradius    = sqrt ( evalsd );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // use first values as rescaling units
    if ( Iteration == 0 ) {
        paramsradiusinit    = NonNull ( paramsradius );
        valuesradiusinit    = NonNull ( valuesradius );
        }
                                        // take any forthcoming max as rescaling units -> max can change in time
//  Maxed ( paramsradiusinit, paramsradius );
//  Maxed ( valuesradiusinit, valuesradius );

                                        // use relative values
    paramsradius   /= paramsradiusinit;
    valuesradius   /= valuesradiusinit;

                                        // finally, the precision of the current iteration
//  double      iterprecision       = max ( paramsradius, valuesradius );               // worst (max) of these 2 estimates
    double      iterprecision       = min ( paramsradius, valuesradius );               // best (min) of these 2 estimates
//  double      iterprecision       = GeometricMean ( paramsradius, valuesradius );     // mix'em both


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // smooth out precision + prevent increasing back (not sure if really needed, just by safety)
    Mined ( CurrentPrecision, 0.90 * CurrentPrecision + 0.10 * iterprecision );

                                        // by safety, in case radii above have overflown over 1
    Clipped ( CurrentPrecision, 0.0, 1.0 );
    

#if defined (DebugGlobalOptimizeConvergence)
    ooo.TableRowNames.Add ( Iteration );
    ooo.PutTable ( eval, 6 );
    ooo.PutTable ( evalsd, 6 );
    ooo.PutTable ( worstgroup );
    ooo.PutTable ( paramsradius, 6 );
    ooo.PutTable ( valuesradius, 6 );
    ooo.PutTable ( iterprecision, 6 );
    ooo.PutTable ( CurrentPrecision, 6 );

    for ( int p = 0; p < numparams; p++ )
        ooo.PutTable ( allparams[ p ]->Value, 6 );

    ooo.Flush ();

    GaugeLoop.SetValue ( SuperGaugeDefaultPart, Iteration + 1 );
#endif

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // reached requested precision?
    if ( CurrentPrecision <= requestedprecision )

        break;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // new progress bar value

                                        // to show it is alive even though precision didn't changed much at the beginning
    double          itsalive        = randunif ( 0.01 * ( CurrentPrecision - requestedprecision ) ); 
                                        // relative log difference of precision
    double          reldiff         =    ( log ( CurrentPrecision ) - log ( requestedprecision ) )
                                       / ( log (                1 ) - log ( requestedprecision ) );
                                        // using max as it oscillates
    currgauge     = max ( currgauge + itsalive, 1 - reldiff );
//  Maxed ( currgauge, 1 - reldiff );

    Gauge.SetValue ( SuperGaugeDefaultPart, currgauge * 100 );

    } // for Iteration - repeat (nearly) forever


Gauge.Finished ();


#if defined (DebugGlobalOptimizeConvergence)
ooo.EndTable ();
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set final results
if ( IsMethodScanBox       ( method ) 
  || IsMethodCrossHairScan ( method ) )

    SetAllParametersValues ();

                                        // optionally provide some final quality measure
if ( stat )
    Evaluate ( stat );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
