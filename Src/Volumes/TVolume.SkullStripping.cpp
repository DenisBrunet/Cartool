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

#if defined(CHECKASSERT)
#include    <assert.h>
#endif

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "TCartoolApp.h"
#include    "Math.Utils.h"
#include    "Math.Stats.h"
#include    "Math.Histo.h"
#include    "Dialogs.TSuperGauge.h"
#include    "TFilters.h"
#include    "Strings.Utils.h"
#include    "GlobalOptimize.Tracks.h"
#include    "TArray1.h"
#include    "TArray3.h"

namespace crtl {

// All the Skull Stripping methods here were developped and tested only on TVolume<float>.
// Any other data type is not guaranteed to give either good or reliable results, or any results at all.

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
double  Volume::EstimateVoxelSize  ( double filevoxelsize )
{
if ( IsNotAllocated () )
    return  0;

                                        // Estimate voxel size
double              backgroundvalue = GetBackgroundValue ();
TBoundingBox<int>   bounding ( *this, false, backgroundvalue );

double              VoxelSizeBox    = /*191.0*/ 170.0 / NonNull ( bounding.MeanSize () ); // approximate scale from smallest bounding box size (usually transverse cut)

double              VoxelSize       = max ( filevoxelsize, VoxelSizeBox );      // take the biggest of all estimates, as the file can also have a wrong size

//DBGV3 ( filevoxelsize, VoxelSizeBox, VoxelSize, "VoxelSizeFile, VoxelSizeBox -> VoxelSize" );


return  VoxelSize;
}


//----------------------------------------------------------------------------
                                        // Dispatching to the appropriate version
bool    Volume::SkullStripping  ( FctParams& params, bool showprogress )
{
if ( IsNotAllocated () )
    return  false;


int                 version         = params ( FilterParamSkullStrippingMethod );

bool                skstrok;

if      ( version == SkullStripping1A   )   skstrok     = SkullStripping_1A     ( params, showprogress );
else if ( version == SkullStripping1B   )   skstrok     = SkullStripping_1B     ( params, showprogress );
else if ( version == SkullStripping2    )   skstrok     = SkullStripping_2      ( params, showprogress );
else if ( version == SkullStripping3    )   skstrok     = SkullStripping_3      ( params, showprogress );
else                                        skstrok     = SkullStripping_1B     ( params, showprogress );

if ( ! skstrok )
    return  false;

                                        // perform brain stem removal only if Skull-Stripping was successful
bool                bsstrok         = BrainstemRemoval ( params, showprogress );

return  bsstrok;
}


//----------------------------------------------------------------------------
template <>
bool    Volume::BrainstemRemoval ( FctParams& params, bool showprogress )
{
if ( IsNotAllocated () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Algorithm control panel

//int               Method          =                     params ( FilterParamBrainstemMethod );
double              VoxelSize       = EstimateVoxelSize ( params ( FilterParamBrainstemVoxelSize ) );
                                        // brain stem size is about 10 [mm] wide
int                 BSsize          = max ( Round ( 10.0 / 2.0 / VoxelSize ), 1 );


//#define             CutBrainStemSaveSteps

//#if defined(CutBrainStemSaveSteps)
//DBGV2 ( VoxelSize, BSsize, "VoxelSize -> BSsize" );
//#endif

#if defined(CutBrainStemSaveSteps)
TFileName           _file;
StringCopy ( _file, "E:\\Data\\BrainstemRemoval." );
GetTempFileName ( StringEnd ( _file ) );
RemoveExtension ( _file );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FctParams           p;

TSuperGauge         Gauge ( FilterPresets[ FilterTypeBrainstemRemoval ].Text, showprogress ? BSsize + 1 : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Erode the size of the brain stem
Volume              nobrainstemmask ( *this );

                                        // repeating n erosion of size 1 is faster
p ( FilterParamDiameter )     = 1;

for ( int i = 0; i < BSsize; i++ ) {

    Gauge.Next ();

    nobrainstemmask.Filter ( FilterTypeErode, p );
    }


#if defined(CutBrainStemSaveSteps)
StringAppend ( _file, ".Mask.Erode." DefaultMriExt );
nobrainstemmask.WriteFile ( _file );
RemoveExtension ( _file );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Cut with the resulting bounding box
Gauge.Next ();


TBoundingBox<int>   bounding ( nobrainstemmask, false, 1 );
int                 x;
int                 y;
int                 z;

                                        // expand enough to prevent details below BSsize to disappear at the edge
bounding.Expand ( Round ( BSsize * 1.70 ) );

//bounding.Show ();

                                        // simply cut if out of the box
for ( int i = 0; i < LinearDim; i++ ) {

    LinearIndexToXYZ ( i, x, y, z );

    if ( ! bounding.Contains ( x, y, z ) )
        Array[ i ]  = 0;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // 2) Keep the brain stem stub
Gauge.Next ();


TBoundingBox<int>   bounding ( nobrainstemmask );
int                 x;
int                 y;
int                 z;

                                        // expand enough to prevent details below BSsize to disappear at the edge
bounding.Expand ( Round ( BSsize * 1.70 ) );

//bounding.Show ();

nobrainstemmask     = *this;

                                        // simply cut if out of the box
for ( int i = 0; i < LinearDim; i++ ) {

    LinearIndexToXYZ ( i, x, y, z );

    nobrainstemmask[ i ]  = Array[ i ] && ! bounding.Contains ( x, y, z );
    }


#if defined(CutBrainStemSaveSteps)
StringAppend ( _file, ".Brainstem." DefaultMriExt );
nobrainstemmask.WriteFile ( _file );
RemoveExtension ( _file );
#endif

                                        // Expand stub
                                        // problems: Region Growing stops when # of points increases, which is pretty soon
                                        //           convergence by delta of # points is very shaky, and hard to stop unless by a hard limit
Gauge.Next ();


p ( RegionGrowingNeighborhood       )   = Neighbors26;
p ( RegionGrowingTolerance          )   = 0.50;
p ( RegionGrowingLocalStatsWidth    )   = 1 * BSsize;
p ( RegionGrowingLessNeighborsThan  )   = 0.50;
p ( RegionGrowingMaxIterations      )   = RegionGrowingMaxIterationsDefault;
RegionGrowing ( nobrainstemmask, (RegionGrowingFlags) ( GrowRegion | LocalStats | StatsMask | Thickness1 | RemoveLessNeighbors ), p, this );


#if defined(CutBrainStemSaveSteps)
StringAppend ( _file, ".Grow." DefaultMriExt );
nobrainstemmask.WriteFile ( _file );
RemoveExtension ( _file );
#endif

//*this   = nobrainstemmask;
*/
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


return  true;
}


//----------------------------------------------------------------------------
                                        // Converts the current brain approximation to a big, full, smooth mask that (should) accompasses the brain
enum                BigBrainMaskMethods
                    {
                    BigMaskDense        = 0x01,
                    BigMaskNotDense     = 0x02,

                    BigMaskToMask       = 0x10,
                    };

#define             BigBrainMaskDenseGaugeSteps     5
#define             BigBrainMaskNotDenseGaugeSteps  3


Volume              BrainToBigMask  (   const Volume&           brain,
                                        BigBrainMaskMethods     how,
                                        TSuperGauge*            gauge
                                    )
{

//double            DilateSize      = max ( 1.6 / VoxelSize, 1.0 );     // about 1 for low rez, 1.7 for high rez (kind of scale factor)
double              DilateSize      = 1;                                // no scaling up, same parameters for all resolutions seem fair enough
FctParams           p;


Volume              bigmask ( brain );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // optionally filling the inside, if this was not done already
if ( gauge )    gauge->Next ();

if ( IsFlag ( how, BigMaskToMask ) ) {

        p ( FilterParamToMaskThreshold )     = 1;
        p ( FilterParamToMaskNewValue  )     = 1;
        p ( FilterParamToMaskCarveBack )     = true;
        bigmask.Filter ( FilterTypeToMask, p );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( IsFlag ( how, BigMaskDense ) ) {
                                        // Version that works well if brain is dense enough to go through erosion - otherwise it will create holes...

                                        // starting with a solid erosing, so we can shrink the major leaks
    if ( gauge )    gauge->Next ();

    p ( FilterParamDiameter )     = 3 * DilateSize;
    bigmask.Filter ( FilterTypeErode, p );

                                        // heavy smoothing
    if ( gauge )    gauge->Next ();

    p ( FilterParamDiameter )     = 5 * DilateSize;
    p ( FilterParamNumRelax )     = 2;
    bigmask.Filter ( FilterTypeRelax, p );

                                        // expand generously as we want an encompassing mask
    if ( gauge )    gauge->Next ();

    p ( FilterParamDiameter )     = 4 * DilateSize;
    bigmask.Filter ( FilterTypeDilate, p );

                                        // did we smooth it out enough? just to be sure, one more time
    if ( gauge )    gauge->Next ();

    p ( FilterParamDiameter )     = 5 * DilateSize;
    p ( FilterParamNumRelax )     = 1;
    bigmask.Filter ( FilterTypeRelax, p );
    } // BigMaskDense

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( IsFlag ( how, BigMaskNotDense ) ) {
                                        // Version that works well even if brain not very dense

                                        // HEAVY smoothing
    if ( gauge )    gauge->Next ();

    p ( FilterParamDiameter )     = 8 * DilateSize;
    p ( FilterParamNumRelax )     = 3;
    bigmask.Filter ( FilterTypeRelax, p );

                                        // expand generously as we want an encompassing mask
    if ( gauge )    gauge->Next ();

    p ( FilterParamDiameter )     = 1 * DilateSize;
    bigmask.Filter ( FilterTypeDilate, p );
    } // BigMaskNotDense


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  bigmask;
}


//----------------------------------------------------------------------------
                                        // Operations that are needed multiple times
void                PostProcessMask (   Volume&     mask,    
                                        int         lessneighbors,      int         numlessneighbors
                                    )
{
FctParams           p;

p ( FilterParamBinarized )     = 1;
mask.Filter ( FilterTypeBinarize, p );


for ( int i = 0; i < numlessneighbors; i++ ) {

    p ( FilterParamNumNeighbors )     = lessneighbors;
    p ( FilterParamNeighborhood )     = Neighbors26;
    mask.Filter ( FilterTypeLessNeighbors, p );
    }


mask.KeepRegion ( SortRegionsCompactCount, mask.GetLinearDim () * 0.001, INT_MAX, Neighbors26, 0 );
};



//----------------------------------------------------------------------------
                                        // Version 1A
                                        // First version with Mean Subtraction
                                        // Main steps are:
                                        //  - Mean Subtraction to extract the brain circumvolutions
                                        //  - Masking results with CoV
                                        //  - Taking central region as seed
                                        //  - Run multiple region growing from seed, then merge results
                                        //  - Expand back & fill mask
template <>
bool    Volume::SkullStripping_1A ( FctParams& params, bool showprogress )
{
if ( IsNotAllocated () )
    return  false;


bool                istemplate      = params ( FilterParamSkullStrippingIsTemplate ); // IsTemplateFileName ( basefile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                title[ 256 ];

StringCopy   ( title, FilterPresets[ FilterTypeSkullStripping ].Text, " (v1A)" );

TSuperGauge         Gauge ( title, showprogress ? 15 + BigBrainMaskDenseGaugeSteps : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Algorithm was first tuned to a 128 scale, then adapted to work a little better on 256
                                        // Calibrated algorithm to now use the real voxel size
double              VoxelSize       = EstimateVoxelSize ( params ( FilterParamSkullStrippingVoxelSize ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Algorithm control panel
//double              HeadMaskErode   = AtLeast ( 1.0, 8 / VoxelSize );

double              MSsize          = AtLeast ( 3.47, 6.0 / VoxelSize );
double              MSHLow          = 0.10;                     // histogram cut
double              MSHHigh         = 0.75; // 0.90

double              CoVsize         = AtLeast ( 2.83, 4.0 / VoxelSize );    // higher values gives a better histogram
//double              CoVcut          = istemplate ? 0.50 : 0.45;             // lower  -> more cut - 0.50 OK, 0.45 if needed?
double              CoVcut          = 0.30;

double              PFsize          = 1.5 * MSsize;             // big enough for wide cuts
double              PFcut           = 36;                       // higher -> more cut

double              DilateSize      = AtLeast ( 2.0, 2.0 / VoxelSize );


//DBGV5 ( VoxelSize, MSsize, CoVsize, PFsize, DilateSize, "VoxelSize -> MSsize, CoVsize, PFsize, DilateSize" );

FctParams           p;


//#define             SkullStripping1ASaveSteps

#if defined(SkullStripping1ASaveSteps)
TFileName           _basefile;
TFileName           _file;
StringCopy ( _basefile, "E:\\Data\\SkullStrip1A." );
GetTempFileName ( StringEnd ( _basefile ) );
RemoveExtension ( _basefile );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0) Get a mask of the full head
Gauge.Next ();


Volume              headmask ( *this );


p ( FilterParamToMaskThreshold )     = headmask.GetBackgroundValue ();
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
headmask.Filter ( FilterTypeToMask, p );


//p ( FilterParamDiameter )     = HeadMaskErode;
//headmask.Filter ( FilterTypeErode, p );


#if defined(SkullStripping1ASaveSteps)
StringCopy ( _file, _basefile, ".0.Mask." DefaultMriExt );
headmask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Pre-processing full head?
Gauge.Next ();


Volume              fullhead  ( *this );


fullhead.ApplyMaskToData ( headmask );


p ( FilterParamThreshold )     = 0; // GetBackgroundValue ();
fullhead.Filter ( FilterTypeRank, p );


#if defined(SkullStripping1ASaveSteps)
StringCopy ( _file, _basefile, ".1.Ranked." DefaultMriExt );
fullhead.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Get the brain circumvolutions with the Mean Subtraction
Gauge.Next ();


Volume              seed ( /**this*/ fullhead );

                                        // we are actually interested in the Positive part, which is totally clean
p ( FilterParamDiameter )       = MSsize;
p ( FilterParamResultType )     = FilterResultPositive;     // going more for shape, sulcus-like
//p ( FilterParamResultType )   = FilterResultAbsolute;     // going for density of points
seed.Filter ( FilterTypeMeanSub, p );


Gauge.Next ();

                                        // check for no-nonsense mask
if ( seed.IsNull () )
    return  false;

                                        // then mask it to centraller part
seed.ApplyMaskToData ( headmask );


#if defined(SkullStripping1ASaveSteps)
StringCopy ( _file, _basefile, ".2.MeanSub." DefaultMriExt );
seed.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) No threshold: take it all
//p ( FilterParamBinarized )     = 1;
//seed.Filter ( FilterTypeBinarize, p );

                                        // 2) threshold to the brain part
THistogram          Hms (   seed,
                            &headmask,
                            0,
                            0,
                            0,  3, 
                            (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramNormNone | HistogramLinear | HistogramIgnoreNulls )
                        );


double              cutmslow        = Hms.GetPercentilePosition ( RealUnit, MSHLow  );
double              cutmshigh       = Hms.GetPercentilePosition ( RealUnit, MSHHigh );

//DBGV4 ( cutmslow, cutmshigh, Hms.GetFirstPosition ( RealUnit ), Hms.GetLastPosition ( RealUnit ), "cutmslow, cutmshigh (min, max)" );

p ( FilterParamThresholdMin )     = cutmslow;
p ( FilterParamThresholdMax )     = cutmshigh;
p ( FilterParamThresholdBin )     = 1;
seed.Filter ( FilterTypeThresholdBinarize, p );


#if defined(SkullStripping1ASaveSteps)
Hms.NormalizeMax ();
StringCopy ( _file, _basefile, ".2.MeanSub.Histo.sef" );
Hms.WriteFile ( _file );

Hms.ToCDF ( HistogramNormMax );
StringCopy ( _file, _basefile, ".2.MeanSub.Histo cdf.sef" );
Hms.WriteFile ( _file );

StringCopy ( _file, _basefile, ".3.MeanSubCut." DefaultMriExt );
seed.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


Hms.DeallocateMemory ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Cut to physical boundaries, using the CoV for that purpose
Gauge.Next ();


Volume              covcut ( /**this*/ fullhead );
p ( FilterParamDiameter   ) = CoVsize;
p ( FilterParamResultType ) = FilterResultSigned;
covcut.Filter ( FilterTypeCoV, p );


covcut.ApplyMaskToData ( headmask );



THistogram          Hcovcut (   covcut,
                                &headmask,
                                0,
                                0,
                                0,  3, 
                                (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramNormNone | HistogramLinear | HistogramIgnoreNulls )
                            );


double              cutcovcut       = Hcovcut.GetPercentilePosition ( RealUnit, CoVcut );
//double            cutcovcut       = Hcovcut.GetPercentilePosition ( RealUnit, 0.80   );   // for Hessian

//DBGV2 ( CoVcut, cutcovcut, "CoVcut -> cutcovcut" );


#if defined(SkullStripping1ASaveSteps)
StringCopy ( _file, _basefile, ".4.1.CoV." DefaultMriExt );
covcut.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );

Hcovcut.NormalizeMax ();
StringCopy ( _file, _basefile, ".4.1.CoV.Histo.sef" );
Hcovcut.WriteFile ( _file );

Hcovcut.ToCDF ( HistogramNormMax );
StringCopy ( _file, _basefile, ".4.1.CoV.Histo cdf.sef" );
Hcovcut.WriteFile ( _file );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Clipping our current brain results with the CoV from the head
Gauge.Next ();


for ( int i = 0; i < LinearDim; i++ )
    if ( covcut[ i ] >= cutcovcut || covcut[ i ] == 0 )
        seed ( i ) = 0;


#if defined(SkullStripping1ASaveSteps)
for ( int i = 0; i < LinearDim; i++ )
    if ( covcut[ i ] >= cutcovcut )
        covcut    ( i ) = 0;

StringCopy ( _file, _basefile, ".4.1.CoVCut." DefaultMriExt );
covcut.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );

StringCopy ( _file, _basefile, ".4.2.BrainCoVCut." DefaultMriExt );
seed.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Next ();


                                        // purge out little bits that can leak later on - plus it helps picking the biggest guy...
p ( FilterParamNumNeighbors )     = 8;
p ( FilterParamNeighborhood )     = Neighbors26;
seed.Filter ( FilterTypeLessNeighbors, p );

                                        // Can add some clean-up before the Percentage of Fullness
seed.KeepRegion ( SortRegionsCompactCount, GetLinearDim () * 0.001, INT_MAX, Neighbors26, 0 );


#if defined(SkullStripping1ASaveSteps)
StringCopy ( _file, _basefile, ".4.3.Seed." DefaultMriExt );
seed.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // 4) Cut again by % of Fullness, to disconnect the sticky temporal bones - not needed anymore?
Gauge.Next ();


p ( FilterParamDiameter )     = PFsize;
seed.Filter ( FilterTypePercentFullness, p );


#if defined(SkullStripping1ASaveSteps)
StringCopy ( _file, _basefile, ".5.1.PercentFullness." DefaultMriExt );
seed.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );
#endif


p ( FilterParamThresholdMin )     = PFcut;
p ( FilterParamThresholdMax )     = 100;
p ( FilterParamThresholdBin )     = 1;
seed.Filter ( FilterTypeThresholdBinarize, p );


#if defined(SkullStripping1ASaveSteps)
StringCopy ( _file, _basefile, ".5.2.PercentFullnessCut." DefaultMriExt );
seed.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 5) Keep biggest region, which should be the brain
Gauge.Next ();

                                        // first with 26 neighbors, faster, to get rid of all major disconnected stuff
seed.KeepRegion ( SortRegionsCompactCount, GetLinearDim () * 0.001, INT_MAX, Neighbors26, 0 );


#if defined(SkullStripping1ASaveSteps)
StringCopy ( _file, _basefile, ".6.BiggestRegion." DefaultMriExt );
seed.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//Gauge.Next ();
//
//                                        // then with 6 neighbors, slower, to disconnect the very last possible stuff, and to clean-up small top patches too
//seed.KeepRegion ( SortRegionsCompactCount, GetLinearDim () * 0.001, INT_MAX, Neighbors6,  0 );
//
//
//#if defined(SkullStripping1ASaveSteps)
//StringCopy ( _file, _basefile, ".6.BiggestRegion.2." DefaultMriExt );
//seed.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
//#endif
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 6) Grow from data

double              MDsize          = AtLeast ( 3.47, 10.0 / 2.0 / VoxelSize );


Gauge.Next ();

Volume              growgrey1 ( seed );
                                        // Quite good, a tiny bit leaky, but fast
p ( RegionGrowingNeighborhood       )   = Neighbors26;
p ( RegionGrowingTolerance          )   = 0.8;
p ( RegionGrowingLocalStatsWidth    )   = 1 * MDsize;
p ( RegionGrowingLessNeighborsThan  )   = 0.15;     // smaller -> more points / leakage
p ( RegionGrowingMaxIterations      )   = RegionGrowingMaxIterationsDefault;
RegionGrowing ( growgrey1, (RegionGrowingFlags) ( GrowRegion | LocalStats | StatsMask | Thickness0 | RemoveLessNeighbors ), p, &headmask );

PostProcessMask ( growgrey1, 12, 3 );


#if defined(SkullStripping1ASaveSteps)
StringCopy ( _file, _basefile, ".5.GrowGrey1." DefaultMriExt );
growgrey1.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Next ();

Volume              growgrey2 ( seed );
                                        // very similar to 1, leaner & a bit less leaky, fast too
p ( RegionGrowingNeighborhood       )   = Neighbors26;
p ( RegionGrowingTolerance          )   = 3;
p ( RegionGrowingLocalStatsWidth    )   = 1.5 * MDsize;
p ( RegionGrowingLessNeighborsThan  )   = 0.10;
p ( RegionGrowingMaxIterations      )   = RegionGrowingMaxIterationsDefault;
RegionGrowing ( growgrey2, (RegionGrowingFlags) ( GrowRegion | LocalStats | StatsMask | Thickness0 | RemoveLessNeighbors ), p, &headmask );

PostProcessMask ( growgrey2, 12, 3 );


#if defined(SkullStripping1ASaveSteps)
StringCopy ( _file, _basefile, ".5.GrowGrey2." DefaultMriExt );
growgrey2.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Next ();

Volume              growgrey3 ( seed );
                                        // Quite good & safe, though a bit slow...
p ( RegionGrowingNeighborhood       )   = Neighbors26;
p ( RegionGrowingTolerance          )   = 0.20;
p ( RegionGrowingLocalStatsWidth    )   = 2 * MDsize;   // big neighborhood is going to be slow
p ( RegionGrowingLessNeighborsThan  )   = 0.50;
p ( RegionGrowingMaxIterations      )   = RegionGrowingMaxIterationsDefault;
RegionGrowing ( growgrey3, (RegionGrowingFlags) ( GrowRegion | LocalStats | StatsMask | Thickness1 | RemoveLessNeighbors ), p, &headmask );

PostProcessMask ( growgrey3, 8, 3 );


#if defined(SkullStripping1ASaveSteps)
StringCopy ( _file, _basefile, ".5.GrowGrey3." DefaultMriExt );
growgrey3.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Merging
Gauge.Next ();

                                        // #1 & #2 are quite similar and a bit lousy, AND-ing will remove some artifacts
                                        // then simply add #3 which is the most robust estimate
Volume              fullbrain       = seed | ( growgrey1 & growgrey2) | growgrey3;


Gauge.Next ();
                                        // a tiny bit of inflation

p ( FilterParamDiameter   ) = DilateSize;
p ( FilterParamResultType ) = FilterResultSigned;
fullbrain.Filter ( FilterTypeMax, p );


#if defined(SkullStripping1ASaveSteps)
StringCopy ( _file, _basefile, ".7.GrownMerged." DefaultMriExt );
fullbrain.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fill all internal holes - not mandatory, but looks more consistent to see the internal CSF parts
                                        // The surface appears slightly more filled, smoothed out
Gauge.Next ();


p ( FilterParamToMaskThreshold )     = 1;
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
fullbrain.Filter ( FilterTypeToMask, p );


#if defined(SkullStripping1ASaveSteps)
StringCopy ( _file, _basefile, ".8.FilledMask." DefaultMriExt );
fullbrain.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Volume              bigmask       = BrainToBigMask ( fullbrain, BigMaskDense, &Gauge );


#if defined(SkullStripping1ASaveSteps)
StringCopy ( _file, _basefile, ".9.BigMask." DefaultMriExt );
bigmask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Postprocessing
Gauge.Next ();

                                        // clip to big mask to remove any ugly tiny leaks
fullbrain  &= bigmask;


#if defined(SkullStripping1ASaveSteps)
StringCopy ( _file, _basefile, ".10.Final." DefaultMriExt );
fullbrain.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // apply data to mask
Gauge.Next ();

                                        // check for no-nonsense mask
if ( fullbrain.IsNull () )
    return  false;


ApplyMaskToData ( fullbrain );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//Gauge.HappyEnd ();


return  true;
}


//----------------------------------------------------------------------------
                                        // Version 1B
                                        // Derived from SkullStripping_1A, with different White seed and Grey growth
template <>
bool    Volume::SkullStripping_1B ( FctParams& params, bool showprogress )
{
if ( IsNotAllocated () )
    return  false;


bool                istemplate      = params ( FilterParamSkullStrippingIsTemplate ); // IsTemplateFileName ( basefile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                title[ 256 ];

StringCopy   ( title, FilterPresets[ FilterTypeSkullStripping ].Text, " (v1B)" );

TSuperGauge         Gauge ( title, showprogress ? 16 + BigBrainMaskNotDenseGaugeSteps : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Algorithm was first tuned to a 128 scale, then adapted to work a little better on 256
                                        // Calibrated algorithm to now use the real voxel size
double              VoxelSize       = EstimateVoxelSize ( params ( FilterParamSkullStrippingVoxelSize ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Algorithm control panel
double              MSsize          = AtLeast ( 3.47, 6.0 / VoxelSize );
double              MSHLow          = 0.10;                     // histogram cut
double              MSHHigh         = 0.75; // 0.90

double              CoVsize         = AtLeast ( 2.83, 4.0 / VoxelSize );    // higher values gives a better histogram
//double              CoVcut          = istemplate ? 0.50 : 0.30;             // lower  -> more cut - 0.50 OK, 0.45 if needed?
double              CoVcut          = 0.30;

double              DilateSize      = AtLeast ( 2.0, 2.0 / VoxelSize );


//DBGV4 ( VoxelSize, MSsize, CoVsize, DilateSize, "VoxelSize -> MSsize, CoVsize, DilateSize" );

FctParams           p;


//#define             SkullStripping1BSaveSteps

#if defined(SkullStripping1BSaveSteps)
TFileName           _basefile;
TFileName           _file;
StringCopy ( _basefile, "E:\\Data\\SkullStrip1B." );
GetTempFileName ( StringEnd ( _basefile ) );
RemoveExtension ( _basefile );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0) Get a mask of the full head
Gauge.Next ();


Volume              headmask ( *this );


p ( FilterParamToMaskThreshold )     = headmask.GetBackgroundValue ();
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
headmask.Filter ( FilterTypeToMask, p );


#if defined(SkullStripping1BSaveSteps)
StringCopy ( _file, _basefile, ".0.Mask." DefaultMriExt );
headmask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Pre-processing full head?
Gauge.Next ();


Volume              fullhead  ( *this );


fullhead.ApplyMaskToData ( headmask );


p ( FilterParamThreshold )     = 0; // GetBackgroundValue ();
fullhead.Filter ( FilterTypeRank, p );


#if defined(SkullStripping1BSaveSteps)
StringCopy ( _file, _basefile, ".1.Ranked." DefaultMriExt );
fullhead.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Get the brain circumvolutions with the Mean Subtraction
Gauge.Next ();


Volume              seed ( /**this*/ fullhead );

                                        // we are actually interested in the Positive part, which is totally clean
p ( FilterParamDiameter )       = MSsize;
p ( FilterParamResultType )     = FilterResultPositive;     // going more for shape, sulcus-like
//p ( FilterParamResultType )   = FilterResultAbsolute;     // going for density of points
seed.Filter ( FilterTypeMeanSub, p );


Gauge.Next ();

                                        // check for no-nonsense mask
if ( seed.IsNull () )
    return  false;

                                        // then mask it to centraller part
seed.ApplyMaskToData ( headmask );


#if defined(SkullStripping1BSaveSteps)
StringCopy ( _file, _basefile, ".2.MeanSub." DefaultMriExt );
seed.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) No threshold: take it all
//p ( FilterParamBinarized )     = 1;
//seed.Filter ( FilterTypeBinarize, p );

                                        // 2) threshold to the brain part
THistogram          Hms (   seed,
                            &headmask,
                            0,
                            0,
                            0,  3, 
                            (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramNormNone | HistogramLinear | HistogramIgnoreNulls )
                        );


double              cutmslow        = Hms.GetPercentilePosition ( RealUnit, MSHLow  );
double              cutmshigh       = Hms.GetPercentilePosition ( RealUnit, MSHHigh );

//DBGV4 ( cutmslow, cutmshigh, Hms.GetFirstPosition ( RealUnit ), Hms.GetLastPosition ( RealUnit ), "cutmslow, cutmshigh (min, max)" );

p ( FilterParamThresholdMin )     = cutmslow;
p ( FilterParamThresholdMax )     = cutmshigh;
p ( FilterParamThresholdBin )     = 1;
seed.Filter ( FilterTypeThresholdBinarize, p );


#if defined(SkullStripping1BSaveSteps)
Hms.NormalizeMax ();
StringCopy ( _file, _basefile, ".2.MeanSub.Histo.sef" );
Hms.WriteFile ( _file );

Hms.ToCDF ( HistogramNormMax );
StringCopy ( _file, _basefile, ".2.MeanSub.Histo cdf.sef" );
Hms.WriteFile ( _file );

StringCopy ( _file, _basefile, ".3.MeanSubCut." DefaultMriExt );
seed.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


Hms.DeallocateMemory ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Cut to physical boundaries, using the CoV for that purpose
Gauge.Next ();


Volume              covcut ( /**this*/ fullhead );
p ( FilterParamDiameter   ) = CoVsize;
p ( FilterParamResultType ) = FilterResultSigned;
covcut.Filter ( FilterTypeCoV, p );


covcut.ApplyMaskToData ( headmask );



THistogram          Hcovcut (   covcut,
                                &headmask,
                                0,
                                0,
                                0,  3, 
                                (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramNormNone | HistogramLinear | HistogramIgnoreNulls )
                            );


double              cutcovcut       = Hcovcut.GetPercentilePosition ( RealUnit, CoVcut );
//double              cutcovcut       = Hcovcut.GetPercentilePosition ( RealUnit, 0.80   ); // for Hessian

//DBGV2 ( CoVcut, cutcovcut, "CoVcut -> cutcovcut" );


#if defined(SkullStripping1BSaveSteps)
StringCopy ( _file, _basefile, ".4.1.CoV." DefaultMriExt );
covcut.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );

Hcovcut.NormalizeMax ();
StringCopy ( _file, _basefile, ".4.1.CoV.Histo.sef" );
Hcovcut.WriteFile ( _file );

Hcovcut.ToCDF ( HistogramNormMax );
StringCopy ( _file, _basefile, ".4.1.CoV.Histo cdf.sef" );
Hcovcut.WriteFile ( _file );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Clipping our current brain results with the CoV from the head
Gauge.Next ();


for ( int i = 0; i < LinearDim; i++ )
    if ( covcut[ i ] >= cutcovcut || covcut[ i ] == 0 )
        seed ( i ) = 0;


#if defined(SkullStripping1BSaveSteps)
for ( int i = 0; i < LinearDim; i++ )
    if ( covcut[ i ] >= cutcovcut )
        covcut    ( i ) = 0;

StringCopy ( _file, _basefile, ".4.1.CoVCut." DefaultMriExt );
covcut.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );

StringCopy ( _file, _basefile, ".4.2.BrainCoVCut." DefaultMriExt );
seed.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Next ();

