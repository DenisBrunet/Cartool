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

#include    "TCartoolDocManager.h"      // dtOpenOptions

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Class to make the opening and closing of documents much easier.
                                        // OwlNext doc manager must be used to open documents. It sends signals and stuff, it can get messy.
                                        // With this class, opening and closing nitty-gritty code is well encapsulated,
                                        // and caller has simple options to open and close the document.
                                        // By default, TOpenDoc will open the file and create the default view (window), and close all upon destruction.
                                        // It will also test if file is already opened, and avoid re-opening it, as well as avoid closing it at the end.
                                        // Caller can also request to have no views (silent opening), or to let the doc open after all is done.
enum                TOpenDocType
                    {
                    OpenDocHidden           = 1,
                    OpenDocVisible,
                    OpenDocVisibleLocked,

                    CloseDocRestoreAsBefore,
                    CloseDocLetOpen,
//                  CloseDocForce,      // ?could be useful, though it could generate side-effects if another pointer tries to access doc!
                    };


template <class TypeD>
class   TOpenDoc    :   public  TCartoolObjects
{
public:
                    TOpenDoc    ();
                    TOpenDoc    ( const char* filepath, TOpenDocType option );
                   ~TOpenDoc    ();


    bool            IsOpen      ()  const                   { return  Doc != 0; }
    bool            IsNotOpen   ()  const                   { return  Doc == 0; }

    const TypeD*    GetDoc      ()  const                   { return  Doc; }
    TypeD*          GetDoc      ()                          { return  Doc; }


    void            Reset       ();
    bool            Open        ( const char* filepath, TOpenDocType option );
    void            Close       ( TOpenDocType option = CloseDocRestoreAsBefore );  // can force the doc to stay open when all is done !maybe not a good idea if doc was open hidden!
    void            Lock        ();
    void            Unlock      ();

                                        // !No Copy nor Assignation written yet - this class is not supposed to be copied for the moment!

    TypeD*          operator    ->          ()              { return  Doc; }        // access to doc*
                    operator    TypeD*      ()              { return  Doc; }        // cast only to a pointer to doc

    bool            operator    ==          ( const TOpenDoc&   op2 )   const;      // true if both doc are open AND point to the same file
    bool            operator    !=          ( const TOpenDoc&   op2 )   const;      // true if they point to different files, or not opened

private:

    bool            AlreadyOpen;
    bool            Locked;
    TypeD*          Doc;

};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
template <class TypeD>
        TOpenDoc<TypeD>::TOpenDoc ()
{
Reset ();
}

                    
template <class TypeD>
        TOpenDoc<TypeD>::TOpenDoc ( const char* filepath, TOpenDocType option )
{
Reset ();

Open ( filepath, option );
}


template <class TypeD>
        TOpenDoc<TypeD>::~TOpenDoc ()
{
Close ();
}


template <class TypeD>
void    TOpenDoc<TypeD>::Reset ()
{
AlreadyOpen     = false;
Locked          = false;
Doc             = 0;
}


//----------------------------------------------------------------------------
template <class TypeD>
bool    TOpenDoc<TypeD>::operator== ( const TOpenDoc& op2 )    const
{
if ( IsNotOpen () || op2.IsNotOpen () )
    return  false;
                                        // Full path equality
return  StringIs ( Doc->GetDocPath (), op2.Doc->GetDocPath () );
}


template <class TypeD>
bool    TOpenDoc<TypeD>::operator!= ( const TOpenDoc& op2 )    const
{
if ( IsNotOpen () || op2.IsNotOpen () )
    return  true;
                                        // Full path inequality
return  StringIsNot ( Doc->GetDocPath (), op2.Doc->GetDocPath () );
}


//----------------------------------------------------------------------------

template <class TypeD>
void    TOpenDoc<TypeD>::Lock ()
{
if ( Locked )
    return;
                                        // remember that we forced the lock
Locked          = true;             
                                        // however, there might be some cases where it was already locked before (quite unlikely, though)
Doc->PreventClosing ();             
}


template <class TypeD>
void    TOpenDoc<TypeD>::Unlock ()
{
if ( ! Locked )
    return;
                                        // just reset my flag, but don't interfere with any existing DoNotClose flag
Locked          = false;
                                        // ?do this or not?
Doc->AllowClosing ();
}


//----------------------------------------------------------------------------
template <class TypeD>
void   TOpenDoc<TypeD>::Close ( TOpenDocType option )
{
                                        // well, are we open or not?
if ( ! IsOpen () ) {
    Reset ();                           // for safety
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // first, check if we did the lock, and reset it (tested by CanClose)
Unlock ();


bool                letopen         =    option == CloseDocLetOpen 
                                      || option == CloseDocRestoreAsBefore && AlreadyOpen;

                                        // close doc if it is open and we want to restore the orginal state
if ( ! letopen
  && Doc->TBaseDoc::CanClose ( true ) ) // TRisDoc doesn't seem to see this function?!

    CartoolDocManager->CloseDoc ( Doc );

                                        // forget about the doc, even if it was not actually closed, the class task is done here
Reset ();
}


//----------------------------------------------------------------------------
template <class TypeD>
bool   TOpenDoc<TypeD>::Open ( const char* filepath, TOpenDocType option )
{
                                        // trying to re-open the current doc?
if ( IsOpen () && StringIs ( filepath, Doc->GetDocPath () ) )
                                        // avoid closing and reopening, we are already good
    return  true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // in all other cases, close first any potentially opened file
Close ();

                                        // make an editable copy
TFileName           path ( filepath, TFilenameExtendedPath );


if (   StringIsEmpty ( path )
  || ! CanOpenFile   ( path ) )         // test beforehand if file exists, so we can gracefully return before requesting the main application - this could be restricted to OpenDocHidden only?
    return  false;

                                        // remember if document was previously opened in the main application
AlreadyOpen     = CartoolDocManager->IsOpen ( path );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // overload flag to avoid opening view if not interactive
if ( CartoolApplication->IsNotInteractive () && option != OpenDocHidden )

    option  = OpenDocHidden;

                                        // open doc - if doc was already open & visible, the hidden flag will have of course no effect here
Doc             = dynamic_cast< TypeD* > ( CartoolDocManager->OpenDoc ( path, option == OpenDocHidden ? dtOpenOptionsNoView : dtOpenOptions ) );

                                        // failing?
if ( Doc == 0 ) {
    Reset ();
    return  false;
    }
                                        // Here opened successfully

if ( option == OpenDocVisibleLocked )   Lock ();
else                                    Unlock ();


return  true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
