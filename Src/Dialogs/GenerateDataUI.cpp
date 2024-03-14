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

#include    "Math.Utils.h"
#include    "Dialogs.Input.h"
#include    "Geometry.TPoints.h"

#include    "TLeadField.h"
#include    "TTracks.h"

#include    "GenerateData.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    TCartoolMdiClient::GenerateDataUI ( owlwparam w )
{
int                 nummapsmin;
int                 nummapsmax;
double              correlationmin;
double              correlationmax;
double              correlationstep;
int                 numsources;
double              mapnoisemin;
double              mapnoisemax;
double              mapnoisestep;
int                 numfiles;
int                 segdurationmin;
int                 segdurationmax;
int                 fileduration;
GenerateSegmentFlag typesegment;
bool                normalize;
TFileName           basefilename;
char                buff[ 256 ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GenerateTypeFlag    what            = w == CM_GENERATEDATAEEG ? GenerateEeg
                                    : w == CM_GENERATEDATARIS ? GenerateRis
                                    :                           GenerateUnknown;

if ( what == GenerateUnknown )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! GetInputFromUser (   "Number of clusters - single value or two values for min/max:", 
                            GenerateDataTitle, buff, "3 15", this ) )
    return;

int         numscanf    = sscanf ( buff, "%ld %ld", &nummapsmin, &nummapsmax );

if      ( numscanf <= 0 )   return;
else if ( numscanf == 1 )   nummapsmax = nummapsmin;


Maxed       ( nummapsmin, 1 );
Maxed       ( nummapsmax, 1 );
CheckOrder  ( nummapsmin, nummapsmax );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get a range of positive correlation, and a step
                                        // The correlation across maps will be constant within that range
                                        // In case of !ignorepolarities, we allow maps to be either +corr or -corr
if ( ! GetInputFromUser (   "Correlation between clusters - single value or three values for min/max/step:", 
                            GenerateDataTitle, buff, "10 90 20", this ) )
    return;

numscanf    = sscanf ( buff, "%lf %lf %lf", &correlationmin, &correlationmax, &correlationstep );

if      ( numscanf <= 0 )   return;
else if ( numscanf == 1 ) { correlationmax = correlationmin;   correlationstep = 0; }
else if ( numscanf == 2 )   return;


correlationmin      = fabs ( correlationmin  ) / 100;
correlationmax      = fabs ( correlationmax  ) / 100;
correlationstep     = fabs ( correlationstep ) / 100;

Clipped     ( correlationmin, correlationmax,  0.0,     0.99 );
Clipped     ( correlationstep,                 0.001,   1.0  );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! GetInputFromUser (   "Noise levels in [%] - single value or three values for min/max/step:", 
                            GenerateDataTitle, buff, "0 100 10", this ) )
    return;

numscanf    = sscanf ( buff, "%lf %lf %lf", &mapnoisemin, &mapnoisemax, &mapnoisestep );

if      ( numscanf <= 0 )   return;
else if ( numscanf == 1 ) { mapnoisemax = mapnoisemin;     mapnoisestep = 0; }
else if ( numscanf == 2 )   return;


mapnoisemin    /= 100;
mapnoisemax    /= 100;
mapnoisestep   /= 100;