                                        // Less Neighbors operator seems enough instead of % of Fullness(?)
p ( FilterParamNumNeighbors )     = 12;
p ( FilterParamNeighborhood )     = Neighbors26;
seed.Filter ( FilterTypeLessNeighbors, p );

p ( FilterParamNumNeighbors )     = 12;
p ( FilterParamNeighborhood )     = Neighbors26;
seed.Filter ( FilterTypeLessNeighbors, p );

                                        // Can add some clean-up before the Percentage of Fullness
seed.KeepRegion ( SortRegionsCompactCount, GetLinearDim () * 0.001, INT_MAX, Neighbors26, 0 );


#if defined(SkullStripping1BSaveSteps)
StringCopy ( _file, _basefile, ".4.3.Seed." DefaultMriExt );
seed.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 6) Grow from data

double              MDsize          = AtLeast ( 3.47, 10.0 / 2.0 / VoxelSize );


Gauge.Next ();
                                        // Try our chance straight from the seed
Volume              growgrey1 ( seed );
                                        // Quite good actually
p ( RegionGrowingNeighborhood       )   = Neighbors26;
p ( RegionGrowingTolerance          )   = 0.8;
p ( RegionGrowingLocalStatsWidth    )   = 1 * MDsize;
p ( RegionGrowingLessNeighborsThan  )   = 0.25;
p ( RegionGrowingMaxIterations      )   = 50;
RegionGrowing ( growgrey1, (RegionGrowingFlags) ( GrowRegion | LocalStats | StatsMask | Thickness0 | RemoveLessNeighbors ), p, &headmask );


