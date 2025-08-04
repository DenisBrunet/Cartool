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

#include    "TParser.h"

#include    "Strings.Utils.h"
#include    "TArray1.h"
#include    "TList.h"
#include    "Files.Extensions.h"

#include    "TExportTracks.h"

#include    "TInverseMatrixDoc.h"
#include    "TRisDoc.h"

#include    "TFileCalculatorDialog.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const char          TokenTypeStrings[][ 16 ] =
                    {
                    "UnknownToken",

                    "TokenIdentifier",

                    "TokenValue",
                    "TokenVariable",
                    "TokenOperator",
                    };


const TTokenOperatorVars    LegalOperators[] = {
                    //  Name        OperatorType          Priority                  #Operands       function

                    {   "=",        OpAssign,         1 + ParenthesisPriority,          2,          false   },

                    {   "(",        OpParenthesisOpen,    ParenthesisPriority,          1,          false   },
                    {   ")",        OpParenthesisClose,   ParenthesisPriority,          1,          false   },

                    {   "Abs",      OpAbs,                                  4,          1,          true    },
                    {   "Scalar",   OpScalar,                               4,          1,          true    },
                    {   "Sqr",      OpSqr,                                  4,          1,          true    },
                    {   "Sqrt",     OpSqrt,                                 4,          1,          true    },

                    {   "*",        OpBinaryMult,                           3,          2,          false   },
                    {   "/",        OpBinaryDiv,                            3,          2,          false   },

                    {   "+",        OpBinaryPlus,                           2,          2,          false   },
                    {   "-",        OpBinaryMinus,                          2,          2,          false   },
                    };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TToken::TToken ()
{
Type            = UnknownToken;
Temp            = false;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TTokenIdentifier::TTokenIdentifier ()
{
Type            = TokenIdentifier;
Temp            = false;

ClearString ( Identifier );
}


        TTokenIdentifier::TTokenIdentifier ( const char* tokenchar )
{
Type            = TokenIdentifier;
Temp            = false;

StringCopy ( Identifier, tokenchar );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TTokenVariable::TTokenVariable ()
{
Reset ();
}


void    TTokenVariable::Reset ()
{
Type            = TokenVariable;
Temp            = false;

ClearString ( Name, TokenVariableNameLength );

Content         = UnknownContent;

DataType        = UnknownAtomType;
Size[ 0 ]       = 0;
Size[ 1 ]       = 0;
Size[ 2 ]       = 0;
Size[ 3 ]       = 0;

ClearString ( OutputExt,        TokenVariableExtLength );
ClearString ( Regularization,   TokenVariableRegLength );

DataFloat       = 0;
DataVector      = 0;
DataGof         = 0;
}


        TTokenVariable::~TTokenVariable ()
{
if ( Temp ) {

    if ( IsGroup () ) {

        if ( NumFiles () ) {

            TFileName       dir ( GetFile () );
                                        // actually nuke the temp files & directory
            RemoveFilename  ( dir );
                                        // all are in 1 directory
            NukeDirectory   ( dir );
            }
                                        // delete object itself
        delete  DataGof;
        DataGof     = 0;
        }

    if ( DataFloat ) {
        delete[]  DataFloat;
        DataFloat   = 0;
        }

    if ( DataVector ) {
        delete[]  DataVector;
        DataVector  = 0;
        }
    }
}


//----------------------------------------------------------------------------
        TTokenVariable::TTokenVariable ( const TToken* token )
{
Reset ();

if ( ! token->IsVariable () )
    return;


const TTokenVariable*   tokvar      = dynamic_cast<const TTokenVariable*> ( token );

StringCopy ( Name,  tokvar->Name );

Content         = tokvar->Content;

DataType        = tokvar->DataType;
Size[ 0 ]       = tokvar->Size[ 0 ];
Size[ 1 ]       = tokvar->Size[ 1 ];
Size[ 2 ]       = tokvar->Size[ 2 ];
Size[ 3 ]       = tokvar->Size[ 3 ];

StringCopy ( OutputExt,         tokvar->OutputExt      );
StringCopy ( Regularization,    tokvar->Regularization );

DataFloat       = tokvar->DataFloat;
DataVector      = tokvar->DataVector;
DataGof         = tokvar->DataGof;
}


//----------------------------------------------------------------------------
                                        // float variable, either single var, 1D, 2D, 3D
        TTokenVariable::TTokenVariable ( const char* name, float* tovar, int size0, int size1, int size2 )
{
Reset ();

StringCopy ( Name,  name );

Content         = ContentData;

DataType        = AtomTypeScalar;
Size[ 0 ]       = (bool) size0 + (bool) size1 + (bool) size2;
Size[ 1 ]       = size0;
Size[ 2 ]       = size1;
Size[ 3 ]       = size2;

DataFloat       = tovar;
}


//----------------------------------------------------------------------------
                                        // vector variable, either single var, 1D, 2D, 3D
        TTokenVariable::TTokenVariable ( const char* name, TVector3Float* tovar, int size0, int size1, int size2 )
{
Reset ();

StringCopy ( Name,  name );

Content         = ContentData;

DataType        = AtomTypeVector;
Size[ 0 ]       = (bool) size0 + (bool) size1 + (bool) size2;
Size[ 1 ]       = size0;
Size[ 2 ]       = size1;
Size[ 3 ]       = size2;

DataVector      = tovar;
}


//----------------------------------------------------------------------------
                                        // GOF variable, plus the size of the file content
        TTokenVariable::TTokenVariable ( const char* name, TGoF* gof, const char* regularization, const char* outputext )
{
Reset ();

StringCopy ( Name,  name );

if ( StringIsNotEmpty ( outputext ) )
    StringCopy ( OutputExt, outputext );

DataGof         = gof;

if ( ! gof || gof->NumFiles () == 0 )
    return;

                                        // now, scan the files to get their types

if      ( IsExtensionAmong ( (*gof)[ 0 ], AllEegFilesExt ) ) {

    int             numtf;
    int             numel;

    ReadFromHeader ( (*gof)[ 0 ], ReadNumTimeFrames,    &numtf );
    ReadFromHeader ( (*gof)[ 0 ], ReadNumElectrodes,    &numel );

    Content         = ContentData;

    DataType        = AtomTypeScalar;
    Size[ 0 ]       = 2;
    Size[ 1 ]       = numel;
    Size[ 2 ]       = numtf;
    Size[ 3 ]       = 0;
    }

else if ( IsExtensionAmong ( (*gof)[ 0 ], AllRisFilesExt ) ) {

    int             numtf;
    int             numsp;
    bool            isscalar;

    ReadFromHeader ( (*gof)[ 0 ], ReadNumTimeFrames,    &numtf );
    ReadFromHeader ( (*gof)[ 0 ], ReadNumSolPoints,     &numsp );
    ReadFromHeader ( (*gof)[ 0 ], ReadInverseScalar,    &isscalar );

    Content         = ContentData;

    DataType        = isscalar ? AtomTypeScalar : AtomTypeVector;
    Size[ 0 ]       = 2;
    Size[ 1 ]       = numsp;
    Size[ 2 ]       = numtf;
    Size[ 3 ]       = 0;
    }

else if ( IsExtensionAmong ( (*gof)[ 0 ], AllInverseFilesExt ) ) {

    int             numel;
    int             numsp;
    bool            isscalar;

    ReadFromHeader ( (*gof)[ 0 ], ReadNumElectrodes,    &numel );
    ReadFromHeader ( (*gof)[ 0 ], ReadNumSolPoints,     &numsp );
    ReadFromHeader ( (*gof)[ 0 ], ReadInverseScalar,    &isscalar );

    Content         = ContentMatrix;

    if ( StringIsNotEmpty ( regularization ) )
        StringCopy ( Regularization, regularization );

    DataType        = isscalar ? AtomTypeScalar : AtomTypeVector;
    Size[ 0 ]       = 2;
    Size[ 1 ]       = numsp;
    Size[ 2 ]       = numel;
    Size[ 3 ]       = 0;
    }

else // we have a problem!

    DataGof         = 0;

}


//----------------------------------------------------------------------------
void    TTokenVariable::Allocate  ( TokenAllocationType how, TTokenVariable* tokvar[], TTokenOperator* tokop )
{
                                        // called only for temp variables
Temp            = true;


if ( IsFlag ( how, AllocateMemory ) ) {

    StringCopy ( Name, "__temp" );

    Content         = ContentData;
                                        // size is the max of all operands
    Size[ 0 ]       = max ( tokvar[ 0 ]->Size[ 0 ], tokvar[ 1 ] ? tokvar[ 1 ]->Size[ 0 ] : 0 );
    Size[ 1 ]       = max ( tokvar[ 0 ]->Size[ 1 ], tokvar[ 1 ] ? tokvar[ 1 ]->Size[ 1 ] : 0 );
    Size[ 2 ]       = max ( tokvar[ 0 ]->Size[ 2 ], tokvar[ 1 ] ? tokvar[ 1 ]->Size[ 2 ] : 0 );
    Size[ 3 ]       = max ( tokvar[ 0 ]->Size[ 3 ], tokvar[ 1 ] ? tokvar[ 1 ]->Size[ 3 ] : 0 );


    if ( IsFlag ( how, AllocateFloat ) ) {

        DataType        = AtomTypeScalar;
                                        // elaborate: choose between virtual memory and heap
        DataFloat       = new float [ GetLinearDim () ];
        }
    else if ( IsFlag ( how, AllocateVector ) ) {

        DataType        = AtomTypeVector;

        DataVector      = new TVector3Float [ GetLinearDim () ];
        }

    } // memory


if ( IsFlag ( how, AllocateGroup ) ) {

    Content         = ContentData;

    DataType        = IsFlag ( how, AllocateVector ) ? AtomTypeVector : AtomTypeScalar;

    if ( IsFlag ( how, AllocateMult ) ) {
                                        // multiplication sets new dimensions
        Size[ 0 ]       = 2;
        Size[ 1 ]       = tokvar[ 0 ]->Size[ 1 ];
        Size[ 2 ]       = tokvar[ 1 ]->Size[ 2 ];
        Size[ 3 ]       = 0;

                                        // both are groups
        CreateGof ( tokvar[ 0 ]->GetFile (), tokvar[ 0 ]->NumFiles () * tokvar[ 1 ]->NumFiles () );
        }
    else {
                                        // size is the max of all operands
        Size[ 0 ]       = max ( tokvar[ 0 ]->Size[ 0 ], tokvar[ 1 ] ? tokvar[ 1 ]->Size[ 0 ] : 0 );
        Size[ 1 ]       = max ( tokvar[ 0 ]->Size[ 1 ], tokvar[ 1 ] ? tokvar[ 1 ]->Size[ 1 ] : 0 );
        Size[ 2 ]       = max ( tokvar[ 0 ]->Size[ 2 ], tokvar[ 1 ] ? tokvar[ 1 ]->Size[ 2 ] : 0 );
        Size[ 3 ]       = max ( tokvar[ 0 ]->Size[ 3 ], tokvar[ 1 ] ? tokvar[ 1 ]->Size[ 3 ] : 0 );

                                        // at least one of the var is a group
        const TTokenVariable*   tokgof  = tokvar[ 0 ]->IsGroup () ? tokvar[ 0 ] : tokvar[ 1 ];

        CreateGof ( tokgof->GetFile (), tokgof->NumFiles () );
        }

                                        // cook file names
    CompoundFilenames ( tokvar, tokop );
    } // group
}


//----------------------------------------------------------------------------
                                        // Create:
                                        // - Gof
                                        // - directory structure
                                        // - the requested amount of temp file names (names only, not the files)
bool    TTokenVariable::CreateGof ( const char* path, int numfiles )
{
if ( ! DataGof ) {
                                        // allocate a new gof
    DataGof         = new TGoF;

                                        // create a random name
    StringCopy      ( Name, "temp_" );
    StringRandom    ( StringEnd ( Name ), 8 );
    }


TFileName           newpath;
TFileName           filename;

                                        // create directory path
StringCopy      ( newpath, path );
                                        // remove last filename, then any trailing temp_ directories (avoiding nested temp dirs!)
do RemoveFilename ( newpath ); while ( StringContains ( (const char*) newpath, "temp_" ) );

StringAppend    ( newpath, "\\", Name );
                                        // delete the whole dir (if existing)
NukeDirectory   ( newpath );

CreatePath      ( newpath, false );

                                        // generate as many files names as requested
for ( int i = 0; i < numfiles; i++ ) {
                                        // set default file names
    sprintf ( filename, "%s\\%s.%0d.%s", (const char*) newpath, Name, i + 1, OutputExt );

    DataGof->Add ( filename, filename.Size () );
    }


return  true;
}


//----------------------------------------------------------------------------
                                        // cook file names according to operands and operator
void    TTokenVariable::CompoundFilenames ( TTokenVariable* tokvar[], TTokenOperator* tokop )
{
const TGoF*         gof0            = tokvar[ 0 ] && tokvar[ 0 ]->IsGroup () ? tokvar[ 0 ]->DataGof : 0;
const TGoF*         gof1            = tokvar[ 1 ] && tokvar[ 1 ]->IsGroup () ? tokvar[ 1 ]->DataGof : 0;

char                file    [ 2 * MaxPathShort ];
char                str0    [ 256 ];
char                str1    [ 256 ];
char                buff    [ 256 ];
char                ext     [ 32 ];
char                legalop [ 32 ];
long                remainlen;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // temp files:     allow to expand, but check maximum length
                                        // original files: shrink, but give a little more to Matrices
auto                MaxChars    = [ &remainlen ] ( const char* str, bool istemp, bool ismatrix )
{
return  istemp  ? AtLeast ( remainlen, StringLength ( str ) )
                : NoMore  ( 12, Round ( ismatrix ? 0.60 * StringLength ( str ) : 0.50 * StringLength ( str ) ) );
};


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Operator
                                        // windows can not stand these chars in filenames
if ( StringContains ( (const char*) "\\/:*?\"<>|", tokop->Name ) )
    StringCopy ( legalop, "." );
else
    StringCopy ( legalop, tokop->Name );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Operands
for ( int fi = 0; fi < NumFiles (); fi++ ) {
                                        // get the allocated file path / name / ext
    StringCopy      ( file, GetFile ( fi ) );
                                        // remember extension
    GetExtension    ( ext, file );
                                        // keep only directory
    RemoveFilename  ( file );
                                        // what room is left to filename (excluding extension)
    remainlen   = NoMore ( WindowsMaxComponentLength, MaxPathShort ) - 16 - StringLength ( file );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // first part - can be: group / variable / value
    if ( gof0 ) {
                                        // index of first group
        int         i       = gof1 && tokop->Code == OpBinaryMult ? fi / gof1->NumFiles (): fi;

                                        // cook the first file name
        StringCopy  ( str0, (*gof0)[ i ] );
        GetFilename ( str0 );

                                        // matrix & regularization?
        if ( tokvar[ 0 ]->IsMatrix () && StringIsNotEmpty ( tokvar[ 0 ]->Regularization ) ) {
                                        // shrink regularization
            StringShrink ( tokvar[ 0 ]->Regularization, buff, 6 );
                                        // then insert, prefixing with R if too scarse
            StringAppend ( str0, isdigit ( buff[ 0 ] ) ? "[R" : "[", buff, "]" );
            }

//      StringShrink ( str0, str0, MaxChars ( str0, tokvar[ 0 ]->Temp, tokvar[ 0 ]->IsMatrix () ) );
                                                 // keep full names
        StringShrink ( str0, str0, MaxChars ( str0, true, tokvar[ 0 ]->IsMatrix () ) );

                                        // what room is left to filename (excluding extension & operator)
        remainlen   = NoMore ( WindowsMaxComponentLength, MaxPathShort ) - 16 - StringLength ( file ) - StringLength ( str0 );
        }

    else if ( tokvar[ 0 ] && tokvar[ 0 ]->IsSingleFloat () ) {

        if ( tokvar[ 0 ]->IsValue () || tokvar[ 0 ]->Temp )
            sprintf ( str0, "%g", *tokvar[ 0 ]->DataFloat );
        else
            sprintf ( str0, "%s",  tokvar[ 0 ]->Name      );
        }

    else if ( tokvar[ 0 ] && ! tokvar[ 0 ]->Temp )
        sprintf ( str0, "%s", tokvar[ 0 ]->Name );

    else                                // well, we have a problem here...
        ClearString ( str0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // second part - can be: group / variable / value / nothing
    if ( gof1 ) {
                                        // index of second group
        int         j       = gof1 && tokop->Code == OpBinaryMult ? fi % gof1->NumFiles (): fi;

                                        // cook the second part
        StringCopy  ( str1, (*gof1)[ j ] );
        GetFilename ( str1 );

//      StringShrink ( str1, str1, MaxChars ( str1, tokvar[ 1 ]->Temp, tokvar[ 1 ]->IsMatrix () ) );
                                                 // keep full names
        StringShrink ( str1, str1, MaxChars ( str1, true, tokvar[ 1 ]->IsMatrix () ) );
        }

    else if ( tokvar[ 1 ] && tokvar[ 1 ]->IsSingleFloat () ) {

        if ( tokvar[ 1 ]->IsValue () || tokvar[ 1 ]->Temp )
            sprintf ( str1, "%g", *tokvar[ 1 ]->DataFloat );
        else
            sprintf ( str1, "%s",  tokvar[ 1 ]->Name      );
        }

    else if ( tokvar[ 1 ] && ! tokvar[ 1 ]->Temp )
        sprintf ( str1, "%s", tokvar[ 1 ]->Name );

    else                                // either nothing, or unknwown case
        ClearString ( str1 );


                                        // finally compose the file name
    if ( StringIsEmpty ( str0 ) )
        continue;                       // silly case, keep the temp names...


    if ( StringIsEmpty ( str1 ) )
        sprintf ( StringEnd ( file ), "\\%s(%s)", legalop, str0 );
    else
        sprintf ( StringEnd ( file ), "\\%s%s%s", str0, legalop, str1 );


                                        // only now can add extension
    AddExtension ( file, ext );

                                        // save the baby!
    StringCopy ( GetFile ( fi ), file );

    } // for file


                                        // check that all files ARE actually different
                                        // maybe, put a limit in the number of files to test?
bool        alldifferent    = true;

for ( int i = 0; alldifferent && i < NumFiles () - 1; i++ )
    for ( int j = i + 1; alldifferent && j < NumFiles (); j++ )
        alldifferent    = StringIsNot ( GetFile ( i ), GetFile ( j ) );

                                        // no?
if ( ! alldifferent )                   // append numbers at the end
    for ( int i = 0; i < NumFiles (); i++ )
        PostfixFilename ( GetFile ( i ), IntegerToString ( buff, i + 1 ) );


/*                                        // just for display
for ( int i = 0; i < NumFiles (); i++ ) {
    StringCopy ( file, GetFile ( i ) );
    GetFilename ( file );
    DBGM ( file, "New Compound" );
    }
*/
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // we create Values as Variables
                                        // the extra work is worth it, as Evaluate will only deal with variables and operators!
        TTokenValue::TTokenValue ()
{
Reset ();

Type            = TokenValue;
Temp            = false;

StringCopy ( Name, "__Value" );

Content         = ContentData;

DataType        = AtomTypeScalar;
Size[ 0 ]       = 0;
Size[ 1 ]       = 0;
Size[ 2 ]       = 0;
Size[ 3 ]       = 0;

DataFloat       = new float[ 1 ];
*DataFloat      = 0;
}


        TTokenValue::TTokenValue ( const char* tokenchar )
{
Reset ();

Type            = TokenValue;
Temp            = false;

StringCopy ( Name, "__Value" );

Content         = ContentData;

DataType        = AtomTypeScalar;
Size[ 0 ]       = 0;
Size[ 1 ]       = 0;
Size[ 2 ]       = 0;
Size[ 3 ]       = 0;

DataFloat       = new float[ 1 ];
*DataFloat      = StringToDouble ( tokenchar );
}


        TTokenValue::TTokenValue ( const TToken* token )
{
Reset ();

Type            = TokenValue;
Temp            = false;

const TTokenValue*  tokval  = dynamic_cast<const TTokenValue*> ( token );

StringCopy ( Name, token->IsValue () ? tokval->Name : "__Value" );

Content         = ContentData;

DataType        = AtomTypeScalar;
Size[ 0 ]       = 0;
Size[ 1 ]       = 0;
Size[ 2 ]       = 0;
Size[ 3 ]       = 0;

DataFloat       = new float[ 1 ];
*DataFloat      = token->IsValue () ? *tokval->DataFloat : 0;
}


        TTokenValue::~TTokenValue ()
{
delete[]  DataFloat;
DataFloat   = 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr int   NumOperators    = sizeof ( LegalOperators ) / sizeof ( TTokenOperatorVars );

                                        // retrieve the operator from the static array
int     StringIsAnOperator ( const char* string )
{
if ( NumOperators == 0 || StringIsEmpty ( string ) )
    return  -1;


for ( int i = 0; i < NumOperators; i ++ )
    if ( StringIs ( LegalOperators[ i ].Name, string ) )
        return  i;

return  -1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TTokenOperator::TTokenOperator ()
{
Reset ();
}


void    TTokenOperator::Reset ()
{
Type            = TokenOperator;
Temp            = false;

Name            = 0;
Code            = OpUnknown;
Priority        = 0;
NumParameters   = 0;
Function        = false;
}


        TTokenOperator::TTokenOperator ( const char* tokenchar )
{
Type            = TokenOperator;
Temp            = false;

int     opi     = StringIsAnOperator ( tokenchar );

if ( opi >= 0 ) {                       // found a matching operator name
    Name            = LegalOperators[ opi ].Name;
    Code            = LegalOperators[ opi ].Code;
    Priority        = LegalOperators[ opi ].Priority;
    NumParameters   = LegalOperators[ opi ].NumParameters;
    Function        = LegalOperators[ opi ].Function;
    }
else
    Reset ();
}


        TTokenOperator::TTokenOperator ( const TTokenOperatorVars& opvar )
{
Type            = TokenOperator;
Temp            = false;

//StringCopy ( Name, opvar.Name );
Name            = opvar.Name;   // a simple copy of the pointers is enough, as we link to static strings
Code            = opvar.Code;
Priority        = opvar.Priority;
NumParameters   = opvar.NumParameters;
Function        = opvar.Function;
}


        TTokenOperator::TTokenOperator ( const TTokenOperator* tokop )
{
Type            = TokenOperator;
Temp            = false;

//StringCopy ( Name, tokop->Name );
Name            = tokop->Name;  // a simple copy of the pointers is enough, as we link to static strings
Code            = tokop->Code;
Priority        = tokop->Priority;
NumParameters   = tokop->NumParameters;
Function        = tokop->Function;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TTokensStack::~TTokensStack ()
{
Reset ( true );
}


void    TTokensStack::Reset ( bool deleteall )
{
if ( Tokens.IsEmpty () )
    return;


for ( int tokeni = 0; tokeni < Tokens.Num (); tokeni++ )
    if ( deleteall || Tokens[ tokeni ]->Temp )
        delete  Tokens[ tokeni ];



Tokens.Reset ( DontDeallocate );        // reset only the pointers to data
}


//----------------------------------------------------------------------------
void    TTokensStack::Push ( const TToken* token )
{
if ( token ) {
    Tokens.Append ( token );

    Tokens.UpdateIndexes ();
    }
}


TToken *TTokensStack::Pop ()
{
if ( IsEmpty () )
    return  0;

TToken*     totok   = Tokens.GetLast ();

Tokens.RemoveLast ( DontDeallocate );   // remove from the list, but don't destroy actual data

Tokens.UpdateIndexes ();

return  totok;
}


//----------------------------------------------------------------------------
const TTokenVariable*   TTokensStack::GetVariable ( const char* varname )   const
{
if ( Tokens.IsEmpty () || StringIsEmpty ( varname ) )
    return  0;


for ( int tokeni = 0; tokeni < Tokens.Num (); tokeni++ ) {

    const TTokenVariable*   tovar   = dynamic_cast<const TTokenVariable*> ( Tokens[ tokeni ] );

    if ( tovar && StringIs ( tovar->Name, varname ) )
        return  tovar;
    }

return  0;
}

                                        // check for the requested amount of operands
bool    TTokensStack::GetOperands ( TTokenVariable* operands[], int n )
{
for ( int i = n - 1; (bool) Tokens; i-- ) {

    if ( ! Tokens.GetLast ()->IsOperand () )
        return  false;
                                        // just copy the pointer to the token
    operands[ i ]   = static_cast<TTokenVariable*> ( Pop () );

    if ( i == 0 )                       // reached the right amount of operands
        return  true;
    }
                                        // not enough tokens
return  false;
}


//----------------------------------------------------------------------------
void    TTokensStack::Show ( const char* title )    const
{
char                    buff[ 1024 ];


sprintf ( buff, "%0d token(s):\n\n", (int) Tokens );

for ( int tokeni = 0; tokeni < Tokens.Num (); tokeni++ ) {

    const TToken*   tok         = Tokens[ tokeni ];

    if      ( tok->IsIdentifier () )    sprintf ( StringEnd ( buff ), "Identifier:\t%s\t\n",    static_cast<const TTokenIdentifier *> ( tok )->Identifier  );
    else if ( tok->IsValue ()      )    sprintf ( StringEnd ( buff ), "Value:\t\t%g\t\n",       static_cast<const TTokenValue      *> ( tok )->Value ()    );
    else if ( tok->IsVariable ()   )    sprintf ( StringEnd ( buff ), "Variable:\t\t%s\t\n",    static_cast<const TTokenVariable   *> ( tok )->Name        );
    else if ( tok->IsOperator ()   )    sprintf ( StringEnd ( buff ), "Operator:\t%s\t\n",      static_cast<const TTokenOperator   *> ( tok )->Name        );
    }

ShowMessage ( buff, title );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool    TParser::CheckChars ( const char* expression, bool verbose )    const
{
int                 l               = StringLength ( expression );
const char*         toc             = expression;
//TArray1<int>    count ( 256 * sizeof ( *expression ) ); // for wide chars


//count.Reset ();

                                        // do a very broad check on the legal characters, just to get rid of real non-sense chars
for ( int i = 0; i < l; i++, toc++ )

    if ( isalnum ( *toc ) )             // makes use of locale -> accented chars OK
        continue;
    else                                // test within a broad range of special characters
        if ( ! StringContains ( " \t\n.,;:+-/*=(){}[]%_^&$!|~<>", *toc ) ) {

        if ( verbose ) {
            char    buff[ 256 ];

            sprintf ( buff, "Illegal character \"%c\" at column %0Id:\n\n%s", *toc, toc - expression + 1, expression );

            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
            }

        return false;
        }
//    else
//        ++count[ *toc ];

/*
                                        // while at it, we can check for missing parenthesis (and the like)!
if ( count[ '(' ] != count[ ')' ] ) {

    if ( verbose ) {
        char    buff[ 256 ];

        sprintf ( buff, "Missing parenthesis!", *toc );
        ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
        }

    return false;
    }


if ( count[ '{' ] != count[ '}' ] ) {

    if ( verbose ) {
        char    buff[ 256 ];

        sprintf ( buff, "Missing curly bracket!", *toc );
        ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
        }

    return false;
    }


if ( count[ '[' ] != count[ ']' ] ) {

    if ( verbose ) {
        char    buff[ 256 ];

        sprintf ( buff, "Missing square bracket!", *toc );
        ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
        }

    return false;
    }
*/

return  true;
}


//----------------------------------------------------------------------------
                                        // split into legal tokens, with infix notation
                                        // output can be: values, operators / functions, variables, identifiers
bool    TParser::Tokenize ( const char* expression, const TTokensStack& variables, TTokensStack& tokens, bool verbose ) const
{
tokens.Reset ( true );


if ( ! CheckChars ( expression, verbose ) )
    return  false;


int                 l               = StringLength ( expression );
const char*         toc             = expression;
const char*         start           = 0;
char*               end;
char                buff[ 256 ];
int                 numops          = NumOperators;

                                        // here we go, browse the string
                                        // we go up to the null char, as to finish any ending token
for ( int i = 0; i <= l; i++, toc++ ) {

                                        // starting a new token, or a space?
    if ( start == 0 || IsSpaceNewline ( *start ) ) {
        start   = toc;                  // set new beginning, then continue
        continue;
        }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // identifier syntax: Letter+[Letter|digit]*
    if ( isalpha ( *start ) ) {

        if ( isalnum ( *toc ) )
            continue;
        else {
                                        // copy the part recognized as an identifier
            StringCopy ( buff, start, toc - start );

                                        // identifier can be a function -> operator
            if      ( StringIsAnOperator ( buff ) != -1 )       tokens.Push ( new TTokenOperator ( buff ) );

            else if ( variables.GetVariable ( buff ) != 0 )     tokens.Push ( new TTokenVariable ( variables.GetVariable ( buff ) ) );

            else                                                tokens.Push ( new TTokenIdentifier ( buff ) );


            i--;                        // go back 1 char
            toc--;
            start  = 0;

//            DBGM ( buff, "Lex: String" );
            continue;
            }
        } // starting with string

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // number syntax: "a la C" (doesn't handle the unary - or + here, this will be done on a later stage)
    else if ( isdigit ( *start ) ) {
                                        // the trick: use strtod which will read as much as a number can be!
        strtod ( start, &end );
                                        // it also returns the end of the number read, so use it!
        i      += end - toc;
        toc     = expression + i;
                                        // chop off the number from the string
        StringCopy ( buff, start, toc - start );

        tokens.Push ( new TTokenValue ( buff ) );

        i--;                            // go back 1 char
        toc--;
        start  = 0;

//        DBGM ( buff, "Lex: Number" );
        continue;
        } // starting with number

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    else {                              // other cases: special chars, like operators, or errors

        int     opi;
                                        // see among the known operators
        for ( opi = 0; opi < numops; opi++ )

            if ( StringStartsWith ( start, LegalOperators[ opi ].Name ) ) {
                                        // jump after the first found
                i      += StringLength ( LegalOperators[ opi ].Name ) - 2;
                toc     = expression + i;

                tokens.Push ( new TTokenOperator ( LegalOperators[ opi ] ) );

                start  = 0;

//                DBGM ( LegalOperators[ opi ].Name, "Lex: Operator" );
                break;                  // or: check for all, take the longest operator?
                }

        if ( opi == numops )           // no operator?
            goto tokenerror;            // that was our last chance, sorry!
        else
            continue;
        } // operators

    } // for each char


return  true;


tokenerror:

if ( verbose ) {
    char    buff[ 256 ];

    sprintf ( buff, "Error at column %0Id:\n\n%s", toc - expression, expression );
    ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
    }

return false;
}


//----------------------------------------------------------------------------
                                        // Merging any mixed series of "+" and "-"
void    TParser::SimplifiesUnaryOperators ( TTokensStack& tokenstack )
{
for ( int i = 0; i < tokenstack.GetNumTokens (); i++ ) {
                                        // Current token
    TToken*             tok             = tokenstack[ i ];

                                        // none of the unary operators
    if ( ! (    tok->IsOperator ( OpBinaryMinus ) 
             || tok->IsOperator ( OpBinaryPlus  ) ) )
        continue;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here token is either "-" or "+" operator, but we have to sort out if it is unitary or binary
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    int                 countminus      = 0;
    int                 countplus       = 0;

    for ( int j = i; j < tokenstack.GetNumTokens (); j++ ) {

        if      ( tokenstack[ j ]->IsOperator ( OpBinaryMinus ) )   countminus++;
        else if ( tokenstack[ j ]->IsOperator ( OpBinaryPlus  ) )   countplus ++;
        else                                                        break;
        }
                                    // series' length
    int                 countmp         = countminus + countplus;
                                        // not a series at all?
    if ( countmp <= 1 )
        continue;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                    // sign of the resulting series
    bool                resultisplus    =    countminus == 0        // only "+"
                                          || IsEven ( countminus ); // even numbers of "-"

                                    // removing the whole series
    for ( int j = countmp - 1; j >= 0; j-- )
        tokenstack.Remove ( tokenstack[ i + j ] );

                                    // replaced by a single unary operator
    tokenstack.Insert ( new TTokenOperator ( resultisplus ? "+" : "-" ), tokenstack[ i ] );

                                    // rescan from scratch - unary operators are processed below
    //i   = -1;
    }
}


//----------------------------------------------------------------------------
                                        // Transform any unary "- <expr>" into its binary equivalent "( 0 - <expr> )"
                                        // The trick is to detect actual unary and not binary "-"...
void    TParser::UnaryOperatorToBinaryOperator ( TTokensStack& tokenstack )
{
                                        // We need any series to be merged first
SimplifiesUnaryOperators ( tokenstack );

//tokenstack.Show ( "Input Tokens - Unary merged" );

                                        // Expanding unary "+" and "-" to binary equivalent operators
for ( int i = 0; i < tokenstack.GetNumTokens () - 1; i++ ) {
                                        // Current token
    TToken*             tok             = tokenstack[ i ];

                                        // none of the unary operators
    if ( ! (    tok->IsOperator ( OpBinaryMinus ) 
             || tok->IsOperator ( OpBinaryPlus  ) ) )
        continue;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here token is either "-" or "+" operator, but we have to sort out if it is unitary or binary
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Next token + conversion attempt to operator
    TToken*             tokafter    = i < tokenstack.GetNumTokens () - 1 ? tokenstack[ i + 1 ] : 0;


    if ( ! tokafter )                   // nothing after current operator?
        continue;                       // error!


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Previous token
    TToken*             tokbefore   = i > 0                              ? tokenstack[ i - 1 ] : 0;

                                        // Binary operation cases:
    if ( tokbefore && (    tokbefore->IsVariable ()                         // "Var   +- <something>"
                        || tokbefore->IsValue ()                            // "Value +- <something>"
                        || tokbefore->IsOperator ( OpParenthesisClose ) ) ) // ")     +- <something>"
                                        // NOT to be converted!
        continue;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Unary cases: "+- value"
    if ( tokafter->IsValue () ) {

        if ( tok->IsOperator ( OpBinaryMinus ) ) {
                                        // simply invert the stored value internally
            TTokenValue*    tokval              = dynamic_cast<TTokenValue*> ( tokafter );
                            tokval->Value ()    = - tokval->Value ();
            }
                                        // we can remove the unary operator, either "+" or "-"
        tokenstack.Remove ( tok );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Other unary cases:
    else if (    tokafter->IsVariable ()                    // "+- variable" 
              || tokafter->IsOperator ( OpParenthesisOpen ) // "+- ( something )"
              || tokafter->IsFunction ()                 ) {// "+- function ()"

                                        // prefix with "( 0 " and postfix with ")"
        tokenstack.Insert ( new TTokenOperator ( "(" ), tok );
        tokenstack.Insert ( new TTokenValue    ( "0" ), tok );
                                                     // tok is either "+" or "-"

        int             insertposti;
        int             parenthesislevel    = 0;
        int             resumei             = 0;
                                        // we need to find the proper end of the expression to correctly insert the ")"
        for ( insertposti = i + 3; insertposti < tokenstack.GetNumTokens () /*- 1*/; insertposti++ ) {
            
            if      ( tokenstack[ insertposti ]->IsVariable () ) {
                                        // resume AFTER variable
                resumei             = i + 4;

                break;
                }

            else if ( tokenstack[ insertposti ]->IsOperator ( OpParenthesisOpen ) )  {
                                        // resume JUST after opened parenthesis
                resumei             = i + 4;

                parenthesislevel    = 1;
                insertposti++;

                while ( parenthesislevel != 0 && insertposti < tokenstack.GetNumTokens () ) {

                    if      ( tokenstack[ insertposti ]->IsOperator ( OpParenthesisOpen  ) )     parenthesislevel++;
                    else if ( tokenstack[ insertposti ]->IsOperator ( OpParenthesisClose ) )     parenthesislevel--;

                    insertposti++;
                    }

                insertposti--;
                                        // here we reached the closing of the outer parenthesis, in case there are nested ones
                break; 
                }

            else if ( tokenstack[ insertposti ]->IsFunction () ) {
                                        // resume JUST after opened parenthesis
                resumei             = i + 5;

                parenthesislevel    = 1;
                insertposti        += 2;

                while ( parenthesislevel != 0 && insertposti < tokenstack.GetNumTokens () ) {

                    if      ( tokenstack[ insertposti ]->IsOperator ( OpParenthesisOpen  ) )     parenthesislevel++;
                    else if ( tokenstack[ insertposti ]->IsOperator ( OpParenthesisClose ) )     parenthesislevel--;

                    insertposti++;
                    }

                insertposti--;
                                        // here we reached the closing of the outer parenthesis, in case there are nested ones
                break; 
                }
            }

                                        // Inmsert needs the one token after, so increment position
        insertposti++;
                                                     // let's be nice and cautious...
        tokenstack.Insert ( new TTokenOperator ( ")" ), insertposti < tokenstack.GetNumTokens () ? tokenstack[ insertposti ] : 0 );

        i   = resumei - 1;  // decrement because it will be incremented right now...
        }

    } // for all tokens

}


//----------------------------------------------------------------------------
bool    TParser::Parse ( const char* expression, const TTokensStack& variables, bool verbose )
{
Reset ();


if ( StringIsEmpty ( expression ) || variables.IsEmpty () )
    return  false;


TTokensStack            tokensinfix;
TTokensStack            operators;
const TTokenOperator*   tokop;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // split into tokens, infix style
if ( ! Tokenize ( expression, variables, tokensinfix, verbose ) )
    return  false;

//tokensinfix.Show ( "Input Tokens" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

UnaryOperatorToBinaryOperator ( tokensinfix );

//tokensinfix.Show ( "Input Tokens - Unary - converted" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // now, we're going to re-write this list into a Postfix Notation (aka RPN)
for ( int tokeni = 0; tokeni < tokensinfix.GetNumTokens (); tokeni++ ) {

    const TToken*   tok         = tokensinfix[ tokeni ];


    if ( tok->IsOperator () ) {

        tokop       = static_cast<const TTokenOperator*> ( tok );


        if ( operators.IsEmpty () )

            operators.Push ( new TTokenOperator ( tokop ) );

                                        // force push the next operator after an "("
        else if ( operators.GetLastOperator ()->Code == OpParenthesisOpen )
                                        // useless "()"
            if ( tokop->Code == OpParenthesisClose )

                delete  operators.Pop (); // deleting the object is not delegated to another list, so do it now

            else

                operators.Push ( new TTokenOperator ( tokop ) );

                                        // special case for ")"
        else if ( tokop->Code == OpParenthesisClose ) {

            bool        foundopen   = false;

            do {                        // pop until the open parenthesis
                if ( operators.GetLastOperator ()->Code == OpParenthesisOpen ) {
                    delete  operators.Pop (); // we don't need to push it on the output stack, plus we delete it now
                    foundopen   = true;
                    break;
                    }

                Tokens.Push ( operators.Pop () );

                } while ( operators.IsNotEmpty () );

            if ( ! foundopen ) {

                if ( verbose )
                    ShowMessage ( "Missing opening parenthesis!", ParserTitleError, ShowMessageWarning );

                return  false;
                }

                                        // special case: closing parenthesis of a function -> pop the function, too
            if ( operators.GetLastOperator () && operators.GetLastOperator ()->IsFunction () )
                                        // push now the function!
                Tokens.Push ( operators.Pop () );

            } // ")"

                                        // more priority than in the operators stack? continue to push
        else if ( tokop->Priority > operators.GetLastOperator ()->Priority )

            operators.Push ( new TTokenOperator ( tokop ) );


        else {                          // equal or less priority: pop all last, push the new

            while ( operators.IsNotEmpty () && operators.GetLastOperator ()->Priority >= tokop->Priority
                                            && operators.GetLastOperator ()->Priority <  ParenthesisPriority )
                Tokens.Push ( operators.Pop () );

            operators.Push ( new TTokenOperator ( tokop ) );
            }

        } // token operator


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    else if ( tok->IsValue () )

        Tokens.Push ( new TTokenValue ( tok ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    else if ( tok->IsVariable () )

        Tokens.Push ( new TTokenVariable ( tok ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    else if ( tok->IsIdentifier () ) {
                                        // getting an identifier here is a problem
        if ( verbose ) {

            char    buff[ 256 ];

            sprintf ( buff, "Undefined variable or function \"%s\"", static_cast<const TTokenIdentifier*> ( tok )->Identifier );

            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
            }

        return  false;

        } // token identifier


//    Tokens.Show ( "Tokens Out" );
//    operators.Show ( "Operators" );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // unstack the remaining operators to the output stack
while ( operators.IsNotEmpty () ) {

    if ( operators.GetLastOperator ()->Code == OpParenthesisOpen ) {

        if ( verbose )
            ShowMessage ( "Missing closing parenthesis!", ParserTitleError, ShowMessageWarning );

        return  false;
        }

    Tokens.Push ( operators.Pop () );
    }

//Tokens.Show ( "Output Tokens" );


                                        // we should have at least something to do!
if ( Tokens.IsEmpty () ) {

    if ( verbose )
        ShowMessage ( "Expression is empty!", ParserTitleError, ShowMessageWarning );

    return  false;
    }



return  true;
}


//----------------------------------------------------------------------------
                                        // Force parsing (from potentially) new expression - not optimal, Parsing should be called only once for the same expression
bool    TParser::Evaluate ( const char* expression, const TTokensStack& variables, bool verbose )
{
if ( ! Parse ( expression, variables, verbose ) )

    return  false;

return  Evaluate ( verbose );
}


//----------------------------------------------------------------------------
bool    TParser::Evaluate ( bool verbose )
{
                                        // parsing should have been done at that point
if ( Tokens.IsEmpty () )

    return  false;

//Tokens.Show ( "Evaluate::Tokens Parsed" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TCartoolDocManager*     docmanager      = Cartool.CartoolDocManager;

constexpr int           MaxOperands     = 4;

TTokensStack            stack;
TTokenVariable*         operands     [ MaxOperands ];
TTokenOperator*         tokop;
TTokenVariable*         tokvartemp  = 0;
bool                    operandswitch;

char                    buff[ KiloByte ];

TTokenVariable*         tokvar1;
TTokenVariable*         tokvar2;

TTracksDoc*             file1;
TTracksDoc*             file2;
TRisDoc*                ris1;
TRisDoc*                ris2;
TInverseMatrixDoc*      isdoc;
int                     numel;
int                     numsp;
int                     numtf;

TTracks<float>          eegb;
TArray1<TVector3Float>  vec1;
TArray1<TVector3Float>  vec2;


TSuperGauge             GaugeS ( "Evaluation Step", Tokens.GetNumTokens () + 1, SuperGaugeLevelInter,   SuperGaugeCount    );
TSuperGauge             GaugeF ( "Files",           100,                        SuperGaugeLevelDefault, SuperGaugeDefault  );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // browse the processed tokens
for ( int tokeni = 0; tokeni < Tokens.GetNumTokens (); tokeni++ ) {

    GaugeS.Next ();
    GaugeF.WindowHide ();               // default is to hide the file gauge, the appropriate code will re-enable it

                                        // push next token to the execute stack
    stack.Push ( Tokens[ tokeni ] );

                                        // test if an operand
    if ( stack.GetLast ()->IsOperand () )
        continue; // stacking

                                        // not an operator / function? really weird!
    if ( ! stack.GetLast ()->IsOperator () ) {

        if ( verbose )
            ShowMessage ( "Syntax error!", ParserTitleError, ShowMessageWarning );

        goto    evalerror;
        }

                                        // unstack the operator
    tokop   = static_cast<TTokenOperator*> ( stack.Pop () );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // operator OK, now try retrieving the operands

    for ( int i = 0; i < MaxOperands; i++ )
        operands[ i ]   = 0;

                                        // count # of operands & unstack them, then transfer them to array
    if ( ! stack.GetOperands ( operands, tokop->NumParameters ) ) {

        if ( verbose ) {
            sprintf ( buff, "Missing or wrong operand(s) for operation \"%s\"", tokop->Name );
            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
            }

        goto    evalerror;
        }
                                        // here, we have the number of operands expected by the current operator

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we have an operator and the right amount of operands, let's try to operate now

                                        // shortcuts to the first 2 operands
    tokvar1     = tokop->NumParameters >= 1 ? operands[ 0 ] : 0;
    tokvar2     = tokop->NumParameters >= 2 ? operands[ 1 ] : 0;

    numel       = tokvar1 ? tokvar1->Size[ 1 ] : 0;
    numsp       = tokvar1 ? tokvar1->Size[ 1 ] : 0;
    numtf       = tokvar2 ? tokvar2->Size[ 2 ] : ( tokvar1 ? tokvar1->Size[ 2 ] : 0 );

    TExportTracks     fileout;

                                        // create output temp variable token
    tokvartemp              = new TTokenVariable ();
    tokvartemp->Temp        = true;


    int         nummemory   = operands[ 0 ]->IsMemory () + ( ! operands[ 1 ] || ! operands[ 1 ]->IsMemory () ? 0 : 1 );
    int         numgof      = tokop->NumParameters - nummemory;
    int         numfloat    = operands[ 0 ]->IsFloat  () + ( ! operands[ 1 ] || ! operands[ 1 ]->IsFloat  () ? 0 : 1 );
    int         numvector   = tokop->NumParameters - numfloat;


    switch ( tokop->Code ) {

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Unary operators
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        case    OpAbs:
        case    OpScalar:
        case    OpSqr:
        case    OpSqrt:

                if ( nummemory == 1 ) {

                    if      ( numfloat  == 1 ) {
                        tokvartemp->Allocate ( CombineFlags ( AllocateMemory, AllocateFloat ),  operands );
                                        // single variable now - generalize to use dimensions
                        if      ( tokop->Code == OpAbs    )     *tokvartemp->DataFloat      = fabs ( *tokvar1->DataFloat );
                        else if ( tokop->Code == OpScalar )     *tokvartemp->DataFloat      = *tokvar1->DataFloat;
                        else if ( tokop->Code == OpSqr    )     *tokvartemp->DataFloat      = *tokvar1->DataFloat * *tokvar1->DataFloat;
                        else if ( tokop->Code == OpSqrt   )     *tokvartemp->DataFloat      = sqrt ( *tokvar1->DataFloat );
                        }

                    else if ( numvector == 1 ) {
                        tokvartemp->Allocate ( CombineFlags ( AllocateMemory, AllocateFloat ),  operands );
                                        // single variable now - generalize to use dimensions
                        if      ( tokop->Code == OpAbs    )     *tokvartemp->DataFloat      = tokvar1->DataVector->Norm ();
                        else if ( tokop->Code == OpScalar )     *tokvartemp->DataFloat      = tokvar1->DataVector->Norm ();
                        else if ( tokop->Code == OpSqr    )     *tokvartemp->DataFloat      = tokvar1->DataVector->Norm2 ();
                        else {
                            if ( verbose ) {
                                sprintf ( buff, "Can not apply operation \"%s\" on this operand.", tokop->Name );
                                ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                                }

                            goto    evalerror;
                            }
                        }

                    else {
                        if ( verbose ) {
                            sprintf ( buff, "Can not apply operation \"%s\" on this operand.", tokop->Name );
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }
                    } // only memory


                else if ( numgof == 1 ) {

                                        // check operand types
                    if ( tokvar1->Content == ContentMatrix ) {
                        if ( verbose ) {
                            sprintf ( buff, "Can not apply operation \"%s\" on these types of files.", tokop->Name );
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }

                                        // we deal only with 2D files
                    if ( ! tokvar1->IsArray2D () ) {
                        if ( verbose ) {
                            sprintf ( buff, "Can not apply operation \"%s\"\non files which are not 2 dimensionals.", tokop->Name );
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }

                                        // no sqrt of vector!
                    if ( tokop->Code == OpSqrt && tokvar1->IsVector () ) {
                        if ( verbose ) {
                            sprintf ( buff, "Can not apply operation \"%s\" on these types of files.", tokop->Name );
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }


                    fileout.CloneParameters ( tokvar1->OutputExt, tokvar1->GetFile () );
                    StringCopy ( tokvartemp->OutputExt, fileout.GetExtension () );
                                        // result type
                    fileout.SetAtomType ( AtomTypeScalar );

                    tokvartemp->Allocate ( CombineFlags ( AllocateGroup, fileout.IsScalar ( AtomTypeUseOriginal ) ? AllocateFloat : AllocateVector ), operands, tokop );

                    eegb.Resize ( numel, 1 );


                    GaugeF.WindowRestore ();


                    for ( int i = 0; i < tokvar1->NumFiles (); i++ ) {

                        GaugeF.SetValue ( SuperGaugeDefaultPart, Percentage ( i, tokvar1->NumFiles () - 1 ) );


                        file1   = dynamic_cast<TTracksDoc*> ( docmanager->OpenDoc ( tokvar1->GetFile ( i ), dtOpenOptionsNoView ) );


                        StringCopy ( fileout.Filename, tokvartemp->GetFile ( i ) );
                        ReplaceExtension ( fileout.Filename, fileout.GetExtension () );


                        for ( int tf = 0; tf < numtf; tf++ ) {

                            file1->GetTracks ( tf, tf, eegb );

                            for ( int e = 0; e < fileout.NumTracks; e++ )
                                if      ( tokop->Code == OpAbs    )     fileout.Write ( fabs ( eegb ( e , 0 ) ) );
                                                                                     // reading has already done the job to go down to scalar
                                else if ( tokop->Code == OpScalar )     fileout.Write ( eegb ( e , 0 ) );
                                                                                     // its either a scalar, or the norm
                                else if ( tokop->Code == OpSqr    )     fileout.Write ( eegb ( e , 0 ) * eegb ( e , 0 ) );
                                                                                     // only for scalar
                                else if ( tokop->Code == OpSqrt   )     fileout.Write ( sqrt ( eegb ( e , 0 ) ) );
                            }

                        if ( file1->CanClose ( true ) )     docmanager->CloseDoc ( file1 );
                        }

                    } // only gof

                break;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Binary operators
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        case    OpBinaryPlus:
        case    OpBinaryMinus:

                if ( nummemory == 2 ) {

                    if      ( numfloat  == 2 ) {
                        tokvartemp->Allocate ( CombineFlags ( AllocateMemory, AllocateFloat ),  operands );
                                        // single variable now - generalize to use dimensions
                        if      ( tokop->Code == OpBinaryPlus  )    *tokvartemp->DataFloat      = *tokvar1->DataFloat + *tokvar2->DataFloat;
                        else if ( tokop->Code == OpBinaryMinus )    *tokvartemp->DataFloat      = *tokvar1->DataFloat - *tokvar2->DataFloat;
                        }

                    else if ( numvector == 2 ) {
                        tokvartemp->Allocate ( CombineFlags ( AllocateMemory, AllocateVector ), operands );
                                        // single variable now - generalize to use dimensions
                        if      ( tokop->Code == OpBinaryPlus  )    *tokvartemp->DataVector     = *tokvar1->DataVector + *tokvar2->DataVector;
                        else if ( tokop->Code == OpBinaryMinus )    *tokvartemp->DataVector     = *tokvar1->DataVector - *tokvar2->DataVector;
                        }

                    else {
                        if ( verbose ) {
                            sprintf ( buff, "Can not apply operation \"%s\" on these operands.", tokop->Name );
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }
                    } // only memory

                                        // group and variable
                else if ( nummemory == 1 && numgof == 1 ) {

                                        // permutate to have tokvar1 as the group
                    if ( ( operandswitch = tokvar2->IsGroup () ) == true ) {
                        TTokenVariable     *tokvar;
                        tokvar  = tokvar1; tokvar1  = tokvar2; tokvar2  = tokvar;
                        }

                                        // check operands types
                    if ( tokvar2->Content != ContentData || tokvar1->Content != ContentData || ! tokvar2->HasSameType ( tokvar1 ) ) {
                        if ( verbose ) {
                            sprintf ( buff, "Can not apply operation \"%s\" on these types of operands.", tokop->Name );
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }

                                        // check for operands dimensions
                    if ( ! tokvar2->IsScalar () || ! tokvar1->IsArray2D () ) {
                        if ( verbose ) {
                            sprintf ( buff, "Can not apply operation \"%s\" on these types of operands.", tokop->Name );
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }


                    fileout.CloneParameters ( tokvar1->OutputExt, tokvar1->GetFile () );
                    StringCopy ( tokvartemp->OutputExt, fileout.GetExtension () );

                    tokvartemp->Allocate ( CombineFlags ( AllocateGroup, fileout.IsScalar ( AtomTypeUseOriginal ) ? AllocateFloat : AllocateVector ), operands, tokop );

                    numel       = fileout.NumTracks;
                    numtf       = fileout.NumTime;

                    if ( fileout.IsScalar ( AtomTypeUseOriginal ) )
                        eegb.Resize ( numel, 1 );
                    else
                        vec1.Resize ( numel );


                    GaugeF.WindowRestore ();


                    for ( int i = 0; i < tokvar1->NumFiles (); i++ ) {

                        GaugeF.SetValue ( SuperGaugeDefaultPart, Percentage ( i, tokvar1->NumFiles () - 1 ) );

                        file1   = dynamic_cast<TTracksDoc*> ( docmanager->OpenDoc ( tokvar1->GetFile ( i ), dtOpenOptionsNoView ) );
                                        // try to upcast it
                        ris1    = fileout.IsScalar ( AtomTypeUseOriginal ) ? 0 : dynamic_cast< TRisDoc     * > ( file1 );


                        StringCopy ( fileout.Filename, tokvartemp->GetFile ( i ) );
                        ReplaceExtension ( fileout.Filename, fileout.GetExtension () );


                        for ( int tf = 0; tf < numtf; tf++ ) {

                            if ( fileout.IsScalar ( AtomTypeUseOriginal ) ) {
                                file1->GetTracks ( tf, tf, eegb );

                                for ( int e = 0; e < numel; e++ )
                                    if      ( tokop->Code == OpBinaryPlus  )    fileout.Write ( eegb ( e , 0 ) + *tokvar2->DataFloat );
                                    else if ( tokop->Code == OpBinaryMinus )    fileout.Write ( operandswitch ? *tokvar2->DataFloat - eegb ( e , 0 ) : eegb ( e , 0 ) - *tokvar2->DataFloat );
                                }
                            else {
                                ris1->GetInvSol  ( 0, tf, tf, vec1, 0 );

                                for ( int e = 0; e < numel; e++ )
                                    if      ( tokop->Code == OpBinaryPlus  )    fileout.Write ( vec1[ e ] + *tokvar2->DataVector );
                                    else if ( tokop->Code == OpBinaryMinus )    fileout.Write ( operandswitch ? *tokvar2->DataVector - vec1[ e ] : vec1[ e ] - *tokvar2->DataVector );
                                }
                            }

                        if ( file1->CanClose ( true ) )     docmanager->CloseDoc ( file1 );
                        } // for file1

                    } // 1 memory, 1 gof


                else if ( numgof == 2 ) {

                                        // check operands types
                    if ( tokvar1->Content == ContentMatrix || tokvar2->Content == ContentMatrix
                      || tokvar1->DataType != tokvar2->DataType ) {
                        if ( verbose ) {
                            sprintf ( buff, "Can not apply operation \"%s\" on these types of files.", tokop->Name );
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }

                                        // check for operands dimensions
                    if ( ! tokvar1->HasSameFiles ( tokvar2 ) || ! tokvar1->HasSameSize ( tokvar2 ) ) {
                        if ( verbose ) {
                            sprintf ( buff, "Can not apply operation \"%s\" because of different number of files,\nor different dimensions between groups.", tokop->Name );
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }

                                        // we deal only with 2D files
                    if ( ! tokvar1->IsArray2D () || ! tokvar2->IsArray2D () ) {
                        if ( verbose ) {
                            sprintf ( buff, "Can not apply operation \"%s\"\non files which are not 2 dimensionals.", tokop->Name );
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }


                    fileout.CloneParameters ( tokvar1->OutputExt, tokvar1->GetFile (), tokvar2->GetFile () );
                    StringCopy ( tokvartemp->OutputExt, fileout.GetExtension () );

                    tokvartemp->Allocate ( CombineFlags ( AllocateGroup, fileout.IsScalar ( AtomTypeUseOriginal ) ? AllocateFloat : AllocateVector ), operands, tokop );

                    if ( fileout.IsScalar ( AtomTypeUseOriginal ) )
                        eegb.Resize ( numel, 2 );
                    else {
                        vec1.Resize ( numel );
                        vec2.Resize ( numel );
                        }


                    GaugeF.WindowRestore ();


                    for ( int i = 0; i < tokvar1->NumFiles (); i++ ) {

                        GaugeF.SetValue ( SuperGaugeDefaultPart, Percentage ( i, tokvar1->NumFiles () - 1 ) );

                        file1   = dynamic_cast<TTracksDoc*> ( docmanager->OpenDoc ( tokvar1->GetFile ( i ), dtOpenOptionsNoView ) );
                        file2   = dynamic_cast<TTracksDoc*> ( docmanager->OpenDoc ( tokvar2->GetFile ( i ), dtOpenOptionsNoView ) );

                                        // try to upcast them
                        ris1    = fileout.IsScalar ( AtomTypeUseOriginal ) ? 0 : dynamic_cast< TRisDoc     * > ( file1 );
                        ris2    = fileout.IsScalar ( AtomTypeUseOriginal ) ? 0 : dynamic_cast< TRisDoc     * > ( file2 );


                        StringCopy ( fileout.Filename, tokvartemp->GetFile ( i ) );
                        ReplaceExtension ( fileout.Filename, fileout.GetExtension () );


                        for ( int tf = 0; tf < numtf; tf++ ) {

                            if ( fileout.IsScalar ( AtomTypeUseOriginal ) ) {
                                file1->GetTracks ( tf, tf, eegb, 0 );
                                file2->GetTracks ( tf, tf, eegb, 1 );

                                for ( int e = 0; e < numel; e++ )
                                    if      ( tokop->Code == OpBinaryPlus  )    fileout.Write ( eegb ( e , 0 ) + eegb ( e , 1 ) );
                                    else if ( tokop->Code == OpBinaryMinus )    fileout.Write ( eegb ( e , 0 ) - eegb ( e , 1 ) );
                                }
                            else {
                                ris1->GetInvSol  ( 0, tf, tf, vec1, 0 );
                                ris2->GetInvSol  ( 0, tf, tf, vec2, 0 );

                                for ( int e = 0; e < numel; e++ )
                                    if      ( tokop->Code == OpBinaryPlus  )    fileout.Write ( vec1[ e ] + vec2[ e ] );
                                    else if ( tokop->Code == OpBinaryMinus )    fileout.Write ( vec1[ e ] - vec2[ e ] );
                                }
                            }

                        if ( file1->CanClose ( true ) )     docmanager->CloseDoc ( file1 );
                        if ( file2->CanClose ( true ) )     docmanager->CloseDoc ( file2 );
                        }

                    } // only gof

                break;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case    OpBinaryMult:

                if ( nummemory == 2 ) {

                    if      ( numfloat  == 2 ) {
                        tokvartemp->Allocate ( CombineFlags ( AllocateMemory, AllocateFloat ),  operands );
                                        // single variable now - generalize to use dimensions
                        *tokvartemp->DataFloat      = *tokvar1->DataFloat * *tokvar2->DataFloat;
                        }

                    else if ( numvector == 2 ) {
                        tokvartemp->Allocate ( CombineFlags ( AllocateMemory, AllocateFloat ), operands );
                                        // single variable now - generalize to use dimensions
                                        // scalar product of vectors
                        *tokvartemp->DataFloat      = tokvar1->DataVector->ScalarProduct ( *tokvar2->DataVector );
                        }

                    else if ( numfloat == 1 && numvector == 1 ) {
                        tokvartemp->Allocate ( CombineFlags ( AllocateMemory, AllocateVector ), operands );

                                        // permutate to have tokvar1 as the vector
                        if ( tokvar2->IsVector () ) {
                            TTokenVariable     *tokvar;
                            tokvar  = tokvar1; tokvar1  = tokvar2; tokvar2  = tokvar;
                            }

                        *tokvartemp->DataVector     = *tokvar1->DataVector * *tokvar2->DataFloat;
                        }

                    else {
                        if ( verbose ) {
                            sprintf ( buff, "Can not apply operation \"%s\" on these operands.", tokop->Name );
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }
                    } // only memory

                                        // group multiplied by variable
                else if ( nummemory == 1 && numgof == 1 ) {

                                        // permutate to have tokvar1 as the group
                    if ( tokvar2->IsGroup () ) {
                        TTokenVariable     *tokvar;
                        tokvar  = tokvar1; tokvar1  = tokvar2; tokvar2  = tokvar;
                        }

                                        // check operands types
                    if ( tokvar2->Content != ContentData || tokvar1->Content == ContentMatrix ) {
                        if ( verbose ) {
                            sprintf ( buff, "Can not apply operation \"%s\" on these types of operands.", tokop->Name );
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }

                                        // check for operands dimensions
                    if ( ! tokvar2->IsSingleFloat () || ! tokvar1->IsArray2D () ) {
                        if ( verbose ) {
                            sprintf ( buff, "Can not apply operation \"%s\" on these types of operands.", tokop->Name );
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }


                    fileout.CloneParameters ( tokvar1->OutputExt, tokvar1->GetFile () );
                    StringCopy ( tokvartemp->OutputExt, fileout.GetExtension () );

                    tokvartemp->Allocate ( CombineFlags ( AllocateGroup, fileout.IsScalar ( AtomTypeUseOriginal ) ? AllocateFloat : AllocateVector ), operands, tokop );

                    numel       = fileout.NumTracks;
                    numtf       = fileout.NumTime;

                    if ( fileout.IsScalar ( AtomTypeUseOriginal ) )
                        eegb.Resize ( numel, 1 );
                    else
                        vec1.Resize ( numel );


                    GaugeF.WindowRestore ();


                    for ( int i = 0; i < tokvar1->NumFiles (); i++ ) {

                        GaugeF.SetValue ( SuperGaugeDefaultPart, Percentage ( i, tokvar1->NumFiles () - 1 ) );

                        file1   = dynamic_cast<TTracksDoc*> ( docmanager->OpenDoc ( tokvar1->GetFile ( i ), dtOpenOptionsNoView ) );
                                        // try to upcast it
                        ris1    = fileout.IsScalar ( AtomTypeUseOriginal ) ? 0 : dynamic_cast< TRisDoc     * > ( file1 );


                        StringCopy ( fileout.Filename, tokvartemp->GetFile ( i ) );
                        ReplaceExtension ( fileout.Filename, fileout.GetExtension () );


                        for ( int tf = 0; tf < numtf; tf++ ) {

                            if ( fileout.IsScalar ( AtomTypeUseOriginal ) ) {
                                file1->GetTracks ( tf, tf, eegb );

                                for ( int e = 0; e < numel; e++ )
                                    fileout.Write ( eegb ( e , 0 ) * *tokvar2->DataFloat );
                                }
                            else {
                                ris1->GetInvSol  ( 0, tf, tf, vec1, 0 );

                                for ( int e = 0; e < numel; e++ )
                                    fileout.Write ( vec1[ e ] * *tokvar2->DataFloat );
                                }
                            }

                        if ( file1->CanClose ( true ) )     docmanager->CloseDoc ( file1 );
                        } // for file1

                    } // 1 memory, 1 gof


                else if ( numgof == 2 ) {
                                        // go for matrix multiplication

                                        // check operands types
                    if ( tokvar1->Content != ContentMatrix || tokvar2->Content != ContentData || tokvar2->DataType == AtomTypeVector ) {
                        if ( verbose ) {
                            sprintf ( buff, "Can not apply operation \"%s\" on these types of files.", tokop->Name );
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }

                                        // check for operands dimensions
                    if ( ! tokvar1->IsArray2D () || ! tokvar2->IsArray2D () || tokvar1->Size[ 2 ] != tokvar2->Size[ 1 ] ) {
                        if ( verbose ) {
                            sprintf ( buff, "Can not apply operation \"%s\" due to a mismatch between dimensions:\n\nFirst group is %0d x %0d\nSecond group is %0d x %0d", tokop->Name, tokvar1->Size[ 1 ], tokvar1->Size[ 2 ], tokvar2->Size[ 1 ], tokvar2->Size[ 2 ] ),
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }


                    fileout.CloneParameters ( 0, tokvar2->GetFile () );
                                        // override as matrix multiplication produces ris
                    fileout.Type                = ExportTracksRis;
                    StringCopy ( tokvartemp->OutputExt, fileout.GetExtension () );
                    fileout.NumTracks           = numsp;
                                                                       // we don't really know here, but it does not harm
                    tokvartemp->Allocate ( CombineFlags ( AllocateGroup, AllocateMult, AllocateFloat ), operands, tokop );


                    eegb.Resize ( numel, 1 );
                    vec1.Resize ( numsp );


                    GaugeF.WindowRestore ();


                    for ( int i = 0; i < tokvar1->NumFiles (); i++ ) {

                        isdoc   = dynamic_cast<TInverseMatrixDoc*> ( docmanager->OpenDoc ( tokvar1->GetFile ( i ), dtOpenOptionsNoView ) );

                        int         reg     = isdoc->GetRegularizationIndex ( tokvar1->Regularization );
//                      DBGM2 ( IntegerToString ( reg ), tokvar1->Regularization, "Matrix regularization" );

                                        // override type of result
                        tokvartemp->DataType    = isdoc->IsScalar ( AtomTypeUseOriginal ) ? AtomTypeScalar : AtomTypeVector;
                        fileout.SetAtomType ( tokvartemp->DataType );


                        for ( int j = 0; j < tokvar2->NumFiles (); j++ ) {

                            GaugeF.SetValue ( SuperGaugeDefaultPart, Percentage ( i * tokvar2->NumFiles () + j, tokvar1->NumFiles () * tokvar2->NumFiles () - 1 ) );

                            file2   = dynamic_cast<TTracksDoc*> ( docmanager->OpenDoc ( tokvar2->GetFile ( j ), dtOpenOptionsNoView ) );


                            StringCopy ( fileout.Filename, tokvartemp->GetFile ( i * tokvar2->NumFiles () + j ) );
                            ReplaceExtension ( fileout.Filename, fileout.GetExtension () );


                            for ( int tf = 0; tf < numtf; tf++ ) {

                                file2->GetTracks ( tf, tf, eegb );
                                        // this handles all the cases scalar / vectorial
                                isdoc->MultiplyMatrix ( reg, eegb, 0, vec1 );

                                for ( int sp = 0; sp < numsp; sp++ )
                                    if ( tokvartemp->IsFloat () )
                                        fileout.Write ( vec1[ sp ].X ); // inverse is scalar, the actual value has been put in the X component
                                    else
                                        fileout.Write ( vec1[ sp ] );   // write vectors
                                }

                            if ( file2->CanClose ( true ) )     docmanager->CloseDoc ( file2 );
                            } // for file2

                        if ( isdoc->CanClose ( true ) )     docmanager->CloseDoc ( isdoc );
                        } // for isdoc

                    } // only  gof

                break;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case    OpBinaryDiv:

                if ( nummemory == 2 ) {

                    if      ( numfloat  == 2 ) {
                        tokvartemp->Allocate ( CombineFlags ( AllocateMemory, AllocateFloat ),  operands );

                        *tokvartemp->DataFloat      = *tokvar1->DataFloat / *tokvar2->DataFloat;
                        }

                    else if ( numfloat == 1 && numvector == 1 && tokvar1->IsVector () ) {
                        tokvartemp->Allocate ( CombineFlags ( AllocateMemory, AllocateVector ), operands );

                        *tokvartemp->DataVector     = *tokvar1->DataVector / *tokvar2->DataFloat;
                        }

                    else {
                        if ( verbose ) {
                            sprintf ( buff, "Can not apply operation \"%s\" on these operands.", tokop->Name );
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }
                    } // only memory

                                        // group divided by variable
                else if ( nummemory == 1 && numgof == 1 ) {

                                        // check operands types
                    if ( tokvar2->IsGroup () || tokvar2->Content != ContentData || tokvar1->Content == ContentMatrix ) {
                        if ( verbose ) {
                            sprintf ( buff, "Can not apply operation \"%s\" on these types of operands.", tokop->Name );
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }

                                        // check for operands dimensions
                    if ( ! tokvar2->IsSingleFloat () || ! tokvar1->IsArray2D () ) {
                        if ( verbose ) {
                            sprintf ( buff, "Can not apply operation \"%s\" on these types of operands.", tokop->Name );
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }


                    fileout.CloneParameters ( tokvar1->OutputExt, tokvar1->GetFile () );
                    StringCopy ( tokvartemp->OutputExt, fileout.GetExtension () );

                    tokvartemp->Allocate ( CombineFlags ( AllocateGroup, fileout.IsScalar ( AtomTypeUseOriginal ) ? AllocateFloat : AllocateVector ), operands, tokop );

                    numel       = fileout.NumTracks;
                    numtf       = fileout.NumTime;

                    if ( fileout.IsScalar ( AtomTypeUseOriginal ) )
                        eegb.Resize ( numel, 1 );
                    else
                        vec1.Resize ( numel );


                    GaugeF.WindowRestore ();


                    for ( int i = 0; i < tokvar1->NumFiles (); i++ ) {

                        GaugeF.SetValue ( SuperGaugeDefaultPart, Percentage ( i, tokvar1->NumFiles () - 1 ) );

                        file1   = dynamic_cast<TTracksDoc*> ( docmanager->OpenDoc ( tokvar1->GetFile ( i ), dtOpenOptionsNoView ) );
                                        // try to upcast it
                        ris1    = fileout.IsScalar ( AtomTypeUseOriginal ) ? 0 : dynamic_cast< TRisDoc     * > ( file1 );


                        StringCopy ( fileout.Filename, tokvartemp->GetFile ( i ) );
                        ReplaceExtension ( fileout.Filename, fileout.GetExtension () );


                        for ( int tf = 0; tf < numtf; tf++ ) {

                            if ( fileout.IsScalar ( AtomTypeUseOriginal ) ) {
                                file1->GetTracks ( tf, tf, eegb );

                                for ( int e = 0; e < numel; e++ )
                                    fileout.Write ( eegb ( e , 0 ) / *tokvar2->DataFloat );
                                }
                            else {
                                ris1->GetInvSol  ( 0, tf, tf, vec1, 0 );

                                for ( int e = 0; e < numel; e++ )
                                    fileout.Write ( vec1[ e ] / *tokvar2->DataFloat );
                                }
                            }

                        if ( file1->CanClose ( true ) )     docmanager->CloseDoc ( file1 );
                        } // for file1

                    } // 1 memory, 1 gof


                else {
                    if ( verbose ) {
                        sprintf ( buff, "Can not apply operation \"%s\" on these operands.", tokop->Name );
                        ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                        }

                    goto    evalerror;
                    }

                break;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case    OpAssign:

                if ( ! tokvar1->IsAllocated () || ! tokvar1->IsVariable () || tokvar1->Temp ) {
                    if ( verbose ) {
                        sprintf ( buff, "Assignation should be done on a variable." ),
                        ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                        }

                    goto    evalerror;
                    }


                if ( tokvar2->IsSingleFloat () ) {

                    if ( tokvar1->IsSingleFloat () )    *tokvar1->DataFloat  = *tokvar2->DataFloat;
                    else {
                        if ( verbose ) {
                            sprintf ( buff, "Can not assign a value to variable \"%s\"", tokvar1->Name );
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }
                    }

                else if ( tokvar2->IsMemory () && tokvar2->IsVector () ) {

                    if ( tokvar1->IsMemory () && tokvar1->IsVector () ) *tokvar1->DataVector = *tokvar2->DataVector;
                    else {
                        if ( verbose ) {
                            sprintf ( buff, "Can not assign a vector to variable \"%s\"", tokvar1->Name );
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }
                    }


                else if ( tokvar2->IsGroup () ) {

                    if ( tokvar1->IsGroup () ) {
                                        // clear existing list of files - should actually delete existing files / dir?
                        tokvar1->DataGof->Reset ();

                                        // set content properties, except variable name
                        tokvar1->Content    = tokvar2->Content;
                        tokvar1->DataType   = tokvar2->DataType;
                        tokvar1->Size[ 0 ]  = tokvar2->Size[ 0 ];
                        tokvar1->Size[ 1 ]  = tokvar2->Size[ 1 ];
                        tokvar1->Size[ 2 ]  = tokvar2->Size[ 2 ];
                        tokvar1->Size[ 3 ]  = tokvar2->Size[ 3 ];

                                        // create temp dir & file names
                        tokvar1->CreateGof ( tokvar2->GetFile (), tokvar2->NumFiles () );


                        GaugeF.WindowRestore ();

                          // temp to temp: already the right types      or any other case with same types
                        if ( tokvar1->Temp && tokvar2->Temp || tokvar1->HasSameExt ( tokvar2 ) ) {
                                        // do a simple file copy

                                        // copy file names
                            for ( int i = 0; i < tokvar1->NumFiles (); i++ ) {

                                GaugeF.SetValue ( SuperGaugeDefaultPart, Percentage ( i, tokvar1->NumFiles () - 1 ) );

                                ReplaceFilename ( tokvar1->GetFile ( i ), ToFileName ( tokvar2->GetFile ( i ) ) );
                                }

                                        // actually copy files
                            tokvar2->DataGof->CopyFilesTo ( *tokvar1->DataGof, CopyAndKeepOriginals );
                            }

                        else {          // need to convert files
                            fileout.CloneParameters ( tokvar1->OutputExt, tokvar2->GetFile () );

                                        // override type of output
                            tokvar1->DataType   = fileout.IsScalar ( AtomTypeUseOriginal ) ? AtomTypeScalar : AtomTypeVector;

                                        // do convert files
                            for ( int i = 0; i < tokvar1->NumFiles (); i++ ) {

                                GaugeF.SetValue ( SuperGaugeDefaultPart, Percentage ( i, tokvar1->NumFiles () - 1 ) );

                                ReplaceFilename  ( tokvar1->GetFile ( i ), ToFileName ( tokvar2->GetFile ( i ) ) );
                                ReplaceExtension ( tokvar1->GetFile ( i ), fileout.GetExtension () );

                                StringCopy ( fileout.Filename, tokvar1->GetFile ( i ) );

                                fileout.Write ( tokvar2->GetFile ( i ) );
                                }
                            }

                        }
                    else {
                        if ( verbose ) {
                            sprintf ( buff, "Can not assign a group of files to variable \"%s\"", tokvar1->Name );
                            ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                            }

                        goto    evalerror;
                        }
                    }

                else {
                    if ( verbose ) {
                        sprintf ( buff, "Can not assign result to variable \"%s\"", tokvar1->Name );
                        ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                        }

                    goto    evalerror;
                    }


                delete  tokvartemp;     // remove the temp token we don't need
                tokvartemp = tokvar1;   // and push the variable

                break;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        default:
                if ( verbose ) {
                    sprintf ( buff, "Can not apply operation \"%s\" on these operands.", tokop->Name );
                    ShowMessage ( buff, ParserTitleError, ShowMessageWarning );
                    }

                goto    evalerror;

        } // switch op

                                        // tracing the variables types
    //sprintf ( buff, "Op1.%s.%s.%s  \"%s\"  %s.%s.%s.%s  ->  Res.%s.%s.%s",
    //                tokvar1->Temp ? "Temp" : "Var", tokvar1->IsFloat () ? "Float" : "Vector", tokvar1->OutputExt,
    //                tokop->Name,
    //                tokvar2 ? "Op2" : "", tokvar2 ? tokvar2->Temp ? "Temp" : "Var" : "", tokvar2 ? tokvar2->IsFloat () ? "Float" : "Vector" : "", tokvar2 ? tokvar2->OutputExt : "",
    //                tokvartemp->Temp ? "Temp" : "Var", tokvartemp->IsFloat () ? "Float" : "Vector", tokvartemp->OutputExt );
    //DBGM ( buff, "TParser trace" );

                                        // push the result to the stack
    stack.Push ( tokvartemp );

    tokvartemp      = 0;                // get ready for next temp

    eegb.DeallocateMemory ();
    vec1.DeallocateMemory ();
    vec2.DeallocateMemory ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // free temp operands
    for ( int i = 0; i < tokop->NumParameters; i++ ) {

        if ( operands[ i ]->Temp )
            delete  operands[ i ];

        operands[ i ]   = 0;
        }

    } // for Tokens


GaugeS.Next ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we should have only 1 result after all operators have been run
if ( stack.GetNumTokens () != 1 ) {

    if ( verbose )
        ShowMessage ( "Error in the number of operands!", ParserTitleError, ShowMessageWarning );

    goto    evalerror;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//GaugeS.Finished ();
GaugeS.HappyEnd ();

stack.Reset ( false );

return  true;



evalerror:

if ( tokvartemp )                       // token was created on the fly, so delete it
    delete  tokvartemp;

stack.Reset ( false );

return  false;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
