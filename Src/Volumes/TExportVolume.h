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

#include    "Files.Utils.h"

#include    "Volumes.AnalyzeNifti.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // This class is used to output common Volume files

#define             ExportVolumeTypes               FILEEXT_MRINII " " FILEEXT_MRIAVW_HDR " " FILEEXT_MRIAVS " " FILEEXT_MRIVMR

#define             ExportVolumeDefaultType         DefaultMriExt

                                        // Nifti defines
#define             ExportVolumeSpaceUnits          NIFTI_UNITS_MM

#define             ExportVolumeTimeUnits           NIFTI_UNITS_UNKNOWN


#define             ExportOrientation               "Orientation="

#define             ExportVolumeDefaultAtomFormat   AtomFormatFloat

#define             NumExportVolumesFormatTypes     2
                                        // The only types currently allowed for writing
extern  const AtomFormatType    ExportVolumesFormatTypes [ NumExportVolumesFormatTypes ];


enum                ExportArrayOrder
                    {
                    ExportArrayOrderZYX,    // usual option: most inner loop dimension is X, most outer loop dimension is Z
                    ExportArrayOrderXYZ,
                    };


//----------------------------------------------------------------------------
                                        // Centralized function to optimally & safely pick the output volume type
                                        // Maybe merge in CheckVolumeFormat?
template <class TypeD>
AtomFormatType      GetVolumeAtomType   (   const TVolume<TypeD>*   datain          = 0,                    // in case we come from a known volume
                                            FilterTypes             filtertype      = FilterTypeNone,   InterpolationType   interpolate     = InterpolateUnknown,   // in case we are transforming a volume
                                            const char*             ext             = 0                     // not all types are being allowed according to the output file type
//                                          AtomFormatType          suggestedtype   = UnknownAtomFormat     // in case we want confirmation from an existing type
                                        );


