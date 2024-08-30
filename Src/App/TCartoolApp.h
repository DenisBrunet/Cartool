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

#include    "owl/decmdifr.h"            // TDecoratedMDIFrame

#include    <owl/controlb.h>
#include    <owl/docking.h>
#include    <owl/rcntfile.h>
#include    <owl/filedoc.h>
#include    <owl/docmanag.h>
#include    <owl/tooltip.h>
#include    <owl/textgadg.h>
#include    <owl/wsyscls.h>             // TDropInfo
#include    <owl/splashwi.h>

#include    "Files.Utils.h"
#include    "System.h"
#include    "OpenGL.h"
#include    "WindowingUtils.h"

#include    "resource.h"
#include    "Files.Extensions.h"
#include    "TCartoolDocManager.h"
#include    "App.DocView.h"             // Cartool custom messages Ids and handlers definitions

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Program name
constexpr char* CartoolTitle            = "Cartool";
constexpr char* HelpShortFileName       = "Cartool.chm";


constexpr int   ShortStringLength       = 64;

                                        // Defines used in the Windows registry
                                        // Software\Company\Program
constexpr char* CartoolRegistryUserHome = "Software\\Cartool\\Cartool";
constexpr char* CartoolRegistryCompany  = CartoolTitle;

                                        // preferences
constexpr char* PrefGraphic             = "graphic";

constexpr char* PrefAcceleration        = "acceleration";
constexpr char* Pref3DTextures          = "3dtextures";

constexpr char* PrefAuto                = "auto";
constexpr char* PrefOn                  = "on";
constexpr char* PrefOff                 = "off";

constexpr char* PrefAuxiliaries         = "auxs";

                                        // WPARAM type inside OWLNext
//using         owlwparam               = WPARAM;       // !NOT working in OwlNext 64 bits, which still fancy 32 bits WPARAM!
using           owlwparam               = unsigned int;

                                        // Used to force the event list to be processed, keeping the application alive
//#define       UpdateApplication       ::GetApplicationObject ()->PumpWaitingMessages ()
#define         UpdateApplication       { if ( IsMainThread () ) CartoolObjects.CartoolApplication->PumpWaitingMessages (); }


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Accessors are not accessible(!) - We need to derive class
class   TCartoolSplashWindow    : public owl::TSplashWindow
{
public:

    TCartoolSplashWindow (  const owl::TDib&    dib,
                            int                 width,
                            int                 height,
                            int                 style   = None,
                            owl::uint           timeOut = 0,
                            LPCTSTR             title   = 0,
                            owl::TModule*       module  = 0     ) : owl::TSplashWindow ( dib, width, height, style, timeOut, title, module )    {}

                                                                 // we currently only request ShrinkToFit, any other value means it is either finished (0) or garbage (random)
    bool        IsActive    ()                  const   {   return  GetStyle () == ShrinkToFit /*!= None*/ /*GetTimeOut () != 0*/; }
//  int         GetStyle    ()                  const   {   return  GetStyle ();         }
//  int         GetTimeOut  ()                  const   {   return  GetTimeOut ();       }
};


//----------------------------------------------------------------------------

class       TGoF;


