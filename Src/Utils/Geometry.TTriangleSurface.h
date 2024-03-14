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

#include    "Geometry.TVertex.h"
#include    "Geometry.TPoints.h"
#include    "Geometry.TTriangleNetwork.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum        IsosurfaceMethod
            {
            UnknownIsosurface,
            IsosurfaceMarchingCube,         // regular marching-cube isosurface - useful for continuous data
            IsosurfaceMinecraft,            // Minecraft-looking blocks         - useful for discrete data like masks
            IsosurfaceBox,                  // just a big box around the data   - useful to show all the data at once
            };

                                            // Bundling all the parameters needed to properly specify an iso-surface
class   TTriangleSurfaceIsoParam
{
public:

    inline              TTriangleSurfaceIsoParam ();


    IsosurfaceMethod    How;                // how to do the isosurface
    double              IsoValue;           // cutting value
    int                 Downsampling;       // of the given volume
    bool                UseOriginalVolume;  // as is, skipping copying and smoothing
    FilterTypes         SmoothData;         // smooth the data     - before isosurface
    bool                SmoothGradient;     // smooth the gradient - after isosurface; not really needed if already SmoothData
    TPointDouble        Rescale;            // the output (for non-isotropic volumes)
    TPointDouble        Origin;             // of final data
};


