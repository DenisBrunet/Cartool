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
                                        // All original defines for each file type
#include    "Files.Format.Analyze7.5.h"
#include    "Files.Format.nifti1.h"
#include    "Files.Format.nifti2.h"

#include    "Strings.Utils.h"
#include    "Files.TFileName.h"
#include    "Dialogs.Input.h"
#include    "Math.Utils.h"
                                        
//BeginBytePacking                      // not needed

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Tools to handle both Analyze and Nifti files

// Very useful website:
// https://brainder.org/2012/09/23/the-nifti-file-format/
// https://brainder.org/2015/04/03/the-nifti-2-file-format/


#define     ANALYZE_HEADER_SIZE     348
#define     NIFTI1_HEADER_SIZE      348
#define     NIFTI2_HEADER_SIZE      540

                                        // When applying method 2 or 3, orientation should end up with this
#define     NiftiOrientation        "RAS"

                                        // Default value, which will force the use of Method 3 / Affine transform
#define     NiftiTransformDefault   NIFTI_XFORM_ALIGNED_ANAT

                                        // Intent Code and Name is not as important as the Transform
#define     NiftiIntentCodeDefault  NIFTI_INTENT_NONE
#define     NiftiIntentCodeRis      NIFTI_INTENT_ESTIMATE
#define     NiftiIntentCodeGreyMask NIFTI_INTENT_LABEL
#define     NiftiIntentCodeLabels   NIFTI_INTENT_LABEL
#define     NiftiIntentCodeSynth    NIFTI_INTENT_LABEL

                                        // Specific Cartool Intent Names
#define     NiftiIntentNameDefault  ""
#define     NiftiIntentNameRis      "RIS"
#define     NiftiIntentNameBrain    "Brain"
#define     NiftiIntentNameGreyMask "Grey Mask"
#define     NiftiIntentNameSkull    "Skull"
#define     NiftiIntentNameLabels   "Labels"
//#define   NiftiIntentNameTemplate "Template"

#define     NiftiIntentNameSize     16

                                        // Cartool special string to specify up to ~15 pairs of value-label
#define     NiftiDescripLabels      "Labels:"


//----------------------------------------------------------------------------

inline const char*  AnalyzeDataTypeToString ( int dt );


inline  bool        IsAnalyzeTypeUnknown    ( int dt )   {   return  dt == DT_UNKNOWN;          }
inline  bool        IsAnalyzeTypeBinary     ( int dt )   {   return  dt == DT_BINARY;           }
inline  bool        IsAnalyzeTypeInteger    ( int dt )   {   return  dt == DT_UNSIGNED_CHAR   
                                                                  || dt == DT_UNSIGNED_SHORT
                                                                  || dt == DT_SIGNED_SHORT    
                                                                  || dt == DT_SIGNED_SHORT_POS
                                                                  || dt == DT_SIGNED_INT;       }
inline  bool        IsAnalyzeTypeFloat      ( int dt )   {   return  dt == DT_FLOAT 
                                                                  || dt == DT_DOUBLE;           }
inline  bool        IsAnalyzeTypeComplex    ( int dt )   {   return  dt == DT_COMPLEX;          }


//----------------------------------------------------------------------------
                                        // Why no defines for these 2 guys?
#define NIFTI_TYPE_UNKNOWN          DT_UNKNOWN
#define NIFTI_TYPE_BINARY           DT_BINARY


inline const char*  NiftiDataTypeToString   ( int dt );


inline  bool        IsNiftiTypeUnknown      ( int dt )   {   return  dt == NIFTI_TYPE_UNKNOWN;      }
inline  bool        IsNiftiTypeBinary       ( int dt )   {   return  dt == NIFTI_TYPE_BINARY;       }
inline  bool        IsNiftiTypeInteger      ( int dt )   {   return  dt == NIFTI_TYPE_UINT8
                                                                  || dt == NIFTI_TYPE_INT8
                                                                  || dt == NIFTI_TYPE_UINT16
                                                                  || dt == NIFTI_TYPE_INT16
                                                                  || dt == NIFTI_TYPE_UINT32
                                                                  || dt == NIFTI_TYPE_INT32
                                                                  || dt == NIFTI_TYPE_UINT64
                                                                  || dt == NIFTI_TYPE_INT64;        }
