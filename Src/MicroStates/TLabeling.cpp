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

#include    "TLabeling.h"
#include    "TMicroStates.h"

#include    "TMaps.h"
#include    "Files.TOpenDoc.h"
#include    "TSegDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TLabeling::TLabeling ()
{
DeallocateMemory ();
}


        TLabeling::TLabeling ( int numdata )                
{
Resize ( numdata );
}


        TLabeling::~TLabeling ()
{
DeallocateMemory ();
}


void    TLabeling::DeallocateMemory ()
{
Labels  .DeallocateMemory ();
Polarity.DeallocateMemory ();
}


void    TLabeling::Resize ( int numdata )
{
DeallocateMemory ();

if ( numdata <= 0 )
    return;

Labels  .Resize ( numdata );
Polarity.Resize ( numdata );

Reset ();
}

                                        // copy constructor
        TLabeling::TLabeling ( const TLabeling& op )
{
Labels      = op.Labels;
Polarity    = op.Polarity;
}

                                        // assignation operator
TLabeling&  TLabeling::operator= ( const TLabeling& op2 )
{
if ( &op2 == this )
    return  *this;


Labels      = op2.Labels;
Polarity    = op2.Polarity;
    

return  *this;
}


//----------------------------------------------------------------------------
                                        // 3 different versions, some parallelized, some not
void    TLabeling::Reset ()
{
Labels          = UndefinedLabel;
Polarity        = PolarityDirect;
}


void    TLabeling::Reset ( long tf )
{
//if ( IsNotAllocated () )  // let's trust caller for speed
//    return;

Labels  [ tf ]  = UndefinedLabel;
Polarity[ tf ]  = PolarityDirect;
}


void    TLabeling::Reset ( long tfmin, long tfmax )
{
if ( IsNotAllocated () )
    return;


Clipped ( tfmin, tfmax, (long) 0, (long) GetNumData () - 1 );


for ( long tf = tfmin; tf <= tfmax; tf++ ) {
    Labels  [ tf ]  = UndefinedLabel;
    Polarity[ tf ]  = PolarityDirect;
    }
}


//----------------------------------------------------------------------------
                                        // !Make sure to call UpdatePolarities after using this function!
void    TLabeling::SetLabel ( long tf, LabelType label )
{
//if ( IsNotAllocated () || ! IsInsideLimits ( tf, (long) 0, (long) GetNumData () - 1 ) )
//    return;

Labels[ tf ]    = label;
//Polarity[ tf ]  = PolarityDirect;
}

                                        // PolarityEvaluate NOT allowed
void    TLabeling::SetLabel ( long tf, LabelType label, PolarityType polarity )
{
//if ( IsNotAllocated () || ! IsInsideLimits ( tf, (long) 0, (long) GetNumData () - 1 ) )
//    return;

Labels[ tf ]    = label;
                                        // caller has to make sure there is only PolarityInvert or PolarityDirect
//Polarity[ tf ]  = (uchar) polarity;
                                        // make sure NO PolarityEvaluate creeps in
Polarity[ tf ]  = (uchar) ( polarity == PolarityInvert ? PolarityInvert : PolarityDirect );
}


void    TLabeling::SetPolarity ( long tf, PolarityType polarity )
{
//if ( IsNotAllocated () || ! IsInsideLimits ( tf, (long) 0, (long) GetNumData () - 1 ) )
//    return;

//Polarity[ tf ]  = (uchar) polarity;
                                        // make sure NO PolarityEvaluate creeps in
Polarity[ tf ]  = (uchar) ( polarity == PolarityInvert ? PolarityInvert : PolarityDirect );
}


//----------------------------------------------------------------------------
                                        // Results will be either PolarityDirect or PolarityInvert, resolving any PolarityEvaluate
void    TLabeling::UpdatePolarities (   const TMaps&    data, 
                                        long            tfmin,      long            tfmax,
                                        const TMaps&    maps,   
                                        PolarityType    polarity )
{
OmpParallelFor

for ( long tf = tfmin; tf <= tfmax; tf++ )

    Polarity[ tf ]  = IsDefined ( tf )                                          // UndefinedLabel -> PolarityDirect
                   && polarity == PolarityEvaluate                             
                   && maps[ Labels[ tf ] ].IsOppositeDirection ( data[ tf ] ) ? (uchar) PolarityInvert : (uchar) PolarityDirect;
}