p ( FilterParamDiameter   ) = 2;
p ( FilterParamResultType ) = FilterResultSigned;
growgrey1.Filter ( FilterTypeMax, p );


#if defined(SkullStripping1BSaveSteps)
StringCopy ( _file, _basefile, ".5.GrowGrey1." DefaultMriExt );
growgrey1.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Next ();

Volume              growwhite ( seed );
                                        // quite good white
p ( RegionGrowingNeighborhood       )   = Neighbors26;
p ( RegionGrowingTolerance          )   = 1;
p ( RegionGrowingLocalStatsWidth    )   = 1.5 * MDsize;
p ( RegionGrowingLessNeighborsThan  )   = 0.10;
p ( RegionGrowingMaxIterations      )   = 25;
RegionGrowing ( growwhite, (RegionGrowingFlags) ( GrowRegion | LocalStats | StatsMask | Thickness0 | RemoveLessNeighbors ), p, &headmask );


#if defined(SkullStripping1BSaveSteps)
StringCopy ( _file, _basefile, ".5.GrowWhite." DefaultMriExt );
growwhite.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 6.3) Grow some Grey Matter
Gauge.Next ();

                                        // Get a seed for the grey from the white
Volume              growgrey2 ( growwhite );
                                        // !internal gradient: at some borders, it is wrong to grow outside, as it will make the region growing leaking its way out!
