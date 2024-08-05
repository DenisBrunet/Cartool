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

//#include  "Time.TTimer.h"
#include    "Time.TAcceleration.h"
#include    "TInterval.h"
#include    "Strings.Grep.h"
#include    "TSetArray2.h"
#include    "TTFCursor.h"

#include    "TTracksDoc.h"
#include    "TElectrodesDoc.h"
#include    "TRoisDoc.h"

#include    "TBaseView.h"
#include    "TTracksViewScrollbar.h"
#include    "TTracksViewMontage.h"
#include    "TTracksViewWindowSlots.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // vertical part assigned to the regular tracks within a window slot / window
constexpr double    RegularTracksVerticalRatio  = 0.80;
                                        // setting a limit to the number of window slots - it can be very big, though
constexpr int       MaxWindowSlots              = 10000;
                                        // optional color scaling width on the right - enough space for floatting point values
constexpr int       ColorScaleWidth             = 75;
                                        // text margin on the left for track names has to be limited - though we give some very liberal limits here
constexpr int       TextMarginMin               =  50;
constexpr int       TextMarginMax               = 500;


constexpr double    EEGGLVIEW_STVMIN            = DBL_MIN;
constexpr double    EEGGLVIEW_STVMAX            = DBL_MAX;
constexpr double    EEGGLVIEW_STVINIT           = 30;
constexpr double    EEGGLVIEW_STVSMALLSTEP      = 1.1;
constexpr double    EEGGLVIEW_STVBIGSTEP        = 1.3;

constexpr double    EEGGLVIEW_STHSMALLSTEP      = 1.2;
constexpr double    EEGGLVIEW_STHBIGSTEP        = 1.4;
constexpr double    EEGGLVIEW_STHMINTF          = 5;


constexpr double    EEGGLVIEW_STEPREADPERCENT   = 0.75;
constexpr int       EEGGLVIEW_MINTRACKSPRECISION= 100;
constexpr int       EEGGLVIEW_DELTATRACKS       =   5;
constexpr double    EEGGLVIEW_MAXSPEEDMIN       = 10.0;


constexpr double    TracksAltitude3D            = 0.10;

constexpr double    CursorHeightFactor3D        = 1.5;

constexpr double    AutoLineWidthMax            = 3.0;


#define AxisColor                           (GLfloat) 0.00,   (GLfloat) 0.00,   (GLfloat) 0.00,   (GLfloat) 0.25
#define TrackColor                          (GLfloat) 0.00,   (GLfloat) 0.00,   (GLfloat) 0.00,   (GLfloat) 1.00
#define AuxColor                            (GLfloat) 0.00,   (GLfloat) 0.00,   (GLfloat) 1.00,   (GLfloat) 0.50
#define PseudotrackColor                    (GLfloat) 0.00,   (GLfloat) 0.00,   (GLfloat) 1.00,   (GLfloat) 1.00
#define HighlightColor                      (GLfloat) 1.00,   (GLfloat) 0.00,   (GLfloat) 1.00,   (GLfloat) 1.00
#define BadColor                            (GLfloat) 1.00,   (GLfloat) 0.00,   (GLfloat) 0.00,   (GLfloat) 0.50
#define MarkerColor                         (GLfloat) 0.00,   (GLfloat) 0.686,  (GLfloat) 0.00,   (GLfloat) 1.00
#define AntiCursorColor                     (GLfloat) 0.21,   (GLfloat) 1.00,   (GLfloat) 1.00,   (GLfloat) 1.00
                                        // old bright red
//#define AntiCursorFillColor               (GLfloat) 0.79,   (GLfloat) 0.00,   (GLfloat) 0.00,   (GLfloat) 1.00 
                                        // pinkish red
#define AntiCursorFillColor                 (GLfloat) 0.02,   (GLfloat) 0.12,   (GLfloat) 0.12,   (GLfloat) 1.00
                                        // interesting: turns tracks to cursor red, but filling appears light cyan
