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

#include    "GenerateRois.h"

#include    "Strings.Utils.h"
#include    "Strings.TStringsMap.h"
#include    "Math.Resampling.h"
#include    "TVolume.h"
#include    "Files.TVerboseFile.h"
#include    "Dialogs.TSuperGauge.h"
#include    "TRois.h"

#include    "Volumes.AAL.h"
#include    "Volumes.TTalairachOracle.h"

#include    "TBaseDoc.h"
#include    "TVolumeDoc.h"
#include    "TSolutionPointsDoc.h"

#include    "TCreateRoisDialog.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

extern  TTalairachOracle    Taloracle;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // CreateRoisInteractiveTracks, CreateRoisInteractiveXyz or CreateRoisInteractiveSp cases
                                        // Note that it could be used for other cases, too...
bool    GenerateRois                    (
                                        CreateRoisTypes             Processing,
                                        const TRois&                Rois,

                                        const TBaseDoc*             BaseDoc,            // mandatory - could be either tracks, electrodes or solution points doc
                                        const TVolumeDoc*           MRIDoc,             // optional
                                        const char*                 RoisLabelsFile,     // optional - labels associated with MRIDoc
                                        const char*                 GenerateRoisFile,   // optional

                                        const char*                 BaseFileName
                                        )
{
if ( ! (    Processing == CreateRoisInteractiveTracks
         || Processing == CreateRoisInteractiveXyz
         || Processing == CreateRoisInteractiveSp     ) 
    || Rois.IsNotAllocated () || Rois.IsEmpty () 
    || BaseDoc == 0 
    || StringIsEmpty ( BaseFileName ) )

    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        
TFileName           roifilename;
TFileName           verbosefile;

StringCopy      ( roifilename,      BaseFileName );
AddExtension    ( roifilename,      FILEEXT_ROIS );

StringCopy      ( verbosefile,      roifilename );
AddExtension    ( verbosefile,      FILEEXT_VRB );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // basically this is it...
Rois.WriteFile  ( roifilename );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TVerboseFile    verbose ( verbosefile, VerboseFileDefaultWidth );

verbose.PutTitle ( "Creating ROIs" );


verbose.NextTopic ( "Input Files:" );
{
if      ( Processing == CreateRoisInteractiveTracks )   verbose.Put ( "Tracks file:",                   BaseDoc->GetDocPath () );
else if ( Processing == CreateRoisInteractiveXyz    )   verbose.Put ( "Electrodes Coordinates file:",   BaseDoc->GetDocPath () );
else if ( Processing == CreateRoisInteractiveSp     )   verbose.Put ( "Solution Points file:",          BaseDoc->GetDocPath () );


if ( MRIDoc )
    verbose.Put ( "ROIs Volume file:",  MRIDoc->GetDocPath () );

if ( StringIsNotEmpty ( RoisLabelsFile ) )
    verbose.Put ( "ROIs Labels file:",  RoisLabelsFile );

if ( StringIsNotEmpty ( GenerateRoisFile ) ) {
    verbose.NextLine ();
    verbose.Put ( "Generating ROIs from file:", GenerateRoisFile );
    }
}


verbose.NextTopic ( "Output Files:" );
{
verbose.Put ( "ROIs file:",             roifilename );
verbose.Put ( "Verbose File (this):",   verbosefile );
}


verbose.NextTopic ( "Processing:" );
{
verbose.Put ( "Mode:", "Interactive, ROIs defined through user selection" );
verbose.Put ( "Number of ROIs:", Rois.GetNumRois () );
}


verbose.NextLine ( 2 );


return  true;
}


//----------------------------------------------------------------------------
                                        // CreateRoisAutomaticSpMri or CreateRoisAutomaticSpAAL cases
