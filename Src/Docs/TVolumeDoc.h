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

#include    "TLimits.h"
#include    "TArray1.h"
#include    "Math.Histo.h"
#include    "Math.Stats.h"
#include    "Geometry.TPoint.h"

#include    "TExportVolume.h"
#include    "Geometry.TTriangleSurface.h"

#include    "TBaseDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr char*     CompanyNameAnalyze          = "Mayo Foundation";
constexpr char*     ProductNameAnalyze          = "Analyze";
constexpr char*     CompanyNameAVS              = "Advanced Visual Systems";
constexpr char*     ProductNameAVS              = "Field";
constexpr char*     CompanyNameNifti            = "NIMH";
constexpr char*     ProductNameNifti            = "Nifti";
constexpr char*     CompanyNameVMR              = "BrainVoyager";
constexpr char*     ProductNameVMR              = "VMR";


constexpr int       MRIDOC_NUMSPACES            = 1;

                                        // overload extra content types
enum                MriContentType
                    {
                    UnknownMriContentType       = 0x0000,
                                        // main category
                    MriContentTypeFullHead      = 0x0001,
                    MriContentTypeSegmented     = 0x0002,
                    MriContentTypeMask          = MriContentTypeFullHead    | MriContentTypeSegmented,

                                        // the following sub-types are all exclusive, specialized version of segmented type:
                    MriContentTypeBinaryMask    = 0x0010,
                    MriContentTypeRoiMask       = 0x0020,
                    MriContentTypeGrey          = 0x0100,   // assume these are masks
                    MriContentTypeWhite         = 0x0200,   // assume these are masks
                    MriContentTypeMaskMask      = MriContentTypeBinaryMask  | MriContentTypeRoiMask
                                                | MriContentTypeGrey        | MriContentTypeWhite,

                    MriContentTypeBrain         = 0x0400,
                    MriContentTypeBlob          = 0x1000,
                    MriContentTypeRis           = 0x2000,
                    MriContentTypeSegmentedMask = MriContentTypeMaskMask
                                                | MriContentTypeBrain       | MriContentTypeBlob        | MriContentTypeRis,

                    NumMriContentType           = 10,
                    };


extern const char   MriContentTypeNames[ NumMriContentType ][ ContentTypeMaxChars ];


inline  bool        IsUnknownMriType    ( int ect )     { return ect == UnknownMriContentType;  }
inline  bool        IsFullHead          ( int ect )     { return ect & MriContentTypeFullHead;  }
inline  bool        IsSegmented         ( int ect )     { return ect & MriContentTypeSegmented; }
inline  bool        IsMask              ( int ect )     { return ect & MriContentTypeMaskMask;  }
inline  bool        IsBinaryMask        ( int ect )     { return ect & MriContentTypeBinaryMask;}
inline  bool        IsRoiMask           ( int ect )     { return ect & MriContentTypeRoiMask;   }
inline  bool        IsGrey              ( int ect )     { return ect & MriContentTypeGrey ;     }
inline  bool        IsWhite             ( int ect )     { return ect & MriContentTypeWhite;     }
inline  bool        IsBrain             ( int ect )     { return ( ect & MriContentTypeBrain ) || IsSegmented ( ect ) && ! ( ect & ( MriContentTypeSegmentedMask & ~MriContentTypeBrain ) ); }  // brain is not always set, let's guess from the other flags
inline  bool        IsBlob              ( int ect )     { return ect & MriContentTypeBlob;      }
inline  bool        IsRis               ( int ect )     { return ect & MriContentTypeRis;       }


char*               GetMriContentTypeName   ( int ect, char* s );
void                ShowMriContentType      ( int ect, const char* title = 0 );


//----------------------------------------------------------------------------

enum                MriInitFlags
                    {
                    MriInitBackgroundKeep       = 0x0001,
                    MriInitBackgroundDetect     = 0x0002,

                    MriInitIsoSurfaceCutKeep    = 0x0004,   // this flag is not currently in use, as it produces inconsistent results in the clipping planes
                    MriInitIsoSurfaceCutDetect  = 0x0008,

                    MriInitTypeKeep             = 0x0010,
                    MriInitTypeDetect           = 0x0020,

                    MriInitReorientKeep         = 0x0100,
                    MriInitReorientDetect       = 0x0200,


                    MriInitDetect               = MriInitBackgroundDetect | MriInitIsoSurfaceCutDetect | MriInitTypeDetect | MriInitReorientDetect,
                    MriInitKeep                 = MriInitBackgroundKeep   | MriInitIsoSurfaceCutKeep   | MriInitTypeKeep   | MriInitReorientKeep,
                    };


