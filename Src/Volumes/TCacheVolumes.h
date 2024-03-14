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

#include    "Time.Utils.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template <class TypeD> class        TArray3;
template <class TypeD> class        TVolume;


//----------------------------------------------------------------------------
                                        // Caching a single volume
template <class TypeD>
class   TCacheVolume
{
public:
                    TCacheVolume ();


    TVolume<TypeD>* OrigData;           // pointer to original data
    TypeD           OrigChecksum;       // the checksum of these data

    TVolume<TypeD>  CacheData;          // cache data with margins, filtered
    ULONG           LastAccessTime;
    double          Smoothing;


    bool            IsACacheOf ( int dim1, int dim2, int dim3 );
    void            Set        ( int dim1, int dim2, int dim3 );

    bool            IsACacheOf ( TVolume<TypeD>* data, int dim1, int dim2, int dim3, TypeD chks = 0 );
    void            Set        ( TVolume<TypeD>* data, int dim1, int dim2, int dim3, TypeD chks = 0 );

    bool            IsACacheOf ( int dim1, int dim2, int dim3, double smoothing );
    void            Set        ( int dim1, int dim2, int dim3, double smoothing );

    void            Accessed   ();

    TypeD           ComputeChecksum ( TVolume<TypeD> *data );
};


//----------------------------------------------------------------------------
                                        // Caching a set of volumes
template <class TypeD>
class   TCacheVolumes
{
public:
                    TCacheVolumes ( int numcaches );
                   ~TCacheVolumes ();


    TCacheVolume<TypeD>*    Caches;
    int                     MaxAllocated;
    int                     NumAllocated;


