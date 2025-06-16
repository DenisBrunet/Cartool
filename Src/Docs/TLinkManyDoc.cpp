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

#include    "TLinkManyDoc.h"

#include    "MemUtil.h"

#include    "TArray1.h"
#include    "TList.h"
#include    "Dialogs.Input.h"
#include    "Files.TSplitLinkManyFile.h"

#include    "TTracksDoc.h"
#include    "TRisDoc.h"
#include    "TElectrodesDoc.h"
#include    "TSolutionPointsDoc.h"
#include    "TInverseMatrixDoc.h"
#include    "TVolumeDoc.h"
#include    "TRoisDoc.h"
#include    "TSegDoc.h"

#include    "TTracksView.h"
#include    "TFrequenciesView.h"
#include    "TPotentialsView.h"
#include    "TInverseView.h"
#include    "TLinkManyView.h"

#include    "TMicroStatesSegDialog.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TLinkManyDoc::TLinkManyDoc ( TDocument *parent )
      : TBaseDoc ( parent )
{
UnspecifiedDocPath  = false;

LastEegViewId       = 0;
CommandsCloning     = true;
}

                                        // OwlNext does not seem to call Close(), so we have to do that explicitly
        TLinkManyDoc::~TLinkManyDoc ()
{
Close ();
}


bool	TLinkManyDoc::InitDoc ()
{
return  Open ( ofRead, 0 );
}


bool    TLinkManyDoc::Commit ( bool force )
{
if ( ! ( IsDirty () || force ) )
    return true;


TOutStream*         os              = OutStream ( ofWrite );

if ( os->fail() )   return false;


for ( int i = 0; i < (int) ListXyzDoc;  i++ )   *os << ListXyzDoc [ i ]->GetDocPath() << NewLine;
for ( int i = 0; i < (int) ListSpDoc;   i++ )   *os << ListSpDoc  [ i ]->GetDocPath() << NewLine;
for ( int i = 0; i < (int) ListIsDoc;   i++ )   *os << ListIsDoc  [ i ]->GetDocPath() << NewLine;
for ( int i = 0; i < (int) ListEegDoc;  i++ )   *os << ListEegDoc [ i ]->GetDocPath() << NewLine;
for ( int i = 0; i < (int) ListRisDoc;  i++ )   *os << ListRisDoc [ i ]->GetDocPath() << NewLine;
for ( int i = 0; i < (int) ListRoisDoc; i++ )   *os << ListRoisDoc[ i ]->GetDocPath() << NewLine;
for ( int i = 0; i < (int) ListMriDoc;  i++ )   *os << ListMriDoc [ i ]->GetDocPath() << NewLine;


delete  os;

                                        // once saved, do not allow updating the file path
UnspecifiedDocPath  = false;

SetDirty ( false );

return true;
}


bool    TLinkManyDoc::Revert ( bool clear )
{
if ( ! TFileDocument::Revert ( clear ) )
    return false;

if ( ! clear ) {

    Close ();

    Open ( ofRead );

    GroupTileViews ( CombineFlags ( GroupTilingViews_BestFitSize, GroupTilingViews_Insert ) );
    }

SetDirty ( false );

return true;
}