inline  bool        IsNiftiTypeFloat        ( int dt )   {   return  dt == NIFTI_TYPE_FLOAT32
                                                                  || dt == NIFTI_TYPE_FLOAT64
                                                                  || dt == NIFTI_TYPE_FLOAT128;     }
inline  bool        IsNiftiTypeComplex      ( int dt )   {   return  dt == NIFTI_TYPE_COMPLEX64
                                                                  || dt == NIFTI_TYPE_COMPLEX128
                                                                  || dt == NIFTI_TYPE_COMPLEX256;   }
inline  bool        IsNiftiTypeRGB          ( int dt )   {   return  dt == NIFTI_TYPE_RGB24
                                                                  || dt == NIFTI_TYPE_RGBA32;       }

inline const char*  NiftiSpatialUnitsToString  ( int u );
inline const char*  NiftiTemporalUnitsToString ( int u );
                                        // Applies to either qform_code or sform_code
                                        // Known codes as per 2019-03-08
inline const char*  NiftiTransformToString ( int qs );


//----------------------------------------------------------------------------

enum                NiftiTransformMethod
                    {
                    UnknwonNiftiMethod,
                    NiftiMethod1,       // "Old way" with a simple voxel rescaling - this is supposed to be a backup method and not the preferred one
                    NiftiMethod2,       // Using the Q (quaternion) rotation + translation transform
                    NiftiMethod3,       // Using the S (standardized) 3x4 transform matrix
                    NumNiftiMethods
                    };


inline const char*  NiftiTransformMethodString      ( NiftiTransformMethod niftimethod );
inline const char*  NiftiTransformMethodShortString ( NiftiTransformMethod niftimethod );

                                        // Do some gymnastic between the 2 codes, and returns what it thinks is the best transform to use
inline NiftiTransformMethod     GetNiftiTransformMethod (   int     qform_code,
                                                            int     sform_code,
                                                            bool    askuser     = false  );


//----------------------------------------------------------------------------

inline int          GuessNiftiIntentFromFilename    ( const char *filename );


//----------------------------------------------------------------------------

enum                AnalyzeFamilyType
                    {
                    UnknwonAnalyze          = 0x0000,

                    Analyze75               = 0x0001,
                    Nifti1                  = 0x0002,
                    Nifti2                  = 0x0004,
                    TypeMask                = Analyze75 | Nifti1 | Nifti2,

                    SingleFile              = 0x0010,
                    DualFiles               = 0x0020,
                    FileMask                = SingleFile | DualFiles,

                    SwapData                = 0x0100,
                    OtherMask               = SwapData,
                    };


inline  bool        IsAnalyze75 ( AnalyzeFamilyType type )  { return  type & Analyze75;                         }
inline  bool        IsNifti1    ( AnalyzeFamilyType type )  { return  type & Nifti1;                            }
inline  bool        IsNifti2    ( AnalyzeFamilyType type )  { return  type & Nifti2;                            }
inline  bool        IsNifti     ( AnalyzeFamilyType type )  { return  IsNifti1 ( type ) || IsNifti2 ( type );   }


//----------------------------------------------------------------------------
                                        // Given a supposed header file, will do all the nitty-gritty work to sort out all Analyze / Nifti combinations
inline AnalyzeFamilyType    GetAnalyzeType  (   const char*     file,               // whatever file we wish to know the structure
                                                char*           filehdr     = 0,    // complimentary resolved header file name
                                                char*           fileimg     = 0     // complimentary resolved image  file name
                                            );


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------

