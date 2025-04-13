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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "Strings.Utils.h"
#include    "Files.Utils.h"
#include    "Dialogs.TSuperGauge.h"
#include    "TVolume.ReadWrite.h"
#include    "Math.Resampling.h"

#include    "Volumes.TTalairachOracle.h"

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const char  TalairachGroups [ NumTalairachGroups ][ 32 ] = {
                "Main Structures",
                "Lobes",
                "Gyri",
                "Matter",
                "Brodmann Areas",
                };


const char  TalairachLabels [ NumTalairachCodes ][ 36 ] = {
                "",

                                        // Label 1 - Main Structures
                "Inter-Hemispheric",
                "Left Cerebrum",
                "Right Cerebrum",
                "Right Cerebellum",
                "Right Brainstem",
                "Left Brainstem",
                "Left Cerebellum",

                                        // Label 2 - Lobes
                "Posterior Lobe",
                "Anterior Lobe",
                "Frontal-Temporal Space",
                "Limbic Lobe",
                "Medulla",
                "Pons",
                "Midbrain",
                "Sub-lobar",
                "Occipital Lobe",
                "Temporal Lobe",
                "Parietal Lobe",
                "Frontal Lobe",

                                        // Label 3 - Gyri
                "Posterior Cingulate",
                "Anterior Cingulate",
                "Subcallosal Gyrus",
                "Sub-Gyral",
                "Transverse Temporal Gyrus",
                "Uncus",
                "Rectal Gyrus",
                "Fusiform Gyrus",
                "Inferior Occipital Gyrus",
                "Inferior Temporal Gyrus",
                "Insula",
                "Parahippocampal Gyrus",
                "Lingual Gyrus",
                "Middle Occipital Gyrus",
                "Orbital Gyrus",
                "Middle Temporal Gyrus",
                "Superior Temporal Gyrus",
                "Superior Occipital Gyrus",
                "Precentral Gyrus",
                "Inferior Frontal Gyrus",
                "Cuneus",
                "Angular Gyrus",
                "Supramarginal Gyrus",
                "Cingulate Gyrus",
                "Inferior Parietal Lobule",
                "Precuneus",
                "Superior Parietal Lobule",
                "Middle Frontal Gyrus",
                "Paracentral Lobule",
                "Postcentral Gyrus",
                "Precentral Gyrus",
                "Superior Frontal Gyrus",
                "Medial Frontal Gyrus",
                "Uvula of Vermis",
                "Pyramis of Vermis",
                "Tuber of Vermis",
                "Declive of Vermis",
                "Culmen of Vermis",
                "Cerebellar Tonsil",
                "Inferior Semi-Lunar Lobule",
                "Fastigium",
                "Nodule",
                "Uvula",
                "Pyramis",
                "Tuber",
                "Declive",
                "Culmen",
                "Cerebellar Lingual",
                "Hippocampus",
                "Extra-Nuclear",
                "Lentiform Nucleus",
                "Amygdala",
                "Hypothalamus",
                "Red Nucleus",
                "Substantia Nigra",
                "Claustrum",
                "Thalamus",
                "Caudate",
                "Cerebro-Spinal Fluid",

                                        // Label 4 - Matter
                "Gray Matter",
                "White Matter",

                                        // Label 5 - Brodmann Areas
                "Brodmann area 1",
                "Brodmann area 2",
                "Brodmann area 3",
                "Brodmann area 4",
                "Brodmann area 5",
                "Brodmann area 6",
                "Brodmann area 7",
                "Brodmann area 8",
                "Brodmann area 9",
                "Brodmann area 10",
                "Brodmann area 11",
                "Brodmann area 12",
                "Brodmann area 13",
                "Brodmann area 17",
                "Brodmann area 18",
                "Brodmann area 19",
                "Brodmann area 20",
                "Brodmann area 21",
                "Brodmann area 22",
                "Brodmann area 23",
                "Brodmann area 24",
                "Brodmann area 25",
                "Brodmann area 27",
                "Brodmann area 28",
                "Brodmann area 29",
                "Brodmann area 30",
                "Brodmann area 31",
                "Brodmann area 32",
                "Brodmann area 33",
                "Brodmann area 34",
                "Brodmann area 35",
                "Brodmann area 36",
                "Brodmann area 37",
                "Brodmann area 38",
                "Brodmann area 39",
                "Brodmann area 40",
                "Brodmann area 41",
                "Brodmann area 42",
                "Brodmann area 43",
                "Brodmann area 44",
                "Brodmann area 45",
                "Brodmann area 46",
                "Brodmann area 47",
                "Caudate Tail",
                "Caudate Body",
                "Caudate Head",
                "Dentate",
                "Ventral Anterior Nucleus",
                "Ventral Posterior Medial Nucleus",
                "Ventral Posterior Lateral Nucleus",
                "Medial Dorsal Nucleus",
                "Lateral Dorsal Nucleus",
                "Pulvinar",
                "Lateral Posterior Nucleus",
                "Ventral Lateral Nucleus",
                "Midline Nucleus",
                "Anterior Nucleus",
                "Mammillary Body",
                "Fourth Ventricle",
                "Optic Tract",
                "Anterior Commissure",
                "Corpus Callosum",
                "Third Ventricle",
                "Medial Globus Pallidus",
                "Lateral Globus Pallidus",
                "Nucleus Accumbens Septi",
                "Medial Geniculum Body",
                "Lateral Geniculum Body",
                "Subthalamic Nucleus",
                "Lateral Ventricle",
                "Putamen"
                };

                                        // Global object definition
