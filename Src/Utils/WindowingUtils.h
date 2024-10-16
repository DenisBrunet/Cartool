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

#include    <owl/window.h>              // TWindow
#include    <owl/gdiobjec.h>            // TDib
#include    "Strings.Utils.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Windows10 seems to be adding some extra borders on each window, due to dropped-shadows(?)
                                        // Defined as const until we find a way to retrieve them from the system
constexpr int       Windows10OffsetW        =  20;
constexpr int       Windows10OffsetH        =  10;
constexpr int       Windows10OffsetX        =  10;
constexpr int       Windows10OffsetY        =   0;


//----------------------------------------------------------------------------
                                        // Wrapping owl::TWindow with simple utilities
enum    WindowState
        {
        UnknownWindowState,
        WindowStateNormal,
        WindowStateMinimized,
        WindowStateMaximized,
        WindowStateHidden,
        };
                                        // Versatile, all-in-one function to get/set a window state
inline  WindowState GetWindowState              ( const owl::TWindow* window );
inline  void        SetWindowState              ( owl::TWindow* window, WindowState newstate );
                                        // Specialized versions
inline  bool        IsWindowMinimized           ( const owl::TWindow* window )                                              { return  window &&   ( GetWindowLong ( window->Handle, GWL_STYLE ) & WS_MINIMIZE );    }
inline  bool        IsWindowMaximized           ( const owl::TWindow* window )                                              { return  window &&   ( GetWindowLong ( window->Handle, GWL_STYLE ) & WS_MAXIMIZE );    }
inline  bool        IsWindowHidden              ( const owl::TWindow* window )                                              { return  window && ! ( GetWindowLong ( window->Handle, GWL_STYLE ) & WS_VISIBLE  );    }   // IsWindowVisible is NOT working as expected...
inline  bool        IsWindowNormal              ( const owl::TWindow* window )                                              { return  window && ! ( IsWindowMinimized ( window ) || IsWindowMaximized ( window ) || IsWindowHidden ( window ) ); }

                                                    // Getting position and size                                            
inline  int         GetWindowLeft               ( const owl::TWindow* window )                                              { return  window ? window->Attr.X                       : 0;    }
inline  int         GetWindowRight              ( const owl::TWindow* window )                                              { return  window ? window->Attr.X + window->Attr.W - 1  : 0;    }
inline  int         GetWindowMiddleHoriz        ( const owl::TWindow* window )                                              { return  window ? window->Attr.X + window->Attr.W / 2  : 0;    }
inline  int         GetWindowTop                ( const owl::TWindow* window )                                              { return  window ? window->Attr.Y                       : 0;    }
inline  int         GetWindowBottom             ( const owl::TWindow* window )                                              { return  window ? window->Attr.Y + window->Attr.H - 1  : 0;    }
inline  int         GetWindowMiddleVert         ( const owl::TWindow* window )                                              { return  window ? window->Attr.Y + window->Attr.H / 2  : 0;    }
inline  int         GetWindowWidth              ( const owl::TWindow* window )                                              { return  window ? window->Attr.W                       : 0;    }
inline  int         GetWindowHeight             ( const owl::TWindow* window )                                              { return  window ? window->Attr.H                       : 0;    }
inline  int         GetWindowMinSide            ( const owl::TWindow* window )                                              { return  window ? min ( GetWindowWidth ( window ), GetWindowHeight ( window ) ) : 0; }
inline  int         GetMinimizedWindowHeight    ( owl::TWindow* window );

                                                                                                                                                      
inline owl::TDib*   RescaleDIB                  ( const owl::TWindow* window, int resid, double scalingfactor );

                                        // Setting position and size                                            
inline  void        WindowMinimize              ( owl::TWindow* window )                                                    { if ( window ) window->ShowWindow ( SW_SHOWMINIMIZED ); }
inline  void        WindowMaximize              ( owl::TWindow* window )                                                    { if ( window ) window->ShowWindow ( SW_SHOWMAXIMIZED ); }
inline  void        WindowRestore               ( owl::TWindow* window )                                                    { if ( window ) window->ShowWindow ( SW_RESTORE       ); }
inline  void        WindowHide                  ( owl::TWindow* window )                                                    { if ( window ) window->ShowWindow ( SW_HIDE          ); }  // Calling WindowRestore before, if we wish to avoid min / max and hidden

