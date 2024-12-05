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

#include    "TVector.h"
#include    "Strings.Utils.h"
#include    "Strings.Grep.h"
#include    "Strings.TStringsMap.h"
#include    "TFilters.h"
#include    "Dialogs.Input.h"

#include    "GlobalOptimize.Tracks.h"
#include    "Math.Resampling.h"
#include    "TVolume.h"
#include    "Volumes.AnalyzeNifti.h"
#include    "TExportTracks.h"
#include    "BrainToSolutionPointsUI.h"

#include    "TVolumeDoc.h"

#include    "TCartoolMdiClient.h"

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const char          MriContentTypeNames[ NumMriContentType ][ ContentTypeMaxChars ] = {
                    "",
                    "Full head",
                    "Segmented volume",

                    "Binary mask",
                    "ROI mask",

                    "Grey matter",
                    "White matter",
                    "Full brain",

                    "Blobs",
                    "Results of Inverse Solutions",
                    };


char*   GetMriContentTypeName   ( int ect, char* s )
{
ClearString     ( s );

                                        // Main type
if      ( IsUnknownMriType  ( ect ) )   StringCopy      ( s,        MriContentTypeNames[ 0 ] );
else if ( IsFullHead        ( ect ) )   StringCopy      ( s,        MriContentTypeNames[ 1 ] );
else if ( IsSegmented       ( ect ) )   StringCopy      ( s,        MriContentTypeNames[ 2 ] );

                                        // Additional sub-type
if      ( IsBinaryMask      ( ect ) )   StringAppend    ( s, ", ",  MriContentTypeNames[ 3 ] );
else if ( IsRoiMask         ( ect ) )   StringAppend    ( s, ", ",  MriContentTypeNames[ 4 ] );
else if ( IsGrey            ( ect ) )   StringAppend    ( s, ", ",  MriContentTypeNames[ 5 ] );
else if ( IsWhite           ( ect ) )   StringAppend    ( s, ", ",  MriContentTypeNames[ 6 ] );
else if ( IsBrain           ( ect ) )   StringAppend    ( s, ", ",  MriContentTypeNames[ 7 ] );
else if ( IsBlob            ( ect ) )   StringAppend    ( s, ", ",  MriContentTypeNames[ 8 ] );
else if ( IsRis             ( ect ) )   StringAppend    ( s, ", ",  MriContentTypeNames[ 9 ] );


return  s;
}