//----------------------------------------------------------------------------
int     TLabeling::CountLabels ( int maxclusters )  const
{
TSelection          count ( maxclusters, OrderSorted );

count.Reset ();


for ( int tf = 0; tf < GetNumData (); tf++ ) {

    if ( IsUndefined ( tf ) )
        continue;


    count.Set ( Labels[ tf ] );

                                        // no need to count more, we're full?
    if ( (int) count == maxclusters )
        break;
    }


return  (int) count;
}


//----------------------------------------------------------------------------
LabelType   TLabeling::MaxLabel ()  const
{
LabelType           maxlabel        = UndefinedLabel;   


for ( int tf = 0; tf < GetNumData (); tf++ )

    if ( IsDefined ( tf ) )
                   // !We don't guarantee that UndefinedLabel will always be -1, so don't use max if undefined!
        maxlabel    = maxlabel != UndefinedLabel ? max ( maxlabel, Labels[ tf ] ) 
                                                 :                 Labels[ tf ];
            

return  maxlabel;
}


//----------------------------------------------------------------------------
int     TLabeling::GetSizeOfClusters (  int     cluster     )  const
{
int                 nummaps         = 0;

                                        // count maps of range of clusters, with optional downsampling step
OmpParallelForSum ( nummaps )

for ( int tf = 0; tf < GetNumData (); tf++ )

    if ( IsDefined ( tf ) && Labels[ tf ] == cluster )

        nummaps++;


return  nummaps;
}


int     TLabeling::GetSizeOfClusters (  int     mincluster, int     maxcluster, int     step )  const
{
int                 nummaps         = 0;

                                        // count maps of range of clusters, with optional downsampling step
OmpParallelForSum ( nummaps )

for ( int tf = 0; tf < GetNumData (); tf+=step )

    if ( IsDefined ( tf ) && IsInsideLimits ( Labels[ tf ], mincluster, maxcluster ) )

        nummaps++;


return  nummaps;
}


//----------------------------------------------------------------------------
                                        // Could totally NOT attribute any label at all - caller should make sure the variables are initialized with UndefinedLabel beforehand
                                        // limitcorr is used as a min correlation to achieve potential labeling
                                        // !Do not parallelize - this should done be from the caller side!
void    TLabeling::KeepBestLabeling (   
                                    const TMap&     map1,
                                    const TMap&     map2,
                                    long            tf,         LabelType       templatenumber,
                                    PolarityType    polarity,
                                    double&         corrmax,    double          limitcorr
                                    )
{
                                        // solve polarity ONLY for non-cloud, cloud will EVALUATE ON EACH MAP
if ( polarity == PolarityEvaluate )
    polarity    = map1.IsOppositeDirection ( map2 ) ? PolarityInvert : PolarityDirect;


double              corr            = Project ( map1, map2, polarity );


  // above global limit   locally above all maps
if ( corr >= limitcorr && corr > corrmax ) {

    corrmax     = corr;
                                        // here polarity is either PolarityDirect or PolarityInvert
    SetLabel ( tf, templatenumber, polarity /*polarity == PolarityInvert ? PolarityInvert : PolarityDirect*/ );
    }
}

//----------------------------------------------------------------------------
                                        // Splitting function above into 2 specialized versions, with less testing
                                        // !Does not update polarity flag, this should be done separately by calling  UpdatePolarities  after!
bool    KeepBestLabelingDirect  (   
                                const TMap&     map1,
                                const TMap&     map2,
                                double&         corrmax,
                                double          limitcorr
                                )
{
double              corr            = Clip ( map1.ScalarProduct ( map2 ), -1.0, 1.0 );

  // above global limit   locally above all maps
if ( corr >= limitcorr && corr > corrmax ) {

    corrmax     = corr;

    return  true;
    }
else
    return  false;
}