p ( FilterParamDiameter )     = 1;
growgrey2.Filter ( FilterTypeMorphGradientInt, p );

                                        // duplicate now
//Volume            growgrey3 ( growgrey2 );

                                        // don't grow back to white region
headmask.ClearMaskToData ( growwhite );
                                        // put back the initial grey!
headmask |= growgrey2;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Next ();

                                        // Quite good, not much leaks - cerebellum can have holes
p ( RegionGrowingNeighborhood       )   = Neighbors26;
p ( RegionGrowingTolerance          )   = 0.30;
p ( RegionGrowingLocalStatsWidth    )   = 2 * MDsize;   // 1.5 * MDsize;
p ( RegionGrowingLessNeighborsThan  )   = 0.50;         // 0.60;
p ( RegionGrowingMaxIterations      )   = 9;            // 25;
RegionGrowing ( growgrey2, (RegionGrowingFlags) ( GrowRegion | LocalStats | StatsMask | Thickness1 | RemoveLessNeighbors ), p, &headmask );


#if defined(SkullStripping1BSaveSteps)
StringCopy ( _file, _basefile, ".6.GrowGrey2." DefaultMriExt );
growgrey2.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // 6.4) Grow some Grey Matter
Gauge.Next ();

                                        // Quite good & safe, very similar to growgrey2, though a bit slow...
p ( RegionGrowingNeighborhood       )   = Neighbors26;
p ( RegionGrowingTolerance          )   = 0.20;
p ( RegionGrowingLocalStatsWidth    )   = 2 * MDsize;   // big neighborhood is going to be slow
p ( RegionGrowingLessNeighborsThan  )   = 0.50;
p ( RegionGrowingMaxIterations      )   = RegionGrowingMaxIterationsDefault;
RegionGrowing ( growgrey3, (RegionGrowingFlags) ( GrowRegion | LocalStats | StatsMask | Thickness1 | RemoveLessNeighbors ), p, &headmask );


#if defined(SkullStripping1BSaveSteps)
StringCopy ( _file, _basefile, ".6.GrowGrey3." DefaultMriExt );
growgrey3.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Merging
Gauge.Next ();


Volume              fullbrain       = seed | growwhite | growgrey1 | growgrey2 /*| growgrey3*/;


Gauge.Next ();

                                        // a bit of cleaning
p ( FilterParamNumNeighbors )     = 10;
p ( FilterParamNeighborhood )     = Neighbors26;
fullbrain.Filter ( FilterTypeLessNeighbors, p );

p ( FilterParamNumNeighbors )     = 10;
p ( FilterParamNeighborhood )     = Neighbors26;
fullbrain.Filter ( FilterTypeLessNeighbors, p );


//Gauge.Next ();
//                                        // a tiny bit of inflation
//
//p ( FilterParamDiameter   ) = DilateSize;
//p ( FilterParamResultType ) = FilterResultSigned;
//fullbrain.Filter ( FilterTypeMax, p );


#if defined(SkullStripping1BSaveSteps)
StringCopy ( _file, _basefile, ".7.GrownMerged." DefaultMriExt );
fullbrain.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fill all internal holes - not mandatory, but looks more consistent to see the internal CSF parts
                                        // The surface appears slightly more filled, smoothed out
Gauge.Next ();


p ( FilterParamToMaskThreshold )     = 1;
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
fullbrain.Filter ( FilterTypeToMask, p );


#if defined(SkullStripping1BSaveSteps)
StringCopy ( _file, _basefile, ".8.FilledMask." DefaultMriExt );
fullbrain.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Volume              bigmask       = BrainToBigMask ( fullbrain, BigMaskNotDense, &Gauge );


#if defined(SkullStripping1BSaveSteps)
StringCopy ( _file, _basefile, ".9.BigMask." DefaultMriExt );
bigmask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Postprocessing
Gauge.Next ();

                                        // clip to big mask to remove any ugly tiny leaks
fullbrain  &= bigmask;


#if defined(SkullStripping1BSaveSteps)
StringCopy ( _file, _basefile, ".10.Final." DefaultMriExt );
fullbrain.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // apply data to mask
Gauge.Next ();

                                        // check for no-nonsense mask
if ( fullbrain.IsNull () )
    return  false;


ApplyMaskToData ( fullbrain );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//Gauge.HappyEnd ();


return  true;
}



//----------------------------------------------------------------------------
/*                                      // Full Brain mask, using K curvature - works good, but no better than Mean Subtraction
                                        // 1) Get the brain circonvolutions with K+ - new way, simpler, better SNR, no histogram
Volume              fullbrain ( *this );

                                        // compute curvature
p ( FilterParamDiameter   )     = 3;
p ( FilterParamResultType )     = FilterResultPositive;
fullbrain.Filter ( FilterTypeKCurvature, p );
//*this       = fullbrain;
//ShowMessage ( "K+", "" );


p ( FilterParamBinarized )     = 255;
fullbrain.Filter ( FilterTypeBinarize, p );

                                        // cut to head mask
fullbrain.ApplyMaskToData ( headmask );


*this       = fullbrain;
ShowMessage ( "new brain K+- cut", "" );
*this       = original;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Cut to physical boundaries, using the CoV for that purpose

Gauge.Next ();

//Volume            cut ( original );
Volume              cut ( *this );
p ( FilterParamDiameter   ) = 3.47;
p ( FilterParamResultType ) = FilterResultSigned;
cut.Filter ( FilterTypeCoV, p );


THistogram          Hc ( &cut, 1, &headmask );
double              cutcut          = Hc.GetPercentilePosition ( 0.50 );    // lower -> more cut
//DBGV ( cutcut, "cutcut" );

//Hc.WriteFile ( "E:\\Data\\Histo.Cut.txt", false );
//Hc.ToCDF ();                             // for display!
//Hc.WriteFile ( "E:\\Data\\Histo.Cut CDF.txt", false );
//return;

//*this       = fullbrain;
//ShowMessage ( "before cut", "" );

for ( int i = 0; i < LinearDim; i++ )
    if ( cut[ i ] >= cutcut )
        fullbrain[ i ] = 0;


*this       = fullbrain;
ShowMessage ( "after CoV cut", "" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 4) Thinning before clustering, to avoid unwanted bridges between the brain and the bone (especially temporal)

                                        // thinning by % of fullness
Volume              perful ( fullbrain );


p ( FilterParamDiameter )     = 5.66;
perful.Filter ( FilterTypePercentFullness, p );

//*this       = perful;
//ShowMessage ( "%Full", "" );
//return;


THistogram          Hpf ( &perful, 1, &headmask );
double              pfcut           = Hpf.GetPercentilePosition ( 0.66 ); // higher -> more cut
//DBGV ( pfcut, "pfcut" );

//Hpf.WriteFile ( "E:\\Data\\Histo.Cut.txt", false );
//Hpf.ToCDF ();                             // for display!
//Hpf.WriteFile ( "E:\\Data\\Histo.Cut CDF.txt", false );
//return;

                                        // cut lowest connectivities
for ( int i = 0; i < LinearDim; i++ )
    if ( perful[ i ] < pfcut ) // 50 for subjects, 60 for MNI
        fullbrain[ i ] = 0;
*/


/*                                      // Raw brain extraction, using the CoV
                                        // Full brain extract
Volume              cov ( original );

p ( FilterParamDiameter   ) = 3.47;
p ( FilterParamResultType ) = FilterResultSigned;
cov.Filter ( FilterTypeCoV, p, true );

cov.Filter ( FilterTypeRevert, p );

cov.ApplyMaskToData ( headmask );

//*this       = cov;
//cov.WriteFile ( "E:\\Data\\CoV." DefaultMriExt );
//ShowMessage ( "CoV", "" );
//return;


                                        // First estimate of the full brain
THistogram          Hb ( &cov, 1, &headmask );
                                        // this is optimal for MNI and subjects (which can go to 0.50)
double              greycut         = Hb.GetPercentilePosition ( 0.60 );

//DBGV ( greycut, "greycut" );
//Hb.ToCDF ();                             // for display!
//Hb.WriteFile ( "E:\\Data\\Histo.Brain.txt", false );
//return;

                                        // brain is the higher end
Volume              brain ( cov );

p ( FilterParamThresholdMin )     = greycut;
p ( FilterParamThresholdMax )     = FLT_MAX;
p ( FilterParamThresholdBin )     = 1;
brain.Filter ( FilterTypeThresholdBinarize, p );
                                        // first part of the Open
                                        // better for template (?)
//p ( FilterParamDiameter )     = 2;
//brain.Filter ( FilterTypeErode, p );
                                        // better for real head
p ( FilterParamDiameter )   = 4;
//p ( FilterParamDiameter )   = 5;  // bigger = better for template
p ( FilterParamResultType ) = FilterResultSigned;
brain.Filter ( FilterTypeMin, p, true );

//*this       = brain;
//ShowMessage ( "CoV cut", "" );
//return;
 

if ( brain.KeepRegion ( SortRegionsCompactCount, GetLinearDim () / 100, INT_MAX, Neighbors26, 0, true ) ) {
                                        // second part of the Open
//    p ( FilterParamDiameter )     = 2;
//    brain.Filter ( FilterTypeDilate, p, true );
    p ( FilterParamDiameter )     = 4;
//    p ( FilterParamDiameter )     = 5;
    p ( FilterParamResultType )   = FilterResultSigned;
    brain.Filter ( FilterTypeMax, p, true );
    }
                                        // Here: a rough brain, including grey / white / cerebellum

//*this       = brain;
//ShowMessage ( "brain estimate", "" );
//brain.WriteFile ( "E:\\Data\\RG Seed." DefaultMriExt );
//return;

//ApplyRegionToData ( region );           // punch mask into data
*/

//----------------------------------------------------------------------------
                                        // Version 2
                                        // Use of Mean Division operator
                                        // Then Region Growing, first the White then the Grey matter
                                        // Merge the 2 + clip to a safety mask (important!) to mitigate region growing leaks
