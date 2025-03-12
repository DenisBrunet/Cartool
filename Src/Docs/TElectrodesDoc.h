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

#include    "TSelection.h"
#include    "Geometry.TPoint.h"
#include    "Geometry.TPoints.h"
#include    "TArray2.h"

#include    "Geometry.TTriangleSurface.h"

#include    "TBaseDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr int       ElectrodesNumSpaces         = DisplaySpaceHeight + 1;
constexpr int       ElectrodesSliceSize         = 110;
constexpr int       ElectrodesClusterMaxName    = 32;
constexpr char*     ElectrodeDefaultName        = "ElName?";

                                        // Electrodes can be grouped by smaller clusters, f.ex. 1 grid + 2 stripes + 5 single electrodes -> 3 clusters
class   TElectrodesCluster
{
public:
                            TElectrodesCluster ()           { Type = ClusterTypeUnknown; NumElectrodes = 0; ClearString ( Name, ElectrodesClusterMaxName ); }

    ClusterType             Type;
    int                     NumElectrodes;
    char                    Name[ ElectrodesClusterMaxName ];
    TBoundingBox<double>    Bounding;
};


//----------------------------------------------------------------------------
                                        // Electrodes extra content types
enum                {
                    UnknownElContentType,
                    ElContentTypeSurface,
                    ElContentTypeGrid,
                    ElContentTypeComplex,

                    NumElContentType
                    };

extern const char   ElelectrodesContentTypeNames[ NumElContentType ][ ContentTypeMaxChars ];


//----------------------------------------------------------------------------

class   TElectrodesDoc  :   public TBaseDoc
{
public:
                            TElectrodesDoc ( owl::TDocument *parent = 0 );


    bool                    InitDoc         ()                  final;


    bool                    IsGrid          ()                  const   { return  ExtraContentType == ElContentTypeGrid; }
    bool                    IsHead          ()                  const   { return  ExtraContentType == ElContentTypeSurface; }


    int                     GetNumElectrodes()                  const   { return NumElectrodes; }
    TPoints&                GetPoints       ( int space )               { return DisplaySpaces[ space ].Points; }
    const TPoints&          GetPoints       ( int space )       const   { return DisplaySpaces[ space ].Points; }
    const TBoundingBox<double>* GetBounding ( int space )       const   { return (bool) DisplaySpaces ? & DisplaySpaces[ space ].Bounding : 0; }
    const TOrientationType* GetProjBoxSides ()                  const   { return ProjBoxSides; }
    void                    SetPoints       ( int space, const TPoints& points );   // force setting new coordinates - used during interactive dialogs


    void                    GetCoordinates ( int e, float* v, int space )                       const   { *( (TPointFloat *) v )  = GetPoints ( space )[ e ]; }
    void                    GetCoordinates ( int e, float& x, float& y, float& z, int space )   const   { const TPoints& sop = GetPoints ( space ); x = sop[ e ].X; y = sop[ e ].Y; z = sop[ e ].Z; }
    void                    GetCoordinates ( int e, double* v, int space )                      const   { const TPoints& sop = GetPoints ( space ); v[ 0 ] = sop[ e ].X; v[ 1 ] = sop[ e ].Y; v[ 2 ] = sop[ e ].Z; }


    double                  GetMedianDistance ( int space )     const   { return DisplaySpaces[ space ].MedianDistance; }
    double                  GetPointRadius    ( int space )     const   { return DisplaySpaces[ space ].PointRadius; }
    double                  GetProjEquator    ()                const   { return ProjEquator; }


    virtual const char*     GetElectrodeName    ( int e = 0 )   const   { return ElectrodesNames[ e ]; }
    virtual const char*     GetElectrodeName    ( const char* name, int* index = 0 );  // returned string with the proper case
    virtual const TStrings* GetElectrodesNames  ()              const   { return &ElectrodesNames; }


    int                     NearestElement   ( TPointFloat& from, int space, double dmax = 0 )  const;


          TTriangleSurface* GetSurfaceRendering ( int space )           { return    space == DisplaySpace3D ? &SurfaceRendering : /*DisplaySpaceProjected, DisplaySpaceHeight*/ &SurfaceRenderingProj; }
    const TTriangleSurface* GetSurfaceRendering ( int space )   const   { return    space == DisplaySpace3D ? &SurfaceRendering : /*DisplaySpaceProjected, DisplaySpaceHeight*/ &SurfaceRenderingProj; }

    void                    GetNeighborhoodIndexes      ( TArray2<int>&     neighborhood )                  const;  // List of all Delaunay indexes
    void                    GetNeighborhoodDistances    ( TArray2<double>&  neighborhood )                  const;  // Matrix of all distances, not sorted
    void                    GetNeighborhoodDistances    ( TArray3<double>&  neighborhood, int maxnumneigh ) const ; // Only needed distances + indexes, sorted increasing


    int                     GetNumClusters      ()              const   { return NumClusters;   }
    const TElectrodesCluster&   GetCluster      ( int c = 0 )   const   { return Clusters[ c ]; }
    const TSelection&       GetBadElectrodes    ()              const   { return BadElectrodes; }
    void                    SetBadElectrodes    ( const TSelection& sel, bool notify = true );


    void                    ExtractToFile   ( const char* xyzfile, const char* selectionstring, bool removeselection )  const;
    virtual void            ExtractToFile   ( const char* xyzfile, TSelection selection, bool removeselection )         const = 0;  // !copy TSelection is on purpose!
    void                    Flip            ( owlwparam wparam );


protected:

    int                     NumElectrodes;
    TStrings                ElectrodesNames;
    double                  ProjEquator;

    int                     NumClusters;                // electrode grouping
    TElectrodesCluster*     Clusters;
    int*                    ClusterIndexes;             // tells which cluster for each electrode
    TSelection              BadElectrodes;

    TTriangleSurface        SurfaceRendering;
    TTriangleSurface        SurfaceRenderingProj;
    TOrientationType        ProjBoxSides[ NumBoxSides ];// orientation labels for projection


    virtual void            Reset ( bool close = false );
    virtual void            Set   ();

    void                    ElectrodesNamesCleanUp  ();
    void                    ProjectElectrodes       ();
    void                    SetBounding             ( int space );
    void                    SetMedianDistance       ( int space );
    virtual void            InitContentType         ();
    void                    GetDimOfGrid            ( int c, double *dims ) const;
    void                    FindOrientation         ();
    void                    SetTesselation          ();
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