#define HintTextBackColor                   (GLfloat) 0.50,   (GLfloat) 0.50,   (GLfloat) 0.50,   (GLfloat) 0.50
#define ThresholdColor                      (GLfloat) 1.00,   (GLfloat) 0.00,   (GLfloat) 0.00,   (GLfloat) 0.80


enum    {
        EEGGLVIEW_CBG_SEP3   = NumBaseViewButtons,
        EEGGLVIEW_CBG_DISPLAYINTENSITY,
        EEGGLVIEW_CBG_NEXTROI,
        EEGGLVIEW_CBG_AVERAGEROIS,
        EEGGLVIEW_CBG_SEP3A,
        EEGGLVIEW_CBG_SHOWTAGS,
        EEGGLVIEW_CBG_PREVMARKER,
        EEGGLVIEW_CBG_NEXTMARKER,
        EEGGLVIEW_CBG_ADDMARKER,
        EEGGLVIEW_CBG_SEP3B,
//      EEGGLVIEW_CBG_SYNCZERO,
//      EEGGLVIEW_CBG_SEP3C,
        EEGGLVIEW_CBG_RNGCURS,
        EEGGLVIEW_CBG_SEP4,
        EEGGLVIEW_CBG_UP,
        EEGGLVIEW_CBG_DOWN,
        EEGGLVIEW_CBG_NEXTSESSION,
        EEGGLVIEW_CBG_SEP5,
        EEGGLVIEW_CBG_LESSTR,
        EEGGLVIEW_CBG_MORETR,
        EEGGLVIEW_CBG_LESSPSEUDOTRACKS,
        EEGGLVIEW_CBG_MOREPSEUDOTRACKS,
        EEGGLVIEW_CBG_SEP6,
        EEGGLVIEW_CBG_TRSUPER,
        EEGGLVIEW_CBG_FLIPVERT,
        EEGGLVIEW_CBG_SHOWSD,
//      EEGGLVIEW_CBG_SEP11,
//      EEGGLVIEW_CBG_STYLEW,
//      EEGGLVIEW_CBG_SPLITWINDOW,
        EEGGLVIEW_CBG_SEP7,
        EEGGLVIEW_CBG_EXTTV,
        EEGGLVIEW_CBG_COMPTV,
        EEGGLVIEW_CBG_SEP8,
        EEGGLVIEW_CBG_EXTTH,
        EEGGLVIEW_CBG_COMPTH,
        EEGGLVIEW_CBG_SEP9,
        EEGGLVIEW_CBG_VUNITS,
        EEGGLVIEW_CBG_HUNITS,
        EEGGLVIEW_CBG_SEP10,
        EEGGLVIEW_CBG_FILTER,
        EEGGLVIEW_CBG_SETAVGREF,
        EEGGLVIEW_CBG_SEP11,
        EEGGLVIEW_CBG_DECBR,
        EEGGLVIEW_CBG_INCBR,
        EEGGLVIEW_CBG_DECCR,
        EEGGLVIEW_CBG_INCCR,
        EEGGLVIEW_CBG_SEP12,
        EEGGLVIEW_CBG_FXDSCL,
        EEGGLVIEW_CBG_COLOR,
        EEGGLVIEW_CBG_NUM
        };


//----------------------------------------------------------------------------
                                        // Acceleration parameters for scrollbar
constexpr ULONG     TracksScrollBarDelayForPolling              = 1000;
constexpr ULONG     TracksScrollBarDelayBeforeAcceleration      =  100;
constexpr ULONG     TracksScrollBarDelayAfterNoAcceleration     = 3000;
constexpr double    TracksScrollBarMinSpeed                     =    1;
constexpr double    TracksScrollBarMaxSpeed                     =  100;
constexpr double    TracksScrollBarMinAcceleration              =    1;