bool    GenerateRoisFromSpAndMri        (
                                        CreateRoisTypes             Processing,

                                        const TSolutionPointsDoc*   SPDoc,
                                        const TVolumeDoc*           MRIDoc,
                                        const char*                 RoisLabelsFile,     // labels associated with MRIDoc

                                        const char*                 OptionalRoisFile,   // optional files with specific ROIS - otherwise it will generate all ROIs

                                        const char*                 BaseFileName
                                        )
{
if ( ! (    Processing == CreateRoisAutomaticSpMri
         || Processing == CreateRoisAutomaticSpAAL ) 
    || SPDoc  == 0 
    || MRIDoc == 0
    || StringIsEmpty ( BaseFileName ) )

    return  false;


int                 genroisnumlines = StringIsNotEmpty ( OptionalRoisFile ) ? CountLines ( OptionalRoisFile ) : 0;
bool                genroisfileok   = genroisnumlines > 0 ;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const Volume&               mribackgdata    = *MRIDoc->GetData ();
const TBoundingBox<int>*    mrisize         =  MRIDoc->GetSize ();

int                         mrixmin         = mrisize->XMin ();
int                         mrixmax         = mrisize->XMax ();
int                         mriymin         = mrisize->YMin ();
int                         mriymax         = mrisize->YMax ();
int                         mrizmin         = mrisize->ZMin ();
int                         mrizmax         = mrisize->ZMax ();

const TPointDouble&         MRIOrigin       = MRIDoc->GetOrigin ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           roifilename;
TFileName           spifilename;
TFileName           verbosefile;

StringCopy      ( roifilename,      BaseFileName );
AddExtension    ( roifilename,      FILEEXT_ROIS );

StringCopy      ( spifilename,      roifilename   );
AddExtension    ( spifilename,      FILEEXT_SPIRR );

StringCopy      ( verbosefile,      roifilename );
AddExtension    ( verbosefile,      FILEEXT_VRB );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // store the association between index and descriptions
TStringsMap         roimap;


if      ( Processing == CreateRoisAutomaticSpMri ) {

    if ( genroisfileok && StringIsNotEmpty ( RoisLabelsFile ) )

        roimap.Add ( RoisLabelsFile );              // real dictionary read from file
    else
        roimap.Add ( 1, MRIDoc->GetMaxValue () );   // build a dummy dictionary up to the max in volume
    }

else if ( Processing == CreateRoisAutomaticSpAAL )

        roimap.Add ( AAL3v1Codes, NumAAL3v1Codes ); // convert an AAL array

                                        // we can cumulate many rois together, remember which, to construct the final roi name
TSelection          namesel ( roimap.GetMaxKey () + 1, OrderArbitrary );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const TPoints&      points          = SPDoc->GetPoints ();
int                 numsp           = SPDoc->GetNumSolPoints ();
TSelection          SelForRoi ( numsp, OrderSorted );

char                roiname[ RoiNameLength * 4 ];
char                buff   [ KiloByte ];
int                 roivoxel;
bool                indexok;
int                 maxrois         = genroisfileok ? genroisnumlines : MRIDoc->GetMaxValue ();
int                 curroi          = 0;
TPointFloat         roicenter;
TPoints             roiscenter;

                                        // pre-allocate rois, as we know the max # of rois
TRois               Rois ( maxrois, numsp );

                                        // we could check some consistency between the dictionary roimap max value & # values, and maxrois

                                        // optional file with a list of rois to generate
ifstream            ifs;

if ( genroisfileok )
    ifs.open ( TFileName ( OptionalRoisFile, TFilenameExtendedPath ) );


TSuperGauge         Gauge ( Processing == CreateRoisAutomaticSpAAL ? "Scanning AAL ROIs" : "Scanning MRI ROIs", maxrois, SuperGaugeLevelInter );

                                        // if it exists, scan file, each line is 1 ROI
                                        // or generate roi values otherwise, for each volume index
do {

    if ( genroisfileok )    ifs.getline     ( buff, KiloByte ); // read next roi from file
    else                    IntegerToString ( buff, ++curroi ); // or simply generate one


    if ( StringIsEmpty ( buff ) || StringIsSpace ( buff ) )
        continue;

    Gauge.Next ();


    SelForRoi.Reset ();
    namesel  .Reset ();
    roicenter   = 0.0;
    ClearString ( roiname );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we can have multiple roi codes on 1 line to be grouped together
    TSplitStrings   tokens ( buff, UniqueStrings );


    for ( int i = 0; i < (int) tokens; i++ ) {

                                        // convert to a MRI index
        indexok     = roimap.ValueToKey ( tokens[ i ], roivoxel );


        if ( ! indexok ) {
            ShowMessage (   "Unrecognized ROI name!"        NewLine 
                            "This field will be skipped.", 
                            tokens[ i ], ShowMessageWarning );
            continue;
            }

                                        // already processed from the same line?
        if ( namesel[ roivoxel ] )
            continue;


        namesel.Set ( roivoxel );


                                        // scan volume
        OmpParallelFor
        
        for ( int x = mrixmin; x <= mrixmax; x++ )
        for ( int y = mriymin; y <= mriymax; y++ )
        for ( int z = mrizmin; z <= mrizmax; z++ ) {

            if ( (int) mribackgdata ( x, y, z ) != roivoxel )   // not the one we search
                continue;


            for ( int sp = 0; sp < numsp; sp++ ) {
                                    // shift of 0.5 or not?
                                                       // mribackgdata.ToAbs ()
                if ( Truncate ( points[ sp ].X + 0.5 ) == Truncate ( x - MRIOrigin.X )
                  && Truncate ( points[ sp ].Y + 0.5 ) == Truncate ( y - MRIOrigin.Y )
                  && Truncate ( points[ sp ].Z + 0.5 ) == Truncate ( z - MRIOrigin.Z ) ) {
                                        // thread-safe
                    SelForRoi.Set ( sp );

                    OmpCritical ( roicenter  += points[ sp ] );
                    }
                } // for sp
            } // for x y z

        } // for input field


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // storing the centroid of current ROI, which could be used later on for display
    if ( SelForRoi.NumSet () != 0 ) {

        roicenter  /= SelForRoi.NumSet ();

        roiscenter.Add ( roicenter );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compose name now, to avoid repeating strings
    for ( TIteratorSelectedForward ni ( namesel ); (bool) ni; ++ni )
                                                                   // get longest string for that index
        StringAppend ( roiname, StringIsEmpty ( roiname ) ? "" : " ", roimap.GetValue ( ni() ) );


    if ( StringLength ( roiname ) >= RoiNameLength )
        StringShrink ( roiname, roiname, RoiNameLength );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( SelForRoi.NumSet () )

        Rois.AddRoi ( SelForRoi, roiname, false, true );
//  else
//      ShowMessage ( "Empty ROI will be be skipped.", roiname, ShowMessageWarning );


    } while (    genroisfileok && ! ifs.eof ()         // either reading input file
            || ! genroisfileok && curroi < maxrois );  // or automatice generation


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( Rois.IsEmpty () ) {

    ShowMessage (   "No ROIs were created due to some lack of overlap" NewLine 
                    "between the Solution Points and the MRI."         NewLine 
                    "Please check your parameters!", 
                    "Automatic ROIs", ShowMessageWarning );

    return  false;
    }


Rois      .WriteFile    ( roifilename );
roiscenter.WriteFile    ( spifilename, Rois.GetRoiNames () );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TVerboseFile    verbose ( verbosefile, VerboseFileDefaultWidth );

verbose.PutTitle ( "Creating ROIs" );


verbose.NextTopic ( "Input Files:" );
{
verbose.Put ( "Solution Points file:",  SPDoc->GetDocPath () );
verbose.Put ( "ROIs Volume file:",      MRIDoc->GetDocPath () );

if ( genroisfileok && StringIsNotEmpty ( RoisLabelsFile ) )
    verbose.Put ( "ROIs Labels file:",  RoisLabelsFile );

if ( StringIsNotEmpty ( OptionalRoisFile ) ) {
    verbose.NextLine ();
    verbose.Put ( "Generating ROIs from file:", OptionalRoisFile );
    }
}


verbose.NextTopic ( "Output Files:" );
{
verbose.Put ( "ROIs file:",             roifilename );
verbose.Put ( "ROIs coordinates file:", spifilename );
verbose.Put ( "Verbose File (this):",   verbosefile );
}


verbose.NextTopic ( "Processing:" );
{
                                    // CreateRoisAutomaticSpMri & CreateRoisAutomaticSpAAL cases
if      ( Processing == CreateRoisAutomaticSpMri )  verbose.Put ( "Mode:", "Automatic, Solution Points intersecting a Volume Mask" );
else if ( Processing == CreateRoisAutomaticSpAAL )  verbose.Put ( "Mode:", "Automatic, Solution Points intersecting the AAL Mask" );

verbose.Put ( "Number of ROIs:", Rois.GetNumRois () );
}


verbose.NextLine ( 2 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.HappyEnd ();

return  Rois.IsNotEmpty ();
}


//----------------------------------------------------------------------------
                                        // CreateRoisAutomaticSpTalairach case
bool    GenerateRoisFromSpAndTalairach  (
                                        CreateRoisTypes             Processing,

                                        const TSolutionPointsDoc*   SPDoc,
                                        const TVolumeDoc*           MRIDoc,             // optional volume

                                        const char*                 GenerateRoisFile,   // mandatory file with specific ROIS

                                        const char*                 BaseFileName
                                        )
{
if (   Processing != CreateRoisAutomaticSpTalairach
    || SPDoc == 0
    || StringIsEmpty ( GenerateRoisFile )
    || StringIsEmpty ( BaseFileName     ) )

    return  false;


int                 genroisnumlines = CountLines ( GenerateRoisFile );


if ( genroisnumlines == 0 )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const TPoints&      points          = SPDoc->GetPoints ();
int                 numsp           = SPDoc->GetNumSolPoints ();
const TBoundingBox<double>* spbounding  = &SPDoc->GetDisplaySpaces ()[ 0 ].Bounding;
TSelection          SelForRoi ( numsp, OrderSorted );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // generate volume mask
const Volume*       mribackgdata    = 0;

Volume              VolOut;             // using floatting points values, as we could have more than 255 ROIs (unlikely, but still)
TVolume<uchar>      VolRoi;             // just masks
TVolume<uchar>      VolRight;
TVolume<uchar>      VolLeft;


TPointDouble        MRIOrigin       = MRIDoc ? MRIDoc->GetOrigin () : TPointDouble ( (double) 0 );

int                 mrixmin;
int                 mrixmax;
int                 mriymin;
int                 mriymax;
int                 mrizmin;
int                 mrizmax;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           roifilename;
TFileName           volroifilename;
TFileName           labelsfilename;
TFileName           verbosefile;

StringCopy      ( roifilename,      BaseFileName );
AddExtension    ( roifilename,      FILEEXT_ROIS );

StringCopy      ( volroifilename,   roifilename   );
AddExtension    ( volroifilename,   DefaultMriExt );

StringCopy      ( labelsfilename,   roifilename );
AddExtension    ( labelsfilename,   FILEEXT_TXT );

StringCopy      ( verbosefile,      roifilename );
AddExtension    ( verbosefile,      FILEEXT_VRB );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( MRIDoc ) {                         // allocate and prepare some working volumes

    mribackgdata                            = MRIDoc->GetData ();
    const TBoundingBox<int>*    mrisize     = MRIDoc->GetSize ();

    mrixmin         = mrisize->XMin ();
    mrixmax         = mrisize->XMax ();
    mriymin         = mrisize->YMin ();
    mriymax         = mrisize->YMax ();
    mrizmin         = mrisize->ZMin ();
    mrizmax         = mrisize->ZMax ();


    VolOut  .Resize ( mribackgdata->GetDim1 (), mribackgdata->GetDim2 (), mribackgdata->GetDim3 () );
    VolRoi  .Resize ( mribackgdata->GetDim1 (), mribackgdata->GetDim2 (), mribackgdata->GetDim3 () );
    VolRight.Resize ( mribackgdata->GetDim1 (), mribackgdata->GetDim2 (), mribackgdata->GetDim3 () );
    VolLeft .Resize ( mribackgdata->GetDim1 (), mribackgdata->GetDim2 (), mribackgdata->GetDim3 () );

    TPointFloat         talpos;

                                        // scan all main structures, and set all that includes either "left" or "right" (accordingly)
    for ( int s = TalairachStructMin; s <= TalairachStructMax; s++ ) {

        bool        doright     = StringStartsWith ( Taloracle.CodeToName ( s ), "Right" );
        bool        doleft      = StringStartsWith ( Taloracle.CodeToName ( s ), "Left"  );


        for ( int x = mrixmin; x <= mrixmax; x++ )
        for ( int y = mriymin; y <= mriymax; y++ )
        for ( int z = mrizmin; z <= mrizmax; z++ ) {

            if ( ! (*mribackgdata) ( x, y, z ) )
                continue;

                          // mribackgdata.ToAbs () + rounding
            talpos.Set ( x - MRIOrigin.X + 0.5, y - MRIOrigin.Y + 0.5, z - MRIOrigin.Z + 0.5 );

            SPDoc->GetGeometryTransform ()->ConvertTo ( talpos, spbounding );

                                // belongs to current structure?
            if ( Taloracle.PositionHasCode ( talpos, s ) )

                if      ( doright )     VolRight ( x, y, z )    = true;
                else if ( doleft  )     VolLeft  ( x, y, z )    = true;

            } // for x y z
        } // for structure
    } // if MRIDoc


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // pre-allocate rois, as we know the max # of rois
TRois               Rois ( genroisnumlines, numsp );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // setup for general left & right selection
TSelection          SelRight ( SelForRoi );
TSelection          SelLeft  ( SelForRoi );

SelRight.Reset ();
SelLeft .Reset ();

                                        // scan all main structures, and set all that includes either "left" or "right" (accordingly)
for ( int s = TalairachStructMin; s <= TalairachStructMax; s++ ) {

    bool            doright     = StringStartsWith ( Taloracle.CodeToName ( s ), "Right" );
    bool            doleft      = StringStartsWith ( Taloracle.CodeToName ( s ), "Left"  );
    TPointFloat     talpos;


    for ( int sp = 0; sp < numsp; sp++ ) {

        talpos.Set ( points[ sp ].X + 0.5, points[ sp ].Y + 0.5, points[ sp ].Z + 0.5 );

        SPDoc->GetGeometryTransform ()->ConvertTo ( talpos, spbounding );

                                        // belongs to current structure?
        if ( Taloracle.PositionHasCode ( talpos, s ) )

            if      ( doright )     SelRight.Set   ( sp );
            else if ( doleft  )     SelLeft .Set   ( sp );
        } // for sp
    } // for structure


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ifstream            ifs ( TFileName ( GenerateRoisFile, TFilenameExtendedPath ) );

TSuperGauge         Gauge ( "Scanning Talairach ROIs", genroisnumlines, SuperGaugeLevelInter );

char                l           [ 5 ][ 64 ];
char                roiname     [ MaxPathShort ];
char                buff        [ KiloByte ];
int                 VolRoiIndex     = 0;

                                        // scan file, each line is 1 ROI
do {
    ifs.getline ( buff, KiloByte );

    if ( StringIsEmpty ( buff ) )
        continue;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Gauge.Next ();


    SelForRoi.Reset ();
    ClearString ( roiname );

    if ( MRIDoc )
        VolRoi.ResetMemory ();

                                        // trick to easily separate the different string fields
    ReplaceChars ( buff, " ",    "_" );
    ReplaceChars ( buff, ",;\t", " " );

    int                 numfields       = sscanf ( buff, "%s %s %s %s %s", l[ 0 ], l[ 1 ], l[ 2 ], l[ 3 ], l[ 4 ] );
    bool                firstfield      = true;
    TPointFloat         talpos;
    UINT                talcode;


    for ( int i = 0; i < numfields; i++ ) {

        ReplaceChars  ( l[ i ], "_", " " );
        StringCleanup ( l[ i ] );


        talcode = Taloracle.NameToCode ( l[ i ] );

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( ! talcode ) {              // not a known talairach name?

                                        // check for another syntax: beginning with "left" or "right"
            if ( firstfield && ( StringStartsWith ( l[ i ], "Right" ) || StringStartsWith ( l[ i ], "Left" ) ) ) {

                StringToLowercase ( l[ i ] );

                bool        doright     = StringStartsWith ( l[ i ], "Right" );

                StringCopy ( roiname, doright ? "Right" : "Left" );

                                        // clean the current field
                StringReplace ( l[ i ], "right", "" );
                StringReplace ( l[ i ], "left", "" );
                StringCleanup ( l[ i ] );

                                        // get the talairach code of the remaining part
                talcode         = Taloracle.NameToCode ( l[ i ] );

                if ( ! talcode ) {      // really continue if the remaining part has not been recognized

                                        // case of only "right" or "left"
                    if ( StringIsEmpty ( l[ i ] ) ) {

                        SelForRoi       = doright ? SelRight : SelLeft;

                        if ( MRIDoc )
                            VolRoi      = doright ? VolRight : VolLeft;
                        }
                    else
                        ShowMessage (   "Unrecognized Talairach / Brodmann name!" NewLine 
                                        "This field will be skipped.", 
                                        l[ i ], ShowMessageWarning );

                    continue;
                    }


                firstfield      = false;
                                        // full copy of whole right / left selection
                SelForRoi       = doright ? SelRight : SelLeft;

                if ( MRIDoc )
                    VolRoi      = doright ? VolRight : VolLeft;

                } // if left or right

            else {                      // bad talcode & not left or right
                ShowMessage (   "Unrecognized Talairach / Brodmann name!" NewLine 
                                "This field will be skipped.", 
                                l[ i ], ShowMessageWarning );
                continue;
                }

            } // if talcode

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here, we have a valid talairach code
        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( StringEndsWith ( roiname, "left" ) || StringEndsWith ( roiname, "right" ) )

            StringAppend    ( roiname, " ", Taloracle.CodeToName ( talcode ) );
        else
            StringAppend    ( roiname, StringIsNotEmpty ( roiname ) ? ", " : "", Taloracle.CodeToName ( talcode ) );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // scan all sol points, and see if they belong to current talairach region
        for ( int sp = 0; sp < numsp; sp++ ) {

            talpos.Set ( points[ sp ].X + 0.5, points[ sp ].Y + 0.5, points[ sp ].Z + 0.5 );
//          talpos.Set ( points[ sp ] );

            SPDoc->GetGeometryTransform ()->ConvertTo ( talpos, spbounding );

                                        // first time: set belonging
            if (   firstfield &&   Taloracle.PositionHasCode ( talpos, talcode ) )
                SelForRoi.Set   ( sp );
                                        // other times: reset if not belonging
            if ( ! firstfield && ! Taloracle.PositionHasCode ( talpos, talcode ) )
                SelForRoi.Reset ( sp );
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( MRIDoc ) {

            OmpParallelFor

            for ( int x = mrixmin; x <= mrixmax; x++ )
            for ( int y = mriymin; y <= mriymax; y++ )
            for ( int z = mrizmin; z <= mrizmax; z++ ) {

                if ( ! (*mribackgdata) ( x, y, z ) )
                    continue;

                                      // mribackgdata.ToAbs () + rounding
                TPointFloat     talpos ( x - MRIOrigin.X + 0.5, y - MRIOrigin.Y + 0.5, z - MRIOrigin.Z + 0.5 );

                SPDoc->GetGeometryTransform ()->ConvertTo ( talpos, spbounding );

                                // first time: set belonging
                if (   firstfield &&   Taloracle.PositionHasCode ( talpos, talcode ) )  VolRoi ( x, y, z )  = true;

                                // other times: reset if not belonging
                if ( ! firstfield && ! Taloracle.PositionHasCode ( talpos, talcode ) )  VolRoi ( x, y, z )  = false;

                } // for x y z
            } // if MRIDoc


        firstfield      = false;
        } // for input field


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( SelForRoi.NumSet () != 0 )

        Rois.AddRoi ( SelForRoi, roiname, false, true );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // !It allows for selected region to be incorporated into ROI volume, even if they were no solution points in said region!
                                        // !This could totally produce more volumre ROIs than solution points ROIs!
    if ( MRIDoc ) {
                                        // save result in output volume, recompacting the indexes, plus write the index <-> labels conversion
        bool        emptyroi    = true;

        for ( int i = 0; i < VolRoi.GetLinearDim (); i++ )
            if ( VolRoi[ i ] != 0 ) {
                emptyroi    = false;
                break;
                }


        if ( ! emptyroi ) {
                                        // next roi to insert
            VolRoiIndex++;

                                        // append to label file
            ofstream            ofl ( labelsfilename, ios::out | ( VolRoiIndex != 1 ? ios::app : 0 ) );
            ofl << VolRoiIndex << Tab << roiname << fastendl;
            ofl.close ();

                                        // cumulate into volume roi
            OmpParallelFor

            for ( int x = mrixmin; x <= mrixmax; x++ )
            for ( int y = mriymin; y <= mriymax; y++ )
            for ( int z = mrizmin; z <= mrizmax; z++ ) {
                                        // check for already existing ROI
                if ( VolRoi ( x, y, z ) && ! VolOut ( x, y, z ) )

                    VolOut ( x, y, z )  = VolRoiIndex;
                }
            }
        } // if MRIDoc


    } while ( ! ifs.eof() );            // roi names file


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( Rois.IsEmpty () ) {

    ShowMessage (   "No ROIs were created, maybe due to erroneous Talairach specifications." NewLine 
                    "Please check your parameters!", 
                    "Automatic ROIs", ShowMessageWarning );

    return  false;
    }


Rois.WriteFile ( roifilename );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( MRIDoc && VolRoiIndex != 0 ) {

    TExportVolume     expvol;

    StringCopy      ( expvol.Filename, volroifilename );
                                        // byte output most of the time
    expvol.VolumeFormat     = GetVolumeAtomType ( &VolOut, FilterTypeNone, InterpolateUnknown, ToExtension ( expvol.Filename ) );

    expvol.MaxValue         = VolOut.GetAbsMaxValue ();

    expvol.VoxelSize        = MRIDoc->GetVoxelSize ();

    expvol.RealSize         = MRIDoc->GetRealSize ();

    expvol.Origin           = MRIDoc->GetOrigin ();

    expvol.NiftiTransform   = AtLeast ( NiftiTransformDefault, MRIDoc->GetNiftiTransform () );

    expvol.NiftiIntentCode  = NiftiIntentCodeLabels;

    StringCopy  ( expvol.NiftiIntentName, NiftiIntentNameLabels, NiftiIntentNameSize - 1 );

//  if ( MRIDoc->HasKnownOrientation () )   // always saving orientation?
        MRIDoc->OrientationToString ( expvol.Orientation );


    expvol.Write ( VolOut, ExportArrayOrderZYX );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TVerboseFile    verbose ( verbosefile, VerboseFileDefaultWidth );

verbose.PutTitle ( "Creating ROIs" );


verbose.NextTopic ( "Input Files:" );
{
verbose.Put ( "Solution Points file:",  SPDoc->GetDocPath () );
if ( MRIDoc )
    verbose.Put ( "ROIs Volume file:",  MRIDoc->GetDocPath () );

verbose.NextLine ();
verbose.Put ( "Generating ROIs from file:", GenerateRoisFile );
}


verbose.NextTopic ( "Output Files:" );
{
verbose.Put ( "ROIs file:",                 roifilename );

if ( MRIDoc && VolRoiIndex != 0 ) {
    verbose.NextLine ();
    verbose.Put ( "Volume ROI file:",           volroifilename );
    verbose.Put ( "Volume ROI file labels:",    labelsfilename );
    verbose.NextLine ();
    }

verbose.Put ( "Verbose File (this):",       verbosefile );
}


verbose.NextTopic ( "Processing:" );
{
                                    // CreateRoisAutomaticSpTalairach case
verbose.Put ( "Mode:", "Automatic, Solution Points belonging to Talairach regions" );
verbose.Put ( "Number of ROIs:", Rois.GetNumRois () );
}


verbose.NextLine ( 2 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.HappyEnd ();

return  Rois.IsNotEmpty ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
