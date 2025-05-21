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

#include    "MemUtil.h"
#include    "Strings.TFixedString.h"
#include    "Math.Stats.h"

#include    "CartoolTypes.h"
#include    "Dialogs.TSuperGauge.h"
#include    "Math.Resampling.h"
#include    "TVolume.h"

#include    "TExportVolume.h"
#include    "Volumes.AnalyzeNifti.h"

#include    "TVolumeAnalyzeDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TVolumeAnalyzeDoc::TVolumeAnalyzeDoc ( TDocument* parent )
      :	TVolumeDoc ( parent )
{
}


//----------------------------------------------------------------------------
bool	TVolumeAnalyzeDoc::Commit ( bool force )
{
if ( ! ( IsDirty () || force ) )
    return true;


if ( Data.IsNotAllocated () ) {

    SetDirty ( false );
    return true;
    }


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


                                        // in case one wants to mingle with the saved dimensions
//expvol.Dimension.X     = Data.GetDim ( 0 );
//expvol.Dimension.Y     = Data.GetDim ( 1 );
//expvol.Dimension.Z     = Data.GetDim ( 2 );
//
//for ( int z = 0; z < expvol.Dimension.Z; z++ )
//for ( int y = 0; y < expvol.Dimension.Y; y++ )
//for ( int x = 0; x < expvol.Dimension.X; x++ )
//    expvol.Write ( Data ( x, y, z ) );


SetDirty ( false );

return  true;
}