constexpr ULONG     TracksScrollWheelDelayForPolling            =  400;
constexpr ULONG     TracksScrollWheelDelayBeforeAcceleration    =  400;
constexpr ULONG     TracksScrollWheelDelayAfterNoAcceleration   = 2000;
constexpr double    TracksScrollWheelMinSpeed                   =    1;
constexpr double    TracksScrollWheelMaxSpeed                   =  100;
constexpr double    TracksScrollWheelMinAcceleration            =    1;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr int       MaxLinkColors               = 14;
constexpr int       DashLinkColors              = 7;
constexpr int       NumUserMarkers              = 10;

                                        // Controlling how tracks are visually dispatched into sub-windows
enum    {
        LayoutOnePage,
        LayoutTwoPages,
        LayoutFourPages,
        LayoutOneTrackOneBox,
        LayoutOneTrackOneBoxXyzFlat,
        LayoutOneTrackOneBoxXyz3D,

        NumLayouts,
        NumLayoutsTooMuchTracks     = LayoutOneTrackOneBox,
        NumLayoutsNoXyz             = LayoutOneTrackOneBox + 1,
        };

                                        // Controlling how tracks are being displayed: lines, bars, colored strips
enum    DisplayTracksEnum
        {                               // main display modes
        DisplayTracks           = 0x01,
        DisplayBars             = 0x02,
        DisplayIntensitySquare  = 0x04,
        DisplayIntensitySmooth  = 0x08,
        DisplayMask             = DisplayTracks | DisplayBars | DisplayIntensitySquare | DisplayIntensitySmooth,
                                        // more specialized flags of main modes
        DisplayIntensityPrecise = 0x10,
        DisplayRoi              = 0x20,
        DisplayOptionsMask      = DisplayIntensityPrecise | DisplayRoi,
        };

                                        // Controlling if tracks as lines could be filled with color
enum    DisplayFillingEnum
        {
        FillingNone,
        FillingVoid,
        FillingColor,
        NumFillingMode
        };

                                        // Controlling the string format for horizontal scale and title
enum    TimeDisplayEnum
        {
        NoTimeDisplay,
        TimeDisplayTime,                // time scale, accounting for seconds, minutes etc...
        TimeDisplayLinear,              // linear scale

        NumTimeDisplayEnum,
        };

                                        // Controlling how the Standard Deviation is plot on top of tracks
enum    DisplaySDEnum
        {
        SDNone,
        SDDashed,
        SDFilled,
        NumSDMode
        };


constexpr int       NumScanTracksScaling        =  1000;
constexpr int       NumScanHistogram            = 10000;


//----------------------------------------------------------------------------
                                        // Class used to save/restore (most of) TTracksView's states (like scaling, display mode, superimposed etc...)
class   TTracksViewState
{
public:

    owl::TWindowAttr    Attr;
    bool                IsMinimized;
    bool                IsMaximized;

    int                 RenderingMode;
    DisplayTracksEnum   DisplayMode;

    TSelection          SelTracks;
    TSelection          BadTracks;
    TSelection          AuxTracks;
    TSelection          Highlighted;

    bool                TracksSuper;
    bool                AverageRois;
    bool                ShowVertScale;
    TimeDisplayEnum     ShowHorizScale;
    bool                ShowTags;
    bool                TracksFlipVert;
    bool                ShowBaseline;
    DisplaySDEnum       ShowSD;
    bool                ShowDate;
    bool                ShowTime;
    MarkerType          DisplayMarkerType;

    GLfloat             LineWidth;


    void                SaveState    ( TTracksView *view );
    void                RestoreState ( TTracksView *view );
};


//----------------------------------------------------------------------------
                                        // Tracks could be EEG, segmentation results, ESI results, histograms, frequencies (derived class), or any type of 2D data.
                                        // Tracks are dispatched into window slots, which could be positionned anywhere in the window,
                                        // but usually correspond to simple geometrical patterns (f.ex. half window left and half window right).
                                        // Tracks views are central to Cartool, and can pilot secondary views (see TSecondaryView) by sending them
                                        // the EEG data and the corresponding time cursor. Said data will be then displayed f.ex. as colored maps.
                                        // Tracks views are highly interactive and respond in real time to keyboard and mouse messages,
                                        // as well as messages sent by other views, allowing f.ex. to sync many of them.
