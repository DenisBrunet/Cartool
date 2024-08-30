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

#pragma once

#include    <owl/docview.h>             // vnCustomBase

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace owl {
                                        // Adding response table macro and event handler signature for WM_DPICHANGED message
                                        // Note: WM_DPICHANGED message returns an X and Y dpi, but as they are identical, we return only one in our handler

                                        // Define macro à la "windowsx.h"
#define HANDLE_WM_DPICHANGED(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (int)LOWORD(wParam), ::owl::TRect ( *reinterpret_cast<LPRECT> (p2) )), 0L)

                                        // OwlNext signal handler definition
template <> struct TDispatch<WM_DPICHANGED>
{

#if OWL_EV_SIGNATURE_CHECK

template <class T> struct THandler { typedef void (T::*type) ( int dpi, const TRect& rect ); };

#endif

template <class T, void (T::*M) ( int, const TRect& )>
static TResult Decode ( void* i, TParam1 p1, TParam2 p2 ) {

    struct  TForwarder {
        T*      i_;
        void    operator () ( HWND, int dpi, const TRect& rect ) { (i_->*M) ( dpi, rect ); }
        }   forwarder   = { static_cast<T*> ( i ) };


    InUse(p1); InUse(p2);
                                        // More explicit conversions done here instead of using macro
    //int             dpi         = LOWORD ( p1 );
    //::owl::TRect    rect        = *reinterpret_cast<LPRECT> ( p2 );
    //forwarder ( 0, dpi, rect );
    //return  0L;
                                        // Windows macro style
    return HANDLE_WM_DPICHANGED ( 0, p1, p2, forwarder );
    }
};

                                        // Macro for response tables
#define EV_WM_DPICHANGED OWL_EV_(WM_DPICHANGED, EvDpiChanged)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace crtl {
                                        // Adding some custom signals Id's and types of handlers for the Doc <-> View communication

using   OwlNotifyIdType = const owl::uint;          // just for the sake of clarity, this is the type used by owl::vnCustomBase

enum    OwlCustomEvents     : OwlNotifyIdType
        {
        vnReloadData        = owl::vnCustomBase,    // Generating automatically the next custom signal value
        vnNewTFCursor,
        vnViewSync,
        vnNewSelection,
        vnNewHighlighted,
        vnNewBadSelection,
        vnNewAuxSelection,
        vnViewDestroyed,
        vnViewUpdated,
        };


//----------------------------------------------------------------------------
                                        // NOTIFY_SIG is a handy define to declare new events (from docview.h)
#if ! defined(OWL5_COMPAT) && ! defined(NOTIFY_SIG)

// Define a DocView notification signature
//   'ID'  is the vnXxxxx name of the notification
//   'ARG' is the type of the argument passed to the handler

#define NOTIFY_SIG(ID,ARG)      template <class T>  inline bool ( T::*ID##_Sig ( bool (T::*pmf) (ARG) ) ) (ARG)  { return pmf; }

#endif


//----------------------------------------------------------------------------
                                        // reload data from doc, data from file has changed
template <class T, bool (T::*M) ( int )>
owl::TResult    DecodeVnReloadData      (   void* i,    owl::TParam1,   owl::TParam2 p2     )
{ return ( static_cast<T*>( i )->*M ) ( int ( p2 ) ) ? TRUE : FALSE; }

NOTIFY_SIG  (vnReloadData,      int )

#define EV_VN_RELOADDATA        VN_DEFINE   (vnReloadData,  VnReloadData, DecodeVnReloadData)

                                        // specifying WHAT to reload:
enum    VnReloadDataCodes : int 
        {
        EV_VN_RELOADDATA_EEG            = 1,
        EV_VN_RELOADDATA_FREQ,
        EV_VN_RELOADDATA_TRG,
        EV_VN_RELOADDATA_DOCPOINTERS,
        EV_VN_RELOADDATA_CAPTION,
        EV_VN_RELOADDATA_REF,
        EV_VN_RELOADDATA_FILTERS
        };


//----------------------------------------------------------------------------
                                        // a new Time Frame cursor is available
class       TTFCursor;

template <class T, bool (T::*M) ( TTFCursor* )>
owl::TResult    DecodeVnNewTFCursor     (   void* i,    owl::TParam1,   owl::TParam2 p2     )
{ return ( static_cast<T*>( i )->*M ) ( (TTFCursor*) ( p2 ) ) ? TRUE : FALSE; }

NOTIFY_SIG  (vnNewTFCursor,     TTFCursor* )

#define EV_VN_NEWTFCURSOR       VN_DEFINE   (vnNewTFCursor, VnNewTFCursor, DecodeVnNewTFCursor)


//----------------------------------------------------------------------------
                                        // a synchronization was done
template <class T, bool (T::*M) ( TTFCursor* )>
owl::TResult    DecodeVnViewSync        (   void* i,    owl::TParam1,   owl::TParam2 p2     )
{ return ( static_cast<T*>( i )->*M ) ( (TTFCursor*) ( p2 ) ) ? TRUE : FALSE; }

NOTIFY_SIG  (vnViewSync,        TTFCursor* )

#define EV_VN_VIEWSYNC          VN_DEFINE   (vnViewSync, VnViewSync, DecodeVnViewSync)


//----------------------------------------------------------------------------
                                        // a new selection is available
class       TSelection;

template <class T, bool (T::*M) ( TSelection* )>
owl::TResult    DecodeVnNewSelection    (   void* i,    owl::TParam1,   owl::TParam2 p2     )
{ return ( static_cast<T*>( i )->*M ) ( (TSelection*) ( p2 ) ) ? TRUE : FALSE; }

NOTIFY_SIG  (vnNewSelection,    TSelection* )

#define EV_VN_NEWSELECTION      VN_DEFINE   (vnNewSelection, VnNewSelection, DecodeVnNewSelection)


//----------------------------------------------------------------------------
                                        // a new highlighted set of tracks is available
template <class T, bool (T::*M) ( TSelection* )>
owl::TResult    DecodeVnNewHighlighted  (   void* i,    owl::TParam1,   owl::TParam2 p2     )
{ return ( static_cast<T*>( i )->*M ) ( (TSelection*) ( p2 ) ) ? TRUE : FALSE; }

NOTIFY_SIG  (vnNewHighlighted,  TSelection* )

#define EV_VN_NEWHIGHLIGHTED    VN_DEFINE   (vnNewHighlighted, VnNewHighlighted, DecodeVnNewHighlighted)


//----------------------------------------------------------------------------
                                        // a new bad set of tracks is available
template <class T, bool (T::*M) ( TSelection* )>
owl::TResult    DecodeVnNewBadSelection (   void* i,    owl::TParam1,   owl::TParam2 p2     )
{ return ( static_cast<T*>( i )->*M ) ( (TSelection*) ( p2 ) ) ? TRUE : FALSE; }

NOTIFY_SIG  (vnNewBadSelection, TSelection* )

#define EV_VN_NEWBADSELECTION   VN_DEFINE   (vnNewBadSelection, VnNewBadSelection, DecodeVnNewBadSelection)


//----------------------------------------------------------------------------
                                        // a new aux set of tracks is available
template <class T, bool (T::*M) ( TSelection* )>
owl::TResult    DecodeVnNewAuxSelection (   void* i,    owl::TParam1,   owl::TParam2 p2     )
{ return ( static_cast<T*>( i )->*M ) ( (TSelection*) ( p2 ) ) ? TRUE : FALSE; }

NOTIFY_SIG  (vnNewAuxSelection, TSelection* )

#define EV_VN_NEWAUXSELECTION   VN_DEFINE   (vnNewAuxSelection, VnNewAuxSelection, DecodeVnNewAuxSelection)


//----------------------------------------------------------------------------
                                        // a view is about to be destroyed
class       TBaseView;

template <class T, bool (T::*M)( TBaseView* )>
owl::TResult    DecodeVnViewDestroyed   (   void* i,    owl::TParam1,   owl::TParam2 p2     )
{ return ( static_cast<T*>( i )->*M ) ( (TBaseView*) ( p2 ) ) ? TRUE : FALSE; }

NOTIFY_SIG  (vnViewDestroyed,   TBaseView* )

#define EV_VN_VIEWDESTROYED     VN_DEFINE   (vnViewDestroyed, VnViewDestroyed, DecodeVnViewDestroyed)


//----------------------------------------------------------------------------
                                        // update views
template <class T, bool (T::*M)( TBaseView* )>
owl::TResult    DecodeVnViewUpdated     (   void* i,    owl::TParam1,   owl::TParam2 p2     )
{ return ( static_cast<T*>( i )->*M ) ( (TBaseView*) ( p2 ) ) ? TRUE : FALSE; }

NOTIFY_SIG  (vnViewUpdated,     TBaseView* )

#define EV_VN_VIEWUPDATED       VN_DEFINE   (vnViewUpdated, VnViewUpdated, DecodeVnViewUpdated)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