//----------------------------------------------------------------------------

class   TVolumeDoc  :   public  TBaseDoc,
                        public  TLimits
{
public:
                    TVolumeDoc ( owl::TDocument *parent = 0 );

                                        // owl::TDocument
    bool            InitDoc ();
    bool            Close   ();
    bool            IsOpen  ()              const       { return  (bool) Data; }
    bool            Revert  ( bool force = false );

                                        // TLimits
    void            InitLimits      ( InitType how )    final;

                                        // TVolumeDoc
    void            ResetThenSetMri ( MriInitFlags flags = MriInitDetect, double backgroundvalue = 0 );


    void                        GetSize ( TVector3Int& s ) const{ s.X = Size.GetXExtent (); s.Y = Size.GetYExtent (); s.Z = Size.GetZExtent (); }
    const TBoundingBox<int>*    GetSize             ()  const   { return &Size;     }
    const TBoundingBox<double>* GetBounding         ()  const   { return Bounding;  }
    TVector3Double              GetVoxelSize        ()  const   { return VoxelSize; }
    TVector3Double              GetRealSize         ()  const   { return RealSize;  }
    TPointDouble                GetOrigin           ()  const   { return Origin;    }
    TPointDouble                GetBarycenter       ()  const   { return Data.GetBarycenter ( BackgroundValue );    }
    TPointDouble                GetBoundingCenter   ()  const   { return Bounding->GetCenter ();                    }
    TPointDouble                GetDefaultCenter    ()  const   { return GetBoundingCenter () /*GetBarycenter ()*/; }   // used as fallback when Origin is not set

                                        // Absolute coordinates account for the Origin shift
    void            ToAbs        ( GLfloat* p )         const   { p[ 0 ] = (GLfloat) ( p[ 0 ] - Origin.X ); p[ 1 ] = (GLfloat) ( p[ 1 ] - Origin.Y ); p[ 2 ] = (GLfloat) ( p[ 2 ] - Origin.Z ); }
    void            ToAbs        ( double*  p )         const   { p[ 0 ] -= Origin.X; p[ 1 ] -= Origin.Y; p[ 2 ] -= Origin.Z; }
    void            ToAbs        ( TPoints& points )    const   { points -= Origin; }
                                        // Relative coordinates are equivalent to Voxel coordinates
    void            ToRel        ( GLfloat* p )         const   { p[ 0 ] = (GLfloat) ( p[ 0 ] + Origin.X ); p[ 1 ] = (GLfloat) ( p[ 1 ] + Origin.Y ); p[ 2 ] = (GLfloat) ( p[ 2 ] + Origin.Z ); }
    void            ToRel        ( double*  p )         const   { p[ 0 ] += Origin.X; p[ 1 ] += Origin.Y; p[ 2 ] += Origin.Z; }
    void            ToRel        ( TPoints& points )    const   { points += Origin; }

          Volume*   GetData         ()                          { return &Data; }
    const Volume*   GetData         ()                  const   { return &Data; }
    int             GetFirstSlice   ( int s )           const   { return (int) Bounding->Min ( s ); }
    int             GetLastSlice    ( int s )           const   { return (int) Bounding->Max ( s ); }

    bool            HasLabels       ()                  const   { return (int) Labels > 1; }    // not accounting for any potential label for null value
    const TStrings& GetLabels       ()                  const   { return Labels; }
    const char*     GetLabel        ( int i )           const   { return IsInsideLimits ( i, 0, (int) Labels - 1 ) ? Labels[ i ] : 0; }

    const TTriangleSurface* GetTessSurfacePos   ()      const   { return &TessSurfacePos; }
    const TTriangleSurface* GetTessSurfaceNeg   ()      const   { return &TessSurfaceNeg; }

    double          GetBackgroundValue ()               const   { return BackgroundValue; }
    double          GetIsoSurfaceCut   ()               const   { return IsoSurfaceCut; }
    double          GetCsfCut          ()               const;
    int             GetIsoDownsampling ()               const   { return IsoDownsampling; }
    double          GetBackthresholdAlpha ()            const   { return IsMask () || AbsMaxValue == 0 ? 0 : IsoSurfaceCut / AbsMaxValue; } // alpha > threshold


    bool            IsUnknownMriType    ()              const   { return crtl::IsUnknownMriType    ( ExtraContentType );    }
    bool            IsFullHead          ()              const;  // doing some more sophisticated exploration
    bool            IsSegmented         ()              const   { return crtl::IsSegmented         ( ExtraContentType );    }
    bool            IsMask              ()              const   { return crtl::IsMask              ( ExtraContentType );    }
    bool            IsBinaryMask        ()              const   { return crtl::IsBinaryMask        ( ExtraContentType );    }
    bool            IsRoiMask           ()              const   { return crtl::IsRoiMask           ( ExtraContentType );    }
    bool            IsBrain             ()              const   { return crtl::IsBrain             ( ExtraContentType );    }
    bool            IsBlob              ()              const   { return crtl::IsBlob              ( ExtraContentType );    }
    bool            IsRis               ()              const   { return crtl::IsRis               ( ExtraContentType );    }
    bool            IsTemplate          ()              const;
    const char*     GetContentTypeName  ( char* s )     const;
    int             GetNiftiTransform   ()              const   { return NiftiTransform; }
    int             GetNiftiIntentCode  ()              const   { return NiftiIntentCode; }
    const char*     GetNiftiIntentName  ()              const   { return NiftiIntentName; }
          char*     GetNiftiIntentName  ()                      { return NiftiIntentName; }


    void            Flip                ( owlwparam wparam );
    void            SetNewOrigin        ();
    void            SetNewOrigin        ( const TPointDouble& origin );
    bool            IsValidOrigin       ()                              const;
    bool            IsValidOrigin       ( const TPointDouble &origin )  const;
    bool            HasKnownOrientation ()                              { return KnownOrientation; }
    void            SetKnownOrientation ( bool o )                      { KnownOrientation = o; }
    void            Estimate1010FromFiducials   ( const TPointFloat& nasion, const TPointFloat& inion, const TPointFloat& lpa, const TPointFloat& rpa, TPoints& tentenpositions, TStrings&    tentennames ) const;
    void            SetIsoSurface       ();
    void            NewIsoSurfaceCut    ( bool automatic, double bt );
    void            NewIsoDownsampling  ( int ds = -1 );
    void            Filter              ( FilterTypes filtertype );
    bool	        ExportDownsampled   ( char* path, int downsampling = 1 )    const;  // !function could update the file extension!
    bool	        ExportSlice         ( const char* path, int thickness )     const;
    bool	        ExportSphericalHistogram    ( const char* path )            const;


protected:
    TBoundingBox<int>       Size;               // size of data in voxels, in integer
    TBoundingBox<double>*   Bounding;           // boundary of non-null data - pointer to a TDisplaySpace.Bounding object
    Volume                  Data;

    TVector3Double  VoxelSize;          // size of 1 voxel
    TVector3Double  RealSize;           // in [mm]
    TPointDouble    Origin;             // voxel of origin

    bool            KnownOrientation;   // if format includes the orientation

    double          BackgroundValue;    // The background value is a threshold separating the useful data from the background noise
    double          IsoSurfaceCut;      // The isosurface cut is also a threshold but only used to tell which part to display (initially set to the background value, though)
    int             IsoDownsampling;

    TStrings        Labels;             // Useful for segmented tissues
                                        // Nifti useful infos
    int             NiftiTransform;     // qform_code / sform_code holding the meaning of the transform
    int             NiftiIntentCode;
    char            NiftiIntentName[ 16 ];


    TTriangleSurface  TessSurfacePos;
    TTriangleSurface  TessSurfaceNeg;


    virtual void    ResetMri    ();
    virtual void    SetMri      ( MriInitFlags flags = MriInitDetect, double backgroundvalue = 0 );

    void            SetBackgroundValue          ();
    void            SetIsoSurfaceCut            ();
    void            SetBounding                 ( double backvalue );
    void            SetContentType              ();
    void            SetGeometryTransform        ();
    void            SetLabels                   ();
    void            FindOrientation             ();
    void            FindOrientationCommon       ( Volume& vol, TPointFloat& center, double& back, TPointInt& boundmin, TPointInt& boundmax );
    void            FindOrientationFullHead     ();
    void            FindOrientationSegmented    ();

    bool            ResliceData     (   TMatrix44&    transform,
                                        TPointDouble&       origin,
                                        const TPointDouble& voxelsize,
                                        int                 niftitransform,     int                 niftiintentcode,    const char* niftiintentname,
                                        const char*         title
                                    );

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