inline  void        WindowSetOrigin             ( owl::TWindow* window,   int left,   int top )                             { if ( window ) window->SetWindowPos ( 0, left, top, 0,     0,      SWP_NOCOPYBITS   | SWP_SHOWWINDOW | SWP_NOSIZE ); }
inline  void        WindowSetSize               ( owl::TWindow* window,   int width,  int height )                          { if ( window ) window->SetWindowPos ( 0, 0,    0,   width, height, SWP_NOCOPYBITS   | SWP_SHOWWINDOW | SWP_NOMOVE ); }
inline  void        WindowSetPosition           ( owl::TWindow* window,   int left,   int top,    int width,  int height )  { if ( window ) window->SetWindowPos ( 0, left, top, width, height, SWP_NOCOPYBITS   | SWP_SHOWWINDOW              ); }
inline  void        WindowSetFrameOrigin        ( owl::TWindow* window,   int left,   int top )                             { if ( window ) window->SetWindowPos ( 0, left, top, 0,     0,      SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOSIZE ); }
inline  void        RepositionMinimizedWindow   ( owl::TWindow* window, int clientheight );
inline  void        WindowSetGaugeSize          ( owl::TWindow* window )                                                    { WindowRestore ( window ); WindowSetSize (  window, 600 /*2.5 * GaugeWidth*/, 240 /*7 * GaugeHeight*/ ); }


inline DPI_AWARENESS        GetDPIAwareness         ();
inline int                  GetWindowDpi            ( const owl::TWindow* window = 0 )                                      { return  window && window->Handle  ? GetDpiForWindow ( window->Handle ) : GetDpiForSystem ();  }   // current window dpi, or system dpi as a fallback
inline const char*          GetCurrentMonitorName   ( HWND window, char* monitorname );
inline const char*          GetMonitorName          ( int monitorindex, char* monitorname );
inline owl::TRect           GetMonitorRect          ( const char* devicename, DWORD how = 0 );
inline vector<owl::TRect>   GetMonitorsResolution   ();
                                        // Used for dialog's point measurement conversion
inline int          MmToPoints                  ( double mm )                                                               { return  Round ( ( 72   / 25.4 ) * mm );   }
inline double       PointsToMm                  ( int    pt )                                                               { return          ( 25.4 / 72   ) * pt;     }


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

WindowState     GetWindowState  ( const owl::TWindow* window )
{
if      ( window == 0 )                     return  UnknownWindowState;
else if ( IsWindowHidden    ( window ) )    return  WindowStateHidden;      // !test early as window could still have the min / max flags on, and be hidden!
else if ( IsWindowMinimized ( window ) )    return  WindowStateMinimized;
else if ( IsWindowMaximized ( window ) )    return  WindowStateMaximized;
else                                        return  WindowStateNormal;
}


void            SetWindowState  ( owl::TWindow* window, WindowState newstate )
{
if      ( window == 0 )                         return;
else if ( newstate == WindowStateMinimized )    WindowMinimize ( window );
else if ( newstate == WindowStateMaximized )    WindowMaximize ( window );
else if ( newstate == WindowStateHidden    )    WindowHide     ( window );
else                                            WindowRestore  ( window );
}


//----------------------------------------------------------------------------
                                        // Assumes the minimized window height can not change during the process life time
int             GetMinimizedWindowHeight ( owl::TWindow* window )
{
                                        // Caching results, so the actual computation is done only once
static int          MinimizedWindowHeight   = 0;

if ( MinimizedWindowHeight != 0 )
    return  MinimizedWindowHeight;


if ( window == 0 )
    return  30;                         // Returns a dummy value until caller passes a valid pointer!


bool                iswinmin        = IsWindowMinimized ( window );

if ( ! iswinmin )
    WindowMinimize ( window );

MinimizedWindowHeight   = window->GetWindowRect ().Height ();

if ( ! iswinmin )
    WindowRestore ( window );

return  MinimizedWindowHeight;
}


//----------------------------------------------------------------------------
                                        // Minimized windows can not be moved with SetWindowPos, so we need a specific function for that...
void            RepositionMinimizedWindow ( owl::TWindow* window, int clientheight )
{
WINDOWPLACEMENT     wndpl;

wndpl.length            = sizeof ( WINDOWPLACEMENT );

if ( window == 0
  || ! GetWindowPlacement  ( window->GetHandle (), &wndpl )
  || wndpl.showCmd != SW_SHOWMINIMIZED )
                                        // Not even trying to move anything
    return;

                                        // Parameters for minimized windows only
wndpl.flags             = WPF_SETMINPOSITION;
wndpl.showCmd           = SW_SHOWMINIMIZED;
wndpl.ptMinPosition.y   = clientheight - GetMinimizedWindowHeight ( window );   // always move minimized window to the bottom - X position left "as is", though

SetWindowPlacement  ( window->GetHandle (), &wndpl );
}


