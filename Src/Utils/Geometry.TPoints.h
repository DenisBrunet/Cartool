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
#include    "TSelection.h"
#include    "Strings.TStrings.h"
#include    "TArray2.h"
#include    "TList.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum                                NeighborhoodType;
template <class TypeD> class        TVolume;


//----------------------------------------------------------------------------
                                        // Geometry of a set of points
enum                {
                    PointsGeometryUnknown       = 0,
                    PointsGeometryIrregular,
                    PointsGeometryGridNotAligned,   // 2 sub-cases for grid
                    PointsGeometryGridAligned,

                    NumPointsGeometry
                    };

extern  char        PointsGeometryNames[ NumPointsGeometry ][ 32 ];


inline  bool        IsGeometryUnknown       ( int gt )      { return gt == PointsGeometryUnknown; }
inline  bool        IsGeometryIrregular     ( int gt )      { return gt == PointsGeometryIrregular; }
inline  bool        IsGeometryGrid          ( int gt )      { return gt == PointsGeometryGridNotAligned || gt == PointsGeometryGridAligned; }
inline  bool        IsGeometryGridNotAligned( int gt )      { return gt == PointsGeometryGridNotAligned;}
inline  bool        IsGeometryGridAligned   ( int gt )      { return gt == PointsGeometryGridAligned;}


//----------------------------------------------------------------------------

class               TVolumeDoc;
class               TMatrix44;

                                        // Thread-safe list of points
class   TPoints
{
public:
                    TPoints ();
                    TPoints ( int numpoints );
                    TPoints ( const char* file, TStrings* names = 0 )   { ReadFile ( file, names ); }


    size_t          AtomSize            ()  const           { return  sizeof ( TPointFloat ); }
    size_t          MemorySize          ()  const           { return  NumPoints * AtomSize (); }

    int             GetNumPoints        ()  const           { return  NumPoints; }
    bool            IsEmpty             ()  const           { return  NumPoints == 0; }
    bool            IsNotEmpty          ()  const           { return  NumPoints != 0; }

    void            Reset               ();
    void            Resize              ( int newnumpoints );

    void            Set                 ( const TPoints& points );
    void            Set                 ( const TPoints& points, const int* indexes, int numindexes );
    void            SetCorners          ( double xmin, double ymin, double zmin, double xmax, double ymax, double zmax );
    void            SetLineOfPoints     ( TPointFloat fromp, TPointFloat top, bool includingextrema, int subsampling = 1 );
    void            SetSphericalSurfaceEqui ( double deltaangle );  // !caller beware to type!
    void            SetSphericalSurfaceEqui ( int numpoints );      // !caller beware to type!
    void            SetSphericalSurfaceRnd  ( int numpoints );
    void            SetSphericalSpiral      ( int numpoints );

    void            Add                 ( const TPointFloat&  point );
    void            Add                 ( const TPointDouble& point )   { Add ( TPointFloat ( point ) ); }
    void            Add                 ( float x, float y, float z )   { Add ( TPointFloat ( x, y, z ) ); }
    void            Add                 ( const TPoints& points );
    void            Add                 ( const Volume& volume, double x0, double y0, double z0, double scale );
    void            AddNoDuplicates     ( const TPointFloat& point );
    void            AddNoDuplicates     ( float x, float y, float z )   { AddNoDuplicates ( TPointFloat ( x, y, z ) ); }
    void            AddNoDuplicates     ( const TPoints& points );

    void            RemoveLast          ( int howmany = 1 );

    const TPointFloat*  Contains        ( const TPointFloat& point )const;

    double          GetMinimumDistance  ( const TVector3Float& p )  const;
    TVector3Float   GetClosestPoint     ( const TVector3Float& p )  const;
    double          GetMedianDistance   ()  const;
    void            GetNeighborsIndexes ( TArray2<int>&  neighbi, NeighborhoodType neightype, double mediandist = 0 )   const;
    TPointFloat     GetCenter           ()  const;
    double          GetRadius           ()  const;
    TPointFloat     GetLimitMin         ()  const;
    TPointFloat     GetLimitMax         ()  const;
    TPointFloat     GetMeanPoint        ()  const;
    TPointFloat     GetMeanPoint        ( const char* selpoints, const TStrings&    pointnames )   const;   // selected regular average point
    TPointFloat     GetMedianPoint      ( bool strictvalue = true ) const;                                  // median of all coordinates taken separately
    TPointFloat     GetMedoidPoint      ()  const;
    TPointFloat     GetModalPoint       ()  const;                                                          // most probable position (although not existing point)


