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

#include    "TBaseDoc.h"

#include    "TArray1.h"
#include    "Volumes.TTalairachOracle.h"

#include    "TPotentialsView.h"
#include    "TInverseView.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TBaseDoc::TBaseDoc ( TDocument* parent )
      : TFileDocument ( parent )
{
LastGroupDoc        = 0;
DoNotClose          = false;
GeometryTransform   = 0;
}


        TBaseDoc::~TBaseDoc ()
{
if ( GeometryTransform ) {
    delete GeometryTransform;
    GeometryTransform = 0;
    }
}


//----------------------------------------------------------------------------
bool    TBaseDoc::CanClose ()
{
return  CanClose ( false );
}


bool    TBaseDoc::CanClose ( bool silent )
{                                       // if App is closing, ignore this test!

//if ( ! CartoolApplication->Closing && (bool) UsedBy ) {

if ( CartoolApplication->Closing ) {    // sauve qui peut!

    return true;
    }
else {

    if ( DoNotClose ) {                 // there is a lock on this doc

//      ShowMessage ( "Can not close now..." NewLine "Finish your current process first.", "Closing", ShowMessageWarning, CartoolMainWindow );
        return false;
        }

    else if ( (bool) UsedBy ) {         // if used

        if ( ! silent ) {

            char                buff [ 1024 ];
            char                title[ 1024 ];

            StringCopy ( buff, (int) UsedBy == 1 ? "Close this document beforehand:\n" : "Close these documents beforehand:\n" );

            for ( int i = 0; i < (int) UsedBy; i++ )
                StringAppend ( buff, NewLine Tab, UsedBy[ i ]->GetTitle () );

            StringCopy  ( title, "Closing ", GetTitle () );

            ShowMessage ( buff, title, ShowMessageWarning, CartoolMainWindow );
            }

        return false;
        }
    else {
        if ( silent ) {
            SetDirty ( false );         // force flag to not dirty
            return  true;               // force success
            }

        return  TDocument::CanClose (); // the regular way
        }
    }
}


//----------------------------------------------------------------------------
bool    TBaseDoc::CanSync ( TWindow* towin )    const
{
return  dynamic_cast<TPotentialsView*> ( towin )
     || dynamic_cast<TInverseView*   > ( towin );
//   || dynamic_cast<TTracksView*    > ( towin );   // that would make sense to implement that at some point...
}


//----------------------------------------------------------------------------
void    TBaseDoc::SetFocus ( uint viewid )
{
if ( viewid ) {

    TView*              v           = CartoolDocManager->GetView ( viewid );

    if ( v )
        v->GetWindow()->GetParentO()->SetFocus();
    }
}


//----------------------------------------------------------------------------
bool    TBaseDoc::NotifyDocViews ( int event, TParam2 item, TView* viewexclude, TDocument* docexclude )
{
TBaseDoc*           doc;
bool                result;
bool                rr;

                                        // send message to its own views firsw
result  = NotifyViews ( event, item, viewexclude );

                                        // .. then to all views of all docs pointing to this doc
for ( int i = 0; i < (int) UsedBy; i++ ) {

    doc     = dynamic_cast<TBaseDoc*> ( UsedBy[ i ] );

    if ( doc && doc != docexclude ) {

        rr      = doc->NotifyDocViews ( event, item, viewexclude, this );  // looks like hierarchical?
//      rr      = doc->NotifyViews ( event, item );
        result  = result || rr;
        }
    }


return result;
}


//----------------------------------------------------------------------------
bool    TBaseDoc::NotifyFriendViews  ( uint friendshipid, int event, TParam2 item, TView* viewexclude )
{
if ( friendshipid == 0 )
    return  false;

                                        // we aim only at TBaseView derived windows
TBaseView*          view            = CartoolDocManager->GetView ( friendshipid );

if ( view == 0 )
    return  false;


uint                friendid        = view->FriendshipId;
bool                result          = false;

for ( TBaseDoc*  doc = CartoolDocManager->DocListNext ( 0 ); doc != 0; doc = CartoolDocManager->DocListNext ( doc ) )
for ( TBaseView* v = doc->GetViewList (); v != 0; v = doc->NextView ( v ) )

    if ( v->IsFriendship ( friendid ) ) {

//      bool    rr      = doc->NotifyViews (event, item, viewexclude);
        bool    rr      = doc->QueryViews ( event, item, viewexclude );

        result  = result || rr;     // !done this way to make sure QueryViews got called, even if result is already true - do not change that!

        break;                      // go to next document
        }


return  result;
}


//----------------------------------------------------------------------------
                                        // !A side effect of dynamic_cast is that it will stop at the first non-TBaseView pointer!
TBaseView* TBaseDoc::GetViewList () const
{
return  dynamic_cast<TBaseView*> ( TDocument::GetViewList() );
}


//----------------------------------------------------------------------------
TBaseView* TBaseDoc::GetViewList ( TLinkManyDoc* group, bool equal )
{
TView*          view;
TBaseView*      baseview;


for ( view = GetViewList(); view != 0; view = NextView ( view ) ) {

    baseview = dynamic_cast<TBaseView*> ( view );

    if ( ! baseview )
        continue;

    if ( ! ( ( baseview->GODoc == group ) ^ equal ) )
        return  baseview;
    }


return  0;
}


TBaseView*  TBaseDoc::NextView ( const TView* view )
{
return  dynamic_cast<TBaseView*> ( TDocument::NextView ( view ) );
}


TBaseView*  TBaseDoc::NextView ( TBaseView* bview, TLinkManyDoc* group, bool equal )
{
TView*          view;
TBaseView*      baseview;


for ( view= NextView ( bview ); view != 0; view = NextView ( view ) ) {

    baseview = dynamic_cast<TBaseView*> ( view );

    if ( !baseview )
        continue;

    if ( ! ( ( baseview->GODoc == group ) ^ equal ) )
        return  baseview;
    }


return  0;
}


//----------------------------------------------------------------------------
void    TBaseDoc::MinimizeViews ()
{
TView*              view;
TBaseView*          baseview;


for ( view = GetViewList (); view != 0; view = NextView ( view ) ) {

    baseview = dynamic_cast< TBaseView*> ( view );

    if ( baseview )
        baseview->WindowMinimize ();
    }
}


//----------------------------------------------------------------------------
char*   TBaseDoc::GetBaseFileName ( char* basefilename )    const
{
if ( basefilename == 0 )
    return  0;

                                        // get full file name with path
StringCopy ( basefilename, GetDocPath () );

                                        // MFF files are directory, so move one dir up
if ( IsGeodesicsMFFPath ( basefilename ) ) {
                                        // remove the "\signal*.bin" part
    RemoveFilename ( basefilename );
                                        // also remove any useless .mff extension
    if ( crtl::IsExtension ( basefilename, FILEEXT_EEGMFFDIR ) )
        RemoveExtension ( basefilename );
    }
else
                                        // just remove the file extension
    RemoveExtension ( basefilename );


return  basefilename;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
