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

#include    "Electrodes.BuildTemplateElectrodes.h"

#include    "Math.Stats.h"
#include    "Math.TMatrix44.h"
#include    "Geometry.TPoints.h"
#include    "Files.TGoF.h"
#include    "Files.Utils.h"
#include    "Files.TVerboseFile.h"

#include    "GlobalOptimize.Points.h"
#include    "TExportTracks.h"
#include    "Electrodes.Utils.h"

#include    "TElectrodesDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

OptimizeOff

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Assumes electrodes have all been tested as compatible
void    BuildTemplateElectrodes (   const TGoF&         filesin,
                                    const TArray1<int>  czindex,
                                    const TArray1<int>  fpzindex,
                                    const TArray1<int>  ozindex,
                                    bool                centercz,
                                    const char*         filesout
                                )
{
if (   filesin.IsEmpty () 
    || StringIsEmpty ( filesout ) 
    || czindex .IsNotAllocated ()
    || fpzindex.IsNotAllocated ()
    || ozindex .IsNotAllocated () )

    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numxyz          = (int) filesin;
vector<TPoints>     xyzpoints ( numxyz );
TStrings            xyznames;

                                        // read all xyz coordinates now
for ( int i = 0; i < numxyz; i++ )

    xyzpoints[ i ].ReadFile ( filesin[ i ], i ? 0 : &xyznames );

                                        // all same number of electrodes
int                 numpoints       = xyzpoints[ 0 ].GetNumPoints ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // remember what are the non-null electrodes, for each file
TArray2<bool>       oktoaverage ( numxyz, numpoints );

for ( int i = 0; i < numxyz;    i++ )
for ( int p = 0; p < numpoints; p++ )

    oktoaverage ( i, p )    = xyzpoints[ i ][ p ].IsNotNull ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           BaseFileName;
TFileName           fileoutprefix;
TFileName           filexyzrawaverage;
TFileName           filexyzfinal;
TFileName           fileerror;
//TFileName         newfile;
TFileName           buff;


StringCopy  ( BaseFileName, (const char *) filesout );

if ( IsExtensionAmong ( BaseFileName, AllCoordinatesFilesExt ) )
    RemoveExtension ( BaseFileName );

StringCopy  ( fileoutprefix,            ToFileName ( BaseFileName ) );


StringCopy  ( filexyzrawaverage,        BaseFileName,   ".Raw Average",             ".",    FILEEXT_XYZ    );
StringCopy  ( filexyzfinal,             BaseFileName,                               ".",    FILEEXT_XYZ    );
StringCopy  ( fileerror,                BaseFileName,   ".Distances",               ".",    FILEEXT_EEGSEF );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Verbose story
TVerboseFile        Verbose;

StringCopy   ( buff, BaseFileName );
AddExtension ( buff, FILEEXT_VRB );

Verbose.Open ( buff, VerboseFileDefaultWidth );

Verbose.PutTitle ( "Averaging Electrodes Coordinates" );


Verbose.NextTopic ( "Input Files:" );
{
Verbose.Put ( "Number of input files:", numxyz    );
for ( int i = 0; i < numxyz; i++ )
    Verbose.Put ( i == 0 ? "Electrodes files:" : "", filesin[ i ] );
}


Verbose.NextTopic ( "Ouput Files:" );
{
Verbose.Put ( "Verbose File (this):", buff );
Verbose.Put ( "Average coordinates:", filexyzfinal );
Verbose.Put ( "Distances file:", fileerror );
}


Verbose.NextTopic ( "Parameters:" );
{
Verbose.Put ( "Number of electrodes:", numpoints );

StringCopy ( buff, xyznames[ czindex [ 0 ] ] );
for ( int i = 1; i < (int) czindex; i++ )
    StringAppend ( buff, ", ", xyznames[ czindex [ i ] ] );
Verbose.Put ( "Electrode(s) for Cz / Vertex:", buff );

StringCopy ( buff, xyznames[ fpzindex [ 0 ] ] );
for ( int i = 1; i < (int) fpzindex; i++ )
    StringAppend ( buff, ", ", xyznames[ fpzindex [ i ] ] );
Verbose.Put ( "Electrode(s) for Fpz / Front:", buff );

StringCopy ( buff, xyznames[ ozindex [ 0 ] ] );
for ( int i = 1; i < (int) ozindex; i++ )
    StringAppend ( buff, ", ", xyznames[ ozindex [ i ] ] );
Verbose.Put ( "Electrode(s) for Oz / Back  :", buff );


Verbose.NextLine ();
Verbose.Put ( "Convergence method:", "Least Squares of distance between electrodes" );
Verbose.Put ( "Force symmetric output:", true );
Verbose.Put ( "Setting origin:", centercz ? "Below the vertex" : "Center of Fpz-Oz axis" );
Verbose.Put ( "Setting orientation:", "RAS" );


//Verbose.NextLine ( 2 );
}


Verbose.Flush ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // past 3, the xyz does not really change,
                                        // though at 7/8/9 the average distance drops (better)
int                 numaveraging    = 3; // GetValue ( "Number of averaging repetitions:", AveragingXyzTitle );
int                 gaugemax        = ( numaveraging + 3 ) * numxyz + 2;

TSuperGauge         Gauge ( "Templatizing Electrodes", gaugemax );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Coregister sets 1..n-1 onto set 0

                                        // the xyz used as a first template does not really matter, except for a slight difference in global scaling
int                 ontoset         = 0; // GetValue ( "Which xyz to use as first template:", AveragingXyzTitle ) - 1;


TArray1<TPoints>    xyzpointsnew ( numxyz );
double              precision       = GODefaultConvergence;


for ( int i = 0; i < numxyz; i++ ) {

    Gauge.Next ();

//  StringCopy      ( newfile, filesin[ i ] );
//  RemoveExtension ( newfile );
//  StringAppend    ( newfile, ".Coreg 0.xyz" );

    xyzpointsnew[ i ]   = xyzpoints[ i ];

    if ( i != ontoset )

        CoregisterXyzToXyz  (   xyzpointsnew[ i ],  xyzpoints[ ontoset ], 
                                FitPointsOnPointPairsMethod,    MatchingPairs,      precision, 
                                1,                  true,
                                0,                  0 // &xyznames,          newfile
                            );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) First template is the average of all XYZ coregistered on the first one
TPoints             xyztemplate;


//StringCopy      ( newfile, filesin[ 0 ] );
//RemoveFilename  ( newfile );
//StringAppend    ( newfile, "\\Average 0.xyz" );

AveragePoints   ( xyzpointsnew, numxyz, oktoaverage, xyztemplate, 0, 0 /*&xyznames, newfile*/ );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TPoints             xyzaverage      = xyztemplate;
int                 avgi            = 0;

                                        // repeat a few times some rigid-body coregistration / intermediate template
do {
                                        // 3) Coregister all sets to template
    for ( int i = 0; i < numxyz; i++ ) {

        Gauge.Next ();

//      StringCopy      ( newfile, filesin[ i ] );
//      RemoveExtension ( newfile );
//      StringAppend    ( newfile, ".Coreg ", IntegerToString ( avgi + 1 ), ".xyz" );

        xyzpointsnew[ i ]   = xyzpoints[ i ];

        CoregisterXyzToXyz  (   xyzpointsnew[ i ],  xyzaverage, 
                                FitPointsOnPointPairsMethod,    MatchingPairs,      precision, 
                                1,                  true,
                                0,                  0 // &xyznames, newfile
                            );
        }


                                        // 4) Average to template
//  StringCopy      ( newfile, filesin[ 0 ] );
//  RemoveFilename  ( newfile );
//  StringAppend    ( newfile, "\\Average ", IntegerToString ( avgi + 1 ), ".xyz" );

    AveragePoints   ( xyzpointsnew, numxyz, oktoaverage, xyzaverage, 0, 0 /*&xyznames, newfile*/ );

    } while ( avgi++ < numaveraging );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we have a new template, so to be 100% correct, we need to coregister one last time to it
for ( int i = 0; i < numxyz; i++ ) {

    Gauge.Next ();

//  StringCopy      ( newfile, filesin[ i ] );
//  RemoveExtension ( newfile );
//  StringAppend    ( newfile, ".Coreg ", IntegerToString ( avgi + 1 ), ".xyz" );

    xyzpointsnew[ i ]   = xyzpoints[ i ];

    CoregisterXyzToXyz  (   xyzpointsnew[ i ],  xyzaverage, 
                            FitPointsOnPointPairsMethod,    MatchingPairs,      precision, 
                            1,                  true,
                            0,                  0 // &xyznames, newfile
                        );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // raw average -> centered and reoriented to RAS
                                        // go through a file, so we can open the Document and access to center and the like...
TMatrix44           matrixcenterorient;


xyzaverage.WriteFile ( filexyzrawaverage, &xyznames );

CenterAndReorient    ( xyzaverage, matrixcenterorient, filexyzrawaverage );

DeleteFiles          ( filexyzrawaverage );


                                        // centers and reorients to RAS the coregistered xyz's
for ( int i = 0; i < numxyz; i++ ) {
                                        // use same transform as average!
    matrixcenterorient.Apply ( xyzpointsnew[ i ] );

//  StringCopy      ( newfile, filesin[ i ] );
//  PostfixFilename ( newfile, ".To Raw " );
//  PostfixFilename ( newfile, fileoutprefix );
//  xyzpointsnew[ i ].WriteFile ( newfile, &xyznames );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // symmetrize the points according to sagittal plane
                                        // at the expense of a slight cost in quality
                                        // note: only the average, not the subject's!
Gauge.Next ();

SymmetrizeXyz ( xyzaverage );

//xyzaverage.WriteFile ( (char *) filesout, &xyznames );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute the final reference points
TPointFloat         cz;
TPointFloat         fpz;
TPointFloat         oz;


for ( int i = 0; i < (int) czindex; i++ )
    cz     += xyzaverage[ czindex[ i ] ];
cz         /= (int) czindex;

for ( int i = 0; i < (int) fpzindex; i++ )
    fpz    += xyzaverage[ fpzindex[ i ] ];
fpz        /= (int) fpzindex;

for ( int i = 0; i < (int) ozindex; i++ )
    oz     += xyzaverage[ ozindex[ i ] ];
oz         /= (int) ozindex;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Next ();

                                        // Set new center & rotate Fpz-Oz to ~ horizontal
                                        // There are no perfect method to do that, hence the current 3 below...
TMatrix44           mat;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Method 1: approximate Fpz-Oz, but central origin and Cz exactly above
/*TPointFloat         center;

                                        // set origin to Cz
mat.Translate ( -cz.X, -cz.Y, -cz.Z, MultiplyLeft );
                                        // set center as middle of Front-Back axis, with Cz as origin
center      = ( fpz + oz ) / 2 - cz;
                                        // force (project) to symmetrical plane
center.X    = 0;

                                        // rotate to have the center aligned below Cz (on the Z axis)
mat.RotateX ( RadiansToDegrees ( - asin ( center.Y / center.Norm () ) ), MultiplyLeft );

                                        // transform center again
center      = ( fpz + oz ) / 2;
                                        // (includes the initial translation to Cz)
mat.Apply ( center );
                                        // final translation to new center
mat.Translate ( -center.X, -center.Y, -center.Z, MultiplyLeft );
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Method 2: center is middle of Fpz-Oz axis, which gets exactly horizontal
                                        // but vertex might not be exactly above center
TPointFloat         center      = ( fpz + oz ) / 2;
                                        // set origin to middle of axis
mat.Translate ( -center.X, -center.Y, -center.Z, MultiplyLeft );

mat.Apply ( fpz );
                                        // rotate to have the center aligned below Cz (on the Z axis)
mat.RotateX ( RadiansToDegrees ( - asin ( fpz.Z / fpz.Norm () ) ), MultiplyLeft );

                                        // optional vertex centering above middle
if ( centercz ) {

    mat.Apply ( cz );

    mat.Translate ( 0, -cz.Y, 0, MultiplyLeft );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Method 3: Set center to BFS - this is quite nice already, but will lose any original alignment
/*                                      // center as center of BFS
TFitModelOnPoints   goxyz    ( xyzaverage );

TBoundingBox<double>    bounding        = goxyz.Bounding;

goxyz.Set ( GOStepsDefault );

                                        // 4 parameters model
                                        // scaling
goxyz.AddGroup ();
goxyz.AddDim   ( Scale, bounding.Radius () * 0.75, bounding.Radius () * 1.25 );
                                    // translations
goxyz.AddGroup ();
goxyz.AddDim   ( TranslationY, -bounding.GetRadius ( 1 ) * 0.20, bounding.GetRadius ( 1 ) * 0.20 );
goxyz.AddDim   ( TranslationZ, -bounding.GetRadius ( 2 ) * 0.20, bounding.GetRadius ( 2 ) * 0.20 );


goxyz.GetSolution   (   FitModelPointsMethod,   FitModelPointsHow, 
                        GODefaultConvergence,   0, 
                        "BFS Center"
                    );


TPointFloat         bfstranslate    = goxyz.GetTranslation ();


mat.Translate ( bfstranslate.X, bfstranslate.Y, bfstranslate.Z, MultiplyLeft );

                                        // Now adjust X rotation
mat.Apply ( fpz );
mat.Apply (  oz );
mat.Apply (  cz );

                                        // we have 3 angles at hand
double              rotxfpz         =                    ArcTangent ( fpz.Z, fpz.Y );           // !we need signed values to get meaningful results in 0..2pi - so not using the norm!
double              rotxoz          = NormalizeAngle ( - ArcTangent (  oz.Z, -oz.Y ), TwoPi );
//double            rotxcz          = NormalizeAngle ( - ArcTangent (  cz.Y,  cz.Z ), TwoPi );

//double            rotx            = ( rotxfpz + rotxoz + rotxcz ) / 3;    // average of the 3 estimates
double              rotx            = ( rotxfpz + rotxoz ) / 2;             // more useful for later coregistration(?)

//DBGV4 ( RadiansToDegrees ( rotxfpz ), RadiansToDegrees ( rotxoz ), RadiansToDegrees ( rotxcz ), RadiansToDegrees ( rotx ), "rotxfpz, rotxoz, rotxcz -> rotx" );

mat.RotateX ( RadiansToDegrees ( -rotx ), MultiplyLeft );
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // finally apply global re-alignment transform
mat.Apply ( xyzaverage );

for ( int i = 0; i < numxyz; i++ )

    mat.Apply ( xyzpointsnew[ i ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // write final file(s)
xyzaverage.WriteFile ( filexyzfinal, &xyznames );


//for ( int i = 0; i < numxyz; i++ ) {
//    StringCopy      ( buff, filesin[ i ] );
//    PostfixFilename ( buff, ".To " );
//    PostfixFilename ( buff, fileoutprefix );
//
//    xyzpointsnew[ i ].WriteFile ( buff, &xyznames );
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 moresubjects    = 2;
int                 moretracks      = 2;
int                 avgxyzi         = numxyz;
int                 dispi           = numxyz + 1;
int                 avgeli          = numpoints;
//int                 sdeli           = numpoints + 1;
int                 coeffveli       = numpoints + 1;
TArray2<double>     dist ( numxyz + moresubjects, numpoints + moretracks );
TEasyStats          stats;

                                        // compute the distance between subjects and the average
for ( int i = 0; i < numxyz; i++ )
for ( int p = 0; p < numpoints; p++ )

    dist ( i, p )       = oktoaverage ( i, p ) ? ( xyzpointsnew[ i ][ p ] - xyzaverage[ p ] ).Norm () : -1;   // -1 for missing value!

                                        // average & SD of all subjects, per electrode
for ( int p = 0; p < numpoints; p++ ) {

    stats.Reset ();

    for ( int i = 0; i < numxyz; i++ )
        if ( dist ( i, p ) >= 0 )
            stats.Add ( dist ( i, p ) );

    dist ( avgxyzi, p )     = stats.Average ();
//  dist ( sdi , p )        = stats.SD ();
    }

                                        // then the dispersion across subjects
for ( int p = 0; p < numpoints; p++ ) {

    stats.Reset ();

    for ( int i1 = 0; i1 < numxyz; i1++ ) {
        if ( ! oktoaverage ( i1, p ) )
            continue;

        for ( int i2 = i1 + 1; i2 < numxyz; i2++ ) {
            if ( ! oktoaverage ( i2, p ) )
                continue;
                                        // average distance within group
            stats.Add ( ( xyzpointsnew[ i1 ][ p ] - xyzpointsnew[ i2 ][ p ] ).Norm () );
            }
        }

    dist ( dispi, p )       = stats.Average ();
    }

                                        // Finally, average everything for all electrodes, including the computed variables
for ( int i = 0; i < dist.GetDim1 (); i++ ) {

    stats.Reset ();

    for ( int p = 0; p < numpoints; p++ )
        if ( dist ( i, p ) >= 0 )
            stats.Add ( dist ( i, p ) );

    dist ( i, avgeli )      = stats.Average ();
//  dist ( i, sdeli  )      = stats.SD      ();
    dist ( i, coeffveli )   = stats.CoefficientOfVariation ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // write to track file
TExportTracks       experror;

StringCopy   ( experror.Filename, fileerror );

//experror.Write ( dist );

experror.SetAtomType ( AtomTypeScalar );
experror.NumTracks       = numpoints;
//experror.NumTracks       = numpoints + moretracks;
experror.NumTime         = numxyz    + moresubjects;

                                        // distance of each subject to the template, 1 subject / TF
for ( int i = 0; i < experror.NumTime; i++ )
for ( int p = 0; p < experror.NumTracks; p++ )

    experror.Write ( dist ( i, p ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Verbose.NextBlock ();

Verbose.NextTopic ( "Results:" );
{
Verbose.NextLine ( 1 );
(ofstream&) Verbose << "Distance of each electrode to the final average, for each file." << NewLine;
(ofstream&) Verbose << "2 more columns  for the average and coefficient of variation of the electrode distances for each subject." << NewLine;
(ofstream&) Verbose << "2 more rows for the average of distance, and the dispersion distance of each electrode." << NewLine;
Verbose.NextLine ( 1 );


Verbose.ResetTable ();

                                        // field names
Verbose.TableColNames.Add ( "Subject" );

for ( int p = 0; p < numpoints; p++ )
    Verbose.TableColNames.Add ( xyznames[ p ] );

Verbose.TableColNames.Add ( "SubjAverage" );
Verbose.TableColNames.Add ( "SubjCoeffVar" );


for ( int i = 0; i < numxyz; i++ )
    Verbose.TableRowNames.Add ( ToFileName ( filesin[ i ] ) );

Verbose.TableRowNames.Add ( "ElecAverage" );
Verbose.TableRowNames.Add ( "ElecDispersion" );


Verbose.BeginTable ( Verbose.TableColNames.GetMaxStringLength () + 2 );

                                        // data & stats
for ( int i = 0; i < numxyz + moresubjects; i++ ) {

    if ( i == numxyz )
        Verbose.NextLine ();

    for ( int p = 0; p < numpoints; p++ )
        Verbose.PutTable ( dist ( i, p ), 2 );

    Verbose.PutTable ( dist ( i, avgeli    ), 2 );
    Verbose.PutTable ( dist ( i, coeffveli ), 2 );
    }


Verbose.EndTable ();

Verbose.Flush ();
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

(ofstream&) Verbose << StreamFormatFixed << StreamFormatLeft;


double              maxdist         = 2 * dist ( avgxyzi, avgeli );
double              maxcoeff        = 0.80; // increase this if average error is getting lower, or use SD instead(?)
bool                firsterror      = true;

                                        // do some final checks, and warn the user
for ( int i = 0; i < numxyz; i++ ) {
                                        // compare the average electrodes of each subject, to the average of average
//  DBGV4 ( i + 1, dist ( i, avgeli ), maxdist, dist ( i, avgeli ) <= maxdist, "#subject avgdist maxdist -> OK" );
//  DBGV4 ( i + 1, dist ( i, coeffveli ), maxcoeff, dist ( i, coeffveli ) <= maxcoeff, "#subject avgcoeff maxcoeff -> OK" );

    if ( dist ( i, avgeli ) > maxdist ) {

        ShowMessage (   "This file seems to not fit correctly with all the others!" NewLine 
                        "You might consider removing it from your list and proceed again...", 
                        ToFileName ( filesin[ i ] ), ShowMessageWarning );

        if ( firsterror )
            Verbose.NextTopic ( "Errors:" );
        firsterror  = false;

        Verbose.Put ( "Dubious file:", filesin[ i ] );
        Verbose.Put ( "Reason:", "Globally, all electrodes seem very far from the template" );
        Verbose.Put ( "Average electrodes distance of file:", dist ( i, avgeli ) );
        Verbose.Put ( "Max average distance allowed       :", maxdist );
        Verbose.NextLine ( 2 );
        }

                                        // look if distances within subject are too spread off -> some bad electrodes
    if ( dist ( i, coeffveli ) > maxcoeff ) {

        ShowMessage (   "It seems this file has some very badly placed electrodes!" NewLine 
                        "You might consider removing it from your list and proceed again...", 
                        ToFileName ( filesin[ i ] ), ShowMessageWarning );

        if ( firsterror )
            Verbose.NextTopic ( "Errors:" );
        firsterror  = false;

        Verbose.Put ( "Dubious file:", filesin[ i ] );
        Verbose.Put ( "Reason:", "Locally, some electrodes are very badly coregistered" );
        Verbose.Put ( "Coefficient of Variation of distances:", dist ( i, coeffveli ) );
        Verbose.Put ( "Max Coefficient of Variation allowed :", maxcoeff );
        Verbose.NextLine ( 2 );
        }
    }


(ofstream&) Verbose << StreamFormatGeneral << StreamFormatLeft;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OptimizeOn

}
