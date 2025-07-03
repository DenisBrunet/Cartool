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

#include    "TFrequenciesView.h"

#include    "MemUtil.h"

#include    "Math.Stats.h"
#include    "TArray2.h"
#include    "TArray1.h"
#include    "TInterval.h"
#include    "OpenGL.h"

#include    "TFreqDoc.h"
#include    "TExportTracks.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// Don't forget to update TSecondaryView response table also, for the buttons

DEFINE_RESPONSE_TABLE1(TFrequenciesView, TTracksView)

//    EV_WM_SIZE,
    EV_WM_SETFOCUS,
    EV_WM_KILLFOCUS,
    EV_WM_KEYDOWN,
    EV_WM_KEYUP,
    EV_WM_LBUTTONDOWN,
    EV_WM_LBUTTONUP,
    EV_WM_MOUSEMOVE,

    EV_COMMAND_AND_ID   ( IDB_FREQDISPLAY_SPECTRUM, CmSetFrequencyMode ),
    EV_COMMAND_AND_ID   ( IDB_FREQDISPLAY_EF,       CmSetFrequencyMode ),
    EV_COMMAND_AND_ID   ( IDB_FREQDISPLAY_FE,       CmSetFrequencyMode ),
    EV_COMMAND_AND_ID   ( IDB_DISPLAYINTENSITY,     CmSetFrequencyMode ),
    EV_COMMAND_AND_ID   ( IDB_AVERAGETRACKS,        CmSetFrequencyMode ),

    EV_COMMAND_AND_ID   ( IDB_ISINCBRIGHT,          CmZoomVert ),
    EV_COMMAND_AND_ID   ( IDB_ISDECBRIGHT,          CmZoomVert ),

    EV_COMMAND_AND_ID   ( IDB_NEXTFREQ,             CmNextFrequency ),
    EV_COMMAND_AND_ID   ( IDB_PREVIOUSFREQ,         CmNextFrequency ),
    EV_COMMAND_AND_ID   ( IDB_MOREFREQS,            CmChangeNumFreqs ),
    EV_COMMAND_AND_ID   ( IDB_LESSFREQS,            CmChangeNumFreqs ),
    EV_COMMAND          ( IDB_FREQNORMALIZE,        CmSetFreqNormalize ),

    EV_COMMAND          ( CM_FREQ3DTEXT,            CmSetShow3DText ),
    EV_COMMAND          ( CM_FREQRESETSCALING,      CmResetScaleFreqs ),
    EV_COMMAND          ( CM_FREQSAVESCALING,       CmSaveScaleFreqs ),
    EV_COMMAND_ENABLE   ( CM_EEGREFMONTAGE,         CmOffEnable ),
    EV_COMMAND_ENABLE   ( CM_EEGREFAVG,             CmOffEnable ),
    EV_COMMAND_ENABLE   ( CM_EEGREFELECSINGLE,      CmOffEnable ),
    EV_COMMAND_ENABLE   ( CM_EEGREFELECMULTIPLE,    CmOffEnable ),
    EV_COMMAND_ENABLE   ( CM_EEGREFNONE,            CmOffEnable ),
    EV_COMMAND_ENABLE   ( CM_EEGSETOFFSET,          CmOffEnable ),
    EV_COMMAND_ENABLE   ( CM_EEGRESETOFFSET,        CmOffEnable ),
    EV_COMMAND_ENABLE   ( IDB_AVERAGETRACKS,        CmAverageEnable ),
    EV_COMMAND_AND_ID   ( IDB_LESSTRACKS,           CmChangeNumTracks ),
    EV_COMMAND_AND_ID   ( IDB_MORETRACKS,           CmChangeNumTracks ),