const char*     AnalyzeDataTypeToString ( int dt )
{
switch ( dt ) {
                                        // not ordered by values, but by family types
    case    DT_UNKNOWN              : return "Unknown Analyze Data Type";

    case    DT_BINARY               : return "Binary 1 bit";

    case    DT_UNSIGNED_CHAR        : return "Unsigned Byte";
    case    DT_UNSIGNED_SHORT       : return "Unsigned Short";
    case    DT_SIGNED_SHORT         : return "Signed Short";
    case    DT_SIGNED_SHORT_POS     : return "Signed Short";
    case    DT_SIGNED_INT           : return "Signed Int";

    case    DT_FLOAT                : return "Single Float";
    case    DT_DOUBLE               : return "Double Float";

    case    DT_COMPLEX              : return "Single Float Complex";
    default                         : return "Unrecognized Analyze Data Type";
    }
}


//----------------------------------------------------------------------------

const char*     NiftiDataTypeToString   ( int dt )
{
switch ( dt ) {
                                        // not ordered by values, but by family types
    case    NIFTI_TYPE_UNKNOWN      : return "Unknown Nifti Data Type";

    case    NIFTI_TYPE_BINARY       : return "Binary 1 bit";

    case    NIFTI_TYPE_UINT8        : return "Unsigned Byte";
    case    NIFTI_TYPE_INT8         : return "Signed Byte";
    case    NIFTI_TYPE_UINT16       : return "Unsigned Short";
    case    NIFTI_TYPE_INT16        : return "Signed Short";
    case    NIFTI_TYPE_UINT32       : return "Unsigned Int";
    case    NIFTI_TYPE_INT32        : return "Signed Int";
    case    NIFTI_TYPE_UINT64       : return "Unsigned Long Long";
    case    NIFTI_TYPE_INT64        : return "Signed Long Long";

    case    NIFTI_TYPE_FLOAT32      : return "Single Float";
    case    NIFTI_TYPE_FLOAT64      : return "Double Float";
    case    NIFTI_TYPE_FLOAT128     : return "Long Double Float";

    case    NIFTI_TYPE_COMPLEX64    : return "Single Float Complex";
    case    NIFTI_TYPE_COMPLEX128   : return "Double Float Complex";
    case    NIFTI_TYPE_COMPLEX256   : return "Long Double Float Complex";

    case    NIFTI_TYPE_RGB24        : return "RGB Bytes";
    case    NIFTI_TYPE_RGBA32       : return "RGBA Bytes";
    default                         : return "Unrecognized Nifti Data Type";
    }
}


const char*     NiftiSpatialUnitsToString ( int u )
{
switch ( XYZT_TO_SPACE ( u ) ) {
    case    NIFTI_UNITS_UNKNOWN     : return "No Nifti Spatial Unit";
    case    NIFTI_UNITS_METER       : return "[meter]";
    case    NIFTI_UNITS_MM          : return "[millimeter]";
    case    NIFTI_UNITS_MICRON      : return "[micrometer]";
    default                         : return "Unknown Nifti Spatial Unit";
    }
}


const char*     NiftiTemporalUnitsToString ( int u )
{
switch ( XYZT_TO_TIME ( u ) ) {
    case    NIFTI_UNITS_UNKNOWN     : return "No Nifti Temporal Unit";
    case    NIFTI_UNITS_SEC         : return "[second]";
    case    NIFTI_UNITS_MSEC        : return "[millisecond]";
    case    NIFTI_UNITS_USEC        : return "[microsecond]";
    case    NIFTI_UNITS_HZ          : return "[Herz]";
    case    NIFTI_UNITS_PPM         : return "[ppm]";
    case    NIFTI_UNITS_RADS        : return "[radian/second]";
    default                         : return "Unknown Nifti Temporal Unit";
    }
}


