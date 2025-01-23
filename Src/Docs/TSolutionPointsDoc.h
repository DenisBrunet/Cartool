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

#include    "Geometry.TPoint.h"
#include    "TWeightedPoints.h"
#include    "TBaseDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// To test in case of change of interpolation stuff:
//
// 1) Shape "XY" of the 4NN according to slice of MRI: should fit equally in both dimensions
// 2) Shape "Z": is the 4NN slice the next correct one?
// 3) Precision of the 4NN in slice: Max localization
// 4) Precision of the Potatoes / Onions vs Berries
// 5) Precision of the Potatoes / Onions vs 4NN in slice
//
// Changing VolumeToAbsolute fct -> re-check AbsoluteToVolume

                                        
constexpr int       SolutionPointsNumSpaces         = 1;

constexpr double    SolutionPointsSubSampling       = 3.0;  // was 2, which is also good (and faster)
                                        
constexpr double    SolutionPointsMinDistanceRatio  = 0.5;  // points are roughly on a grid, this is the half distance between two slices
                                        // define this to have a 4NN display, un-define to have a 1NN display
#define             DisplayInverseInterpolation4NN

constexpr USHORT    UndefinedInterpolation1NN       = USHRT_MAX;


enum        SPInterpolationType
            {
            SPInterpolationNone,
            SPInterpolation1NN,
            SPInterpolation4NN,

            NumSPInterpolationTypes,
            };

extern const char   SPInterpolationTypeNames[ NumSPInterpolationTypes ][ 64 ];


//----------------------------------------------------------------------------
                                        // Base class for all different solution points files
class   TSolutionPointsDoc  :   public  TBaseDoc
{
public:
                    TSolutionPointsDoc  ( owl::TDocument *parent = 0 );


    bool            Commit	            ( bool force = false );
    bool            InitDoc             ();
    bool            Revert	            ( bool force = false );


    bool            IsOpen	            ()                      const   { return  (bool) GetPoints (); }
    bool            IsGrid              ()                      const   { return  IsGeometryGrid            ( ExtraContentType ); }
    bool            IsGridNotAligned    ()                      const   { return  IsGeometryGridNotAligned  ( ExtraContentType ); }
    bool            IsGridAligned       ()                      const   { return  IsGeometryGridAligned     ( ExtraContentType ); }


    int                 GetNumSolPoints     ()                  const   { return  (int) GetPoints (); }
    const TPoints&      GetPoints           ( int space = 0 )   const   { return DisplaySpaces[ space ].Points; }
          TPoints&      GetPoints           ( int space = 0 )           { return DisplaySpaces[ space ].Points; }
    const TBoundingBox<double>* GetBounding ( int space = 0 )   const   { return (bool) DisplaySpaces ? & DisplaySpaces[ space ].Bounding : 0; }
          TBoundingBox<double>* GetBounding ( int space = 0 )           { return (bool) DisplaySpaces ? & DisplaySpaces[ space ].Bounding : 0; }

    double              GetMedianDistance   ( int space = 0 )   const   { return DisplaySpaces[ space ].MedianDistance; }
    double              GetPointRadius      ( int space = 0 )   const   { return DisplaySpaces[ space ].PointRadius; }

    const TPointDouble& GetVoxelSize        ()                  const   { return  VoxelSize; }
    TPointInt           GetVolumeSize       ();

    int                 GetNearestElementIndex  ( const TPointFloat& p, double dmax = 0 )   const;
    int                 GetIndex                ( char* spname )                            const;
    void                GetNeighborsIndexes     ( TArray2<int>&  neighbi, NeighborhoodType neightype )  const;
    void                GetNeighborhoodDistances( TArray3<double>&  neighborhood, int maxnumneigh )     const ; // Only needed distances + indexes, sorted increasing

    virtual const TStrings*     GetSPNames      ()              const   { return &SPNames;      }
    virtual const char*         GetSPName       ( int e = 0 )   const   { return  SPNames[ e ]; }


    void            ExportText ()                               const;


    bool                            HasInterpol1NN ()           const   { return  Interpol1NN.IsAllocated (); }
    bool                            HasInterpol4NN ()           const   { return  Interpol4NN.IsAllocated (); }
    const TArray3<USHORT>*          GetInterpol1NN ()           const   { return &Interpol1NN; }
    const TArray3<TWeightedPoints4>*GetInterpol4NN ()           const   { return &Interpol4NN; }

    bool            BuildInterpolation ( SPInterpolationType interpol, const TVolumeDoc* MRIGrey );

                                        // conversion functions to go back and forth between the interpolation volume and the spatial coordinates
    void            VolumeToAbsolute  ( TPointFloat &p )        const;
    void            AbsoluteToVolume  ( TPointFloat &p )        const;
    double          AbsoluteToVolumeX ( double xa )             const;
    double          AbsoluteToVolumeY ( double ya )             const;
    double          AbsoluteToVolumeZ ( double za )             const;


protected:
    TStrings        SPNames;
    bool            HasNames;

                                        // Data for 3D rendering
    TPointDouble    VoxelSize;          // size of 1 voxel of SPVolume - will constrain the 3D rendering size - see GetVolumeSize()
//  TPointDouble    RealSize;           // size in [mm]
    TPointDouble    OriginShift;        // to fine tune position (regular case)

    TBoundingBox<double>        MRIBounding;
    TArray3<USHORT>             Interpol1NN;    // !currently up to 65534 points!
    TArray3<TWeightedPoints4>   Interpol4NN;    // !currently up to 65535 points!


    void            SetBounding         ();
    void            SetMedianDistance   ();
    void            SetOriginShift      ();
    void            Sort                ( TPointFloatI* ListSPI, int l, int r );
    void            FindOrientation     ();


    void            ComputeInterpol1NN ( const TPointFloatI* ListSPI, double step, const TVolumeDoc* MRIGrey, const Volume& greyvol );
    void            ComputeInterpol4NN ( const TPointFloatI* ListSPI, double step, const TVolumeDoc* MRIGrey, const Volume& greyvol );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