class   TTracksView :   public  TBaseView,
                        public  TTracksViewWindowSlots
{
public:
                        TTracksView ( TTracksDoc &doc, owl::TWindow *parent = 0, TLinkManyDoc *group = 0, bool isderived = false );
                       ~TTracksView ();


    friend              TTracksViewScrollbar;   // these 2 classes work together
    friend              TTracksViewState;       // full access to save/restore functionalities
    TTracksViewScrollbar*   ScrollBar;


    static const char*  StaticName              ()          { return "&Tracks Display"; }
    const char*         GetViewName             ()  const   { return StaticName(); }
    bool                CanClose                ();


    TElectrodesDoc*     XYZDoc;
    int                 CurrXyz;

    TSolutionPointsDoc* SPDoc;
    int                 CurrSp;

    TRois*              Rois;
    int                 CurrRois;


    TSetArray2<float>   EegBuff;            // eeg                buffer with all electrodes up-to-date
    TTracks<float>      SdBuff;             // standard deviation buffer with all electrodes up-to-date
    TSelection          SelTracks;          // currently displayed tracks
    TSelection          BadTracks;
    TSelection          AuxTracks;
    bool                HasFilling;
    DisplayFillingEnum  IsFilling;
    TGLColoring**       Filling;
    int                 NumFilling;
    TSelection*         SelFilling;
    TGLColor<GLfloat>   LinkColors[ MaxLinkColors ];

    TTracksViewMontage  Montage;


    void                AntialiasingPaint       ();
    void                NestedPaints            ();
    void                GLPaint                 ( int how, int renderingmode, TGLClipPlane* otherclipplane );
    void                HintsPaint              ();

    virtual void        UpdateCaption               ();   // with time of cursor
    void                UpdateCaptionTracksValues   ( char* buff );
    virtual void        DrawTFCursor                ( bool undrawold, bool localhdc = true );
//  void                ZoomWindow                  ( TEegWinViewPartTracks* tocw );
    virtual void        DrawHighlighted             ( int i, bool highlight );

    void                GetHighlighted          ( TSelection *s )           const   { s->Copy ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastPseudoIndex (), &Highlighted ); }
    void                GetBadTracks            ( TSelection *s )           const   { s->Copy ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastPseudoIndex (), &BadTracks   ); }
    const TInterval*    GetCDP                  ()                          const   { return CDP; }


    virtual void        GetTracks               ( long tf1, long tf2, TArray2<float> &buff, ReferenceType ref = ReferenceUsingCurrent );

    virtual const char*         GetElectrodeName    ( int e, bool mtg = true ); // return the electrode name by priority: montage, xyz, pseudo-tracks, then original name
    virtual const TStrings*     GetElectrodesNames  ()                      const   { return XYZDoc ? XYZDoc->GetElectrodesNames () : EEGDoc->GetElectrodesNames (); }

    MarkerType          GetMarkerType           ()                          const   { return DisplayMarkerType; }
    void                SetMarkerType           ( MarkerType markertype )             { DisplayMarkerType = KeepFlags ( markertype, AllMarkerTypes ); CmSetMarkerDisplay ( 0 ); }
    DisplayTracksEnum   GetDisplayMode          ()                          const   { return DisplayMode; }
    void                SetDisplayMode          ( DisplayTracksEnum displaymode )   { DisplayMode = displaymode; Invalidate ( false ); }
    void                SetSelTracks            ( TSelection &seltracks )           { SelTracks.Copy ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastPseudoIndex (), &seltracks ); SetPartWindows ( SelTracks ); Invalidate ( false ); }
    void                SetColoring             ( int which, int from, ColoringEnum how, TGLColor<GLfloat> cmin, TGLColor<GLfloat> cmax, double minv = 0, double maxv = 0 );
    void                Set3DMode               ( int current3dset );

    const TBaseDoc*     GetGeometryDoc          ()                          const   { return XYZDoc ? XYZDoc : BaseDoc; }   // get geometrical info from XYZDoc
    TTracksDoc*         GetEEGDoc               ()                                  { return EEGDoc; }

    void                TFToString              ( long   tf, char* stf, char* stime = 0, TimeDisplayEnum horizscale = NoTimeDisplay, char* sdate = 0, bool interval = false )   const;
    void                TFToString              ( double tf, char* stf, char* stime = 0, TimeDisplayEnum horizscale = NoTimeDisplay )                                           const;
    void                CursorToTitle           ( TTFCursor *tfc, char *title ) const;


    void                EvKeyDown               ( owl::uint key, owl::uint repeatCount, owl::uint flags );
    void                EvKeyUp                 ( owl::uint key, owl::uint repeatCount, owl::uint flags );

    virtual bool        VnReloadData            ( int what );
    virtual bool        VnNewTFCursor           ( TTFCursor *tfcursor );
    virtual bool        VnNewSelection          ( TSelection *sel );
    virtual bool        VnNewHighlighted        ( TSelection *sel );
    virtual bool        VnNewBadSelection       ( TSelection *sel );
    virtual bool        VnNewAuxSelection       ( TSelection *sel );