Clipped     ( mapnoisemin,  mapnoisemax, 0.0,  1.0 );
Clipped     ( mapnoisestep,              0.01, 1.0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                answer          = GetOptionFromUser (   "Type of segments:" Tab "- (S)eed maps only" NewLine 
                                                            Tab Tab             Tab "- (C)onstant segments" NewLine 
                                                            Tab Tab             Tab "- (T)ransitioning segments" NewLine 
                                                            Tab Tab             Tab "- (O)verlapping segments", 
                                                            GenerateDataTitle, "S C T O", "O", this );
if ( answer == EOS )   return;

typesegment = answer == 'S' ? SeedMaps
            : answer == 'C' ? ConstantSegments
            : answer == 'T' ? HanningSegments
            : answer == 'O' ? LeakySegments
            :                 ConstantSegments;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                cyclicsegs      = false;


if ( typesegment == SeedMaps ) {
                                        // not relevant for this case
    segdurationmin  = 1;
    segdurationmax  = 1;
    fileduration    = 1;

    normalize       = true;
    }
else {
    GetInputFromUser    (   "Segments min and max (opt) duration, and file duration (opt), in [TF]:", 
                            GenerateDataTitle, buff, 
                            typesegment == ConstantSegments ? "1 1 300" : "5 10 500", this );

    numscanf    = sscanf ( buff, "%ld %ld %ld", &segdurationmin, &segdurationmax, &fileduration );

    if      ( numscanf <= 0 )   return;
    else if ( numscanf == 1 ) { segdurationmax = segdurationmin;   fileduration = 20 * segdurationmin; }
    else if ( numscanf == 2 )   fileduration = 20 * ( segdurationmin + segdurationmax ) / 2;


    Maxed       ( segdurationmin, 1 );
    Maxed       ( segdurationmax, 1 );
    Maxed       ( fileduration,   1 );
    CheckOrder  ( segdurationmin, segdurationmax );

    {
    char                answer          = GetOptionFromUser (   "Power of segments:" Tab "- (N)ormalizing each map" NewLine 
                                                                Tab Tab              Tab "- (R)andom power per segment", 
                                                                GenerateDataTitle, "N R", "N", this );
    if ( answer == EOS )   return;

    normalize       = answer != 'R';
    }

    {
    char                answer          = GetOptionFromUser (   "Sequence of segments:" Tab "- (R)andom order" NewLine 
                                                                Tab Tab                 Tab "- (C)yclic order", 
                                                                GenerateDataTitle, "R C", "R", this );
    if ( answer == EOS )   return;

    cyclicsegs      = answer == 'C';
    }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TLeadField          leadfield;
TTracks<float>      K;
TPoints             solp;
TFileName           leadfieldfile;
TFileName           spfile;


if      ( what == GenerateEeg ) {

    GetFileFromUser     getlffile ( "Opening Lead Field file:", AllLeadFieldFilesFilter, 1, GetFileRead );

    if ( ! getlffile.Execute () || (int) getlffile < 1 )
        return;
    else 
        StringCopy ( leadfieldfile, getlffile[ 0 ] );

                                            // try opening & check
    if ( ! leadfield.Set ( leadfieldfile ) ) {
        ShowMessage (   "There seems to be a problem with the Lead Field file, which can be either:" NewLine 
                        Tab "- The file type is not recognized" NewLine 
                        Tab "- The Lead Field data type is not of vectorial type" NewLine 
                        Tab "- The file itself might be somehow corrupted" NewLine NewLine 
                        "This will end your processing now, bye bye...", ToFileName ( leadfieldfile ), ShowMessageWarning );
        return;
        }

                                            // try retrieving the matrix & check
    if ( ! leadfield.ReadFile ( K ) ) {
        ShowMessage (   "There seems to be a problem with the Lead Field file," NewLine 
                        " check the file type is correct." NewLine NewLine 
                        "This will end your processing now, bye bye...", ToFileName ( leadfieldfile ), ShowMessageWarning );
        return;
        }
    }

else if ( what == GenerateRis ) {

    GetFileFromUser     getspfile ( "Opening Solution Points file:", AllSolPointsFilesFilter, 1, GetFileRead );

    if ( ! getspfile.Execute () || (int) getspfile < 1 )
        return;
    else 
        StringCopy ( spfile, getspfile[ 0 ] );

                                            // try opening & check
    solp.ReadFile ( spfile );

    if ( solp.IsEmpty () ) {
        ShowMessage (   "There seems to be a problem with the Solution Points file!" NewLine NewLine 
                        "This will end your processing now, bye bye...", ToFileName ( spfile ), ShowMessageWarning );
        return;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! GetValueFromUser (   "Number of dipole sources:", 
                            GenerateDataTitle, numsources, "3", this ) )
    return;

Clipped     ( numsources, 1, 10 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // actually a flag for spontaneous vs ERP
bool                ignorepolarity  = true;

if ( what == GenerateEeg )

    ignorepolarity      = GetAnswerFromUser (   "Ignore polarity results, like spontaneous data?", 
                                                GenerateDataTitle );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Used to assess best clustering criteria / meta-crierion
bool                runsegmentation     = false;
int                 numrandtrials       = 0;

//if ( runsegmentation ) {
//
//    if ( ! GetValueFromUser (   "Clustering: Number of random trials per individual segmentation (0 or <esc> to skip this step):", 
//                                GenerateDataTitle, numrandtrials, "20", this )
//        || numrandtrials <= 0 ) {
//
//        runsegmentation = false;
//        numrandtrials   = 0;
//        }
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! GetValueFromUser (   "Number of output files, per cluster/correlation/noise level:", 
                            GenerateDataTitle, numfiles, "30", this ) )
    return;

Maxed       ( numfiles, 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GetFileFromUser     getbasefile ( "Output directory:", AllFilesFilter, 1, GetFileDirectory );

if ( ! getbasefile.Execute ( what == GenerateEeg ? (const char*) leadfieldfile 
                           : what == GenerateRis ? (const char*) spfile 
                           :                       (const char*) 0              ) || (int) getbasefile < 1 )
    return;
else 
    StringCopy ( basefilename, getbasefile[ 0 ] );

                                        // force 1 common output directory
//StringCopy ( basefilename, "E:\\Data\\Segmentation\\SynthData\\SynthData" );

                                        // not a good idead for concurrent computing
//TFileName           cleaningfiles;
//                                        // delete any previous files, with my prefix only!
//StringCopy  ( cleaningfiles,    basefilename,     "\\",   ToFileName ( basefilename ), "*" );
//
//DeleteFiles ( cleaningfiles );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Does all the computation and saving
GenerateData    (   what,
                    nummapsmin,     nummapsmax,
                    correlationmin, correlationmax, correlationstep,
                    mapnoisemin,    mapnoisemax,    mapnoisestep,
                    segdurationmin, segdurationmax,
                    typesegment,
                    cyclicsegs,
                    ignorepolarity,
                    normalize,
                    leadfield,      K,              numsources,
                    solp,
                    numfiles,       fileduration, 
                    basefilename,   
                    true /*typesegment != SeedMaps*/,       true,           false,  // NOT saving ris
                    runsegmentation, numrandtrials 
                );

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