template <>
bool    Volume::SkullStripping_2  ( FctParams& params, bool showprogress )
{
if ( IsNotAllocated () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                title[ 256 ];

StringCopy   ( title, FilterPresets[ FilterTypeSkullStripping ].Text, " (v2)" );

TSuperGauge         Gauge ( title, showprogress ? 15 + BigBrainMaskNotDenseGaugeSteps : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Algorithm control panel

double              VoxelSize       = EstimateVoxelSize ( params ( FilterParamSkullStrippingVoxelSize ) );
                                        // circonvolutions look like 10[mm] wide in average, use about half for the filter size
double              MDsize          = AtLeast ( 3.47, 10.0 / 2.0 / VoxelSize );
                                        // this will condition the seed for Region Growing
double              MDHistoCutLow   = 0.30;
double              MDHistoCutHigh  = 0.70; // not too much

double              PFsize          = 1.5 * MDsize;                     // size should be big enough to really severe the brain from the other tissues
double              PFHistocut      = 0.80;                             // seems enough to detach all casese

FctParams           p;


//#define             SkullStripping2SaveSteps

#if defined(SkullStripping2SaveSteps)
//DBGV4 ( VoxelSize, MDsize, PFsize, PFHistocut, "VoxelSize -> MDsize, PFsize, PFHistocut" );

TFileName           _basefile;
TFileName           _file;
StringCopy ( _basefile, "E:\\Data\\SkullStrip1." );
GetTempFileName ( StringEnd ( _basefile ) );
RemoveExtension ( _basefile );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0) Get a mask of the full head
Gauge.Next ();


Volume              headmask ( *this );


p ( FilterParamToMaskThreshold )     = headmask.GetBackgroundValue ();
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
headmask.Filter ( FilterTypeToMask, p );

//DBGV3 ( p(FilterParamToMaskThreshold), p(FilterParamToMaskNewValue), p(FilterParamToMaskCarveBack), "ToMask params" );


#if defined(SkullStripping2SaveSteps)
StringCopy ( _file, _basefile, ".0.Mask." DefaultMriExt );
headmask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Mean Division to head as a first guess of the brain
                                        // !Note: Mean Division negative part gives a very good skull approximation!
Gauge.Next ();


Volume              seed ( *this /*fullhead*/ );

                                        // apply mask even before processing
seed.ApplyMaskToData ( headmask );

                                        // Mean Division is the magic here, it "normalizes" the level like a Bias Field Correction, and gives a well defined signature for the brain part...
p ( FilterParamDiameter   ) = MDsize;
p ( FilterParamResultType ) = FilterResultSigned;
seed.Filter ( FilterTypeMeanDiv, p );


if ( seed.IsNull () )
    return  false;

                                        // mask again
seed.ApplyMaskToData ( headmask );


#if defined(SkullStripping2SaveSteps)
StringCopy ( _file, _basefile, ".2.MeanDiv." DefaultMriExt );
seed.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) threshold to the brain part
Gauge.Next ();


THistogram          Hmd (   seed,
                            &headmask,
                            0,
                            0,
                            0,  3, 
                            (HistogramOptions) ( HistogramPDF | HistogramRaw | HistogramNormNone | HistogramLinear | HistogramIgnoreNulls )
                        );


double              cutmdlow            = Hmd.GetPercentilePosition ( RealUnit, MDHistoCutLow  );
double              cutmdhigh           = Hmd.GetPercentilePosition ( RealUnit, MDHistoCutHigh );

                                        // Focusing on the brain part
p ( FilterParamThresholdMin )     = cutmdlow ;
p ( FilterParamThresholdMax )     = cutmdhigh;
p ( FilterParamThresholdBin )     = 1;
seed.Filter ( FilterTypeThresholdBinarize, p );

                                        // purge out little bits that can leak later on - plus it helps picking the biggest guy...
//p ( FilterParamNumNeighbors )     = 13;
//p ( FilterParamNeighborhood )     = Neighbors26;
//seed.Filter ( FilterTypeLessNeighbors, p );
//p ( FilterParamNumNeighbors )     = 13;
//p ( FilterParamNeighborhood )     = Neighbors26;
//seed.Filter ( FilterTypeLessNeighbors, p );


#if defined(SkullStripping2SaveSteps)
StringCopy ( _file, _basefile, ".2.MeanDivCut." DefaultMriExt );
seed.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif

Hmd.DeallocateMemory ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Cut by % of Fullness, to disconnect the temporal bones & cerebellum
Gauge.Next ();


p ( FilterParamDiameter )     = PFsize;
seed.Filter ( FilterTypeFastGaussian /*FilterTypeGaussian*/, p );


#if defined(SkullStripping2SaveSteps)
StringCopy ( _file, _basefile, ".3.PF." DefaultMriExt );
seed.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute optimal PF cut
Gauge.Next ();


THistogram          Hpf (   seed,
                            &headmask,
                            0,
                            0,
                            0,  3, 
                            (HistogramOptions) ( HistogramPDF | HistogramRaw | HistogramNormNone | HistogramLinear | HistogramIgnoreNulls )
                        );

double              cutpflow        = Hpf.GetPercentilePosition ( RealUnit, PFHistocut );


#if defined(SkullStripping2SaveSteps)
StringCopy ( _file, _basefile, ".3.PFHisto." FILEEXT_EEGSEF );
Hpf.WriteFile ( _file );
//DBGV3 ( cutpflow, Hpf.GetFirstPosition (), Hpf.GetLastPosition (), "cutpflow  (min, max)" );

Hpf.ToCDF ( HistogramNormMax );
PostfixFilename ( _file, " CDF" );
Hpf.WriteFile ( _file );

//DBGV2 ( PFHistocut, cutpflow, "PF cut: PFHistocut% -> cut" );
#endif

Hpf.DeallocateMemory ();


                                        // retrieve a thick mask (about 70% CDF)
p ( FilterParamThreshold )     = cutpflow;
seed.Filter ( FilterTypeThresholdAbove, p );


#if defined(SkullStripping2SaveSteps)
StringCopy ( _file, _basefile, ".3.PFHistoCut." DefaultMriExt );
seed.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // 3) Cut by removing voxels with Less Neighbors, to disconnect the temporal bones & cerebellum
Gauge.Next ();


/ *                                        // We can loop through LessNeighbors + KeepCompact until delta is less than 1%, then take the previous result
                                        // With LN20, it converges at 2 or 3 iterations, which means it is possible to do 2, and optionally a 3d
int                 numsetbefore;
int                 numsetln;
int                 numsetkc;


for ( int thini = 1; thini <= 3; thini++ ) {

    numsetbefore= seed.GetNumSet ();

    p ( FilterParamNumNeighbors )     = 20; // 18;
    p ( FilterParamNeighborhood )     = Neighbors26;
    seed.Filter        ( FilterTypeLessNeighbors, p );
    numsetln    = seed.GetNumSet ();

    seed.KeepRegion    ( SortRegionsCompactCount, GetLinearDim () * 0.001, INT_MAX, Neighbors26, 0 );
    numsetkc    = seed.GetNumSet ();

    double      deltakcln   = ( numsetln - numsetkc ) / (double) numsetln;

    //DBGV6 ( numsetbefore, numsetln, numsetkc, 100 * numsetln / (double) numsetbefore, 100 * numsetkc / (double) numsetbefore, 100 * numsetkc / (double) numsetln, "#before #LN #KC %LN/before %KC/before %KC/LN" );
    DBGV5 ( thini, 100 * numsetln / (double) numsetbefore, 100 * numsetkc / (double) numsetbefore, 100 * numsetkc / (double) numsetln, 100 * deltakcln, "#i %LN/before %KC/before %KC/LN %DeltaKCLN" );
    }
* /

                                        // Doing a heavy erosion-like step
p ( FilterParamNumNeighbors )     = 23;           // very heavy
p ( FilterParamNeighborhood )     = Neighbors26;
seed.Filter ( FilterTypeLessNeighbors, p );
p ( FilterParamNumNeighbors )     = 20;           // quite heavy
p ( FilterParamNeighborhood )     = Neighbors26;
seed.Filter ( FilterTypeLessNeighbors, p );



#if defined(SkullStripping2SaveSteps)
StringCopy ( _file, _basefile, ".3.LN." DefaultMriExt );
seed.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );
#endif
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 4) Keep biggest region, which should be, well, the brain
Gauge.Next ();

                                        // purge out little bits that can leak later on - plus it helps picking the biggest guy...
p ( FilterParamNumNeighbors )     = 13;
p ( FilterParamNeighborhood )     = Neighbors26;
seed.Filter ( FilterTypeLessNeighbors, p );

p ( FilterParamNumNeighbors )     = 13;
p ( FilterParamNeighborhood )     = Neighbors26;
seed.Filter ( FilterTypeLessNeighbors, p );


Gauge.Next ();

                                        // first with 26 neighbors, faster, to get rid of all major disconnected stuff
seed.KeepRegion ( SortRegionsCompactCount, GetLinearDim () * 0.001, INT_MAX, Neighbors26, 0 );


#if defined(SkullStripping2SaveSteps)
StringCopy ( _file, _basefile, ".4.Seed." DefaultMriExt );
seed.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Instead of expanding / filling with Morphology, use some Region Growing which is a smarter (though more sensitive) way to fill the gaps
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 6.1) Grow some White Matter
Gauge.Next ();


Volume              growwhite ( seed );


//p ( RegionGrowingNeighborhood       )   = Neighbors26;
//p ( RegionGrowingTolerance          )   = 3;
//p ( RegionGrowingLocalStatsWidth    )   = 1 * MDsize;
//p ( RegionGrowingLessNeighborsThan  )   = 0.20;
//p ( RegionGrowingMaxIterations      )   = RegionGrowingMaxIterationsDefault;
//RegionGrowing ( growwhite, (RegionGrowingFlags) ( GrowRegion | LocalStats | StatsMask | Thickness0 | RemoveLessNeighbors ), p, &headmask );


p ( RegionGrowingNeighborhood       )   = Neighbors26;
p ( RegionGrowingTolerance          )   = 1;
p ( RegionGrowingLocalStatsWidth    )   = 1 * MDsize;
p ( RegionGrowingLessNeighborsThan  )   = 0.55;
p ( RegionGrowingMaxIterations      )   = RegionGrowingMaxIterationsDefault;
RegionGrowing ( growwhite, (RegionGrowingFlags) ( GrowRegion | LocalStats | StatsMask | Thickness0 | RemoveLessNeighbors ), p, &headmask );

                                        // Using Median seems to give quite robust results
//p ( RegionGrowingNeighborhood       )   = Neighbors26;
//p ( RegionGrowingTolerance          )   = 0.10; // 0.15; // 0.20;
//p ( RegionGrowingLocalStatsWidth    )   = 1 * MDsize; // seems good enough with this size
//p ( RegionGrowingLessNeighborsThan  )   = 1.0;
//p ( RegionGrowingMaxIterations      )   = RegionGrowingMaxIterationsDefault;
//RegionGrowing ( growwhite, (RegionGrowingFlags) ( GrowRegion | LocalStats | StatsMask | Thickness0 | RemoveMedian ), p, &headmask );


#if defined(SkullStripping2SaveSteps)
StringCopy ( _file, _basefile, ".6.GrowWhite." DefaultMriExt );
growwhite.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
                                        // 6.2) Grow some Grey Matter
Gauge.Next ();


Volume              growgrey1 ( seed );

                                        // a bit leaky, but we are going to AND it
//p ( RegionGrowingNeighborhood       )   = Neighbors26;
//p ( RegionGrowingTolerance          )   = 0.05;
//p ( RegionGrowingLocalStatsWidth    )   = 1.0 * MDsize; // results look better with 1.5, but at the cost of longer processing time
//p ( RegionGrowingLessNeighborsThan  )   = 150; // 100;
//p ( RegionGrowingMaxIterations      )   = RegionGrowingMaxIterationsDefault;
//RegionGrowing ( growgrey1, (RegionGrowingFlags) ( GrowRegion | LocalStats | StatsMask | Thickness1 | RemoveMedian ), p, &headmask );