class TCartoolApp : public  owl::TApplication,
                    public  TCartoolObjects,    // this is the first and only derived object that will actually set these pointers
                    public  owl::TRecentFiles,
                    public  TPreferences
{
public:
                                                                  // default Owl module
                    TCartoolApp ( LPCTSTR name, TModule*& tomodule = owl::Module, owl::TAppDictionary* appdir = 0 );
                   ~TCartoolApp ();


    owl::THarbor*               ApxHarbor;
    owl::TDockableControlBar*   ControlBar;

    TBaseView*      LastActiveBaseView;

    bool            Bitmapping;         // if the output is a metafile (or bitmap, in fact)
    bool            Closing;            // force linked doc to CanClose
    bool            AnimateViews;       // global flag to tell windows to skip or not opening animations


    TFileName       ApplicationFullPath;        // full path to executable
    TFileName       ApplicationDir;             // full path to directory
    char            ApplicationFileName [ ShortStringLength ];  // file name only (no directory, no extension)
    TFileName       HelpFullPath;               // full path to help file

    char            ProdName            [ ShortStringLength ];  // this is used to access the registry, i.e. storing and retrieving options - is should remain as 'Cartool' to be able to access the same options in case of different executables
    char            ProdVersion         [ ShortStringLength ];
    char            ProdRevision        [ ShortStringLength ];
    char            DefaultTitle        [ MaxAppTitleLength ];


    std::unique_ptr<owl::TCursor>   CursorSyncViews;
    std::unique_ptr<owl::TCursor>   CursorAddToGroup;
    std::unique_ptr<owl::TCursor>   CursorLinkView;
    std::unique_ptr<owl::TCursor>   CursorMagnify;
    std::unique_ptr<owl::TCursor>   CursorOperation;

    FormatWish      PrefGraphicAccel;
    FormatWish      PrefGraphic3DTextures;


    void            InitMainWindow      ();
    void            InitInstance        ();
    void            SetScreen           ();
    bool            CanClose            ();
    bool            ProcessAppMsg       ( MSG& msg );

    bool            IsInteractive       ()                                          const   { return  owl::TModule::IsLoaded (); /*CartoolMainWindow != 0;*/ }
    bool            IsNotInteractive    ()                                          const   { return  ! IsInteractive (); }


    void            CreateGadgets       ( bool server = false );
    void            InsertGadgets       ( owl::TGadget** listgadgets, int numgadgets );
    void            RemoveGadgets       ( int afterID = CM_VIEWCREATE );

    void            ResetRegisterInfo   ();
    void            UnRegisterInfo      ();

    TBaseView*      GetViewFromDrop     ( owl::TDropInfo& drop );
    void            OpenDroppedFiles    ( TGoF &files, TBaseView *view = 0 );

    void            CreateSplashScreen  ();
    void            DestroySplashScreen ();


    void            WindowMinimize      ()                                          const   { crtl::WindowMinimize    ( CartoolMainWindow ); }
    void            WindowMaximize      ()                                          const   { crtl::WindowMaximize    ( CartoolMainWindow ); }
    void            WindowRestore       ()                                          const   { crtl::WindowRestore     ( CartoolMainWindow ); }
    void            WindowHide          ()                                          const   { crtl::WindowHide        ( CartoolMainWindow ); }

    void            WindowSetOrigin     ( int left, int top )                       const   { crtl::WindowSetOrigin   ( CartoolMainWindow, left, top );                }
    void            WindowSetSize       ( int width, int height )                   const   { crtl::WindowSetSize     ( CartoolMainWindow, width, height );            }
    void            WindowSetPosition   ( int left, int top, int width, int height )const   { crtl::WindowSetPosition ( CartoolMainWindow, left, top, width, height ); }
    void            WindowSetGaugeSize  ()                                          const   { WindowRestore (); WindowSetSize ( 600 /*2.5 * GaugeWidth*/, 240 /*7 * GaugeHeight*/ ); }


    int             GetWindowDpi        ( const owl::TWindow* window = 0 )          const   { return  window && window->Handle  ? GetDpiForWindow ( window->Handle ) : GetDpiForSystem ();  }   // current window dpi, or system dpi as a fallback
    double          GetActualDpi        ()                                          const   { return  ActualDpi;                                            }
    double          GetActualDpmm       ()                                          const   { return  ActualDpi / 25.4;                                     }   // returns a floating point, due to rescaling
    double          RescaleSizeActualDpi()                                          const   { return  GetActualDpi () / (double) USER_DEFAULT_SCREEN_DPI;   }   // Rescale default 96 dpi pixel size into an equivalent pixel size of any arbitrary dpi
    int             RescaleButtonActualDpi()                                        const   { return  Round ( RescaleSizeActualDpi () );                    }   // Buttons need to be rescaled by integer increments so as to preserve their "pixel-art" aspect
    double          MmToPixels          ( double mm     )                           const   { return  mm     * ( GetActualDpmm () * 0.96 );                 }   // not sure why it needs this adjustment - to be checked on other screens
    double          PixelsToMm          ( int    pixels )                           const   { return  pixels / ( GetActualDpmm () * 0.96 );                 }

                                        // for long batch processing, call these to set/reset temporary main window title (instead of plain Cartool) 
    void            SetMainTitle        ( const char* title, const char* path = 0 );
    void            SetMainTitle        ( const char* prestring, const char* path, TSuperGauge& gauge );
    void            SetMainTitle        ( TSuperGauge& gauge );
    void            ResetMainTitle      ();


protected:
    char            Title       [ MaxAppTitleLength ]; // What is the actual length in Windows?
    char            TitlePrefix [ MaxAppTitleLength ];

                                        // Screen & dpi
    int             MaxScreenWidth;
    int             MaxScreenHeight;
    int             ScreenWidth;
    int             ScreenHeight;
    double          MonitorDpi;
    double          ActualDpi;
    owl::TRect      MDIClientRect;


    void            CmHelpAbout         ();
    void            CmHelpContents      ( owlwparam w );
//  void            CmHelpUsing         ();
    void            EvDropFiles         ( owl::TDropInfo    drop );
    void            EvNewView           ( owl::TView&       view );
    void            EvCloseView         ( owl::TView&       view );
    void            EvOwlDocument       ( owl::TDocument&   doc );
    LRESULT         CmFileSelected      ( WPARAM wp, LPARAM lp );

    void            EvDisplayChange     ( owl::uint bbp, owl::uint resx, owl::uint resy );
    void            EvDpiChanged        ( int dpi, const owl::TRect& rect );


    void            RetrievePreferences                 ();     // reads the Registry, then stores the results internally
    void            CmPrefsRegistry                     ( owlwparam w );
    void            CmPrefsGraphic                      ( owlwparam w );
    void            CmPrefsGraphicAccelAutoEnable       ( owl::TCommandEnabler &tce );
    void            CmPrefsGraphicAccelOnEnable         ( owl::TCommandEnabler &tce );
    void            CmPrefsGraphicAccelOffEnable        ( owl::TCommandEnabler &tce );
    void            CmPrefsGraphic3DTexturesAutoEnable  ( owl::TCommandEnabler &tce );
    void            CmPrefsGraphic3DTexturesOnEnable    ( owl::TCommandEnabler &tce );
    void            CmPrefsGraphic3DTexturesOffEnable   ( owl::TCommandEnabler &tce );


private:
    bool            HelpState;          // Has the help engine been used.
    bool            ContextHelp;        // SHIFT-F1 state(context sensitive HELP).
    owl::TCursor*   HelpCursor;         // Context sensitive help cursor.

    TCartoolSplashWindow*   Splash;     // Splash screen window


    void            SetupSpeedBar       ( owl::TDecoratedMDIFrame* frame );
    void            ProcessCommandLine  ( const char* CmdLine, bool beforeinit );
    void            RegisterInfo        ();


    DECLARE_RESPONSE_TABLE(TCartoolApp);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