TTalairachOracle    Taloracle;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TTalairachOracle::TTalairachOracle ( const char* filepath )
{

if ( StringIsNotEmpty ( filepath ) )
    Read ( filepath );
}


//----------------------------------------------------------------------------
                                        // Converting .fin list of points to a volume
                                        // Provide the path to these lists
void    TTalairachOracle::PointsToVolume ( const char* path )
{
TFileName           file;
int                 scanxmin        = -100;
int                 scanxmax        =  100;
int                 xmin            = Highest ( xmin );
int                 xmax            = Lowest  ( xmax );
int                 ymin            = Highest ( ymin );
int                 ymax            = Lowest  ( ymax );
int                 zmin            = Highest ( zmin );
int                 zmax            = Lowest  ( zmax );
UINT                l1, l2, l3, l4, l5;
char                buff[ 1 * KiloByte ];


TSuperGauge         gauge ( "Talairach Points to Volume", 2 * ( scanxmax - scanxmin + 1 ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                    // scan files to recover spatial limits
for ( int x = scanxmin; x <= scanxmax; x++ ) {

    gauge.Next ();

    StringCopy  ( file, path, "\\X", IntegerToString ( x ), ".fin" );

    if ( ! CanOpenFile ( file ) )
        continue;


    Mined ( xmin, x );
    Maxed ( xmax, x );


    ifstream        ifs ( TFileName ( file, TFilenameExtendedPath ) );

    do {
        if ( ! *GetToken ( &ifs, buff ) )  break;

        int         y       = StringToInteger ( buff );
        int         z       = StringToInteger ( GetToken ( &ifs, buff ) );

        GetToken ( &ifs, buff );    // skip the following 5 tokens
        GetToken ( &ifs, buff );
        GetToken ( &ifs, buff );
        GetToken ( &ifs, buff );
        GetToken ( &ifs, buff );

        ymin    = min ( y, ymin );
        ymax    = max ( y, ymax );
        zmin    = min ( z, zmin );
        zmax    = max ( z, zmax );
        } while ( true );

    } // for x


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                    // create volume
int                 dim1            = xmax - xmin + 1;
int                 dim2            = ymax - ymin + 1;
int                 dim3            = zmax - zmin + 1;


TVolume<UINT>       tal ( dim1, dim2, dim3 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                    // rescan and fill volume
for ( int x = scanxmin; x <= scanxmax; x++ ) {

    gauge.Next ();

    StringCopy  ( file, path, "\\X", IntegerToString ( x ), ".fin" );

    if ( ! CanOpenFile ( file ) )
        continue;


    ifstream        ifs ( TFileName ( file, TFilenameExtendedPath ) );

    do {
        if ( ! *GetToken ( &ifs, buff ) )  break;

        int         y       = StringToInteger ( buff );
        int         z       = StringToInteger ( GetToken ( &ifs, buff ) );

        l1  = StringToInteger ( GetToken ( &ifs, buff ) );
        l2  = StringToInteger ( GetToken ( &ifs, buff ) );
        l3  = StringToInteger ( GetToken ( &ifs, buff ) );
        l4  = StringToInteger ( GetToken ( &ifs, buff ) );
        l5  = StringToInteger ( GetToken ( &ifs, buff ) );


           // origin offset
        tal ( x - xmin, y - ymin, z - zmin )    = TalairachEncodeLabel1 ( l1 )
                                                | TalairachEncodeLabel2 ( l2 )
                                                | TalairachEncodeLabel3 ( l3 )
                                                | TalairachEncodeLabel4 ( l4 )
                                                | TalairachEncodeLabel5 ( l5 );
        } while ( true );

    } // for x


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                                        // create a few slices for missing coordinates
//for ( int z = 0; z <= 5; z++ ) {
//
//    StringCopy  ( file, path, "\\MissingTal", IntegerToString ( z ), ".txt" );
//
//    ofstream    mis ( file );
//
//    for ( int x = xmin; x <= xmax; x++ )
//    for ( int y = ymin; y <= ymax; y++ )
//
//        mis << x << Tab << y << Tab << z << NewLine;
//    }


//                                        // read the slices converted by Talairach Daemon (obsolete?)
//for ( int z = 0; z <= 5; z++ ) {
//
//    StringCopy  ( file, path, "\\MissingTal", IntegerToString ( z ), ".td" );
//
//    ifstream    mis ( TFileName ( file, TFilenameExtendedPath ) );
//
//    mis.getline ( buff, KiloByte );
//
//    for ( int x = xmin; x <= xmax; x++ )
//    for ( int y = ymin; y <= ymax; y++ ) {
//
//        mis.getline ( buff, KiloByte );
//
//        if ( StringContains ( buff, (const char*) "No data" ) )
//            continue;
//
//        for ( int s = 8; s <= 19; s++ ) {
//            if ( ! StringContains ( buff, TalairachLabels [ s ] ) )
//                continue;
//
//            tal ( x - xmin, y - ymin, z - zmin )   |= TalairachEncodeLabel2 ( s );
//            }
//        }
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // write Talaraich volume as .dlf file
StringCopy          ( file, path, "\\", TalairachOracleFileName );
CheckNoOverwrite    ( file );

tal.WriteFile ( file );

                                        // for visualization as a Nifti
//StringCopy          ( file, path, "\\Talairach." FILEEXT_MRINII );
//CheckNoOverwrite    ( file );
//tal.WriteFile ( file, &TPointDouble ( TalairachOracleOriginX, TalairachOracleOriginY, TalairachOracleOriginZ ) );

gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
bool    TTalairachOracle::Read ( const char* filepath )
{
                                        // init only once
if ( IsAllocated () )
    return  true;


TFileName           file;

StringCopy ( file, StringIsEmpty ( filepath ) ? TalairachOracleFileName : filepath );

if ( ! CanOpenFile ( file, CanOpenFileRead /*CanOpenFileReadAndAsk*/ ) )
    return  false;

                                        // we know this
int                 dim1            = TalairachOracleDim1;
int                 dim2            = TalairachOracleDim2;
int                 dim3            = TalairachOracleDim3;


TalairachVolume.Resize ( dim1, dim2, dim3 );

TalairachVolume.ReadFile ( file );

return  true;
}


//----------------------------------------------------------------------------
const char* TTalairachOracle::CodeToName ( UINT talcode )   const
{
if ( IsInsideLimits ( talcode, TalairachStructMin, TalairachBrodmannMax ) )
    return   TalairachLabels [ talcode ];
else
    return  "";
}


//----------------------------------------------------------------------------
UINT        TTalairachOracle::NameToCode ( char *talname )  const
{
if ( StringIsEmpty ( talname ) )
    return  0;

                                        // take care of this casus belli
StringReplace ( talname, "grey", "gray" );


for( int i = 0; i < NumTalairachCodes; i++ )
    if ( StringIs ( TalairachLabels [ i ], talname ) )
        return  i;


return  0;
}


//----------------------------------------------------------------------------
UINT        TTalairachOracle::PositionToCode ( const TPointFloat& pos )     const
{
if ( IsNotAllocated () )
    return  0;

                                        // convert to relative / voxel space + checking boundaries
return  TalairachVolume.GetValueChecked ( pos + TPointFloat ( TalairachOracleOriginX, TalairachOracleOriginY, TalairachOracleOriginZ ) );
}


//----------------------------------------------------------------------------
                                        // Talaraich is splitted in 5 different groups
bool        TTalairachOracle::PositionToCodes ( const TPointFloat& pos, int codes[ NumTalairachGroups ] )   const
{
codes[ 0 ]  = codes[ 1 ]  = codes[ 2 ]  = codes[ 3 ]  = codes[ 4 ]  = 0;

if ( IsNotAllocated () )
    return  false;


UINT                t           = PositionToCode ( pos );

if ( ! t )
    return  false;

                                        // decode into the 5 groups
codes[ 0 ]  = TalairachDecodeLabel1 ( t );
codes[ 1 ]  = TalairachDecodeLabel2 ( t );
codes[ 2 ]  = TalairachDecodeLabel3 ( t );
codes[ 3 ]  = TalairachDecodeLabel4 ( t );
codes[ 4 ]  = TalairachDecodeLabel5 ( t );


return  true;
}


//----------------------------------------------------------------------------
bool        TTalairachOracle::PositionHasCode ( const TPointFloat& pos, UINT talcode )  const
{
if ( talcode == 0 || IsNotAllocated () )
    return  false;


UINT                t           = PositionToCode ( pos );

if ( ! t )
    return  false;


if ( TalairachDecodeLabel1 ( t ) == talcode )   return  true;
if ( TalairachDecodeLabel2 ( t ) == talcode )   return  true;
if ( TalairachDecodeLabel3 ( t ) == talcode )   return  true;
if ( TalairachDecodeLabel4 ( t ) == talcode )   return  true;
if ( TalairachDecodeLabel5 ( t ) == talcode )   return  true;


return  false;
}


//----------------------------------------------------------------------------
const char* TTalairachOracle::PositionToString ( const TPointFloat& pos, char *name, bool groupresult )     const
{
ClearString ( name );


int                 talcodes[ NumTalairachGroups ];

if ( ! PositionToCodes ( pos, talcodes ) ) {

    if ( ! groupresult )
        StringAppend ( name, Tab Tab Tab Tab );
    return  name;
    }


//for ( int i = 0; i < NumTalairachGroups; i++ )
//    if ( talcodes[ i ] )   StringAppend ( name, TalairachLabels[ talcodes[ i ] ], NewLine );


if ( groupresult ) {
                                        // group the labels somehow
    if ( talcodes[ 0 ] )        StringAppend ( name,                            TalairachLabels[ talcodes[ 0 ] ] );
    if ( talcodes[ 3 ] )        StringAppend ( name, talcodes[ 0 ] ? ", " : "", TalairachLabels[ talcodes[ 3 ] ] );
    if ( talcodes[ 0 ] 
      || talcodes[ 3 ] )        StringAppend ( name, NewLine );

    if ( talcodes[ 1 ] )        StringAppend ( name,                            TalairachLabels[ talcodes[ 1 ] ] );
    if ( talcodes[ 2 ] )        StringAppend ( name, talcodes[ 1 ] ? ", " : "", TalairachLabels[ talcodes[ 2 ] ] );
    if ( talcodes[ 1 ] 
      || talcodes[ 2 ] )        StringAppend ( name, NewLine );

    if ( talcodes[ 4 ] )        StringAppend ( name,                            TalairachLabels[ talcodes[ 4 ] ] );
    if ( talcodes[ 4 ] )        StringAppend ( name, NewLine );
    }
else
                                sprintf ( name, "%s" Tab "%s" Tab "%s" Tab "%s" Tab "%s", TalairachLabels[ talcodes[ 0 ] ], TalairachLabels[ talcodes[ 1 ] ], TalairachLabels[ talcodes[ 2 ] ], TalairachLabels[ talcodes[ 3 ] ], TalairachLabels[ talcodes[ 4 ] ] );

return  name;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