protected:
    int                 LastVisibleTrack;   // index of last single displayed track
    int                 LastSelectedTrack;


    TTracksDoc*         EEGDoc;
    DisplayTracksEnum   DisplayMode;        // various flags on the way to paint (intensity, tracks, etc..)


    TInterval*          CDP;                // to use CDPt
    TInterval           CDPt;               // Current Displayed Page, interval TFs
    TTFCursor           TFCursor;           // current Time Frame cursor
    bool                SyncCDPtTFCursor;   // sync between display and cursor
    long                LastCursorPosMin;
    long                LastCursorPosMax;
    TSelection          Highlighted;        // user-selected tracks
    long                BuffSize;           // # of TF in buffers
    TArray1<double>     ScaleTracks;        // scaling normalization, to uniformize data of different dimensions
    double              OffsetTracks;
//  TTimer              AnimCursor;

    char                MarkerFilter[ 1024 ];   // display filter
    char                MarkerSearch[ 1024 ];
    TStringGrep         GrepMarkerFilter;
    TStringGrep         GrepMarkerSearch;
    int                 LastTagIndex;
    TStrings            MarkerStrings;
    MarkerType          DisplayMarkerType;

    double              STH;                // Scale Tracks Horizontally
    double              DisplayMinTF;
    bool                TracksSuper;        // superimpose flag
    bool                AverageRois;
    bool                ShowVertScale;
    TimeDisplayEnum     ShowHorizScale;
    TimeDisplayEnum     ShowHorizScaleLegal;
    bool                ShowTags;
    bool                TracksFlipVert;     // flip vertically the real tracks ( < auxiliary )
    bool                ShowBaseline;
    DisplaySDEnum       ShowSD;             // show Standard Deviation
    bool                ShowDate;
    bool                ShowTime;
    UseThisDC*          DrawTFCursorDC;
    TTracksViewWindowSlot*  CurrentWindowSlot;
    TAcceleration       Acceleration;
    double              GalMinValue;
    double              GalMaxValue;
    double              TextMargin;

    bool&               IntensityOverride   = TBaseView::ControlKey;    // keyboard shortcut to force intensity scaling

                                        // OpenGL
    GLfloat             LineWidth;
    TGLQuadMesh         QuadMesh;

                                        // Display mode handling
    bool                IsTracksMode            ()  const   { return IsFlag ( DisplayMode, DisplayTracks          );    }
    bool                IsBarsMode              ()  const   { return IsFlag ( DisplayMode, DisplayBars            );    }
    bool                IsIntensitySquareMode   ()  const   { return IsFlag ( DisplayMode, DisplayIntensitySquare );    }
    bool                IsIntensitySmoothMode   ()  const   { return IsFlag ( DisplayMode, DisplayIntensitySmooth );    }
    bool                IsIntensityModes        ()  const   { return IsFlag ( DisplayMode, CombineFlags ( DisplayIntensitySmooth, DisplayIntensitySquare ) );   }
                                                                                                                                                              // segments need 1 more position, too
    bool                OneMoreHoriz            ()  const   { return IsFlag ( DisplayMode, CombineFlags ( DisplayBars, DisplayIntensitySquare ) ) || IsFilling; }

    bool                IsIntensityPrecise      ()  const   { return IsFlag ( DisplayMode, DisplayIntensityPrecise );   }
    bool                IsRoiMode               ()  const   { return IsFlag ( DisplayMode, DisplayRoi              );   }