//----------------------------------------------------------------------------
bool	TVolumeAnalyzeDoc::Open	(int /*mode*/, const char* path)
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


    if ( ! IsAnalyze75 ( type ) ) { // this should be resolved before, but check it again
        ShowMessage ( "This doesn't seem to be an Analyze7.5 file.\nMaybe try to open as a Nifti file maybe?", AnalyzeTitle, ShowMessageWarning ); 
        return  false;
        }


    bool                swap            = type & SwapData;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Read header
    SetDocPath ( fileinhdr );

    ifstream            fih ( fileinhdr, ios::binary );

    struct dsr          hin;

    fih.read ( (char *) &hin, sizeof ( struct dsr ) );


    int                 dim[ 8 ];
                        dim[ 0 ]        = SwapBytes ( hin.dime.dim[ 0 ],      swap );
                        dim[ 1 ]        = SwapBytes ( hin.dime.dim[ 1 ],      swap );
                        dim[ 2 ]        = SwapBytes ( hin.dime.dim[ 2 ],      swap );
                        dim[ 3 ]        = SwapBytes ( hin.dime.dim[ 3 ],      swap );
                        dim[ 4 ]        = SwapBytes ( hin.dime.dim[ 4 ],      swap );
    int                 datatype	    = SwapBytes ( hin.dime.datatype,      swap );
    int                 glmin           = SwapBytes ( hin.dime.glmin,         swap );
    int                 glmax           = SwapBytes ( hin.dime.glmax,         swap );
    double              pixdim[ 8 ];
                        pixdim[ 1 ]     = SwapBytes ( hin.dime.pixdim[ 1 ],   swap );
                        pixdim[ 2 ]     = SwapBytes ( hin.dime.pixdim[ 2 ],   swap );
                        pixdim[ 3 ]     = SwapBytes ( hin.dime.pixdim[ 3 ],   swap );
                        pixdim[ 4 ]     = SwapBytes ( hin.dime.pixdim[ 4 ],   swap );   // non-standard, but reasonable
    double              spmslope        = SwapBytes ( hin.dime.funused1,      swap );   // non-standard, SPM extension
    double              spminter        = SwapBytes ( hin.dime.funused2,      swap );   // non-standard, SPM extension
                                        // this seems to be short int
    double              org1            = SwapBytes ( (short int) ( (uchar) hin.hist.originator[ 0 ] | ( (uchar) hin.hist.originator[ 1 ] << 8 ) ), swap );
    double              org2            = SwapBytes ( (short int) ( (uchar) hin.hist.originator[ 2 ] | ( (uchar) hin.hist.originator[ 3 ] << 8 ) ), swap );
    double              org3            = SwapBytes ( (short int) ( (uchar) hin.hist.originator[ 4 ] | ( (uchar) hin.hist.originator[ 5 ] << 8 ) ), swap );


    fih.close ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check header
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // some orientation info in the comments?
    if ( StringContains ( (const char*) hin.hist.descrip, (const char*) ExportOrientation ) ) {
                                        // get the 3 letters that summarize the orientation
        const char*			toorientation       = StringContains ( (const char*) hin.hist.descrip, (const char*) ExportOrientation ) + StringLength ( ExportOrientation );

//        DBGM ( toorientation, "Opening: Orientation" );
                                        // try to use them
        KnownOrientation    = SetOrientation ( toorientation );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if (     datatype == DT_UNKNOWN
      || ! ( datatype == DT_UNSIGNED_CHAR
          || datatype == DT_SIGNED_SHORT
          || datatype == DT_FLOAT              
          || datatype == DT_DOUBLE        ) ) {
        char            buff[ 256 ];
        StringCopy  ( buff, "Cartool does not support data type  '", (char*) IntegerToString ( datatype ), "' / ", AnalyzeDataTypeToString ( datatype ), "  for the moment, sorry..." );
        ShowMessage ( buff, AnalyzeTitle, ShowMessageWarning );
        return false;
        }

                                        // refining the SHORT type by guessing
    if ( datatype == DT_SIGNED_SHORT ) {
        if      (               glmax == 0xFFFF )   datatype    = DT_UNSIGNED_SHORT;
        else if ( glmin == 0 && glmax == 0x7FFF )   datatype    = DT_SIGNED_SHORT_POS;
        else                                        datatype    = DT_SIGNED_SHORT;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if      ( dim[ 0 ] == 4 ) {
        if ( dim[ 4 ] > 1 ) {
            ShowMessage ( "Data is a sequence of volumes,\nonly the first one can be read for the moment...", AnalyzeTitle, ShowMessageWarning );
            dim[ 0 ]    = 3;
            dim[ 4 ]    = 1;
            }
        // else OK;
        }
    else if ( dim[ 0 ] != 3  ) {
        ShowMessage ( "Can not read file with dimensions different from 3 or 4...", AnalyzeTitle, ShowMessageWarning );
        return false;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Filling product info
    StringCopy ( CompanyName, CompanyNameAnalyze );
    StringCopy ( ProductName, ProductNameAnalyze );
    Version     = 7;    // 7.5
    Subversion  = 5;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Some software use negative pixdim to indicate reverted voxel ordering
    pixdim[ 1 ]     = fabs ( pixdim[ 1 ] );
    pixdim[ 2 ]     = fabs ( pixdim[ 2 ] );
    pixdim[ 3 ]     = fabs ( pixdim[ 3 ] );


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

        RealSize.X      = VoxelSize[ 0 ] * dim[ 1 ];
        RealSize.Y      = VoxelSize[ 1 ] * dim[ 2 ];
        RealSize.Z      = VoxelSize[ 2 ] * dim[ 3 ];
        }
    else {
        VoxelSize       = 1;

        RealSize.X      = filedim1;
        RealSize.Y      = filedim2;
        RealSize.Z      = filedim3;
        }

                                        // for later:
    //double          SamplingFrequency   = pixdim[ 4 ] > 0 ? 1 / pixdim[ 4 ] : 0;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // allow origin to be anywhere
    Origin      = TPointDouble ( org1, org2, org3 );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    TSuperGauge         Gauge;
    Gauge.Set           ( AnalyzeTitle, SuperGaugeLevelInter );
    Gauge.AddPart       ( 0, filedim3, 100 );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Opening second file, the .img one
    ifstream            fii ( fileinimg, ios::binary );

                                        // Read according to format, then converted to MriType anyway
    if      ( datatype == DT_UNSIGNED_CHAR      ) {

        for ( int z=0; z < filedim3; z++ ) {

            Gauge.Next ( 0 );

            for ( int y=0; y < filedim2; y++ )
            for ( int x=0; x < filedim1; x++ ) {

                Data( x, y, z ) = (uchar) fii.get ();
                }
            }
        }

    else if ( datatype == DT_SIGNED_SHORT
           || datatype == DT_SIGNED_SHORT_POS   ) {

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

    else if ( datatype == DT_UNSIGNED_SHORT     ) {

        uint16              usd;

        for ( int z=0; z < filedim3; z++ ) {

            Gauge.Next ( 1 );

            for ( int y=0; y < filedim2; y++ )
            for ( int x=0; x < filedim1; x++ ) {

                fii.read ( (char *) &usd, sizeof ( uint16 ) );

                SwappedBytes ( usd, swap );

                Data( x, y, z ) = usd;
                }
            }
        }

    else if ( datatype == DT_FLOAT              ) {

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

    else if ( datatype == DT_DOUBLE             ) {

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
                                        // Optional values linear transform - Non-standard
    if ( spmslope != 0 ) {
        
        Data   *= spmslope;

        Data   += spminter;

                                        // integer rescaling might lead to unwanted artifacts
        if ( IsAnalyzeTypeInteger ( datatype ) && spmslope != 1.0 ) {

            AtomFormatType      atomformattype  = datatype == DT_UNSIGNED_CHAR      ? AtomFormatUnsignedInt8 
                                                : datatype == DT_UNSIGNED_SHORT     ? AtomFormatUnsignedInt16
                                                : datatype == DT_SIGNED_SHORT       ? AtomFormatSignedInt16
                                                : datatype == DT_SIGNED_SHORT_POS   ? AtomFormatSignedInt16
                                                : datatype == DT_FLOAT              ? AtomFormatFloat32
                                                : datatype == DT_DOUBLE             ? AtomFormatFloat64
                                                :                                     UnknownAtomFormat;

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

/*                                      // Taken from Nifti, but it seems to never occur because of its non-standard nature
        if ( IsAnalyzeTypeInteger ( datatype ) && spmslope != 1.0 ) {

            double              absmax          = Data.GetAbsMaxValue ();

            AtomFormatType      atomformattype  = datatype == DT_UNSIGNED_CHAR      ? AtomFormatUnsignedInt8 
                                                : datatype == DT_UNSIGNED_SHORT     ? AtomFormatUnsignedInt16
                                                : datatype == DT_SIGNED_SHORT       ? AtomFormatSignedInt16
                                                : datatype == DT_SIGNED_SHORT_POS   ? AtomFormatSignedInt16
                                                : datatype == DT_FLOAT              ? AtomFormatFloat32
                                                : datatype == DT_DOUBLE             ? AtomFormatFloat64
                                                :                                     UnknownAtomFormat;

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

                                        // is there any numerical difference between any directions?
    if ( ! VoxelSize.IsIsotropic ( 0.01 ) ) {

        TPointDouble        sourcevoxelratio ( 1 );

                                        // boosting to the highest resolution from any axis
        targetvoxelsize     = VoxelSize.Min ();

                                        // don't allow less than ~ 1% difference between voxel sizes
        sourcevoxelratio.X  = RoundTo ( VoxelSize.X / targetvoxelsize, 0.01 );
        sourcevoxelratio.Y  = RoundTo ( VoxelSize.Y / targetvoxelsize, 0.01 );
        sourcevoxelratio.Z  = RoundTo ( VoxelSize.Z / targetvoxelsize, 0.01 );


        transform.Scale ( sourcevoxelratio.X, sourcevoxelratio.Y, sourcevoxelratio.Z, MultiplyRight );

                                        // manually update the origin
        transform.Apply ( Origin );
        } // Voxel resizing


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Will perform the most optimal reslicing
    if (    ResliceData     (   transform,
                                Origin,
                                VoxelSize,
                                NiftiTransform,     NiftiIntentCode,        NiftiIntentName,
                                AnalyzeMethodTitle ) ) {

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
else {
/*
                                        // Reading Taloracle.dlf
    TFileName           path;
    TFileName           file;
    int                 x;
    int                 y;
    int                 z;

    StringCopy ( path, CartoolApplication->ApplicationDir );

                                        // volume exists!
    int                 dim1            = 171;
    int                 dim2            = 172;
    int                 dim3            = 110;


    TVolume<MriType>    tal ( dim1, dim2, dim3 );

                                        // get the baby!
    StringCopy  ( file, path, "\\", TalairachOracleFileName );

    tal.ReadFile ( file );

                                        // generate a volume from one of the labels
    Size.Set ( 0, dim1 - 1, 0, dim2 - 1, 0, dim3 - 1 );

    Data.Resize ( dim1, dim2, dim3 );


    for ( x = 0; x < Size.GetXExtent (); x++ )
    for ( y = 0; y < Size.GetYExtent (); y++ )
    for ( z = 0; z < Size.GetZExtent (); z++ )
        Data ( x, y, z ) = TalairachDecodeLabel2 ( tal ( x, y, z ) );

    MaxValue        = Data.GetMaxValue ();

    VoxelSize       = 1;
    RealSize.X      = dim1;
    RealSize.Y      = dim2;
    RealSize.Z      = dim3;


    SetDocPath ( "fld doc" );

//  Commit (true);                      // force to write it NOW
*/
    return false;
    }


return true;
}


//----------------------------------------------------------------------------

}
