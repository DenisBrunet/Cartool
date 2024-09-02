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

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Wrapping owl::TWindow with simple utilities

inline  bool    IsWindowMinimized   ( const owl::TWindow* window )                                              { return  window && window->IsIconic ();    }
inline  bool    IsWindowMaximized   ( const owl::TWindow* window )                                              { return  window && window->IsZoomed ();    }

                                        // Getting position and size                                            
inline  int     GetWindowLeft       ( const owl::TWindow* window )                                              { return  window ? window->Attr.X                       : 0;    }
inline  int     GetWindowRight      ( const owl::TWindow* window )                                              { return  window ? window->Attr.X + window->Attr.W - 1  : 0;    }
inline  int     GetWindowMiddleHoriz( const owl::TWindow* window )                                              { return  window ? window->Attr.X + window->Attr.W / 2  : 0;    }
inline  int     GetWindowTop        ( const owl::TWindow* window )                                              { return  window ? window->Attr.Y                       : 0;    }
inline  int     GetWindowBottom     ( const owl::TWindow* window )                                              { return  window ? window->Attr.Y + window->Attr.H - 1  : 0;    }
inline  int     GetWindowMiddleVert ( const owl::TWindow* window )                                              { return  window ? window->Attr.Y + window->Attr.H / 2  : 0;    }
inline  int     GetWindowWidth      ( const owl::TWindow* window )                                              { return  window ? window->Attr.W                       : 0;    }
inline  int     GetWindowHeight     ( const owl::TWindow* window )                                              { return  window ? window->Attr.H                       : 0;    }
inline  int     GetWindowMinSide    ( const owl::TWindow* window )                                              { return  window ? min ( GetWindowWidth ( window ), GetWindowHeight ( window ) ) : 0; }


inline  void    RepositionMinimizedWindow   ( owl::TWindow* window, int clientheight );

                                                                                                                                                  
inline owl::TDib*   RescaleDIB      ( const owl::TWindow* window, int resid, double scalingfactor );

                                        // Setting position and size                                            
inline  void    WindowMinimize      ( owl::TWindow* window )                                                    { if ( window ) window->ShowWindow ( SW_SHOWMINIMIZED ); }
inline  void    WindowMaximize      ( owl::TWindow* window )                                                    { if ( window ) window->ShowWindow ( SW_SHOWMAXIMIZED ); }
inline  void    WindowRestore       ( owl::TWindow* window )                                                    { if ( window ) window->ShowWindow ( SW_RESTORE       ); }
inline  void    WindowHide          ( owl::TWindow* window )                                                    { if ( window ) window->ShowWindow ( SW_HIDE          ); }

inline  void    WindowSetOrigin     ( owl::TWindow* window,   int left,   int top )                             { if ( window ) window->SetWindowPos ( 0, left, top, 0,     0,      SWP_NOCOPYBITS   | SWP_SHOWWINDOW | SWP_NOSIZE ); }
inline  void    WindowSetSize       ( owl::TWindow* window,   int width,  int height )                          { if ( window ) window->SetWindowPos ( 0, 0,    0,   width, height, SWP_NOCOPYBITS   | SWP_SHOWWINDOW | SWP_NOMOVE ); }
inline  void    WindowSetPosition   ( owl::TWindow* window,   int left,   int top,    int width,  int height )  { if ( window ) window->SetWindowPos ( 0, left, top, width, height, SWP_NOCOPYBITS   | SWP_SHOWWINDOW              ); }
inline  void    WindowSetFrameOrigin( owl::TWindow* window,   int left,   int top )                             { if ( window ) window->SetWindowPos ( 0, left, top, 0,     0,      SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOSIZE ); }


//----------------------------------------------------------------------------
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
wndpl.ptMinPosition.y   = clientheight - window->GetWindowRect ().Height ();    // always move minimized window to the bottom - X position does not need to be adjusted, though

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

}
