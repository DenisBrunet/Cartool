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

#include    <owl/dc.h>
#include    <owl/buttonga.h>
#include    <owl/eventhan.h>

#include    "Time.TTimer.h"
#include    "OpenGL.Colors.h"
#include    "OpenGL.Lighting.h"
#include    "WindowingUtils.h"

#include    "Geometry.TGeometryTransform.h"
#include    "Geometry.TDisplaySpaces.h"
#include    "Geometry.TOrientation.h"

#include    "TDisplayScaling.h"
#include    "TGlobalOpenGL.h"

#include    "TBaseDoc.h"

#include    "TCartoolMdiClient.h"

namespace crtl {

//----------------------------------------------------------------------------
                                        // This class includes common variables and tasks for all OpenGL views
//----------------------------------------------------------------------------
                                        // Our preferred DC
using               UseThisDC                   = owl::TPaintDC;    // owl::TWindowDC;
                                        // Max message ID used in Cartool
constexpr UINT      MaxUserCommandId            = 20000;

constexpr int       MaxViewTitleLength          = 2 * KiloByte;


//----------------------------------------------------------------------------
                                        // All sorts of predefined windows sizes
                                        // ratio actual full screen size -> next Cartool child window size
constexpr double    WindowHeightRatio           = 0.75;
                                        // child window ratios
//constexpr double  WindowHeightToWidthRatio    = 4.0 / 3.0;
constexpr double    WindowHeightToWidthRatio    = SqrtTwo;
                                        // secondary windows need a little more space horizontally, for gadgets
constexpr double    MoreWidthRatio              = 1.20;

                                        // Using defines as values need to be evaluated at each call, in case dpi has changed during process lifetime
                                        // For very big screen, it is a bit useless to have windows opening bigger than that:
#define             MinWindowHeight             Round ( MmToPixels (  80 ) )
#define             MaxWindowHeight             Round ( MmToPixels ( 200 ) )
#define             MaxWindowHeightLarge        Round ( MmToPixels ( 300 ) )

                                        // what is a small window?
#define             SmallWindowHeight           Round ( MmToPixels ( 40 ) )
#define             SmallWindowWidth            Round ( MmToPixels ( 40 ) )

                                        // Common scale from which all window sizes will be derived
#define             DefaultWindowSize           Round ( CartoolApplication->MmToPixels ( 100 ) )

#define             MRIWindowSizeW              DefaultWindowSize
#define             MRIWindowSizeH              DefaultWindowSize

#define             XYZWindowSize               DefaultWindowSize
#define             MapsWindowSizeH             XYZWindowSize
#define             MapsWindowSizeW             Round ( XYZWindowSize * MoreWidthRatio )
#define             InverseWindowSizeH          XYZWindowSize
#define             InverseWindowSizeW          Round ( XYZWindowSize * MoreWidthRatio )

#define             SolPointsWindowSize         DefaultWindowSize
                                        // set default height the same height as pot maps / inverse
#define             TracksWindowSizeH           DefaultWindowSize
#define             TracksWindowSizeW           Round ( TracksWindowSizeH / WindowHeightToWidthRatio )
#define             TracksBigWindowSizeH        TracksWindowSizeH
#define             TracksBigWindowSizeW        Round ( 2 * TracksWindowSizeW )

#define             LMWindowSizeH               DefaultWindowSize
#define             LMWindowSizeW               DefaultWindowSize
#define             MinLMWindowWidth            MmToPixels (  80 )
#define             MaxLMWindowWidth            MmToPixels ( 160 )


//----------------------------------------------------------------------------
                                        // OpenGL colors (kept as defines, because they are lists)
#define             GLBASE_BACKCOLOR_NORMAL     (GLfloat) 0.00, (GLfloat) 0.00, (GLfloat) 0.30, (GLfloat) 1.00
#define             GLBASE_BACKCOLOR_PRINTING   (GLfloat) 1.00, (GLfloat) 1.00, (GLfloat) 1.00, (GLfloat) 1.00
                                                // fog colors depending on the background, to render an equivalent perception
#define             GLBASE_FOGCOLOR_NORMAL      (GLfloat) 0.10, (GLfloat) 0.10, (GLfloat) 0.10, (GLfloat) 1.00
#define             GLBASE_FOGCOLOR_PRINTING    (GLfloat) 0.50, (GLfloat) 0.50, (GLfloat) 0.50, (GLfloat) 1.00

#define             GLBASE_GROUPCOLOR_EEG       (GLfloat) 0.30, (GLfloat) 0.00, (GLfloat) 1.00, (GLfloat) 1.00
#define             GLBASE_GROUPCOLOR_ROI       (GLfloat) 1.00, (GLfloat) 0.20, (GLfloat) 0.20, (GLfloat) 1.00
#define             GLBASE_GROUPCOLOR_XYZ       (GLfloat) 0.00, (GLfloat) 0.00, (GLfloat) 0.00, (GLfloat) 1.00
#define             GLBASE_GROUPCOLOR_IS        (GLfloat) 0.00, (GLfloat) 0.60, (GLfloat) 0.00, (GLfloat) 1.00
#define             GLBASE_GROUPCOLOR_RIS       (GLfloat) 0.00, (GLfloat) 0.50, (GLfloat) 0.80, (GLfloat) 1.00
#define             GLBASE_GROUPCOLOR_SP        (GLfloat) 0.00, (GLfloat) 0.00, (GLfloat) 0.00, (GLfloat) 1.00
#define             GLBASE_GROUPCOLOR_MRI       (GLfloat) 0.50, (GLfloat) 0.50, (GLfloat) 0.50, (GLfloat) 1.00
#define             GLBASE_GROUPCOLOR_OTHER     (GLfloat) 0.50, (GLfloat) 0.50, (GLfloat) 0.50, (GLfloat) 1.00
#define             GLBASE_ROI_NAME             GLBASE_GROUPCOLOR_ROI
#define             GLBASE_ROI_INFO             (GLfloat) 0.00, (GLfloat) 0.00, (GLfloat) 0.00, (GLfloat) 1.00


//----------------------------------------------------------------------------
                                        // Mouse & keyboard control
constexpr double    RotationStep                = 2.5;

constexpr double    MaxShiftDepthRange          = 0.15;