p ( RegionGrowingNeighborhood       )   = Neighbors26;
p ( RegionGrowingTolerance          )   = 0.20;
p ( RegionGrowingLocalStatsWidth    )   = 1.5 * MDsize;
p ( RegionGrowingLessNeighborsThan  )   = 0.50;
p ( RegionGrowingMaxIterations      )   = RegionGrowingMaxIterationsDefault;
RegionGrowing ( growgrey1, (RegionGrowingFlags) ( GrowRegion | LocalStats | StatsMask | Thickness1 | RemoveLessNeighbors ), p, &headmask );


#if defined(SkullStripping2SaveSteps)
StringCopy ( _file, _basefile, ".6.GrowGrey1." DefaultMriExt );
growgrey1.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 6.3) Grow some Grey Matter
Gauge.Next ();

                                        // Get a seed for the grey from the white
Volume              growgrey2 ( growwhite );
                                        // !internal gradient: at some borders, it is wrong to grow outside, as it will make the region growing leaking its way out!
p ( FilterParamDiameter )     = 1;
growgrey2.Filter ( FilterTypeMorphGradientInt, p );

                                        // duplicate now
Volume              growgrey3 ( growgrey2 );

                                        // don't grow back to white region
headmask.ClearMaskToData ( growwhite );
                                        // put back the initial grey!
headmask |= growgrey2;


//#if defined(SkullStripping2SaveSteps)
//StringCopy ( _file, _basefile, ".6.HeadMaskForGrey." DefaultMriExt );
//headmask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
//#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Next ();


//p ( RegionGrowingNeighborhood       )   = Neighbors26;
//p ( RegionGrowingTolerance          )   = 0.20;
//p ( RegionGrowingLocalStatsWidth    )   = 1.20 * MDsize; // 6; // 1.5 * MDsize;
//p ( RegionGrowingLessNeighborsThan  )   = 0.65;
//p ( RegionGrowingMaxIterations      )   = RegionGrowingMaxIterationsDefault;
//RegionGrowing ( growgrey2, (RegionGrowingFlags) ( GrowRegion | LocalStats | StatsMask | Thickness1 | RemoveLessNeighbors ), p, &headmask );
 
                                        // Quite good, not much leaks - cerebellum can have holes
p ( RegionGrowingNeighborhood       )   = Neighbors26;
p ( RegionGrowingTolerance          )   = 0.30;
p ( RegionGrowingLocalStatsWidth    )   = 1.5 * MDsize; // 2 * MDsize;
p ( RegionGrowingLessNeighborsThan  )   = 0.58; // 0.55;
p ( RegionGrowingMaxIterations      )   = RegionGrowingMaxIterationsDefault;
RegionGrowing ( growgrey2, (RegionGrowingFlags) ( GrowRegion | LocalStats | StatsMask | Thickness1 | RemoveLessNeighbors ), p, &headmask );

                                        // as above, but a bit less leaky, tends to add some pie-mere
//p ( RegionGrowingNeighborhood       )   = Neighbors26;
//p ( RegionGrowingTolerance          )   = 2;
//p ( RegionGrowingLocalStatsWidth    )   = 2 * MDsize;
//p ( RegionGrowingLessNeighborsThan  )   = 0.63;         // a bit better?
//p ( RegionGrowingMaxIterations      )   = RegionGrowingMaxIterationsDefault;
//RegionGrowing ( growgrey2, (RegionGrowingFlags) ( GrowRegion | LocalStats | StatsMask | Thickness1 | RemoveLessNeighbors ), p, &headmask );
                                        // less leaky, faster
//p ( RegionGrowingNeighborhood       )   = Neighbors26;
//p ( RegionGrowingTolerance          )   = 0.25;
//p ( RegionGrowingLocalStatsWidth    )   = 1.5 * MDsize;
//p ( RegionGrowingLessNeighborsThan  )   = 0.65;         // smaller -> more growth
//p ( RegionGrowingMaxIterations      )   = RegionGrowingMaxIterationsDefault;
//RegionGrowing ( growgrey2, (RegionGrowingFlags) ( GrowRegion | LocalStats | StatsMask | Thickness1 | RemoveLessNeighbors ), p, &headmask );


#if defined(SkullStripping2SaveSteps)
StringCopy ( _file, _basefile, ".6.GrowGrey2." DefaultMriExt );
growgrey2.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 6.4) Grow some Grey Matter
Gauge.Next ();

                                        // Pretty good, fast, thin, CEREBELLUM in - some tiny leaks though
p ( RegionGrowingNeighborhood       )   = Neighbors26;
p ( RegionGrowingTolerance          )   = 10;
p ( RegionGrowingLocalStatsWidth    )   = 2 * MDsize;
p ( RegionGrowingLessNeighborsThan  )   = 0.20; // 0.15;
p ( RegionGrowingMaxIterations      )   = RegionGrowingMaxIterationsDefault;
RegionGrowing ( growgrey3, (RegionGrowingFlags) ( GrowRegion | LocalStats | StatsMask | Thickness0 | RemoveLessNeighbors ), p, &headmask );


#if defined(SkullStripping2SaveSteps)
StringCopy ( _file, _basefile, ".6.GrowGrey3." DefaultMriExt );
growgrey3.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 7) Merge all masks
Gauge.Next ();


Volume              fullbrain     = growwhite /*| growgrey1*/ | growgrey2 | growgrey3;


p ( FilterParamToMaskThreshold )     = 1;
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
fullbrain.Filter ( FilterTypeToMask, p );


#if defined(SkullStripping2SaveSteps)
StringCopy ( _file, _basefile, ".7.MergedMasks." DefaultMriExt );
fullbrain.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 8) Make a mask from the white + grey
Volume              bigmask         = BrainToBigMask ( fullbrain, BigMaskNotDense, &Gauge );


#if defined(SkullStripping2SaveSteps)
StringCopy ( _file, _basefile, ".8.BigMask." DefaultMriExt );
bigmask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 9) Postprocessing
Gauge.Next ();

                                        // clip to big mask to remove any ugly tiny leaks
fullbrain  &= bigmask;


#if defined(SkullStripping2SaveSteps)
StringCopy ( _file, _basefile, ".10.Final." DefaultMriExt );
fullbrain.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // apply data to mask
Gauge.Next ();

                                        // check for no-nonsense mask
if ( fullbrain.IsNull () )
    return  false;


ApplyMaskToData ( fullbrain );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//Gauge.HappyEnd ();


return  true;
}


//----------------------------------------------------------------------------
/*                                      // Version 2
                                        // Use of SNR filter
                                        // This version is not fully tuned parameters wise, but it works
bool    Volume::SkullStripping_2  ( FctParams& /*params* /, bool showprogress )
{
if ( IsNotAllocated () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FctParams           p;

char                title[ 256 ];

StringCopy   ( title, FilterPresets[ FilterTypeSkullStripping ].Text, " (v2)" );

TSuperGauge         Gauge ( title, showprogress ? 6 : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Algorithm was first tuned to a 128 scale, then adapted to work a little better on 256
                                        // It could be an option to downsample to go faster
int                 bestsize        = 128;
int                 maxsize         = MaxSize ();
double              scale           = maxsize / bestsize;
//double              scale           = 1;// no rescaling

//DBGV3 ( maxsize, scale, istemplate, FilterPresets[ FilterTypeSkullStripping ].Text": maxsize, scale, IsTemplate" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Algorithm control panel
double              SNRsize         = max ( 2 * scale, 3.47 );
double              SNRHLow         = 0.60;                     // histogram cut
//double              SNRHHigh        = 1.00;

double              PFsize          = SNRsize + 1;
double              PFcut           = 88;                       // higher -> more cut

double              DilateSize      = 3.5; // 2.5 + 0.5 * scale;        // slightly smaller for smaller volumes


//#define             SkullStripping2SaveSteps

//#if defined(SkullStripping2SaveSteps)
//DBGV4 ( scale, SNRsize, PFsize, DilateSize, "scale, SNRsize, PFsize, DilateSize" );
//#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0) Get a mask of the full head
Gauge.Next ();


//Volume            fullhead  ( *this );  // not needed, except in debug mode
Volume              headmask ( *this /*fullhead* / );


p ( FilterParamToMaskThreshold )     = GetBackgroundValue ();
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
headmask.Filter ( FilterTypeToMask, p );

//DBGV3 ( p(FilterParamToMaskThreshold), p(FilterParamToMaskNewValue), p(FilterParamToMaskCarveBack), "ToMask params" );

#if defined(SkullStripping2SaveSteps)
headmask.WriteFile ( "E:\\Data\\SkullStrip.0.Mask." DefaultMriExt );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Get the brain circonvolutions with the SNR
Gauge.Next ();


Volume              fullbrain ( *this );

                                        // we are actually interested in the Positive part, which is totally clean
p ( FilterParamDiameter )   = SNRsize;
p ( FilterParamResultType ) = FilterResultPositive;
p ( FilterParamLogOffset )  = 0.01;
fullbrain.Filter ( FilterTypeLogSNR, p );


if ( fullbrain.IsNull () )
    return  false;

                                        // then mask it to centraller part
fullbrain.ApplyMaskToData ( headmask );


#if defined(SkullStripping2SaveSteps)
fullbrain.WriteFile ( "E:\\Data\\SkullStrip.1.SNR." DefaultMriExt );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) threshold to the brain part

                                        // Actually, histogram looks like there are 4 modes, and we need to cut between 2 and 3
                                        // the code here is only an approximation

THistogram          Hsnr ( &fullbrain, 1, &headmask );
Hsnr[ 0 ]   = 0;                       // ignore any possible 0 values in the histogram
double              cutsnrlow       = Hsnr.GetPercentilePosition ( RealUnit, SNRHLow  );
double              cutsnrhigh      = fullbrain.GetMaxValue (); // Hsnr.GetPercentilePosition ( RealUnit, SNRHHigh );


//Hsnr.WriteFileData ( "E:\\Data\\Histo.SNR.ep" );
////Hsnr.ToCDF ();
////Hsnr.WriteFileData ( "E:\\Data\\Histo.SNR cdf.ep", (HistoWriteType) ( HistoWriteLinear | HistoWriteNormalizeMax ) );
//DBGV4 ( cutsnrlow, cutsnrhigh, Hsnr.GetFirstPosition (), Hsnr.GetLastPosition (), "cutsnrlow, cutsnrhigh (min, max)" );


p ( FilterParamThresholdMin )     = cutsnrlow;
p ( FilterParamThresholdMax )     = cutsnrhigh;
p ( FilterParamThresholdBin )     = 1;
fullbrain.Filter ( FilterTypeThresholdBinarize, p );


#if defined(SkullStripping2SaveSteps)
fullbrain.WriteFile ( "E:\\Data\\SkullStrip.2.SNR.Cut." DefaultMriExt );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) clean-up lone voxels
//Gauge.Next ();
//
//
//p ( FilterParamNumNeighbors )     = 13;
//p ( FilterParamNeighborhood )     = Neighbors26;
//fullbrain.Filter ( FilterTypeLessNeighbors, p );
//
//
//#if defined(SkullStripping2SaveSteps)
//fullbrain.WriteFile ( "E:\\Data\\SkullStrip.3.SNR.Cut.LessNeigh." DefaultMriExt );
//#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 4) Cut again by % of Fullness, to disconnect the temporal bones & cerebellum
Gauge.Next ();


p ( FilterParamDiameter )     = PFsize;
fullbrain.Filter ( FilterTypePercentFullness, p );

                                        //
//THistogram          Hpf ( &fullbrain, 1, &headmask );
//double              cutpflow        = Hpf.GetPercentilePosition ( RealUnit, 0.85  );
//double              cutpfhigh       = fullbrain.GetMaxValue (); // Hpf.GetPercentilePosition ( RealUnit, SNRHHigh );

p ( FilterParamThreshold )     = PFcut;
fullbrain.Filter ( FilterTypeThresholdAbove, p );


#if defined(SkullStripping2SaveSteps)
fullbrain.WriteFile ( "E:\\Data\\SkullStrip.4.SNR.Cut.LessNeigh.PF." DefaultMriExt );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 5) Keep biggest region, which should be the brain
Gauge.Next ();

                                        // first with 26 neighbors, faster, to get rid of all major disconnected stuff
fullbrain.KeepRegion ( SortRegionsCompactCount, GetLinearDim () * 0.001, INT_MAX, Neighbors26, 0 );


#if defined(SkullStripping2SaveSteps)
fullbrain.WriteFile ( "E:\\Data\\SkullStrip.5.SNR.Cut.LessNeigh.PF.Big." DefaultMriExt );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 6) Expand to include more of the grey part
Gauge.Next ();


p ( FilterParamDiameter )     = DilateSize;
//p ( FilterParamResultType )   = FilterResultSigned;
//fullbrain.Filter ( FilterTypeMax, p );  // finer tuning than Dilate, and smoother results
fullbrain.Filter ( FilterTypeDilate, p );


#if defined(SkullStripping2SaveSteps)
fullbrain.WriteFile ( "E:\\Data\\SkullStrip.6.SNR.Cut.LessNeigh.PF.Big.Dilate." DefaultMriExt );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // apply data to mask
Gauge.Next ();

                                        // check for no-nonsense mask
if ( fullbrain.IsNull () )
    return  false;


ApplyMaskToData ( fullbrain );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//Gauge.HappyEnd ();


return  true;
}
*/

