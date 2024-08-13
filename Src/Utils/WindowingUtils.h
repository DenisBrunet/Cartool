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

inline  bool    IsWindowMinimized   ( const owl::TWindow* window )                                              { return  window ? window->IsIconic ()                  : false; }
inline  bool    IsWindowMaximized   ( const owl::TWindow* window )                                              { return  window ? window->IsZoomed ()                  : false; }

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

                                                                                                                                                  
inline  int     GetWindowDpi        ( const owl::TWindow* window = 0 )                                          { return  window && window->Handle  ? GetDpiForWindow ( window->Handle ) : GetDpiForSystem ();  }   // current window dpi, or system dpi as a fallback
inline  double  GetWindowDpmm       ( const owl::TWindow* window = 0 )                                          { return  crtl::GetWindowDpi ( window ) / 25.4;                 }   // returns a floating point, due to rescaling

inline  double  MmToPixels          ( const owl::TWindow* window, double mm     )                               { return  mm     * ( GetWindowDpmm ( window ) * 0.96 );         }   // not sure why it needs this adjustment - to be checked on other screens
inline  double  PixelsToMm          ( const owl::TWindow* window, int    pixels )                               { return  pixels / ( GetWindowDpmm ( window ) * 0.96 );         }

inline  double  RescaleSizeDpi      ( const owl::TWindow* window )                                              { return  crtl::GetWindowDpi ( window ) / (double) USER_DEFAULT_SCREEN_DPI; }   // Rescale default 96 dpi pixel size into an equivalent pixel size of any arbitrary dpi
inline  int     RescaleButtonDpi    ( const owl::TWindow* window = 0 )                                          { return  Round ( crtl::RescaleSizeDpi ( window ) );            }   // Buttons need to be rescaled by integer increments so as to preserve their "pixel-art" aspect
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
                                        // Load and rescale a resource bitmap
                                        // There are a lot of hoops to jump through to be able to use Win32 / GDI functions
owl::TDib*  RescaleDIB  ( const owl::TWindow* window, int resid, double scalingfactor )
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