//  void                ClearDisplay            ()          { ResetFlags    ( DisplayMode, DisplayMask );                }
//  void                ClearIntensityPrecise   ()          { ResetFlags    ( DisplayMode, DisplayIntensityPrecise );    }
    void                ClearRoiMode            ()          { ResetFlags    ( DisplayMode, DisplayRoi );                 }

    void                SetTracksMode           ()          { SetFlags      ( DisplayMode, DisplayMask, DisplayTracks          );    }
    void                SetBarsMode             ()          { SetFlags      ( DisplayMode, DisplayMask, DisplayBars            );    }
    void                SetIntensitySquareMode  ()          { SetFlags      ( DisplayMode, DisplayMask, DisplayIntensitySquare );    }
    void                SetIntensitySmoothMode  ()          { SetFlags      ( DisplayMode, DisplayMask, DisplayIntensitySmooth );    }
    void                SetIntensityPrecise     ()          { SetFlags      ( DisplayMode, DisplayIntensityPrecise             );    }   // these flags are just options of the modes above, they do not reset anything by themselves
    void                SetRoiMode              ()          { SetFlags      ( DisplayMode, DisplayRoi                          );    }


    bool                ValidView                   ()                          const   { return EEGDoc && EEGDoc->GetNumTimeFrames() > 0 && EEGDoc->GetNumElectrodes() > 0; }
    virtual void        SetPartWindows              ( TSelection &sel, bool refresh = true );
    void                SetTextMargin               ();
    virtual void        UpdateBuffers               ( long oldtfmin, long oldtfmax, long newtfmin, long newtfmax );
    void                ReloadBuffers               ();
    virtual bool        IsStandDevAvail             ()                          const   { return ShowSD && EEGDoc->IsStandDevAvail (); }
    virtual void        ResetScaleTracks            ( TSelection *sel = 0 );
    double              ScalingContrastToColorTable ( double scalingcontrast )  const   { return    0.1 + 999.9 * scalingcontrast * scalingcontrast; }
    double              ColorTableToScalingContrast ( double contrast        )  const   { return    sqrt ( ( contrast - 0.1 ) / 999.9 ); }


    void                UpdateTimeRange             ( TArray2<float> &data, TSelection &goodtf );
    void                ScanTracksEnergy            ( TArray2<float> &data, TSelection &goodtf );
    void                ScanTracksTooHigh           ( TArray2<float> &data, TSelection &goodtf );
//  void                ScanTracksTooMuchVariability( TArray2<float> &data, TSelection &goodtf );
//  void                ScanTracksNoisy             (  TArray2<float> &data, TSelection &goodtf, bool lownoise );
    void                ScanTracksGfp               ( TArray2<float> &data, TSelection &goodtf );
