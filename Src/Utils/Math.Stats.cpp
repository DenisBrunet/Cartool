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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "Math.Resampling.h"
#include    "Math.Random.h"
#include    "Math.Stats.h"
#include    "Math.Histo.h"
#include    "TArray1.h"
#include    "TVector.h"
#include    "CartoolTypes.h"            // PolarityType

#include    "TExportTracks.h"
#include    "GlobalOptimize.Tracks.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TEasyStats::TEasyStats ()
{
Reset ();
}


//        TEasyStats::~TEasyStats ()
//{
//DBGV2 ( IsAllocated (), NumItems, "Delete TEasyStats:  IsAllocated ()  NumItems" );
//}


void    TEasyStats::Reset ()
{
Data.ResetMemory ();                    // Do NOT deallocate the Data array so that it can be re-used, just reset content

NumItems            = 0;
Sorted              = true;

CacheSum            = 0;
CacheSum2           = 0;
CacheMin            =  DBL_MAX;
CacheMax            = -DBL_MAX;
}


            TEasyStats::TEasyStats ( const TEasyStats &op )
{
Data                = op.Data;
NumItems            = op.NumItems;
Sorted              = op.Sorted;

CacheSum            = op.CacheSum;
CacheSum2           = op.CacheSum2;
CacheMin            = op.CacheMin;
CacheMax            = op.CacheMax;
}


TEasyStats& TEasyStats::operator= ( const TEasyStats &op2 )
{
if ( &op2 == this )
    return  *this;


Data                = op2.Data;
NumItems            = op2.NumItems;
Sorted              = op2.Sorted;

CacheSum            = op2.CacheSum;
CacheSum2           = op2.CacheSum2;
CacheMin            = op2.CacheMin;
CacheMax            = op2.CacheMax;

return  *this;
}


//----------------------------------------------------------------------------
void    TEasyStats::Show ( const char *title )
{
char                buff[ 1024 ];


StringCopy      ( buff,     "Number of Data: "  Tab,        IntegerToString ( NumItems ) );

StringAppend    ( buff,     NewLine );

StringAppend    ( buff,     NewLine "Average: " Tab Tab,    FloatToString ( Average () ) );
StringAppend    ( buff,     NewLine "SD: "      Tab Tab,    FloatToString ( SD ()      ) );
StringAppend    ( buff,     NewLine "CoV: "     Tab Tab,    FloatToString ( CoV ()     ) );
StringAppend    ( buff,     NewLine "SNR: "     Tab Tab,    FloatToString ( SNR ()     ) );
StringAppend    ( buff,     NewLine "Min: "     Tab Tab,    FloatToString ( Min ()     ) );
StringAppend    ( buff,     NewLine "Max: "     Tab Tab,    FloatToString ( Max ()     ) );


if ( IsAllocated () ) {

    StringAppend    ( buff,     NewLine );

    StringAppend    ( buff,     NewLine "Median: "  Tab Tab,    FloatToString ( Median ()             ) );
    StringAppend    ( buff,     NewLine "Mode: "    Tab Tab,    FloatToString ( MaxModeHistogram ()   ) );
    StringAppend    ( buff,     NewLine "IQRange: " Tab,        FloatToString ( InterQuartileRange () ) );
    StringAppend    ( buff,     NewLine "Sn: "      Tab Tab,    FloatToString ( Sn ( 1000 )           ) );
    StringAppend    ( buff,     NewLine "Qn: "      Tab Tab,    FloatToString ( Qn ( 1000 )           ) );
    StringAppend    ( buff,     NewLine "MAD: "     Tab Tab,    FloatToString ( MAD ()                ) );
    StringAppend    ( buff,     NewLine "MCoV: "    Tab Tab,    FloatToString ( RobustCoV ()          ) );
    }


ShowMessage ( buff, StringIsEmpty ( title ) ? "Statistics" : title );
}


void    TEasyStats::ShowData ( const char *title )  const
{
                                        // array can be allocated and still have no items in the stat object
if ( IsNotAllocated () || NumItems == 0 ) {
    ShowMessage ( "- Statistic array empty -", StringIsEmpty ( title ) ? "Easy Stats" : title );
    return;
    }


char                buff      [ 32 ];
char                localtitle[ 256 ];


for ( int i = 0; i < NumItems; i++ ) {

    StringCopy  ( localtitle, StringIsEmpty ( title ) ? "Easy Stats" : title, " / Item#", FloatToString ( buff, i + 1 ) );

    ShowMessage ( FloatToString ( buff, Data[ i ] ), localtitle );
    }
}


//----------------------------------------------------------------------------
void    TEasyStats::WriteFileData ( char *file )
{
                                        // can write an empty file
Data.WriteFile ( file, "Stats", NumItems );
}


void    TEasyStats::WriteFileVerbose ( char *file )
{
if ( StringIsEmpty ( file ) )
    return;


ofstream            ofs ( TFileName ( file, TFilenameExtendedPath ) );

if ( ofs.fail () )
    return;


ofs << StreamFormatScientific;


ofs << "Number of Data:"            Tab << StreamFormatInt32   << NumItems              << NewLine;
ofs << "Average:"               Tab Tab << StreamFormatFloat64 << Average ()            << NewLine;
ofs << "SD:"            Tab Tab Tab Tab << StreamFormatFloat64 << SD ()                 << NewLine;
ofs << "CoV:"               Tab Tab Tab << StreamFormatFloat64 << CoV ()                << NewLine;
ofs << "SNR:"               Tab Tab Tab << StreamFormatFloat64 << SNR ()                << NewLine;
ofs << "Min:"               Tab Tab Tab << StreamFormatFloat64 << Min ()                << NewLine;
ofs << "Max:"               Tab Tab Tab << StreamFormatFloat64 << Max ()                << NewLine;

if ( IsAllocated () ) {
    ofs << "Median:"            Tab Tab Tab << StreamFormatFloat64 << Median ()             << NewLine;
    ofs << "MaxMode:"           Tab Tab Tab << StreamFormatFloat64 << MaxModeHistogram ()   << NewLine;
    ofs << "MAD:"               Tab Tab Tab << StreamFormatFloat64 << MAD ()                << NewLine;
    ofs << "InterQuartileRange:"        Tab << StreamFormatFloat64 << InterQuartileRange () << NewLine;
    ofs << "RobustCoV:"             Tab Tab << StreamFormatFloat64 << RobustCoV ()          << NewLine;
    }
}


void    TEasyStats::WriteFileHistogram ( char *file, bool smooth )
{
if ( StringIsEmpty ( file ) || ! IsAllocated () )
    return;


THistogram          curve;
                                        // normalize so max mode == 1, giving a relative probability
                                        // normalizing by area (sum area = 1) is not what we want, as it will take the tail into account
if ( smooth )
    curve.ComputeHistogram  (   *this,
                                0,      0,      0, 
                                3,      10, 
                                (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramNormMax | HistogramLinear )
                            );
else
    curve.ComputeHistogram  (   *this,
                                0,      0,      0, 
                                0,      1, 
                                (HistogramOptions) ( HistogramPDF | HistogramRaw | HistogramNormMax | HistogramLinear )
                            );

curve.WriteFile ( file );
}


//----------------------------------------------------------------------------
void    TEasyStats::Resize ( int numitems )
{
Data.Resize ( AtLeast ( 0, numitems ) );    // will also gracefully deallocate if size is 0

Reset ();
}


//----------------------------------------------------------------------------
void    TEasyStats::Set ( const TArray1<double> &array1, bool allocate )
{
                                        // allocate at least the needed size, but don't deallocate
                                        // will also Reset content all the time
Resize ( AtLeast ( (size_t) MaxSize (), allocate ? array1.GetLinearDim () : 0 ) );


for ( int i = 0; i < array1.GetLinearDim (); i++ )
    Add ( array1[ i ], ThreadSafetyIgnore );
}


void    TEasyStats::Set ( const TArray1<float> &array1, bool allocate )
{
                                        // allocate at least the needed size, but don't deallocate
                                        // will also Reset content all the time
Resize ( AtLeast ( (size_t) MaxSize (), allocate ? array1.GetLinearDim () : 0 ) );


for ( int i = 0; i < array1.GetLinearDim (); i++ )
    Add ( array1[ i ], ThreadSafetyIgnore );
}


void    TEasyStats::Set ( const TMap* maps, bool allocate )
{
                                        // allocate at least the needed size, but don't deallocate
                                        // will also Reset content all the time
Resize ( AtLeast ( (size_t) MaxSize (), allocate ? maps->GetLinearDim () : 0 ) );


for ( int i = 0; i < maps->GetLinearDim (); i++ )
    Add ( maps->GetValue ( i ), ThreadSafetyIgnore );
}


//----------------------------------------------------------------------------
void    TEasyStats::Set ( const TArray2<int> &array2, bool allocate )
{
                                        // allocate at least the needed size, but don't deallocate
                                        // will also Reset content all the time
Resize ( AtLeast ( (size_t) MaxSize (), allocate ? array2.GetLinearDim () : 0 ) );


for ( int i = 0; i < array2.GetLinearDim (); i++ )
    Add ( array2.GetValue ( i ), ThreadSafetyIgnore );
}

                                        // Filling with all tracks, optionally subsampling the array "linearly"
void    TEasyStats::Set ( const TArray2<float> &array2, bool allocate, int maxitems )
{
int                 linearsize      = array2.GetLinearDim ();
int                 stepmaps        = 1;


if ( allocate ) 
    
    if ( maxitems <= 0 )

        maxitems    = linearsize;

    else {
        stepmaps    = AtLeast  ( 1, Truncate ( linearsize / maxitems ) );
        maxitems    = Truncate ( ( linearsize + stepmaps - 1 ) / stepmaps );
        }
else

    maxitems    = 0;
                                        // allocate at least the needed size, but don't deallocate
                                        // will also Reset content all the time
Resize ( AtLeast ( MaxSize (), maxitems ) );


for ( int i = 0; i < linearsize; i += stepmaps )
    Add ( array2.GetValue ( i ), ThreadSafetyIgnore );
}


//----------------------------------------------------------------------------
void    TEasyStats::Set ( const TArray3<uchar> &array3, bool ignorenulls, bool allocate )
{
                                        // allocate at least the needed size, but don't deallocate
                                        // will also Reset content all the time
Resize ( AtLeast ( (size_t) MaxSize (), allocate ? array3.GetLinearDim () : 0 ) );


for ( int i = 0; i < array3.GetLinearDim (); i++ ) {

    if ( ignorenulls && ! array3  [ i ] )   continue;

    Add ( array3[ i ], ThreadSafetyIgnore );
    }
}


void    TEasyStats::Set ( const Volume& array3, const Volume* mask, bool ignorenulls, bool allocate, int numdownsamples )
{
                                        // in case of mask, we can restrict to the exact number of set voxels
int                 numitems        = allocate  ? ( mask ? mask->GetNumSet () : array3.GetLinearDim () ) 
                                                : 0;
                                        // this will handle all cases, even if numdownsamples is <= 0
TDownsampling       down    ( numitems, numdownsamples );

                                        // optional downsampling?
bool                isdown          = down.Step > 1;


Resize ( AtLeast ( MaxSize (), down.NumDownData ) );


for ( int i = 0, j = 0; i < array3.GetLinearDim (); i++ ) {

    if ( mask        && ! (*mask) ( i ) )   continue;
    if ( ignorenulls && ! array3  [ i ] )   continue;
    if ( isdown && ( j++ % down.Step ) )    continue;   // test must be on a separate line, downsampling count is after masking

    Add ( array3[ i ], ThreadSafetyIgnore );
    }
}


void    TEasyStats::Set ( const TArray3<double>& array3, const Volume* mask, bool ignorenulls, bool allocate )
{
                                        // in case of mask, we can restrict to the exact number of set voxels
Resize ( AtLeast ( (size_t) MaxSize (), allocate ? ( mask ? mask->GetNumSet () : array3.GetLinearDim () ) : 0 ) );


for ( int i = 0; i < array3.GetLinearDim (); i++ ) {

    if ( mask        && ! (*mask) ( i ) )   continue;
    if ( ignorenulls && ! array3  [ i ] )   continue;

    Add ( array3[ i ], ThreadSafetyIgnore );
    }
}


//----------------------------------------------------------------------------
                                        // Filling with all tracks, optionally with only a subset of maps (but all tracks put in), to approximately reach the required amount of data
void    TEasyStats::Set ( const TMaps& maps, bool allocate, int maxitems )
{
int                 linearsize      = maps.GetLinearDim ();
int                 stepmaps        = 1;


if ( allocate ) 
    
    if ( maxitems <= 0 )

        maxitems    = linearsize;

    else {
        stepmaps    = AtLeast  ( 1, Truncate ( linearsize / maxitems ) );
        maxitems    = Truncate ( ( linearsize + stepmaps - 1 ) / stepmaps );
        }
else // ! allocate

    maxitems    = 0;
                                        // allocate at least the needed size, but doesn't deallocate
                                        // will also Reset content all the time
Resize ( AtLeast ( MaxSize (), maxitems ) );


for ( int mi = 0; mi < maps.GetNumMaps   (); mi+=stepmaps )
for ( int e  = 0; e  < maps.GetDimension (); e ++ )

    Add ( maps[ mi ][ e ], ThreadSafetyIgnore );
}


