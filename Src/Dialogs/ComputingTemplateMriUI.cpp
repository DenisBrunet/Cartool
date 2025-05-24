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

#include    <owl/pch.h>

#include    "Files.TFileName.h"
#include    "Files.TGoF.h"
#include    "TVolume.h"                 // NeighborhoodType
#include    "Dialogs.Input.h"

#include    "ComputingTemplateMri.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

OptimizeOff

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void    TCartoolMdiClient::ComputingTemplateMriUI ()
{
char                answer          = GetOptionFromUser (   "Template geometrical reference:"   NewLine
                                                            NewLine
                                                            Tab "- (M)NI brain template"        NewLine
                                                            Tab "- (S)elf-referenced template",
                                                            TemplateMriTitle, "M S", "M" );
if ( answer == EOS )   return;


BuildTemplateType   howtemplate     = answer == 'M' ? BuildTemplateMNI
                                    : answer == 'S' ? BuildTemplateSelfRef
                                    :                 BuildTemplateMNI;

//BuildTemplateType   howtemplate     = BuildTemplateMNI;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static GetFileFromUser  getfiles ( "Give all brains to be templatized:", AllMriFilesFilter, 1, GetFileMulti );
TGoF                mrifiles;


if ( ! getfiles.Execute () )
    return;


mrifiles    = (TGoF&) getfiles;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need a template brain to boot the initial template
TFileName           mnifile;

static GetFileFromUser  getmnifile ( "Provide the reference MNI Brain:", AllMriFilesFilter, 1, GetFileRead );


if ( howtemplate == BuildTemplateMNI ) {

    if ( ! getmnifile.Execute () )
        return;

    mnifile     = getmnifile[ 0 ];
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*char*/            answer          = GetOptionFromUser (   "Method for computing the Solution Points:"             NewLine
                                                            NewLine
                                                            Tab "- (L)oading some existing Solution Points file"    NewLine
                                                            Tab "- (E)xtracting Solution Points from new Template"  NewLine
                                                            Tab "- (N)o Solution Points needed, thanks",
                                                            TemplateMriTitle, "N L E", howtemplate == BuildTemplateMNI ? "L" : "E" );

if ( answer == EOS )   return;


TemplateSPType      howsp           = answer == 'N' ? TemplateNoSP
                                    : answer == 'E' ? TemplateExtractSPFromTemplate
                                    : answer == 'L' ? TemplateLoadSPFromFile
                                    :                 TemplateExtractSPFromTemplate;


TFileName           mnispfile;

static GetFileFromUser  getspfile ( "MNI Solution Points file:", AllSolPointsFilesFilter, 0, GetFileRead );


if ( howsp == TemplateLoadSPFromFile ) {

    if ( ! getspfile.Execute () )
        return;

    mnispfile   = getspfile[ 0 ];
    } // if TemplateLoadSPFromFile


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Solution Points options
int                 numsolpoints    = 0;
double              ressolpoints    = 0;


if ( howsp == TemplateExtractSPFromTemplate ) {
    
    double          v;

    if ( ! GetValueFromUser ( "Number of solution points, or resolution in [mm]:", TemplateMriTitle, v, "" ) )
        return;

    if      ( v <= 0 )  return;
    else if ( v > 50 )  numsolpoints    = Round ( v );
    else                ressolpoints    = v;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Template options
                    answer              = GetOptionFromUser ( "Compute a (S)ymmetrical or (A)symmetrical template?" , 
                                                              TemplateMriTitle, "S A", "A" );

if ( answer == EOS )   return;


bool                templatesymmetrical = answer == 'S';


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Grey mask options
GreyMatterFlags     greyflagspreproc    = GreyMatterNothing;

GreyMatterFlags     greyflagsproc       = /*GreyMatterFat*/ GreyMatterRegular;

GreyMatterFlags     greyflagspostproc;

GreyMatterFlags     greyflags;

                                        // even if not used, set these parameters
greyflagspostproc   = answer == 'S' ? GreyMatterSymmetric
                    : answer == 'A' ? GreyMatterAsymmetric
                                    : GreyMatterSymmetric;

greyflagspostproc   = GreyMatterFlags ( greyflagspostproc | GreyMatterPostprocessing );

greyflags           = GreyMatterFlags ( greyflagspreproc | greyflagsproc | greyflagspostproc );


GreyMatterFlags     spflagspostproc     = greyflagspostproc;    // same as grey - doen't care about GreyMatterPostprocessing

GreyMatterFlags     spflagscheck        = GreyMatterFlags ( GreyMatterLauraCheck | GreyMatterSinglePointCheck );

GreyMatterFlags     spflags             = GreyMatterFlags ( spflagspostproc | spflagscheck );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Convergence options
double              precision;

if ( ! GetValueFromUser ( "Precision for convergence, from high (1e-6) to low (1e-2):", TemplateMriTitle, precision, "1e-6" ) )
    return;

precision       = NoMore ( fabs ( precision ), 1e-2 );

                                          
int                 numiterations   = howtemplate == BuildTemplateSelfRef ? 1   // compute rigid average, then coregistering a second time on it, with full affine transform
                                                                          : 0;  // coregistered full affine directly to MNI brain

bool                savingcoregmris     = GetAnswerFromUser ( "Saving every individually coregistered MRIs?", TemplateMriTitle );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                fileprefix[ 256 ];


StringCopy ( fileprefix, howtemplate == BuildTemplateMNI ? "MNI" InfixTemplate : "Self" InfixTemplate );

if ( ! GetInputFromUser ( "Prefix for output files:", TemplateMriTitle, fileprefix, fileprefix, 0 ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ComputingTemplateMri    (   mrifiles,
                            howtemplate,
                            howsp,
                            mnifile,
                            mnispfile,
                            templatesymmetrical,
                            greyflags,
                            numsolpoints,           ressolpoints,
                            spflags,
                            precision,              numiterations,
                            fileprefix,
                            savingcoregmris
                        );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OptimizeOn

}
