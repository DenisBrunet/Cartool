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
{
of                  = 0;

Reset ();

                                        // Fields below are set only at creation, NOT via Reset

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
End ();


Filename.Reset ();
ClearString ( Type );
                                        // Not resetting VolumeFormat

Dim      [ 0 ]      =
Dim      [ 1 ]      = 
Dim      [ 2 ]      = 
Dim      [ 3 ]      = 0;
VoxelSize.Reset ();
RealSize .Reset ();
Origin   .Reset ();

ClearString ( Orientation, 4 );

MaxValue            = 0;
                                        // Not resetting these Nifti options:  NiftiTransform, NiftiIntentCode, NiftiIntentParameters, NiftiIntentName

                                        // internal variables
DoneBegin           = false;
EndOfHeader         = 0;
CurrentPosition     = 0;
AtomSize            = 1;
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

                                        // compute the correct size to be filled after the header
size_t              sizetoreset     = (LONGLONG) Dim[ 0 ] * Dim[ 1 ] * Dim[ 2 ] /** Dim[ 3 ]*/ * AtomSize;


AllocateFileSpace   ( of, sizetoreset, FILE_CURRENT );

                                        // go back to data origin
of->seekp ( EndOfHeader, ios::beg );
}


//----------------------------------------------------------------------------
void    TExportVolume::End ()
{
if ( IsOpen () ) {

//  of->flush ();
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
Filename.CheckFileName ( (TFilenameFlags) ( TFilenameExtendedPath | TFilenameSibling ) );


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


CheckVolumeFormat ();

                                        // retrieve atom memory size
AtomSize    = AtomFormatTypePresets[ VolumeFormat ].NumBytes;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do some checking

                                        // don't force max if it is not known
//if ( MaxValue == 0 )
//    MaxValue    = VolumeFormat == AtomFormatByte ? UCHAR_MAX : 0;


if ( ! ( RealSize.X && RealSize.Y && RealSize.Z ) ) {

    if ( VoxelSize.X && VoxelSize.Y && VoxelSize.Z ) {
        RealSize.X      = VoxelSize.X * Dim[ 0 ];
        RealSize.Y      = VoxelSize.Y * Dim[ 1 ];
        RealSize.Z      = VoxelSize.Z * Dim[ 2 ];
        }
    else {
        RealSize.X      = Dim[ 0 ];
        RealSize.Y      = Dim[ 1 ];
        RealSize.Z      = Dim[ 2 ];
        }
    }


if ( ! ( VoxelSize.X && VoxelSize.Y && VoxelSize.Z ) ) {

    if ( RealSize.X && RealSize.Y && RealSize.Z ) {
        VoxelSize.X     = RealSize.X / Dim[ 0 ];
        VoxelSize.Y     = RealSize.Y / Dim[ 1 ];
        VoxelSize.Z     = RealSize.Z / Dim[ 2 ];
        }
    else
        VoxelSize       = 1.0;
    }


//DBGV3 ( Dim[ 0 ], Dim[ 1 ], Dim[ 2 ], "Export Volume: new Dimension" );
//DBGV3 ( RealSize[ 0 ], RealSize[ 1 ], RealSize[ 2 ], "Export Volume: new Real Size" );
//DBGV3 ( VoxelSize[ 0 ], VoxelSize[ 1 ], VoxelSize[ 2 ], "Export Volume: new Voxel Size" );
//DBGV3 ( Origin.X, Origin.Y, Origin.Z, "Export Volume: new Origin" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // create the correct type of file
of      = new ofstream ( Filename, ios::binary );
//of      = new ofstream ( std::filesystem::u8path ( Filename.FileName ), ios::binary );


if ( ! of->good () ) {
    ShowMessage ( "There seems to be a problem while trying to write this file." NewLine "Maybe it is write-protected, could you check that for me, as I have no hands...", ToFileName ( Filename ), ShowMessageWarning );
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


    StringCopy ( hout.hk.db_name, "Cartool" );
    StringCopy ( hout.hist.descrip, ExportedByCartool );

                                        // has some orientation info to sneak in (& enough space)?
    if (    StringLength ( Orientation ) == 3 
         && StringLength ( hout.hist.descrip ) + StringLength ( "; " ExportOrientation ) + 4 < 80 )

        StringAppend ( hout.hist.descrip, "; " ExportOrientation, Orientation );

                                        // output either a 3 or a 4 dimensional volume
    hout.dime.dim[ 0 ]      = (short) ( Dim[ 3 ] ? 4 : 3 );
    hout.dime.dim[ 1 ]      = (short)   Dim[ 0 ];
    hout.dime.dim[ 2 ]      = (short)   Dim[ 1 ];
    hout.dime.dim[ 3 ]      = (short)   Dim[ 2 ];
    hout.dime.dim[ 4 ]      = (short) ( Dim[ 3 ] ? 1 : 0 );     // for now, only 1 time point

                                        // pixdim is parallel to dim, but pixdim[ 0 ] does not mean anything
    hout.dime.pixdim[ 1 ]   = VoxelSize.X;
    hout.dime.pixdim[ 2 ]   = VoxelSize.Y;
    hout.dime.pixdim[ 3 ]   = VoxelSize.Z;
    hout.dime.pixdim[ 4 ]   = Dim[ 3 ] ? 1 : 0;


    hout.dime.vox_offset    = 0;        // offset of voxels inside .img
    hout.dime.glmin         = 0;
    hout.dime.glmax         = (int) ( VolumeFormat == AtomFormatFloat ? 0 : MaxValue );   // glmax is int, so it does not work as a max for float!
    hout.dime.cal_min       = 0;
    hout.dime.cal_max       = 0;        // no calibration (used to be MaxValue)
    hout.dime.funused1      = 1;        // SPM scale factor
    hout.dime.funused2      = 0;        // SPM offset factor
    hout.dime.datatype      = (short) ( VolumeFormat == AtomFormatByte ? DT_UNSIGNED_CHAR : DT_FLOAT );
    hout.dime.bitpix        = (short) AtomFormatTypePresets[ VolumeFormat ].NumBits ();


    hout.hist.orient        = 0;        // there are no flags to tell we are in RAS f.ex., or any of the 48 possible right-handed orientations

                                        // we have to round to closest voxel, as Analyze store origin in short int...
    ((short *) hout.hist.originator)[ 0 ]   = (short) Truncate ( Origin.X );
    ((short *) hout.hist.originator)[ 1 ]   = (short) Truncate ( Origin.Y );
    ((short *) hout.hist.originator)[ 2 ]   = (short) Truncate ( Origin.Z );


    of->write ( (char *) &hout, sizeof ( hout ) );


                                        // finished with header
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
    headernii.dim[ 0 ]      = ( Dim[ 3 ] ? 4 : 3 );
    headernii.dim[ 1 ]      =   Dim[ 0 ];
    headernii.dim[ 2 ]      =   Dim[ 1 ];
    headernii.dim[ 3 ]      =   Dim[ 2 ];
    headernii.dim[ 4 ]      = ( Dim[ 3 ] ? 1 : 0 );     // for now, only 1 time point

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


//    StringCopy  ( headernii.descrip, NiftiDescripLabels, "1Scalp" "4CSF" "5Blood" "6Eye" "7Air" "9SkullC" "10SkullS" "13Grey" "14White" );
    //TSelection          seltissues ( NumTissuesIndex, OrderSorted );
    //seltissues.Set ( ScalpIndex         );
    //seltissues.Set ( CsfIndex           );
    //seltissues.Set ( BloodIndex         );
    //seltissues.Set ( EyeIndex           );
    //seltissues.Set ( AirIndex           );
    //seltissues.Set ( SkullCompactIndex  );
    //seltissues.Set ( SkullSpongyIndex   );
    //seltissues.Set ( GreyIndex          );
    //seltissues.Set ( WhiteIndex         );


    headernii.datatype      = (short) ( VolumeFormat == AtomFormatByte ? NIFTI_TYPE_UINT8 : NIFTI_TYPE_FLOAT32 );
                                        // this is silly, just some duplicated info from above
    headernii.bitpix        = (short) AtomFormatTypePresets[ VolumeFormat ].NumBits ();


    headernii.pixdim[ 0 ]   = 1;        // qfac special value: +1 or -1
    headernii.pixdim[ 1 ]   = VoxelSize.X;
    headernii.pixdim[ 2 ]   = VoxelSize.Y;
    headernii.pixdim[ 3 ]   = VoxelSize.Z;
    headernii.pixdim[ 4 ]   = Dim[ 3 ] ? 1 : 0;

                                        // a priori, we work in [mm]
    headernii.xyzt_units    = SPACE_TIME_TO_XYZT ( ExportVolumeSpaceUnits, ExportVolumeTimeUnits );

                                        // y = scl_slope * x + scl_inter
    headernii.scl_slope     = 1;
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
                                        // write header
                                        // see: http://help.avs.com/Express/doc/help/books/dv/dvfdtype.html
    *of << "# AVS field file"        << fastendl;
    *of << "# " << ExportedByCartool << fastendl;
                                        // has some orientation info to sneak in?
    if ( StringLength ( Orientation ) == 3 )
        *of << "# " << ExportOrientation << Orientation << fastendl;
    *of << "#" << fastendl;

    *of << "ndim=3"                 << fastendl;
    *of << "dim1=" << Dim[ 0 ]      << fastendl;
    *of << "dim2=" << Dim[ 1 ]      << fastendl;
    *of << "dim3=" << Dim[ 2 ]      << fastendl;
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
    i16         = (short) Dim[ 0 ];
    of->write ( (char *) &i16, sizeof ( i16 ) );
    i16         = (short) Dim[ 1 ];
    of->write ( (char *) &i16, sizeof ( i16 ) );
    i16         = (short) Dim[ 2 ];
    of->write ( (char *) &i16, sizeof ( i16 ) );

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

    UCHAR               ucv             = (UCHAR) Clip ( value, 0, UCHAR_MAX );

    of->put ( ucv );
    }


else if ( VolumeFormat == AtomFormatFloat ) {

    float               f               = (float) value;

                                        // AVS files come from UNIX world
    if ( StringIs ( Type, FILEEXT_MRIAVS ) )
        SwappedBytes ( f );

    of->write ( (char *) &f, AtomSize );
    }


CurrentPosition++;

                                        // this should be the end!
if ( CurrentPosition >= Dim[ 0 ] * Dim[ 1 ] * Dim[ 2 ] )
    End ();
}
*/

//----------------------------------------------------------------------------
void    TExportVolume::Write ( double value )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


if      ( VolumeFormat == AtomFormatByte ) {

    UCHAR               ucv             = (UCHAR) Clip ( value, (double) 0, (double) UCHAR_MAX );

    of->put ( ucv );
    }


else if ( VolumeFormat == AtomFormatFloat ) {

    float               f               = (float) value;

                                        // AVS files come from UNIX world
    if ( StringIs ( Type, FILEEXT_MRIAVS ) )
        SwappedBytes ( f );

    of->write ( (char *) &f, AtomSize );
    }


CurrentPosition++;

                                        // this should be the end!
if ( CurrentPosition >= Dim[ 0 ] * Dim[ 1 ] * Dim[ 2 ] )
    End ();
}