//----------------------------------------------------------------------------
void    TEasyStats::Resample ( TEasyStats& substats, double percentage, TVector<int>& randindex, TRandUniform* randunif )    const
{
int                 samplesize          = Clip ( percentage, 0.0, 1.0 ) * NumItems;

Resample ( substats, samplesize, randindex, randunif );
}


void    TEasyStats::Resample ( TEasyStats& substats, int samplesize, TVector<int>& randindex, TRandUniform* randunif )    const
{
substats.Reset ();

if ( ! IsAllocated () || samplesize <= 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Resize upward only, then reset
substats.Resize ( AtLeast ( samplesize, substats.MaxSize () ) );

                                        // get a random series of indexes
//TVector<int>        randindex;

randindex.RandomSeries ( samplesize, NumItems, randunif );

                                        // copy the necessary amount of data
for ( int i = 0; i < samplesize; i++ )

    substats.Add ( Data[ randindex[ i ] ], ThreadSafetyIgnore );
}


//----------------------------------------------------------------------------
                                        // Keep only data within inclusive range
void    TEasyStats::KeepWithin  ( double minvalue, double maxvalue )
{
if ( ! ( IsAllocated () && NumItems ) )
    return;


CheckOrder ( minvalue, maxvalue );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Descending sort
Sort ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Getting rid of special cases
int                 lastitem        = NumItems - 1;

                                        // All data already within range?
if (    Data[ 0        ] <= maxvalue   
     && Data[ lastitem ] >= minvalue ) 

    return;

                                        // All data outside range?
if (    Data[ 0        ] < minvalue
     || Data[ lastitem ] > maxvalue ) {
                                        // Remove everything
    Reset ();

    return;
    }

                                        // Here data has some partial overlap with the range to keep
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Scan all data
OmpParallelFor

for ( int i = 0; i < NumItems; i++ )

    if ( ! IsInsideLimits ( (double) Data[ i ], minvalue, maxvalue ) )
                                        // assign an out of bound, below min so that these values will end up at the end of sorted array
        Data[ i ]   = minvalue - 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Descending sort, bad guys at the end
Sort ( true );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Shorten list from the end i.e lowest values which are the outliers
for ( ; NumItems > 0; NumItems-- )
                                        // If tail is above min, we can stop, otherwise keep on removing
    if ( Data[ NumItems - 1 ] >= minvalue )

        break;
}


//----------------------------------------------------------------------------
                                        // Remove null data - works even with a mix of positive and negative values
void    TEasyStats::RemoveNulls ()
{
if ( ! ( IsAllocated () && NumItems ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Descending sort
Sort ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Getting rid of special cases
int                 lastitem        = NumItems - 1;

if (    Data[ 0        ] < 0            // only strictly negative values?
     || Data[ lastitem ] > 0 )          // only strictly positive values?
                                        // Therefor there are no nulls there
    return;

                                        // Here data can be >= 0, <= 0, or full positive and negative range (with or w/o 0's)
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // last index of equal-to-0 data
int                 laste0i;

for ( laste0i = lastitem; laste0i >= 0; laste0i-- )

    if ( Data[ laste0i ] == 0 ) break;

                                        // no null values at all?
if ( laste0i < 0 )

    return;

                                        // Here we have at least 1 null value
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // first index of equal-to-0 data
int                 firste0;

for ( firste0 = laste0i; firste0 >= 0; firste0-- )

    if ( Data[ firste0 ] != 0 ) break;

                                        // we stopped at the first non-null values, go back 1 step
firste0++;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Finally getting rid of the 0's
                                        // This should be <= NumItems
int                 num0            = laste0i - firste0 + 1;

                                        // Degenerate case
if ( num0 == NumItems ) {
                                        // Let's be clean and call Reset
    Reset ();

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Case of trail of 0's
if ( laste0i == lastitem ) {
                                        // just ignoring trail is enough - no need to move any memory
    NumItems   -= num0;

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here we have an existing range of 0's in the middle of the data

                                        // Move the negative part to join the positive part, overwriting the 0's
MoveVirtualMemory   (   Data.GetArray () + firste0,
                        Data.GetArray () + laste0i + 1,
                        ( lastitem - laste0i ) * Data.AtomSize () );

                                        // Adjust remaining count
NumItems   -= num0;
}


//----------------------------------------------------------------------------
                                        // Randomized (some) functions, returning the Mean of all sampling
double  TEasyStats::Randomize   (   TEasyStatsFunctionType  how, 
                                    int                     numresampling,  int         samplesize,
                                    TArray1<double>*        params
                                )
{
                                        // run randoms subsampling
TEasyStats          substats;
TVector<int>        randindex;
TRandUniform        randunif;

double              var         = 0;

if      ( how == TEasyStatsFunctionMADLeft ) {
    (*params)[ 1 ]  = 0;
    }
else if ( how == TEasyStatsFunctionMADAsym ) {
    (*params)[ 1 ]  = 0;
    (*params)[ 2 ]  = 0;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//OmpParallelForSum // but one for var, another for (*params)[ 1 ] etc

for ( int i = 0; i < numresampling; i++ ) {
                                        // use only a % of the whole dataset
    Resample ( substats, samplesize, randindex, &randunif );


    if      ( how == TEasyStatsFunctionMaxMode              )   var    += substats.MaxModeHSM ();        // set a default mode
    else if ( how == TEasyStatsFunctionMaxModeHistogram     )   var    += substats.MaxModeHistogram ();
    else if ( how == TEasyStatsFunctionMaxModeHSM           )   var    += substats.MaxModeHSM ();
    else if ( how == TEasyStatsFunctionMaxModeHRM           )   var    += substats.MaxModeHRM ();

    else if ( how == TEasyStatsFunctionSD                   )   var    += substats.SD ();
    else if ( how == TEasyStatsFunctionInterQuartileRange   )   var    += substats.InterQuartileRange ();
    else if ( how == TEasyStatsFunctionMAD                  )   var    += substats.MAD ( CanAlterData );

    else if ( how == TEasyStatsFunctionMADLeft              ) { 
        double          madleft;

        substats.MADLeft ( (*params)[ 0 ], madleft );  

        (*params)[ 1 ]     += madleft; 
        }

    else if ( how == TEasyStatsFunctionMADAsym              ) {
        double          madleft;
        double          madright;

        substats.MADAsym ( (*params)[ 0 ], madleft, madright );  

        (*params)[ 1 ]     += madleft; 
        (*params)[ 2 ]     += madright; 
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if      ( how == TEasyStatsFunctionMADLeft ) {
    (*params)[ 1 ]     /= numresampling;
    }
else if ( how == TEasyStatsFunctionMADAsym ) {
    (*params)[ 1 ]     /= numresampling;
    (*params)[ 2 ]     /= numresampling;
    }
else
    var    /= numresampling;


return  var;
}


//----------------------------------------------------------------------------
                                        // No thread checks - this is our protected method
void    TEasyStats::_Add ( double v )
{
                                        // Can not really do stats with these guys - sorry
//if ( IsNotAProperNumber ( v ) )
//    return;


if ( IsAllocated () ) {
                                        // is the buffer full?
    if ( NumItems >= (int) Data ) {
                                        // let's be _super_ nice (though not optimal, but only on the first overflow): reallocate data to store all needed data

                                        // Allocate exponentially at the beginning, then linearly to avoid explosions!
                                                                                    //   2^0.5-1     2^0.25 - 1   but no more than 10MB
        size_t              deltamemsize    = NoMore ( Truncate ( Data.MemorySize () * /*0.4142136*/ 0.18921 ),   10 * (1 << 20) );
                                        // delta in number of elements should be worth it
        size_t              deltasize       = AtLeast ( (size_t) 16, deltamemsize / Data.AtomSize () );
                                        // finally our new size
        size_t              newsize         = (int) Data + deltasize;

        Data.Resize ( newsize, (MemoryAllocationType) ( MemoryAuto | ResizeKeepMemory ) );
        }

                                        // insert at the end of the list
    Data[ NumItems ]    = v;

    Sorted      = false;
    } // IsAllocated

else {
                                        // cheap stats, just update these variables on the fly
    CacheSum   += v;
    CacheSum2  += v * v;

    Maxed ( CacheMax, v );
    Mined ( CacheMin, v );
    } // ! IsAllocated


NumItems++;
}
                                        // !Caller must be very careful to the thread-safety:
                                        // ! - default is to use the critical section
                                        // ! - however calling with unnecessary ThreadSafe flag will slow down runtime
                                        // A good way to avoid locks is to allocate the TEasyStats objects within the parallel section
                                        // or to use a TGoEasyStats corresponding to the outer loop.
void    TEasyStats::Add ( double v, ThreadSafety safety )
{
                                        // Can not really do stats with these guys - sorry
                                        // For the moment, we can do the test here only
if ( IsNotAProperNumber ( v ) )
    return;

                                // !force skipping critical section / lock if not in a parallel block!
if ( safety == ThreadSafetyCare && IsInParallelCode () ) {
                                        // wrapping the whole Add, to make it atomic
    OmpCriticalBegin (TEasyStatsAdd)

    _Add ( v );

    OmpCriticalEnd
    }
else
    _Add ( v );
}


//----------------------------------------------------------------------------
void    TEasyStats::Add ( complex<float> c, ThreadSafety safety )
{
                                        // store the norm of complex
Add ( abs ( c ), safety );
}


//----------------------------------------------------------------------------
                                        // !! formula needs some more testing !!
void    TEasyStats::AddAngle ( double a, ThreadSafety safety )
{
if ( NumItems ) {

    double      avg     = Average ();

    if      ( a < avg - Pi )    a  += TwoPi;    // do a better formula for more than 2Pi distance
    else if ( a > avg + Pi )    a  -= TwoPi;

    Add ( a, safety );
    }
else {
                                        // make sure the angle is between 0 and 2Pi
    a       = NormalizeAngle ( a, TwoPi );
                                        // then insert
    Add ( a, safety );
    }
}


//----------------------------------------------------------------------------
void    TEasyStats::Add ( const TArray1<float> &array1, ThreadSafety safety )
{
if ( array1.IsNotAllocated () )
    return;

                                // !force skipping critical section / lock if not in a parallel block!
if ( safety == ThreadSafetyCare && IsInParallelCode () ) {
                                        // wrapping the whole loop instead of each individual calls
    OmpCriticalBegin (TEasyStatsAddArray1)

    for ( int i = 0; i < array1.GetLinearDim (); i++ )
        Add ( array1[ i ], ThreadSafetyIgnore );

    OmpCriticalEnd
    }
else

    for ( int i = 0; i < array1.GetLinearDim (); i++ )
        Add ( array1[ i ], ThreadSafetyIgnore );
}


//----------------------------------------------------------------------------
//  Data ordering / storage not relevant
//----------------------------------------------------------------------------
double  TEasyStats::Sum ()
{
if ( ! NumItems )
    return  0;


if ( IsAllocated () ) {
    double              sum             = 0;

    OmpParallelForSum ( sum )

    for ( int i = 0; i < NumItems; i++ )
        sum    += Data[ i ];

    return  sum;
    }
else
    return  CacheSum;
}


double  TEasyStats::Sum2 ()
{
if ( ! NumItems )
    return  0;


if ( IsAllocated () ) {
    double              sum2            = 0;

    OmpParallelForSum ( sum2 )

    for ( int i = 0; i < NumItems; i++ )
        sum2   += Square ( Data[ i ] );

    return  sum2;
    }
else
    return  CacheSum2;
}


double  TEasyStats::Average ()
{
return  NumItems ? Sum () / NumItems : 0;
}


double  TEasyStats::Variance ()
{
                                        // variance needs at least 2 data!
if ( NumItems < 2 )
    return  0;


if ( IsAllocated () ) {
    double              mean            = Average ();
    double              var             = 0;

                                        // explicit formula, should be numerically more precise as the summed items are smaller
    OmpParallelForSum ( var )

    for ( int i = 0; i < NumItems; i++ )
        var    += Square ( Data[ i ] - mean );

                                        // unbiased estimator
    return  var / ( NumItems - 1 );
    }
else
                                        // unbiased estimator
    return  AtLeast ( 0.0, ( CacheSum2 - Square ( CacheSum ) / NumItems ) / ( NumItems - 1 ) );


//return  NumItems > 1 ? CacheSum == DBL_MAX ? 1e100
//                                             : ( CacheSum2 - Square ( CacheSum ) / NumItems ) / ( NumItems - 1 )
//                      : 0;
}


double  TEasyStats::Variance ( double center )
{
                                        // variance needs at least 2 data!
if ( NumItems < 2 )
    return  0;


if ( IsAllocated () ) {
    double              var             = 0;

                                        // explicit formula, should be numerically more precise as the summed items are smaller
    OmpParallelForSum ( var )

    for ( int i = 0; i < NumItems; i++ )
        var    += Square ( Data[ i ] - center );

                                        // unbiased estimator
    return  var / ( NumItems - 1 );
    }
else
                                        
    return  center == 0 ? AtLeast ( 0.0, CacheSum2 / ( NumItems - 1 ) ) // the only case we can answer correctly here
                        : 0 /*Variance ()*/;                            // otherwise, return the (wrong) regular variance(?)


//return  NumItems > 1 ? CacheSum == DBL_MAX ? 1e100
//                                             : ( CacheSum2 - Square ( CacheSum ) / NumItems ) / ( NumItems - 1 )
//                      : 0;
}


double  TEasyStats::RMS ()
{
return  NumItems ? sqrt ( Sum2 () / NumItems ) : 0;


//return  NumItems > 1 ? Sum == DBL_MAX ? 1e100
//                                        : sqrt ( Sum2 / NumItems )
//                      : 0;
}


double  TEasyStats::ZScore ( double v )
{
return  crtl::ZScore ( v, Average (), SD () );
}


double  TEasyStats::ZScoreRobust ( double v )
{
return  crtl::ZScore ( v, Median ( false ), InterQuartileRange () );
}


double  TEasyStats::CoV ()
{
double              avg             = Average ();

return  avg ? SD () / avg : 0;
}


double  TEasyStats::SignalToNoiseRatio ()
{
double              sd              = SD ();

return  sd ? Average () / sd : 0;
}

                                        // Mean is penalized by SD, itself normalized by range
                                        // SD can be given more or less penalty: 0 none, 1 standard, 2 heavy
double  TEasyStats::ConsistentMean ( double penalty )
{
//double              m               = Mean ();
//double              s               = SD () / NonNull ( AbsoluteMax () /*Range ()*/ );    // weighting the mean, more complex formula
//return  m * Clip ( 1 - penalty * s, (double) 0, (double) 1 );

double              m               = Mean ();
double              s               = SD   ();
return  m - penalty * s;

//double              m               = Mean ();
//double              s               = SD   ();
//return  m / ( 1 + s );
}

                                        // IQR can be given more or less penalty: 0 none, 1 standard, 2 heavy
double  TEasyStats::ConsistentMedian ( double penalty )
{
//double              m               = Median ();
//double              s               = InterQuartileRange () / NonNull ( AbsoluteMax () /*Range ()*/ );
//return  m * Clip ( 1 - penalty * s, (double) 0, (double) 1 );

double              m               = Median             ( false );
double              s               = InterQuartileRange ();

//double              sdleft;
//double              sdright;
//SDAsym ( sdleft, sdright  );
//double              s               = ( InterQuartileRange () + Sn () + Qn () + MAD () + sdleft ) / 5;   // seems to give always a mid-section best cluster

return  m - penalty * s;

//double              m               = Median ();
//double              s               = InterQuartileRange    ();
//return  m / ( 1 + penalty * s );
}


double  TEasyStats::Normalize ( double v )
{
return  ( v - Min () ) / NonNull ( Range () );
}


//----------------------------------------------------------------------------
//  Data ordering / storage relevant
//----------------------------------------------------------------------------
void    TEasyStats::Sort ( bool force )
{
if ( /*! IsAllocated () ||*/ Sorted && ! force )
    return;

                                        // off for the moment - it seems all calls behave correctly as it has to handle Add before anyway(?)
//OmpCriticalBegin(TEasyStatsSort)

Data.Sort ( Descending, NumItems );

Sorted      = true;

//OmpCriticalEnd
}


//----------------------------------------------------------------------------
                                        // Useful when stats contain all pairs within a dataset and we want to recover the original number of items/nodes
int     TEasyStats::PairsToNumItems ( int numpairs )
{
if ( numpairs < 0 )
    numpairs    = NumItems;

return   ( sqrt ( (double) 8 * numpairs + 1 ) + 1 ) / 2;
}


int     TEasyStats::NumItemsToPairs ( int numitems )
{
if ( numitems < 0 )
    numitems    = NumItems;

return  numitems * ( numitems - 1 ) / 2;
}


//----------------------------------------------------------------------------
double  TEasyStats::Max ()
{
if ( IsAllocated () ) {

    Sort ();
                                        // sort is descending: first item
    return  NumItems ? Data[ 0 ] : 0;
    }
else
    return  CacheMax;
}


double  TEasyStats::Min ()
{
if ( IsAllocated () ) {

    Sort ();
                                        // sort is descending: last item
    return  NumItems ? Data[ NumItems - 1 ] : 0;
    }
else
    return  CacheMin;
}


double  TEasyStats::MinMax ( double v )
{
return  v <= ( Min () + Max () ) / 2 ? Min () : Max ();
}

/*
double  TEasyStats::MinMaxRobust ( double v )
{
                                        // robust version
//double              q25             = Quantile ( 0.25 );
//double              q75             = Quantile ( 0.75 );
double              q25             = TruncatedMean ( 0.00, 0.25 );
double              q75             = TruncatedMean ( 0.75, 1.00 );

return  v <= ( q25 + q75 ) / 2 ? q25 : q75;
}
*/

double  TEasyStats::Median ( bool strictvalue )
{
if ( ! ( IsAllocated () && NumItems ) )
    return  0;

                                        // get degenerate case out of the way
if ( NumItems == 1 )
    return  Data[ 0 ];
                                        // here at least 2 data

Sort ();

                                        // get index to half truncated position
int                 halfi           = ( NumItems - 1 ) / 2;

return  strictvalue || IsOdd ( NumItems ) ?   Data[ halfi ]                             // return the exact center value, or the closest one to remain within the dataset
                                          : ( Data[ halfi ] + Data[ halfi + 1 ] ) / 2;  // in case of even numbers, do the average between each sides - the result is NOT part of the original dataset, which sometimes could be problematic
}

                                        // Do a Variance separately on right and left
void    TEasyStats::VarianceAsym ( double center, double &varleft, double &varright )
{
varleft     = 0;
varright    = 0;

if ( ! ( IsAllocated () && NumItems ) )
    return;


//double              center          = Mean ();
double              vl              = 0;
double              vr              = 0;
int                 nl              = 0;
int                 nr              = 0;

                                        // explicit formula, should be numerically more precise as the summed items are smaller
OmpParallelForSum ( vl, vr, nl, nr )

for ( int i = 0; i < NumItems; i++ )

    if ( Data[ i ] > center ) { vr += Square ( Data[ i ] - center ); nr++; }
    else                      { vl += Square ( Data[ i ] - center ); nl++; }

                                        // unbiased estimators
varleft     = nl < 2 ? 0 : vl / ( nl - 1 );
varright    = nr < 2 ? 0 : vr / ( nr - 1 );
}

                                        // Do a SD separately on right and left
void    TEasyStats::SDAsym ( double center, double &sdleft, double &sdright )
{
VarianceAsym ( center, sdleft, sdright );

sdleft      = sqrt ( sdleft  );
sdright     = sqrt ( sdright );
}

                                        // Do a MAD separately on right and left
void    TEasyStats::MADAsym ( double center, double &madleft, double &madright )
{
madleft     =
madright    = 0;

if ( ! ( IsAllocated () && NumItems ) )
    return;

                                        // 1) get absolute deviations on each side of the given center
TEasyStats          deltal ( NumItems );
TEasyStats          deltar ( NumItems );
double              d;

for ( int i = 0; i < NumItems; i++ ) {

    d       = Data[ i ] - center;

    if ( d < 0 )    deltal.Add ( d, ThreadSafetyIgnore );
    else            deltar.Add ( d, ThreadSafetyIgnore );
    }

                                        // 2) get the medians of the deviations
                                        // rescaling to be an unbiased estimator of standard deviation sigma
madleft     = fabs ( deltal.Median ( false ) ) * MADToSigma;
madright    =        deltar.Median ( false )   * MADToSigma;
}


void    TEasyStats::MADLeft ( double center, double &madleft )
{
madleft     = 0;

if ( ! ( IsAllocated () && NumItems ) )
    return;

                                        // 1) get absolute deviations on LEFT side of the given center
TEasyStats          deltal ( NumItems );
double              d;

for ( int i = 0; i < NumItems; i++ ) {

    d       = Data[ i ] - center;

    if ( d < 0 )    deltal.Add ( d, ThreadSafetyIgnore );
    }

                                        // 2) get the medians of the deviations
                                        // rescaling to be an unbiased estimator of standard deviation sigma
madleft     = fabs ( deltal.Median ( false ) ) * MADToSigma;
}


void    TEasyStats::MADRight ( double center, double &madright )
{
madright    = 0;

if ( ! ( IsAllocated () && NumItems ) )
    return;

                                        // 1) get absolute deviations on RIGHT side of the given center
TEasyStats          deltar ( NumItems );
double              d;

for ( int i = 0; i < NumItems; i++ ) {

    d       = Data[ i ] - center;

    if ( d >= 0 )   deltar.Add ( d, ThreadSafetyIgnore );
    }

                                        // 2) get the medians of the deviations
                                        // rescaling to be an unbiased estimator of standard deviation sigma
madright    =        deltar.Median ( false )   * MADToSigma;
}

                                        // Caller can speed up things by allowing to alter the data - useful if it is going to be ditched soon after...
double  TEasyStats::MAD ( double center, StorageDataLife datalife )
{
if ( ! ( IsAllocated () && NumItems ) )
    return  0;


if ( datalife == CanAlterData ) {
                                        // Caller is fined with us destroying data
    for ( int i = 0; i < NumItems; i++ )
        Data[ i ]   = abs ( Data[ i ] - center );
                                        // !important, otherwise Median below will fail!
    Sorted  = false;
                                        // 2) get the median of the deviations
                                        // rescaling to be an unbiased estimator of standard deviation sigma
    return  Median ( false ) * MADToSigma;
    }

else {                                  // Caller wished to preserve the data - We will need some temp stats, which is costly
                                        
                                        // 1) get absolute deviations from median
    TEasyStats          delta ( NumItems );

    for ( int i = 0; i < NumItems; i++ )
        delta.Add ( abs ( Data[ i ] - center ), ThreadSafetyIgnore );
                                        // 2) get the median of the deviations
                                        // rescaling to be an unbiased estimator of standard deviation sigma
    return  delta.Median ( false ) * MADToSigma;
    }
}


double  TEasyStats::MAD ( StorageDataLife datalife )
{
return  MAD ( Median ( false ), datalife );
}


double  TEasyStats::RobustCoV ()
{
if ( ! ( IsAllocated () && NumItems ) )
    return  0;

double              median          = Median ( false );

return  median ? /*InterQuartileRange ()*/ MAD () / median : 0;
}


double  TEasyStats::RobustSNR ()
{
if ( ! ( IsAllocated () && NumItems ) )
    return  0;

double              sd              = MAD (); // InterQuartileRange ();

return  sd ? Median ( false ) / sd : 0;
}


double  TEasyStats::Quantile ( double p )
{
if ( ! ( IsAllocated () && NumItems ) )
    return  0;


Sort ();
                                        // truncated position, always returning an existing value
//return  Data[ Round ( ( NumItems - 1 ) * Clip ( 1 - p, (double) 0, (double) 1 ) ) ];  // data sorted descending

                                        // interpolated value, which will return a non-existing value (most of the time)
                                                            // data is sorted descending, Quantile starts from the lowest values
double              cut             = ( NumItems - 1 ) * Clip ( 1 - p, (double) 0, (double) 1 );
double              frac            = Fraction ( cut );

                                        // returns an interpolated value if needed (can matter with few data points)
if ( frac == 0 )    return  Data[ (int) cut ];
else                return  ( 1 - frac ) * Data[ (int) cut ] + frac * Data[ (int) cut + 1 ];    // fractional part -> there are neighbors on each side
}


double  TEasyStats::InterQuartileRange ()
{
if ( ! ( IsAllocated () && NumItems ) )
    return  0;
                                                 // rescaling to be an unbiased estimator of standard deviation sigma
return  ( Quantile ( 0.75 ) - Quantile ( 0.25 ) ) * IQRToSigma;
}

                                        // Note: by splitting the data in 2, it finally boils down to MADAsym...
void    TEasyStats::InterQuartileRangeAsym ( double center, double &iqrleft, double &iqrright )
{
iqrleft     =
iqrright    = 0;

if ( ! ( IsAllocated () && NumItems ) )
    return;

                                        // 1) make 2 stats, for all values on the left or on the right of center
TEasyStats          left  ( NumItems );
TEasyStats          right ( NumItems );


for ( int i = 0; i < NumItems; i++ )

    if ( Data[ i ] < center )   left .Add ( Data[ i ], ThreadSafetyIgnore );
    else                        right.Add ( Data[ i ], ThreadSafetyIgnore );


                                        // 2) use the median of each halves
iqrleft     = ( center                  - left .Quantile ( 0.50 ) ) * IQRToSigma * 2;
iqrright    = ( right.Quantile ( 0.50 ) - center                  ) * IQRToSigma * 2;
}


double  TEasyStats::TruncatedMean ( double qfrom, double qto )
{
if ( ! ( IsAllocated () && NumItems ) )
    return  0;

Sort ();

double              sum             = 0;
int                 fromi           = Round ( ( NumItems - 1 ) * Clip ( 1 - qto,   (double) 0, (double) 1 ) );  // data sorted descending
int                 toi             = Round ( ( NumItems - 1 ) * Clip ( 1 - qfrom, (double) 0, (double) 1 ) );


OmpParallelForSum ( sum )

for ( int i = fromi; i <= toi; i++ )
    sum        += Data[ i ];


return  sum / ( toi - fromi + 1 );
}


//----------------------------------------------------------------------------
                                        // adjusted Fisher-Pearson, parametric, sensitive to outliers
double  TEasyStats::SkewnessPearson ( double center )
{
if ( ! ( IsAllocated () && NumItems >= 3 ) )
    return  0;


double              den             = Cube ( SD ( center ) );

if ( den == 0 )
    return  0; // BigDoubleFloat;


double              skew            = 0;

OmpParallelForSum ( skew )

for ( int i = 0; i < NumItems; i++ )
    skew   += Cube ( Data[ i ] - center );

                                        // unbiased estimator
return  skew     / den
      * NumItems / (double) ( ( NumItems - 1 ) * ( NumItems - 2 ) );
}


double  TEasyStats::SkewnessPearson ()
{
return  SkewnessPearson ( Average () );
}
                                        // Do a Skewness separately on right and left
                                        // Although it seems not really intuitive, as the Skewness kind of compares the curve left vs right
                                        // this kind of still works if thought of 3d moment computation only. The left and right absolute values
                                        // can then be compared.
void    TEasyStats::SkewnessPearsonAsym ( double center, double &skewleft, double &skewright )
{
skewleft    = 0;
skewright   = 0;

if ( ! ( IsAllocated () && NumItems >= 2 * 3 ) )
    return;


double              sdleft;
double              sdright;

SDAsym ( center, sdleft, sdright );

double              denleft         = Cube ( sdleft  );
double              denright        = Cube ( sdright );


double              sl              = 0;
double              sr              = 0;
int                 nl              = 0;
int                 nr              = 0;

OmpParallelForSum ( sl, sr, nl, nr )

for ( int i = 0; i < NumItems; i++ )

    if ( Data[ i ] > center ) { sr += Cube ( Data[ i ] - center    ); nr++; }
    else                      { sl += Cube ( center    - Data[ i ] ); nl++; }  // to keep a positive value

                                        // unbiased estimator
skewleft    = nl < 3 ? 0 : sl * nl / (double) ( ( nl - 1 ) * ( nl - 2 ) ) / denleft;
skewright   = nr < 3 ? 0 : sr * nr / (double) ( ( nr - 1 ) * ( nr - 2 ) ) / denright;
}

                                        // Pearson, non-parametric, less sensitive to outliers
double  TEasyStats::RobustSkewnessPearson ()
{
if ( ! ( IsAllocated () && NumItems >= 3 ) )
    return  0;


double              mean            = Average ();
double              median          = Median  ( false );
double              sd              = MAD     ();   // should be SD, but MAD is way better!

if ( sd == 0 )
    return  0; // BigDoubleFloat;


return  3 * ( mean - median ) / sd;
}

                                        // non-parametric, robust, general formula
double  TEasyStats::RobustSkewnessQuantiles ( double alpha )
{
if ( ! ( IsAllocated () && NumItems >= 3 ) )
    return  0;


double              q1              = Quantile (     alpha );
double              q2              = Quantile (     0.50  );   // median
double              q3              = Quantile ( 1 - alpha );

if ( q3 - q1 == 0 )
    return  0; // BigDoubleFloat;

                                        // difference between delta quartiles to median, then normalized by IQR
return  ( q3 + q1 - 2 * q2 ) / ( q3 - q1 );
}

                                        // non-parametric, robust
double  TEasyStats::RobustSkewnessQuartiles ()
{
                                        // difference between delta quartiles to median, then normalized by IQR
return  RobustSkewnessQuantiles ( 0.25 );
}

                                        // non-parametric, robust
double  TEasyStats::RobustSkewnessMAD ( double center )
{
                                        // Get MADs on each side separately
double              madleft;
double              madright;

MADAsym ( center, madleft, madright );

if ( madleft + madright == 0 )
    return  0; // BigDoubleFloat;


return  ( madright - madleft ) / ( madleft + madright ) * 2;
}

                                        // Another one: ( mean - median ) / E ( data - median )

//----------------------------------------------------------------------------
                                        // Heuristic to guess if a dataset looks like angular values
bool    TEasyStats::IsAngular   ()
{
                                        // Test how far data are from the Uniform Distribution: compare to the expected mean, range and SD
double              relrange        = Range () / TwoPi;
double              relmean         = Mean ()  / Pi;
double              relsd           = SD ()    / sqrt ( Square ( TwoPi ) / 12 );
//double              skew            = SkewnessPearson ();           // expected: 0 -  not sure if data are stored
//double              allrels         = relrange * relmean * relsd;   // compound measure


//DBGV3 ( relrange, relmean, relsd, "IsAngular: relrange, relmean, relsd" );

                                        
return  ( IsInsideLimits ( relmean,  0.90, 1.15 )   //   0..pi case
       || IsInsideLimits ( relmean, -0.10, 0.10 ) ) // -pi..pi case
       && IsInsideLimits ( relrange, 0.80, 1.01 )   // quite some margin on the low side, we might be missing some high values due to sub/down-sampling
       && IsInsideLimits ( relsd,    0.90, 1.15 );

                                        // Other ideas:
                                        // abrupt transitions between 0 and 2pi, vs ramps !will not work correctly for sub-sampled data!
}


//----------------------------------------------------------------------------
                                        // Testing if the whole dataset is integer
bool    TEasyStats::IsInteger   ()
{
if ( ! ( IsAllocated () && NumItems > 0 ) )
    return  false;

                                        // no need to scan all dataset
TDownsampling       down ( NumItems, 1000 );

for ( int i = down.From; i <= down.To; i += down.Step )

    if ( IsFraction ( Data[ i ] ) )

        return  false;

return  true;
}


//----------------------------------------------------------------------------
int     TEasyStats::IsSkewed ()
{
int                 isskewed        = 0;

if ( ! IsAllocated () )
    return  isskewed;

                                        // Signed data are considered un-de-skewable (for the moment)
if ( Min () < 0 )
    return  isskewed;

                                        // Skewness functions can return 0 if they failed (lot of 0's f.ex. which mess up with stats)
double              skewness2       = RobustSkewnessPearson ();             //  1.05 +-0.32 / 0.24 +-0.16
//double            skewness3       = RobustSkewnessQuantiles ( 0.01 );     //  0.55 +-0.13 / 0.12 +-0.02
//double            skewness5       = RobustSkewnessMAD ( MaxModeHRM () );  //  0.73 +-0.13 / 0.22 +-0.20
double              skewness6       = RobustSkewnessMAD ( MaxModeHistogram () );    // same as above, but the HRM might be too sensitive and/or too long for our purpose here


if ( skewness2 && fabs ( skewness2 ) > 0.36 
//|| skewness5 && fabs ( skewness5 ) > 0.47
  || skewness6 && fabs ( skewness6 ) > 0.47 )

    isskewed    = TrueToPlus ( skewness6 > 0 );

//DBGV3 ( skewness2, skewness6, isskewed, "Skewed?" );


return  isskewed;
}

                                        // !The returned type is the one applied!
int     TEasyStats::DeSkew ( DeSkewType how )
{
int                 isskewed        = IsSkewed ();


if      ( how == DeSkewRight 
       || how == DeSkewAuto && isskewed ==  1 ) {

    ToLog ();

    return  1;
    }

else if ( how == DeSkewLeft 
       || how == DeSkewAuto && isskewed == -1 ) {

//  ToExp ();
    ToAntiLog ();

    return -1;
    }

else if ( how == DeSkewNone 
       || how == DeSkewAuto && isskewed ==  0 ) {

    return  0;
    }


return  isskewed;
}

                                        // Goes from linear to log, in case a log is too "strong" to transform right-skewed data
inline  double  LinToLog ( double x, double w )
{
                                        // Interpolate linearly from y=x to y=log(x)
                                        // w in [0..1]
//return  ( 1 - w ) *       x 
//      +       w   * log ( x );

                                        // Shift the log with a bias - data is assumed to be > 0
                                        // w > 0
return  log ( x + w );
//return  log ( x + w + 1e-10 );
}

                                        // Convergence search
double  TEasyStats::GetLinLogFactor ( double minll, double maxll )
{
if ( ! IsAllocated () )
    return  0;

                                        // work on duplicate
TEasyStats          logdata ( NumItems );

                                        // look at the shape of the left and right tails
//  diff        = fabs ( log ( sdleft ) - log ( sdright ) ) /*fabs ( deltalog - log ( 1.0 ) )* /;// look for the ratio closer to 1

//    DIFF        = fabs ( log ( sdleft ) - log ( sdright ) );
//    DIFF        = fabs ( sdleft - sdright ) / NonNull ( logdata.Quantile ( 0.50 ) );
//    DIFF        = RelativeDifference ( sdleft, sdright );

                                        // Quantile ranges are from after actual data
auto                GetTailsDiff    = [ & ] ( double ll )
{
logdata.Reset ();

for ( int i = 0; i < NumItems; i++ )
    if ( Data[ i ] + ll > 0 )
        logdata.Add ( LinToLog ( Data[ i ], ll ), ThreadSafetyIgnore );

double              sdleft          = logdata.Quantile ( 0.09 ) - logdata.Quantile ( 0.01 );
double              sdright         = logdata.Quantile ( 0.99 ) - logdata.Quantile ( 0.93 );

return  fabs ( sdleft - sdright ) / NonNull ( logdata.Quantile ( 0.50 ) );
};


double              diffc;
double              diffl;
double              diffr;
double              lintologfactor  = ( maxll + minll ) / 2;
double              step            = ( maxll - minll ) / 4;
double              convergence     = 1e-10;

                                        // get initial center value
                                        // Could also do a first round of exhaustive and coarse search, then use the best value for center and iterate from there
diffc   = GetTailsDiff ( lintologfactor );


do {
                                        // get left value
    diffl   = GetTailsDiff ( lintologfactor - step );

                                        // get right value
    diffr   = GetTailsDiff ( lintologfactor + step );


    if ( diffl < diffr && diffl < diffc ) {
        diffc           = diffl;
        lintologfactor -= step;
        }

    if ( diffr < diffl && diffr < diffc ) {
        diffc           = diffr;
        lintologfactor += step;
        }

    // else: stay at current position

                                        // getting closer...
    step   /= 2;


//    DBGV5 ( lb, sdleft * 1000, sdright * 1000, diff * 1000, logbase, "lb: sdleft / sdright, Diff -> BestLogBase" );

//    TFileName           _file;
//    StringCopy ( _file, "E:\\Data\\Data.LogBase " );
//    FloatToString ( StringEnd ( _file ), lb, 2 );
//    AddExtension ( _file, FILEEXT_EEGSEF );
//    logdata.WriteFileData ( _file );

    } while ( step > convergence );


return  lintologfactor;
}

                                        // !Needs to reinsert everything to have the stats right!
                                        // !Will remove all <= 0 data!
void    TEasyStats::ToLog ()
{
if ( ! IsAllocated () )
    return;

                                        // copy old data
TVector<float>          OrigData ( Data );


Reset ();

for ( int i = 0; i < (int) OrigData; i++ )
    if ( OrigData[ i ] > 0 )
        Add ( log ( OrigData[ i ] ), ThreadSafetyIgnore );
}

                                        // !Needs to reinsert everything to have the stats right!
void    TEasyStats::ToExp ()
{
if ( ! IsAllocated () )
    return;


                                        // copy old data
TVector<float>          OrigData ( Data );


Reset ();

for ( int i = 0; i < (int) OrigData; i++ )
    Add ( exp ( OrigData[ i ] ), ThreadSafetyIgnore );
}    

                                        // !Needs to reinsert everything to have the stats right!
void    TEasyStats::ToAntiLog ()
{
if ( ! IsAllocated () )
    return;


                                        // copy old data
TVector<float>          OrigData ( Data );
double                  maxdata         = Max ();


Reset ();

for ( int i = 0; i < (int) OrigData; i++ )
    Add ( - log ( maxdata + 1 - OrigData ( i ) ), ThreadSafetyIgnore );
}    


void    TEasyStats::ToAbsolute ()
{
if ( ! IsAllocated () )
    return;

OmpParallelFor

for ( int i = 0; i < NumItems; i++ )
    Data[ i ]   = fabs ( Data[ i ] );
}    


//----------------------------------------------------------------------------
                                        // Regular parametric flatness / pointiness
double  TEasyStats::Kurtosis ()
{
if ( ! ( IsAllocated () && NumItems >= 4 ) )
    return  0;


double              mean            = Average ();
double              den             = Square ( Variance () );
double              kurt            = 0;

OmpParallelForSum ( kurt )

for ( int i = 0; i < NumItems; i++ )
    kurt   += pow ( Data[ i ] - mean, 4 );

                                    // unbiased estimator
return  kurt / den
      * ( NumItems * ( NumItems + 1 ) ) / (double) ( ( NumItems - 1 ) * ( NumItems - 2 ) * ( NumItems - 3 ) )
      - 3 * Square ( (double) ( NumItems - 1 ) ) / (double) ( ( NumItems - 2 ) * ( NumItems - 3 ) );
}


void    TEasyStats::KurtosisAsym ( double center, double &kurtleft, double &kurtright )
{
kurtleft    = 0;
kurtright   = 0;

if ( ! ( IsAllocated () && NumItems >= 2 * 4 ) )
    return;


double              denleft;
double              denright;

VarianceAsym ( center, denleft, denright );

denleft         = Square ( denleft );
denright        = Square ( denright );


double              kl              = 0;
double              kr              = 0;
int                 nl              = 0;
int                 nr              = 0;

OmpParallelForSum ( kl, kr, nl, nr )

for ( int i = 0; i < NumItems; i++ )

    if ( Data[ i ] > center ) { kr += pow ( Data[ i ] - center, 4 ); nr++; }
    else                      { kl += pow ( Data[ i ] - center, 4 ); nl++; }

                                        // unbiased estimator
kurtleft    = nl < 4 ? 0 
                     :   kl / denleft
                       * ( nl * ( nl + 1 ) ) / (double) ( ( nl - 1 ) * ( nl - 2 ) * ( nl - 3 ) )
                       - 3 * Square ( (double) ( nl - 1 ) ) / (double) ( ( nl - 2 ) * ( nl - 3 ) );

kurtright   = nr < 4 ? 0 
                     :   kr / denright
                       * ( nr * ( nr + 1 ) ) / (double) ( ( nr - 1 ) * ( nr - 2 ) * ( nr - 3 ) )
                       - 3 * Square ( (double) ( nr - 1 ) ) / (double) ( ( nr - 2 ) * ( nr - 3 ) );
}


double  TEasyStats::RobustKurtosis ()
{
if ( ! ( IsAllocated () && NumItems >= 4 ) )
    return  0;


double              mean            = MaxModeHRM (); // MaxModeHistogram (); // Median ( false );
double              den             = pow ( MAD ( mean ), 4 );
double              kurt            = 0;


OmpParallelForSum ( kurt )

for ( int i = 0; i < NumItems; i++ )
    kurt   += pow ( Data[ i ] - mean, 4 );

                                    // ?is normalization by sqrt ( NumItems ) correct?
return  ( kurt / den
      * ( NumItems * ( NumItems + 1 ) ) / (double) ( ( NumItems - 1 ) * ( NumItems - 2 ) * ( NumItems - 3 ) )
      - 3 * Square ( (double) ( NumItems - 1 ) ) / (double) ( ( NumItems - 2 ) * ( NumItems - 3 ) ) ) / sqrt ( (double) NumItems );
}


double  TEasyStats::RobustKurtosisOctiles ()
{
if ( ! ( IsAllocated () && NumItems >= 4 ) )
    return  0;


double              o75             = Quantile ( 7 / 8.0 ) - Quantile ( 5 / 8.0 );
double              o31             = Quantile ( 3 / 8.0 ) - Quantile ( 1 / 8.0 );
double              o62             = Quantile ( 6 / 8.0 ) - Quantile ( 2 / 8.0 );

                                        // centered to normal distribution
return  ( o75 + o31 ) / NonNull ( o62 ) - 1.23;
}
                                        // Using a ratio of asymmetrical upper and lower averages
                                        // better for heavy tails
double  TEasyStats::RobustKurtosisHogg ()
{
if ( ! ( IsAllocated () && NumItems >= 4 ) )
    return  0;


#define             HoggKurtosisAlpha   0.05
#define             HoggKurtosisBeta    0.50
double              la              = TruncatedMean ( 0.00,                     HoggKurtosisAlpha );
double              ua              = TruncatedMean ( 1.00 - HoggKurtosisAlpha, 1.00              );
double              lb              = TruncatedMean ( 0.00,                     HoggKurtosisBeta  );
double              ub              = TruncatedMean ( 1.00 - HoggKurtosisBeta,  1.00              );

                                        // centered to normal distribution
return  ( ua - la ) / NonNull ( ub - lb ) - 2.59;
}

                                        // Crow Siddiqui
double  TEasyStats::RobustKurtosisCS ()
{
if ( ! ( IsAllocated () && NumItems >= 4 ) )
    return  0;


#define             CSKurtosisAlpha     0.025
#define             CSKurtosisBeta      0.250
double              oa              = Quantile ( 1 - CSKurtosisAlpha ) + Quantile ( CSKurtosisAlpha );
double              ob              = Quantile ( 1 - CSKurtosisBeta  ) - Quantile ( CSKurtosisBeta  );

                                        // centered to normal distribution
return  oa / NonNull ( ob ) - 2.91;
}


//----------------------------------------------------------------------------
                                        // Get the optimal "sampling" across data
                                        // This is like a radius for the Gaussian (SqrtTwo * SD)
double  TEasyStats::GaussianKernelDensity ( KernelDensityType kdetype, int numgaussians )
{
if ( ! NumItems )
    return  0;


if ( kdetype == KernelDensitySilvermanRobust && IsNotAllocated () )
    kdetype = KernelDensitySilverman;

if ( kdetype == KernelDensityMultipleGaussians && ( IsNotAllocated () || numgaussians <= 1 ) )
    kdetype = KernelDensitySilverman;


double              density         = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // case where data are of discrete values, like indexes
if      ( kdetype == KernelDensityDiscrete ) {

    Sort ();

                                        // count # of unique values
    int                 numunique       = 0;

    OmpParallelForSum ( numunique )

    for ( int i = 0; i < NumItems; i++ )
        if ( !i || Data[ i ] != Data[ i - 1 ] )
            numunique++;

                                        // not enough unique values?
    if ( numunique < 2 )
        return  0;

                                        // transfer only the unique values to a temp stat
    TVector<double>         unique ( numunique );

    for ( int i = 0, j = 0; i < NumItems; i++ )
        if ( !i || Data[ i ] != Data[ i - 1 ] )
            unique[ j++ ]   = Data[ i ];


                                        // convert to deltas (needs at least 2 unique values)
    for ( int i = 1; i < numunique; i++ )
        unique[ i - 1 ] -= unique[ i ];

    unique[ numunique - 1 ] = DBL_MAX;// to neutralize the last element

                                        // get the smallest step on the data grid
    unique.Sort ( Ascending );


    density     = unique [ 0 ];         // will NOT be 0, as we took off repeating values

//    DBGV4 ( NumItems, GaussianKernelDensity ( kdetype ), numunique, density, "#items KernelDensity -> #uniquevalues KernelDensity" );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( kdetype == KernelDensitySilverman 
       || kdetype == KernelDensitySilvermanRobust ) {

    double              sigma           = kdetype == KernelDensitySilvermanRobust   ? min ( SD (), InterQuartileRange () )
                                                                                    :       SD ();

                                        // Assuming data have a Gaussian distribution, compute gaussian Kernel size
    density     = PowerRoot ( 4.0 / 3.0 * Power ( sigma, 5 ) / NumItems, 5.0 );   // Gaussian Kernel density estimation, with the new scaling
//  density     = 1.06 * sigma * Power ( NumItems, -1 / 5.0 );                    // simplified estimator

                                        // special case if data are quantized to integer
    if ( IsInteger () )
                                        // also make the kernel integer, otherwise, there will be some aliasing effect across bins
        density = Round ( density );

//  DBGV6 ( NumItems, IsInteger (), SD (), InterQuartileRange (), sigma, density, "NumItems, IsInteger (), SD (), InterQuartileRange (), sigma, density" );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // See if we expect n Gaussians how the KD behaves
else if ( kdetype == KernelDensityMultipleGaussians ) {

    TEasyStats          splitstats ( NumItems * 2 / ( numgaussians + 1 ) );
    TEasyStats          kdstats;
    double              mindata         = Quantile ( 0.001 );   // skip super-extreme border values?
    double              maxdata         = Quantile ( 0.999 );


    for ( int wi = 0; wi < numgaussians; wi++ ) {
                                        // overlapping intervals
        double          w1          =   wi       / (double) ( numgaussians + 1 );
        double          w2          = ( wi + 2 ) / (double) ( numgaussians + 1 );
                                        // either cutting through range
        float           lowcut      = ( 1 - w1 ) * mindata + w1 * maxdata;
        float           highcut     = ( 1 - w2 ) * mindata + w2 * maxdata;
                                        // or cutting through CDF
//        float           lowcut      = Quantile ( w1 );    // ?too small intervals?
//        float           highcut     = Quantile ( w2 );


        splitstats.Reset ();

        for ( int i = 0; i < NumItems; i++ )

            if ( Data[ i ] != 0 && IsInsideLimits ( Data[ i ], lowcut, highcut ) )  

                splitstats.Add ( Data[ i ], ThreadSafetyIgnore );


        kdstats.Add ( splitstats.GaussianKernelDensity ( KernelDensitySilverman ), ThreadSafetyIgnore );
        }


//    kdstats.Show ("KD Stats");

    density     = kdstats.Min () > 0 ? kdstats.Min () : kdstats.Mean ();
//    density     = kdstats.Mean ();

                                        // special case if data are quantized to integer
    if ( IsInteger () )
                                        // also make the kernel integer, otherwise, there will be some aliasing effect across bins
        density = Round ( density );

    
//  DBGV4 ( mindata, maxdata, numgaussians, density, "KDMultiGauss: mindata maxdata #Gauss -> KD" );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // returning no-nonsense value - 0 can happen if all data are the same values, hence SD is 0
                                        // but returning a density of 0 is likely to crash any histogram initialization, although it doesn't really matter in the end
if ( density <= 0 )     density = 1;

return  density;
}


//----------------------------------------------------------------------------
                                        // This is an estimation of the mode of a discrete set of continuous data
                                        // It is done by estimating the probability density, then taking the max of that function
                                        // The probability density is itself estimated by convolving each spike of data with a properly chosen Gaussian (Kernel density estimation)
                                        // EPDFM (Empirical Probability Density Function Mode): using a specific density function
double  TEasyStats::MaxModeHistogram ( THistogram* h )
{
if ( ! ( IsAllocated () && NumItems ) )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Degenerate case: all constant values
if ( Data == Data[ 0 ] )
                                        // then mode is this value, no need to mingle with complex histogram
    return  Data[ 0 ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              kerneldensity;


//if ( EPDFM ) {
//
//    double          sigma       = min ( SD (), InterQuartileRange () /*MAD ()*/ );
//                                        // usually much smaller than GaussianKernelDensity
//                                        // which can make it converge to local max instead of global max
//    kerneldensity   = Power ( 0.9 * sigma / NumItems, 1.0 / 5.0 );
//    }
//else
                                        // regular density kernel
    kerneldensity   = GaussianKernelDensity ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

THistogram          H;


H.ComputeHistogram  (   *this,
                        kerneldensity,      0,                  0, 
                        6,                  6,  // increasing the resolution will also improve a tiny bit the accuracy, maybe at the expense of more computation(?)
                                                         // We will handle the smoothing ourselves
                        (HistogramOptions) ( HistogramPDF /*| HistogramSmooth*/ | HistogramNormNone | HistogramLinear )
                    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Smoothing improves things, the amount of smoothing depends on the number of modes, though
                                        // The Kernel Density should be taking care of that, but it currently doesn't estimate the number of underlying modes
FctParams           p;

                                        // Filtering: 1 Mode -> 8 KS; 2 Modes -> 4 KS
p ( FilterParamDiameter )     = 4 * H.GetKernelSubsampling () + 1;


H.Filter ( FilterTypeGaussian, p, false );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Mode is the max of the density function (most probable value)
                                        // For safety reasons, clip the answer to the actual data range (KD might not be perfect and filtering could leak the data outside of range)
double              maxmodeposition = Clip ( H.GetMaxPosition ( RealUnit ), Min (), Max () );

                                        // in case caller wants to have a peek at the histogram...
if ( h )
    *h   = H;


return  maxmodeposition;
}


//----------------------------------------------------------------------------
                                        // Half Sample Mode
                                        // Through iteratively reducing an interval length, pick the sub-interval where data vary less (~more of them)
                                        // Bickel, Frühwirth "On a fast, robust estimator of the mode"
double  TEasyStats::MaxModeHSM ()
{
if ( ! ( IsAllocated () && NumItems ) )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Degenerate case: all constant values
if ( Data == Data[ 0 ] )
                                        // then mode is this value, no need to mingle with complex histogram
    return  Data[ 0 ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Sort is descending
Sort ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // this works with any size, including non-powers of 2
int                 N               = RoundAbove ( NumItems / 2.0 );
int                 istart          = 0;
int                 iend            = NumItems - N;
double              w;
double              wmin;
int                 imin            = 0;

                                        // loop while the search interval is big enough
while ( N > 3 ) {
                                        // start scan from previous interval
    wmin    = DBL_MAX;
    imin    = istart;

    for ( int i = istart; i <= iend; i++ ) {
                                        // get the data width for N consecutive samples
        w   = Data[ i ] - Data[ i + N - 1 ];
                                        // found the position where data spread is minimal (i.e. higher density)?
        if ( w < wmin ) {
            wmin    = w;
            imin    = i;                // remember this position
            }

        } // for all intervals

//    DBGV4 ( N, w, imin, Data[ imin + N / 2 ], "N, w -> imin Data" );

                                        // reduce search interval - note that with this formula, N will always be >= 2
    N       = RoundAbove ( N / 2.0 );
    istart  = imin;
    iend    = NoMore ( NumItems - N, istart + 2 * N );  // !For single mode, turning this off provides much better results - But not for multiple modes however!

    } // while N > 3


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Mode is the max of the density function (most probable value)
double              maxmodeposition;


if        ( N == 1 )    maxmodeposition     =   Data[ imin ];
else   if ( N == 2 )    maxmodeposition     = ( Data[ imin ] + Data[ imin + 1 ] ) / 2;
else /*if ( N == 3 )*/  maxmodeposition     = fabs ( Data[ imin ] - Data[ imin + 1 ] )     < fabs ( Data[ imin + 1 ] - Data[ imin + 2 ] )
                                            ?      ( Data[ imin ] + Data[ imin + 1 ] ) / 2 :      ( Data[ imin + 1 ] + Data[ imin + 2 ] ) / 2;

//DBGV ( maxmodeposition, "MaxModeHSM" );

return  maxmodeposition;
}


//----------------------------------------------------------------------------
                                        // Half Range Mode
                                        // Through iteratively reducing an interval length, pick the sub-interval where data vary less (~more of them)
                                        // Bickel, Frühwirth "On a fast, robust estimator of the mode"
double  TEasyStats::MaxModeHRM ()
{
if ( ! ( IsAllocated () && NumItems ) )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Degenerate case: all constant values
if ( Data == Data[ 0 ] )
                                        // then mode is this value, no need to mingle with complex histogram
    return  Data[ 0 ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Sort is descending
Sort ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // this works with any size, including non-powers of 2
int                 istart          = 0;
int                 iend            = NumItems - 1;
double              W               = ( Data[ istart ] - Data[ iend ] ) / 2;
int                 imin            = istart;
int                 imax            = iend;
int                 density         = 1;
int                 densitymax;
int                 jmin;
int                 substep         = 50;
TVector<float>      densityarray ( 2 * substep );
TArray1<int>        imaxarray    ( 2 * substep );
int                 stepi;

                                        // loop while the search interval is big enough
do {
                                        // start scan from previous interval
    densityarray    = 0;
    imaxarray       = 0;
    stepi           = AtLeast ( 1, ( iend - istart + 1 ) / substep );  // we can step faster at beginning


    for ( int i = istart, istep = 0; i <= iend; i+=stepi, istep++ ) {

        density     = 0;
                                        // this loop, however, is not downsampled
        for ( jmin = i; jmin < NumItems; jmin++ )

            if ( Data[ jmin ] >= Data[ i ] - W )    density++;
            else                                    break;

                                        // store in downsampled array
        densityarray[ istep ]  = density;
        imaxarray   [ istep ]  = jmin - 1;

                                        // reached last i position (not enough data for interval)?
        if ( jmin == NumItems )
            break;
        } // for all intervals


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // constant size in downsampled array -> heavier smoothing at beginning
    densityarray.Filter ( FilterTypeGaussian, 5 );

                                        // now search for max density within smoothed curve
    densitymax      = 0;

    for ( int istep = 0; istep < (int) densityarray; istep++ ) {
                                        // best highest density?
        if ( densityarray[ istep ] > densitymax ) {
            densitymax  = densityarray[ istep ];
            imin        = istart      + istep * stepi;
            imax        = imaxarray   [ istep ];
            }
        } // for all intervals


//    if ( VkQuery () ) {
//    densityarray.WriteFile ( "E:\\Data\\Density.sef" );
//    DBGV5 ( W, imin, imax, ( imin + imax ) / 2, Data[ ( imin + imax ) / 2 ], "W -> imin..imax icentral -> Data" );
//    }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // reduce new search interval
    istart  = imin;
    iend    = imax;
                                        // new half range
    W       = ( Data[ imin ] - Data[ imax ] ) / 2;
                             // W (width of data) can be 0 if data are "stuck in a plateau" - this is OK though, this is our mode
    } while ( densitymax > 2 && W != 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Mode is the max of the density function (most probable value)
double              maxmodeposition;


if        ( density == 1 )    maxmodeposition     =   Data[ imin ];
else /*if ( density == 2 )*/  maxmodeposition     = ( Data[ imin ] + Data[ imax ] ) / 2;

//DBGV ( maxmodeposition, "MaxModeHRM" );

return  maxmodeposition;
}


//----------------------------------------------------------------------------
                                        // Running the most reliables Max Mode estimates
                                        // !statcenter is NOT reset NOR resized, in case user wants to cumulate results!
                                        // Also statcenter is not required to be robust / allocated
void    TEasyStats::MaxModeRobust ( TEasyStats& statcenter )
{
//statcenter.Resize ( AtLeast ( MaxSize (), NumMaxModeRobustEstimates ) );

if ( ! ( IsAllocated () && NumItems ) ) {
//                                      // returning something?
//  statcenter.Add ( 0, ThreadSafetyIgnore );

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Degenerate case: all constant values
if ( Data == Data[ 0 ] ) {
                                        // then mode is this value, no need to mingle with complex histogram
    statcenter.Add ( Data[ 0 ] );

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//#if defined (_DEBUG)
//double              c0      = Median ();            // lame estimator, but fast
//
//statcenter.Add ( c0 );
//
//#else
                                        // Bunch of estimates for center
double              c1      = MaxModeHistogram ();  // too much on the right
double              c2      = MaxModeHSM ();        // better for 1 mode(?)
double              c3      = MaxModeHRM ();        // not bad, but a bit on the left
double              c4      = FirstMode ( 0.50 );   // quite the best for positive data


statcenter.Add ( c1 );
statcenter.Add ( c2 );
statcenter.Add ( c3 );
                                        // not using this estimator if null
if ( c4 )   statcenter.Add ( c4 );

//#endif

//if ( VkQuery () ) statcenter.Show ( "TEasyStats::MaxModeRobust" );
}


//----------------------------------------------------------------------------
double  TEasyStats::LastMode ( double noisethreshold )
{
if ( ! ( IsAllocated () && NumItems ) )
    return  0;

Clipped ( noisethreshold, 0.0, 1.0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

THistogram          H;


H.ComputeHistogram  (   *this,
                        0,      0,      0, 
                        6,      6, 
                        (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramNormNone | HistogramLinear )
                    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // skip noise value
double              minv            = H.GetMaxValue () * noisethreshold;


for ( int i = (int) H - 2; i >= 1; i-- )
                                        // local max above noise?
    if ( H[ i ] > minv
      && H[ i ] > H[ i - 1 ]
      && H[ i ] > H[ i + 1 ] )

        return  H.ToReal ( RealUnit, i );


return  0; // H.ToReal ( RealUnit, 0 );
}


//----------------------------------------------------------------------------
double  TEasyStats::FirstMode ( double noisethreshold )
{
if ( ! ( IsAllocated () && NumItems ) )
    return  0;

Clipped ( noisethreshold, 0.0, 1.0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

THistogram          H;


H.ComputeHistogram  (   *this,
                        0,      0,      0, 
                        6,      6, 
                        (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramNormNone | HistogramLinear )
                    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // skip noise value
double              minv            = H.GetMaxValue () * noisethreshold;


for ( int i = 1; i < (int) H - 1; i++ )
                                        // local max above noise?
    if ( H[ i ] > minv
      && H[ i ] > H[ i - 1 ]
      && H[ i ] > H[ i + 1 ] )

        return  H.ToReal ( RealUnit, i );


return  0; // H.ToReal ( RealUnit, 0 );
}


//----------------------------------------------------------------------------
                                        // Rousseeuw and Croux Sn for Robust Standard Deviation
                                        // Taken from their original Fortran source code (better than in Wikipedia!): http://ftp.uni-bayreuth.de/math/statlib/general/snqn
                                        // Quoting the code:
                                        // "
                                        // Choosing between Sn and Qn depends on the situation. On the
                                        // one hand, Qn is more efficient than Sn. On the other hand,
                                        // Sn is more robust in the sense of a smaller bias and a lower
                                        // gross-error sensitivity. As a default choice, we recommend
                                        // using the estimator Sn.
                                        // "
double  TEasyStats::Sn ( int maxitems )
{
if ( ! ( IsAllocated () && NumItems >= 2 ) )
    return  0;


Maxed ( maxitems, 10 );
                                        // downsample data, this stuff can explode (numerically speaking)!
int                 step            = AtLeast ( 1, Round ( NumItems / ( (double) maxitems / 1.5 ) ) );
int                 numitems        = ( NumItems + step - 1 ) / step;


TEasyStats          deltai ( numitems     );
TEasyStats          deltaj ( numitems - 1 );


for ( int i = 0; i < NumItems; i += step ) {

    deltaj.Reset ();

    for ( int j = 0; j < NumItems; j += step )
        if ( j != i )
            deltaj.Add ( fabs ( Data[ i ] - Data[ j ] ), ThreadSafetyIgnore );
        

    deltai.Add ( deltaj.Median ( false ), ThreadSafetyIgnore );
    }

                                        // correction factor, depending on the sample size
double              csn             = NumItems >= 10 ? IsOdd ( numitems ) ? numitems / (double) ( numitems - 0.9 ) 
                                                                          : 1
                                    : NumItems == 1  ? 1    // doesn't happen, just for the sake of code consistency
                                    : NumItems == 2  ? 0.743
                                    : NumItems == 3  ? 1.851
                                    : NumItems == 4  ? 0.954
                                    : NumItems == 5  ? 1.351
                                    : NumItems == 6  ? 0.993
                                    : NumItems == 7  ? 1.198
                                    : NumItems == 8  ? 1.005
                                    : NumItems == 9  ? 1.131
                                    :                  1;   // doesn't happen, just for the sake of code consistency

                                        // final formula
double              Sn              = csn * 1.1926 * deltai.Median ( false );

return  Sn;
}


double  TEasyStats::Qn ( int maxitems )
{
if ( ! ( IsAllocated () && NumItems >= 2 ) )
    return  0;


Maxed ( maxitems, 10 );
                                        // downsample data, this stuff can explode (numerically speaking)!
int                 step            = AtLeast ( 1, Round ( NumItems / ( (double) maxitems / 1.5 ) ) );
int                 numitems        = ( NumItems + step - 1 ) / step;


TEasyStats          delta ( numitems * ( numitems - 1 ) / 2 );

                                        // all pairwise distances
for ( int i = 0;     i < NumItems; i += step ) 
for ( int j = i + 1; j < NumItems; j += step )

    delta.Add ( fabs ( Data[ i ] - Data[ j ] ), ThreadSafetyIgnore );
        

                                        // correction factor, depending on the sample size
double              cqn             = NumItems >= 10 ? IsOdd ( numitems ) ? numitems / (double) ( numitems + 1.4 ) 
                                                                          : numitems / (double) ( numitems + 3.8 ) 
                                    : NumItems == 1  ? 1    // doesn't happen, just for the sake of code consistency
                                    : NumItems == 2  ? 0.399
                                    : NumItems == 3  ? 0.994
                                    : NumItems == 4  ? 0.512
                                    : NumItems == 5  ? 0.844
                                    : NumItems == 6  ? 0.611
                                    : NumItems == 7  ? 0.857
                                    : NumItems == 8  ? 0.669
                                    : NumItems == 9  ? 0.872
                                    :                  1;   // doesn't happen, just for the sake of code consistency

int                 h               = numitems / 2 + 1;
int                 k               = ( h * ( h - 1 ) ) / 2;
                                        // order of the sort is Descending here, we need to invert the index
delta.Sort ();

                                        // final formula
double              Qn              = cqn * 2.2219 * delta[ delta.NumItems - k ];

return  Qn;
}


//----------------------------------------------------------------------------
                                        // EM Algorithm
double  TEasyStats::GetGaussianMixture  (   int                 numgaussian,    
                                            double              requestedprecision, GaussianMixtureOptions      options,
                                            TTracks<double>&    mixg,
                                            TTracks<double>*    gaussmix
                                        )
{
mixg.Resize ( numgaussian, NumFunctionParams );

if ( ! ( IsAllocated () && NumItems > 0 && numgaussian > 0 ) )
    return  -1;

bool                dofaster        = IsFlag ( options, GaussianMixtureFaster );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TVector<double>     alpha   ( numgaussian );            // mixture weights, global probability for each component
TVector<double>     mean    ( numgaussian );            // mean  of each Gaussian
TVector<double>     sigma   ( numgaussian );            // sigma of each Gaussian
TMaps               weight  ( NumItems, numgaussian );  // membership weights of each data point to each Gaussian


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Initialization values
double              firstpos        = Quantile ( 0.001 );
double              lastpos         = Quantile ( 0.999 );
double              deltapos        = lastpos - firstpos;


                                        // Guessing mean and width
for ( int k = 0; k < numgaussian; k++ ) {
                                        // Equally spaced centers across data range
    mean ( k )  = firstpos + ( k + 0.5 ) * deltapos / numgaussian;
                                        // Equally spreads
    sigma( k )  = deltapos / numgaussian * 0.25;
                                        // Equally weighted Gaussians
    alpha( k )  = 1;
                                        // Increasingly weighted Gaussians - fine for our currrent use Grey / White levels
//  alpha( k )  = AtLeast ( 1e-3, ( k + 1 ) / (double) numgaussian );
    }

alpha.NormalizeArea ();

                                        // Estimating alpha with a M step?


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Global loop
#define             MaxGaussiantMixtureIter     250
double              sump;
double              sumlogsump      = 0;
double              oldsumlogsump;
double              Nk;
double              alphak;
double              meank;
double              vark;
double              sigmak;


//TExportTracks       experror;
//StringCopy           ( experror.Filename, "E:\\Data\\Stat.EMGaussianMixture.Error.sef" );
//experror.SetAtomType ( AtomTypeScalar );
//experror.NumTracks           = 1 + 3 * numgaussian;
//experror.NumTime             = MaxGaussiantMixtureIter;



for ( int iter = 1; iter <= MaxGaussiantMixtureIter; iter++ ) {

//    for ( int k = 0; k < numgaussian; k++ )
//        DBGV5 ( iter, k+1, mean [ k ], sigma [ k ], alpha [ k ], "#iter k -> mean width alpha" );


//    experror.TimeMax++;  // used as a counter here
//    experror.Write ( sumlogsump );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // (E)xpectation step
    OmpParallelFor

    for ( int i = 0; i < NumItems; i++ ) {

        for ( int k = 0; k < numgaussian; k++ )
                                        // membership weight with Normal distribution (could be anything else BTW)
            weight ( i, k ) = alpha ( k ) * Normal ( Data[ i ], mean [ k ], sigma[ k ] );

                                        // Sum of weights across all Gaussians must be 1
        weight[ i ].NormalizeArea ();
        } // for i


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // (M)aximization step
    for ( int k = 0; k < numgaussian; k++ ) {

//        experror.Write ( alpha( k ) );
//        experror.Write ( mean ( k ) );
//        experror.Write ( sigma( k ) );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Nk      = 0;                    // "Number" of points assigned to Gaussian k (with soft assignation)
        meank   = 0;                    // We can compute the new mean at the same time

        OmpParallelForSum ( Nk, meank )

        for ( int i = 0; i < NumItems; i++ ) {
                                        // How much this Gaussian actually "weights" in total
            Nk     += weight ( i, k );
                                        // New mean
            meank  += weight ( i, k ) * Data[ i ];
            }

                                        // New estimate of mixture weight: average of its weights
        alphak      = Nk / NumItems;
                                        // New estimate for center of Gaussian is weighted mean
        meank      /= NonNull ( Nk );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optionally stepping faster                                        
//      double          faster      = 1;    // back to default method
//      double          faster      = 1.7;  // globally good / optimal results, but first 20 iterations are chaotics
        double          faster;


        if ( dofaster ) {
                                        // smaller steps first, then increasing as convergence is getting slower and slower
            faster      = LinearRescale ( iter, 1, 20, 1.0, 2.0, true );
//          faster      = LinearRescale ( Log ( iter ), Log ( 1 ), Log ( 10 ), 1.0, 2.0, true );

                                        // new variables, with extra step
            alpha( k ) += ( alphak - alpha( k ) ) * faster;

            mean ( k ) += ( meank  - mean ( k ) ) * faster;

                                        // Safety test, in case stepping was too big
            if ( ! ( alpha ( k ) > 0
                     && IsInsideLimits ( mean ( k ), Min (), Max () ) ) ) {
                                        // Skipping faster step for all variables
                faster      = 1;
                alpha ( k ) = alphak;
                mean  ( k ) = meank;
                }

            } // if dofaster
        else {

            alpha ( k ) = alphak;

            mean  ( k ) = meank;
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // New estimate for covariance
        vark    = 0;

        OmpParallelForSum ( vark )

        for ( int i = 0; i < NumItems; i++ )

            vark   += weight ( i, k ) * Square ( Data[ i ] - mean ( k ) );

                                        // Weighted SD
        sigmak      = sqrt ( vark / NonNull ( Nk ) );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optionally stepping faster                                        
        if ( dofaster && faster != 1 ) {

                                        // new variable, with extra step
            sigma ( k )+= ( sigmak - sigma ( k ) ) * faster;


            if ( sigma ( k ) <= 0 ) {
                                        // Skipping faster step for all variables
//              faster      = 1;
                alpha ( k ) = alphak;
                mean  ( k ) = meank;
                sigma ( k ) = sigmak;   // !Note that this value is slightly wrong, SD should be recomputed from original Mean!
                }

            } // if dofaster
        else
            sigma ( k ) = sigmak;

        } // for k


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // Saving intermediate mixture
   if ( iter % 10 == 0 ) {
    THistogram          H ( *this, 0, 0, 0, 3, 3, (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramNormArea | HistogramLinear ) );

    TArray2<double>     gaussmix ( numgaussian + 2, (int) H );

                                            // we can copy any existing scaling from original track...
    gaussmix.Index2     = H.Index1;


    double              sumg;
    double              v;

    for ( int i = 0; i < (int) H; i++ ) {

        sumg        = 0;

        for ( int k = 0; k < numgaussian; k++ ) {

            v                   = alpha ( k ) * Normal ( H.ToReal ( RealUnit, i ), mean [ k ], sigma[ k ] );

            gaussmix ( k, i )   = v;

            sumg               += v;
//          sumg               += Square ( v );
            }


        gaussmix ( numgaussian    , i ) = sumg;
//      gaussmix ( numgaussian    , i ) = sqrt ( sumg );    // more beautiful, but less exact?
        gaussmix ( numgaussian + 1, i ) = H[ i ];
        }


    TExportTracks       expfile;
    sprintf ( expfile.Filename, "E:\\Data\\Stat.EMGaussianMixture.%02d.sef", iter );
    expfile.SetAtomType ( AtomTypeScalar );
    expfile.NumTracks           = gaussmix.GetDim1 ();
    expfile.NumTime             = gaussmix.GetDim2 ();
    expfile.Write ( gaussmix, true );

    expfile.Filename.Open ();
    }
*/
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute new Log-Likelihood
    oldsumlogsump   = sumlogsump;

    sumlogsump      = 0;


    OmpParallelForSum ( sumlogsump )

    for ( int i = 0; i < NumItems; i++ ) {

        double      sump    = 0;

        for ( int k = 0; k < numgaussian; k++ )

            sump   += alpha ( k ) * Normal ( Data[ i ], mean [ k ], sigma[ k ] );

        sumlogsump += Log ( sump );
        }

                                        // Break when reaching convergence
    if ( RelativeDifference ( sumlogsump, oldsumlogsump ) < requestedprecision )
        break;

    } // main loop


//experror.NumTime     = experror.TimeMax;
//experror.WriteHeader ( true );
//experror.End ();
//experror.Filename.Open ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Transfer final results
for ( int k = 0; k < numgaussian; k++ ) {

    mixg ( k, FunctionCenter    )   = mean  [ k ];
    mixg ( k, FunctionWidth     )   = GaussianSigmaToWidth ( sigma [ k ] );
    mixg ( k, FunctionHeight    )   = alpha [ k ];

//    DBGV4 ( k+1, mean [ k ], sigma [ k ], alpha [ k ], "results: k -> mean width alpha" );
    } // for k


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Returning each Gaussian in a track
if ( gaussmix ) {

    THistogram          H ( *this, 0, 0, 0, 3, 3, (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramNormArea | HistogramLinear ) );

    gaussmix->Resize ( numgaussian + 2, (int) H );
                                                // we can copy any existing scaling from original track...
    gaussmix->Index2    = H.Index1;


    double              area            = 0;
    double              sumg;
    double              v;

    for ( int i = 0; i < (int) H; i++ ) {

        sumg        = 0;

        for ( int k = 0; k < numgaussian; k++ ) {

            v                       = alpha ( k ) * Normal ( H.ToReal ( RealUnit, i ), mean [ k ], sigma[ k ] );

            (*gaussmix) ( k, i )    = v;

            sumg                   += v;
//          sumg                   += Square ( v );
            }

        area       += sumg;

        (*gaussmix) ( numgaussian    , i )  = sumg;
//      (*gaussmix) ( numgaussian    , i )  = sqrt ( sumg );    // more beautiful, but less exact?
        (*gaussmix) ( numgaussian + 1, i )  = H[ i ];
        }

                                        // manually rescaling the mixture so its area is 1 - same rescaling for the individual Gaussians, too
    for ( int i = 0; i < (int) H; i++ ) 
    for ( int k = 0; k < numgaussian + 1; k++ )

            (*gaussmix) ( k, i )   /= area;

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // returning an average value
return  sumlogsump / NumItems;
}


//----------------------------------------------------------------------------
/*                                        // Kind of working, but needs to be improved
double  TEasyStats::GetAsymGaussianMixture  (   int                 numgaussian,    
                                            double              requestedprecision, GaussianMixtureOptions      options,
                                            TTracks<double>&    mixg,
                                            TTracks<double>*    gaussmix
                                        )
{
mixg.Resize ( numgaussian, NumFunctionParams );

if ( ! ( IsAllocated () && NumItems > 0 && numgaussian > 0 ) )
    return  -1;

bool                dofaster        = IsFlag ( options, GaussianMixtureFaster );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TVector<double>     alpha   ( numgaussian );            // mixture weights, global probability for each component
TVector<double>     mean    ( numgaussian );            // mean  of each Gaussian
TVector<double>     sigmal  ( numgaussian );            // sigma left  of each Gaussian
TVector<double>     sigmar  ( numgaussian );            // sigma right of each Gaussian
TMaps               weight  ( NumItems, numgaussian );  // membership weights of each data point to each Gaussian


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Initialization values
double              firstpos        = Quantile ( 0.001 );
double              lastpos         = Quantile ( 0.999 );
double              deltapos        = lastpos - firstpos;


                                        // Guessing mean and width
for ( int k = 0; k < numgaussian; k++ ) {
                                        // Equally spaced centers across data range
    mean ( k )  = firstpos + ( k + 0.5 ) * deltapos / numgaussian;
                                        // Equally spreads
    sigmal( k ) = 
    sigmar( k ) = deltapos / numgaussian * 0.25;
                                        // Equally weighted Gaussians
    alpha( k )  = 1;
                                        // Increasingly weighted Gaussians - fine for our currrent use Grey / White levels
//  alpha( k )  = AtLeast ( 1e-3, ( k + 1 ) / (double) numgaussian );
    }

alpha.NormalizeArea ();

                                        // Estimating alpha with a M step?


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Global loop
#define             MaxGaussiantMixtureIter     250
double              sump;
double              sumlogsump      = 0;
double              oldsumlogsump;
double              Nk;
double              Nkl;
double              Nkr;
double              alphak;
double              meank;
double              varkl;
double              varkr;
double              sigmakl;
double              sigmakr;


//TExportTracks       experror;
//StringCopy           ( experror.Filename, "E:\\Data\\Stat.EMGaussianMixture.Error.sef" );
//experror.SetAtomType ( AtomTypeScalar );
//experror.NumTracks           = 1 + 4 * numgaussian;
//experror.NumTime             = MaxGaussiantMixtureIter;



for ( int iter = 1; iter <= MaxGaussiantMixtureIter; iter++ ) {

//    for ( int k = 0; k < numgaussian; k++ )
//        DBGV6 ( iter, k+1, mean [ k ], sigmal[ k ], sigmar[ k ], alpha [ k ], "#iter k -> mean width alpha" );


//    experror.TimeMax++;  // used as a counter here
//    experror.Write ( sumlogsump );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // (E)xpectation step
    OmpParallelFor

    for ( int i = 0; i < NumItems; i++ ) {

        for ( int k = 0; k < numgaussian; k++ )
                                        // membership weight with Normal distribution (could be anything else BTW)
            weight ( i, k ) = alpha ( k ) * AsymmetricalNormal ( Data[ i ], mean [ k ], sigmal[ k ], sigmar[ k ] );
//            weight ( i, k ) = alpha ( k ) * Normal ( Data[ i ], mean [ k ], ( sigmal[ k ] + sigmar[ k ] ) / 2 );

                                        // Sum of weights across all Gaussians must be 1
        weight[ i ].NormalizeArea ();
        } // for i


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // (M)aximization step
    for ( int k = 0; k < numgaussian; k++ ) {

//        experror.Write ( alpha ( k ) );
//        experror.Write ( mean  ( k ) );
//        experror.Write ( sigmal( k ) );
//        experror.Write ( sigmar( k ) );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Nk      = 0;                    // "Number" of points assigned to Gaussian k (with soft assignation)
        Nkl     = 0;                    // "Number" of points assigned to Gaussian k (with soft assignation)
        Nkr     = 0;                    // "Number" of points assigned to Gaussian k (with soft assignation)
        meank   = 0;                    // We can compute the new mean at the same time

        OmpParallelForSum ( Nk, meank )

        for ( int i = 0; i < NumItems; i++ ) {
                                        // How much this Gaussian actually "weights" in total
            Nk     += weight ( i, k );

            if ( Data[ i ] < mean ( k ) )   Nkl    += weight ( i, k );
            else                            Nkr    += weight ( i, k );

                                        // New mean
            meank  += weight ( i, k ) * Data[ i ];
            }

                                        // New estimate of mixture weight: average of its weights
        alphak      = Nk / NumItems;
                                        // New estimate for center of Gaussian is weighted mean
        meank      /= NonNull ( Nk );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optionally stepping faster                                        
//      double          faster      = 1;    // back to default method
//      double          faster      = 1.7;  // globally good / optimal results, but first 20 iterations are chaotics
        double          faster;


        if ( dofaster ) {
                                        // smaller steps first, then increasing as convergence is getting slower and slower
            faster      = LinearRescale ( iter, 1, 20, 1.0, 2.0, true );
//          faster      = LinearRescale ( Log ( iter ), Log ( 1 ), Log ( 10 ), 1.0, 2.0, true );

                                        // new variables, with extra step
            alpha( k ) += ( alphak - alpha( k ) ) * faster;

            mean ( k ) += ( meank  - mean ( k ) ) * faster;

                                        // Safety test, in case stepping was too big
            if ( ! ( alpha ( k ) > 0
                     && IsInsideLimits ( mean ( k ), Min (), Max () ) ) ) {
                                        // Skipping faster step for all variables
                faster      = 1;
                alpha ( k ) = alphak;
                mean  ( k ) = meank;
                }

            } // if dofaster
        else {

            alpha ( k ) = alphak;

            mean  ( k ) = meank;
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // New estimate for covariance
        varkl   = 0;
        varkr   = 0;

        OmpParallelForSum ( varkl, varkr )

        for ( int i = 0; i < NumItems; i++ )

            if ( Data[ i ] < mean ( k ) )   varkl  += weight ( i, k ) * Square ( Data[ i ] - mean ( k ) );
            else                            varkr  += weight ( i, k ) * Square ( Data[ i ] - mean ( k ) );
//            if ( Data[ i ] < mean ( k ) )   varkl  += alpha ( k ) * AsymmetricalNormal ( Data[ i ], mean [ k ], sigmal[ k ], sigmar[ k ] ) * Square ( Data[ i ] - mean ( k ) );
//            else                            varkr  += alpha ( k ) * AsymmetricalNormal ( Data[ i ], mean [ k ], sigmal[ k ], sigmar[ k ] ) * Square ( Data[ i ] - mean ( k ) );

                                        // Weighted SD
        sigmakl     = sqrt ( varkl / NonNull ( Nk ) );  // Nkl ?
        sigmakr     = sqrt ( varkr / NonNull ( Nk ) );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optionally stepping faster                                        
        if ( dofaster && faster != 1 ) {

                                        // new variable, with extra step
            sigmal ( k )   += ( sigmakl - sigmal ( k ) ) * faster;
            sigmar ( k )   += ( sigmakr - sigmar ( k ) ) * faster;


            if ( sigmal ( k ) <= 0 ||sigmar ( k ) <= 0 ) {
                                        // Skipping faster step for all variables
//              faster      = 1;
                alpha ( k ) = alphak;
                mean  ( k ) = meank;
                sigmal( k ) = sigmakl;  // !Note that this value is slightly wrong, SD should be recomputed from original Mean!
                sigmar( k ) = sigmakr;  // !Note that this value is slightly wrong, SD should be recomputed from original Mean!
                }

            } // if dofaster
        else {
            sigmal ( k )    = sigmakl;
            sigmar ( k )    = sigmakr;
            }

        } // for k


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // Saving intermediate mixture
   if ( iter % 10 == 0 ) {
    THistogram          H ( *this, 0, 0, 0, 3, 3, (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramNormArea | HistogramLinear ) );

    TArray2<double>     gaussmix ( numgaussian + 2, (int) H );

                                            // we can copy any existing scaling from original track...
    gaussmix.Index2     = H.Index1;


    double              sumg;
    double              v;

    for ( int i = 0; i < (int) H; i++ ) {

        sumg        = 0;

        for ( int k = 0; k < numgaussian; k++ ) {

            v                   = alpha ( k ) * AsymmetricalNormal ( H.ToReal ( RealUnit, i ), mean [ k ], sigmal[ k ], sigmar[ k ] );

            gaussmix ( k, i )   = v;

            sumg               += v;
//          sumg               += Square ( v );
            }


        gaussmix ( numgaussian    , i ) = sumg;
//      gaussmix ( numgaussian    , i ) = sqrt ( sumg );    // more beautiful, but less exact?
        gaussmix ( numgaussian + 1, i ) = H[ i ];
        }


    TExportTracks       expfile;
    sprintf ( expfile.Filename, "E:\\Data\\Stat.EMGaussianMixture.%02d.sef", iter );
    expfile.SetAtomType ( AtomTypeScalar );
    expfile.NumTracks           = gaussmix.GetDim1 ();
    expfile.NumTime             = gaussmix.GetDim2 ();
    expfile.Write ( gaussmix, true );

    expfile.Filename.Open ();
    }
* /
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute new Log-Likelihood
    oldsumlogsump   = sumlogsump;

    sumlogsump      = 0;


    OmpParallelForSum ( sumlogsump )

    for ( int i = 0; i < NumItems; i++ ) {

        double      sump    = 0;

        for ( int k = 0; k < numgaussian; k++ )

            sump   += alpha ( k ) * AsymmetricalNormal ( Data[ i ], mean [ k ], sigmal[ k ], sigmar[ k ] );
//            sump   += alpha ( k ) * Normal ( Data[ i ], mean [ k ], ( sigmal[ k ] + sigmar[ k ] ) / 2 );

        sumlogsump += Log ( sump );
        }

                                        // Break when reaching convergence
    if ( RelativeDifference ( sumlogsump, oldsumlogsump ) < requestedprecision )
        break;

    } // main loop


//experror.NumTime     = experror.TimeMax;
//experror.WriteHeader ( true );
//experror.End ();
//experror.Filename.Open ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Transfer final results
for ( int k = 0; k < numgaussian; k++ ) {

    mixg ( k, FunctionCenter    )   = mean  [ k ];
    mixg ( k, FunctionWidthLeft )   = GaussianSigmaToWidth ( sigmal [ k ] );
    mixg ( k, FunctionWidthRight)   = GaussianSigmaToWidth ( sigmar [ k ] );
    mixg ( k, FunctionHeight    )   = alpha [ k ];

//    DBGV5 ( k+1, mean [ k ], sigmal[ k ], sigmar[ k ], alpha [ k ], "results: k -> mean width alpha" );
    } // for k


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Returning each Gaussian in a track
if ( gaussmix ) {

    THistogram          H ( *this, 0, 0, 0, 3, 3, (HistogramOptions) ( HistogramPDF | HistogramSmooth | HistogramNormArea | HistogramLinear ) );

    gaussmix->Resize ( numgaussian + 2, (int) H );
                                                // we can copy any existing scaling from original track...
    gaussmix->Index2    = H.Index1;


    double              area            = 0;
    double              sumg;
    double              v;

    for ( int i = 0; i < (int) H; i++ ) {

        sumg        = 0;

        for ( int k = 0; k < numgaussian; k++ ) {

            v                       = alpha ( k ) * AsymmetricalNormal ( H.ToReal ( RealUnit, i ), mean [ k ], sigmal[ k ], sigmar[ k ] );

            (*gaussmix) ( k, i )    = v;

            sumg                   += v;
//          sumg                   += Square ( v );
            }

        area       += sumg;

        (*gaussmix) ( numgaussian    , i )  = sumg;
//      (*gaussmix) ( numgaussian    , i )  = sqrt ( sumg );    // more beautiful, but less exact?
        (*gaussmix) ( numgaussian + 1, i )  = H[ i ];
        }

                                        // manually rescaling the mixture so its area is 1 - same rescaling for the individual Gaussians, too
    for ( int i = 0; i < (int) H; i++ ) 
    for ( int k = 0; k < numgaussian + 1; k++ )

            (*gaussmix) ( k, i )   /= area;

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // returning an average value
return  sumlogsump / NumItems;
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TGoEasyStats::TGoEasyStats ()
{
Stats               = 0;
NumStats            = 0;

Resize ( 0 );
}

                                        // numdata could be 0
        TGoEasyStats::TGoEasyStats ( int numstats, int numdata )
{
Stats               = 0;
NumStats            = 0;

Resize ( numstats, numdata );
}


        TGoEasyStats::~TGoEasyStats ()
{
Resize ( 0 );
}

                                        // Resets memory to 0 - NOT deallocating
void    TGoEasyStats::Reset ()
{
for ( int i = 0; i < NumStats; i++ )
    Stats[ i ].Reset ();
}

                                        // numstats: 0 -> deallocate, >0 allocate
                                        // numdata : 0 -> deallocate TEasyStats storage, >0 allocate TEasyStats storage
void    TGoEasyStats::Resize ( int numstats, int numdata )
{
                                        // deallocate & reset
if ( numstats <= 0 ) {

    if ( Stats )
        delete[]     Stats;

    Stats               = 0;
    NumStats            = 0;

    return;
    }


else { // if ( numstats > 0 ) {
                                        // first allocate the array of TEasyStats
                                        // delete array only if the new size is different
    if ( numstats != NumStats ) {

        if ( Stats )
            delete[]     Stats;

        Stats               = new TEasyStats [ numstats ];
        NumStats            = numstats;
        }
                                        // then resize/deallocate each TEasyStats
                                        // Resets each TEasyStats
    for ( int i = 0; i < NumStats; i++ )
        Stats[ i ].Resize ( numdata );
    }

}


            TGoEasyStats::TGoEasyStats ( const TGoEasyStats &op )
{
Stats               = 0;
                                        // allocate space
Resize ( op.NumStats );

                                        // copy content, including storage
for ( int i = 0; i < NumStats; i++ )
    Stats[ i ]  = op.Stats[ i ];
}


TGoEasyStats& TGoEasyStats::operator= ( const TGoEasyStats &op2 )
{
if ( &op2 == this )
    return  *this;


Stats               = 0;
                                        // allocate space
Resize ( op2.NumStats );

                                        // copy content, including storage
for ( int i = 0; i < NumStats; i++ )
    Stats[ i ]  = op2.Stats[ i ];


return  *this;
}


//----------------------------------------------------------------------------
void    TGoEasyStats::Cumulate ( const TVector<float>& v, PolarityType polarity, const TVector<float>& refv )
{
bool                invert          = polarity == PolarityEvaluate && v.IsOppositeDirection ( refv );


if ( invert )

    for ( int i = 0; i < v.GetDim (); i++  )

        Stats[ i ].Add ( - v[ i ] );
else
    for ( int i = 0; i < v.GetDim (); i++  )

        Stats[ i ].Add (   v[ i ] );
}


//----------------------------------------------------------------------------
void    TGoEasyStats::Show ( char *title )
{
                                        // could create temp title with group index + title
for ( int i = 0; i < NumStats; i++ )
    Stats[ i ].Show ( title );
}


//----------------------------------------------------------------------------
                                        // Type will apply to ALL stats
TArray1<int>    TGoEasyStats::DeSkew ( DeSkewType how )
{
TArray1<int>        skewness ( NumStats );


for ( int i = 0; i < NumStats; i++ )
    
    skewness ( i )  = Stats[ i ].DeSkew ( how );


return  skewness;
}


//----------------------------------------------------------------------------
void    TGoEasyStats::Median ( TVector<float>& v, bool strictvalue )
{
for ( int i = 0; i < NumStats; i++ )

    v[ i ]  = Stats[ i ].Median ( strictvalue );
}


//----------------------------------------------------------------------------
double  TGoEasyStats::Max ()
{
TEasyStats          allstat;

for ( int i = 0; i < NumStats; i++ )
    allstat.Add ( Stats[ i ].Max (), ThreadSafetyIgnore );

return  allstat.Max ();
}


double  TGoEasyStats::Min ()
{
TEasyStats          allstat;

for ( int i = 0; i < NumStats; i++ )
    allstat.Add ( Stats[ i ].Min (), ThreadSafetyIgnore );

return  allstat.Min ();
}


double  TGoEasyStats::Quantile ( double p )
{
TEasyStats          allstat;

for ( int i = 0; i < NumStats; i++ )
    allstat.Add ( Stats[ i ].Quantile ( p ), ThreadSafetyIgnore );

return  allstat.Mean ();
}


//----------------------------------------------------------------------------
                                        // Get the optimal "sampling" across data
                                        // This is like a radius for the Gaussian (SqrtTwo * SD)
double  TGoEasyStats::GaussianKernelDensity ( KernelDensityType kdetype )
{
TEasyStats          allstat;

for ( int i = 0; i < NumStats; i++ )

    allstat.Add ( Stats[ i ].GaussianKernelDensity ( kdetype ), ThreadSafetyIgnore );

return  allstat.Mean ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