//----------------------------------------------------------------------------
bool    TLinkManyDoc::Open ( int /*mode*/, const char* path )
{
TSplitLinkManyFile  lm;

TTracksDoc*         doceeg;
TFreqDoc*           docfreq;
TRoisDoc*           docrois;
TElectrodesDoc*     docxyz;
TSolutionPointsDoc* docsp;
TInverseMatrixDoc*  docis;
TRisDoc*            docris;
TVolumeDoc*         docmri;


if ( path )
    SetDocPath ( path );


if ( GetDocPath () ) {

    SetDirty ( false );
                                        // read lm and organize it into lists
    lm.Open ( GetDocPath () );

    }
else {                                  // create -> ask which files to link through dialogs
    SetDirty ( true );


    GetFileFromUser  getfiles  ( "Open Files",         AllUsualFilesFilter, 1, GetFileMulti );
    GetFileFromUser  getlmfile ( "New Link Many File", AllLmFilesFilter,    1, GetFileWrite );


    if ( ! getfiles.Execute () )
        return  false;


    if ( ! getlmfile.Execute () )
        return  false;

    if ( ! CanOpenFile ( (char*) getlmfile, CanOpenFileWriteAndAsk ) ) {
        return false;
        }


    SetDocPath ( getlmfile );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    for ( int i = 0; i < (int) getfiles; i++ ) {

        char*   tof     = getfiles[ i ];

        if ( crtl::IsExtensionAmong ( tof, AllEegFreqFilesExt ) ) {
            lm.leeg.Add ( tof );
            continue;
            }

        if ( crtl::IsExtensionAmong ( tof, AllCoordinatesFilesExt ) ) {
            lm.lxyz.Add ( tof );
            continue;
            }

        if ( crtl::IsExtensionAmong ( tof, AllSolPointsFilesExt ) ) {
            lm.lsp.Add ( tof );
            continue;
            }

        if ( crtl::IsExtensionAmong ( tof, AllInverseFilesExt ) ) {
            lm.lis.Add ( tof );
            continue;
            }

        if ( crtl::IsExtensionAmong ( tof, AllRisFilesExt ) ) {
            lm.lris.Add ( tof );
            continue;
            }

        if ( crtl::IsExtensionAmong ( tof, AllRoisFilesExt ) ) {
            lm.lrois.Add ( tof );
            continue;
            }

        if ( crtl::IsExtensionAmong ( tof, AllMriFilesExt ) ) {
            lm.lmri.Add ( tof );
            continue;
            }
        }

    lm.Sort ();
    } // create doc


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                                        // if a new empty lm, allow to update the file path with future dropped files
UnspecifiedDocPath  = StringIs ( GetTitle (), EmptyLmFilename );    // or StringEndsWith?

                                        // can we find or ask for all the requested files ?
if ( lm.leeg .IsNotEmpty () && ! lm.leeg .CanOpenFiles ( CanOpenFileReadAndAsk ) )      goto AbortOpen;
if ( lm.lris .IsNotEmpty () && ! lm.lris .CanOpenFiles ( CanOpenFileReadAndAsk ) )      goto AbortOpen;
if ( lm.lxyz .IsNotEmpty () && ! lm.lxyz .CanOpenFiles ( CanOpenFileReadAndAsk ) )      goto AbortOpen;
if ( lm.lsp  .IsNotEmpty () && ! lm.lsp  .CanOpenFiles ( CanOpenFileReadAndAsk ) )      goto AbortOpen;
if ( lm.lis  .IsNotEmpty () && ! lm.lis  .CanOpenFiles ( CanOpenFileReadAndAsk ) )      goto AbortOpen;
if ( lm.lrois.IsNotEmpty () && ! lm.lrois.CanOpenFiles ( CanOpenFileReadAndAsk ) )      goto AbortOpen;
if ( lm.lmri .IsNotEmpty () && ! lm.lmri .CanOpenFiles ( CanOpenFileReadAndAsk ) )      goto AbortOpen;


//if ( ! (bool) leeg && ! (bool) lris )     goto AbortOpen;

                                        // test validity between all files
if ( ! Compatibility ( lm.leeg, lm.lrois, lm.lxyz, lm.lsp, lm.lis, lm.lris ) )
    goto    AbortOpen;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here all the files match together
                                        // we can open the doc's, finishing with Eeg's and Ris's
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // These Docs do not allow their views to be part of a group
                                        // therefore it is useless to create a new view belonging to this group
int                 i;


for ( /*int*/ i = 0; i < (int) lm.lxyz; i++ ) {

    docxyz  = dynamic_cast <TElectrodesDoc*> ( CartoolDocManager->IsOpen ( lm.lxyz[ i ] ) );

    if ( ! docxyz ) {
        docxyz = dynamic_cast <TElectrodesDoc*> ( CartoolDocManager->OpenDoc ( lm.lxyz[ i ], dtOpenOptions ) );
        docxyz->GetViewList ()->WindowMinimize ();
        }
    if ( docxyz )
        ListXyzDoc.Append ( docxyz );
    }


for ( /*int*/ i = 0; i < (int) lm.lsp; i++ ) {

    docsp   = dynamic_cast <TSolutionPointsDoc*> ( CartoolDocManager->IsOpen ( lm.lsp[ i ] ) );

    if ( ! docsp ) {
        docsp = dynamic_cast <TSolutionPointsDoc*> ( CartoolDocManager->OpenDoc ( lm.lsp[ i ], dtOpenOptions ) );
        docsp->GetViewList ()->WindowMinimize ();
        }
    if ( docsp )
        ListSpDoc.Append ( docsp );
    }


for ( /*int*/ i = 0; i < (int) lm.lis; i++ ) {

    docis   = dynamic_cast <TInverseMatrixDoc*> ( CartoolDocManager->IsOpen ( lm.lis[ i ] ) );

    if ( ! docis ) {
        docis = dynamic_cast <TInverseMatrixDoc*> ( CartoolDocManager->OpenDoc ( lm.lis[ i ], dtOpenOptions ) );
        docis->GetViewList ()->WindowMinimize ();
        }
    if ( docis )
        ListIsDoc.Append ( docis );
    }


for ( /*int*/ i = 0; i < (int) lm.lmri; i++ ) {

    docmri  = dynamic_cast <TVolumeDoc *> ( CartoolDocManager->IsOpen ( lm.lmri[ i ] ) );

    if ( ! docmri ) {
        docmri = dynamic_cast <TVolumeDoc *> ( CartoolDocManager->OpenDoc ( lm.lmri[ i ], dtOpenOptions ) );
        docmri->GetViewList ()->WindowMinimize ();
        }
    if ( docmri )
        ListMriDoc.Append ( docmri );
    }


for ( /*int*/ i = 0; i < (int) lm.lrois; i++ ) {

    docrois  = dynamic_cast <TRoisDoc*> ( CartoolDocManager->IsOpen ( lm.lrois[ i ] ) );

    if ( ! docrois ) {
        docrois = dynamic_cast <TRoisDoc*> ( CartoolDocManager->OpenDoc ( lm.lrois[ i ], dtOpenOptions ) );
        docrois->GetViewList ()->WindowMinimize ();
        }
    if ( docrois )
        ListRoisDoc.Append ( docrois );
    }

                                        // These Docs will create a new view belonging to this group
                                        // we open them now, so they can find pointers to other docs (XYZ...)
for ( /*int*/ i = 0; i < (int) lm.leeg; i++ ) {

    doceeg  = dynamic_cast <TTracksDoc*> ( CartoolDocManager->IsOpen ( lm.leeg[ i ] ) );

    if ( ! doceeg )                      // open without the default view
        doceeg = dynamic_cast <TTracksDoc*> ( CartoolDocManager->OpenDoc ( lm.leeg[ i ], dtOpenOptions | dtNoAutoView ) );

                                        // manually and selectively create the view with this group
    TBaseView*          view;

    if ( ( docfreq = dynamic_cast <TFreqDoc*> ( doceeg ) ) != 0 )
        view    = new TFrequenciesView ( *docfreq, 0, this );
    else
        view    = new TTracksView      ( *doceeg,  0, this );

    CartoolDocManager->PostEvent ( dnCreate, *view );

    view->WindowMinimize ();

    if ( doceeg )
        ListEegDoc.Append ( doceeg );
    }


for ( /*int*/ i = 0; i < (int) lm.lris; i++ ) {

    docris  = dynamic_cast <TRisDoc *> ( CartoolDocManager->IsOpen ( lm.lris[ i ] ) );

    if ( ! docris )
        docris = dynamic_cast <TRisDoc *> ( CartoolDocManager->OpenDoc ( lm.lris[ i ], dtOpenOptions | dtNoAutoView ) );

    TBaseView*          view        = new TTracksView ( *docris, 0, this );

    CartoolDocManager->PostEvent ( dnCreate, *view );

    view->WindowMinimize ();

    if ( docris )
        ListRisDoc.Append ( docris );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // lock these docs
for ( /*int*/ i = 0; i < (int) ListXyzDoc;  i++ )   ListXyzDoc [ i ]->AddLink ( this );
for ( /*int*/ i = 0; i < (int) ListSpDoc;   i++ )   ListSpDoc  [ i ]->AddLink ( this );
for ( /*int*/ i = 0; i < (int) ListIsDoc;   i++ )   ListIsDoc  [ i ]->AddLink ( this );
for ( /*int*/ i = 0; i < (int) ListMriDoc;  i++ )   ListMriDoc [ i ]->AddLink ( this );
for ( /*int*/ i = 0; i < (int) ListEegDoc;  i++ )   ListEegDoc [ i ]->AddLink ( this );
for ( /*int*/ i = 0; i < (int) ListRisDoc;  i++ )   ListRisDoc [ i ]->AddLink ( this );
for ( /*int*/ i = 0; i < (int) ListRoisDoc; i++ )   ListRoisDoc[ i ]->AddLink ( this );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numspdoc        = GetNumSpDoc ();

if ( numspdoc && (bool) ListMriDoc ) {

    TVolumeDoc*         tohead;
    TVolumeDoc*         tobrain;
    TVolumeDoc*         togrey;

    GuessHeadBrainGreyMris ( tohead, tobrain, togrey );

                                        // 1NN interpolation could now cope with whatever MRI is given
                                                    // tobrain is not used at the moment
#if defined(DisplayInverseInterpolation4NN)
    if ( ! ListSpDoc[ 0 ]->BuildInterpolation ( SPInterpolation4NN, togrey ) ) {
#else
    if ( ! ListSpDoc[ 0 ]->BuildInterpolation ( SPInterpolation1NN, togrey ) ) {
#endif
//      ListSpDoc()->RemoveLink ( this );
//      ListSpDoc.Remove ( ListSpDoc(), DontDeallocate );

        Close ();               // a bit rough, but needed to clean up correctly the place
        return false;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // can create scalp potential ?
if ( (bool) ListEegDoc && (bool) ListXyzDoc ) {

    for ( /*int*/ i = 0; i < (int) ListEegDoc; i++ ) {
                                        // find the right Eeg view id
        for ( TBaseView* view = ListEegDoc[ i ]->GetViewList ( this ); view != 0; view = ListEegDoc[ i ]->NextView ( view, this ) )

            if ( dynamic_cast<TTracksView *> ( view ) )

                { LastEegViewId   = view->GetViewId(); break; }


        TPotentialsView*    viewpm      = new TPotentialsView ( *ListEegDoc[ i ], 0, this );

        CartoolDocManager->PostEvent ( dnCreate, *viewpm );

        viewpm->WindowMinimize ();
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // can create inverse solution display from eeg ?
if ( (bool) ListEegDoc && numspdoc && (bool) ListIsDoc && (bool) ListMriDoc ) {

    for ( /*int*/ i = 0; i < (int) ListEegDoc; i++ ) {
                                        // find the right Eeg view id
        for ( TBaseView* view = ListEegDoc[ i ]->GetViewList ( this ); view != 0; view = ListEegDoc[ i ]->NextView ( view, this ) )

            if ( dynamic_cast<TTracksView *> ( view ) )

                { LastEegViewId   = view->GetViewId(); break; }


        TInverseView*       viewis      = new TInverseView ( *ListEegDoc[ i ], 0, this );

        CartoolDocManager->PostEvent ( dnCreate, *viewis );

        viewis->WindowMinimize ();
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // can create inverse solution display from ris ?
if ( (bool) ListRisDoc && numspdoc && (bool) ListMriDoc ) {

    for ( /*int*/ i = 0; i < (int) ListRisDoc; i++ ) {
                                        // find the right Eeg view id
        for ( TBaseView* view = ListRisDoc[ i ]->GetViewList ( this ); view != 0; view = ListRisDoc[ i ]->NextView ( view, this ) ) {

            if ( dynamic_cast<TTracksView *> ( view ) )

                { LastEegViewId   = view->GetViewId(); break; }
            }

        TInverseView*       viewis      = new TInverseView ( *ListRisDoc[ i ], 0, this );

        CartoolDocManager->PostEvent ( dnCreate, *viewis );

        viewis->WindowMinimize ();
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // can we sync something ?
if ( (bool) ListEegDoc && numspdoc && (bool) ListIsDoc && (bool) ListXyzDoc && (bool) ListMriDoc ) {

    for ( /*int*/ i = 0; i < (int) ListEegDoc; i++ )    // scan pairs of views
    for ( TBaseView* view1 = ListEegDoc[ i ]->GetViewList ( this ); view1 != 0; view1 = ListEegDoc[ i ]->NextView ( view1, this ) )

        if ( dynamic_cast<TPotentialsView *> ( view1 ) )

            for ( TBaseView* view2 = ListEegDoc[ i ]->GetViewList ( this ); view2 != 0; view2 = ListEegDoc[ i ]->NextView (view2, this) )

                if ( dynamic_cast<TInverseView *> ( view2 )
                  && view1->LinkedViewId == view2->LinkedViewId )

                    view1->SetFriendView ( view2 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SyncUtility ( CM_SYNCALL );

Commit ();                              // write if new file

return true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // single exit point, if anything fails
AbortOpen:

SetDirty ( false );

return false;
}


//----------------------------------------------------------------------------
bool	TLinkManyDoc::CanClose ()
{
                                        // Don't ask if closing these special LM files(?)
if ( IsStringAmong ( GetTitle (), AllOpenedLmFilename " " EmptyLmFilename ) )

    SetDirty ( false );

return TBaseDoc::CanClose ();
}


//----------------------------------------------------------------------------
bool    TLinkManyDoc::Close ()
{
if ( ! HasDocs () )
    return  true;

                                        // application might already have forced-close some documents, it is not safe for testing anymore
if ( ! CartoolApplication->Closing ) {

    TBaseDoc*           doc;
    TTracksDoc*         doceeg;


    for ( int i = 0; i < (int) ListEegDoc; i++ ){
        doceeg = dynamic_cast <TTracksDoc*> ( ListEegDoc[ i ] );
        doceeg->RemoveLink ( this );

        if ( doceeg->CanClose ( true ) )    CartoolDocManager->CloseDoc ( doceeg, false );
        }


    for ( int i = 0; i < (int) ListRisDoc; i++ ) {
        doc = dynamic_cast <TBaseDoc *> ( ListRisDoc[ i ] );
        doc->RemoveLink ( this );

        if ( doc->CanClose ( true ) )       CartoolDocManager->CloseDoc ( doc, false );
        }

    for ( int i = 0; i < (int) ListIsDoc;  i++ ) {
        doc = dynamic_cast <TBaseDoc *> ( ListIsDoc[ i ] );
        doc->RemoveLink ( this );

        if ( doc->CanClose ( true ) )       CartoolDocManager->CloseDoc ( doc, false );
        }

    for ( int i = 0; i < (int) ListSpDoc;  i++ ) {
        doc = dynamic_cast <TBaseDoc *> ( ListSpDoc[ i ] );
        doc->RemoveLink ( this );

        if ( doc->CanClose ( true ) )       CartoolDocManager->CloseDoc ( doc, false );
        }

    for ( int i = 0; i < (int) ListXyzDoc; i++ ) {
        doc = dynamic_cast <TBaseDoc *> ( ListXyzDoc[ i ] );
        doc->RemoveLink ( this );

        if ( doc->CanClose ( true ) )       CartoolDocManager->CloseDoc ( doc, false );
        }

    for ( int i = 0; i < (int) ListMriDoc; i++ ) {
        doc = dynamic_cast <TBaseDoc *> ( ListMriDoc[ i ] );
        doc->RemoveLink ( this );

        if ( doc->CanClose ( true ) )       CartoolDocManager->CloseDoc ( doc, false );
        }

    for ( int i = 0; i < (int) ListRoisDoc; i++ ) {
        doc = dynamic_cast <TBaseDoc *> ( ListRoisDoc[ i ] );
        doc->RemoveLink ( this );

        if ( doc->CanClose ( true ) )       CartoolDocManager->CloseDoc ( doc, false );
        }

    } // not closing application


ListEegDoc  .Reset ( DontDeallocate );
ListRisDoc  .Reset ( DontDeallocate );
ListXyzDoc  .Reset ( DontDeallocate );
ListSpDoc   .Reset ( DontDeallocate );
ListIsDoc   .Reset ( DontDeallocate );
ListMriDoc  .Reset ( DontDeallocate );
ListRoisDoc .Reset ( DontDeallocate );


return  TFileDocument::Close ();
}


//----------------------------------------------------------------------------
bool    TLinkManyDoc::IsOpen ()
{
                                        // pretending to be open for proper buttons initialization - see TBaseView::EvSetFocus
return  true;
}


bool    TLinkManyDoc::HasDocs ()    const
{
return  (bool) ListEegDoc 
     || (bool) ListXyzDoc 
     || (bool) ListSpDoc
     || (bool) ListIsDoc  
     || (bool) ListRisDoc 
     || (bool) ListMriDoc 
     || (bool) ListRoisDoc;
}


int     TLinkManyDoc::GetNumDocs () const
{
return  (int) ListEegDoc 
      + (int) ListXyzDoc 
      + (int) ListSpDoc
      + (int) ListIsDoc  
      + (int) ListRisDoc 
      + (int) ListMriDoc 
      + (int) ListRoisDoc;
}


//----------------------------------------------------------------------------
void    TLinkManyDoc::GroupTileViews ( GroupTilingViewsFlags flags, UINT viewidabove )
{
                                        // Any tracks-like document?
if ( (int) ListEegDoc + (int) ListRisDoc == 0 )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TWindow*            lastfocus       = CartoolDocManager->GetCurrentView ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Reposition all minimized windows so they remain within the Client
TRect               mdicr           = CartoolMdiClient->GetClientRect ().Normalized ();


for ( TBaseDoc*  doc = CartoolDocManager->DocListNext ( 0 ); doc != 0; doc = CartoolDocManager->DocListNext ( doc ) )
for ( TBaseView* view = doc->GetViewList (); view != 0; view = doc->NextView ( view ) )

    if ( view->IsWindowMinimized () )

        view->RepositionMinimizedWindow ( mdicr.Height () );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Tiling is done from the outer (decorating) window
auto    GetViewRect     = [] ( const TBaseView* view )
{
TRect               r;
                                        // Biggest outter window
view->GetParentO ()->GetWindowRect  ( r );
CartoolMdiClient   ->ScreenToClient ( r );

return  r;
};


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Find the top of all open windows
int                 maxtop          = 0;
int                 maxleft         = Highest<int> ();


for ( TBaseDoc*  doc = CartoolDocManager->DocListNext ( 0 ); doc != 0; doc = CartoolDocManager->DocListNext ( doc ) )
for ( TBaseView* view = doc->GetViewList (); view != 0; view = doc->NextView ( view ) ) {

    TRect       r       = GetViewRect ( view );

    Mined ( maxtop,  r.Top () );
    Mined ( maxleft, r.Left() );
    }


if ( maxleft == Highest<int> () )   maxleft  = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Where to insert ?
TLinkManyDoc*       lastgod         = 0;
int                 maxright;
int                 xinsertion;
int                 myleft          = Highest<int> ();
int                 myright;


if ( IsFlag ( flags, GroupTilingViews_RightSide ) ) {

    maxright    = Lowest<int> ();

    for ( TBaseDoc*  doc = CartoolDocManager->DocListNext ( 0 ); doc != 0; doc = CartoolDocManager->DocListNext ( doc ) )
    for ( TBaseView* view = doc->GetViewList ( this, false ); view != 0; view = doc->NextView ( view, this, false ) )

        if ( ! view->IsWindowMinimized () ) {

            TRect       r       = GetViewRect ( view );

            Maxed ( maxright, r.Right() );
            }

    if ( maxright == Lowest<int> () )   xinsertion  = maxright = maxleft;
    else                                xinsertion  = maxright + GroupTilingSpaceBetweenGroups;
    }

else {
                                        // find my left side
    for ( TBaseDoc*  doc = CartoolDocManager->DocListNext ( 0 ); doc != 0; doc = CartoolDocManager->DocListNext ( doc ) )
    for ( TBaseView* view = doc->GetViewList ( this ); view != 0; view = doc->NextView ( view, this ) )

        if ( ! view->IsWindowMinimized () ) {

            TRect       r       = GetViewRect ( view );

            Mined ( myleft, r.Left() );
            }

    if ( myleft == Highest<int> () )    myleft  = maxleft;


    xinsertion  = myleft;   // if no move

    if ( IsFlag ( flags, GroupTilingViews_Insert ) ) {

        maxright    = Lowest<int> ();
        lastgod     = 0;

                                        // find insertion point
        for ( TBaseDoc*  doc = CartoolDocManager->DocListNext ( 0 ); doc != 0; doc = CartoolDocManager->DocListNext ( doc ) )
        for ( TBaseView* view = doc->GetViewList ( this, false ); view != 0; view = doc->NextView ( view, this, false ) )

            if ( ! view->IsWindowMinimized () ) {

                TRect       r       = GetViewRect ( view );

                if ( r.Right() >  myleft && r.Left()  <= myleft && r.Right() > maxright )   { maxright    = r.Right(); lastgod = view->GODoc; }
                if ( r.Right() <= myleft && r.Right() >  maxright                       )   { maxright    = r.Right(); lastgod = view->GODoc; }
                }

                                        // find the max right of the group that was found
        if ( lastgod != 0 ) {

            for ( TBaseDoc*  doc = CartoolDocManager->DocListNext ( 0 ); doc != 0; doc = CartoolDocManager->DocListNext ( doc ) )
            for ( TBaseView* view = doc->GetViewList ( lastgod ); view != 0; view = doc->NextView ( view, lastgod ) )

                if ( ! view->IsWindowMinimized () ) {

                    TRect       r       = GetViewRect ( view );

                    Maxed ( maxright, r.Right() );
                    }
            }

        if ( maxright == Lowest<int> () )   xinsertion = maxleft;
        else                                xinsertion = maxright + GroupTilingSpaceBetweenGroups;
        } // GroupTilingViews_Insert
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Now what to do?
if ( IsFlag ( flags, GroupTilingViews_SizeMask ) ) {

    int                 numrows         = (int) ListEegDoc + (int) ListRisDoc;

    int                 wh              = mdicr.Height() 
                                        - GetMinimizedWindowHeight ( GetViewList()->GetParentO () );    // keep some room at the bottom for the minimized windows to remain visible

    int                 ww              = mdicr.Width()
                                        - GetViewList ()->GetWindowWidth ();                            // remove width of our lm view


                                        // if relevant, resize my own views
    if ( IsFlag ( flags, GroupTilingViews_StandSize ) )

        for ( TBaseView* view = GetViewList (); view != 0; view = NextView ( view ) )

            view->SetStandSize ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // first move my own views
    int                 y               = maxtop;


    for ( TBaseView* view = GetViewList (); view != 0; view = NextView ( view ), y += 8 ) {

        view->WindowRestore ();
        view->WindowSetOrigin ( xinsertion, y );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // shift the insertion after my views
    int                 maxlmvieww      = 0;


    for ( TBaseView* view = GetViewList (); view != 0; view = NextView ( view ) ) {

        TRect       r       = GetViewRect ( view );

        if ( r.Right() > xinsertion )
            xinsertion  = r.Right();

        Maxed ( maxlmvieww, r.Width () );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // find how to spread the windows
                                        // first get max TSize for each eeg/ris
    int                 maxsumw         = 0;
    int                 maxsumh         = 0;


    for ( int i = 0; i < (int) ListEegDoc; i++ ) {  // process all Eeg

        int         sumw    = 0;
        int         sumh    = 0;

        for ( TBaseView* view = ListEegDoc[ i ]->GetViewList ( this ); view != 0; view = ListEegDoc[ i ]->NextView ( view, this ) ) {

            sumw    += view->StandSize.X() + view->WindowClientOffset.X();
            sumh    += view->StandSize.Y() + view->WindowClientOffset.Y();
            }

        Maxed ( maxsumw, sumw );
        Maxed ( maxsumh, sumh );
        } // EEG


    for ( int i = 0; i < (int) ListRisDoc; i++ ) {  // process all Ris

        int         sumw    = 0;
        int         sumh    = 0;

        for (TBaseView*  view = ListRisDoc[ i ]->GetViewList ( this ); view != 0; view = ListRisDoc[ i ]->NextView ( view, this ) ) {

            sumw    += view->StandSize.X() + view->WindowClientOffset.X();
            sumh    += view->StandSize.Y() + view->WindowClientOffset.Y();
            }

        Maxed ( maxsumw, sumw );
        Maxed ( maxsumh, sumh );
        } // RIS


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // second, fit it to the current window
    double              HorizontalFavor     = 1.5;
    TSize               sizemorehoriz ( HorizontalFavor * maxsumw, maxsumh );
    int                 wcurr;
    int                 byx;
    int                 byy;
    int                 ei                  = 0;

    TRect               winrect             = mdicr;
                                        // remove LM window width from the count
    winrect.SetWH ( 0, 0, winrect.Width () - maxlmvieww, winrect.Height () );

    GetViewList ()->FitItemsInWindow ( numrows, sizemorehoriz, byx, byy, &winrect );


    if ( IsFlag ( flags, GroupTilingViews_BestFitSize ) ) {

        for ( int i = 0; i < (int) ListEegDoc; i++ ) {  // process all Eeg

                        wcurr   = xinsertion + ( ei % byx ) * ww / byx;
            int         hbegin  = maxtop     + ( ei / byx ) * wh / byy;
            ei++;
            int         sumw    = 0;


            for ( TBaseView* view = ListEegDoc[ i ]->GetViewList ( this ); view != 0; view = ListEegDoc[ i ]->NextView ( view, this ) )

                sumw    += view->StandSize.X() + view->WindowClientOffset.X();


            for ( TBaseView* view = ListEegDoc[ i ]->GetViewList ( this ); view != 0; view = ListEegDoc[ i ]->NextView ( view, this ) ) {

                int     x       = wcurr;
                int     y       = hbegin;
                int     w       = ( view->StandSize.X() + view->WindowClientOffset.X() ) * ww / sumw / byx;
                int     h       = wh / byy;
                wcurr  += w;

                view->WindowRestore ();
                view->WindowSetPosition ( x, y, w, h );

                UpdateApplication;
                }
            } // EEG


        for ( int i = 0; i < (int) ListRisDoc; i++ ) {  // process all Ris

                        wcurr   = xinsertion + ( ei % byx ) * ww / byx;
            int         hbegin  = maxtop     + ( ei / byx ) * wh / byy;
            ei++;
            int         sumw    = 0;

            for ( TBaseView* view = ListRisDoc[ i ]->GetViewList ( this ); view != 0; view = ListRisDoc[ i ]->NextView ( view, this ) )

                sumw    += view->StandSize.X() + view->WindowClientOffset.X();


            for ( TBaseView* view = ListRisDoc[ i ]->GetViewList ( this ); view != 0; view = ListRisDoc[ i ]->NextView ( view, this ) ) {

                int     x       = wcurr;
                int     y       = hbegin;
                int     w       = ( view->StandSize.X() + view->WindowClientOffset.X() ) * ww / sumw / byx;
                int     h       = wh / byy;
                wcurr  += w;

                view->WindowRestore ();
                view->WindowSetPosition ( x, y, w, h );

                UpdateApplication;
                }
            } // RIS

        } // if GroupTilingViews_BestFitSize


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    else {  // move or standard size

        for ( int i = 0; i < (int) ListEegDoc; i++ ) {  // process all Eeg

            if ( ( ei % byx ) == 0 )
                wcurr   = xinsertion;
            int         hbegin  = maxtop + ( ei / byx ) * wh / byy;
            ei++;


            for ( TBaseView* view = ListEegDoc[ i ]->GetViewList ( this ); view != 0; view = ListEegDoc[ i ]->NextView ( view, this ) ) {

                if ( view->GetViewId () < viewidabove )

                    Maxed ( wcurr, view->GetParentO()->GetWindowRect().Right() );

                else {
                    if ( IsFlag ( flags, GroupTilingViews_StandSize ) )     view->SetStandSize ();

                    int     x       = wcurr;
                    int     y       = hbegin;

                    wcurr  += view->GetParentO()->GetWindowRect().Width();

                    view->WindowRestore ();
                    view->WindowSetOrigin ( x, y );

                    UpdateApplication;
                    }
                }
            } // EEG


        for ( int i = 0; i < (int) ListRisDoc; i++ ) {  // process all Ris

            if ( ( ei % byx ) == 0 )
                wcurr   = xinsertion;
            int         hbegin  = maxtop + ( ei / byx ) * wh / byy;
            ei++;


            for ( TBaseView* view = ListRisDoc[ i ]->GetViewList ( this ); view != 0; view = ListRisDoc[ i ]->NextView ( view, this ) ) {

                if ( view->GetViewId () < viewidabove )

                    wcurr   = max ( view->GetParentO()->GetWindowRect().Right(), wcurr );

                else {
                    if ( IsFlag ( flags, GroupTilingViews_StandSize ) )     view->SetStandSize ();

                    int     x       = wcurr;
                    int     y       = hbegin;

                    wcurr  += view->GetParentO()->GetWindowRect().Width();

                    view->WindowRestore ();
                    view->WindowSetOrigin ( x, y );

                    UpdateApplication;
                    }
                }
            } // RIS

        } // move or standard size

    } // if GroupTilingViews_Move


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Finally shift overlapping windows
if ( IsFlag ( flags, GroupTilingViews_Insert ) ) {
                                        // find my max right
    myright = Lowest<int> ();

    for ( TBaseDoc*  doc = CartoolDocManager->DocListNext ( 0 ); doc != 0; doc = CartoolDocManager->DocListNext ( doc ) )
    for ( TBaseView* view = doc->GetViewList ( this ); view != 0; view = doc->NextView ( view, this ) )

        if ( ! view->IsWindowMinimized () ) {

            TRect       r       = GetViewRect ( view );

            Maxed ( myright, r.Right() );
            }


    if ( myright == Lowest<int> () )    myright = myleft + 1;

                                        // find the min left of non-group overlapping windows
    int     minleft     = Highest<int> ();

    for ( TBaseDoc*  doc = CartoolDocManager->DocListNext ( 0 ); doc != 0; doc = CartoolDocManager->DocListNext ( doc ) )
    for ( TBaseView* view = doc->GetViewList ( lastgod, false ); view != 0; view = doc->NextView ( view, lastgod, false ) )

        if ( view->GODoc != this && ! view->IsWindowMinimized () ) {

            TRect       r       = GetViewRect ( view );

            if ( r.Left() >= myleft && r.Left() < minleft )     minleft = r.Left();
            }


    if ( minleft == Highest<int> () )   minleft = myleft;


    int     delta       = myright - minleft + GroupTilingSpaceBetweenGroups;

                                        // shift windows, except the overlapping group
    for ( TBaseDoc*  doc = CartoolDocManager->DocListNext ( 0 ); doc != 0; doc = CartoolDocManager->DocListNext ( doc ) )
    for ( TBaseView* view = doc->GetViewList ( lastgod, false ); view != 0; view = doc->NextView ( view, lastgod, false ) )

        if ( view->GODoc != this && ! view->IsWindowMinimized () ) {

            TRect       r       = GetViewRect ( view );

            if ( r.Left() >= myleft ) {
                view->WindowRestore ();
                view->WindowSetOrigin ( r.Left() + delta, r.Top() );
                UpdateApplication;
                }
            }
    } // GroupTilingViews_Insert


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( lastfocus )    lastfocus->GetParentO ()->SetFocus ();
}


//----------------------------------------------------------------------------
                                        // we need to retrieve these guys
void    TLinkManyDoc::GuessHeadBrainGreyMris    (   TVolumeDoc*&        head, 
                                                    TVolumeDoc*&        brain, 
                                                    TVolumeDoc*&        grey, 
                                                    int*                toheadi, 
                                                    int*                tobraini, 
                                                    int*                togreyi 
                                                )   const
{
head        = 0;
brain       = 0;
grey        = 0;
                                        // set the indexes, if needed
if ( toheadi  )     *toheadi    = -1;
if ( tobraini )     *tobraini   = -1;
if ( togreyi  )     *togreyi    = -1;

if ( ! (bool) ListMriDoc )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 headi           = -1;
int                 braini          = -1;
int                 greyi           = -1;
int                 nummris         = ListMriDoc.Num ();

                                        // loop through all MRIs
for ( int i = 0; i < nummris; i++ ) {

                                        // store first head
    if ( ! head  && ListMriDoc[ i ]->IsFullHead () ) {

        head    = (TVolumeDoc*) ListMriDoc[ i ];   // Oh my!
        headi   = i;

        continue;
        }
                                        // store first brain
    if ( ! brain && ListMriDoc[ i ]->IsSegmented () && ! ListMriDoc[ i ]->IsMask () ) {

        brain   = (TVolumeDoc*) ListMriDoc[ i ];
        braini  = i;

        continue;
        }
                                        // store first grey
    if ( ! grey  && ListMriDoc[ i ]->IsBinaryMask () ) {

        grey    = (TVolumeDoc*) ListMriDoc[ i ];
        greyi   = i;

        continue;
        }
                                        // are we done?
    if ( head && brain && grey )
        break;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need to fill the missing parts
if ( head  == 0 
  && brain == 0 
  && grey  == 0 ) {
                                        // here it seems we have no freaking idea...
                                        // assume this sequence: grey / brain / head
                                        // we could also test the bounding boxes
    greyi   =                       0;
    braini  = NoMore ( nummris - 1, 1 );
    headi   = NoMore ( nummris - 1, 2 );

    grey    = (TVolumeDoc*) ListMriDoc[ greyi  ];
    brain   = (TVolumeDoc*) ListMriDoc[ braini ];
    head    = (TVolumeDoc*) ListMriDoc[ headi  ];
    }
else {
                                        // here at least one volume is non-null, try to pick the best scenario
    if ( ! head  )  { head    = brain ? brain : grey;   headi   = braini >= 0 ? braini : greyi; }
    if ( ! brain )  { brain   = grey  ? grey  : head;   braini  = greyi  >= 0 ? greyi  : headi; }
    if ( ! grey  )  { grey    = brain ? brain : head;   greyi   = braini >= 0 ? braini : headi; }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // copy the indexes, if needed
if ( toheadi  )     *toheadi    = headi;
if ( tobraini )     *tobraini   = braini;
if ( togreyi  )     *togreyi    = greyi;


//DBGM ( head  ? (char *) head ->GetDocPath () : "None", "Head" );
//DBGM ( brain ? (char *) brain->GetDocPath () : "None", "Brain" );
//DBGM ( grey  ? (char *) grey ->GetDocPath () : "None", "Grey Mask" );
}


//----------------------------------------------------------------------------
bool    TLinkManyDoc::AddToGroup ( TBaseDoc *doc, bool refresh )
{
if ( doc == 0 || doc == this )
    return false;


 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                       // is it a link? then copy all its content
if ( dynamic_cast<TLinkManyDoc*> ( doc ) ) {

    TLinkManyDoc*       godoc           = dynamic_cast<TLinkManyDoc*> ( doc );
    bool                rvg             = true;
    bool                rv;

    for ( int i=0; i < godoc->GetNumEegDoc(); i++ ) {
        rv = AddToGroup ( godoc->GetEegDoc( i ) );
        rvg &= rv;
        }

    for ( int i=0; i < godoc->GetNumRoiDoc(); i++ ) {
        rv = AddToGroup ( godoc->GetRoisDoc( i ) );
        rvg &= rv;
        }

    for ( int i=0; i < godoc->GetNumXyzDoc(); i++ ) {
        rv = AddToGroup ( godoc->GetXyzDoc( i ) );
        rvg &= rv;
        }

    for ( int i=0; i < godoc->GetNumIsDoc(); i++ ) {
        rv = AddToGroup ( godoc->GetIsDoc( i ) );
        rvg &= rv;
        }

    for ( int i=0; i < godoc->GetNumRisDoc(); i++ ) {
        rv = AddToGroup ( godoc->GetRisDoc( i ) );
        rvg &= rv;
        }

    for ( int i=0; i < godoc->GetNumSpDoc(); i++ ) {
        rv = AddToGroup ( godoc->GetSpDoc( i ) );
        rvg &= rv;
        }

    for ( int i=0; i < godoc->GetNumMriDoc(); i++ ) {
        rv = AddToGroup ( godoc->GetMriDoc( i ) );
        rvg &= rv;
        }

    return rvg;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // not a correct doc?                                    extension seems to crash sometimes?!
if ( ! ( ( dynamic_cast<TTracksDoc        *> ( doc )  && ! dynamic_cast<TSegDoc *> ( doc ) ) // && ! IsExtensionAmong ( doc->GetDocPath (), FILEEXT_SEG" "FILEEXT_EEGEPSD" "FILEEXT_EEGEPSE ) )
        || dynamic_cast<TFreqDoc          *> ( doc )
        || dynamic_cast<TElectrodesDoc    *> ( doc )
        || dynamic_cast<TSolutionPointsDoc*> ( doc )
        || dynamic_cast<TInverseMatrixDoc *> ( doc )
        || dynamic_cast<TRisDoc           *> ( doc )
        || dynamic_cast<TRoisDoc          *> ( doc )
        || dynamic_cast<TVolumeDoc        *> ( doc ) ) ) {

//  ShowMessage ( (char *) doc->GetTitle (), "Can not add to LM!", ShowMessageWarning );
    return false;
    }

/*
if ( ! ( IsExtensionAmong ( doc->GetDocPath (), AllEegFreqFilesExt     )
      || IsExtensionAmong ( doc->GetDocPath (), AllCoordinatesFilesExt )
      || IsExtensionAmong ( doc->GetDocPath (), AllSolPointsFilesExt   )
      || IsExtensionAmong ( doc->GetDocPath (), AllInverseFilesExt     )
      || IsExtensionAmong ( doc->GetDocPath (), AllRisFilesExt         )
      || IsExtensionAmong ( doc->GetDocPath (), AllMriFilesExt         )
      || IsExtensionAmong ( doc->GetDocPath (), AllRoisFilesExt        ) ) )
    return  false;
*/


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // doc already in this group ?
if ( ListEegDoc .IsInside ( (TTracksDoc*)         doc ) )   return  false;
if ( ListXyzDoc .IsInside ( (TElectrodesDoc*)     doc ) )   return  false;
if ( ListSpDoc.  IsInside ( (TSolutionPointsDoc*) doc ) )   return  false;
if ( ListIsDoc.  IsInside ( (TInverseMatrixDoc*)  doc ) )   return  false;
if ( ListRisDoc .IsInside ( (TRisDoc*)            doc ) )   return  false;
if ( ListMriDoc .IsInside ( (TVolumeDoc*)         doc ) )   return  false;
if ( ListRoisDoc.IsInside ( (TRoisDoc*)           doc ) )   return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // recreate n lists of file names
TGoF                leeg;
TGoF                lris;
TGoF                lxyz;
TGoF                lsp;
TGoF                lis;
TGoF                lrois;
TGoF                lmri;


for ( int i = 0; i < (int) ListEegDoc;  i++ )   leeg .Add ( ListEegDoc [ i ]->GetDocPath () );
for ( int i = 0; i < (int) ListXyzDoc;  i++ )   lxyz .Add ( ListXyzDoc [ i ]->GetDocPath () );
for ( int i = 0; i < (int) ListSpDoc;   i++ )   lsp  .Add ( ListSpDoc  [ i ]->GetDocPath () );
for ( int i = 0; i < (int) ListIsDoc;   i++ )   lis  .Add ( ListIsDoc  [ i ]->GetDocPath () );
for ( int i = 0; i < (int) ListRisDoc;  i++ )   lris .Add ( ListRisDoc [ i ]->GetDocPath () );
for ( int i = 0; i < (int) ListMriDoc;  i++ )   lmri .Add ( ListMriDoc [ i ]->GetDocPath () );
for ( int i = 0; i < (int) ListRoisDoc; i++ )   lrois.Add ( ListRoisDoc[ i ]->GetDocPath () );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // add the new doc to the right list
TFileName           buff            = doc->GetDocPath ();


enum                LmFilesAdded 
                    {
                    addeeg,
                    addfreq,
                    addxyz,
                    addsp,
                    addis,
                    addris,
                    addroi,
                    addmri
                    };

LmFilesAdded        adding;

auto    addingtracks    = [ &adding ] () { return adding == addeeg || adding == addfreq || adding == addris; };
auto    addingeegfreq   = [ &adding ] () { return adding == addeeg || adding == addfreq; };


if      ( crtl::IsExtensionAmong ( buff, AllEegFilesExt ) ) {
    adding = addeeg;
    leeg.Add ( buff );
    }
else if ( crtl::IsExtensionAmong ( buff, AllFreqFilesExt ) ) {
    adding = addfreq;
    leeg.Add ( buff );
    }
else if ( crtl::IsExtensionAmong ( buff, AllCoordinatesFilesExt ) ) {
    adding = addxyz;
    lxyz.Add ( buff );
    }
else if ( crtl::IsExtensionAmong ( buff, AllSolPointsFilesExt ) ) {
    adding = addsp;
    lsp .Add ( buff );
    }
else if ( crtl::IsExtensionAmong ( buff, AllInverseFilesExt ) ) {
    adding = addis;
    lis .Add ( buff );
    }
else if ( crtl::IsExtensionAmong ( buff, AllRisFilesExt ) ) {
    adding = addris;
    lris.Add ( buff );
    }
else if ( crtl::IsExtensionAmong ( buff, AllRoisFilesExt ) ) {
    adding = addroi;
    lrois.Add ( buff );
    }
else if ( crtl::IsExtensionAmong ( buff, AllMriFilesExt ) ) {
    adding = addmri;
    lmri.Add ( buff );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // now, are all these files matching ?
if ( ! Compatibility ( leeg, lrois, lxyz, lsp, lis, lris ) ) {

//  goto AbortAddToGroup;
    SetDirty ( false );
    return false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // put to the right list
if      ( addingeegfreq () ) { 
                                        // re-sort the whole list after each insertion, better f.ex. for subjects or epochs
    leeg.Sort ();
                                        // !don't delete doc pointers!
    ListEegDoc.Reset ( DontDeallocate );
                                        // now can loop in the correct order
    for ( int gofi = 0; gofi < (int) leeg; gofi++ )
                                        // insert back       IsOpen returns the document* from the path, and we know it is open already
        ListEegDoc.Append ( (TTracksDoc*) CartoolDocManager->IsOpen ( leeg [ gofi ] ) );
    }

else if ( adding == addxyz   )   ListXyzDoc .Append ( (TElectrodesDoc    *) doc );
else if ( adding == addsp    )   ListSpDoc  .Append ( (TSolutionPointsDoc*) doc );
else if ( adding == addis    )   ListIsDoc  .Append ( (TInverseMatrixDoc *) doc );
else if ( adding == addris   )   ListRisDoc .Append ( (TRisDoc           *) doc );
else if ( adding == addroi   )   ListRoisDoc.Append ( (TRoisDoc          *) doc );
else if ( adding == addmri   )   ListMriDoc .Append ( (TVolumeDoc        *) doc );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // lock to this doc
doc->AddLink ( this );


bool                newview         = false;

                                        // get first view (more elaborated: scan all for the first non-owned view)
TBaseView*          view            = doc->GetViewList ();

                                        // adding a tracks view?
if ( addingtracks () ) {

    if ( view->GODoc != 0 ) {           // already owned by a group?

        if ( adding == addfreq )    view    = new TFrequenciesView ( *((TFreqDoc*)   doc), 0, this );
        else                        view    = new TTracksView      ( *((TTracksDoc*) doc), 0, this );

        CartoolDocManager->PostEvent ( dnCreate, *view );

        view->WindowMinimize ();
        }
    else                                // grab view for its own use
        view->GODoc = this;


    newview     = true;
    }

                                        // notify my views to update their pointers
doc->NotifyDocViews ( vnReloadData, EV_VN_RELOADDATA_DOCPOINTERS );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get the right tracks view ID with this GODoc
auto    GetLastEegViewId    = [ this ] ( TTracksDoc* doceeg )
{
if ( doceeg == 0 )
    return  (uint) 0;

for ( TBaseView* view = doceeg->GetViewList (); view != 0; view = doceeg->NextView ( view ) )

    if ( dynamic_cast<TTracksView*> ( view ) && view->GODoc == this )
        
        return  view->GetViewId (); 

return  (uint) 0;
};


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numspdoc        = GetNumSpDoc ();

                                        // if a SP, compute the interpolation with the MRI mask
                                        // the SP will test if it has already made the interpolation.
if ( adding == addsp && (bool) ListMriDoc ) {

    TVolumeDoc*         tohead;
    TVolumeDoc*         tobrain;
    TVolumeDoc*         togrey;

    GuessHeadBrainGreyMris ( tohead, tobrain, togrey );

#if defined(DisplayInverseInterpolation4NN)
    if ( ! ListSpDoc[ 0 ]->BuildInterpolation ( SPInterpolation4NN, togrey ) ) {
#else
    if ( ! ListSpDoc[ 0 ]->BuildInterpolation ( SPInterpolation1NN, togrey ) ) {
#endif
        doc->RemoveLink ( this );
        ListSpDoc.Remove ( (TSolutionPointsDoc*) doc, DontDeallocate );
        goto AbortAddToGroup;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // if a MRI, compute the interpolation
if ( adding == addmri && numspdoc ) {

    TVolumeDoc*         tohead;
    TVolumeDoc*         tobrain;
    TVolumeDoc*         togrey;

    GuessHeadBrainGreyMris ( tohead, tobrain, togrey );

#if defined(DisplayInverseInterpolation4NN)
    if ( ! ListSpDoc[ 0 ]->BuildInterpolation ( SPInterpolation4NN, togrey ) ) {
#else
    if ( ! ListSpDoc[ 0 ]->BuildInterpolation ( SPInterpolation1NN, togrey ) ) {
#endif
        doc->RemoveLink ( this );
        ListSpDoc.Remove ( (TSolutionPointsDoc*) doc, DontDeallocate );
        goto AbortAddToGroup;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // create additional views if appropriate?
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // can create a potentials view?
if ( addingeegfreq () && (bool) ListXyzDoc ) {

    TTracksDoc*         doceeg      = dynamic_cast <TTracksDoc*> ( doc );

    LastEegViewId   = GetLastEegViewId ( doceeg );

    TPotentialsView*    viewpm      = new TPotentialsView ( *doceeg, 0, this );

    CartoolDocManager->PostEvent ( dnCreate, *viewpm );

    viewpm->WindowMinimize ();

    newview         = true;
    }


if ( adding == addxyz && (int) ListXyzDoc == 1 && (bool) ListEegDoc ) {

    for ( int i = 0; i < (int) ListEegDoc; i++ ) {

        LastEegViewId   = GetLastEegViewId ( ListEegDoc[ i ] );
        
        TPotentialsView*    viewpm      = new TPotentialsView ( *ListEegDoc[ i ], 0, this );

        CartoolDocManager->PostEvent ( dnCreate, *viewpm );

        viewpm->WindowMinimize ();
        }

    newview         = true;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // can create inverse view?
if ( addingeegfreq () && numspdoc && (bool) ListIsDoc && (bool) ListMriDoc 
  || adding == addris && numspdoc && (bool) ListMriDoc ) {

    TTracksDoc*         doctracks   = dynamic_cast <TTracksDoc*> ( doc );

    LastEegViewId   = GetLastEegViewId ( doctracks );

    TInverseView*       viewis      = new TInverseView ( *doctracks, 0, this );

    CartoolDocManager->PostEvent ( dnCreate, *viewis );

    viewis->WindowMinimize ();

    newview         = true;
    }


if ( adding == addsp  &&       numspdoc   == 1 && (bool) ListEegDoc && (bool) ListIsDoc && (bool) ListMriDoc
  || adding == addis  && (int) ListIsDoc  == 1 && (bool) ListEegDoc && numspdoc && (bool) ListMriDoc
  || adding == addmri && (int) ListMriDoc == 1 && (bool) ListEegDoc && numspdoc && (bool) ListIsDoc ) {

    for ( int i = 0; i < (int) ListEegDoc; i++ ) {

        LastEegViewId   = GetLastEegViewId ( ListEegDoc[ i ] );

        TInverseView*       viewis      = new TInverseView ( *ListEegDoc[ i ], 0, this );

        CartoolDocManager->PostEvent ( dnCreate, *viewis );

        viewis->WindowMinimize ();
        }

    newview         = true;
    }


if ( adding == addsp  &&       numspdoc   == 1 && (bool) ListRisDoc && (bool) ListMriDoc
  || adding == addmri && (int) ListMriDoc == 1 && (bool) ListRisDoc && numspdoc ) {

    for ( int i = 0; i < (int) ListRisDoc; i++ ) {

        LastEegViewId   = GetLastEegViewId ( ListRisDoc[ i ] );

        TInverseView*       viewis      = new TInverseView ( *ListRisDoc[ i ], 0, this );

        CartoolDocManager->PostEvent ( dnCreate, *viewis );

        viewis->WindowMinimize ();
        }

    newview         = true;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // can we sync some views?
if ( (bool) ListEegDoc && numspdoc && (bool) ListIsDoc && (bool) ListXyzDoc && (bool) ListMriDoc ) {

    for ( int i = 0; i < (int) ListEegDoc; i++ )    // scan pairs of views
    for ( TBaseView* view1 = ListEegDoc[ i ]->GetViewList (); view1 != 0; view1 = ListEegDoc[ i ]->NextView ( view1 ) )

        if ( dynamic_cast<TPotentialsView*> ( view1 )
            && view1->GODoc == this )

            for ( TBaseView* view2 = ListEegDoc[ i ]->GetViewList (); view2 != 0; view2 = ListEegDoc[ i ]->NextView (view2) )

                if ( dynamic_cast<TInverseView*> ( view2 )
                    && view2->GODoc == this
                    && view1->LinkedViewId == view2->LinkedViewId )

                    view1->SetFriendView ( view2 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // can we update the file path?
if ( UnspecifiedDocPath ) {
    TFileName           match;
    TGoF                biglist;

    ClearString ( match );

                                        // prioritize according to content
    if ( (bool) leeg || (bool) lris ) {
        biglist.Add ( leeg );
        biglist.Add ( lris );
        }
    else if ( (bool) lmri || (bool) lis || (bool) lsp ) {
        biglist.Add ( lmri );
        biglist.Add ( lis  );
        biglist.Add ( lsp  );
        }
    else {
        biglist.Add ( lxyz );
        biglist.Add ( lrois );
        }


    biglist.GetCommonString ( match, true );

    AddExtension ( match, FILEEXT_LM );

    if ( IsAbsoluteFilename ( match ) )
        SetDocPath ( match );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // by security, also refresh all our windows
if ( refresh ) {

    RefreshWindows ( newview );

                                        // These type of windows can be minimized once the joined a lm (seems to crash sometimes?)
    if ( adding == addxyz 
      || adding == addsp 
      || adding == addis 
      || adding == addroi 
      || adding == addmri )

        view->WindowMinimize ();
    }

SetDirty ( true );

return  true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // common exit, if anything fails
AbortAddToGroup:

SetDirty ( false );

return  false;
}


void    TLinkManyDoc::RefreshWindows ( bool retile )
{
for ( TBaseView* view = GetViewList (); view != 0; view = NextView ( view ) ) {

    TLinkManyView*  lmview  = dynamic_cast<TLinkManyView*> ( view );

    if ( lmview ) {

        lmview->StandSize   = lmview->GetBestSize ();

        view->EvSize ( 0, view->StandSize );
        }
    }

                                        // do a nice tiling
if ( retile )
    GroupTileViews ( CombineFlags ( GroupTilingViews_BestFitSize, GroupTilingViews_Insert ) );
}


//----------------------------------------------------------------------------
bool    TLinkManyDoc::Compatibility     (   const   TGoF&   leeg,
                                            const   TGoF&   lrois,
                                            const   TGoF&   lxyz,
                                            const   TGoF&   lsp,
                                            const   TGoF&   lis,
                                            const   TGoF&   lris 
                                        )   const
{
TracksCompatibleClass       CompatEegs;
TracksCompatibleClass       CompatRis;
ElectrodesCompatibleClass   CompatElectrodes;
InverseCompatibleClass      CompatSp;
InverseCompatibleClass      CompatInverse;
RoisCompatibleClass         CompatRois;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check first the last group's own coherence, as tracks files
if ( leeg.IsNotEmpty () ) {

    leeg.AllTracksAreCompatible ( CompatEegs );


    if      ( CompatEegs.NumTracks == CompatibilityNotConsistent ) {
        ShowMessage ( "Tracks files don't seem to have the same number of electrodes!\nCheck again your input files...", LinkingFilesTitle, ShowMessageWarning );
        return  false;
        }
    else if ( CompatEegs.NumTracks == CompatibilityIrrelevant ) {
        ShowMessage ( "Tracks files don't seem to have any electrodes at all!\nCheck again your input files...", LinkingFilesTitle, ShowMessageWarning );
        return  false;
        }


    if      ( CompatEegs.NumTF == CompatibilityIrrelevant ) {
        ShowMessage ( "Tracks files don't seem to have any samples or time at all!\nCheck again your input files...", LinkingFilesTitle, ShowMessageWarning );
        return  false;
        }


//                                      // frequencies are optional, just check that if they exist that they should be consistent
//  if ( CompatEegs.NumFreqs == CompatibilityNotConsistent ) {
//                                      // we need all files from a group to have the same length!
//      ShowMessage ( "Frequency Files don't seem to have the same number of frequencies!\nCheck again your input files...", LinkingFilesTitle, ShowMessageWarning );
//      return;
//      }


/*  FreqsCompatibleClass    fc;

                                        // Check the new group's own coherence, as frequency files
    if ( CompatEegs.NumFreqs >= CompatibilityConsistent ) {

        leeg.AllFreqsAreCompatible ( fc );

                                            // do we actually care?
    //  if      ( fc.OriginalSamplingFrequency == CompatibilityNotConsistent ) {
    //      ShowMessage ( "Files don't seem to have the same original sampling frequencies!", LinkingFilesTitle, ShowMessageWarning );
    //      return;
    //      }


        if      ( fc.FreqType == CompatibilityNotConsistent ) {
            ShowMessage ( "Files don't seem to be of the same frequency types!", LinkingFilesTitle, ShowMessageWarning );
            return;
            }

        if ( ! ( IsFreqTypePhase    ( (FrequencyAnalysisType) fc.FreqType ) 
              || IsFreqTypePositive ( (FrequencyAnalysisType) fc.FreqType ) ) ) {
            ShowMessage ( "Files do seem to be of Phase or Intensity types!", LinkingFilesTitle, ShowMessageWarning );
            return;
            }
        }
*/

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Very similar to EEG tracks
if ( lris.IsNotEmpty () ) {

    lris.AllTracksAreCompatible ( CompatRis );


    if      ( CompatRis.NumSolPoints == CompatibilityNotConsistent ) {
        ShowMessage ( "RIS files don't seem to have the same number of Solution Points!", LinkingFilesTitle, ShowMessageWarning );
        return  false;
        }
    else if ( CompatRis.NumSolPoints == CompatibilityIrrelevant ) {
        ShowMessage ( "RIS files seem to be missing Solution Points!", LinkingFilesTitle, ShowMessageWarning );
        return  false;
        }


    if      ( CompatRis.NumTF == CompatibilityIrrelevant ) {
        ShowMessage ( "RIS files don't seem to have any samples or time at all!\nCheck again your input files...", LinkingFilesTitle, ShowMessageWarning );
        return  false;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( lxyz.IsNotEmpty () ) {

    lxyz.AllElectrodesAreCompatible ( CompatElectrodes );


    if      ( CompatElectrodes.NumElectrodes == CompatibilityNotConsistent ) {
        ShowMessage ( "Electrodes Coordinates don't seem to have the same number of electrodes!", LinkingFilesTitle, ShowMessageWarning );
        return  false;
        }
    else if ( CompatElectrodes.NumElectrodes == CompatibilityIrrelevant ) {
        ShowMessage ( "Electrodes Coordinates don't seem to have any electrodes at all!", LinkingFilesTitle, ShowMessageWarning );
        return  false;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( lsp.IsNotEmpty () ) {

    lsp.AllInverseAreCompatible ( CompatSp );


    if      ( CompatSp.NumSolPoints == CompatibilityNotConsistent ) {
        ShowMessage ( "Solution Points don't seem to have the same number of electrodes!", LinkingFilesTitle, ShowMessageWarning );
        return  false;
        }
    else if ( CompatSp.NumSolPoints == CompatibilityIrrelevant ) {
        ShowMessage ( "Solution Points don't seem to have any coordinates at all!", LinkingFilesTitle, ShowMessageWarning );
        return  false;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // testing a single inverse is a bit silly, but it stores the data about the inverse - it can also straightforward upgrade to a group of inverse
if ( lis.IsNotEmpty () ) {

    lis.AllInverseAreCompatible ( CompatInverse );


    if      ( CompatInverse.NumElectrodes == CompatibilityNotConsistent ) {
        ShowMessage ( "Inverse matrices don't seem to have the same number of electrodes!\nCheck again your input files...", LinkingFilesTitle, ShowMessageWarning );
        return  false;
        }
    else if ( CompatInverse.NumElectrodes == CompatibilityIrrelevant ) {
        ShowMessage ( "Inverse matrices don't seem to have any electrodes at all!\nCheck again your input files...", LinkingFilesTitle, ShowMessageWarning );
        return  false;
        }


    if      ( CompatInverse.NumSolPoints == CompatibilityNotConsistent ) {
        ShowMessage ( "Inverse matrices don't seem to have the same number of Solution Points!\nCheck again your input files...", LinkingFilesTitle, ShowMessageWarning );
        return  false;
        }
    else if ( CompatInverse.NumSolPoints == CompatibilityIrrelevant ) {
        ShowMessage ( "Inverse matrices don't seem to have any Solution Points at all!\nCheck again your input files...", LinkingFilesTitle, ShowMessageWarning );
        return  false;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // There is currently no handy function for that, just try to open the ROIs and see
if ( lrois.IsNotEmpty () ) {

    lrois.AllRoisAreCompatible ( CompatRois );


    if      ( CompatRois.NumDimensions == CompatibilityNotConsistent ) {
        ShowMessage ( "ROIs don't seem to have the same number of Solution Points!", LinkingFilesTitle, ShowMessageWarning );
        return  false;
        }
    else if ( CompatRois.NumDimensions == CompatibilityIrrelevant ) {
        ShowMessage ( "ROIs don't seem to have the right dimension at all!", LinkingFilesTitle, ShowMessageWarning );
        return  false;
        }


    if      ( CompatRois.NumRois == CompatibilityNotConsistent ) {
        ShowMessage ( "ROIs don't seem to have the same number regions!", LinkingFilesTitle, ShowMessageWarning );
        return  false;
        }
    else if ( CompatRois.NumRois == CompatibilityIrrelevant ) {
        ShowMessage ( "ROIs don't seem to have the right dimension at all!", LinkingFilesTitle, ShowMessageWarning );
        return  false;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Testing EEG vs XYZ / IS
if ( leeg.IsNotEmpty () && lxyz.IsNotEmpty ()
  && CompatEegs.NumTracks != CompatElectrodes.NumElectrodes ) {

//    char                buff[ 256 ];
//    sprintf ( buff, "Not the same amount of Electrodes:\n\n  - EEG files \t= %0d\n  - Electrodes file \t= %0d\n\nPlease check again your files!", CompatEegs.NumTracks, xyznumel );

    ShowMessage ( "EEG files don't seem to have the same number of electrodes with the Electrodes Coordinates!\nCheck again your input files...", LinkingFilesTitle, ShowMessageWarning );

    return  false;
    }


if ( leeg.IsNotEmpty () && lis.IsNotEmpty ()
  && CompatEegs.NumTracks != CompatInverse.NumElectrodes ) {

    ShowMessage ( "EEG files don't seem to have the same number of electrodes with the Inverse Matrix!", LinkingFilesTitle, ShowMessageWarning );

    return  false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Testing XYZ vs IS
if ( lxyz.IsNotEmpty () && lis.IsNotEmpty ()
  && CompatElectrodes.NumElectrodes != CompatInverse.NumElectrodes ) {

    ShowMessage ( "Electrodes Coordinates file doesn't seem to have the same number of electrodes as the Inverse Matrix!", LinkingFilesTitle, ShowMessageWarning );
    return  false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Testing SP vs IS / ROIs / RIS
if ( lsp.IsNotEmpty () && lis.IsNotEmpty ()
  && CompatSp.NumSolPoints != CompatInverse.NumSolPoints ) {

    ShowMessage ( "Solution Points file doesn't seem to be compatible with the Inverse Matrix!", LinkingFilesTitle, ShowMessageWarning );
    return  false;
    }

                                        // also checking vs numelectrodes?
if ( lsp.IsNotEmpty () && lrois.IsNotEmpty ()
//  && CompatRois.NumRois != CompatSp.NumSolPoints ) {
  && ( CompatRois.NumDimensions != CompatSp.NumSolPoints && CompatRois.NumRois != CompatSp.NumSolPoints ) ) {

//    DBGV4 ( CompatRois.NumDimensions, CompatSp.NumSolPoints, CompatRois.NumRois, CompatSp.NumSolPoints, "CompatRois.NumDimensions, CompatSp.NumSolPoints, CompatRois.NumRois, CompatSp.NumSolPoints" );

    ShowMessage ( "Number of ROIs doesn't match the number of Solution Points!\nMake sure your Solution Points file is the one for ROIs...", LinkingFilesTitle, ShowMessageWarning );
    return  false;
    }

                                        // also checking vs numelectrodes?
if ( lsp.IsNotEmpty () && lris.IsNotEmpty ()
  && CompatRis.NumSolPoints != CompatSp.NumSolPoints ) {

    ShowMessage ( "RIS Files don't seem to match with the Solution Points!", LinkingFilesTitle, ShowMessageWarning );
    return  false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Did it survive until here?
return  true;
}


//----------------------------------------------------------------------------
void    TLinkManyDoc::SyncUtility ( owlwparam w )
{
TBaseView          *view;

TListIterator<TTracksDoc>   iteratoreeg;
TListIterator<TRisDoc>      iteratorris;


if      ( w == CM_DESYNCALL ) {

    foreachin ( ListEegDoc, iteratoreeg ) {
        for ( view = iteratoreeg ()->GetViewList ( this ); view != 0; view = iteratoreeg ()->NextView ( view, this ) )
            view->CancelFriendship ();
        }

    foreachin ( ListRisDoc, iteratorris ) {
        for ( view = iteratorris ()->GetViewList ( this ); view != 0; view = iteratorris ()->NextView ( view, this ) )
            view->CancelFriendship ();
        }
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else if ( w == CM_DESYNCBETWEENEEG ) {

    TListAtom<TTracksDoc>*  eegatom2;
    TListAtom<TRisDoc>*     risatom2;
    TTracksDoc*             doceeg2;
    TRisDoc*                docris2;
    TBaseView*              view2;
    TBaseView*              view3;
    unsigned                oldfrid;
    unsigned                newfrid;

                                        // scan all pairs of different eeg views
    foreachin ( ListEegDoc, iteratoreeg ) {

        for ( view = iteratoreeg ()->GetViewList ( this ); view != 0; view = iteratoreeg ()->NextView ( view, this ) ) {

                                        // eeg-to-eeg
            for ( eegatom2 = ((TListAtom<TTracksDoc> *) iteratoreeg)->Next; eegatom2 != 0; eegatom2 = eegatom2->Next ) {

                doceeg2 = eegatom2->ToData;

                for ( view2 = doceeg2->GetViewList ( this ); view2 != 0; view2 = doceeg2->NextView (view2, this) )
                                        // this pair shouldn't remains friends...
                    if ( view->IsFriendView ( view2 ) ) {

                        oldfrid = view->FriendshipId;

                                        // replace in eeg1 all old Ids
                        newfrid = view->GetViewId();
                        for ( view3 = iteratoreeg ()->GetViewList ( this ); view3 != 0; view3 = iteratoreeg ()->NextView (view3, this) )
                            if ( view3->IsFriendship ( oldfrid ) )
                                view3->SetFriendship ( newfrid );

                                        // replace in eeg2 all old Ids
                        newfrid = view2->GetViewId();
                        for ( view3 = doceeg2->GetViewList ( this ); view3 != 0; view3 = doceeg2->NextView (view3, this) )
                            if ( view3->IsFriendship ( oldfrid ) )
                                view3->SetFriendship ( newfrid );

                        } // if same Id
                } // eegatom2

                                        // eeg-to-ris
            foreachin ( ListRisDoc, iteratorris ) {
                for ( view2 = iteratorris ()->GetViewList ( this ); view2 != 0; view2 = iteratorris ()->NextView (view2, this) )
                                        // this pair shouldn't remains friends...
                    if ( view->IsFriendView ( view2 ) ) {

                        oldfrid = view->FriendshipId;

                                        // replace in eeg1 all old Ids
                        newfrid = view->GetViewId();
                        for ( view3 = iteratoreeg ()->GetViewList ( this ); view3 != 0; view3 = iteratoreeg ()->NextView (view3, this) )
                            if ( view3->IsFriendship ( oldfrid ) )
                                view3->SetFriendship ( newfrid );

                                        // replace in eeg2 all old Ids
                        newfrid = view2->GetViewId();
                        for ( view3 = iteratorris ()->GetViewList ( this ); view3 != 0; view3 = iteratorris ()->NextView (view3, this) )
                            if ( view3->IsFriendship ( oldfrid ) )
                                view3->SetFriendship ( newfrid );

                        } // if same Id
                } // risatom2
            } // view
        } // ListEegDoc

                                        // scan all pairs of different ris-to-ris views
    foreachin ( ListRisDoc, iteratorris ) {

        for ( view = iteratorris ()->GetViewList ( this ); view != 0; view = iteratorris ()->NextView ( view, this ) )

            for ( risatom2 = ((TListAtom<TRisDoc> *) iteratorris)->Next; risatom2 != 0; risatom2 = risatom2->Next ) {

                docris2 = risatom2->ToData;

                for ( view2 = docris2->GetViewList ( this ); view2 != 0; view2 = docris2->NextView (view2, this) )
                                        // this pair shouldn't remains friends...
                    if ( view->IsFriendView ( view2 ) ) {

                        oldfrid = view->FriendshipId;

                                        // replace in eeg1 all old Ids
                        newfrid = view->GetViewId();
                        for ( view3 = iteratorris ()->GetViewList ( this ); view3 != 0; view3 = iteratorris ()->NextView (view3, this) )
                            if ( view3->IsFriendship ( oldfrid ) )
                                view3->SetFriendship ( newfrid );

                                        // replace in eeg2 all old Ids
                        newfrid = view2->GetViewId();
                        for ( view3 = docris2->GetViewList ( this ); view3 != 0; view3 = docris2->NextView (view3, this) )
                            if ( view3->IsFriendship ( oldfrid ) )
                                view3->SetFriendship ( newfrid );

                        } // if same Id
                } // risatom2
        } // ListRisDoc
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else if ( w == CM_SYNCALL ) {
    TBaseView      *firstview = 0;

    foreachin ( ListEegDoc, iteratoreeg ) {
        for ( view = iteratoreeg ()->GetViewList ( this ); view != 0; view = iteratoreeg ()->NextView ( view, this ) ) {
//          if ( dynamic_cast<TTracksView *> ( view ) )
//              continue;

            if ( firstview == 0 )
                firstview = view;

            if ( view->GODoc == firstview->GODoc ) {
                view->SetFriendView ( firstview );
                view->GetParentO()->SetFocus();
                }
            }
        }

    foreachin ( ListRisDoc, iteratorris ) {
        for ( view = iteratorris ()->GetViewList ( this ); view != 0; view = iteratorris ()->NextView ( view, this ) ) {

            if ( firstview == 0 )
                firstview = view;

            if ( view->GODoc == firstview->GODoc ) {
                view->SetFriendView ( firstview );
                view->GetParentO()->SetFocus();
                }
            }
        }
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else if ( w == CM_SYNCBETWEENEEG ) {
    TBaseView      *firstview;

    foreachin ( ListEegDoc, iteratoreeg ) {
        firstview = 0;

        for ( view = iteratoreeg ()->GetViewList ( this ); view != 0; view = iteratoreeg ()->NextView ( view, this ) ) {
            if ( firstview == 0 )
                firstview = view;

            if ( view->GODoc == firstview->GODoc ) {
                view->SetFriendView ( firstview );
                view->GetParentO()->SetFocus();
                }
            }
        }

    foreachin ( ListRisDoc, iteratorris ) {
        firstview = 0;

        for ( view = iteratorris ()->GetViewList ( this ); view != 0; view = iteratorris ()->NextView ( view, this ) ) {
            if ( firstview == 0 )
                firstview = view;

            if ( view->GODoc == firstview->GODoc ) {
                view->SetFriendView ( firstview );
                view->GetParentO()->SetFocus();
                }
            }
        }
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// refresh new TF cursor
}


//----------------------------------------------------------------------------
                                        // !No messages sent to MRI / ISMatrix / ROIs - Update if needed!
bool    TLinkManyDoc::NotifyDocViews  ( int event, TParam2 item, TView *vexclude, TDocument *dexclude )
{
bool                result;
bool                rr;

//DBGM ( "Notifying LM views", "NotifyDocViews" );
                                        // 1) send message to its own views, and pointed by documents
result  = TBaseDoc::NotifyDocViews ( event, item, vexclude );


                                        // 2) then send to the list of docs - prevent calling itself back
TListIterator<TElectrodesDoc>       iteratorel;
TListIterator<TSolutionPointsDoc>   iteratorsp;
TListIterator<TTracksDoc>           iteratoreeg;
TListIterator<TRisDoc>              iteratorris;
//TListIterator<TInverseMatrixDoc>  iteratoris;


//DBGM ( "Notifying EEG views", "NotifyDocViews" );

foreachin ( ListEegDoc, iteratoreeg )
    if ( iteratoreeg () != dexclude ) {
        rr      = iteratoreeg ()->NotifyViews ( event, item, vexclude );  // send to my views only
        result  = result || rr;
        }


//DBGM ( "Notifying RIS views", "NotifyDocViews" );

foreachin ( ListRisDoc, iteratorris )
    if ( iteratorris () != dexclude ) {
        rr      = iteratorris ()->NotifyViews ( event, item, vexclude );
        result  = result || rr;
        }


//DBGM ( "Notifying XYZ views", "NotifyDocViews" );

foreachin ( ListXyzDoc, iteratorel )
    if ( iteratorel () != dexclude ) {
        rr      = iteratorel ()->NotifyViews ( event, item, vexclude );
        result  = result || rr;
        }


//DBGM ( "Notifying SPI views", "NotifyDocViews" );

foreachin ( ListSpDoc, iteratorsp )
    if ( iteratorsp () != dexclude ) {
        rr      = iteratorsp ()->NotifyViews ( event, item, vexclude );
        result  = result || rr;
        }


//foreachin ( ListIsDoc, iteratoris )
//    if ( iteratoris () != dexclude ) {
//        rr      = iteratoris ()->NotifyViews ( event, item, vexclude );
//        result  = result || rr;
//        }


return result;
}


//----------------------------------------------------------------------------
void    TLinkManyDoc::Segment ()
{
if ( ! (bool) ListEegDoc && ! (bool) ListRisDoc )
    return;


TMicroStatesSegFilesDialog  segdlg ( CartoolMdiClient, IDD_SEGMENT1 );

                                        // clear-up all current groups
segdlg.GoGoF.Reset ();

                                        // then add my files - Eeg
if ( (bool) ListEegDoc ) {

    segdlg.GoGoF.Add ( new TGoF );

    TListIterator<TTracksDoc>   iteratoreeg;

    foreachin ( ListEegDoc, iteratoreeg )
        segdlg.GoGoF.GetLast ().Add ( iteratoreeg ()->GetDocPath (), MaxPathShort );
    }

                                        // then add my files - Ris
if ( (bool) ListRisDoc ) {

    segdlg.GoGoF.Add ( new TGoF );

    TListIterator<TRisDoc>          iteratorris;

    foreachin ( ListRisDoc, iteratorris )
        segdlg.GoGoF.GetLast ().Add ( iteratorris ()->GetDocPath (), MaxPathShort );
    }


segdlg.Execute();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TDynamicLinkManyDoc::TDynamicLinkManyDoc ( TTracksDoc* doceeg, TElectrodesDoc* docxyz, TDocument* parent )
      : TLinkManyDoc ( parent )
{
ListXyzDoc.Append ( docxyz );
ListEegDoc.Append ( doceeg );

                                        // lock the xyz
for ( int i = 0; i < (int) ListXyzDoc; i++ )
    ListXyzDoc[ i ]->AddLink ( this );

                                        // lock the eeg
for ( int i = 0; i < (int) ListEegDoc; i++ )
    ListEegDoc[ i ]->AddLink ( this );


char                title[ KiloByte ];
StringCopy  ( title, "%s + %s", doceeg->GetTitle(), " + ", docxyz->GetTitle() );
SetTitle    ( title );
}


        TDynamicLinkManyDoc::~TDynamicLinkManyDoc ()
{
Close ();
}


bool    TDynamicLinkManyDoc::Close ()
{
if ( ! HasDocs () )
    return  true;

                                        // just removing references, NOT actually closing these docs
for ( int i = 0; i < (int) ListEegDoc; i++ )
    ListEegDoc[ i ]->RemoveLink ( this );


for ( int i = 0; i < (int) ListXyzDoc; i++ )
    ListXyzDoc[ i ]->RemoveLink ( this );


ListEegDoc.Reset ( DontDeallocate );
ListXyzDoc.Reset ( DontDeallocate );

return  true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