inline  TTriangleSurfaceIsoParam::TTriangleSurfaceIsoParam () 
{
How                 = UnknownIsosurface;
IsoValue            = 0;
Downsampling        = 1;
UseOriginalVolume   = true;
SmoothData          = FilterTypeNone;
SmoothGradient      = false;
Rescale             = 1;
Origin.Reset ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                                                // additional margin..
constexpr int           TesselationVolumeMargin     = 2         // ..for correct isosurface closure of border voxels
                                                    + 1;        // ..and for filtering

constexpr int           MaxVerticePerBlock          = 550000;   // enough for 1 block to contain all triangles for a 128^3 MRI
                                        
constexpr int           VerticeGranularity          = 6;        // max # of vertices that need to be added at each step - biggest value comes from Minecraft, 1 face = 2 triangles = 6 vertice
                                        
constexpr int           MaxBlocksOfVertice          = 128;      // 1 for 128^3 MRI, 18 for 256^3 MRI, 100 for 512^3 MRI


//----------------------------------------------------------------------------
                                        // Class that will generate all sorts of triangles surfaces to be used for drawing by OpenGL
                                        // Surfaces can be generated from volumes via different iso-surface methods,
                                        // or from lists of 3D points, as per electrodes setups.
class   TTriangleSurface
{
public:
    inline              TTriangleSurface ();
                                                // Volumes isosurfaces
    inline              TTriangleSurface ( Volume& dataorig, const TTriangleSurfaceIsoParam& isoparam );
                                                // Various points tesselations
    inline              TTriangleSurface ( const TPoints& listp, const TSelection& sel, int type, double *params );
                      
    inline             ~TTriangleSurface ();


    bool                IsEmpty                 ()  const   { return NumPoints == 0; }
    bool                IsNotEmpty              ()  const   { return NumPoints != 0; }

    inline void         Reset                   ();

    int                 GetNumPoints            ()  const   { return NumPoints;    }
    int                 GetNumTriangles         ()  const   { return NumTriangles; }
    const TArray1<D3DPointI>&   GetListPoints   ()  const   { return ListPoints; }
    const TPointFloat*  GetListPointsNormals    ()  const   { return (const TPointFloat*) ListPointsNormals;    }
    const TPointFloat*  GetListTriangles        ()  const   { return (const TPointFloat*) ListTriangles;        }
          TPointFloat*  GetListTriangles        ()          { return (TPointFloat*)       ListTriangles;        } // non-const version used to update triangles
    const TPointFloat*  GetListTrianglesNormals ()  const   { return (const TPointFloat*) ListTrianglesNormals; }
    const int*          GetListTrianglesIndexes ()  const   { return ListTrianglesIndexes.GetArray ();          }


    void                IsosurfaceFromVolume    ( Volume& dataorig, const TTriangleSurfaceIsoParam& isoparam );
    void                SurfaceThroughPoints    ( const TPoints& listp, const TSelection& sel, int type, double *params );


    inline              TTriangleSurface        ( const TTriangleSurface& op );
    inline              TTriangleSurface        ( const TTriangleSurface& op, const TPoints& altlist ); // Special copy - duplicates an existing surface while replacing only the positions
    inline TTriangleSurface&    operator=       ( const TTriangleSurface& op2 );

    inline TTriangleSurface&    operator+=      ( const TTriangleSurface& op2 );    // concatenating from another tesselation


protected:

    int                 NumPoints;              // = 3 * NumTriangles
    int                 NumTriangles;
    double              MeanDistance;

    TArray1<D3DPointI>  ListPoints;             // deallocated after init
    TPoints             ListPointsNormals;      // at each triangle vertex  3 x #Triangles
    TPoints             ListTriangles;          // list of vertices         3 x #Triangles
    TPoints             ListTrianglesNormals;   // 1 for each triangle      1 x #Triangles
    TArray1<int>        ListTrianglesIndexes;   // actually data associated with triangles, here an electrode #,  3 x #Triangles


    void                ComputeIsoSurfaceBox            (   const Volume*   data, 
                                                            TVertex**       listvert,   int         pointsperblock,     int&        currlistblock 
                                                        );
    void                ComputeIsoSurfaceMarchingCube   (   const Volume*   data, 
                                                            double          cutabove,
                                                            bool            smoothgradient, 
                                                            TVertex**       listvert,   int         pointsperblock,     int&        currlistblock 
                                                        );
    void                ComputeIsoSurfaceMinecraft      (   const Volume*   data, 
                                                            double          isovalue, 
                                                            TVertex**       listvert,   int         pointsperblock,     int&        currlistblock 
                                                        );


    void                ComputeDelaunay3D               ();
    void                ComputeLine                     ( const TPoints& listp, const TSelection& sel, double mindist );
    void                ComputePoint                    ( const TPoints& listp, const TSelection& sel, double mindist );
    void                ComputeTrianglesNormals         ();
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TTriangleSurface::TTriangleSurface ()
{
Reset ();
}

                                        // Triangles from volume iso-surface
        TTriangleSurface::TTriangleSurface ( Volume& dataorig, const TTriangleSurfaceIsoParam& isoparam )
{
Reset ();

IsosurfaceFromVolume ( dataorig, isoparam );
}

                                        // Triangles from list of 3D points (electrodes)
        TTriangleSurface::TTriangleSurface ( const TPoints& listp, const TSelection& sel, int type, double *params )
{
Reset ();

SurfaceThroughPoints ( listp, sel, type, params );
}


//----------------------------------------------------------------------------
        TTriangleSurface::~TTriangleSurface ()
{
                                        // not actually needed, arrays will just deallocate themselves
Reset ();
}


void    TTriangleSurface::Reset ()
{
NumPoints           = 0;
NumTriangles        = 0;
MeanDistance        = 0;

ListPoints          .DeallocateMemory ();
ListPointsNormals   .Reset ();
ListTriangles       .Reset ();
ListTrianglesNormals.Reset ();
ListTrianglesIndexes.DeallocateMemory ();
}


//------------------------------------------------------------------------------
                                        // Regular copy
                    TTriangleSurface::TTriangleSurface ( const TTriangleSurface& op )
{
NumPoints               = op.NumPoints;
NumTriangles            = op.NumTriangles;
MeanDistance            = op.MeanDistance;

ListPoints              = op.ListPoints;
ListPointsNormals       = op.ListPointsNormals;
ListTriangles           = op.ListTriangles;
ListTrianglesNormals    = op.ListTrianglesNormals;
ListTrianglesIndexes    = op.ListTrianglesIndexes;
}


//----------------------------------------------------------------------------
                                        // Special copy: keeping an existing topological structure (who is neighbor to who) while replacing the actual coordinates
                                        // Used to "project" a 3D tesselation to 2D points
                    TTriangleSurface::TTriangleSurface ( const TTriangleSurface& op, const TPoints& altlist )
{
Reset ();

NumPoints               = op.GetNumPoints    ();
NumTriangles            = op.GetNumTriangles ();
MeanDistance            = 0;

                                        // direct copy
ListPointsNormals       = op.ListPointsNormals;
ListTrianglesIndexes    = op.ListTrianglesIndexes;

                                        // generated
ListTriangles       .Resize ( NumPoints    );
ListTrianglesNormals.Resize ( NumTriangles );

                                        // generate normals to triangles
for ( int i=0; i < NumTriangles; i++ )

    ListTrianglesNormals[ i ] = TPointFloat ( 0, 0, -1 );

                                        // transpose the triangles
for ( int i=0; i < NumPoints; i++ )

    ListTriangles[ i ]  = altlist[ ListTrianglesIndexes[ i ] ];
}


//----------------------------------------------------------------------------
TTriangleSurface&   TTriangleSurface::operator= ( const TTriangleSurface& op2 )
{
                                        // in case of self assignation
if ( &op2 == this )
    return  *this;

                                        // destroy old arrays - not actually needed, as overwriting arrays work fine
Reset ();

NumPoints               = op2.NumPoints;
NumTriangles            = op2.NumTriangles;
MeanDistance            = op2.MeanDistance;

ListPoints              = op2.ListPoints;
ListPointsNormals       = op2.ListPointsNormals;
ListTriangles           = op2.ListTriangles;
ListTrianglesNormals    = op2.ListTrianglesNormals;
ListTrianglesIndexes    = op2.ListTrianglesIndexes;

return  *this;
}


//----------------------------------------------------------------------------
                                        // concatenate (& clean) everything, operands consistency is the sole responsability of caller
TTriangleSurface&   TTriangleSurface::operator+= ( const TTriangleSurface& op2 )
{
ListPointsNormals      += op2.ListPointsNormals;
ListTriangles          += op2.ListTriangles;
ListTrianglesNormals   += op2.ListTrianglesNormals;


if ( op2.ListPoints.IsAllocated () ) {

    ListPoints.ResizeDelta ( op2.NumPoints );

    CopyVirtualMemory ( &ListPoints[ NumPoints ], op2.ListPoints.GetArray (), op2.ListPoints.MemorySize () );
    }


if ( op2.ListTrianglesIndexes.IsAllocated () ) {

    ListTrianglesIndexes.ResizeDelta ( op2.NumPoints );

    CopyVirtualMemory ( &ListTrianglesIndexes[ NumPoints ], op2.ListTrianglesIndexes.GetArray (), op2.ListTrianglesIndexes.MemorySize () );
    }

                                        // new amounts of points & triangles
NumPoints              += op2.NumPoints;
NumTriangles           += op2.NumTriangles;
MeanDistance            = ( MeanDistance + op2.MeanDistance ) / 2;  // approximative


return  *this;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

}