//----------------------------------------------------------------------------
void    TExportVolume::Write ( const TVolume<UCHAR>& vol, ExportArrayOrder arrayorder )
{
                                        // get from volume
Dim[ 0 ]    = vol.GetDim1 ();
Dim[ 1 ]    = vol.GetDim2 ();
Dim[ 2 ]    = vol.GetDim3 ();
Dim[ 3 ]    = 1;                        // set for a 4 dimension volume


MaxValue    = vol.GetMaxValue ();


//TSuperGauge         Gauge ( "Writing Volume", arrayorder == ExportArrayOrderZYX ? Dim[ 2 ] : Dim[ 0 ] );

                                        // output content
if ( arrayorder == ExportArrayOrderZYX )

    for ( int z = 0; z < Dim[ 2 ]; z++ ) {

        UpdateApplication;
//      Gauge.Next ();

        for ( int y = 0; y < Dim[ 1 ]; y++ )
        for ( int x = 0; x < Dim[ 0 ]; x++ )
            Write ( vol ( x, y, z ) );
        }
else
    for ( int x = 0; x < Dim[ 0 ]; x++ ) {

        UpdateApplication;
//      Gauge.Next ();

        for ( int y = 0; y < Dim[ 1 ]; y++ )
        for ( int z = 0; z < Dim[ 2 ]; z++ )
            Write ( vol ( x, y, z ) );
        }


End ();


//Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
void    TExportVolume::Write ( const TVolume<UINT>& vol, ExportArrayOrder arrayorder )
{
                                        // get from volume
Dim[ 0 ]    = vol.GetDim1 ();
Dim[ 1 ]    = vol.GetDim2 ();
Dim[ 2 ]    = vol.GetDim3 ();
Dim[ 3 ]    = 1;                        // set for a 4 dimension volume


MaxValue    = vol.GetMaxValue ();


//TSuperGauge         Gauge ( "Writing Volume", arrayorder == ExportArrayOrderZYX ? Dim[ 2 ] : Dim[ 0 ] );

                                        // output content
if ( arrayorder == ExportArrayOrderZYX )

    for ( int z = 0; z < Dim[ 2 ]; z++ ) {

        UpdateApplication;
//      Gauge.Next ();

        for ( int y = 0; y < Dim[ 1 ]; y++ )
        for ( int x = 0; x < Dim[ 0 ]; x++ )
            Write ( vol ( x, y, z ) );
        }
else
    for ( int x = 0; x < Dim[ 0 ]; x++ ) {

        UpdateApplication;
//      Gauge.Next ();

        for ( int y = 0; y < Dim[ 1 ]; y++ )
        for ( int z = 0; z < Dim[ 2 ]; z++ )
            Write ( vol ( x, y, z ) );
        }


End ();


//Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
void    TExportVolume::Write ( const TVolume<float>& vol, ExportArrayOrder arrayorder )
{
                                        // get from volume
Dim[ 0 ]    = vol.GetDim1 ();
Dim[ 1 ]    = vol.GetDim2 ();
Dim[ 2 ]    = vol.GetDim3 ();
Dim[ 3 ]    = 1;                        // set for a 4 dimension volume


MaxValue    = vol.GetMaxValue ();


//TSuperGauge         Gauge ( "Writing Volume", arrayorder == ExportArrayOrderZYX ? Dim[ 2 ] : Dim[ 0 ] );

                                        // output content
if ( arrayorder == ExportArrayOrderZYX )

    for ( int z = 0; z < Dim[ 2 ]; z++ ) {

        UpdateApplication;
//      Gauge.Next ();

        for ( int y = 0; y < Dim[ 1 ]; y++ )
        for ( int x = 0; x < Dim[ 0 ]; x++ )
            Write ( vol ( x, y, z ) );
        }
else
    for ( int x = 0; x < Dim[ 0 ]; x++ ) {

        UpdateApplication;
//      Gauge.Next ();

        for ( int y = 0; y < Dim[ 1 ]; y++ )
        for ( int z = 0; z < Dim[ 2 ]; z++ )
            Write ( vol ( x, y, z ) );
        }


End ();


//Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
void    TExportVolume::Write ( const TVolume<double>& vol, ExportArrayOrder arrayorder )
{
                                        // get from volume
Dim[ 0 ]    = vol.GetDim1 ();
Dim[ 1 ]    = vol.GetDim2 ();
Dim[ 2 ]    = vol.GetDim3 ();
Dim[ 3 ]    = 1;                        // set for a 4 dimension volume


MaxValue    = vol.GetMaxValue ();


//TSuperGauge         Gauge ( "Writing Volume", arrayorder == ExportArrayOrderZYX ? Dim[ 2 ] : Dim[ 0 ] );

                                        // output content
if ( arrayorder == ExportArrayOrderZYX )

    for ( int z = 0; z < Dim[ 2 ]; z++ ) {

        UpdateApplication;
//      Gauge.Next ();

        for ( int y = 0; y < Dim[ 1 ]; y++ )
        for ( int x = 0; x < Dim[ 0 ]; x++ )
            Write ( vol ( x, y, z ) );
        }
else
    for ( int x = 0; x < Dim[ 0 ]; x++ ) {

        UpdateApplication;
//      Gauge.Next ();

        for ( int y = 0; y < Dim[ 1 ]; y++ )
        for ( int z = 0; z < Dim[ 2 ]; z++ )
            Write ( vol ( x, y, z ) );
        }


End ();


//Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