//----------------------------------------------------------------------------
                                        // Version 3
                                        // Use of Mean Division operator, as per Version 2, but then generate multiple estimates of masks,
                                        // currently 6, which are then merged together. If less than 50% masks fail, this will not show up in the end.
                                        // Also the series of masks helps filling difficult parts, like around the cerebellum.
                                        // Currently designed for 1mm voxel size, no rescaling taken into account, althought it works somehow on low rez MRIs.
template <>
bool    Volume::SkullStripping_3  ( FctParams& params, bool showprogress )
{
if ( IsNotAllocated () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FctParams           p;

char                title[ 256 ];

StringCopy   ( title, FilterPresets[ FilterTypeSkullStripping ].Text, " (v3)" );

TSuperGauge         Gauge ( title, showprogress ? 43 : 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Algorithm control panel

double              VoxelSize       = EstimateVoxelSize ( params ( FilterParamSkullStrippingVoxelSize ) );
                                        // circonvolutions look like 10[mm] wide in average, use about half for the filter size
double              MDsize          = AtLeast ( 3.47, 10.0 / 2.0 / VoxelSize );

double              MDHistoCutLow   = 0.20; // 
double              MDHistoCutHigh  = 0.90; // generous cut, resulting in a lot of voxels in


double              PFsize          = 2 * MDsize;                       // size should be big enough to really severe the brain from the other tissues

//double            DilateSize      = max ( 1.6 / VoxelSize, 1.0 );     // about 1 for low rez, 1.7 for high rez (kind of scale factor)
//double            DilateSize      = 1;                                // no scaling up, same parameters for all resolutions seem fair enough
double              DilateSize      = AtLeast ( 1.0, 1.0 / VoxelSize );


//#define             SkullStripping3SaveSteps

//#if defined(SkullStripping3SaveSteps)
//DBGV4 ( VoxelSize, MDsize, PFsize, DilateSize, "VoxelSize -> MDsize, PFsize, DilateSize" );
//#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 0) Get a shrunk mask of the full head
Gauge.Next ();


Volume              headmask ( *this );


p ( FilterParamToMaskThreshold )     = headmask.GetBackgroundValue ();
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
headmask.Filter ( FilterTypeToMask, p );

//DBGV3 ( p(FilterParamToMaskThreshold), p(FilterParamToMaskNewValue), p(FilterParamToMaskCarveBack), "ToMask params" );


#if defined(SkullStripping3SaveSteps)
TFileName           _file;
StringCopy ( _file, "E:\\Data\\SkullStrip3." );
GetTempFileName ( StringEnd ( _file ) );
RemoveExtension ( _file );

StringAppend ( _file, ".1.Mask." DefaultMriExt );
headmask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
RemoveExtension ( _file, 3 );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Mean Division to head to guess the brain
                                        // !Note: Mean Division negative part gives a very good skull approximation!
Gauge.Next ();


Volume              fullbrain ( *this );

                                        // apply mask even before processing
fullbrain.ApplyMaskToData ( headmask );

                                        // Mean Division is the magic here, it "normalizes" the level like a Bias Field Correction, and gives a well defined signature for the brain part...
p ( FilterParamDiameter   ) = MDsize;
p ( FilterParamResultType ) = FilterResultSigned;
fullbrain.Filter ( FilterTypeMeanDiv, p );


if ( fullbrain.IsNull () )
    return  false;

                                        // mask again
fullbrain.ApplyMaskToData ( headmask );


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".2.MeanDiv." DefaultMriExt );
fullbrain.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );
RemoveExtension ( _file, 3 );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) threshold to the brain part
Gauge.Next ();


THistogram          Hmd (   fullbrain,
                            &headmask,
                            0,
                            0,
                            0,  3, 
                            (HistogramOptions) ( HistogramPDF | HistogramRaw | HistogramNormNone | HistogramLinear | HistogramIgnoreNulls )
                        );


double              cutmdlow            = Hmd.GetPercentilePosition ( RealUnit, MDHistoCutLow  );
double              cutmdhigh           = Hmd.GetPercentilePosition ( RealUnit, MDHistoCutHigh );

                                        // Focusing on the brain part
p ( FilterParamThresholdMin )     = cutmdlow ;
p ( FilterParamThresholdMax )     = cutmdhigh;
p ( FilterParamThresholdBin )     = 1;
fullbrain.Filter ( FilterTypeThresholdBinarize, p );

                                        // purge out little bits that can leak later on - plus it helps picking the biggest guy...
//p ( FilterParamNumNeighbors )     = 13;
//p ( FilterParamNeighborhood )     = Neighbors26;
//fullbrain.Filter ( FilterTypeLessNeighbors, p );
//fullbrain.Filter ( FilterTypeLessNeighbors, p );


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".2.MeanDivCut." DefaultMriExt );
fullbrain.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
RemoveExtension ( _file, 3 );
#endif

Hmd.DeallocateMemory ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Working with a series of masks
Gauge.Next ();


Volume              allmasks ( Dim1, Dim2, Dim3 );
Volume              onemask  ( Dim1, Dim2, Dim3 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // First series of masks (2)
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Mask LMD1 - good upper part, less good at cerebellum
Gauge.Next ();

onemask     = fullbrain;


                                        // if we put a lot of data in (high threshold), we have enough data to do some Erode
Gauge.Next ();

p ( FilterParamDiameter )     = 1 * DilateSize;
onemask.Filter ( FilterTypeErode, p );


p ( FilterParamNumNeighbors )     = 19;                       // some more removal, but not too much either, or details start to disappear!
p ( FilterParamNeighborhood )     = Neighbors26;
onemask.Filter ( FilterTypeLessNeighbors, p );

                                        // try to directly keep the biggest part (without fullness), which should be disconnected by now
Gauge.Next ();

onemask.KeepRegion ( SortRegionsCompactCount, GetLinearDim () * 0.001, INT_MAX, Neighbors26, 0 );


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".25.KeepA0." DefaultMriExt );
onemask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
RemoveExtension ( _file, 3 );
#endif


Gauge.Next ();
                                        // includes first part for Close
p ( FilterParamDiameter )     = ( 4 + 2 ) * DilateSize;
onemask.Filter ( FilterTypeDilate, p );
//p ( FilterParamDiameter )     = 2 * 3.5 * DilateSize;
//p ( FilterParamResultType )   = FilterResultSigned;
//onemask.Filter ( FilterTypeMax, p );    // Max is slower, but this mask sure looks good!


Gauge.Next ();

p ( FilterParamToMaskThreshold )     = SingleFloatEpsilon;
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
onemask.Filter ( FilterTypeToMask, p );

                                        // second part for Close
p ( FilterParamDiameter )     = 2 * DilateSize;
onemask.Filter ( FilterTypeErode, p );


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".25.MaskA0." DefaultMriExt );
onemask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
RemoveExtension ( _file, 3 );
#endif


allmasks   += onemask;
allmasks   += onemask;  // !add more weight to this mask, which looks more defined in the upper part!


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Mask LMD2 - good global shape, but more eroded
Gauge.Next ();

onemask     = fullbrain;


p ( FilterParamNumNeighbors )     = 19;                       // More delicate than Erode - in case we have less data and need to be picky..
p ( FilterParamNeighborhood )     = Neighbors26;
onemask.Filter ( FilterTypeLessNeighbors, p );
onemask.Filter ( FilterTypeLessNeighbors, p );
onemask.Filter ( FilterTypeLessNeighbors, p );
onemask.Filter ( FilterTypeLessNeighbors, p );
//onemask.Filter ( FilterTypeLessNeighbors, p );    // some rare cases need more erosion - could use some smart iterative erosion
//onemask.Filter ( FilterTypeLessNeighbors, p );

                                        // try to directly keep the biggest part (without fullness), which should be disconnected by now
Gauge.Next ();

onemask.KeepRegion ( SortRegionsCompactCount, GetLinearDim () * 0.001, INT_MAX, Neighbors26, 0 );


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".25.KeepB0." DefaultMriExt );
onemask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
RemoveExtension ( _file, 3 );
#endif


Gauge.Next ();
                                        // includes first part for Close
p ( FilterParamDiameter )     = ( 4 + 2 ) * DilateSize;
onemask.Filter ( FilterTypeDilate, p );
//p ( FilterParamDiameter )     = 2 * 4 * DilateSize;
//p ( FilterParamResultType )   = FilterResultSigned;
//onemask.Filter ( FilterTypeMax, p );    // Max is slower, but this mask sure looks good!


Gauge.Next ();

p ( FilterParamToMaskThreshold )     = SingleFloatEpsilon;
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
onemask.Filter ( FilterTypeToMask, p );

                                        // second part for Close
p ( FilterParamDiameter )     = 2 * DilateSize;
onemask.Filter ( FilterTypeErode, p );


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".25.MaskB0." DefaultMriExt );
onemask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
RemoveExtension ( _file, 3 );
#endif


allmasks   += onemask;
allmasks   += onemask;  // add more weight to this mask, which looks more defined in the upper part


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Second series of masks (4)
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Cut by % of Fullness, to disconnect the temporal bones & cerebellum
Gauge.Next ();
                                        // we can help by removing some small guys first
p ( FilterParamNumNeighbors )     = 13;
p ( FilterParamNeighborhood )     = Neighbors26;
fullbrain.Filter ( FilterTypeLessNeighbors, p );
p ( FilterParamNumNeighbors )     = 13;
p ( FilterParamNeighborhood )     = Neighbors26;
fullbrain.Filter ( FilterTypeLessNeighbors, p );


