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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "Strings.Utils.h"
#include    "Files.Utils.h"
#include    "Files.Stream.h"

#include    "Geometry.TGeometryTransform.h"

#include    "TVolumeDoc.h"

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const char  GeometryTransformName[ NumGeometryTransformTypes ][ 32 ] = {
        "Unknown Transform",
        "Talairach",
        "Talairach",
        "Talairach",
        "Talairach",
        "Talairach",
        "Talairach",
        "Talairach",
        };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void    GetNormalizationTransform ( const TVolumeDoc*   MriFull,                const TVolumeDoc*   MriBrain,
                                    bool                estimatecenter,         TPointDouble&       MriCenter, 
                                    TMatrix44*          AbsNorm_to_RelVoxel,    TMatrix44*          RelVoxel_to_AbsNorm 
                                  )
{
MriCenter.Reset ();

if ( AbsNorm_to_RelVoxel )  AbsNorm_to_RelVoxel->SetIdentity ();
if ( RelVoxel_to_AbsNorm )  RelVoxel_to_AbsNorm->SetIdentity ();


if ( MriFull == 0 && MriBrain == 0 )

    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Get center in voxel relative space
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Trust the center to be the MRI origin?
if ( ! estimatecenter ) {

    MriCenter.Reset ();

                                        // 1.1.1) try first to get the brain origin
    if ( MriCenter.IsNull () && MriBrain && MriBrain->IsValidOrigin () )
        MriCenter   = MriBrain->GetOrigin ();

                                        // 1.1.2) if not, try the full head origin
    if ( MriCenter.IsNull () && MriFull  && MriFull ->IsValidOrigin () )
        MriCenter   = MriFull ->GetOrigin ();

    } // ! estimatecenter


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Doesn't trust the MRI origin, or it doesn't exist, we need to compute the center by starting with an estimate
if ( estimatecenter 
  || MriCenter.IsNull () ) {


    MriCenter.Reset ();
                                        // first, use bounding box to get a center

                                        // 1.2.1) try first to get the brain bounding center
    if ( MriCenter.IsNull () && MriBrain )

        MriCenter   = MriBrain->GetDefaultCenter ();

                                        // 1.2.2) if not, try the full head bounding center
    if ( MriCenter.IsNull () && MriFull )

        MriCenter   = MriFull ->GetDefaultCenter ();

                                        // 1.2.3) still not, use the middle size as last resort
    if ( MriCenter.IsNull () && MriBrain )

        MriBrain->GetSize     ()->GetMiddle ( MriCenter );

                                        // 1.2.4) then override the left-right center by using the origin, if it exists,
                                        // as it is a good starting point and if not using Sagittal tuning, this is correct
    if      ( MriBrain && MriBrain->IsValidOrigin () ) {

        int         lri     = MriBrain->GetAxisIndex ( LeftRight );
        MriCenter[ lri ]    = MriBrain->GetOrigin ()[ lri ];
        }

    else if ( MriFull  && MriFull ->IsValidOrigin () ) {

        int         lri     = MriFull ->GetAxisIndex ( LeftRight );
        MriCenter[ lri ]    = MriFull  ->GetOrigin ()[ lri ];
        }

                                        // use an integer center
    MriCenter.Truncate ();
    } // estimatecenter


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Get re-orientation rotation matrix
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // No need to compute the actual matrices here...
if ( AbsNorm_to_RelVoxel == 0 && RelVoxel_to_AbsNorm == 0 )

    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Normalization Matrix, centering and re-orienting
const TVolumeDoc*   Mri             = MriFull ? MriFull : MriBrain;
TMatrix44           AbsRAS_to_RelVol;

                                        // RAS -> Local
AbsRAS_to_RelVol    = Mri->GetStandardOrientation ( RASToLocal );

                                        // RAS -> Local -> Translate Center
AbsRAS_to_RelVol.Translate ( MriCenter.X, MriCenter.Y, MriCenter.Z, MultiplyLeft );


if ( AbsNorm_to_RelVoxel )
    *AbsNorm_to_RelVoxel    = AbsRAS_to_RelVol;


if ( RelVoxel_to_AbsNorm )
    *RelVoxel_to_AbsNorm    = TMatrix44 ( AbsRAS_to_RelVol ).Invert ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TGeometryTransform::TGeometryTransform ( TGeometryTransformType type )
{
Type        = type;

if ( Type < 0 || Type >= NumGeometryTransformTypes )
    Type    = GeometryUnknown;

StringCopy ( Name, GeometryTransformName[ Type ] );
}


//----------------------------------------------------------------------------
void    TGeometryTransform::ConvertTo ( TPointFloat& p, const TBoundingBox<double>* bounding )  const
{

if ( Type == GeometryUnknown )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Talairach

                                        // First get MNI coordinates
                                        // preprocess coordinates through this prefefined transform
if      ( Type == TalairachAvg152T1_smac ) {

    TMatrix44 toavg152fld ( 0.999807, -0.019639,  -0.000040,   -7.156305,
                            0.019639,  0.999807,  -0.000003,  -20.173638,
                            0.000040,  0.000003,   1,         -19.002158,
                            0,         0,          0,           1         );

    toavg152fld.Apply ( p );
    }

else if ( Type == TalairachAve152_sym ) {

    TMatrix44 toavg152fld ( 0.996266, -0.023904,  -0.000061,   -6.603792,
                            0.023823,  0.999647,  -0.000100,  -20.457883,
                            0.000064,  0.000101,   0.986697,  -17.169489,
                            0,         0,          0,           0.999995  );

    toavg152fld.Apply ( p );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TPointDouble        pmni;

                                        // set origin
if ( Type == TalairachAvg152T1Fld
  || Type == TalairachAvg152T1_smac
  || Type == TalairachAve152_sym ) {
    pmni.X      =   p.Z - 45;
    pmni.Y      = - p.X + 45;
    pmni.Z      = - p.Y + 55;
    }
else                                    // already centered
    pmni.Set ( p );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set scaling
if ( bounding ) {
                                        // detect the downsampling factor for a MNI152 full/brain
                                        // works for downsampling range from 1 to 5
    int                 downsampling    = 176.3 / ( bounding->GetMeanExtent () + 1 ) + 0.41;

    if ( downsampling == 0 )    downsampling = 1;

//    if ( VkQuery () )
//        DBGV3 ( bounding->GetMeanExtent (), 176.3 / ( bounding->GetMeanExtent () + 1 ), downsampling, "GetMeanExtent  ratio  downsampling" );

    pmni       *= downsampling;
    }
else if ( Type == TalairachAvg152T1Fld
       || Type == TalairachAvg152T1_smac
       || Type == TalairachAve152_sym )
                                        // we know / assume these are downsampled by 2 version
    pmni       *= 2;

                                        // here, we have MNI coordinates

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // MNI to Talairach
if ( Type == TalairachAvg152T1Fld
  || Type == TalairachAvg152T1_smac
  || Type == TalairachAve152_sym
  || Type == TalairachMNI152_2001
  || Type == TalairachMNI152_2009
  || Type == TalairachNIHPD_Infant ) {
                                        // from http://imaging.mrc-cbu.cam.ac.uk/imaging/MniTalairach, Approach 2: a non-linear transform of MNI to Talairach
                                        // use different scaling if above or below AC plane
    if ( pmni.Z > 0 ) {
        p.X     =  0.9900 * pmni.X;
        p.Y     =                    0.9688 * pmni.Y + 0.0460 * pmni.Z;
        p.Z     =                  - 0.0485 * pmni.Y + 0.9189 * pmni.Z;
        }
    else {
        p.X     =  0.9900 * pmni.X;
        p.Y     =                    0.9688 * pmni.Y + 0.0420 * pmni.Z;
        p.Z     =                  - 0.0485 * pmni.Y + 0.8390 * pmni.Z;
        }

                                        // convert from Infant to Adult / MNI152
    if ( Type == TalairachNIHPD_Infant ) {
        p.X    *= (float) 1.21988;
        p.Y    *= (float) 1.23510;
        p.Z    *= (float) 1.28654;
        }

    } // MNI to Talairach

else // if ( Type == Talairach )
//  p       = pmni;
    p.Set ( pmni );

                                        // for Talairach, precision is useless
//if      ( p.X > 0 ) p.X = (int) ( p.X + 0.5 );
//else if ( p.X < 0 ) p.X = (int) ( p.X - 0.5 );
//if      ( p.Y > 0 ) p.Y = (int) ( p.Y + 0.5 );
//else if ( p.Y < 0 ) p.Y = (int) ( p.Y - 0.5 );
//if      ( p.Z > 0 ) p.Z = (int) ( p.Z + 0.5 );
//else if ( p.Z < 0 ) p.Z = (int) ( p.Z - 0.5 );


                                        // give it a little more room, either for display or computations
if ( p.X > 70 || p.X <  -70             // right    / left
  || p.Y > 72 || p.Y < -104             // anterior / posterior
  || p.Z > 76 || p.Z <  -44  )          // superior / inferior

    p.X = p.Y = p.Z = 0;

}


//----------------------------------------------------------------------------
void    TGeometryTransform::ConvertTo ( char *filein, const TBoundingBox<double>* bounding )    const
{
TFileName           fileout;

                                        // construct output file
StringCopy      ( fileout, filein );
RemoveExtension ( fileout );
StringAppend    ( fileout, InfixToCoregistered, StringIsNotEmpty ( Name ) ? Name : "Geometry" );
AddExtension    ( fileout, ToExtension ( filein ) );

                                        // files ok?
if ( ! CanOpenFile ( filein,  CanOpenFileRead  )
  || ! CanOpenFile ( fileout, CanOpenFileWrite ) ) {
    return;
    }


ifstream            ifs ( TFileName ( filein,  TFilenameExtendedPath ) );
ofstream            ofs ( TFileName ( fileout, TFilenameExtendedPath ) );
TPointFloat         p;
char                buff[ 2 * KiloByte ];


//ofs << StreamFormatScientific;
ofs << StreamFormatFixed;

do {
    ifs.getline ( buff, 2 * KiloByte );

    if ( ifs.fail () )                  // reading error?
        break;
                                        // extract data
                                        // trick is, we can have some trailing info on the line, which we don't care!
    if ( sscanf ( buff, "%f %f %f", &p.X, &p.Y, &p.Z ) < 3 )
        continue;                       // not enough fields in this line, skip it


    ConvertTo ( p, bounding );


    ofs << StreamFormatFloat32 << p.X << Tab;
    ofs << StreamFormatFloat32 << p.Y << Tab;
    ofs << StreamFormatFloat32 << p.Z;
    ofs << NewLine;

    } while ( ! ifs.eof () );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
