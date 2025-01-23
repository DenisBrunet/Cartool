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

#include    "Geometry.TPoint.h"
#include    "Files.TGoF.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // types of tokens
enum                TokenType
                    {
                    UnknownToken,

                    TokenIdentifier,    // used during tokenization

                    TokenValue,         // constant number
                    TokenVariable,      // variable provided by caller
                    TokenOperator,      // arithmetic operators and predefined functions
                    };

extern const char   TokenTypeStrings[][ 16 ];

                                        // List of operators
enum                OperatorType
                    {
                    OpUnknown,

                    OpParenthesisOpen,
                    OpParenthesisClose,

                    OpAbs,
                    OpScalar,
                    OpSqr,
                    OpSqrt,

                    OpBinaryMult,
                    OpBinaryDiv,

                    OpBinaryPlus,       // unary + and - are not defined here, but are still allowed - they are converted to equivalent binary op at some points
                    OpBinaryMinus,

                    OpAssign
                    };


//----------------------------------------------------------------------------
                                        // General definition of a token
class   TToken
{
public:
                    TToken ();
    virtual        ~TToken ()   {};     // important, as we will have list of derived objects referenced as TToken


    TokenType       Type;
    bool            Temp;               // used to flag temp variables for deletion


    bool            IsIdentifier    ()  const           { return    Type == TokenIdentifier;                    }
    bool            IsValue         ()  const           { return    Type == TokenValue;                         }
    bool            IsVariable      ()  const           { return    Type == TokenVariable;                      }

    bool            IsOperator      ()  const           { return    Type == TokenOperator;                      }
    virtual bool    IsOperator      ( OperatorType )    const   { return  TToken::IsOperator ();                }   // to be overriden by TTokenOperator

    virtual bool    IsFunction      ()  const           { return    false;                                      }   // to be overriden by TTokenOperator
                                        // function and () results in TokenValue
    bool            IsOperand       ()  const           { return    Type == TokenValue || Type == TokenVariable;}

};


//----------------------------------------------------------------------------
                                        // Identifier token: anything not an operator and not a number, like a variable name, a function name, a keyword, etc...
                                        // There should not remain any once the expression string has been parsed, though
constexpr int       TokenIdentifierNameLength   = 64;


class   TTokenIdentifier :  public  TToken
{
public:
                    TTokenIdentifier ();
                    TTokenIdentifier ( const char* tokenchar );


    char            Identifier[ TokenIdentifierNameLength ];
};


//----------------------------------------------------------------------------

class   TTokenOperator;


enum                TokenContent
                    {
                    UnknownContent,

                    ContentData,
                    ContentMatrix,
                    };


enum                TokenAllocationType
                    {
                    AllocateMemory      = 0x001,
                    AllocateGroup       = 0x002,

                    AllocateFloat       = 0x010,
                    AllocateVector      = 0x020,

                    AllocateMult        = 0x100,
                    };


constexpr int       TokenVariableNameLength     = 64;
constexpr int       TokenVariableExtLength      = 32;
constexpr int       TokenVariableRegLength      = 16;

                                        // Variable token
                                        // the variable itself is accessed through a pointer to some external data
class   TTokenVariable :    public  TToken
{
public:
                    TTokenVariable ();
                   ~TTokenVariable ();

                    TTokenVariable ( const TToken* token );
                    TTokenVariable ( const char* name, float*         tovar, int size0 = 0, int size1 = 0, int size2 = 0 );
                    TTokenVariable ( const char* name, TVector3Float* tovar, int size0 = 0, int size1 = 0, int size2 = 0 );
                    TTokenVariable ( const char* name, TGoF*          gof, const char* regularization = 0, const char* outputext = 0 );


    char            Name [ TokenVariableNameLength ];   // Name is different from Identifier: Name has the real definition, Identifier is the token retrieved from parsing


    TokenContent    Content;                            // Data typology: data (eeg, ris) / matrix

