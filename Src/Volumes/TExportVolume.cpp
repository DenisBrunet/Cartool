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

#include    "Files.Extensions.h"
#include    "TCartoolApp.h"

#include    "MemUtil.h"

#include    "Math.Utils.h"
#include    "Files.Stream.h"

#include    "TVolume.h"

#include    "Volumes.AnalyzeNifti.h"

#include    "TBaseDialog.h"

#include    "TExportVolume.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const AtomFormatType    ExportVolumesFormatTypes [ NumExportVolumesFormatTypes ] = 
                        {
                        AtomFormatByte,
                        AtomFormatFloat,
                        };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TExportVolume::TExportVolume ( AtomFormatType volumeformat )
      : of ( 0 )
{
Reset ();
                                        // Fields below are set only at creation, NOT when calling Reset

VolumeFormat    = volumeformat;
                                        // make sure this is a legal format
CheckVolumeFormat ();

                                        // Setting default Nifti options
NiftiTransform              = NiftiTransformDefault;

NiftiIntentCode             = NiftiIntentCodeDefault;
NiftiIntentParameters[0]    =
NiftiIntentParameters[1]    = 
NiftiIntentParameters[2]    = 0;

StringCopy  ( NiftiIntentName, NiftiIntentNameDefault, NiftiIntentNameSize - 1 );
}


        TExportVolume::~TExportVolume ()
{
End ();
}

                                        // Some fields are not resetted to allow multiple calls with the same TExportVolume object - These fields are reset in the creator, though
void    TExportVolume::Reset ()
{
End ();                                 // will take care of stream 'of'


Filename.Clear ();
ClearString ( Type );
                                        // !NOT resetting VolumeFormat!

NumDimensions       = 3;                // !set to 3 for most usual cases!
Dimension           = 0;
NumTimeFrames       = 1;                // !set to 1 for most usual cases!
SamplingFrequency   = 0;

VoxelSize.Reset ();
RealSize .Reset ();

Origin   .Reset ();
TimeOrigin          = 0;

ClearString ( Orientation, 4 );
MaxValue            = 0;

                                        // !NOT resetting these Nifti options:  NiftiTransform, NiftiIntentCode, NiftiIntentParameters, NiftiIntentName!

                                        // private fields
DoneBegin           = false;
EndOfHeader         = 0;
CurrentPosition     = 0;
AtomSize            = 1;
RescalingToInteger  = 1;
}


//----------------------------------------------------------------------------
void    TExportVolume::CheckVolumeFormat ()
{
                                        // loop through all legal types
for ( int i = 0; i < NumExportVolumesFormatTypes; i++ )
    if ( ExportVolumesFormatTypes [ i ] == VolumeFormat )
        return;

                                        // not found, set default
VolumeFormat    = ExportVolumeDefaultAtomFormat;
}


//----------------------------------------------------------------------------
                                        // pre-fill the (big) file for better performance
void    TExportVolume::PreFillFile ()
{
if ( ! DoneBegin )
    return;                             // this should called right after the header has been written


of->seekp ( EndOfHeader, ios::beg );

AllocateFileSpace   ( of, GetMemorySize (), FILE_CURRENT );
                                        // go back to data origin
of->seekp ( EndOfHeader, ios::beg );
}


//----------------------------------------------------------------------------
void    TExportVolume::End ()
{
                                        // CloseStream
if ( of != 0 ) {

    of->close ();
    delete  of;
    of  = 0;
    }

DoneBegin           = false;
}