Gauge.Next ();

p ( FilterParamDiameter )     = PFsize;
fullbrain.Filter ( FilterTypeFastGaussian, p );


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".3.PF." DefaultMriExt );
fullbrain.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );
RemoveExtension ( _file, 3 );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute a set of PF cuts, each of them being processed on its own
Gauge.Next ();

THistogram          Hpf (   fullbrain,
                            &headmask,
                            0,
                            0,
                            0,  3, 
                            (HistogramOptions) ( HistogramPDF | HistogramRaw | HistogramNormNone | HistogramLinear | HistogramIgnoreNulls )
                        );


double              cutpflow1       = Hpf.GetPercentilePosition ( RealUnit, 0.50 ); // lowest cuts will include more floatting voxels, hence will need more erosion, but results usually look better
double              cutpflow2       = Hpf.GetPercentilePosition ( RealUnit, 0.60 );
double              cutpflow3       = Hpf.GetPercentilePosition ( RealUnit, 0.70 );
double              cutpflow4       = Hpf.GetPercentilePosition ( RealUnit, 0.80 ); // highest cuts will exclude more floatting voxels, and will be easier on the erosion, but results may not be the best looking ones


headmask.DeallocateMemory ();


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".3.PFHisto." FILEEXT_EEGSEF );
//Hpf.WriteFile ( _file, "PDF" );
//DBGV3 ( cutpflow, Hpf.GetFirstPosition (), Hpf.GetLastPosition (), "cutpflow  (min, max)" );

Hpf.ToCDF ( HistogramNormMax );
PostfixFilename ( _file, " CDF" );
Hpf.WriteFile ( _file, "CDF" );
RemoveExtension ( _file, 3 );

//DBGV2 ( PFHistocut, cutpflow, "PF cut: PFHistocut% -> cut" );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Mask PF1
Gauge.Next ();

onemask     = fullbrain;


p ( FilterParamThreshold )     = cutpflow1;
onemask.Filter ( FilterTypeThresholdAbove, p );


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".3.PFHistoCut1." DefaultMriExt );
onemask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );
RemoveExtension ( _file, 3 );
#endif


p ( FilterParamDiameter )     = 4 * DilateSize;
onemask.Filter ( FilterTypeErode, p );

                                        // purge out little bits that can leak later on - plus it helps picking the biggest guy...
//p ( FilterParamNumNeighbors )     = 13;
//p ( FilterParamNeighborhood )     = Neighbors26;
//onemask.Filter ( FilterTypeLessNeighbors, p );
//onemask.Filter ( FilterTypeLessNeighbors, p );

                                        // first with 26 neighbors, faster, to get rid of all major disconnected stuff
Gauge.Next ();

onemask.KeepRegion ( SortRegionsCompactCount, GetLinearDim () * 0.001, INT_MAX, Neighbors26, 0 );


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".4.Keep1." DefaultMriExt );
onemask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
RemoveExtension ( _file, 3 );
#endif


Gauge.Next ();
                                        // a lot of dilation, this mask is pretty small
p ( FilterParamDiameter )     = ( 7 + 2 ) * DilateSize;
onemask.Filter ( FilterTypeDilate, p );


Gauge.Next ();

p ( FilterParamToMaskThreshold )     = SingleFloatEpsilon;
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
onemask.Filter ( FilterTypeToMask, p );


Gauge.Next ();

p ( FilterParamDiameter )     = 7 * DilateSize;
p ( FilterParamNumRelax )     = 1;
onemask.Filter ( FilterTypeRelax, p );


Gauge.Next ();

p ( FilterParamDiameter )     = 2 * DilateSize;
onemask.Filter ( FilterTypeErode, p );


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".5.Mask1." DefaultMriExt );
onemask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
RemoveExtension ( _file, 3 );
#endif


allmasks   += onemask;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Mask PF2
Gauge.Next ();

onemask     = fullbrain;


p ( FilterParamThreshold )     = cutpflow2;
onemask.Filter ( FilterTypeThresholdAbove, p );


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".3.PFHistoCut2." DefaultMriExt );
onemask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );
RemoveExtension ( _file, 3 );
#endif


p ( FilterParamDiameter )     = 2 * DilateSize;
onemask.Filter ( FilterTypeErode, p );

                                        // purge out little bits that can leak later on - plus it helps picking the biggest guy...
//p ( FilterParamNumNeighbors )     = 13;
//p ( FilterParamNeighborhood )     = Neighbors26;
//onemask.Filter ( FilterTypeLessNeighbors, p );
//onemask.Filter ( FilterTypeLessNeighbors, p );

                                        // first with 26 neighbors, faster, to get rid of all major disconnected stuff
Gauge.Next ();

onemask.KeepRegion ( SortRegionsCompactCount, GetLinearDim () * 0.001, INT_MAX, Neighbors26, 0 );


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".4.Keep2." DefaultMriExt );
onemask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
RemoveExtension ( _file, 3 );
#endif


Gauge.Next ();

p ( FilterParamDiameter )     = ( 6 + 2 ) * DilateSize;
onemask.Filter ( FilterTypeDilate, p );


Gauge.Next ();

p ( FilterParamToMaskThreshold )     = SingleFloatEpsilon;
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
onemask.Filter ( FilterTypeToMask, p );


Gauge.Next ();

p ( FilterParamDiameter )     = 5 * DilateSize;
p ( FilterParamNumRelax )     = 1;
onemask.Filter ( FilterTypeRelax, p );


Gauge.Next ();

p ( FilterParamDiameter )     = 1 * DilateSize; // 2?
onemask.Filter ( FilterTypeErode, p );


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".5.Mask2." DefaultMriExt );
onemask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
RemoveExtension ( _file, 3 );
#endif


allmasks   += onemask;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Mask PF3
Gauge.Next ();

onemask     = fullbrain;


p ( FilterParamThreshold )     = cutpflow3;
onemask.Filter ( FilterTypeThresholdAbove, p );


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".3.PFHistoCut3." DefaultMriExt );
onemask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );
RemoveExtension ( _file, 3 );
#endif


p ( FilterParamDiameter )     = 1 * DilateSize; // 2?
onemask.Filter ( FilterTypeErode, p );

                                        // purge out little bits that can leak later on - plus it helps picking the biggest guy...
//p ( FilterParamNumNeighbors )     = 13;
//p ( FilterParamNeighborhood )     = Neighbors26;
//onemask.Filter ( FilterTypeLessNeighbors, p );
//onemask.Filter ( FilterTypeLessNeighbors, p );

                                        // first with 26 neighbors, faster, to get rid of all major disconnected stuff
Gauge.Next ();

onemask.KeepRegion ( SortRegionsCompactCount, GetLinearDim () * 0.001, INT_MAX, Neighbors26, 0 );


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".4.Keep3." DefaultMriExt );
onemask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
RemoveExtension ( _file, 3 );
#endif


Gauge.Next ();

p ( FilterParamDiameter )     = ( 5 + 2 ) * DilateSize;
onemask.Filter ( FilterTypeDilate, p );


Gauge.Next ();

p ( FilterParamToMaskThreshold )     = SingleFloatEpsilon;
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
onemask.Filter ( FilterTypeToMask, p );


Gauge.Next ();

p ( FilterParamDiameter )     = 5 * DilateSize;
p ( FilterParamNumRelax )     = 1;
onemask.Filter ( FilterTypeRelax, p );


Gauge.Next ();

p ( FilterParamDiameter )     = 1 * DilateSize;
onemask.Filter ( FilterTypeErode, p );


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".5.Mask3." DefaultMriExt );
onemask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
RemoveExtension ( _file, 3 );
#endif


allmasks   += onemask;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Mask PF4
Gauge.Next ();

onemask     = fullbrain;


p ( FilterParamThreshold )     = cutpflow4;
onemask.Filter ( FilterTypeThresholdAbove, p );


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".3.PFHistoCut4." DefaultMriExt );
onemask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatFloat );
RemoveExtension ( _file, 3 );
#endif


//p ( FilterParamDiameter )     = 1 * DilateSize;
//onemask.Filter ( FilterTypeErode, p );


//                                        // purge out little bits that can leak later on - plus it helps picking the biggest guy...
//p ( FilterParamNumNeighbors )     = 13;
//p ( FilterParamNeighborhood )     = Neighbors26;
//onemask.Filter ( FilterTypeLessNeighbors, p );
////onemask.Filter ( FilterTypeLessNeighbors, p );

                                        // first with 26 neighbors, faster, to get rid of all major disconnected stuff
Gauge.Next ();

onemask.KeepRegion ( SortRegionsCompactCount, GetLinearDim () * 0.001, INT_MAX, Neighbors26, 0 );


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".4.Keep4." DefaultMriExt );
onemask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
RemoveExtension ( _file, 3 );
#endif


Gauge.Next ();

p ( FilterParamDiameter )     = ( 5 + 2 ) * DilateSize;
onemask.Filter ( FilterTypeDilate, p );


Gauge.Next ();

p ( FilterParamToMaskThreshold )     = SingleFloatEpsilon;
p ( FilterParamToMaskNewValue  )     = 1;
p ( FilterParamToMaskCarveBack )     = true;
onemask.Filter ( FilterTypeToMask, p );


Gauge.Next ();

p ( FilterParamDiameter )     = 5 * DilateSize;
p ( FilterParamNumRelax )     = 1;
onemask.Filter ( FilterTypeRelax, p );


Gauge.Next ();

p ( FilterParamDiameter )     = 1 * DilateSize; // 2?
onemask.Filter ( FilterTypeErode, p );


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".5.Mask4." DefaultMriExt );
onemask.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
RemoveExtension ( _file, 3 );
#endif


allmasks   += onemask;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here, we are done with all masks
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".6.AllMasks." DefaultMriExt );
allmasks.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
RemoveExtension ( _file, 3 );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Final mask == Median of 4 masks
//Gauge.Next ();
//
//p ( FilterParamDiameter   ) = MDsize;
//p ( FilterParamResultType ) = FilterResultSigned;
//allmasks.Filter ( FilterTypeMedian, p );


Gauge.Next ();

//p ( FilterParamThresholdMin )   = 4;                        // 4 means either at least the first 2 masks, or at least all the 4 other masks
p ( FilterParamThresholdMin )     = 5;                        // 5 for more than the 2 first masks
p ( FilterParamThresholdMax )     = FLT_MAX;
p ( FilterParamThresholdBin )     = 1;
allmasks.Filter ( /*FilterTypeThreshold*/ FilterTypeThresholdBinarize, p );

                                        // With current settings, all intermediate masks were chosen to be a little fatter
                                        // we can apply a very tiny shrinking at the end
Gauge.Next ();

p ( FilterParamDiameter   ) = 2 * DilateSize;
p ( FilterParamResultType ) = FilterResultSigned;
allmasks.Filter ( FilterTypeMin, p );


#if defined(SkullStripping3SaveSteps)
StringAppend ( _file, ".7.MaskFinal." DefaultMriExt );
allmasks.WriteFile ( _file, 0, 0, 0, 0, NiftiTransformDefault, NiftiIntentCodeDefault, NiftiIntentNameDefault, AtomFormatByte );
RemoveExtension ( _file, 3 );
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // apply data to mask
Gauge.Next ();

                                        // check for no-nonsense mask
if ( allmasks.IsNull () )
    return  false;


ApplyMaskToData ( allmasks );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//Gauge.HappyEnd ();


return  true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