    TVolume<TypeD>* GetCache ( int dim1, int dim2, int dim3, bool &reload );
    TVolume<TypeD>* GetCache ( TVolume<TypeD> *data, int dim1, int dim2, int dim3, bool &reload );
    TVolume<TypeD>* GetCache ( int dim1, int dim2, int dim3, double smoothing, bool &reload );
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
template <class TypeD>
        TCacheVolume<TypeD>::TCacheVolume ()
{
OrigData        = 0;
OrigChecksum    = 0;
LastAccessTime  = 0;
Smoothing       = 0;
}


template <class TypeD>
TypeD   TCacheVolume<TypeD>::ComputeChecksum ( TVolume<TypeD> *data )
{
                                        // have a proportional step
TDownsampling       down ( data->MaxSize(), 64 );
TypeD*              todata          = data->GetArray ();
TypeD               chks            = 0;


//OmpParallelForSum ( chks )            // reduction will give different values, as the sum of floating points is not the same!

for ( int i = 0; i < data->GetLinearDim (); i += down.Step )
                                        // multiply data by position: in case of axis permutation, f.ex., we need the checksum to be different!
    chks   += todata[ i ] * i;


//DBGV3 ( data->MaxSize(), down.Step, chks, "MaxSize step -> checksum" );

return chks;
}


//----------------------------------------------------------------------------
                                        // simpler function, testing only dimensions
template <class TypeD>
bool     TCacheVolume<TypeD>::IsACacheOf ( int dim1, int dim2, int dim3 )
{
return  dim1 == CacheData.GetDim1() && dim2 == CacheData.GetDim2() && dim3 == CacheData.GetDim3();
}


template <class TypeD>
bool     TCacheVolume<TypeD>::IsACacheOf ( TVolume<TypeD> *data, int dim1, int dim2, int dim3, TypeD chks )
{
if ( chks == 0 )                        // checksum provided?
    chks = ComputeChecksum ( data );    // no, compute it

                                        // test if the same original data
return  data == OrigData
     && dim1 == CacheData.GetDim1() && dim2 == CacheData.GetDim2() && dim3 == CacheData.GetDim3()
     && chks == OrigChecksum;
}


template <class TypeD>
bool     TCacheVolume<TypeD>::IsACacheOf ( int dim1, int dim2, int dim3, double smoothing )
{
                                        // test dimensions & smoothing factor
return  dim1 == CacheData.GetDim1() && dim2 == CacheData.GetDim2() && dim3 == CacheData.GetDim3()
     && smoothing == Smoothing;
}


//----------------------------------------------------------------------------
                                        // simpler function, testing only dimensions
template <class TypeD>
void     TCacheVolume<TypeD>::Set ( int dim1, int dim2, int dim3 )
{
                                        // don't need these
OrigData        = 0;
OrigChecksum    = 0;
Smoothing       = 0;
                                        // then create the cache
CacheData.Resize ( dim1, dim2, dim3 );

Accessed ();
}


template <class TypeD>
void     TCacheVolume<TypeD>::Set ( TVolume<TypeD> *data, int dim1, int dim2, int dim3, TypeD chks )
{
                                        // don't need these
Smoothing       = 0;
                                        // store informations
OrigData        = data;
                                        // compute the checksum if skipped in the test
OrigChecksum    = chks ? chks : ComputeChecksum ( data );
                                        // then create the cache
CacheData.Resize ( dim1, dim2, dim3 );

Accessed ();
}


template <class TypeD>
void     TCacheVolume<TypeD>::Set ( int dim1, int dim2, int dim3, double smoothing )
{
                                        // don't need these
OrigData        = 0;
OrigChecksum    = 0;
                                        // store informations
Smoothing       = smoothing;
                                        // then create the cache
CacheData.Resize ( dim1, dim2, dim3 );

Accessed ();
}


//----------------------------------------------------------------------------
template <class TypeD>
void     TCacheVolume<TypeD>::Accessed ()
{
                                        // store current time
LastAccessTime  = GetWindowsTimeInMillisecond ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // create an integrated set of cached volumes
template <class TypeD>
        TCacheVolumes<TypeD>::TCacheVolumes ( int numcaches )
{
MaxAllocated    = numcaches > 0 ? numcaches : 1;
NumAllocated    = 0;

Caches          = new TCacheVolume<TypeD> [ MaxAllocated ];
}


template <class TypeD>
        TCacheVolumes<TypeD>::~TCacheVolumes ()
{
delete[]    Caches;
Caches          = 0;
}


//----------------------------------------------------------------------------
                                        // simpler function, testing only dimensions
template <class TypeD>
TVolume<TypeD>* TCacheVolumes<TypeD>::GetCache ( int dim1, int dim2, int dim3, bool &reload )
{
                                        // scan for existing caches, return the correct one if it exists
for ( int i = 0; i < NumAllocated; i++ ) {

    if ( Caches[ i ].IsACacheOf ( dim1, dim2, dim3 ) ) {

        Caches[ i ].Accessed ();        // freshen access time

        reload  = false;                // tell not to overwrite!

//        if ( VkQuery () ) DBGV ( i, "Found cache #" );

        return  &Caches[ i ].CacheData;
        }
    }
                                        // does not exist, so we have to reload it
reload  = true;

                                        // does not exist, create a new cache
if ( NumAllocated < MaxAllocated ) {    // still some space?

    Caches[ NumAllocated ].Set ( dim1, dim2, dim3 );
//    if ( VkQuery () ) DBGV ( NumAllocated, "Create cache #" );

    return &Caches[ NumAllocated++ ].CacheData;
    }
else {                                  // no more free slot -> find the oldest used Cache, and pitylessly overwrite it
    ULONGLONG       la          = Caches[ 0 ].LastAccessTime;
    int             lai         = 0;

    for ( int i = 1; i < NumAllocated; i++ )
        if ( Caches[ i ].LastAccessTime < la ) {
            la  = Caches[ i ].LastAccessTime;
            lai = i;
            }

    Caches[ lai ].Set ( dim1, dim2, dim3 );
//    if ( VkQuery () ) DBGV ( lai, "Overload cache #" );

    return  &Caches[ lai ].CacheData;
    }
}


//----------------------------------------------------------------------------
template <class TypeD>
TVolume<TypeD>* TCacheVolumes<TypeD>::GetCache ( TVolume<TypeD> *data, int dim1, int dim2, int dim3, bool &reload )
{
                                        // do this once only, can be costly!
TypeD               chks            = Caches->ComputeChecksum ( data );

                                        // scan for existing caches, return the correct one if it exists
for ( int i = 0; i < NumAllocated; i++ ) {

    if ( Caches[ i ].IsACacheOf ( data, dim1, dim2, dim3, chks ) ) {

        Caches[ i ].Accessed ();        // freshen access time

        reload  = false;                // tell not to overwrite!

//        if ( VkQuery () ) DBGV ( i, "Found cache #" );

        return  &Caches[ i ].CacheData;
        }
    }
                                        // does not exist, so we have to reload it
reload  = true;

                                        // does not exist, create a new cache
if ( NumAllocated < MaxAllocated ) {    // still some space?

    Caches[ NumAllocated ].Set ( data, dim1, dim2, dim3, chks );
//    if ( VkQuery () ) DBGV ( NumAllocated, "Create cache #" );

    return &Caches[ NumAllocated++ ].CacheData;
    }
else {                                  // no more free slot -> find the oldest used Cache, and pitylessly overwrite it
    ULONGLONG       la          = Caches[ 0 ].LastAccessTime;
    int             lai         = 0;

    for ( int i = 1; i < NumAllocated; i++ )
        if ( Caches[ i ].LastAccessTime < la ) {
            la  = Caches[ i ].LastAccessTime;
            lai = i;
            }

    Caches[ lai ].Set ( data, dim1, dim2, dim3, chks );
//    if ( VkQuery () ) DBGV ( lai, "Overload cache #" );

    return  &Caches[ lai ].CacheData;
    }
}


//----------------------------------------------------------------------------
template <class TypeD>
TVolume<TypeD>* TCacheVolumes<TypeD>::GetCache ( int dim1, int dim2, int dim3, double smoothing, bool &reload )
{
                                        // scan for existing caches, return the correct one if it exists
for ( int i = 0; i < NumAllocated; i++ ) {

    if ( Caches[ i ].IsACacheOf ( dim1, dim2, dim3, smoothing ) ) {

        Caches[ i ].Accessed ();        // freshen access time

        reload  = false;                // tell not to overwrite!

//        DBGV2 ( i, smoothing, "Found cache #, smoothing" );

        return  &Caches[ i ].CacheData;
        }
    }
                                        // does not exist, so we have to reload it
reload  = true;

                                        // does not exist, create a new cache
if ( NumAllocated < MaxAllocated ) {    // still some space?

    Caches[ NumAllocated ].Set ( dim1, dim2, dim3, smoothing );

//    DBGV2 ( NumAllocated, smoothing, "Create cache #, smoothing" );

    return &Caches[ NumAllocated++ ].CacheData;
    }
else {                                  // no more free slot -> find the further smoothing Cache, and pitylessly overwrite it
    double  maxsmoothdist   = 0;
    int     maxsmoothdisti  = 0;

    for ( int i = 0; i < NumAllocated; i++ )
        if ( fabs ( Caches[ i ].Smoothing - smoothing ) >= maxsmoothdist ) {
            maxsmoothdist   = fabs ( Caches[ i ].Smoothing - smoothing );
            maxsmoothdisti  = i;
            }

//    DBGV3 ( maxsmoothdisti, Caches[ maxsmoothdisti ].Smoothing, smoothing, "Overload cache #, old smoothing -> new smoothing" );

    Caches[ maxsmoothdisti ].Set ( dim1, dim2, dim3, smoothing );

    return  &Caches[ maxsmoothdisti ].CacheData;
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
