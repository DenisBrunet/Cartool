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

#include    <owl/pch.h>
#include    <fstream>

#include    "Strings.Utils.h"
#include    "Strings.Grep.h"
#include    "Dialogs.TSuperGauge.h"
#include    "Math.Resampling.h"
#include    "TVolume.h"

#include    "TExportVolume.h"
#include    "Volumes.AnalyzeNifti.h"

#include    "TVolumeNiftiDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TVolumeNiftiDoc::TVolumeNiftiDoc ( TDocument* parent )
      : TVolumeDoc ( parent )
{
}


//----------------------------------------------------------------------------
bool	TVolumeNiftiDoc::Commit ( bool force )
{
if ( ! ( IsDirty () || force ) )
    return true;


if ( Data.IsNotAllocated () ) {

    SetDirty ( false );
    return true;
    }


/*                                        // Saving the central slice instead of the whole volume
if ( GetAnswerFromUser ( "Saving the central slice?", ToFileName ( GetDocPath () ) ) ) {

    TFileName           _file;
    StringCopy ( _file, GetDocPath () );

    PostfixFilename ( _file, ".SagittalSlice" );

    int                 thickness;

    if ( ! GetValueFromUser ( "Thickness of slice:", "Export Sagittal Slice", thickness, "1" ) )
        return  false;

    ExportSlice ( _file, thickness );

    return  true;
    }
*/


TExportVolume       expvol;

                                        // try to open header and image files
StringCopy          ( expvol.Filename, GetDocPath () );

CheckMriExtension   ( expvol.Filename );

SetDocPath          ( expvol.Filename );


expvol.VolumeFormat     = GetVolumeAtomType ( &Data, FilterTypeNone, InterpolateUnknown, ToExtension ( expvol.Filename ) );

expvol.MaxValue         = Data.GetAbsMaxValue ();

expvol.Dimension.X      = Data.GetDim1 ();
expvol.Dimension.Y      = Data.GetDim2 ();
expvol.Dimension.Z      = Data.GetDim3 ();

expvol.VoxelSize        = VoxelSize;

expvol.Origin           = Origin;

                                        // Do we have some knowledge here of the type of data?
expvol.NiftiTransform   = NiftiTransform;   // ?maybe some filtering should reset that?

expvol.NiftiIntentCode  = NiftiIntentCode;

StringCopy  ( expvol.NiftiIntentName, NiftiIntentName, NiftiIntentNameSize - 1 );


//if ( KnownOrientation )               // always saving orientation?
    OrientationToString ( expvol.Orientation );


expvol.Write ( Data, ExportArrayOrderZYX );


SetDirty ( false );

return  true;
}


