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

#include    "TVolumeAvsDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TVolumeAvsDoc::TVolumeAvsDoc ( TDocument* parent )
      : TVolumeDoc ( parent )
{
}


//----------------------------------------------------------------------------
bool	TVolumeAvsDoc::Commit ( bool force )
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
bool	TVolumeAvsDoc::Open ( int /*mode*/, const char* path )
{
if ( path )
    SetDocPath ( path );


SetDirty ( false );


if ( GetDocPath () ) {
                                        // first open in text mode
    TInStream*          InputStream     = InStream ( ofRead );

    if ( ! InputStream )
        return false;


    char                buff    [ 256 ];
    char                datatype[ 32 ];
    int                 dim1;
    int                 dim2;
    int                 dim3;
    double              minval  = -1;
    double              maxval  = -1;
    int                 ndim;
    int                 nspace;
    int                 veclen;
    char                field[32];
    int                 numfieldsneeded = 0;


    do {
        InputStream->getline ( buff, 256 );

                                        // some orientation info in the comments?
        if ( StringContains ( (const char*) buff, (const char*) ExportOrientation ) ) {
                                        // get the 3 letters that summarize the orientation
            const char*			toorientation       = StringContains ( (const char*) buff, (const char*) ExportOrientation ) + StringLength ( ExportOrientation );

//            DBGM ( toorientation, "Opening: Orientation" );
                                        // try to use them
            KnownOrientation    = SetOrientation ( toorientation );
            }

                                        // other comments
        if ( StringStartsWith ( buff, "#" ) )  continue;


        if ( StringStartsWith ( buff, "ndim" ) ) {
            numfieldsneeded++;
            sscanf ( buff, "ndim=%d", &ndim );
            if ( ndim != 3 ) {
                ShowMessage ( "Unrecognized file format!\nNumber of dimensions does not match.", "Open File", ShowMessageWarning );
                numfieldsneeded = -100;
                break;
                }
            continue;
            }

        if ( StringStartsWith ( buff, "dim1" ) ) {
            numfieldsneeded++;
            sscanf ( buff, "dim1=%d", &dim1 );
            if ( dim1 <= 0 || dim1 > 256 ) {
                ShowMessage ( "Dimension 1 does not match!", "Open File", ShowMessageWarning );
                numfieldsneeded = -100;
                break;
                }
            continue;
            }
        if ( StringStartsWith ( buff, "dim2" ) ) {
            numfieldsneeded++;
            sscanf ( buff, "dim2=%d", &dim2 );
            if ( dim2 <= 0 || dim2 > 256 ) {
                ShowMessage ( "Dimension 2 does not match!", "Open File", ShowMessageWarning );
                numfieldsneeded = -100;
                break;
                }
            continue;
            }
        if ( StringStartsWith ( buff, "dim3" ) ) {
            numfieldsneeded++;
            sscanf ( buff, "dim3=%d", &dim3 );
            if ( dim3 <= 0 || dim3 > 256 ) {
                ShowMessage ( "Dimension 3 does not match!", "Open File", ShowMessageWarning );
                numfieldsneeded = -100;
                break;
                }
            continue;
            }


        if ( StringStartsWith ( buff, "min_val" ) ) {
            numfieldsneeded++;
            sscanf ( buff, "min_val=%lf", &minval );
            continue;
            }
        if ( StringStartsWith ( buff, "max_val" ) ) {
            numfieldsneeded++;
            sscanf ( buff, "max_val=%lf", &maxval );
            continue;
            }

        if ( StringStartsWith ( buff, "max_ext" ) ) {
            sscanf ( buff, "max_ext=%lf %lf %lf", &RealSize.X, &RealSize.Y, &RealSize.Z );
            continue;
            }


        if ( StringStartsWith ( buff, "nspace" ) ) {
            sscanf ( buff, "nspace=%d", &nspace );
            if ( nspace != 3 ) {
                ShowMessage ( "Unrecognized file format!", "Open File", ShowMessageWarning );
                numfieldsneeded = -100;
                break;
                }
            continue;
            }

        if ( StringStartsWith ( buff, "veclen" ) ) {
            sscanf ( buff, "veclen=%d", &veclen );
            if ( veclen != 1 ) {
                ShowMessage ( "Unrecognized file format!", "Open File", ShowMessageWarning );
                numfieldsneeded = -100;
                break;
                }
            continue;
            }

        if ( StringStartsWith ( buff, "data" ) ) {
            numfieldsneeded++;
            sscanf ( buff, "data=%s", datatype );
            if ( ! IsStringAmong ( datatype, MriAvsDataTypes ) ) {
                ShowMessage ( "Unrecognized file format!\nData type is not byte or short.", "Open File", ShowMessageWarning );
                numfieldsneeded = -100;
                break;
                }
            continue;
            }

        if ( StringStartsWith ( buff, "field" ) ) {
            sscanf ( buff, "field=%s", field );
            if ( StringIsNot ( field, "uniform" ) ) {
                ShowMessage ( "Unrecognized file format!\nField is not uniform.", "Open File", ShowMessageWarning );
                numfieldsneeded = -100;
                break;
                }
            continue;
            }

        } while ( *buff != 0x0C );

    delete  InputStream;


    if ( numfieldsneeded < 5 )
        return false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here we have all the header infos.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Filling product info
    StringCopy ( CompanyName, CompanyNameAVS );
    StringCopy ( ProductName, ProductNameAVS );
//  Version     = ndim;
//  Subversion  = veclen;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    int     filedim1        = dim1;
    int     filedim2        = dim2;
    int     filedim3        = dim3;

    Size.Set ( 0, dim1 - 1, 0, dim2 - 1, 0, dim3 - 1 );

    Data.Resize ( dim1, dim2, dim3 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // deduce voxel size
    if ( RealSize.IsNotNull () ) {
        VoxelSize.X     = RealSize.X / dim1;
        VoxelSize.Y     = RealSize.Y / dim2;
        VoxelSize.Z     = RealSize.Z / dim3;
        }
    else {
        VoxelSize       = 1;

        RealSize.X      = dim1;
        RealSize.Y      = dim2;
        RealSize.Z      = dim3;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Origin  = 0.0;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    TSuperGauge         Gauge;
    Gauge.Set           ( MriAvsTitle, SuperGaugeLevelInter );
    Gauge.AddPart       ( 0, filedim3, 100 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // reopen in binary to get the data
    InputStream     = InStream ( ofRead | ofBinary );

    if ( ! InputStream ) 
        return false;

                                        // skip header until ctrl-L alias FF
    do  InputStream->get ( *buff ); while ( *buff != 0x0c );

    do  InputStream->get ( *buff ); while ( *buff == 0x0c );

    InputStream->putback ( *buff );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if      ( StringIs ( datatype, "byte" ) ) {

        for ( int z=0; z < filedim3; z++ ) {

            Gauge.Next ( 0 );

            for ( int y=0; y < filedim2; y++ )
            for ( int x=0; x < filedim1; x++ ) {

                Data( x, y, z ) = (uchar) InputStream->get ();
                }
            }
        }

    else if ( StringIs ( datatype, "short" ) ) {

        uint16          sd;

        for ( int z=0; z < filedim3; z++ ) {

            Gauge.Next ( 0 );

            for ( int y=0; y < filedim2; y++ )
            for ( int x=0; x < filedim1; x++ ) {

                InputStream->read ( (char *) &sd, 2 );

                SwappedBytes ( sd );

                Data( x, y, z ) = sd;
                }
            }
        }

    else if ( StringIs ( datatype, "float" ) ) {

        float               f;

        for ( int z=0; z < filedim3; z++ ) {

            Gauge.Next ( 0 );

            for ( int y=0; y < filedim2; y++ )
            for ( int x=0; x < filedim1; x++ ) {

                InputStream->read ( (char *) &f, 4 );

                SwappedBytes ( f );
                                        // ignore NaN
                Data( x, y, z ) = IsNotAProperNumber ( f ) ? 0 : f;
                }
            }
        }


    delete  InputStream;


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
                                MriAvsMethodTitle ) ) {

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
else {                                  // create?
    return false;
    }


return true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