                                                        // Data format
    AtomType        DataType;                           // float, double, int, vector 3D etc of the data
    int             Size[ 4 ];                          // Size[ 0 ] is # of dimensions, then Size[ 1 / 2 / 3 ] the corresponding sizes
    char            OutputExt       [ TokenVariableExtLength ]; // Preferred extension for Group of Files
    char            Regularization  [ TokenVariableRegLength ]; // required for inverse matrices, ignored otherwise

                                                        // one of these pointers goes to the right variable
    float*          DataFloat;
    TVector3Float*  DataVector;
    TGoF*           DataGof;


    void            Reset           ();

    bool            IsAllocated     ()    const             { return      DataGof != 0 || DataFloat != 0 || DataVector != 0; }
    void            Allocate        ( TokenAllocationType how, TTokenVariable* tokvar[], TTokenOperator* tokop = 0 );
    bool            CreateGof       ( const char* path, int numfiles );


    bool            IsGroup         ()  const             { return      (bool) DataGof; }
    bool            IsMemory        ()  const             { return    ! (bool) DataGof; }

    bool            IsUnknownContent()  const             { return    Content == UnknownContent;}
    bool            IsData          ()  const             { return    Content == ContentData;   }
    bool            IsMatrix        ()  const             { return    Content == ContentMatrix; }

    bool            IsFloat         ()  const             { return    DataType == AtomTypeScalar; }
    bool            IsVector        ()  const             { return    DataType == AtomTypeVector; }

    bool            IsScalar        ()  const             { return    Size[ 0 ] == 0; }
    bool            IsArray2D       ()  const             { return    Size[ 0 ] == 2; }
    bool            IsSingleFloat   ()  const             { return    IsMemory () && IsFloat () && IsScalar (); }


    int             GetLinearDim    ()  const             { return    ( Size[ 1 ] ? Size[ 1 ] : 1 ) * ( Size[ 2 ] ? Size[ 2 ] : 1 ) * ( Size[ 3 ] ? Size[ 3 ] : 1 ); }
    const char*     GetFile         ( int i = 0 )   const { return    IsGroup () && DataGof ? (*DataGof)[ i ] : 0; }
          char*     GetFile         ( int i = 0 )         { return    IsGroup () && DataGof ? (*DataGof)[ i ] : 0; }
    int             NumFiles        ()  const             { return    IsGroup () && DataGof ?   DataGof->NumFiles () : 0; }


    bool            HasSameType     ( const TTokenVariable* op2 )   const   { return    DataType == op2->DataType; }
    bool            HasSameSize     ( const TTokenVariable* op2 )   const   { return    Size[ 0 ] == op2->Size[ 0 ] && Size[ 1 ] == op2->Size[ 1 ] && Size[ 2 ] == op2->Size[ 2 ] && Size[ 3 ] == op2->Size[ 3 ]; }
    bool            HasSameGroup    ( const TTokenVariable* op2 )   const   { return    IsGroup () == op2->IsGroup (); }
    bool            HasSameFiles    ( const TTokenVariable* op2 )   const   { return    IsGroup () && op2->IsGroup () && DataGof->NumFiles () == op2->DataGof->NumFiles (); }
    bool            HasSameExt      ( const TTokenVariable* op2 )   const   { return    StringIs ( OutputExt, op2->OutputExt ); }


protected:

    void            CompoundFilenames   ( TTokenVariable* tokvar[], TTokenOperator* tokop );

};


//----------------------------------------------------------------------------
                                        // Scalar value token (constants)
class   TTokenValue :       public  TTokenVariable
{
public:
                    TTokenValue ();
                   ~TTokenValue ();

                    TTokenValue ( const char*   tokenchar );
                    TTokenValue ( const TToken* token );


    const float&    Value ()    const                       { return  *DataFloat; }
          float&    Value ()                                { return  *DataFloat; }
};


//----------------------------------------------------------------------------

constexpr int       ParenthesisPriority     = 100;

                                        // Fields needed for an operator
class   TTokenOperatorVars
{
public:
    const char*     Name;
    OperatorType    Code;
    int             Priority;
    int             NumParameters;
    bool            Function;
};
                                        // table which defines the legal operations
                                        // if strings of operators begin with the SAME chars: put first the one with the highest priority