//----------------------------------------------------------------------------
void    TExportVolume::Begin ()
{
End ();                                 // close any open file


if ( Filename.IsEmpty () ) {            // no filename is kind of a problem...
    Reset ();
    return;
    }

                                        // can modify Filename
Filename.CheckFileName ( (TFilenameFlags) ( TFilenameExtendedPath | TFilenameToSibling ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( StringIsEmpty ( Type ) ) {         // no Type override? get it from the filename

    GetExtension    ( Type, Filename );
                                        // special case
    if ( StringIs ( Type, FILEEXT_MRIAVW_IMG ) )
        StringCopy ( Type, FILEEXT_MRIAVW_HDR );
    }

                                        // if export type incorrect, select a default one
if ( ! IsStringAmong ( Type, ExportVolumeTypes ) )

    StringCopy ( Type, ExportVolumeDefaultType );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check data type
CheckVolumeFormat ();

                                        // retrieve atom memory size
AtomSize        = AtomFormatTypePresets[ VolumeFormat ].NumBytes;

                                        // factor applied BEFORE writing to file (before integer conversion)
RescalingToInteger  = VolumeFormat == AtomFormatByte  ? ( IsInteger ( MaxValue )                                            // integer test would better be ran on the whole data to be 100% correct
                                                       && IsInsideLimits ( (int) MaxValue, 1, (int) Highest<UCHAR>() ) ? 1  // data is integer and fits in 1 byte already
                                                                                                                       : Highest<UCHAR>() / NonNull ( MaxValue ) 
                                                    )
                    : VolumeFormat == AtomFormatFloat ? 1   // floating point values need no rescaling
                    :                                   1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check size
if ( RealSize.IsSomeNull () )

    RealSize    = VoxelSize.IsSomeNull () ? Dimension : VoxelSize * Dimension;


if ( VoxelSize.IsSomeNull () )

    VoxelSize   = RealSize .IsSomeNull () ? 1.0       : RealSize  / Dimension;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // create the correct type of file
of      = new ofstream ( Filename, ios::binary );


if ( ! of->good () ) {
    ShowMessage     (   "There seems to be a problem while trying to write this file." NewLine 
                        "Maybe it is write-protected, could you check that for me, as I have no hands...", 
                        ToFileName ( Filename ), ShowMessageWarning );
    Reset ();
    return;
    }


WriteHeader ();


CurrentPosition         = 0;
DoneBegin               = true;

                                        // they're all big binary files
PreFillFile ();
}


//----------------------------------------------------------------------------
void    TExportVolume::WriteHeader ()
{
                                        // go back to beginning, if needed
of->seekp ( 0, ios::beg );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Write any specific header
if      ( StringIs ( Type, FILEEXT_MRIAVW_HDR  ) ) {
                                        // !reminder if calling WriteHeader more than once: 'of' stream is switched to the img file when done!

                                        // clear header
    struct  dsr             hout;

    ClearVirtualMemory ( &hout, sizeof ( hout ) );

                                        // fill the usual fields
    hout.hk.sizeof_hdr      = ANALYZE_HEADER_SIZE;
    hout.hk.regular         = 'r';


    StringCopy ( hout.hk.db_name,   CartoolTitle );
    StringCopy ( hout.hist.descrip, ExportedByCartool );

                                        // has some orientation info to sneak in (& enough space)?
    if (    StringLength ( Orientation ) == 3 
         && StringLength ( hout.hist.descrip ) + StringLength ( "; " ExportOrientation ) + 4 < 80 )

        StringAppend ( hout.hist.descrip, "; " ExportOrientation, Orientation );

                                        // output either a 3 or a 4 dimensional volume
    hout.dime.dim[ 0 ]      = NumDimensions;
    hout.dime.dim[ 1 ]      = Dimension.X;
    hout.dime.dim[ 2 ]      = Dimension.Y;
    hout.dime.dim[ 3 ]      = Dimension.Z;
    hout.dime.dim[ 4 ]      = GetTimeDimension ();

                                        // pixdim is parallel to dim, but pixdim[ 0 ] does not mean anything
    hout.dime.pixdim[ 1 ]   = VoxelSize.X;
    hout.dime.pixdim[ 2 ]   = VoxelSize.Y;
    hout.dime.pixdim[ 3 ]   = VoxelSize.Z;
    hout.dime.pixdim[ 4 ]   = SamplingFrequency > 0 ? 1 / SamplingFrequency : 0;    // write even if single time point


    hout.dime.vox_offset    = 0;        // offset of voxels inside .img
    hout.dime.glmin         = 0;
    hout.dime.glmax         = 0;        // skip global file min / max (not rescaled)
    hout.dime.cal_min       = 0;
    hout.dime.cal_max       = 0;        // no calibration (used to be MaxValue)
    hout.dime.funused1      = RescalingToInteger == 0 || RescalingToInteger == 1 ? 0 : 1 / RescalingToInteger;    // SPM scale factor == Nifti scl_slope
    hout.dime.funused2      = 0;        // SPM offset factor
    hout.dime.datatype      = (short) ( VolumeFormat == AtomFormatByte ? DT_UNSIGNED_CHAR : DT_FLOAT );
    hout.dime.bitpix        = (short) AtomFormatTypePresets[ VolumeFormat ].NumBits ();


    hout.hist.orient        = 0;        // there are no flags to tell we are in RAS f.ex., or any of the 48 possible right-handed orientations

                                        // we have to round to closest voxel, as Analyze store origin in short int...
    ((short *) hout.hist.originator)[ 0 ]   = (short) Truncate ( Origin.X );
    ((short *) hout.hist.originator)[ 1 ]   = (short) Truncate ( Origin.Y );
    ((short *) hout.hist.originator)[ 2 ]   = (short) Truncate ( Origin.Z );


    of->write ( (char *) &hout, sizeof ( hout ) );


                                        // done writing header
    of->close ();
    delete  of;
    of  = 0;

                                        // switch to data file
    TFileName           fileoutimg ( Filename );

    ReplaceExtension ( fileoutimg, FILEEXT_MRIAVW_IMG );

    of                      = new ofstream ( TFileName ( fileoutimg, TFilenameExtendedPath ), ios::binary );

    } // FILEEXT_MRIAVW_HDR

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( StringIs ( Type, FILEEXT_MRINII  ) ) {

                                        // Brace yourself!
    struct nifti_2_header   headernii;

    ClearVirtualMemory  ( &headernii, sizeof ( headernii ) );

                                        // We will work with Nifti2+ (single file)
    headernii.sizeof_hdr    = sizeof ( headernii ); // NIFTI2_HEADER_SIZE

    StringCopy  ( headernii.magic, "n+2" );

                                    // output either a 3 or a 4 dimensional volume
    headernii.dim[ 0 ]      = NumDimensions;
    headernii.dim[ 1 ]      = Dimension.X;
    headernii.dim[ 2 ]      = Dimension.Y;
    headernii.dim[ 3 ]      = Dimension.Z;
    headernii.dim[ 4 ]      = GetTimeDimension ();

                                        // Can be set by the caller - default values otherwise
    headernii.intent_code   = NiftiIntentCode;
                                        // additional parameters to the intent above
    headernii.intent_p1     = NiftiIntentParameters[ 0 ];
    headernii.intent_p2     = NiftiIntentParameters[ 1 ];
    headernii.intent_p3     = NiftiIntentParameters[ 2 ];

    StringCopy  ( headernii.intent_name, NiftiIntentName, 15 );


    StringCopy ( headernii.descrip, ExportedByCartool );
                                        // has some orientation info to sneak in (& enough space)?
    if (    StringLength ( Orientation ) == 3
//       && StringIsNot  ( Orientation, NiftiOrientation )  // don't specify if already Nifti orientation?
         && StringLength ( headernii.descrip ) + StringLength ( "; " ExportOrientation ) + 4 < 80 )

        StringAppend ( headernii.descrip, "; " ExportOrientation, Orientation );


    headernii.datatype      = (short) ( VolumeFormat == AtomFormatByte ? NIFTI_TYPE_UINT8 : NIFTI_TYPE_FLOAT32 );
                                        // this is silly, just some duplicated info from above
    headernii.bitpix        = (short) AtomFormatTypePresets[ VolumeFormat ].NumBits ();


    headernii.pixdim[ 0 ]   = 1;        // qfac special value: +1 or -1
    headernii.pixdim[ 1 ]   = VoxelSize.X;
    headernii.pixdim[ 2 ]   = VoxelSize.Y;
    headernii.pixdim[ 3 ]   = VoxelSize.Z;
    headernii.pixdim[ 4 ]   = SamplingFrequency > 0 ? 1 / SamplingFrequency : 0;    // write even if single time point
    headernii.toffset       = TimeOrigin;                                           // same

                                        // a priori, we work in [mm]
    headernii.xyzt_units    = SPACE_TIME_TO_XYZT ( ExportVolumeSpaceUnits, ExportVolumeTimeUnits );

                                        // y = scl_slope * x + scl_inter
    headernii.scl_slope     = RescalingToInteger == 0 || RescalingToInteger == 1 ? 0 : 1 / RescalingToInteger;    // reverting the scaling factor used when writing data to file
    headernii.scl_inter     = 0;
                                        // prefered display calibration, if non-zero
    headernii.cal_min       = 0;
    headernii.cal_max       = 0;

                                        // !Make the Q and S transform identical, to avoid any confusion!
    headernii.qform_code    = NiftiTransform;   // ?make it at least >= NiftiTransformDefault?
    headernii.sform_code    = NiftiTransform;   // ?make it at least >= NiftiTransformDefault?

                                        // !Cartool doesn't work with anisotropic voxels, and loaded Nifti are resliced, so there is no transform at that point to be saved!
                                        // Quaternions: no transform
    headernii.quatern_b     = 0;
    headernii.quatern_c     = 0;
    headernii.quatern_d     = 0;
                                        // Quaternions: (rescaled) translation to origin
    headernii.qoffset_x     = -Origin.X * VoxelSize.X;
    headernii.qoffset_y     = -Origin.Y * VoxelSize.Y;
    headernii.qoffset_z     = -Origin.Z * VoxelSize.Z;

                                        // Matrix: voxel rescaling + translation, so when opening file it rescale to its original size
    headernii.srow_x[ 0 ]   = VoxelSize.X;  headernii.srow_x[ 1 ]   = 0;            headernii.srow_x[ 2 ]   = 0;            headernii.srow_x[ 3 ]   = -Origin.X * VoxelSize.X;
    headernii.srow_y[ 0 ]   = 0;            headernii.srow_y[ 1 ]   = VoxelSize.Y;  headernii.srow_y[ 2 ]   = 0;            headernii.srow_y[ 3 ]   = -Origin.Y * VoxelSize.Y;
    headernii.srow_z[ 0 ]   = 0;            headernii.srow_z[ 1 ]   = 0;            headernii.srow_z[ 2 ]   = VoxelSize.Z;  headernii.srow_z[ 3 ]   = -Origin.Z * VoxelSize.Z;

                                        // Data offset - some package require multiple of 16
    headernii.vox_offset    = RoundToAbove ( headernii.sizeof_hdr, 16 );

                                        // Send the whole header
    of->write ( (char *) &headernii, sizeof ( headernii ) );

                                        // Fill any optional space between header and beginning of data
    for ( int offset = headernii.vox_offset - sizeof ( headernii ); offset > 0; offset-- )

        of->put ( (UCHAR) 0 );

                                        // Here we are good to write the data
    } // FILEEXT_MRINII

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( StringIs ( Type, FILEEXT_MRIAVS ) ) {
                                        // write header for AVS Express
    *of << "# AVS field file"        << fastendl;
    *of << "# " << ExportedByCartool << fastendl;
                                        // has some orientation info to sneak in?
    if ( StringLength ( Orientation ) == 3 )
        *of << "# " << ExportOrientation << Orientation << fastendl;
    *of << "#" << fastendl;

    *of << "ndim=3"                 << fastendl;
    *of << "dim1=" << Dimension.X   << fastendl;
    *of << "dim2=" << Dimension.Y   << fastendl;
    *of << "dim3=" << Dimension.Z   << fastendl;
    *of << "nspace=3"               << fastendl;    // 3D data
    *of << "veclen=1"               << fastendl;    // 1 value per point

                                        // New AVS Express are (supposedly the native types): byte, integer, float, double
                                        // Old AVS (?): byte, short (long?), ...?
    *of << "data=" << ( VolumeFormat == AtomFormatByte ? "byte" : "float" ) << fastendl;
    *of << "field=uniform"          << fastendl;    // opposite of 'irregular'

    *of << "min_ext=0.0 0.0 0.0"    << fastendl;
    *of << "max_ext=" << RealSize.X << " " 
                      << RealSize.Y << " " 
                      << RealSize.Z << fastendl;

    *of << "min_val=0"              << fastendl;
    *of << "max_val=" << MaxValue   << fastendl;

                                        // there does not seem to be any scaling factors in AVS -> force reset rescaling
    RescalingToInteger  = 1;
     

    of->put ( (char) 0x0C );            // 2 FF
    of->put ( (char) 0x0C );

    } // FILEEXT_MRIAVS

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Note that the vmr file format implementation is very limited
else if ( StringIs ( Type, FILEEXT_MRIVMR ) ) {

                                        // write header
    short int           i16;

                                        // 2 short int to 0, don't know what for...
    i16         = 0;
    of->write ( (char *) &i16, sizeof ( i16 ) );
    of->write ( (char *) &i16, sizeof ( i16 ) );

                                        // dimensions X, Y, Z
    i16         = Dimension.X;      of->write ( (char *) &i16, sizeof ( i16 ) );
    i16         = Dimension.Y;      of->write ( (char *) &i16, sizeof ( i16 ) );
    i16         = Dimension.Z;      of->write ( (char *) &i16, sizeof ( i16 ) );

                                        // format is loosely implemented
    RescalingToInteger  = 1;
     
    } // FILEEXT_MRIVMR

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                            // remember where data starts, mostly for binary files
EndOfHeader     = of->tellp ();
}


//----------------------------------------------------------------------------
/*
void    TExportVolume::Write ( int value )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


if      ( VolumeFormat == AtomFormatByte ) {

    UCHAR               ucv             = Clip ( Round ( RescalingToInteger * value ), 0, (int) Highest<UCHAR>() );

    of->put ( ucv );
    }


else if ( VolumeFormat == AtomFormatFloat ) {

    float               f               = RescalingToInteger * value;

                                        // AVS files come from UNIX world
    if ( StringIs ( Type, FILEEXT_MRIAVS ) )
        SwappedBytes ( f );

    of->write ( (char *) &f, AtomSize );
    }


CurrentPosition++;
                                        // this should be the end!
if ( CurrentPosition >= GetLinearDim () )
    End ();
}
*/

//----------------------------------------------------------------------------
void    TExportVolume::Write ( double value )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


if      ( VolumeFormat == AtomFormatByte ) {

    UCHAR               ucv             = Clip ( Round ( RescalingToInteger * value ), 0, (int) Highest<UCHAR>() );

    of->put ( ucv );
    }


else if ( VolumeFormat == AtomFormatFloat ) {

    float               f               = RescalingToInteger * value;

                                        // AVS files come from UNIX world
    if ( StringIs ( Type, FILEEXT_MRIAVS ) )
        SwappedBytes ( f );

    of->write ( (char *) &f, AtomSize );
    }


CurrentPosition++;
                                        // this should be the end!
if ( CurrentPosition >= GetLinearDim () )
    End ();
}


//----------------------------------------------------------------------------
void    TExportVolume::Write ( const TVolume<UCHAR>& vol, ExportArrayOrder arrayorder )
{
                                        // should have been done before by the caller, this will save the day only for a first call
Maxed ( MaxValue, (double) vol.GetMaxValue () );

                                        // output content
if ( arrayorder == ExportArrayOrderZYX )

    for ( int z = 0; z < vol.GetDim3 (); z++ ) {

        UpdateApplication;

        for ( int y = 0; y < vol.GetDim2 (); y++ )
        for ( int x = 0; x < vol.GetDim1 (); x++ )
            Write ( vol ( x, y, z ) );
        }
else
    for ( int x = 0; x < vol.GetDim1 (); x++ ) {

        UpdateApplication;

        for ( int y = 0; y < vol.GetDim2 (); y++ )
        for ( int z = 0; z < vol.GetDim3 (); z++ )
            Write ( vol ( x, y, z ) );
        }
}


//----------------------------------------------------------------------------
void    TExportVolume::Write ( const TVolume<UINT>& vol, ExportArrayOrder arrayorder )
{
                                        // should have been done before by the caller, this will save the day only for a first call
Maxed ( MaxValue, (double) vol.GetMaxValue () );

                                        // output content
if ( arrayorder == ExportArrayOrderZYX )

    for ( int z = 0; z < vol.GetDim3 (); z++ ) {

        UpdateApplication;

        for ( int y = 0; y < vol.GetDim2 (); y++ )
        for ( int x = 0; x < vol.GetDim1 (); x++ )
            Write ( vol ( x, y, z ) );
        }
else
    for ( int x = 0; x < vol.GetDim1 (); x++ ) {

        UpdateApplication;

        for ( int y = 0; y < vol.GetDim2 (); y++ )
        for ( int z = 0; z < vol.GetDim3 (); z++ )
            Write ( vol ( x, y, z ) );
        }
}


//----------------------------------------------------------------------------
void    TExportVolume::Write ( const TVolume<float>& vol, ExportArrayOrder arrayorder )
{
                                        // should have been done before by the caller, this will save the day only for a first call
Maxed ( MaxValue, (double) vol.GetMaxValue () );

                                        // output content
if ( arrayorder == ExportArrayOrderZYX )

    for ( int z = 0; z < vol.GetDim3 (); z++ ) {

        UpdateApplication;

        for ( int y = 0; y < vol.GetDim2 (); y++ )
        for ( int x = 0; x < vol.GetDim1 (); x++ )
            Write ( vol ( x, y, z ) );
        }
else
    for ( int x = 0; x < vol.GetDim1 (); x++ ) {

        UpdateApplication;

        for ( int y = 0; y < vol.GetDim2 (); y++ )
        for ( int z = 0; z < vol.GetDim3 (); z++ )
            Write ( vol ( x, y, z ) );
        }
}


//----------------------------------------------------------------------------
void    TExportVolume::Write ( const TVolume<double>& vol, ExportArrayOrder arrayorder )
{
                                        // should have been done before by the caller, this will save the day only for a first call
Maxed ( MaxValue, vol.GetMaxValue () );

                                        // output content
if ( arrayorder == ExportArrayOrderZYX )

    for ( int z = 0; z < vol.GetDim3 (); z++ ) {

        UpdateApplication;

        for ( int y = 0; y < vol.GetDim2 (); y++ )
        for ( int x = 0; x < vol.GetDim1 (); x++ )
            Write ( vol ( x, y, z ) );
        }
else
    for ( int x = 0; x < vol.GetDim1 (); x++ ) {

        UpdateApplication;

        for ( int y = 0; y < vol.GetDim2 (); y++ )
        for ( int z = 0; z < vol.GetDim3 (); z++ )
            Write ( vol ( x, y, z ) );
        }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