//----------------------------------------------------------------------------
bool	TVolumeNiftiDoc::Open	(int /*mode*/, const char* path)
{
if ( path )
    SetDocPath ( path );

SetDirty ( false );


if ( GetDocPath () ) {

    TFileName           fileinhdr;
    TFileName           fileinimg;


    AnalyzeFamilyType   type            = GetAnalyzeType ( GetDocPath (), fileinhdr, fileinimg );


    if ( type == UnknwonAnalyze )

        return  false;


    if ( ! IsNifti ( type ) ) { // this should be resolved before, but check it again
        ShowMessage ( "This doesn't seem to be a Nifti file.\nMaybe try to open as a Analyze7.5 file maybe?", NiftiTitle, ShowMessageWarning ); 
        return  false;
        }


    bool                swap            = type & SwapData;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Read header
    SetDocPath ( fileinhdr );


    ifstream            fih ( fileinhdr, ios::binary );

    int                 dim     [ 8 ];
    int                 intent_code;
    char*               intent_name;
    char*               descrip;
    int                 datatype        = NIFTI_TYPE_UNKNOWN;
    double              pixdim  [ 8 ];
    double              toffset;
//  int                 xyzt_units;
    double              scl_slope;      // y = scl_slope * x + scl_inter
    double              scl_inter;
//  double              cal_min;        // intended display scaling
//  double              cal_max;
    int                 qform_code;
    double              qa;
    double              qb;
    double              qc;
    double              qd;
    double              qfac;
    double              qox;
    double              qoy;
    double              qoz;
    double              srx[4] ;
    double              sry[4] ;
    double              srz[4] ;
    int                 sform_code;
    ulong               vox_offset;


    if      ( IsNifti1 ( type ) ) {

        struct nifti_1_header   headernii;

        fih.read ( (char *) &headernii, sizeof ( headernii ) );


        dim[ 0 ]    = SwapBytes ( headernii.dim[ 0 ],        swap );
        dim[ 1 ]    = SwapBytes ( headernii.dim[ 1 ],        swap );
        dim[ 2 ]    = SwapBytes ( headernii.dim[ 2 ],        swap );
        dim[ 3 ]    = SwapBytes ( headernii.dim[ 3 ],        swap );
        dim[ 4 ]    = SwapBytes ( headernii.dim[ 4 ],        swap );

        intent_code = SwapBytes ( headernii.intent_code,     swap ); // predefined codes
        intent_name =             headernii.intent_name;             // more freedom to put something here
        descrip     =             headernii.descrip;

        datatype    = SwapBytes ( headernii.datatype,        swap );

        pixdim[ 0 ] = SwapBytes ( headernii.pixdim[ 0 ],     swap ); // special value: +1 or -1
        pixdim[ 1 ] = SwapBytes ( headernii.pixdim[ 1 ],     swap );
        pixdim[ 2 ] = SwapBytes ( headernii.pixdim[ 2 ],     swap );
        pixdim[ 3 ] = SwapBytes ( headernii.pixdim[ 3 ],     swap );
        pixdim[ 4 ] = SwapBytes ( headernii.pixdim[ 4 ],     swap );
        toffset     = SwapBytes ( headernii.toffset,         swap );

//      xyzt_units  = SwapBytes ( headernii.xyzt_units,      swap );

        scl_slope   = SwapBytes ( headernii.scl_slope,       swap );
        scl_inter   = SwapBytes ( headernii.scl_inter,       swap );
//      cal_min     = SwapBytes ( headernii.cal_min,         swap );
//      cal_max     = SwapBytes ( headernii.cal_max,         swap );

        qform_code  = SwapBytes ( headernii.qform_code,      swap );
        sform_code  = SwapBytes ( headernii.sform_code,      swap );

        qb          = SwapBytes ( headernii.quatern_b,       swap );
        qc          = SwapBytes ( headernii.quatern_c,       swap );
        qd          = SwapBytes ( headernii.quatern_d,       swap );
        qox         = SwapBytes ( headernii.qoffset_x,       swap );
        qoy         = SwapBytes ( headernii.qoffset_y,       swap );
        qoz         = SwapBytes ( headernii.qoffset_z,       swap );

        srx[ 0 ]    = SwapBytes ( headernii.srow_x[ 0 ],     swap );
        srx[ 1 ]    = SwapBytes ( headernii.srow_x[ 1 ],     swap );
        srx[ 2 ]    = SwapBytes ( headernii.srow_x[ 2 ],     swap );
        srx[ 3 ]    = SwapBytes ( headernii.srow_x[ 3 ],     swap );
        sry[ 0 ]    = SwapBytes ( headernii.srow_y[ 0 ],     swap );
        sry[ 1 ]    = SwapBytes ( headernii.srow_y[ 1 ],     swap );
        sry[ 2 ]    = SwapBytes ( headernii.srow_y[ 2 ],     swap );
        sry[ 3 ]    = SwapBytes ( headernii.srow_y[ 3 ],     swap );
        srz[ 0 ]    = SwapBytes ( headernii.srow_z[ 0 ],     swap );
        srz[ 1 ]    = SwapBytes ( headernii.srow_z[ 1 ],     swap );
        srz[ 2 ]    = SwapBytes ( headernii.srow_z[ 2 ],     swap );
        srz[ 3 ]    = SwapBytes ( headernii.srow_z[ 3 ],     swap );


        vox_offset  = type & SingleFile ? SwapBytes ( headernii.vox_offset,      swap ) : 0; // by safety, no offset in the dual file
        } // Nifti1

    else if ( IsNifti2 ( type ) ) {

        struct nifti_2_header   headernii;

        fih.read ( (char *) &headernii, sizeof ( headernii ) );

                                        // We have to restrict ourselves to the lowest 32 bits - but that should be good here!
        dim[ 0 ]    = SwapBytes ( headernii.dim[ 0 ],        swap );
        dim[ 1 ]    = SwapBytes ( headernii.dim[ 1 ],        swap );
        dim[ 2 ]    = SwapBytes ( headernii.dim[ 2 ],        swap );
        dim[ 3 ]    = SwapBytes ( headernii.dim[ 3 ],        swap );
        dim[ 4 ]    = SwapBytes ( headernii.dim[ 4 ],        swap );

        intent_code = SwapBytes ( headernii.intent_code,     swap ); // predefined codes
        intent_name =             headernii.intent_name;             // more freedom to put something here
        descrip     =             headernii.descrip;

        datatype    = SwapBytes ( headernii.datatype,        swap );

        pixdim[ 0 ] = SwapBytes ( headernii.pixdim[ 0 ],     swap ); // special value: +1 or -1
        pixdim[ 1 ] = SwapBytes ( headernii.pixdim[ 1 ],     swap );
        pixdim[ 2 ] = SwapBytes ( headernii.pixdim[ 2 ],     swap );
        pixdim[ 3 ] = SwapBytes ( headernii.pixdim[ 3 ],     swap );
        pixdim[ 4 ] = SwapBytes ( headernii.pixdim[ 4 ],     swap );
        toffset     = SwapBytes ( headernii.toffset,         swap );

//      xyzt_units  = SwapBytes ( headernii.xyzt_units,      swap );

        scl_slope   = SwapBytes ( headernii.scl_slope,       swap );
        scl_inter   = SwapBytes ( headernii.scl_inter,       swap );
//      cal_min     = SwapBytes ( headernii.cal_min,         swap );
//      cal_max     = SwapBytes ( headernii.cal_max,         swap );

        qform_code  = SwapBytes ( headernii.qform_code,      swap );
        sform_code  = SwapBytes ( headernii.sform_code,      swap );

        qb          = SwapBytes ( headernii.quatern_b,       swap );
        qc          = SwapBytes ( headernii.quatern_c,       swap );
        qd          = SwapBytes ( headernii.quatern_d,       swap );
        qox         = SwapBytes ( headernii.qoffset_x,       swap );
        qoy         = SwapBytes ( headernii.qoffset_y,       swap );
        qoz         = SwapBytes ( headernii.qoffset_z,       swap );

        srx[ 0 ]    = SwapBytes ( headernii.srow_x[ 0 ],     swap );
        srx[ 1 ]    = SwapBytes ( headernii.srow_x[ 1 ],     swap );
        srx[ 2 ]    = SwapBytes ( headernii.srow_x[ 2 ],     swap );
        srx[ 3 ]    = SwapBytes ( headernii.srow_x[ 3 ],     swap );
        sry[ 0 ]    = SwapBytes ( headernii.srow_y[ 0 ],     swap );
        sry[ 1 ]    = SwapBytes ( headernii.srow_y[ 1 ],     swap );
        sry[ 2 ]    = SwapBytes ( headernii.srow_y[ 2 ],     swap );
        sry[ 3 ]    = SwapBytes ( headernii.srow_y[ 3 ],     swap );
        srz[ 0 ]    = SwapBytes ( headernii.srow_z[ 0 ],     swap );
        srz[ 1 ]    = SwapBytes ( headernii.srow_z[ 1 ],     swap );
        srz[ 2 ]    = SwapBytes ( headernii.srow_z[ 2 ],     swap );
        srz[ 3 ]    = SwapBytes ( headernii.srow_z[ 3 ],     swap );


        vox_offset  = type & SingleFile ? SwapBytes ( headernii.vox_offset, swap ) : 0; // by safety, no offset in the dual file
        } // Nifti2


    fih.close ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check header
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if (     datatype == NIFTI_TYPE_UNKNOWN
      || ! ( datatype == NIFTI_TYPE_UINT8
          || datatype == NIFTI_TYPE_INT8
          || datatype == NIFTI_TYPE_UINT16
          || datatype == NIFTI_TYPE_INT16
          || datatype == NIFTI_TYPE_INT32
          || datatype == NIFTI_TYPE_FLOAT32              
          || datatype == NIFTI_TYPE_FLOAT64 ) ) {

        char            buff[ 256 ];
        StringCopy  ( buff, "Cartool does not support data type  '", (char*) IntegerToString ( datatype ), "' / ", NiftiDataTypeToString ( datatype ), "  for the moment, sorry..." );
        ShowMessage ( buff, NiftiTitle, ShowMessageWarning );
        return false;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if      ( dim[ 0 ] == 4 ) {
        if ( dim[ 4 ] > 1 ) {
            ShowMessage ( "Data is a sequence of volumes,\nonly the first one can be read for the moment...", NiftiTitle, ShowMessageWarning );
            dim[ 0 ]    = 3;
            dim[ 4 ]    = 1;
            }
        // else OK;
        }
    else if ( dim[ 0 ] != 3  ) {
        ShowMessage ( "Can not read file with dimensions different from 3 or 4...", NiftiTitle, ShowMessageWarning );
        return false;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check and set for quaternion "sign"
    qfac    = pixdim  [ 0 ];

    if ( ! ( qfac == 1 || qfac == -1 ) )
        qfac    = 1;

                                        // Taken from:  nifti1_io.c
                                        // setting the missing quaternion value
    qa   = 1 - ( Square ( qb ) + Square ( qc ) + Square ( qd ) );

                                        // special case of (near) 180 degrees rotation
    if ( qa < SingleFloatEpsilon ) {
                                        // make it real 0
        qa      = 0;
                                        // normalize vector (b,c,d)
        double      norm    = sqrt ( Square ( qb ) + Square ( qc ) + Square ( qd ) );
        qb     /=   norm;
        qc     /=   norm;
        qd     /=   norm;
        }
    else                                // OK case, angle = 2 * acos ( qa )
        qa      = sqrt ( qa );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Picking the preferred transform method
    NiftiTransformMethod    method      = GetNiftiTransformMethod ( qform_code, sform_code /*, true*/ );

                                        // Set once only
    NiftiIntentCode = intent_code;                      // seldom set

    StringCopy  ( NiftiIntentName, intent_name, 15 );   // seldom set

    NiftiTransform  = method == NiftiMethod1 ?  NIFTI_XFORM_UNKNOWN 
                    : method == NiftiMethod2 ?  qform_code
                    : method == NiftiMethod3 ?  sform_code
                    :                           NiftiTransformDefault;

                                        // Special case where Cartool saved some label short strings in the header?
    if ( StringContains ( (const char*) descrip, (const char*) NiftiDescripLabels ) ) {
                                        // force EOS by safety
        descrip[ 80 - 1 ]   = EOS;
                                        // 
        const char*		tolabels        = StringContains ( (const char*) descrip, (const char*) NiftiDescripLabels ) + StringLength ( NiftiDescripLabels );

                                        // pair integer-name ##ABCD
        TStringGrep     greppy ( "(\\d+)(\\D+)", GrepOptionDefault );
        TStrings        matches;
                                        // storing intermediate pairs, sorted
        std::map< int, char[ 32 ] >  labels;

        do {
            if ( ! greppy.Matched ( tolabels, &matches ) ) 
                break;


            int             index       = StringToInteger ( matches[ 1 ] );
            const char*		tolabel     =                   matches[ 2 ];

                                        // fill the hash table
            StringCopy ( labels[ index ], tolabel );

                                        // move the pointer forward
            tolabels   += StringLength ( matches[ 0 ] );

//            matches.Show ( "matches" );
            } while ( true );

                                        // transfer to our simpler string list
        int                 maxindex    = labels.empty () ? -1 : labels.rbegin()->first;

        Labels.Reset ();
                                        // copy up to the max index, non-existing labels are still allocated as empty string
        for ( int i = 0; i <= maxindex; i++ )
                                                                       // be nice and add a background string            
            Labels.Add ( StringIsNotEmpty ( labels[ i ] ) ? labels[ i ] : i == 0 ? "Background" : "" );

//      Labels.Show ( "Labels" );


                                        // Override, just in case it was not set already
        NiftiIntentCode     = NiftiIntentCodeLabels;

        StringCopy  ( NiftiIntentName, NiftiIntentNameLabels, 15 );
        }


//    DBGV ( NiftiIntentCode, NiftiIntentName );
//    DBGM2 ( NiftiTransformMethodShortString ( method ), NiftiTransformToString ( NiftiTransform ), "method  NiftiTransform" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Filling product info
    StringCopy ( CompanyName, CompanyNameNifti );
    StringCopy ( ProductName, ProductNameNifti );
    Version     = method;
    StringCopy ( VersionName, NiftiTransformMethodShortString ( method ) );
    Subversion  = method == NiftiMethod1 ?  NIFTI_XFORM_UNKNOWN 
                : method == NiftiMethod2 ?  qform_code
                : method == NiftiMethod3 ?  sform_code
                :                           NiftiTransformDefault;
    StringCopy ( SubversionName, NiftiTransformToString ( Subversion ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Some orientation info in the comments?
                                        // It could be non-RAS if data was coming from Analyze files f.ex., or it could be duplicating the default RAS
    if ( StringContains ( (const char*) descrip, (const char*) ExportOrientation ) ) {
                                        // get the 3 letters that summarize the orientation
        const char*			toorientation       = StringContains ( (const char*) descrip, (const char*) ExportOrientation ) + StringLength ( ExportOrientation );
                                        // try to use them
        KnownOrientation    = SetOrientation ( toorientation );
        }
                                        // NiftiMethod1 doesn't certify to end up in RAS orientation
    else if (    method == NiftiMethod2 
              || method == NiftiMethod3 )

        KnownOrientation    = SetOrientation ( NiftiOrientation );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    int                 filedim1        = dim[ 1 ];
    int                 filedim2        = dim[ 2 ];
    int                 filedim3        = dim[ 3 ];
                                        // store new size
    Size.Set ( 0, dim[ 1 ] - 1, 0, dim[ 2 ] - 1, 0, dim[ 3 ] - 1 );

    Data.Resize ( dim[ 1 ], dim[ 2 ], dim[ 3 ] );

                                        // deduce real size
    if ( pixdim[ 1 ] && pixdim[ 2 ] && pixdim[ 3 ] ) {
        VoxelSize.X     = pixdim[ 1 ];
        VoxelSize.Y     = pixdim[ 2 ];
        VoxelSize.Z     = pixdim[ 3 ];

        RealSize.X      = VoxelSize.X * dim[ 1 ];
        RealSize.Y      = VoxelSize.Y * dim[ 2 ];
        RealSize.Z      = VoxelSize.Z * dim[ 3 ];
        }
    else {
        VoxelSize       = 1;

        RealSize.X      = filedim1;
        RealSize.Y      = filedim2;
        RealSize.Z      = filedim3;
        }

                                        // for later:
    //double          SamplingFrequency   = pixdim[ 4 ] > 0 ? 1 / pixdim[ 4 ] : 0;
    //double          TimeOrigin          = toffset;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    TSuperGauge         Gauge;
    Gauge.Set           ( NiftiTitle, SuperGaugeLevelInter );
    Gauge.AddPart       ( 0, filedim3, 100 );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Either re-opening the single file, or the dual .img file
    ifstream            fii ( type & SingleFile ? fileinhdr : fileinimg, ios::binary );

                                        // Data starting position
    fii.seekg ( vox_offset );

                                        // Read according to format, then converted to MriType anyway
    if      ( datatype == NIFTI_TYPE_UINT8   ) {

        for ( int z=0; z < filedim3; z++ ) {

            Gauge.Next ( 0 );

            for ( int y=0; y < filedim2; y++ )
            for ( int x=0; x < filedim1; x++ ) {

                Data( x, y, z ) = (uchar) fii.get ();
                }
            }
        }

    else if ( datatype == NIFTI_TYPE_INT8    ) {

        char                cd;

        for ( int z=0; z < filedim3; z++ ) {

            Gauge.Next ( 0 );

            for ( int y=0; y < filedim2; y++ )
            for ( int x=0; x < filedim1; x++ ) {

                fii.get ( cd );

                Data( x, y, z ) = cd;
                }
            }
        }

    else if ( datatype == NIFTI_TYPE_UINT16  ) {

        uint16              usd;

        for ( int z=0; z < filedim3; z++ ) {

            Gauge.Next ( 0 );

            for ( int y=0; y < filedim2; y++ )
            for ( int x=0; x < filedim1; x++ ) {

                fii.read ( (char *) &usd, sizeof ( uint16 ) );

                SwappedBytes ( usd, swap );

                Data( x, y, z ) = usd;
                }
            }
        }

    else if ( datatype == NIFTI_TYPE_INT16   ) {

        int16               sd;

        for ( int z=0; z < filedim3; z++ ) {

            Gauge.Next ( 0 );

            for ( int y=0; y < filedim2; y++ )
            for ( int x=0; x < filedim1; x++ ) {

                fii.read ( (char *) &sd, sizeof ( int16 ) );

                SwappedBytes ( sd, swap );

                Data( x, y, z ) = sd;
                }
            }
        }

    else if ( datatype == NIFTI_TYPE_INT32   ) {

        int32               si;

        for ( int z=0; z < filedim3; z++ ) {

            Gauge.Next ( 0 );

            for ( int y=0; y < filedim2; y++ )
            for ( int x=0; x < filedim1; x++ ) {

                fii.read ( (char *) &si, sizeof ( int32 ) );

                SwappedBytes ( si, swap );

                Data( x, y, z ) = si;
                }
            }
        }

    else if ( datatype == NIFTI_TYPE_FLOAT32 ) {

        float               f;

        for ( int z=0; z < filedim3; z++ ) {

            Gauge.Next ( 0 );

            for ( int y=0; y < filedim2; y++ )
            for ( int x=0; x < filedim1; x++ ) {

                fii.read ( (char *) &f, sizeof ( float ) );

                SwappedBytes ( f, swap );
                                        // ignore NaN
                Data( x, y, z ) = IsNotAProperNumber ( f ) ? 0 : f;
                }
            }
        }

    else if ( datatype == NIFTI_TYPE_FLOAT64 ) {

        double              d;

        for ( int z=0; z < filedim3; z++ ) {

            Gauge.Next ( 0 );

            for ( int y=0; y < filedim2; y++ )
            for ( int x=0; x < filedim1; x++ ) {

                fii.read ( (char *) &d, sizeof ( double ) );

                SwappedBytes ( d, swap );
                                        // ignore NaN
                Data( x, y, z ) = IsNotAProperNumber ( d ) ? 0 : d;
                }
            }
        }


    fii.close ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optional values linear transform
    if ( scl_slope != 0 ) {
        
        Data   *= scl_slope;

        Data   += scl_inter;

                                        // integer rescaling might lead to unwanted artifacts
        if ( IsNiftiTypeInteger ( datatype ) && scl_slope != 1.0 ) {

            AtomFormatType      atomformattype  = datatype == NIFTI_TYPE_UINT8   ? AtomFormatUnsignedInt8 
                                                : datatype == NIFTI_TYPE_INT8    ? AtomFormatSignedInt8
                                                : datatype == NIFTI_TYPE_UINT16  ? AtomFormatUnsignedInt16
                                                : datatype == NIFTI_TYPE_INT16   ? AtomFormatSignedInt16
                                                : datatype == NIFTI_TYPE_INT32   ? AtomFormatSignedInt32
                                                : datatype == NIFTI_TYPE_FLOAT32 ? AtomFormatFloat32
                                                : datatype == NIFTI_TYPE_FLOAT64 ? AtomFormatFloat64
                                                :                                  UnknownAtomFormat;

                                        // actual precision with the amount of bits from type
            double              maxpossiblevalue= Power ( 2, (int) AtomFormatTypePresets[ atomformattype ].NumBits () - ( IsSigned ( atomformattype ) ? 1 : 0 ) );
                                        // round it below
            double              precision       = 1 / Power ( 10, RoundBelow ( Log10 ( maxpossiblevalue ) ) - 1 );


            double              minval          = Data.GetMinValue ();
            double              maxval          = Data.GetMaxValue ();
                                        // min looks like a negative artifact?
            if ( minval < 0 && maxval > 0 ) {

                minval  = fabs ( minval );

                if      ( minval / maxval < precision )  {   minval = 0;  Data.UnaryOp ( OperandData, OperationThresholdAbove, &minval ); }
                else if ( maxval / minval < precision )  {   maxval = 0;  Data.UnaryOp ( OperandData, OperationThresholdBelow, &maxval ); }
                }
            }

/*                                      // Data might need a tiny bit of rounding after rescaling from integer(?)
        if ( IsNiftiTypeInteger ( datatype ) && scl_slope != 1.0 ) {

            double              absmax          = Data.GetAbsMaxValue ();

            AtomFormatType      atomformattype  = datatype == NIFTI_TYPE_UINT8   ? AtomFormatUnsignedInt8 
                                                : datatype == NIFTI_TYPE_INT8    ? AtomFormatSignedInt8
                                                : datatype == NIFTI_TYPE_UINT16  ? AtomFormatUnsignedInt16
                                                : datatype == NIFTI_TYPE_INT16   ? AtomFormatSignedInt16
                                                : datatype == NIFTI_TYPE_INT32   ? AtomFormatSignedInt32
                                                : datatype == NIFTI_TYPE_FLOAT32 ? AtomFormatFloat32
                                                : datatype == NIFTI_TYPE_FLOAT64 ? AtomFormatFloat64
                                                :                                  UnknownAtomFormat;

                                        // actual precision with the amount of bits from type
            double              precision       = Power ( 2, AtomFormatTypePresets[ atomformattype ].NumBits () - ( IsSigned ( atomformattype ) ? 1 : 0 ) );
                                        // round it below
                                precision       = Power ( 10, RoundBelow ( Log10 ( precision ) ) );
                                        // then relative to current max - we are rounding after the rescaling
            double              roundto         = absmax / precision;

            for ( int i = 0; i < Data.GetLinearDim (); i++ )
                Data[ i ]   = RoundTo ( Data[ i ], roundto );
            }
*/
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    TMatrix44           transform;
    double              targetvoxelsize;

                                        // A.k.a. voxel resizing
    if      ( method == NiftiMethod1 ) {

        TPointDouble        sourcevoxelratio ( 1 );

                                        // is there any numerical difference between any directions?
//      if ( ! VoxelSize.IsIsotropic ( 0.01 ) ) {
                                        // don't downsample high res isotropic MRIs, like for Mouse
//      if ( ! ( VoxelSize.IsIsotropic ( 0.01 ) && VoxelSize.Min () < NiftiTargetRes ) ) {

                                        // boosting to the highest resolution from any axis
//          targetvoxelsize     = VoxelSize.Min ();
                                        // always targetting this standard resolution
//          targetvoxelsize     = NiftiTargetRes;
                                        // targetting standard resolution, but avoiding high-sampling
//          targetvoxelsize     = max ( VoxelSize.Min (), NiftiTargetRes );
                                        // allowing high-sampling up to 1[mm], otherwise keep the high-resolution MRI
            targetvoxelsize     = min ( VoxelSize.Min (), NiftiTargetRes );

                                        // don't allow less than ~ 1% difference between voxel sizes
            sourcevoxelratio.X  = RoundTo ( VoxelSize.X / targetvoxelsize, 0.01 );
            sourcevoxelratio.Y  = RoundTo ( VoxelSize.Y / targetvoxelsize, 0.01 );
            sourcevoxelratio.Z  = RoundTo ( VoxelSize.Z / targetvoxelsize, 0.01 );


            transform.Scale ( sourcevoxelratio.X, sourcevoxelratio.Y, sourcevoxelratio.Z, MultiplyRight );
//          }
        } // NiftiMethod1

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Quaternion transform
    else if ( method == NiftiMethod2 ) {

        TMatrix44           rotation;
        TMatrix44           scaling;
        TMatrix44           translation;


        rotation ( 0, 0 )   = Square ( qa ) + Square ( qb ) - Square ( qc ) - Square ( qd );
        rotation ( 1, 0 )   = 2 * ( qb * qc + qa * qd );
        rotation ( 2, 0 )   = 2 * ( qb * qd - qa * qc );

        rotation ( 0, 1 )   = 2 * ( qb * qc - qa * qd );
        rotation ( 1, 1 )   = Square ( qa ) + Square ( qc ) - Square ( qb ) - Square ( qd );
        rotation ( 2, 1 )   = 2 * ( qc * qd + qa * qb );

        rotation ( 0, 2 )   = 2 * ( qb * qd + qa * qc );
        rotation ( 1, 2 )   = 2 * ( qc * qd - qa * qb );
        rotation ( 2, 2 )   = Square ( qa ) + Square ( qd ) - Square ( qb ) - Square ( qc );

                                        // make sure ALL scaling factors are positive
        if ( ! ( pixdim[ 1 ] > 0 && pixdim[ 2 ] > 0 && pixdim[ 3 ] > 0 ) )

            pixdim[ 1 ] = pixdim[ 2 ] = pixdim[ 3 ] = 1;
                                        // rescaling will results in 1[mm] voxel size
        targetvoxelsize     = NiftiTargetRes;

        scaling.Scale ( pixdim[ 1 ] / targetvoxelsize, 
                        pixdim[ 2 ] / targetvoxelsize, 
                        pixdim[ 3 ] / targetvoxelsize * qfac,   // correcting for the left-handedness during the scaling
                        MultiplyRight );


        translation.SetTranslation  ( qox, qoy, qoz );

                                        // rescale voxels, then rotate, then translate
        transform   = translation * rotation * scaling;
        } // NiftiMethod2

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3x4 Matrix transform
    else if ( method == NiftiMethod3 ) {

        TMatrix44           mat34;
        TMatrix44           scaling;


        mat34.Set   (   srx[ 0 ], srx[ 1 ], srx[ 2 ], srx[ 3 ], // part specified in the header
                        sry[ 0 ], sry[ 1 ], sry[ 2 ], sry[ 3 ],
                        srz[ 0 ], srz[ 1 ], srz[ 2 ], srz[ 3 ],
                        0,        0,        0,        1       );// part not specified, set to default

                                        // force rescaling to final 1[mm] voxel size
        targetvoxelsize     = NiftiTargetRes;

        scaling.Scale   (   1 / targetvoxelsize, 
                            1 / targetvoxelsize, 
                            1 / targetvoxelsize,
                            MultiplyRight );

                                        // 3x4 transform (scaling, rotation, translation), then final rescaling
        transform   = scaling * mat34;
        } // NiftiMethod3

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//  transform.Show ( NiftiTransformMethodShortString ( method ) );

                                        // Will perform the most optimal reslicing
    if (    ResliceData     (   transform,
                                Origin,
                                VoxelSize,
                                NiftiTransform,     NiftiIntentCode,        NiftiIntentName,
                                NiftiTransformMethodShortString ( method ) ) ) {

                                        // new isotropic voxel size
        VoxelSize   = targetvoxelsize;

        RealSize.X  = VoxelSize.X * Data.GetDim ( 0 );
        RealSize.Y  = VoxelSize.Y * Data.GetDim ( 1 );
        RealSize.Z  = VoxelSize.Z * Data.GetDim ( 2 );

        Size.Set ( 0, Data.GetDim ( 0 ) - 1, 0, Data.GetDim ( 1 ) - 1, 0, Data.GetDim ( 2 ) - 1 );

                                        // here data has been resliced
//      SetDirty ( true );
        }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    }

else {                                  // can not create
    return false;

/*
    int                 dim1            = 2 * 100 + 1;
    int                 dim2            = 2 * 100 + 1;
    int                 dim3            = 2 * 100 + 1;

                                        // store new size
    Size.Set    ( 0, dim1 - 1, 0, dim2 - 1, 0, dim3 - 1 );

    Data.Resize ( dim1, dim2, dim3 );

    VoxelSize   = 1;

    RealSize.Set ( dim1, dim2, dim3 );
    RealSize   *= VoxelSize;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Picking the preferred transform method
    NiftiTransformMethod    method      = NiftiMethod3;

    NiftiIntentCode     = NiftiIntentCodeSynth;

    StringCopy  ( NiftiIntentName, NiftiIntentNameDefault, 15 );

    NiftiTransform      = NiftiTransformDefault;

    KnownOrientation    = SetOrientation ( NiftiOrientation );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Filling product info
    StringCopy ( CompanyName, CompanyNameNifti );
    StringCopy ( ProductName, ProductNameNifti );
    Version     = method;
    StringCopy ( VersionName, NiftiTransformMethodShortString ( method ) );
    Subversion  = NiftiTransform;
    StringCopy ( SubversionName, NiftiTransformToString ( Subversion ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Origin.Set ( ( dim1 - 1 ) / 2.0, ( dim2 - 1 ) / 2.0, ( dim3 - 1 ) / 2.0 );

    TPointDouble        center1 ( ( dim1 - 1 ) * 0.25, ( dim2 - 1 ) * 0.50, ( dim3 - 1 ) * 0.50 );
    TPointDouble        center2 ( ( dim1 - 1 ) * 0.75, ( dim2 - 1 ) * 0.50, ( dim3 - 1 ) * 0.50 );


    double              v;
    double              v1;
    double              v2;

    for ( int x = 0; x < dim1; x++ )
    for ( int y = 0; y < dim2; y++ )
    for ( int z = 0; z < dim3; z++ ) {
                                        // 1 centered sphere
//        v   = ( TPointDouble ( x, y, z ) - Origin ).Norm ();
//        Data ( x, y, z )    = v;

//        v1  = ( TPointDouble ( x, y, z ) - center1 ).Norm ();
//        v2  = ( TPointDouble ( x, y, z ) - center2 ).Norm ();
//        Data ( x, y, z )    = v1 * v2;

        v1  = cos ( ( TPointDouble ( x, y, z ) - center1 ).Norm () * 0.1 );
        v2  = cos ( ( TPointDouble ( x, y, z ) - center2 ).Norm () * 0.1 );
        Data ( x, y, z )    = v1 * v2;
        }

                                        // Invert intensities if needed (spheres)
//    double              maxdata     = Data.GetMaxValue ();
//
//    for ( int i = 0; i < Data.GetLinearDim (); i++ )
//
//        Data ( i )  = maxdata - Data ( i );

                                        // Normalize intensities
    Data   *= 100 / Data.GetAbsMaxValue ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    SetDocPath ( "Two Spheres" "." FILEEXT_MRINII );

    SetDirty ( true );

//  Commit (true);                      // force to write it NOW
*/
    }


return true;
}


//----------------------------------------------------------------------------

}