    int             GetGeometryType     ()  const;
    bool            IsGrid              ()  const           { return  IsGeometryGrid            ( GetGeometryType () ); }
    bool            IsGridNotAligned    ()  const           { return  IsGeometryGridNotAligned  ( GetGeometryType () ); }
    bool            IsGridAligned       ()  const           { return  IsGeometryGridAligned     ( GetGeometryType () ); }
//  bool            IsHead              ()  const           { return  ! IsGeometryGrid          ( GetGeometryType () ); }   // is this reliable enough?


    void            DownsamplePoints    ( int targetsize );
    void            GetSurfacePoints    ( const Volume& volume, const TPointFloat& center, bool includebottom = true );
    void            Invert              ();
    void            KeepTopHeadPoints   ( const TVolumeDoc& mridoc, TVector3Float centertranslate, bool rebalance, const TMatrix44* mriabstoguillotine = 0 );
    void            Normalize           ();
    void            ResurfacePoints     ( const Volume& surface, const Volume& gradient, TPointFloat center, const TMatrix44* mriabstoguillotine = 0, double inflating = 0 );

    void            Sort                ();     // results in decreasing order


    void            Show                ( const char *title = 0 )   const;
    bool            ReadFile            ( const char* file, TStrings*    names = 0 );     // xyz sxyz els spi loc
    void            WriteFile           ( const char* file, const TStrings*    names = 0, TSelection* exclsel = 0, int numpoints = -1, ClusterType type = Cluster3D )   const;
    void            ExtractToFile       ( const char* file, const char* exclpoints, TStrings*    names /*, bool removeselection*/ ) const;


    TPointFloat&        operator    []              ( int i )       { return Points[ i ]; }
    const TPointFloat&  operator    []              ( int i ) const { return Points[ i ]; }
                    operator        bool            ()        const { return NumPoints != 0; }
                    operator        int             ()        const { return NumPoints; }
                    operator        TPointFloat*    ()              { return Points.data (); }
                    operator        const TPointFloat*  ()    const { return Points.data (); }


                    TPoints                         ( const TPoints&        op  );
    TPoints&        operator        =               ( const TPoints&        op2 );
    TPoints&        operator        =               ( const Volume&         op2 );

    TPoints         operator        +               ( const TPoints&        op2 )   { TPoints temp ( *this ); temp += op2; return temp; }   // concatenation

    TPoints&        operator        +=              ( double                op2 );
    TPoints&        operator        +=              ( const TPointFloat&    op2 );
    TPoints&        operator        +=              ( const TPointDouble&   op2 );
    TPoints&        operator        +=              ( const TPoints&        op2 );  // concatenation
    TPoints&        operator        -=              ( double                op2 );
    TPoints&        operator        -=              ( const TPointFloat&    op2 );
    TPoints&        operator        -=              ( const TPointDouble&   op2 );
    TPoints&        operator        /=              ( double                op2 );
    TPoints&        operator        /=              ( const TPointFloat&    op2 );
    TPoints&        operator        /=              ( const TArray1<float>& op2 );  // element-wise operation
    TPoints&        operator        *=              ( double                op2 );
    TPoints&        operator        *=              ( const TPointFloat&    op2 );


protected:

    int                         NumPoints;
    std::vector<TPointFloat>    Points;
};


//----------------------------------------------------------------------------
                                        // Data structure used to save multiple sets of points into a single file
struct  ClusterPoints
{
    char                ClusterName[ 256 ];
    ClusterType         ClusterType;
    TPoints             Points;
    TStrings            Names;
};


void                WritePoints  ( const char* file, const std::vector<ClusterPoints>& clusters );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