END_RESPONSE_TABLE;


        TFrequenciesView::TFrequenciesView ( TFreqDoc& doc, TWindow* parent, TLinkManyDoc* group )
      : TTracksView ( doc, parent, group, true ), FreqDoc ( &doc )
{

SelSize             = FreqDoc->GetTotalElectrodes ();


                                        // OpenGL
CurrentDisplaySpace = DisplaySpaceNone;
LineWidth           = 0;                // set to smart width
Zoom                = 0.9;

                                        // exit if a bad document is provided (cancel)
if ( ! ValidView() ) {
    NotOK ();                           // skip allocation stuff
    return;
    }

                                        // get max size for display
BuffSize            = NoMore ( FreqDoc->GetNumTimeFrames (), (long) FreqMaxPointsDisplay );

CDPt                = TInterval ( 0, FreqDoc->GetNumTimeFrames () - 1, BuffSize );

                                        // now set CDP for frequencies
CDPf                = TInterval ( 0, FreqDoc->GetNumFrequencies () - 1 );
CDPf.SetMinMax ( CDPf.GetLimitMin (), CDPf.GetLimitMax () );

                                        // showing all frequencies upon creation
FCursor             = TTFCursor ( FreqDoc, CDPf.GetLimitMin (), CDPf.GetLimitMax (), CDPf.GetLimitMin (), CDPf.GetLimitMax () );
FCursor.SentTo      = 0;
FCursor.SentFrom    = 0;

                                        // this comes after the EEG choices, it's enough to turn off the dissimilarity
//if ( FreqDoc->HasPseudoElectrodes () )
//    SelTracks.Reset ( EEGDoc->GetDisIndex () );
FreqDoc->ClearPseudo ( SelTracks );

                                        // allocate a "2D" buffer, duplicating a normal Eeg #freqs times
EegBuff   .Resize ( FreqDoc->GetNumFrequencies (), FreqDoc->GetTotalElectrodes (), BuffSize );

ScaleFreqs.Resize ( FreqDoc->GetNumFrequencies () + 1 );
ScaleFreqs.Index1.IndexMin      = StringToDouble ( FreqDoc->GetFrequencyName ( 0 ) );
ScaleFreqs.Index1.IndexRatio    = 1 / NonNull ( StringToDouble ( FreqDoc->GetFrequencyName ( 1 ) ) - StringToDouble ( FreqDoc->GetFrequencyName ( 0 ) ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Show3DText              = true;

LastVisibleFreq         = 0;
LastSelectedFreq        = -1;

                                        // set defaults init
FreqNormalize           = false;
SyncCDPtTFCursor        = false;


FrequenciesOverride     = false;
                                        // calibrate frequency normalization
FreqMode                = FreqModeSpectrum;
//DisplayMode             = DisplayTracks;
AverageMode             = false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SetCDP ();
CDPt.SetMinLength ( 0, CDPt.GetLimitLength(), true );

ReloadBuffers   ();
ResetScaleFreqs ();                     // use the beginning of the file to calibrate the frequency scaling


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // calibrate tracks scaling
FreqMode                = FreqModeEF;
//DisplayMode             = DisplayTracks;
AverageMode             = false;

                                        // display doesn't look nice when there is no time
if ( FreqDoc->GetNumTimeFrames () == 1 ) {
    FreqMode            = FreqModeSpectrum;
    SetTracksMode ();
    TracksSuper         = true;
    }


SetCDP ();
CDPt.SetLength ( CDPt.GetLimitLength(), TFCursor.GetPosMin(), true );
ComputeSTHfromCDPt ();

ReloadBuffers    ();
ResetScaleTracks ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

RenderingMode           = LayoutOnePage;

                                        // signed: FreqDoc->IsFreqTypeFFTApproximation () || FreqDoc->GetFreqType () == FreqUnknown
SetColorTable ( FreqDoc->GetAtomType ( AtomTypeUseCurrent ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // re-use EEG menu, as it is mainly the same, thus avoiding tedious updates in both
SetViewMenu ( new TMenuDescr (IDM_EEG) );
                                        // then add my local stuff
GetViewMenu ()->InsertMenu ( CM_SHOWALL,        MF_BYCOMMAND, CM_FREQ3DTEXT,        "Show/hide last di&mension info" );
GetViewMenu ()->InsertMenu ( CM_EEGSETOFFSET,   MF_BYCOMMAND, CM_FREQRESETSCALING,  "Reset frequency &normalization" );
GetViewMenu ()->InsertMenu ( CM_EEGSETOFFSET,   MF_BYCOMMAND, CM_FREQSAVESCALING,   "Saving &frequency normalization" );
}


//----------------------------------------------------------------------------
void    TFrequenciesView::CreateGadgets ()
{
NumControlBarGadgets    = FREQGLVIEW_CBG_NUM;
ControlBarGadgets       = new TGadget * [ NumControlBarGadgets ];

CreateBaseGadgets ();

ControlBarGadgets[ FREQGLVIEW_CBG_SEP3              ]   = new TSeparatorGadget ( DefaultSeparator );
                                                                               
ControlBarGadgets[ FREQGLVIEW_CBG_SPECTRUM          ]   = new TButtonGadgetDpi ( IDB_FREQDISPLAY_SPECTRUM,   IDB_FREQDISPLAY_SPECTRUM,   TButtonGadget::Exclusive, true, IsModeSpectrum ()  ? TButtonGadget::Down : TButtonGadget::Up );
ControlBarGadgets[ FREQGLVIEW_CBG_EF                ]   = new TButtonGadgetDpi ( IDB_FREQDISPLAY_EF,         IDB_FREQDISPLAY_EF,         TButtonGadget::Exclusive, true, IsModeEF       ()  ? TButtonGadget::Down : TButtonGadget::Up );
ControlBarGadgets[ FREQGLVIEW_CBG_FE                ]   = new TButtonGadgetDpi ( IDB_FREQDISPLAY_FE,         IDB_FREQDISPLAY_FE,         TButtonGadget::Exclusive, true, IsModeFE       ()  ? TButtonGadget::Down : TButtonGadget::Up );
                                                                               
ControlBarGadgets[ FREQGLVIEW_CBG_SEP3A             ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ FREQGLVIEW_CBG_AVERAGETRACKS     ]   = new TButtonGadgetDpi ( IDB_AVERAGETRACKS,          IDB_AVERAGETRACKS,          TButtonGadget::NonExclusive, true, AverageMode     ? TButtonGadget::Down : TButtonGadget::Up, false );
ControlBarGadgets[ FREQGLVIEW_CBG_DISPLAYINTENSITY  ]   = new TButtonGadgetDpi ( IDB_DISPLAYINTENSITY,       IDB_DISPLAYINTENSITY,       TButtonGadget::Command );
                                                                                 
ControlBarGadgets[ FREQGLVIEW_CBG_SEP3B             ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ FREQGLVIEW_CBG_SHOWTAGS          ]   = new TButtonGadgetDpi ( IDB_SHOWMARKERS,            IDB_SHOWMARKERS,            TButtonGadget::NonExclusive, true, ShowTags ? TButtonGadget::Down : TButtonGadget::Up, false );
ControlBarGadgets[ FREQGLVIEW_CBG_PREVMARKER        ]   = new TButtonGadgetDpi ( IDB_PREVMARKER,             IDB_PREVMARKER,             TButtonGadget::Command);
ControlBarGadgets[ FREQGLVIEW_CBG_NEXTMARKER        ]   = new TButtonGadgetDpi ( IDB_NEXTMARKER,             IDB_NEXTMARKER,             TButtonGadget::Command);
ControlBarGadgets[ FREQGLVIEW_CBG_ADDMARKER         ]   = new TButtonGadgetDpi ( IDB_ADDMARKER,              IDB_ADDMARKER,              TButtonGadget::Command);
                                                                               
ControlBarGadgets[ FREQGLVIEW_CBG_SEP3C             ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ FREQGLVIEW_CBG_RNGCURS           ]   = new TButtonGadgetDpi ( IDB_RANGECURSOR,            IDB_RANGECURSOR,            TButtonGadget::NonExclusive);
                                                                                 
ControlBarGadgets[ FREQGLVIEW_CBG_SEP4              ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ FREQGLVIEW_CBG_UP                ]   = new TButtonGadgetDpi ( IDB_UP,                     IDB_UP,                     TButtonGadget::Command);
ControlBarGadgets[ FREQGLVIEW_CBG_DOWN              ]   = new TButtonGadgetDpi ( IDB_DOWN,                   IDB_DOWN,                   TButtonGadget::Command);
                                                                                 
ControlBarGadgets[ FREQGLVIEW_CBG_SEP5              ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ FREQGLVIEW_CBG_LESSTR            ]   = new TButtonGadgetDpi ( IDB_LESSTRACKS,             IDB_LESSTRACKS,             TButtonGadget::Command);
ControlBarGadgets[ FREQGLVIEW_CBG_MORETR            ]   = new TButtonGadgetDpi ( IDB_MORETRACKS,             IDB_MORETRACKS,             TButtonGadget::Command);
ControlBarGadgets[ FREQGLVIEW_CBG_LESSPSEUDOTRACKS  ]   = new TButtonGadgetDpi ( IDB_LESSPSEUDOTRACKS,       IDB_LESSPSEUDOTRACKS,       TButtonGadget::Command);
ControlBarGadgets[ FREQGLVIEW_CBG_MOREPSEUDOTRACKS  ]   = new TButtonGadgetDpi ( IDB_MOREPSEUDOTRACKS,       IDB_MOREPSEUDOTRACKS,       TButtonGadget::Command);
                                                                                 
ControlBarGadgets[ FREQGLVIEW_CBG_SEP6              ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ FREQGLVIEW_CBG_TRSUPER           ]   = new TButtonGadgetDpi ( IDB_TRACKSSUPER,            IDB_TRACKSSUPER,            TButtonGadget::NonExclusive);
                                                                                 
ControlBarGadgets[ FREQGLVIEW_CBG_SEP7              ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ FREQGLVIEW_CBG_PREVIOUSFREQ      ]   = new TButtonGadgetDpi ( IDB_PREVIOUSFREQ,           IDB_PREVIOUSFREQ,           TButtonGadget::Command);
ControlBarGadgets[ FREQGLVIEW_CBG_NEXTFREQ          ]   = new TButtonGadgetDpi ( IDB_NEXTFREQ,               IDB_NEXTFREQ,               TButtonGadget::Command);
ControlBarGadgets[ FREQGLVIEW_CBG_LESSFREQS         ]   = new TButtonGadgetDpi ( IDB_LESSFREQS,              IDB_LESSFREQS,              TButtonGadget::Command);
ControlBarGadgets[ FREQGLVIEW_CBG_MOREFREQS         ]   = new TButtonGadgetDpi ( IDB_MOREFREQS,              IDB_MOREFREQS,              TButtonGadget::Command);
ControlBarGadgets[ FREQGLVIEW_CBG_FREQNORMALIZE     ]   = new TButtonGadgetDpi ( IDB_FREQNORMALIZE,          IDB_FREQNORMALIZE,          TButtonGadget::NonExclusive, true, FreqNormalize ? TButtonGadget::Down : TButtonGadget::Up );
                                                                                 
ControlBarGadgets[ FREQGLVIEW_CBG_SEP7B             ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ FREQGLVIEW_CBG_EXTTV             ]   = new TButtonGadgetDpi ( IDB_EXTTRACKSV,             IDB_EXTTRACKSV,             TButtonGadget::Command);
ControlBarGadgets[ FREQGLVIEW_CBG_COMPTV            ]   = new TButtonGadgetDpi ( IDB_COMPTRACKSV,            IDB_COMPTRACKSV,            TButtonGadget::Command);
                                                                                 
ControlBarGadgets[ FREQGLVIEW_CBG_SEP8              ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ FREQGLVIEW_CBG_EXTTH             ]   = new TButtonGadgetDpi ( IDB_EXTTRACKSH,             IDB_EXTTRACKSH,             TButtonGadget::Command );
ControlBarGadgets[ FREQGLVIEW_CBG_COMPTH            ]   = new TButtonGadgetDpi ( IDB_COMPTRACKSH,            IDB_COMPTRACKSH,            TButtonGadget::Command );
                                                                                 
ControlBarGadgets[ FREQGLVIEW_CBG_SEP9              ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ FREQGLVIEW_CBG_VUNITS            ]   = new TButtonGadgetDpi ( IDB_VERTUNITS,              IDB_VERTUNITS,              TButtonGadget::NonExclusive);
ControlBarGadgets[ FREQGLVIEW_CBG_HUNITS            ]   = new TButtonGadgetDpi ( IDB_HORIZUNITS,             IDB_HORIZUNITS,             TButtonGadget::NonExclusive);
                                                                                 
ControlBarGadgets[ FREQGLVIEW_CBG_SEP10             ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ FREQGLVIEW_CBG_DECBR             ]   = new TButtonGadgetDpi ( IDB_ISDECBRIGHT,            IDB_ISDECBRIGHT,            TButtonGadget::Command);
ControlBarGadgets[ FREQGLVIEW_CBG_INCBR             ]   = new TButtonGadgetDpi ( IDB_ISINCBRIGHT,            IDB_ISINCBRIGHT,            TButtonGadget::Command);
ControlBarGadgets[ FREQGLVIEW_CBG_DECCR             ]   = new TButtonGadgetDpi ( IDB_ISDECCONTRAST,          IDB_ISDECCONTRAST,          TButtonGadget::Command);
ControlBarGadgets[ FREQGLVIEW_CBG_INCCR             ]   = new TButtonGadgetDpi ( IDB_ISINCCONTRAST,          IDB_ISINCCONTRAST,          TButtonGadget::Command);
                                                                                 
ControlBarGadgets[ FREQGLVIEW_CBG_SEP11             ]   = new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ FREQGLVIEW_CBG_FXDSCL            ]   = new TButtonGadgetDpi ( IDB_FIXEDSCALE,             IDB_FIXEDSCALE,             TButtonGadget::NonExclusive, true, ScalingAuto ? TButtonGadget::Down : TButtonGadget::Up );
ControlBarGadgets[ FREQGLVIEW_CBG_COLOR             ]   = new TButtonGadgetDpi ( IDB_SPCOLOR,                IDB_SPCOLOR,                TButtonGadget::Command);
}


//----------------------------------------------------------------------------
void    TFrequenciesView::EvSetFocus ( HWND hwnd )
{
                                        // in case of doc closing with multiple views, prevent all the other views from inheriting the focus, too
if ( ! FreqDoc->IsOpen () )
    return;


TTracksView::EvSetFocus ( hwnd );
                                        // update keys that might have changed between focus switches
                                        // VkKey is a real-time polling
FrequenciesOverride     = VkKey ( HotKeyFrequencies );
}


void    TFrequenciesView::EvKillFocus ( HWND hwnd )
{
FCursor.StopExtending ();

FrequenciesOverride     = false;

TTracksView::EvKillFocus ( hwnd );
}


//----------------------------------------------------------------------------
const char* TFrequenciesView::GetElectrodeName     (   int     eli,    bool    mtg )
{
return  Montage && mtg && eli < FreqDoc->GetNumRegularElectrodes () ?   Montage[ eli ].Name 
      : XYZDoc         && eli < FreqDoc->GetNumRegularElectrodes () ?   XYZDoc ->GetElectrodeName ( eli ) 
      :                                                                 FreqDoc->GetElectrodeName ( eli );
}


const TStrings*     TFrequenciesView::GetElectrodesNames  ()   const
{
return  XYZDoc ? XYZDoc->GetElectrodesNames () : FreqDoc->GetElectrodesNames (); 
}


//----------------------------------------------------------------------------
void    TFrequenciesView::UpdateCaption ()
{
char                buff[ MaxViewTitleLength ];

                                        // Doc + Cursor
CursorToTitle ( &TFCursor, Title );

                                        // + Frequencies
if ( FCursor.IsSplitted () )
    StringAppend ( Title, "  Freq=", FreqDoc->GetFrequencyName ( FCursor.GetPosMin() ), "-", FreqDoc->GetFrequencyName ( FCursor.GetPosMax() ) );
else
    StringAppend ( Title, "  Freq=", FreqDoc->GetFrequencyName ( FCursor.GetPosMin() ) );

                                        // + Values
if ( IsModeEFEnum () && !FreqNormalize && FCursor.GetLength() == 1 ) {
    UpdateCaptionTracksValues ( buff );

    StringAppend ( Title, "  ", buff );
    }

                                        // + Using
UpdateCaptionUsing ( buff );

if ( StringIsNotEmpty ( buff ) ) {
    StringAppend ( Title, " " );
                                        // Using can be rather long, so check the length
    StringCopy ( StringEnd ( Title ), buff, MaxViewTitleLength - StringLength ( Title ) - 1 );
    }


GetParentO()->SetCaption ( Title );

ScrollBar->UpdateIt();
}

                            // either highlights or draws to normal this track
void    TFrequenciesView::DrawHighlighted ( int /*i*/, bool /*highlight*/ )
{
//for ( int pw=0; pw < numPartWindows; pw++ )
//    listPartWindows[pw]->DrawHighlighted ( i, highlight );
}


void    TFrequenciesView::SetPartWindows ( TSelection &sel, bool refresh )
{
#define     MarginVariables \
bool                        enoughw     = HasWindowSlots () && CurrentDisplaySpace == DisplaySpaceNone && ( WindowSlots[0].ToRight.Norm() >= TextMargin + 100 /*80*/ );\
bool                        drawmargin  = enoughw;\
double                      margin      = 50.0 * drawmargin;

//MarginVariables;

int                 numtracks       = FreqDoc->GetNumSelectedRegular ( sel );
int                 numpstracks     = FreqDoc->GetNumSelectedPseudo  ( sel );
int                 numtf;
int                 pw;
//int               s;
//TSelection        swp ( FreqDoc->GetTotalElectrodes(), OrderSorted );
double              llx, lly, llz;
double              torx, tory, torz;
double              toux, touy, touz;

                                        // subdivide with scrollbar's space removed
double              W               = GetClientRect().Width() - ( HasColorScale () ? ColorScaleWidth : 0 );
double              H               = GetClientRect().Height() - EegScrollbarHeight;
double              X               = GetClientRect().Left();
double              Y               = GetClientRect().Top()    + EegScrollbarHeight;


//numtf       = CDPt.GetLength();
numtf       = TFCursor.GetLength();     // cursor can be 1 TF, not CDPt...

                                        // fixed depth
llz  = -0.5;
tory = torz = 0;
toux = touz = 0;

                                        // no tracks -> no slots
if ( numpstracks == 0 && numtracks == 0 ) {
    DeallocateWindowSlots ();
    }

                                        // special case, horizontal blocks of windows
else if ( IsModeSpectrumEnum () && numtf > 1 ) {

    MaxRegularTracksPerSlot = 1;
    MaxPseudoTracksPerSlot  = 1;

//    X += margin;
//    W -= margin;

    if ( numtracks && numpstracks ) {

        AllocateWindowSlots ( 2 * numtf );

        for ( pw = 0; pw < numtf; pw++ ) {

            WindowSlots[ pw ].SelTracks = sel;
            FreqDoc->ClearPseudo ( WindowSlots[ pw ].SelTracks );
            WindowSlots[ pw ].RegularTracks             = true;

            llx  = X + (double) pw / numtf                * W;
            lly  = Y + ( 1 - RegularTracksVerticalRatio ) * H;
            torx = 1.0 / numtf                            * W;
            touy =           RegularTracksVerticalRatio   * H;
            WindowSlots[ pw ].SetFrame ( llx, lly, llz, torx, tory, torz, toux, touy, touz );


            WindowSlots[ numtf + pw ].SelTracks = sel;
            FreqDoc->ClearRegular ( WindowSlots[ numtf + pw ].SelTracks );
            WindowSlots[ numtf + pw ].RegularTracks     = false;

            llx  = X + (double) pw / numtf              * W;
            lly  = Y;
            torx = 1.0 / numtf                          * W;
            touy = ( 1 - RegularTracksVerticalRatio )   * H;
            WindowSlots[ numtf + pw ].SetFrame ( llx, lly, llz, torx, tory, torz, toux, touy, touz );

                                        // store the time index (relative to 0, for buffer use)
            WindowSlots[ pw ].UserData = WindowSlots[ numtf + pw ].UserData = pw;

            MaxRegularTracksPerSlot         = max ( MaxRegularTracksPerSlot, (int) WindowSlots[ pw         ].SelTracks );
            MaxPseudoTracksPerSlot          = max ( MaxPseudoTracksPerSlot,  (int) WindowSlots[ numtf + pw ].SelTracks );
            }
        }
    else {

        AllocateWindowSlots ( numtf );

        for ( pw = 0; pw < numtf; pw++ ) {
            WindowSlots[ pw ].SelTracks = sel;
            llx  = X + (double) pw / numtf              * W;
            lly  = Y;
            torx = 1.0 / numtf                          * W;
            touy = 1.0                                  * H;
            WindowSlots[ pw ].SetFrame ( llx, lly, llz, torx, tory, torz, toux, touy, touz );
            WindowSlots[ pw ].RegularTracks             = true;

                                        // store the time position
            WindowSlots[ pw ].UserData = pw;

            MaxRegularTracksPerSlot         = max ( MaxRegularTracksPerSlot, (int) WindowSlots[ pw ].SelTracks );
            }
        }
    }

else {
                                        // A watered-down version from tracks
    if ( IsModeFE () ) {
                                        // clear-up pseudo?
//      FreqDoc->ClearPseudo ( sel );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        int                 numfreqs        = FCursor.GetLength ();

                                                // do a local copy of required layout mode
        int                 renderingmode   = RenderingMode;

                                                // do some checking, and downgrade rendering mode if needed
        if ( renderingmode == LayoutFourPages && numfreqs < 4 )     renderingmode = LayoutTwoPages;
        if ( renderingmode == LayoutTwoPages  && numfreqs < 2 )     renderingmode = LayoutOnePage;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                                // get # of slots for regular
        int                 numregslots     = renderingmode == LayoutFourPages ? 4
                                            : renderingmode == LayoutTwoPages  ? 2
                                            : renderingmode == LayoutOnePage   ? 1
                                            :                                    numfreqs;

        if ( numfreqs == 0 )                    // reset
            numregslots = 0;


        int                 numpseudoslots  = 0;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                                // no slots?
        if ( numregslots + numpseudoslots == 0 ) {
            DeallocateWindowSlots ();
            goto FinishedSetPartWindows;
            }

                                                // adjust to the needed number of slots
        AllocateWindowSlots ( numregslots );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        MaxRegularTracksPerSlot = 1;
        MaxPseudoTracksPerSlot  = 1;

        int             s;

        for ( s = 0; s < numregslots; s++ ) {
                                        // every and each frequency slot contains all the tracks
            WindowSlots[ s ].SelTracks  = sel;
//          FreqDoc->ClearPseudo  ( WindowSlots[ s ].SelTracks );

                                        // allocate and store frequencies
            WindowSlots[ s ].SelFreqs   = TSelection ( FCursor.GetLimitLength (), OrderArbitrary );


            int     minf    = s == 0 ? FCursor.GetPosMin () : WindowSlots[ s - 1 ].SelFreqs.FirstSelected () + 1;
            int     maxf    =          FCursor.GetPosMin () + Truncate ( (double) ( s + 1 ) * ( numfreqs - 1 ) / numregslots );

                                        // reverse selection order
            for ( int fi = maxf; fi >= minf; fi-- )
                WindowSlots[ s ].SelFreqs.Set ( fi );


            WindowSlots[ s ].RegularTracks  = true;
            WindowSlots[ s ].Color.Set ( TrackColor );
            ClearString ( WindowSlots[ s ].Name );

            MaxRegularTracksPerSlot         = max ( MaxRegularTracksPerSlot, (int) WindowSlots[ s ].SelTracks );
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1 track in 1 box
        if ( renderingmode == LayoutOneTrackOneBox
          || renderingmode == LayoutOneTrackOneBoxXyzFlat
          || renderingmode == LayoutOneTrackOneBoxXyz3D   ) {


            int     numslots    = numregslots + numpseudoslots;


            int     byx, byy;
            FitItemsInWindow ( numslots, TSize ( 100, 50 ), byx, byy );
//            FitItemsInWindow ( numslots, TSize ( CDPt.GetLength (), (int) sel ), byx, byy );

            for ( s = 0; s < numslots; s++ ) {
                torx    = W / byx;
                touy    = H / byy;
                llx     = X + W *       (double)         ( s % byx )   / byx;
                lly     = Y + H * ( 1 - (double) ( (int) ( s / byx ) ) / byy ) - touy;

                WindowSlots[ numslots - 1 - s ].SetFrame ( llx, lly, llz, torx, tory, torz, toux, touy, touz );
                }
            }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        else if ( renderingmode == LayoutFourPages
               || renderingmode == LayoutTwoPages
               || renderingmode == LayoutOnePage   ) {

                                            // heights for regulars and pseudos
            double      hreg        = numpseudoslots ? RegularTracksVerticalRatio : 1;


            if ( numfreqs ) {

                for ( s = 0; s < numregslots; s++ ) {
                    llx     = WindowSlots[ s ].RegLowLeftX ( X, W, s, numregslots       );
                    lly     = WindowSlots[ s ].RegLowLeftY ( Y, H,                hreg  );
                    torx    = WindowSlots[ s ].RegToRightX (    W,    numregslots       );
                    touy    = WindowSlots[ s ].RegToUpY    (    H, 1,             hreg  );

                    WindowSlots[ numregslots - 1 - s ].SetFrame ( llx, lly, llz, torx, tory, torz, toux, touy, touz );
                    }
                }

            } // else layout "a la page"

        } // if IsModeFE

    else

        TTracksView::SetPartWindows ( sel, false );
    }


FinishedSetPartWindows:

if ( refresh ) {

    ShowNow();
//  UpdateCaption ();

    RefreshLinkedWindow () ;
    }
}

/*
void    TFrequenciesView::ZoomWindow ( TEegWinViewPartTracks *tocw )
{
tocw->GetSelectedTracks ( SelTracks );  // specifies which local track to display
SetPartWindows ( SelTracks );
}
*/


void    TFrequenciesView::RefreshLinkedWindow ( bool force )
{
                                        // right now, it concerns only the 3D view, linked by the XE window
if ( CurrentDisplaySpace == DisplaySpaceNone && !force )
    return;

BaseDoc->NotifyDocViews ( vnViewUpdated, (TParam2) this, this );
}


//----------------------------------------------------------------------------
                                        // horizontal range of buffer for display
void    TFrequenciesView::GetRelHorizRange ( long& minp, long& maxp, long& nump )
{
//minp        = 0;
//maxp        = CDP->GetLength() - 1;

                                        // don't trust CDP!
/*
if      ( IsModeET () ) {               // classic view
    minp        = 0;
    maxp        = CDPt.GetLength() - 1;
    }
else if ( IsModeEF () ) {
    minp        = 0;

    if      ( IsFreqHorizontal () )
        maxp        = CDPf.GetLimitMax ();
    else // if ( FreqModeEFFullTime ( FreqMode ) )
        maxp        = CDPt.GetLength () - 1;
    }
else if ( IsModeFT () ) {
    minp        = 0;
    maxp        = CDPt.GetLength() - 1;
    }
*/

if      ( IsTimeHorizontal () ) {
    minp        = 0;
    maxp        = CDPt.GetLength() - 1;
    }
else if ( IsFreqHorizontal () ) {
    minp        = 0;
    maxp        = CDPf.GetLimitMax ();
    }


nump        = maxp - minp + 1;
}

                                        // set the max limits of display range, according to mode
void    TFrequenciesView::SetCDP ()
{
/*
if      ( IsModeET () )
    CDP = &CDPt;
else if ( IsModeEF () ) {
    if      ( IsFreqHorizontal () )
        CDP = &CDPf;
    else // if ( FreqMode == FreqModeEFiT || FreqMode == FreqModeEFvT )
        CDP = &CDPt;
    }
else if ( IsModeFT () )
    CDP = &CDPt;
*/

if      ( IsTimeHorizontal () )     CDP = &CDPt;
else if ( IsFreqHorizontal () )     CDP = &CDPf;

//ScrollBar->UpdateIt();
}


void    TFrequenciesView::SetupHorizontalGrid ( double &hgridorg, double &hgridstep )
{
if      ( IsTimeHorizontal () )                 // as in EEG

    TTracksView::SetupHorizontalGrid ( hgridorg, hgridstep );

else if ( IsFreqHorizontal () && FreqDoc->GetNumFrequencies () >= 3 ) {

                                        // we need some conversion from frequencies to index and vice-versa
    double              firstfreq       = StringToDouble ( FreqDoc->GetFrequencyName ( 0 ) );

    double              stepfreq        = StringToDouble ( FreqDoc->GetFrequencyName ( 1                                 ) ) 
                                        - StringToDouble ( FreqDoc->GetFrequencyName ( 0                                 ) );
    double              stepfreqend     = StringToDouble ( FreqDoc->GetFrequencyName ( FreqDoc->GetNumFrequencies () - 1 ) )
                                        - StringToDouble ( FreqDoc->GetFrequencyName ( FreqDoc->GetNumFrequencies () - 2 ) );
    bool                islinear        = stepfreq == stepfreqend;

    if ( ! islinear || stepfreq < 1e-6 )
        stepfreq        = 1;


                                        // first estimate to the step
    hgridstep   = 13                                                // some constant to have enough space
                * StringLength ( FreqDoc->GetFrequencyName ( 0 ) )  // space is proportional to the string size, as frequency names could be "10Hz" or "035.25Hz", which of course matters
                * SFont->GetAvgWidth ()                             // depends on the font being used, of course
                * stepfreq                                          // we can take more space for fractional steps
                * CDPf.GetLength() / WindowSlots[0].ToRight.Norm()  // some ratio number of frequencies displayed / horizonzal space
                + MinLogValue;                                      // safety for the log

                                        // Using various log divisions, then picking the one that seems the fittest for the current spacing
                                        // The  "-constant"  at the end of the formula helps tuning the appearance of that current division: higher cst means more relative appearance
    double              hgs02           = Power (  2, Truncate ( Log2    ( hgridstep    ) - 1.80 ) );
    double              hgs05           = Power (  5, Truncate ( LogBase ( hgridstep, 5 ) - 0.40 ) );
    double              hgs10           = Power ( 10, Truncate ( Log10   ( hgridstep    )        ) );
    double              hgs             = min ( hgs02, hgs05, hgs10 );  // choosing between the 3 different scaling

                                        // convert to index space
    hgridstep           = AtLeast ( 1, Round ( hgs / stepfreq ) );

                                        // convert to index space
    firstfreq      /= stepfreq;
                                        // is the first frequency a multiple of hgridstep?
    if ( Fraction ( firstfreq / hgridstep ) < 1e-6 )

        hgridorg        = 0;

    else {
                                        // getting the next multiple of hgridsetp above firstfreq
        hgridorg        = TruncateTo ( firstfreq + hgridstep /*- 1e-6*/, hgridstep ) - firstfreq;

                                        // resetting?
        if ( hgridorg == FreqDoc->GetNumFrequencies () - 1 ) { hgridorg = FreqDoc->GetNumFrequencies (); hgridstep = 1; } // hgridorg   = 0;
        }

    } // IsFreqHorizontal
else {                                  // shouldn't occur..
    hgridorg    = 0;
    hgridstep   = 1;
    }
}


void    TFrequenciesView::SetColorTable ( AtomType datatype )
{
                                        // a bit different than default(?)
if ( IsScalar ( datatype ) ) {
                                        // disable all tables
    ColorTable.SetTableAllowed ( NoTables );

    ColorTable.SetTableAllowed ( SignedColorTable_BlueWhiteRed );
    ColorTable.SetTableAllowed ( SignedColorTable_MagentaBlueCyanGrayGreenYellowRed );

    if ( ColorTableIndex[ datatype ] == UnknownTable )
        ColorTableIndex[ datatype ] = SignedColorTable_BlueWhiteRed;

    ColorTable.Set ( ColorTableIndex[ datatype ] );
    }
else
    TBaseView::SetColorTable ( datatype );

                                        // color scheme for frequency tracks (spectrum)
ColorFreqs.SetTableAllowed  ( NoTables );
ColorFreqs.SetTableAllowed  ( AbsColorTable_DarkRedYellowGreenCyanBlueMagenta );
ColorFreqs.Set              ( AbsColorTable_DarkRedYellowGreenCyanBlueMagenta );
ColorFreqs.SetParameters    ( CDPf.GetLimitMax () ? CDPf.GetLimitMax () : 1, CDPf.GetLimitMin (), 1.0, 1.0 ); // AlphaLinear );
}


//----------------------------------------------------------------------------

void    TFrequenciesView::GLPaint ( int how, int renderingmode, TGLClipPlane *otherclipplane )
{
if ( ! HasWindowSlots () )
    return;

                                        // this mode is not legal
if ( CurrentDisplaySpace == DisplaySpace3D && ( IsModeSpectrumEnum () /*&& numtf > 1*/ ) ) {
    glColor4f ( TrackColor );
    BFont->Print ( 0, 0, 0, "Not a legal combination of Rendering and Frequency Mode", TA_CENTER | TA_CENTERY );
    return;
    }

                                        // check all parameters
if ( renderingmode == RenderingUnchanged )
    renderingmode = RenderingMode;      // use local rendering mode



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TGLClipPlane*       clipplane;

                                        // take clipping from parameters, if both caller and owner share some clipping
if ( ( (bool) otherclipplane[ 0 ] || (bool) otherclipplane[ 1 ] || (bool) otherclipplane[ 2 ] )
  && ( (bool) ClipPlane     [ 0 ] || (bool) ClipPlane     [ 1 ] || (bool) ClipPlane     [ 2 ] )
  || ( how & GLPaintForceClipping ) )

    clipplane   = otherclipplane;
else                                    // otherwise, use local clipping
    clipplane   = ClipPlane;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static  bool        NestedPaint     = false;            // check for re-entrant called (could be put in some GLPAINT_... if generalized)
static  TListIterator<TBaseView>    iteratorview;


MatModelView.CopyModelView ();          // copy current transformation


if ( CurrentDisplaySpace == DisplaySpace3D && ( how & GLPaintLinked ) )
    GLFogOn         ();                 // previous fog (called from outside)
                                        // pb is called from XE, fog color is black, which does not render very well in 3D


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// The opaque stuff should be drawn first
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the tracks
                                        // opaque, though using transparent mode to have some blending antialiasing
if ( how & GLPaintOpaque ) {

    GLLinesModeOn       ( false );
    GLLineSmoothOff     ();             // we are ready for smooth lines, but we wait until past the scaling grids

    //bool                        drawmargin  = enoughh && enoughw;

    MarginVariables;

    int                         numsel          = IsModeFE () ? CDPf.GetLength () : max ( MaxRegularTracksPerSlot, MaxPseudoTracksPerSlot );
    double                      maxhpertrack    = WindowSlots[ 0 ].ToUp.Norm () / numsel;

    bool                        enoughh         = CurrentDisplaySpace == DisplaySpaceNone && HasWindowSlots () &&   maxhpertrack >= 2;
    bool                        enoughwdeco1    = CurrentDisplaySpace == DisplaySpaceNone && HasWindowSlots () && ( WindowSlots[0].ToRight.Norm() >= 40 );
    bool                        enoughwdeco2    = CurrentDisplaySpace == DisplaySpaceNone && HasWindowSlots () && ( WindowSlots[0].ToRight.Norm() >= 10 );

    double                      scalev          = CurrentDisplaySpace == DisplaySpaceNone ? ScalingLevel
                                                                                          : ScalingLevel * XYZDoc->GetBounding( CurrentDisplaySpace )->Radius() / 600;
    double                      scaleh;

    double                      scalet;

    double                      descalex;
    double                      descaley;
    TTracksViewWindowSlot*      toslot;
    int                         stepy           = 100;
    double                      contrast;
    double                      alpha;
    bool                        notdiss;
    bool                        ispositive      = FreqDoc->GetAtomType ( AtomTypeUseCurrent ) == AtomTypePositive;


    int                         firstsel;
    int                         numTracks;                          // current slot only
    double                      numTracksR;
//  int                         totalTracks     = SelTracks.NumSet ();  // of all slots
//  int                         numPseudoTracks = SelTracks.NumSet ( FreqDoc->GetNumElectrodes(), FreqDoc->GetTotalElectrodes() - 1 );

    long                        minp;
    long                        maxp;
    long                        nump;
    TSelection*                 sel;
    TGLCoordinates<GLfloat>     trorg;
    TGLCoordinates<GLfloat>     trdelta;
    bool                        textenum;
    int                         numel           = FreqDoc->GetNumElectrodes ();
    long                        firstfreq       = FCursor.GetPosMin ();
    long                        lastfreq        = FCursor.GetPosMax ();
    long                        numfreq         = FCursor.GetLength ();
    long                        numtf           = TFCursor.GetLength ();
    double                      minValue;
    double                      maxValue;
    double                      trh;
    double                      dtrh;
    bool                        trackssuper;
    bool                        drawvertscale;
    bool                        drawhoriscale;
    bool                        enoughtrsp;
    double                      hgridstep;
    double                      hgridorg;
    char                        buff[ 256 ];
    int                         numt;
    double                      v;
    GLfloat                     linewidth;

    bool                        draworigin      =    IsTimeHorizontal () 
                                                  && CurrentDisplaySpace == DisplaySpaceNone 
                                                  && ShowBaseline 
                                                /*&& allnames*/ 
                                                  && FreqDoc->GetSamplingFrequency () 
                                                  && ! ( FreqDoc->DateTime.RelativeTime && FreqDoc->DateTime.RelativeTimeOffset == 0 );

    double                      orgreltf        = draworigin ? TFCursor.AbsoluteMicrosecondsToRelativeTF ( 0 ) - CDPt.GetMin() : 0;


                                        // compensate the averaging by increasing the scaling
    if ( AverageMode ) {

//      if      ( IsModeSpectrum () )  scalev  = scalev * ( 1 + (double) ( numtf - 1 ) / BuffSize ) * 1.5;
        if      ( IsModeSpectrum () )  scalev *= PowerRoot ( numtf, 4 );
        else if ( IsModeEF ()       )  scalev *= sqrt ( (double) numfreq );
//      else if ( IsModeFE ()       )  scalev *= sqrt ( SelTracks.NumSet () );
        }

                                        // give some more space to any spectrum (until we use a log display)
    if ( IsModeSpectrum () )
        scalev *= 2.0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // rescaling each frequency or not
    auto    Scaling     = [ this, &notdiss ] ( int f ) { return FreqNormalize && notdiss ? ScaleFreqs[ f ] : 1; };


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // have to account for the number of subdivisions
    auto    SubTrhFEEnum    = [ & ] () {

        trh     = CurrentDisplaySpace != DisplaySpaceNone   ? fabs ( trdelta.Y )  * descaley * ( numfreq == 1 ? 2 : 0.90 )
                                                            : trdelta.Y ? fabs ( trdelta.Y )  * descaley * ( numfreq == 1 ? 2 : 0.90 )
                                                            : toslot->ToUp.Y      * 0.75 * descaley;

        numt   = sel->NumSet ();

        dtrh = trh / (double) ( numt > 1 ? numt + 1 : 1 );
        };

                                        // have to account for the number of subdivisions
    auto    SubTrhEFEnum    = [ & ] () {

        trh     = CurrentDisplaySpace != DisplaySpaceNone   ? fabs ( trdelta.Y )  * descaley * ( numTracks == 1 ? 2 : 0.90 )
                                                            : trdelta.Y ? fabs ( trdelta.Y )  * descaley * ( numTracks == 1 ? 2 : 0.90 )
                                                            : toslot->ToUp.Y      * 0.75 * descaley;

        dtrh    = trh / (double) ( numfreq > 1 ? numfreq + 1 : 1 );
        };


//  auto    SubTrhEFInt     = [ & ] () {
//
//      trh     = CurrentDisplaySpace != DisplaySpaceNone   ?             fabs ( trdelta.Y )         * descaley * ( numTracks == 1 ? 2 : 1 ) - ( textenum ? descaley : 0 )
//                                                          : trdelta.Y ? fabs ( trdelta.Y )         * descaley * ( numTracks == 1 ? 2 : 1 ) - ( textenum ? descaley : 0 )
//                                                          :             toslot->ToUp.Y      * 0.75 * descaley;
//
//      dtrh    = trh / numfreq;
//      };


//  auto    SubTrhFEInt     = [ & ] () {
//
//      trh     = CurrentDisplaySpace != DisplaySpaceNone   ?             fabs ( trdelta.Y )         * descaley * ( numfreq == 1 ? 2 : 1 ) - ( textenum ? descaley : 0 )
//                                                          : trdelta.Y ? fabs ( trdelta.Y )         * descaley * ( numfreq == 1 ? 2 : 1 ) - ( textenum ? descaley : 0 )
//                                                          :             toslot->ToUp.Y      * 0.75 * descaley;
//
//      numt    = sel->NumSet ();
//      dtrh    = trh / (double) ( numt > 1 ? numt : 1 );
//      };


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get horizontal range
    GetRelHorizRange ( minp, maxp, nump );

    draworigin &= orgreltf >= 0 && orgreltf <= nump;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Horizontal grid step set-up
    if ( ShowHorizScale && ( how & GLPaintOwner ) )

        SetupHorizontalGrid ( hgridorg, hgridstep );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute scaling factors
    constexpr double    FSaturate  = 0.75;

    if ( IsIntensityModes () ) {
                                        // contrast factor for the color computation
        contrast    = ScalingContrastToColorTable ( ScalingContrast );

        alpha       = (bool) Using ? 1.0 / ( 1 + (int) Using ) : 1.0;

//      GalMinValue = Highest ( GalMinValue );
//      GalMaxValue = Lowest  ( GalMaxValue );


        if (       ScalingAuto == ScalingAutoSymmetric /*!= ScalingAutoOff*/ 
            && ! ( RButtonDown && IntensityOverride && MouseAxis == MouseAxisHorizontal ) ) {
                                        // find min max values in current page
            minValue    = Highest ( minValue );
            maxValue    = Lowest  ( maxValue );

                                        // scan only what is in the current display
            for ( TIteratorSelectedForward sti ( SelTracks ); (bool) sti; ++sti ) {

                notdiss = sti() != FreqDoc->GetDisIndex();

                for ( int f = firstfreq; f <= lastfreq; f++ )
                for ( int t = minp; t <= maxp; t++ ) {

                    v   =  Scaling ( f ) * EegBuff ( f, sti(), t );
                                        // min max in display window
                    Maxed ( maxValue, v );
                    Mined ( minValue, v );
                    }
                }

            Maxed ( maxValue,    0.0 );
            Mined ( minValue,    0.0 );

                                        // to avoid some errors, just in case
            if ( maxValue == minValue ) {
                maxValue += 1e-6;
                minValue -= 1e-6;
                }

//          if ( minValue == 0 )    minValue = -1e-12;
//          if ( maxValue == 0 )    maxValue =  1e-12;

                                        // if we want the color table to show the window min max
            GalMinValue = minValue;
            GalMaxValue = maxValue;

                                        // saturate enough to see only the main blobs
            double          absmax      = max ( maxValue, fabs ( minValue ) );
            double          ctmin       = ScalingAuto == ScalingAutoAsymmetric ? minValue : - absmax;
            double          ctmax       = ScalingAuto == ScalingAutoAsymmetric ? maxValue :   absmax;

            ColorTable.SetParameters ( ctmax * ( IsIntensityPrecise () ? 1 : FSaturate + ScalingContrast * ( 1 - FSaturate ) ),
                                       ctmin * ( IsIntensityPrecise () ? 1 : FSaturate + ScalingContrast * ( 1 - FSaturate ) ), 
                                       contrast, 
                                       alpha );
            }

        else if ( ScalingAuto == ScalingAutoOff ) {
/*
            maxValue        =  FreqDoc->GetAbsMaxValue () * ScaleTracks[ SelTracks.FirstSelected() ];

            minValue        = -maxValue;

                                        // to avoid some errors, just in case
            if ( maxValue == minValue ) {
                maxValue += 1e-6;
                minValue -= 1e-6;
                }

            if ( minValue == 0 )    minValue = -1e-12;
            if ( maxValue == 0 )    maxValue =  1e-12;

            double  s       = (double) 60 / ScalingPMax;   // normalized scaling, and relatively scaled for intensities
//            double  s       = (double) 60 / ScalingPMax * ( AverageMode ? 2 : 1 );   // normalized scaling, and relatively scaled for intensities

            ColorTable.SetParameters ( maxValue * s, minValue * s, contrast, alpha );
*/

            maxValue    = Lowest  ( maxValue );

            for ( TIteratorSelectedForward sti ( SelTracks ); (bool) sti; ++sti ) {

                notdiss = sti() != FreqDoc->GetDisIndex();

                for ( int f = firstfreq; f <= lastfreq; f++ )
                for ( int t = minp; t <= maxp; t++ ) {

                    v =  fabs ( Scaling ( f ) * EegBuff ( f, sti(), t ) );
                    Maxed ( maxValue, v );
                    }
                }


            minValue        = FreqDoc->IsAbsolute ( AtomTypeUseCurrent ) ? 0 : - maxValue;

                                        // to avoid some errors, just in case
            if ( maxValue == minValue ) {
                maxValue += 1e-6;
                minValue -= 1e-6;
                }

//          if ( minValue == 0 )    minValue = -1e-12;
//          if ( maxValue == 0 )    maxValue =  1e-12;

                                        // if we want the color table to show the window min max
            GalMinValue = minValue;
            GalMaxValue = maxValue;


            minValue        = minValue / ScalingPMax * TrackHeightToIntensity * ( IsIntensityPrecise () ? 1 : FSaturate + ScalingContrast * ( 1 - FSaturate ) );
            maxValue        = maxValue / ScalingPMax * TrackHeightToIntensity * ( IsIntensityPrecise () ? 1 : FSaturate + ScalingContrast * ( 1 - FSaturate ) );

                                        // ?not tested - taken from Tracks Display - it needs the current track index st!
//            minValue       *= TrackHeightToIntensity * ( IsIntensityPrecise () ? 1 : FSaturate + ScalingContrast * ( 1 - FSaturate ) ) / ( ScalingPMax * ScaleTracks[ st ] );
//            maxValue       *= TrackHeightToIntensity * ( IsIntensityPrecise () ? 1 : FSaturate + ScalingContrast * ( 1 - FSaturate ) ) / ( ScalingPMax * ScaleTracks[ st ] );

            ColorTable.SetParameters ( maxValue, minValue, contrast, alpha );
            }

        } // IsIntensityModes ()


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // an optional 2 passes for 3D
  for ( int pass = CurrentDisplaySpace == DisplaySpace3D ? 0 : 1; pass < 2; pass++ )

    for ( int slot = 0; slot < GetNumWindowSlots (); slot++ ) {

        glPushMatrix ();                // save current orientation & origin, before any local shift


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        toslot      = &WindowSlots[ slot ];
        sel         = &toslot->SelTracks;
        firstsel    = sel->FirstSelected ();                                    // to have the same spacing across slots of the same type

        numTracks   = IsModeFE ()           ? RoundAbove ( (double) numfreq / GetNumWindowSlots () ) 
                    : toslot->RegularTracks ? MaxRegularTracksPerSlot 
                    :                         MaxPseudoTracksPerSlot;
        numTracksR  = 1 / (double) ( numTracks + 1 );

        textenum    = toslot->ToUp.Norm() * numTracksR > SFont->GetHeight();    // enough room for all text?
        scaleh      =      CurrentDisplaySpace == DisplaySpaceNone 
                    && ! ( IsBarsMode () || IsIntensitySquareMode () )    ? (double) ( toslot->ToRight.Norm() - margin - 1 ) / ( nump > 1 ? nump - 1 : 1 )
                                                                    : (double) ( toslot->ToRight.Norm() - margin     ) /   nump ;

                                        // smart superimpose: asked / not enough space / not pseudo tracks
        trackssuper =    ( ! IsIntensityModes () || IsModeSpectrum () ) 
                      && ( ( TracksSuper && ( firstsel < numel || AverageMode /*IsModeFEAvg*/ ) ) || ! enoughh );


        if ( CurrentDisplaySpace != DisplaySpaceNone ) {  // all shifts are already included

            trorg.X     = toslot->LowLeft.X;
            trorg.Y     = toslot->LowLeft.Y;
            trorg.Z     = toslot->LowLeft.Z;

            trdelta.X   = 0;
            trdelta.Y   = toslot->ToUp.Norm() * numTracksR * 0.90 * CursorHeightFactor3D;
            trdelta.Z   = 0;


//          if ( IsModeFEAvg () )       // high frequencies point upward
                trdelta.Y   *= -1;

                                        // get world coordinates
            GLApplyModelMatrix ( trorg, MatModelView );
                                        // add a screen-relative shift to recenter the track
            trorg.X    -= toslot->ToRight.Norm() / 2 + margin;
                                        // reset, keep only Z translation
            glLoadIdentity();
                                        // redo the shift

                                        // test for 2 Z halves, and draw according to current pass
            if ( CurrentDisplaySpace == DisplaySpace3D )
                if ( !pass ) {
                    if ( trorg.Z >  ModelRadius )    { glPopMatrix (); continue; }
                    }
                else {
                    if ( trorg.Z <= ModelRadius )    { glPopMatrix (); continue; }
                    }
            }

        else if ( trackssuper ) {

            trorg.X     = toslot->LowLeft.X + margin;
            trorg.Y     = toslot->LowLeft.Y + 0.5 * toslot->ToUp.Y;
            trorg.Z     = toslot->LowLeft.Z;

            trdelta.X   =
            trdelta.Y   =
            trdelta.Z   = 0;

            //if ( ispositive && ! IsIntensityModes () ) { // slightly shift downward
                //trorg.Y     = toslot->LowLeft.Y + ( TracksFlipVert ? 0.6 : 0.4 ) * toslot->ToUp.Y;
                //}
            }

        else {

            trorg.X     = toslot->LowLeft.X + margin;
            trorg.Y     = toslot->LowLeft.Y + toslot->ToUp.Y * ( 1 - ( 0.5 + ( numTracksR - 0.5 ) * 0.90 ) );
            trorg.Z     = toslot->LowLeft.Z;

            trdelta.X   = 0;
            trdelta.Y   = - toslot->ToUp.Y * numTracksR * 0.90;
            trdelta.Z   = 0;

            if ( renderingmode == LayoutOneTrackOneBox && IsModeFE () && numfreq > 1 ) // not sure why this mode needs to be rescaled
                trdelta.Y  *= 2;

            //if ( ispositive && ! IsIntensityModes () ) { // slightly shift downward
                //trorg.Y     = toslot->LowLeft.Y + toslot->ToUp.Y * ( 1 - ( 0.5 + ( numTracksR - 0.5 ) * 0.90 ) - ( TracksFlipVert ? -0.50 : 0.25 ) * numTracksR );
                //trdelta.Y   = - toslot->ToUp.Y * numTracksR * 0.9;
                //}
            }


        enoughtrsp      = trackssuper || trdelta.Y * -4.0 > stepy;

//      drawvertscale   = ShowVertScale /*&& !Is3DimensionVertical* / && ( !IsModeFT () || FreqMode == FreqModeFT && ( trackssuper || numTracks == 1 ) || FreqMode == FreqModeFTiE ) && numTracks
        drawvertscale   = ShowVertScale && ( ! IsModeFE () || IsModeFE () && ( AverageMode && ! IsIntensityModes () && ( trackssuper || numTracks == 1 ) || IsIntensityModes () ) ) && numTracks
                       && CurrentDisplaySpace == DisplaySpaceNone && ( enoughh || trackssuper ) && enoughtrsp && enoughwdeco1
                       && ( how & GLPaintOwner );

        drawhoriscale   = ShowHorizScale && numTracks
                       && CurrentDisplaySpace == DisplaySpaceNone 
                       && ( toslot->ToRight.Norm() >= EEGGLVIEW_ENOUGHWHORIZGRID ) 
                       && enoughwdeco1 
                    // && enoughh && enoughw
                       && ( how & GLPaintOwner );

                                        // save current settings for outer/later use
        toslot->TrOrg   = trorg;
        toslot->TrDelta = trdelta;
        toslot->ScaleH  = scaleh;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( ! LineWidth ) {

            if ( CurrentDisplaySpace != DisplaySpaceNone ) {
                                        // horizontal scaling
                linewidth   = NoMore ( 1.0, scaleh * 500 );

                                        // global window size
                double  ws  = GetCurrentWindowSize ( how & GLPaintOwner );
                linewidth  *= NoMore ( 2.5, ws * 0.0035 );

                                        // zoom factor
                linewidth  *= GetCurrentZoomFactor ( how & GLPaintOwner );

                                        // object size factor
                linewidth  *= XYZDoc->GetBounding ( CurrentDisplaySpace )->Radius ();

                                        // more lines due to frequencies
                linewidth  /= ( IsEnumerateVertical () || IsModeFE () ? numfreq : 1 );
                }
            else {
                                        // horizontal scaling
//              linewidth   = TracksSuper || trackssuper ? 0  // to have the same value, even with pseudos
//                                                       : NoMore ( 0.5, scaleh * 0.04 );
                linewidth   = NoMore ( 0.5, scaleh * 0.04 );

                                        // vertical scaling
                linewidth  *= NoMore ( 2.0,
                                       sqrt ( WindowSlots[0].ToUp.Norm() / ( IsModeFEAvg () ? numfreq : WindowSlots[ 0 ].SelTracks.NumSet () )
                                                                         / ( IsEnumerateVertical () ? numfreq : 1 ) ) * 0.15 );

                                        // intensity scaling - difficult to make it consistent, though
                if ( scaleh > 1.0 )     // restrict to "zoom" view
                    linewidth  *= Clip ( sqrt ( scalev ) * 0.02, 0.75, 1.5 );
                }

                                        // resize with current DPI & clip to a global max
            linewidth   = Clip ( (GLfloat) MmToPixels ( linewidth ), GLLineWidthMin, AutoLineWidthMax );
            }
        else
            linewidth = LineWidth;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw vertical left limit
        if ( CurrentDisplaySpace == DisplaySpaceNone 
          && RenderingMode != LayoutOneTrackOneBoxXyzFlat 
          && enoughwdeco2 
          && ( how & GLPaintOwner ) ) {

            glColor4f ( AxisColor );

            glBegin ( GL_LINES );
            glVertex3f ( trorg.X, toslot->LowLeft.Y,                  toslot->LowLeft.Z );
            glVertex3f ( trorg.X, toslot->LowLeft.Y + toslot->ToUp.Y, toslot->LowLeft.Z );
            glEnd ();
            }
/*
        else {
            glBegin ( GL_LINES );
            glVertex3f ( trorg.X, trorg.Y - toslot->ToUp.Norm() / 2, trorg.Z );
            glVertex3f ( trorg.X, trorg.Y + toslot->ToUp.Norm() / 2, trorg.Z );
            glEnd ();
            }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // triggers
        if (     ShowTags 
            &&   IsTimeHorizontal () 
            && ( how & GLPaintOwner )
            && ( CurrentDisplaySpace != DisplaySpace3D || trorg.Z > ModelRadius ) ) {   // no triggers in back part of 3D

            DrawTriggers ( toslot );

            GLColoringOn        ();
            }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the horizontal grid - pre
        if (   drawhoriscale 
            && IsTracksMode ()
            && ( how & GLPaintOwner ) ) {

            double  x, y1, y2, z;

            glColor4f ( AxisColor );

            y1  = toslot->LowLeft.Y;
            y2  = toslot->LowLeft.Y + toslot->ToUp.Y;
            z   = toslot->LowLeft.Z;

            if ( IsFreqHorizontal () ) {

                glBegin ( GL_LINES );

                for ( int f = hgridorg; f <= maxp; f += hgridstep ) {
                    x       = trorg.X + ( f + ( IsBarsMode () ? 0.5 : 0 ) ) * scaleh;
                    glVertex3f ( x, y1, z );
                    glVertex3f ( x, y2, z );
                    }

                glEnd ();

                for ( int f = hgridorg; f <= maxp; f += hgridstep )
                    SFont->Print ( trorg.X + ( f + ( IsBarsMode () ? 0.5 : 0 ) ) * scaleh + 3, toslot->LowLeft.Y, toslot->LowLeft.Z, FreqDoc->GetFrequencyName ( f ), TA_LEFT | TA_BOTTOM );
                }

            else if ( IsTimeHorizontal () ) {

                #define     stf     buff
                #define     stime   ( buff + 128 )

                glBegin ( GL_LINES );

                for ( double g = hgridorg - CDPt.GetMin(); g < CDPt.GetLength(); g += hgridstep ) {
                    x       = trorg.X + ( g + ( IsBarsMode () ? 0.5 : 0 ) ) * scaleh;
                    glVertex3f ( x, y1, z );
                    glVertex3f ( x, y2, z );
                    }

                glEnd ();

                for ( double g = hgridorg - CDPt.GetMin(); g < CDPt.GetLength(); g += hgridstep ) {

                    TFToString ( g + CDPt.GetMin(), stf, stime, ShowHorizScaleLegal );

                    SFont->Print ( trorg.X + ( g + ( IsBarsMode () ? 0.5 : 0 ) ) * scaleh + 3, y1, z, *stime ? stime : stf, TA_LEFT | TA_BOTTOM );
                    }
                }
            } // drawhoriscale - pre


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // go to first track
                                        // from here, use glVertex2f, as z=0 all along
        glTranslatef ( trorg.X, trorg.Y, trorg.Z );

                                        // 1st name
        bool        firsttrack  = true;
        int         st          = sel->FirstSelected ();

        if (     drawmargin 
            && ( ! textenum || trackssuper ) 
            && ( how & GLPaintOwner )       ) {
                                        // select color (copy of further code)
            if      ( Highlighted[ st ] )   glColor4f ( HighlightColor   );
            else if ( BadTracks  [ st ] )   glColor4f ( BadColor         );
            else if ( st >= numel )         glColor4f ( PseudotrackColor );
            else if ( AuxTracks  [ st ] )   glColor4f ( AuxColor         );
                 else                       glColor4f ( TrackColor       );

                                        // draw first of summary depending on superimpose
            const char*     ton = IsModeFE () ? FreqDoc->GetFrequencyName ( lastfreq ) : GetElectrodeName ( st );

            if ( IsModeFE () )
                ColorFreqs.GLize ( (double) lastfreq );

            if ( trackssuper ) {
                if ( numTracks > 1 )
                    glTranslatef ( 0,   SFont->GetHeight() / 2, 0 );

                SFont->Print ( -4, 0, 0, ton, TA_RIGHT | TA_CENTERY );

                if ( numTracks > 1 )
                    glTranslatef ( 0, - SFont->GetHeight() / 2, 0 );
                }
            else
                SFont->Print ( -4, 0, 0, ton, TA_RIGHT | TA_CENTERY );
            }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // globally rescale everything
        glScaled ( scaleh, scalev, 1 );
                                        // and keep scale-back factors
        descalex    = 1 / scaleh;
        descaley    = 1 / scalev;

                                        // draw each selected track
        for ( TIteratorSelectedForward sti ( *sel ); (bool) sti; ++sti, firsttrack = false ) {

            st          = sti();

            scalet      = ScaleTracks [ st ];
            notdiss     = st != FreqDoc->GetDisIndex();

            glScaled ( 1, scalet, 1 );
            descaley    = 1 / scalev / scalet;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the vertical grid - pre
            if (      drawvertscale 
                 && ( IsModeSpectrum () || ! IsIntensityModes () ) 
                 && ( ! trackssuper     || firsttrack         ) 
                 && ( ! IsModeEFEnum () || numfreq == 1       ) ) {

                //bool        regel   = st <= EEGDoc->GetLastRegularIndex();  // regular electrode
                double      dv;
                double      y;
                double      ot      = /*regel ? OffsetTracks :*/ 0;
                double      maxy    = ( ( numTracks == 1 || trackssuper ) ? toslot->ToUp.Y                     * descaley / 2
                                                                          : toslot->ToUp.Y / ( numTracks + 2 ) * descaley / 2 );
                                        // vertical step, in value
                dv          = fabs ( stepy * descaley );
                dv          = Power ( 10, Truncate ( Log10 ( dv ) ) );

                if      ( dv / descaley < 20  )   dv *= 2;  // too close -> increase space
                else if ( dv / descaley > 200 )   dv /= 10; // too far   -> reduce space


                glColor4f ( AxisColor );
                                        // min and max indexes, in steps
                int         mini    = - maxy / dv * ( ispositive && ! trackssuper ? 0.15 : 1 )  + (int) ( ot / dv );
                int         maxi    =   maxy / dv * ( ispositive ? 2.25 : 1 )                   + (int) ( ot / dv );

                for ( int i = mini; i <= maxi; i++ ) {
                    y   = i * dv;       // step -> current value

                    if ( ispositive && y < 0 )
                        continue;

                    glBegin ( GL_LINES );
                    glVertex2f ( 0,                              y - ot );
                    glVertex2f ( maxp + ( IsBarsMode () ? 1 : 0 ),  y - ot );
                    glEnd ();

                    FloatToString ( buff, y );
                    SFont->Print ( 3 * descalex,  y - ot, trorg.Z, buff, TA_LEFT | TA_TOP );
                    }
                } // drawvertscale - pre


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // select color (but not applied yet)

            if ( NestedPaint || (bool) Using ) { // account for multiple Eegs

                int index = NestedPaint ? ( (int) iteratorview + 1 ) % MaxLinkColors : 0;
                LineColor[ 0 ] = LinkColors[ index ];

                if ( index >= DashLinkColors ) {
                    glLineStipple ( /*CaptureMode == CaptureGLMagnify ? Magnifier[2] :*/ 1, 0xf0f0 );
                    glEnable ( GL_LINE_STIPPLE );
                    }
                }
            else if ( Highlighted[ st ] )   LineColor.Set ( 0, HighlightColor );
            else if ( BadTracks  [ st ] )   LineColor.Set ( 0, BadColor );
            else {                      // this Eeg alone
                if ( st >= numel )          LineColor.Set ( 0, PseudotrackColor );
                else if ( AuxTracks[ st ] ) LineColor.Set ( 0, AuxColor );
                     else                   LineColor.Set ( 0, TrackColor );
                }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw baseline axis - pre
            if ( ( ShowBaseline || Show3DText ) 
              && ( ! trackssuper || firsttrack ) 
              && ( how & GLPaintOwner )
              && ( ! IsIntensityModes () || IsModeSpectrum () ) 
              && CurrentDisplaySpace == DisplaySpaceNone    ) {

                bool    text;           // if text has to be displayed
                bool    alltext;        // test if display text and compact all names in 1 line

                if ( IsFilling && Filling[ st ] )   LineColor.GLize ( 0 );
                else                                glColor4f ( AxisColor );


                if ( IsModeFEEnum () && ( CurrentDisplaySpace == DisplaySpaceNone || numfreq == 1 ) ) {

                    SubTrhFEEnum ();

                    if ( numt <= 1 )    trh = dtrh = 0;

                    glPushMatrix ();

                    text    = Show3DText && enoughw && ( numt == 1 && enoughtrsp || dtrh / descaley > SFont->GetHeight() );
                    alltext = Show3DText && enoughw && ! text && ( textenum || trackssuper );

                    if ( ! text && alltext )
                        sel->ToText ( buff, FreqDoc->GetElectrodesNames() );

                    #define     MinBaselineH            1.5
                    bool dobl = !dtrh || dtrh / descaley > MinBaselineH;


                    for ( TIteratorSelectedForward fi ( toslot->SelFreqs ); (bool) fi && dobl && ( ! trackssuper || fi.GetIndex () == 0 ); ++fi ) {

                        glTranslatef ( 0, trh / 2 - dtrh, 0 );

                        if ( alltext ) {
                            glColor4f ( ThirdDimensionTextColor );

                            SFont->Print ( 3 * descalex, 0, trorg.Z, buff, TA_LEFT | TA_BOTTOM );

                            if ( IsFilling && Filling[ st ] )   LineColor.GLize ( 0 );
                            else                                glColor4f ( AxisColor );
                            alltext = false;
                            }

                        for ( TIteratorSelectedForward sti ( *sel, firstsel ); (bool) sti; ++sti ) {

                            if ( ShowBaseline ) {
                                glBegin ( GL_LINE_STRIP );
                                glVertex2f ( minp,                           0 );
                                glVertex2f ( maxp + ( IsBarsMode () ? 1 : 0 ),  0 );
                                glEnd ();
                                }

                            if ( draworigin ) {
                                glLineWidth ( 2 );
                                glBegin ( GL_LINES );
                                glVertex2f ( orgreltf + ( IsBarsMode () ? 0.5 : 0 ),  4 * descaley );
                                glVertex2f ( orgreltf + ( IsBarsMode () ? 0.5 : 0 ), -4 * descaley );
                                glEnd ();
                                glLineWidth ( 1 );
                                }

                            if ( text ) {
                                glColor4f ( ThirdDimensionTextColor );

                                StringCopy ( buff, FreqDoc->GetElectrodeName ( sti() ) );
                                SFont->Print ( 3 * descalex, 0, trorg.Z, buff, TA_LEFT | TA_BOTTOM );

                                if ( IsFilling && Filling[ sti() ] )    LineColor.GLize ( 0 );
                                else                                    glColor4f ( AxisColor );
                                }
                            glTranslatef ( 0, - dtrh, 0 );
                            }
                        glTranslatef ( trdelta.X * descalex, trdelta.Y * descaley + trh / 2, trdelta.Z );
                        }
                    glPopMatrix ();
                    }

                else if ( IsModeFE () && ( CurrentDisplaySpace == DisplaySpaceNone || numfreq == 1 ) ) { // other FE

                    if ( firstsel <= FreqDoc->GetLastRegularIndex() )
                        sel->ToText ( buff, FreqDoc->GetElectrodesNames() );
                    else
                        StringCopy ( buff, FreqDoc->GetElectrodeName ( firstsel ) );

                    glPushMatrix ();
                                        // scan at once all freq baselines
                    for ( TIteratorSelectedForward fi ( toslot->SelFreqs ); (bool) fi && ( ! trackssuper || fi.GetIndex () == 0 ); ++fi ) {

                        text = Show3DText && enoughw && ( textenum || trackssuper ) && fi() == lastfreq;

                        if ( ShowBaseline ) {
                            glBegin ( GL_LINE_STRIP );
                            glVertex2f ( minp,                           0 );
                            glVertex2f ( maxp + ( IsBarsMode () ? 1 : 0 ),  0 );
                            glEnd ();
                            }

                        if ( draworigin ) {
                            glLineWidth ( 2 );
                            glBegin ( GL_LINES );
                            glVertex2f ( orgreltf + ( IsBarsMode () ? 0.5 : 0 ),  4 * descaley );
                            glVertex2f ( orgreltf + ( IsBarsMode () ? 0.5 : 0 ), -4 * descaley );
                            glEnd ();
                            glLineWidth ( 1 );
                            }

                        if ( text ) {
                            glColor4f ( ThirdDimensionTextColor );
                            SFont->Print ( 3 * descalex, 0, trorg.Z, buff, TA_LEFT | TA_BOTTOM );
                            if ( IsFilling && Filling[ st ] )   LineColor.GLize ( 0 );
                            else                                glColor4f ( AxisColor );
                            }

                        glTranslatef ( trdelta.X * descalex, trdelta.Y * descaley, trdelta.Z );
                        }

                    glPopMatrix ();
                    }

                else if ( IsModeEFAvg () ) {

                    text = Show3DText && enoughw && ( textenum || trackssuper ) && firsttrack;

                    if ( ShowBaseline ) {
                        glBegin ( GL_LINES );
                        glVertex2f ( minp,                           0 );
                        glVertex2f ( maxp + ( IsBarsMode () ? 1 : 0 ),  0 );
                        glEnd ();
                        }

                    if ( draworigin ) {
                        glLineWidth ( 2 );
                        glBegin ( GL_LINES );
                        glVertex2f ( orgreltf + ( IsBarsMode () ? 0.5 : 0 ),  4 * descaley );
                        glVertex2f ( orgreltf + ( IsBarsMode () ? 0.5 : 0 ), -4 * descaley );
                        glEnd ();
                        glLineWidth ( 1 );
                        }

                    if ( text ) {
                        ColorFreqs.GLize ( (double) ( firstfreq + lastfreq ) / 2 );

                        if ( numfreq == 1 )     StringCopy  ( buff, FreqDoc->GetFrequencyName ( firstfreq ) );
                        else                    StringCopy  ( buff, FreqDoc->GetFrequencyName ( firstfreq ), " - ", FreqDoc->GetFrequencyName ( lastfreq ) );

                        SFont->Print ( 3 * descalex, 0, trorg.Z, buff, TA_LEFT | TA_BOTTOM );

                        if ( IsFilling && Filling[ st ] )   LineColor.GLize ( 0 );
                        else                                glColor4f ( AxisColor );
                        }
                    }

                else if ( IsModeEFEnum () ) {

                    SubTrhEFEnum ();

                    if ( numfreq <= 1 ) trh = dtrh = 0;

                    trh  = - trh / 2 + dtrh;

                    text    = Show3DText && enoughw && ( numfreq == 1 && enoughtrsp || dtrh / descaley > SFont->GetHeight() );
                    alltext = Show3DText && enoughw && ! text && ( textenum || trackssuper ) && firsttrack;

                    if ( ! text && alltext )
                        if ( numfreq == 1 )     StringCopy  ( buff, FreqDoc->GetFrequencyName ( firstfreq ) );
                        else                    StringCopy  ( buff, FreqDoc->GetFrequencyName ( firstfreq ), " - ", FreqDoc->GetFrequencyName ( lastfreq ) );

                    bool dobl = !dtrh || dtrh / descaley > MinBaselineH;


                    for ( int f = firstfreq; f <= lastfreq && dobl; f++, trh += dtrh ) {

                        if ( ShowBaseline ) {
                            glBegin ( GL_LINES );
                            glVertex2f ( 0,                              trh );
                            glVertex2f ( maxp + ( IsBarsMode () ? 1 : 0 ),  trh );
                            glEnd ();
                            }

                        if ( draworigin ) {
                            glLineWidth ( 2 );
                            glBegin ( GL_LINES );
                            glVertex2f ( orgreltf + ( IsBarsMode () ? 0.5 : 0 ), trh + 4 * descaley );
                            glVertex2f ( orgreltf + ( IsBarsMode () ? 0.5 : 0 ), trh - 4 * descaley );
                            glEnd ();
                            glLineWidth ( 1 );
                            }

                        if ( text ) {
                            ColorFreqs.GLize ( (double) f );
                            SFont->Print ( 3 * descalex, trh, trorg.Z, FreqDoc->GetFrequencyName ( f ), TA_LEFT | TA_BOTTOM );
                            if ( IsFilling && Filling[ st ] )   LineColor.GLize ( 0 );
                            else                                glColor4f ( AxisColor );
                            }
                        } // for f

                    if ( alltext ) {
                        ColorFreqs.GLize ( (double) ( firstfreq + lastfreq ) / 2 );
                        SFont->Print ( 3 * descalex, trh - dtrh, trorg.Z, buff, TA_LEFT | TA_BOTTOM );
                        if ( IsFilling && Filling[ st ] )   LineColor.GLize ( 0 );
                        else                                glColor4f ( AxisColor );
                        }
                    }

                else if ( IsModeEF () || IsModeSpectrum () ) { // other EF

                    text = Show3DText && drawhoriscale && enoughw && ( textenum || trackssuper ) && firsttrack;

                    if ( ShowBaseline ) {
                        glBegin ( GL_LINES );
                        glVertex2f ( minp,                          0 );
                        glVertex2f ( maxp + ( IsBarsMode () ? 1 : 0 ), 0 );
                        glEnd ();
                        }

                    if ( text ) {

                        if ( IsModeSpectrumEnum () || ! TFCursor.IsSplitted() ) {

                            #define     stfmin      buff
                            #define     stimemin    ( buff + 32 )
                            #define     stfmax      ( buff + 64 )
                            #define     stimemax    ( buff + 96 )

                            TFToString ( CDPt.GetMin () + (long) toslot->UserData,   stfmin,   stimemin,    ShowHorizScaleLegal,   0 );

                            if ( *stimemin )    StringCopy  ( buff, stimemin );
                            else                StringCopy  ( buff, stfmin   );
                            }
                        else {
                            TFToString ( TFCursor.GetPosMin (),   stfmin,   stimemin,    ShowHorizScaleLegal,   0 );
                            TFToString ( TFCursor.GetPosMax (),   stfmax,   stimemax,    ShowHorizScaleLegal,   0 );

                            if ( *stimemin )    StringCopy  ( buff, stimemin, " - ", stimemax );
                            else                StringCopy  ( buff, stfmin,   " - ", stfmax   );
                            }

    //                    text = ( SFont->GetStringWidth ( buff ) < WindowSlots[0].ToRight.Norm() )
    //                        && textenum;

    //                    if ( text )
                        glColor4f ( ThirdDimensionTextColor );
                        SFont->Print ( 3 * descalex, 0, trorg.Z, buff, TA_LEFT | TA_BOTTOM );
                        if ( IsFilling && Filling[ st ] )   LineColor.GLize ( 0 );
                        else                                glColor4f ( AxisColor );
                        }
                    }

                } // if ShowBaseline


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

            GLLineSmoothOn      ();

                                        // draw background behind track (copy of following code)
                                        // - actually not implemented -

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set line width for track
            glLineWidth ( linewidth );
            LineColor.GLize ( 0 );

                                        // draw name
            if (   drawmargin 
                && textenum 
                && ! trackssuper 
                && ! IsModeFE () 
                && ( how & GLPaintOwner ) )

                SFont->Print ( -4 * descalex, 0, 0, GetElectrodeName ( st ), TA_RIGHT | TA_CENTERY );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

            if ( IsModeSpectrum () ) {  // frequencies by electrodes

                if ( AverageMode ) {    // either 1 TF or an average of n TFs

                    glBegin ( IsBarsMode () ? GL_QUADS : GL_LINE_STRIP );

                    if ( (bool) Using )                 LineColor[ 0 ].GLize ();

                    for ( int f = minp; f <= maxp; f++ ) {

                        if ( (bool) Using )             ;
                        else if ( Highlighted[ st ] )   glColor4f ( HighlightColor );
                        else if ( BadTracks  [ st ] )   glColor4f ( BadColor );
                        else if ( AuxTracks  [ st ] )   glColor4f ( AuxColor );
                        else                            ColorFreqs.GLize ( (double) f );

                        double      sumv    = 0;    // average value done on-demand to avoid messing the buffers

                        for ( int t = 0; t < numtf; t++ ) {
                            v       = EegBuff ( f, st, t );

                            if ( FreqDoc->IsFreqTypeFFTApproximation () )
                                sumv    += v * v;
                            else
                                sumv    += v;
                            }
                        sumv   *= ( FreqDoc->IsFreqTypeFFTApproximation () ? Scaling ( f ) * Scaling ( f ) : Scaling ( f ) ) / numtf;

                        if ( IsBarsMode () ) {
                            glVertex2f ( f    , 0    );
                            glVertex2f ( f    , sumv );
                            glVertex2f ( f + 1, sumv );
                            glVertex2f ( f + 1, 0    );
                            }
                        else
                            glVertex2f ( f, sumv );
                        }

                    glEnd ();
                    } // FreqModeSpectrum


                else { // ! AverageMode

                    int     t   = toslot->UserData; // time has been conveniently stored here

                    glBegin ( IsBarsMode () ? GL_QUADS : GL_LINE_STRIP );

                    if ( (bool) Using )                 LineColor[ 0 ].GLize ();
                    else if ( !enoughwdeco2 )           LineColor[ 0 ].GLize ();

                    for ( int f = minp; f <= maxp; f++ ) {

                        if ( (bool) Using || !enoughwdeco2 )            ;
                        else if ( Highlighted[ st ] )   glColor4f ( HighlightColor );
                        else if ( BadTracks  [ st ] )   glColor4f ( BadColor );
                        else if ( AuxTracks  [ st ] )   glColor4f ( AuxColor );
                        else                            ColorFreqs.GLize ( (double) f );

                        v       = Scaling ( f ) * EegBuff ( f, st, t );

                        if ( FreqDoc->IsFreqTypeFFTApproximation () )
                            v  *= v;

                        if ( IsBarsMode () ) {
                            glVertex2f ( f    , 0 );
                            glVertex2f ( f    , v );
                            glVertex2f ( f + 1, v );
                            glVertex2f ( f + 1, 0 );
                            }
                        else
                            glVertex2f ( f, v );
                        }

                    glEnd ();
                    } // FreqModeSpectrum

                } // IsModeSpectrum


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

            else if ( IsModeEF () ) {   // electrodes by frequencies / time

                if      ( IsModeEFAvg () ) {

                    if ( (bool) Using )             LineColor[ 0 ].GLize ();
                    else if ( Highlighted[ st ] )   glColor4f ( HighlightColor );
                    else if ( BadTracks  [ st ] )   glColor4f ( BadColor );
                    else if ( AuxTracks  [ st ] )   glColor4f ( AuxColor );
                    else                            ColorFreqs.GLize ( (double) ( firstfreq + lastfreq ) / 2 );

                    glBegin ( IsBarsMode () ? GL_QUADS : GL_LINE_STRIP );

                    for ( int t = minp; t <= maxp; t++ ) {
                        v = 0;
                        for ( int f = 0; f < numfreq; f++ ) // !! ScaleFreqs in absolute, EegBuff in relative
                            v += Scaling ( f + firstfreq ) * EegBuff ( f + firstfreq, st, t );
                        v /= numfreq;

                        if ( IsBarsMode () ) {
                            glVertex2f ( t    , 0 );
                            glVertex2f ( t    , v );
                            glVertex2f ( t + 1, v );
                            glVertex2f ( t + 1, 0 );
                            }
                        else
                            glVertex2f ( t, v );
                        }

                    glEnd ();
                    } // IsModeEFAvg


                else if ( IsModeEFEnum () ) {

                    SubTrhEFEnum ();

                    if ( numfreq <= 1 ) trh = dtrh = 0;

                    glTranslatef ( 0, - trh / 2 + dtrh, 0 );

                    if ( (bool) Using )                 LineColor[ 0 ].GLize ();

                    for ( int f = firstfreq; f <= lastfreq; f++ ) {

                        if ( (bool) Using )             ;
                        else if ( Highlighted[ st ] )   glColor4f ( HighlightColor );
                        else if ( BadTracks  [ st ] )   glColor4f ( BadColor );
                        else if ( AuxTracks  [ st ] )   glColor4f ( AuxColor );
                        else                            ColorFreqs.GLize ( (double) f );

                        glBegin ( IsBarsMode () ? GL_QUADS : GL_LINE_STRIP );

                        for ( int t = minp; t <= maxp; t++ )
                            if ( IsBarsMode () ) {
                                glVertex2f ( t,     0 );
                                glVertex2f ( t,     Scaling ( f ) * EegBuff ( f, st, t ) );
                                glVertex2f ( t + 1, Scaling ( f ) * EegBuff ( f, st, t ) );
                                glVertex2f ( t + 1, 0 );
                                }
                            else
                                glVertex2f ( t, Scaling ( f ) * EegBuff ( f, st, t ) );

                        glEnd ();

                        glTranslatef ( 0, dtrh, 0 );
                        } // for f

                    glTranslatef ( 0, - trh / 2, 0 );
                    } // FreqModeEF


                else if ( IsIntensityModes () ) {

                                        // special case: get auto-scaling for each track
                    if ( ScalingAuto == ScalingAutoAsymmetric && ! ( RButtonDown && IntensityOverride && MouseAxis == MouseAxisHorizontal ) ) {
                                                    // find min/max
                        minValue    =  DBL_MAX;
                        maxValue    = -DBL_MAX;

                                                    // scan only what is in the current display
                        scalet  = ScaleTracks [ st ];
                        notdiss = st != FreqDoc->GetDisIndex();

                        for ( int f = firstfreq; f <= lastfreq; f++ )
                        for ( int t = minp; t <= maxp; t++ ) {

                            v =  Scaling ( f ) * EegBuff ( f, st, t );
                            Maxed ( maxValue, v );
                            Mined ( minValue, v );
                            }

                        if ( maxValue < 0 )     maxValue = 0;
                        if ( minValue > 0 )     minValue = 0;

                                                    // to avoid some errors, just in case
                        if ( maxValue == minValue ) {
                            maxValue += 1e-6;
                            minValue -= 1e-6;
                            }

//                      if ( minValue == 0 )    minValue = -1e-12;
//                      if ( maxValue == 0 )    maxValue =  1e-12;

                        double          ctmin       = minValue;
                        double          ctmax       = maxValue;

                        ColorTable.SetParameters ( ctmax * ( IsIntensityPrecise () ? 1 : FSaturate + ScalingContrast * ( 1 - FSaturate ) ),
                                                   ctmin * ( IsIntensityPrecise () ? 1 : FSaturate + ScalingContrast * ( 1 - FSaturate ) ), 
                                                   contrast, 
                                                   alpha );
                        }


//                  SubTrhEFInt ();
//                  double      yoff    = ( trh - dtrh ) / 2;

                    SubTrhEFEnum ();

//                    if ( VkKey ( 'Q' ) ) DBGV2 ( trh, dtrh, "trh dtrh" );

                    if ( numfreq <= 1 && IsIntensitySquareMode () )    dtrh /= 2;  // or  trh *= 2  for bigger stripes
                                        // offset
                    double      yoff    = trh / 2 - ( IsIntensitySquareMode () ? dtrh / 2 : dtrh );

                    glTranslatef ( 0, - yoff, 0 );


//                    bool        logmode     = false; // true; // a priori always true
//                    double      logstrength = 1 + 100 * ScalingContrast;
                    double      scaling1;

                                                           // not too much surface    numsel
//                    int         quality     = Outputing () || ( SelTracks.NumSet () * nump < 500 && !( RButtonDown || VkLButton () ) ) ? QuadBestQuality
//                    int         quality     = Outputing () || ( SelTracks.NumSet () * numfreq * nump < 40000 && !( RButtonDown || VkLButton () ) ) ? QuadBestQuality

//                  int         quality     = Outputing () || ( SelTracks.NumSet () <= 10 && ( SelTracks.NumSet () * numfreq * nump < 40000 ) && !( RButtonDown || VkLButton () ) ) ? QuadBestQuality
//                                          : VkLButton () || VkRButton () || /*RButtonDown ||*/ AnimFx || AnimTF || dtrh < descaley                  ? QuadFastestQuality
//                                          :                                                                                                           QuadRegularQuality;

                    int         quality;
                    quality     = ! Outputing () && ( ( GetFocus () == GetHandle () && ( ( CurrentDisplaySpace != DisplaySpaceNone && VkLButton () ) || VkRButton () ) ) || AnimFx || AnimTF || dtrh < descaley )    ? QuadFastestQuality
                                : Outputing () || ( SelTracks.NumSet () <= 10 && SelTracks.NumSet () * numfreq * nump < 40000 )                                                             ? QuadBestQuality
                                :                                                                                                                                                             QuadRegularQuality;


                    GLfloat     v1[ 3 ];
                    GLfloat     v2[ 3 ];
                    v1[ 2 ] = 0;
                    v2[ 2 ] = 0;


                                        // Pixel-style, start
                    if ( IsIntensitySquareMode () ) {

                        v1[ 1 ]     = 0;

                        glBegin ( GL_QUADS );

                        for ( int f = firstfreq, f0 = 0; f <= lastfreq; f++, f0++ ) {

                            v2[ 1 ]     =   v1[ 1 ];
                            v1[ 1 ]     = ( f0 + 1 ) * dtrh;
                            scaling1    = Scaling ( f );

                            for ( *v1 = minp, *v2 = minp; *v1 <= maxp; ) {

                                ColorTable.GLize ( scaling1 * EegBuff ( f, st, *v1 ) );

                                glVertex3fv ( v1 ); glVertex3fv ( v2 );
                                (*v1)++; (*v2)++;
                                glVertex3fv ( v2 ); glVertex3fv ( v1 );
                                } // for v1
                            } // for f

                        glEnd();
                                        // Pixel-style, end
                        }

                                        // Quad-style, start
                    else { // if ( IsIntensitySmoothMode () ) {

                        QuadMesh.Begin ( maxp - minp + 1, quality, ColorTable );


                                        // margin below, half a frequency
                        int     f   = firstfreq;
                        int     f0;
                        v1[ 1 ]     = - ( firstfreq == lastfreq ? 1.0 : 0.5 ) * dtrh;
                        scaling1    = Scaling ( f );

                        for ( *v1 = minp; *v1 <= maxp; (*v1)++ )
                            QuadMesh.NextVertex ( v1, scaling1 * EegBuff ( f, st, *v1 ) );


                                        // in between frequencies
                        for ( f = firstfreq, f0 = 0; f <= lastfreq; f++, f0++ ) {

                            v1[ 1 ]     =   f0 * dtrh;
                            scaling1    = Scaling ( f );

                            for ( *v1 = minp; *v1 <= maxp; (*v1)++ ) {
    //                          if ( logmode )
    //                              val1 =   ( log ( fabs ( val1 ) + logstrength ) - log ( logstrength ) )
    //                                     / ( log ( ScalingLevel  + logstrength ) - log ( logstrength ) ) * ( val1 < 0 ? -1 : 1 );

                                QuadMesh.NextVertex ( v1, scaling1 * EegBuff ( f, st, *v1 ) );
                                } // for v1
                            } // for f


                                        // margin above, half a frequency
                        if ( firstfreq != lastfreq ) {
                            f           = lastfreq;
                            v1[ 1 ]     = ( numfreq - 0.5 ) * dtrh;
                            scaling1    = Scaling ( f );

                            for ( *v1 = minp; *v1 <= maxp; (*v1)++ )
                                QuadMesh.NextVertex ( v1, scaling1 * EegBuff ( f, st, *v1 ) );
                            }

                        QuadMesh.GLize ();
                        }


                    glTranslatef ( 0, yoff, 0 );

                                        // draw a box around selected track
                    if ( Highlighted[ st ] && trh / descaley > 5.0 ) {

                        glColor4f ( HighlightColor );
                        glLineWidth ( 2 );

                        glTranslatef ( 0, - 0.5 * numfreq * dtrh, 0 );

                        if ( IsIntensitySquareMode () )    maxp++;

                        glBegin ( GL_LINE_STRIP );
                        glVertex3f ( minp, 0, 0 );
                        glVertex3f ( maxp, 0, 0 );
                        glVertex3f ( maxp, numfreq * dtrh, 0 );
                        glVertex3f ( minp, numfreq * dtrh, 0 );
                        glVertex3f ( minp, 0, 0 );
                        glEnd ();

                        if ( IsIntensitySquareMode () )    maxp--;

                        glTranslatef ( 0,  0.5 * numfreq * dtrh, 0 );
                        glLineWidth ( 1 );
                        }


                    LineColor.GLize ( 0 );
                    } // IsIntensityModes ()

                } // FreqModeEF

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

            if ( IsModeFE () ) {        // frequencies by electrodes / time

                int     lastst;

                if ( CurrentDisplaySpace != DisplaySpaceNone )
                    glTranslatef ( 0, - trdelta.Y * ( numTracks * 0.5 - 0.5 ) * descaley, 0 );


                if      ( IsModeFEAvg () ) {    // 1 el or averaged n el

                    for ( int f = lastfreq; f >= firstfreq; f-- ) {

                        ColorFreqs.GLize ( (double) f );

                                        // draw name
                        if ( drawmargin && textenum && ! trackssuper && ( how & GLPaintOwner ) )
                            SFont->Print ( -4 * descalex, 0, 0, FreqDoc->GetFrequencyName ( f ), TA_RIGHT | TA_CENTERY );

                        if ( firstsel <= FreqDoc->GetLastRegularIndex() ) {
                            lastst = FreqDoc->GetLastRegularIndex();
                            numt   = sel->NumSet ();
                            }
                        else {          // don't average pseudo tracks
                            lastst = firstsel;
                            numt   = 1;
                            }

                        if ( (bool) Using )                         LineColor[ 0 ].GLize ();
                        else if ( numt == 1 ) {
                            if      ( Highlighted[ firstsel ] )     glColor4f ( HighlightColor );
                            else if ( BadTracks  [ firstsel ] )     glColor4f ( BadColor );
                            else if ( AuxTracks  [ firstsel ] )     glColor4f ( AuxColor );
                            }
                        else                                        ColorFreqs.GLize ( (double) f );


                        glBegin ( IsBarsMode () ? GL_QUADS : GL_LINE_STRIP );

                        for ( int t = minp; t <= maxp; t++ ) {
                            v = 0;      // average value done on-demand to avoid messing the buffers

                            for ( TIteratorSelectedForward sti ( *sel, firstsel ); (bool) sti && sti() <= lastst; ++sti )
                                v += EegBuff ( f, sti(), t );
                            v /= numt;

                            if ( IsBarsMode () ) {
                                glVertex2f ( t    , 0                 );
                                glVertex2f ( t    , Scaling ( f ) * v );
                                glVertex2f ( t + 1, Scaling ( f ) * v );
                                glVertex2f ( t + 1, 0                 );
                                }
                            else
                                glVertex2f ( t, Scaling ( f ) * v );
                            } // for t

                        glEnd ();

                        glTranslatef ( trdelta.X * descalex, trdelta.Y * descaley, trdelta.Z );
                        } // for f
                    }


                else if ( IsModeFEEnum () ) {

                    SubTrhFEEnum ();

                    if ( numt <= 1 )    trh = dtrh = 0;

                    for ( TIteratorSelectedForward fi ( toslot->SelFreqs ); (bool) fi; ++fi ) {

                        ColorFreqs.GLize ( (double) fi() );

                                        // draw name
                        if ( drawmargin && textenum && ! trackssuper && ( how & GLPaintOwner ) )
                            SFont->Print ( -4 * descalex, 0, 0, FreqDoc->GetFrequencyName ( fi() ), TA_RIGHT | TA_CENTERY );

                        glTranslatef ( 0, trh / 2 - dtrh, 0 );

                        if ( (bool) Using )                 LineColor[ 0 ].GLize ();

                        for ( TIteratorSelectedForward sti ( *sel, firstsel ); (bool) sti; ++sti ) {

                            if ( (bool) Using )                 ;
                            else if ( Highlighted[ sti() ] )    glColor4f ( HighlightColor );
                            else if ( BadTracks  [ sti() ] )    glColor4f ( BadColor );
                            else if ( AuxTracks  [ sti() ] )    glColor4f ( AuxColor );
                            else                                ColorFreqs.GLize ( (double) fi() );

                            glBegin ( IsBarsMode () ? GL_QUADS : GL_LINE_STRIP );

                            for ( int t = minp; t <= maxp; t++ )
                                if ( IsBarsMode () ) {
                                    glVertex2f ( t,     0                                             );
                                    glVertex2f ( t,     Scaling ( fi() ) * EegBuff ( fi(), sti(), t ) );
                                    glVertex2f ( t + 1, Scaling ( fi() ) * EegBuff ( fi(), sti(), t ) );
                                    glVertex2f ( t + 1, 0                                             );
                                    }
                                else
                                    glVertex2f ( t,     Scaling ( fi() ) * EegBuff ( fi(), sti(), t ) );

                            glEnd ();

                            glTranslatef ( 0, - dtrh, 0 );
                            } // for sti

                        glTranslatef ( trdelta.X * descalex, trdelta.Y * descaley + trh / 2, trdelta.Z );
                        } // for fi
                    }


                else if ( IsIntensityModes () ) {

//                  SubTrhFEInt ();

                    SubTrhFEEnum ();

                    double      scaling1;
//                    int         quality     = Outputing ()                                     ? QuadBestQuality
//                                            : dtrh / descaley >= 1.0                           ? QuadRegularQuality
//                                            :                                                    QuadFastestQuality;
                    int         quality     = Outputing () || ( SelTracks.NumSet () <= 10 && ( SelTracks.NumSet () * numfreq * nump < 40000 ) && !( RButtonDown || VkLButton () ) ) ? QuadBestQuality
                                            : VkLButton () || VkRButton () || /*RButtonDown ||*/ AnimFx || AnimTF || dtrh < descaley                  ? QuadFastestQuality
                                            :                                                                                                           QuadRegularQuality;

                    double      yoff        = ( trh - ( sel->NumSet () <= 1 ? 0 : dtrh ) ) / 2;


                    GLfloat     v1[ 3 ];
                    GLfloat     v2[ 3 ];
                    v1[ 2 ] = 0;
                    v2[ 2 ] = 0;


                    for ( TIteratorSelectedForward fi ( toslot->SelFreqs ); (bool) fi; ++fi ) {

                                        // special case: get auto-scaling for each frequency
                        if ( ScalingAuto == ScalingAutoAsymmetric && ! ( RButtonDown && IntensityOverride && MouseAxis == MouseAxisHorizontal ) ) {
                                                        // find min/max
                            minValue    =  DBL_MAX;
                            maxValue    = -DBL_MAX;

                                                // scan only what is in the current display
                            for ( TIteratorSelectedForward sti ( *sel, firstsel ); (bool) sti; ++sti ) {

//                              scalet  = ScaleTracks [ sti() ];
                                scalet  = 1;
                                notdiss = sti() != FreqDoc->GetDisIndex();

                                for ( int t = minp; t <= maxp; t++ ) {

                                    v =  Scaling ( fi() ) * EegBuff ( fi(), sti(), t );
                                    Maxed ( maxValue, v );
                                    Mined ( minValue, v );
                                    }
                                }


                            if ( maxValue < 0 )     maxValue = 0;
                            if ( minValue > 0 )     minValue = 0;

                                                        // to avoid some errors, just in case
                            if ( maxValue == minValue ) {
                                maxValue += 1e-6;
                                minValue -= 1e-6;
                                }

//                          if ( minValue == 0 )    minValue = -1e-12;
//                          if ( maxValue == 0 )    maxValue =  1e-12;

                            double          ctmin       = minValue;
                            double          ctmax       = maxValue;

                            ColorTable.SetParameters ( ctmax * ( IsIntensityPrecise () ? 1 : FSaturate + ScalingContrast * ( 1 - FSaturate ) ),
                                                       ctmin * ( IsIntensityPrecise () ? 1 : FSaturate + ScalingContrast * ( 1 - FSaturate ) ), 
                                                       contrast, 
                                                       alpha );
                            }


                                        // draw name
                        if ( drawmargin && textenum && ! trackssuper && ( how & GLPaintOwner ) ) {
                            ColorFreqs.GLize ( (double) fi() );
                            SFont->Print ( -4 * descalex, 0, 0, FreqDoc->GetFrequencyName ( fi() ), TA_RIGHT | TA_CENTERY );
                            }

                        glTranslatef ( 0, yoff, 0 );

                        scaling1 = Scaling ( fi() );

                                        // Pixel-style, start
                        if ( IsIntensitySquareMode () ) {

                            glBegin ( GL_QUADS );

                            for ( TIteratorSelectedForward sti ( *sel, firstsel ); (bool) sti; ++sti ) {

                                v1[ 1 ] = - ( sti.GetIndex ()     ) * dtrh;
                                v2[ 1 ] = - ( sti.GetIndex () + 1 ) * dtrh;

                                for ( *v1 = *v2 = minp; *v1 <= maxp; ) {

                                    ColorTable.GLize ( scaling1 * EegBuff ( fi(), sti(), *v1 ) );

                                    glVertex3fv ( v1 ); glVertex3fv ( v2 );
                                    (*v1)++; (*v2)++;
                                    glVertex3fv ( v2 ); glVertex3fv ( v1 );
                                    }

                                } // for sti

                            glEnd();
                                            // Pixel-style, end
                            }
                                        // Quad-style, start
                        else { // if ( IsIntensitySmoothMode () ) {
/*                                      // Horizontal strips - old code
                            for ( TIteratorSelectedForward sti ( *sel, firstsel ); (bool) sti; ++sti ) {

                                QuadStrip.Begin ( quality, ColorTable );

                                v1[ 1 ] = - ( sti.GetIndex ()     ) * dtrh;
                                v2[ 1 ] = - ( sti.GetIndex () + 1 ) * dtrh;

                                for ( *v1 = *v2 = minp; *v1 <= maxp; (*v1)++, (*v2)++ )
                                    QuadStrip.NextVertices ( v1, v2, scaling1 * EegBuff ( fi(), sti(), *v1 ), scaling1 * EegBuff ( fi(), sti(), *v1 ) );

                                QuadStrip.GLize ();
                                } // for sti
*/

                                        // QuadMesh
                            QuadMesh.Begin ( maxp - minp + 1, quality, ColorTable );

                                        // margin above, half a track
                            st          = firstsel;
                            int     s0  = -1;
                            v1[ 1 ] = - ( s0 + 0.5 + 0.5 ) * dtrh;
                            v2[ 1 ] = v1[ 1 ] - 0.5 * dtrh;

                            for ( *v1 = *v2 = minp; *v1 <= maxp; (*v1)++, (*v2)++ )
                                QuadMesh.NextVertex ( v1, scaling1 * EegBuff ( fi(), st, *v1 ) );

                            s0      = 0;
                            for ( TIteratorSelectedForward sti ( *sel, firstsel ); (bool) sti; ++sti, ++s0 ) {

                                v1[ 1 ] = - ( s0 + 0.5 ) * dtrh;
                                v2[ 1 ] = v1[ 1 ] - dtrh;

                                for ( *v1 = *v2 = minp; *v1 <= maxp; (*v1)++, (*v2)++ )
                                    QuadMesh.NextVertex ( v1, scaling1 * EegBuff ( fi(), sti(), *v1 ) );

                                } // for sti

                                        // margin below, half a track
                            st      = sel->LastSelected ();
                            s0--;
                            v1[ 1 ] = - ( s0 + 0.5 + 0.5 ) * dtrh;
                            v2[ 1 ] = v1[ 1 ] - 0.5 * dtrh;

                            for ( *v1 = *v2 = minp; *v1 <= maxp; (*v1)++, (*v2)++ )
                                QuadMesh.NextVertex ( v1, scaling1 * EegBuff ( fi(), st, *v1 ) );


                            QuadMesh.GLize ();
                            }


                        glTranslatef ( trdelta.X * descalex, trdelta.Y * descaley - yoff, trdelta.Z );
                        } // for fi

                    LineColor.GLize ( 0 );
                    } // IsIntensityModes ()

                                        // for a clean early exit of the big st loop
//              st = sel->LastSelected ();
                firsttrack = false;
                } // IsModeFE


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // restore line properties, anyway
            glLineWidth         ( 1.0 );
            GLLineSmoothOff     ();

            if ( how & GLPaintLinked )
                glDisable ( GL_LINE_STIPPLE );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the vertical grid - post
//            if ( drawvertscale && IsIntensityModes () /* && numTracks == 1 */ ) {

                                        // draw baseline axis - post
            if (   Show3DText 
              && ( ! trackssuper || firsttrack ) 
              && ( how & GLPaintOwner )
              && IsIntensityModes () ) {

                bool    baseline;       // if baseline has to be drawn
                bool    text;           // if text has to be displayed
                bool    alltext;        // test if display text and compact all names in 1 line
                int     step;
                double  yoff;

                glColor4f ( AxisColor );

                                        // can add case: trackssuper && tracksmode
                if      ( IsModeEF () && IsIntensityModes () ) {

//                  SubTrhEFInt;
//                  yoff        = ( dtrh - trh ) / 2;

                    SubTrhEFEnum ();

                                        // offset
                    yoff        = - ( trh / 2 - dtrh );


                    if ( draworigin ) {

//                      if ( IsIntensityModes () ) {

                            glTranslatef ( orgreltf, yoff - dtrh / 2 - ( numfreq == 1 ? 0.5 * trh : 0 ), 0 );
                            glLineWidth ( 2 );

                            glBegin ( GL_LINES );
                            glVertex2f ( descalex, 0 );
                            glVertex2f ( descalex, numfreq * dtrh );
                            glEnd ();

                            BackColor.GLize ();

                            glBegin ( GL_LINES );
                            glVertex2f ( -descalex, 0 );
                            glVertex2f ( -descalex, numfreq * dtrh );
                            glEnd ();

                            glColor4f ( AxisColor );
                            glLineWidth ( 1 );
                            glTranslatef ( -orgreltf, - ( yoff - dtrh / 2 - ( numfreq == 1 ? 0.5 * trh : 0 ) ), 0 );
                            }
/*
                        else if ( trackssuper ) { // tracks

                            glLineWidth ( 2 );

                            glBegin ( GL_LINES );
                            glVertex2f ( orgreltf + descalex,  4 * descaley );
                            glVertex2f ( orgreltf + descalex, -4 * descaley );
                            glEnd ();

                            BackColor.GLize ();

                            glBegin ( GL_LINES );
                            glVertex2f ( orgreltf - descalex,  4 * descaley );
                            glVertex2f ( orgreltf - descalex, -4 * descaley );
                            glEnd ();

                            glColor4f ( AxisColor );
                            glLineWidth ( 1 );
                            }
*/
                                        // draw the baseline if enough height
                    baseline    =  Show3DText 
                                && numfreq > 1 
                                && dtrh / descaley > 2 * MinBaselineH 
                                && dtrh / descaley > SFont->GetHeight();

                    text        =  Show3DText 
                                && enoughw 
                                && dtrh / descaley > SFont->GetHeight();

                    alltext     =  Show3DText 
                                && enoughw 
                                && ! text 
                                && firsttrack;

                    step        = (double) ( SFont->GetHeight() ) / ( ( dtrh ? dtrh : 1 ) / descaley ) + 0.75;
                                        // do a smart stepping
                    if ( step < 1 )   step = 1;
                    step       = Power ( 2, Round ( Log2 ( step ) ) ) + 0.5;
                    if ( step < 1 )   step = 1;


                    if ( alltext ) {
                        ColorFreqs.GLize ( (double) ( firstfreq + lastfreq ) / 2 );

                        StringCopy  ( buff, FreqDoc->GetFrequencyName ( firstfreq ), " - ", FreqDoc->GetFrequencyName ( lastfreq ) );

                        SFont->Print ( 3 * descalex, ( trh - dtrh ) / 2, trorg.Z, buff, TA_LEFT | TA_BOTTOM );
                        glColor4f ( AxisColor );

                        alltext = false;
                        }

                    dtrh *= step;


                    GLBlendOn ();
                    glColor4f ( AxisColor );

                    for ( int f = firstfreq, f0 = 0; f <= lastfreq && step; f+=step, f0+=step, yoff += dtrh ) {

                        if ( baseline ) {
                            glBegin ( GL_LINES );
                            glVertex2f ( minp,                                      yoff );
                            glVertex2f ( maxp + ( IsIntensitySquareMode () ? 1 : 0 ),  yoff );
                            glEnd ();
                            }

                        if ( text ) {
                            ColorFreqs.GLize ( (double) f );
                            SFont->Print ( 3 * descalex, yoff, trorg.Z, FreqDoc->GetFrequencyName ( f ), TA_LEFT | TA_BOTTOM );
                            glColor4f ( AxisColor );
                            }
                        } // for f
                    } // FreqModeEF


                else if ( IsModeFE () && IsIntensityModes () ) {

//                  SubTrhFEInt ();

                    SubTrhFEEnum ();

                                        // draw the baseline if emough height
                    baseline    = Show3DText && numt > 1 && dtrh / descaley > 2 * MinBaselineH && dtrh / descaley > SFont->GetHeight();
                    text        = Show3DText && enoughw && dtrh / descaley > SFont->GetHeight();
                    alltext     = Show3DText && enoughw && ! text && ( textenum || trackssuper );

                    step        = (double) ( SFont->GetHeight() ) / ( ( dtrh ? dtrh : 1 ) / descaley ) + 0.75;
                                        // do a smart stepping
                    if ( step < 1 )   step = 1;
                    step       = Power ( 2, Round ( Log2 ( step ) ) ) + 0.5;
                    if ( step < 1 )   step = 1;

                                        // re-position to first track
                    TSelection*     slotsel     = &toslot->SelTracks;
                    glTranslatef ( 0, - numTracks * trdelta.Y * descaley - ( slotsel->NumSet () <= 1 ? dtrh / 2 : dtrh ), 0 );


                    for ( TIteratorSelectedForward fi ( toslot->SelFreqs ); (bool) fi; ++fi ) {

                        if ( alltext ) {
                            slotsel->ToText ( buff, FreqDoc->GetElectrodesNames() );

                            glColor4f ( ThirdDimensionTextColor );
                            SFont->Print ( 3 * descalex, trh / 2 + dtrh / 2, trorg.Z, buff, TA_LEFT | TA_BOTTOM );
                            glColor4f ( AxisColor );

                            alltext = false;
                            }

                        glTranslatef ( 0, trh / 2, 0 );


                        if ( draworigin ) {
                            glLineWidth ( 2 );

                            glBegin ( GL_LINES );
                            glVertex2f ( orgreltf + descalex, 0.5 * dtrh );
                            glVertex2f ( orgreltf + descalex, ( 0.5 - numt ) * dtrh );
                            glEnd ();

                            BackColor.GLize ();

                            glBegin ( GL_LINES );
                            glVertex2f ( orgreltf - descalex, 0.5 * dtrh );
                            glVertex2f ( orgreltf - descalex, ( 0.5 - numt ) * dtrh );
                            glEnd ();

                            glColor4f ( AxisColor );
                            glLineWidth ( 1 );
                            }


                        GLBlendOn ();
                        glColor4f ( AxisColor );

                        yoff    = 0;
                        for ( int sti = 0, st = slotsel->GetValue ( sti ); sti < slotsel->NumSet (); sti++, st = slotsel->GetValue ( sti ), yoff -= step * dtrh ) {

                            if ( baseline && yoff ) {
                                glBegin ( GL_LINES );
                                glVertex2f ( minp,                                     yoff + dtrh / 2 );
                                glVertex2f ( maxp + ( IsIntensitySquareMode () ? 1 : 0 ), yoff + dtrh / 2 );
                                glEnd ();
                                }

                            if ( text ) {

                                if ( (bool) Using )             ;
                                else if ( Highlighted[ st ] )   glColor4f ( HighlightColor );
                                else if ( BadTracks  [ st ] )   glColor4f ( BadColor );
                                else if ( AuxTracks  [ st ] )   glColor4f ( AuxColor );
                                else                            glColor4f ( ThirdDimensionTextColor );

//                              glColor4f ( ThirdDimensionTextColor );
                                SFont->Print ( 3 * descalex, yoff, trorg.Z, FreqDoc->GetElectrodeName ( st ), TA_LEFT | /*TA_BOTTOM*/ TA_CENTERY );
                                glColor4f ( AxisColor );
                                }
                            } // for sti

                        glTranslatef ( trdelta.X * descalex, trdelta.Y * descaley - trh / 2, trdelta.Z );
                        } // for fi


//                  st = sel->LastSelected ();
//                  firsttrack = false;
                    } // FreqModeFE

                } // drawvertscale - post


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

            if (   ! trackssuper 
                && ! IsModeFEAvg () )   // translation accounts for the scaling

                glTranslatef ( trdelta.X * descalex, trdelta.Y * descaley, trdelta.Z );


            glScaled ( 1, 1 / scalet, 1 );
            descaley    = 1 / scalev;


            if ( IsModeFE () )
                break;                  // force quit the tracks loop
            } // for sti

                                        // restore previous referential
//      glScaled ( descalex, descaley, 1 );             // de-scale
//      glTranslatef ( -trorg.X, -trorg.Y, -trorg.Z );  // de-translate
        st      = sel->LastSelected ();                 // set the valid last st


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                                        // last name when summarizing
        if ( drawmargin && ( !textenum || trackssuper ) && ( how & GLPaintOwner ) ) {
                                        // select color (copy of further code)
            if      ( Highlighted[ st ] )   glColor4f ( HighlightColor );
            else if ( BadTracks  [ st ] )   glColor4f ( BadColor );
            else if ( st >= numel )         glColor4f ( PseudotrackColor );
            else if ( AuxTracks  [ st ] )   glColor4f ( AuxColor );
                 else                       glColor4f ( TrackColor );

            const char*     ton = IsModeFE () ? FreqDoc->GetFrequencyName ( firstfreq ) : GetElectrodeName ( sel->LastSelected () );

            if ( IsModeFE () )
                ColorFreqs.GLize ( (double) firstfreq );

            if ( ! trackssuper )         // go back 1 track above
                glTranslatef ( -trdelta.X * descalex, -trdelta.Y * descaley, -trdelta.Z );
            else
                glTranslatef ( 0, - SFont->GetHeight() / 2 * descaley , 0 );

            if ( numTracks > 1 )
                SFont->Print ( -4 * descalex, 0, 0, ton, TA_RIGHT | TA_CENTERY );
            }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // restore as when entering current slot
        glPopMatrix ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the horizontal grid - post
        if ( drawhoriscale 
          && ( IsModeSpectrum () && firsttrack || ! IsTracksMode () )
          && ( how & GLPaintOwner ) ) {

            double  x, y1, y2, z;

            glColor4f ( AxisColor );

            y1  = toslot->LowLeft.Y;
            y2  = toslot->LowLeft.Y + toslot->ToUp.Y;
            z   = toslot->LowLeft.Z;

            if ( IsFreqHorizontal () ) {

                glBegin ( GL_LINES );

                for ( int f = hgridorg; f <= maxp; f += hgridstep ) {
                    x = trorg.X + f / descalex;
                    glVertex3f ( x, y1, z );
                    glVertex3f ( x, y2, z );
                    }

                glEnd ();

                for ( int f = hgridorg; f <= maxp; f += hgridstep )
                    SFont->Print ( trorg.X + f / descalex + 3, toslot->LowLeft.Y, toslot->LowLeft.Z, FreqDoc->GetFrequencyName ( f ), TA_LEFT | TA_BOTTOM );
                }

            else if ( IsTimeHorizontal () ) {

//              char    stf  [16];
//              char    stime[16];

                glBegin ( GL_LINES );

                for ( double g = hgridorg - CDPt.GetMin(); g < CDPt.GetLength(); g += hgridstep ) {
                    x = trorg.X + g / descalex;
                    glVertex3f ( x, y1, z );
                    glVertex3f ( x, y2, z );
                    }

                glEnd ();

                for ( double g = hgridorg - CDPt.GetMin(); g < CDPt.GetLength(); g += hgridstep ) {

                    TFToString ( g + CDPt.GetMin(), stf, stime, ShowHorizScaleLegal );

                    SFont->Print ( trorg.X + g / descalex + 3, y1, z, *stime ? stime : stf, TA_LEFT | TA_BOTTOM );
                    }
                }
            } // drawhoriscale - post

        } // window slots


    GLLinesModeOff      ( false );
//  glLineWidth ( 1.0 );
    } // GLPaintOpaque


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                                        // be smart: allow a window linking to this
                                        // to receive the linked window from here
                                        // allowing hierarchical links
if ( (int) Using && ( how & GLPaintOpaque ) && !NestedPaint ) {

    NestedPaint = true;
                                        // paint the others (EEG only), in the opposite manner as usual
                                        // Substitute to own EEG pointer the others EEGs pointers, in sequence,
                                        // load data, and draw as "normal", by calling the local GLPaint (and not the remote ones)
                                        // then restore original EEG pointer and data. Not so efficient, but VERY convenient, safe, and fast enough

    TFreqDoc*       FreqDocSave     = FreqDoc;

    foreachin ( Using, iteratorview ) {

        FreqDoc = dynamic_cast< TFreqDoc* > ( & iteratorview ()->GetDocument() );

        ReloadBuffers ();               // load all data in my buffer
                                        // then paint it from here
        GLPaint ( GLPaintLinked | GLPaintOpaque, RenderingUnchanged, clipplane );
        }
                                        // currently, transparency is not used, so avoid calling for nothing

                                        // restore our pointer and reload our data
    FreqDoc = FreqDocSave;

    ReloadBuffers ();

    NestedPaint = false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // allow drawing the cursor in external linked window
if (   ! Outputing () 
    && ! NestedPaint 
    && ( how & GLPaintTransparent ) )

    DrawTFCursor ( false, false );
}


//----------------------------------------------------------------------------
void    TFrequenciesView::Paint ( TDC &dc, bool /*erase*/, TRect &rect )
{
//if ( CartoolApplication->Closing )
//    return;

double  clipmax = CurrentDisplaySpace == DisplaySpaceNone ? 0 : XYZDoc->GetBounding ( CurrentDisplaySpace )->MaxSize() * ( CurrentDisplaySpace != DisplaySpace3D ? 0.475 : 0.525 );

PrePaint ( dc, rect, clipmax, clipmax, clipmax );
//PrePaint ( dc, rect, clipmax, clipmax, CurrentDisplaySpace != DisplaySpace3D ? -ProjModelRadius : -ModelRadius );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

glLightModeli ( GL_LIGHT_MODEL_TWO_SIDE, 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( CurrentDisplaySpace == DisplaySpaceNone ) {
    // already done
    }
else {
    glTranslated ( 0, 0, ModelRadius );

    ModelRotMatrix.GLize ();

    glTranslated ( -ModelCenter.X, -ModelCenter.Y, -ModelCenter.Z );
    }

                                        // save this matrix for external drawings
MatProjection.CopyProjection ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( CurrentDisplaySpace == DisplaySpace3D ) {
    Fog.SetColor( BackColor[ Outputing() ] );
    Fog.GLize();                        // my fog
    }
else
    GLFogOff        ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLPaint ( GLPaintNormal, RenderingUnchanged, ClipPlane );
//AntialiasingPaint ();


DepthRange.unGLize ();                  // reset depth range
Fog.unGLize();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // switch to window coordinates
if ( NotSmallWindow () ) {

    SetWindowCoordinates ();

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*    if ( DrawMode3DIcon ) {
        #define     iconsize    20

        GLfloat     icony   = (double) PaintRect.Height() - iconsize - 5;
        GLfloat     iconx   = ( 50.0 - iconsize )  / 2;
        GLfloat     iconz   =  -0.5;

        glLineWidth ( 3.0 );


        if ( IsFreqHorizontal () )              glColor3f ( 0.00, 0.00, 1.00 );
        else /* if ( IsTimeHorizontal () ) * /  glColor3f ( 1.00, 0.00, 0.00 );

        glBegin ( GL_LINES );
        glVertex3f ( iconx,            icony,           iconz );
        glVertex3f ( iconx + iconsize, icony,           iconz );
        glEnd ();


        glColor3f ( 1.00, 1.00, 0.00 );
        glBegin ( GL_LINES );
        glVertex3f ( iconx,            icony + iconsize * 0.25,           iconz );
        glVertex3f ( iconx + iconsize, icony + iconsize * 0.25,           iconz );
        glVertex3f ( iconx,            icony + iconsize * 0.50,           iconz );
        glVertex3f ( iconx + iconsize, icony + iconsize * 0.50,           iconz );
        glVertex3f ( iconx,            icony + iconsize * 0.75,           iconz );
        glVertex3f ( iconx + iconsize, icony + iconsize * 0.75,           iconz );
        glEnd ();


        if ( IsFreqVertical () )                glColor3f ( 0.00, 0.00, 1.00 );
        else /* if ( IsTimeHorizontal () ) * /  glColor3f ( 1.00, 0.00, 0.00 );

        glBegin ( GL_LINES );
        glVertex3f ( iconx,            icony,            iconz );
        glVertex3f ( iconx           , icony + iconsize, iconz );
        glEnd ();


        glLineWidth ( 1.0 );
        }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // ?not sure why this part does not call ColorTable.Draw?
    if ( HasColorScale () ) {
                                        // ScaleTracks is not used in color mode, so the color scale is correct for all tracks here

        TGLColor<GLfloat>   glcol;
        int                 colormax        = ColorTable.GetMaxIndex ();
        int                 color0          = ColorTable.GetZeroIndex ();
    //  int                 colordelta      = ColorTable.GetDeltaIndex ();
        int                 GalMinIndex;
        int                 GalMaxIndex;

        GLfloat             scaleh          = NoMore ( (int) ( (double) PaintRect.Height() / 2 ), ColorMapHeight );
        GLfloat             scalew          = ColorMapWidth;

        GLfloat             scaleox         = (int) ( (double) PaintRect.Width() - scalew );
        GLfloat             scaleoy         = (int) ( (double) PaintRect.Height() / 2 - scaleh / 2 );
        GLfloat             scaleoz         =  -0.5;        // furthest back

                                            // draw background
        glBegin ( GL_QUADS );
        glColor3f  ( 0.0, 0.0, 0.0 );
        glVertex3f ( scaleox,          scaleoy,          scaleoz );
        glVertex3f ( scaleox + scalew, scaleoy,          scaleoz );
        glVertex3f ( scaleox + scalew, scaleoy + scaleh, scaleoz );
        glVertex3f ( scaleox         , scaleoy + scaleh, scaleoz );

        scaleox += 2;   scaleoy += 2;
        scalew  -= 4;   scaleh  -= 4;
        glEnd();

                                            // draw color scale
        double  ci0 = color0 * scaleh / colormax;

        glBegin ( GL_QUAD_STRIP );
        for ( int ci=0; ci <= scaleh; ci++ ) {
            if ( ci < ci0 )
                ColorTable.GLize ( ColorTable.GetNMax () * ( ci - ci0 ) /          - ci0   );
            else
                ColorTable.GLize ( ColorTable.GetPMax () * ( ci - ci0 ) / ( scaleh - ci0 ) );   // colors visually correct - not scaling

            glVertex3f ( scaleox,          scaleoy + ci,    scaleoz );
            glVertex3f ( scaleox + scalew, scaleoy + ci,    scaleoz );
            }
        glEnd();


        if ( ! ( ScalingAuto == ScalingAutoAsymmetric || FreqNormalize ) ) {
                                            // draw the min/max positions
            GalMaxIndex = ColorTable.GetIndex ( GalMaxValue );  
            Clipped ( GalMaxIndex, 0, colormax );

            GalMinIndex = ColorTable.GetIndex ( GalMinValue );
            Clipped ( GalMinIndex, 0, colormax );


            glEnable  ( GL_COLOR_LOGIC_OP );
            glLogicOp ( GL_OR_REVERSE );

            double  ipmin, ipmax;
            glBegin ( GL_QUADS );

            glColor3f ( 0, 0, 0 );

//          if ( ! FreqDoc->IsAbsolute ( AtomTypeUseCurrent ) ) {
                ipmin  = GalMinIndex * scaleh / colormax;
                glVertex3f ( scaleox,          scaleoy + ipmin + 2,scaleoz );
                glVertex3f ( scaleox + scalew, scaleoy + ipmin + 2,scaleoz );
                glVertex3f ( scaleox + scalew, scaleoy + ipmin,    scaleoz );
                glVertex3f ( scaleox,          scaleoy + ipmin,    scaleoz );
//              }

            ipmax  = GalMaxIndex * scaleh / colormax;
            glVertex3f ( scaleox,          scaleoy + ipmax - 2,scaleoz );
            glVertex3f ( scaleox + scalew, scaleoy + ipmax - 2,scaleoz );
            glVertex3f ( scaleox + scalew, scaleoy + ipmax,    scaleoz );
            glVertex3f ( scaleox,          scaleoy + ipmax,    scaleoz );
            glEnd();

                                            // print the min/max values
            char                mindata     [ 16 ];
            char                maxdata     [ 16 ];
            char                mindisplay  [ 16 ];
            char                maxdisplay  [ 16 ];

            FloatToString   ( mindata, GalMinValue );
            FloatToString   ( maxdata, GalMaxValue );


            glColor3f ( 0, 0, 0 );
//          TextColor.GLize ( Outputing() ? 1 : 0 );
                                            // print at variable position
//          if ( ! FreqDoc->IsAbsolute ( AtomTypeUseCurrent ) )
                SFont->Print ( scaleox + scalew, scaleoy - 4 + ipmin, scaleoz, mindata, TA_RIGHT | TA_TOP );
            SFont->Print ( scaleox + scalew, scaleoy + 4 + ipmax, scaleoz, maxdata, TA_RIGHT | TA_BOTTOM );

            glDisable ( GL_COLOR_LOGIC_OP );


            if ( ScalingAuto == ScalingAutoOff ) {

                FloatToString   ( mindisplay, (double) ColorTable.GetNMax () );
                FloatToString   ( maxdisplay, (double) ColorTable.GetPMax () );
                                            // print at fixed position
                if ( ! FreqDoc->IsAbsolute ( AtomTypeUseCurrent ) ) 
                    SFont->Print ( scaleox + scalew, scaleoy - 4          - SFont->GetHeight(), scaleoz, mindisplay, TA_RIGHT | TA_TOP );
                SFont->Print ( scaleox + scalew, scaleoy + 4 + scaleh + SFont->GetHeight(), scaleoz, maxdisplay, TA_RIGHT | TA_BOTTOM );
                }

            } // scaling correct

        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw orientation
    if ( CurrentDisplaySpace != DisplaySpaceNone && ShowOrientation )

        DrawOrientation ( CurrentDisplaySpace != DisplaySpace3D ? XYZDoc->GetProjBoxSides() : XYZDoc->GetBoxSides() );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    ResetWindowCoordinates ();
    } // window mode


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

PostPaint ( dc );


UpdateCaption ();
}


//----------------------------------------------------------------------------
                                        // either draws or erases the cursor (XOR operation)
void    TFrequenciesView::DrawTFCursor ( bool undrawold, bool localhdc )
{
                                        // no cursors in these modes
if ( IsModeSpectrumEnum () 
  && ! (    HasWindowSlots () 
         && CurrentDisplaySpace == DisplaySpaceNone 
         && WindowSlots[ 0 ].ToRight.Norm () >= FREQGLVIEW_EF_MINSIZECURSOR 
        ) )

    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( localhdc ) {                       // make local context
                                        // bypass a very weird bug
    if ( CurrentDisplaySpace != DisplaySpaceNone ) {
        ShowNow ();
        return;
        }


    if ( DrawTFCursorDC == 0 )
        DrawTFCursorDC      = new UseThisDC ( *this );

    GLrc.GLize ( *DrawTFCursorDC );

                                        // set again, by security
    GLSmoothEdgesOff();
    GLColoringOn    ();
    GLWriteDepthOff ();
    glLineWidth ( 1.0 );


//  glPixelTransferi ( GL_MAP_COLOR, GL_FALSE );
//  glPixelZoom ( 1, 1 );

                                        // copy color buffer FRONT to BACK
    glReadBuffer ( GL_FRONT );
    glDrawBuffer ( GL_BACK  );
                                        // set the origin of destination copy
    glRasterPos3f ( 0, 0, 0 );

    glCopyPixels ( ViewportOrgSize[0], ViewportOrgSize[1], ViewportOrgSize[2], ViewportOrgSize[3], GL_COLOR );
                                        // restore to regular setting
    glReadBuffer ( GL_BACK  );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

long                minp;
long                maxp;
long                nump;
long                posmin;
long                posmax;

                                        // get horizontal range
GetRelHorizRange ( minp, maxp, nump );


if ( IsFreqHorizontal () ) {
    FCursor.GetPos ( posmin, posmax );
    posmin -= CDPf.GetMin();
    posmax -= CDPf.GetMin();

//  glColor4f ( AntiFreqCursorColor );
    }
else {
    TFCursor.GetPos ( posmin, posmax );
    posmin -= CDPt.GetMin();
    posmax -= CDPt.GetMin();

//  glColor4f ( AntiCursorColor );
    }

                                        // boundary check: keep inside visible part
Clipped ( posmin, minp, maxp );
Clipped ( posmax, minp, maxp );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! localhdc ) {
    GLColoringOn    ();
    GLWriteDepthOff ();
    }

glLogicOp ( GL_XOR );
glEnable  ( GL_COLOR_LOGIC_OP );

                                        // draw a hollow cursor, to not alter the colors
//if ( IsIntensityModes () )  glPolygonMode ( GL_FRONT_AND_BACK, GL_LINE );

//if ( localhdc && CurrentDisplaySpace != DisplaySpaceNone )
//    Fog.GLize();


if ( CurrentDisplaySpace != DisplaySpaceNone ) {
    glLoadIdentity();
                                        // redo the shift
//  glTranslated ( 0, 0, -ModelRadius );
    }
//else
//  MatModelView. GLize ( ReplaceModelView ); // not really needed


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GLfloat             fromx;
GLfloat             tox;
GLfloat             fromy;
GLfloat             toy;
GLfloat             fromz;

                                        // lambda function, used twice here: to erase previous cursor, then to draw the new one
auto        drawcursor      = [&]   (   const long&     posmin,     const long&     posmax,     // TF values
                                        const GLfloat&  fromx,      const GLfloat&  tox         // OpenGL axis positions
                                    )
    {
                                        // Same TF position, no matter how are the OpenGL positions?
    if ( posmin == posmax && ! OneMoreHoriz () ) {

//      if ( thin ) {
//          glBegin ( GL_LINES );
//          glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
//          glEnd();
//          }
//      else { // thick
            glBegin ( GL_QUADS );
            glVertex3f ( fromx + 1, fromy, fromz );     glVertex3f ( fromx + 1, toy,   fromz );
            glVertex3f ( fromx - 1, toy,   fromz );     glVertex3f ( fromx - 1, fromy, fromz );
            glEnd();
//          }
        }
                                        // Not the same TF positions - Drawing will depend on the OpenGL values
    else {

        GLfloat             v1[ 3 ];
        GLfloat             v2[ 3 ];

        v1[ 0 ] = fromx;
        v2[ 0 ] = tox;
        v1[ 1 ] = v2[ 1 ] = fromy;
        v1[ 2 ] = v2[ 2 ] = fromz;

        GLGetScreenCoord ( v1, ViewportOrgSize, MatProjection );
        GLGetScreenCoord ( v2, ViewportOrgSize, MatProjection );

                                        // Less than a voxel wide?
        if ( v2[ 0 ] - v1[ 0 ] < 1 ) {

            glBegin ( GL_LINES );
            glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
            glEnd ();
            } // if less 1 voxel wide
                                        // More than a voxel wide
        else {

            glPolygonMode ( GL_FRONT_AND_BACK, GL_LINE );

            glBegin ( GL_QUADS );
            glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
            glVertex3f ( tox,   toy,   fromz );     glVertex3f ( tox,   fromy, fromz );
            glEnd();

            glPolygonMode ( GL_FRONT_AND_BACK, GL_FILL );

                                        // filling box?
            if ( CurrentDisplaySpace == DisplaySpaceNone && ! IsIntensityModes ()
                && v2[ 0 ] - v1[ 0 ] > 1 /*&& IsFilling != FillingColor*/ ) {

                if ( IsFreqHorizontal () )  glColor4f ( AntiFreqCursorFillColor );
                else                        glColor4f ( AntiCursorFillColor     );

                glBegin ( GL_QUADS );
                glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
                glVertex3f ( tox,   toy,   fromz );     glVertex3f ( tox,   fromy, fromz );
                glEnd();
                } // filling box

            } // more 1 voxel wide

        } // not same TF positions

    }; // drawcursor

/*                                      // old way(?)
        if ( CurrentDisplaySpace == DisplaySpaceNone  ) {

            glPolygonMode ( GL_FRONT_AND_BACK, GL_LINE );

            glBegin ( GL_QUADS );
            glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
            glVertex3f ( tox,   toy,   fromz );     glVertex3f ( tox,   fromy, fromz );
            glEnd ();

            glPolygonMode ( GL_FRONT_AND_BACK, GL_FILL );

            if ( ! IsIntensityModes () ) {

                glColor4f ( AntiCursorFillColor );

                glBegin ( GL_QUADS );
                glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
                glVertex3f ( tox,   toy,   fromz );     glVertex3f ( tox,   fromy, fromz );
                glEnd ();
                }
            }
        else {

            GLfloat             v1[ 3 ];
            GLfloat             v2[ 3 ];

            v1[ 0 ] = fromx;
            v2[ 0 ] = tox;
            v1[ 1 ] = v2[ 1 ] = fromy;
            v1[ 2 ] = v2[ 2 ] = fromz;

            GLGetScreenCoord ( v1, ViewportOrgSize, MatProjection );
            GLGetScreenCoord ( v2, ViewportOrgSize, MatProjection );


            if ( v2[ 0 ] - v1[ 0 ] < 1 ) {
                glBegin ( GL_LINES );
                glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
                glEnd ();
                }
//          else {
//              glBegin ( GL_QUADS );
//              glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
//              glVertex3f ( tox,   toy,   fromz );     glVertex3f ( tox,   fromy, fromz );
//              glEnd ();
//              }
   
            else {
                glPolygonMode ( GL_FRONT_AND_BACK, GL_LINE );

                glBegin ( GL_QUADS );
                glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
                glVertex3f ( tox,   toy,   fromz );     glVertex3f ( tox,   fromy, fromz );
                glEnd ();

                glPolygonMode ( GL_FRONT_AND_BACK, GL_FILL );

                if ( ! IsIntensityModes () ) {

                    glColor4f ( AntiCursorFillColor );

                    glBegin ( GL_QUADS );
                    glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
                    glVertex3f ( tox,   toy,   fromz );     glVertex3f ( tox,   fromy, fromz );
                    glEnd ();
                    }
                }
            }
*/


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TTracksViewWindowSlot*      toslot;
TGLCoordinates<GLfloat>     trorg;
double                      scaleh;
GLfloat                     cursh;
//bool                      solid           = IsBarsMode () || IsIntensitySquareMode ();


for ( int slot = 0; slot < GetNumWindowSlots (); slot++ ) {

    toslot      = &WindowSlots[ slot ];
    trorg       = toslot->TrOrg;
    scaleh      = toslot->ScaleH;
    cursh       = toslot->ToUp.Norm();

    if ( CurrentDisplaySpace == DisplaySpaceNone ) {
        fromy       = toslot->LowLeft.Y;
        toy         = toslot->LowLeft.Y + cursh;
        fromz       = toslot->LowLeft.Z;
        }
    else {
        fromy       = trorg.Y - cursh * CursorHeightFactor3D * 0.5;
        toy         = trorg.Y + cursh * CursorHeightFactor3D * 0.5;
//      fromy       = trorg.Y;
//      toy         = trorg.Y + cursh * 1.5;
        fromz       = trorg.Z;
                                        // skip back cursors, as it does not fog but Xors
        if ( CurrentDisplaySpace == DisplaySpace3D && fromz < ModelRadius )    continue;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // undraw previously drawn cursor?
    if ( undrawold && LastCursorPosMin >= 0 ) {

//      fromx = trorg.X + ( LastCursorPosMin + ( solid && LastCursorPosMin == LastCursorPosMax ? 0.5 : 0 )     ) * scaleh;
//      tox   = trorg.X + ( LastCursorPosMax + ( solid  ? LastCursorPosMin == LastCursorPosMax ? 0.5 : 1 : 0 ) ) * scaleh + ( CurrentDisplaySpace == DisplaySpaceNone && ! solid ? 1 : 0 );

        fromx = trorg.X + ( LastCursorPosMin                             ) * scaleh;
        tox   = trorg.X + ( LastCursorPosMax + ( OneMoreHoriz ()  ? 1 : 0 ) ) * scaleh; //  + ( CurrentDisplaySpace == DisplaySpaceNone && ! OneMoreHoriz () ? 1 : 0 );


        if ( IsFreqHorizontal () )  glColor4f ( AntiFreqCursorColor );
        else                        glColor4f ( AntiCursorColor     );


        drawcursor ( LastCursorPosMin, LastCursorPosMax, fromx, tox );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // now draw the current cursor position
//  fromx = trorg.X + ( posmin + ( solid && posmin == posmax ? 0.5 : 0 )     ) * scaleh;
//  tox   = trorg.X + ( posmax + ( solid  ? posmin == posmax ? 0.5 : 1 : 0 ) ) * scaleh + ( CurrentDisplaySpace == DisplaySpaceNone && ! solid ? 1 : 0 );

    fromx = trorg.X + ( posmin                             ) * scaleh;
    tox   = trorg.X + ( posmax + ( OneMoreHoriz ()  ? 1 : 0 ) ) * scaleh; //  + ( CurrentDisplaySpace == DisplaySpaceNone && ! OneMoreHoriz () ? 1 : 0 );


    if ( IsFreqHorizontal () )  glColor4f ( AntiFreqCursorColor );
    else                        glColor4f ( AntiCursorColor     );


    drawcursor ( posmin, posmax, fromx, tox );
    } // for GetNumWindowSlots ()


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // for next undraw
LastCursorPosMin    = posmin;
LastCursorPosMax    = posmax;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // restore default
//if ( IsIntensityModes () )  glPolygonMode ( GL_FRONT_AND_BACK, GL_FILL );

glDisable ( GL_COLOR_LOGIC_OP );
GLColoringOff   ();
GLWriteDepthOn  ();


if ( localhdc ) {                       // send to front & finish

//  glFinish ();

    SwapBuffers ( *DrawTFCursorDC );

    GLrc.unGLize ();

//  delete  DrawTFCursorDC;             // give back the DC?
//  DrawTFCursorDC  = 0;
    }


UpdateCaption ();
}


//----------------------------------------------------------------------------
                                        // Current trick for all frequencies at once: Shift, F, + then Shift again
void    TFrequenciesView::EvKeyDown ( uint key, uint repeatCount, uint flags )
{
                                        // set common keypressed first
TBaseView::EvKeyDown ( key, repeatCount, flags );


switch ( key ) {
    case '1':
        CmSetFrequencyMode ( IDB_FREQDISPLAY_SPECTRUM );
        break;

    case '2':
        CmSetFrequencyMode ( IDB_FREQDISPLAY_EF );
        break;

    case '3':
        CmSetFrequencyMode ( IDB_FREQDISPLAY_FE );
        break;

    case 'A':
        if ( IsModeSpectrum () || ! IsIntensityModes () )
            CmSetFrequencyMode ( IDB_AVERAGETRACKS );
        break;

    case 'D':
        if ( IsIntensityModes () )
            CmSetScalingAdapt ();
        break;

    case 'N':
        CmSetFreqNormalize ();
        break;

    case HotKeyFrequencies:
        FrequenciesOverride = true;
        break;

    case HotKeyIntensity:
        CmSetFrequencyMode ( IDB_DISPLAYINTENSITY );
        break;

    case VK_ADD:                        // VK_ADD doesn't seem to fire when Shift and F are also pressed?!

        if ( FrequenciesOverride )  CmChangeNumFreqs ( IDB_MOREFREQS );
        else                        TTracksView::EvKeyDown ( key, repeatCount, flags );
        break;

    case VK_SUBTRACT:

        if ( FrequenciesOverride )  CmChangeNumFreqs ( IDB_LESSFREQS );
        else                        TTracksView::EvKeyDown ( key, repeatCount, flags );
        break;


    default:
        TTracksView::EvKeyDown ( key, repeatCount, flags );
        break;
    }
}


void    TFrequenciesView::EvKeyUp ( uint key, uint repeatCount, uint flags )
{
                                        // reset common keypressed first
TBaseView::EvKeyUp ( key, repeatCount, flags );


switch ( key ) {

    case HotKeyFrequencies:
        FrequenciesOverride = false;
        break;

    default:
        TTracksView::EvKeyUp ( key, repeatCount, flags );
        break;
    }
}


//----------------------------------------------------------------------------
void    TFrequenciesView::EvLButtonDown ( uint i, const TPoint &p )
{
MousePos            = p;
LastMouseMove       = TPoint ( 0, 0 );


if      ( CaptureMode == CaptureGLLink ) {

    TBaseView*      view    = ClientToBaseView ( p );

    if ( ! view ) {
        Cm2Object ();                   // clean-up
        return;
        }

    if ( view->GetViewId () != GetViewId () ) {
                                        // a Tracks view?
        if ( ! dynamic_cast<TFrequenciesView*> ( view ) )
            return;
                                        // already pointed to?
        if ( Using.IsInside ( view ) ) {

            view->UsedBy.Remove ( this, DontDeallocate );
            Using       .Remove ( view, DontDeallocate );
            Invalidate ( false );
            }
        else {
            if ( ! UsedBy.IsInside ( view ) ) {

                TFreqDoc*       freq2       = dynamic_cast<TFreqDoc*> ( view->BaseDoc );

                if ( freq2
                  && freq2->GetTotalElectrodes()   == EEGDoc->GetTotalElectrodes()
                  && freq2->GetNumTimeFrames()     == EEGDoc->GetNumTimeFrames()
                  && FreqDoc->GetNumFrequencies () == freq2->GetNumFrequencies () ) {

//                    && freq2->GetSamplingFrequency() == freqDoc->GetSamplingFrequency() ) {

                    view->UsedBy.Append ( this );
                    Using.Append ( view );
                    Invalidate ( false );
                    }
                else {
                    Cm2Object ();       // finish
                    ShowMessage ( "Frequency file does not match, either by its number of electrodes\nor frequencies, or its duration.", "Linking error", ShowMessageWarning );
                    }
                }
            else {
                Cm2Object ();           // finish
                ShowMessage ( "Can not create a loop of links.", "Linking error", ShowMessageWarning );
                }
            }
        }
    else {
        Cm2Object ();                   // clean-up

        TListIterator<TBaseView>    iteratorview;

        foreachin ( Using, iteratorview )
            iteratorview ()->UsedBy.Remove ( this, DontDeallocate );

        Using.Reset ( DontDeallocate );
        Invalidate ( false );
        }
    }

else if ( CurrentDisplaySpace == DisplaySpaceNone 
       && CaptureMode == CaptureNone 
       && ! ControlKey
       && IsFreqHorizontal () ) {

    TTracksViewWindowSlot*  toslot;
    long                tf;
    CurrentWindowSlot   = toslot = ClosestWindowSlot ( p.X(), GetClientRect().Height() - p.Y() );

    if ( toslot && toslot->ToRight.Norm() >= FREQGLVIEW_EF_MINSIZECURSOR ) {

        tf      = (double) ( p.X () - toslot->TrOrg.X ) / toslot->ScaleH - ( IsBarsMode () || IsIntensitySquareMode () ? 0.5 : 0 ) + 0.5;

        if      ( tf < 0                        )   tf  = CDP->GetMin();
        else if ( tf > (long) CDP->GetLength()  )   tf  = CDP->GetMax();
        else                                        tf += CDP->GetMin();

/*
        if ( SyncCDPtTFCursor && IsTimeHorizontal () ) {
            TFCursor.StopExtending ();
                                        // don't modify the length, just move it, et basta!
            TFCursor.ShiftPos ( pos - TFCursor.GetPosMiddle () );
            CheckCDPtAndTFCursor ( true );

            Invalidate ( false );
            return;
            }
        else if ( IsFreqHorizontal () ) {*/
            FCursor.StopExtending ();
            FCursor.SetFixedPos ( tf );
//          }

//      DrawTFCursor ( true );

        ButtonGadgetSetState ( IDB_RANGECURSOR, FCursor.IsExtending() );

        CaptureMode         = CaptureGLFCursor;
        }
    else                                // no slot / not big enough
        return;

    if ( CaptureMode == CaptureNone )
        CaptureMode = CaptureLeftButton;

    LButtonDown = true;

    CaptureMouse ( Capture );

    EvMouseMove ( i, p );
    }

else

    TTracksView::EvLButtonDown ( i, p );
}


void    TFrequenciesView::EvLButtonUp ( uint i, const TPoint &p )
{
MouseAxis           = MouseAxisNone;
CurrentWindowSlot   = 0;


if ( CaptureMode == CaptureGLFCursor ) {

    FCursor.StopExtending ();

    ButtonGadgetSetState ( IDB_RANGECURSOR, FCursor.IsExtending() );

//  FreqDoc->NotifyFriendViews ( LinkedViewId, vnNewTFCursor, (TParam2) &TFCursor, this ); !!! send freq?
//  ReloadBuffers (); // or not?

    CaptureMode = CaptureNone;
    }


TTracksView::EvLButtonUp ( i, p );
}


//----------------------------------------------------------------------------
void    TFrequenciesView::EvMouseMove ( uint i, const TPoint &p )
{
int                 dx              = p.X() - MousePos.X();
int                 dy              = p.Y() - MousePos.Y();
int                 adx             = abs ( dx );
int                 ady             = abs ( dy );


if ( RButtonDown && ! ShiftKey ) { // && !ControlKey ) {
                                        // be more sensitive
    if ( adx >= MinMouseMove / 2 + 1 || ady >= MinMouseMove / 2 + 1 ) {

        if      ( adx > ady * ( MouseAxis == MouseAxisVertical   ? 3 : 1 ) )

            MouseAxis   = MouseAxisHorizontal;

        else if ( ady > adx * ( MouseAxis == MouseAxisHorizontal ? 3 : 1 ) )

            MouseAxis   = MouseAxisVertical;
        }

    if ( MouseAxis == MouseAxisNone )
        return;


    if ( MouseAxis == MouseAxisVertical && IsFreqVertical () && FrequenciesOverride ) {

//      if ( ady >= MouseMoveScale ) {
        if ( ady >  MinMouseMove ) {

//          FCursor.ShiftPos ( ( dy < 0 ? -1 : 1 ) * min ( ady, MouseMoveScale ) / (double) MouseMoveScale * ( ady > MouseMoveScaleFast ? 6.0 : 1.0 ) );
            FCursor.ShiftPos ( ( dy < 0 ? -1 : 1 ) * min ( ady, MinMouseMove   ) / (double) MinMouseMove   * ( ady > MouseMoveScaleFast ? 6.0 : 1.0 ) );

            CmNextFrequency ( 0 );

			LastMouseMove       = TPoint ( dx, dy );
			MousePos            = p;
            }
        return;
        }
    }

else if ( LButtonDown && CurrentDisplaySpace == DisplaySpaceNone && ! ControlKey ) {

    if ( CaptureMode == CaptureGLFCursor && CurrentWindowSlot ) {

        TTracksViewWindowSlot*  toslot      = CurrentWindowSlot;
        int                     tf          = (double) ( p.X () - toslot->TrOrg.X ) / toslot->ScaleH - ( IsBarsMode () || IsIntensitySquareMode () ? 0.5 : 0 ) + 0.5 + CDPf.GetMin();

        FCursor.SetExtendingPos ( tf );

        CmNextFrequency ( 0 );          // does the updating job

		LastMouseMove       = TPoint ( dx, dy );
		MousePos            = p;

        return;
//        UpdateCaption ();
//        DrawTFCursor ( true );
        }
    } // CurrentDisplaySpace == DisplaySpaceNone


TTracksView::EvMouseMove ( i, p );
}


//----------------------------------------------------------------------------
void    TFrequenciesView::HintsPaint ()
{
                                        // draw additional user interaction hints
if ( LButtonDown || MButtonDown || RButtonDown ) {


    SetWindowCoordinates ();

    GLBlendOn       ();

    glColor4f           ( GLBASE_CURSORHINTCOLOR );
    BFont->SetBoxColor  ( HintTextBackColor );

//    glTranslated ( PaintRect.Width() / 2, PaintRect.Height() / 2, 0 );
    glTranslated    ( MousePos.X(), PaintRect.Height() - MousePos.Y(), 0 );
    glScaled        ( CursorHintSize, CursorHintSize, 1 );


    if ( RButtonDown && ! ShiftKey ) {

        if ( MouseAxis == MouseAxisVertical && IsFreqVertical () && FrequenciesOverride ) {
                                        // frequency scrolling
            char    buff[ 64 ];

            if ( FCursor.IsSplitted () )
                StringCopy  ( buff, "Frequencies\n", FreqDoc->GetFrequencyName ( FCursor.GetPosMin() ), "-", FreqDoc->GetFrequencyName ( FCursor.GetPosMax() ) );
            else
                StringCopy  ( buff, "Frequency\n",   FreqDoc->GetFrequencyName ( FCursor.GetPosMin() ) );

            if ( LastMouseMove.Y() < 0 ) {
                BFont->Print ( 1, 0.50, 0, buff, TA_LEFT | TA_CENTERY | TA_BOX );
                Prim.DrawArrow ( 0,  0.50, 0, 2.00, 0.25, 1.00, 0.50,  90 );
                }
            else {
                BFont->Print ( 1, 0.0, 0, buff, TA_LEFT | TA_CENTERY | TA_BOX );
                Prim.DrawArrow ( 0, -0.50, 0, 2.00, 0.25, 1.00, 0.50, -90 );
                }

            GLBlendOff      ();
            ResetWindowCoordinates ();
            return;
            }

        } // RButtonDown


    GLBlendOff      ();
    ResetWindowCoordinates ();

    TTracksView::HintsPaint ();
    }
}


//----------------------------------------------------------------------------
bool    TFrequenciesView::VnNewBadSelection ( const TSelection *bad )
{
TTracksView::VnNewBadSelection ( bad );

ResetScaleFreqs ();

if ( FreqNormalize )
    Invalidate ( false );
//ShowNow ();

return  true;
}


bool    TFrequenciesView::VnNewAuxSelection ( const TSelection *aux )
{
TTracksView::VnNewAuxSelection ( aux );

ResetScaleFreqs ();

if ( FreqNormalize )
    Invalidate ( false );
//ShowNow ();

return  true;
}


//----------------------------------------------------------------------------
                                        // lazy solution: change the plane to fool the EEG view, then call it
                                        // problems may arise if another part of the program access also the buffer at that time!
                                        // better solution would be to include the freq parameter in GetTracks?
void    TFrequenciesView::GetTracks ( long tf1, long tf2, TArray2<float> &buff, ReferenceType ref )
{
if ( ! IsModeEFAvg () )
    EegBuff.SetCurrentPlane ( FCursor.GetPosMin () );


TTracksView::GetTracks ( tf1, tf2, buff, ref );


if ( ! IsModeEFAvg () )
    EegBuff.SetCurrentPlane ( 0 );
}


//----------------------------------------------------------------------------
/*                                        // Optimized reload of the freq buffer; handles ALL the cases.
void    TFrequenciesView::UpdateBuffers ( long oldtfmin, long oldtfmax, long newtfmin, long newtfmax )
{
                                        // avoid some stupid job
if ( oldtfmin == newtfmin && oldtfmax == newtfmax )
    return;

int         deltamin    = newtfmin - oldtfmin;
int         deltamax    = newtfmax - oldtfmax;
int         tomove;
int         offset;
bool        allfreqs    = IsModeEF () || IsModeFT ();
auto        bigblock    = [ & ] ()  { return  ( ( allfreqs ? EegBuff.GetMaxPlanes () : 1 ) * EegBuff.GetDim1 () * EegBuff.GetDim2 () ); };

                                        // reload everything ?
                                        // in case of some filters, reload everything
if ( newtfmin > oldtfmax || newtfmax < oldtfmin
  || FreqDoc->AreFiltersActivated () ) {
    if ( allfreqs )
        FreqDoc->GetFrequencies ( newtfmin, newtfmax, EegBuff, 0, AtomTypeUseCurrent, ComputePseudoTracks );
    else
        FreqDoc->GetTracks      ( newtfmin, newtfmax, EegBuff, 0, AtomTypeUseCurrent, ComputePseudoTracks );

    return;
    }


if ( deltamin >= 0 && deltamax <= 0 ) { // (shrink)
    tomove  = bigblock () - deltamin + deltamax;

    MoveVirtualMemory ( EegBuff[0], EegBuff[0] + deltamin, tomove * EegBuff.AtomSize () );

    return;
    }
                                        // intersect partly, for sure
else if ( deltamin >= 0 ) {             // case 1 (shift buffer left)
    tomove  = bigblock () - deltamin;
    offset  = oldtfmax - newtfmin + 1;

    MoveVirtualMemory ( EegBuff[0], EegBuff[0] + deltamin, tomove * EegBuff.AtomSize () );

    if ( allfreqs )
        FreqDoc->GetFrequencies ( oldtfmax + 1, newtfmax, EegBuff, offset, AtomTypeUseCurrent, ComputePseudoTracks );
    else
        FreqDoc->GetTracks      ( oldtfmax + 1, newtfmax, EegBuff, offset, AtomTypeUseCurrent, ComputePseudoTracks );

    return;
    }

else if ( deltamax <= 0 ) {             // case 2 (shift buffer right)
//  tomove  = bigblock () + deltamin + deltamax;
    tomove  = bigblock () + deltamin;

    MoveVirtualMemory ( EegBuff[0] - deltamin, EegBuff[0], tomove * EegBuff.AtomSize () );

    if ( allfreqs )
        FreqDoc->GetFrequencies ( newtfmin, oldtfmin - 1, EegBuff, 0, AtomTypeUseCurrent, ComputePseudoTracks );
    else
        FreqDoc->GetTracks      ( newtfmin, oldtfmin - 1, EegBuff, 0, AtomTypeUseCurrent, ComputePseudoTracks );

    return;
    }

else {                                  // case 3: deltamin < 0 && deltamax > 0 (expand)
    tomove  = bigblock () + deltamin;
    offset  = oldtfmax - newtfmin + 1;

    MoveVirtualMemory ( EegBuff[0] - deltamin, EegBuff[0], tomove * EegBuff.AtomSize () );

    if ( allfreqs ) {
        FreqDoc->GetFrequencies ( newtfmin, oldtfmin - 1, EegBuff, 0     , AtomTypeUseCurrent, ComputePseudoTracks );
        FreqDoc->GetFrequencies ( oldtfmax + 1, newtfmax, EegBuff, offset, AtomTypeUseCurrent, ComputePseudoTracks );
        }
    else {
        FreqDoc->GetTracks      ( newtfmin, oldtfmin - 1, EegBuff, 0     , AtomTypeUseCurrent, ComputePseudoTracks );
        FreqDoc->GetTracks      ( oldtfmax + 1, newtfmax, EegBuff, offset, AtomTypeUseCurrent, ComputePseudoTracks );
        }

    return;
    }
}
*/
                                        // Optimized reload of the freq buffer; handles ALL the cases.
                                        // All freqs, all electrodes are always updated
void    TFrequenciesView::UpdateBuffers ( long oldtfmin, long oldtfmax, long newtfmin, long newtfmax )
{
                                        // avoid some stupid job
if ( oldtfmin == newtfmin && oldtfmax == newtfmax )
    return;

int                 deltamin        = newtfmin - oldtfmin;
int                 deltamax        = newtfmax - oldtfmax;
int                 tomove;
int                 offset;
auto                bigblock        = [ & ] ()  { return  ( EegBuff.GetMaxPlanes () * EegBuff.GetDim1 () * EegBuff.GetDim2 () ); };

                                        // temporarily reset block shift
if ( IsModeEFAvg () )
    EegBuff.SetCurrentPlane ( 0 );

                                        // reload everything ?
if ( newtfmin > oldtfmax || newtfmax < oldtfmin ) {

    FreqDoc->GetFrequencies ( newtfmin, newtfmax, EegBuff, 0, AtomTypeUseCurrent, ComputePseudoTracks );

    if ( IsModeEFAvg () )
        EegBuff.SetCurrentPlane ( FCursor.GetPosMin () );

    return;
    }


if ( deltamin >= 0 && deltamax <= 0 ) { // (shrink)
    tomove  = bigblock () - deltamin + deltamax;

    MoveVirtualMemory ( EegBuff[0], EegBuff[0] + deltamin, tomove * EegBuff.AtomSize () );

    if ( IsModeEFAvg () )
        EegBuff.SetCurrentPlane ( FCursor.GetPosMin () );

    return;
    }
                                        // intersect partly, for sure
else if ( deltamin >= 0 ) {             // case 1 (shift buffer left)
    tomove  = bigblock () - deltamin;
    offset  = oldtfmax    - newtfmin + 1;

    MoveVirtualMemory ( EegBuff[0], EegBuff[0] + deltamin, tomove * EegBuff.AtomSize () );

    FreqDoc->GetFrequencies ( oldtfmax + 1, newtfmax, EegBuff, offset, AtomTypeUseCurrent, ComputePseudoTracks );

    if ( IsModeEFAvg () )
        EegBuff.SetCurrentPlane ( FCursor.GetPosMin () );

    return;
    }

else if ( deltamax <= 0 ) {             // case 2 (shift buffer right)
//  tomove  = bigblock () + deltamin + deltamax;
    tomove  = bigblock () + deltamin;

    MoveVirtualMemory ( EegBuff[0] - deltamin, EegBuff[0], tomove * EegBuff.AtomSize () );

    FreqDoc->GetFrequencies ( newtfmin, oldtfmin - 1, EegBuff, 0, AtomTypeUseCurrent, ComputePseudoTracks );

    if ( IsModeEFAvg () )
        EegBuff.SetCurrentPlane ( FCursor.GetPosMin () );

    return;
    }

else {                                  // case 3: deltamin < 0 && deltamax > 0 (expand)
    tomove  = bigblock () + deltamin;
    offset  = oldtfmax    - newtfmin + 1;

    MoveVirtualMemory ( EegBuff[0] - deltamin, EegBuff[0], tomove * EegBuff.AtomSize () );

    FreqDoc->GetFrequencies ( newtfmin, oldtfmin - 1, EegBuff, 0     , AtomTypeUseCurrent, ComputePseudoTracks );
    FreqDoc->GetFrequencies ( oldtfmax + 1, newtfmax, EegBuff, offset, AtomTypeUseCurrent, ComputePseudoTracks );

    if ( IsModeEFAvg () )
        EegBuff.SetCurrentPlane ( FCursor.GetPosMin () );

    return;
    }
}


//----------------------------------------------------------------------------
                                        // regular tracks relative scaling set to 1
                                        // the others are relatively scaled to an average ratio
void    TFrequenciesView::ResetScaleTracks ( const TSelection *sel )
{
//int               firstfreq   = IsModeEFAvg () ? FCursor.GetPosMin () : 0;
//int               lastfreq    = IsModeEFAvg () ? FCursor.GetPosMin () : FreqDoc->GetNumFrequencies () - 1;

                                                // for all mode, for a smarter auto-scaling
int                 firstfreq   = IsModeSpectrum () ? 0                                 : FCursor.GetPosMin ();
int                 lastfreq    = IsModeSpectrum () ? FreqDoc->GetNumFrequencies () - 1 : FCursor.GetPosMax ();

long                fromtf          = 0;
long                totf            = CDPt.GetLength() - 1;

double              maxt            = 0;
double              v;
//TEasyStats        stats ( totf - fromtf + 1 );
TVector<float>      stats ( totf - fromtf + 1 );    // faster stats
int                 stati;
                                        // get the max of all regular tracks
                                        // including the frequency normalization
                                        // scan 1 / all frequencies
if ( IsModeEFAvg () )
    EegBuff.SetCurrentPlane ( 0 );


for ( int f = firstfreq; f <= lastfreq; f++ ) {

    for ( int e = FreqDoc->GetFirstRegularIndex (); e <= FreqDoc->GetLastRegularIndex (); e++ ) {

        if ( BadTracks[ e ] || AuxTracks[ e ] ) continue;

//      stats.Reset ();
        stats.ResetMemory ();
        stati   = 0;

        for ( int tf = fromtf; tf <= totf; tf++ ) {

            v   = ( FreqNormalize ? ScaleFreqs[ f ] : 1 ) * fabs ( EegBuff ( f, e, tf ) );

            if ( v )
//              stats.Add ( v, ThreadSafetyIgnore );
                stats[ stati++ ]    = v;
            } // for tf

        stats.Sort ( Descending );

        Maxed ( maxt, 10.0 * stats[ stati / 2 ] );
        } // for e
    } // for f


maxt    = NonNull ( maxt );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int e = 0; e < FreqDoc->GetTotalElectrodes (); e++ ) {

    if ( sel && sel->IsNotSelected ( e ) )  continue;


    if ( e <= FreqDoc->GetLastRegularIndex () && ! ( BadTracks[ e ] || AuxTracks[ e ] ) )

        ScaleTracks[ e ]    = 1.0 / maxt;

    else {                              // auxs and bads are scaled individually, as to be "neutralized"

        double      maxp    = 0;

        for ( int f = firstfreq; f <= lastfreq; f++ ) {

//          stats.Reset ();
            stats.ResetMemory ();
            stati   = 0;

            for ( int tf = fromtf; tf <= totf; tf++ ) {

                v   = ( FreqNormalize ? ScaleFreqs[ f ] : 1 ) * fabs ( EegBuff ( f, e, tf ) );

                if ( v )
//                  stats.Add ( v, ThreadSafetyIgnore );
                    stats[ stati++ ]    = v;
                } // for tf

            stats.Sort ( Descending );

            Maxed ( maxp, 10.0 * stats[ stati / 2 ] );
            } // for f

        maxp    = NonNull ( maxp );
                                        // shrink a little bit auxs, pseudos, and bads
        ScaleTracks[ e ]    = 1.0 / maxp * 0.5;
        } // special electrode
    } // for e


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( IsModeEFAvg () )
    EegBuff.SetCurrentPlane ( FCursor.GetPosMin () );


/*                                        // also update the ranges (intensity and normalization will give very different ranges)
if ( IsIntensityModes () ) {

    double  scalef  = FreqNormalize ? ScaleFreqs[ firstfreq ] : 1;
    double  maxValue    = scalef * FreqDoc->GetAbsMaxValue ();
    SetScalingLimits ( maxValue * 1e-9, maxValue * 1e3 );
    }
else {
    SetScalingLimits ( EEGGLVIEW_STVMIN, EEGGLVIEW_STVMAX );
    }
*/

SetScalingLimits ( EEGGLVIEW_STVMIN, EEGGLVIEW_STVMAX );
}


//----------------------------------------------------------------------------
                                        // regular freqs relative scaling set to 1
                                        // the others are relatively scaled to an average ratio
void    TFrequenciesView::ResetScaleFreqs ()
{
int                 numel           = FreqDoc->GetNumElectrodes  ();
int                 numfreq         = FreqDoc->GetNumFrequencies ();
int                 numtf           = CDPt.GetLength ();

                                        // temporarily reset block shift
if ( IsModeEFAvg () )
    EegBuff.SetCurrentPlane ( 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // weighting is done through AVG or GFP, then rescaled to give a mean weight of 1
TEasyStats          stat;


for ( int f = 0; f < numfreq; f++ ) {

    stat.Reset ();

    for ( int e  = 0; e  < numel; e++  )
    for ( int tf = 0; tf < numtf; tf++ )

        stat.Add ( abs ( EegBuff ( f, e, tf ) ), ThreadSafetyIgnore );


    ScaleFreqs[ f ]     = stat.Average ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get the min of averages
double              minavg      = ScaleFreqs[ 0 ];

for ( int f = 0; f < numfreq; f++ )
    Mined ( minavg, ScaleFreqs[ f ] );

                                        // normalize the curve
if ( minavg != 0 ) {

    double      avgscale    = 0;

    for ( int f = 0; f < numfreq; f++ ) {
                                        // invert curve
        ScaleFreqs[ f ] = minavg / ( ScaleFreqs[ f ] + 1e-60 );
                                        // cumulate for proper normalization
        avgscale       += ScaleFreqs[ f ];
        }

    avgscale   = NonNull ( avgscale / numfreq );


    for ( int f = 0; f < numfreq; f++ ) // average of scaling factors is 1
        ScaleFreqs[ f ]    /= avgscale;
    }

else { // minavg == 0

    ScaleFreqs  = 1;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
                                        // post-processing (smoothing) the curve - not really needed(?)
double              temp1;
double              temp2;

for ( int filtersize = 1 ; filtersize > 0; filtersize-- ) {

    temp2 = 0;

    for ( int f = 0; f < numfreq; f++ ) {

        temp1 = ScaleFreqs[ f ];

        if      ( f == 0           )    ScaleFreqs[ f ] = ( 3 * ScaleFreqs[ f ] + ScaleFreqs[ f + 1 ]         ) / 4;
        else if ( f == numfreq - 1 )    ScaleFreqs[ f ] = ( 3 * ScaleFreqs[ f ] + temp2                       ) / 4;
        else                            ScaleFreqs[ f ] = ( 2 * ScaleFreqs[ f ] + ScaleFreqs[ f + 1 ] + temp2 ) / 4;

        temp2 = temp1;
        }
    } // for filtersize
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // setting one more dummy scaling at the end of the vector, for safety reasons in the ET display mode
ScaleFreqs[ numfreq ] = 1;


if ( IsModeEFAvg () )
    EegBuff.SetCurrentPlane ( FCursor.GetPosMin () );
}


//----------------------------------------------------------------------------
void    TFrequenciesView::CmSetRenderingMode ()
{
                                        // legal splitting?
if ( ! ( IsModeSpectrumEnum () && TFCursor.GetLength() > 1 ) ) {
    TTracksView::CmSetRenderingMode ();
    return;
    }


RenderingMode   = LayoutOnePage;

                                        // choose 3D set here
if      ( RenderingMode == LayoutOneTrackOneBoxXyzFlat )    CurrentDisplaySpace     = DisplaySpaceProjected;
else if ( RenderingMode == LayoutOneTrackOneBoxXyz3D   )    CurrentDisplaySpace     = DisplaySpace3D;
else                                                        CurrentDisplaySpace     = DisplaySpaceNone;

InitXyz ();

Orientation         = DefaultPotentialOrientation - ( ShiftKey ? -1 : 1 );
CmOrient ();


SetPartWindows ( SelTracks );

RefreshLinkedWindow ( true ) ;
}


void    TFrequenciesView::CmSetScalingAdapt ()
{
ScalingAuto     = NextState ( ScalingAuto, NumScalingAuto, ShiftKey );  // Frequency display can use the asymmetrical as a per-track scaling instead

Invalidate ( false );

ButtonGadgetSetState ( IDB_FIXEDSCALE, ScalingAuto != ScalingAutoOff );
}


//----------------------------------------------------------------------------
void    TFrequenciesView::CmNextFrequency ( owlwparam w )
{
if ( w == IDB_PREVIOUSFREQ && FCursor.GetPosMin () == CDPf.GetLimitMin () && !TFCursor.IsExtending()
  || w == IDB_NEXTFREQ     && FCursor.GetPosMax () == CDPf.GetLimitMax () && !TFCursor.IsExtending() )
    return;


int                 deltac          = w == IDB_PREVIOUSFREQ ? -1 
                                    : w == IDB_NEXTFREQ     ?  1 
                                    :                          0;

                                        // bigger jumps if shift key is pressed
if ( ShiftKey )     deltac         *= FCursor.GetLength ();


if ( FCursor.IsExtending() )
    FCursor.SetExtendingPos ( FCursor.GetExtendingPos () + deltac );
else
    FCursor.ShiftPos ( deltac );


if ( IsModeEFAvg () ) {
                                        // shift buffer
    EegBuff.SetCurrentPlane ( FCursor.GetPosMin () );

    ResetScaleTracks ();

//  FreqDoc->NotifyDocViews ( vnReloadData, EV_VN_RELOADDATA_EEG, this );
    }
/*
else {
    EegBuff.SetCurrentPlane ( FCursor.GetPosMin () );
    ResetScaleTracks ();
    EegBuff.SetCurrentPlane ( 0 );
    }
*/
else if ( ! IsModeSpectrum () ) {
    ResetScaleTracks ();

    SetPartWindows ( SelTracks );
    }

                                        // tell the Doc, but not back to the views
FreqDoc->SetCurrentFrequency ( FCursor.GetPosMin (), false ); // !!!!! remove later?

                                        // for maps and IS
FreqDoc->NotifyDocViews ( vnReloadData, EV_VN_RELOADDATA_EEG, this );


ShowNow ();


//FreqDoc->NotifyFriendViews ( LinkedViewId, vnNewTFCursor, (TParam2) TFCursor, this ); // its equivalent to FCursor?
RefreshLinkedWindow ();
}


bool    TFrequenciesView::VnReloadData ( int what )
{
switch ( what ) {

  case EV_VN_RELOADDATA_FREQ:
                                        // the boss is the document
    FCursor.SetPos ( FreqDoc->GetCurrentFrequency () );

//  ReloadBuffers ();

    FreqDoc->NotifyDocViews ( vnReloadData, EV_VN_RELOADDATA_EEG );

    Invalidate ( false );
    return  true;

  default:
    return  TTracksView::VnReloadData ( what );
    }
}


//----------------------------------------------------------------------------
                                        // change modes, set and reset all that is needed
void    TFrequenciesView::CmSetFrequencyMode ( owlwparam w )
{
FreqModes           ofreqmode       = FreqMode;


if      ( w == IDB_AVERAGETRACKS )

    AverageMode     = ! AverageMode;


else if ( w == IDB_DISPLAYINTENSITY ) {

    if      ( IsModeSpectrum () ) {     // limited to tracks & bars
        if      ( IsTracksMode ()          )    SetBarsMode ();
        else                                    SetTracksMode ();
        }

    else if ( ShiftKey ) {              // can do all 3 modes
        if      ( IsTracksMode ()          )   SetIntensitySmoothMode ();
        else if ( IsBarsMode ()            )   SetTracksMode ();
        else if ( IsIntensitySquareMode () )   SetBarsMode ();
        else if ( IsIntensitySmoothMode () )   SetIntensitySquareMode ();
        }

    else {
        if      ( IsTracksMode ()          )   SetBarsMode ();
        else if ( IsBarsMode ()            )   SetIntensitySquareMode ();
        else if ( IsIntensitySquareMode () )   SetIntensitySmoothMode ();
        else if ( IsIntensitySmoothMode () )   SetTracksMode ();
        }

//  if ( IsIntensityModes () && AverageMode )
//      AverageMode = false;    // or disable?
    }


ButtonGadgetSetState ( IDB_AVERAGETRACKS,    AverageMode     );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                others          = w == IDB_AVERAGETRACKS        || w == IDB_DISPLAYINTENSITY;
bool                spectrum        = w == IDB_FREQDISPLAY_SPECTRUM || others && IsModeSpectrum ();
bool                ef              = w == IDB_FREQDISPLAY_EF       || others && IsModeEF ();
bool                fe              = w == IDB_FREQDISPLAY_FE       || others && IsModeFE ();


if      ( spectrum ) {

    if ( ! IsModeSpectrum () ) {         // entering this mode?

        FreqMode   = FreqModeSpectrum;

        SavedCDP = *CDP;

//      RenderingMode   = EEGGLVIEW_SPLIT_NO;
                                        // no intensity for the moment in spectrum
        if ( IsIntensityModes () )      SetTracksMode ();
        }


    EegBuff.SetCurrentPlane ( 0 );

    SetCDP ();

    if      (   FreqModeEFShortTime ( ofreqmode ) && ! FreqModeEFShortTime ( FreqMode ) ) {
        DisplayMinTF            = EEGGLVIEW_STHMINTF;

        CDP->SetLength ( SavedCDP.GetLength(), CDP->GetMiddle (), true );
        ComputeSTHfromCDPt ();
        SyncCDPtTFCursor        = false;
        CheckCDPtAndTFCursor ( false );
        }
    else if ( ! FreqModeEFShortTime ( ofreqmode ) &&   FreqModeEFShortTime ( FreqMode ) ) {
        DisplayMinTF            = 1;

        SyncCDPtTFCursor        = true; // cursor and display must follow each other
        CheckCDPtAndTFCursor ( true );
        }

                                        // trick, if cursor is not wide enough, copy back
    if ( TFCursor.GetLength () < DisplayMinTF ) {
                                    // first, force CDPt
        if ( CDPt.GetLength () < DisplayMinTF ) {
            CDPt.SetLength ( DisplayMinTF, TFCursor.GetPosMiddle (), true );
            ComputeSTHfromCDPt ();
//          CheckCDPtAndTFCursor ( true );
            }
                                        // then contaminate TFCursor
        CheckCDPtAndTFCursor ( false );
        }

    ButtonGadgetSetState ( IDB_FREQDISPLAY_SPECTRUM, IsModeSpectrum () );

    ReloadBuffers ();

    ResetScaleTracks ();

    SetPartWindows ( SelTracks, false );

    ShowNow ();
    }

else if ( ef ) {

    if ( !IsModeEF () ) {               // entering this mode?

        FreqMode   = FreqModeEF;
                                        // shift buffer
        EegBuff.SetCurrentPlane ( IsModeEFAvg () ? FCursor.GetPosMin () : 0 );

        DisplayMinTF            = EEGGLVIEW_STHMINTF;

        SetCDP ();

        if ( FreqModeEFShortTime ( ofreqmode ) ) {
            CDP->SetLength ( SavedCDP.GetLength(), CDP->GetMiddle (), true );
            ComputeSTHfromCDPt ();
            }


//      *CDP = SavedCDP;
        SyncCDPtTFCursor        = false;
        CheckCDPtAndTFCursor ( false );

//      RenderingMode   = EEGGLVIEW_SPLIT_NO;
        }
    else {                              // already in this mode
        SetCDP ();

        EegBuff.SetCurrentPlane ( IsModeEFAvg () ? FCursor.GetPosMin () : 0 );
        }


    ReloadBuffers ();

    ResetScaleTracks ();

    SetPartWindows ( SelTracks, false );

    ShowNow ();

    FreqDoc->NotifyDocViews ( vnReloadData, EV_VN_RELOADDATA_EEG, this );

    ButtonGadgetSetState ( IDB_FREQDISPLAY_EF, IsModeEF () );
    }

else if ( fe ) {

    if ( !IsModeFE () ) {               // entering this mode?

        FreqMode   = FreqModeFE;

        EegBuff.SetCurrentPlane ( 0 );

        DisplayMinTF            = EEGGLVIEW_STHMINTF;

        SetCDP ();
        if ( FreqModeEFShortTime ( ofreqmode ) ) {
            CDP->SetLength ( SavedCDP.GetLength(), CDP->GetMiddle (), true );
            ComputeSTHfromCDPt ();
            }
//      *CDP = SavedCDP;
        SyncCDPtTFCursor        = false;
        CheckCDPtAndTFCursor ( false );


        ReloadBuffers ();               // need to reload, as we switch to full 3D data

//      RenderingMode   = LayoutOnePage;
        }
    else {                              // already in this mode
        EegBuff.SetCurrentPlane ( 0 );

        SetCDP ();

//      RenderingMode   = LayoutOnePage;
        }


    ResetScaleTracks ();

    SetPartWindows ( SelTracks, false );

    ShowNow ();

    ButtonGadgetSetState ( IDB_FREQDISPLAY_FE, IsModeFE () );
    }

}


//----------------------------------------------------------------------------
void    TFrequenciesView::CmZoomVert ( owlwparam w )
{
TTracksView::CmZoomVert ( w );
}


void    TFrequenciesView::CmZoomHorz ( owlwparam w )
{
TTracksView::CmZoomHorz ( w );


if ( IsModeSpectrumEnum () /*&& TFCursor.GetLength() > 1*/ )   // in this mode, number of slots is size of CDPt
    SetPartWindows ( SelTracks );
}


//----------------------------------------------------------------------------
void    TFrequenciesView::CmShiftTracks ( owlwparam w )
{
if ( IsFreqVertical () && FrequenciesOverride ) {
    if      ( w == IDB_UP )     CmNextFrequency ( IDB_NEXTFREQ );
    else if ( w == IDB_DOWN )   CmNextFrequency ( IDB_PREVIOUSFREQ );
    }
else
    TTracksView::CmShiftTracks ( w );
}


//----------------------------------------------------------------------------
void    TFrequenciesView::CmForward ( owlwparam /*w*/ )
{
                                        // move frequency cursor
if ( ! IsFreqVertical () && FrequenciesOverride ) {
    CmNextFrequency ( IDB_NEXTFREQ );
    }
else                                    // else Time cursor
    TTracksView::CmForward ( 0 );
}


void    TFrequenciesView::CmBackward ( owlwparam /*w*/ )
{
                                        // move frequency cursor
if ( ! IsFreqVertical () && FrequenciesOverride ) {
    CmNextFrequency ( IDB_PREVIOUSFREQ );
    }
else                                    // else Time cursor
    TTracksView::CmBackward ( 0 );
}


//----------------------------------------------------------------------------
void    TFrequenciesView::CmChangeNumTracks ( owlwparam w )
{
TTracksView::CmChangeNumTracks ( w );
}


void    TFrequenciesView::CmChangeNumPseudoTracks ( owlwparam w )
{
TTracksView::CmChangeNumPseudoTracks ( w );
}


void    TFrequenciesView::CmChangeNumFreqs ( owlwparam w )
{                                       // will modify FCursor, NOT CDPf
int                 oldlen          = FCursor.GetLength ();

if ( w == IDB_MOREFREQS && oldlen == FCursor.GetLimitLength ()
  || w == IDB_LESSFREQS && oldlen == 1 )
    return;


if ( ShiftKey ) {

    if      ( w == IDB_MOREFREQS )  FCursor.SetPos    ( FCursor.GetLimitMin (),   FCursor.GetLimitMax () );
    else if ( w == IDB_LESSFREQS )  FCursor.SetPos    ( FCursor.GetPosMin () ); // FCursor.SetPos    ( FCursor.GetPosMiddle () );
    }

else {

    if      ( w == IDB_MOREFREQS ) {

        int             stepf       = AtLeast ( 1, Round ( FCursor.GetLength () * 0.10 ) );

        if      ( FCursor.GetPosMin () == FCursor.GetLimitMin () )      FCursor.SetPos    ( FCursor.GetPosMin (),           FCursor.GetPosMax () + stepf );
        else if ( FCursor.GetPosMax () == FCursor.GetLimitMax () )      FCursor.SetPos    ( FCursor.GetPosMin () - stepf,   FCursor.GetPosMax ()         );
        else                                                            FCursor.SetPos    ( FCursor.GetPosMin () - stepf,   FCursor.GetPosMax () + stepf );
        }
    else if ( w == IDB_LESSFREQS ) {

        int             stepf       = AtLeast ( 1, Round ( FCursor.GetLength () * 0.08 ) );

        if ( oldlen == 2 )              // allow to decrease back to a single frequency
            FCursor.SetPos    ( FCursor.GetPosMin () );
        else
            FCursor.SetPos    ( FCursor.GetPosMin () + stepf,     FCursor.GetPosMax () - stepf );
        }

    }


CmNextFrequency ( 0 );                  // will do the actual updating job
}


//----------------------------------------------------------------------------
void    TFrequenciesView::CmSetFreqNormalize ()
{
FreqNormalize   = ! FreqNormalize;

ResetScaleTracks ();

//SetScaling ( ScalingLevel * ( FreqNormalize ? 1 cc : ScaleFreqs[ 0 ] ) );

Invalidate ( false );
ButtonGadgetSetState ( IDB_FREQNORMALIZE, FreqNormalize );
}


void    TFrequenciesView::CmOffEnable ( TCommandEnabler &tce )
{
tce.Enable ( false );
}


void    TFrequenciesView::CmAverageEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsModeSpectrum () || ! IsIntensityModes () );
}


void    TFrequenciesView::CmSetShow3DText ()
{
Show3DText  = ! Show3DText;

Invalidate ( false );
}


void    TFrequenciesView::CmResetScaleFreqs ()
{
ResetScaleFreqs ();

if ( FreqNormalize )
    Invalidate ( false );
}


void    TFrequenciesView::CmSaveScaleFreqs ()
{
TFileName           file ( FreqDoc->GetDocPath () );

file.ReplaceExtension ( "Normalization" "." InfixSpectrum "." FILEEXT_EEGSEF );

ScaleFreqs.WriteFile ( file, "FreqNorm", FreqDoc->GetNumFrequencies () );

file.Open ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