void    ShowMriContentType  ( int ect, const char* title )
{
char                info[ 256 ];

GetMriContentTypeName   ( ect, info );

ShowMessage             ( info, title ? title : "Mri Content" );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TVolumeDoc::TVolumeDoc ( TDocument *parent )
      : TBaseDoc ( parent )
{
ContentType         = ContentTypeVolume;
ExtraContentType    = UnknownMriContentType;
CopyVirtualMemory ( ExtraContentTypeNames, MriContentTypeNames, NumMriContentType * ContentTypeMaxChars );

BackgroundValue     = 0;
IsoSurfaceCut       = 0;
IsoDownsampling     = -1;

VoxelSize           = 1.0;
RealSize            = 0.0;
Origin              = 0.0;

KnownOrientation    = false;

NiftiTransform      = NiftiTransformDefault;
NiftiIntentCode     = NiftiIntentCodeDefault;
StringCopy  ( NiftiIntentName, NiftiIntentNameDefault, NiftiIntentNameSize - 1 );

DisplaySpaces.Set ( MRIDOC_NUMSPACES );
Bounding            = & DisplaySpaces[ 0 ].Bounding;
}


//----------------------------------------------------------------------------
bool    TVolumeDoc::IsFullHead ()  const
{
                                        // straightforward answer:
//return ::IsFullHead ( ExtraContentType );

                                        // but we can do better than that:

                                        // guess from content - note that some contrast files are detected as segmented, which they are actually!
bool                isfullheadcont  = crtl::IsFullHead  ( ExtraContentType );

                                        // guess from file name, as content might fail
MriContentType      guessmriname    = GuessTypeFromFilename ( GetDocPath () );

bool                isfullheadname  = crtl::IsFullHead  ( guessmriname );     // if we get lucky, file contains something like "Head"

bool                issegmentname   = crtl::IsGrey      ( guessmriname )      // if we get lucky, file contains something like "Brain" or "Grey"
                                   || crtl::IsWhite     ( guessmriname )
                                   || crtl::IsRoiMask   ( guessmriname )
                                   || crtl::IsBrain     ( guessmriname );

                                        // finally combining all these guesses together
bool                isfullhead      = (     isfullheadcont      // either content
                                       ||   isfullheadname )    // or file name
                                     &&   ! issegmentname;      // but not if Brain / Grey in name

//DBGM4 ( BoolToString ( isfullheadcont ), BoolToString ( isfullheadname ), BoolToString ( ! issegmentname ), BoolToString ( isfullhead ), "isfullheadcont isfullheadname !issegmentname isfullhead" );


return  isfullhead;
}


const char*     TVolumeDoc::GetContentTypeName ( char* s ) const
{
if ( s == 0 )
    return  "";

if ( IsUnknownMriType () )  StringCopy ( s, ContentNames[ ContentType ] );  // just to show something
else                        GetMriContentTypeName ( ExtraContentType, s );

return  s;
}


//        TVolumeDoc::~TVolumeDoc ()
//{
//ResetMri(); // actually called in Close
//}


//----------------------------------------------------------------------------
void    TVolumeDoc::ResetMri ()
{
//TessSurfacePos.Reset ();  // don't - we take advantage of existing tessellation in SetMri
//TessSurfaceNeg.Reset ();
}


//----------------------------------------------------------------------------
void    TVolumeDoc::SetMri (   MriInitFlags    flags,  double  backgroundvalue
                            )
{

InitLimits ();                          // scan limits, f.ex. if some filtering has been applied


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( IsFlag ( flags, MriInitBackgroundDetect ) )
                                        // important value, extracted from histogram
    SetBackgroundValue ();
else // MriInitBackgroundKeep
                                        // background value must be within data range
    BackgroundValue   = Clip ( backgroundvalue, MinValue, MaxValue );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Not needed anymore?
//if ( IsFlag ( flags, MriInitIsoSurfaceCutDetect ) )
//
//    IsoSurfaceCut   = BackgroundValue;


double          lowcut          = AtLeast ( 0.0, MinValue ) * 0.75 + BackgroundValue * 0.25;

if ( IsFlag ( flags, MriInitTypeDetect ) ) {

    SetBounding     ( lowcut );         // box the data with a large boundary

    SetContentType  ();                 // do all sorts of guesses (segmented brain, mask, etc...)  (use Bounding & BackgroundValue)
    }

//DBGV5 ( IsFlag ( flags, MriInitTypeDetect ), IsFullHead (), IsSegmented (), IsMask (), IsTemplate (), "MRI: (autotype)  IsFullHead (), IsSegmented (), IsMask (), IsTemplate ()" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( IsFlag ( flags, MriInitIsoSurfaceCutDetect ) )

    SetIsoSurfaceCut ();                // set default threshold value for isosurface               (use ExtraContentType & BackgroundValue)


SetBounding ( IsBlob () ? lowcut : IsoSurfaceCut );  // keep a large boundary                       (use IsoSurfaceCut)

if ( IsFlag ( flags, MriInitReorientDetect ) )

    if ( ! KnownOrientation )           // if orientation not specified from file

        FindOrientation ();             // guess orientation                                        (use ExtraContentType, Bounding & BackgroundValue)


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( IsBlob () )

    SetBounding ( IsoSurfaceCut );      // set to smaller bounding, using IsoSurfaceCut             (use IsoSurfaceCut)


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Don't initialize the tesselation surface here - a costly operation - and let the first view do that
                                        // On the other hand, if it already exists, it has to be redone
if ( TessSurfacePos.IsNotEmpty ()
  || TessSurfaceNeg.IsNotEmpty () )

    SetIsoSurface ();                   // then do isosurface                                       (use ExtraContentType & IsoSurfaceCut)


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//if ( GeometryTransform == 0 )
//    SetGeometryTransform ();          // Setting Geometry transform                               (use file name & NiftiTransform)
}


//----------------------------------------------------------------------------
void    TVolumeDoc::ResetThenSetMri    (   MriInitFlags    flags,  double  backgroundvalue
                                        )
{
ResetMri ();

SetMri  ( flags, backgroundvalue );

SetDirty ( true );

NotifyDocViews ( vnViewUpdated );

CartoolMdiClient->RefreshWindows ();
}


//----------------------------------------------------------------------------
bool	TVolumeDoc::InitDoc ()
{
if ( ! IsOpen () ) {

    if ( ! Open ( ofRead, 0 ) )
        return  false;

                                        // We need to this only at opening time
    if ( GeometryTransform == 0 )
        SetGeometryTransform ();        // Setting Geometry transform                               (use file name & NiftiTransform)

                                        // some origins are just bullshit - bug is coming from Analyze, but it's safer to test for all MRIs, too...
                                        // do it only at opening time, as to not interfere with user setting any origing later on
    if ( ! Size.Contains ( Origin, 2 ) /*! IsValidOrigin ()*/ )
        Origin.Reset ();

                                        // Reading any labels from any buddy file
    SetLabels ();


    SetMri ();
    }


//Size.Show ( "MRI Size" );
//Bounding->Show ( "MRI Bounding" );
//DBGV3 ( RealSize[ 0 ], RealSize[ 1 ], RealSize[ 2 ], "Volume: Real Size" );
//DBGV3 ( VoxelSize[ 0 ], VoxelSize[ 1 ], VoxelSize[ 2 ], "Volume: Voxel Size" );
//DBGV3 ( RelativeDifference ( VoxelSize[ 0 ], VoxelSize[ 1 ] ), RelativeDifference ( VoxelSize[ 0 ], VoxelSize[ 2 ] ), RelativeDifference ( VoxelSize[ 1 ], VoxelSize[ 2 ] ), "Volume: Voxel Size RelDiff XY XZ YZ" );
//DBGV3 ( Origin.X, Origin.Y, Origin.Z, "Volume: Origin" );
//DBGV4 ( Bounding->GetXExtent (), Bounding->GetYExtent (), Bounding->GetZExtent (), Bounding->Step ( 128, false ) + 0.5, "Volume Boundary -> Downsampling factor for 128" );


//TArray1<double>     values;
//Data.GetGreyWhiteValues ( values, true, 0 );
//DBGV4 ( values[ BlackMin ], values[ GreyMin ], values[ GreyMax ], values[ WhiteMax ], "background - greymin - greymax - whitemax" );

                                        // could be retrieved from markers - current estimates for adult MNI:
//TPointFloat         nasion  (   0.0000000,   85.000000,  -48.0000000 );
//TPointFloat         inion   (   0.0000000, -116.500000,  -42.0000000 );
//TPointFloat         lpa     ( -81.0000000,  -21.500000,  -48.8000000 );
//TPointFloat         rpa     (  81.0000000,  -21.500000,  -48.8000000 );
//
//TPoints             tentenpositions;
//TStrings            tentennames;
//
//Estimate1010FromFiducials ( nasion, inion, lpa, rpa, tentenpositions, tentennames );
//
//TFileName           file1010    = GetDocPath ();
//RemoveExtension     ( file1010 );
//StringAppend        ( file1010, ".10-10" );
//AddExtension        ( file1010, FILEEXT_XYZ );
//tentenpositions.WriteFile ( file1010, &tentennames );


return  true;
}


//----------------------------------------------------------------------------
void    TVolumeDoc::InitLimits ( bool /*precise*/ )
{
ResetLimits ();


Data.GetMinMaxValues ( MinValue, MaxValue, MinPosition, MaxPosition );


AbsMaxValue     = max ( abs ( MinValue ), abs ( MaxValue ) );


AbsMaxTF        = 0;                    // useless here
MaxGfp          = 0;
MaxGfpTF        = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Merged InitAtomType here, as we have a useful stat done here...
//void    TVolumeDoc::InitAtomType ()

                                        // OriginalAtomType is intrinsic to the type of data and is set only once
                                        // Note that some document could have already set the OriginalAtomType by themselves
if ( IsUnknownType ( AtomTypeUseOriginal ) ) {

    if      ( MinValue >= 0                             // strict formula
//         || fabs ( MinValue ) < MaxValue / 1e2        // min value less than 1/100 of max value - but what if max is already small?
           || MinValue > -1e-6                          // absolute negative epsilon above which it is assumed positive data
                                                    )   SetAtomType ( AtomTypePositive );

    else                                                SetAtomType ( AtomTypeScalar );
    }

else {
                                        // Will NOT modify OriginalAtomType, but only CurrentAtomType
    if      ( MinValue >= 0 )                           SetAtomType ( AtomTypePositive );

    else if ( MinValue <  0 )                           SetAtomType ( AtomTypeScalar );
                                        // by safety
    else if ( IsUnknownType ( AtomTypeUseCurrent ) )    SetAtomType ( OriginalAtomType );
    } // ! IsUnknownType


//DBGM2 ( GetAtomTypeName ( AtomTypeUseOriginal ), GetAtomTypeName ( AtomTypeUseCurrent ), "OriginalAtomType / CurrentAtomType" );
}


//----------------------------------------------------------------------------
bool	TVolumeDoc::Close()
{
ResetMri ();

return  TFileDocument::Close ();
}


bool	TVolumeDoc::Revert ( bool clear )
{
if ( ! TFileDocument::Revert ( clear ) )
    return false;

if ( ! clear ) {
    Close ();

    Open ( 0, GetDocPath () );

    SetMri ();

    NotifyDocViews ( vnViewUpdated );
    }

return true;
}


//----------------------------------------------------------------------------
void    TVolumeDoc::SetBackgroundValue ()
{
                                        // In case of changes in the computation of BackgroundValue, consider checking:
                                        //   - SetContentType
                                        //   - FindOrientation
//BackgroundValue   = Clip ( Data.GetBackgroundValue (), MinValue, MaxValue );
                                        // currently using positive BackgroundValue for the negative part, converted later to -BackgroundValue - so we can't check range here
BackgroundValue   = Data.GetBackgroundValue ();

//DBGV ( BackgroundValue, "Background value" );
}


//----------------------------------------------------------------------------
void    TVolumeDoc::SetIsoSurfaceCut ()
{
if      ( IsBlob ()     )   IsoSurfaceCut   = BackgroundValue + ( MaxValue - BackgroundValue ) * 0.333; // not used since long
else if ( IsRis  ()     )   IsoSurfaceCut   = RISBackgroundValue;                                       // epsilon
else if ( IsMask ()     )   IsoSurfaceCut   = max ( MaskBackgroundValue, MinValue, -MaxValue );         // at least 1 - also accounting for negative data
else                        IsoSurfaceCut   = BackgroundValue;                                          // all other cases
}


//----------------------------------------------------------------------------
void    TVolumeDoc::SetBounding ( double backvalue )
{
Bounding->Set ( Data, false, backvalue );

//DisplaySpaces[ 0 ].Bounding = Bounding;
}


//----------------------------------------------------------------------------
void    TVolumeDoc::SetContentType ()
{
ExtraContentType    = UnknownMriContentType;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Guessing by file name
if      ( StringContains ( GetTitle (), "." FILEEXT_RIS "." ) )         ExtraContentType    = MriContentTypeSegmented | MriContentTypeRis;

                                        // Guessing from Nifti header
else if ( StringIsNot ( NiftiIntentName, NiftiIntentNameDefault ) ) {

    if      ( StringIs ( NiftiIntentName, NiftiIntentNameRis      ) )   ExtraContentType    = MriContentTypeSegmented | MriContentTypeRis;
    else if ( StringIs ( NiftiIntentName, NiftiIntentNameBrain    ) )   ExtraContentType    = MriContentTypeSegmented | MriContentTypeBrain;
    else if ( StringIs ( NiftiIntentName, NiftiIntentNameGreyMask ) )   ExtraContentType    = MriContentTypeSegmented | MriContentTypeGrey;
    else if ( StringIs ( NiftiIntentName, NiftiIntentNameLabels   ) )   ExtraContentType    = MriContentTypeSegmented | ( MaxValue == 1 ? MriContentTypeBinaryMask 
                                                                                                                                        : MriContentTypeRoiMask    );
    else if ( StringIs ( NiftiIntentName, NiftiIntentNameSkull    ) )   ExtraContentType    = MriContentTypeSegmented; // | MriContentTypeSkull;
    }

                                        // Guessing from Nifti header
else if ( NiftiIntentCode == NIFTI_INTENT_LABEL )                       ExtraContentType    = MriContentTypeSegmented | MriContentTypeRoiMask;

                                        // In last resort, guessing by content, which is a tiny bit more adventurous...
else                                                                    ExtraContentType    = Data.GuessTypeFromContent ( BackgroundValue, Bounding );
}


//----------------------------------------------------------------------------
void    TVolumeDoc::SetIsoSurface ()
{
                                        // reset default downsampling?
if ( IsoDownsampling <= 0 )
                                        // other possibilities: using GetBounding (); testing IsMask ()
    IsoDownsampling  = IsMask () ? 1 : AtLeast ( 1, Round ( (double) GetSize ()->MaxSize () / /*128*/ 256 ) );


TTriangleSurfaceIsoParam    isoparam;

isoparam.How                = UnknownIsosurface;
isoparam.SmoothData         = IsMask () || IsRis () ? FilterTypeNone : FilterTypeFastGaussian;  // Don't filter mask nor Ris - Otherwise, smoothing will dramatically reduce the number of triangles (factor 2)
isoparam.SmoothGradient     = false;                                // not needed if data has been smoothed out, and neither for mask
isoparam.Downsampling       = IsoDownsampling;
isoparam.UseOriginalVolume  = isoparam.SmoothData == FilterTypeNone;
isoparam.Rescale            = 1.0;                                  // no geometrical rescaling
isoparam.Origin             = Origin;                               // shifting OpenGL object


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // any positive data?
if ( GetMaxValue () > 0 ) {
                                        // 0 is OK
    isoparam.IsoValue       =   fabs ( IsoSurfaceCut );

    isoparam.How            = isoparam.IsoValue == 0    ?   IsosurfaceBox           // force big box
                            : IsMask ()                 ?   IsosurfaceMinecraft 
                            :                               IsosurfaceMarchingCube;

                                        // we can simplify a lot of parameters when using the big box
    if ( isoparam.How == IsosurfaceBox ) {

        isoparam.SmoothData         = FilterTypeNone;
        isoparam.SmoothGradient     = false;
        isoparam.Downsampling       = 1;
        isoparam.UseOriginalVolume  = isoparam.SmoothData == FilterTypeNone;
        }


    TessSurfacePos.IsosurfaceFromVolume ( Data, isoparam );
    }
else

    TessSurfacePos.Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // any negative data?
if ( GetMinValue () < 0 
  && isoparam.How != IsosurfaceBox ) {  // !do not compute twice the big box!
                                        // 0 is OK
    isoparam.IsoValue       = - fabs ( IsoSurfaceCut );

    isoparam.How            = isoparam.IsoValue == 0    ?   IsosurfaceBox           // force big box
                            : IsMask ()                 ?   IsosurfaceMinecraft 
                            :                               IsosurfaceMarchingCube;

                                        // we can simplify a lot of parameters when using the big box
    if ( isoparam.How == IsosurfaceBox ) {

        isoparam.SmoothData         = FilterTypeNone;
        isoparam.SmoothGradient     = false;
        isoparam.Downsampling       = 1;
        isoparam.UseOriginalVolume  = isoparam.SmoothData == FilterTypeNone;
        }


    TessSurfaceNeg.IsosurfaceFromVolume ( Data, isoparam );
    }
else
                                        // no negative data, or isosurface is 0, but positive will handle that for both positive and negative data
    TessSurfaceNeg.Reset ();

}


//----------------------------------------------------------------------------
                                        // Setting Geometry transform
void    TVolumeDoc::SetGeometryTransform ()
{
if ( GeometryTransform ) {
    delete GeometryTransform;
    GeometryTransform = 0;
    }


TFileName           filename;
StringCopy      ( filename,  GetTitle () );
GetFilename     ( filename );

TFileName           extension;
GetExtension    ( extension, GetTitle () );


static TStringGrep      grepmniavs      ( "^(?i)avg152T1(v2.gray|)$",               GrepOptionDefault );
static TStringGrep      grepmniavwsmac  ( "^(?i)avg152T1_smac_(pro|brain|grey)$",   GrepOptionDefault );
static TStringGrep      grepmniavwsym   ( "^(?i)ave152_sym_(pro|brain|grey)$",      GrepOptionDefault );
static TStringGrep      greptalvmr      ( "(?i)(_TAL|_TAL_MASKED)(\\..+|)$",        GrepOptionDefault );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Nifti, the new guy in town
if      ( StringIs ( extension, FILEEXT_MRINII ) ) {

                                        // Hopefully, this field has been set - saves some ambiguity
    if ( NiftiTransform == NIFTI_XFORM_MNI_152 )

        GeometryTransform = new TGeometryTransform ( TalairachMNI152_2009 );
                                        // Otherwise, do it the old way by file name guessing
    else if ( StringStartsWith ( (const char*) filename, "MNI152.Nlin" )                        // Cartool MNI
           || StringStartsWith ( (const char*) filename, "mni_icbm152_" )                       // official MNI
           || StringStartsWith ( (const char*) filename, "MNI152_" )                            // official MNI
           || StringContains   ( (const char*) filename, InfixToCoregistered "MNI152.Nlin" )    // any coregistration
           || StringContains   ( (const char*) filename, InfixToCoregistered "mni_icbm152_" ) 
           || StringContains   ( (const char*) filename, InfixToCoregistered "MNI152_" ) 
            )

        GeometryTransform = new TGeometryTransform ( TalairachMNI152_2009 );

//    if ( GeometryTransform )    DBGM ( GeometryTransform->Name, "Nifti GeometryTransform" );
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Analyze is a mess, many cases here are for backward compatibility
else if ( StringIs ( extension, FILEEXT_MRIAVW_HDR ) ) {

    if      ( grepmniavwsmac.Matched ( (const char*) filename ) )

        GeometryTransform = new TGeometryTransform ( TalairachAvg152T1_smac );

    else if ( grepmniavwsym .Matched ( (const char*) filename ) )

        GeometryTransform = new TGeometryTransform ( TalairachAve152_sym );

    else if ( StringStartsWith ( (const char*) filename, "MNI152.Nlin" )
           || StringStartsWith ( (const char*) filename, "NIHPD324.NlinAsym" )
           || StringContains   ( (const char*) filename, InfixToCoregistered "MNI152.Nlin" ) )

        GeometryTransform = new TGeometryTransform ( TalairachMNI152_2009 );

    else if ( StringStartsWith ( (const char*) filename, "MNI152T1" )
           || StringContains   ( (const char*) filename, InfixToCoregistered "MNI152T1" ) )

        GeometryTransform = new TGeometryTransform ( TalairachMNI152_2001 );

    else if ( StringStartsWith ( (const char*) filename, "NIHPD.Asym.Infant" )
           || StringContains   ( (const char*) filename, InfixToCoregistered "NIHPD.Asym.Infant" ) )

        GeometryTransform = new TGeometryTransform ( TalairachNIHPD_Infant );

    } // Analyze

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Super old, kept for backward compatibiliy
else if ( StringIs ( extension, FILEEXT_MRIAVS ) ) {

    if      ( grepmniavs    .Matched ( (const char*) filename ) )

        GeometryTransform = new TGeometryTransform ( TalairachAvg152T1Fld );

    } // AVS

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Used for BESA compatibility only
else if ( StringIs ( extension, FILEEXT_MRIVMR ) ) {

    if ( greptalvmr.Matched ( (const char*) filename ) )

        GeometryTransform = new TGeometryTransform ( Talairach );

    } // VMR

}


//----------------------------------------------------------------------------
                                        // Reading labels from external file
void    TVolumeDoc::SetLabels ()
{
TFileName           filelabels ( GetDocPath () );

                                        // There could be 2 cases(!):
                                        // file.ext.txt
AddExtension    ( filelabels, FILEEXT_TXT );


if ( ! filelabels.CanOpenFile () ) {
                                        // file.txt
    RemoveExtension ( filelabels, 2 );
    AddExtension    ( filelabels, FILEEXT_TXT );
    }

                                        // neither case
if ( ! filelabels.CanOpenFile () )
    return;

//filelabels.Show ( "labels" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TStringsMap         roimap;

roimap.Add ( filelabels );

if ( roimap.IsEmpty () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // now, we can overload any existing Labels (like coming from header)
Labels.Reset ();

int                 maxindex    = roimap.GetMaxKey ();

for ( int i = 0; i <= maxindex; i++ )
                                                                                            // be nice and add a background string            
    Labels.Add ( StringIsNotEmpty ( roimap.GetValue ( i ) ) ? roimap.GetValue ( i ) : i == 0 ? "Background" : "" );
}


//----------------------------------------------------------------------------
                                        // function could update the file extension
bool	TVolumeDoc::ExportDownsampled ( char* path, int downsampling )   const
{
if ( StringIsEmpty ( path ) || downsampling < 1 )
    return false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // downsampling precision set to max
                                        // use a median in case of mask / ROI
Volume              downvol ( Data, downsampling, downsampling, IsMask () ? FilterTypeMedian : FilterTypeMean );

                                        // updating with downsampling
TPointDouble        origin          = Origin    / downsampling;
TVector3Double      voxelsize       = VoxelSize * downsampling;
TVector3Double      realsize        = RealSize; // !real size doesn't change!
char                orientation[ 4 ];

//if ( KnownOrientation )               // always saving orientation?
    OrientationToString ( orientation );


TFileName           filedown ( path );

CheckMriExtension   ( filedown );

StringCopy          ( path, filedown );

                                        // call TVolume, that will use a TExportVolume
downvol.WriteFile   ( filedown, &origin, &voxelsize, &realsize, orientation,
                      AtLeast ( NiftiTransformDefault, NiftiTransform ), NiftiIntentCode, NiftiIntentName
                    );

return  true;
}


//----------------------------------------------------------------------------
                                        // Reslicing data with this transform
bool    TVolumeDoc::ResliceData    (   TMatrix44&          transform,  // can be updated for optimization
                                        TPointDouble&       origin,     // same
                                        const TPointDouble& voxelsize,
                                        int                 niftitransform,     int                 niftiintentcode,    const char* niftiintentname,
                                        const char*         title
                                    )
{
                                        // Analyze the transform to apply the optimal processing
MatrixAnalysis      ma              = transform.Analyze ( SingleFloatEpsilon );


//DBGM                ( NiftiTransformToString ( niftitransform ), title );
//transform.Show      ( "Reslicing: Transform Matrix" );
//ShowMatrixAnalysis  ( ma, "Reslicing: Transform Matrix" );

                                        // "real" reslicing means more than a simple translation / origin shift
bool                doesreslice     = false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Detach the translation from the global transform
                                        // !does not seem to work for all cases, especially with upsampling!
if ( ( ma & HasTranslation )
     && transform.GetTranslationX () < 0    // restrict to these cases
     && transform.GetTranslationY () < 0
     && transform.GetTranslationZ () < 0 ) {

                                        // convert the (post) translation into the origin
    origin.Set ( -transform.GetTranslationX (), -transform.GetTranslationY (), -transform.GetTranslationZ () );

                                        // trial - does not work for all cases
//  origin.Set ( fabs ( transform.GetTranslationX () ) + ( transform.GetTranslationX () > 0 ),
//               fabs ( transform.GetTranslationY () ) + ( transform.GetTranslationY () > 0 ), 
//               fabs ( transform.GetTranslationZ () ) + ( transform.GetTranslationZ () > 0 ) );

                                        // then reset it from the transform itself
    transform.ResetTranslation ();

                                        // also resetting all translation flags
    ResetFlags  ( ma, HasTranslationMask );

                                        // !don't forget to remove &origin from the call to TransformAndSave!
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Now do the remaining transform, if any
if      ( IsMatrixIdentity ( ma ) )

    ;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // Boosting special case of flipping a single axis?
else if (      ( ma & HasSymmetry ) &&   ( ma & HasOrtho    ) 
          && ! ( ma & HasScaling  ) && ! ( ma & HasShearing ) ) {

                                    // look more precisely at single axis flipping - symmetry flag could actually involve 1 rotation + 1 flipping
    int     numflip     =   ( transform ( 0, 0 ) <= -1 + SingleFloatEpsilon )
                          + ( transform ( 1, 1 ) <= -1 + SingleFloatEpsilon )
                          + ( transform ( 2, 2 ) <= -1 + SingleFloatEpsilon );

    DBGV ( numflip, "Flipping axis" );

    Flip ( axis );
    }
*/
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // All the other cases: rescaling (isotropic or to 1[mm]), rotations, complex symmetry
else {
    MriContentType      contenttype     = Data.GuessTypeFromContent (); // !currently, it will be called again in Set / SetContentType!

    bool                ismask          = crtl::IsMask ( contenttype );
                                        // is orthogonal in the general case?
    bool                isortho         = IsOrthogonal ( ma );

//    ShowMriContentType  ( contenttype, "Reslicing: Mri Content Type" );


    FilterTypes         filtertype;
    InterpolationType   interpolate;
    int                 numsubsampling;
    BoundingSizeType    targetsize;
    Volume              ReslicedData;
    TPointDouble        TASshift;


    filtertype      = ismask    ? FilterTypeMedian           
                                : FilterTypeNone;

    interpolate     = ismask    ?                         InterpolateNearestNeighbor    // interpolation is totally forbidden
                    : isortho   ? ( ( ma & HasScaling ) ? InterpolateLinear             // fast and good enough(?)
                                                        : InterpolateNearestNeighbor )  // no scaling, no need to interpolate
                    :                                     InterpolateLanczos3;          // any other case that requires good results

    numsubsampling  = 1;


//  targetsize      = isortho   ? BoundingSizeBiggest   // preserving the original data size
//                              : BoundingSizeOptimal;  // any complex transform will end up with a tilted box, so cutting around the actual data is more efficient
    targetsize      = BoundingSizeBiggest;              // using the biggest ensures that Head and Brain MRIs will still have the same relative storage size and origin - drawback is that it takes more space

                                        // apply transform and save to file
    Data.TransformAndSave   (   transform,       
                                TransformAndSaveFlags (  TransformToTarget 
                                                       | TransformSourceRelative 
                                                       | TransformTargetRelative ),
                                filtertype,         interpolate,    numsubsampling,
                                0,                  0,
                                voxelsize,          0,
                                targetsize,         0,
                                0,
                                AtLeast ( NiftiTransformDefault, niftitransform ),  niftiintentcode,        niftiintentname,
                                0,                  &ReslicedData,  &TASshift,
                                title
                            );

                                        // Save might have added some shift due to to optimal cropping / rotation
    origin += TASshift;

                                        // this is our new data
    Data        = ReslicedData;

                                        // here data has been actually resliced
    doesreslice = true;
    } // reslicing NiftiMethod3


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  doesreslice;
}


//----------------------------------------------------------------------------
                                        // Another clipping value, the "black" matter, which is above the background
double  TVolumeDoc::GetCsfCut ()   const
{
if ( IsMask () )
    return  0;


TArray1<double>     values;

Data.GetGreyWhiteValues ( values, false );

return  max ( BackgroundValue, values[ BlackMax ] );
}


//----------------------------------------------------------------------------
                                        // Saving sagittal central slice
bool	TVolumeDoc::ExportSlice ( const char* path, int thickness )   const
{
if ( StringIsEmpty ( path ) || thickness < 1 )
    return false;


thickness   = RoundToOdd ( thickness );

int         midthickness    = thickness / 2;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // copy central slice
Volume              slice ( thickness, Size.YMax (), Size.ZMax () );

int                 ox              = Truncate ( Origin.X );

for ( int x0 = 0; x0 < thickness; x0++ )
for ( int z = 0; z < Size.ZMax (); z++ )
for ( int y = 0; y < Size.YMax (); y++ )

    slice ( x0, y, z )  = Data ( x0 + ( ox - midthickness ), y, z );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // save to file
TExportVolume       expvol;


StringCopy          ( expvol.Filename, path );

CheckMriExtension   ( expvol.Filename );


expvol.VolumeFormat     = GetVolumeAtomType ( &Data, FilterTypeNone, InterpolateUnknown, ToExtension ( expvol.Filename ) );

expvol.MaxValue         = Data.GetAbsMaxValue ();

expvol.MaxValue         = slice.GetMaxValue ();     // we know that

expvol.VoxelSize        = VoxelSize;

expvol.Origin           = Origin;
expvol.Origin.X         = midthickness;

expvol.NiftiTransform   = AtLeast ( NiftiTransformDefault, NiftiTransform );

expvol.NiftiIntentCode  = NiftiIntentCode;

StringCopy  ( expvol.NiftiIntentName, NiftiIntentName, NiftiIntentNameSize - 1 );

//if ( KnownOrientation )               // always saving orientation?
    OrientationToString ( expvol.Orientation );


expvol.Write ( slice, ExportArrayOrderZYX );


return  true;
}


//----------------------------------------------------------------------------
                                        // Saving a spherical histogram of inner and outer layers radii + corresponding thickness
                                        // Note that the radi are not computed orthogonally to the surface encountered
                                        // but just in the straight direction from the center.
                                        // Geometrical center used here corresponds to the Best Fitting Sphere used in the inverse model, for the MNI template.
bool	TVolumeDoc::ExportSphericalHistogram ( const char* path )   const
{
if ( StringIsEmpty ( path ) )
    return false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 thetasize       = 360 + 1;  // Theta is aroud the equator, range in [0..360]
int                 phisize         = 180 + 1;  // Phi is from pole to pole,   range in [0..180]
TVolume<float>      histosph        ( thetasize, phisize, 3 );
TArray2<float>      histosphin      ( thetasize, phisize );
TArray2<float>      histosphout     ( thetasize, phisize );
TArray2<float>      histosphthck    ( thetasize, phisize );
double              phi;
double              theta;

TPointDouble        BFSoffset       ( 0.0, 20.62, 0.66 ); // MNI to BFS - taken from one inverse processing (2021-04)
TPointDouble        center          = Origin - BFSoffset;
double              radiusmax       = Bounding->Radius () * 1.80;
double              backgroundvalue = max ( 0.25, BackgroundValue );

double              subvoxel        = 20;
TPointDouble        p;
double              rin;
double              rout;

                                        // scan uniformly the solid sphere
for ( int phii = 0; phii < phisize; phii++ ) {

    phi     = phii / (double) ( phisize - 1 ) * Pi;

    for ( int thetai = 0; thetai < thetasize; thetai++ ) {

        theta   = thetai / (double) ( thetasize - 1 ) * TwoPi;

                                        // center toward surface
        for ( rin = 1; rin < radiusmax; rin++ ) {
            
            p.ToCartesian ( theta, phi, rin );

            p   += center;
                                        // reached an edge?
            if ( Data.GetValueChecked(p.X, p.Y, p.Z, InterpolateLinear) >= backgroundvalue ) {
                                        // backtracking for sub-voxel accuracy
                do {
                    rin -= 1 / subvoxel;

                    p.ToCartesian ( theta, phi, rin );

                    p   += center;
                    } while ( Data.GetValueChecked ( p.X, p.Y, p.Z, InterpolateLanczos3 ) >= backgroundvalue );
                                        // interface is in-between
                rin += 0.5 / subvoxel;

                break;
                }
            }

                                        // nothing? skip remaining, histogram will remain null
        if ( rin >= radiusmax )
            continue;

                                        // surface toward center
        for ( rout = radiusmax; rout > 1; rout-- ) {
            
            p.ToCartesian ( theta, phi, rout);

            p   += center;
                                        // reached an edge?
            if ( Data.GetValueChecked ( p.X, p.Y, p.Z, InterpolateLinear ) >= backgroundvalue ) {
                                        // backtracking for sub-voxel accuracy
                do {
                    rout += 1 / subvoxel;

                    p.ToCartesian ( theta, phi, rout);

                    p   += center;
                    } while ( Data.GetValueChecked ( p.X, p.Y, p.Z, InterpolateLanczos3 ) >= backgroundvalue );
                                        // interface is in-between
                rout -= 0.5 / subvoxel;

                break;
                }
            }

                                        // nothing? skip remaining, histogram will remain null
        if ( rout <= 1 )
            continue;


        histosph        ( thetai, phii, 0 ) = rin;
        histosph        ( thetai, phii, 1 ) = rout;
        histosph        ( thetai, phii, 2 ) = AtLeast ( 0.0, rout - rin );
        histosphin      ( thetai, phii )    = rin;
        histosphout     ( thetai, phii )    = rout;
        histosphthck    ( thetai, phii )    = AtLeast ( 0.0, rout - rin );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Post-processing - Off due to borders effects
/*
FctParams           params;

param ( FilterParamDiameter ) = 5;

//histosphin  .Filter ( FilterTypeMedian, param, true );
//histosphout .Filter ( FilterTypeMedian, param, true );
histosphin  .Filter ( FilterTypeGaussian, param, true );
histosphout .Filter ( FilterTypeGaussian, param, true );

//histosphthck.Filter ( FilterTypeMedian, param, true );

histosphthck    = histosphout - histosphin;
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Saving as a volume file
TFileName           file ( path );

CheckNoOverwrite    ( file );

histosph.WriteFile  ( file );


TExportTracks       exptracks;

                                        // Saving as a freq file
//ReplaceExtension    ( file, FILEEXT_FREQ );
//exptracks.NumTime       = histosph.GetDim2 ();
//exptracks.NumTracks     = histosph.GetDim1 ();
//exptracks.NumFrequencies= histosph.GetDim3 ();
//exptracks.Write ( histosph );

                                        // Saving as 3 separate sef files
StringCopy          ( exptracks.Filename, path );
RemoveExtension     ( exptracks.Filename );
StringAppend        ( exptracks.Filename, ".In" );
AddExtension        ( exptracks.Filename, FILEEXT_EEGSEF );
CheckNoOverwrite    ( exptracks.Filename );

exptracks.NumTracks     = histosphin.GetDim1 ();
exptracks.NumTime       = histosphin.GetDim2 ();

exptracks.Write ( histosphin, Transposed );


StringCopy          ( exptracks.Filename, path );
RemoveExtension     ( exptracks.Filename );
StringAppend        ( exptracks.Filename, ".Out" );
AddExtension        ( exptracks.Filename, FILEEXT_EEGSEF );
CheckNoOverwrite    ( exptracks.Filename );

exptracks.NumTracks     = histosphout.GetDim1 ();
exptracks.NumTime       = histosphout.GetDim2 ();

exptracks.Write ( histosphout, Transposed );


StringCopy          ( exptracks.Filename, path );
RemoveExtension     ( exptracks.Filename );
StringAppend        ( exptracks.Filename, ".Thick" );
AddExtension        ( exptracks.Filename, FILEEXT_EEGSEF );
CheckNoOverwrite    ( exptracks.Filename );

exptracks.NumTracks     = histosphthck.GetDim1 ();
exptracks.NumTime       = histosphthck.GetDim2 ();

exptracks.Write ( histosphthck, Transposed );


return  true;
}


//----------------------------------------------------------------------------
void    TVolumeDoc::NewIsoSurfaceCut ( bool automatic, double bt )
{
double              isosurfacecut;


if ( automatic ) {

    char                title [ 256 ];
    char                answer[ 256 ];

    StringCopy ( title, "Current Isosurface thresholding value:  ", FloatToString ( IsoSurfaceCut ) );

    if ( ! GetOptionsFromUser ( "Give the new Isosurface thresholding value, or (A)uto:", 
                                title, "A+-.0123456789", "A", answer ) )
        return;

    if      ( *answer == 'A' )  isosurfacecut   = 0;                      // !keep it automatic here!
    else                      { isosurfacecut   = StringToDouble ( answer ); automatic = false; }
    }
else
    isosurfacecut   = bt;


//Clipped ( isosurfacecut, MinValue, MaxValue );


//if ( isosurfacecut == IsoSurfaceCut )
//    return;


ResetMri ();                            // doesn't actually release the array


if ( automatic )    SetIsoSurfaceCut    ();
else                IsoSurfaceCut   = isosurfacecut;


SetBounding         ( IsoSurfaceCut );


SetIsoSurface ();


CartoolMdiClient->RefreshWindows ();
}


//----------------------------------------------------------------------------
void    TVolumeDoc::NewIsoDownsampling ( int ds )
{
int                 down;


if ( ds <= 0 ) {

    char                title [ 256 ];

    StringCopy ( title, "Current downsampling = ", IntegerToString ( IsoDownsampling ) );

    if ( ! GetValueFromUser ( "Give a new isosurface downsampling (0 for default):", title, down, "0", CartoolMainWindow ) )
        return;
    }
else
    down    = ds;


if ( IsoDownsampling == down && ds <= 0 )
    return;


ResetMri ();                            // don't remove the array


if ( down <= 0 || down > Size.MaxSize() )
    IsoDownsampling = 0;                // reset
else
    IsoDownsampling = down;             // new value


SetIsoSurface ();


CartoolMdiClient->RefreshWindows();
}


//----------------------------------------------------------------------------
void    TVolumeDoc::Flip ( owlwparam wparam )
{
                                        // not allowed to permute if it is already in use
//if ( (bool) UsedBy )
//    return;

int                 width;
int                 height;
int                 depth;
TArray1<MriType>    d ( Size.GetMaxExtent () );
Volume              newdata;


switch ( wparam ) {
    case IDB_FLIPX:

        for ( int z=0; z < Size.GetZExtent(); z++ )
        for ( int y=0; y < Size.GetYExtent(); y++ ) {

            for ( int x=0; x < Size.GetXExtent(); x++ )     d[x] = Data ( x, y, z );
            for ( int x=0; x < Size.GetXExtent(); x++ )     Data ( x, y, z ) = d[Size.GetXExtent() - 1 - x];
            }
        break;

    case IDB_FLIPY:

        for ( int z=0; z < Size.GetZExtent(); z++ )
        for ( int x=0; x < Size.GetXExtent(); x++ ) {

            for ( int y=0; y < Size.GetYExtent(); y++ )     d[y] = Data ( x, y, z );
            for ( int y=0; y < Size.GetYExtent(); y++ )     Data ( x, y, z ) = d[Size.GetYExtent() - 1 - y];
                }
        break;

    case IDB_FLIPZ:

        for ( int y=0; y < Size.GetYExtent(); y++ )
        for ( int x=0; x < Size.GetXExtent(); x++ ) {

            for ( int z=0; z < Size.GetZExtent(); z++ )     d[z] = Data ( x, y, z );
            for ( int z=0; z < Size.GetZExtent(); z++ )     Data ( x, y, z ) = d[Size.GetZExtent() - 1 - z];
            }
        break;

    case IDB_FLIPXY:

        width   = Size.GetXExtent();
        height  = Size.GetYExtent();
        depth   = Size.GetZExtent();

        newdata.Resize ( height, width, depth );

        for ( int x=0; x < Size.GetXExtent(); x++ )
        for ( int y=0; y < Size.GetYExtent(); y++ )
        for ( int z=0; z < Size.GetZExtent(); z++ )

            newdata ( y, x, z ) = Data ( x, y, z );

        Size.Set ( 0, height - 1, 0, width - 1, 0, depth - 1 );

        Data    = newdata;

        break;

    case IDB_FLIPYZ:

        width   = Size.GetXExtent();
        height  = Size.GetYExtent();
        depth   = Size.GetZExtent();

        newdata.Resize ( width, depth, height );

        for ( int x=0; x < Size.GetXExtent(); x++ )
        for ( int y=0; y < Size.GetYExtent(); y++ )
        for ( int z=0; z < Size.GetZExtent(); z++ )

            newdata ( x, z, y ) = Data ( x, y, z );

        Size.Set ( 0, width - 1, 0, depth - 1, 0, height - 1 );

        Data    = newdata;

        break;

    case IDB_FLIPXZ:

        width   = Size.GetXExtent();
        height  = Size.GetYExtent();
        depth   = Size.GetZExtent();

        newdata.Resize ( depth, height, width );

        for ( int x=0; x < Size.GetXExtent(); x++ )
        for ( int y=0; y < Size.GetYExtent(); y++ )
        for ( int z=0; z < Size.GetZExtent(); z++ )

            newdata ( z, y, x ) = Data ( x, y, z );

        Size.Set ( 0, depth - 1, 0, height - 1, 0, width - 1 );

        Data    = newdata;

        break;

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ResetMri ();

                                        // only case where we should destroy this
if ( GeometryTransform ) {
    delete GeometryTransform;
    GeometryTransform = 0;
    }

                                        // this is new orientation!
KnownOrientation    = false;

                                        // recreate content & orientation
//SetMri  ( (MriInitFlags) ( MriInitBackgroundKeep | MriInitIsoSurfaceCutKeep | MriInitTypeKeep | MriInitReorientDetect ), BackgroundValue );
                                        // MriInitIsoSurfaceCutKeep is buggy, we have use MriInitIsoSurfaceCutDetect for the moment
SetMri  ( (MriInitFlags) ( MriInitBackgroundKeep | MriInitIsoSurfaceCutDetect | MriInitTypeKeep | MriInitReorientDetect ), BackgroundValue );

/*
TMriWinView  *view;
                                        // remember the first window's position
TRect   wr = GetViewList()->GetParentO()->GetWindowRect();
CartoolMdiClient->ScreenToClient ( wr );

                                        // kill all old views
CartoolDocManager->Close Views ( this );// use dnClose
                                        // and just before it closes the doc
                                        // create a new one
view    = new TMriWinView ( *this, 0 );
CartoolDocManager->PostEvent ( dnCreate, *view );

                                        // restore the window position
view->WindowSetPosition ( wr.left, wr.top, wr.Width(), wr.Height() );
*/

SetDirty ( true );

CartoolMdiClient->RefreshWindows();
}


//----------------------------------------------------------------------------
void    TVolumeDoc::SetNewOrigin ()
{
char                answer[ 256 ];
TPointDouble        origin;


if ( ! GetInputFromUser ( "Provide the new X, Y, Z coordinates of the voxel origin:", "New Origin", answer, "", CartoolMainWindow ) )
    return;


ReplaceChars ( answer, ",;:/[](){}", "          " );

int                 numanswer       = sscanf ( answer, "%lf %lf %lf", &origin.X, &origin.Y, &origin.Z );

if ( ! ( numanswer == 1 || numanswer == 3 ) )
    return;

if ( numanswer == 1 )
    origin.Y    = origin.Z  = origin.X;
    

SetNewOrigin ( origin );

SetDirty ( true );
}


void    TVolumeDoc::SetNewOrigin ( const TPointDouble& origin )
{
if ( Origin == origin )
    return;

Origin      = origin;

                                        // recreate content - !MriInitIsoSurfaceCutKeep is buggy!
//ResetThenSetMri ( MriInitKeep, BackgroundValue );


//double      oldisosurfacecut = IsoSurfaceCut;
                                        // MriInitIsoSurfaceCutKeep is buggy, we have to recreate the isosurface automatically...
ResetThenSetMri ( (MriInitFlags) ( MriInitBackgroundKeep | MriInitIsoSurfaceCutDetect | MriInitTypeKeep | MriInitReorientKeep ), BackgroundValue );
                                        // !dirty fix, we call again with the original isosurface cutting value!
//IsoSurfaceCut = oldisosurfacecut;
//
//ResetThenSetMri ( MriInitKeep, BackgroundValue );
}


//----------------------------------------------------------------------------
bool    TVolumeDoc::IsValidOrigin ()   const
{
return  IsValidOrigin ( Origin );
}


bool    TVolumeDoc::IsValidOrigin ( const TPointDouble &origin )   const
{
                                        // Likelihood decreases for each next formula

                                        // non-initialized point
if ( origin.IsNull () )
    return  false;

                                        // being on any vertex of the cube is like an uninitialized point that has been rotated at some point
//if ( ( origin.X == Size.XMin () || origin.X == Size.XMax () )
//  && ( origin.Y == Size.YMin () || origin.Y == Size.YMax () )
//  && ( origin.Z == Size.ZMin () || origin.Z == Size.ZMax () ) )
//    return  false;

                                        // too far outside the size box is highly unlikely a good origin
//if ( ! Size.Contains ( origin, 2 ) )
//    return  false;

                                        // any dimension outside size is quite suspicious - this also includes the vertices test above
if ( ( origin.X <= Size.XMin () || origin.X >= Size.XMax () )
  || ( origin.Y <= Size.YMin () || origin.Y >= Size.YMax () )
  || ( origin.Z <= Size.ZMin () || origin.Z >= Size.ZMax () ) )
    return  false;

                                        // Heuristic test on origin being too off-center
                                        // Coming from the old Analyze inconsistent format - we might phase out this part now?
constexpr double    MinOriginExcentricity   = 0.90; // was 0.50, but we can push the limit now?

                                        // excentricity on each axis
//double              excx            = fabs ( 2 * origin.X - Size.XMax () ) / Size.XMax ();
//double              excy            = fabs ( 2 * origin.Y - Size.YMax () ) / Size.YMax ();
//double              excz            = fabs ( 2 * origin.Z - Size.ZMax () ) / Size.ZMax ();
//
//                                        // any axis not centered enough
//if ( excx > MinOriginExcentricity 
  //|| excy > MinOriginExcentricity 
//  || excz > MinOriginExcentricity )
//    return  false;

                                        // radial excentricity - anything close to corners will be more penalized
double              exc             = sqrt (   Square ( ( 2 * origin.X - Size.XMax () ) / Size.XMax () )
                                             + Square ( ( 2 * origin.Y - Size.YMax () ) / Size.YMax () )
                                             + Square ( ( 2 * origin.Z - Size.ZMax () ) / Size.ZMax () ) );

                                        // not inside half size centered sphere
if ( exc > MinOriginExcentricity )
    return  false;


/*                                        // testing with data content
if ( ! Bounding->IsEmpty () ) {
                                        // data at origin is empty is suspicious, but that doesn't make it a certainty, though...
//  if ( Data.GetValueChecked ( origin ) == 0 )
//      return  false;

                                        // not inside data box is more indicative of something fishy, although still not certain, like cutting through MRI
//  if ( ! Bounding->Contains ( origin, 0 ) )
//      return  false;
    }
*/


return  true;
}


//----------------------------------------------------------------------------
                                        // input points are in absolute coordinates
void    TVolumeDoc::Estimate1010FromFiducials  (   const TPointFloat&      nasion, 
                                                    const TPointFloat&      inion, 
                                                    const TPointFloat&      lpa, 
                                                    const TPointFloat&      rpa,
                                                    TPoints&                tentenpositions,
                                                    TStrings&               tentennames
                                                )  const
{
tentenpositions.Reset ();
tentennames    .Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double              threshold       = BackgroundValue;

TPointFloat         front           = nasion;
TPointFloat         back            = inion;
                                        // new center is middle point between nasion in inion - in relative coordinates
TPointFloat         centerfrontback = ( front + back ) / 2 + Origin;
                                        // so we need to shift the original points
TPointFloat         centertoorigin1 = centerfrontback - Origin;
TPointFloat         origintocenter1 = Origin          - centerfrontback;

                    front          += origintocenter1;
                    back           += origintocenter1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // angular scan from nasion to inion
double              fromalpha       = ArcTangent ( front.Z, front.Y );    // center is in between nasion and inion, starting angle is therefor
int                 numstepsalpha   = 360;                          // 0.5 degree step corresponds approximately to 1 voxel
double              radius          = Bounding->Radius () * 1.10;   // extra kick for safety

TPoints             frontbackline ( numstepsalpha );

OmpParallelFor

for ( int alphai = 0; alphai < numstepsalpha; alphai++ ) {
                                        // we deal exactly with 180 degrees range by geometrical construct
    double          alpha       = fromalpha + alphai / (double) ( numstepsalpha - 1 ) * Pi;

    TPointFloat     p;
                                        // in absolute, center space
    p.Y     = cos ( alpha ) * radius;
    p.Z     = sin ( alpha ) * radius;

    p.ResurfacePoint ( Data, centerfrontback, threshold );

    frontbackline[ alphai ] = p;
    }


frontbackline   += centertoorigin1;

//frontbackline.WriteFile ( "E:\\Data\\NasionInionLine.spi" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // cumulating curvilinear length
TVector<double>     frontbackdist ( numstepsalpha );

frontbackdist[ 0 ]   = 0;

for ( int alphai = 1; alphai < numstepsalpha; alphai++ )

    frontbackdist[ alphai ]  = frontbackdist[ alphai - 1 ] + ( frontbackline[ alphai ] - frontbackline[ alphai - 1 ] ).Norm ();


frontbackdist.NormalizeMax ();

//frontbackdist.WriteFile ( "E:\\Data\\NasionInionLine.Dist1.sef", "Dist1" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              czcl            = 0.50;     // curvilinear distance from one side
double              fpzcl           = 0.10;
double              ozcl            = 0.90;
double              t8cl            = 0.10;
double              t7cl            = 0.90;
TPointFloat         cz;                         // 10-10 coordinates
TPointFloat         fpz;
TPointFloat         oz;
TPointFloat         t8;
TPointFloat         t7;


for ( int alphai = 0; alphai < numstepsalpha - 1; alphai++ ) {

    if ( frontbackdist[ alphai ] <= czcl  && frontbackdist[ alphai + 1 ] > czcl  )      cz  = frontbackline[ alphai ];
    if ( frontbackdist[ alphai ] <= fpzcl && frontbackdist[ alphai + 1 ] > fpzcl )      fpz = frontbackline[ alphai ];
    if ( frontbackdist[ alphai ] <= ozcl  && frontbackdist[ alphai + 1 ] > ozcl  )      oz  = frontbackline[ alphai ];
    }

                                        // results in absolute original coordinates
tentenpositions.Add ( cz  );    tentennames.Add ( "Cz"  );
tentenpositions.Add ( fpz );    tentennames.Add ( "Fpz" );
tentenpositions.Add ( oz  );    tentennames.Add ( "Oz"  );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TPointFloat         left            = lpa;
TPointFloat         right           = rpa;
                                        // new center is middle point between nasion in inion - in relative coordinates
TPointFloat         centerleftright = ( left + right ) / 2 + Origin;
                                        // so we need to shift the original points
TPointFloat         centertoorigin2 = centerleftright - Origin;
TPointFloat         origintocenter2 = Origin  - centerleftright;

                    left           += origintocenter2;
                    right          += origintocenter2;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // angular scan from right to left
TPoints             rightleftline ( numstepsalpha );
                                        // tilt from Cz
double              alphacz     = ArcTangent ( cz.Y + origintocenter2.Y, cz.Z + origintocenter2.Z );

TMatrix44           mat;

mat.RotateX ( RadiansToDegrees ( -alphacz ), MultiplyLeft );


OmpParallelFor

for ( int alphai = 0; alphai < numstepsalpha; alphai++ ) {
                                        // we deal exactly with 180 degrees range by geometrical construct
    double          alpha       = alphai / (double) ( numstepsalpha - 1 ) * Pi;

    TPointFloat     p;
                                        // in absolute, center space
    p.X     = cos ( alpha ) * radius;
    p.Z     = sin ( alpha ) * radius;
    p.Y     = 0;

    mat.Apply ( p );

    p.ResurfacePoint ( Data, centerleftright, threshold );

    rightleftline[ alphai ] = p;
    }

rightleftline     += centertoorigin2;

//rightleftline.WriteFile ( "E:\\Data\\RightLeftLine.spi" );

//TPoints             crosslines      = frontbackline + rightleftline;
//crosslines.WriteFile ( "E:\\Data\\CrossLines.spi" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // cumulating curvilinear length
TVector<double>     rightleftdist ( numstepsalpha );

rightleftdist[ 0 ]     = 0;

for ( int alphai = 1; alphai < numstepsalpha; alphai++ )

    rightleftdist[ alphai ]    = rightleftdist[ alphai - 1 ] + ( rightleftline[ alphai ] - rightleftline[ alphai - 1 ] ).Norm ();


rightleftdist.NormalizeMax ();

//rightleftdist.WriteFile ( "E:\\Data\\RightLeftLine.Dist1.sef", "Dist1" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int alphai = 0; alphai < numstepsalpha - 1; alphai++ ) {

    if ( rightleftdist[ alphai ] <= t8cl && rightleftdist[ alphai + 1 ] > t8cl )    t8  = rightleftline[ alphai ];
    if ( rightleftdist[ alphai ] <= t7cl && rightleftdist[ alphai + 1 ] > t7cl )    t7  = rightleftline[ alphai ];
    }

                                        // force symmetrize
t8.Y    = t7.Y  = ( t8.Y + t7.Y ) / 2;
t8.Z    = t7.Z  = ( t8.Z + t7.Z ) / 2;
t8.X    = ( abs ( t8.X ) + abs ( t7.X ) ) / 2;
t7.X    = -t8.X;

                                        // in absolute original coordinates
tentenpositions.Add ( t8 );     tentennames.Add ( "T8" );
tentenpositions.Add ( t7 );     tentennames.Add ( "T7" );
}


//----------------------------------------------------------------------------
bool    TVolumeDoc::IsTemplate ()  const
{
return  IsTemplateFileName ( (char *) GetDocPath () );
}


//----------------------------------------------------------------------------
void    TVolumeDoc::Filter ( FilterTypes filtertype )
{
char                answer[ 32 ];
FctParams           p;


if ( filtertype == FilterTypeMean
  || filtertype == FilterTypeGaussian
  || filtertype == FilterTypeFastGaussian
  || filtertype == FilterTypeAnisoGaussian ) {
                                        // Trick: Fast Gaussian can have negative values, used as the number of repetitions
    if ( ! GetValueFromUser ( "Diameter of the filter (any value >= 2.0):", FilterPresets[ filtertype ].Text, p ( FilterParamDiameter ), "3.47", CartoolMainWindow )
      || ! ( filtertype == FilterTypeFastGaussian || p ( FilterParamDiameter ) > 0 ) )
        return;

                                        // here, we bypass user's will to boost the processing, or make sure we filter something...
    if      ( filtertype == FilterTypeGaussian     && p ( FilterParamDiameter ) >= 4 )    filtertype  = FilterTypeFastGaussian;
    else if ( filtertype == FilterTypeFastGaussian && p ( FilterParamDiameter ) <  4 )    filtertype  = FilterTypeGaussian;

    StringCopy  ( NiftiIntentName, NiftiIntentNameDefault );
    }


else if ( filtertype == FilterTypeIntensityMult
       || filtertype == FilterTypeIntensityDiv ) {

    if ( ! GetValueFromUser ( filtertype == FilterTypeIntensityMult ? "Intensity multiplying factor:" : "Intensity dividing factor:", FilterPresets[ filtertype ].Text, p ( FilterParamOperand ), "", CartoolMainWindow )
      || p ( FilterParamOperand ) == 1.0 )
        return;
    }


else if ( filtertype == FilterTypeIntensityAdd
       || filtertype == FilterTypeIntensitySub ) {

    if ( ! GetValueFromUser ( filtertype == FilterTypeIntensityAdd ? "Intensity adding value:" : "Intensity subtracting value:", FilterPresets[ filtertype ].Text, p ( FilterParamOperand ), "", CartoolMainWindow )
      || p ( FilterParamOperand ) == 0.0 )
        return;
    }


else if ( filtertype == FilterTypeIntensityRemap ) {

    if ( ! GetValueFromUser ( "Remapping from value:", FilterPresets[ filtertype ].Text, p ( FilterParamRemapFrom ), "", CartoolMainWindow ) )
        return;

    if ( ! GetValueFromUser ( "Into remapped value:", FilterPresets[ filtertype ].Text, p ( FilterParamRemapTo ), "", CartoolMainWindow ) )
        return;

                                        // nothing to be remapped?
    if ( p ( FilterParamRemapFrom ) == p ( FilterParamRemapTo ) )
        return;
    }


else if ( filtertype == FilterTypeKeepValue ) {

    if ( ! GetValueFromUser ( "Keeping only value:", FilterPresets[ filtertype ].Text, p ( FilterParamKeepValue ), "", CartoolMainWindow ) )
        return;
    }


else if ( filtertype == FilterTypeIntensityNorm ) {
                                        // no parameters

    StringCopy  ( NiftiIntentName, NiftiIntentNameDefault );
    }


else if ( filtertype == FilterTypeMin
       || filtertype == FilterTypeMax
       || filtertype == FilterTypeMinMax
       || filtertype == FilterTypeRange
       || filtertype == FilterTypeMedian
       || filtertype == FilterTypeInterquartileMean
       || filtertype == FilterTypeMaxMode
       || filtertype == FilterTypeMeanSub
       || filtertype == FilterTypeLogMeanSub
       || filtertype == FilterTypeMeanDiv
       || filtertype == FilterTypeLogMeanDiv
       || filtertype == FilterTypeMAD
       || filtertype == FilterTypeSD
       || filtertype == FilterTypeSDInv
       || filtertype == FilterTypeMADSDInv
       || filtertype == FilterTypeCoV
       || filtertype == FilterTypeLogCoV
       || filtertype == FilterTypeLogSNR
       || filtertype == FilterTypeMCoV
       || filtertype == FilterTypeLogMCoV
       || filtertype == FilterTypeModeQuant
       || filtertype == FilterTypeEntropy
       || filtertype == FilterTypePercentFullness
       || filtertype == FilterTypeCutBelowFullness
       || filtertype == FilterTypeSAV
       || filtertype == FilterTypeCompactness
       || filtertype == FilterTypeRelax ) {

    if ( ! GetValueFromUser ( "Diameter of the filter (2 / 2.83 / 3.47 and multiples, or integers):", FilterPresets[ filtertype ].Text, p ( FilterParamDiameter ), "3.47", CartoolMainWindow ) || p ( FilterParamDiameter ) < 0 )
        return;


    if ( filtertype == FilterTypeModeQuant ) {

        if ( ! GetValueFromUser ( "Number of final grey levels, including background (>=2):", FilterPresets[ filtertype ].Text, p ( FilterParamNumModeQuant ), "", CartoolMainWindow ) || p ( FilterParamNumModeQuant ) < 2 )
            return;

        StringCopy  ( NiftiIntentName, NiftiIntentNameLabels );
        }

    else if ( filtertype == FilterTypeMeanSub
           || filtertype == FilterTypeLogMeanSub
//         || filtertype == FilterTypeMeanDiv
           || filtertype == FilterTypeLogMeanDiv
           || filtertype == FilterTypeLogCoV
           || filtertype == FilterTypeLogSNR     
           || filtertype == FilterTypeLogMCoV    ) {

        p ( FilterParamResultType )     = GetResultTypeFromUser ( FilterPresets[ filtertype ].Text, filtertype == FilterTypeMeanSub ? "N" : "S", CartoolMainWindow );
        if ( p ( FilterParamResultType ) < 0 )  return;

        p ( FilterParamLogOffset )     = filtertype == FilterTypeLogSNR ? 0.001 : 0.1;

        StringCopy  ( NiftiIntentName, NiftiIntentNameDefault );
        }

    else if ( filtertype == FilterTypeRelax ) {

        if ( ! GetValueFromUser ( "Number of repetitions:", FilterPresets[ filtertype ].Text, p ( FilterParamNumRelax ), "1", CartoolMainWindow ) || p ( FilterParamNumRelax ) <= 0 )
            return;
        }

    else if ( filtertype == FilterTypeCutBelowFullness ) {

        if ( ! GetValueFromUser ( "Percentage of fullness to cut (0..100):", FilterPresets[ filtertype ].Text, p ( FilterParamFullnessCut ), "50", CartoolMainWindow ) )
            return;

        p ( FilterParamFullnessCut )    /= 100.0;
        if ( p ( FilterParamFullnessCut ) <= 0 )  return;

        StringCopy  ( NiftiIntentName, NiftiIntentNameDefault );
        }

    }


else if ( filtertype == FilterTypeThreshold ) {

    FloatToString ( answer, BackgroundValue );

    if ( ! GetValueFromUser ( "Min value to keep:", FilterPresets[ filtertype ].Text, p ( FilterParamThresholdMin ), answer, CartoolMainWindow ) )
        return;

    FloatToString ( answer, MaxValue );

    if ( ! GetValueFromUser ( "Max value to keep:", FilterPresets[ filtertype ].Text, p ( FilterParamThresholdMax ), answer, CartoolMainWindow ) )
        return;

    StringCopy  ( NiftiIntentName, NiftiIntentNameDefault );
    }


else if ( filtertype == FilterTypeThresholdAbove
       || filtertype == FilterTypeThresholdBelow ) {

    FloatToString ( answer, BackgroundValue );

    if ( ! GetValueFromUser ( filtertype == FilterTypeThresholdAbove ? "Min value to keep:" : "Max value to keep:", FilterPresets[ filtertype ].Text, p ( FilterParamThreshold ), answer, CartoolMainWindow ) )
        return;

    StringCopy  ( NiftiIntentName, NiftiIntentNameDefault );
    }


else if ( filtertype == FilterTypeBinarize ) {

//  if ( ! GetValueFromUser ( "Binarize to new value:", FilterPresets[ filtertype ].Text, p ( FilterParamBinarized ), "1", CartoolMainWindow ) )
//      return;
                                        // not asking, a binary mask will have special treatment if value is 1
    p ( FilterParamBinarized )     = 1;

    StringCopy  ( NiftiIntentName, NiftiIntentNameLabels );
    }


else if ( filtertype == FilterTypeToMask ) {

    FloatToString ( answer, BackgroundValue );

    if ( ! GetValueFromUser ( "Background threshold:", FilterPresets[ filtertype ].Text, p ( FilterParamToMaskThreshold ), answer, CartoolMainWindow ) )
        return;

    IntegerToString ( answer, IsInteger ( MaxValue ) ? MaxValue : 1 );

//  if ( ! GetValueFromUser ( "Binarize to new value:", FilterPresets[ filtertype ].Text, p ( FilterParamToMaskNewValue ), answer, CartoolMainWindow ) )
//      return;
                                        // not asking, a binary mask will have special treatment if value is 1
    p ( FilterParamToMaskNewValue )     = 1;

//  p ( FilterParamToMaskCarveBack )     = GetAnswerFromUser ( "Carving back all concave parts?", FilterPresets[ filtertype ].Text, CartoolMainWindow );
                                        // not asking, this seems the 99% used case anyway
    p ( FilterParamToMaskCarveBack )     = true;

    StringCopy  ( NiftiIntentName, NiftiIntentNameLabels );
    }


else if ( filtertype == FilterTypeSymmetrize ) {

    char                answeraxis      = GetOptionFromUser ( "Axis of symmetry, X, Y or Z:", FilterPresets[ filtertype ].Text, "X Y Z", 0, CartoolMainWindow );

    if ( answeraxis == EOS )
        return;
                                        // X:0  Y:1  Z:2
    p ( FilterParamSymmetrizeAxis )     = answeraxis - 'X';

                                        // Suggesting current origin as the center of symmetry, or center if not defined
    IntegerToString ( answer, Truncate ( Origin.IsNotNull () ? Origin             [ p ( FilterParamSymmetrizeAxis ) ] 
                                                             : GetDefaultCenter ()[ p ( FilterParamSymmetrizeAxis ) ] ) );

    if ( ! GetValueFromUser ( "Center of symmetry, in voxel space:", FilterPresets[ filtertype ].Text, p ( FilterParamSymmetrizeOrigin ), answer, CartoolMainWindow ) )
        return;

    p ( FilterParamSymmetrizeOrigin )     = Truncate ( p ( FilterParamSymmetrizeOrigin ) );
    }


else if ( filtertype == FilterTypeLessNeighbors
       || filtertype == FilterTypeMoreNeighbors ) {

//  p ( FilterParamNumNeighbors )     = GetValueFromUser ( filtertype == FilterTypeLessNeighbors ? "Reset voxels having less neighbors than (1..18):" : "Set voxels having more neighbors than (0..17):",
    if ( ! GetValueFromUser ( filtertype == FilterTypeLessNeighbors ? "Reset voxels having less neighbors than (1..26):" : "Set voxels having more neighbors than (0..26):",
                      FilterPresets[ filtertype ].Text, p ( FilterParamNumNeighbors ), "", CartoolMainWindow ) || p ( FilterParamNumNeighbors ) < 1 )
        return;

//  p ( FilterParamNeighborhood )     = Neighbors18;
    p ( FilterParamNeighborhood )     = Neighbors26;
//    if ( ! GetValueFromUser ( "Type of neighborhood:", FilterPresets[ filtertype ].Text, p ( FilterParamNeighborhood ), "", CartoolMainWindow ) )
//        return;
    }


else if ( filtertype == FilterTypeKeepBiggestRegion
       || filtertype == FilterTypeKeepCompactRegion
       || filtertype == FilterTypeKeepBiggest2Regions
       || filtertype == FilterTypeKeepCompact2Regions ) {
                                        // no parameters

    StringCopy  ( NiftiIntentName, NiftiIntentNameLabels );
    }


else if ( filtertype == FilterTypeClustersToRegions ) {

    if ( ! GetValueFromUser ( "Minimum number of voxels per region:", FilterPresets[ filtertype ].Text, p ( FilterParamRegionMinSize ), "50", CartoolMainWindow ) )
        return;

    StringCopy  ( NiftiIntentName, NiftiIntentNameLabels );
    }


else if ( filtertype == FilterTypeRevert )
    ;   // no parameters


else if ( filtertype == FilterTypeHistoEqual ) {
                                        // no parameters

    StringCopy  ( NiftiIntentName, NiftiIntentNameDefault );
    }


else if ( filtertype == FilterTypeHistoCompact ) {

    if ( ! GetValueFromUser ( "Compact bins / grey levels which are empty, or nearly empty." NewLine "Give the threshold below which bins will be removed (0..100%):", FilterPresets[ filtertype ].Text, p ( FilterParamThreshold ), "0", CartoolMainWindow ) )
        return;

    StringCopy  ( NiftiIntentName, NiftiIntentNameDefault );
    }


else if ( filtertype == FilterTypeHistoEqualBrain ) {
                                        // no parameters

    StringCopy  ( NiftiIntentName, NiftiIntentNameDefault );
    }

else if ( filtertype == FilterTypeBiasField ) {

    p ( FilterParamBiasFieldRepeat )     = 1;
//    p ( FilterParamBiasFieldRepeat )     = GetValueFromUser ( "Number of repetitions (>= 1):", FilterPresets[ filtertype ].Text, CartoolMainWindow );
//    if ( p ( FilterParamBiasFieldRepeat ) <= 0 )  return;

    StringCopy  ( NiftiIntentName, NiftiIntentNameDefault );
    }


else if (  filtertype == FilterTypeErode
        || filtertype == FilterTypeDilate
        || filtertype == FilterTypeOpen
        || filtertype == FilterTypeClose
        || filtertype == FilterTypeMorphGradient
        || filtertype == FilterTypeMorphGradientInt
        || filtertype == FilterTypeMorphGradientExt ) {


    if ( ! GetValueFromUser ( "Diameter of the structural element (integer value >= 1):", FilterPresets[ filtertype ].Text, p ( FilterParamDiameter ), "1", CartoolMainWindow ) || p ( FilterParamDiameter ) < 0 )
        return;

    StringCopy  ( NiftiIntentName, NiftiIntentNameLabels );
    }


else if ( filtertype == FilterTypeThinning ) {

    if ( ! GetValueFromUser ( "Number of iterations (integer value >= 1, or 0 until convergence):", FilterPresets[ filtertype ].Text, p ( FilterParamThinningRepeat ), "0", CartoolMainWindow ) || p ( FilterParamThinningRepeat ) < 0 )
        return;

    StringCopy  ( NiftiIntentName, NiftiIntentNameLabels );
    }


else if ( filtertype == FilterTypeWaterfallRidges
       || filtertype == FilterTypeWaterfallValleys ) {

    StringCopy  ( NiftiIntentName, NiftiIntentNameLabels );
    }


else if (  filtertype == FilterTypeGradient
        || filtertype == FilterTypeCanny
        || filtertype == FilterTypeLaplacian
        || filtertype == FilterTypeHessianEigenMax
        || filtertype == FilterTypeHessianEigenMin
        || filtertype == FilterTypeKCurvature
        || filtertype == FilterTypeKCCurvature ) {

    if ( ! GetValueFromUser ( "Diameter of the filter (integer and odd values: 0, 3, 5...):", FilterPresets[ filtertype ].Text, p ( FilterParamDiameter ), "3", CartoolMainWindow ) || p ( FilterParamDiameter ) < 0 )
        return;


    if ( filtertype == FilterTypeKCCurvature ) {

        if ( ! GetValueFromUser ( "C Ratio (from size filter: 3->1e4, 5->1e3, 7->100):", FilterPresets[ filtertype ].Text, p ( FilterParamCRatio ), "", CartoolMainWindow ) || p ( FilterParamCRatio ) <= 0 )
            return;
        }


    if ( filtertype == FilterTypeGradient
      || filtertype == FilterTypeCanny
      || filtertype == FilterTypeHessianEigenMax
      || filtertype == FilterTypeHessianEigenMin )
        p ( FilterParamResultType )     = FilterResultPositive; // always
    else {
        p ( FilterParamResultType )     = GetResultTypeFromUser ( FilterPresets[ filtertype ].Text, "N", CartoolMainWindow );
        if ( p ( FilterParamResultType ) < 0 )  return;
        }

    StringCopy  ( NiftiIntentName, NiftiIntentNameDefault );
    }


else if ( filtertype == FilterTypePeeling
       || filtertype == FilterTypeRank
       || filtertype == FilterTypeRankRamp ) {

    FloatToString ( answer, BackgroundValue );

    if ( ! GetValueFromUser ( "Background threshold:", FilterPresets[ filtertype ].Text, p ( FilterParamThreshold ), answer, CartoolMainWindow ) )
        return;

    StringCopy  ( NiftiIntentName, NiftiIntentNameDefault );
    }


else if ( filtertype == FilterTypeSegmentWhite
       || filtertype == FilterTypeSegmentGrey
       || filtertype == FilterTypeSegmentCSF
       || filtertype == FilterTypeSegmentTissues
       || filtertype == FilterTypeSkullStripping
       || filtertype == FilterTypeBrainstemRemoval ) {

    if      ( filtertype == FilterTypeSegmentWhite ) {
        if ( ! GetValueFromUser ( "Details level 0..100:", FilterPresets[ filtertype ].Text, p ( FilterParamWhiteDetails ), "", CartoolMainWindow ) )
            return;

        p ( FilterParamWhiteDetails )    /= 100;
        if ( p ( FilterParamWhiteDetails ) < 0 )  return;

        StringCopy  ( NiftiIntentName, NiftiIntentNameLabels );
        }

    else if ( filtertype == FilterTypeSegmentGrey ) {

        int                 g;

        if ( ! GetValueFromUser ( "Thickness type  1:Thin / 2:Regular / 3:Fat / 4:Whole brain :", FilterPresets[ filtertype ].Text, g, "2", CartoolMainWindow ) )
            return;

        if ( g < 0 || g > 4 )   return;

        p ( FilterParamGreyType )     = ( g == 1 ? GreyMatterSlim
                                        : g == 2 ? GreyMatterRegular
                                        : g == 3 ? GreyMatterFat
                                        : g == 4 ? GreyMatterWhole
                                        :          GreyMatterRegular );

        bool                bf  = false; // g != 4 ? GetAnswerFromUser ( "Correct for Bias Field beforehand?", FilterPresets[ filtertype ].Text, CartoolMainWindow ) : false;

//      p ( FilterParamGreyType )     = ( (GreyMatterFlags) p ( FilterParamGreyType ) ) |        GreyMatterGaussian;
        p ( FilterParamGreyType )     = ( (GreyMatterFlags) p ( FilterParamGreyType ) ) | ( bf ? GreyMatterBiasField : GreyMatterNothing );
        p ( FilterParamGreyType )     = ( (GreyMatterFlags) p ( FilterParamGreyType ) ) |        GreyMatterAsymmetric;
//      p ( FilterParamGreyType )     = ( (GreyMatterFlags) p ( FilterParamGreyType ) ) |        GreyMatterSymmetric;
        p ( FilterParamGreyType )     = ( (GreyMatterFlags) p ( FilterParamGreyType ) ) |        GreyMatterPostprocessing;

        p ( FilterParamGreyAxis )     = GetAxisIndex ( LeftRight );                   // for symmetric case
        p ( FilterParamGreyOrigin )   = GetOrigin ()[ GetAxisIndex ( LeftRight ) ] ? GetOrigin        ()[ GetAxisIndex ( LeftRight ) ]
                                                                                   : GetDefaultCenter ()[ GetAxisIndex ( LeftRight ) ];   // risky if center is not the symmetric pivot!

        StringCopy  ( NiftiIntentName, NiftiIntentNameGreyMask );
        }

    else if ( filtertype == FilterTypeSegmentCSF ) {

        StringCopy  ( NiftiIntentName, NiftiIntentNameLabels );
        }

    else if ( filtertype == FilterTypeSegmentTissues ) {

        bool                bf  = false; // GetAnswerFromUser ( "Correct for Bias Field beforehand?", FilterPresets[ filtertype ].Text, CartoolMainWindow );

        p ( FilterParamTissuesParams )     = bf ? GreyMatterBiasField : 0;

        StringCopy  ( NiftiIntentName, NiftiIntentNameLabels );
        }

    else if ( filtertype == FilterTypeSkullStripping ) {

        p ( FilterParamSkullStrippingMethod )     = GetSkullStrippingTypeFromUser ( FilterPresets[ filtertype ].Text, SkullStrippingDialogDefault, CartoolMainWindow );
        
        if ( p ( FilterParamSkullStrippingMethod ) == SkullStrippingNone )
            return;

        p ( FilterParamSkullStrippingVoxelSize )     = VoxelSize.Mean ();
        p ( FilterParamSkullStrippingIsTemplate )    = IsTemplateFileName ( (char *) GetDocPath () );    // only for version 1 - this works if new name has not altered the very beginning of the file name

        StringCopy  ( NiftiIntentName, NiftiIntentNameBrain );
        }

    else if ( filtertype == FilterTypeBrainstemRemoval ) {

//      p ( FilterParamBrainstemMethod    ) = -1;               // not used
        p ( FilterParamBrainstemVoxelSize ) = VoxelSize.Mean ();

//      StringCopy  ( NiftiIntentName, NiftiIntentNameBrain );
        }
    }

    else if ( filtertype == FilterTypeMaskToSolutionPoints ) {

                                        // init with 'this' - otherwise let BrainToSolutionPointsUI handle all other cases
        TVolumeDoc*         braindoc        = IsBrain () ? this : 0;
        TVolumeDoc*         greydoc         = IsMask  () ? this : 0;

        BrainToSolutionPointsUI ( braindoc, greydoc );
                                        // volume has not been changed - leave now
        return;
        }

/*
else if ( filtertype == FilterTypeResize ) {

    if ( ! GetValueFromUser ( "New size in X:", FilterPresets[ filtertype ].Text, p ( FilterParamResizeDim1 ), "", CartoolMainWindow ) || p ( FilterParamResizeDim1 ) < 0 )
        return;

    char            buff[ 32 ];
    IntegerToString ( buff, p ( FilterParamResizeDim1 ) );

    if ( ! GetValueFromUser ( "New size in Y:", FilterPresets[ filtertype ].Text, p ( FilterParamResizeDim2 ), buff, CartoolMainWindow ) || p ( FilterParamResizeDim2 ) < 0 )
        return;

    IntegerToString ( buff, p ( FilterParamResizeDim2 ) );

    if ( ! GetValueFromUser ( "New size in Z:", FilterPresets[ filtertype ].Text, p ( FilterParamResizeDim3 ), buff, CartoolMainWindow ) || p ( FilterParamResizeDim3 ) < 0 )
        return;

    if ( ! (    IsInsideLimits ( p ( FilterParamResizeDim1 ), 1.0, 1024.0 ) 
             && IsInsideLimits ( p ( FilterParamResizeDim2 ), 1.0, 1024.0 ) 
             && IsInsideLimits ( p ( FilterParamResizeDim3 ), 1.0, 1024.0 ) ) )
        return;

    StringCopy  ( NiftiIntentName, NiftiIntentNameDefault );

    Size.Set ( 0, p ( FilterParamResizeDim1 ) - 1, 0, p ( FilterParamResizeDim2 ) - 1, 0, p ( FilterParamResizeDim3 ) - 1 );
                                        // after?
    NotifyDocViews ( vnViewUpdated );
    }
*/

                                        // pass all other filters without parameters(?)
//else // unknown filter
//    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Some filters / operations can take quite some time (we could be more specific here),
                                        // and Filter being not already called from within a batch, we can temporarily lower the process priority
SetProcessPriority ( BatchProcessingPriority );

Data.Filter ( filtertype, p, true );

SetProcessPriority ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check for filters that always give a low isosurface cut
bool                lowisocut       = filtertype == FilterTypeRange
                                   || filtertype == FilterTypeMinMax
                                   || filtertype == FilterTypeMeanSub
                                   || filtertype == FilterTypeLogMeanSub
                                   || filtertype == FilterTypeMeanDiv
                                   || filtertype == FilterTypeLogMeanDiv
                                   || filtertype == FilterTypeMAD
                                   || filtertype == FilterTypeSD
                                   || filtertype == FilterTypeSDInv
                                   || filtertype == FilterTypeMADSDInv
                                   || filtertype == FilterTypeCoV
                                   || filtertype == FilterTypeLogCoV
                                   || filtertype == FilterTypeLogSNR
                                   || filtertype == FilterTypeMCoV
                                   || filtertype == FilterTypeLogMCoV
                                   || filtertype == FilterTypeModeQuant
                                   || filtertype == FilterTypeEntropy
                                   || filtertype == FilterTypePercentFullness
                                   || filtertype == FilterTypeSAV
                                   || filtertype == FilterTypeCompactness
                                   || filtertype == FilterTypeWaterfallRidges
                                   || filtertype == FilterTypeWaterfallValleys
                                   || filtertype == FilterTypeGradient
                                   || filtertype == FilterTypeCanny
                                   || filtertype == FilterTypeLaplacian
                                   || filtertype == FilterTypeHessianEigenMax
                                   || filtertype == FilterTypeHessianEigenMin
                                   || filtertype == FilterTypeKCurvature
                                   || filtertype == FilterTypeKCCurvature
                                   || filtertype == FilterTypeSegmentCSF
                                   || filtertype == FilterTypeSegmentGrey
                                   || filtertype == FilterTypeSegmentWhite
                                   || filtertype == FilterTypeSegmentTissues
                                   || filtertype == FilterTypePeeling;


ResetThenSetMri ( (MriInitFlags) ( ( lowisocut  ?   MriInitBackgroundKeep           // low isosurface will force a background of 0
                                                :   MriInitBackgroundDetect   )     // otherwise, use the default automatic background

                                   |                MriInitIsoSurfaceCutDetect      // update isosurface cut
                                   |                MriInitTypeDetect               // update type..
                                   |                MriInitReorientKeep       ),    // ..but not orientation
                                   0 );                                             // the forced background value


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Nicely updating the file name
TFileName           newfilename ( GetDocPath (), TFilenameExtendedPath );

                                        // Some filters change the content type, f.ex. ".Head." into ".Brain." or ".Brain." into ".Grey."
if      ( filtertype == FilterTypeSkullStripping )

    StringReplace   ( newfilename,  "." InfixHead ".",  "." );

if      ( filtertype == FilterTypeSegmentCSF 
       || filtertype == FilterTypeSegmentGrey 
       || filtertype == FilterTypeSegmentWhite 
       || filtertype == FilterTypeSegmentTissues )

    StringReplace   ( newfilename,  "." InfixBrain ".", "." );


PostfixFilename     ( newfilename,  "." );
PostfixFilename     ( newfilename,  FilterPresets[ filtertype ].Ext );
                                        // force change extension, maybe? - problem is f.ex. Analyze can have any orientation, which will kind of mess up with Nifti
ReplaceExtension    ( newfilename,  DefaultMriExt );

CheckNoOverwrite    ( newfilename );
                                        // set to new name
SetDocPath          ( newfilename );

                                        // and force save to file(?)
Commit ( true );
}


//----------------------------------------------------------------------------
void    TVolumeDoc::FindOrientation ()
{
//DBGM ( "FindOrientation", "MRI" );

                                        // empty volumes are not welcomed here
if ( MinValue == MaxValue || MaxValue == 0
                                        // or too small volume, which don't have enough features
  || GetSize ()->MaxSize () <= 16 || GetBounding ()->MaxSize () <= 16
                                        // or way too elongated
  || GetSize ()->MinSize () <= 25 || GetSize ()->MaxSize () / GetSize ()->MinSize () > 5 )
    return;


double              maxvoxelsize    = VoxelSize.Max ();
double              minvoxelsize    = VoxelSize.Min ();
                                        // voxels are too anisotropic?
if ( maxvoxelsize > 0 && minvoxelsize > 0 && maxvoxelsize / minvoxelsize > 2.5 )
    return;


if ( IsFullHead () )    FindOrientationFullHead  ();
else                    FindOrientationSegmented ();
}


//----------------------------------------------------------------------------
void    TVolumeDoc::FindOrientationCommon  (   Volume&         vol,
                                                TPointFloat&    center, 
                                                double&         back, 
                                                TPointInt&      boundmin,   TPointInt&          boundmax 
                                            )
{
int                 spatialdownsampling = AtLeast ( 1, Round ( Size.MaxSize () / 32.0 ) );

//DBGV4 ( Size.GetXExtent (), Size.GetYExtent (), Size.GetZExtent (), spatialdownsampling, "spatialdownsampling" );


bool                ismask          = IsMask ();
bool                isbinarymask    = IsBinaryMask ();

TPointFloat         middle;
TPointFloat         barycenter;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get background value
back            = IsMask () || IsBlob () ? 1 : BackgroundValue;

//DBGV ( back, "Back threshold, original" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get some informations about size and center
TPointInt           p1;
TPointInt           p2;
TPointInt           d;
double              sumv;

                                        // stepping
if ( ismask )                           // in case of sparse content, we need to be more thorought
    d.Set ( spatialdownsampling / 2.0 + 0.5, spatialdownsampling / 2.0 + 0.5, spatialdownsampling / 2.0 + 0.5 );
else
    d.Set ( spatialdownsampling, spatialdownsampling, spatialdownsampling );


boundmin.X      = Bounding->XMin ();
boundmin.Y      = Bounding->YMin ();
boundmin.Z      = Bounding->ZMin ();
boundmax.X      = Bounding->XMax ();
boundmax.Y      = Bounding->YMax ();
boundmax.Z      = Bounding->ZMax ();

Bounding->GetMiddle ( middle );
                                        // default is to use the middle as the center
center          = middle;

                                        // now, compute a center based on the global shape we have

sumv            = 0;

for ( p1.X = boundmin.X; p1.X <= boundmax.X; p1.X += d.X )
for ( p1.Y = boundmin.Y; p1.Y <= boundmax.Y; p1.Y += d.Y )
for ( p1.Z = boundmin.Z; p1.Z <= boundmax.Z; p1.Z += d.Z )

    if ( Data ( p1.X, p1.Y, p1.Z ) >= back ) {
        barycenter += TPointFloat ( p1.X, p1.Y, p1.Z );
        sumv++;
        }

barycenter     /= sumv ? sumv : 1;

                                        // choose to switch to barycenter if it appears to be far enough from center,
                                        // which is the case when there is noise on the background,
                                        // but not when it is caused by missing parts in the brain
//double              centershift     = ( barycenter - center ).Norm () / Bounding->Radius ();

                                        // min     shifted: 18% (MNI 10.12% ?)
                                        // max not shifted:  8%
//if ( centershift > 0.09 && barycenter != 0 ) // 0.13
                                        // always, except if new center is 0
if ( barycenter != 0 )
    center      = barycenter;

//DBGV2  ( centershift * 100, centershift > 0.09, "DeltaCenter  newcenter?" );
//middle.    Show ( "Middle" );
//barycenter.Show ( "Barycenter" );
//center.    Show ( "Center" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // now work with a downsampled version (smaller, better, simpler)
                                        // masks should avoid blurred filtering; ROI masks can approximate with a Max filter, too (or even thresholded?)
vol             = Volume    (   Data, 
                                spatialdownsampling,    ismask ? 3 : 2, 
                                ismask  ? ( isbinarymask ? FilterTypeMax : FilterTypeMax /*FilterTypeMedian*/ ) 
                                        : FilterTypeMean 
                            );

                                        // don't update background if a mask, the downsampling might have screw up the auto detection
back            = IsMask () || IsBlob () ? 1 : vol.GetBackgroundValue ();

//if ( StringContains ( GetTitle (), "MNI305.raw." ) )
//    back    = 35;

//DBGV ( back, "Back threshold, downsampled" );


boundmin       /= spatialdownsampling;
boundmax        = ( boundmax + TPointInt ( spatialdownsampling - 1, spatialdownsampling - 1, spatialdownsampling - 1 ) ) / spatialdownsampling;
                                        // go 1 more voxels deep, for later scan will access neighbors
boundmin.X      = max ( boundmin.X, 1 );
boundmin.Y      = max ( boundmin.Y, 1 );
boundmin.Z      = max ( boundmin.Z, 1 );
boundmax.X      = min ( boundmax.X, vol.GetDim1 () - 2 );
boundmax.Y      = min ( boundmax.Y, vol.GetDim2 () - 2 );
boundmax.Z      = min ( boundmax.Z, vol.GetDim3 () - 2 );

middle         /= spatialdownsampling;
barycenter     /= spatialdownsampling;
//center          = ( center + TPointFloat ( spatialdownsampling / 2, spatialdownsampling / 2, spatialdownsampling / 2 ) ) / spatialdownsampling;
                                        // truncate to integer, like re-opening the donwsampled MRI
center.X        = (int) ( center.X / spatialdownsampling );
center.Y        = (int) ( center.Y / spatialdownsampling );
center.Z        = (int) ( center.Z / spatialdownsampling );

//center.Show ( "Downsampled Center" );

d.Set ( 1, 1, 1 );

/*
if ( VkQuery () ) {
    TFileName           _file;
    double              mricenter[ 3 ];

    mricenter[ 0 ]  = center.X;
    mricenter[ 1 ]  = center.Y;
    mricenter[ 2 ]  = center.Z;

    StringCopy ( _file, GetDocPath () );
    PostfixFilename ( _file, ".Downsampled" );

    vol.WriteFile ( _file, mricenter );
    }
*/
}


//----------------------------------------------------------------------------
void    TVolumeDoc::FindOrientationSegmented ()
{
bool                ismask          = IsMask ();

TEasyStats          stat1;
TEasyStats          stat2;
TEasyStats          stat3;
TEasyStats          quality;

TOrientationType    XAxis           = OrientationUnknown;
TOrientationType    YAxis           = OrientationUnknown;
TOrientationType    ZAxis           = OrientationUnknown;

Volume              vol;
TPointFloat         center;
double              back;
TPointInt           boundmin;
TPointInt           boundmax;
TPointInt           p1;
TPointInt           p2;
double              sumv;


FindOrientationCommon   (   vol, 
                            center, 
                            back, 
                            boundmin,   boundmax 
                        );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) find Left-Right axis

                                        // Compute the SD of the voxel differences between the supposed Left and Right
                                        // symetrically to the center
                                        // the lowest SD means the less differences between the two halves
                                        // which correspond to the left-right axis

stat1.Reset ();     stat2.Reset ();     stat3.Reset ();

for ( p1.Y = boundmin.Y; p1.Y <= boundmax.Y; p1.Y++ )
for ( p1.Z = boundmin.Z; p1.Z <= boundmax.Z; p1.Z++ )
for ( p1.X = center.X + 1, p2.X = center.X - 1; p1.X < boundmax.X && p2.X > boundmin.X; p1.X++, p2.X-- )
                                        // filter data, then just count all boolean difference (ie shape)
    stat1.Add ( fabs ( (double) 2 * ( vol ( p2.X, p1.Y, p1.Z ) >= back ) + ( vol ( p2.X - 1, p1.Y, p1.Z ) >= back ) + ( vol ( p2.X + 1, p1.Y, p1.Z ) >= back )
                              - 2 * ( vol ( p1.X, p1.Y, p1.Z ) >= back ) - ( vol ( p1.X - 1, p1.Y, p1.Z ) >= back ) - ( vol ( p1.X + 1, p1.Y, p1.Z ) >= back ) ) );


for ( p1.X = boundmin.X; p1.X <= boundmax.X; p1.X++ )
for ( p1.Z = boundmin.Z; p1.Z <= boundmax.Z; p1.Z++ )
for ( p1.Y = center.Y + 1, p2.Y = center.Y - 1; p1.Y < boundmax.Y && p2.Y > boundmin.Y; p1.Y++, p2.Y-- )

    stat2.Add ( fabs ( (double) 2 * ( vol ( p1.X, p2.Y, p1.Z ) >= back ) + ( vol ( p1.X, p2.Y - 1, p1.Z ) >= back ) + ( vol ( p1.X, p2.Y + 1, p1.Z ) >= back )
                              - 2 * ( vol ( p1.X, p1.Y, p1.Z ) >= back ) - ( vol ( p1.X, p1.Y - 1, p1.Z ) >= back ) - ( vol ( p1.X, p1.Y + 1, p1.Z ) >= back ) ) );


for ( p1.X = boundmin.X; p1.X <= boundmax.X; p1.X++ )
for ( p1.Y = boundmin.Y; p1.Y <= boundmax.Y; p1.Y++ )
for ( p1.Z = center.Z + 1, p2.Z = center.Z - 1; p1.Z < boundmax.Z && p2.Z > boundmin.Z; p1.Z++, p2.Z-- )

    stat3.Add ( fabs ( (double) 2 * ( vol ( p1.X, p1.Y, p2.Z ) >= back ) + ( vol ( p1.X, p1.Y, p2.Z - 1 ) >= back ) + ( vol ( p1.X, p1.Y, p2.Z + 1 ) >= back )
                              - 2 * ( vol ( p1.X, p1.Y, p1.Z ) >= back ) - ( vol ( p1.X, p1.Y, p1.Z - 1 ) >= back ) - ( vol ( p1.X, p1.Y, p1.Z + 1 ) >= back ) ) );


//DBGV3 ( fabs ( stat1.Average () ), stat1.SD (), fabs ( stat1.CoefficientOfVariation () ), "X  stat1: Avg SD Coeff" );
//DBGV3 ( fabs ( stat2.Average () ), stat2.SD (), fabs ( stat2.CoefficientOfVariation () ), "Y  stat2: Avg SD Coeff" );
//DBGV3 ( fabs ( stat3.Average () ), stat3.SD (), fabs ( stat3.CoefficientOfVariation () ), "Z  stat3: Avg SD Coeff" );


TVector< double >   symavg ( 3 );
symavg[ 0 ]     = 1 / NonNull ( stat1.Average () );
symavg[ 1 ]     = 1 / NonNull ( stat2.Average () );
symavg[ 2 ]     = 1 / NonNull ( stat3.Average () );
symavg.Normalize1 ();

//DBGV3 ( symavg[ 0 ] * 100, symavg[ 1 ] * 100, symavg[ 2 ] * 100, "Left-Right Avg" );


TVector< double >   symsd ( 3 );
symsd[ 0 ]      = 1 / NonNull ( stat1.SD () );
symsd[ 1 ]      = 1 / NonNull ( stat2.SD () );
symsd[ 2 ]      = 1 / NonNull ( stat3.SD () );
symsd.Normalize1 ();

//DBGV3 ( symsd[ 0 ] * 100, symsd[ 1 ] * 100, symsd[ 2 ] * 100, "Left-Right SD" );


                                        // Look for a central rift, at the place of corpus callosum
stat1.Reset ();     stat2.Reset ();     stat3.Reset ();
double              v;
double              v1;
double              v2;
double              v3;


for ( p1.Y = boundmin.Y; p1.Y <= boundmax.Y; p1.Y++ )
for ( p1.Z = boundmin.Z; p1.Z <= boundmax.Z; p1.Z++ )
    {
    v1  = 2 * ( vol ( center.X, p1.Y, p1.Z ) >= back ) - ( vol ( center.X - 1, p1.Y, p1.Z ) >= back ) - ( vol ( center.X + 1, p1.Y, p1.Z ) >= back );
    center.X--;
    v2  = 2 * ( vol ( center.X, p1.Y, p1.Z ) >= back ) - ( vol ( center.X - 1, p1.Y, p1.Z ) >= back ) - ( vol ( center.X + 1, p1.Y, p1.Z ) >= back );
    center.X+=2;
    v3  = 2 * ( vol ( center.X, p1.Y, p1.Z ) >= back ) - ( vol ( center.X - 1, p1.Y, p1.Z ) >= back ) - (vol ( center.X + 1, p1.Y, p1.Z ) >= back );
    center.X--;

    v   = min ( v1, v2, v3 );

    if ( v < 0 )
        stat1.Add ( fabs ( v ) );
    }


for ( p1.X = boundmin.X; p1.X <= boundmax.X; p1.X++ )
for ( p1.Z = boundmin.Z; p1.Z <= boundmax.Z; p1.Z++ )
    {
    v1  = 2 * ( vol ( p1.X, center.Y, p1.Z ) >= back ) - ( vol ( p1.X, center.Y - 1, p1.Z ) >= back ) - ( vol ( p1.X, center.Y + 1, p1.Z ) >= back );
    center.Y--;
    v2  = 2 * ( vol ( p1.X, center.Y, p1.Z ) >= back ) - ( vol ( p1.X, center.Y - 1, p1.Z ) >= back ) - ( vol ( p1.X, center.Y + 1, p1.Z ) >= back );
    center.Y+=2;
    v3  = 2 * ( vol ( p1.X, center.Y, p1.Z ) >= back ) - ( vol ( p1.X, center.Y - 1, p1.Z ) >= back ) - ( vol ( p1.X, center.Y + 1, p1.Z ) >= back );
    center.Y--;

    v   = min ( v1, v2, v3 );

    if ( v < 0 )
        stat2.Add ( fabs ( v ) );
    }


for ( p1.X = boundmin.X; p1.X <= boundmax.X; p1.X++ )
for ( p1.Y = boundmin.Y; p1.Y <= boundmax.Y; p1.Y++ )
    {
    v1  = 2 * ( vol ( p1.X, p1.Y, center.Z ) >= back ) - ( vol ( p1.X, p1.Y, center.Z - 1 ) >= back ) - ( vol ( p1.X, p1.Y, center.Z + 1 ) >= back );
    center.Z--;
    v2  = 2 * ( vol ( p1.X, p1.Y, center.Z ) >= back ) - ( vol ( p1.X, p1.Y, center.Z - 1 ) >= back ) - ( vol ( p1.X, p1.Y, center.Z + 1 ) >= back );
    center.Z+=2;
    v3  = 2 * ( vol ( p1.X, p1.Y, center.Z ) >= back ) - ( vol ( p1.X, p1.Y, center.Z - 1 ) >= back ) - ( vol ( p1.X, p1.Y, center.Z + 1 ) >= back );
    center.Z--;

    v   = min ( v1, v2, v3 );

    if ( v < 0 )
        stat3.Add ( fabs ( v ) );
    }

//DBGV3 ( fabs ( stat1.Average () ), stat1.SD (), fabs ( stat1.CoefficientOfVariation () ), "X  stat1: Avg SD Coeff" );
//DBGV3 ( fabs ( stat2.Average () ), stat2.SD (), fabs ( stat2.CoefficientOfVariation () ), "Y  stat2: Avg SD Coeff" );
//DBGV3 ( fabs ( stat3.Average () ), stat3.SD (), fabs ( stat3.CoefficientOfVariation () ), "Z  stat3: Avg SD Coeff" );


TVector< double >   centralrift ( 3 );
                                        // avoid the rift criterion for masks, either it does not exist, or it can be misleading (f.ex. Connectivity)
centralrift[ 0 ]    = ismask ? 1 : stat1.Average () * stat1.SD ();
centralrift[ 1 ]    = ismask ? 1 : stat2.Average () * stat2.SD ();
centralrift[ 2 ]    = ismask ? 1 : stat3.Average () * stat3.SD ();
centralrift.Normalize1 ();

//DBGV3 ( centralrift[ 0 ] * 100, centralrift[ 1 ] * 100, centralrift[ 2 ] * 100, "Left-Right Centralrift" );

                                        // Extent criterion: longer size is penalized
                                        // this usually doesn't help that much, but in some critical cases, this can make the difference!
TVector< double >   extent ( 3 );
extent[ 0 ]         = ( Bounding->GetYExtent () + Bounding->GetZExtent () ) / 2 / Bounding->GetXExtent ();
extent[ 1 ]         = ( Bounding->GetXExtent () + Bounding->GetZExtent () ) / 2 / Bounding->GetYExtent ();
extent[ 2 ]         = ( Bounding->GetXExtent () + Bounding->GetYExtent () ) / 2 / Bounding->GetZExtent ();
extent.Normalize1 ();

//DBGV3 ( extent[ 0 ] * 100, extent[ 1 ] * 100, extent[ 2 ] * 100, "Left-Right Extents" );


                                        // compose criterion
TVector< double >   symetry ( 3 );
                                        // avoid the rift criterion for masks, either it does not exist, or it can be misleading (f.ex. Connectivity)
symetry ( 0 )    = symavg[ 0 ] * symsd[ 0 ] * NonNull ( centralrift[ 0 ] ) * extent[ 0 ];
symetry ( 1 )    = symavg[ 1 ] * symsd[ 1 ] * NonNull ( centralrift[ 1 ] ) * extent[ 1 ];
symetry ( 2 )    = symavg[ 2 ] * symsd[ 2 ] * NonNull ( centralrift[ 2 ] ) * extent[ 2 ];
symetry.Normalize1 ();

//DBGV3 ( symetry ( 0 ) * 100, symetry ( 1 ) * 100, symetry ( 2 ) * 100, "Left-Right ratios" );

                                        // choose the max, save the relative max to the second max
if      ( symetry ( 0 ) >= symetry ( 1 ) && symetry ( 0 ) >= symetry ( 2 ) )    { XAxis   = LeftRight; quality.Add ( symetry ( 0 ) / ( symetry ( 0 ) + max ( symetry ( 1 ), symetry ( 2 ) ) ) ); }
else if ( symetry ( 1 ) >= symetry ( 0 ) && symetry ( 1 ) >= symetry ( 2 ) )    { YAxis   = LeftRight; quality.Add ( symetry ( 1 ) / ( symetry ( 1 ) + max ( symetry ( 0 ), symetry ( 2 ) ) ) ); }
else if ( symetry ( 2 ) >= symetry ( 1 ) && symetry ( 2 ) >= symetry ( 0 ) )    { ZAxis   = LeftRight; quality.Add ( symetry ( 2 ) / ( symetry ( 2 ) + max ( symetry ( 0 ), symetry ( 1 ) ) ) ); }


//DBGM3 ( OrientationLabels[ XAxis ], OrientationLabels[ YAxis ], OrientationLabels[ ZAxis ], "Geometry 1)" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) find Up-Down axis
                                        // Look at the Spherical / Diameter / Length on each side of the cube

TPoints*            surfp[ NumCubeSides ];

double              sphericalsd   [ NumCubeSides ];
double              sphericalcoeff[ NumCubeSides ];
double              diameterhalf  [ NumCubeSides ];
TPointFloat         bettercenter;
TPointFloat         normp;


for ( int s = 0; s < NumCubeSides; s++ )
    surfp[ s ]  = new TPoints;

                                        // get all safe surface points, from all sides of the cube
vol.GetSurfacePoints    (   surfp, 
                            back, 
                            TPointInt ( 1, 1, 1 ), 
                            center, 
                            0.80, 
                            (SurfacePointsFlags) ( SurfaceSafer | SurfaceInside ) 
                        );

/*
if ( VkQuery () ) {
    surfp[ 0 ]->WriteFile ( "E:\\Data\\CubSide.XMin.spi" );
    surfp[ 1 ]->WriteFile ( "E:\\Data\\CubSide.XMax.spi" );
    surfp[ 2 ]->WriteFile ( "E:\\Data\\CubSide.YMin.spi" );
    surfp[ 3 ]->WriteFile ( "E:\\Data\\CubSide.YMax.spi" );
    surfp[ 4 ]->WriteFile ( "E:\\Data\\CubSide.ZMin.spi" );
    surfp[ 5 ]->WriteFile ( "E:\\Data\\CubSide.ZMax.spi" );
    }
*/

for ( int s = 0; s < NumCubeSides; s++ ) {
                                        // be more precise: get the boundaries of the current side points
    TBoundingBox<double>    bounding ( *surfp[ s ] );

    bettercenter    = center;
                                        // set the center to the correct extrema
    if      ( s == 0 )      bettercenter.X  = bounding.XMax ();
    else if ( s == 1 )      bettercenter.X  = bounding.XMin ();
    else if ( s == 2 )      bettercenter.Y  = bounding.YMax ();
    else if ( s == 3 )      bettercenter.Y  = bounding.YMin ();
    else if ( s == 4 )      bettercenter.Z  = bounding.ZMax ();
    else if ( s == 5 )      bettercenter.Z  = bounding.ZMin ();

                                        // look for the sphericality & diameter
    stat1.Reset ();

    for ( int i = 0; i < surfp[ s ]->GetNumPoints (); i++ ) {
                                        // point from center
        normp       = (*surfp[ s ])[ i ] - bettercenter;
                                        // norm expresses sphericality
        stat1.Add ( normp.Norm () );
        }

                                        // store these variables per side
    sphericalsd   [ s ] = stat1.SD ();                      // quite solid
    sphericalcoeff[ s ] = stat1.CoefficientOfVariation ();  // even better
    diameterhalf  [ s ] = sqrt ( Square ( bounding.GetRadius ( s / 2 == 0 ? 1 : 0 ) )
                               + Square ( bounding.GetRadius ( s / 2 == 2 ? 1 : 2 ) ) );


//    DBGV3 ( s + 1, sphericalcoeff[ s ], diameterhalf[ s ], "side  Spherical  Diameter" );
//    DBGV4 ( s + 1, fabs ( stat1.Average () ), stat1.SD (), fabs ( stat1.CoefficientOfVariation () ), "side  stat1: Avg SD Coeff" );
    }


                                        // spherical criterion
TVector< double >   spherical ( 3 );
spherical[ 0 ]          = sphericalcoeff[ 0 ] + sphericalcoeff[ 1 ];
spherical[ 1 ]          = sphericalcoeff[ 2 ] + sphericalcoeff[ 3 ];
spherical[ 2 ]          = sphericalcoeff[ 4 ] + sphericalcoeff[ 5 ];
spherical.Normalize1 ();

//DBGV3 ( spherical[ 0 ] * 100, spherical[ 1 ] * 100, spherical[ 2 ] * 100, "Spherical SD" );


                                        // diameter criterion, not very powerful but it goes on the right direction
TVector< double >   diameter ( 3 );
diameter[ 0 ]           = diameterhalf[ 0 ] + diameterhalf[ 1 ];
diameter[ 1 ]           = diameterhalf[ 2 ] + diameterhalf[ 3 ];
diameter[ 2 ]           = diameterhalf[ 4 ] + diameterhalf[ 5 ];
diameter.Normalize1 ();

//DBGV3 ( diameter[ 0 ] * 100, diameter[ 1 ] * 100, diameter[ 2 ] * 100, "Diameter" );


                                        // extent criterion, also goes on the right direction
//DBGV3 ( extent[ 0 ] * 100, extent[ 1 ] * 100, extent[ 2 ] * 100, "Up-Down Extents" );


                                        // compose criterion
TVector< double >   updownratio ( 3 );
updownratio[ 0 ]        = NonNull ( spherical[ 0 ] ) * NonNull ( diameter[ 0 ] ) * NonNull ( extent[ 0 ] );
updownratio[ 1 ]        = NonNull ( spherical[ 1 ] ) * NonNull ( diameter[ 1 ] ) * NonNull ( extent[ 1 ] );
updownratio[ 2 ]        = NonNull ( spherical[ 2 ] ) * NonNull ( diameter[ 2 ] ) * NonNull ( extent[ 2 ] );
updownratio.Normalize1 ();

if ( updownratio == 0.0 )
    return;

//DBGV3 ( updownratio[ 0 ] * 100, updownratio[ 1 ] * 100, updownratio[ 2 ] * 100, "UpDown ratios" );


if      ( XAxis )
    if ( updownratio[ 1 ] > updownratio[ 2 ] )  { YAxis   = UpDown; quality.Add ( updownratio[ 1 ] / ( updownratio[ 1 ] + updownratio[ 2 ] ) ); }
    else                                        { ZAxis   = UpDown; quality.Add ( updownratio[ 2 ] / ( updownratio[ 1 ] + updownratio[ 2 ] ) ); }
else if ( YAxis )
    if ( updownratio[ 0 ] > updownratio[ 2 ] )  { XAxis   = UpDown; quality.Add ( updownratio[ 0 ] / ( updownratio[ 0 ] + updownratio[ 2 ] ) ); }
    else                                        { ZAxis   = UpDown; quality.Add ( updownratio[ 2 ] / ( updownratio[ 0 ] + updownratio[ 2 ] ) ); }
else if ( ZAxis )
    if ( updownratio[ 0 ] > updownratio[ 1 ] )  { XAxis   = UpDown; quality.Add ( updownratio[ 0 ] / ( updownratio[ 0 ] + updownratio[ 1 ] ) ); }
    else                                        { YAxis   = UpDown; quality.Add ( updownratio[ 1 ] / ( updownratio[ 0 ] + updownratio[ 1 ] ) ); }


//DBGM3 ( OrientationLabels[ XAxis ], OrientationLabels[ YAxis ], OrientationLabels[ ZAxis ], "Geometry 2)" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Up / Down direction
                                        // Look for a max for the superior direction

                                        // get the up down indexes
int                 updowni1        = XAxis == UpDown ? 0 : YAxis == UpDown ? 2 : 4;
int                 updowni2        = updowni1 + 1;

                                        // Re-use the previous spherical criterion
                                        // normalize
sumv        = sphericalsd[ updowni1 ] + sphericalsd[ updowni2 ];
sumv        = NonNull ( sumv );

sphericalsd[ updowni1 ]    /= sumv;
sphericalsd[ updowni2 ]    /= sumv;

//DBGV2 ( sphericalsd[ updowni1 ] * 100, sphericalsd[ updowni2 ] * 100, "Up/Down SphericalSD" );


                                        // normalize
sumv        = sphericalcoeff[ updowni1 ] + sphericalcoeff[ updowni2 ];
sumv        = NonNull ( sumv );

sphericalcoeff[ updowni1 ]    /= sumv;
sphericalcoeff[ updowni2 ]    /= sumv;

//DBGV2 ( sphericalcoeff[ updowni1 ] * 100, sphericalcoeff[ updowni2 ] * 100, "Up/Down SphericalCoeff" );


                                        // Scan for difference of shapes / accident on the surface (like for the face detection)
                                        // re-do the surface, with different parameters
for ( int s = 0; s < NumCubeSides; s++ )
    if ( surfp[ s ] ) {
        delete  surfp[ s ];
        surfp[ s ]  = 0;
        }

surfp[ updowni1 ] = new TPoints;
surfp[ updowni2 ] = new TPoints;

                                                            // was 1.50 ?
vol.GetSurfacePoints    (   surfp, 
                            back, 
                            TPointInt ( 1, 1, 1 ), 
                            center, 
                            1.00, 
                            (SurfacePointsFlags) ( SurfaceSafer | SurfaceInside ) 
                        );


TPointFloat         pf1;
TPointFloat         pf2;
double              d12;

stat1.Reset ();
stat2.Reset ();
                                        // scan for all pairs of neighbors
for ( int i = 0; i < surfp[ updowni1 ]->GetNumPoints (); i++ ) {

    pf1         = (*surfp[ updowni1 ])[ i ];

    for ( int j = i + 1; j < surfp[ updowni1 ]->GetNumPoints (); j++ ) {

        pf2         = (*surfp[ updowni1 ])[ j ];
        normp       = pf2 - pf1;
        d12         = fabs ( normp[ updowni1 / 2 ] );

        normp[ updowni1 / 2 ] = 0;
                                        // not a neighbor or too far in depth?
        if ( normp.Norm2 () == 0 || normp.Norm2 () > 1.5 || d12 > 5 ) {
            pf1         = pf2;
            continue;
            }

        stat1.Add ( d12 );
        }
    }


for ( int i = 0; i < surfp[ updowni2 ]->GetNumPoints (); i++ ) {

    pf1         = (*surfp[ updowni2 ])[ i ];

    for ( int j = i + 1; j < surfp[ updowni2 ]->GetNumPoints (); j++ ) {

        pf2         = (*surfp[ updowni2 ])[ j ];
        normp       = pf2 - pf1;
        d12         = fabs ( normp[ updowni1 / 2 ] );

        normp[ updowni1 / 2 ] = 0;
                                        // not a neighbor or too far in depth?
        if ( normp.Norm2 () == 0 || normp.Norm2 () > 1.5 || d12 > 5 ) {
            pf1         = pf2;
            continue;
            }

        stat2.Add ( d12 );
        }
    }

//DBGV3 ( stat1.Average (), stat1.SD (), stat1.CoefficientOfVariation (), "UD1:  Avg SD Coeff" );
//DBGV3 ( stat2.Average (), stat2.SD (), stat2.CoefficientOfVariation (), "UD2:  Avg SD Coeff" );


                                        // retrieve a set of variables
double              ud11            = stat1.CoefficientOfVariation ();
double              ud12            = stat2.CoefficientOfVariation ();
                                        // normalize
sumv        = ud11 + ud12;
sumv        = NonNull ( sumv );
ud11       /= sumv;
ud12       /= sumv;
//DBGV2 ( ud11 * 100, ud12 * 100, "UD ratios1" );


double              ud21            = stat1.SD ();
double              ud22            = stat2.SD ();
                                        // normalize
sumv        = ud21 + ud22;
sumv        = NonNull ( sumv );
ud21       /= sumv;
ud22       /= sumv;
//DBGV2 ( ud21 * 100, ud22 * 100, "UD ratios2" );


                                        // combine criterion
double              updown1;
double              updown2;

updown1         = sphericalcoeff[ updowni1 ] * sphericalsd[ updowni1 ] * ud11 * ud21;
updown2         = sphericalcoeff[ updowni2 ] * sphericalsd[ updowni2 ] * ud12 * ud22;

                                        // normalize
sumv        = updown1 + updown2;
sumv        = NonNull ( sumv );

updown1        /= sumv;
updown2        /= sumv;

//DBGV2 ( updown1 * 100, updown2 * 100, "Up/Down" );


if      ( XAxis == UpDown )     XAxis = updown1 < updown2 ? ToDown : ToUp;
else if ( YAxis == UpDown )     YAxis = updown1 < updown2 ? ToDown : ToUp;
else if ( ZAxis == UpDown )     ZAxis = updown1 < updown2 ? ToDown : ToUp;


quality.Add ( max ( updown1, updown2 ) );

//DBGM3 ( OrientationLabels[ XAxis ], OrientationLabels[ YAxis ], OrientationLabels[ ZAxis ], "Geometry 3)" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 4) Front-Back axis axis is the remaining one

if      ( XAxis && YAxis )  ZAxis = FrontBack;
else if ( XAxis && ZAxis )  YAxis = FrontBack;
else if ( YAxis && ZAxis )  XAxis = FrontBack;


//DBGM3 ( OrientationLabels[ XAxis ], OrientationLabels[ YAxis ], OrientationLabels[ ZAxis ], "Geometry 4)" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 5) Front / Back

int                 frontback1      = XAxis == FrontBack ? 0 : YAxis == FrontBack ? 2 : 4;
int                 frontback2      = frontback1 + 1;


                                        // diameter criterion, taken from previous computations (weak effect, but why not use it?)
diameter[ 0 ]           = diameterhalf[ frontback1 ];
diameter[ 1 ]           = diameterhalf[ frontback2 ];
                                        // normalize
sumv        = diameter[ 0 ] + diameter[ 1 ];
sumv        = NonNull ( sumv );

diameter[ 0 ]          /= sumv;
diameter[ 1 ]          /= sumv;

//DBGV2 ( diameter[ 0 ] * 100, diameter[ 1 ] * 100, "FB Diameter" );


                                        // Do a thourough scan of slices
int                 updowni         = updowni1 / 2;
int                 frontbacki      = XAxis == FrontBack ? 0 : YAxis == FrontBack ? 1 : 2;
int                 leftrighti      = updowni + frontbacki == 1 ? 2 : updowni + frontbacki == 3 ? 0 : 1;

int                 size[ 3 ];
size[ 0 ]       = vol.GetDim ( 0 );
size[ 1 ]       = vol.GetDim ( 1 );
size[ 2 ]       = vol.GetDim ( 2 );

TPointFloat         fbmin;
TPointFloat         fbmax;
TPointFloat         lrmin;
TPointFloat         lrmax;
TPointFloat         udmin;
TPointFloat         udmax;
int                 fbmini;
int                 fbmaxi;
int                 lrmini;
int                 lrmaxi;
//int                 udmini;
//int                 udmaxi;

double              d1;
double              d2;

                                        // First, we study the variation of sizes on each of the front / back sides
stat1.Reset ();
stat2.Reset ();

                                        // scanning from one front-back side
for ( pf1[ frontbacki ] = 0; pf1[ frontbacki ] < center[ frontbacki ]; pf1[ frontbacki ]++ ) {

    udmin       = udmax     = lrmin     = lrmax     = 0.0;

                                        // scan from each side, until we reach the volume
    for ( pf1[ updowni ] = 0; pf1[ updowni ] < size[ updowni ] && udmin == 0; pf1[ updowni ]++ )
    for ( pf1[ leftrighti ] = 0; pf1[ leftrighti ] < size[ leftrighti ]; pf1[ leftrighti ]++ )
        if ( vol ( pf1 ) >= back ) {
            udmin   = pf1;
            break;
            }

    if ( udmin == 0 )
        continue;                       // no other sides to search!


    for ( pf1[ updowni ] = size[ updowni ] - 1; pf1[ updowni ] >= 0 && udmax == 0; pf1[ updowni ]-- )
    for ( pf1[ leftrighti ] = 0; pf1[ leftrighti ] < size[ leftrighti ]; pf1[ leftrighti ]++ )
        if ( vol ( pf1 ) >= back ) {
            udmax   = pf1;
            break;
            }

                                      // left right positions
    for ( pf1[ leftrighti ] = 0; pf1[ leftrighti ] < size[ leftrighti ] && lrmin == 0; pf1[ leftrighti ]++ )
    for ( pf1[ updowni ] = 0; pf1[ updowni ] < size[ updowni ]; pf1[ updowni ]++ )
        if ( vol ( pf1 ) >= back ) {
            lrmin   = pf1;
            break;
            }


    for ( pf1[ leftrighti ] = size[ leftrighti ] - 1; pf1[ leftrighti ] >= 0 && lrmax == 0; pf1[ leftrighti ]-- )
    for ( pf1[ updowni ] = 0; pf1[ updowni ] < size[ updowni ]; pf1[ updowni ]++ )
        if ( vol ( pf1 ) >= back ) {
            lrmax   = pf1;
            break;
            }


    lrmax  -= lrmin;
    udmax  -= udmin;

    d1      = fabs ( ( lrmax[ leftrighti ] + 1 ) * ( udmax[ updowni ] + 1 ) );

//    DBGV ( d1, "box1" );

    stat1.Add ( d1 );
    }


                                        // scanning from the other front-back side
for ( pf1[ frontbacki ] = size[ frontbacki ] - 1; pf1[ frontbacki ] > center[ frontbacki ]; pf1[ frontbacki ]-- ) {

    udmin       = udmax     = lrmin     = lrmax     = 0.0;

                                        // scan from each side, until we reach the volume
    for ( pf1[ updowni ] = 0; pf1[ updowni ] < size[ updowni ] && udmin == 0; pf1[ updowni ]++ )
    for ( pf1[ leftrighti ] = 0; pf1[ leftrighti ] < size[ leftrighti ]; pf1[ leftrighti ]++ )
        if ( vol ( pf1 ) >= back ) {
            udmin   = pf1;
            break;
            }

    if ( udmin == 0 )
        continue;                       // no other sides to search!


    for ( pf1[ updowni ] = size[ updowni ] - 1; pf1[ updowni ] >= 0 && udmax == 0; pf1[ updowni ]-- )
    for ( pf1[ leftrighti ] = 0; pf1[ leftrighti ] < size[ leftrighti ]; pf1[ leftrighti ]++ )
        if ( vol ( pf1 ) >= back ) {
            udmax   = pf1;
            break;
            }

                                      // left right positions
    for ( pf1[ leftrighti ] = 0; pf1[ leftrighti ] < size[ leftrighti ] && lrmin == 0; pf1[ leftrighti ]++ )
    for ( pf1[ updowni ] = 0; pf1[ updowni ] < size[ updowni ]; pf1[ updowni ]++ )
        if ( vol ( pf1 ) >= back ) {
            lrmin   = pf1;
            break;
            }


    for ( pf1[ leftrighti ] = size[ leftrighti ] - 1; pf1[ leftrighti ] >= 0 && lrmax == 0; pf1[ leftrighti ]-- )
    for ( pf1[ updowni ] = 0; pf1[ updowni ] < size[ updowni ]; pf1[ updowni ]++ )
        if ( vol ( pf1 ) >= back ) {
            lrmax   = pf1;
            break;
            }


    lrmax  -= lrmin;
    udmax  -= udmin;

    d1      = fabs ( ( lrmax[ leftrighti ] + 1 ) * ( udmax[ updowni ] + 1 ) );

//    DBGV ( d1, "box2" );

    stat2.Add ( d1 );
    }

//DBGV3 ( stat1.Average (), stat1.SD (), stat1.CoefficientOfVariation (), "FB box1:  Avg SD Coeff" );
//DBGV3 ( stat2.Average (), stat2.SD (), stat2.CoefficientOfVariation (), "FB box2:  Avg SD Coeff" );

                                        // difference of sizes on each sides
double              fb31            = stat1.SD ();
double              fb32            = stat2.SD ();
                                        // normalize
sumv        = fb31 + fb32;
sumv        = NonNull ( sumv );
fb31       /= sumv;
fb32       /= sumv;

//DBGV2 ( fb31 * 100, fb32 * 100, "FB diameter variations" );


                                        // Then, we study the shape of the upper part of the brain: looking for asymmetries and slopes
int                 startud;
int                 maxud;
TPointFloat         fbminold[ 2 ];
TPointFloat         fbmaxold[ 2 ];

                                        // get the right side index of the top
if      ( XAxis == ToDown )     { startud = 0;        maxud = center.X + 1; }
else if ( XAxis == ToUp   )     { startud = center.X; maxud = vol.GetDim1 (); }
else if ( YAxis == ToDown )     { startud = 0;        maxud = center.Y + 1; }
else if ( YAxis == ToUp   )     { startud = center.Y; maxud = vol.GetDim2 (); }
else if ( ZAxis == ToDown )     { startud = 0;        maxud = center.Z + 1; }
else if ( ZAxis == ToUp   )     { startud = center.Z; maxud = vol.GetDim3 (); }


stat1.Reset ();
stat2.Reset ();
stat3.Reset ();

                                        // start scanning from the top
for ( pf1[ updowni ] = startud; pf1[ updowni ] < maxud; pf1[ updowni ]++ ) {

    fbmin       = fbmax     = lrmin     = lrmax     = 0.0;
    fbmini      = fbmaxi    = lrmini    = lrmaxi    = 0;

                                        // scan from each side, until we reach the volume, compute the average position (!)
    for ( pf1[ frontbacki ] = 0; pf1[ frontbacki ] < size[ frontbacki ]; pf1[ frontbacki ]++ ) {
        for ( pf1[ leftrighti ] = 0; pf1[ leftrighti ] < size[ leftrighti ]; pf1[ leftrighti ]++ )
            if ( vol ( pf1 ) >= back ) {
                fbmin  += pf1;
                fbmini++;
                }

        if ( fbmini ) {
            fbmin  /= fbmini;
            break;
            }
        }

    if ( fbmini == 0 )
        continue;                       // no other sides to search!


    for ( pf1[ frontbacki ] = size[ frontbacki ] - 1; pf1[ frontbacki ] >= 0; pf1[ frontbacki ]-- ) {
        for ( pf1[ leftrighti ] = 0; pf1[ leftrighti ] < size[ leftrighti ]; pf1[ leftrighti ]++ )
            if ( vol ( pf1 ) >= back ) {
                fbmax  += pf1;
                fbmaxi++;
                }

        if ( fbmaxi ) {
            fbmax  /= fbmaxi;
            break;
            }
        }

                                      // left right positions
    for ( pf1[ leftrighti ] = 0; pf1[ leftrighti ] < size[ leftrighti ]; pf1[ leftrighti ]++ ) {
        for ( pf1[ frontbacki ] = 0; pf1[ frontbacki ] < size[ frontbacki ]; pf1[ frontbacki ]++ )
            if ( vol ( pf1 ) >= back ) {
                lrmin  += pf1;
                lrmini++;
                }

        if ( lrmini ) {
            lrmin  /= lrmini;
            break;
            }
        }


    for ( pf1[ leftrighti ] = size[ leftrighti ] - 1; pf1[ leftrighti ] >= 0; pf1[ leftrighti ]-- ) {
        for ( pf1[ frontbacki ] = 0; pf1[ frontbacki ] < size[ frontbacki ]; pf1[ frontbacki ]++ )
            if ( vol ( pf1 ) >= back ) {
                lrmax  += pf1;
                lrmaxi++;
                }

        if ( lrmaxi ) {
            lrmax  /= lrmaxi;
            break;
            }
        }

                                        // average position of the sides
    lrmin[ frontbacki ] = lrmax[ frontbacki ] = ( lrmin[ frontbacki ] + lrmax[ frontbacki ] ) / 2;
    fbmin[ leftrighti ] = fbmax[ leftrighti ] = ( fbmin[ leftrighti ] + fbmax[ leftrighti ] ) / 2;


    d1      = fabs ( fbmin[ frontbacki ] - lrmin[ frontbacki ] );
    d2      = fabs ( fbmax[ frontbacki ] - lrmin[ frontbacki ] );

//    DBGV3 ( d1, d2, d1 / ( d1 + d2 - 1 ), "asymmetry" );

    if ( d1 + d2 != 0 )
        stat1.Add ( d1 / ( d1 + d2 ) );

/*
    if ( fbminold[ 1 ] != 0 ) {
        d1      = fabs ( fbmin[ frontbacki ] - fbminold[ 1 ][ frontbacki ] ) / 2;
        d2      = fabs ( fbmax[ frontbacki ] - fbmaxold[ 1 ][ frontbacki ] ) / 2;

        stat2.Add ( d1 );
        stat3.Add ( d2 );

//        DBGV2 ( d1, d2, "slope1 slope2" );
        }
*/

    fbminold[ 1 ]   = fbminold[ 0 ];
    fbminold[ 0 ]   = fbmin;
    fbmaxold[ 1 ]   = fbmaxold[ 0 ];
    fbmaxold[ 0 ]   = fbmax;
    }

//DBGV3 ( stat1.Average () * 100, stat1.SD (), stat1.CoefficientOfVariation (), "FB asymmetry:  Avg SD Coeff" );
//DBGV3 ( stat2.Average (), stat2.SD (), stat2.CoefficientOfVariation (), "FB slope1:  Avg SD Coeff" );
//DBGV3 ( stat3.Average (), stat3.SD (), stat3.CoefficientOfVariation (), "FB slope2:  Avg SD Coeff" );


                                        // retrieve a set of variables
                                        // asymmetry of length from each fron-back extremities, related to the orthogonal bissection
double              fb11            = 1 - stat1.Average ();
double              fb12            = stat1.Average ();

//DBGV2 ( fb11 * 100, fb12 * 100, "FB asymmetry" );

/*                                      // in case of high rotation, this behaves very badly, and overwhelmes the other criterion
                                        // average of the slopes on each side
double              fb21            = stat3.Average ();
double              fb22            = stat2.Average ();
                                        // normalize
sumv        = fb21 + fb22;
sumv        = NonNull ( sumv );
fb21       /= sumv;
fb22       /= sumv;

//DBGV2 ( fb21 * 100, fb22 * 100, "FB slopes" );
*/

                                        // combine criterion
double              back1;
double              back2;

//back1       = diameter[ 0 ] * fb11 * fb21 * fb31;
//back2       = diameter[ 1 ] * fb12 * fb22 * fb32;
back1       = diameter[ 0 ] * fb11 * fb31;
back2       = diameter[ 1 ] * fb12 * fb32;

                                        // normalize
sumv        = back1 + back2;
sumv        = NonNull ( sumv );

back1      /= sumv;
back2      /= sumv;

//DBGV2 ( back1 * 100, back2 * 100, "Front/Back" );


if      ( XAxis == FrontBack )  XAxis   = back1 < back2 ? ToBack : ToFront;
else if ( YAxis == FrontBack )  YAxis   = back1 < back2 ? ToBack : ToFront;
else if ( ZAxis == FrontBack )  ZAxis   = back1 < back2 ? ToBack : ToFront;


quality.Add ( max ( back1, back2 ) );

//DBGM3 ( OrientationLabels[ XAxis ], OrientationLabels[ YAxis ], OrientationLabels[ ZAxis ], "Geometry 5)" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int s = 0; s < NumCubeSides; s++ )
    if ( surfp[ s ] ) {
        delete  surfp[ s ];
        surfp[ s ]  = 0;
        }

                                        // finish the job with a vectorial product
CompleteOrientation ( XAxis, YAxis, ZAxis );

//DBGM3 ( OrientationLabels[XAxis], OrientationLabels[YAxis], OrientationLabels[ZAxis], "Geometry Orientation" );
//DBGV ( quality.Average () * 100, "Segmented Orientation Confidence" );

SetOrientation ( XAxis, YAxis, ZAxis );
}


//----------------------------------------------------------------------------
void    TVolumeDoc::FindOrientationFullHead ()
{
TEasyStats          stat1;
TEasyStats          stat2;
TEasyStats          stat3;
TEasyStats          quality;

TOrientationType    XAxis           = OrientationUnknown;
TOrientationType    YAxis           = OrientationUnknown;
TOrientationType    ZAxis           = OrientationUnknown;

Volume              vol;
TPointFloat         center;
double              back;
TPointInt           boundmin;
TPointInt           boundmax;
TPointInt           p1;
TPointInt           p2;
double              sumv;


FindOrientationCommon   (   vol, 
                            center, 
                            back, 
                            boundmin,   boundmax 
                        );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) find Left-Right axis

                                        // Compute the SD of the voxel differences between the supposed Left and Right
                                        // symetrically to the center
                                        // the lowest SD means the less differences between the two halves
                                        // which correspond to the left-right axis

stat1.Reset ();     stat2.Reset ();     stat3.Reset ();

for ( p1.Y = boundmin.Y; p1.Y <= boundmax.Y; p1.Y++ )
for ( p1.Z = boundmin.Z; p1.Z <= boundmax.Z; p1.Z++ )
for ( p1.X = center.X + 1, p2.X = center.X - 1; p1.X < boundmax.X && p2.X > boundmin.X; p1.X++, p2.X-- )
                                        // filter data & get L-R difference
    stat1.Add ( fabs ( 2 * vol ( p2.X, p1.Y, p1.Z ) + vol ( p2.X - 1, p1.Y, p1.Z ) + vol ( p2.X + 1, p1.Y, p1.Z )
                     - 2 * vol ( p1.X, p1.Y, p1.Z ) - vol ( p1.X - 1, p1.Y, p1.Z ) - vol ( p1.X + 1, p1.Y, p1.Z ) ) );


for ( p1.X = boundmin.X; p1.X <= boundmax.X; p1.X++ )
for ( p1.Z = boundmin.Z; p1.Z <= boundmax.Z; p1.Z++ )
for ( p1.Y = center.Y + 1, p2.Y = center.Y - 1; p1.Y < boundmax.Y && p2.Y > boundmin.Y; p1.Y++, p2.Y-- )

    stat2.Add ( fabs ( 2 * vol ( p1.X, p2.Y, p1.Z ) + vol ( p1.X, p2.Y - 1, p1.Z ) + vol ( p1.X, p2.Y + 1, p1.Z )
                     - 2 * vol ( p1.X, p1.Y, p1.Z ) - vol ( p1.X, p1.Y - 1, p1.Z ) - vol ( p1.X, p1.Y + 1, p1.Z ) ) );


for ( p1.X = boundmin.X; p1.X <= boundmax.X; p1.X++ )
for ( p1.Y = boundmin.Y; p1.Y <= boundmax.Y; p1.Y++ )
for ( p1.Z = center.Z + 1, p2.Z = center.Z - 1; p1.Z < boundmax.Z && p2.Z > boundmin.Z; p1.Z++, p2.Z-- )

    stat3.Add ( fabs ( 2 * vol ( p1.X, p1.Y, p2.Z ) + vol ( p1.X, p1.Y, p2.Z - 1 ) + vol ( p1.X, p1.Y, p2.Z + 1 )
                     - 2 * vol ( p1.X, p1.Y, p1.Z ) - vol ( p1.X, p1.Y, p1.Z - 1 ) - vol ( p1.X, p1.Y, p1.Z + 1 ) ) );


//DBGV3 ( fabs ( stat1.Average () ), stat1.SD (), fabs ( stat1.CoefficientOfVariation () ), "X  stat1: Avg SD Coeff" );
//DBGV3 ( fabs ( stat2.Average () ), stat2.SD (), fabs ( stat2.CoefficientOfVariation () ), "Y  stat2: Avg SD Coeff" );
//DBGV3 ( fabs ( stat3.Average () ), stat3.SD (), fabs ( stat3.CoefficientOfVariation () ), "Z  stat3: Avg SD Coeff" );


if ( stat1.IsEmpty () || stat2.IsEmpty () || stat3.IsEmpty () )
    return;


TVector< double >   symavg ( 3 );
symavg[ 0 ]     = 1 / NonNull ( stat1.Average () );
symavg[ 1 ]     = 1 / NonNull ( stat2.Average () );
symavg[ 2 ]     = 1 / NonNull ( stat3.Average () );
symavg.Normalize1 ();

//DBGV3 ( symavg[ 0 ] * 100, symavg[ 1 ] * 100, symavg[ 2 ] * 100, "Left-Right Avg" );


TVector< double >   symsd ( 3 );
symsd[ 0 ]      = 1 / NonNull ( stat1.SD () );
symsd[ 1 ]      = 1 / NonNull ( stat2.SD () );
symsd[ 2 ]      = 1 / NonNull ( stat3.SD () );
symsd.Normalize1 ();

//DBGV3 ( symsd[ 0 ] * 100, symsd[ 1 ] * 100, symsd[ 2 ] * 100, "Left-Right SD" );


                                        // Look for a central rift, at the place of corpus callosum
stat1.Reset ();     stat2.Reset ();     stat3.Reset ();
double              v;
double              v1;
double              v2;
double              v3;


for ( p1.Y = boundmin.Y; p1.Y <= boundmax.Y; p1.Y++ )
for ( p1.Z = boundmin.Z; p1.Z <= boundmax.Z; p1.Z++ ) {
    v1  = 2 * vol ( center.X, p1.Y, p1.Z ) - vol ( center.X - 1, p1.Y, p1.Z ) - vol ( center.X + 1, p1.Y, p1.Z );
    center.X--;
    v2  = 2 * vol ( center.X, p1.Y, p1.Z ) - vol ( center.X - 1, p1.Y, p1.Z ) - vol ( center.X + 1, p1.Y, p1.Z );
    center.X+=2;
    v3  = 2 * vol ( center.X, p1.Y, p1.Z ) - vol ( center.X - 1, p1.Y, p1.Z ) - vol ( center.X + 1, p1.Y, p1.Z );
    center.X--;

    v   = min ( v1, v2, v3 );

    if ( v < 0 )
        stat1.Add ( fabs ( v ) );
    }


for ( p1.X = boundmin.X; p1.X <= boundmax.X; p1.X++ )
for ( p1.Z = boundmin.Z; p1.Z <= boundmax.Z; p1.Z++ ) {
    v1  = 2 * vol ( p1.X, center.Y, p1.Z ) - vol ( p1.X, center.Y - 1, p1.Z ) - vol ( p1.X, center.Y + 1, p1.Z );
    center.Y--;
    v2  = 2 * vol ( p1.X, center.Y, p1.Z ) - vol ( p1.X, center.Y - 1, p1.Z ) - vol ( p1.X, center.Y + 1, p1.Z );
    center.Y+=2;
    v3  = 2 * vol ( p1.X, center.Y, p1.Z ) - vol ( p1.X, center.Y - 1, p1.Z ) - vol ( p1.X, center.Y + 1, p1.Z );
    center.Y--;

    v   = min ( v1, v2, v3 );

    if ( v < 0 )
        stat2.Add ( fabs ( v ) );
    }


for ( p1.X = boundmin.X; p1.X <= boundmax.X; p1.X++ )
for ( p1.Y = boundmin.Y; p1.Y <= boundmax.Y; p1.Y++ ) {
    v1  = 2 * vol ( p1.X, p1.Y, center.Z ) - vol ( p1.X, p1.Y, center.Z - 1 ) - vol ( p1.X, p1.Y, center.Z + 1 );
    center.Z--;
    v2  = 2 * vol ( p1.X, p1.Y, center.Z ) - vol ( p1.X, p1.Y, center.Z - 1 ) - vol ( p1.X, p1.Y, center.Z + 1 );
    center.Z+=2;
    v3  = 2 * vol ( p1.X, p1.Y, center.Z ) - vol ( p1.X, p1.Y, center.Z - 1 ) - vol ( p1.X, p1.Y, center.Z + 1 );
    center.Z--;

    v   = min ( v1, v2, v3 );

    if ( v < 0 )
        stat3.Add ( fabs ( v ) );
    }

//DBGV3 ( fabs ( stat1.Average () ), stat1.SD (), fabs ( stat1.CoefficientOfVariation () ), "X  stat1: Avg SD Coeff" );
//DBGV3 ( fabs ( stat2.Average () ), stat2.SD (), fabs ( stat2.CoefficientOfVariation () ), "Y  stat2: Avg SD Coeff" );
//DBGV3 ( fabs ( stat3.Average () ), stat3.SD (), fabs ( stat3.CoefficientOfVariation () ), "Z  stat3: Avg SD Coeff" );


TVector< double >   centralrift ( 3 );
centralrift[ 0 ]    = stat1.Average () * stat1.SD ();
centralrift[ 1 ]    = stat2.Average () * stat2.SD ();
centralrift[ 2 ]    = stat3.Average () * stat3.SD ();
centralrift.Normalize1 ();

//DBGV3 ( centralrift[ 0 ] * 100, centralrift[ 1 ] * 100, centralrift[ 2 ] * 100, "Left-Right Centralrift" );


                                        // cumulate data
TVector< double >   symetry ( 3 );
symetry ( 0 )    = symavg[ 0 ] * symsd[ 0 ] * centralrift[ 0 ];
symetry ( 1 )    = symavg[ 1 ] * symsd[ 1 ] * centralrift[ 1 ];
symetry ( 2 )    = symavg[ 2 ] * symsd[ 2 ] * centralrift[ 2 ];
symetry.Normalize1 ();

//DBGV3 ( symetry ( 0 ) * 100, symetry ( 1 ) * 100, symetry ( 2 ) * 100, "Left-Right ratios" );

                                        // choose the max, save the relative max to the second max
if ( symetry ( 0 ) > symetry ( 1 ) && symetry ( 0 ) > symetry ( 2 ) )   { XAxis   = LeftRight; quality.Add ( symetry ( 0 ) / ( symetry ( 0 ) + max ( symetry ( 1 ), symetry ( 2 ) ) ) ); }
if ( symetry ( 1 ) > symetry ( 0 ) && symetry ( 1 ) > symetry ( 2 ) )   { YAxis   = LeftRight; quality.Add ( symetry ( 1 ) / ( symetry ( 1 ) + max ( symetry ( 0 ), symetry ( 2 ) ) ) ); }
if ( symetry ( 2 ) > symetry ( 1 ) && symetry ( 2 ) > symetry ( 0 ) )   { ZAxis   = LeftRight; quality.Add ( symetry ( 2 ) / ( symetry ( 2 ) + max ( symetry ( 0 ), symetry ( 1 ) ) ) ); }


//DBGM3 ( OrientationLabels[ XAxis ], OrientationLabels[ YAxis ], OrientationLabels[ ZAxis ], "Geometry 1)" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) find Up-Down axis
                                        // Look at histogram of distances between successive surface points
TPoints*            surfp[ NumCubeSides ];

double              distpoints  [ NumCubeSides ][ 2 ];
TVector< double >   histodistratio ( 3 );
//TVector< double >   flatnessratio ( 3 );


for ( int s = 0; s < NumCubeSides; s++ )
    surfp[ s ]  = new TPoints;

                                        // get all safe surface points, from all sides of the cube
vol.GetSurfacePoints    (   surfp, 
                            back, 
                            TPointInt ( 1, 1, 1 ), 
                            center, 
                            1.10, 
                            (SurfacePointsFlags) ( SurfaceSafer | SurfaceInside ) 
                        );


//if ( VkQuery () ) {
//    surfp[ 0 ]->WriteFile ( "E:\\Data\\CubSide.XMin.spi" );
//    surfp[ 1 ]->WriteFile ( "E:\\Data\\CubSide.XMax.spi" );
//    surfp[ 2 ]->WriteFile ( "E:\\Data\\CubSide.YMin.spi" );
//    surfp[ 3 ]->WriteFile ( "E:\\Data\\CubSide.YMax.spi" );
//    surfp[ 4 ]->WriteFile ( "E:\\Data\\CubSide.ZMin.spi" );
//    surfp[ 5 ]->WriteFile ( "E:\\Data\\CubSide.ZMax.spi" );
//    }


THistogram          histod ( 101 );
TArray1<int>        topack;
TArray1<int>        tounpack;
TPointFloat         sp1;
TPointFloat         sp2;


for ( int s = 0; s < NumCubeSides; s++ ) {

    histod.Reset ();
    sp1   .Reset ();

    for ( int i = 0; i < surfp[ s ]->GetNumPoints (); i++ ) {

        sp2     = sp1;
        sp1     = (*surfp[ s ])[ i ];

        if ( i == 0 )
            continue;

        histod[ histod.ToBin ( BinUnit, ( sp2 - sp1 ).Norm () * 10 ) ]++;
        }

    histod.Pack ( topack, tounpack );
                                        // save first 2 values
    distpoints[ s ][ 0 ]    = histod[ 0 ];
    distpoints[ s ][ 1 ]    = histod[ 1 ];

//  TFileName           _file;
//  sprintf ( _file, "%s.Distance Histogram.%0d.sef", GetDocPath (), s + 1 );
//  histod.WriteFile ( _file, "CubeSide" );
    }


histodistratio[ 0 ]     = fabs ( ( distpoints[ 0 ][ 0 ] - distpoints[ 1 ][ 0 ] ) - ( distpoints[ 0 ][ 1 ] - distpoints[ 1 ][ 1 ] ) );
histodistratio[ 1 ]     = fabs ( ( distpoints[ 2 ][ 0 ] - distpoints[ 3 ][ 0 ] ) - ( distpoints[ 2 ][ 1 ] - distpoints[ 3 ][ 1 ] ) );
histodistratio[ 2 ]     = fabs ( ( distpoints[ 4 ][ 0 ] - distpoints[ 5 ][ 0 ] ) - ( distpoints[ 4 ][ 1 ] - distpoints[ 5 ][ 1 ] ) );
//histodistratio[ 0 ]     = RelativeDifference ( distpoints[ 0 ][ 0 ], distpoints[ 1 ][ 0 ] ) + RelativeDifference ( distpoints[ 0 ][ 1 ], distpoints[ 1 ][ 1 ] );
//histodistratio[ 1 ]     = RelativeDifference ( distpoints[ 2 ][ 0 ], distpoints[ 3 ][ 0 ] ) + RelativeDifference ( distpoints[ 2 ][ 1 ], distpoints[ 3 ][ 1 ] );
//histodistratio[ 2 ]     = RelativeDifference ( distpoints[ 4 ][ 0 ], distpoints[ 5 ][ 0 ] ) + RelativeDifference ( distpoints[ 4 ][ 1 ], distpoints[ 5 ][ 1 ] );

//DBGV3 ( histodistratio[ 0 ], histodistratio[ 1 ], histodistratio[ 2 ], "Histo distance X Y Z " );

histodistratio.Normalize1 ();

//DBGV3 ( histodistratio[ 0 ] * 100, histodistratio[ 1 ] * 100, histodistratio[ 2 ] * 100, "Histo distance ratio X Y Z " );


if ( histodistratio.IsNull () )
//  return;
    histodistratio = 1;

//DBGV3 ( histodistratio[ 0 ] * 100, histodistratio[ 1 ] * 100, histodistratio[ 2 ] * 100, "UpDown ratios X Y Z " );


if      ( XAxis )
    if ( histodistratio[ 1 ] > histodistratio[ 2 ] )    { YAxis   = UpDown; quality.Add ( histodistratio[ 1 ] / ( histodistratio[ 1 ] + histodistratio[ 2 ] ) ); }
    else                                                { ZAxis   = UpDown; quality.Add ( histodistratio[ 2 ] / ( histodistratio[ 1 ] + histodistratio[ 2 ] ) ); }
else if ( YAxis )
    if ( histodistratio[ 0 ] > histodistratio[ 2 ] )    { XAxis   = UpDown; quality.Add ( histodistratio[ 0 ] / ( histodistratio[ 0 ] + histodistratio[ 2 ] ) ); }
    else                                                { ZAxis   = UpDown; quality.Add ( histodistratio[ 2 ] / ( histodistratio[ 0 ] + histodistratio[ 2 ] ) ); }
else if ( ZAxis )
    if ( histodistratio[ 0 ] > histodistratio[ 1 ] )    { XAxis   = UpDown; quality.Add ( histodistratio[ 0 ] / ( histodistratio[ 0 ] + histodistratio[ 1 ] ) ); }
    else                                                { YAxis   = UpDown; quality.Add ( histodistratio[ 1 ] / ( histodistratio[ 0 ] + histodistratio[ 1 ] ) ); }


//DBGM3 ( OrientationLabels[ XAxis ], OrientationLabels[ YAxis ], OrientationLabels[ ZAxis ], "Geometry 2)" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 3) Up / Down direction
                                        // Look for the sign of the difference of histograms

                                        // get the up down index
int                 updowni         = XAxis == UpDown ? 0 : YAxis == UpDown ? 1 : 2;


histodistratio[ 0 ]     = ( distpoints[ 0 ][ 0 ] - distpoints[ 1 ][ 0 ] ) - ( distpoints[ 0 ][ 1 ] - distpoints[ 1 ][ 1 ] );
histodistratio[ 1 ]     = ( distpoints[ 2 ][ 0 ] - distpoints[ 3 ][ 0 ] ) - ( distpoints[ 2 ][ 1 ] - distpoints[ 3 ][ 1 ] );
histodistratio[ 2 ]     = ( distpoints[ 4 ][ 0 ] - distpoints[ 5 ][ 0 ] ) - ( distpoints[ 4 ][ 1 ] - distpoints[ 5 ][ 1 ] );

//DBGV ( histodistratio[ updowni ], "Up/Down sign" );


if      ( XAxis == UpDown )     XAxis = histodistratio[ updowni ] > 0 ? ToUp : ToDown;
else if ( YAxis == UpDown )     YAxis = histodistratio[ updowni ] > 0 ? ToUp : ToDown;
else if ( ZAxis == UpDown )     ZAxis = histodistratio[ updowni ] > 0 ? ToUp : ToDown;


//DBGM3 ( OrientationLabels[ XAxis ], OrientationLabels[ YAxis ], OrientationLabels[ ZAxis ], "Geometry 3)" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // 2) find Up-Down axis
                                        // Look at the ratio Spherical / Flatness on each side of the cube

TPoints*            surfp[ NumCubeSides ];

TListIterator< TPointFloat >    iterator;
//double              sphericalavg[ NumCubeSides ];
double              sphericalsd [ NumCubeSides ];
double              flatnessavg [ NumCubeSides ];
double              flatnesssd  [ NumCubeSides ];
double              updowncoeff [ NumCubeSides ];
double              updownavg   [ NumCubeSides ];
TVector< double >   sphericalratio ( 3 );
TVector< double >   flatnessratio  ( 3 );
TPointFloat         bettercenter;
TPointFloat         betterextrema;
TPointFloat         rescale;
TPointFloat         normp;


for ( int s = 0; s < NumCubeSides; s++ )
    surfp[ s ]  = new TPoints;

                                        // get all safe surface points, from all sides of the cube
vol.GetSurfacePoints    (   surfp, 
                            back, 
                            TPointInt ( 1, 1, 1 ), 
                            center, 
                            1.10,
                            (SurfacePointsFlags) ( SurfaceSafer | SurfaceInside ) 
                        );


//if ( VkQuery () ) {
//    surfp[ 0 ]->WriteFile ( "E:\\Data\\CubSide.XMin.spi" );
//    surfp[ 1 ]->WriteFile ( "E:\\Data\\CubSide.XMax.spi" );
//    surfp[ 2 ]->WriteFile ( "E:\\Data\\CubSide.YMin.spi" );
//    surfp[ 3 ]->WriteFile ( "E:\\Data\\CubSide.YMax.spi" );
//    surfp[ 4 ]->WriteFile ( "E:\\Data\\CubSide.ZMin.spi" );
//    surfp[ 5 ]->WriteFile ( "E:\\Data\\CubSide.ZMax.spi" );
//    }



for ( int s = 0; s < NumCubeSides; s++ ) {
                                        // be more precise: get the boundaries of the current side points
    TBoundingBox<double>    bounding ( *surfp[ s ] );

    bettercenter    = center;
    betterextrema   = center;
                                        // set the center to the correct extrema
    if      ( s == 0 )      bettercenter.X  = bounding.XMax ();
    else if ( s == 1 )      bettercenter.X  = bounding.XMin ();
    else if ( s == 2 )      bettercenter.Y  = bounding.YMax ();
    else if ( s == 3 )      bettercenter.Y  = bounding.YMin ();
    else if ( s == 4 )      bettercenter.Z  = bounding.ZMax ();
    else if ( s == 5 )      bettercenter.Z  = bounding.ZMin ();

    if      ( s == 0 )      betterextrema.X  = bounding.XMin ();
    else if ( s == 1 )      betterextrema.X  = bounding.XMax ();
    else if ( s == 2 )      betterextrema.Y  = bounding.YMin ();
    else if ( s == 3 )      betterextrema.Y  = bounding.YMax ();
    else if ( s == 4 )      betterextrema.Z  = bounding.ZMin ();
    else if ( s == 5 )      betterextrema.Z  = bounding.ZMax ();

                                        // we can also use the boundary for the rescaling, to look for a local ellipsoid
    rescale.Set ( bounding.GetXExtent (), bounding.GetYExtent (), bounding.GetZExtent () );
                                        // box is actually elongated
//  rescale.X      /= ( s / 2 == 0 ) ? 1 : 2;
//  rescale.Y      /= ( s / 2 == 1 ) ? 1 : 2;
//  rescale.Z      /= ( s / 2 == 2 ) ? 1 : 2;

                                        // look for the sphericality & flatness
    stat1.Reset ();
    stat2.Reset ();
    stat3.Reset ();

    foreachin ( *surfp[ s ], iterator ) {
                                        // point from center
        normp       = ( *iterator() - bettercenter ) / rescale;
                                        // norm expresses sphericality
        stat1.Add ( normp.Norm () );
                                        // distance indicates flatness
        stat2.Add ( fabs ( normp[ s / 2 ] ) );
                                        // point from border
        normp       = ( *iterator() - betterextrema ) / rescale;
                                        // this distance indicates better the flatness side
        stat3.Add ( fabs ( normp[ s / 2 ] ) );
        }

                                        // store these variables per side
//  sphericalavg[ s ]   = stat1.Average ();
    sphericalsd [ s ]   = stat1.SD ();
//  spherical  [ s ]    = stat1.Average () * stat1.SD ();

    flatnessavg[ s ]    = stat2.Average ();
    flatnesssd [ s ]    = stat2.SD ();
//  flatness   [ s ]    = stat2.Average () * stat2.SD ();

    updowncoeff[ s ]    = 1 / NonNull ( stat3.CoefficientOfVariation () );
    updownavg  [ s ]    = 1 / NonNull ( stat1.Average () );


//    DBGV4 ( s + 1, fabs ( stat1.Average () ), stat1.SD (), fabs ( stat1.CoefficientOfVariation () ), "side  stat1: Avg SD Coeff" );
//    DBGV4 ( s + 1, fabs ( stat2.Average () ), stat2.SD (), fabs ( stat2.CoefficientOfVariation () ), "side  stat2: Avg SD Coeff" );
//    DBGV4 ( s + 1, fabs ( stat3.Average () ), stat3.SD (), fabs ( stat3.CoefficientOfVariation () ), "side  stat3: Avg SD Coeff" );
//    DBGV4 ( s + 1, sphericalsd[ s ], flatness[ s ], updowncoeff[ s ], "side  SphericalSD  FlatnessSD  UpDown" );
    }

                                        // spherical criterion: relative difference between each directions
sphericalratio[ 0 ]     = RelativeDifference ( sphericalsd[ 0 ], sphericalsd[ 1 ] );
sphericalratio[ 1 ]     = RelativeDifference ( sphericalsd[ 2 ], sphericalsd[ 3 ] );
sphericalratio[ 2 ]     = RelativeDifference ( sphericalsd[ 4 ], sphericalsd[ 5 ] );

//DBGV3 ( sphericalratio[ 0 ] * 1000, sphericalratio[ 1 ] * 1000, sphericalratio[ 2 ] * 1000, "Spherical ratio X Y Z " );

                                        // flatness: absolute difference of the SDs and Averages
flatnessratio[ 0 ]      = fabs ( ( flatnessavg[ 0 ] - flatnessavg[ 1 ] ) * ( flatnesssd[ 0 ] - flatnesssd[ 1 ] ) );
flatnessratio[ 1 ]      = fabs ( ( flatnessavg[ 2 ] - flatnessavg[ 3 ] ) * ( flatnesssd[ 2 ] - flatnesssd[ 3 ] ) );
flatnessratio[ 2 ]      = fabs ( ( flatnessavg[ 4 ] - flatnessavg[ 5 ] ) * ( flatnesssd[ 4 ] - flatnesssd[ 5 ] ) );

//DBGV3 ( flatnessratio[ 0 ] * 1000, flatnessratio[ 1 ] * 1000, flatnessratio[ 2 ] * 1000, "Flatness ratio X Y Z " );

                                        // combine spherical & flatness together
sphericalratio     *= flatnessratio;
//DBGV3 ( sphericalratio[ 0 ], sphericalratio[ 1 ], sphericalratio[ 2 ], "Spherical * Flatness X Y Z " );

sphericalratio.Normalize1 ();
//DBGV3 ( sphericalratio[ 0 ] * 100, sphericalratio[ 1 ] * 100, sphericalratio[ 2 ] * 100, "Spherical*Flatness ratio X Y Z " );


if ( sphericalratio == 0 )
    sphericalratio  = 1;

                                        // add information from the Left-Right criterion: the Up-Down is the less symetric
sphericalratio[ 0 ]    /= NonNull ( symetry ( 0 ) );
sphericalratio[ 1 ]    /= NonNull ( symetry ( 1 ) );
sphericalratio[ 2 ]    /= NonNull ( symetry ( 2 ) );

                                        // normalize
sphericalratio.Normalize1 ();
//DBGV3 ( sphericalratio[ 0 ] * 100, sphericalratio[ 1 ] * 100, sphericalratio[ 2 ] * 100, "UpDown ratios X Y Z " );


if ( sphericalratio == 0 )
    return;

if      ( XAxis )
    if ( sphericalratio[ 1 ] > sphericalratio[ 2 ] )    { YAxis   = UpDown; quality.Add ( sphericalratio[ 1 ] / ( sphericalratio[ 1 ] + sphericalratio[ 2 ] ) ); }
    else                                                { ZAxis   = UpDown; quality.Add ( sphericalratio[ 2 ] / ( sphericalratio[ 1 ] + sphericalratio[ 2 ] ) ); }
else if ( YAxis )
    if ( sphericalratio[ 0 ] > sphericalratio[ 2 ] )    { XAxis   = UpDown; quality.Add ( sphericalratio[ 0 ] / ( sphericalratio[ 0 ] + sphericalratio[ 2 ] ) ); }
    else                                                { ZAxis   = UpDown; quality.Add ( sphericalratio[ 2 ] / ( sphericalratio[ 0 ] + sphericalratio[ 2 ] ) ); }
else if ( ZAxis )
    if ( sphericalratio[ 0 ] > sphericalratio[ 1 ] )    { XAxis   = UpDown; quality.Add ( sphericalratio[ 0 ] / ( sphericalratio[ 0 ] + sphericalratio[ 1 ] ) ); }
    else                                                { YAxis   = UpDown; quality.Add ( sphericalratio[ 1 ] / ( sphericalratio[ 0 ] + sphericalratio[ 1 ] ) ); }


//DBGM3 ( OrientationLabels[ XAxis ], OrientationLabels[ YAxis ], OrientationLabels[ ZAxis ], "Geometry 2)" );
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // 3) Up / Down direction
                                        // Look for a max for the inferior direction

                                        // get the up down indexes
int                 updowni1        = XAxis == UpDown ? 0 : YAxis == UpDown ? 2 : 4;
int                 updowni2        = updowni1 + 1;

                                        // no needs for normalization

                                        // normalize
//sumv        = updowncoeff[ updowni1 ] + updowncoeff[ updowni2 ];
//sumv        = NonNull ( sumv );
//updowncoeff[ updowni1 ]    /= sumv;
//updowncoeff[ updowni2 ]    /= sumv;
//DBGV2 ( updowncoeff[ updowni1 ] * 100, updowncoeff[ updowni2 ] * 100, "Up/Down Coeff" );

                                        // normalize
//sumv        = updownavg[ updowni1 ] + updownavg[ updowni2 ];
//sumv        = NonNull ( sumv );
//updownavg[ updowni1 ]      /= sumv;
//updownavg[ updowni2 ]      /= sumv;
//DBGV2 ( updownavg[ updowni1 ] * 100, updownavg[ updowni2 ] * 100, "Up/Down Avg" );


                                        // combine criterion
updowncoeff[ updowni1 ]    *= updownavg[ updowni1 ];
updowncoeff[ updowni2 ]    *= updownavg[ updowni2 ];


                                        // normalize
sumv        = updowncoeff[ updowni1 ] + updowncoeff[ updowni2 ];
sumv        = NonNull ( sumv );
updowncoeff[ updowni1 ]    /= sumv;
updowncoeff[ updowni2 ]    /= sumv;

//DBGV2 ( updowncoeff[ updowni1 ] * 100, updowncoeff[ updowni2 ] * 100, "Up/Down ratios" );


if      ( XAxis == UpDown )     XAxis = updowncoeff[ updowni1 ] > updowncoeff[ updowni2 ] ? ToDown : ToUp;
else if ( YAxis == UpDown )     YAxis = updowncoeff[ updowni1 ] > updowncoeff[ updowni2 ] ? ToDown : ToUp;
else if ( ZAxis == UpDown )     ZAxis = updowncoeff[ updowni1 ] > updowncoeff[ updowni2 ] ? ToDown : ToUp;


quality.Add ( max ( updowncoeff[ updowni1 ], updowncoeff[ updowni2 ] ) );

//DBGM3 ( OrientationLabels[ XAxis ], OrientationLabels[ YAxis ], OrientationLabels[ ZAxis ], "Geometry 3)" );
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 4) Front-Back axis axis is the remaining one

if      ( XAxis && YAxis )  ZAxis = FrontBack;
else if ( XAxis && ZAxis )  YAxis = FrontBack;
else if ( YAxis && ZAxis )  XAxis = FrontBack;


//DBGM3 ( OrientationLabels[ XAxis ], OrientationLabels[ YAxis ], OrientationLabels[ ZAxis ], "Geometry 4)" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                                        // 5) Front / Back

                                        // get the front back indexes
int                 frontback1      = XAxis == FrontBack ? 0 : YAxis == FrontBack ? 2 : 4;
int                 frontback2      = frontback1 + 1;
double              front1;
double              front2;
TPointFloat         pf1;
TPointFloat         pf2;
double              d12;
TPointFloat         normp;

/*                                      // from previous computation, a quite weak factor, but still...
                                        // normalize
sumv        = updownavg[ frontback1 ] + updownavg[ frontback2 ];
sumv        = NonNull ( sumv );
updownavg[ frontback1 ]    /= sumv;
updownavg[ frontback2 ]    /= sumv;

//DBGV2 ( updownavg[ frontback1 ] * 100, updownavg[ frontback2 ] * 100, "Front/Back updownavg" );
*/

                                        // re-do the surface, with different parameters
for ( int s = 0; s < NumCubeSides; s++ )
    if ( surfp[ s ] ) {
        delete  surfp[ s ];
        surfp[ s ]  = 0;
        }

surfp[ frontback1 ] = new TPoints;
surfp[ frontback2 ] = new TPoints;
//TPoints             frontbackpoints;


vol.GetSurfacePoints    (   surfp, 
                            back, 
                            TPointInt ( 1, 1, 1 ), 
                            center, 
                            1.00, 
                            SurfaceInside 
                        );

//vol.GetSurfacePoints    (   surfp, 
//                            back, 
//                            vol.Steps ( 128 ),    // vol is already a downsampled version
//                            center, 
//                            1.00, 
//                            SurfaceInside 
//                        );


//surfp[ frontback1 ]->WriteFile ( "E:\\Data\\CubSide.Front1.spi" );
//surfp[ frontback2 ]->WriteFile ( "E:\\Data\\CubSide.Front2.spi" );



stat1.Reset ();
stat2.Reset ();
                                        // scan for all pairs of neighbors
for ( int i = 0; i < surfp[ frontback1 ]->GetNumPoints (); i++ ) {

    pf1         = (*surfp[ frontback1 ])[ i ];

    for ( int j = i + 1; j < surfp[ frontback1 ]->GetNumPoints (); j++ ) {

        pf2         = (*surfp[ frontback1 ])[ j ];
        normp       = pf2 - pf1;
        d12         = fabs ( normp[ frontback1 / 2 ] );

        normp[ frontback1 / 2 ] = 0;
                                        // not a neighbor or too far in depth?
        if ( normp.Norm2 () == 0 || normp.Norm2 () > 1.5 || d12 > 5 ) {
            pf1         = pf2;
            continue;
            }

        stat1.Add ( d12 );

//      frontbackpoints.AddNoDuplicates ( pf1 );
//      frontbackpoints.AddNoDuplicates ( pf2 );
        }
    }


for ( int i = 0; i < surfp[ frontback2 ]->GetNumPoints (); i++ ) {

    pf1         = (*surfp[ frontback2 ])[ i ];

    for ( int j = i + 1; j < surfp[ frontback2 ]->GetNumPoints (); j++ ) {

        pf2         = (*surfp[ frontback2 ])[ j ];
        normp       = pf2 - pf1;
        d12         = fabs ( normp[ frontback1 / 2 ] );

        normp[ frontback1 / 2 ] = 0;
                                        // not a neighbor or too far in depth?
        if ( normp.Norm2 () == 0 || normp.Norm2 () > 1.5 || d12 > 5 ) {
            pf1         = pf2;
            continue;
            }

        stat2.Add ( d12 );

//      frontbackpoints.AddNoDuplicates ( pf1 );
//      frontbackpoints.AddNoDuplicates ( pf2 );
        }
    }

//frontbackpoints.WriteFile ( "E:\\Data\\CubSide.Front12.spi" );
//DBGV3 ( stat1.Average (), stat1.SD (), stat1.CoefficientOfVariation (), "Front1:  Avg SD Coeff" );
//DBGV3 ( stat2.Average (), stat2.SD (), stat2.CoefficientOfVariation (), "Front2:  Avg SD Coeff" );

                                        // retrieve a set of variables
double              fb11            = stat1.Average ();
double              fb12            = stat2.Average ();
                                        // normalize
sumv        = fb11 + fb12;
sumv        = NonNull ( sumv );
fb11       /= sumv;
fb12       /= sumv;
//DBGV2 ( fb11 * 100, fb12 * 100, "Front/Back ratios1" );


double              fb21            = stat2.CoefficientOfVariation (); // ! refer to the opposite side !
double              fb22            = stat1.CoefficientOfVariation ();
                                        // normalize
sumv        = fb21 + fb22;
sumv        = NonNull ( sumv );
fb21       /= sumv;
fb22       /= sumv;
//DBGV2 ( fb21 * 100, fb22 * 100, "Front/Back ratios2" );


double              fb31            = stat1.SD ();
double              fb32            = stat2.SD ();
                                        // normalize
sumv        = fb31 + fb32;
sumv        = NonNull ( sumv );
fb31       /= sumv;
fb32       /= sumv;
//DBGV2 ( fb31 * 100, fb32 * 100, "Front/Back ratios3" );


                                        // combine + incorporate from previous computation
//front1      = fb11 * fb21 * fb31 * updownavg[ frontback2 ];
//front2      = fb12 * fb22 * fb32 * updownavg[ frontback1 ];
front1      = fb11 * fb21 * fb31;
front2      = fb12 * fb22 * fb32;

                                        // normalize
sumv        = front1 + front2;
sumv        = NonNull ( sumv );
front1     /= sumv;
front2     /= sumv;
//DBGV2 ( front1 * 100, front2 * 100, "Front/Back ratios" );


if      ( XAxis == FrontBack )  XAxis   = front1 > front2 ? ToBack : ToFront;
else if ( YAxis == FrontBack )  YAxis   = front1 > front2 ? ToBack : ToFront;
else if ( ZAxis == FrontBack )  ZAxis   = front1 > front2 ? ToBack : ToFront;


quality.Add ( max ( front1, front2 ) );

//DBGM3 ( OrientationLabels[ XAxis ], OrientationLabels[ YAxis ], OrientationLabels[ ZAxis ], "Geometry 5)" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int s = 0; s < NumCubeSides; s++ )
    if ( surfp[ s ] ) {
        delete  surfp[ s ];
        surfp[ s ]  = 0;
        }

                                        // finish the job with a vectorial product
CompleteOrientation ( XAxis, YAxis, ZAxis );

//DBGM3 ( OrientationLabels[XAxis], OrientationLabels[YAxis], OrientationLabels[ZAxis], "Geometry Orientation" );
//DBGV ( quality.Average () * 100, "Full Head Orientation Confidence" );

SetOrientation ( XAxis, YAxis, ZAxis );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