//----------------------------------------------------------------------------
/*                                        // Retrieving various decoration sizes - unfinished prototype
void            GetMainWindowMeasures   ( owl::TMDIFrame* window )
{
owl::TRect          mainwr          = window                    ->GetWindowRect ().Normalized ();
//owl::TRect        maincr          = window                    ->GetClientRect ().Normalized ();
owl::TRect          mdiwr           = window->GetClientWindow ()->GetWindowRect ().Normalized ();
owl::TRect          mdicr           = window->GetClientWindow ()->GetClientRect ().Normalized ();

//DBGV6 ( mainwr.Left (), mainwr.Top (), mainwr.Right (), mainwr.Bottom (), mainwr.Width (), mainwr.Height (), "mainwr" );
//DBGV6 ( maincr.Left (), maincr.Top (), maincr.Right (), maincr.Bottom (), maincr.Width (), maincr.Height (), "maincr" );
//DBGV6 ( mdiwr .Left (), mdiwr .Top (), mdiwr .Right (), mdiwr .Bottom (), mdiwr .Width (), mdiwr .Height (), "mdiwr " );
//DBGV6 ( mdicr .Left (), mdicr .Top (), mdicr .Right (), mdicr .Bottom (), mdicr .Width (), mdicr .Height (), "mdicr " );

                                        // Main window precise sizes and deltas
int                 transparentmargin   = abs ( mainwr.Left () );                   // on each 4 sides of the main window

int                 mainwidth           = mainwr.Width  () - 2 * transparentmargin; // actual visible main window size
int                 mainheight          = mainwr.Height () - 2 * transparentmargin;


int                 decomargin          = ( mdiwr.Width () - mdicr.Width () ) / 2;  // small deco on each 4 axis of client

int                 mdidecotop          = mdiwr.Top () + decomargin;    // + transparentmargin?
int                 mdidecobottom       = decomargin;
int                 mdidecoleft         = decomargin;
int                 mdidecoright        = decomargin;

//DBGV5 ( mainwidth, mainheight, transparentmargin, mdicr .Width (), mdicr .Height (), "mainwidth, mainheight, transparentmargin, MDI client" );
//DBGV2 ( decomargin, mdidecotop, "decomargin, maindecotop" );
}
*/

//----------------------------------------------------------------------------
                                        // Load and rescale a resource bitmap
                                        // There are a lot of hoops to jump through to be able to use Win32 / GDI functions