extern const TTokenOperatorVars     LegalOperators[];

                                        // Operator token
class   TTokenOperator  :   public  TToken,
                            public  TTokenOperatorVars
{
public:
                    TTokenOperator ();
                    TTokenOperator ( const char*               tokenchar );
                    TTokenOperator ( const TTokenOperatorVars& opvar );
                    TTokenOperator ( const TTokenOperator*     tokop );


    using           TToken::IsOperator;
    bool            IsOperator      ( OperatorType code )   const   { return  TToken::IsOperator () && Code == code; }
    bool            IsFunction      ()                      const   { return  Function;                              }

protected:

    void            Reset ();

};


//----------------------------------------------------------------------------
                                        // Compiled expression at various stages
class   TTokensStack
{
public:
//                          TTokensStack ();
                           ~TTokensStack ();


    bool                    IsEmpty         ()  const   { return    Tokens.IsEmpty    (); }
    bool                    IsNotEmpty      ()  const   { return    Tokens.IsNotEmpty (); }


    void                    Reset           ( bool deleteall ); // if false, delete only Temp tokens

    void                    Push            ( const TToken* token );
    TToken*                 Pop             ();
    void                    Insert          ( TToken* token, const TToken* beforetoken )    { Tokens.Insert ( token, beforetoken ); Tokens.UpdateIndexes (); }
    void                    Remove          ( const TToken* token )                         { Tokens.Remove ( token );              Tokens.UpdateIndexes (); }


    int                     GetNumTokens    ()  const   { return    Tokens.Num (); }

    const TToken*           GetLast         ()  const   { return    Tokens.GetLast (); }
    const TTokenOperator*   GetLastOperator ()  const   { return    Tokens.GetLast () && Tokens.GetLast ()->IsOperator () ? static_cast<const TTokenOperator*> ( Tokens.GetLast () ) : 0; }
    const TTokenVariable*   GetLastVariable ()  const   { return    Tokens.GetLast () && Tokens.GetLast ()->IsVariable () ? static_cast<const TTokenVariable*> ( Tokens.GetLast () ) : 0; }
    const TTokenValue*      GetLastValue    ()  const   { return    Tokens.GetLast () && Tokens.GetLast ()->IsValue    () ? static_cast<const TTokenValue*   > ( Tokens.GetLast () ) : 0; }

    const TTokenVariable*   GetVariable     ( const char* varname ) const;

    bool                    GetOperands     ( TTokenVariable* operands[], int n );  // enough operands on the top of stack?


    void                    Show            ( const char* title )   const;


    const TToken*           operator    []  ( int index )   const           { return    Tokens[ index ]; }
          TToken*           operator    []  ( int index )                   { return    Tokens[ index ]; }

protected:

    TList<TToken>           Tokens;         // this list will never move its nodes, important for insertion / deletion relative to a specific token
};


//----------------------------------------------------------------------------

constexpr char*     ParserTitleError        = "Syntax Error";

                                        // This is a simple parser used for arithmetic evaluation
                                        // It recognizes constants, variables from caller, and a few functions
                                        // The interesting part, though, is at execution time, at it can
                                        // apply an expression to whole groups of files at once.
                                        // It also cares for scalar and 3D vectors results at the same time,
                                        // with all possible combinations.
class   TParser
{
public:
//                  TParser ();
//                 ~TParser ();


    void            Reset               ()              { Tokens.Reset ( true ); }

    bool            Parse               ( const char* expression, const TTokensStack& variables, bool verbose );
    bool            Evaluate            ( const char* expression, const TTokensStack& variables, bool verbose );
    bool            Evaluate            ( bool verbose );

protected:

    TTokensStack    Tokens;

    bool            CheckChars          ( const char* expression, bool verbose )    const;
    bool            Tokenize            ( const char* expression, const TTokensStack& variables, TTokensStack& tokens, bool verbose )   const;
    void            SimplifiesUnaryOperators        ( TTokensStack& tokenstack );
    void            UnaryOperatorToBinaryOperator   ( TTokensStack& tokenstack );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
