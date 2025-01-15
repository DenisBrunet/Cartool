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

#include    <owl/pch.h>

#include    "MemUtil.h"

#include    "Dialogs.TSuperGauge.h"
#include    "Math.Utils.h"
#include    "TExportVolume.h"

#include    "Math.Resampling.h"

#include    "TVolumeVmrDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TVolumeVmrDoc::TVolumeVmrDoc ( TDocument* parent )
      : TVolumeDoc ( parent )
{
}


//----------------------------------------------------------------------------

bool	TVolumeVmrDoc::Commit ( bool force )
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


bool                savingvmr       = crtl::IsExtension ( expvol.Filename, FILEEXT_MRIVMR );


SetDocPath ( expvol.Filename );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // don't invert the Besa-ification if saving to another format
bool                InvertBesa      = ForceBesa && savingvmr;
                                        // we need a copy of Data that can be un Besa-ed if needed
Volume              SavingData;
int                 xindex          = InvertBesa ? 1 : 0;
int                 yindex          = InvertBesa ? 2 : 1;
int                 zindex          = InvertBesa ? 0 : 2;


if ( InvertBesa ) {
                                        // permutate axis dimensions (RAS -> PIR so XYZ -> YZX)
    int             dim1o ( Data.GetDim ( xindex ) );
    int             dim2o ( Data.GetDim ( yindex ) );
    int             dim3o ( Data.GetDim ( zindex ) );

                                        // reallocate original array
    SavingData.Resize ( dim1o, dim2o, dim3o );

                                        // redistribute the data
    for ( int x = 0; x < SavingData.GetDim1 (); x++ )
    for ( int y = 0; y < SavingData.GetDim2 (); y++ )
    for ( int z = 0; z < SavingData.GetDim3 (); z++ )

        SavingData ( x, y, z )  = Data ( z, dim1o - 1 - x, dim2o - 1 - y );
    }
else
    SavingData      = Data;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

expvol.VolumeFormat     = GetVolumeAtomType ( &Data, FilterTypeNone, InterpolateUnknown, ToExtension ( expvol.Filename ) );

expvol.MaxValue         = Data.GetAbsMaxValue ();

expvol.Dim      [ 0 ]   = SavingData.GetDim1 ();
expvol.Dim      [ 1 ]   = SavingData.GetDim2 ();
expvol.Dim      [ 2 ]   = SavingData.GetDim3 ();

expvol.VoxelSize.X      = VoxelSize[ xindex ];
expvol.VoxelSize.Y      = VoxelSize[ yindex ];
expvol.VoxelSize.Z      = VoxelSize[ zindex ];

expvol.Origin.X         = Origin   [ xindex ];
expvol.Origin.Y         = Origin   [ yindex ];
expvol.Origin.Z         = Origin   [ zindex ];

                                        // Do we have some knowledge here of the type of data?
expvol.NiftiTransform   = NiftiTransform;   // ?maybe some filtering should reset that?

expvol.NiftiIntentCode  = NiftiIntentCode;

StringCopy  ( expvol.NiftiIntentName, NiftiIntentName, NiftiIntentNameSize - 1 );


//if ( KnownOrientation )               // always saving orientation?
    OrientationToString ( expvol.Orientation );

//expvol.Orientation[ 0 ] = 'P';
//expvol.Orientation[ 1 ] = 'I';
//expvol.Orientation[ 2 ] = 'R';


if ( savingvmr )
                                        // explicit write as to perform the LR flip
    for ( int z = 0; z < SavingData.GetDim3 (); z++ )
    for ( int y = 0; y < SavingData.GetDim2 (); y++ )
    for ( int x = 0; x < SavingData.GetDim1 (); x++ )
                                        // officially, format is PIL (though some packages could be using PIR...)
        expvol.Write ( SavingData ( x, y, SavingData.GetDim3 () - 1 - z ) );
else
    expvol.Write ( SavingData, ExportArrayOrderZYX );


SetDirty ( false );


return  true;
}


//----------------------------------------------------------------------------
//      There seems to be very little informations on this BrainVoyager format:
// Update:
// http://support.brainvoyager.com/installation-introduction/23-file-formats/385-developer-guide-26-the-format-of-vmr-files.html
//
// http://brainvoyager.de/BV2000OnlineHelp/BrainVoyagerWebHelp/mergedProjects/FileFormats/The_format_of_VMR_files.htm
// http://support.brainvoyager.com/available-tools/49-available-plugins/166-nifti-conversion-volumetric-files.html
// http://i2at.msstate.edu/pdf/I2AT_Brain_Voyager_Manual_Rev11.19.pdf
// http://support.brainvoyager.com/documents/Available_Tools/Available_Plugins/niftiplugin_manual_180610.pdf