const char*     NiftiTransformToString ( int qs )
{
switch ( qs ) {
    case    NIFTI_XFORM_UNKNOWN         : return "Arbitrary Transform (0)";
    case    NIFTI_XFORM_SCANNER_ANAT    : return "Scanner-based coordinates (1)";
    case    NIFTI_XFORM_ALIGNED_ANAT    : return "Re-aligned coordinates (2)";
    case    NIFTI_XFORM_TALAIRACH       : return "Talairach coordinates (3)";
    case    NIFTI_XFORM_MNI_152         : return "MNI coordinates (4)";
    case    NIFTI_XFORM_TEMPLATE_OTHER  : return "Normalized coordinates (5)";
    default                             : return "Unknown Nifti Transform";
    }
}


//----------------------------------------------------------------------------
const char*     NiftiTransformMethodString  ( NiftiTransformMethod niftimethod )
{
switch ( niftimethod ) {

    case    UnknwonNiftiMethod: return  "Unknown transform method";                                                 break;
    case    NiftiMethod1:       return  "Nifti Method 1: Voxel transform (scaling)";                                break;
    case    NiftiMethod2:       return  "Nifti Method 2: Quaternion transform (scaling + rotation + translation)";  break;
    case    NiftiMethod3:       return  "Nifti Method 3: Standardization transform (3x4 transform matrix)";         break;
    default :                   return  "Unknown transform method";                                                 break;
    };
}

const char*     NiftiTransformMethodShortString ( NiftiTransformMethod niftimethod )
{
switch ( niftimethod ) {

    case    UnknwonNiftiMethod: return  "Unknown method";   break;
    case    NiftiMethod1:       return  "Nifti Method 1";   break;
    case    NiftiMethod2:       return  "Nifti Method 2";   break;
    case    NiftiMethod3:       return  "Nifti Method 3";   break;
    default :                   return  "Unknown method";   break;
    };
}


//----------------------------------------------------------------------------
                                        // Do some gymnastic between the 2 codes, and returns what it thinks is the best transform to use
NiftiTransformMethod    GetNiftiTransformMethod (   int     qform_code,
                                                    int     sform_code,
                                                    bool    askuser     )
{
if ( askuser ) {
                                        // Ask first for the type of transform - could check the feasable one first...
    char                answer          = GetOptionFromUser ( "Which Nifti transform method to use: \n\t0) (N)one \n\t1) (V)oxel \n\t2) (Q)uaternion \n\t3) (S)tandardization \n\t4) (A)uto", 
                                                              "Nifti Transform", "0 N 1 V 2 Q 3 S 4 A", "A" );

    //if ( answer == EOS )   return  UnknwonNiftiMethod;


    if      ( answer == '0' 
           || answer == 'N' )   return  UnknwonNiftiMethod;
    else if ( answer == '1' 
           || answer == 'V' )   return  NiftiMethod1;
    else if ( answer == '2' 
           || answer == 'Q' )   return  ( qform_code != NIFTI_XFORM_UNKNOWN ? NiftiMethod2 : sform_code != NIFTI_XFORM_UNKNOWN ? NiftiMethod3 : NiftiMethod1 );
    else if ( answer == '3' 
           || answer == 'S' )   return  ( sform_code != NIFTI_XFORM_UNKNOWN ? NiftiMethod3 : qform_code != NIFTI_XFORM_UNKNOWN ? NiftiMethod2 : NiftiMethod1 );
    // else automatic choice below
    }


if      ( qform_code == NIFTI_XFORM_UNKNOWN
       && sform_code == NIFTI_XFORM_UNKNOWN )
                                        // No transform available, using the back-up voxel transform
    return  NiftiMethod1;
                                        // Here, at least one transform exists
else if ( qform_code == sform_code )    
                                        // Both transforms say they do the same thing, pick S
    return  NiftiMethod3;
                                        // Here transforms differ
else if ( qform_code == NIFTI_XFORM_UNKNOWN )
                                        // Q is null, use S
    return  NiftiMethod3;
                                        // Here Q not null
else if ( sform_code == NIFTI_XFORM_UNKNOWN )
                                        // S is null, use Q
    return  NiftiMethod2;
                                        // Here transforms differ and are not null
else {
                                        // This is now the tricky part, we have to choose between 2 transforms
    if ( sform_code >= qform_code )
                                        // Currently favor the one transform with "higher meaning", that is MNI > Talairach > Scanner
        return  NiftiMethod3;
    else
        return  NiftiMethod2;
    }
}