//  void                ScanTracksSD                ( TArray2<float> &data, TSelection &goodtf );


    void                ReloadXyz               ();
    void                ReloadSp                ();
    void                ReloadRoi               ();
    void                InitXyz                 ();
    void                BestXYZBox              ( double &w, double &h );


    void                ScrollCursor            ( int desiredPoints, bool forward );
    void                ScrollDisplay           ( int numpoints );
    void                RefreshLinkedWindow     ( bool force = false ); // if it exists, used for implicit link by XE view
    void                CheckCDPtAndTFCursor    ( bool fromtfcursor );
    void                ComputeSTHfromCDPt      ()                          { STH = (double) CDPt.GetLimitLength () / CDPt.GetLength(); }
    virtual void        SetupHorizontalGrid     ( double &hgridorg, double &hgridstep );
    void                DrawTriggers            ( TTracksViewWindowSlot* toslot );
    void                RenderingModeToCurrent3DSet ();
    int                 GetMaxTracks            ()                          { return IsRoiMode () ? Rois->GetTotalSelected () : EEGDoc->GetNumElectrodes (); }


    void                SetupWindow             ();
    void                Paint                   ( owl::TDC& dc, bool erase, owl::TRect& rect );
    void                EvSize                  ( owl::uint, const owl::TSize & );
    void                EvSetFocus              ( HWND );
    void                EvKillFocus             ( HWND );
    void                GLEditCopyBitmap        ();
    void                EvLButtonDblClk         ( owl::uint, const owl::TPoint &p );
    void                EvLButtonDown           ( owl::uint, const owl::TPoint &p );
    void                EvLButtonUp             ( owl::uint, const owl::TPoint &p );
    void                EvMButtonDown           ( owl::uint, const owl::TPoint &p );
    void                EvMButtonUp             ( owl::uint, const owl::TPoint &p );
    void                EvRButtonDown           ( owl::uint, const owl::TPoint &p );
    void                EvRButtonUp             ( owl::uint, const owl::TPoint &p );
    void                EvMouseMove             ( owl::uint, const owl::TPoint &p );
    void                EvTimer                 ( owl::uint timerId );
    void                EvDropFiles             ( owl::TDropInfo drop );
    void                EvMouseWheel            ( owl::uint modKeys, int zDelta, const owl::TPoint& p );

    void                Cm2Object               ()  { TBaseView::Cm2Object   ();                }
    void                CmMagnifier             ()  { TBaseView::CmMagnifier ();                }

                                        // Handlers used to catch up messages usually going to the MDI Client, allowing to call these processing using the current view's state
    void                CmExportTracks          ();
    void                CmFreqAnalysis          ();
    void                CmInterpolateTracks     ();
    void                CmRisToVolume           ();
    void                RisToCloudVectorsUI     ();

    void                CmOrient                ();
    void                CmEditCopy              ();
    virtual void        CmZoomVert              ( owlwparam w );
    virtual void        CmZoomHorz              ( owlwparam w );
    virtual void        CmForward               ( owlwparam w );
    virtual void        CmBackward              ( owlwparam w );
    virtual void        CmPageForward           ( owlwparam w );
    virtual void        CmPageBackward          ( owlwparam w );
    virtual void        CmChangeNumTracks       ( owlwparam w );
    virtual void        CmChangeNumPseudoTracks ( owlwparam w );
    void                ShiftTracks             ( owlwparam w, int num = 1 );
    virtual void        CmShiftTracks           ( owlwparam w );
    void                CmTracksSuper           ();
    void                CmVertGrid              ();
    void                CmHorizGrid             ();
    void                CmRangeCursor           ();
    void                CmSetReference          ( owlwparam w );
    void                CmSetScaling            ( owlwparam w );
    void                CmShowTags              ();
    void                CmPreviousNextMarker    ( owlwparam w );
    void                GoToMarker              ( const TMarker* marker, bool centerextended );
    void                CmTagsEnable            ( owl::TCommandEnabler &tce );
    void                CmPrevNextTagEnable     ( owl::TCommandEnabler &tce );
    void                CmFilter                ();
    void                CmFilterEnable          ( owl::TCommandEnabler &tce );
    void                CmAddMarker             ();
    void                AddMarker               ( int predefined = 0 );
    void                AddFileMarkers          ( char *file );
    void                CmAddFileMarkers        ();
    void                CmDeleteMarker          ();
    void                CmDeleteMarkersByName   ();
    void                CmSetMarkerFile         ();
    void                CmClearMarkers          ();
    void                CmSetMarkerDisplay      ( owlwparam w );
    void                CmSetMarkerDisplayTEnable ( owl::TCommandEnabler &tce );
    void                CmSetMarkerDisplayEEnable ( owl::TCommandEnabler &tce );
    void                CmSetMarkerDisplayMEnable ( owl::TCommandEnabler &tce );
    void                CmManageMarkers         ();
    void                CmSearchMarker          ( owlwparam w );
    void                CmTriggerToMarker       ();
    void                CmSplitMarkers          ();
    void                CmRenameOverwriteMarker ();
    void                CmRenameSubstringMarkers();
    void                CmRenameRegexpMarkers   ();
    void                CmMergeOverlappingMarkers ();
    void                CmMergeContiguousMarkers();
    void                CmSetMarkerFilter       ();
    void                CmGenerateMarkers       ();
    void                CmQuickMarkers          ( owlwparam w );

    void                CmShowSD                ();
    void                CmShowSDEnable          ( owl::TCommandEnabler &tce );
    virtual void        CmSetRenderingMode      ();
    void                CmPenSize               ();
    void                CmSelection             ( owlwparam w );
    void                CmTimeRef               ( owlwparam w );
    void                CmGotoTimeOrigin        ();
    void                CmGotoTime              ();
    void                CmSetSamplingFrequency  ();
    void                CmTimeRefAbsEnable      ( owl::TCommandEnabler &tce );
    void                CmTimeRefRelEnable      ( owl::TCommandEnabler &tce );
    void                CmTimeRefRelToEnable    ( owl::TCommandEnabler &tce );
    void                CmFlipVert              ();
    void                CmBaseline              ();