template <class TypeD>
AtomFormatType      GetVolumeAtomType   (   const TVolume<TypeD>*   datain          ,                       // in case we come from a known volume
                                            FilterTypes             filtertype      ,   InterpolationType   interpolate     ,   // in case we are transforming a volume
                                            const char*             ext                                     // not all types are being allowed according to the output file type
//                                          AtomFormatType          suggestedtype                           // in case we want confirmation from an existing type
                                        )
{
AtomFormatType      atomtype        = ExportVolumeDefaultAtomFormat;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Most of the time, we have an input volume
                                        // Other cases are generated volumes, in this case caller can suggest its preference with suggestedtype 
if ( datain ) {

    if ( typeid (TypeD) == typeid (UCHAR) ) {
                                        // technically, it can only holds bytes
        atomtype        = AtomFormatByte;

        }
    else {
                                        // Major choice is first done here
        MriType         minvalue        = datain->GetMinValue ();
        MriType         maxvalue        = datain->GetMaxValue ();
        bool            isint           = datain->IsInteger   ();

                                        // masks & (most of) ROIs
        if ( IsInsideLimits ( minvalue, maxvalue, (MriType) 0, (MriType) UCHAR_MAX )
          && isint
            )

            atomtype    = AtomFormatByte;

        else                            // all the other cases go to this fallback
            atomtype    = AtomFormatFloat;
        } // not ubyte
    }

//else
//  atomtype        = suggestedtype != UnknownAtomFormat    ? suggestedtype
//                                                          : ExportVolumeDefaultAtomFormat;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // TransformAndSave case
if ( interpolate != InterpolateUnknown ) {

    bool    transformint    = (    filtertype == FilterTypeMedian       // only these filters preserve integers
                                || filtertype == FilterTypeMin          
                                || filtertype == FilterTypeMax          
                                || filtertype == FilterTypeMinMax
                                || filtertype == FilterTypeNone     )   // no sub-sampling case, value are being copied

                            && IsNotFractionning ( interpolate );       // only a few interpolations will not create intermediat values

                                        // according to the transformation, we may have to change the output type from integer to float
    if      ( IsFormatInteger ( atomtype ) && ! transformint )

        atomtype    = AtomFormatFloat;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Finally, not all output files support all atom types
if ( StringIsEmpty ( ext ) && StringIs ( DefaultMriExt, FILEEXT_MRINII )
  || StringIs ( ext, FILEEXT_MRINII ) ) {
                                        // !other types allowed later on!
    if ( ! ( atomtype == AtomFormatByte || atomtype == AtomFormatFloat ) )

        atomtype    = ExportVolumeDefaultAtomFormat;
    }
else if ( StringIs ( ext, FILEEXT_MRIVMR )
       || StringIs ( ext, FILEEXT_MRIAVS ) ) {
                                        // only byte type for these files
        atomtype    = AtomFormatByte;
    }
else { // if ( StringIs ( ext, FILEEXT_MRIAVW_HDR ) ) {
                                        // only ubyte and float
    if ( ! ( atomtype == AtomFormatByte || atomtype == AtomFormatFloat ) )

        atomtype    = ExportVolumeDefaultAtomFormat;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//char            buff[ 1024 ];
//StringCopy  ( buff, "Min Max IsInt Filt+Intrp Ext:", ext, " -> ", AtomFormatTypePresets[ atomtype ].Text );
//DBGV5 ( datain ? datain->GetMinValue () : 0, datain ? datain->GetMaxValue () : 0, datain ? datain->IsInteger ( 9973 ) : 0, filtertype, interpolate, buff );

return  atomtype;
}


//----------------------------------------------------------------------------

constexpr int       ExportTypeSize          = 16;


class   TExportVolume
{
public:
                    TExportVolume ( AtomFormatType volumeformat = ExportVolumeDefaultAtomFormat );
                   ~TExportVolume ();

                                        // most fields have to be filled
    TFileName       Filename;
    char            Type        [ ExportTypeSize ];
    AtomFormatType  VolumeFormat;

                                        // Optional variables
    int             Dim         [ 4 ];
    TVector3Double  VoxelSize;          // size of 1 voxel
    TVector3Double  RealSize;           // in [mm]
    TPointDouble    Origin;             // in voxel, Origin = -Translation
    char            Orientation [ 4 ];  // 3 optional chars that tells the axis meaning (like SAR, PIR...)
    double          MaxValue;

                                        // Nifti specials
    int             NiftiTransform;     // qform_code / sform_code holding the meaning of the transform
    int             NiftiIntentCode;    // depending of the code, some of the parameters below should be set
    double          NiftiIntentParameters[ 3 ];
    char            NiftiIntentName      [ NiftiIntentNameSize ];

                                        // Either create a new object each time: calling Write will do the all the job
                                        // Or use 1 object multiple times, then call in sequence: Reset, Begin, Write, End
    void            Reset               ();
    bool            IsOpen              ()      const   { return  of != 0; }
    void            CheckVolumeFormat   ();


    void            Begin               ();     // checks, create stream, call write header - Called automatically
    void            WriteHeader         ();     // separate the function, in case we need to call it more than once (overwrite/update)

//  void            Write               ( int    value );                                               // off: a TVolume could cast itself to int and call this
    void            Write               ( double value );
    void            Write               ( const TVolume<UCHAR>&  vol, ExportArrayOrder arrayorder );    // write a full array at once
    void            Write               ( const TVolume<UINT>&   vol, ExportArrayOrder arrayorder );
    void            Write               ( const TVolume<float>&  vol, ExportArrayOrder arrayorder );
    void            Write               ( const TVolume<double>& vol, ExportArrayOrder arrayorder );
    
    void            End                 ();                 // close stream - Called automatically


                    operator std::ofstream* ()          { return of; }

protected:

    std::ofstream*  of;
    bool            DoneBegin;
    LONGLONG        EndOfHeader;
    LONGLONG        CurrentPosition;
    size_t          AtomSize;


    void            PreFillFile         ();
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