bool	TVolumeVmrDoc::Open ( int /*mode*/, const char* path )
{
if ( path )
    SetDocPath ( path );

SetDirty ( false );


if ( GetDocPath () ) {

    ifstream            ifs ( TFileName ( GetDocPath (), TFilenameExtendedPath ), ios::binary );

    if ( ! ifs.good () )
        return  false;


    ushort              version;
    ushort              unused;
    ushort              dim1;
    ushort              dim2;
    ushort              dim3;

    ifs.read ( (char *) &version, sizeof ( version ) );

    if ( version == 0 )
        ifs.read ( (char *) &unused, sizeof ( unused ) );
                                        // dimension x, y ,z
    ifs.read ( (char *) &dim1, sizeof ( dim1 ) );
    ifs.read ( (char *) &dim2, sizeof ( dim2 ) );
    ifs.read ( (char *) &dim3, sizeof ( dim3 ) );

                                        // current version is 4, but we only have files with 0
    if ( version > 0 ) {
        ShowMessage ( "Unsupported version!\nVolume might be read incorrectly!", "VMR File", ShowMessageWarning );
//      return  false;
        }

                                        // Besa files will be internally reoriented & recentered to be consistent with the internal behavior of this program
//  static TStringGrep      grepbesa ( "(?i)(_ACPC|_TAL|_TAL_MASKED)\\." );
//  ForceBesa       = grepbesa.Matched ( GetTitle () );
    ForceBesa       = version == 0;

//    DBGV ( ForceBesa, "Force BESA" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here we have all the header infos.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Filling product info
    StringCopy ( CompanyName, CompanyNameVMR );
    StringCopy ( ProductName, ProductNameVMR );
    Version     = version;
    StringCopy  ( SubversionName, ForceBesa ? "Besa" : "" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( ForceBesa ) {
                                        // permutate axis dimensions (PIR -> RAS so XYZ -> ZXY)
        ushort          dim1o ( dim1 );
        ushort          dim2o ( dim2 );
        ushort          dim3o ( dim3 );

        dim1            = dim3o;
        dim2            = dim1o;
        dim3            = dim2o;
        }


    Size.Set ( 0, dim1 - 1, 0, dim2 - 1, 0, dim3 - 1 );


    Data.Resize ( dim1, dim2, dim3 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // deduce voxel size
//  if ( RealSize.IsNotNull () ) {
//      VoxelSize.X = RealSize.X / dim1;
//      VoxelSize.Y = RealSize.Y / dim2;
//      VoxelSize.Z = RealSize.Z / dim3;
//      }
//  else {
        VoxelSize   = 1;

        RealSize.X  = dim1;
        RealSize.Y  = dim2;
        RealSize.Z  = dim3;
//      }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // format does not set the origin
    Origin  = 0.0;


    if ( ForceBesa ) {
                                        // assume origin is the center, at least for now
        Origin.X    = dim1 / 2;
        Origin.Y    = dim2 / 2;
        Origin.Z    = dim3 / 2;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // default is always PIL, we set to PIR and flip the data in Z below
    KnownOrientation    = ForceBesa ? SetOrientation ( "RAS" ) : SetOrientation ( "PIR" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    TSuperGauge         Gauge;
    Gauge.Set           ( MriVmrTitle, SuperGaugeLevelInter );
    Gauge.AddPart       ( 0, dim3, 100 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Data encountered so far seem to be only unsigned byte, so there is no need for rescaling
    if ( ForceBesa ) {
                                        // override orientation
        for ( int z = 0; z < dim1; z++ ) {

            Gauge.Next ( 0 );

            for ( int y = 0; y < dim3; y++ )
            for ( int x = 0; x < dim2; x++ ) {
                              // !Left!
                Data ( dim1 - 1 - z, dim2 - 1 - x, dim3 - 1 - y )   = (uchar) ifs.get ();
                }
            }
        }
    else {
                                        // regular file
        for ( int z = 0; z < dim3; z++ ) {

            Gauge.Next ( 0 );

            for ( int y = 0; y < dim2; y++ )
            for ( int x = 0; x < dim1; x++ ) {
                                    // !Left! officially, format is PIL (though some packages could be using PIR...)
                Data ( x, y, dim3 - 1 - z ) = (uchar) ifs.get ();
                }
            }
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
                                MriVmrMethodTitle ) ) {

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
    return false;
    }


return true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