                                        // minimum mouse move, in some distance units, to initiate the direction detection
//#define           MinMouseMovePixels          5
#define             MinMouseMove                MmToPixels ( 1.5 )
                                        // a common scale, in some distance units, for many mouse displacement
//#define           MouseMoveScalePixels        20
#define             MouseMoveScale              MmToPixels ( 5 )
                                        // another one, but for faster movemements
//#define           MouseMoveScaleFastPixels    33
#define             MouseMoveScaleFast          MmToPixels ( 8.5 )

constexpr ULONG     MouseMoveHitDelay           = 500;
constexpr char      MouseMiddeClicksurrogate    = ' ';


//----------------------------------------------------------------------------
                                        // give some space from AbsRadius, for extra depth
constexpr double    DepthPositionRatio          = 3.0;
constexpr double    ExtraSize3D                 = 1.25;

constexpr double    FogDefaultNear              = 0.95;
constexpr double    FogDefaultFar               = 1.25;

constexpr GLfloat   DefaultLineWidthMin         =  0.1;
constexpr GLfloat   DefaultLineWidthMax         = 10.0;


//----------------------------------------------------------------------------

constexpr OrientEnum    DefaultPotentialOrientation = OrientTransverseTop;
constexpr OrientEnum    DefaultVolumeOrientation    = OrientSagittalLeft;


//----------------------------------------------------------------------------
                                        // Hints are glyphs / text that pop-out when controlling some operations with the mouse, like brightness or contrast
#define             CursorHintSize              MmToPixels ( 5 )

#define             GLBASE_CURSORHINTCOLOR      (GLfloat) 1.00, (GLfloat) 1.00, (GLfloat) 0.00, (GLfloat) 0.80
#define             GLBASE_CURSORHINTBACKCOLOR  (GLfloat) 0.00, (GLfloat) 0.00, (GLfloat) 0.00, (GLfloat) 0.60

#define             GLBASE_MINCOLOR             (GLfloat) 0.00, (GLfloat) 0.40, (GLfloat) 1.00
#define             GLBASE_MAXCOLOR             (GLfloat) 1.00, (GLfloat) 0.20, (GLfloat) 0.20