//  void                CmResetFilling          ();
    void                CmVoidFilling           ();
    void                CmFillingEnable         ( owl::TCommandEnabler &tce );
    void                CmNextSession           ();
    void                CmNextSessionEnable     ( owl::TCommandEnabler &tce );
    void                CmEasyPublish           ();
    void                CmScanForTriggers       ();
    void                CmScanForBadEpochs      ();
    void                CmOrientEnable          ( owl::TCommandEnabler &tce );
    void                CmSetOffset             ( owlwparam w );
    void                CmOpenSeg               ( owlwparam w );
    void                CmSelectSegments        ( owlwparam w );
    void                CmNextSegment           ( owlwparam w );
    void                CmSegmentEnable         ( owl::TCommandEnabler &tce );
    void                CmSetTimeWindow         ();
    void                CmSelectAllTime         ();
    void                CmSetIntensityMode      ();
    void                CmSetContrast           ( owlwparam wparam );
    virtual void        CmSetScalingAdapt       ();
    void                CmNextColorTable        ();
    void                CmSetIntensityEnable    ( owl::TCommandEnabler &tce );
    void                CmSetBrightnessEnable   ( owl::TCommandEnabler &tce );
    void                CmSetContrastEnable     ( owl::TCommandEnabler &tce );
    void                CmTracksEnable          ( owl::TCommandEnabler &tce );
    void                CmShowDate              ();
    void                CmShowTime              ();
    void                CmShowDateEnable        ( owl::TCommandEnabler &tce );
    void                CmShowTimeEnable        ( owl::TCommandEnabler &tce );
    void                CmShowColorScale        ();
    void                CmShowAll               ( owlwparam w );
    bool                HasColorScale           ();
    void                CmNextRois              ();
    void                CmAverageRois           ();
    void                CmNextRoisEnable        ( owl::TCommandEnabler &tce );
    void                CmAverageRoisEnable     ( owl::TCommandEnabler &tce );
    void                CmAnalyzeTracks         ( owlwparam w );
    void                CmHistogram             ( owlwparam w );


    DECLARE_RESPONSE_TABLE (TTracksView);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
