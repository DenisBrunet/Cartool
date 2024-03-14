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

#include    "TList.h"
#include    "Math.Utils.h"

#include    "TVolume.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // A Volume containing the mask, an Index, some variables like Center...
                                        // !Index must start from 1!

constexpr double    DefaultRegionBackground   = 1e-6;   // use an epsilon instead of the real background value


using   RegionType  = uchar;                            // the smallest size, as a region is basically an array of boolean


class   TVolumeRegion : public  TVolume<RegionType>,
                        public  TIndex
{
public:
                    TVolumeRegion ();
                    TVolumeRegion ( const Volume* volume, int index );    // same size as volume, but empty


    TPointFloat     Center;
    double          Radius;
    double          Inertia;
    double          Compactness;


    void            Reset               ();
    void            Set                 ( bool compact );
    void            Compact             ();     // to the minimum size

    bool            IsEmpty             ()                          const   { return ! NumPoints; }

    int             GetNumPoints        ()                          const   { return  NumPoints; }
    int             GetCompactCount     ()                          const   { return  (int) ( sqrt ( (double) NumPoints ) * Compactness ); /*NumPoints * Compactness;*/ }  // measure of compactness used to retain interesting regions
//  double          GetBackgroundValue  ()                          const   { return BackgroundValue; }

                                        // Absolute coordinates account for the Translation shift
    void            CoordinatesToAbs    ( int &x, int &y, int &z )  const   { x          += Translation.X; y          += Translation.Y; z          += Translation.Z; }
    void            CoordinatesToAbs    ( TPointFloat&  p        )  const   { p += Translation; }
    void            CoordinatesToAbs    ( TPointDouble& p        )  const   { p += Translation; }
                                        // Relative coordinates is equivalent to Voxel coordinates
    void            CoordinatesToRel    ( int &x, int &y, int &z )  const   { x          -= Translation.X; y          -= Translation.Y; z          -= Translation.Z; }
    void            CoordinatesToRel    ( TPointFloat&  p        )  const   { p -= Translation; }
    void            CoordinatesToRel    ( TPointDouble& p        )  const   { p -= Translation; }


    void            CopyTo              ( Volume &array, bool clear )   const;


    void            UpdateStats         ();
    void            ComputeDataStats    ( const Volume &data, TEasyStats &stat );
    double          ComparedTo          ( const TVolumeRegion& region );

    void            Show                ( char *title = 0 ) const;


                    TVolumeRegion       ( const TVolumeRegion& op );
    TVolumeRegion&  operator    =       ( const TVolumeRegion& op2 );
//  TVolumeRegion&  operator    =       ( const Volume&        op2 );
//  TVolumeRegion&  operator    +=      ( const TVolumeRegion& op2 );


                    operator    int             ()              const   { return (int)  NumPoints; }
                    operator    bool            ()              const   { return (bool) NumPoints; }


protected:

                    TVolumeRegion ( int dim1, int dim2, int dim3, int shift1, int shift2, int shift3, int index );  // Shrinking from a bigger volume


    int             NumPoints;
    TPointInt       Translation;        // A region is a sub-volume, this is the offset to access the absolute coordinates

};


//----------------------------------------------------------------------------

enum            VolumeRegionsSort
                {
                SortRegionsCount,
                SortRegionsCompact,
                SortRegionsCompactCount
                };

                                        // Group of Regions, indexed from [1..n]
class   TVolumeRegions
{
public:
                    TVolumeRegions ();
                   ~TVolumeRegions ();


    int             NumRegions      ()              const   { return    Group.Num (); }
    bool            IsEmpty         ()              const   { return    Group.IsEmpty (); }

    void            Reset           ();

    void            Add             ( TVolumeRegion* region /*, bool copy = false*/ );

    void            Sort            ( VolumeRegionsSort how );

    void            UpdateStats     ();

    void            Show            ( char *title = 0 ) const;

                                        // !index starts from 1!
    TVolumeRegion&          operator    []   ( int index )          { return *Group[ index - 1 ]; }
    const TVolumeRegion&    operator    []   ( int index )  const   { return *Group[ index - 1 ]; }

                    operator    int  ()             const   { return (int)  Group; }
                    operator    bool ()             const   { return (bool) Group; }
                    operator    TList<TVolumeRegion>& ()    { return Group; }

protected:

    crtl::TList<TVolumeRegion>  Group;


    void                    Sort  ( int l, int r, bool (*f) ( const TVolumeRegion* va, const TVolumeRegion* vb ) );

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