                                        // Using defines on purpose, as this should evaluate at each call
#define             ColorMapWidth               Round ( MmToPixels (  4 ) )
#define             ColorMapHeight              Round ( MmToPixels ( 53 ) )


//----------------------------------------------------------------------------
                                        // Billboards spheres parameters
constexpr int       BbSphereLowNumRounds        = 3;
constexpr int       BbSphereLowNumSlices        = 16;
constexpr int       BbSphereHighNumRounds       = 8;
constexpr int       BbSphereHighNumSlices       = 40;


//----------------------------------------------------------------------------
                                        // Cartool makes use of 2 different fonts

#define             SmallFontSize               AtLeast ( 13, Round ( CartoolObjects.CartoolApplication->MmToPixels ( 3.4 ) ) )
#define             BigFontSize                 AtLeast ( 15, Round ( CartoolObjects.CartoolApplication->MmToPixels ( 4.6 ) ) )

#define             SmallFontParameters         "Tahoma",  SmallFontSize, 0, 0, 0, FW_BOLD
#define             BigFontParameters           "Verdana", BigFontSize,   0, 0, 0, FW_NORMAL


//----------------------------------------------------------------------------
                                        // Print parameters in windows coordinates
constexpr double    PrintLeftPos                = 5;
inline double       PrintRightPos   ( double width )    { return width  - 5; }
inline double       PrintCenterPos  ( double width )    { return width  / 2; }
inline double       PrintTopPos     ( double height )   { return height - 3; }
constexpr double    PrintBottomPos              = 1;
constexpr double    PrintDepthPosBack           = -0.5;
constexpr double    PrintDepthPosFront          = 1;
inline double       PrintVerticalStep   ( TGLBitmapFont* font ) { return font->GetHeight () + 2; }


//----------------------------------------------------------------------------
                                        // Using a non-regular, though uniform distribution of pixel sub-sampling
                                        // See https://www.glprogramming.com/red/chapter10.html

constexpr int       NumAntiAliasingJitter       = 8;

extern const double AntiAliasingJitter[ NumAntiAliasingJitter ][ 2 ];


//----------------------------------------------------------------------------

enum    TextColorEnum
        {
        TextColorNone,
        TextColorLight,
        TextColorDark
        };


enum    ShowLabelEnum
        {
        ShowLabelNone,
        ShowLabelText,
        ShowLabelIndex,
        NumShowLabels
        };


enum    ShiftAxisEnum
        {
        ShiftNone           = -1,
        ShiftX              = 0,
        ShiftY,
        ShiftZ
        };


enum    {                               // keeping this as an int for derived windows have their own rendering
        RenderingUnchanged  = -1,
        RenderingOpaque,
        RenderingTransparent
        };


enum    {                               // keeping this as an int
        TimerTFCursor       = 1,
        TimerStartup,
        TimerToFlat,
        TimerTo3D,
        TimerMagnifierIn,
        TimerMagnifierOut,
        TimerRefresh,
        TimerCursor,
        };

                                        // Different ways of capturing the mouse
enum    CaptureEnum
        {
        CaptureNone,
        CaptureTFSync,                  // specifically synchronizing 2 secondary windows
        CaptureAddToGroup,              // adding new windows to group
        CaptureGLLink,                  // graphical link between windows
        CaptureGLMagnify,               // graphical magnifier
        CaptureGLTFCursor,              // extending time cursor in tracks view
        CaptureGLSelect,                // graphical selection (tracks)
        CaptureGLFCursor,               // extending frequency cursor in frequency view
        CaptureOperation,               // operation with another window (like masking)
        CaptureLeftButton,              // basic operations with left-click
        CaptureMiddleButton,            // basic operations with middle-click
        CaptureRightButton,             // all operations with right-click
        };

                                        // Parameters to CaptureMouse
enum    CaptureMouseEnum
        {
        Release,
        Capture,
        ForceCapture,
        ForceRelease,
        };


enum    MouseAxisEnum
        {  
        MouseAxisNone,
        MouseAxisHorizontal,
        MouseAxisVertical,
        MouseAxisDiagonal,
        };

                                        // point sizes
enum    PointsRendering
        {
        PointsShowNone,
        PointsTiny,
        PointsSmall,
        PointsNormal,
        PointsBig,

        PointsNumRendering
        };

                                        // relative ratio point radius
extern const double PointRadiusRatio    [ PointsNumRendering ];

                                        // give more radius boost for small points, less boost for big points
extern const double HighlightRadiusBoost[ PointsNumRendering ];

