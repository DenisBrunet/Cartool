/************************************************************************\
� 2024-2025 Denis Brunet, University of Geneva, Switzerland.

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

#include    "TFreqDoc.h"

#include    "TTracksView.h"             // TFrequenciesView is derived from TTracksView

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr double    FREQGLVIEW_EF_MINSIZECURSOR = 100;  // make dpi-aware?

                                        // handy constant to visually convert tracks' heights into an intuitively equivalent intensity
constexpr double    TrackHeightToIntensity      = 1;

                                        // bright blue
#define     AntiFreqCursorColor             (GLfloat) 1.00,   (GLfloat) 1.00,   (GLfloat) 0.33,   (GLfloat) 1.00
#define     AntiFreqCursorFillColor         (GLfloat) 0.08,   (GLfloat) 0.08,   (GLfloat) 0.03,   (GLfloat) 1.00

#define     ThirdDimensionTextColor         (GLfloat) 0.85,   (GLfloat) 0.33,   (GLfloat) 0.33,   (GLfloat) 1.00


enum    {
        FREQGLVIEW_CBG_SEP3   = NumBaseViewButtons,
        FREQGLVIEW_CBG_SPECTRUM,
        FREQGLVIEW_CBG_EF,
        FREQGLVIEW_CBG_FE,
        FREQGLVIEW_CBG_SEP3A,
        FREQGLVIEW_CBG_AVERAGETRACKS,
        FREQGLVIEW_CBG_DISPLAYINTENSITY,
        FREQGLVIEW_CBG_SEP3B,
        FREQGLVIEW_CBG_SHOWTAGS,
        FREQGLVIEW_CBG_PREVMARKER,
        FREQGLVIEW_CBG_NEXTMARKER,
        FREQGLVIEW_CBG_ADDMARKER,
        FREQGLVIEW_CBG_SEP3C,
        FREQGLVIEW_CBG_RNGCURS,
        FREQGLVIEW_CBG_SEP4,
        FREQGLVIEW_CBG_UP,
        FREQGLVIEW_CBG_DOWN,
        FREQGLVIEW_CBG_SEP5,
        FREQGLVIEW_CBG_LESSTR,
        FREQGLVIEW_CBG_MORETR,
        FREQGLVIEW_CBG_LESSPSEUDOTRACKS,
        FREQGLVIEW_CBG_MOREPSEUDOTRACKS,
        FREQGLVIEW_CBG_SEP6,
        FREQGLVIEW_CBG_TRSUPER,
        FREQGLVIEW_CBG_SEP7,
        FREQGLVIEW_CBG_PREVIOUSFREQ,
        FREQGLVIEW_CBG_NEXTFREQ,
        FREQGLVIEW_CBG_LESSFREQS,
        FREQGLVIEW_CBG_MOREFREQS,
        FREQGLVIEW_CBG_FREQNORMALIZE,
        FREQGLVIEW_CBG_SEP7B,
        FREQGLVIEW_CBG_EXTTV,
        FREQGLVIEW_CBG_COMPTV,
        FREQGLVIEW_CBG_SEP8,
        FREQGLVIEW_CBG_EXTTH,
        FREQGLVIEW_CBG_COMPTH,
        FREQGLVIEW_CBG_SEP9,
        FREQGLVIEW_CBG_VUNITS,
        FREQGLVIEW_CBG_HUNITS,
        FREQGLVIEW_CBG_SEP10,
        FREQGLVIEW_CBG_DECBR,
        FREQGLVIEW_CBG_INCBR,
        FREQGLVIEW_CBG_DECCR,
        FREQGLVIEW_CBG_INCCR,
        FREQGLVIEW_CBG_SEP11,
        FREQGLVIEW_CBG_FXDSCL,
        FREQGLVIEW_CBG_COLOR,
        FREQGLVIEW_CBG_NUM
        };

                                        // Controlling which 2 axis to pick out of the 3 of frequency contents
enum    FreqModes
        {
        FreqModeSpectrum,
        FreqModeEF,
        FreqModeFE
        };


constexpr char      HotKeyFrequencies           = 'F';
constexpr char      HotKeyTracks                = 'T';
constexpr char      HotKeyIntensity             = 'I';


//----------------------------------------------------------------------------

class   TFrequenciesView    :   public  TTracksView
{
public:
                    TFrequenciesView ( TFreqDoc& doc, owl::TWindow* parent = 0, TLinkManyDoc* group = 0 );

                                        // these 2 classes are really interrelated
    friend          TTracksViewScrollbar;

    static const char*  StaticName      ()              { return "&Frequency Display"; }
    const char*         GetViewName     ()  final       { return StaticName(); }

    void                CreateGadgets   ()  final;


    TFreqDoc*       GetFreqDoc          ()              { return FreqDoc; }
    void            UpdateCaption       ();
    void            DrawTFCursor        ( bool undrawold, bool localhdc = true )    final;
    void            GLPaint             ( int how, int renderingmode, TGLClipPlane *otherclipplane )    final;
    void            DrawHighlighted     ( int i, bool highlight )                   final;
                                        // return the electrode name by priority: montage, xyz, pseudo-tracks, then original name
    const char*         GetElectrodeName    ( int eli, bool mtg = true )            final;
    const TStrings*     GetElectrodesNames  ()                                const final;
//  virtual void    Set3DMode ();


    void            GetTracks ( long tf1, long tf2, TArray2<float> &buff, ReferenceType ref = ReferenceUsingCurrent )   final;


    void            HintsPaint  ()                                                  final;
    void            Paint       ( owl::TDC& dc, bool erase, owl::TRect& rect )      final;
    void            EvKeyDown   ( owl::uint key, owl::uint repeatCount, owl::uint flags );
    void            EvKeyUp     ( owl::uint key, owl::uint repeatCount, owl::uint flags );

    bool            VnReloadData        ( int what )                                final;
//  bool            VnNewTFCursor       ( const TTFCursor* tfcursor )               final;
//  bool            VnNewSelection      ( const TSelection *sel )                   final;
//  bool            VnNewHighlighted    ( const TSelection *hl )                    final;
    bool            VnNewBadSelection   ( const TSelection *bad )                   final;
    bool            VnNewAuxSelection   ( const TSelection *bad )                   final;
    void            EvLButtonDown       ( owl::uint, const owl::TPoint &p );
    void            EvLButtonUp         ( owl::uint, const owl::TPoint &p );
    void            EvMouseMove         ( owl::uint, const owl::TPoint &p );


protected:
    int             LastVisibleFreq;    // index of last single displayed freq
    int             LastSelectedFreq;

    TFreqDoc*       FreqDoc;

    bool            FrequenciesOverride;// special key 'F', similar use as Shift or Control keys

    FreqModes       FreqMode;
    bool            AverageMode;

    bool            FreqNormalize;
    TVector<double> ScaleFreqs;         // used to normalize the power spectrum
    bool            Show3DText;

    TInterval       CDPf;               // Current Displayed Page, in frequencies
    TInterval       SavedCDP;           // saved Current Displayed Page
    TTFCursor       FCursor;            // current Frequencies cursor

    TGLColorTable   ColorFreqs;         // add a colortable to hold frequency coloring
                                        
                                        // Frequency mode handling
    bool            IsModeSpectrum      ()  const       { return FreqMode == FreqModeSpectrum; }
    bool            IsModeEF            ()  const       { return FreqMode == FreqModeEF;       }
    bool            IsModeFE            ()  const       { return FreqMode == FreqModeFE;       }

    bool            IsModeEFAvg         ()  const       { return IsModeEF ()       &&   AverageMode && ! IsIntensityModes (); }
    bool            IsModeEFEnum        ()  const       { return IsModeEF ()       && ! AverageMode && ! IsIntensityModes (); }
    bool            IsModeFEAvg         ()  const       { return IsModeFE ()       &&   AverageMode && ! IsIntensityModes (); }
    bool            IsModeFEEnum        ()  const       { return IsModeFE ()       && ! AverageMode && ! IsIntensityModes (); }
    bool            IsModeSpectrumAvg   ()  const       { return IsModeSpectrum () &&   AverageMode                      ; }
    bool            IsModeSpectrumEnum  ()  const       { return IsModeSpectrum () && ! AverageMode                      ; }

    bool            IsTimeHorizontal    ()  const       { return FreqMode != FreqModeSpectrum; }
    bool            IsFreqHorizontal    ()  const       { return FreqMode == FreqModeSpectrum; }
    bool            IsFreqVertical      ()  const       { return FreqMode != FreqModeSpectrum; }
    bool            IsEnumerateVertical ()  const       { return ! IsModeSpectrum () && ! AverageMode; }
//  bool            Is3DimensionVertical()  const       { return FreqMode != FreqModeSpectrum && ! AverageMode && ! IsIntensityModes (); }

    bool            FreqModeEFFullTime  ( const FreqModes& fm ) const   { return fm != FreqModeSpectrum; }
    bool            FreqModeEFShortTime ( const FreqModes& fm ) const   { return fm == FreqModeSpectrum; }


    void            EvSetFocus          ( HWND );
    void            EvKillFocus         ( HWND );

    bool            ValidView           ()                                                              final   { return FreqDoc && FreqDoc->GetNumTimeFrames() > 0 && FreqDoc->GetNumElectrodes() > 0; }
    void            SetPartWindows      ( TSelection &sel, bool refresh = true )                        final;
    void            UpdateBuffers       ( long oldtfmin, long oldtfmax, long newtfmin, long newtfmax )  final;
    bool            HasStandardDeviation()  const final { return false; }
    void            ResetScaleTracks    ( const TSelection* sel = 0 )                                   final;
    void            ResetScaleFreqs     ();
//  void            BestXYZBox          ();
//  void            ScrollCursor        ( int desiredPoints, bool forward );
//  void            ScrollDisplay       ( int numpoints );
    void            RefreshLinkedWindow ( bool force = false ); // if it exists, used for implicit link by XE view
    void            SetupHorizontalGrid ( double &hgridorg, double &hgridstep )                         final;

                                        // a few internal functions to make things a bit simpler
    void            SetCDP              ();
    void            GetRelHorizRange    ( long& minp, long& maxp, long& nump ); // returns the correct horizontal range depending on mode
    void            SetColorTable       ( AtomType datatype )   final;


    void            CmSetRenderingMode      ()                  final;
    void            CmSetScalingAdapt       ();
    void            CmSetFrequencyMode      ( owlwparam w );    
    void            CmNextFrequency         ( owlwparam w );    
    void            CmZoomVert              ( owlwparam w )     final;
    void            CmZoomHorz              ( owlwparam w )     final;
    void            CmForward               ( owlwparam w )     final;
    void            CmBackward              ( owlwparam w )     final;
    void            CmShiftTracks           ( owlwparam w )     final;
    void            CmChangeNumTracks       ( owlwparam w )     final;
    void            CmChangeNumPseudoTracks ( owlwparam w )     final;
    void            CmChangeNumFreqs        ( owlwparam w );
    void            CmSetFreqNormalize      ();
    void            CmSetManageRangeCursor  ( owlwparam w );
    void            CmSetShow3DText         ();
    void            CmResetScaleFreqs       ();
    void            CmSaveScaleFreqs        ();
    void            CmOffEnable             ( owl::TCommandEnabler &tce );
    void            CmAverageEnable         ( owl::TCommandEnabler &tce );


    DECLARE_RESPONSE_TABLE (TFrequenciesView);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