bool    KeepBestLabelingEvaluate(   
                                const TMap&     map1,
                                const TMap&     map2,
                                double&         corrmax,
                                double          limitcorr
                                )
{
double              corr            = Clip ( fabs ( map1.ScalarProduct ( map2 ) ), 0.0, 1.0 );

  // above global limit   locally above all maps
if ( corr >= limitcorr && corr > corrmax ) {

    corrmax     = corr;

    return  true;
    }
else
    return  false;
}


//----------------------------------------------------------------------------
                                        // Compact maps and labels indexes if some maps have disappeared from the labeling
                                        // Returns the number of final maps
int     TLabeling::PackLabels ( TMaps&      maps )
{
                                        // get highest possible label
LabelType           maxlabel        = MaxLabel ();


if ( maxlabel == UndefinedLabel )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // let's wrap this into a convenient lambda
auto    labelexists     = [this] ( int nc ) 
{
for ( long tf = 0; tf < GetNumData (); tf++ ) 
    if ( Labels[ tf ] == nc )
        return  true;                   // found at least 1 label for cluster nc -> label nc exists

return  false;
};

                                        // scan each cluster
for ( int nc = 0; nc <= maxlabel;  ) {

    if ( labelexists ( nc ) ) {
        nc++;                           // jump to next candidate
        continue;                       // no need to compact
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here cluster nc is empty, compact everything above nc by shifting values
    OmpParallelFor

    for ( long tf = 0; tf < GetNumData (); tf++ )

        if ( IsDefined ( tf ) && Labels[ tf ] > nc )
            Labels[ tf ]--;              // shift labels


    for ( int nc2 = nc; nc2 <= maxlabel - 1; nc2++ )
        maps[ nc2 ] = maps[ nc2 + 1 ];  // shift maps

                                        // NOT incrementing nc, but decrementing maxlabel, as all indexes have been shifted
    maxlabel--;
    } // for nc


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // returning the final number of clusters
return  maxlabel + 1;
}


//----------------------------------------------------------------------------
                                        // We blindly trust ordering array here...
void    TLabeling::ReorderLabels    (   TMaps&          maps,
                                        TArray2<int>&   ordering
                                    )
{
int                 nclusters       = ordering.GetDim1 ();

TMaps               oldmaps ( maps );   // save the maps with old ordering


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // reassign the labels
for ( long tf = 0; tf < GetNumData (); tf++ )

    if ( IsDefined ( tf ) )
        Labels[ tf ] = ordering ( Labels[ tf ], scanbackindex );

                                        
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // rewrite the sequence of maps
for ( int nc = 0; nc < nclusters; nc++ )

    maps[ nc ]  = oldmaps[ ordering ( nc, scanindex ) ];
}


//----------------------------------------------------------------------------
void    TLabeling::ReadFile     ( const char*  filename )
{
DeallocateMemory ();

if ( StringIsEmpty ( filename ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TOpenDoc<TSegDoc>   segdoc ( filename, OpenDocHidden );

int                 NumFiles        = segdoc->GetNumFiles     ();
int                 NumVars         = segdoc->GetNumVars      ();
int                 NumTimeFrames   = segdoc->GetNumTimeFrames();

                                        // !all files will have the same maximum length!
TArray2<float>      tracks ( segdoc->GetNumElectrodes (), NumTimeFrames );

segdoc->ReadRawTracks   ( 0, NumTimeFrames - 1, tracks );


Resize ( NumFiles * NumTimeFrames );

for ( int fi = 0, tfl = 0; fi < NumFiles; fi++ ) {

    for ( long tf0 = 0; tf0 < NumTimeFrames; tf0++, tfl++ ) {

        int             seglabel    = tracks ( fi * NumVars + SegVarSegment,  tf0 );
        LabelType       label       = seglabel == 0 ? UndefinedLabel : seglabel - 1;

        int             segpolarity = tracks ( fi * NumVars + SegVarPolarity, tf0 );        // this will also conveniently ignore old files "Dis" line - but don't rely on it!
        PolarityType    polarity    = segpolarity == -1 ? PolarityInvert : PolarityDirect;

        SetLabel    ( tfl, label    );
        SetPolarity ( tfl, polarity );
        }
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