//----------------------------------------------------------------------------

int                 GuessNiftiIntentFromFilename    ( const char *filename )
{
if      ( StringContains ( filename, "." InfixP       "."                   ) ) return  NIFTI_INTENT_PVAL;
else if ( StringContains ( filename, "." Infix1MinusP "."                   ) ) return  NIFTI_INTENT_PVAL;      // 1-p doesn't seem to exist
else if ( StringContains ( filename, "." InfixLogP    "."                   ) ) return  NIFTI_INTENT_LOG10PVAL;
else if ( StringContains ( filename, "." InfixT       "."                   ) ) return  NIFTI_INTENT_TTEST;
//elseif( StringContains ( filename, "." InfixRank    "."                   ) ) return  NIFTI_INTENT_UNIFORM;
//elseif( StringContains ( filename, "." InfixRoi     "."                   ) ) return  NIFTI_INTENT_LABEL;
else if ( StringContains ( filename, "." PostfixStandardizationZScore       ) ) return  NIFTI_INTENT_ZSCORE;    // statistical tests are done AFTER Z-Scoring
else if ( IsExtension    ( filename, FILEEXT_RIS                            ) ) return  NIFTI_INTENT_ESTIMATE;  // default for ris is a sort of estimate
else                                                                            return  NIFTI_INTENT_NONE;
}


//----------------------------------------------------------------------------
                                        // Given a supposed header file, will do all the nitty-gritty work to sort out all Analyze / Nifti combinations