owl::TDib*      RescaleDIB  ( const owl::TWindow* window, int resid, double scalingfactor )
{
using namespace owl;

if ( scalingfactor <= 0 || scalingfactor == 1 )

    return  new TDib ( 0, TResId ( resid ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We need device contexts all along
TClientDC           dc          ( window->GetHandle () );
                                        // Create dib from resource
TDib                srcdib      ( 0, TResId ( resid ) );

bool                haspalette      = srcdib.SizeColors () > 0;

auto                srcpal          = haspalette ? unique_ptr<TPalette> ( new TPalette ( srcdib ) ) : 0;
                                        // Get the bitmap data, with optional palette
TBitmap             srcbitmap   ( srcdib, srcpal.get () );
                                        // We need a memory dc context for source bitmap
TMemoryDC           srcdc       ( dc );
                                        // Load source bitmap to context
srcdc.SelectObject ( srcbitmap );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                
int                 destwidth       = Round ( scalingfactor * srcbitmap.Width  () );
int                 destheight      = Round ( scalingfactor * srcbitmap.Height () );
                                        // Create new destination bitmap
TBitmap             destbitmap  ( dc, destwidth, destheight );
                                        // We need a memory dc context for destination bitmap
TMemoryDC           destdc      ( dc );
                                        // Load source bitmap to context
destdc.SelectObject ( destbitmap );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Set the stretch mode to high-quality scaling
destdc.SetStretchBltMode ( HALFTONE );
                                        // Rescale the DIB image using StretchBlt
StretchBlt  (   destdc,     0,  0,  destwidth,          destheight, 
                srcdc,      0,  0,  srcbitmap.Width (), srcbitmap.Height (), 
                SRCCOPY
            );
                                        // Create resized DIB, with optional palette
return  new TDib ( destbitmap, srcpal.get() );

                                        // In theory, we should clean after ourselves, but these dc are temps, aren't they?
//srcdc. RestoreBitmap ();
//destdc.RestoreBitmap ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // DPI-awareness test, i.e. if process is allowed to resize its UI to adjust in real-time to different monitors' DPIs
DPI_AWARENESS   GetDPIAwareness ()
{
                                        // Simple test
if ( ! IsProcessDPIAware () )
    return  DPI_AWARENESS_UNAWARE;

                                        // Finer test for DPI-awareness
DPI_AWARENESS_CONTEXT   dpiawarenesscontext = GetThreadDpiAwarenessContext ();

DPI_AWARENESS           dpiawareness        = GetAwarenessFromDpiAwarenessContext ( dpiawarenesscontext );

return  dpiawareness;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Returns a string like "\\.\DISPLAY1", "\\.\DISPLAY2"...
const char*     GetCurrentMonitorName ( HWND hwindow, char* monitorname )
{
if ( monitorname == 0 )
    return  0;


HMONITOR            monitor         = MonitorFromWindow ( hwindow, MONITOR_DEFAULTTONEAREST );


MONITORINFOEX       monitorinfo;
monitorinfo.cbSize  = sizeof ( MONITORINFOEX );

GetMonitorInfo ( monitor, &monitorinfo );


return  StringCopy ( monitorname, monitorinfo.szDevice, CCHDEVICENAME - 1 );
}


//----------------------------------------------------------------------------
const char*     GetMonitorName ( int monitorindex, char* monitorname )
{
if ( StringIsEmpty ( monitorname ) )
    return  0;

char                buff[ 32 ];

GetCurrentMonitorName   ( 0, monitorname );
DeleteChars             ( monitorname, "0123456789" );
StringAppend            ( monitorname, IntegerToString ( buff, monitorindex ) );

return  monitorname;
}


//----------------------------------------------------------------------------
owl::TRect      GetMonitorRect  ( const char* monitorname, DWORD how )
{
owl::TRect          r;

if ( StringIsEmpty ( monitorname ) )
    return  r;


DEVMODE             devmode;
devmode.dmSize      = sizeof ( DEVMODE );

                                        // Caller has been specific about only retrieving the current state?
if ( how == ENUM_CURRENT_SETTINGS 
  || how == ENUM_REGISTRY_SETTINGS ) {
    
    EnumDisplaySettings ( monitorname, how, &devmode );

    r.SetWH (   devmode.dmPosition.x,   devmode.dmPosition.y, 
                devmode.dmPelsWidth,    devmode.dmPelsHeight  );
    }
else {
                                        // Loop through all possible states and retrieve the max size
    int             maxscreenwidth      = 0;
    int             maxscreenheight     = 0;

    for ( DWORD dmi = 0; EnumDisplaySettings ( monitorname, dmi, &devmode ) != 0; dmi++ ) {

        Maxed ( maxscreenwidth,  (int) devmode.dmPelsWidth  );
        Maxed ( maxscreenheight, (int) devmode.dmPelsHeight );
        }

    r.SetWH (   devmode.dmPosition.x,   devmode.dmPosition.y, 
                maxscreenwidth,         maxscreenheight       );
    }


return  r;
}


//----------------------------------------------------------------------------
                                        // Retrieves the TRect positions of each monitor current settings, within the Desktop virtual workspace
vector<owl::TRect>  GetMonitorsResolution   ()
{
vector<owl::TRect>  monitorsrect;

DISPLAY_DEVICE      displaydevice;
displaydevice.cb    = sizeof ( DISPLAY_DEVICE );


for ( DWORD ddi = 0; EnumDisplayDevices ( NULL, ddi, &displaydevice, 0 ) != 0; ddi++ ) {

                                        // Is that a real, connected monitor?
    if ( ! IsFlag ( displaydevice.StateFlags, (DWORD) DISPLAY_DEVICE_ATTACHED_TO_DESKTOP ) )
        continue;


//  bool            ismainmonitor   = IsFlag ( displaydevice.StateFlags, (DWORD) DISPLAY_DEVICE_PRIMARY_DEVICE );
    const char*     tomonitorname   = displaydevice.DeviceName;
    owl::TRect      r               = GetMonitorRect ( tomonitorname, ENUM_REGISTRY_SETTINGS );

    monitorsrect.push_back ( r );
    }


return  monitorsrect;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
