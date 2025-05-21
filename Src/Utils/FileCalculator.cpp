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

#include    "FileCalculator.h"

#include    "Strings.Utils.h"
#include    "Strings.TFixedString.h"
#include    "Files.Utils.h"
#include    "Files.TVerboseFile.h"
#include    "Files.TGoF.h"

#include    "TBaseDialog.h"             // EditSizeTextLong
#include    "TParser.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    FileCalculator  (   const char*     expr,
                            const TGoGoF&   gogof,
                            const char*     regularization,
                            const char*     basedir,    const char*     fileext,    bool            compoundfilenames
                        )
{
if ( StringIsEmpty ( expr    ) 
  || StringIsEmpty ( basedir ) 
  || StringIsEmpty ( fileext ) 
  || gogof.IsEmpty () )

    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // too radical?
//NukeDirectory ( basedir );

                                        // re-create directory
CreatePath ( basedir, false );

                                        // delete any previous files, with my prefix only!
//StringCopy ( buff, basedir );
//AppendFilenameAsSubdirectory ( buff );
//StringAppend ( buff, "*" );
//DBGM ( buff, "Deleting files" );
//DeleteFiles ( buff );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check which groups are actually used!
TFileName           buff;
int                 numgroups       = gogof.NumGroups ();
TSelection          selgroups ( numgroups, OrderSorted );

selgroups.Reset ();


for ( int gofi = 0; gofi < (int) gogof; gofi++ ) {

    StringCopy  ( buff, "Group", IntegerToString ( gofi + 1 ) );

    selgroups.Set ( gofi, (bool) StringContains ( expr, buff ) );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // fill variables with the current groups
TParser             parser;
TTokensStack        variables;
TGoF*               gof;

                                        // restrict ourselves to these groups
for ( TIteratorSelectedForward gi ( selgroups ); (bool) gi; ++gi ) {

    gof         = const_cast<TGoF*> ( &gogof[ gi() ] ); // yes, we shouldn't, see comment below

    StringCopy  ( buff, "Group", IntegerToString ( gi() + 1 ) );
                                        // add a variable for each group
                                        // Note: we should be providing some "const TGoF*" but TTokenVariable does not store 
                                        // both const (input files) and non-const (temp files) groups of files.
                                        // Evaluating  the expression should not modify these guys anway.
    variables.Push ( new TTokenVariable ( buff, gof, regularization ) );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // allocate a set of variables of different types to hold the results
float               resultf;
TVector3Float       resultv;
TGoF                result;
TFileName           resultdir;


variables.Push ( new TTokenVariable ( "resultf", &resultf ) );
variables.Push ( new TTokenVariable ( "resultv", &resultv ) );
variables.Push ( new TTokenVariable ( "result",  &result, 0, fileext ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           VerboseFile;

StringCopy                      ( VerboseFile, basedir );
AppendFilenameAsSubdirectory    ( VerboseFile );
AddExtension                    ( VerboseFile, FILEEXT_VRB );

CheckNoOverwrite                ( VerboseFile );


TVerboseFile    verbose ( VerboseFile, VerboseFileDefaultWidth );

verbose.PutTitle ( "File Calculator" );


verbose.NextTopic ( "Calculating:" );

verbose.Put ( "Expression:", expr );
if ( StringIsNotEmpty ( regularization ) )
    verbose.Put ( "Inverse Solution Regularization:", regularization );


verbose.NextTopic ( "Input Files:" );

verbose.Put ( "Number of group(s) used:", (int) selgroups );
verbose.NextLine ();

for ( TIteratorSelectedForward gi ( selgroups ); (bool) gi; ++gi ) {

    if ( gi.GetIndex () > 0 )
        verbose.NextLine ();

    verbose.Put ( "Group #:", gi() + 1 );

    const TGoF&         gof         = gogof[ gi() ];

    for ( int fi = 0; fi < (int) gof; fi++ )
        verbose.Put ( fi ? "" : "File(s):", gof[ fi ] );
    }


verbose.NextTopic ( "Output Files:" );

verbose.Put ( "Verbose File (this):", VerboseFile );
verbose.NextLine ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // adding an assignation to the expression if it is missing - otherwise results will not be retrievable..
char                checkedexpr[ EditSizeTextLong ];

if ( StringContains ( (const char*) expr, "result" ) )  StringCopy  ( checkedexpr,              expr );
else                                                    StringCopy  ( checkedexpr, "result = ", expr );

                                        // go!
if ( ! parser.Evaluate ( checkedexpr, variables, true ) )
    goto Finished;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // save final results directory
StringCopy      ( resultdir, result[ 0 ] );
RemoveFilename  ( resultdir );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // moving / renaming resulting files
if ( compoundfilenames ) {
                                        // already compounded file names
                                        // move & rename result files, including buddy files from BrainVision
    result.CopyFilesTo ( basedir, (CopyToFlags) ( CopyAndDeleteOriginals | CopyAndUpdateFileNames | CopyAllKnownBuddies ) );


    for ( int fi = 0; fi < (int) result; fi++ )
        verbose.Put ( fi ? "" : "Final file name(s):", result[ fi ] );
    }
else {                                  // create a new list of the renamed files
    TGoF                renamed;
    TFileName           dir;
    TFileName           filename;

    StringCopy                      ( dir, basedir );
    CreateSubdirectoryFromFilename  ( dir );

    for ( int i = 1; i <= (int) result; i++ ) {

        StringCopy  ( filename, dir, ".", IntegerToString ( i, 4 ), ".", fileext );

        renamed.Add ( filename );
        }

                                        // move & rename result files, including buddy files from BrainVision
    result.CopyFilesTo ( renamed, (CopyToFlags) ( CopyAndDeleteOriginals | CopyAllKnownBuddies ) );

                                        // give a hint of how the renaming was done!
    for ( int fi = 0; fi < (int) result; fi++ )
        verbose.Put ( fi ? "" : "Temporary file name(s):", ToFileName ( result[ fi ] ) );

    verbose.NextLine ( 2 );

    for ( int fi = 0; fi < (int) renamed; fi++ )
        verbose.Put ( fi ? "" : "Final file name(s):", renamed[ fi ] );
    }


verbose.NextLine ( 2 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // delete last temp directory
NukeDirectory   ( resultdir );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Finished:

variables.Reset ( true );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