AnalyzeFamilyType   GetAnalyzeType  (   const char*     file,               // whatever file we wish to know the structure
                                        char*           filehdr,            // complimentary resolved header file name
                                        char*           fileimg             // complimentary resolved image  file name
                                    )
{
ClearString ( filehdr );
ClearString ( fileimg );

if ( StringIsEmpty ( file ) )

    return  UnknwonAnalyze;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setting the header file right
TFileName           fileinhdr ( file, TFilenameExtendedPath );

                                        // .img transformed to .hdr
if      (        fileinhdr.IsExtension ( FILEEXT_MRIAVW_IMG ) )

    ReplaceExtension   ( fileinhdr, FILEEXT_MRIAVW_HDR );
                                        // not a legit .hdr or .nii file?
else if ( ! (    fileinhdr.IsExtension ( FILEEXT_MRIAVW_HDR )
              || fileinhdr.IsExtension ( FILEEXT_MRINII     ) ) )

    return  UnknwonAnalyze;

                                    // Testing only header file now
if ( ! CanOpenFile ( fileinhdr, CanOpenFileRead /*| CanOpenFileVerbose*/ ) )

    return  UnknwonAnalyze;



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Open file
ifstream            fih ( fileinhdr, ios::binary );

                                        // read full headers - maybe we can optimize this later
//struct dsr              headeranlz; // not this one, as nifti_1_header is designed to superimpose on it
struct nifti_1_header   headernii1;
struct nifti_2_header   headernii2;


//fih.seekg   ( 0, ios::beg );
//fih.read    ( (uchar *) &headeranlz, sizeof ( headeranlz ) );

fih.seekg   ( 0, ios::beg );
fih.read    ( (char *) &headernii1, sizeof ( headernii1 ) );

fih.seekg   ( 0, ios::beg );
fih.read    ( (char *) &headernii2, sizeof ( headernii2 ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

AnalyzeFamilyType   type            = UnknwonAnalyze;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // First, test for endianness

                                        // All 3 cases share an int as the size of the header, which can be used as a magic number
int                 sizeof_hdr  = headernii1.sizeof_hdr; // same as old hin.hk.sizeof_hdr

                                        // If none of these predefined constants, then should try swapping
bool                swap        = ! (    sizeof_hdr == ANALYZE_HEADER_SIZE
                                      || sizeof_hdr == NIFTI1_HEADER_SIZE
                                      || sizeof_hdr == NIFTI2_HEADER_SIZE );

if ( swap ) {

    SwappedBytes ( sizeof_hdr );
                                        // now it should work, otherwise, we have a real problem...
    if ( ! (    sizeof_hdr == ANALYZE_HEADER_SIZE
             || sizeof_hdr == NIFTI1_HEADER_SIZE
             || sizeof_hdr == NIFTI2_HEADER_SIZE ) )

        return  UnknwonAnalyze;

    SetFlags ( type, OtherMask, SwapData );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Deciphering the type between Analyze / Nifti and Single File / Dual Files (thanks guys)

                                        // dual file Nifti 1.0?
if      ( sizeof_hdr == NIFTI1_HEADER_SIZE && StringStartsWith ( headernii1.magic, "ni1", 3 ) ) {

    SetFlags ( type, TypeMask, Nifti1       );
    SetFlags ( type, FileMask, DualFiles    );
    }
                                        // single file Nifti 1.1?
else if ( sizeof_hdr == NIFTI1_HEADER_SIZE && StringStartsWith ( headernii1.magic, "n+1", 3 ) ) {

    SetFlags ( type, TypeMask, Nifti1       );
    SetFlags ( type, FileMask, SingleFile   );
    }
                                        // doc says "In the absence of this string, the file should be treated as analyze."
else if ( sizeof_hdr == ANALYZE_HEADER_SIZE /*&& StringIsEmpty ( headernii1.magic )*/ ) {

    SetFlags ( type, TypeMask, Analyze75    );
    SetFlags ( type, FileMask, DualFiles    );
    }
                                        // single file Nifti 2 + dual files (not sure if this beast really exists)?
else if ( sizeof_hdr == NIFTI2_HEADER_SIZE && StringStartsWith ( headernii2.magic, "ni2", 3 ) ) {

    SetFlags ( type, TypeMask, Nifti2       );
    SetFlags ( type, FileMask, DualFiles    );
    }
                                        // single file Nifti 2 + single file?
else if ( sizeof_hdr == NIFTI2_HEADER_SIZE && StringStartsWith ( headernii2.magic, "n+2", 3 ) ) {

    SetFlags ( type, TypeMask, Nifti2       );
    SetFlags ( type, FileMask, SingleFile   );
    }

else
    return  UnknwonAnalyze;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Testing that the dual file does actually exist
TFileName           fileinimg;

if ( IsFlag ( type, DualFiles ) ) {

    fileinimg.Set       ( fileinhdr, TFilenameExtendedPath );
                                        // All dual files format have a .img buddy file
    ReplaceExtension    ( fileinimg, FILEEXT_MRIAVW_IMG );

    if ( ! CanOpenFile ( fileinimg, CanOpenFileRead /*| CanOpenFileVerbose*/ ) )

        return  UnknwonAnalyze;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Type should be trustworthy here
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Optionally returning the nicely preprocessed file names, saving some work for the caller

//DBGM3 ( IsAnalyze75 ( type ) ? "Analyze" : IsNifti1 ( type ) ? "Nifti1" : IsNifti2 ( type ) ? "Nifti2" : "Unknown type",
//        ( type & SingleFile ) ? "Single File" : ( type & DualFiles ) ? "Dual Files" : "Unknown files",
//        ( type & SwapData ) ? "Swapping" : "Not Swapping",
//        ToFileName ( fileinhdr ) );


if ( filehdr )  StringCopy  ( filehdr, fileinhdr );
if ( fileimg )  StringCopy  ( fileimg, fileinimg ); // will be empty anyway if no dual files


return  type;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}

//EndBytePacking