                                        // clicking closer to this ratio * median distance -> selection
constexpr double    PointNearFactor             = 0.40;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*  How all Paint-related methods split the workload of display:

    Paint                   from the local view, called from Windows
                            sets the world & viewport dimensions
                            sets the lights, world center & viewing rotation
                            virtual method, per TWindowView

        PrePaint            Sets up the OpenGL context & the projection

        AntialiasingPaint   Does the repeating job of sub-pixel multipass rendering

            NestedPaints    Loops through the 'Using' windows, calling their GLPaint, hence cumulating their rendering

                GLPaint     The actual OpenGL rendering, this is were you code the rendering of the window
                            While here, you have things all set for you (OpenGL context, window DC etc..)
                            virtual method, each view has of course a different rendering!

        PostPaint           Closes the rendering & sends the display buffer to screen

            HintsPaint      Optionally adding some graphic hints on top of the window
                            virtual method, allowing each view its own hints
*/

                                        // Parameters used to control GLPaint
enum    {
        GLPaintOwner                = 0x001,    // when called by the owner of the window
        GLPaintLinked               = 0x002,    // when called from a linked window

        GLPaintFast                 = 0x004,    // hint for window to speed up display - used for user interactions f.ex.

        GLPaintOpaque               = 0x010,    // draw only the opaque part      - separating these two is mandatory for proper transparency rendering
        GLPaintTransparent          = 0x020,    // draw only the transparent part

        GLPaintForceClipping        = 0x100,    // superseed the local clipping planes by those given as parameters, i.e. from another window

        GLPaintOpaqueOrTransparent  = ( GLPaintOpaque | GLPaintTransparent ),
        GLPaintNormal               = ( GLPaintOwner  | GLPaintOpaqueOrTransparent )
        };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Buttons common to all views, first on the left
enum    {
        BaseViewButtonSeparatorA,
        BaseViewButtonToObject,
        BaseViewButtonRendering,
        BaseViewButtonOrientation,
        BaseViewButtonMagnifier,

        NumBaseViewButtons
        };

                                        // Separators are rescaled by the same integer ratio as the other Button Gadgets
//#define             DefaultSeparator            6 * RescaleButtonDpi (), 0, true
#define             DefaultSeparator            6 * CartoolApplication->RescaleSizeActualDpi (), 0, true

                                        // Deriving class to make it dpi-aware
class   TButtonGadgetDpi    :   public  owl::TButtonGadget
{
public:

    using           owl::TButtonGadget::TButtonGadget;

protected:

    owl::TDib*      GetGlyphDib ();     // Overridden method will create a dpi-rescaled DIB - called at creation, and system color change

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class   TBaseView :         public      owl::TWindowView,
                            public      TCartoolObjects,
                            public      TGlobalOpenGL,
                            protected   TDisplayScaling
{
public:
                            TBaseView ( TBaseDoc& doc, owl::TWindow* parent = 0, TLinkManyDoc* group = 0 );
    virtual                ~TBaseView ();


                                        // Cartool
    TBaseDoc*               BaseDoc;    // points to the document owning this window
    TLinkManyDoc*           GODoc;      // points to the optional group which owns this window - used for messaging


    crtl::TList<TBaseView>  Using;      // Other windows that will be used to render this one, cumulating their graphical results
    crtl::TList<TBaseView>  UsedBy;     // Windows that use me for rendering - the counterpart of Using
    bool                    IsUsedBy ()                                     const   { return (bool) UsedBy; }

                                        // Windows
    UINT                    FriendshipId;       // General syncing between any windows: views with the same FriendshipId will share messages
                                        // Friendship : refers to friendship IDs; FriendView : refers to view
    bool                    IsFriendship        ( UINT friendshipid     )   const   { return  FriendshipId == friendshipid; }
    bool                    IsFriendView        ( const TBaseView* view )   const   { return  FriendshipId == view->FriendshipId; }
    bool                    IsFriendView        ( UINT viewid           )   const;
    bool                    HasOtherFriendship  ()                                  { return  FriendshipId != GetViewId (); }   // still possible that another view has this view Id..
    void                    ResetFriendship     ()                                  { SetFriendship ( GetViewId () ); }         // setting friendship to its own ID will cancel this view
    bool                    CancelFriendship    ();                                                                             // doing more than ResetFriendship because of transitivity
    void                    SetFriendship       ( UINT friendshipid     )           { FriendshipId = friendshipid; }
    bool                    SetFriendView       ( const TBaseView* view );


    owl::TSize              WindowClientOffset;
    owl::TSize              StandSize;
    UINT                    LinkedViewId;       // view ID which controls another one: f.ex. 1 potentials view -> 1 tracks view
    owl::TResult            EvCommand           ( owl::uint id, THandle hWndCtl, owl::uint notifyCode );
    owl::TResult            WindowProc          ( owl::uint msg, owl::TParam1 p1, owl::TParam2 p2 );


    bool                    IsWindowMinimized   ()                                          const   { return crtl::IsWindowMinimized ( GetParentO () ); }
    bool                    IsWindowMaximized   ()                                          const   { return crtl::IsWindowMaximized ( GetParentO () ); }

    void                    WindowMinimize      ()                                          const   { crtl::WindowMinimize          ( GetParentO () ); }
    void                    WindowMaximize      ()                                          const   { crtl::WindowMaximize          ( GetParentO () ); }
    void                    WindowRestore       ()                                          const   { crtl::WindowRestore           ( GetParentO () ); }
    void                    WindowHide          ()                                          const   { crtl::WindowHide              ( GetParentO () ); }

    void                    WindowSetOrigin     ( int left, int top )                       const   { crtl::WindowSetOrigin         ( GetParentO (), left,  top );                  }
    void                    WindowSetSize       ( int width, int height )                   const   { crtl::WindowSetSize           ( GetParentO (), width, height );               }
    void                    WindowSetPosition   ( int left, int top, int width, int height )const   { crtl::WindowSetPosition       ( GetParentO (), left,  top,   width, height ); }
    void                    RepositionMinimizedWindow   ( int clientheight )                const   { crtl::RepositionMinimizedWindow ( GetParentO (), clientheight );              }

    int                     GetWindowLeft       ()                                          const   { return crtl::GetWindowLeft    ( GetParentO () ); }
    int                     GetWindowRight      ()                                          const   { return crtl::GetWindowRight   ( GetParentO () ); }
    int                     GetWindowTop        ()                                          const   { return crtl::GetWindowTop     ( GetParentO () ); }
    int                     GetWindowBottom     ()                                          const   { return crtl::GetWindowBottom  ( GetParentO () ); }
    int                     GetWindowWidth      ()                                          const   { return crtl::GetWindowWidth   ( GetParentO () ); }
    int                     GetWindowHeight     ()                                          const   { return crtl::GetWindowHeight  ( GetParentO () ); }

    int                     GetActualDpi        ()                                          const   { return CartoolApplication->GetActualDpi   (); }
    double                  MmToPixels          ( double mm     )                           const   { return CartoolApplication->MmToPixels     ( mm     );  }
    double                  PixelsToMm          ( int    pixels )                           const   { return CartoolApplication->PixelsToMm     ( pixels );  }


    void                    CaptureMouse        ( CaptureMouseEnum how );


    int                     NumControlBarGadgets;
    owl::TGadget**          ControlBarGadgets;  // pointers to local gadgets
    virtual void            CreateGadgets       ()                      = 0;
    virtual void            DestroyGadgets      ();
    void                    ButtonGadgetSetState( owl::TGadget *tog, bool down );
    void                    ButtonGadgetSetState( int Id, bool down );

                                        // Cartool
    void                    SetStandSize        ()                              { EvSize ( 0 , StandSize ); }
    virtual void            FitItemsInWindow    ( int numitems, owl::TSize itemsize, int &byx, int &byy, owl::TRect *winrect = 0 );
    bool                    Outputing           ()                      const   { return CartoolApplication->Bitmapping; }
    bool                    ShowNow             ( UINT redrawflags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN ) { return RedrawWindow ( 0, 0, redrawflags ); }
    const char*             GetTitle            ()                      const   { return Title; }
    virtual bool            ModifyPickingInfo   ( TPointFloat& Picking, char* buff )    { ClearString ( buff ); return false; }
    TPointFloat             GetLastPick         ()                      const   { return Picking; }
    bool                    IsCommandSender     ()                      const   { return ! GetCommandsCloning () || CartoolApplication->LastActiveBaseView == this; } // regular case, or forwarded message to itself
    bool                    IsCommandReceiver   ()                      const   { return   GetCommandsCloning () && CartoolApplication->LastActiveBaseView != this; } // receiving message from another view via command cloning

                                        // owl::TWindowView
    bool                    CanClose            ();
    void                    SetupWindow         ();
    virtual void            SetFocusBack        ( UINT backto );
    void                    EvSetFocus          ( HWND );
    void                    EvKillFocus         ( HWND );
    void                    EvKeyDown           ( owl::uint key, owl::uint repeatCount, owl::uint flags );
    void                    EvKeyUp             ( owl::uint key, owl::uint repeatCount, owl::uint flags );
    void                    EvSize              ( owl::uint, const owl::TSize& );
    bool                    EvEraseBkgnd        ( HDC );
    void                    EvMouseWheel        ( owl::uint modKeys, int zDelta, const owl::TPoint& p );

                                        // OpenGL
    TGLColorTable*          GetColorTable           ()                              { return &ColorTable; }
    int                     GetRenderingMode        ()                      const   { return RenderingMode; }
    void                    SetRenderingMode        ( int renderingmode )           { RenderingMode  = renderingmode; }
    virtual bool            IsRenderingMode         ( int renderingmode )           { return false; }
    TGLDepthRange*          GetDepthRange           ()                              { return &DepthRange; }
    double                  GetCurrentWindowSize    ( bool locally );
    double                  GetCurrentZoomFactor    ( bool locally );
    TGLMatrix*              GetModelRotMatrix       ()                              { return &ModelRotMatrix; }

                                        // basically taking the Doc transform, but allows to branch to another docs/views
    virtual const TBaseDoc*             GetGeometryDoc      ()              const   { return BaseDoc; }
    virtual const TGeometryTransform*   GetGeometryTransform()              const   { return GetGeometryDoc ()->GetGeometryTransform (); }
    virtual const TDisplaySpaces&       GetDisplaySpaces    ()              const   { return GetGeometryDoc ()->GetDisplaySpaces     (); }
    int                                 GetCurrentSpace     ()              const   { return CurrentDisplaySpace; };


protected:
                                        // owl::TWindowView
//  long                    GetClassName    ( LPTSTR className, int maxCount ) const    { return owl::TWindow::GetClassName ( "Cartool.BaseWindow", 18 ); } // ?Do we really need these?
//  LPCTSTR                 GetClassName()                                              { return "Cartool.BaseWindow"; }
//  void                    GetWindowClass ( WNDCLASS &wc );
    char                    Title[ MaxViewTitleLength ];  // Document's title - default, or possibly upgraded by derived to include more infos

                                        // Cartool
    TTimer                  AnimFx;
    TTimer                  AnimTF;

                                        // Keeping tracks of keyboard state, updated from the queued messages
    bool                    ControlKey;
    bool                    ShiftKey;
//  bool                    AltKey;
    bool                    LeftKey;
    bool                    RightKey;
    bool                    UpKey;
    bool                    DownKey;
    bool                    HasAlternateKeys ()                         const   { return ! ( ControlKey || ShiftKey /*|| AltKey*/ ); }

                                        // General Windows controls
    owl::TDC*               MouseDC;
    bool                    LButtonDown;
    bool                    MButtonDown;
    bool                    RButtonDown;
    owl::TPoint             MousePos;
    MouseAxisEnum           MouseAxis;
    owl::TPoint             LastMouseMove;
    CaptureEnum             CaptureMode;    // owner window's current capture operation, even if receiving messages from another window

                                        // General OpenGL controls
    int                     RenderingMode;
    TPointFloat             ModelCenter;
    GLfloat                 ModelRadius;            // used for depth / fog
    TGLMatrix               ModelRotMatrix;
    TGLMatrix               MatProjection;
    TGLMatrix               MatModelView;
    double                  Zoom;
    GLint                   ViewportOrgSize [ 4 ];
    owl::TRect              PaintRect;

    ShowLabelEnum           ShowLabels;             // usually names of electrodes, solution points...
    TextColorEnum           ShowTextColor;          // color of said labels
    bool                    ShowInfos;
    bool                    ShowAxis;
    bool                    ShowColorScale;
    bool                    ShowOrientation;
    int                     Orientation;
    bool                    ShowSizeBox;
    bool                    ShowBoundingBox;

    ShiftAxisEnum           ShiftAxis;
    double                  Magnifier       [ 3 ];
    double                  ModelRotAngle   [ 4 ];
    int                     CurrentDisplaySpace;    // each view can have a different current display space

                                        // General OpenGL objects
    TGLColorTable           ColorTable;
    ColorTablesEnum         ColorTableIndex[ NumAtomTypes ];
    TGLMultiColor<GLfloat>  BackColor;
    TGLMultiColor<GLfloat>  FogColor;
    TGLMultiColor<GLfloat>  TextColor;
    TGLMultiColor<GLfloat>  LineColor;

    TGLLight<GLfloat>       Light0;
    TGLLight<GLfloat>       Light1;
    TGLFog<GLfloat>         Fog;

    TGLArrow<GLfloat>       Xaxis;
    TGLArrow<GLfloat>       Yaxis;
    TGLArrow<GLfloat>       Zaxis;
    TPointFloat             Picking;
    double                  OriginRadius;

    TGLClipPlane            ClipPlane[ 3 ];
    TGLDepthRange           DepthRange;

    TGLPrimitives           Prim;

                                        // Cartool
    virtual bool            ValidView               ()                      { return true; }
    owl::TWindow*           GetWindowPtr            ( THandle handle )      { return CartoolApplication->GetWindowPtr ( handle ); }
    virtual void            CreateBaseGadgets       ();
    TBaseView*              ClientToBaseView        ( const owl::TPoint& p );
    bool                    GetCommandsCloning      ()      const;

                                        // owl::TWindowView
    bool                    SetDocTitle             ( LPCTSTR docname, int index );
    void                    EvGetMinMaxInfo         ( MINMAXINFO& minmaxinfo );
    void                    EvTimer                 ( owl::uint timerId );
    void                    UpdateCaptionUsing      ( char *buff );

    virtual bool            VnViewDestroyed         ( TBaseView* view );

    virtual void            CmEditUndo                  ();
    virtual void            CmEditUndoEnable            ( owl::TCommandEnabler& tce );
    virtual void            CmGeometryTransform         ();
    virtual void            CmGeometryTransformEnable   ( owl::TCommandEnabler& tce );
    virtual void            Cm2Object                   ();

                                        // OpenGL
    virtual void            PrePaint                ( owl::TDC& dc, owl::TRect& rect, double objectradiusx, double objectradiusy, double objectradiusz );
    virtual void            AntialiasingPaint       ();
    virtual void            NestedPaints            ();
    virtual void            GLPaint                 ( int how, int renderingmode, TGLClipPlane* clipplane ) {}
    virtual void            PostPaint               ( owl::TDC& dc );
    virtual void            HintsPaint              (); // more related to the window itself

    virtual void            SetColorTable           ( AtomType datatype );

    void                    SetWindowCoordinates    ( bool savestate = true );
    void                    ResetWindowCoordinates  ( bool restorestate = true );
    virtual void            GLEditCopyBitmap        ();// special copy, due to OpenGL specificities
    void                    GLLButtonDown           ( owl::uint, const owl::TPoint& p );
    void                    GLMouseMove             ( owl::uint, const owl::TPoint& p );

    virtual void            CmSetRenderingMode      ()                      {}
    virtual void            CmOrient                ();
    void                    SetOrient               ( TBaseDoc* doc );
    virtual void            CmMagnifier             ();
    virtual void            CmShowAll               ( owlwparam w );
    virtual void            CmShowInfos             ();
    virtual void            CmShowAxis              ();
    virtual void            CmShowColorScale        ();
    virtual void            CmShowOrientation       ();
    virtual void            CmShowSizeBox           ();
    virtual void            CmShowBoundingBox       ();
    virtual void            CmSetShiftDepthRange    ( owlwparam w );
    int                     NextRois                ( int currrois, int dimrois );

    virtual bool            NotSmallWindow          ()                      { return PaintRect.Height () >= SmallWindowHeight && PaintRect.Width () >= SmallWindowWidth; }
    void                    AxisToBorder            ( TGLCoordinates<GLfloat>& dir, TGLCoordinates<GLfloat>& border1, int& textattr1, TGLCoordinates<GLfloat>& border2, int& textattr2 );
    void                    DrawOrientation         ( const TOrientationType* boxsides = 0 );
    void                    DrawAxis                ();
    void                    DrawCross               ( const TPointFloat& p, double r, bool colormin );


    DECLARE_RESPONSE_TABLE (TBaseView);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
