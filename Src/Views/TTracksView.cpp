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

#include    <typeinfo>

#include    <owl/pch.h>
#include    <owl/picklist.h>            // TPickListPopup

#include    "TTracksView.h"

#include    "Math.Stats.h"
#include    "Math.Resampling.h"
#include    "TArray2.h"
#include    "TArray1.h"
#include    "OpenGL.h"
#include    "TInterval.h"
#include    "Strings.Utils.h"
#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"
#include    "Files.ReadFromHeader.h"
#include    "Files.Conversions.h"

#include    "TScanTriggersDialog.h"
#include    "BadEpochs.h"

#include    "TSegDoc.h"
#include    "TRisDoc.h"

#include    "TExportTracksDialog.h"
#include    "TInterpolateTracksDialog.h"
#include    "TFrequencyAnalysisDialog.h"
#include    "TCreateRoisDialog.h"
#include    "TMicroStatesFitDialog.h"
#include    "TStatisticsDialog.h"
#include    "TRisToVolumeDialog.h"
#include    "TScanTriggersDialog.h"

#include    "TElectrodesView.h"
#include    "TPotentialsView.h"
#include    "TInverseView.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

extern  TExportTracksStructEx       ExportTracksTransfer;
extern  TFrequencyAnalysisStructEx  FrequencyAnalysisTransfer;
extern  TInterpolateTracksStructEx  InterpolateTracksTransfer;
extern  TCreateRoisStruct           CreateRoisTransfer;
extern  TCreateRoisDialog*          CreateRoisDlg;
extern  TMicroStatesFitStruct       FitTransfer;
extern  TStatStruct                 StatTransfer;
extern  TRisToVolumeStructEx        RisToVolumeTransfer;
extern  TScanTriggersStruct         ScanTriggersTransfer;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    TTracksViewState::SaveState ( TTracksView *view )
{
if ( ! view )
    return;


Attr                = view->GetParentO ()->Attr;
IsMinimized         = view->IsWindowMinimized ();
IsMaximized         = view->IsWindowMaximized ();

RenderingMode       = view->RenderingMode;
DisplayMode         = view->DisplayMode;

SelTracks           = view->SelTracks;
BadTracks           = view->BadTracks;
AuxTracks           = view->AuxTracks;
Highlighted         = view->Highlighted;

TracksSuper         = view->TracksSuper;
AverageRois         = view->AverageRois;
ShowVertScale       = view->ShowVertScale;
ShowHorizScale      = view->ShowHorizScale;
ShowTags            = view->ShowTags;
TracksFlipVert      = view->TracksFlipVert;
ShowBaseline        = view->ShowBaseline;
ShowSD              = view->ShowSD;
ShowDate            = view->ShowDate;
ShowTime            = view->ShowTime;
DisplayMarkerType   = view->DisplayMarkerType;

LineWidth           = view->LineWidth;
}


void    TTracksViewState::RestoreState ( TTracksView *view )
{
if ( ! view )
    return;


view->WindowSetPosition ( Attr.X, Attr.Y, Attr.W, Attr.H );

if      ( IsMinimized )     view->WindowMinimize ();
else if ( IsMaximized )     view->WindowMaximize ();


view->RenderingMode = RenderingMode;
view->DisplayMode   = DisplayMode;

view->SetSelTracks ( SelTracks );
view->BadTracks     = BadTracks;
view->AuxTracks     = AuxTracks;
view->Highlighted   = Highlighted;

view->TracksSuper   = TracksSuper;
view->AverageRois   = AverageRois;
view->ShowVertScale = ShowVertScale;
view->ShowHorizScale= ShowHorizScale;
view->TracksFlipVert= TracksFlipVert;
view->ShowBaseline  = ShowBaseline;
view->ShowSD        = ShowSD;
view->ShowDate      = ShowDate;
view->ShowTime      = ShowTime;
view->SetMarkerType ( DisplayMarkerType );
//view->ShowTags      = ShowTags;         // (after SetMarkerType) don't, ShowTags will update itself according to content

view->LineWidth     = LineWidth;

                                        // activate window
view->GetParentO()->SetFocus();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// Don't forget to update TSecondaryView response table also, for the buttons

DEFINE_RESPONSE_TABLE1(TTracksView, TBaseView)

//    EV_WM_GETMINMAXINFO,
    EV_WM_SIZE,
    EV_WM_SETFOCUS,
    EV_WM_KILLFOCUS,
    EV_WM_LBUTTONDBLCLK,
    EV_WM_LBUTTONDOWN,
    EV_WM_LBUTTONUP,
    EV_WM_MBUTTONDOWN,
    EV_WM_MBUTTONUP,
    EV_WM_RBUTTONDOWN,
    EV_WM_RBUTTONUP,
    EV_WM_MOUSEMOVE,
    EV_WM_KEYDOWN,
    EV_WM_KEYUP,
    EV_WM_TIMER,
    EV_WM_DROPFILES,
    EV_WM_MOUSEWHEEL,

    EV_VN_RELOADDATA,
    EV_VN_NEWTFCURSOR,
    EV_VN_NEWSELECTION,
    EV_VN_NEWHIGHLIGHTED,
    EV_VN_NEWBADSELECTION,
    EV_VN_NEWAUXSELECTION,
    EV_VN_SESSIONUPDATED,

    EV_COMMAND          ( IDB_2OBJECT,                  Cm2Object ),
    EV_COMMAND          ( IDB_ORIENT,                   CmOrient ),
    EV_COMMAND_ENABLE   ( IDB_ORIENT,                   CmOrientEnable ),
    EV_COMMAND          ( IDB_MAGNIFIER,                CmMagnifier ),
    EV_COMMAND          ( IDB_SURFACEMODE,              CmSetRenderingMode ),
    EV_COMMAND          ( CM_EDITCOPY,                  CmEditCopy ),

    EV_COMMAND_AND_ID   ( IDB_EXTTRACKSV,               CmZoomVert ),
    EV_COMMAND_AND_ID   ( IDB_COMPTRACKSV,              CmZoomVert ),
    EV_COMMAND_AND_ID   ( IDB_EXTTRACKSH,               CmZoomHorz ),
    EV_COMMAND_AND_ID   ( IDB_COMPTRACKSH,              CmZoomHorz ),

    EV_COMMAND_AND_ID   ( IDB_FORWARD,                  CmForward ),
    EV_COMMAND_AND_ID   ( IDB_BACKWARD,                 CmBackward ),
    EV_COMMAND_AND_ID   ( IDB_FASTFORWARD,              CmForward ),
    EV_COMMAND_AND_ID   ( IDB_FASTBACKWARD,             CmBackward ),
    EV_COMMAND_AND_ID   ( IDB_PAGEFORWARD,              CmPageForward ),
    EV_COMMAND_AND_ID   ( IDB_PAGEBACKWARD,             CmPageBackward ),

    EV_COMMAND_AND_ID   ( IDB_LESSTRACKS,               CmChangeNumTracks ),
    EV_COMMAND_AND_ID   ( IDB_MORETRACKS,               CmChangeNumTracks ),
    EV_COMMAND_AND_ID   ( IDB_LESSPSEUDOTRACKS,         CmChangeNumPseudoTracks ),
    EV_COMMAND_AND_ID   ( IDB_MOREPSEUDOTRACKS,         CmChangeNumPseudoTracks ),
    EV_COMMAND_AND_ID   ( IDB_UP,                       CmShiftTracks ),
    EV_COMMAND_AND_ID   ( IDB_DOWN,                     CmShiftTracks ),

    EV_COMMAND          ( IDB_NEXTSESSION,              CmNextSession ),
    EV_COMMAND          ( IDB_TRACKSSUPER,              CmTracksSuper ),
    EV_COMMAND          ( IDB_VERTUNITS,                CmVertGrid ),
    EV_COMMAND          ( IDB_HORIZUNITS,               CmHorizGrid ),
    EV_COMMAND          ( IDB_RANGECURSOR,              CmRangeCursor ),

    EV_COMMAND          ( IDB_SHOWMARKERS,              CmShowTags ),
    EV_COMMAND_AND_ID   ( IDB_PREVMARKER,               CmPreviousNextMarker ),
    EV_COMMAND_AND_ID   ( IDB_NEXTMARKER,               CmPreviousNextMarker ),
    EV_COMMAND_ENABLE   ( IDB_SHOWMARKERS,              CmTagsEnable ),
    EV_COMMAND_ENABLE   ( IDB_PREVMARKER,               CmPrevNextTagEnable ),
    EV_COMMAND_ENABLE   ( IDB_NEXTMARKER,               CmPrevNextTagEnable ),
    EV_COMMAND          ( IDB_ADDMARKER,                CmAddMarker ),
    EV_COMMAND_AND_ID   ( CM_EEGMRKQUICK1,              CmQuickMarkers ),
    EV_COMMAND_AND_ID   ( CM_EEGMRKQUICK2,              CmQuickMarkers ),
    EV_COMMAND_AND_ID   ( CM_EEGMRKQUICK3,              CmQuickMarkers ),
    EV_COMMAND_AND_ID   ( CM_EEGMRKQUICK4,              CmQuickMarkers ),
    EV_COMMAND_AND_ID   ( CM_EEGMRKQUICK5,              CmQuickMarkers ),
    EV_COMMAND_AND_ID   ( CM_EEGMRKQUICK6,              CmQuickMarkers ),
    EV_COMMAND_AND_ID   ( CM_EEGMRKQUICK7,              CmQuickMarkers ),
    EV_COMMAND_AND_ID   ( CM_EEGMRKQUICK8,              CmQuickMarkers ),
    EV_COMMAND_AND_ID   ( CM_EEGMRKQUICK9,              CmQuickMarkers ),
    EV_COMMAND_AND_ID   ( CM_EEGMRKQUICKRESET,          CmQuickMarkers ),

    EV_COMMAND          ( IDB_FILTER,                   CmFilter ),
    EV_COMMAND_ENABLE   ( IDB_FILTER,                   CmFilterEnable ),
    EV_COMMAND          ( IDB_SHOWSD,                   CmShowSD ),
    EV_COMMAND_ENABLE   ( IDB_SHOWSD,                   CmShowSDEnable ),
//  EV_COMMAND          ( IDB_SPLITWINDOW,              CmSetRenderingMode ),
    EV_COMMAND          ( IDB_FLIPVERT,                 CmFlipVert ),

    EV_COMMAND_AND_ID   ( CM_EEGREFNONE,                CmSetReference ),
    EV_COMMAND_AND_ID   ( CM_EEGREFAVG,                 CmSetReference ),
    EV_COMMAND_AND_ID   ( IDB_SETAVGREF,                CmSetReference ),
    EV_COMMAND_AND_ID   ( CM_EEGREFELECSINGLE,          CmSetReference ),
    EV_COMMAND_AND_ID   ( CM_EEGREFELECMULTIPLE,        CmSetReference ),
    EV_COMMAND_AND_ID   ( CM_EEGREFMONTAGE,             CmSetReference ),
    EV_COMMAND_ENABLE   ( CM_EEGREFNONE,                CmSetReferenceNoneEnable     ),
    EV_COMMAND_ENABLE   ( CM_EEGREFAVG,                 CmSetReferenceAvgEnable      ),
    EV_COMMAND_ENABLE   ( CM_EEGREFELECSINGLE,          CmSetReferenceSingleEnable   ),
    EV_COMMAND_ENABLE   ( CM_EEGREFELECMULTIPLE,        CmSetReferenceMultipleEnable ),
    EV_COMMAND_ENABLE   ( CM_EEGREFMONTAGE,             CmSetReferenceMontageEnable  ),

    EV_COMMAND_AND_ID   ( CM_EEGUSERSCALE,              CmSetScaling ),
    EV_COMMAND_AND_ID   ( CM_EEGRESETSCALING,           CmSetScaling ),
    EV_COMMAND_AND_ID   ( CM_EEGSETOFFSET,              CmSetOffset ),
    EV_COMMAND_AND_ID   ( CM_EEGRESETOFFSET,            CmSetOffset ),

    EV_COMMAND          ( CM_EEGMRKADDFILE,             CmAddFileMarkers ),
    EV_COMMAND          ( CM_EEGMRKDELETE,              CmDeleteMarker ),
    EV_COMMAND          ( CM_EEGMRKDELETENAME,          CmDeleteMarkersByName ),
    EV_COMMAND          ( CM_EEGMRKTRIGGERTOMARKER,     CmDuplicateTriggersToMarkers ),
    EV_COMMAND          ( CM_EEGMRKSPLIT,               CmSplitMarkers ),
    EV_COMMAND          ( CM_EEGMRKOVERWRITENAMES,      CmRenameOverwriteMarker ),
    EV_COMMAND          ( CM_EEGMRKSUBSTITUTENAMES,     CmRenameSubstringMarkers ),
    EV_COMMAND          ( CM_EEGMRKREGEXPNAMES,         CmRenameRegexpMarkers ),
    EV_COMMAND          ( CM_EEGMRKMERGEOVERLAPPING,    CmMergeOverlappingMarkers ),
    EV_COMMAND          ( CM_EEGMRKMERGECONTIGUOUS,     CmMergeContiguousMarkers ),
    EV_COMMAND          ( CM_EEGMRKSELFILE,             CmSetMarkerFile ),
    EV_COMMAND          ( CM_EEGMRKCLEARALL,            CmClearMarkers ),
    EV_COMMAND_AND_ID   ( CM_EEGMRKNONE,                CmSetMarkerDisplay ),
    EV_COMMAND_AND_ID   ( CM_EEGMRKTRIGGER,             CmSetMarkerDisplay ),
    EV_COMMAND_AND_ID   ( CM_EEGMRKEVENT,               CmSetMarkerDisplay ),
    EV_COMMAND_AND_ID   ( CM_EEGMRKMARKER,              CmSetMarkerDisplay ),
    EV_COMMAND_AND_ID   ( CM_EEGMRKALL,                 CmSetMarkerDisplay ),
    EV_COMMAND_ENABLE   ( CM_EEGMRKTRIGGER,             CmSetMarkerDisplayTEnable ),
    EV_COMMAND_ENABLE   ( CM_EEGMRKEVENT,               CmSetMarkerDisplayEEnable ),
    EV_COMMAND_ENABLE   ( CM_EEGMRKMARKER,              CmSetMarkerDisplayMEnable ),
    EV_COMMAND          ( CM_EEGMRKMANAGE,              CmManageMarkers ),
    EV_COMMAND_AND_ID   ( CM_EEGMRKSEARCH,              CmSearchMarker ),
    EV_COMMAND_AND_ID   ( CM_EEGMRKSEARCHNEXT,          CmSearchMarker ),
    EV_COMMAND_AND_ID   ( CM_EEGMRKSEARCHPREVIOUS,      CmSearchMarker ),
    EV_COMMAND          ( CM_EEGMRKDISPLAYFILTER,       CmSetMarkerFilter ),
    EV_COMMAND          ( CM_EEGMRKGENERATE,            CmGenerateMarkers ),
    EV_COMMAND          ( CM_EEGMRKSCANBADEPOCHS,       CmScanForBadEpochs ),

    EV_COMMAND          ( CM_EEGPENSIZE,                CmPenSize ),

    EV_COMMAND_AND_ID   ( CM_KEEPSEL,                   CmSelection ),
    EV_COMMAND_AND_ID   ( CM_REMOVESEL,                 CmSelection ),
    EV_COMMAND_AND_ID   ( CM_CLEARSEL,                  CmSelection ),
    EV_COMMAND_AND_ID   ( CM_SELECTALL,                 CmSelection ),
    EV_COMMAND_AND_ID   ( CM_SELECTALLVISIBLE,          CmSelection ),
    EV_COMMAND_AND_ID   ( CM_INVERTSEL,                 CmSelection ),
    EV_COMMAND_AND_ID   ( CM_INPUTSEL,                  CmSelection ),
    EV_COMMAND_AND_ID   ( CM_ABOVESEL,                  CmSelection ),
    EV_COMMAND_AND_ID   ( CM_SHOWSELNAMES,              CmSelection ),
    EV_COMMAND_AND_ID   ( CM_SHOWSELINDEXES,            CmSelection ),

    EV_COMMAND_AND_ID   ( CM_BADSET,                    CmSelection ),
    EV_COMMAND_AND_ID   ( CM_BADRESET,                  CmSelection ),
    EV_COMMAND_AND_ID   ( CM_BADNONE,                   CmSelection ),
    EV_COMMAND_AND_ID   ( CM_BADKEEP,                   CmSelection ),
    EV_COMMAND_AND_ID   ( CM_BADEXCLUDE,                CmSelection ),
    EV_COMMAND_AND_ID   ( CM_BADSELECT,                 CmSelection ),
    EV_COMMAND_AND_ID   ( CM_BADDESELECT,               CmSelection ),
    EV_COMMAND_AND_ID   ( CM_BADSELECTNON,              CmSelection ),
    EV_COMMAND_AND_ID   ( CM_BADAUXSELECTNON,           CmSelection ),

    EV_COMMAND_AND_ID   ( CM_AUXSET,                    CmSelection ),
    EV_COMMAND_AND_ID   ( CM_AUXRESET,                  CmSelection ),
    EV_COMMAND_AND_ID   ( CM_AUXNONE,                   CmSelection ),
    EV_COMMAND_AND_ID   ( CM_AUXTODEFAULT,              CmSelection ),
    EV_COMMAND_AND_ID   ( CM_AUXKEEP,                   CmSelection ),
    EV_COMMAND_AND_ID   ( CM_AUXEXCLUDE,                CmSelection ),
    EV_COMMAND_AND_ID   ( CM_AUXSELECT,                 CmSelection ),
    EV_COMMAND_AND_ID   ( CM_AUXDESELECT,               CmSelection ),
    EV_COMMAND_AND_ID   ( CM_AUXSELECTNON,              CmSelection ),

    EV_COMMAND          ( CM_EXPORTTRACKS,              CmExportTracks ),       // from "Tools" menu
    EV_COMMAND          ( CM_FREQANALYSIS,              CmFreqAnalysis ),
    EV_COMMAND          ( CM_INTERPOLATE,               CmInterpolateTracks ),
    EV_COMMAND          ( CM_RISTOVOLUME,               CmRisToVolume ),
    EV_COMMAND          ( CM_RISTOPOINTS,               RisToCloudVectorsUI ),
    EV_COMMAND          ( CM_SCANTRIGGERS,              CmScanTriggers ),

    EV_COMMAND          ( CM_EEGBASELINE,               CmBaseline ),

//  EV_COMMAND          ( CM_EEGRESETFILLING,           CmResetFilling ),
    EV_COMMAND          ( CM_EEGVOIDFILLING,            CmVoidFilling ),
    EV_COMMAND_ENABLE   ( CM_EEGVOIDFILLING,            CmFillingEnable ),

    EV_COMMAND          ( CM_EEGNEXTSESSION,            CmNextSession ),
    EV_COMMAND_ENABLE   ( CM_EEGNEXTSESSION,            CmNextSessionEnable ),

    EV_COMMAND          ( CM_EASYPUBLISH,               CmEasyPublish ),

    EV_COMMAND_AND_ID   ( CM_OPENSEG,                   CmOpenSeg ),
    EV_COMMAND_AND_ID   ( CM_OPENTEMPLATES,             CmOpenSeg ),
    EV_COMMAND_AND_ID   ( CM_OPENFITTINGTEMPLATES1,     CmOpenSeg ),
    EV_COMMAND_AND_ID   ( CM_OPENFITTINGTEMPLATES2,     CmOpenSeg ),
    EV_COMMAND_AND_ID   ( CM_OPENSEGVERBOSE,            CmOpenSeg ),
    EV_COMMAND_AND_ID   ( CM_OPENSTATSFITTING,          CmOpenSeg ),

    EV_COMMAND_AND_ID   ( CM_SELECTSEGMENTSFROMLIST,    CmSelectSegments ),
    EV_COMMAND_AND_ID   ( CM_SELECTSEGMENTSFROMCURSOR,  CmSelectSegments ),
    EV_COMMAND_AND_ID   ( CM_NEXTSEGMENT,               CmNextSegment ),
    EV_COMMAND_AND_ID   ( CM_PREVSEGMENT,               CmNextSegment ),
    EV_COMMAND_ENABLE   ( CM_SELECTSEGMENTSFROMLIST,    CmSegmentEnable ),
    EV_COMMAND_ENABLE   ( CM_SELECTSEGMENTSFROMCURSOR,  CmSegmentEnable ),
    EV_COMMAND_ENABLE   ( CM_NEXTSEGMENT,               CmSegmentEnable ),
    EV_COMMAND_ENABLE   ( CM_PREVSEGMENT,               CmSegmentEnable ),

    EV_COMMAND          ( CM_SETTIMEWINDOW,             CmSetTimeWindow ),
    EV_COMMAND          ( CM_SELECTALLTIME,             CmSelectAllTime ),
    EV_COMMAND_AND_ID   ( CM_EEGTIMEABSOLUTE,           CmTimeRef ),
    EV_COMMAND_AND_ID   ( CM_EEGTIMERELATIVE,           CmTimeRef ),
    EV_COMMAND_AND_ID   ( CM_EEGTIMERELATIVETO,         CmTimeRef ),
    EV_COMMAND          ( CM_EEGTIME0,                  CmGotoTimeOrigin ),
    EV_COMMAND_ENABLE   ( CM_EEGTIMEABSOLUTE,           CmTimeRefAbsEnable ),
    EV_COMMAND_ENABLE   ( CM_EEGTIMERELATIVE,           CmTimeRefRelEnable ),
    EV_COMMAND_ENABLE   ( CM_EEGTIMERELATIVETO,         CmTimeRefRelToEnable ),
    EV_COMMAND          ( CM_EEGSHOWDATE,               CmShowDate ),
    EV_COMMAND          ( CM_EEGSHOWTIME,               CmShowTime ),
    EV_COMMAND_ENABLE   ( CM_EEGSHOWDATE,               CmShowDateEnable ),
    EV_COMMAND_ENABLE   ( CM_EEGSHOWTIME,               CmShowTimeEnable ),
    EV_COMMAND          ( CM_GOTOTIME,                  CmGotoTime ),
    EV_COMMAND          ( CM_EEGSAMPLINGFREQUENCY,      CmSetSamplingFrequency ),
    EV_COMMAND_ENABLE   ( CM_EEGSAMPLINGFREQUENCY,      CmFilterEnable ),

    EV_COMMAND          ( IDB_DISPLAYINTENSITY,         CmSetIntensityMode ),
    EV_COMMAND_AND_ID   ( IDB_ISINCBRIGHT,              CmZoomVert ),
    EV_COMMAND_AND_ID   ( IDB_ISDECBRIGHT,              CmZoomVert ),
    EV_COMMAND_ENABLE   ( IDB_ISINCBRIGHT,              CmSetBrightnessEnable ),
    EV_COMMAND_ENABLE   ( IDB_ISDECBRIGHT,              CmSetBrightnessEnable ),
    EV_COMMAND_AND_ID   ( IDB_ISINCCONTRAST,            CmSetContrast ),
    EV_COMMAND_AND_ID   ( IDB_ISDECCONTRAST,            CmSetContrast ),
    EV_COMMAND_ENABLE   ( IDB_ISINCCONTRAST,            CmSetContrastEnable ),
    EV_COMMAND_ENABLE   ( IDB_ISDECCONTRAST,            CmSetContrastEnable ),
    EV_COMMAND          ( IDB_FIXEDSCALE,               CmSetScalingAdapt ),
    EV_COMMAND          ( IDB_SPCOLOR,                  CmNextColorTable ),
    EV_COMMAND_ENABLE   ( IDB_FIXEDSCALE,               CmSetIntensityEnable ),
    EV_COMMAND_ENABLE   ( IDB_SPCOLOR,                  CmSetIntensityEnable ),

    EV_COMMAND_ENABLE   ( IDB_TRACKSSUPER,              CmTracksEnable ),

    EV_COMMAND          ( IDB_NEXTROI,                  CmNextRois ),
    EV_COMMAND          ( IDB_AVERAGEROIS,              CmAverageRois ),
    EV_COMMAND_ENABLE   ( IDB_NEXTROI,                  CmNextRoisEnable ),
    EV_COMMAND_ENABLE   ( IDB_AVERAGEROIS,              CmAverageRoisEnable ),

    EV_COMMAND_AND_ID   ( CM_ANALYZESELFLAT,            CmAnalyzeTracks ),
    EV_COMMAND_AND_ID   ( CM_ANALYZESELHIGH,            CmAnalyzeTracks ),
    EV_COMMAND_AND_ID   ( CM_ANALYZESELGFP,             CmAnalyzeTracks ),
//  EV_COMMAND_AND_ID   ( CM_ANALYZESELNOISY,           CmAnalyzeTracks ),
    EV_COMMAND_AND_ID   ( CM_ANALYZESELARTEFACTS,       CmAnalyzeTracks ),

    EV_COMMAND_AND_ID   ( CM_HISTOTIMERAW,              CmHistogram ),
    EV_COMMAND_AND_ID   ( CM_HISTOTIMESMOOTH,           CmHistogram ),
    EV_COMMAND_AND_ID   ( CM_HISTOTIMELOG,              CmHistogram ),
    EV_COMMAND_AND_ID   ( CM_HISTOTIMECDF,              CmHistogram ),
    EV_COMMAND_AND_ID   ( CM_HISTOTRACKSRAW,            CmHistogram ),
    EV_COMMAND_AND_ID   ( CM_HISTOTRACKSSMOOTH,         CmHistogram ),
    EV_COMMAND_AND_ID   ( CM_HISTOTRACKSLOG,            CmHistogram ),
    EV_COMMAND_AND_ID   ( CM_HISTOTRACKSCDF,            CmHistogram ),
    EV_COMMAND_AND_ID   ( CM_HISTOALLRAW,               CmHistogram ),
    EV_COMMAND_AND_ID   ( CM_HISTOALLSMOOTH,            CmHistogram ),
    EV_COMMAND_AND_ID   ( CM_HISTOALLLOG,               CmHistogram ),
    EV_COMMAND_AND_ID   ( CM_HISTOALLCDF,               CmHistogram ),

END_RESPONSE_TABLE;


        TTracksView::TTracksView ( TTracksDoc &doc, TWindow *parent, TLinkManyDoc *group, bool isderived )
      : TBaseView ( doc, parent, group ), EEGDoc ( &doc )
{
XYZDoc              = 0;
CurrXyz             = 0;
                                        // currently SPDoc is used only for SP names, not any 3D rendering
SPDoc               = 0;
CurrSp              = 0;

Rois                = 0;
CurrRois            = 0;

LButtonDown         = false;
LastSelectedTrack   = -1;
Filling             = 0;
NumFilling          = 0;
ScrollBar           = 0;
SelFilling          = 0;
DrawTFCursorDC      = 0;
CurrentWindowSlot   = 0;

                                        // Display could handle empty files but with the cost of a lot of tests - is it worth it? just warn the user
                                        // Note that this is only the view failing - opening viewless should still work, to the responsibility of the caller...
if ( EEGDoc->GetNumElectrodes () == 0 
  || EEGDoc->GetNumTimeFrames () == 0 ) {

    ShowMessage ( "File appears to be empty!", ToFileName ( EEGDoc->GetDocPath () ), ShowMessageWarning, 0 );
    NotOK (); 
    return;                             // exit view
    }                


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get max size for display
BuffSize            = NoMore ( EEGDoc->GetNumTimeFrames (), (long) EegMaxPointsDisplay );

CDPt                = TInterval ( 0, EEGDoc->GetNumTimeFrames () - 1, BuffSize );
CDP                 = &CDPt;
SelSize             = EEGDoc->GetTotalElectrodes ();


TFCursor            = TTFCursor ( EEGDoc,
                                  CDPt.GetLimitMin (), CDPt.GetLimitMax (),
                                  CDPt.GetLimitMin ()                           // opening all files at the beginning
//                                  EEGDoc->IsContentType ( ContentTypeSeg )
//                               || EEGDoc->IsP ()     
////                             || EEGDoc->IsContentType ( ContentTypeErrorData )  // .error.data: nope, use fake MaxGfpTF, which is the last track, the best criterion
//                               || EEGDoc->IsTemplates () ? CDPt.GetLimitMin ()
//                                                         : EEGDoc->GetMaxGfpTF ()
                                 );

if ( EEGDoc->IsContentType ( ContentTypeErrorData ) && EEGDoc->GetNumMarkers () >= 1 )
    TFCursor.SetPos ( (*EEGDoc)[ 0 ]->From );


TFCursor.SentTo     = 0;
TFCursor.SentFrom   = 0;

LastCursorPosMin    = -1;
LastCursorPosMax    = -1;
                                        // a general offset, to view around this specific value
OffsetTracks        = 0;

ClearString ( MarkerFilter );
ClearString ( MarkerSearch );
LastTagIndex        = -1;               // used to tab-jump across markers

                                        // set opening display range
if ( EEGDoc->GetSamplingFrequency () >= 100.0 )
    CDPt.SetLength ( NoMore ( CDPt.GetLimitLength (), (ulong) EEGDoc->GetSamplingFrequency () * 10 ), TFCursor.GetPosMin(), true );
else
    CDPt.SetLength ( NoMore ( CDPt.GetLimitLength (), (ulong) 10000 ), TFCursor.GetPosMin(), true );

ComputeSTHfromCDPt ();

SyncCDPtTFCursor    = false;
//SyncCDPtTFCursor    = true;
//CheckCDPtAndTFCursor ( false );


DisplayMinTF        = EEGGLVIEW_STHMINTF;
TracksSuper         = false;
AverageRois         = false;
TracksFlipVert      = false;
ShowSD              = SDNone;

ShowVertScale       = false;
ShowHorizScale      = NoTimeDisplay;
                                        // Horizontal scale can be either time, or some linear transform
ShowHorizScaleLegal = EEGDoc->IsContentType ( ContentTypeHistogram )    ? TimeDisplayLinear
                                                                        : TimeDisplayTime;  // all recording, spontaneous EEG, ERPs, frequency files..

ShowBaseline        = true;
ShowDate            = EEGDoc->DateTime.IsOriginDateAvailable ();
ShowTime            = EEGDoc->GetSamplingFrequency ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

DisplayMarkerType   = CombineFlags ( MarkerTypeTrigger, MarkerTypeUserCoded );  // AllMarkerTypes;
ShowTags            = EEGDoc->GetNumMarkers ( DisplayMarkerType );              // any marker available ?

                                        // retrieve marker names
MarkerType          markertype;

if      ( EEGDoc->GetNumMarkers (                                   MarkerTypeUserCoded )   == 0              )  markertype =                MarkerTypeTrigger;
else if ( EEGDoc->GetNumMarkers ( CombineFlags ( MarkerTypeTrigger, MarkerTypeUserCoded ) ) <= NumUserMarkers )  markertype = CombineFlags ( MarkerTypeTrigger, MarkerTypeUserCoded );
else                                                                                                             markertype =                                   MarkerTypeUserCoded;

EEGDoc->GetMarkerNames ( MarkerStrings, markertype, 50 /*200*/ );

//MarkerStrings.Show ( "MarkerStrings" );

                                        // add default marker name
MarkerStrings.AddNoDuplicates ( MarkerNameDefault );
MarkerStrings.Sort ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set the margin for text
SetTextMargin ();

                                        // OpenGL
RenderingMode       = LayoutOnePage;
DisplayMode         = EEGDoc->GetNumTimeFrames() <= 1 ? DisplayBars : DisplayTracks;

CurrentDisplaySpace = DisplaySpaceNone;
LineWidth           = 0;                // set to smart width
Zoom                = 1;

Orientation         = DefaultPotentialOrientation - 1;


SetColorTable ( EEGDoc->GetAtomType ( AtomTypeUseCurrent ) );


BackColor.Set ( 0, (GLfloat) 1, (GLfloat) 1, (GLfloat) 1, (GLfloat) 1 );
BackColor.Set ( 1, (GLfloat) 1, (GLfloat) 1, (GLfloat) 1, (GLfloat) 1 );
BackColor.Set ( 2, (GLfloat) 1, (GLfloat) 1, (GLfloat) 1, (GLfloat) 0 );        // need a transparent color

LineColor.Set ( 0, 0.00, 0.00, 0.00, 1.00 );
LineColor.Set ( 1, 0.50, 0.50, 0.75, 1.00 );
LineColor.Set ( 2, 0.66, 0.66, 1.00, 1.00 );

TextColor.Set ( 1, 1.00, 1.00, 1.00, 1.00 );
TextColor.Set ( 0, 0.00, 0.00, 0.00, 1.00 );
TextColor.Set ( 3, 0.75, 0.75, 1.00, 1.00 );
TextColor.Set ( 2, 0.20, 0.20, 0.60, 1.00 );

                                        // linked tracks coloring
LinkColors[ 0 ].Set ( TrackColor );
LinkColors[ 1 ].Set ( 1.00, 0.00, 0.00, 1.00 ); // red
LinkColors[ 2 ].Set ( 0.00, 0.60, 0.00, 1.00 ); // dark green
LinkColors[ 3 ].Set ( 0.00, 0.00, 1.00, 1.00 ); // blue
LinkColors[ 4 ].Set ( 0.00, 0.70, 0.70, 1.00 ); // dark cyan
LinkColors[ 5 ].Set ( 0.60, 0.00, 0.60, 1.00 ); // dark magenta
LinkColors[ 6 ].Set ( 0.85, 0.85, 0.00, 1.00 ); // dark yellow
                                        // same colors, but drawn with a dash pattern
for ( int i = DashLinkColors; i < MaxLinkColors; i++ )
    LinkColors[ i ] = LinkColors[ i - DashLinkColors ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // exit if a bad document is provided (cancel)
if ( ! ValidView () ) { 
    NotOK (); 
    return;                             // skip allocation stuff
    }                


if ( ! isderived ) {

    EegBuff.Resize ( 1, EEGDoc->GetTotalElectrodes (), BuffSize );

    if ( EEGDoc->HasStandardDeviation () )
        SdBuff.Resize ( EEGDoc->GetTotalElectrodes (), BuffSize );  // !force Dim1 with pseudo-tracks so both buffers have exactly the same memory footprint!
    }

TFCursor.SentTo     = 0;
TFCursor.SentFrom   = GetViewId ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // all TSelection should be of the same type, as we can pick either one of them f.ex. to copy values
                                        // init as sorted, can be switched to other type later on
SelTracks           = TSelection ( EEGDoc->GetTotalElectrodes (), OrderSorted );
SelTracks.Set ();

if ( EEGDoc->HasPseudoElectrodes () ) {

    EEGDoc->ClearPseudo ( SelTracks );

    if ( EEGDoc->GetNumRegularElectrodes () > 1 ) {
        SelTracks.Set ( EEGDoc->GetGfpIndex () );
        SelTracks.Set ( EEGDoc->GetDisIndex () );
        }
//  else if ( StringContains ( EEGDoc->GetElectrodeName ( EEGDoc->GetNumElectrodes () - 1 ), "REF" ) )  // remove VREF from MFF\signal2.bin - why is that later resetted?
//      SelTracks.Reset ( EEGDoc->GetNumElectrodes () - 1 );
    }


Highlighted         = TSelection ( EEGDoc->GetTotalElectrodes (), OrderSorted );
Highlighted.Reset ();
Highlighted.SentTo  = 0;
Highlighted.SentFrom= GetViewId ();


BadTracks           = TSelection ( EEGDoc->GetTotalElectrodes (), OrderSorted );
BadTracks.Reset ();
BadTracks.SentTo    = 0;
BadTracks.SentFrom  = GetViewId ();

                                        // get any setup from the original document
BadTracks           = EEGDoc->GetBadTracks ();
AuxTracks           = EEGDoc->GetAuxTracks ();


LastVisibleTrack    = 0;
ScaleTracks.Resize ( EEGDoc->GetTotalElectrodes () );

if ( ! isderived ) { // i.e. tracks display, not frequency display

    ReloadBuffers    ();

    ResetScaleTracks ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

HasFilling          = false;
IsFilling           = FillingNone;
NumFilling          = EEGDoc->GetTotalElectrodes ();
Filling             = new TGLColoring * [ NumFilling ];
for ( int e = 0; e < NumFilling; e++ )
    Filling[ e ] = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Set window size according to content size
if ( BuffSize > EegMaxPointsDisplay / 2
  || ( EEGDoc->GetSamplingFrequency () && TimeFrameToSeconds ( EEGDoc->GetNumTimeFrames (), EEGDoc->GetSamplingFrequency () ) > 50 )
  || EEGDoc->IsSpontaneous () ) {
                                        // Wider size
    Attr.H      = Clip ( Round ( GetWindowMinSide ( CartoolMdiClient ) * WindowHeightRatio ), MinWindowHeight, MaxWindowHeightLarge );

    Attr.W      = Attr.H / WindowHeightToWidthRatio * 2;

                                        // don't be too wide!
    if ( Attr.W > crtl::GetWindowWidth ( CartoolMdiClient ) ) {
        Attr.H *= (double) crtl::GetWindowWidth ( CartoolMdiClient ) / Attr.W;
        Attr.W  = crtl::GetWindowWidth ( CartoolMdiClient );
        }

    StandSize   = TSize ( TracksBigWindowSizeW, TracksBigWindowSizeH );
//  Attr.H      = StandSize.Y ();
//  Attr.W      = StandSize.X ();
    }
else {
                                        // Regular size
    Attr.H      = Clip ( Round ( GetWindowMinSide ( CartoolMdiClient ) * WindowHeightRatio ), MinWindowHeight, MaxWindowHeight );

    Attr.W      = EEGDoc->GetNumElectrodes () == EEGDoc->GetNumTimeFrames () ? Attr.H / 1.10    // if squared array of data, we might be looking at a kind of matrix, switch to a square display
                                                                             : Attr.H / WindowHeightToWidthRatio;

                                        // don't be too wide!
    if ( Attr.W > crtl::GetWindowWidth ( CartoolMdiClient ) ) {
        Attr.H *= (double) crtl::GetWindowWidth ( CartoolMdiClient ) / Attr.W;
        Attr.W  = crtl::GetWindowWidth ( CartoolMdiClient );
        }

    StandSize   = TSize ( TracksWindowSizeW, TracksWindowSizeH );
//  Attr.H      = StandSize.Y ();
//  Attr.W      = StandSize.X ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do some display tunings according to the file characteristics
TFileName           ext2;
                                        // get second-to-last "extension"
StringCopy      ( ext2, EEGDoc->GetTitle () );
RemoveExtension ( ext2 );
GetExtension    ( ext2, ext2 );

                                        // .t .p .sd  etc...
if ( EEGDoc->IsSpecialInfix () 
  || EEGDoc->IsP ()            ) {

    EEGDoc->ClearPseudo ( SelTracks );


    if ( EEGDoc->IsP () || IsStringAmong ( ext2, InfixP " " InfixT " " InfixDelta ) ) {
                                        // bar mode?
//      SetBarsMode ();
                                        // or intensity mode?
        SetIntensitySquareMode ();
        ScalingAuto     = ScalingAutoSymmetric;

//      if ( EEGDoc->IsContentType ( ContentTypeData ) ) {
//          Attr.H      = min ( Width ( CartoolMdiClient ), Height ( CartoolMdiClient ) ) * WindowHeightRatio;
//          Attr.W      = Attr.H / WindowHeightToWidthRatio * 2;
//          }
        }

                                        // overriding color tables type
    if ( EEGDoc->IsPositive ( AtomTypeUseCurrent ) )
        SetColorTable ( AtomTypePositive );


    SetIntensityPrecise ();

    } // SpecialFilesInfix


else if ( EEGDoc->IsContentType ( ContentTypeSeg ) ) {

    HasFilling  = true;                 // can fill
    IsFilling   = FillingColor;         // with color by default

                                        // smart preselect tracks: GFPs, otherwise DISs, else all
    SelTracks.Reset ();

    for ( int e=0; e < EEGDoc->GetTotalElectrodes (); e++ )
        if ( StringStartsWith ( GetElectrodeName ( e ), TrackNameGFP )
          || StringStartsWith ( GetElectrodeName ( e ), TrackNameRMS ) )
            SelTracks.Set ( e );

    if ( SelTracks.NumSet () == 0 )
        for ( int e=0; e < EEGDoc->GetTotalElectrodes (); e++ )
            if ( StringStartsWith ( GetElectrodeName ( e ), TrackNameDIS )
              || StringStartsWith ( GetElectrodeName ( e ), TrackNameDISPlus ) )
                SelTracks.Set ( e );

    if ( SelTracks.NumSet () == 0 )
        SelTracks.Set ();

                                        // get max GEV for a good color scaling
    float       maxgev      = 0;
    for ( int e = 0; e < EEGDoc->GetTotalElectrodes (); e++ )
        if ( StringStartsWith ( GetElectrodeName ( e ), TrackNameGEV ) )
            for ( int tf = 0; tf < BuffSize; tf++ )
                Maxed ( maxgev, EegBuff[ e ][ tf ] );


    for ( int e = 0; e < EEGDoc->GetTotalElectrodes (); e++ ) {

        if ( StringStartsWith ( GetElectrodeName ( e ), TrackNameGFP        )
          || StringStartsWith ( GetElectrodeName ( e ), TrackNameRMS        )
          || StringStartsWith ( GetElectrodeName ( e ), TrackNameDIS        )
          || StringStartsWith ( GetElectrodeName ( e ), TrackNameDISPlus    )
          || StringStartsWith ( GetElectrodeName ( e ), TrackNamePolarity   )
          || StringStartsWith ( GetElectrodeName ( e ), TrackNameSeg        )   // set a default coloring, maybe overriden later
          || StringStartsWith ( GetElectrodeName ( e ), TrackNameGEV        )
          || StringStartsWith ( GetElectrodeName ( e ), TrackNameCorr       ) ) {

            for ( int f = e; f < EEGDoc->GetTotalElectrodes (); ) {

                if ( StringStartsWith ( GetElectrodeName ( f ), TrackNameSeg ) ) {
                                        // force from 1 to NumClusters to get all same colors
                    SetColoring ( e, f, Discrete, 
                                  TGLColor<GLfloat> ( 0.25, 0.25, 0.25, 1.0 ),
                                  TGLColor<GLfloat> ( 0.75, 0.75, 0.75, 1.0 ),
                                  1, AtLeast ( 1, ((TSegDoc *) EEGDoc)->GetNumClusters () ) );
                    break;
                    }

                f   += StringStartsWith ( GetElectrodeName ( e ), TrackNameCorr ) 
                    || StringStartsWith ( GetElectrodeName ( e ), TrackNameGEV  ) ? -1 : 1;
                }
            }


        if ( StringStartsWith ( GetElectrodeName ( e ), TrackNameSeg ) )

            for ( int f = e + 1; f < EEGDoc->GetTotalElectrodes (); f++ )

                if ( StringStartsWith ( GetElectrodeName ( f ), TrackNameGEV ) ) {

                    SetColoring ( e, f, Analog, 
                                  TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 1.0 ), 
                                  TGLColor<GLfloat> ( 1.00, 1.00, 0.00, 1.0 ), 
                                  0.0, maxgev );
                    break;
                    }
        }


    Attr.H      = NoMore ( Round ( GetWindowMinSide ( CartoolMdiClient ) * WindowHeightRatio ), MaxWindowHeightLarge );

    Attr.W      = Attr.H / WindowHeightToWidthRatio;
                                        // relatively adjust height to # of tracks
    Attr.H     *= Clip ( (int) SelTracks + 1, 1, 10 ) / 10.0;


    if ( EEGDoc->IsPositive ( AtomTypeUseCurrent ) )
        SetColorTable ( AtomTypePositive );

    SetIntensityPrecise ();

                                        // does it have at least one discrete segment?
    bool    isdiscrete = false;

    for ( int e = 0; e < EEGDoc->GetNumElectrodes (); e++ ) {
        isdiscrete  = Filling[ e ]->GetHow () == Discrete;
        if ( isdiscrete ) break;
        }

    if ( isdiscrete ) {
        SelFilling      = new TSelection ( ((TSegDoc *) EEGDoc )->GetNumClusters() + 1, OrderSorted );
        SelFilling->Set   ();
        SelFilling->Reset ( 0 );
        }

    } // Seg file


else if ( EEGDoc->IsContentType ( ContentTypeData      ) 
       || EEGDoc->IsContentType ( ContentTypeErrorData ) ) {
                                        // like .error.data from Segmentation
    SelTracks.Reset ();

    for ( int i=0; i < EEGDoc->GetTotalElectrodes(); i++ ) {
                                        // test for the tracks NOT to be highlighted
//        if ( ! IsStringAmong ( GetElectrodeName ( i ), TrackNameClust" "TrackNameMaps" "TrackNameGEV" "TrackNameWforKL ) )
//            Highlighted.Set ( i );
////            SelTracks.Set ( i );

                                        // criterion used for big mix
//      if ( IsStringAmong ( GetElectrodeName ( i ), TrackNameGamma
//                                               " " TrackNameGPlus
//                                               " " TrackNameSilhouettes
//                                               " " TrackNameCalinskiHarabasz
//                                               " " TrackNameCIndex
//                                               " " TrackNameDunnRobust
//                                               " " TrackNameDaviesBouldin
//                                               " " TrackNameFreyVanGroenewoud
//                                               " " TrackNameMarriott
//                                               " " TrackNamePointBiserial
//                                               " " TrackNameHartigan
//                                               " " TrackNameRatkowski
//                                               " " TrackNameMcClain       ) )
//          Highlighted.Set ( i );

                                        // mixes of criterion
        if ( IsStringAmong ( GetElectrodeName ( i ), TrackNameAllScores
                                                 " " TrackNameAllVotes
                                                 " " TrackNameAllRanks
                                                 " " TrackNameMeanRanks
                                                 " " TrackNameMetaCriterion
                                                 " " TrackNameArgMaxHisto
                                                 " " TrackNameAllInAll     ) ) {
            SelTracks.Set ( i );

            SetBarsMode ();

            if ( IsStringAmong ( GetElectrodeName ( i ), TrackNameAllRanks " " TrackNameMetaCriterion " " TrackNameAllInAll ) )
                Highlighted.Set ( i );
            }
        }

                                        // some old .data could have nothing selected, show everything in that case
    if ( (int) SelTracks == 0 )
        SelTracks.Set ();

                                        // maybe check for ContentTypeErrorData
    if ( EEGDoc->IsPositive ( AtomTypeUseCurrent ) )
        SetColorTable ( AtomTypePositive );

    SetIntensityPrecise ();

//  LineWidth           = 2.0;

                                        // relatively adjust height to # of tracks
    Attr.H     *= Clip ( (int) SelTracks + 1, 1, 10 ) / 8.0;

//  Attr.W      = Attr.H / WindowHeightToWidthRatio * 1.414;

    if  ( EEGDoc->IsContentType ( ContentTypeErrorData ) )
        ShowHorizScale  = ShowHorizScaleLegal;
    } // Data file


else if ( EEGDoc->IsContentType ( ContentTypeHistogram ) ) {

    EEGDoc->ClearPseudo ( SelTracks );

    if ( EEGDoc->GetNumElectrodes () < 50 )
        SetBarsMode ();


    Attr.H      = Clip ( Round ( GetWindowMinSide ( CartoolMdiClient ) * WindowHeightRatio ), MinWindowHeight, MaxWindowHeight );

    Attr.W      = Attr.H / WindowHeightToWidthRatio * 1.5;
                                        // relatively adjust height to # of tracks
    Attr.H     *= Clip ( sqrt ( (double) SelTracks.NumSet () ) + 1, 1.0, 10.0 ) / 4.0;
    Mined ( Attr.H, MaxWindowHeight );

    ShowHorizScale  = ShowHorizScaleLegal;
    }

                                        // general case
else {

    if ( IsExtensionAmong ( EEGDoc->GetTitle (), FILEEXT_EEGEP " " FILEEXT_EEGEPH " " FILEEXT_EEGSEF " " FILEEXT_RIS )
      && EEGDoc->GetNumRegularElectrodes () < 32 )

        EEGDoc->ClearPseudo ( SelTracks );

                                        // to have correct color wrapping
    if ( EEGDoc->IsAngular ( AtomTypeUseCurrent ) ) {
        SetIntensityPrecise ();
        ScalingAuto     = ScalingAutoSymmetric;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setting scaling absolute limits
SetScalingLimits    ( EEGGLVIEW_STVMIN, EEGGLVIEW_STVMAX );
                                        // Needs SelTracks and Attr
ResetScalingLevel   ();

ResetScalingContrast();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


ScrollBar              = new TTracksViewScrollbar ( this, this, IDB_HSCROLL, 0, 0, 130, EegScrollbarHeight, true );


if ( ! isderived ) {
                                        // set menu  TMenu
    SetViewMenu ( new TMenuDescr ( IDM_EEG ) );

                                        // then add my local stuff (in reverse order)
    if      ( EEGDoc->IsContentType ( ContentTypeErrorData ) ) {

        GetViewMenu ()->InsertMenu ( CM_EEGUSERSCALE,           MF_BYCOMMAND | MF_SEPARATOR	, CM_SEPARATOR, "" );
        GetViewMenu ()->InsertMenu ( CM_SEPARATOR,              MF_BYCOMMAND, CM_OPENFITTINGTEMPLATES1, "Open F&itting Dialog for templates at cursor position" );
        GetViewMenu ()->InsertMenu ( CM_OPENFITTINGTEMPLATES1,  MF_BYCOMMAND, CM_OPENSEGVERBOSE,        "Open &Verbose at cursor position" );
        GetViewMenu ()->InsertMenu ( CM_OPENSEGVERBOSE,         MF_BYCOMMAND, CM_OPENTEMPLATES,         "Open Te&mplates at cursor position" );
        GetViewMenu ()->InsertMenu ( CM_OPENTEMPLATES,          MF_BYCOMMAND, CM_OPENSEG,               "Open Se&gmentation(s) at cursor position" );
        }

    else if ( EEGDoc->IsContentType ( ContentTypeSeg ) ) {
        GetViewMenu ()->InsertMenu ( CM_EEGUSERSCALE,           MF_BYCOMMAND | MF_SEPARATOR	, CM_SEPARATOR, "" );
        GetViewMenu ()->InsertMenu ( CM_SEPARATOR,              MF_BYCOMMAND, CM_OPENSTATSFITTING, "Open Statisti&cs for this Fitting" );
        GetViewMenu ()->InsertMenu ( CM_OPENSTATSFITTING,       MF_BYCOMMAND, CM_OPENFITTINGTEMPLATES2, "Open F&itting Dialog for this segmentation" );
        }
    }
}


//----------------------------------------------------------------------------
void    TTracksView::ResetScalingLevel ()
{
                                        // General case first
//SetScaling ( EEGGLVIEW_STVINIT * ( EEGDoc->IsAbsolute ( AtomTypeUseCurrent ) ? 1.5 : 1 ) ); // old formula

double          winheight       = Clip ( Attr.H / (double) MmToPixels ( 200 ), 0.10, 1.00 );    // formula accounts only for a window height between 2 and 20 cm
double          winwidth        = Clip ( Attr.W / (double) MmToPixels ( 300 ), 0.50, 1.00 ) ;   // formula accounts only for a window width between 15 and 30 cm
double          pagesize        = Clip ( CDPt.GetLength () / 500.0,            0.20, 1.00 );    // formula accounts only for a number of points between 100 and 500

SetScaling ( ( EEGGLVIEW_STVINIT * winheight * winwidth ) / ( 2.5 * pagesize ) );

                                        // Then we can refine for specific contents:
if ( EEGDoc->IsSpecialInfix () 
  || EEGDoc->IsP ()            ) {

    TFileName           ext2;
                                        // get second-to-last "extension"
    StringCopy      ( ext2, EEGDoc->GetTitle () );
    RemoveExtension ( ext2 );
    GetExtension    ( ext2, ext2 );


    if ( EEGDoc->IsP () 
      || IsStringAmong ( ext2, InfixP " " InfixT " " InfixDelta ) ) {
                                        // we want to clearly see the P values, with no overlaps
//      if ( EEGDoc->GetNumElectrodes () < Attr.H ) // enough room?
            SetScaling ( EEGGLVIEW_STVINIT * Attr.H / (double) NonNull ( 142 * sqrt ( (int) SelTracks ) ) );
        }
    } // SpecialFilesInfix

else if ( EEGDoc->IsContentType ( ContentTypeSeg        ) ) SetScaling ( EEGGLVIEW_STVINIT * Attr.H / (double) NonNull ( 125 * sqrt (       (int) SelTracks ) ) );
else if ( EEGDoc->IsContentType ( ContentTypeErrorData  ) ) SetScaling ( EEGGLVIEW_STVINIT * Attr.H / (double) NonNull (  66 *              (int) SelTracks )   );  
else if ( EEGDoc->IsContentType ( ContentTypeHistogram  ) ) SetScaling ( EEGGLVIEW_STVINIT * Attr.H / (double) NonNull (  33 * NoMore ( 50, (int) SelTracks ) ) );
}


void    TTracksView::ResetScalingContrast ()
{
                                        // General case first
SetScalingContrast ( ColorTableToScalingContrast (  1.0  ) );

                                        // .t .p .sd  etc...
if ( EEGDoc->IsSpecialInfix () 
  || EEGDoc->IsP ()            ) {

    TFileName           ext2;
                                            // get second-to-last "extension"
    StringCopy      ( ext2, EEGDoc->GetTitle () );
    RemoveExtension ( ext2 );
    GetExtension    ( ext2, ext2 );


    if      ( EEGDoc->IsP () 
           || IsStringAmong ( ext2, InfixP                ) )   SetScalingContrast ( ColorTableToScalingContrast ( 10.0  ) );

    else if ( IsStringAmong ( ext2, InfixT " " InfixDelta ) )   SetScalingContrast ( ColorTableToScalingContrast (  0.25 ) );
    } // SpecialFilesInfix

//else if ( EEGDoc->IsAngular ( AtomTypeUseCurrent ) )          SetScalingContrast ( ColorTableToScalingContrast (  1.0  ) );
}


//----------------------------------------------------------------------------
void    TTracksView::CreateGadgets ()
{
NumControlBarGadgets    = EEGGLVIEW_CBG_NUM;
ControlBarGadgets       = new TGadget * [ NumControlBarGadgets ];

CreateBaseGadgets ();

ControlBarGadgets[ EEGGLVIEW_CBG_SEP3               ]= new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ EEGGLVIEW_CBG_DISPLAYINTENSITY   ]= new TButtonGadgetDpi ( IDB_DISPLAYINTENSITY,     IDB_DISPLAYINTENSITY,   TButtonGadget::Command );
ControlBarGadgets[ EEGGLVIEW_CBG_NEXTROI            ]= new TButtonGadgetDpi ( IDB_NEXTROI,              IDB_NEXTROI,            TButtonGadget::NonExclusive, true, IsRoiMode () ? TButtonGadget::Down : TButtonGadget::Up, false );
ControlBarGadgets[ EEGGLVIEW_CBG_AVERAGEROIS        ]= new TButtonGadgetDpi ( IDB_AVERAGEROIS,          IDB_AVERAGEROIS,        TButtonGadget::NonExclusive, true, AverageRois  ? TButtonGadget::Down : TButtonGadget::Up, false );
                                                                                  
ControlBarGadgets[ EEGGLVIEW_CBG_SEP3A              ]= new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ EEGGLVIEW_CBG_SHOWTAGS           ]= new TButtonGadgetDpi ( IDB_SHOWMARKERS,          IDB_SHOWMARKERS,        TButtonGadget::NonExclusive, true, ShowTags     ? TButtonGadget::Down : TButtonGadget::Up, false );
ControlBarGadgets[ EEGGLVIEW_CBG_PREVMARKER         ]= new TButtonGadgetDpi ( IDB_PREVMARKER,           IDB_PREVMARKER,         TButtonGadget::Command);
ControlBarGadgets[ EEGGLVIEW_CBG_NEXTMARKER         ]= new TButtonGadgetDpi ( IDB_NEXTMARKER,           IDB_NEXTMARKER,         TButtonGadget::Command);
ControlBarGadgets[ EEGGLVIEW_CBG_ADDMARKER          ]= new TButtonGadgetDpi ( IDB_ADDMARKER,            IDB_ADDMARKER,          TButtonGadget::Command);
                                                                                  
ControlBarGadgets[ EEGGLVIEW_CBG_SEP3B              ]= new TSeparatorGadget ( DefaultSeparator );
//ControlBarGadgets[ EEGGLVIEW_CBG_SYNCZERO           ]= new TButtonGadgetDpi ( IDB_SYNCZERO,             IDB_SYNCZERO,           TButtonGadget::Command);
//ControlBarGadgets[ EEGGLVIEW_CBG_SEP3C              ]= new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ EEGGLVIEW_CBG_RNGCURS            ]= new TButtonGadgetDpi ( IDB_RANGECURSOR,          IDB_RANGECURSOR,        TButtonGadget::NonExclusive);
                                                                                  
ControlBarGadgets[ EEGGLVIEW_CBG_SEP4               ]= new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ EEGGLVIEW_CBG_UP                 ]= new TButtonGadgetDpi ( IDB_UP,                   IDB_UP,                 TButtonGadget::Command);
ControlBarGadgets[ EEGGLVIEW_CBG_DOWN               ]= new TButtonGadgetDpi ( IDB_DOWN,                 IDB_DOWN,               TButtonGadget::Command);
ControlBarGadgets[ EEGGLVIEW_CBG_NEXTSESSION        ]= new TButtonGadgetDpi ( IDB_NEXTSESSION,          CM_EEGNEXTSESSION,      TButtonGadget::Command);
                                                                                  
ControlBarGadgets[ EEGGLVIEW_CBG_SEP5               ]= new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ EEGGLVIEW_CBG_LESSTR             ]= new TButtonGadgetDpi ( IDB_LESSTRACKS,           IDB_LESSTRACKS,         TButtonGadget::Command);
ControlBarGadgets[ EEGGLVIEW_CBG_MORETR             ]= new TButtonGadgetDpi ( IDB_MORETRACKS,           IDB_MORETRACKS,         TButtonGadget::Command);
ControlBarGadgets[ EEGGLVIEW_CBG_LESSPSEUDOTRACKS   ]= new TButtonGadgetDpi ( IDB_LESSPSEUDOTRACKS,     IDB_LESSPSEUDOTRACKS,   TButtonGadget::Command);
ControlBarGadgets[ EEGGLVIEW_CBG_MOREPSEUDOTRACKS   ]= new TButtonGadgetDpi ( IDB_MOREPSEUDOTRACKS,     IDB_MOREPSEUDOTRACKS,   TButtonGadget::Command);

ControlBarGadgets[ EEGGLVIEW_CBG_SEP6               ]= new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ EEGGLVIEW_CBG_TRSUPER            ]= new TButtonGadgetDpi ( IDB_TRACKSSUPER,          IDB_TRACKSSUPER,        TButtonGadget::NonExclusive);
ControlBarGadgets[ EEGGLVIEW_CBG_FLIPVERT           ]= new TButtonGadgetDpi ( IDB_FLIPVERT,             IDB_FLIPVERT,           TButtonGadget::NonExclusive);
ControlBarGadgets[ EEGGLVIEW_CBG_SHOWSD             ]= new TButtonGadgetDpi ( IDB_SHOWSD,               IDB_SHOWSD,             TButtonGadget::Command, true, TButtonGadget::Up);
                                                                                  
//ControlBarGadgets[ EEGGLVIEW_CBG_SEP11              ]= new TSeparatorGadget ( DefaultSeparator );
//ControlBarGadgets[ EEGGLVIEW_CBG_STYLEW             ]= new TButtonGadgetDpi ( IDB_STYLEWINDOW,          IDB_STYLEWINDOW,        TButtonGadget::Command);
//ControlBarGadgets[ EEGGLVIEW_CBG_SPLITWINDOW        ]= new TButtonGadgetDpi ( IDB_SPLITWINDOW,          IDB_SPLITWINDOW,        TButtonGadget::Command);
                                                                                  
ControlBarGadgets[ EEGGLVIEW_CBG_SEP7               ]= new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ EEGGLVIEW_CBG_EXTTV              ]= new TButtonGadgetDpi ( IDB_EXTTRACKSV,           IDB_EXTTRACKSV,         TButtonGadget::Command);
ControlBarGadgets[ EEGGLVIEW_CBG_COMPTV             ]= new TButtonGadgetDpi ( IDB_COMPTRACKSV,          IDB_COMPTRACKSV,        TButtonGadget::Command);

ControlBarGadgets[ EEGGLVIEW_CBG_SEP8               ]= new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ EEGGLVIEW_CBG_EXTTH              ]= new TButtonGadgetDpi ( IDB_EXTTRACKSH,           IDB_EXTTRACKSH,         TButtonGadget::Command );
ControlBarGadgets[ EEGGLVIEW_CBG_COMPTH             ]= new TButtonGadgetDpi ( IDB_COMPTRACKSH,          IDB_COMPTRACKSH,        TButtonGadget::Command );

ControlBarGadgets[ EEGGLVIEW_CBG_SEP9               ]= new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ EEGGLVIEW_CBG_VUNITS             ]= new TButtonGadgetDpi ( IDB_VERTUNITS,            IDB_VERTUNITS,          TButtonGadget::NonExclusive);
ControlBarGadgets[ EEGGLVIEW_CBG_HUNITS             ]= new TButtonGadgetDpi ( IDB_HORIZUNITS,           IDB_HORIZUNITS,         TButtonGadget::NonExclusive);
                                                                                  
ControlBarGadgets[ EEGGLVIEW_CBG_SEP10              ]= new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ EEGGLVIEW_CBG_FILTER             ]= new TButtonGadgetDpi ( IDB_FILTER,               IDB_FILTER,             TButtonGadget::Command, true, EEGDoc->AreFiltersActivated () ? TButtonGadget::Down : TButtonGadget::Up );
ControlBarGadgets[ EEGGLVIEW_CBG_SETAVGREF          ]= new TButtonGadgetDpi ( IDB_SETAVGREF,            IDB_SETAVGREF,          TButtonGadget::Command, true, EEGDoc->GetReferenceType () == ReferenceAverage ? TButtonGadget::Down : TButtonGadget::Up );
                                                                                  
ControlBarGadgets[ EEGGLVIEW_CBG_SEP11              ]= new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ EEGGLVIEW_CBG_DECBR              ]= new TButtonGadgetDpi ( IDB_ISDECBRIGHT,          IDB_ISDECBRIGHT,        TButtonGadget::Command);
ControlBarGadgets[ EEGGLVIEW_CBG_INCBR              ]= new TButtonGadgetDpi ( IDB_ISINCBRIGHT,          IDB_ISINCBRIGHT,        TButtonGadget::Command);
ControlBarGadgets[ EEGGLVIEW_CBG_DECCR              ]= new TButtonGadgetDpi ( IDB_ISDECCONTRAST,        IDB_ISDECCONTRAST,      TButtonGadget::Command);
ControlBarGadgets[ EEGGLVIEW_CBG_INCCR              ]= new TButtonGadgetDpi ( IDB_ISINCCONTRAST,        IDB_ISINCCONTRAST,      TButtonGadget::Command);

ControlBarGadgets[ EEGGLVIEW_CBG_SEP12              ]= new TSeparatorGadget ( DefaultSeparator );
ControlBarGadgets[ EEGGLVIEW_CBG_FXDSCL             ]= new TButtonGadgetDpi ( IDB_FIXEDSCALE,           IDB_FIXEDSCALE,         TButtonGadget::NonExclusive, true, ScalingAuto ? TButtonGadget::Down : TButtonGadget::Up );
ControlBarGadgets[ EEGGLVIEW_CBG_COLOR              ]= new TButtonGadgetDpi ( IDB_SPCOLOR,              IDB_SPCOLOR,            TButtonGadget::Command);
}


//----------------------------------------------------------------------------
        TTracksView::~TTracksView ()
{
if ( Rois ) {
    delete  Rois;
    Rois = 0;
    }


if ( Filling ) {
//  if ( ! CartoolApplication->Closing )
        for ( int e = 0; e < NumFilling; e++ )
            if ( Filling[ e ] )
                delete Filling[ e ];

    delete[] Filling;
    Filling     = 0;

    IsFilling   = FillingNone;
    NumFilling  = 0;
    HasFilling  = false;
    }


if ( SelFilling ) {
    delete  SelFilling;
    SelFilling = 0;
    }


if ( ScrollBar ) {
    delete  ScrollBar;
    ScrollBar = 0;
    }


if ( DrawTFCursorDC ) {
    delete  DrawTFCursorDC;
    DrawTFCursorDC = 0;
    }


if ( MouseDC ) {
    CaptureMouse ( Release );

    delete  MouseDC;
    MouseDC     = 0;

    CaptureMode = CaptureNone;
    }
}


//----------------------------------------------------------------------------
void    TTracksView::SetupWindow ()
{
TBaseView::SetupWindow ();

                                        // !This will modify the current selection SelTracks!
if ( ! (   EEGDoc->IsContentType ( ContentTypeSeg       )
        || EEGDoc->IsContentType ( ContentTypeData      )
        || EEGDoc->IsContentType ( ContentTypeErrorData ) ) )

    CmNextRois ();


DragAcceptFiles ( true );

                                        // PaintRect needs to be updated for the color scaling to have some room - Regular windows sometime fail to update upon creation, so force call
//if ( IsIntensityModes () )
    GetClientRect ( PaintRect );

                                        // This needs an active window
VnReloadData ( EV_VN_RELOADDATA_DOCPOINTERS );  // update XYZDoc / SPDoc / Rois

SetOrient ( XYZDoc );

//AnimCursor  = TTimer ( GetHandle (), TimerCursor, 500, INT_MAX );
//AnimCursor.Set ();

                                        // Window sometime "forgets" to draw itself if the opening time seems too long
Invalidate ( false );
}


bool    TTracksView::CanClose ()
{
return  TBaseView::CanClose();
}


//----------------------------------------------------------------------------
void    TTracksView::EvSetFocus ( HWND hwnd )
{
                                        // in case of doc closing with multiple views, prevent all the other views from inheriting the focus, too
if ( ! EEGDoc->IsOpen () )
    return;


TBaseView::EvSetFocus ( hwnd );


if ( GODoc )    GODoc->LastEegViewId    = GetViewId();

EEGDoc->NotifyFriendViews ( LinkedViewId, vnNewTFCursor, (TParam2) &TFCursor, this );
}


void    TTracksView::EvKillFocus ( HWND hwnd )
{
TFCursor.StopExtending ();

TBaseView::EvKillFocus ( hwnd );
}


//----------------------------------------------------------------------------
                                        // set the margin for text, accounting for
                                        //   - plain names vs montage names
                                        //   - EEG or XYZ names
                                        //   - ROI names
void    TTracksView::SetTextMargin ()
{
TextMargin       = 0;


if ( IsRoiMode () && Rois ) {

    for ( int s = 0; s < Rois->NumSet (); s++ )
                                        // !SetPartWindows should have been called before!
        Maxed ( TextMargin, (double) SFont->GetStringWidth ( WindowSlots[ s ].Name ) );
    }

else { // tracks

    for ( int i = 0; i < EEGDoc->GetTotalElectrodes (); i++ )

        Maxed ( TextMargin, (double) SFont->GetStringWidth ( GetElectrodeName ( i ) ) );
    }

                                        // adding a little extra
TextMargin     += 1.5 * SFont->GetAvgWidth ();

                                        // clip within quite liberal limits, still clipping
Clipped ( TextMargin, (double) TextMarginMin, (double) TextMarginMax );


//DBGV3 ( (IsRoiMode () && Rois), (bool) XYZDoc, TextMargin, "ROIs? XYZ? TextMargin" );
}


//----------------------------------------------------------------------------
                                        // Format duration from TF to text, and optionally to a time string, too
void    TTracksView::TFToString (   long        tf, 
                                    char*       stf, 
                                    char*       stime,  TimeDisplayEnum     horizscale, 
                                    char*       sdate,
                                    bool        interval 
                                )   const
{
if ( ! interval /*&& ( EEGDoc->IsTemplates () || EEGDoc->IsContentType ( ContentTypeData ) )*/ )
    tf      = EEGDoc->RelToAbsTime ( tf );  // add first "TF" value


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // format time frames - also OK for frequency display
TimeFrameToString ( stf, tf );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // optionally format the time
if ( stime ) {

    ClearString ( stime );

    if ( EEGDoc->GetSamplingFrequency() != 0 ) {

        double      timeus      = TimeFrameToMicroseconds ( tf, EEGDoc->GetSamplingFrequency () );

        if ( EEGDoc->GetDim2Type () == DimensionTypeFrequency )

            EEGDoc->DateTime.GetStringFrequency ( timeus, stime, interval );

        else {

            if      ( horizscale == TimeDisplayTime )

                EEGDoc->DateTime.GetStringTime ( timeus, stime, interval );

            else if ( horizscale == TimeDisplayLinear )

                EEGDoc->DateTime.GetStringData ( timeus, stime, interval );

            } // time axis
        } // sampling frequency
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // optionally format the date
if ( sdate ) {

    if ( interval || EEGDoc->GetDim2Type () == DimensionTypeFrequency )

        ClearString ( sdate );
    else
        EEGDoc->DateTime.GetStringDate ( sdate );
    }
}


//----------------------------------------------------------------------------
                                        // double version, for non-integer TFs steps used for horizontal grids
void    TTracksView::TFToString (   double      dtf, 
                                    char*       stf, 
                                    char*       stime,  TimeDisplayEnum     horizscale 
                                )   const
{
                                        // doing the time frame part (stf)
TFToString ( (long) dtf, stf );


if ( stime ) {

    ClearString ( stime );

    if ( EEGDoc->GetSamplingFrequency() != 0 ) {

        double      timeus      = TimeFrameToMicroseconds ( dtf, EEGDoc->GetSamplingFrequency () );
                                        // keep into floating point
        if ( EEGDoc->GetDim2Type () == DimensionTypeFrequency )

            EEGDoc->DateTime.GetStringFrequency ( timeus, stime, false );

        else {

            if      ( horizscale == TimeDisplayTime )

                EEGDoc->DateTime.GetStringTime ( timeus, stime, false );

            else if ( horizscale == TimeDisplayLinear )

                EEGDoc->DateTime.GetStringData ( timeus, stime, false );

            } // time axis
        } // sampling frequency
    }
}


//----------------------------------------------------------------------------
                                        // cooks an informative title from given cursor
void    TTracksView::CursorToTitle ( TTFCursor *tfc, char *title )  const
{
long                tfmin;
long                tfmax;
long                tfdelta;
char                stfmin    [ 32 ];
char                stfmax    [ 32 ];
char                stfdelta  [ 32 ];
char                stimemin  [ 32 ];
char                stimemax  [ 32 ];
char                stimedelta[ 32 ];
char                sdate     [ 32 ];

                                        // "Title   "
StringCopy ( title, (char *) EEGDoc->GetTitle (), "   " );

                                        // get the specific name of the X dimension
                                        // "Title   X="
if ( EEGDoc->GetDim2Type () )
    StringAppend ( title, EEGDoc->GetDim2TypeName (), "=" );


                                        // get position
tfc->GetPos ( tfmin, tfmax );

tfdelta     = tfmax - tfmin + 1;

ClearString ( sdate    );
ClearString ( stimemin );


if ( tfc->IsSplitted () ) {

    TFToString ( tfmin,   stfmin,   ShowTime ? stimemin   : 0, ShowHorizScaleLegal, ShowDate ? sdate : 0 );
    TFToString ( tfmax,   stfmax,   ShowTime ? stimemax   : 0, ShowHorizScaleLegal, 0 );
    TFToString ( tfdelta, stfdelta, ShowTime ? stimedelta : 0, ShowHorizScaleLegal, 0, true );

                                        // "Title   X=tf..tf (dtf) / ms..ms (dms)  date"
    if      ( *stimemin ) { StringAppend ( title, stfmin, "..", stfmax, " (", stfdelta, ") / " );
                            StringAppend ( title, stimemin, "..", stimemax, " (", stimedelta, ")  ", sdate );
                            }
                                        // "Title   X=tf..tf (dtf)  date"
    else if ( *sdate   )    StringAppend ( title, stfmin, "..", stfmax, " (", stfdelta, ")  ", sdate );
                                        // "Title   X=tf..tf (dtf)"
    else                    StringAppend ( title, stfmin, "..", stfmax, " (", stfdelta, ")" );

    }
else {

    TFToString ( tfmin,   stfmin,   ShowTime ? stimemin   : 0, ShowHorizScaleLegal, ShowDate ? sdate : 0 );

                                        // "Title   X=tf / ms  date"
    if      ( *stimemin )   StringAppend ( title, stfmin, " / ", stimemin, "  ", sdate );
                                        // "Title   X=tf  date"
    else if ( *sdate   )    StringAppend ( title, stfmin, "  ", sdate );
                                        // "Title   X=tf"
    else                    StringAppend ( title, stfmin );
    }

}


//----------------------------------------------------------------------------
void    TTracksView::UpdateCaptionTracksValues ( char *buff )
{
                                        // add min/max smart informations on current display ONLY
                                        // how many displayed + highlighted ?
int             el;
int             nhr     = EEGDoc->GetNumSelectedRegular ( Highlighted );
int             nhp     = EEGDoc->GetNumSelectedPseudo  ( Highlighted );
int             nsr     = EEGDoc->GetNumSelectedRegular ( SelTracks   );
int             nsp     = EEGDoc->GetNumSelectedPseudo  ( SelTracks   );

int             tfi;
int             tf1,    tf2;
TSelection     *tosel;
double          minv;
double          maxv;

ClearString ( buff );


if ( ( !nhr && nhp ) || ( !nsr && nsp ) ) { // only pseudos, highlighted or selected
                                        // select highlighted or display
    tosel = nhp ? &Highlighted : &SelTracks;

    if ( TFCursor.IsSplitted() ) {      // scan all TFs

        tf1     = TFCursor.GetPosMin() - CDPt.GetMin();
        tf2     = TFCursor.GetPosMax() - CDPt.GetMin();

        Clipped ( tf1, 0, EegBuff.GetDim2 () - 1 );
        Clipped ( tf2, 0, EegBuff.GetDim2 () - 1 );

                                        // process each pseudo electrode separately
        ClearString ( buff );

        for ( TIteratorSelectedForward eli ( *tosel, EEGDoc->GetFirstPseudoIndex (), EEGDoc->GetLastPseudoIndex () ); (bool) eli; ++eli ) {

            minv    =  DBL_MAX;
            maxv    = -DBL_MAX; 

            for ( tfi = tf1; tfi <= tf2; tfi++ ) {
                if ( EegBuff[ eli() ][ tfi ] < minv )  minv = EegBuff[ eli() ][ tfi ];
                if ( EegBuff[ eli() ][ tfi ] > maxv )  maxv = EegBuff[ eli() ][ tfi ];
                }

            if ( *buff )
                StringAppend ( buff, "  " );

            sprintf ( StringEnd ( buff ), "%s=%lg to %lg", GetElectrodeName ( eli() ), minv, maxv );
            }
        }

    else { // not splitted
        ClearString ( buff );

        tfi     = Clip ( (int) ( TFCursor.GetPosMin() - CDPt.GetMin() ), 0, EegBuff.GetDim2 () - 1 );

        for ( TIteratorSelectedForward eli ( *tosel, EEGDoc->GetFirstPseudoIndex (), EEGDoc->GetLastPseudoIndex () ); (bool) eli; ++eli ) {

            if ( StringIsNotEmpty ( buff ) )
                StringAppend ( buff, "  " );

            sprintf ( StringEnd ( buff ), "%s=%lg", GetElectrodeName ( eli() ), EegBuff[ eli() ][ tfi ] );
            }
        }
    } // if pseudos

else if ( nhr || nsr ) {                // some regular tracks, ignoring pseudos if any
                                        // select highlighted or display
    tosel = nhr ? &Highlighted : &SelTracks;

    minv    =  DBL_MAX;
    maxv    = -DBL_MAX;

    if ( TFCursor.IsSplitted() ) {      // scan all TFs

        tf1     = TFCursor.GetPosMin() - CDPt.GetMin();
        tf2     = TFCursor.GetPosMax() - CDPt.GetMin();

        Clipped ( tf1, 0, EegBuff.GetDim2 () - 1 );
        Clipped ( tf2, 0, EegBuff.GetDim2 () - 1 );

                                        // process all electrodes
        for ( TIteratorSelectedForward eli ( *tosel, EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () ); (bool) eli; ++eli ) {

            for ( tfi = tf1; tfi <= tf2; tfi++ ) {
                if ( EegBuff[ eli() ][ tfi ] < minv )  minv = EegBuff[ eli() ][ tfi ];
                if ( EegBuff[ eli() ][ tfi ] > maxv )  maxv = EegBuff[ eli() ][ tfi ];
                }
            }
        }
    else { // not splitted

        tfi     = TFCursor.GetPosMin() - CDPt.GetMin();

        Clipped ( tfi, 0, EegBuff.GetDim2 () - 1 );

                                        // search for min and max
        for ( TIteratorSelectedForward eli ( *tosel, EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () ); (bool) eli; ++eli ) {

            if ( EegBuff[ eli() ][ tfi ] < minv )  minv = EegBuff[ eli() ][ tfi ];
            if ( EegBuff[ eli() ][ tfi ] > maxv )  maxv = EegBuff[ eli() ][ tfi ];
            }
        }

    if ( minv == maxv && !TFCursor.IsSplitted() )
        sprintf ( buff, "%s=%lg", GetElectrodeName ( tosel->FirstSelected () ), minv );
    else
        sprintf ( buff, "values=%lg to %lg", minv, maxv );
    } // else regular
}


//----------------------------------------------------------------------------
void    TTracksView::UpdateCaption ()
{
char                buff[ MaxViewTitleLength ];

                                        // Doc + Cursor
CursorToTitle ( &TFCursor, Title );


//if ( ! EEGDoc->IsTemplates () ) {
                                        // + Values (useless for templates)
    UpdateCaptionTracksValues ( buff );

    StringAppend ( Title, "  ", buff );
//  }

                                        // optionally display the current segments
if ( EEGDoc->IsContentType ( ContentTypeSeg ) 
  && SelFilling 
  && SelFilling->NumSet () > 0 && SelFilling->NumSet () <= SelFilling->MaxValue () - 1 ) {

    StringAppend ( Title, "  Segment", StringPlural ( SelFilling->NumSet () ) );

    for ( TIteratorSelectedForward segi ( *SelFilling ); (bool) segi; ++segi )
        StringAppend ( Title, " ", IntegerToString ( buff, segi() ) );
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


//----------------------------------------------------------------------------
                            // either highlights or draws to normal this track
void    TTracksView::DrawHighlighted ( int /*i*/, bool /*highlight*/ )
{
//for ( int pw=0; pw < numPartWindows; pw++ )
//    listPartWindows[pw]->DrawHighlighted ( i, highlight );
}


//----------------------------------------------------------------------------
void    TTracksView::SetPartWindows ( TSelection &sel, bool refresh )
{
                                        // subdivide with scrollbar's space removed
double              W               = GetClientRect().Width  () - ( HasColorScale () ? ColorScaleWidth : 0 );
double              H               = GetClientRect().Height () - EegScrollbarHeight;
double              X               = GetClientRect().Left   ();
double              Y               = GetClientRect().Top    () + EegScrollbarHeight;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numrois         = IsRoiMode () ? Rois->NumSet () : 0;
int                 numtracks       = IsRoiMode () ? numrois         : EEGDoc->GetNumSelectedRegular ( sel );
int                 numpstracks     = EEGDoc->GetNumSelectedPseudo ( sel );

                                        // do a local copy of required layout mode
int                 renderingmode   = RenderingMode;

                                        // do some checking, and downgrade rendering mode if needed
if ( renderingmode == LayoutFourPages && numtracks < 4 )    renderingmode = LayoutTwoPages;
if ( renderingmode == LayoutTwoPages  && numtracks < 2 )    renderingmode = LayoutOnePage;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get # of slots for regular
int                 numregslots     = renderingmode == LayoutFourPages ? 4
                                    : renderingmode == LayoutTwoPages  ? 2
                                    : renderingmode == LayoutOnePage   ? 1
                                    :                                    numtracks;
if ( IsRoiMode () )                     // override in roi mode
    numregslots = numrois;

if ( numtracks == 0 )                   // reset
    numregslots = 0;
                                        // only pseudos?
if ( numregslots == 0 && ( renderingmode == LayoutFourPages || renderingmode == LayoutTwoPages ) )
    renderingmode = LayoutOnePage;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get # of slots for pseudos
int                 numpseudoslots  = renderingmode == LayoutFourPages ? 4
                                    : renderingmode == LayoutTwoPages  ? 2
                                    : renderingmode == LayoutOnePage   ? 1
                                    :                                    numpstracks;
if ( numpstracks == 0 )
    numpseudoslots = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // no slots?
if ( numregslots + numpseudoslots == 0 ) {
    DeallocateWindowSlots ();
    goto FinishedSetPartWindows;
    }

                                        // adjust to the needed number of slots
AllocateWindowSlots ( numregslots + numpseudoslots );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // initialize contents
MaxRegularTracksPerSlot = 1;
MaxPseudoTracksPerSlot  = 1;

int                 s;

                                        // content for regular slots
if ( IsRoiMode () && Rois ) {

    for ( TIteratorSelectedForward roii ( *Rois->GetRoisSelected () ); (bool) roii && roii.GetIndex () < numrois; ++roii ) {
        
        int     s   = roii.GetIndex ();

        WindowSlots[ s ].SelTracks      = (*Rois)[ roii() ].Selection;

        WindowSlots[ s ].RegularTracks  = true;

        WindowSlots[ s ].Color          = (*Rois)[ roii() ].Color;

        StringCopy ( WindowSlots[ s ].Name, Rois->GetRoiName ( roii() ), WindowSlotNameLength - 1 );

        MaxRegularTracksPerSlot         = max ( MaxRegularTracksPerSlot, (int) WindowSlots[ s ].SelTracks );
        }

    }

else { // tracks

    for ( s = 0; s < numregslots; s++ ) {

        if ( renderingmode == LayoutOneTrackOneBox
          || renderingmode == LayoutOneTrackOneBoxXyzFlat
          || renderingmode == LayoutOneTrackOneBoxXyz3D   )
            WindowSlots[ s ].SelTracks.SetOnly ( sel.GetValue ( s ) );
        else {
            WindowSlots[ s ].SelTracks = sel;
            EEGDoc->ClearPseudo  ( WindowSlots[ s ].SelTracks );

                                        // clear trailing
            if ( numregslots > 1 && s < numregslots - 1 )
                WindowSlots[ s ].SelTracks.Reset ( sel.GetValue ( (double) ( s + 1 ) / numregslots * numtracks ), sel.GetValue ( numtracks - 1 ) );

                                        // clear beginning
            if ( numregslots > 1 && s > 0 )
                WindowSlots[ s ].SelTracks.Reset ( sel.GetValue ( 0 ), sel.GetValue ( (double) s / numregslots * numtracks - 1 ) );
            }


        WindowSlots[ s ].RegularTracks  = true;
        WindowSlots[ s ].Color.Set ( TrackColor );
        ClearString ( WindowSlots[ s ].Name );

        MaxRegularTracksPerSlot         = max ( MaxRegularTracksPerSlot, (int) WindowSlots[ s ].SelTracks );
        }
    }

                                        // content for pseudo slots
for ( s = numregslots; s < numregslots + numpseudoslots; s++ ) {

    if ( renderingmode == LayoutOneTrackOneBox
      || renderingmode == LayoutOneTrackOneBoxXyzFlat
      || renderingmode == LayoutOneTrackOneBoxXyz3D   ) {

        WindowSlots[ s ].SelTracks.SetOnly ( sel.GetValue ( IsRoiMode () ? EEGDoc->GetNumSelectedRegular ( sel ) + s - numregslots : s ) );
//      DBGV2 ( s, WindowSlots[ s ].SelTracks.GetValue ( 0 ), "pseudo slot   pseudo set" );
        }
    else {
        WindowSlots[ s ].SelTracks = sel;
        EEGDoc->ClearRegular ( WindowSlots[ s ].SelTracks );
        }


    WindowSlots[ s ].RegularTracks  = false;
    WindowSlots[ s ].Color.Set ( PseudotrackColor );
    ClearString ( WindowSlots[ s ].Name );

    MaxPseudoTracksPerSlot          = max ( MaxPseudoTracksPerSlot, (int) WindowSlots[ s ].SelTracks );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // initialize positions
double              llx             = 0;
double              lly             = 0;
double              llz             = -0.5;     // fixed depth
double              torx            = 0;
double              tory            = 0;
double              torz            = 0;
double              toux            = 0;
double              touy            = 0;
double              touz            = 0;


                                        // 1 track in 1 box
if ( renderingmode == LayoutOneTrackOneBox
  || renderingmode == LayoutOneTrackOneBoxXyzFlat
  || renderingmode == LayoutOneTrackOneBoxXyz3D   ) {


    int             numslots        = numregslots + numpseudoslots;


    if ( renderingmode == LayoutOneTrackOneBox ) {

        int         byx;
        int         byy;
        FitItemsInWindow ( numslots, TSize ( 100, 50 ), byx, byy );

        for ( s = 0; s < numslots; s++ ) {
            torx    = W / byx;
            touy    = H / byy;
            llx     = X + W *       (double)         ( s % byx )   / byx;
            lly     = Y + H * ( 1 - (double) ( (int) ( s / byx ) ) / byy ) - touy;

            WindowSlots[ s ].SetFrame ( llx, lly, llz, torx, tory, torz, toux, touy, touz );
            }
        } // if LayoutOneTrackOneBox


    else if ( renderingmode == LayoutOneTrackOneBoxXyzFlat
           || renderingmode == LayoutOneTrackOneBoxXyz3D   ) {
                                        // compute a rectangle perpendicular to electrode (relative to center)
                                        // and vertically aligned

        const TBoundingBox<double>*     b   = XYZDoc->GetBounding ( CurrentDisplaySpace );
        double              w;
        double              h;

        TVector3Double      center;
        TVector3Double      el;
        TVector3Double      lowleft;
        TVector3Double      el1;
        TVector3Double      el2;
        TVector3Double      toel;
        TVector3Double      toup;
        TVector3Double      toupproj;
        TVector3Double      toright;
        double              projlength;
        double              numaftermin     = EEGDoc->GetNumPseudoElectrodes ();
        double              regrescale;


        if ( IsRoiMode () && Rois->GetNumRois () ) {
                                        // rescale rois with the mean number of tracks in rois
            regrescale  = 0;

            for ( int r = 0; r < Rois->GetNumRois (); r++ )
                regrescale  += (*Rois)[ r ].Selection.MaxSet ();

            regrescale  = sqrt ( regrescale / Rois->GetNumRois () );
            }
        else
            regrescale  = 1;


        if ( CurrentDisplaySpace != DisplaySpace3D )
            BestXYZBox ( w, h );        // all the job is done here
        else {
            w       = b->Radius() / 6;
            h       = w / 2;

            if ( IsRoiMode () ) {
                w   *= regrescale;
                h   *= regrescale;
                }
            }

                                        // arbitrary, should ask the XYZDoc
        toup[ 0 ] =  0;
        toup[ 1 ] =  0;
        toup[ 2 ] =  1;
                                        // get center
        center  = b->GetCenter ();

        int     psindex = 0;
        int     pw      = 0;

        for ( TIteratorSelectedForward si ( sel ); (bool) si && pw < numslots; ++si, ++pw ) {

            int     s       = si();

                                        // get electrode position
            if ( (bool) Montage && (bool) Montage[ s ] ) {

                XYZDoc->GetCoordinates ( Montage[ s ].El1, el1, CurrentDisplaySpace != DisplaySpace3D );
                XYZDoc->GetCoordinates ( Montage[ s ].El2, el2, CurrentDisplaySpace != DisplaySpace3D );
                el = ( el1 + el2 ) * 0.5;
                }

            else if ( IsRoiMode () && pw < numregslots ) {

                el.Reset ();
                                        // scan all tracks in this roi, and compute barycenter of all electrodes
                for ( TIteratorSelectedForward roii ( WindowSlots[ pw ].SelTracks ); (bool) roii; ++roii ) {
                    XYZDoc->GetCoordinates ( roii(), el1, CurrentDisplaySpace != DisplaySpace3D );
                    el += el1;
                    }

                el /=  (int) WindowSlots[ pw ].SelTracks;

                                        // the average will drift toward center, so kick it out of it!
                if ( CurrentDisplaySpace == DisplaySpace3D ) {
                    toel = el - center;
                    toel.Normalize ();
                    el  = center + toel * b->Radius();
                    }
                }

            else if ( ! IsRoiMode () && s <= EEGDoc->GetLastRegularIndex () )

                XYZDoc->GetCoordinates ( s, el, CurrentDisplaySpace != DisplaySpace3D );

            else { // pseudo

                toright [ 0 ] =  0;
                toright [ 1 ] = -1;
                toright [ 2 ] =  0;

                toupproj[ 0 ] = -1;
                toupproj[ 1 ] =  0;
                toupproj[ 2 ] =  0;

                if ( CurrentDisplaySpace == DisplaySpace3D )
//                    el = center - ( toup * b->Radius() * 1.10 )
//                       + toright * ( ( (double) psindex - numaftermin / 2 + 0.5  )
//                                     * w * 1.5 );
                    el = center - ( toup * b->Radius() * 1.05 )
                       + toright * ( ( (double) psindex - ( numaftermin - 1 ) / 2.0 )
                                     * w * 1.5 / regrescale );
                else
//                    el = center - ( toupproj * ( b->XMin() - 2.0 * h ) )
//                       + toright * ( ( (double) psindex - numaftermin / 2 + 0.5  )
//                                     * w * 1.5 );
                    el = center - ( toupproj * ( b->XMin() - 2.0 * h / regrescale ) )
                       + toright * ( ( (double) psindex - ( numaftermin - 1 ) / 2.0 )
                                     * w * 1.5 / regrescale );

                psindex++;
                }

                                        // shift origin either "up", or radially
            if ( CurrentDisplaySpace != DisplaySpace3D ) {

                el = el + toup * ( b->Radius() / 15 );

                toright [ 0 ] =  0;
                toright [ 1 ] = -1;
                toright [ 2 ] =  0;

                toupproj[ 0 ] = -1;
                toupproj[ 1 ] =  0;
                toupproj[ 2 ] =  0;

                lowleft =  el;
                }
            else {
                                        // get vector orthogonal to electrode
                toel = el - center;
                toel.Normalize ();
                                        // get an up vector that belongs to orthogonal plane to electrode
                projlength = toup.ScalarProduct ( toel );

                if ( fabs ( projlength ) >= 1 ) {
                    toupproj[ 0 ] = -1;
                    toupproj[ 1 ] =  0;
                    toupproj[ 2 ] =  0;
                    }
                else {
                    toupproj = toup - ( toel * projlength );
                    toupproj.Normalize ();
                    }
                                        // the right is obtained from cross product
                toright = toupproj.VectorialProduct ( toel );
                toright.Normalize ();

                lowleft =  el + ( el - center ) * TracksAltitude3D;
                }

                                        // put them at scale
            toright  *= w;
            toupproj *= h;

                                        // revert the scaling for pseudos in rois
            if ( pw >= numregslots && IsRoiMode () ) {
                toright  /= regrescale;
                toupproj /= regrescale;
                }

                                        // we have all coordinates
            WindowSlots[ pw ].SetFrame ( lowleft [ 0 ], lowleft [ 1 ], lowleft [ 2 ],
                                         toright [ 0 ], toright [ 1 ], toright [ 2 ],
                                         toupproj[ 0 ], toupproj[ 1 ], toupproj[ 2 ] );
            }
        } // LayoutOneTrackOneBoxXyzFlat || LayoutOneTrackOneBoxXyz3D

    } // if 1 track in 1 box


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // layout "à la page"
else if ( renderingmode == LayoutFourPages
       || renderingmode == LayoutTwoPages
       || renderingmode == LayoutOnePage   ) {

                                        // heights for regulars and pseudos
    double      hreg        =       numpseudoslots ? RegularTracksVerticalRatio : 1;
    double      hpseudos    = 1 - ( numregslots    ? RegularTracksVerticalRatio : 0 );


    int         numcolumns  = renderingmode == LayoutFourPages ? 4 :
                              renderingmode == LayoutTwoPages  ? 2 :
                              renderingmode == LayoutOnePage   ? 1 :
                                                                 1;

    if ( numtracks ) {

        if ( IsRoiMode () ) {
            int     numroispercolumn    = WindowSlots[ s ].NumRoisPerColumn ( numrois, numcolumns );

            for ( s = 0; s < numregslots; s++ ) {
                llx     = WindowSlots[ s ].RoiLowLeftX ( X, W, s, numroispercolumn, numcolumns      );
                lly     = WindowSlots[ s ].RoiLowLeftY ( Y, H, s, numroispercolumn,            hreg );
                torx    = WindowSlots[ s ].RegToRightX (    W,                      numcolumns      );
                touy    = WindowSlots[ s ].RegToUpY    (    H,    numroispercolumn,            hreg );

                WindowSlots[ s ].SetFrame ( llx, lly, llz, torx, tory, torz, toux, touy, touz );
                }
            }
        else {
            for ( s = 0; s < numregslots; s++ ) {
                llx     = WindowSlots[ s ].RegLowLeftX ( X, W, s, numregslots       );
                lly     = WindowSlots[ s ].RegLowLeftY ( Y, H,                hreg  );
                torx    = WindowSlots[ s ].RegToRightX (    W,    numregslots       );
                touy    = WindowSlots[ s ].RegToUpY    (    H, 1,             hreg  );

                WindowSlots[ s ].SetFrame ( llx, lly, llz, torx, tory, torz, toux, touy, touz );
                }
            }
        }


    if ( numpstracks ) {
        for ( s = 0; s < numpseudoslots; s++ ) {
            llx     = WindowSlots[ s ].PseudosLowLeftX ( X, W, s, numpseudoslots            );
            lly     = WindowSlots[ s ].PseudosLowLeftY ( Y                                  );
            torx    = WindowSlots[ s ].PseudosToRightX (    W,    numpseudoslots            );
            touy    = WindowSlots[ s ].PseudosToUpY    (    H,                   hpseudos   );

            WindowSlots[ numregslots + s ].SetFrame ( llx, lly, llz, torx, tory, torz, toux, touy, touz );
            }
        }
    } // else layout "a la page"


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                                                // when everything has been computed, set only one track per ROI
if ( IsRoiMode () && Rois && AverageRois )

    for ( TIteratorSelectedForward roii ( *Rois->GetRoisSelected () ); (bool) roii && roii.GetIndex () < numrois; ++roii )

        WindowSlots[ roii.GetIndex () ].SelTracks.SetOnly ( (*Rois)[ roii() ].Selection.FirstSelected () );


FinishedSetPartWindows:

if ( refresh ) {
    ShowNow();
//  UpdateCaption ();

    RefreshLinkedWindow () ;
    }
}

/*
void    TTracksView::ZoomWindow ( TEegWinViewPartTracks *tocw )
{
tocw->GetSelectedTracks ( SelTracks );  // specifies which local track to display
SetPartWindows ( SelTracks );
}
*/

void    TTracksView::RefreshLinkedWindow ( bool force )
{
                                        // right now, it concerns only the 3D view, linked by the XE window
if ( CurrentDisplaySpace == DisplaySpaceNone && !force )
    return;

EEGDoc->NotifyDocViews ( vnViewUpdated, (TParam2) this, this );
}


//----------------------------------------------------------------------------
                                        // computes the origin and stepping for the horizontal grid
                                        // works in ms if possible, in TFs otherwise
                                        // returns double values to allow a nice grid even with sampling frequencies odd with 1000
void    TTracksView::SetupHorizontalGrid ( double& hgridorg, double& hgridstep )
{
if ( EEGDoc->IsContentType ( ContentTypeErrorData ) ) {
                                        // error.data gets a specific formula
    hgridorg    = 0;

    hgridstep   = AtLeast ( 1, Round ( 2.5 * (   EEGDoc->GetNumTimeFrames () 
                                                * NumIntegerDigits ( EEGDoc->GetNumTimeFrames () )
                                                * SFont->GetAvgWidth () 
                                             ) 
                                                / WindowSlots[ 0 ].ToRight.Norm () 
                                     ) 
                          );

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Used to convert TF to ms
double              tr              = EEGDoc->GetSamplingFrequency () ? TimeFrameToMilliseconds ( 1, EEGDoc->GetSamplingFrequency () ) : 1;

                                        // horizontal space, normalized from the number of characters to number of TFs
                                        // cumulate length for each possible time display
hgridstep   = (                                                             5
                + (      EEGDoc->GetSamplingFrequency ()
                    && ! EEGDoc->IsContentType ( ContentTypeHistogram )   ? 7 : 0 )
                + (      EEGDoc->DateTime.MicrosecondPrecision            ? 5 : 0 )
                + (      EEGDoc->DateTime.RelativeTime                    ? 5 : 9 )     // relative time is quite compact, while full time is not
              )
            * 1.5
            * SFont->GetAvgWidth ()
            * CDPt.GetLength () / WindowSlots[ 0 ].ToRight.Norm ()
            * tr
            + 1;

                                        // powers of 10, with smart subdivision to have a 5 10 50 100 500 1000... series
double              lr              = Log10 ( hgridstep ) + 0.6;

hgridstep   = Power10 ( Truncate ( lr ) ) / ( Fraction ( lr ) > 0.5 ? 1 : 2 );
                                        // but avoid step less than..
//if ( hgridstep < 5 )   hgridstep = 5;
                                        // then retransfrom in TF count
hgridstep  /= tr;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute origin

                                        // first TF to start the grid with
hgridorg    = TruncateTo ( CDPt.GetMin() + hgridstep - 1e-6, hgridstep );


                                        // account for an absolute residual offset in ms
if      ( ! EEGDoc->DateTime.RelativeTime && EEGDoc->DateTime.GetMillisecond () ) {
                                        // shift origin, converted in TF space
    hgridorg       -= EEGDoc->DateTime.GetMillisecond () / tr;

    hgridorg       += TruncateTo ( CDPt.GetMin() - hgridorg - 0.5, hgridstep );

    if ( hgridorg < CDPt.GetMin() /*- ( OneMoreHoriz () ? 0.5 : 0 )*/ )    // ?not sure about the OneMoreHoriz () subtraction?
        hgridorg   += TruncateTo ( CDPt.GetMin() - hgridorg, hgridstep ) + hgridstep;
    }
                                        // arbitrary relative origin
else if (   EEGDoc->DateTime.RelativeTime && EEGDoc->DateTime.RelativeTimeOffset ) {
                                        // shift origin, converted in TF space
    hgridorg       -= EEGDoc->DateTime.RelativeTimeOffset / 1000.0 / tr;

    hgridorg       += TruncateTo ( CDPt.GetMin() - hgridorg - 0.5, hgridstep );

    if ( hgridorg < CDPt.GetMin() /*- ( OneMoreHoriz () ? 0.5 : 0 )*/ )    // ?not sure about the OneMoreHoriz () subtraction?
        hgridorg   += TruncateTo ( CDPt.GetMin() - hgridorg, hgridstep ) + hgridstep;
    }
}


//----------------------------------------------------------------------------
void    TTracksView::GLPaint ( int how, int renderingmode, TGLClipPlane *otherclipplane )
{
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

static  bool        NestedPaint     = false;    // avoid re-entrant calls (could be put in some GLPAINT_... if generalized)
static  TListIterator<TBaseView>    iteratorview;


MatModelView.CopyModelView ();          // copy current transformation


if ( CurrentDisplaySpace == DisplaySpace3D && ( how & GLPaintLinked ) )
    GLFogOn         ();                 // previous fog (called from outside)
                                        // pb is called from XE, fog color is black, which does not render very well in 3D


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// The opaque stuff should be drawn first
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#define     MarginVariables \
bool                enoughw         = HasWindowSlots () && CurrentDisplaySpace == DisplaySpaceNone && ( WindowSlots[0].ToRight.Norm() >= TextMargin + 100 /*80*/ );\
bool                drawmargin      = enoughw;\
double              margin          = drawmargin ? TextMargin : 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the tracks
                                        // opaque, though using transparent mode to have some blending antialiasing
if ( HasWindowSlots () && ( how & GLPaintOpaque ) ) {

    GLLinesModeOn       ( false );
    GLLineSmoothOff     ();             // we are ready for smooth lines, but we wait until past the scaling grids


    //bool                        drawmargin  = enoughh && enoughw;

    MarginVariables;

                                                                                 // averaging ends up with 1 track
    bool                        avgrois         = IsRoiMode () && Rois && AverageRois;
    double                      maxhpertrack    = WindowSlots[ 0 ].ToUp.Norm () / ( avgrois ? 1 : max ( MaxRegularTracksPerSlot, MaxPseudoTracksPerSlot ) );
    bool                        enoughh         = CurrentDisplaySpace == DisplaySpaceNone && HasWindowSlots () && maxhpertrack >= 2;
    double                      scalev          = CurrentDisplaySpace == DisplaySpaceNone ? ScalingLevel
                                                                                          : ScalingLevel * XYZDoc->GetBounding ( CurrentDisplaySpace )->Radius() / 600;
    double                      scaleh;
    double                      descalex;
    double                      descaley;
    double                      scalet;
    TTracksViewWindowSlot*      toslot;
    int                         stepy           = 100;
    bool                        trackssuper;
    double                      contrast;
    double                      alpha;
    bool                        ispositive      = false; // EEGDoc->IsAbsolute ( AtomTypeUseCurrent );  // this makes the linking across files inconsistent, if files are not of the same types

    int                         t;
    bool                        regel;
    int                         p;
    int                         firstsel;
    int                         lastsel;
    int                         numTracks;
    double                      numTracksR;
//  int                         numPseudoTracks = SelTracks.NumSet ( EEGDoc->GetNumElectrodes(), EEGDoc->GetTotalElectrodes() - 1 );
    int                         minp            = 0;
    int                         maxp            = CDPt.GetLength() - 1;
    int                         nump            = maxp - minp + 1;
    float                      *tobuff;
    float                      *tobuff1;
    float                      *tobuff2;
    float                      *tosdbuff;
    TSelection                 *sel;
    TGLCoordinates<GLfloat>     trorg;
    TGLCoordinates<GLfloat>     trdelta;
    bool                        allnames;
    bool                        firsttrack;
    TMarker                     marker;
    double                      hgridstep;
    double                      hgridorg;
    GLfloat                     linewidth;

    bool                        draworigin      =  CurrentDisplaySpace == DisplaySpaceNone 
                                                && ShowBaseline 
                                              /*&& allnames*/ 
                                                && EEGDoc->GetSamplingFrequency () 
                                                && !( EEGDoc->DateTime.RelativeTime && EEGDoc->DateTime.RelativeTimeOffset == 0 );

    double                      orgreltf        = draworigin ? TFCursor.AbsoluteMicrosecondsToRelativeTF ( 0 ) - CDPt.GetMin() : 0;
                                draworigin     &= orgreltf >= 0 && orgreltf <= nump;

    double                      minValue;
    double                      maxValue;
    double                      v;
    GLfloat                     v1[ 3 ];
    GLfloat                     v2[ 3 ];
    int                         quality;

    bool                        roiname;
//  bool                        threshabove = EEGDoc->GetFilters ()->IsThresholdAbove ();
//  bool                        threshbelow = EEGDoc->GetFilters ()->IsThresholdBelow ();
    char                        buff[ 32 ];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Horizontal grid step set-up
    if ( ShowHorizScale && ( how & GLPaintOwner ) )

        SetupHorizontalGrid ( hgridorg, hgridstep );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute scaling factors
    if ( IsIntensityModes () ) {
                                        // find min max values in current page
//      GalMinValue = Highest ( GalMinValue );
//      GalMaxValue = Lowest  ( GalMaxValue );
        minValue    = Highest ( minValue );
        maxValue    = Lowest  ( maxValue );

                                        // scan only what is in the current display
                                        // Montage case is missing...
        for ( TIteratorSelectedForward sti ( SelTracks ); (bool) sti; ++sti ) {

            for ( t = minp; t <= maxp; t++ ) {

                v   =  EegBuff[ sti() ][ t ];
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

        if ( minValue == 0 )    minValue = -1e-12;
        if ( maxValue == 0 )    maxValue =  1e-12;

                                        // if we want the color table to show the window min max
        GalMinValue = minValue;
        GalMaxValue = maxValue;

                                        // contrast factor for the color computation
        contrast    = ScalingContrastToColorTable ( ScalingContrast );

        alpha       = (bool) Using ? 1.0 / ( 1 + (int) Using ) : 1.0;
        } // IsIntensityModes ()


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // an optional 2 passes for full 3D
  for ( int pass = CurrentDisplaySpace == DisplaySpace3D ? 0 : 1; pass < 2; pass++ )

    for ( int s = 0; s < GetNumWindowSlots (); s++ ) {

        glPushMatrix ();                // save current orientation & origin, before any local shift


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        toslot      = &WindowSlots[ s ];
        sel         = &toslot->SelTracks;

        firstsel    = sel->FirstSelected ();
        lastsel     = sel->LastSelected  ();

        numTracks   = toslot->RegularTracks ? MaxRegularTracksPerSlot : MaxPseudoTracksPerSlot; // to have the same spacing across slots of the same type
        numTracksR  = 1 / (double) ( numTracks + ( ispositive && ! IsIntensityModes () ? 0.5 : 1 ) );

        roiname     = *toslot->Name && IsRoiMode ();                            // choose to put the roi name or not

        allnames    = toslot->ToUp.Norm() * numTracksR > SFont->GetHeight();    // enough room for text?

        scaleh      = CurrentDisplaySpace == DisplaySpaceNone && ! OneMoreHoriz () ? (double) ( toslot->ToRight.Norm() - margin - 1 ) / ( nump > 1 ? nump - 1 : 1 )
                                                                                   : (double) ( toslot->ToRight.Norm() - margin     ) /   nump ;

                                        // smart superimpose: asked / not enough space / not pseudo tracks
        trackssuper =  ! IsIntensityModes () 
                    && ( ( TracksSuper && firstsel <= EEGDoc->GetLastRegularIndex() ) || ! enoughh );



        if ( CurrentDisplaySpace != DisplaySpaceNone ) {  // all shifts are already included

            trorg.X     = toslot->LowLeft.X;
            trorg.Y     = toslot->LowLeft.Y;
            trorg.Z     = toslot->LowLeft.Z;

            trdelta.X   = 0;
            trdelta.Y   = toslot->ToUp.Norm() * numTracksR * 0.90 * CursorHeightFactor3D;
            trdelta.Z   = 0;

                                        // get world coordinates
            GLApplyModelMatrix ( trorg, MatModelView );
                                        // add a screen-relative shift to recenter the track
            trorg.X    -= toslot->ToRight.Norm() / 2 + margin;
//          trorg.Y    -= toslot->ToUp.Y    / 2;
                                        // reset, keep only Z translation
            glLoadIdentity ();
                                        // redo the shift
//          glTranslated ( 0, 0, ModelRadius );     // may not be correct, if linked in != depth ?
//          glTranslated ( 0, 0, - MatModelView.GetTranslationZ() );   // use current transform Z shift

                                        // test for 2 Z halves, and draw according to current pass
            if ( CurrentDisplaySpace == DisplaySpace3D )

                if ( pass == 0 )    {   if ( trorg.Z <=  ModelRadius )     { glPopMatrix (); continue; }   }
                else                {   if ( trorg.Z >   ModelRadius )     { glPopMatrix (); continue; }   }
            }
                                        // averaging rois also behaves like superimposing
        else if ( trackssuper || avgrois ) {

            trorg.X     = toslot->LowLeft.X + margin;
            trorg.Y     = toslot->LowLeft.Y + 0.5 * toslot->ToUp.Y;
            trorg.Z     = toslot->LowLeft.Z;

            trdelta.X   =
            trdelta.Y   =
            trdelta.Z   = 0;

            if ( ispositive && ! IsIntensityModes () ) { // slightly shift downward
                trorg.Y     = toslot->LowLeft.Y + ( TracksFlipVert ? 0.75 : 0.25 ) * toslot->ToUp.Y;
                }

//          trorg.X     = toslot->LowLeft.X + drawmargin * 0.10 * toslot->ToRight.X + 0.5 * toslot->ToUp.X;
//          trorg.Y     = toslot->LowLeft.Y + drawmargin * 0.10 * toslot->ToRight.Y + 0.5 * toslot->ToUp.Y;
//          trorg.Z     = toslot->LowLeft.Z + drawmargin * 0.10 * toslot->ToRight.Z + 0.5 * toslot->ToUp.Z;
//          trdelta.X   = - toslot->ToUp.X * numTracksR;
//          trdelta.Y   = - toslot->ToUp.Y * SFont->GetHeight() / 2; // toslot->ToUp.Y / 10 * 0.90;
//          trdelta.Z   = - toslot->ToUp.Z * numTracksR;
            }

        else {

            trorg.X     = toslot->LowLeft.X + margin;
            trorg.Y     = toslot->LowLeft.Y + toslot->ToUp.Y * ( 1 - ( 0.5 + ( numTracksR - 0.5 ) * 0.90 ) );
            trorg.Z     = toslot->LowLeft.Z;

            trdelta.X   = 0;
            trdelta.Y   = - toslot->ToUp.Y * numTracksR * 0.90;
            trdelta.Z   = 0;

            if ( ispositive && ! IsIntensityModes () ) { // slightly shift downward
                trorg.Y     = toslot->LowLeft.Y + toslot->ToUp.Y * ( 1 - ( 0.5 + ( numTracksR - 0.5 ) * 0.90 ) - ( TracksFlipVert ? -0.50 : 0.25 ) * numTracksR );
                trdelta.Y   = - toslot->ToUp.Y * numTracksR * 0.9;
                }


//          trorg.X     = toslot->LowLeft.X + drawmargin * 0.10 * toslot->ToRight.X + toslot->ToUp.X * ( 1 - numTracksR ) * 0.95;
//          trorg.Y     = toslot->LowLeft.Y + drawmargin * 0.10 * toslot->ToRight.Y + toslot->ToUp.Y * ( 1 - numTracksR ) * 0.95;
//          trorg.Z     = toslot->LowLeft.Z + drawmargin * 0.10 * toslot->ToRight.Z + toslot->ToUp.Z * ( 1 - numTracksR ) * 0.95;
//          trdelta.X   = - toslot->ToUp.X * numTracksR;
//          trdelta.Y   = - toslot->ToUp.Y * numTracksR * 0.90;
//          trdelta.Z   = - toslot->ToUp.Z * numTracksR;
            }


//        bool                        drawvertscale   = ShowVertScale  && CurrentDisplaySpace == DisplaySpaceNone && ( RenderingMode == LayoutOnePage || RenderingMode == LayoutTwoPages ) && ( trackssuper || -trdelta.Y > stepy / 4.0 );
//        bool                        drawhoriscale   = ShowHorizScale && CurrentDisplaySpace == DisplaySpaceNone && ( RenderingMode == LayoutOnePage || RenderingMode == LayoutTwoPages );
//        bool                        drawvertscale   = ShowVertScale  && CurrentDisplaySpace == DisplaySpaceNone && enoughh && ( trackssuper || -trdelta.Y > stepy / 4.0 );

        bool                        canvertgrid     =  numTracks 
                                                    && CurrentDisplaySpace == DisplaySpaceNone 
                                                    && ( trackssuper || enoughh || -trdelta.Y > stepy / 4.0 );

        bool                        drawvertscale   =  ShowVertScale  
                                                    && canvertgrid;

        bool                        drawhoriscale   =  ShowHorizScale 
                                                    && numTracks 
                                                    && CurrentDisplaySpace == DisplaySpaceNone 
                                                 // && enoughw
                                                    && ( toslot->ToRight.Norm() >= EEGGLVIEW_ENOUGHWHORIZGRID ); 

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
                linewidth  *= NoMore ( CurrentDisplaySpace != DisplaySpace3D ? ( ( how & GLPaintOwner ) ? 0.25 : 0.35 )
                                                                             : 0.75,
                                       ws * 0.0035 );

                                        // zoom factor
                linewidth  *= GetCurrentZoomFactor ( how & GLPaintOwner );

                                        // object size factor
                linewidth  *= XYZDoc->GetBounding ( CurrentDisplaySpace )->Radius ();
                }

            else { // CurrentDisplaySpace == DisplaySpaceNone
                                        // horizontal scaling
                linewidth   = NoMore ( 0.5, scaleh * 0.04 );

                                        // vertical scaling
                linewidth  *= NoMore ( 2.0, sqrt ( maxhpertrack ) * 0.15 );

                                        // extra kick for average ROIs
                if ( avgrois )
                    linewidth  *= 2;

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
        if ( CurrentDisplaySpace == DisplaySpaceNone && ( how & GLPaintOwner ) ) {

            glColor4f ( AxisColor );

            glBegin ( GL_LINES );
            glVertex3f ( trorg.X, toslot->LowLeft.Y,                  toslot->LowLeft.Z );
            glVertex3f ( trorg.X, toslot->LowLeft.Y + toslot->ToUp.Y, toslot->LowLeft.Z );
            glEnd();
            }
/*
        else {
            glBegin ( GL_LINES );
            glVertex3f ( trorg.X, trorg.Y - toslot->ToUp.Norm() / 2, trorg.Z );
            glVertex3f ( trorg.X, trorg.Y + toslot->ToUp.Norm() / 2, trorg.Z );
            glEnd();
            }
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // triggers
        if (     ShowTags 
            && ( how & GLPaintOwner )
            && ( CurrentDisplaySpace != DisplaySpace3D || trorg.Z <= ModelRadius ) ) {    // no triggers in back part of 3D

            DrawTriggers ( toslot );
            GLColoringOn        ();
            }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the horizontal grid - pre
        if (   CurrentDisplaySpace == DisplaySpaceNone 
            && drawhoriscale 
            && IsTracksMode () 
            && ( how & GLPaintOwner ) ) {

            double              x;
            double              y1;
            double              y2;
            double              z;

            GLBlendOn ();

            glColor4f ( AxisColor );

            y1  = toslot->LowLeft.Y;
            y2  = toslot->LowLeft.Y + toslot->ToUp.Y;
            z   = toslot->LowLeft.Z;

            char                stf  [ 32 ];
            char                stime[ 32 ];

            glBegin ( GL_LINES );
            for ( double g = hgridorg - CDPt.GetMin(); g < CDPt.GetLength(); g += hgridstep ) {
                x       = trorg.X + ( g + ( OneMoreHoriz () ? 0.5 : 0 ) ) * scaleh;
                glVertex3f ( x, y1, z );
                glVertex3f ( x, y2, z );
                }
            glEnd();

            for ( double g = hgridorg - CDPt.GetMin(); g < CDPt.GetLength(); g += hgridstep ) {

                TFToString ( g + CDPt.GetMin(), stf, stime, ShowHorizScaleLegal );

                SFont->Print ( trorg.X + ( g + ( OneMoreHoriz () ? 0.5 : 0 ) ) * scaleh + 3, y1, z, *stime ? stime : stf, TA_LEFT | TA_BOTTOM );
                }
            } // drawhoriscale - pre


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // go to first track
                                        // from here, we can use glVertex2f, as z=0 all along
        glTranslatef ( trorg.X, trorg.Y, trorg.Z );

                                        // 1st name
        firsttrack  = true;
        int         st          = sel->FirstSelected ();

                                        // select color
        if      ( Highlighted[ st ] )   glColor4f ( HighlightColor );
        else if ( BadTracks  [ st ] )   glColor4f ( BadColor       );
        else if ( AuxTracks  [ st ] )   glColor4f ( AuxColor       );
        else                            toslot->Color.GLize ();


        if ( drawmargin && ( how & GLPaintOwner ) ) {

            if ( roiname ) {
                                            // result is visually centered in both cases
                if ( trackssuper || avgrois )

                    SFont->Print ( -4, 0, 0,  toslot->Name, TA_RIGHT | TA_CENTERY );

                else

                    SFont->Print ( -4, toslot->ToUp.Y * -0.42, 0, toslot->Name, TA_RIGHT | TA_CENTERY );
                } // roiname

            else if ( ! allnames || trackssuper ) {
                                            // draw first of summary depending on superimpose
                if ( trackssuper ) {

                    if ( numTracks > 1 )

                        glTranslatef ( 0, SFont->GetHeight() / 2, 0 );

                    SFont->Print ( -4, 0, 0, GetElectrodeName ( st ), TA_RIGHT | TA_CENTERY );

                    if ( numTracks > 1 )

                        glTranslatef ( 0, - SFont->GetHeight() / 2, 0 );
                    }
                else
                    SFont->Print ( -4, 0, 0, GetElectrodeName ( st ), TA_RIGHT | TA_CENTERY );
                } // ! allnames

            }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // globally rescale everything
        glScaled ( scaleh, scalev, 1 );
                                        // and keep factors to scale-backward
        descalex    = 1 / scaleh;
        descaley    = 1 / scalev;


//      quality     = Outputing () || ( ( SelTracks.NumSet () * nump < 40000 ) && !( RButtonDown || VkLButton () ) )  ? QuadBestQuality
//                  : VkLButton () || VkRButton ()                                                                    ? QuadFastestQuality
//                  :                                                                                                   QuadRegularQuality;

        quality     = ! Outputing () && ( GetFocus () == GetHandle () && ( ( CurrentDisplaySpace != DisplaySpaceNone && VkLButton () ) || VkRButton () ) ) ? QuadFastestQuality
                    :   Outputing () || ( SelTracks.NumSet () * nump < 40000 )                                                                             ? QuadBestQuality
                    :                                                                                                                                        QuadRegularQuality;


                                        // draw each selected track
        int     st0     = 0;
        for ( TIteratorSelectedForward sti ( *sel ); (bool) sti; ++sti, ++st0, firsttrack = false ) {

            st          = sti();

            scalet      = ( ! TracksFlipVert || st > EEGDoc->GetLastRegularIndex () ? 1 : -1 ) * ScaleTracks[ st ];
                                        // workaround bug for QuadMesh that behave strangely with irregular scaling
            if ( IsIntensityModes () ) scalet = 1;

            regel       = st <= EEGDoc->GetLastRegularIndex();  // regular electrode

            glScaled ( 1, scalet, 1 );
            descaley    = 1 / scalev / scalet;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the vertical grid - pre
            if (   CurrentDisplaySpace == DisplaySpaceNone 
                && drawvertscale 
                && ! IsIntensityModes () 
                && ( !trackssuper || st == firstsel ) 
                && ( how & GLPaintOwner )             ) {

                double      dv;
                double      pol     = TracksFlipVert && ! AuxTracks[ st ] ? -1 : 1;;
                double      y;
                double      ot      = regel ? OffsetTracks : 0;
                double      maxy    = ( ( numTracks == 1 || trackssuper ) ? toslot->ToUp.Y                     * descaley / 2
                                                                          : toslot->ToUp.Y / ( numTracks + 2 ) * descaley / 2 )
                                      * pol;
                                        // vertical step, in value
                dv          = fabs ( stepy * descaley );
                dv          = Power ( 10, Truncate ( Log10 ( dv ) ) );

                if      ( pol * dv / descaley < 20  )   dv *= 2;  // too close -> increase space
                else if ( pol * dv / descaley > 200 )   dv /= 10; // too far   -> reduce space


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
                    glVertex2f ( maxp + ( OneMoreHoriz () ? 1 : 0 ),  y - ot );
                    glEnd ();

                    sprintf ( buff, "%.6g",  y );
                    SFont->Print ( 3 * descalex,  y - ot, trorg.Z, buff, TA_LEFT | TA_TOP );
                    }
                } // drawvertscale - pre


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // draw the thresholds - pre
            if (    canvertgrid 
               && ! IsIntensityModes () 
               && ( ! trackssuper || st == firstsel ) 
               && regel
               && ( threshabove || threshbelow ) && ( how & GLPaintOwner ) ) {

                double      t;

                glColor4f ( ThresholdColor );
//              glLineWidth ( 2 );

                if ( threshabove ) {
                    t   = EEGDoc->GetFilters ()->ThresholdAbove;
                    sprintf ( buff, "%.6g", t );
                    t  -= OffsetTracks;

                    glBegin ( GL_LINES );
                    glVertex2f ( minp, t );
                    glVertex2f ( maxp, t );
                    glEnd();

                    if ( enoughw && GetNumWindowSlots () < 100 )
                        SFont->Print ( 3 * descalex, t, trorg.Z, buff, TA_LEFT | TA_TOP );
                    }

                if ( threshbelow ) {
                    t   = EEGDoc->GetFilters ()->ThresholdBelow;
                    sprintf ( buff, "%.6g", t );
                    t  -= OffsetTracks;

                    glBegin ( GL_LINES );
                    glVertex2f ( minp, t );
                    glVertex2f ( maxp, t );
                    glEnd();

                    if ( enoughw && GetNumWindowSlots () < 100 )
                        SFont->Print ( 3 * descalex, t, trorg.Z, buff, TA_LEFT | TA_TOP );
                    }

//              glLineWidth ( 1 );
                } // thresholds - pre
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // select color (but not applied yet)
            if      ( Highlighted[ st ] )   LineColor.Set ( 0, HighlightColor );
            else if ( BadTracks  [ st ] )   LineColor.Set ( 0, BadColor       );
            else if ( (bool) Using ) { // override when linking multiple Eegs

                int     index   = NestedPaint ? ( (int) iteratorview + 1 ) % MaxLinkColors : 0;
                LineColor[ 0 ]  = LinkColors[ index ];

                if ( index >= DashLinkColors ) {
                    glLineStipple   ( /*CaptureMode == CaptureGLMagnify ? Magnifier[2] :*/ 1, 0xf0f0 );
                    glEnable        ( GL_LINE_STIPPLE );
                    }
                }
            else if ( AuxTracks        [ st ] )     LineColor.Set ( 0, AuxColor       );
            else                                    LineColor[ 0 ] = toslot->Color;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Filling with computed colors (indexed or analog)
            if (   IsFilling 
                && IsTracksMode () 
                && Filling[ st ] ) {

                int                 ref         = Filling[ st ]->GetRef();
                TGLColor<GLfloat>   col1;

                bool                discrete    = Filling[ st ]->GetHow() == Discrete;
                bool                drawtext    = discrete && enoughw && enoughh && ( CaptureMode == CaptureGLMagnify && ! (bool) AnimFx || WindowSlots[0].ToRight.Norm() / CDPt.GetLength() > 1 );
                float*              toref       = EegBuff[ ref ] + minp;

                                        // handy lambdas
                auto    IsSegmentTransition = [ &toref  ] ( int p ) { return p == 0 /*minp*/ || *toref != *(toref-1); };

                auto    IsSharpTransition   = [ &tobuff ] ( int p ) { return    p == 1                                  // transition on first TF
                                                                             || p >  0 && *tobuff * *(tobuff-1) <= 0    // sign inversion
                                                                             || p >  1 && *(tobuff-1) == *(tobuff-2); };// blocks of constant values

                                        // Segments' coloring
                if ( how & GLPaintOwner ) {     // overlapping hides everything... maybe transparent?

                    if ( IsFilling == FillingVoid
                      || (bool) Using
                      || ( discrete && SelFilling && SelFilling->IsNotSelected ( *toref ) )
                      || ! Filling[ st ]->GetColor ( *toref, col1 ) )

                        col1    = BackColor[ 2 ];  // BackColor[ Outputing() ];

                                        // start first segment
                    glBegin ( GL_QUAD_STRIP );
                    col1.GLize ();


                    for ( tobuff = EegBuff[ st ] + minp, p = minp; p <= maxp; p++, tobuff++, toref++ ) {

                        if ( IsFilling == FillingVoid
                         || (bool) Using
                         || ( discrete && SelFilling && SelFilling->IsNotSelected ( *toref ) )
                         || ! Filling[ st ]->GetColor ( *toref, col1 ) )

                            col1    = BackColor[ 2 ]; // BackColor[ Outputing() ];

                                        // accounting for sharp transitions ACROSS segments as well as WITHIN segment (can happen)
                        if ( IsSharpTransition ( p ) ) {

                            glVertex2f ( p, 0           - OffsetTracks );
                            glVertex2f ( p, *(tobuff-1) - OffsetTracks );
                                        // end current segment and start a new one, with identical color
                            //glEnd   ();
                            //glBegin ( GL_QUAD_STRIP );
                            col1.GLize ();
                            }
                                        // do we have a color transition?
                        else if ( IsSegmentTransition ( p ) ) {
                                        // do some continuous transition
                            glVertex2f ( p, 0       - OffsetTracks );
                            glVertex2f ( p, *tobuff - OffsetTracks );
                                        // end current segment and start a new one, with new color
                            //glEnd   ();
                            //glBegin ( GL_QUAD_STRIP );
                            col1.GLize ();
                            } // color transition

                                        // within color segment, just output current coordinates
                        glVertex2f ( p, 0       - OffsetTracks );
                        glVertex2f ( p, *tobuff - OffsetTracks );
                        } // for p

                                        // 1 more step to show the last TF
                    glVertex2f ( maxp + 1, 0                     - OffsetTracks );
                    glVertex2f ( maxp + 1, EegBuff[ st ][ maxp ] - OffsetTracks );  // maxp + 1, but it might not exist, nor is up-to-date

                    glEnd ();
                    } // Segments' coloring

                                        // draw the segment number and vertical line separators
                if ( drawtext ) {

                    glLineWidth         ( linewidth );
//                  GLLineSmoothOn      ();


                    if ( IsFilling == FillingVoid )
                        col1    = LineColor[ 0 ];

                    if ( (bool) Using )
                        LineColor.GLize ( 0 );


                    for ( tobuff = EegBuff[ st ] + minp, toref = EegBuff[ ref ] + minp, p = minp; p <= maxp; p++, tobuff++, toref++ )

                                        // do we have a color transition?
                        if ( IsSegmentTransition ( p ) ) {

                            bool    nextseg     = SelFilling          && SelFilling->IsSelected (  *toref    ); // entering a selected seg
                            bool    prevseg     = SelFilling && p > 0 && SelFilling->IsSelected ( *(toref-1) ); // leaving  a selected seg

                                        // out of boundaries in filling indexes?
                            if ( IsFilling == FillingColor && ! Filling[ st ]->GetColor ( *toref, col1 ) && ! (bool) Using
                              || ! nextseg )
                                col1    = TextColor[ 0 ];

                            if ( ! (bool) Using )
                                col1.GLize ();

                                        // prevent printing next not-selected segment
                            if ( *toref != 0 && nextseg )
                                SFont->Print ( p, - ( 2 + ( ( how & GLPaintOwner ) || !(bool) Using ? 0 : ( (int) iteratorview + 1 ) * SFont->GetHeight() ) ) * descaley, trorg.Z, IntegerToString ( buff, *toref ), TA_LEFT | TA_TOP );


                            if ( ! (bool) Using )
                                LineColor.GLize ( 0 );


                            if ( prevseg ) {
                                        // draw end of segment's separator
                                if ( IsSharpTransition ( p ) ) {
                                    glBegin ( GL_LINES );
                                    glVertex2f ( p, 0       );
                                    glVertex2f ( p, *(tobuff-1) );
                                    glEnd   ();
                                    }
                                else {
                                    glBegin ( GL_LINES );
                                    glVertex2f ( p, 0       );
                                    glVertex2f ( p, *tobuff );
                                    glEnd   ();
                                    }
                                }

                            if (   nextseg      // draw beginning of segment's separator - !needed if tracks have sharp sign transitions!
                                && p > 0 ) {    // don't draw a line if starting point of segment is out of sight

                                glBegin ( GL_LINES );
                                glVertex2f ( p, 0       );
                                glVertex2f ( p, *tobuff );
                                glEnd   ();
                                }
                            } // color transition

                                        // separator for 1st TF
                    if ( CDPt.GetMin () == 0 ) {
                        glBegin ( GL_LINES );
                        glVertex2f ( 0,    0                     );
                        glVertex2f ( 0,    EegBuff[ st ][ minp ] );
                        glEnd();
                        }

                                        // separator for last TF
                    if ( CDPt.GetMax () == CDPt.GetLimitMax () ) {
                        glBegin ( GL_LINES );
                        glVertex2f ( maxp + 1, 0                     );
                        glVertex2f ( maxp + 1, EegBuff[ st ][ maxp ] );
                        glEnd();
                        }


                    glLineWidth         ( 1 );
//                  GLLineSmoothOff     ();
                    } // if drawtext

                } // if IsFilling


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // drawing thicker background line, underneath the real plot - !copy of regular drawing code!
            if (   ! IsIntensityModes () 
                && ( ! regel || CurrentDisplaySpace != DisplaySpaceNone ) ) {

                BackColor.GLize ();

                if ( IsTracksMode () ) {

                    glLineWidth ( linewidth + 2 );

                    glBegin ( GL_LINE_STRIP );

                    if ( (bool) Montage && regel ) {

                        if ( (bool) Montage[ st ] ) // draw the graphical subtraction

                            if ( Montage[ st ].TwoTracks () )
                                for ( tobuff1 = Montage[ st ].ToData1 + minp,
                                      tobuff2 = Montage[ st ].ToData2 + minp, p = minp; p <= maxp; p++, tobuff1++, tobuff2++ )
                                    glVertex2f ( p, *tobuff1 - *tobuff2 - OffsetTracks );
                            else
                                for ( tobuff1 = Montage[ st ].ToData1 + minp, p = minp; p <= maxp; p++, tobuff1++ )
                                    glVertex2f ( p, *tobuff1 - OffsetTracks );
//                      else
//                          ;               // no montage, simply skip
                        } // if Montage
                    else {                  // regular plotting
                        if ( regel )
                            for ( tobuff = EegBuff[ st ] + minp, p = minp; p <= maxp; p++, tobuff++ )
                                glVertex2f ( p, *tobuff - OffsetTracks );
                        else
                            for ( tobuff = EegBuff[ st ] + minp, p = minp; p <= maxp; p++, tobuff++ )
                                glVertex2f ( p, *tobuff );


                        if ( IsFilling )    // 1 more step to show the last TF
                            glVertex2f ( maxp + 1, EegBuff[ st ][ maxp ] - ( regel ? OffsetTracks : 0 ) );  // maxp + 1, but it might not exist, nor is up-to-date
                        }

                    glEnd();
                    } // IsTracksMode ()

                else if ( IsBarsMode () ) {

                    BackColor.GLize ();
                    glLineWidth ( 2 );
                    glPolygonMode ( GL_FRONT_AND_BACK, GL_LINE );

                    glBegin ( GL_QUADS );

                    if ( (bool) Montage && regel ) {

                        if ( (bool) Montage[ st ] ) // draw the graphical subtraction

                            if ( Montage[ st ].TwoTracks () )
                                for ( tobuff1 = Montage[ st ].ToData1 + minp, tobuff2 = Montage[ st ].ToData2 + minp, p = minp; p <= maxp; p++, tobuff1++, tobuff2++ ) {
                                    glVertex2f ( p    ,                     - OffsetTracks );
                                    glVertex2f ( p    , *tobuff1 - *tobuff2 - OffsetTracks );
                                    glVertex2f ( p + 1, *tobuff1 - *tobuff2 - OffsetTracks );
                                    glVertex2f ( p + 1,                     - OffsetTracks );
                                    }
                            else
                                for ( tobuff1 = Montage[ st ].ToData1 + minp, p = minp; p <= maxp; p++, tobuff1++ ) {
                                    glVertex2f ( p    ,          - OffsetTracks );
                                    glVertex2f ( p    , *tobuff1 - OffsetTracks );
                                    glVertex2f ( p + 1, *tobuff1 - OffsetTracks );
                                    glVertex2f ( p + 1,          - OffsetTracks );
                                    }
    //                  else
    //                      ;               // no montage, simply skip
                        }
                    else                    // regular plotting
                        if ( regel )
                            for ( tobuff = EegBuff[ st ] + minp, p = minp; p <= maxp; p++, tobuff++ ) {
                                glVertex2f ( p    ,         - OffsetTracks );
                                glVertex2f ( p    , *tobuff - OffsetTracks );
                                glVertex2f ( p + 1, *tobuff - OffsetTracks );
                                glVertex2f ( p + 1,         - OffsetTracks );
                                }

                        else
                            for ( tobuff = EegBuff[ st ] + minp, p = minp; p <= maxp; p++, tobuff++ ) {
                                glVertex2f ( p    , 0       );
                                glVertex2f ( p    , *tobuff );
                                glVertex2f ( p + 1, *tobuff );
                                glVertex2f ( p + 1, 0       );
                                }

                    glEnd();
                    glPolygonMode ( GL_FRONT_AND_BACK, GL_FILL );
//                    glLineWidth ( linewidth );

                    } // IsBarsMode ()

                glLineWidth ( 1 );

                } // draw background


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw baseline axis
            if (   ShowBaseline 
                && ! IsIntensityModes () 
                && ( firsttrack || ! trackssuper ) 
              /*&& ( how & GLPaintOwner )*/        ) {

                if ( IsFilling && Filling[ st ] )
                    LineColor.GLize ( 0 );
                else
                    glColor4f ( AxisColor );

                glBegin ( GL_LINES );
                glVertex2f ( minp,                          0 );
                glVertex2f ( maxp + ( OneMoreHoriz () ? 1 : 0 ),  0 );
                glEnd();

                                        // draw time origin position - pre tracks
                if ( draworigin ) {
//                  glColor4f ( AxisColor );
                    glLineWidth ( 2 );

                    glBegin ( GL_LINES );
                    glVertex2f ( orgreltf + ( OneMoreHoriz () ? 0.5 : 0 ),  4 * descaley );
                    glVertex2f ( orgreltf + ( OneMoreHoriz () ? 0.5 : 0 ), -4 * descaley );
                    glEnd();

//                  SFont->Print ( x, 0, 0, "Origin", TA_LEFT | TA_TOP );
                    } // draw origin - pre

                } // if ShowBaseline


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

            GLLineSmoothOn      ();

                                        // no SD if montage - test EEGDoc directly, if painting for an outer EEG
            if (   ShowSD 
                && IsTracksMode () 
                && EEGDoc->HasStandardDeviation () 
                && regel 
                && ! (bool) Montage          ) {

                glColor4f   (   NoMore ( 0.80, LineColor[ 0 ].Red   + 0.50 ),
                                NoMore ( 0.80, LineColor[ 0 ].Green + 0.50 ),
                                NoMore ( 0.80, LineColor[ 0 ].Blue  + 0.50 ),
                                ShowSD == SDFilledTransparent ? SDTransparency : 1.00
                            );

                glLineWidth ( 1.0 );

                                        // fill anti-aliased
                if ( ShowSD == SDFilledTransparent
                  || ShowSD == SDFilledOpaque ) {

                    glBegin ( GL_QUAD_STRIP );

                    for ( tobuff = EegBuff[ st ] + minp, tosdbuff = SdBuff[ st ] + minp, p = minp; p <= maxp; p++, tobuff++, tosdbuff++ ) {
                        glVertex2f ( p, *tobuff + *tosdbuff - OffsetTracks );
                        glVertex2f ( p, *tobuff - *tosdbuff - OffsetTracks );
                        }
                                        // do we have SD on top of filled (segmentation) display?
//                  if ( IsFilling )    // 1 more step to show the last TF
                    glEnd();
                    } // SDFilledTransparent, SDFilledOpaque

                                        // draw 2 borders
                if ( ShowSD == SDDashed ) { // either dashed or on top of the filled to emulate anti-aliasing
                    glLineStipple   ( 1, 0x3333 );
                    glEnable        ( GL_LINE_STIPPLE );
                    }


                glBegin ( GL_LINE_STRIP );

                for ( tobuff = EegBuff[ st ] + minp, tosdbuff = SdBuff[ st ] + minp, p = minp; p <= maxp; p++, tobuff++, tosdbuff++ )
                    glVertex2f ( p, *tobuff + *tosdbuff - OffsetTracks );
                                        // do we have SD on top of filled (segmentation) display?
//              if ( IsFilling )        // 1 more step to show the last TF
//                  glVertex2f ( maxp + 1, EegBuff[ st ][ maxp ] + SdBuff[ st ][ maxp ] - OffsetTracks );  // maxp + 1, but it might not exist, nor is up-to-date

                glEnd();


                glBegin ( GL_LINE_STRIP );

                for ( tobuff = EegBuff[ st ] + minp, tosdbuff = SdBuff[ st ] + minp, p = minp; p <= maxp; p++, tobuff++, tosdbuff++ )
                    glVertex2f ( p, *tobuff - *tosdbuff - OffsetTracks );
                                        // do we have SD on top of filled (segmentation) display?
//              if ( IsFilling )        // 1 more step to show the last TF
//                  glVertex2f ( maxp + 1, EegBuff[ st ][ maxp ] - SdBuff[ st ][ maxp ] - OffsetTracks );  // maxp + 1, but it might not exist, nor is up-to-date

                glEnd();


                if ( ShowSD == SDDashed )
                    glDisable ( GL_LINE_STIPPLE );
                } // ShowSD TracksMode


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set line width for track
            if      ( Highlighted[ st ] )
                glLineWidth ( linewidth + 1 );
            else
                glLineWidth ( linewidth );

            LineColor.GLize ( 0 );

                                        // draw name
                            // remove to show all individual tracks names
            if (   drawmargin 
                && ! roiname 
                && allnames 
                && ! avgrois 
                && ! trackssuper 
                && ( how & GLPaintOwner ) )

                SFont->Print ( -4 * descalex, 0, 0, GetElectrodeName ( st ), TA_RIGHT | TA_CENTERY );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the track, with its own relative scaling and polarity

            if      ( IsIntensityModes () ) {

                constexpr double    ESaturate   = 0.90;

                v1[ 2 ] = v2[ 2 ] = 0;

                                        // change the whole ColorTable once for each track, with all scalings inside it
                if (   ScalingAuto != ScalingAutoOff 
                    && ! ( RButtonDown && IntensityOverride && MouseAxis == MouseAxisHorizontal ) ) {

                    double          absmax      = max ( maxValue, fabs ( minValue ) );
                    double          ctmin       = ScalingAuto == ScalingAutoAsymmetric ? minValue : - absmax;
                    double          ctmax       = ScalingAuto == ScalingAutoAsymmetric ? maxValue :   absmax;

                    ColorTable.SetParameters ( ctmax * ( IsIntensityPrecise () ? 1 : ESaturate + ScalingContrast * ( 1 - ESaturate ) ),
                                               ctmin * ( IsIntensityPrecise () ? 1 : ESaturate + ScalingContrast * ( 1 - ESaturate ) ), 
                                               contrast, 
                                               alpha );
                    }
                else

                    ColorTable.SetParameters (  15 * ( IsIntensityPrecise () ? 1 : ESaturate + ScalingContrast * ( 1 - ESaturate ) ) / ( ScalingLevel * ScaleTracks[ st ] ),
                                               -15 * ( IsIntensityPrecise () ? 1 : ESaturate + ScalingContrast * ( 1 - ESaturate ) ) / ( ScalingLevel * ScaleTracks[ st ] ),
                                                contrast, 
                                                alpha );

                                        // Pixel-style, start
                if ( IsIntensitySquareMode () ) {

                    v1[ 1 ] =   trdelta.Y * descaley / 2;
                    v2[ 1 ] = - v1[ 1 ];

                    glBegin ( GL_QUADS );

                    if ( (bool) Montage && regel ) {

                        if ( (bool) Montage[ st ] ) // draw the graphical subtraction

                            if ( Montage[ st ].TwoTracks () )
                                for ( tobuff1 = Montage[ st ].ToData1 + minp, tobuff2 = Montage[ st ].ToData2 + minp, *v1 = *v2 = minp; *v1 <= maxp; tobuff1++, tobuff2++ ) {
                            
                                    ColorTable.GLize ( *tobuff1 - *tobuff2 );

                                    glVertex3fv ( v1 ); glVertex3fv ( v2 );
                                    (*v1)++; (*v2)++;
                                    glVertex3fv ( v2 ); glVertex3fv ( v1 );
                                    }
                            else
                                for ( tobuff1 = Montage[ st ].ToData1 + minp, *v1 = *v2 = minp; *v1 <= maxp; tobuff1++ ) {
                            
                                    ColorTable.GLize ( *tobuff1 );

                                    glVertex3fv ( v1 ); glVertex3fv ( v2 );
                                    (*v1)++; (*v2)++;
                                    glVertex3fv ( v2 ); glVertex3fv ( v1 );
                                    }
                        else                        // null track (no montage)
                            for ( *v1 = *v2 = minp; *v1 <= maxp; ) {
                            
                                ColorTable.GLize ( 0 );

                                glVertex3fv ( v1 ); glVertex3fv ( v2 );
                                (*v1)++; (*v2)++;
                                glVertex3fv ( v2 ); glVertex3fv ( v1 );
                                }
                        } // Montage
                    else                        // regular plotting
                        for ( tobuff = EegBuff[ st ] + minp, *v1 = *v2 = minp; *v1 <= maxp; tobuff++ ) {
                            
                            ColorTable.GLize ( *tobuff );

                            glVertex3fv ( v1 ); glVertex3fv ( v2 );
                            (*v1)++; (*v2)++;
                            glVertex3fv ( v2 ); glVertex3fv ( v1 );
                            }

                    glEnd();
                                        // Pixel-style, end
                    }

                else { // IsIntensitySmoothMode ()
                                        // Mesh-style, start
                    glTranslatef ( 0, - st0 * trdelta.Y * descaley, 0 );


                    if ( st == firstsel ) {

                        QuadMesh.Begin ( maxp - minp + 1, quality, ColorTable );

                        v1[ 1 ] = trdelta.Y * -0.5 * descaley;

                                            // copy of code
                        if ( (bool) Montage && regel ) {

                            if ( (bool) Montage[ st ] ) // draw the graphical subtraction

                                if ( Montage[ st ].TwoTracks () )
                                    for ( tobuff1 = Montage[ st ].ToData1 + minp, tobuff2 = Montage[ st ].ToData2 + minp, *v1 = minp; *v1 <= maxp; (*v1)++, tobuff1++, tobuff2++ )
                                        QuadMesh.NextVertex ( v1, *tobuff1 - *tobuff2 );
                                else
                                    for ( tobuff1 = Montage[ st ].ToData1 + minp, *v1 = minp; *v1 <= maxp; (*v1)++, tobuff1++ )
                                        QuadMesh.NextVertex ( v1, *tobuff1 );
                            else                        // null track (no montage)
                                for ( *v1 = minp; *v1 <= maxp; (*v1)++ )
                                    QuadMesh.NextVertex ( v1, 0 );
                            } // Montage
                        else                        // regular plotting
                            for ( tobuff = EegBuff[ st ] + minp, *v1 = minp; *v1 <= maxp; (*v1)++, tobuff++ )
                                QuadMesh.NextVertex ( v1, *tobuff );
                        } // firstsel


                    v1[ 1 ] = trdelta.Y * st0 * descaley;

                    if ( (bool) Montage && regel ) {

                        if ( (bool) Montage[ st ] ) // draw the graphical subtraction

                            if ( Montage[ st ].TwoTracks () )
                                for ( tobuff1 = Montage[ st ].ToData1 + minp, tobuff2 = Montage[ st ].ToData2 + minp, *v1 = minp; *v1 <= maxp; (*v1)++, tobuff1++, tobuff2++ )
                                    QuadMesh.NextVertex ( v1, *tobuff1 - *tobuff2 );
                            else
                                for ( tobuff1 = Montage[ st ].ToData1 + minp, *v1 = minp; *v1 <= maxp; (*v1)++, tobuff1++ )
                                    QuadMesh.NextVertex ( v1, *tobuff1 );
                        else                        // null track (no montage)
                            for ( *v1 = minp; *v1 <= maxp; (*v1)++ )
                                QuadMesh.NextVertex ( v1, 0 );
                        } // Montage
                    else                        // regular plotting
                        for ( tobuff = EegBuff[ st ] + minp, *v1 = minp; *v1 <= maxp; (*v1)++, tobuff++ )
                            QuadMesh.NextVertex ( v1, *tobuff );


                    if ( st == lastsel ) {

                        v1[ 1 ] = trdelta.Y * ( st0 + 0.5 ) * descaley;
                                            // copy of code
                        if ( (bool) Montage && regel ) {

                            if ( (bool) Montage[ st ] ) // draw the graphical subtraction

                                if ( Montage[ st ].TwoTracks () )
                                    for ( tobuff1 = Montage[ st ].ToData1 + minp, tobuff2 = Montage[ st ].ToData2 + minp, *v1 = minp; *v1 <= maxp; (*v1)++, tobuff1++, tobuff2++ )
                                        QuadMesh.NextVertex ( v1, *tobuff1 - *tobuff2 );
                                else
                                    for ( tobuff1 = Montage[ st ].ToData1 + minp, *v1 = minp; *v1 <= maxp; (*v1)++, tobuff1++ )
                                        QuadMesh.NextVertex ( v1, *tobuff1 );
                            else                        // null track (no montage)
                                for ( *v1 = minp; *v1 <= maxp; (*v1)++ )
                                    QuadMesh.NextVertex ( v1, 0 );
                            } // Montage
                        else                        // regular plotting
                            for ( tobuff = EegBuff[ st ] + minp, *v1 = minp; *v1 <= maxp; (*v1)++, tobuff++ )
                                QuadMesh.NextVertex ( v1, *tobuff );


                        QuadMesh.GLize ();
                        } // lastsel


                    glTranslatef ( 0, st0 * trdelta.Y * descaley, 0 );
                    v1[ 1 ] =   trdelta.Y * descaley / 2;
                    v2[ 1 ] = - v1[ 1 ];
                                            // Mesh-style, end
                    } // regel


/*                                        // add a separator between electrodes
                if ( !firsttrack && CurrentDisplaySpace == DisplaySpaceNone && v1[ 1 ] < -0.5 ) {
                    glColor4f ( AxisColor );
                    glBegin ( GL_LINES );
                    glVertex2f ( minp, v2[ 1 ] );
                    glVertex2f ( maxp, v2[ 1 ] );
                    glEnd();
                    } // separator
*/
                                        // draw a box around selected track
                if ( Highlighted[ st ] && trdelta.Y < -2 ) {

                    glColor4f ( HighlightColor );
                    glLineWidth ( 2 );

                    if ( IsIntensitySquareMode () )    maxp++;

                    glBegin ( GL_LINE_STRIP );
                    glVertex2f ( minp, v1[ 1 ] );
                    glVertex2f ( maxp, v1[ 1 ] );
                    glVertex2f ( maxp, v2[ 1 ] );
                    glVertex2f ( minp, v2[ 1 ] );
                    glVertex2f ( minp, v1[ 1 ] );
                    glEnd();

                    if ( IsIntensitySquareMode () )    maxp--;

                    glLineWidth ( 1 );
                    LineColor.GLize ( 0 ); // needed?
                    }


                if ( draworigin && ( how & GLPaintOwner ) ) {

                    glColor4f ( AxisColor );
                    glLineWidth ( 2 );

                    glBegin ( GL_LINES );
                    glVertex2f ( orgreltf + descalex,  0.5 * trdelta.Y * descaley );
                    glVertex2f ( orgreltf + descalex, -0.5 * trdelta.Y * descaley );
                    glEnd();

                    BackColor.GLize ();

                    glBegin ( GL_LINES );
                    glVertex2f ( orgreltf - descalex,  0.5 * trdelta.Y * descaley );
                    glVertex2f ( orgreltf - descalex, -0.5 * trdelta.Y * descaley );
                    glEnd();

//                  SFont->Print ( x, 0, 0, "Origin", TA_LEFT | TA_TOP );
                    }

                } // IsIntensityModes ()

            else if ( IsBarsMode () ) {

                glBegin ( GL_QUADS );

                if ( (bool) Montage && regel ) {

                    if ( (bool) Montage[ st ] ) // draw the graphical subtraction

                        if ( Montage[ st ].TwoTracks () )
                            for ( tobuff1 = Montage[ st ].ToData1 + minp, tobuff2 = Montage[ st ].ToData2 + minp, p = minp; p <= maxp; p++, tobuff1++, tobuff2++ ) {
                                glVertex2f ( p    ,                     - OffsetTracks );
                                glVertex2f ( p    , *tobuff1 - *tobuff2 - OffsetTracks );
                                glVertex2f ( p + 1, *tobuff1 - *tobuff2 - OffsetTracks );
                                glVertex2f ( p + 1,                     - OffsetTracks );
                                }
                        else
                            for ( tobuff1 = Montage[ st ].ToData1 + minp, p = minp; p <= maxp; p++, tobuff1++ ) {
                                glVertex2f ( p    ,          - OffsetTracks );
                                glVertex2f ( p    , *tobuff1 - OffsetTracks );
                                glVertex2f ( p + 1, *tobuff1 - OffsetTracks );
                                glVertex2f ( p + 1,          - OffsetTracks );
                                }
//                  else
//                      ;               // no montage, simply skip
                    }
                else                    // regular plotting
                    if ( regel )
                        for ( tobuff = EegBuff[ st ] + minp, p = minp; p <= maxp; p++, tobuff++ ) {
                            glVertex2f ( p    ,         - OffsetTracks );
                            glVertex2f ( p    , *tobuff - OffsetTracks );
                            glVertex2f ( p + 1, *tobuff - OffsetTracks );
                            glVertex2f ( p + 1,         - OffsetTracks );
                            }

                    else
                        for ( tobuff = EegBuff[ st ] + minp, p = minp; p <= maxp; p++, tobuff++ ) {
                            glVertex2f ( p    , 0       );
                            glVertex2f ( p    , *tobuff );
                            glVertex2f ( p + 1, *tobuff );
                            glVertex2f ( p + 1, 0       );
                            }

                glEnd();

                } // IsBarsMode ()

            else { // IsTracksMode ()

                glBegin ( GL_LINE_STRIP );

                if ( (bool) Montage && regel ) {

                    if ( (bool) Montage[ st ] ) // draw the graphical subtraction

                        if ( Montage[ st ].TwoTracks () )
                            for ( tobuff1 = Montage[ st ].ToData1 + minp, tobuff2 = Montage[ st ].ToData2 + minp, p = minp; p <= maxp; p++, tobuff1++, tobuff2++ )
                                glVertex2f ( p, *tobuff1 - *tobuff2 - OffsetTracks );
                        else
                            for ( tobuff1 = Montage[ st ].ToData1 + minp, p = minp; p <= maxp; p++, tobuff1++ )
                                glVertex2f ( p, *tobuff1 - OffsetTracks );
//                  else
//                      ;               // no montage, simply skip
                    }
                else {                  // regular plotting

                    if ( regel )

                        for ( tobuff = EegBuff[ st ] + minp, p = minp; p <= maxp; p++, tobuff++ ) {

                            if (   IsFilling 
                                && Filling[ st ]
                          //    && IsSharpTransition ( p )     // not quite
                                && (   p >  0 && *tobuff * *(tobuff-1) <= 0
                                    || p >  1 && *(tobuff-1) == *(tobuff-2) ) ) // does not detect 1 TF constant before...

                                glVertex2f ( p, *(tobuff-1) - OffsetTracks );   // add intermediate point to have a sharp line

                            glVertex2f ( p, *tobuff - OffsetTracks );
                            }
                    else
                        for ( tobuff = EegBuff[ st ] + minp, p = minp; p <= maxp; p++, tobuff++ )
                            glVertex2f ( p, *tobuff );


                    if ( IsFilling )    // 1 more step to show the last TF
                        glVertex2f ( maxp + 1, EegBuff[ st ][ maxp ] - ( regel ? OffsetTracks : 0 ) );  // maxp + 1, but it might not exist, nor is up-to-date
                    }

                glEnd();

                } // IsTracksMode ()


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // in BarsMode, we have to draw the SD on top of the bars
            if (   ShowSD 
                && IsBarsMode () 
                && EEGDoc->HasStandardDeviation () 
                && regel 
                && ! (bool) Montage          ) {

                glColor4f   (   NoMore ( 0.80, LineColor[ 0 ].Red   + 0.50 ),
                                NoMore ( 0.80, LineColor[ 0 ].Green + 0.50 ),
                                NoMore ( 0.80, LineColor[ 0 ].Blue  + 0.50 ),
                                ShowSD == SDFilledTransparent ? SDTransparency : 1.00
                            );

                glLineWidth         ( 1.0 );
                GLLineSmoothOff     (); // to make the lines clearer (no anti-aliasing)


                if ( ShowSD == SDFilledTransparent
                  || ShowSD == SDFilledOpaque ) {

                    glBegin ( GL_QUADS );

                    for ( tobuff = EegBuff[ st ] + minp, tosdbuff = SdBuff[ st ] + minp, p = minp; p <= maxp; p++, tobuff++, tosdbuff++ ) {
                        glVertex2f ( p,     *tobuff - *tosdbuff - OffsetTracks );
                        glVertex2f ( p,     *tobuff + *tosdbuff - OffsetTracks );
                        glVertex2f ( p + 1, *tobuff + *tosdbuff - OffsetTracks );
                        glVertex2f ( p + 1, *tobuff - *tosdbuff - OffsetTracks );
                        }

                    glEnd();

                                        // re-draw the lines of the data, on top of the SD
                    LineColor.GLize ( 0 );

                    glBegin ( GL_LINES );

                    for ( tobuff = EegBuff[ st ] + minp, p = minp; p <= maxp; p++, tobuff++ ) {
                        glVertex2f ( p,     *tobuff - OffsetTracks );
                        glVertex2f ( p + 1, *tobuff - OffsetTracks );
                        }

                    glEnd();
                    } // SDFilledTransparent, SDFilledOpaque

                else { // if ( ShowSD == SDDashed ) {

                    glLineStipple   ( 1, 0x3333 );
                    glEnable        ( GL_LINE_STIPPLE );

                    glBegin ( GL_LINES );

                    for ( tobuff = EegBuff[ st ] + minp, tosdbuff = SdBuff[ st ] + minp, p = minp; p <= maxp; p++, tobuff++, tosdbuff++ ) {
                        glVertex2f ( p,     *tobuff - *tosdbuff - OffsetTracks );
                        glVertex2f ( p + 1, *tobuff - *tosdbuff - OffsetTracks );
                        glVertex2f ( p,     *tobuff + *tosdbuff - OffsetTracks );
                        glVertex2f ( p + 1, *tobuff + *tosdbuff - OffsetTracks );
                        }

                    glEnd();

                    glDisable ( GL_LINE_STIPPLE );
                    } // SDDashed

                } // ShowSD BarsMode


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // restoring line properties anyway
            glLineWidth         ( 1.0 );
            GLLineSmoothOff     ();
            if ( how & GLPaintLinked )
                glDisable ( GL_LINE_STIPPLE );


            if ( ! trackssuper )        // translation accounts for the scaling
                glTranslatef ( trdelta.X * descalex, trdelta.Y * descaley, trdelta.Z );


            glScaled ( 1, 1 / scalet, 1 );
            descaley    = 1 / scalev;
            } // for st


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // redraw the baseline in superimpose mode
        if (   CurrentDisplaySpace == DisplaySpaceNone 
            && ShowBaseline 
            && ! IsIntensityModes () 
            && trackssuper 
            && ( how & GLPaintOwner )                  ) {

            BackColor.GLize ();
//          glLineWidth ( 2.0 );
                                        // draw 0 axis
            glBegin ( GL_LINES );
            glVertex2f ( minp,                          0.5 / scalev );
            glVertex2f ( maxp + ( IsBarsMode () ? 1 : 0 ), 0.5 / scalev );
            glEnd();

//          glLineWidth ( 1.0 );
            glColor4f ( AxisColor );
                                        // draw 0 axis
            glBegin ( GL_LINES );
            glVertex2f ( minp,                            -0.5 / scalev );
            glVertex2f ( maxp + ( IsBarsMode () ? 0.5 : 0 ), -0.5 / scalev );
            glEnd();


                                        // draw time origin position - post tracks
            if ( draworigin ) {

                glLineWidth ( 2 );
                glColor4f ( AxisColor );

                glBegin ( GL_LINES );
                glVertex2f ( orgreltf + ( IsBarsMode () ? 0.5 : 0 ) + 1 * descalex,  4 / scalev );
                glVertex2f ( orgreltf + ( IsBarsMode () ? 0.5 : 0 ) + 1 * descalex, -4 / scalev );
                glEnd();

                BackColor.GLize ();

                glBegin ( GL_LINES );
                glVertex2f ( orgreltf + ( IsBarsMode () ? 0.5 : 0 ) - 1 * descalex,  4 / scalev );
                glVertex2f ( orgreltf + ( IsBarsMode () ? 0.5 : 0 ) - 1 * descalex, -4 / scalev );
                glEnd();
                } // draw origin - post


            glLineWidth ( 1 );
            LineColor.GLize ( 0 );
            }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // de-scale - if needed
//      glScaled ( descalex, descaley, 1 );
        st  = sel->LastSelected ();     // set to the last valid track


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // last name when summarizing
        if (   drawmargin 
            && ! roiname 
            && ( ! allnames || trackssuper ) 
            && ( how & GLPaintOwner )        ) {

            if ( !trackssuper )         // go back 1 track above
                glTranslatef ( -trdelta.X * descalex, -trdelta.Y * descaley, -trdelta.Z );
            else
                glTranslatef ( 0, - SFont->GetHeight() / 2 * descaley, 0 );

            if ( numTracks > 1 ) {
                LineColor.GLize ( 0 );
                SFont->Print ( -4 * descalex, 0, 0, GetElectrodeName ( st ), TA_RIGHT | TA_CENTERY );
                }
            }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        glPopMatrix ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // draw the horizontal grid - post
        if (   CurrentDisplaySpace == DisplaySpaceNone 
            && drawhoriscale 
            && ! IsTracksMode () 
            && ( how & GLPaintOwner ) ) {

            double              x;
            double              y1;
            double              y2;
            double              z;

            GLBlendOn ();

            glColor4f ( AxisColor );

            y1  = toslot->LowLeft.Y;
            y2  = toslot->LowLeft.Y + toslot->ToUp.Y;
            z   = toslot->LowLeft.Z;

            char    stf  [32];
            char    stime[32];

            glBegin ( GL_LINES );

            for ( double g = hgridorg - CDPt.GetMin(); g < CDPt.GetLength(); g+= hgridstep ) {
                x       = trorg.X + g * scaleh;
                glVertex3f ( x, y1, z );
                glVertex3f ( x, y2, z );
                }

            glEnd();

            for ( double g = hgridorg - CDPt.GetMin(); g < CDPt.GetLength(); g+= hgridstep ) {

                TFToString ( g + CDPt.GetMin(), stf, stime, ShowHorizScaleLegal );

                SFont->Print ( trorg.X + g / descalex + 3, y1, z, *stime ? stime : stf, TA_LEFT | TA_BOTTOM );
                }
            } // drawhoriscale - post

        } // window slots


    GLLinesModeOff      ( false );
//  glLineWidth ( 1.0 );
    } // GetNumWindowSlots ()


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // draw electrode names
if ( true && ( how & GLPaintOpaque ) ) {


        TextColor.GLize ( how & GLPaintOwner ? Outputing() ^ ( ShowTextColor == TextColorDark )? 1 : 0
                                             : Outputing() ^ ( ShowTextColor == TextColorDark )? 3 : 2 );

    float           x,  y,  z;
//  double          r;


    for ( int e = 0; e < XYZDoc->GetNumElectrodes(); e++ ) {

        if ( (bool) AnimFx && ( AnimFx.GetTimerId() == TimerToFlat || AnimFx.GetTimerId() == TimerTo3D ) ) {
            float   x1,  y1,  z1;
            float   x2,  y2,  z2;
            double  w1, w2;

            if ( AnimFx.GetTimerId() == TimerToFlat  )
                w2 = AnimFx.GetPercentageCompletion(), w1 = AnimFx.GetPercentageRemaining();
            else
                w1 = AnimFx.GetPercentageCompletion(), w2 = AnimFx.GetPercentageRemaining();

            XYZDoc->GetCoordinates ( e, x1, y1, z1, false );
            XYZDoc->GetCoordinates ( e, x2, y2, z2, true );

            x = w1 * x1 + w2 * x2;
            y = w1 * y1 + w2 * y2;
            z = w1 * z1 + w2 * z2;
            }
        else
            XYZDoc->GetCoordinates ( e, x, y, z, CurrentDisplaySpace );

        if ( CurrentDisplaySpace != DisplaySpace3D && !(bool) AnimFx ) {
            z = XYZDoc->GetProjBounding()->Radius() / 15;
/*
            if ( ShowElectrodes ) {
                                        // shift radially on the plane
                r  = sqrt ( x * x + y * y );
                if ( r == 0 ) {
                    x += glh;
                    }
                else {
                    x += (double) x / r * glh;
                    y += (double) y / r * glh * 1.2;
                    }
                }
* /
            }
        else
                                        // shift outward radially
//          x *= 1.07, y *= 1.07, z *= 1.07;
            x += ( x - ModelCenter.X ) * 0.10,
            y += ( y - ModelCenter.Y ) * 0.10,
            z += ( z - ModelCenter.Z ) * 0.10;

        if ( z == 0 )
            z = XYZDoc->GetProjBounding()->Radius() / 15;


        SFont->Print ( x, y, z, (char *) XYZDoc->GetElectrodeName (e),
                        TA_CENTER | ( CurrentDisplaySpace != DisplaySpace3D && ShowElectrodes ? TA_TOP : TA_CENTERY ) );
        }


    const TElectrodesCluster*   clu;
    float                       dx;
    float                       dy;
    float                       dz;

    if ( CurrentDisplaySpace == DisplaySpace3D )
        for ( int c = 0; c < XYZDoc->GetNumClusters(); c++ ) {
            clu = XYZDoc->GetCluster ( c );

            dx = clu->Bounding.GetXExtent();
            dy = clu->Bounding.GetYExtent();
            dz = clu->Bounding.GetZExtent();

            if      ( dx > dy && dx > dz )
                x = clu->Bounding.XMin() + dx / 2,
                y = clu->Bounding.YMin() - dx / 10,
                z = clu->Bounding.ZMin() - dx / 10;
            else if ( dy > dx && dy > dz )
                x = clu->Bounding.XMin() - dy / 10,
                y = clu->Bounding.YMin() + dy / 2,
                z = clu->Bounding.ZMin() - dy / 10;

            else if ( dz > dx && dz > dy )
                x = clu->Bounding.XMin() - dz / 10,
                y = clu->Bounding.YMin() - dz / 10,
                z = clu->Bounding.ZMin() + dz / 2;

    //        x = clu->Bounding.XMin();
    //        y = clu->Bounding.YMin();
    //        z = clu->Bounding.ZMin();

            SFont->Print ( x, y, z, (char *) clu->Name, TA_CENTER | TA_TOP );
            }

//    glDisable ( GL_COLOR_LOGIC_OP );
    }
*/


                                        // be smart: allow a window linking to this
                                        // to receive the linked window from here
                                        // allowing hierarchical links
if ( (int) Using && ( how & GLPaintOpaque ) && ! NestedPaint ) {

    NestedPaint = true;
                                        // paint the others (EEG only), in the opposite manner as usual
                                        // Substitute to own EEG pointer the others EEGs pointers, in sequence,
                                        // load data, and draw as "normal", by calling the local GLPaint (and not the remote ones)
                                        // then restore original EEG pointer and data. Not so efficient, but VERY convenient, safe and fast enough

    TTracksDoc*     EEGDocSave  = EEGDoc; // hopefully, not needed to save SD pointer, it is handled by TTracksDoc, therefore EEGDoc does it

    foreachin ( Using, iteratorview ) {

        EEGDoc  = dynamic_cast< TTracksDoc* > ( & iteratorview ()->GetDocument() );

        ReloadBuffers ();               // load all data in my buffer
                                        // then paint it from here
        GLPaint ( GLPaintLinked | GLPaintOpaque, RenderingUnchanged, clipplane );
        }
                                        // currently, transparency is not used, so avoid calling for nothing

                                        // restore our pointer and reload our data
    EEGDoc  = EEGDocSave;

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
void    TTracksView::AntialiasingPaint ()
{
if ( CurrentDisplaySpace != DisplaySpaceNone )

    TBaseView::AntialiasingPaint ();
else
                                        // Don't use the general antialiasing function when in 2D - it has already been antialiased
    NestedPaints ();
}


void    TTracksView::NestedPaints ()
{
                                        // Simply call GLPaint that will handle the Using stuff
                                        // This behavior is specific to the tracks display, which allows only linking to other tracks windows
GLPaint ( GLPaintNormal, RenderingUnchanged, ClipPlane );
}


//----------------------------------------------------------------------------
void    TTracksView::Paint ( TDC &dc, bool /*erase*/, TRect &rect )
{

if ( CurrentDisplaySpace == DisplaySpaceNone )

    PrePaint ( dc, rect, 0, 0, 0 );

else {
    const TBoundingBox<double>*     b   = XYZDoc->GetBounding ( CurrentDisplaySpace );
    double              radius          = b->MaxSize() / 2 * ( CurrentDisplaySpace == DisplaySpace3D ? ExtraSizeXyz3D * ( 1 + TracksAltitude3D ) : ExtraSize3D );

    PrePaint ( dc, rect, radius, radius, ModelRadius );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

glLightModeli ( GL_LIGHT_MODEL_TWO_SIDE, 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( CurrentDisplaySpace == DisplaySpaceNone ) {
    // already done
    }
else {
    glTranslated ( 0, 0, -ModelRadius );

    ModelRotMatrix.GLize ();

    glTranslated ( -ModelCenter.X, -ModelCenter.Y, -ModelCenter.Z );
    }

                                        // save these matrices for external drawings
MatProjection.CopyProjection ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( CurrentDisplaySpace == DisplaySpace3D ) {
    Fog.SetColor( BackColor[ Outputing() ] );
    Fog.GLize();                        // my fog
    }
else
    GLFogOff        ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

AntialiasingPaint ();


DepthRange.unGLize ();                  // reset depth range
Fog.unGLize();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // switch to window coordinates
if ( NotSmallWindow () ) {

    SetWindowCoordinates ();

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( HasColorScale () )

        ColorTable.Draw     (   PaintRect,
                                LineColor[ Outputing() ? 1 : 0 ],   TextColor[ Outputing() ? 1 : 0 ],
                                ColorMapWidth,                      ColorMapHeight,
                                GalMinValue,                        GalMaxValue,
                                true,                               ScalingAuto == ScalingAutoOff,      2,      0,
                                SFont
                            );


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
void    TTracksView::DrawTriggers ( TTracksViewWindowSlot* toslot )
{
int                 indexmin;
int                 indexmax;
                                        // retrieve range of indexes concerned with current display
if ( ! EEGDoc->TimeRangeToIndexes ( DisplayMarkerType, CDPt.GetMin (), CDPt.GetMax (), indexmin, indexmax ) )
    return;                             // nothing inside display


const MarkersList&          markers         = EEGDoc->GetMarkersList ();
const TMarker*              tomarker;
TGLCoordinates<GLfloat>&    trorg           = toslot->TrOrg;
double&                     scaleh          = toslot->ScaleH;
TGLColor<GLfloat>           col;
int                         numdraw         = 0;


GLColoringOn        ();
                                        // also, GL_BLEND is currently On

//glColor4f ( MarkerColor );

GLfloat             fromx;
GLfloat             tox;
GLfloat             lastfromx;
GLfloat             lasttox;
GLfloat             fromy;
GLfloat             toy;
GLfloat             fromz;
GLfloat             v1[ 3 ];
GLfloat             v2[ 3 ];


if ( CurrentDisplaySpace == DisplaySpaceNone ) {
    fromy       = toslot->LowLeft.Y;
    toy         = toslot->LowLeft.Y + toslot->ToUp.Y;
    fromz       = toslot->LowLeft.Z;
    }
else {
    fromy       = trorg.Y - toslot->ToUp.Norm() * 0.75;
    toy         = trorg.Y + toslot->ToUp.Norm() * 0.75;
    fromz       = trorg.Z;// + trorg.Z;    // this is before the translation of  trorg.Z  done later
    }

                                        // Use some alpha on extended triggers, full color on single TF and names (!called with GL_BLEND already enabled!)

                                        // XOR colors, useful for overlapping triggers
//glEnable ( GL_COLOR_LOGIC_OP );
//glLogicOp ( GL_OR_REVERSE );

lastfromx   = lasttox   = -1;

                                        // first pass draw the stripes
for ( int i = indexmin; i <= indexmax; i++ ) {

    tomarker   = markers[ i ];                // not the best optimized access, but it works

    if ( ! IsFlag ( tomarker->Type, DisplayMarkerType ) )
        continue;
                                        // index range can encompass smaller triggers that are not visible
    if ( tomarker->IsNotOverlappingInterval ( CDPt.GetMin (), CDPt.GetMax () ) )
        continue;

    if ( (bool) GrepMarkerFilter && ! GrepMarkerFilter.Matched ( tomarker->Name ) )
        continue;

    numdraw++;

                                        // compute & send color to OpenGL
    tomarker->ColorGLize ();


    fromx   = trorg.X + ( tomarker->From < CDPt.GetMin () ? 0
                        : tomarker->From > CDPt.GetMax () ? CDPt.GetLength ()
                        : tomarker->From - CDPt.GetMin ()                          ) * scaleh;
    tox     = trorg.X + ( tomarker->To   < CDPt.GetMin () ? 0
                        : tomarker->To   > CDPt.GetMax () ? CDPt.GetLength () - 1
                        : tomarker->To   - CDPt.GetMin () + ( OneMoreHoriz () ? 1 : 0 ) ) * scaleh; // + ( CurrentDisplaySpace == DisplaySpaceNone ? 1 : 0 );


    if ( fromx == lastfromx && tox == lasttox )

        if ( scaleh >= 1 && fromx == tox )
            fromx++, tox++;             // single TF markers are allowed to stack 1 pixel afar, if there is enough room for that
        else
            continue;                   // don't draw on top of a previous marker, that will make the coloring worse


    lastfromx   = fromx; // will not work if more than 2 triggers.. to do
    lasttox     = tox;


    if ( tomarker->IsNotExtended () && ! OneMoreHoriz () ) {
        GLBlendOff          ();
        glBegin ( GL_LINES );
        glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
        glEnd();
        GLBlendOn           ();
        }

    else { // extended

        if ( CurrentDisplaySpace == DisplaySpaceNone ) {
            glBegin ( GL_QUADS );
            glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
            glVertex3f ( tox,   toy,   fromz );     glVertex3f ( tox,   fromy, fromz );
            glEnd();

            GLBlendOff          ();
            glBegin ( GL_LINES ); // GL_LINE_LOOP
            glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
            glVertex3f ( tox,   toy,   fromz );     glVertex3f ( tox,   fromy, fromz );
            glEnd();
            GLBlendOn           ();
            }
        else { // ! DisplaySpaceNone
            v1[0] = fromx;
            v2[0] = tox;
            v1[1] = v2[1] = fromy;
            v1[2] = v2[2] = fromz;
            GLGetScreenCoord ( v1, ViewportOrgSize, MatProjection );
            GLGetScreenCoord ( v2, ViewportOrgSize, MatProjection );

            if ( v2[0] - v1[0] < 1 ) {
                GLBlendOff          ();
                glBegin ( GL_LINES );
                glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
                glEnd();
                GLBlendOn           ();
                }
            else {
                glBegin ( GL_QUADS );
                glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
                glVertex3f ( tox,   toy,   fromz );     glVertex3f ( tox,   fromy, fromz );
                glEnd();

                GLBlendOff          ();
                glBegin ( GL_LINES );
                glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
                glVertex3f ( tox,   toy,   fromz );     glVertex3f ( tox,   fromy, fromz );
                glEnd();
                GLBlendOn           ();
                }
            }
        }
    } // for i


                                        // second pass to print the names, as to be on top of the stripes

                                        // don't draw the names if too high density of names per display size
if ( (double) toslot->ToRight.Norm () >= 25 * numdraw ) {

//  LineColor.GLize ( how & GLPaintOwner ? 0 : 1 );
//  glColor4f ( MarkerColor );

                                        // text in full color, no alpha
    GLBlendOff          ();

                                                // how many names do we wish to stack vertically?
    #define             numoverlap  32
    GLfloat             maxoverlap[ numoverlap ];
    GLfloat             smarttoy;
    GLfloat             stringw;
    GLfloat             toright;

    ClearVirtualMemory ( maxoverlap, numoverlap * sizeof ( *maxoverlap ) );


    for ( int i = indexmin; i <= indexmax; i++ ) {

        tomarker   = markers[ i ];

        if ( ! IsFlag ( tomarker->Type, DisplayMarkerType ) )
            continue;
                                        // index range can encompass smaller triggers that are not visible
        if ( tomarker->IsNotOverlappingInterval ( CDPt.GetMin (), CDPt.GetMax () ) )
            continue;

        if ( (bool) GrepMarkerFilter && ! GrepMarkerFilter.Matched ( tomarker->Name ) )
            continue;

                                        // compute & send color to OpenGL
        tomarker->ColorGLize ();


        fromx   = trorg.X + ( tomarker->From < CDPt.GetMin () ? 0
                            : tomarker->From > CDPt.GetMax () ? CDPt.GetLength ()
                            : tomarker->From - CDPt.GetMin () + ( OneMoreHoriz () && tomarker->IsNotExtended () ? 1.05 : 0.05 ) ) * scaleh + 3;
        stringw = SFont->GetStringWidth ( tomarker->Name );
        tox     = fromx + stringw + 5;
        toright = toslot->LowLeft.X + toslot->ToRight.X;

                                        // find the best line to write
        int     imo;

        for ( imo = 0; imo < numoverlap; imo++ ) {

            if ( tox <  toright && maxoverlap[ imo ] <= fromx                   // regular case
              || tox >= toright && maxoverlap[ imo ] <  toright - stringw ) {   // special case when going further than border
                Maxed ( maxoverlap[ imo ], tox );
                smarttoy          = toy - imo * SFont->GetHeight ();
                break;
                }
            }
        if ( imo == numoverlap ) {
            Maxed ( maxoverlap[ 0 ], tox );
            smarttoy = toy;
            }


//      if ( tox > toslot->LowLeft.X + toslot->ToRight.X && smarttoy == toy )
//            SFont->Print ( fromx - 2, smarttoy - 2, fromz, tomarker->Name, TA_RIGHT | TA_TOP );
//        else

        if ( tox < toright )
            SFont->Print ( fromx,       smarttoy - 2, fromz, tomarker->Name, TA_LEFT | TA_TOP );
//          ( LastTagIndex == i ? BFont : SFont )->Print ( fromx,     smarttoy - 2, fromz, tomarker->Name, TA_LEFT | TA_TOP );   // draw bigger if current marker, but it needs a refresh each time the cursor jumps, which is quite costly
        else
            SFont->Print ( toright - 3, smarttoy - 2, fromz, tomarker->Name, TA_RIGHT | TA_TOP );

        } // for i

                                        // restore to current default
    GLBlendOn           ();
    } // print names


//glDisable ( GL_COLOR_LOGIC_OP );
}


//----------------------------------------------------------------------------
                                        // either draws or erases the cursor (XOR operation)
void    TTracksView::DrawTFCursor ( bool undrawold, bool localhdc )
{
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
//  if ( CurrentDisplaySpace == DisplaySpaceNone )
        glRasterPos3f ( 0, 0, 0 );
//  else
//      glRasterPos3f ( -1.5, -1.5, 0 );

//  glCopyPixels ( 0, 0, GetClientRect().Width(), GetClientRect().Height(), GL_COLOR );
    glCopyPixels ( ViewportOrgSize[0], ViewportOrgSize[1], ViewportOrgSize[2], ViewportOrgSize[3], GL_COLOR );
                                        // restore to regular setting
    glReadBuffer ( GL_BACK  );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

long                nump            = CDPt.GetLength();
long                posmin;
long                posmax;


TFCursor.GetPos ( posmin, posmax );

posmin     -= CDPt.GetMin();
posmax     -= CDPt.GetMin();
                                        // boundary check: keep inside visible part
Clipped ( posmin, (long) 0, nump - 1 );
Clipped ( posmax, (long) 0, nump - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! localhdc ) {
    GLColoringOn    ();
    GLWriteDepthOff ();
    }

glLogicOp ( GL_XOR );
glEnable  ( GL_COLOR_LOGIC_OP );

                                        // draw a hollow cursor, to not alter the colors
//if ( IsFilling == FillingColor )    glPolygonMode ( GL_FRONT_AND_BACK, GL_LINE );

//if ( localhdc && CurrentDisplaySpace != DisplaySpaceNone )
//    Fog.GLize();


if ( CurrentDisplaySpace != DisplaySpaceNone ) {
    glLoadIdentity();
                                        // redo the shift
//  glTranslated ( 0, 0, ModelRadius );
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
                                        const GLfloat&  fromx,      const GLfloat&  tox,        // OpenGL axis positions
                                        bool            thin
                                    )
    {
                                        // Same TF position, no matter how are the OpenGL positions?
    if ( posmin == posmax && ! OneMoreHoriz () ) {                                                           

        if ( thin ) {
            glBegin ( GL_LINES );
            glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
            glEnd();
            }
        else { // thick
            glBegin ( GL_QUADS );
            glVertex3f ( fromx + 1, fromy, fromz );     glVertex3f ( fromx + 1, toy,   fromz );
            glVertex3f ( fromx - 1, toy,   fromz );     glVertex3f ( fromx - 1, fromy, fromz );
            glEnd();
            }
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
  
            if ( thin ) {
                glBegin ( GL_LINES );
                glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
                glEnd();
                }
            else { // thick
                glBegin ( GL_QUADS );
                glVertex3f ( fromx + 1, fromy, fromz );     glVertex3f ( fromx + 1, toy,   fromz );
                glVertex3f ( fromx - 1, toy,   fromz );     glVertex3f ( fromx - 1, fromy, fromz );
                glEnd();
                }
            } // if less 1 voxel wide

//      else { // old fully filled red cursor - a bit too flashy
//          if ( CurrentDisplaySpace != DisplaySpaceNone || IsIntensityModes () )
//              glPolygonMode ( GL_FRONT_AND_BACK, GL_LINE );
//
//          glBegin ( GL_QUADS );
//          glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
//          glVertex3f ( tox,   toy,   fromz );     glVertex3f ( tox,   fromy, fromz );
//          glEnd ();
//
//          if ( CurrentDisplaySpace != DisplaySpaceNone || IsIntensityModes () )
//              glPolygonMode ( GL_FRONT_AND_BACK, GL_FILL );
//          }
                                        // More than a voxel wide
        else {

            if ( thin ) {
                                    // empty box outside position
                glPolygonMode ( GL_FRONT_AND_BACK, GL_LINE );

                glBegin ( GL_QUADS );
                glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
                glVertex3f ( tox,   toy,   fromz );     glVertex3f ( tox,   fromy, fromz );
                glEnd();

                glPolygonMode ( GL_FRONT_AND_BACK, GL_FILL );
                }
            else { // thick
                glBegin ( GL_QUADS );
                glVertex3f ( fromx,     fromy, fromz );     glVertex3f ( fromx,     toy,   fromz );
                glVertex3f ( fromx - 2, toy,   fromz );     glVertex3f ( fromx - 2, fromy, fromz );
                glVertex3f ( tox,       toy,   fromz );     glVertex3f ( tox,       fromy, fromz );
                glVertex3f ( tox   + 2, fromy, fromz );     glVertex3f ( tox   + 2, toy,   fromz );
                glEnd();
                }

                                        // filling box?
            if ( CurrentDisplaySpace == DisplaySpaceNone && ! IsIntensityModes ()
                && v2[ 0 ] - v1[ 0 ] > 1 /*&& IsFilling != FillingColor*/ ) {

                glColor4f ( AntiCursorFillColor );

                glBegin ( GL_QUADS );
                glVertex3f ( fromx, fromy, fromz );     glVertex3f ( fromx, toy,   fromz );
                glVertex3f ( tox,   toy,   fromz );     glVertex3f ( tox,   fromy, fromz );
                glEnd();
                } // filling box

            } // more 1 voxel wide

        } // not same TF positions

    }; // drawcursor


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TTracksViewWindowSlot*      toslot;
TGLCoordinates<GLfloat>     trorg;
double                      scaleh;
bool                        thin;


for ( int s = 0; s < GetNumWindowSlots (); s++ ) {

    toslot      = &WindowSlots[ s ];
    trorg       = toslot->TrOrg;
    scaleh      = toslot->ScaleH;

    if ( CurrentDisplaySpace == DisplaySpaceNone ) {
        fromy       = toslot->LowLeft.Y;
        toy         = toslot->LowLeft.Y + toslot->ToUp.Y;
        fromz       = toslot->LowLeft.Z;
        }
    else {
        GLfloat     cursh   = min ( toslot->ToUp.Norm(), toslot->ToRight.Norm() );

        fromy       = trorg.Y - cursh * CursorHeightFactor3D * 0.3;
        toy         = trorg.Y + cursh * CursorHeightFactor3D * 0.3;
        fromz       = trorg.Z;
                                        // skip back cursors, as it does not fog but Xor's
        if ( CurrentDisplaySpace == DisplaySpace3D && fromz >= ModelRadius )   continue;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // undraw previously drawn cursor?
    if ( undrawold && LastCursorPosMin >= 0 ) {

        fromx   = trorg.X + ( LastCursorPosMin                             ) * scaleh;
        tox     = trorg.X + ( LastCursorPosMax + ( OneMoreHoriz ()  ? 1 : 0 ) ) * scaleh; //  + ( CurrentDisplaySpace == DisplaySpaceNone && ! OneMoreHoriz () ? 1 : 0 );
        thin    = CurrentDisplaySpace != DisplaySpaceNone || OneMoreHoriz () || LastCursorPosMax != LastCursorPosMin;

        glColor4f ( AntiCursorColor );

        drawcursor ( LastCursorPosMin, LastCursorPosMax, fromx, tox, thin );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // now draw the current cursor position
    fromx   = trorg.X + ( posmin                             ) * scaleh;
    tox     = trorg.X + ( posmax + ( OneMoreHoriz ()  ? 1 : 0 ) ) * scaleh; //  + ( CurrentDisplaySpace == DisplaySpaceNone && ! OneMoreHoriz () ? 1 : 0 );
    thin    = CurrentDisplaySpace != DisplaySpaceNone || OneMoreHoriz () || posmax != posmin;

    glColor4f ( AntiCursorColor );

    drawcursor ( posmin, posmax, fromx, tox, thin );
    } // for GetNumWindowSlots ()


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // storing for the next undraw
LastCursorPosMin    = posmin;
LastCursorPosMax    = posmax;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // restore default
//if ( IsFilling == FillingColor )    glPolygonMode ( GL_FRONT_AND_BACK, GL_FILL );

glDisable ( GL_COLOR_LOGIC_OP );
GLColoringOff   ();
GLWriteDepthOn  ();


if ( localhdc ) {                       // send to front & finish

//  glFinish ();

    SwapBuffers( *DrawTFCursorDC );

    GLrc.unGLize ();

//  delete  DrawTFCursorDC;             // give back the DC?
//  DrawTFCursorDC  = 0;
    }


UpdateCaption ();
}


//----------------------------------------------------------------------------
void    TTracksView::EvKeyDown ( uint key, uint repeatCount, uint flags )
{
                                        // set common keypressed first
TBaseView::EvKeyDown ( key, repeatCount, flags );


switch ( key ) {

    case    '0':        // zero
    case    VK_NUMPAD0:
        CmGotoTimeOrigin ();
        break;

    case    'L':
        Cm2Object ();
        break;

    case    'O':
        CmOrient ();
        break;

    case    'A':
        if ( ControlKey )   CmSelectAllTime ();
        else                CmSetReference ( IDB_SETAVGREF );
        break;

    case    'C':
        CmNextColorTable ();
        break;

    case    'D':
        if      ( IsIntensityModes () )                 CmSetScalingAdapt ();
        else if ( EEGDoc->HasStandardDeviation () )     CmShowSD ();
        break;

    case    'E':
        CmAverageRois ();
        break;

    case    'F':
        if ( ControlKey )   CmSearchMarker ( CM_EEGMRKSEARCH );
        else                CmFilter ();
        break;

    case    'G':
        if ( ControlKey )   CmGotoTime ();
        else                CmMagnifier ();
        break;

    case    'H':
        CmHorizGrid ();
        ButtonGadgetSetState ( IDB_HORIZUNITS, ShowHorizScale );
        break;

    case    'I':
        if ( ControlKey )   CmNextSession ();
        else                CmSetIntensityMode ();
        break;

    case    'M':
        CmAddMarker ();
        break;

    case    'N':
        CmNextRois ();
        break;

    case    'P':
        CmFlipVert ();
        ButtonGadgetSetState ( IDB_FLIPVERT, TracksFlipVert );
        break;

    case    'R':
        if ( ControlKey )   CmRenameOverwriteMarker ();
        else                CmSetRenderingMode ();
        break;

    case    VK_NUMPAD1:
    case    VK_NUMPAD2:
    case    VK_NUMPAD3:
    case    VK_NUMPAD4:
    case    VK_NUMPAD5:
    case    VK_NUMPAD6:
    case    VK_NUMPAD7:
    case    VK_NUMPAD8:
    case    VK_NUMPAD9:

        if ( ControlKey )
            AddMarker ( key - VK_NUMPAD1 + 1);

        break;

    case    '1':
    case    '2':
    case    '3':
    case    '4':
    case    '5':
    case    '6':
    case    '7':
    case    '8':
    case    '9':

        if ( ControlKey )
            AddMarker ( key - '1' + 1);

        break;

    case    'S':
        if ( ! IsIntensityModes () )
            CmTracksSuper ();
        break;

    case    'T':
        if ( EEGDoc->GetNumMarkers ( DisplayMarkerType ) )
            CmShowTags ();
        break;

    case    'V':
        CmVertGrid ();
        ButtonGadgetSetState ( IDB_VERTUNITS, ShowVertScale );
        break;

    case 'X':
        ModelRotMatrix.RotateY ( ShiftKey ? -RotationStep : RotationStep, MultiplyLeft );
        Invalidate ( false );
        break;

    case 'Y':
        ModelRotMatrix.RotateX ( ShiftKey ? -RotationStep : RotationStep, MultiplyLeft );
        Invalidate ( false );
        break;

    case 'Z':
        ModelRotMatrix.RotateZ ( ShiftKey ? -RotationStep : RotationStep, MultiplyLeft );
        Invalidate ( false );
        break;

    case VK_RIGHT:
        if ( ShiftKey )     CmZoomHorz ( IDB_EXTTRACKSH );
        else                CmForward ( IDB_FORWARD );
        break;

    case VK_LEFT:
        if ( ShiftKey )     CmZoomHorz ( IDB_COMPTRACKSH );
        else                CmBackward ( IDB_BACKWARD );
        break;

    case VK_UP:
        if      ( ControlKey )                      CmZoomVert    ( IDB_EXTTRACKSV );
        else if ( ShiftKey && IsIntensityModes () ) CmSetContrast ( IDB_ISINCCONTRAST );
        else                                        CmShiftTracks ( IDB_UP );
        break;

    case VK_DOWN:
        if      ( ControlKey )                      CmZoomVert    ( IDB_COMPTRACKSV );
        else if ( ShiftKey && IsIntensityModes () ) CmSetContrast ( IDB_ISDECCONTRAST );
        else                                        CmShiftTracks ( IDB_DOWN );
         break;

    case VK_NEXT:
        CmPageForward ( IDB_PAGEFORWARD );
        break;

    case VK_PRIOR:
        CmPageBackward ( IDB_PAGEBACKWARD );
        break;

    case VK_HOME:
        CmPageBackward ( 0 );
        break;

    case VK_END:
        CmPageForward ( 0 );
        break;

    case VK_ADD:
        if ( ControlKey )   CmChangeNumPseudoTracks ( IDB_MOREPSEUDOTRACKS );
        else                CmChangeNumTracks ( IDB_MORETRACKS );
        break;

    case VK_SUBTRACT:
        if ( ControlKey )   CmChangeNumPseudoTracks ( IDB_LESSPSEUDOTRACKS );
        else                CmChangeNumTracks ( IDB_LESSTRACKS );
        break;

    case VK_RETURN:
        if ( CartoolDocManager->GetView ( LinkedViewId ) != 0 )
            CartoolDocManager->GetView ( LinkedViewId )->SetFocusBack ( GetViewId () );
        break;

    case VK_SPACE:
        CmRangeCursor ();
        break;

    case VK_TAB:
        if      ( SelFilling )  CmNextSegment ( ShiftKey ? CM_PREVSEGMENT : CM_NEXTSEGMENT );
        else if ( ShowTags )    CmPreviousNextMarker ( ShiftKey ? IDB_PREVMARKER : IDB_NEXTMARKER );
        break;

    case VK_F3:
        if ( ShiftKey )     CmSearchMarker ( CM_EEGMRKSEARCHPREVIOUS );
        else                CmSearchMarker ( CM_EEGMRKSEARCHNEXT     );
        break;

    case VK_DELETE:
        CmDeleteMarker ();
        break;

//  default:
//      TBaseView::EvKeyDown ( key, repeatCount, flags );
//      break;
    }
}


void    TTracksView::EvKeyUp ( uint key, uint repeatCount, uint flags )
{
                                        // reset common keypressed first
TBaseView::EvKeyUp ( key, repeatCount, flags );


switch ( key ) {

    case VK_RIGHT:
    case VK_LEFT:

        Acceleration.StopAccelerating ();

        break;

//  default:
//      TBaseView::EvKeyUp ( key, repeatCount, flags );
//      break;
    }
}


//----------------------------------------------------------------------------
void    TTracksView::EvSize ( uint sizeType, const TSize &size )
{
//int                 scrollh         = 10 * scrollh < size.Y() ? EegScrollbarHeight : 0;
int                 scrollh         = EegScrollbarHeight;

if ( size.X () != crtl::GetWindowWidth ( this ) || size.Y () != crtl::GetWindowHeight ( this ) ) {

    Attr.W  = size.X ();
    Attr.H  = size.Y ();
//  GLSize();
    }

if ( scrollh ) {

//  crtl::WindowRestore ( ScrollBar );

    crtl::WindowSetPosition ( ScrollBar, crtl::GetWindowLeft ( this ), crtl::GetWindowBottom ( this ) + 1 - scrollh, size.X (), scrollh );

    ScrollBar->RedrawWindow ( 0, 0, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE );
    }
//else
//  ::WindowHide ( ScrollBar );



TBaseView::EvSize ( sizeType, size );

SetPartWindows ( SelTracks, false );

//Invalidate ( false );
ShowNow ( RDW_UPDATENOW | RDW_INVALIDATE | RDW_NOERASE | RDW_ALLCHILDREN );

UpdateCaption ();
}


//----------------------------------------------------------------------------
void    TTracksView::EvTimer ( uint timerId )
{
TBaseView::EvTimer ( timerId );

/*
switch ( timerId ) {

//    case TimerCursor:
//        AnimCursor++;
//        DrawTFCursor ( false );
////        ShowNow ( RDW_INVALIDATE | RDW_UPDATENOW );
//        break;

    default:
        TBaseView::EvTimer ( timerId );
    }
*/
}


//----------------------------------------------------------------------------
void    TTracksView::EvLButtonDblClk ( uint, const TPoint &p )
{
//if ( LinkedViewId )
//    EEGDoc->SetFocus ( LinkedViewId );

CurrentWindowSlot = ClosestWindowSlot ( p.X (), GetClientRect ().Height () - p.Y () );

if ( ! CurrentWindowSlot )
    return;

SelTracks       = CurrentWindowSlot->SelTracks;

RenderingMode   = LayoutOnePage;

SetPartWindows ( SelTracks );
}


//----------------------------------------------------------------------------
void    TTracksView::EvLButtonDown ( uint i, const TPoint &p )
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
        if ( ! dynamic_cast<TTracksView*> ( view ) )
            return;
                                        // already pointed to?
        if ( Using.IsInside ( view ) ) {

            view->UsedBy.Remove ( this, DontDeallocate );
            Using       .Remove ( view, DontDeallocate );
            Invalidate ( false );
            }
        else {
            if ( ! UsedBy.IsInside ( view ) ) {

                TTracksDoc*     EEG2  = dynamic_cast<TTracksDoc*> ( view->BaseDoc );

                if ( EEG2
                  && EEG2->GetTotalElectrodes()   == EEGDoc->GetTotalElectrodes()
                  && EEG2->GetNumTimeFrames()     == EEGDoc->GetNumTimeFrames() ) {

//                    && EEG2->GetSamplingFrequency() == EEGDoc->GetSamplingFrequency() ) {

                    view->UsedBy.Append ( this );
                    Using.Append ( view );
                    Invalidate ( false );
                    }
                else {
                    Cm2Object ();       // finish
                    ShowMessage ( "Eeg file does not match, either by its number of electrodes,\nor its duration.", "Linking error", ShowMessageWarning );
                    }
                }
            else {
                Cm2Object ();           // finish
                ShowMessage ( "Can not create a loop of links.", "Linking error", ShowMessageWarning );
                }
            }
        }
    else {
        Cm2Object();                    // clean-up

        TListIterator<TBaseView>    iteratorview;

        foreachin ( Using, iteratorview )
            iteratorview ()->UsedBy.Remove ( this, DontDeallocate );

        Using.Reset ( DontDeallocate );
        Invalidate ( false );
        }
    }

else if ( CurrentDisplaySpace == DisplaySpaceNone && ControlKey ) {  // make an alias Ctrl-Left = Middle

    EvMButtonDown ( i, p );

    return;
    }
                                        // GL biz?
else if ( CaptureMode == CaptureGLMagnify ) {

    GLLButtonDown ( i, p );

    return;
    }

else {

    if ( CurrentDisplaySpace == DisplaySpaceNone && CaptureMode == CaptureNone ) {

        TTracksViewWindowSlot*  toslot;
        long                    tf;
        CurrentWindowSlot   = toslot    = ClosestWindowSlot ( p.X (), GetClientRect().Height () - p.Y () );

        if ( toslot ) {

            tf      = (double) ( p.X () - toslot->TrOrg.X ) / toslot->ScaleH - ( OneMoreHoriz () ? 0.5 : 0 ) + 0.5;

            if      ( tf < 0                        )   tf  = CDPt.GetMin();
            else if ( tf > (long) CDPt.GetLength()  )   tf  = CDPt.GetMax();
            else                                        tf += CDPt.GetMin();

            TFCursor.StopExtending ();
            TFCursor.SetFixedPos ( tf );

//          DrawTFCursor ( true );

            ButtonGadgetSetState ( IDB_RANGECURSOR, TFCursor.IsExtending() );

            CaptureMode         = CaptureGLTFCursor;
            }
        }

    if ( CaptureMode == CaptureNone )
        CaptureMode = CaptureLeftButton;

    LButtonDown = true;

    CaptureMouse ( Capture );

    EvMouseMove ( i, p );
    }
}


void    TTracksView::EvLButtonUp ( uint i, const TPoint &p )
{
MouseAxis           = MouseAxisNone;
CurrentWindowSlot   = 0;


if      ( CaptureMode == CaptureGLTFCursor ) {

    TFCursor.StopExtending ();

    CheckCDPtAndTFCursor ( true );
//  Invalidate ( false );

    ButtonGadgetSetState ( IDB_RANGECURSOR, TFCursor.IsExtending() );

    EEGDoc->NotifyFriendViews ( LinkedViewId, vnNewTFCursor, (TParam2) &TFCursor, this );

    CaptureMode = CaptureNone;
    }

else if ( CaptureMode == CaptureGLSelect ) {  // make an alias Ctrl-Left = Middle

    EvMButtonUp ( i, p );

    return;
    }

else if ( CaptureMode == CaptureLeftButton )

    CaptureMode = CaptureNone;

else if ( CaptureMode != CaptureNone )

    return;


LButtonDown = false;

CaptureMouse ( Release );

if ( CurrentDisplaySpace != DisplaySpaceNone )
    Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TTracksView::EvMButtonDown ( uint i, const TPoint &p )
{
if ( CaptureMode != CaptureNone )
    return;

MousePos            = p;
LastMouseMove       = TPoint ( 0, 0 );


if ( CurrentDisplaySpace == DisplaySpaceNone ) {

    CurrentWindowSlot   = ClosestWindowSlot ( p.X(), GetClientRect().Height() - p.Y() );

    LastSelectedTrack   = -1;

    if ( CurrentWindowSlot )
        CaptureMode         = CaptureGLSelect;
    else {
        CmSelection ( CM_CLEARSEL );
        return;
        }
    }


MButtonDown = true;

CaptureMouse ( Capture );

EvMouseMove ( i, p );
}


void    TTracksView::EvMButtonUp (uint, const TPoint &/*p*/ )
{
MouseAxis   = MouseAxisNone;

if      ( CaptureMode == CaptureGLSelect ) {

    LastSelectedTrack = -1;

    CaptureMode = CaptureNone;
    }

else if ( CaptureMode != CaptureNone )

    return;


MButtonDown = false;

CaptureMouse ( Release );

if ( CurrentDisplaySpace != DisplaySpaceNone )
    Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TTracksView::EvRButtonDown ( uint i, const TPoint &p )
{
MousePos            = p;
LastMouseMove       = TPoint ( 0, 0 );

if ( CaptureMode != CaptureNone )
    return;

RButtonDown = true;

CaptureMode = CaptureRightButton;

CaptureMouse ( Capture );

EvMouseMove ( i, p );
}


void    TTracksView::EvRButtonUp (uint, const TPoint &/*p*/)
{
MouseAxis   = MouseAxisNone;


if ( CaptureMode == CaptureRightButton )

    CaptureMode = CaptureNone;

else if ( CaptureMode != CaptureNone )

    return;


RButtonDown = false;
ShiftAxis   = ShiftNone;

CaptureMouse ( Release );

                                        // reset to 1 when user stops forcing the contrast
if ( EEGDoc->IsAngular ( AtomTypeUseCurrent ) )
    SetScalingContrast ( ColorTableToScalingContrast ( 1.0 ) );


//if ( CurrentDisplaySpace != DisplaySpaceNone )
    Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TTracksView::EvMouseMove ( uint i, const TPoint &p )
{

int                 dx              = p.X() - MousePos.X();
int                 dy              = p.Y() - MousePos.Y();
int                 adx             = abs ( dx );
int                 ady             = abs ( dy );


if ( RButtonDown ) {
                                        // be more sensitive
    if ( adx >= MinMouseMove / 2 + 1 || ady >= MinMouseMove / 2 + 1 ) {

                                        // trial for diagonal zooming
        if ( adx > MinMouseMove / 2 && ady > MinMouseMove / 2 && RelativeDifference ( adx, ady ) < 0.5 ) {

            MouseAxis   = MouseAxisDiagonal;

            ady     = adx   = ( min ( adx, ady ) + MinMouseMove ) / 2;  // not too fast + min speed looks smooth
            dy      = Sign ( dy ) * ady;
            dx      = Sign ( dx ) * adx;
            }

        else if ( adx > ady * ( MouseAxis == MouseAxisVertical   ? 3 : 1 ) )

            MouseAxis   = MouseAxisHorizontal;

        else if ( ady > adx * ( MouseAxis == MouseAxisHorizontal ? 3 : 1 ) )

            MouseAxis   = MouseAxisVertical;
        }

    if ( MouseAxis == MouseAxisNone )
        return;


    if ( MouseAxis == MouseAxisHorizontal
      || MouseAxis == MouseAxisDiagonal   ) {

        if ( IsIntensityModes () && IntensityOverride ) {

            if ( dx > 0 )   ScalingLevel *= 1 + min ( adx, MouseMoveScale ) / (double) MouseMoveScale * ( adx > MouseMoveScaleFast ? 1.0 : 0.2 );
            else            ScalingLevel /= 1 + min ( adx, MouseMoveScale ) / (double) MouseMoveScale * ( adx > MouseMoveScaleFast ? 1.0 : 0.2 );

            CmZoomVert ( 0 );
            }

        else if ( ShiftKey ) {

            ulong       oldlen      = CDPt.GetLength();

            if ( dx > 0 )   STH *= 1 + min ( adx, MouseMoveScale ) / (double) MouseMoveScale * ( adx > MouseMoveScaleFast ? 0.6 : 0.2 );
            else            STH /= 1 + min ( adx, MouseMoveScale ) / (double) MouseMoveScale * ( adx > MouseMoveScaleFast ? 0.6 : 0.2 );

            CmZoomHorz ( 0 );
                                        // force the change
            if ( oldlen == CDPt.GetLength() && adx > 2 )
                CmZoomHorz ( dx > 0 ? IDB_EXTTRACKSH : IDB_COMPTRACKSH );
            }

        else {
            int     deltas  = (double) adx / ( CurrentDisplaySpace == DisplaySpaceNone ? WindowSlots[0].ScaleH : ViewportOrgSize[2] * Zoom * Zoom / 10 / CDPt.GetLength() );
                                        // wait for at least 1 TF move
            if      ( deltas < 1 )  return;

            if ( WindowSlots[0].ScaleH < 1 && (double) deltas / CDPt.GetLength() > 0.20 )
                deltas = CDPt.GetLength() * min ( 0.666, (double) adx / MouseMoveScaleFast * 0.25 );


            ScrollDisplay ( dx > 0 ? -deltas : deltas );
/*
            MarginVariables;
            int     deltas  = CDPt.GetLength() * (double) dx / ( CurrentDisplaySpace == DisplaySpaceNone ? WindowSlots[0].ToRight.X - margin : ViewportOrgSize[2] * Zoom * Zoom / 10 );

            if      ( abs ( deltas ) < 1 )
                if  ( dx > 0 )  deltas =  1;
                else            deltas = -1;
            else if ( abs ( deltas ) > CDPt.GetLimitLength() * 0.10 )
                if  ( dx > 0 )  deltas =  CDPt.GetLimitLength() * 0.10 + 1;
                else            deltas = -CDPt.GetLimitLength() * 0.10 - 1;

            if ( adx > 30 )                 // super fast scroll?
                deltas = CDPt.GetLength() * 0.15 * ( dx > 0 ? 1 : -1 );
*/
            }
        }

    /*else*/ if ( MouseAxis == MouseAxisVertical 
               || MouseAxis == MouseAxisDiagonal ) {

        if ( IsIntensityModes () && IntensityOverride ) {

            if ( dy > 0 )
                ScalingContrast    *= 1 - (double) dy / 30;
            else
                ScalingContrast     = AtLeast ( 0.01, ScalingContrast ) * ( 1 - (double) dy / 30 );

            SetScalingContrast ( ScalingContrast );

            ShowNow ();
            }

        else if ( ShiftKey ) {

            if ( ControlKey && (bool) Highlighted ) {
                                            // adjust the relative scaling of the highlighted tracks only
                for ( int e = 0; e < EEGDoc->GetTotalElectrodes (); e++ ) {

                    if ( ! Highlighted[ e ] )  continue;

                    if ( dy > 0 )   ScaleTracks[ e ] /= 1 + min ( ady, MouseMoveScale ) / (double) MouseMoveScale * 0.3;
                    else            ScaleTracks[ e ] *= 1 + min ( ady, MouseMoveScale ) / (double) MouseMoveScale * 0.3;
                    }

                //Invalidate ( false );
                ShowNow ();
                } // ControlKey
            else { // ! ControlKey
                                            // adjust global scaling
                double  osl     = ScalingLevel;

                if ( dy > 0 )   ScalingLevel /= 1 + min ( ady, MouseMoveScale ) / (double) MouseMoveScale * ( ady > MouseMoveScaleFast ? 1.0 : 0.3 );
                else            ScalingLevel *= 1 + min ( ady, MouseMoveScale ) / (double) MouseMoveScale * ( ady > MouseMoveScaleFast ? 1.0 : 0.3 );

                CmZoomVert ( 0 );
                } // ! ControlKey
            } // ShiftKey

        else if ( ControlKey ) { // && ! ShiftKey
                                            // adjust offset
//          double          scalev          = CurrentDisplaySpace == DisplaySpaceNone ? ScalingLevel : ScalingLevel * XYZDoc->GetBounding( CurrentDisplaySpace )->Radius() / 300;
            double          scalev          = ScalingLevel;
            scalev                         *= ScaleTracks[ 0 ];

            if ( dy > 0 )   OffsetTracks   += min ( ady, MouseMoveScale ) / (double) MouseMoveScale * ( ady > MouseMoveScaleFast ? 75 : 25 ) / scalev * ( TracksFlipVert ? -1 : 1 );
            else            OffsetTracks   -= min ( ady, MouseMoveScale ) / (double) MouseMoveScale * ( ady > MouseMoveScaleFast ? 75 : 25 ) / scalev * ( TracksFlipVert ? -1 : 1 );

            //Invalidate ( false );
            ShowNow ();
            RefreshLinkedWindow () ;
            } // ControlKey

        else { // ! ShiftKey && ! ControlKey

            if ( ady > MinMouseMove ) {

                if ( IsRoiMode () ) {

                    if ( ady > 35 )
                        ShiftTracks ( dy > 0 ? IDB_UP : IDB_DOWN, 1 );
                    else
                        return; // don't update MousePos
                    }
                else
                    if ( ady < 25 )
                        ShiftTracks ( dy > 0 ? IDB_UP : IDB_DOWN, 1 );
                    else if ( ady < 30 )
                        ShiftTracks ( dy > 0 ? IDB_UP : IDB_DOWN, 1 + min ( ady, MouseMoveScale ) / (double) MouseMoveScale * 10 );
                    else
                        ShiftTracks ( dy > 0 ? IDB_UP : IDB_DOWN, 1 + min ( ady, MouseMoveScale ) / (double) MouseMoveScale * (double) EEGDoc->GetNumElectrodes() / 50 );
                }
            else
                return; // don't update MousePos
            }
        }

//    Invalidate ( false );

//    MousePos   = p;
    }


else if ( LButtonDown && CurrentDisplaySpace == DisplaySpaceNone && ! ControlKey ) {

    if ( CaptureMode == CaptureGLTFCursor && CurrentWindowSlot ) {
        const TTracksViewWindowSlot*    toslot      = CurrentWindowSlot;
        int                             tf          = (double) ( p.X() - toslot->TrOrg.X ) / toslot->ScaleH - ( OneMoreHoriz () ? 0.5 : 0 ) + 0.5 + CDPt.GetMin();
        long                            otf         = TFCursor.GetExtendingPos ();


        TFCursor.SetExtendingPos ( tf );

//        CheckCDPtAndTFCursor ( true );

        UpdateCaption ();
                                    // if out of window, scroll according to the missing part
        if      ( tf < CDPt.GetMin() ) {
            if ( tf < otf )
                ScrollDisplay ( tf - CDPt.GetMin() );
            }
        else if ( tf > CDPt.GetMax() ) {
            if ( tf > otf )
                ScrollDisplay ( tf - CDPt.GetMax() );
            }
        else                        // just redraw the cursor
            DrawTFCursor ( true );
        }
    } // CurrentDisplaySpace == DisplaySpaceNone


else if ( CurrentDisplaySpace == DisplaySpaceNone && MButtonDown
       || LButtonDown && CurrentDisplaySpace == DisplaySpaceNone && ControlKey ) {   // track selection

    if ( CaptureMode == CaptureGLSelect ) {
                                        // smart update of current slot
        TTracksViewWindowSlot*  toslot      = CurrentWindowSlot;
        CurrentWindowSlot   = ClosestWindowSlot ( p.X(), GetClientRect().Height() - p.Y() );

        if ( CurrentWindowSlot == 0 )
            CurrentWindowSlot = toslot;
/*
        if ( CurrentWindowSlot == 0 ) {
            CurrentWindowSlot = toslot;
            CmSelection ( CM_CLEARSEL );
            return;
            }
*/
        double              pos;
        int                 index;
        bool                doingbads   = ShiftKey;
        TSelection&         chg         = doingbads ? BadTracks : Highlighted;  // which selection do we address?


        if ( toslot->TrDelta.Y == 0 )   // no shift -> superimposed -> take all tracks

            chg ^= toslot->SelTracks;

        else {
                                        // visual position of closest tracks
            pos     = (double) ( GetClientRect().Height() - p.Y() - toslot->TrOrg.Y ) / toslot->TrDelta.Y + 0.5;
                                        // translated back to the real track index
            index   = pos < 0 ? -1 : toslot->SelTracks.GetValue ( (int) pos );


            if ( index < 0 ) {                  // too far from tracks?

                if ( LastSelectedTrack == -1 )  // clear selection only if not already selecting (don't lose the job!)
                    CmSelection ( CM_CLEARSEL );
                return;
                }


/*           if ( index != LastSelectedTrack ) {
                if ( chg[ index ] )
                    if ( LastSelectedTrack == -1 )  chg.Reset ( index );
                    else                            chg.Reset ( LastSelectedTrack, index );
                else
                    if ( LastSelectedTrack == -1 )  chg.Set   ( index );
                    else                            chg.Set   ( LastSelectedTrack, index );
                }
            LastSelectedTrack = index;
*/

                                        // this one accounts for template order of toslot
            if ( (int) pos != LastSelectedTrack ) {

                if ( LastSelectedTrack == -1 )

                    chg.Set ( index, ! chg[ index ] );

                else {

                    if ( LastSelectedTrack <= (int) pos )

                        for ( int value = LastSelectedTrack; value <= (int) pos; value++ )
                            chg.Set ( toslot->SelTracks.GetValue ( value ), ! chg[ index ] );
                    else

                        for ( int value = LastSelectedTrack; value >= (int) pos; value-- )
                            chg.Set ( toslot->SelTracks.GetValue ( value ), ! chg[ index ] );
                    }
                }


            LastSelectedTrack = index == -1 ? -1 : (int) pos;

            if ( doingbads )
                EEGDoc->ClearPseudo ( BadTracks );
            }


        if ( doingbads ) {
            EEGDoc->SetBadTracks ( &BadTracks );

//          CmSelection ( CM_BADEXCLUDE );      // remove from display, too

            ResetScaleTracks ();
            }
        else
//          EEGDoc->NotifyFriendViews ( ParentView->LinkedViewId, vnNewHighlighted, (TParam2) Highlighted, this );
            EEGDoc->NotifyDocViews ( vnNewHighlighted, (TParam2) &Highlighted, this );


        ShowNow ();
//      MousePos   = p;
        }
    }

else                                    // regular 3D handling
    GLMouseMove ( i, p );


LastMouseMove       = TPoint ( dx, dy );
MousePos            = p;
}


void    TTracksView::EvDropFiles ( TDropInfo drop )
{
TGoF                markerfiles     ( drop, AllMarkersFilesExt );
TGoF                remainingfiles  ( drop, AllMarkersFilesExt, 0, true );

drop.DragFinish ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int i = 0; i < (int) markerfiles; i++ ) {

    char                buff    [ 256 ];
    TFileName           filename;

    StringCopy  ( filename, markerfiles[ i ] );
    GetFilename ( filename );
    StringCopy  ( buff, "Do you want to add the markers from file:" NewLine NewLine Tab, filename );

    if ( GetAnswerFromUser ( buff, "Adding Markers" ) )
        AddFileMarkers ( markerfiles[ i ] );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // transmit all remaining files to the app
CartoolApplication->OpenDroppedFiles ( remainingfiles );
}


//----------------------------------------------------------------------------
                                        // modKeys: MK_CONTROL, MK_SHIFT, MK_LBUTTON etc...
void    TTracksView::EvMouseWheel ( uint modKeys, int zDelta, const TPoint& )
{
                                        // simultaneous horizontal and vertical zoom
if ( ControlKey && ShiftKey ) {
                                        // code is a remix of CmZoomHorz & CmZoomVert
    double              oldSTH          = STH;
    ulong               oldlength       = CDPt.GetLength ();
    long                oldtfmin        = CDPt.GetMin ();
    long                oldtfmax        = CDPt.GetMax ();
    ulong               mintf           = oldlength < DisplayMinTF ? 2 : DisplayMinTF;
    double              sthmin          = 1.0;
    double              sthmax          = CDPt.GetLimitLength ();

                                        // focus on horiztontal zoom first
    if ( zDelta > 0 )   STH            *= SqrtTwo;
    else                STH            /= SqrtTwo;

    Clipped ( STH, sthmin, sthmax );

                                            // force an odd interval, which remain nicely centered on cursor, except when zoom maxed in
    ulong               oddtf           = mintf <= 1 || oldlength <= 11 ? 0 : 1;

    ulong               newlength       = ( (int) ( ( CDPt.GetLimitLength () - mintf ) / STH + mintf ) ) | oddtf;

                                            // recheck limits
    if      ( newlength > CDPt.GetLimitLength () )      newlength = CDPt.GetLimitLength ();
    else if ( newlength < mintf                  )      newlength = mintf | oddtf;

                                        // leave now if we are already zoomed to the max
    if ( newlength /*CDPt.GetLength ()*/ == oldlength ) {

        STH     = oldSTH;
        return;
        }

                                        // don't update vertical scaling if in intensity mode
    if ( ! IsIntensityModes ()  ) {

        if ( zDelta > 0 )   ScalingLevel   *= SqrtTwo;
        else                ScalingLevel   /= SqrtTwo;

        SetScaling ( ScalingLevel );
        }

                                            // modify CDPt
    CDPt.SetLength ( newlength, TFCursor.GetPosMiddle (), true );
                                            // contaminate TFCursor
    CheckCDPtAndTFCursor ( false );

    RefreshLinkedWindow ();

    UpdateBuffers ( oldtfmin, oldtfmax, CDPt.GetMin (), CDPt.GetMax () );
                                            // ?Not sure if really needed?
    //EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_EEG );

    //Invalidate ( false );
    ShowNow ();

    ScrollBar->UpdateIt ();
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // vertical zoom / brightness increase
else if ( ControlKey && ! ShiftKey ) {

    if ( zDelta > 0 )   ScalingLevel   *= SqrtTwo;
    else                ScalingLevel   /= SqrtTwo;

    CmZoomVert ( 0 );
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // horizontal zoom
else if ( ! ControlKey && ShiftKey ) {

    ulong               oldlength       = CDPt.GetLength ();

    if ( zDelta > 0 )   STH            *= SqrtTwo;
    else                STH            /= SqrtTwo;

    CmZoomHorz ( 0 );
    }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // panning
else {

/*                                        // using our own Acceleration object?
    if ( Acceleration.GetTimeSinceLastPolled () > 1 )
        Acceleration.StopAccelerating ();

    Acceleration.DelayForPolling            = 250;
    Acceleration.DelayBeforeAcceleration    = 500;
    Acceleration.DelayAfterNoAcceleration   = 1000;
    Acceleration.MinSpeed                   = AtLeast ( (ulong) EEGGLVIEW_MAXSPEEDMIN, CDPt.GetLength() / 30 );
    Acceleration.MaxSpeed                   = AtLeast ( (ulong) EEGGLVIEW_MAXSPEEDMIN, CDPt.GetLength() /  2 );
    Acceleration.MinAcceleration            = 1;

    int                 scrolling       = Acceleration.GetSpeed () * Sign ( zDelta );
*/

                                        // borrowing the scroll bar own acceleration object, which kind of makes sense as it is aimed at scrolling
    if ( ScrollBar->GetTimeSinceLastPolled () > 1 )
        ScrollBar->StopAccelerating ();

                                        // except we set different acceleration parameters
    ScrollBar->DelayForPolling          = TracksScrollWheelDelayForPolling;
    ScrollBar->DelayBeforeAcceleration  = TracksScrollWheelDelayBeforeAcceleration;
    ScrollBar->DelayAfterNoAcceleration = TracksScrollWheelDelayAfterNoAcceleration;
    ScrollBar->MinSpeed                 = AtLeast ( (ulong) EEGGLVIEW_MAXSPEEDMIN, CDPt.GetLength() / 20 );
    ScrollBar->MaxSpeed                 = AtLeast ( (ulong) EEGGLVIEW_MAXSPEEDMIN, CDPt.GetLength() );
    ScrollBar->MinAcceleration          = TracksScrollWheelMinAcceleration;


    int                 scrolling       = ScrollBar->GetSpeed () * - Sign ( zDelta );

    ScrollDisplay ( scrolling );
    }
}


//----------------------------------------------------------------------------
void    TTracksView::HintsPaint ()
{
                                        // draw additional user interaction hints
if ( LButtonDown || MButtonDown || RButtonDown ) {
//   && ( abs ( LastMouseMove.X() ) + abs ( LastMouseMove.Y() ) ) > 10 ) {


    SetWindowCoordinates ();

    GLBlendOn           ();

    glColor4f           ( GLBASE_CURSORHINTCOLOR );
    BFont->SetBoxColor  ( HintTextBackColor );

//    glTranslated ( PaintRect.Width() / 2, PaintRect.Height() / 2, 0 );
    glTranslated    ( MousePos.X(), PaintRect.Height() - MousePos.Y(), 0 );
    glScaled        ( CursorHintSize, CursorHintSize, 1 );


    if ( RButtonDown ) {
        if ( MouseAxis == MouseAxisHorizontal && LastMouseMove.X() ) {

            if ( IsIntensityModes () && IntensityOverride ) {
/*
                char    buff[ 32 ];
                                        // brightness
                sprintf ( buff, "%g   %g", ScalingNMax, ScalingPMax );
//              sprintf ( buff, "%g   %g", ColorTable.GetNMax(), ColorTable.GetPMax() ); // this one accurate for EEG, but not Freqs

                BFont->Print ( 0, -2, 1, buff, TA_TOP | TA_CENTER | TA_BOX );
*/
                Prim.DrawBrightness ( 0, 0, 0, 1 );

                if      ( LastMouseMove.X() < 0 )
                    Prim.DrawRectangle ( -2.50, 0.00, 0, 0.50, 0.12, 0 );
                else if ( LastMouseMove.X() > 0 )
                    Prim.DrawPlus (  2.50, 0.00, 0, 0.50 );
                }

            else if ( ShiftKey ) {

                if ( LastMouseMove.X() > 0 ) {
                                        // horizontal expand
                    Prim.DrawArrow (  0.50, 0, 0, 2.00, 0.25, 1.00, 0.50,   0 );
                    Prim.DrawTriangle ( -1.00,  0.00, 0, 180 );
                    }
                else {
                                        // horizontal contract
                    Prim.DrawArrow ( -1.00, 0, 0, 1.00, 0.25, 1.00, 0.50,   0 );
                    Prim.DrawArrow (  1.00, 0, 0, 1.00, 0.25, 1.00, 0.50, 180 );
                    }
                }
            else {
                                        // horizontal scrolling
                if ( LastMouseMove.X() > 0 ) {
                    BFont->Print ( 0.50, -1, 0, "Time", TA_TOP | TA_CENTER | TA_BOX );
                    Prim.DrawArrow (  0.50, 0, 0, 2.00, 0.25, 1.00, 0.50,   0 );
                    }
                else {
                    BFont->Print ( -0.50, -1, 0, "Time", TA_TOP | TA_CENTER | TA_BOX );
                    Prim.DrawArrow ( -0.50, 0, 0, 2.00, 0.25, 1.00, 0.50, 180 );
                    }
                }
            } // MouseAxisHorizontal


        else if ( MouseAxis == MouseAxisVertical && LastMouseMove.Y() ) {

            if ( IsIntensityModes () && IntensityOverride ) {
                char    buff[ 32 ];
                                        // contrast
//                sprintf ( buff, "%g", ScalingContrast );
                sprintf ( buff, "%g", ColorTable.GetContrast () );
                BFont->Print ( 2, 0, 1, buff, TA_LEFT | TA_CENTERY | TA_BOX );

                Prim.DrawContrast ( 0, 0, 0, 1 );

                if      ( LastMouseMove.Y() > 0 )
                    Prim.DrawRectangle ( 0.00, -1.50, 0, 0.50, 0.12, 0 );
                else if ( LastMouseMove.Y() < 0 )
                    Prim.DrawPlus (  0.00, 1.75, 0, 0.50 );
                }

            else if ( ShiftKey ) {

                if ( LastMouseMove.Y() < 0 ) {
                                        // vertical expand
                    Prim.DrawArrow ( 0,  0.50, 0, 2.00, 0.25, 1.00, 0.50,  90 );
                    Prim.DrawTriangle (  0.00, -1.00, 0, -90 );
                    }
                else {
                                        // vertical contract
                    Prim.DrawArrow ( 0, -1.00, 0, 1.00, 0.25, 1.00, 0.50,  90 );
                    Prim.DrawArrow ( 0,  1.00, 0, 1.00, 0.25, 1.00, 0.50, -90 );
                    }
                }

            else if ( ControlKey ) {
                                        // offset shifting
//                BFont->Print ( 1, 0, 1, "Offset", TA_LEFT | TA_CENTERY | TA_BOX );

                char    buff[ 32 ];
                sprintf ( buff, "Offset\n%g", OffsetTracks );
                BFont->Print ( 2, 0, 1, buff, TA_LEFT | TA_CENTERY | TA_BOX );

                Prim.DrawRectangle ( 0, 0, 0, 0.50, 0.25,   0 );

                if ( LastMouseMove.Y() < 0 )
                    Prim.DrawArrow ( 0,  1.12, 0, 0.75, 0.25, 1.00, 0.50,  90 );
                else
                    Prim.DrawArrow ( 0, -1.12, 0, 0.75, 0.25, 1.00, 0.50, -90 );
                }

            else {
                                        // vertical scrolling
                char    buff[ 64 ];
                int     nt = SelTracks.NumSet ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () );

                ClearString ( buff );

                if ( IsRoiMode () ) {
                    if      ( Rois->NumSet () >  1 )
                        sprintf ( buff, "ROIs\n%s - %s",    Rois->GetRoiName ( Rois->GetRoisSelected ()->FirstSelected () ),
                                                            Rois->GetRoiName ( Rois->GetRoisSelected ()->LastSelected  () )  );
                    else if ( Rois->NumSet () == 1 )
                        sprintf ( buff, "ROI\n%s",          Rois->GetRoiName ( Rois->GetRoisSelected ()->FirstSelected () ) );
                    }
                else {
                    if      ( nt >  1 )
                        sprintf ( buff, "Tracks\n%s - %s",  GetElectrodeName ( SelTracks.FirstSelected ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () ) ),
                                                            GetElectrodeName ( SelTracks.LastSelected  ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () ) ) );
                    else if ( nt == 1 )
                        sprintf ( buff, "Track\n%s",        GetElectrodeName ( SelTracks.FirstSelected ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () ) ) );
                    }


                if ( LastMouseMove.Y () < 0 ) {
                    BFont->Print ( 1, 0.50, 0, buff, TA_LEFT | TA_CENTERY | TA_BOX );
                    Prim.DrawArrow ( 0,  0.50, 0, 2.00, 0.25, 1.00, 0.50,  90 );
                    }
                else {
                    BFont->Print ( 1, 0.0, 0, buff, TA_LEFT | TA_CENTERY | TA_BOX );
                    Prim.DrawArrow ( 0, -0.50, 0, 2.00, 0.25, 1.00, 0.50, -90 );
                    }
                }
            } // MouseAxisVertical

        } // RButtonDown


    else if ( CurrentDisplaySpace != DisplaySpaceNone ) {
        GLBlendOff          ();
        ResetWindowCoordinates ();

        TBaseView::HintsPaint ();

        return;
        } // 3D stuff


    GLBlendOff          ();
    ResetWindowCoordinates ();          // this will also restore a clean ModelView
    }

}


//----------------------------------------------------------------------------
void    TTracksView::ReloadXyz ()
{
XYZDoc              = 0;
CurrXyz             = 0;

if ( ! ( GODoc && GODoc->GetNumXyzDoc () && ! EEGDoc->IsContentType ( ContentTypeRis ) ) )
    return;                             // should't happen in fact

                                        // try to read the actual linked potential maps
TPotentialsView*    potmap          = dynamic_cast< TPotentialsView * > ( CartoolDocManager->GetView ( LinkedViewId ) );


if ( potmap ) {
    CurrXyz = potmap->GetCurrXyz ();
    XYZDoc  = GODoc->GetXyzDoc ( CurrXyz );

//  DBGV2 ( CurrXyz + 1, XYZDoc->GetMinDistance ( DisplaySpace3D ), "currXYZ  midist" );
    }
else
    XYZDoc  = GODoc->GetXyzDoc ( CurrXyz );


SetTextMargin ();
}


void    TTracksView::ReloadSp ()
{
SPDoc               = 0;
CurrSp              = 0;

if ( ! ( GODoc && GODoc->GetNumSpDoc () && EEGDoc->IsContentType ( ContentTypeRis ) ) )
    return;                             // should't happen in fact

                                        // try to read the actual linked potential maps
//TInverseView*       isview          = dynamic_cast< TInverseView * > ( CartoolDocManager->GetView ( LinkedViewId ) );
//
//
//if ( isview ) {
//    CurrSp  = isview->GetCurrSp ();
//    SPDoc   = GODoc->GetSpDoc ( CurrSp );
//    }
//else
                                        // currently reloading only the first SPDoc (if any) - !user shouldn't be able to play with multiple SP files!
    SPDoc   = GODoc->GetSpDoc ( CurrSp );


SetTextMargin ();
}


void    TTracksView::ReloadRoi ()
{
if ( ! IsRoiMode () )
    return;                             // but don't change anything

                                        // this in contrary is weird
if ( ! ( GODoc && GODoc->GetNumRoiDoc () ) ) {
    ClearRoiMode ();

    if ( Rois ) delete Rois, Rois = 0;
    CurrRois    = 0;
    return;
    }

Rois    = new TRois ( (char *) GODoc->GetRoisDoc ( CurrRois )->GetDocPath () );

Rois->Set ();

Rois->CumulateInto ( SelTracks, EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () );
}


void    TTracksView::InitXyz ()
{
if ( CurrentDisplaySpace == DisplaySpaceNone || ! XYZDoc )
    return;


const TBoundingBox<double>*     b   = XYZDoc->GetBounding ( CurrentDisplaySpace );


ModelCenter = b->GetCenter ();
                                       // take the radius of the whole data box
ModelRadius = DepthPositionRatio * b->AbsRadius ();



Fog         = TGLFog<GLfloat>   ( ModelRadius * FogXyzNear,
                                  ModelRadius * FogXyzFar,
                                  GLBASE_FOGCOLOR_NORMAL );
}

/*
void    TTracksView::BestXYZBox ()
{
if ( CurrentDisplaySpace == DisplaySpaceNone || ! XYZDoc )
    return;


if ( (bool) Montage ) {
                                        // need to expand the montage to have correct bounds
    TSelection  msel ( EEGDoc->GetTotalElectrodes () );
    msel.Reset ();

    for ( int e = 0; e < EEGDoc->GetNumElectrodes (); e++ )

        if ( (bool) Montage[ e ] ) {

            msel.Set ( Montage[ e ].El1 );
            msel.Set ( Montage[ e ].El2 );
            }

    BoundingElectrodes.Set ( XYZDoc->GetPoints ( DisplaySpaceProjected ), EEGDoc->GetNumElectrodes(), &msel );
    }
else
    BoundingElectrodes.Set ( XYZDoc->GetPoints ( DisplaySpaceProjected ), EEGDoc->GetNumElectrodes(), &SelTracks );



TPointFloat    *top     = XYZDoc->GetPoints ( DisplaySpaceProjected );
double          dx, dy;
double          dminx   = DBL_MAX;
double          dminy   = DBL_MAX;

double          ex, ey;
double          fx, fy;


for ( int e = 0; e < EEGDoc->GetNumElectrodes(); e++ ) {

    if ( ! SelTracks[ e ] ) continue;

    if ( (bool) Montage && (bool) Montage[ e ] ) {

        ex = ( top[ Montage[ e ].El1 ].x + top[ Montage[ e ].El2 ].x ) / 2;
        ey = ( top[ Montage[ e ].El1 ].y + top[ Montage[ e ].El2 ].y ) / 2;
        }
    else {
        ex = top[ e ].x;
        ey = top[ e ].y;
        }

    for ( int f = e + 1; f < EEGDoc->GetNumElectrodes(); f++ ) {

        if ( ! SelTracks[ f ] ) continue;

        if ( (bool) Montage && (bool) Montage[ f ] ) {

            fx = ( top[ Montage[ f ].El1 ].x + top[ Montage[ f ].El2 ].x ) / 2;
            fy = ( top[ Montage[ f ].El1 ].y + top[ Montage[ f ].El2 ].y ) / 2;
            }
        else {
            fx = top[ f ].x;
            fy = top[ f ].y;
            }

        dx  = fabs ( ey - fy );
        dy  = fabs ( ex - fx );
        if ( dx < dy ) {            // take the infinite norm, that is the max
            if ( dy < dminy )   dminy   = dy;   // test separately the min distances
            }
        else if ( dy < dx ) {
            if ( dx < dminx )   dminx   = dx;
            }
        else {
            if ( dx < dminx )   dminx   = dx;
            if ( dy < dminy )   dminy   = dy;
            }
        }
    }

if      ( dminx == DBL_MAX && dminy == DBL_MAX ) {
    SplitPartSize.x = BoundingElectrodes.Width();
    SplitPartSize.y = BoundingElectrodes.Height();
    }
else if ( dminx == DBL_MAX ) {
    SplitPartSize.x = dminx;
    SplitPartSize.y = BoundingElectrodes.Height();
    }
else if ( dminy == DBL_MAX ) {
    SplitPartSize.x = BoundingElectrodes.Width();
    SplitPartSize.y = dminy;
    }
else {
    SplitPartSize.y = dminx;
    SplitPartSize.x = dminy;
    }
}
*/

void    TTracksView::BestXYZBox ( double &w, double &h )
{
if ( CurrentDisplaySpace == DisplaySpaceNone || ! XYZDoc )
    return;


TPoints             pointsorig;
TPoints             points;
int                 numpoints;

double              ex;
double              ey;
double              fx;
double              fy;
double              dx;
double              dy;
double              dminx           = DBL_MAX;
double              dminy           = DBL_MAX;

pointsorig  = XYZDoc->GetPoints ( DisplaySpaceProjected );

                                        // create the exact list of points according to each case
if ( IsRoiMode () ) {

    numpoints   = Rois->GetRoisSelected ()->NumSet ();
    points.Resize ( numpoints );

    TSelection  *roisel;

                                        // scan each roi
    for ( TIteratorSelectedForward roii ( *Rois->GetRoisSelected () ); (bool) roii; ++roii ) {

        roisel = & (*Rois)[ roii() ].Selection;

        ex  = ey = 0;
                                        // average position of current roi
        for ( TIteratorSelectedForward i ( *roisel ); (bool) i; ++i ) {

            ex  += pointsorig[ i() ].X;
            ey  += pointsorig[ i() ].Y;
            }

        points[ roii.GetIndex () ].X = ex / ( roisel->NumSet () ? roisel->NumSet () : 1 );
        points[ roii.GetIndex () ].Y = ey / ( roisel->NumSet () ? roisel->NumSet () : 1 );
        }
    }

else if ( (bool) Montage ) {

    numpoints   = 0;

    for ( TIteratorSelectedForward sti ( SelTracks ); (bool) sti; ++sti )
        if ( (bool) Montage[ sti() ] )
            numpoints++;

    points.Resize ( numpoints );


    for ( TIteratorSelectedForward sti ( SelTracks ); (bool) sti; ++sti ) {

        if ( ! (bool) Montage[ sti() ] )
            continue;

        points[ sti.GetIndex () ].X = ( pointsorig[ Montage[ sti() ].El1 ].X + pointsorig[ Montage[ sti() ].El2 ].X ) / 2;
        points[ sti.GetIndex () ].Y = ( pointsorig[ Montage[ sti() ].El1 ].Y + pointsorig[ Montage[ sti() ].El2 ].Y ) / 2;
        }
    }

else { // regular case
    numpoints   = EEGDoc->GetNumSelectedRegular ( SelTracks );
    points.Resize ( numpoints );

    int     e   = 0;
    for ( TIteratorSelectedForward sti ( SelTracks ); (bool) sti; ++sti )
        if ( sti() <= EEGDoc->GetLastRegularIndex () )
            points[ e++ ] = pointsorig[ sti() ];
    }


TBoundingBox<double>    boundingelectrodes ( points );


for ( int e = 0; e < numpoints; e++ ) {

    ex  = points[ e ].X;
    ey  = points[ e ].Y;

    for ( int f = e + 1; f < numpoints; f++ ) {

        fx  = points[ f ].X;
        fy  = points[ f ].Y;

        dx  = fabs ( ey - fy );
        dy  = fabs ( ex - fx );

        if ( dx < dy ) {                // take the infinite norm, that is the max
            if ( dy < dminy )   dminy   = dy;   // test separately the min distances
            }
        else if ( dy < dx ) {
            if ( dx < dminx )   dminx   = dx;
            }
        else {
            if ( dx < dminx )   dminx   = dx;
            if ( dy < dminy )   dminy   = dy;
            }
        }
    }


if ( IsRoiMode () ) {                      // rois are already quite big
    dminx   *= 0.95;
    dminy   *= 0.95;
    }


if      ( dminx == DBL_MAX && dminy == DBL_MAX ) {
    w   = boundingelectrodes.GetXExtent ();
    h   = boundingelectrodes.GetYExtent ();
    }
else if ( dminy == DBL_MAX ) {
    w   = dminx;
    h   = boundingelectrodes.GetYExtent ();
    }
else if ( dminx == DBL_MAX ) {
    w   = boundingelectrodes.GetXExtent ();
    h   = dminy;
    }
else {
    w   = dminx;
    h   = dminy;
    }

                                        // don't dare replying 0 as we shall fear the wrath of the evil division by 0
if      ( w == 0 && h == 0 )    w   = h   = 1;
else if ( w == 0           )    w   = h;
else if (           h == 0 )    h   = w;
}


//----------------------------------------------------------------------------
bool    TTracksView::VnReloadData ( int what )
{
switch ( what ) {

  case EV_VN_RELOADDATA_EEG:

    ReloadBuffers ();

//  ResetScaleTracks (); // seems to interfere when averaging...
    Invalidate ( false );
    break;


  case EV_VN_RELOADDATA_REF:

    ReloadBuffers ();

    ButtonGadgetSetState ( IDB_SETAVGREF, EEGDoc->GetReferenceType () == ReferenceAverage );
    break;


  case EV_VN_RELOADDATA_FILTERS:

    ButtonGadgetSetState ( IDB_FILTER, EEGDoc->AreFiltersActivated () );
    break;


  case EV_VN_RELOADDATA_TRG:            // if we receive this event from outside
                                        // then force to show up the markers
    ShowTags   = ! EEGDoc->GetNumMarkers ( DisplayMarkerType );
    if ( ! ShowTags )
        CmShowTags ();

    Invalidate ( false );
    break;


  case EV_VN_RELOADDATA_DOCPOINTERS:

    ReloadXyz ();
    ReloadSp  ();
    ReloadRoi ();

    if ( ! ( XYZDoc || SPDoc || Rois ) )
        break;

    RenderingModeToCurrent3DSet ();
                                        // update OpenGL things from XYZDoc
    InitXyz ();
                                        // rebuild 3D windows
    SetPartWindows ( SelTracks );
                                        // update names of electrodes
//  if ( HasWindowSlots () )   Invalidate ( false );
    break;


  case EV_VN_RELOADDATA_CAPTION:

    UpdateCaption ();
    break;
    }

return true;
}


//----------------------------------------------------------------------------
bool    TTracksView::VnNewTFCursor ( const TTFCursor* tfcursor )
{
if ( tfcursor->SentTo != GetViewId() )
    return false;                       // not for me !

                                        // keep tracks of the sender
LinkedViewId    = tfcursor->SentFrom;


long    pmin    = TFCursor.TranslateCursorTF ( tfcursor, tfcursor->GetPosMin() );
long    pmax    = TFCursor.TranslateCursorTF ( tfcursor, tfcursor->GetPosMax() );


if ( SyncCDPtTFCursor )
    CheckCDPtAndTFCursor ( false );

else {
    TFCursor           = *tfcursor;          // copy new cursor
    TFCursor.SentTo    = tfcursor->SentFrom;
    TFCursor.SentFrom  = GetViewId ();

    if ( pmin >= CDPt.GetMin() && pmax <= CDPt.GetMax() )
        DrawTFCursor ( true );
    else {                              // need to reload some data
        CDPt.SetMin ( TFCursor.GetPosMin() - CDPt.GetLength() * 0.1, true );
                                        // reload all rather, for filtering security
        ReloadBuffers ();

        ShowNow ();
        }
    }

                                        // send the new cursor to friend views (especially eeg view)
//EEGDoc->NotifyFriendViews ( LinkedViewId, vnNewTFCursor, (TParam2) &TFCursor, this ); // sends for ever if != origins and 1 without SF

                                        // seems enough for the other eeg views of the same eeg doc
EEGDoc->NotifyViews ( vnNewTFCursor, (TParam2) &TFCursor, this );

return  true;
}


//----------------------------------------------------------------------------
bool    TTracksView::VnNewSelection ( const TSelection *sel )
{
if ( SelTracks != *sel ) {

    SelTracks  = *sel;

    ReloadBuffers ();

    SetPartWindows ( SelTracks );
    }

return  true;
}


//----------------------------------------------------------------------------
bool    TTracksView::VnNewHighlighted ( const TSelection *sel )
{
    // message can be either for electrodes or SPs                                if 0, accept all
if ( abs ( sel->Size () - Highlighted.Size () ) > NumPseudoTracks || sel->SentFrom && sel->SentFrom != LinkedViewId )
    return true;                        // not for me !


if ( Highlighted != *sel ) {
                                        // switch only the tracks that differ
    for ( int i = 0; i < min ( Highlighted.Size (), sel->Size () ); i++ ) {
        if ( Highlighted[ i ] != (*sel)[ i ] ) {
            Highlighted.Invert ( i );
            DrawHighlighted ( i, Highlighted[ i ] );
            }
        }


//  Highlighted  = *sel;                // nope, sel can be without the extra pseudos, so Highlighted will be clipped...

                                        // send a message
    EEGDoc->NotifyFriendViews ( LinkedViewId, vnNewHighlighted, (TParam2) &Highlighted, this );
//  EEGDoc->NotifyViews ( vnNewHighlighted, (TParam2) Highlighted, this );

//  Invalidate ( false );
    ShowNow ();
    }

return  true;
}


//----------------------------------------------------------------------------
bool    TTracksView::VnNewBadSelection ( const TSelection *sel )
{
  // if 0, accept all
if ( sel->SentFrom && sel->SentFrom != LinkedViewId )
    return true;                      // not for me !

                                        // smart copy
BadTracks  = *sel;
EEGDoc->ClearPseudo ( BadTracks );
                                        // reload data which may have changed due to bad tracks
ReloadBuffers ();

ResetScaleTracks ();

Invalidate ( false );

return  true;
}


//----------------------------------------------------------------------------
bool    TTracksView::VnNewAuxSelection ( const TSelection *sel )
{
//if ( sel->SentFrom != LinkedViewId )
//    return true;                      // not for me !

                                        // smart copy
AuxTracks  = *sel;
EEGDoc->ClearPseudo ( AuxTracks );
                                        // reload data which may have changed due to bad tracks
ReloadBuffers ();

ResetScaleTracks ();

Invalidate ( false );

return  true;
}


//----------------------------------------------------------------------------
                                        // Changing session on-the-fly will modify:
                                        //  - Number of time frames
                                        //  - Date and time of origin
                                        //  - Sampling Frequency
bool    TTracksView::VnSessionUpdated ( void* )
{
                                        // Most importantly, duration has changed, so we need to update these buffers
BuffSize            = NoMore ( EEGDoc->GetNumTimeFrames (), (long) EegMaxPointsDisplay );

EegBuff.Resize ( 1, EEGDoc->GetTotalElectrodes (), BuffSize );

if ( EEGDoc->HasStandardDeviation () )
    SdBuff.Resize ( EEGDoc->GetTotalElectrodes (), BuffSize );


                                        // Updated as collaterals:
CDPt                = TInterval (   0, 
                                    EEGDoc->GetNumTimeFrames () - 1, 
                                    BuffSize,
                                    CDPt.GetMin (),
                                    CDPt.GetMax ()
                                );
CDP                 = &CDPt;


auto                savedtfcursor   = TFCursor;

TFCursor            = TTFCursor (   EEGDoc,
                                    CDPt.GetLimitMin  (), 
                                    CDPt.GetLimitMax  (),
                                    CDPt.ClipLimit    ( TFCursor.GetPosMin () ),
                                    CDPt.ClipLimit    ( TFCursor.GetPosMax () )
                                 );

TFCursor.SentTo     = savedtfcursor.SentTo;
TFCursor.SentFrom   = savedtfcursor.SentFrom;

LastCursorPosMin    = -1;
LastCursorPosMax    = -1;


//CheckCDPtAndTFCursor ( true );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Resetting a few display flags
if ( ! EEGDoc->DateTime.IsOriginDateAvailable () )
    ShowDate        = false;

if ( EEGDoc->GetSamplingFrequency () == 0 )
    ShowTime        = false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // reload data which may have changed due to bad tracks
ReloadBuffers ();

ResetScaleTracks ();

Invalidate ( false );

if ( TFCursor != savedtfcursor )
                                        // propagate the new TFCursor
    EEGDoc->NotifyViews ( vnNewTFCursor, (TParam2) &TFCursor, this );

return  true;
}


//----------------------------------------------------------------------------
void    TTracksView::CheckCDPtAndTFCursor ( bool fromtfcursor )
{
if ( !SyncCDPtTFCursor )                // not my business?
    return;

                                        // or nothing to do?
if ( TFCursor.GetPosMin() == CDPt.GetMin ()
  && TFCursor.GetPosMax() == CDPt.GetMax () )
    return;


if ( fromtfcursor ) {                   // which direction of transfer?
    long    oldtfmin    = CDPt.GetMin();
    long    oldtfmax    = CDPt.GetMax();

                                        // transfer from TFCursor to CDPt
    CDPt.SetMinMax ( TFCursor.GetPosMin(), TFCursor.GetPosMax() );

                                        // check for min length
    if ( CDPt.GetLength() < DisplayMinTF )
        CDPt.SetLength ( DisplayMinTF, TFCursor.GetPosMiddle(), true ); // here we may have a difference between TFCursor and CDPt

    else if ( TFCursor.GetPosMin() != CDPt.GetMin ()
           || TFCursor.GetPosMax() != CDPt.GetMax () ) {
                                        // and back, as CDPt might have changed the extend
        if ( TFCursor.IsExtending() )
            TFCursor.SetExtendingPos ( CDPt.GetMax () );
        else
            TFCursor.SetPos ( CDPt.GetMin (), CDPt.GetMax () );
                                        // therefore, we need to propagate
        EEGDoc->NotifyFriendViews ( LinkedViewId, vnNewTFCursor, (TParam2) &TFCursor, this );
        }
                                        // as we modified CDPt without the user's consent
                                        // estimate the user control STH to allow a consistent feedback
    ComputeSTHfromCDPt ();

                                        // then refresh the display buffer
    UpdateBuffers ( oldtfmin, oldtfmax, CDPt.GetMin(), CDPt.GetMax() );

    Invalidate ( false );
    ScrollBar->UpdateIt();
    }
else {
//    DBGV2 ( CDPt.GetMin (), CDPt.GetMax (), "CDPt" );
                                        // transfer from CDPt to TFCursor
    TFCursor.SetPos ( CDPt.GetMin (), CDPt.GetMax () );

//  Invalidate ( false );
    EEGDoc->NotifyFriendViews ( LinkedViewId, vnNewTFCursor, (TParam2) &TFCursor, this );
    }

}


//----------------------------------------------------------------------------
void    TTracksView::CmEditCopy ()
{
long                timemin         = TFCursor.GetPosMin ();
long                timemax         = TFCursor.GetPosMax ();
TSelection*         tosel           = (bool) Highlighted ? &Highlighted : &SelTracks;
int                 numtracks       = tosel->NumSet ();


constexpr int       clipboardfloatwidth     = 14;
constexpr int       clipboardfloatprecision =  7;

TClipboardEx        clipboard ( *this, ( timemax - timemin + 1 ) * ( numtracks * ( clipboardfloatwidth + 1 ) + 2 ) + 3 );

if ( ! (bool) clipboard )  return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                vbuff[ 32 ];
int                 bufflen         = 0;
bool                firstval;

for ( int tf = timemin; tf <= timemax; tf++ ) {

    firstval = true;

    for ( TIteratorSelectedForward eli ( *tosel ); (bool) eli; ++eli ) {

        if ( ! firstval ) {
                                        // the tabs allow a nice pasting in Excel
            CopyVirtualMemory ( &clipboard[ bufflen ], Tab, 1 );
            bufflen++;
            }

        firstval = false;

                                        // convert to string
        StringCopy  ( vbuff, FloatToString ( EegBuff[ eli() ][ tf - CDPt.GetMin() ], clipboardfloatwidth, clipboardfloatprecision ) );

                                        // manual concatenation, bypassing the super-inefficient strcat
        CopyVirtualMemory ( &clipboard[ bufflen ], vbuff, clipboardfloatwidth );
        bufflen += clipboardfloatwidth;
        }

                                        // add a new line, without the \n of C, which code 1 char, but are 2 under windows
    clipboard.AddNewLine ();
    bufflen += 2;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // whole string ending
clipboard[ bufflen ] = EOS;

                                        // send it!
clipboard.Send ( CF_TEXT );
}


void    TTracksView::GLEditCopyBitmap ()
{
crtl::WindowHide ( ScrollBar );

TBaseView::GLEditCopyBitmap ();

crtl::WindowRestore ( ScrollBar );
}


//----------------------------------------------------------------------------
                                        // Copy directly from EEG buffer of the connected EEG view
                                        // Faster, and really display what is on the EEG view!
                                        // All tracks are being read all the time, so we can poke into EegBuff even if eeg view is partial
                                        // Can force an average reference
                                        // Called from: TInverseMatrixDoc, TFrequenciesView, TPotentialsView
void    TTracksView::GetTracks ( long tf1, long tf2, TArray2<float> &buff, ReferenceType ref )
{
                                        // if out of bound, call the doc instead to get all requested data
if ( ! IsInsideLimits ( tf1, tf2, CDPt.GetMin (), CDPt.GetMax () ) ) { 

    EEGDoc->GetTracks   (   tf1,    tf2, 
                            buff,   0, 
                            AtomTypeUseCurrent, 
                            NoPseudoTracks, 
                            ref 
                        );

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check & adjust buffer size if needed
int                 numel           = EEGDoc->GetNumElectrodes ();
int                 numtf           = tf2 - tf1 + 1;

                                        // + room for pseudo tracks (although only average might be used here)
buff.Resize (   AtLeast ( buff.GetDim1 (), numel + NumPseudoTracks ),
                AtLeast ( buff.GetDim2 (), numtf                   )  );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // shift to buffer origin
tf1         -= CDPt.GetMin ();
//tf2         -= CDPt.GetMin ();


for ( int el = 0; el < numel; el++ )

    CopyVirtualMemory ( buff[ el ], EegBuff[ el ] + tf1, numtf * EegBuff.AtomSize () );


                                        // finally subtract the average reference
if ( ref == ReferenceAverage ) {

    double              avg;

    for ( int tfi0 = 0, tfbuffi = tf1; tfi0 < numtf; tfi0++, tfbuffi++ ) {

                                        // we assume the average is always present in the buffer..
        avg     = EegBuff[ EEGDoc->GetAvgIndex () ][ tfbuffi ];


        for ( int el = 0; el < numel; el++ )
            if ( ! AuxTracks[ el ] )   
                buff[ el ][ tfi0 ] -= avg;
        }

    } // if ReferenceAverage

}


//----------------------------------------------------------------------------
const char* TTracksView::GetElectrodeName   (   int     e,      bool    mtg )
{
if ( mtg && (bool) Montage && e < EEGDoc->GetNumElectrodes() )

    return                                                Montage[ e ].Name;

else

    return  XYZDoc && e < XYZDoc->GetNumElectrodes ()   ? XYZDoc->GetElectrodeName  ( e )   // for EEG tracks
          : SPDoc  && e < SPDoc ->GetNumSolPoints  ()   ? SPDoc ->GetSPName         ( e )   // for RIS tracks
                                                        : EEGDoc->GetElectrodeName  ( e );
}


//----------------------------------------------------------------------------
void    TTracksView::ReloadBuffers ()
{
UpdateBuffers ( -1, -1, CDPt.GetMin(), CDPt.GetMax() );
}


//----------------------------------------------------------------------------
                                        // Optimized reload of the eeg buffer; handles ALL the cases.
void    TTracksView::UpdateBuffers ( long oldtfmin, long oldtfmax, long newtfmin, long newtfmax )
{
                                        // avoid useless job
if ( oldtfmin == newtfmin && oldtfmax == newtfmax )
    return;

                                        // trick: in case of 2D5 buffer, this will still do the current page
auto                bigblock        = [] ( const TArray2<float>& buff ) -> size_t   { return  buff.GetDim1 () * buff.GetDim2 (); };

int                 deltamin        = newtfmin - oldtfmin;
int                 deltamax        = newtfmax - oldtfmax;
size_t              tomove;
int                 offset;
PseudoTracksType    pseudotracks    = EEGDoc->HasPseudoElectrodes () ? ComputePseudoTracks : NoPseudoTracks;
const TRois*        rois            = IsRoiMode () && Rois && AverageRois ? Rois : 0;

                                        // reload everything: upon request, or in case of temporal filters
if ( newtfmin > oldtfmax 
  || newtfmax < oldtfmin
  || EEGDoc->AreFiltersActivated () && EEGDoc->GetFilters ()->HasTemporalFilter () ) {  // only temporal filters need to care for margins and full reload

    EEGDoc->GetTracks   (   newtfmin,       newtfmax, 
                            EegBuff,        0, 
                            AtomTypeUseCurrent, 
                            pseudotracks, 
                            ReferenceUsingCurrent,  0,
                            rois 
                        );

    if ( HasStandardDeviation () )
        EEGDoc->GetStandardDeviation ( newtfmin, newtfmax, SdBuff, 0, rois );
    return;
    }


if ( deltamin >= 0 && deltamax <= 0 ) { // (shrink)

    tomove  = bigblock ( EegBuff ) - deltamin + deltamax;

    MoveVirtualMemory ( EegBuff[0], EegBuff[0] + deltamin, tomove * EegBuff.AtomSize () );

    if ( HasStandardDeviation () ) {

        tomove  = bigblock ( SdBuff ) - deltamin + deltamax;

        MoveVirtualMemory ( SdBuff[0], SdBuff[0] + deltamin, tomove * SdBuff.AtomSize () );
        }

    return;
    }
                                        // intersect partly, for sure
else if ( deltamin >= 0 ) {             // case 1 (shift buffer left)

    tomove  = bigblock ( EegBuff ) - deltamin;
    offset  = oldtfmax - newtfmin + 1;

    MoveVirtualMemory ( EegBuff[0], EegBuff[0] + deltamin, tomove * EegBuff.AtomSize () );

    EEGDoc->GetTracks   (   oldtfmax + 1,   newtfmax, 
                            EegBuff,        offset,                             
                            AtomTypeUseCurrent, 
                            pseudotracks, 
                            ReferenceUsingCurrent,  0,
                            rois 
                        );

    if ( HasStandardDeviation () ) {

        tomove  = bigblock ( SdBuff ) - deltamin;

        MoveVirtualMemory ( SdBuff[0], SdBuff[0] + deltamin, tomove * SdBuff.AtomSize () );

        EEGDoc->GetStandardDeviation ( oldtfmax + 1, newtfmax, SdBuff, offset, rois );
        }

    return;
    }

else if ( deltamax <= 0 ) {             // case 2 (shift buffer right)

    tomove  = bigblock ( EegBuff ) + deltamin;

    MoveVirtualMemory ( EegBuff[0] - deltamin, EegBuff[0], tomove * EegBuff.AtomSize () );

    EEGDoc->GetTracks   (   newtfmin,       oldtfmin - 1, 
                            EegBuff,        0, 
                            AtomTypeUseCurrent, 
                            pseudotracks, 
                            ReferenceUsingCurrent,  0,
                            rois 
                        );


    if ( HasStandardDeviation () ) {

        tomove  = bigblock ( SdBuff ) + deltamin;

        MoveVirtualMemory ( SdBuff[0] - deltamin, SdBuff[0], tomove * SdBuff.AtomSize () );

        EEGDoc->GetStandardDeviation ( newtfmin, oldtfmin - 1, SdBuff,  0, rois );
        }

    return;
    }

else {                                  // case 3: deltamin < 0 && deltamax > 0 (expand)

    tomove  = bigblock ( EegBuff ) + deltamin;
    offset  = oldtfmax - newtfmin + 1;

    MoveVirtualMemory ( EegBuff[0] - deltamin, EegBuff[0], tomove * EegBuff.AtomSize () );

    EEGDoc->GetTracks   (   newtfmin,       oldtfmin - 1, 
                            EegBuff,        0,
                            AtomTypeUseCurrent, 
                            pseudotracks, 
                            ReferenceUsingCurrent,  0,
                            rois 
                        );


    EEGDoc->GetTracks   (   oldtfmax + 1,   newtfmax, 
                            EegBuff,        offset,
                            AtomTypeUseCurrent, 
                            pseudotracks, 
                            ReferenceUsingCurrent,  0,
                            rois 
                        );


    if ( HasStandardDeviation () ) {

        tomove  = bigblock ( SdBuff ) + deltamin;

        MoveVirtualMemory ( SdBuff[0] - deltamin, SdBuff[0], tomove * SdBuff.AtomSize () );

        EEGDoc->GetStandardDeviation ( newtfmin, oldtfmin - 1, SdBuff, 0,      rois );

        EEGDoc->GetStandardDeviation ( oldtfmax + 1, newtfmax, SdBuff, offset, rois );
        }

    return;
    }
}


//----------------------------------------------------------------------------
                                        // regular tracks relative scaling set to 1
                                        // the others are relatively scaled to an average ratio
void    TTracksView::ResetScaleTracks ( const TSelection* sel )
{
long                fromtf          = 0;
long                totf            = CDPt.GetLength() - 1;


                                        // segment file
if      ( EEGDoc->IsContentType ( ContentTypeSeg ) ) {

    enum                {
                        scalegfp        = 0,
                        scaledis,
                        scalegev,
                        scalecorr,
                        scaleev,
                        scalepol,
                        scaleseg,
                        scaleothers,
                        numscales,
                        };

    TGoEasyStats        scale       ( numscales );
    TArray1<int>        scaleindex  ( EEGDoc->GetTotalElectrodes () );
    int                 si;


    for ( int e = 0; e < EEGDoc->GetTotalElectrodes (); e++ ) {
                                        // we test only the beginning of the string, as we have something like "Gfp1" "Gfp2" "Gfp3"
                                        // so that all "Gfp*" tracks will share the same scaling
        if      ( StringStartsWith ( GetElectrodeName ( e ), TrackNameGFP       ) )     si  = scalegfp;
        else if ( StringStartsWith ( GetElectrodeName ( e ), TrackNameRMS       ) )     si  = scalegfp;
        else if ( StringStartsWith ( GetElectrodeName ( e ), TrackNameDIS       ) )     si  = scaledis;
        else if ( StringStartsWith ( GetElectrodeName ( e ), TrackNameDISPlus   ) )     si  = scaledis;
        else if ( StringStartsWith ( GetElectrodeName ( e ), TrackNameGEV       ) )     si  = scalegev;
        else if ( StringStartsWith ( GetElectrodeName ( e ), TrackNameCorr      ) )     si  = scalecorr;
        else if ( StringStartsWith ( GetElectrodeName ( e ), TrackNameEV        ) )     si  = scaleev;
        else if ( StringStartsWith ( GetElectrodeName ( e ), TrackNamePolarity  ) )     si  = scalepol;
        else if ( StringStartsWith ( GetElectrodeName ( e ), TrackNameSeg       ) )     si  = scaleseg;
        else                                                                            si  = scaleothers;
                                        // store this for later
        scaleindex ( e )    = si;
                                        // we know the scaling for these variables
        if ( si == scalecorr || si == scaleev || si == scalepol || si == scaleseg )
            continue;

        for ( int tf = fromtf; tf <= totf; tf++ )
            scale ( si ).Add ( EegBuff[ e ][ tf ], ThreadSafetyIgnore );
        }

                                        // force scaling
    int                 NumClusters     = AtLeast ( 1, dynamic_cast<TSegDoc*> ( EEGDoc )->GetNumClusters () );

    scale ( scalecorr ).Add ( 1,           ThreadSafetyIgnore );
    scale ( scaleev   ).Add ( 1,           ThreadSafetyIgnore );
    scale ( scalepol  ).Add ( 3,           ThreadSafetyIgnore );    // no need to show this at full scale
    scale ( scaleseg  ).Add ( NumClusters, ThreadSafetyIgnore );

                                        // set the scaling with the right stat
    for ( int e = 0; e < EEGDoc->GetTotalElectrodes (); e++ )

        ScaleTracks[ e ]    = 1.0 / NonNull ( scale ( scaleindex ( e ) ).AbsoluteMax () );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // .t .p spectrum and the like will have a common scale
else if ( EEGDoc->IsContentType ( ContentTypeData      )
       || EEGDoc->IsContentType ( ContentTypeErrorData ) ) {
                                        // Give each track its own scaling
    TGoEasyStats        scale       ( EEGDoc->GetTotalElectrodes () );
    bool                singlescale = EEGDoc->IsSpecialInfix () || EEGDoc->IsP ();  // special case: join all scales


    for ( int e = 0; e < EEGDoc->GetTotalElectrodes (); e++ )
    for ( int tf = fromtf; tf <= totf; tf++ )

//      scale ( singlescale ? 0 : e ).Add ( EegBuff[ e ][ tf ], ThreadSafetyIgnore );
                                        // either 1 common scale, or 1 scale for each of the first 3 tracks, then a common one
        scale ( singlescale ? 0 : NoMore ( 3, e ) ).Add ( EegBuff[ e ][ tf ], ThreadSafetyIgnore );


                                        // set the scaling with the right stat
    for ( int e = 0; e < EEGDoc->GetTotalElectrodes (); e++ )

//      ScaleTracks[ e ]    = 1.0 / NonNull ( scale ( singlescale ? 0 : e ).AbsoluteMax () );
        ScaleTracks[ e ]    = 1.0 / NonNull ( scale ( singlescale ? 0 : NoMore ( 3, e ) ).AbsoluteMax () ) * 0.80;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else {                                  // regular file
    TEasyStats          stats;
    TDownsampling       downtf ( fromtf, totf, NumScanTracksScaling );
    constexpr double    defaultscaletracks  = 100;

                                        // stats on the absolute values of all the regular tracks together
    for ( int e = 0; e < EEGDoc->GetNumElectrodes (); e++ ) {

        if ( BadTracks[ e ] || AuxTracks[ e ] ) continue;


        for ( int tf = downtf.From; tf <= downtf.To; tf += downtf.Step ) {

            if ( (bool) Montage ) {

                if ( (bool) Montage[ e ] ) {

                    if ( Montage[ e ].TwoTracks () )
                        stats.Add ( fabs ( Montage[ e ].ToData1[ tf ] - Montage[ e ].ToData2[ tf ] ), ThreadSafetyIgnore );
                    else
                        stats.Add ( fabs ( Montage[ e ].ToData1[ tf ] ),                              ThreadSafetyIgnore );
                    }
                //else // montage, but they are all auxiliaries
                //    v   = 0;
                } // if montage

            else // no Montage
                stats.Add ( fabs ( EegBuff[ e ][ tf ] ), ThreadSafetyIgnore );

            } // for tf
        } // for e

                                        // Mixing multiple measures for max tracks
    double          maxtracks       = stats.AbsoluteMax () == 0 ? defaultscaletracks 
                                                                : NonNull ( GeometricMean ( 4 * stats.Average (), 4 * stats.SD (), stats.AbsoluteMax () ) );

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    for ( int e = 0; e < EEGDoc->GetTotalElectrodes (); e++ ) {

        if ( sel && sel->IsNotSelected ( e ) )  continue;


        if ( e <= EEGDoc->GetLastRegularIndex () 
          && ! ( BadTracks[ e ] || AuxTracks[ e ] ) ) {

            ScaleTracks[ e ]    = 1 / maxtracks;
            }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        else {                          // auxs and bads are scaled individually

            stats.Reset ();

            for ( int tf = fromtf; tf <= totf; tf++ ) {

                if ( (bool) Montage && e <= EEGDoc->GetLastRegularIndex () ) {

                    if ( (bool) Montage[ e ] ) {

                        if ( Montage[ e ].TwoTracks () )
                            stats.Add ( fabs ( Montage[ e ].ToData1[ tf ] - Montage[ e ].ToData2[ tf ] ), ThreadSafetyIgnore );
                        else
                            stats.Add ( fabs ( Montage[ e ].ToData1[ tf ] ),                              ThreadSafetyIgnore );
                        }
                    //else // montage, but they are all auxiliaries
                    //    v   = 0;
                    } // if montage

                else // no Montage, or pseudo electrode
                    stats.Add ( fabs ( EegBuff[ e ][ tf ] ), ThreadSafetyIgnore );
                } // for tf

                                        // Scale for current track
            double          maxtrack        = stats.AbsoluteMax () == 0 ? defaultscaletracks 
                                                                        : GeometricMean ( 4 * stats.Average (), 4 * stats.SD (), stats.AbsoluteMax () );

            ScaleTracks[ e ]    = 1 / NonNull ( maxtrack );

                                        // slight adjustments for pseudo-tracks:
            if      ( e == EEGDoc->GetGfpIndex () )     ScaleTracks[ e ]   *= 2.0;
            else if ( e == EEGDoc->GetDisIndex () )     ScaleTracks[ e ]   *= 1.5;
            else if ( e == EEGDoc->GetAvgIndex () 
                   && maxtrack / NonNull ( EEGDoc->GetAbsMaxValue () ) < 1e-6 )     // nearly 0?
                                                        ScaleTracks[ e ]    = 1;    // then reset scaling, to avoid showing a noisy line

            } // special electrode
        } // for e
    } // regular file
}


//----------------------------------------------------------------------------
void    TTracksView::CmZoomVert ( owlwparam w )
{
double              osl             = ScalingLevel;


if      ( w == IDB_COMPTRACKSV || w == IDB_ISDECBRIGHT )  ScalingLevel /= ShiftKey ? ScalingLevelBigStep : ScalingLevelSmallStep ;
else if ( w == IDB_EXTTRACKSV  || w == IDB_ISINCBRIGHT )  ScalingLevel *= ShiftKey ? ScalingLevelBigStep : ScalingLevelSmallStep;


SetScaling ( ScalingLevel );

RefreshLinkedWindow ();

if ( w != 0 && osl == ScalingLevel )
    return;

//Invalidate ( false );
ShowNow ();
}


void    TTracksView::CmZoomHorz ( owlwparam w )
{
ulong               oldlength       = CDPt.GetLength ();
long                oldtfmin        = CDPt.GetMin ();
long                oldtfmax        = CDPt.GetMax ();
ulong               newlength;
ulong               mintf           = oldlength < DisplayMinTF ? 2 : DisplayMinTF;
double              sthmin          = 1.0;
double              sthmax          = CDPt.GetLimitLength ();


do {                                // update STH until it produces a new length
    if      ( w == IDB_EXTTRACKSH  )    STH *= ShiftKey ? EEGGLVIEW_STHBIGSTEP : EEGGLVIEW_STHSMALLSTEP;
    else if ( w == IDB_COMPTRACKSH )    STH /= ShiftKey ? EEGGLVIEW_STHBIGSTEP : EEGGLVIEW_STHSMALLSTEP;


    Clipped ( STH, sthmin, sthmax );

                                        // force an odd interval, which remain nicely centered on cursor, except when zoom maxed in
    ulong           oddtf           = mintf <= 1 || oldlength <= 11 ? 0 : 1;

    newlength   = ( (int) ( ( CDPt.GetLimitLength () - mintf ) / STH + mintf ) ) | oddtf;

                                        // recheck limits
    if      ( newlength > CDPt.GetLimitLength () )      newlength = CDPt.GetLimitLength ();
    else if ( newlength < mintf                  )      newlength = mintf | oddtf;

    } while ( w && newlength == oldlength && ! ( STH == sthmax || STH == sthmin ) );


if ( newlength == oldlength )           // still no change?
    return;

                                        // modify CDPt
CDPt.SetLength ( newlength, TFCursor.GetPosMiddle (), true );
                                        // contaminate TFCursor
CheckCDPtAndTFCursor ( false );

RefreshLinkedWindow ();

if ( oldtfmin == CDPt.GetMin () 
  && oldtfmax == CDPt.GetMax () )

    return;


UpdateBuffers ( oldtfmin, oldtfmax, CDPt.GetMin (), CDPt.GetMax () );

                                        // ?Not sure if really needed?
//EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_EEG );

//Invalidate ( false );
ShowNow ();

ScrollBar->UpdateIt ();
}


//----------------------------------------------------------------------------
void    TTracksView::ScrollDisplay ( int numpoints )
{
long    oldTFMin    = CDPt.GetMin();
long    oldTFMax    = CDPt.GetMax();

                                        // update window
CDPt.SetMin ( CDPt.GetMin() + numpoints, true );
                                        // then buffer
UpdateBuffers ( oldTFMin, oldTFMax, CDPt.GetMin(), CDPt.GetMax() );

CheckCDPtAndTFCursor ( false );

                                        // need to move the cursor ?
if ( TFCursor.GetPosMax() > CDPt.GetMax()
  || TFCursor.GetPosMin() < CDPt.GetMin() ) {
    if ( TFCursor.IsExtending() )
        TFCursor.SetExtendingPos ( TFCursor.GetExtendingPos() + CDPt.GetMin() - oldTFMin );
    else
        TFCursor.ShiftPos ( CDPt.GetMin() - oldTFMin );

    EEGDoc->NotifyFriendViews ( LinkedViewId, vnNewTFCursor, (TParam2) &TFCursor, this );
    }

                                        // ?Not sure if really needed?
//EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_EEG );

                                        // et hop!
ShowNow ( RDW_UPDATENOW | RDW_INVALIDATE | RDW_NOERASE | RDW_ALLCHILDREN );
ScrollBar->UpdateIt();

RefreshLinkedWindow () ;
}


void    TTracksView::ScrollCursor ( int desiredPoints, bool forward )
{

int     readpoints  = desiredPoints;


if ( forward ) {
                                        // do not read more points than allowed by current window
    if ( readpoints > (int) CDPt.GetLength() )
        readpoints  = CDPt.GetLength();

                                        // can not read beyong last data
    if ( (CDPt.GetMax() + readpoints) > CDPt.GetLimitMax () )
        readpoints = CDPt.GetLimitMax () - CDPt.GetMax();

    if ( (CDPt.GetMax() + readpoints) < TFCursor.GetPosMax() ) {  // reading is not enough, so jump
        long    tf1 = CDPt.GetMin();
        long    tf2 = CDPt.GetMax();

        CDPt.SetMax ( TFCursor.GetPosMax(), true );

        UpdateBuffers ( tf1, tf2, CDPt.GetMin(), CDPt.GetMax() );
        //Invalidate ( false );
        ShowNow ();
        }
    else {
        UpdateBuffers ( CDPt.GetMin(), CDPt.GetMax(), CDPt.GetMin() + readpoints, CDPt.GetMax() + readpoints );
        CDPt.SetMin ( CDPt.GetMin() + readpoints, true );

//        if ( ! ( ShowVertScale || TFCursor.IsExtending() || EEGDoc->AreFiltersActivated () ) )
//            ScrollPartWindows ( -readpoints );
//        else
//            Invalidate ( false );
            ShowNow ( RDW_UPDATENOW | RDW_INVALIDATE | RDW_NOERASE | RDW_ALLCHILDREN );
        }
    }
else {
                                        // do not read more points than allowed by current window
    if ( readpoints > (int) CDPt.GetLength() )
        readpoints  = CDPt.GetLength();
                                        // can not read before first data
    if ( (CDPt.GetMin() - readpoints) < CDPt.GetLimitMin () )
        readpoints = CDPt.GetMin();

    if ( (CDPt.GetMin() - readpoints) > TFCursor.GetPosMin() ) {  // reading is not enough, so jump
        long    tf1 = CDPt.GetMin();
        long    tf2 = CDPt.GetMax();

        CDPt.SetMin ( TFCursor.GetPosMin(), true );

        UpdateBuffers ( tf1, tf2, CDPt.GetMin(), CDPt.GetMax() );
        //Invalidate ( false );
        ShowNow ();
        }
    else {
        UpdateBuffers ( CDPt.GetMin(), CDPt.GetMax(), CDPt.GetMin() - readpoints, CDPt.GetMax() - readpoints );
        CDPt.SetMin ( CDPt.GetMin() - readpoints, true );

//        if ( ! ( ShowVertScale || TFCursor.IsExtending() || EEGDoc->AreFiltersActivated () ) )
//            ScrollPartWindows ( readpoints );
//        else
//            Invalidate ( false );
            ShowNow ( RDW_UPDATENOW | RDW_INVALIDATE | RDW_NOERASE | RDW_ALLCHILDREN );
        }
    }

ScrollBar->UpdateIt();
}


void    TTracksView::CmForward ( owlwparam w )
{
if ( TFCursor.GetPosMax () == CDPt.GetLimitMax () && !TFCursor.IsExtending() )
    return;


int                 deltas;

if ( ControlKey && TFCursor.IsSplitted () ) {

    TFCursor.ShiftPos ( TFCursor.GetLength () );
    deltas      = 0;
    }

else {
//    if ( Acceleration.GetTimeSinceLastPolled () > 10 )
//        Acceleration.StopAccelerating ();

    Acceleration.DelayForPolling            = DefaultDelayForPolling;
    Acceleration.DelayBeforeAcceleration    = DefaultDelayBeforeAcceleration;
    Acceleration.DelayAfterNoAcceleration   = DefaultDelayAfterNoAcceleration;
    Acceleration.MinSpeed                   = 1;
    Acceleration.MaxSpeed                   = AtLeast ( (ulong) EEGGLVIEW_MAXSPEEDMIN, CDPt.GetLength() /  2 );
    Acceleration.MinAcceleration            = DefaultMinAcceleration;


    int                 speed           = EEGDoc->IsSpontaneous () && ! ControlKey ? Acceleration.GetSpeed () : Acceleration.MinSpeed;
    int                 deltac          = speed;
                        deltas          = w ? max ( speed, Truncate ( CDPt.GetLength() * EEGGLVIEW_STEPREADPERCENT ) ) : deltac;

    if ( TFCursor.IsExtending() )
        TFCursor.SetExtendingPos ( TFCursor.GetExtendingPos () + deltac );
    else
        TFCursor.ShiftPos ( deltac );
    }

                                        // getting out of the window ? -> get new points
if ( SyncCDPtTFCursor ) {
    CheckCDPtAndTFCursor ( true );
//  Invalidate ( false );
    }
else if ( TFCursor.GetPosMax () > CDPt.GetMax() )
    ScrollCursor ( deltas, true );
else
    DrawTFCursor ( true );


EEGDoc->NotifyFriendViews ( LinkedViewId, vnNewTFCursor, (TParam2) &TFCursor, this );
}


void    TTracksView::CmBackward ( owlwparam w )
{
if ( TFCursor.GetPosMin () == CDPt.GetLimitMin () && !TFCursor.IsExtending() )
    return;


int                 deltas;

if ( ControlKey && TFCursor.IsSplitted () ) {

    TFCursor.ShiftPos ( - TFCursor.GetLength () );
    deltas      = 0;
    }

else {
//    if ( Acceleration.GetTimeSinceLastPolled () > 10 )
//        Acceleration.StopAccelerating ();

    Acceleration.DelayForPolling            = DefaultDelayForPolling;
    Acceleration.DelayBeforeAcceleration    = DefaultDelayBeforeAcceleration;
    Acceleration.DelayAfterNoAcceleration   = DefaultDelayAfterNoAcceleration;
    Acceleration.MinSpeed                   = 1;
    Acceleration.MaxSpeed                   = AtLeast ( (ulong) EEGGLVIEW_MAXSPEEDMIN, CDPt.GetLength() /  2 );
    Acceleration.MinAcceleration            = DefaultMinAcceleration;


    int                 speed           = EEGDoc->IsSpontaneous () && ! ControlKey ? Acceleration.GetSpeed () : Acceleration.MinSpeed;
    int                 deltac          = speed;
                        deltas          = w ? max ( speed, Truncate ( CDPt.GetLength() * EEGGLVIEW_STEPREADPERCENT ) ) : deltac;

    if ( TFCursor.IsExtending() )
        TFCursor.SetExtendingPos ( TFCursor.GetExtendingPos () - deltac );
    else
        TFCursor.ShiftPos ( - deltac );
    }

                                        // getting out the window ? -> reading new points
if ( SyncCDPtTFCursor ) {
    CheckCDPtAndTFCursor ( true );
//  Invalidate ( false );
    }
else if ( TFCursor.GetPosMin () < CDPt.GetMin() )
    ScrollCursor ( deltas, false );
else
    DrawTFCursor ( true );


EEGDoc->NotifyFriendViews ( LinkedViewId, vnNewTFCursor, (TParam2) &TFCursor, this );
}


//----------------------------------------------------------------------------
void    TTracksView::CmPageForward ( owlwparam w )
{
//if ( CDPt    .GetMax()    == CDPt.GetLimitMax ()
//  && TFCursor.GetPosMax() == CDPt.GetLimitMax () ) {
//                                        // pressing a second time End will reset the cursor
    if ( w == 0 && TFCursor.IsSplitted () ) {
        TFCursor.SetPos ( CDPt.GetLimitMax () );
        TFCursor.StopExtending ();
        ButtonGadgetSetState ( IDB_RANGECURSOR, TFCursor.IsExtending() );
        }
//    else 
//        return;
//    }


long                oldTFMin        = CDPt.GetMin();
long                oldTFMax        = CDPt.GetMax();


if ( TFCursor.IsExtending() )
    TFCursor.SetExtendingPos ( TFCursor.GetExtendingPos() + CDPt.GetMin() - oldTFMin );
else
    TFCursor.ShiftPos ( CDPt.GetMin() - oldTFMin );


if ( ShiftKey || w == 0 ) {

    CDPt.SetMax ( CDPt.GetLimitMax (), true );

    if ( TFCursor.IsExtending() )
        TFCursor.SetExtendingPos ( CDPt.GetLimitMax () );
    else
        TFCursor.SetPos ( CDPt.GetLimitMax () - TFCursor.GetLength() + 1, CDPt.GetLimitMax () );
    }
else
    CDPt.SetMin ( CDPt.GetMin() + CDPt.GetLength(), true );


UpdateBuffers ( oldTFMin, oldTFMax, CDPt.GetMin(), CDPt.GetMax() );


CheckCDPtAndTFCursor ( false );

                                        // need to move the cursor ?
if ( TFCursor.GetPosMin() > CDPt.GetMax()
  || TFCursor.GetPosMax() < CDPt.GetMin() ) {
    if ( TFCursor.IsExtending() )
        TFCursor.SetExtendingPos ( TFCursor.GetExtendingPos() + CDPt.GetMin() - oldTFMin );
    else
        TFCursor.ShiftPos ( CDPt.GetMin() - oldTFMin );
    }


EEGDoc->NotifyFriendViews ( LinkedViewId, vnNewTFCursor, (TParam2) &TFCursor, this );

//Invalidate ( false );
ShowNow ();

ScrollBar->UpdateIt();
}


void    TTracksView::CmPageBackward ( owlwparam w )
{
//if ( CDPt    .GetMin()    == CDPt.GetLimitMin ()
//  && TFCursor.GetPosMin() == CDPt.GetLimitMin () ) {
//                                        // pressing a second time Home will reset the cursor
    if ( w == 0 && TFCursor.IsSplitted () ) {
        TFCursor.SetPos ( CDPt.GetLimitMin () );
        TFCursor.StopExtending ();
        ButtonGadgetSetState ( IDB_RANGECURSOR, TFCursor.IsExtending() );
        }
//    else 
//        return;
//    }


long                oldTFMin        = CDPt.GetMin();
long                oldTFMax        = CDPt.GetMax();


if ( TFCursor.IsExtending() )
    TFCursor.SetExtendingPos ( TFCursor.GetExtendingPos() + CDPt.GetMin() - oldTFMin );
else
    TFCursor.ShiftPos ( CDPt.GetMin() - oldTFMin );


if ( ShiftKey || w == 0 ) {

    CDPt.SetMin ( CDPt.GetLimitMin (), true );

    if ( TFCursor.IsExtending() )
        TFCursor.SetExtendingPos ( CDPt.GetLimitMin () );
    else
        TFCursor.SetPos ( CDPt.GetLimitMin (), CDPt.GetLimitMin () + TFCursor.GetLength() - 1 );
    }
else
    CDPt.SetMin ( CDPt.GetMin() - CDPt.GetLength(), true );


UpdateBuffers ( oldTFMin, oldTFMax, CDPt.GetMin(), CDPt.GetMax() );


CheckCDPtAndTFCursor ( false );

                                        // need to move the cursor ?
if ( TFCursor.GetPosMin() > CDPt.GetMax()
  || TFCursor.GetPosMax() < CDPt.GetMin() ) {
    if ( TFCursor.IsExtending() )
        TFCursor.SetExtendingPos ( TFCursor.GetExtendingPos() + CDPt.GetMin() - oldTFMin );
    else
        TFCursor.ShiftPos ( CDPt.GetMin() - oldTFMin );
    }


EEGDoc->NotifyFriendViews ( LinkedViewId, vnNewTFCursor, (TParam2) &TFCursor, this );

//Invalidate ( false );
ShowNow ();

ScrollBar->UpdateIt();
}


//----------------------------------------------------------------------------
void    TTracksView::CmChangeNumTracks ( owlwparam w )
{
if ( IsRoiMode () ) {
                                        // we deal directly region by region, not track by track
    const TSelection*   roissel     = Rois->GetRoisSelected ();
    int             r;


    if ( w == IDB_LESSTRACKS ) {
                                        // no more rois?
        if ( roissel->IsNoneSet () )
            return;

                                        // if tracks selected, remove their rois
        if ( (bool) Highlighted ) {

            EEGDoc->ClearPseudo ( Highlighted );

            for ( int r = 0; r < Rois->GetNumRois (); r++ )
                if ( (*Rois)[ r ].Selection.NumSetAnd ( Highlighted ) != 0 )
                    Rois->Reset ( r );

            Highlighted.Reset ();

            EEGDoc->NotifyDocViews ( vnNewHighlighted, (TParam2) &Highlighted, this );
            RefreshLinkedWindow () ;
            }
        else if ( ShiftKey )
            Rois->Reset ();
        else
            Rois->Reset ( roissel->LastSelected () );

        } // IDB_LESSTRACKS

    else if ( w == IDB_MORETRACKS ) {
                                        // if tracks selected, select their rois
        if ( (bool) Highlighted ) {

            EEGDoc->ClearPseudo ( Highlighted );
                                        // set all rois on
            Rois->Set ();
                                        // then remove the ones not intersecting
            for ( int r = 0; r < Rois->GetNumRois (); r++ )
                if ( (*Rois)[ r ].Selection.NumSetAnd ( Highlighted ) == 0 )
                    Rois->Reset ( r );

            Highlighted.Reset ();

            EEGDoc->NotifyDocViews ( vnNewHighlighted, (TParam2) &Highlighted, this );
            RefreshLinkedWindow () ;
            }
        else {
                                        // some more rois?
            if ( roissel->IsAllSet () )
                return;


            if ( ShiftKey )
                Rois->Set ();
            else
                if ( ( r = roissel->FirstNotSelected ( roissel->FirstSelected () ) ) != -1 )
                    Rois->Set ( r );
                else
                    Rois->Set ( roissel->LastNotSelected () );
            }

        } // IDB_MORETRACKS

                                        // sum up all rois into a single selection
    Rois->CumulateInto ( SelTracks, EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () );
    }

else { // not IsRoiMode ()
    int         numTracks   = SelTracks.NumSet ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () );
    int         t;


    if ( w == IDB_LESSTRACKS ) {
                                        // if tracks selected, remove them
        if ( (bool) Highlighted ) {
            EEGDoc->ClearPseudo ( Highlighted );
            CmSelection ( CM_REMOVESEL );
            return;
            }


        if ( numTracks <= 0 )
            return;


        if ( ShiftKey || numTracks == 1 ) {
            LastVisibleTrack = SelTracks.FirstSelected ();
            EEGDoc->ClearRegular ( SelTracks );
            SetPartWindows ( SelTracks );
            return;
            }


        int     numrem  = numTracks > 100                           ? numTracks * 0.10
                        : numTracks > EEGGLVIEW_MINTRACKSPRECISION  ? EEGGLVIEW_DELTATRACKS 
                        :                                             1;


        while ( numrem ) {
            t           = SelTracks.LastSelected ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () );

            if ( t == -1 )
                break;

            SelTracks.Reset ( t );
            numrem--;
            }
        } // IDB_LESSTRACKS

    else if ( w == IDB_MORETRACKS ) {
                                        // if tracks selected, keep them
        if ( (bool) Highlighted ) {
            if ( EEGDoc->HasPseudoElectrodes () )
                Highlighted.Copy ( EEGDoc->GetFirstPseudoIndex (), EEGDoc->GetLastPseudoIndex (), &SelTracks );
            CmSelection ( CM_KEEPSEL );
            return;
            }


        if ( numTracks == GetMaxTracks () )
            return;


        if ( ShiftKey ) {
            if ( (bool) Montage )
                SelTracks.Set ( 0, Montage.GetNumActiveSlots () - 1 );
            else
                EEGDoc->SetRegular ( SelTracks );
            }
        else {
                                        // compute number of tracks to add
            int     numadd  = numTracks > 100                           ? numTracks * 0.10
                            : numTracks >= EEGGLVIEW_MINTRACKSPRECISION ? EEGGLVIEW_DELTATRACKS 
                            :                                             1;


            if ( numTracks == 0 ) {     // restore last tracks?
                SelTracks.Set ( LastVisibleTrack );
                numadd--;
                }


            int     firstsel    = SelTracks.FirstSelected ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () );
            int     lastsel     = SelTracks.LastSelected  ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () );
            int     gap         = numTracks > 0 ? SelTracks.NumNotSet ( firstsel, lastsel ) : 0;


            while ( gap > 0 && numadd > 0 ) {
                                        // redo these limits
                firstsel    = SelTracks.FirstSelected ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () );
                lastsel     = SelTracks.LastSelected  ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () );

                t           = SelTracks.FirstNotSelected ( firstsel, lastsel );

                if ( t == -1 )
                    break;

    //            DBGV ( t, "adding gap" );
                SelTracks.Set ( t );
                numadd--;
                gap--;
                }


            while ( numadd > 0 ) {
                t           = SelTracks.NextValue ( SelTracks.LastSelected ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () ) );

                if ( t == -1 || t > EEGDoc->GetLastRegularIndex () )
                    break;

    //            DBGV ( t, "adding trailing" );
                SelTracks.Set ( t );
                numadd--;
                }


            while ( numadd > 0 ) {
    //          t           = SelTracks.LastNotSelected ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () );
                t           = SelTracks.PreviousValue ( SelTracks.FirstSelected ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () ) );

                if ( t == -1 )
                    break;

    //            DBGV ( t, "adding beginning" );
                SelTracks.Set ( t );
                numadd--;
                }

/*
                                        // get space in any holes
            int     firstsel    = SelTracks.FirstSelected ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () );
            int     lastsel     = SelTracks.LastSelected  ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () );

            int     gap         = numTracks > 0 ? SelTracks.NumNotSet ( firstsel, lastsel ) : 0;


            if ( gap ) {                // fill the holes first
                for ( t = SelTracks.FirstNotSelected ( firstsel, lastsel ); t >= 0 && gap > 0 && numadd > 0; t = SelTracks.NextNotSelected (), gap--, numadd-- )
                    SelTracks.Set ( t );
                }
*/

/*
            if ( numadd > 0 ) {         // need to add some more? look at the beginning
                t   = SelTracks.FirstSelected ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () );

                if ( t - numadd >= EEGDoc->GetFirstRegularIndex () ) {
                    SelTracks.Set ( t - numadd, t - 1 );
    //              numadd = 0;
                    }
                else if ( t > EEGDoc->GetFirstRegularIndex () ) {
    //              numadd -= SelTracks.NumNotSet ( EEGDoc->GetFirstRegularIndex (), t - 1 );
                                        // numadd may not be 0 here, but there is no more room for some more - ignore
                    SelTracks.Set ( EEGDoc->GetFirstRegularIndex (), t - 1 );
                    }
                }
*/
            }
        } // IDB_MORETRACKS
    } // else IsRoiMode ()


SetPartWindows ( SelTracks );
}


void    TTracksView::CmChangeNumPseudoTracks (owlwparam w)
{
int     numPseudoTracks;

if ( ! EEGDoc->HasPseudoElectrodes () )
    return;

numPseudoTracks = EEGDoc->GetNumSelectedPseudo  ( SelTracks );


if ( w == IDB_LESSPSEUDOTRACKS ) {
                                        // if tracks selected, remove them
    if ( (bool) Highlighted ) {
        EEGDoc->ClearRegular ( Highlighted );
        CmSelection ( CM_REMOVESEL );
        return;
        }

    if ( numPseudoTracks <= 0 )     return;

    if ( ShiftKey )  EEGDoc->ClearPseudo ( SelTracks );
    else            SelTracks.Reset ( SelTracks.LastSelected ( EEGDoc->GetFirstPseudoIndex (), EEGDoc->GetLastPseudoIndex () ) );

    SetPartWindows ( SelTracks );
    }
else { // IDB_MOREPSEUDOTRACKS
                                        // if tracks selected, keep them
    if ( (bool) Highlighted ) {
        Highlighted.Copy ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex (), &SelTracks );
        CmSelection ( CM_KEEPSEL );
        return;
        }


    if ( numPseudoTracks == NumPseudoTracks )  return;


    if ( ShiftKey )  EEGDoc->SetPseudo ( SelTracks );
    else            SelTracks.Set ( SelTracks.FirstNotSelected ( EEGDoc->GetFirstPseudoIndex (), EEGDoc->GetLastPseudoIndex () )  );

//    ReloadBuffers ();

    SetPartWindows ( SelTracks );
    }
}


//----------------------------------------------------------------------------
void    TTracksView::ShiftTracks ( owlwparam w, int num )
{
if ( num <= 0 )
    return;


if ( IsRoiMode () ) {
    int     n1      = Rois->NumSet ();
    int     n2      = EEGDoc->GetNumSelectedPseudo  ( SelTracks );


    if ( n1 == 0 && n2 == 0 )           // empty
        return;
                                        // full
    if ( n1 == Rois->GetNumRois () && n2 == EEGDoc->GetNumPseudoElectrodes () )
        return;


    if      ( w == IDB_UP   ) {
        if   ( n1 && n1 < Rois->GetNumRois () ) Rois    ->ShiftUp   ( ShiftKey ? n1 : num );
        else                                    SelTracks.ShiftUp   ( EEGDoc->GetFirstPseudoIndex  (), EEGDoc->GetLastPseudoIndex  (), ShiftKey ? n2 : num );
        }
    else if ( w == IDB_DOWN ) {
        if   ( n1 && n1 < Rois->GetNumRois () ) Rois    ->ShiftDown ( ShiftKey ? n1 : num );
        else                                    SelTracks.ShiftDown ( EEGDoc->GetFirstPseudoIndex  (), EEGDoc->GetLastPseudoIndex  (), ShiftKey ? n2 : num );
        }

    Rois->CumulateInto ( SelTracks, EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () );
    }

else { // not IsRoiMode ()
    int     n1      = EEGDoc->GetNumSelectedRegular ( SelTracks );
    int     n2      = EEGDoc->GetNumSelectedPseudo  ( SelTracks );


    if ( n1 == 0 && n2 == 0 )           // empty
        return;
                                        // full
    if ( n1 == GetMaxTracks () && n2 == EEGDoc->GetNumPseudoElectrodes () )
        return;


    if      ( w == IDB_UP   ) {
        if   ( n1 && n1 < GetMaxTracks () ) SelTracks.ShiftUp   ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex (), ShiftKey ? n1 : num );
        else                                SelTracks.ShiftUp   ( EEGDoc->GetFirstPseudoIndex  (), EEGDoc->GetLastPseudoIndex  (), ShiftKey ? n2 : num );
        }
    else if ( w == IDB_DOWN ) {
        if   ( n1 && n1 < GetMaxTracks () ) SelTracks.ShiftDown ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex (), ShiftKey ? n1 : num );
        else                                SelTracks.ShiftDown ( EEGDoc->GetFirstPseudoIndex  (), EEGDoc->GetLastPseudoIndex  (), ShiftKey ? n2 : num );
        }
    }


//ReloadBuffers ();

SetPartWindows ( SelTracks );
}


void    TTracksView::CmShiftTracks ( owlwparam w )
{
ShiftTracks ( w, 1 );
}


//----------------------------------------------------------------------------
void    TTracksView::CmTracksSuper ()
{
TracksSuper = ! TracksSuper;

ButtonGadgetSetState ( IDB_TRACKSSUPER, TracksSuper );

Invalidate ( false );
}


void    TTracksView::CmVertGrid ()
{
ShowVertScale    = ! ShowVertScale;

Invalidate ( false );
}


void    TTracksView::CmHorizGrid ()
{
                                        // Cycling through ALL horizontal scales - but not relevant to all data
//ShowHorizScale  = NextState ( ShowHorizScale, NumTimeDisplayEnum, ShiftKey );
                                        // Alternating between Off and only one Legal horizontal scale
ShowHorizScale  = ShowHorizScale ? NoTimeDisplay : ShowHorizScaleLegal;

Invalidate ( false );
}


void    TTracksView::CmFlipVert ()
{
TracksFlipVert = ! TracksFlipVert;

Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TTracksView::CmRangeCursor ()
{
if ( ! TFCursor.IsExtending() ) {
    TFCursor.SetFixedPos ( TFCursor.GetPosMin () );
    DrawTFCursor ( true );

    EEGDoc->NotifyFriendViews ( LinkedViewId, vnNewTFCursor, (TParam2) &TFCursor, this );
    }
else {
    TFCursor.StopExtending();
    CheckCDPtAndTFCursor ( true );
    //Invalidate ( false );
    ShowNow ();
    }

ButtonGadgetSetState ( IDB_RANGECURSOR, TFCursor.IsExtending() );
}


//----------------------------------------------------------------------------
void    TTracksView::CmSetReference ( owlwparam w )
{
char                buff[ KiloByte ];
int                 el;

                                        // convert button ID to corresponding menu ID
if ( w == IDB_SETAVGREF )

    w   = EEGDoc->GetReferenceType () == ReferenceAverage ? CM_EEGREFNONE : CM_EEGREFAVG;

                                        // force Montage reset
if ( w != CM_EEGREFMONTAGE && (bool) Montage ) {

    Montage.Reset   ();

    SetTextMargin   ();

    ResetScaleTracks();

    SetPartWindows  ( SelTracks );

    AuxTracks.Reset ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

switch ( w ) {

    case CM_EEGREFNONE:

        EEGDoc->SetReferenceType ( ReferenceAsInFile );

        break;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case CM_EEGREFAVG:

        EEGDoc->SetReferenceType ( ReferenceAverage );

        break;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // These two cases just boils down to the same case internally - we just fancy giving some more specific input message
    case CM_EEGREFELECSINGLE:

        ClearString ( buff );

        if ( ! GetInputFromUser ( "Give the name of the new reference track:", "Reference Track", buff, "", this ) )
            return;

        EEGDoc->SetReferenceType ( ReferenceArbitraryTracks, buff, GetElectrodesNames () );

        break;


    case CM_EEGREFELECMULTIPLE:

        ClearString ( buff );

        if ( ! GetInputFromUser ( "Give the names of the new reference tracks:", "Reference Tracks", buff, "", this ) )
            return;

        EEGDoc->SetReferenceType ( ReferenceArbitraryTracks, buff, GetElectrodesNames () );

        break;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    case CM_EEGREFMONTAGE:

        static GetFileFromUser  getfile ( "Montage File", AllMontageFilesFilter, 1, GetFilePath );


        if ( ! getfile.Execute () )
            return;

        ifstream            ifm ( TFileName ( (const char *) getfile, TFilenameExtendedPath ) );

                                        // Testing magic number
        ifm.getline ( buff, KiloByte );

        if ( ! IsMagicNumber ( buff, MTGTXT_MAGICNUMBER1) ) {
            ShowMessage ( "It seems your montage file has a wrong magic number!" NewLine "Check your input file, and specifically its first line...", "Montage File", ShowMessageWarning );
            return;
            }


        Montage.Set ( EEGDoc->GetTotalElectrodes() );

        AuxTracks.Reset ();


        char                el1[ 32 ];
        char                el2[ 32 ];
        char                mname[ MONTAGE_MAXCHARELECNAME ];
        int                 mi              = 0;
        int                 nscan;

        do {
            ifm.getline ( buff, KiloByte );

            ClearString ( el1 );
            ClearString ( el2 );
            ClearString ( mname );

            nscan = sscanf ( buff, "%s %s %s", el1, el2, mname );


            if ( nscan == 0 )   continue;

            if ( nscan <= 2 )           // no optional text
                ClearString ( mname );


            for ( el = 0; el < EEGDoc->GetTotalElectrodes(); el++ )

                if ( StringIs ( GetElectrodeName ( el, false ), el1 ) ) {

                    Montage[ mi ].El1      = el;            // store both electrode index
                    Montage[ mi ].ToData1  = EegBuff[ el ]; // and a direct pointer to data buffer
                    break;
                    }
                                        // first electrode not valid
            if ( ! Montage[ mi ].ToData1 )    continue;

                                        // another electrode?
            if ( nscan >= 2 ) {

                for ( el = 0; el < EEGDoc->GetTotalElectrodes(); el++ )

                    if ( StringIs ( GetElectrodeName ( el, false ), el2 ) ) {

                        Montage[ mi ].El2      = el;
                        Montage[ mi ].ToData2  = EegBuff[ el ];
                        break;
                        }
                                        // second electrode not valid
                if ( ! Montage[ mi ].ToData2 )    continue;
                }

            else {
                Montage[ mi ].El2      = Montage[ mi ].El1;
                }

                                        // use the given name
            if ( StringIsNotEmpty ( mname ) )

                StringCopy ( Montage[ mi ].Name, mname, MONTAGE_MAXCHARELECNAME - 1 );

            else                        // otherwise give it myself
                if ( nscan == 2 )
                    sprintf ( Montage[ mi ].Name, "%s-%s", el1, el2 );
                else
                    sprintf ( Montage[ mi ].Name, "%s", el1 );

                                        // update auxs
            if ( IsElectrodeNameAux ( GetElectrodeName ( Montage[ mi ].El1, false ) )
              || IsElectrodeNameAux ( GetElectrodeName ( Montage[ mi ].El2, false ) )
              || StringIsNotEmpty ( mname ) && IsElectrodeNameAux ( mname ) )

                AuxTracks.Set ( mi );

                                        // ready for next montage slot
            mi++;

            } while ( ! ifm.eof () && mi <= EEGDoc->GetNumElectrodes() );


                                        // oops, montage appears empty after scan?
        if ( mi == 0 ) {

            Montage.Reset   ();

            SetTextMargin   ();

            ResetScaleTracks();

            SetPartWindows  ( SelTracks );

            AuxTracks.Reset ();

            return;
            }

        else {

            SetTextMargin ();

            EEGDoc->SetReferenceType ( ReferenceAsInFile );

            EEGDoc->ClearRegular ( SelTracks );
            SelTracks.Set ( 0, mi - 1 );    // show only the number of montaged tracks

            ReloadBuffers    ();

            ResetScaleTracks ();

            SetPartWindows   ( SelTracks );

            EEGDoc->SetAuxTracks ( &AuxTracks );
            }

        return;                         // don't tell anybody else!

    } // switch w


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_REF );

SetColorTable ( EEGDoc->GetAtomType ( AtomTypeUseCurrent ) );

RefreshLinkedWindow () ;

//ButtonGadgetSetState ( IDB_SETAVGREF, EEGDoc->GetReferenceType () == ReferenceAverage );

ResetScaleTracks ();

Invalidate ( false );
}


void    TTracksView::CmSetReferenceNoneEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( ! (bool) Montage && EEGDoc->GetReferenceType () == ReferenceNone );
}


void    TTracksView::CmSetReferenceAvgEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( ! (bool) Montage && EEGDoc->GetReferenceType () == ReferenceAverage );
}


void    TTracksView::CmSetReferenceSingleEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( ! (bool) Montage && EEGDoc->GetReferenceType () == ReferenceArbitraryTracks && EEGDoc->GetReferenceTracks ().NumSet () == 1 );
}


void    TTracksView::CmSetReferenceMultipleEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( ! (bool) Montage && EEGDoc->GetReferenceType () == ReferenceArbitraryTracks && EEGDoc->GetReferenceTracks ().NumSet () > 1 );
}


void    TTracksView::CmSetReferenceMontageEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( (bool) Montage );
}


//----------------------------------------------------------------------------
void    TTracksView::CmSetScaling ( owlwparam w )
{
if ( w == CM_EEGUSERSCALE ) {           // set global scaling

    double              scaling;

    if ( ! GetValueFromUser ( "Give an absolute value, or 0 to reset:", "New Scaling", scaling, "", this ) )
        return;

    scaling     = fabs ( scaling );

    if ( scaling == 0 )
        scaling     = EEGDoc->GetAbsMaxValue ();

    scaling         = WindowSlots[0].ToUp.Norm() / ( scaling * ScaleTracks[ 0 ] * 2 );

    SetScaling ( scaling );

    }
else if ( w == CM_EEGRESETSCALING ) {   // reset relative tracks scaling

    if ( (bool) Highlighted )   ResetScaleTracks ( &Highlighted );
    else                        ResetScaleTracks ();
                                        // reset global scaling, too
    ResetScalingLevel ();
    }


Invalidate ( false );

RefreshLinkedWindow () ;
}


void    TTracksView::CmSetOffset ( owlwparam w )
{
if ( w == CM_EEGSETOFFSET ) {           // set offset

    if ( ! GetValueFromUser ( "Give a new offset value:", "New Offset", OffsetTracks, "", this ) )
        return;
    }

else if ( w == CM_EEGRESETOFFSET ) {    // reset offset

    OffsetTracks = 0;
    }


//Invalidate ( false );
ShowNow ();

RefreshLinkedWindow () ;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    TTracksView::CmShowTags ()
{
ShowTags    = ! ShowTags;

ButtonGadgetSetState ( IDB_SHOWMARKERS, ShowTags );

Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TTracksView::GoToMarker ( const TMarker* marker, bool centerextended )
{
                                        // out of current display ?
if ( marker->From < CDPt.GetMin() || marker->From > CDPt.GetMax()
  || marker->To   < CDPt.GetMin() || marker->To   > CDPt.GetMax() ) {


    if ( centerextended && marker->IsExtended ()
    || ! centerextended && marker->Length () >= CDPt.GetLength() ) // only markers that don't fit entirely

        CDPt.SetMin ( ( marker->From + marker->To ) / 2 - CDPt.GetLength() / 2, true );

    else                                // have more room on one side of the window
//      if ( marker->From < CDPt.GetMin() )
//          CDPt.SetMin ( ( marker->From + marker->To ) / 2 - CDPt.GetLength() * 9 / 10.0, true );
//      else
//          CDPt.SetMin ( ( marker->From + marker->To ) / 2 - CDPt.GetLength()     / 10.0, true );
                                        // always in the middle
        CDPt.SetMin ( ( marker->From + marker->To ) / 2 - CDPt.GetLength() * 0.50, true );


    CheckCDPtAndTFCursor ( false );


    if ( TFCursor.IsExtending () )
        TFCursor.SetExtendingPos ( marker->From );
    else
        TFCursor.SetPos ( marker->From, marker->To );


    ReloadBuffers ();

    //Invalidate ( false );
    ShowNow ();
    }
else {
    if ( TFCursor.IsExtending() )
        TFCursor.SetExtendingPos ( marker->From );
    else
        TFCursor.SetPos ( marker->From, marker->To );


    if ( SyncCDPtTFCursor ) {
        CheckCDPtAndTFCursor ( true );
        //Invalidate ( false );
        ShowNow ();
        }
    else
        DrawTFCursor ( true );
    }


EEGDoc->NotifyFriendViews ( LinkedViewId, vnNewTFCursor, (TParam2) &TFCursor, this );
}


void    TTracksView::CmPreviousNextMarker ( owlwparam w )
{
const MarkersList&  markers         = EEGDoc->GetMarkersList ();
const TMarker*      tolastmarker    = LastTagIndex >= 0 && LastTagIndex < EEGDoc->GetNumMarkers () ? markers[ LastTagIndex ] : 0;

                                        // we need an exhaustive search from current cursor position if there is no known last trigger
if ( ! (     tolastmarker != 0
        && IsFlag ( tolastmarker->Type, DisplayMarkerType )
        && tolastmarker->From == TFCursor.GetPosMin ()
        && tolastmarker->To   == TFCursor.GetPosMax () ) ) {

    if ( w == IDB_NEXTMARKER ) {
                                        // scan all triggers
        for ( int i = 0; i < markers.Num (); i++ ) {

            if ( ! IsFlag ( markers[ i ]->Type, DisplayMarkerType ) )
                continue;

            if ( ! ( markers[ i ]->From >  TFCursor.GetPosMin ()
                  || markers[ i ]->From == TFCursor.GetPosMin () && markers[ i ]->To > TFCursor.GetPosMax () ) )
                continue;

            if ( (bool) GrepMarkerFilter && ! GrepMarkerFilter.Matched ( markers[ i ]->Name ) )
                continue;

            LastTagIndex    = i;

            GoToMarker ( markers[ i ], false );

            return;
            } // for marker
                                        // no next marker? get out of here
        LastTagIndex    = -1;
        return;
        }

    else {
                                        // backward
        for ( int i = markers.Num () - 1; i >= 0; i-- ) {

            if ( ! IsFlag ( markers[ i ]->Type, DisplayMarkerType ) )
                continue;

            if ( ! ( markers[ i ]->To <  TFCursor.GetPosMax ()
                  || markers[ i ]->To >= TFCursor.GetPosMax () && markers[ i ]->From < TFCursor.GetPosMin () ) )
                continue;

            if ( (bool) GrepMarkerFilter && ! GrepMarkerFilter.Matched ( markers[ i ]->Name ) )
                continue;

            LastTagIndex    = i;

            GoToMarker ( markers[ i ], false );

            return;
            } // for marker
                                        // no previous marker? get out of here
        LastTagIndex    = -1;
        return;
        }

    } // exhaustive search


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // cursor has not changed since last jump, so simply go to the next index
const TMarker*      tomarker;
int                 lastmarkerindex = LastTagIndex; // save last known good marker index
bool                notmarkertype   = false;
bool                notposition     = false;
bool                notfilter       = false;

                                        // use the index to jump, but avoid standing still in case of overlapping markers!
do {
    LastTagIndex   += TrueToPlus ( w == IDB_NEXTMARKER );

    Clipped ( LastTagIndex, 0, EEGDoc->GetNumMarkers () - 1 );

    tomarker        = markers[ LastTagIndex ];


    notmarkertype   = ! IsFlag ( tomarker->Type, DisplayMarkerType );
    if ( notmarkertype )
        continue;


    notposition     =    tomarker->From == TFCursor.GetPosMin ()
                      && tomarker->To   == TFCursor.GetPosMax ();
    if ( notposition )
        continue;


    notfilter       = (bool) GrepMarkerFilter && ! GrepMarkerFilter.Matched ( tomarker->Name );

    if ( notfilter )
        continue;

                                        // found next!
    break;

    } while ( LastTagIndex != 0 && LastTagIndex != EEGDoc->GetNumMarkers () - 1 );


                                        // first/last marker stop may have stopped on a wrong type of marker type!
if ( LastTagIndex == 0 || LastTagIndex == EEGDoc->GetNumMarkers () - 1 )
    if ( notmarkertype || notfilter ) {
        LastTagIndex    = lastmarkerindex;
        tomarker        = markers[ LastTagIndex ];
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // finally!
GoToMarker ( tomarker, false );

}


void    TTracksView::CmTagsEnable ( TCommandEnabler &tce )
{
tce.Enable ( EEGDoc->GetNumMarkers ( DisplayMarkerType ) );
}


void    TTracksView::CmPrevNextTagEnable ( TCommandEnabler &tce )
{
tce.Enable ( ShowTags );
}


//----------------------------------------------------------------------------
void    TTracksView::CmAddMarker ()
{
AddMarker ();
}

                                        // static, so all windows share the user's predefined markers
static char         QuickMarkerStrings[ 9 ][ MarkerNameMaxLength ];

void    TTracksView::AddMarker ( int predefined )
{
char                name    [ MarkerNameMaxLength ];

                                        // using some numpad shortcuts
if ( IsInsideLimits ( predefined, 1, 9 ) ) {
        // ShiftKey does not work with the numpad...
    if ( /*ShiftKey ||*/ StringIsEmpty ( QuickMarkerStrings[ predefined ] ) )

        if ( ! GetInputFromUser ( "Give the new name:", "Adding Marker", QuickMarkerStrings[ predefined ], QuickMarkerStrings[ predefined ], this ) )

            return;

    StringCopy ( name, QuickMarkerStrings[ predefined ], MarkerNameMaxLength - 1 );
    }

else if ( (int) MarkerStrings == 1 )   // only 1 code available?
                                        // straightforward, don't ask anything
    StringCopy ( name, MarkerStrings[ 0 ] );

else {                                  // ask user to choose in a list
    TPickListPopup          pickl ( this, "Markers" );

    for ( int i = 0; i < (int) MarkerStrings; i++ )

        pickl.AddString ( MarkerStrings[ i ] );

    UpdateApplication;                  // there could be some keyboard inputs that might interfere

    pickl.Execute ();

    if ( pickl.GetResult () < 0 )
        return;

    StringCopy ( name, MarkerStrings[ pickl.GetResult () ] );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

EEGDoc->AppendMarker ( TMarker ( TFCursor.GetPosMin (), TFCursor.GetPosMax (), MarkerDefaultCode, name, MarkerTypeMarker ) );

EEGDoc->SortAndCleanMarkers ();

EEGDoc->CommitMarkers ();


ShowTags    = true;

ButtonGadgetSetState ( IDB_SHOWMARKERS, ShowTags );

EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );
}


//----------------------------------------------------------------------------
void    TTracksView::CmQuickMarkers ( owlwparam w )
{
if ( IsInsideLimits ( (int) w, CM_EEGMRKQUICK1, CM_EEGMRKQUICK9 ) )

    AddMarker ( w - CM_EEGMRKQUICK1 + 1);

else if ( w == CM_EEGMRKQUICKRESET ) {
    
    for ( int i = 0; i < 9; i++ )
        ClearString ( QuickMarkerStrings[ i ], MarkerNameMaxLength );
    }
}


//----------------------------------------------------------------------------
void    TTracksView::CmManageMarkers ()
{
                                        // build the list of current strings
TPickListPopup      pickl ( this, "Markers" );
int                 mi;

for ( int i = 0; i < (int) MarkerStrings; i++ )

    pickl.AddString ( MarkerStrings[ i ] );

                                        // and a dummy entry, to invite the user for input
pickl.AddString ( "->NEW MARKER NAME<-" );


pickl.Execute ();

if ( pickl.GetResult () < 0 )
    return;
else
    mi  = pickl.GetResult ();


char                buff[ MarkerNameMaxLength ];
                                        // now ask for new string
if ( ! GetInputFromUser ( "Give a new marker name, or let blank to remove it:", "Marker Name", buff, "", this ) )
    return;


if ( StringIsEmpty ( buff ) ) {         // clear marker?

    if ( (int) MarkerStrings == 1 || mi == (int) MarkerStrings )
        return;                         // don't delete last marker code

    MarkerStrings.RemoveRef ( MarkerStrings[ mi ] );
    }
else {                                  // replace
    if ( mi < (int) MarkerStrings )
        MarkerStrings.RemoveRef ( MarkerStrings[ mi ] );

    buff[ MarkerNameMaxLength - 1 ] = 0;
    MarkerStrings.Add    ( buff );
    }


MarkerStrings.Sort ();
}


//----------------------------------------------------------------------------
void    TTracksView::CmDuplicateTriggersToMarkers ()
{
constexpr char*     title           = "Duplicating Triggers to Markers";

if ( EEGDoc->GetNumMarkers () == 0 )
    return;


bool                allsessions     = EEGDoc->HasMultipleSessions () && GetAnswerFromUser ( "Applying to all sessions?", title, this );
int                 currsession     = EEGDoc->GetCurrentSession ();
int                 fromsession     = EEGDoc->HasMultipleSessions () ? ( allsessions ? 1                         : currsession ) : 0;
int                 tosession       = EEGDoc->HasMultipleSessions () ? ( allsessions ? EEGDoc->GetNumSessions () : currsession ) : 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int sessioni = fromsession; sessioni <= tosession; sessioni++ ) {

    EEGDoc->GoToSession ( sessioni );

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    const MarkersList&  markers         = EEGDoc->GetMarkersList ();
    TMarker             marker;

                                            // scan all triggers
    for ( int i = 0; i < markers.Num (); i++ ) {

        if ( ! IsFlag ( markers[ i ]->Type, MarkerTypeHardCoded ) )
            continue;
                                            // copy & change type
        marker          = *(markers[ i ]);
        marker.Type     = MarkerTypeMarker;

        EEGDoc->AppendMarker ( marker );
        }


    if ( EEGDoc->AreMarkersDirty () ) {     // does not check for MarkersInsertErrors errors

        EEGDoc->SortAndCleanMarkers ();
                                            // just commit once
        EEGDoc->CommitMarkers ();
                                            // assume we want to see the results!
        ShowTags    = true;

        if ( ! IsFlag ( DisplayMarkerType, MarkerTypeMarker ) )
            CmSetMarkerDisplay ( CM_EEGMRKMARKER );

        ButtonGadgetSetState ( IDB_SHOWMARKERS, ShowTags );

        EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );
        }
    }


if ( allsessions )
    EEGDoc->GoToSession ( currsession );
}


//----------------------------------------------------------------------------
void    TTracksView::CmSplitMarkers ()
{
constexpr char*     title           = "Bitwise Splitting Markers Names";

if ( EEGDoc->GetNumMarkers () == 0 ) {
    ShowMessage ( "There are no triggers or markers for you here!", "Splitting Markers", ShowMessageWarning );
    return;
    }


ulong               mask            = 0xFF;
char                buff[ 256 ];

sprintf ( buff, "%u", mask );

if ( ! GetInputFromUser ( "Give an integer value as a mask:", title, buff, buff, this ) )
    return;


mask    = (ulong) atol ( buff );

if ( ! mask )
    return;


bool                allsessions     = EEGDoc->HasMultipleSessions () && GetAnswerFromUser ( "Applying to all sessions?", title, this );
int                 currsession     = EEGDoc->GetCurrentSession ();
int                 fromsession     = EEGDoc->HasMultipleSessions () ? ( allsessions ? 1                         : currsession ) : 0;
int                 tosession       = EEGDoc->HasMultipleSessions () ? ( allsessions ? EEGDoc->GetNumSessions () : currsession ) : 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int sessioni = fromsession; sessioni <= tosession; sessioni++ ) {

    EEGDoc->GoToSession ( sessioni );

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    const MarkersList&  markers         = EEGDoc->GetMarkersList ();
    MarkersList         splitmarkers;

                                            // scan all triggers
    for ( int i = 0; i < markers.Num (); i++ ) {

        TMarker     marker      = *(markers[ i ]);

        if ( ! IsFlag ( marker.Type, DisplayMarkerType ) ) // ( MarkerTypeUserCoded | MarkerTypeHardCoded ) ) )
            continue;
                                            // convert name to integer
        long        code        = StringToLong ( marker.Name );
                                            // nothing?
        if ( ! code || ! ( code & mask ) )
            continue;
                                            // just change the type
        marker.Type       = MarkerTypeMarker;

                                            // now scan all of its bits
        for ( uint i = 0, b = 1; i < 32; i++, b <<= 1 )

            if ( code & mask & b ) {
                                            // just for consistency
                marker.Code    = (MarkerCode) b;
                                            // create new name
                sprintf ( marker.Name, "%u", b );

                splitmarkers.Append ( new TMarker ( marker ) );
                } // if b
        } // for i


    if ( splitmarkers.IsNotEmpty () ) {

        EEGDoc->InsertMarkers   ( splitmarkers );
                                            // just commit once
        EEGDoc->CommitMarkers   ();

                                            // assume we want to see the results!
        ShowTags    = true;

        if ( ! IsFlag ( DisplayMarkerType, MarkerTypeMarker ) )
            CmSetMarkerDisplay ( CM_EEGMRKMARKER );

        ButtonGadgetSetState ( IDB_SHOWMARKERS, ShowTags );

        EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );
        }
    }


if ( allsessions )
    EEGDoc->GoToSession ( currsession );
}


//----------------------------------------------------------------------------
void    TTracksView::CmRenameRegexpMarkers ()
{
constexpr char*     title           = "Regexp Renaming Markers";

if ( EEGDoc->GetNumMarkers ( MarkerTypeUserCoded ) == 0 ) {
    ShowMessage ( "There are no markers for you here!", title, ShowMessageWarning );
    return;
    }


bool                withintime      = TFCursor.IsSplitted ();
//                                 || ! GetAnswerFromUser ( "Do you wish to modify ALL markers?", title, this );


static char         search [ MarkerNameMaxLength ];
static char         replace[ MarkerNameMaxLength ];

if ( ! GetInputFromUser (   withintime ? "Regexp 'search' string, within current time range:" : "Regexp 'search' string, on the whole file:", 
                            title, 
                            search, search, 
                            this ) )
    return;

if ( StringIsEmpty ( search ) )
    return;


if ( ! GetInputFromUser ( "Regexp 'replace' string:", title, replace, replace, this ) )
    return;

if ( /* StringIsEmpty ( to ) ||*/ StringIs ( replace, search, CaseSensitive ) )
    return;


bool                allsessions     = ! withintime && EEGDoc->HasMultipleSessions () && GetAnswerFromUser ( "Applying to all sessions?", title, this );
int                 currsession     = EEGDoc->GetCurrentSession ();
int                 fromsession     = EEGDoc->HasMultipleSessions () ? ( allsessions ? 1                         : currsession ) : 0;
int                 tosession       = EEGDoc->HasMultipleSessions () ? ( allsessions ? EEGDoc->GetNumSessions () : currsession ) : 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Initialize grep search
TStringGrep         greppy ( search, replace, GrepOptionDefault );

if ( ! greppy.IsValid () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int sessioni = fromsession; sessioni <= tosession; sessioni++ ) {

    EEGDoc->GoToSession ( sessioni );

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    MarkersList&        markers         = EEGDoc->GetMarkersList ();
    char                name   [ 2 * MarkerNameMaxLength ];

                                            // scan all triggers
    for ( int i = 0; i < markers.Num (); i++ ) {

        TMarker*    tomarker    = markers[ i ];

        if ( ! IsFlag ( tomarker->Type, MarkerTypeUserCoded ) )
            continue;

        if ( withintime && tomarker->From > TFCursor.GetPosMax () )
            break;

        if ( withintime && ! IsInsideLimits ( tomarker->From, tomarker->To, TFCursor.GetPosMin (), TFCursor.GetPosMax () ) )
            continue;

                                            // copy to bigger temp variable
        StringCopy ( name, tomarker->Name );
    

        if ( greppy.SearchAndReplace ( name ) ) {
                                            // safely clip result
            StringCopy ( tomarker->Name, name, MarkerNameMaxLength - 1 );

            EEGDoc->SetMarkersDirty ();
            }
        } // for i


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( EEGDoc->AreMarkersDirty () ) {

        EEGDoc->CommitMarkers ();
                                            // assume we want to see the results!
        ShowTags    = true;

        if ( ! IsFlag ( DisplayMarkerType, MarkerTypeMarker ) )
            CmSetMarkerDisplay ( CM_EEGMRKMARKER );

        ButtonGadgetSetState ( IDB_SHOWMARKERS, ShowTags );

        EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );
        }
    }


if ( allsessions )
    EEGDoc->GoToSession ( currsession );
}


//----------------------------------------------------------------------------
void    TTracksView::CmRenameSubstringMarkers ()
{
constexpr char*     title           = "Substrings Renaming Markers";

if ( EEGDoc->GetNumMarkers ( MarkerTypeUserCoded ) == 0 ) {
    ShowMessage ( "There are no markers for you here!", title, ShowMessageWarning );
    return;
    }


bool                withintime      = TFCursor.IsSplitted ();
//                                 || ! GetAnswerFromUser ( "Do you wish to modify ALL markers?", title, this );


static char         from   [ MarkerNameMaxLength ];
static char         to     [ MarkerNameMaxLength ];

if ( ! GetInputFromUser (   withintime ? "Substring to be replaced (case sensitive), within current time range:" : "Substring to be replaced (case sensitive), on the whole file:", 
                            title, 
                            from, from, 
                            this ) )
    return;

if ( StringIsEmpty ( from ) )
    return;


if ( ! GetInputFromUser ( "Replaced by new substring (empty for deletion):", title, to, to, this ) )
    return;

if ( /* StringIsEmpty ( to ) ||*/ StringIs ( to, from, CaseSensitive ) )
    return;


bool                allsessions     = ! withintime && EEGDoc->HasMultipleSessions () && GetAnswerFromUser ( "Applying to all sessions?", title, this );
int                 currsession     = EEGDoc->GetCurrentSession ();
int                 fromsession     = EEGDoc->HasMultipleSessions () ? ( allsessions ? 1                         : currsession ) : 0;
int                 tosession       = EEGDoc->HasMultipleSessions () ? ( allsessions ? EEGDoc->GetNumSessions () : currsession ) : 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int sessioni = fromsession; sessioni <= tosession; sessioni++ ) {

    EEGDoc->GoToSession ( sessioni );

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    MarkersList&        markers         = EEGDoc->GetMarkersList ();
    char                name   [ 2 * MarkerNameMaxLength ];
    char                before [ MarkerNameMaxLength ];
    char                after  [ MarkerNameMaxLength ];

                                            // scan all triggers
    for ( int i = 0; i < markers.Num (); i++ ) {

        TMarker*    tomarker    = markers[ i ];

        if ( ! IsFlag ( tomarker->Type, MarkerTypeUserCoded ) )
            continue;

        if ( withintime && tomarker->From > TFCursor.GetPosMax () )
            break;

        if ( withintime && ! IsInsideLimits ( tomarker->From, tomarker->To, TFCursor.GetPosMin (), TFCursor.GetPosMax () ) )
            continue;


        StringCopy ( name, tomarker->Name );
        int         index       = 0;


        do {
            char*       toc;

            if ( ( toc = StringContains ( name + index, from, CaseSensitive ) ) == 0 )
                break;

            int         db  = toc - name;
            if ( db )       StringCopy  ( before, name, db );
            else            ClearString ( before );

            int         da  = StringLength ( name ) - ( ( toc - name ) + StringLength ( from ) );
            if ( da )       StringCopy  ( after, toc + StringLength ( from ), da );
            else            ClearString ( after );

            StringCopy  ( name, before, to, after );

            index = db + StringLength ( to );
            } while ( true );

                                            // update marker?
        if ( StringIsNot ( tomarker->Name, name, CaseSensitive ) ) {
                                            // directly poke into the marker
            StringCopy ( tomarker->Name, name, MarkerNameMaxLength - 1 );

            EEGDoc->SetMarkersDirty ();
            }
        } // for i


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( EEGDoc->AreMarkersDirty () ) {

        EEGDoc->CommitMarkers ();
                                            // assume we want to see the results!
        ShowTags    = true;

        if ( ! IsFlag ( DisplayMarkerType, MarkerTypeMarker ) )
            CmSetMarkerDisplay ( CM_EEGMRKMARKER );

        ButtonGadgetSetState ( IDB_SHOWMARKERS, ShowTags );

        EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );
        }

    }


if ( allsessions )
    EEGDoc->GoToSession ( currsession );
}


//----------------------------------------------------------------------------
void    TTracksView::CmRenameOverwriteMarker ()
{
constexpr char*     title           = "Overwriting Marker Names";

if ( EEGDoc->GetNumMarkers ( MarkerTypeUserCoded ) == 0 ) {
    ShowMessage ( "There are no markers for you here!", title, ShowMessageWarning );
    return;
    }

                                        // This could be quite brutal to overwrite all markers, better ask
bool                withintime      = TFCursor.IsSplitted ();
//                                   || ! GetAnswerFromUser ( "Do you wish to modify ALL markers?", title, this );

static char         newname[ 2 * MarkerNameMaxLength ];

if ( ! GetInputFromUser (   withintime ? "Give the new marker name, applied within current time range:" : "Give the new marker name, applied on the whole file:", 
                            title, 
                            newname, newname, 
                            this ) )
    return;

if ( StringIsEmpty ( newname ) )
    return;

                                        // clip that guy now
newname[ MarkerNameMaxLength - 1 ] = 0;


bool                allsessions     = ! withintime && EEGDoc->HasMultipleSessions () && GetAnswerFromUser ( "Applying to all sessions?", title, this );
int                 currsession     = EEGDoc->GetCurrentSession ();
int                 fromsession     = EEGDoc->HasMultipleSessions () ? ( allsessions ? 1                         : currsession ) : 0;
int                 tosession       = EEGDoc->HasMultipleSessions () ? ( allsessions ? EEGDoc->GetNumSessions () : currsession ) : 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int sessioni = fromsession; sessioni <= tosession; sessioni++ ) {

    EEGDoc->GoToSession     ( sessioni );

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    MarkersList&        markers         = EEGDoc->GetMarkersList ();

                                        // scan all triggers
    for ( int i = 0; i < markers.Num (); i++ ) {

        TMarker*    tomarker    = markers[ i ];

        if ( ! IsFlag ( tomarker->Type, MarkerTypeUserCoded ) )
            continue;

        if ( withintime && tomarker->From > TFCursor.GetPosMax () )
            break;

        if ( withintime && ! IsInsideLimits ( tomarker->From, tomarker->To, TFCursor.GetPosMin (), TFCursor.GetPosMax () ) )
            continue;


        if ( StringIsNot ( tomarker->Name, newname, CaseSensitive ) ) {

            StringCopy ( tomarker->Name, newname );

            EEGDoc->SetMarkersDirty ();
            }
        } // for markers


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( EEGDoc->AreMarkersDirty () ) {

        EEGDoc->CommitMarkers ();
                                        // assume we want to see the results!
        ShowTags    = true;

        if ( ! IsFlag ( DisplayMarkerType, MarkerTypeMarker ) )
            CmSetMarkerDisplay ( CM_EEGMRKMARKER );

        ButtonGadgetSetState ( IDB_SHOWMARKERS, ShowTags );

        EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );
        }
    }


if ( allsessions )
    EEGDoc->GoToSession ( currsession );
}


//----------------------------------------------------------------------------
void    TTracksView::CmMergeOverlappingMarkers ()
{
constexpr char*     title           = "Merging Overlapped Markers";

if ( EEGDoc->GetNumMarkers ( MarkerTypeUserCoded ) == 0 ) {
    ShowMessage ( "There are no markers for you here!", title, ShowMessageWarning );
    return;
    }

if ( ! GetAnswerFromUser ( "Are you sure you want to merge exactly overlapping markers (no undo) ?", title ) )
    return;


bool                allsessions     = EEGDoc->HasMultipleSessions () && GetAnswerFromUser ( "Applying to all sessions?", title, this );
int                 currsession     = EEGDoc->GetCurrentSession ();
int                 fromsession     = EEGDoc->HasMultipleSessions () ? ( allsessions ? 1                         : currsession ) : 0;
int                 tosession       = EEGDoc->HasMultipleSessions () ? ( allsessions ? EEGDoc->GetNumSessions () : currsession ) : 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int sessioni = fromsession; sessioni <= tosession; sessioni++ ) {

    EEGDoc->GoToSession ( sessioni );

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    TSuperGauge     Gauge;

    Gauge.Set       ( "Merging Markers" );

    Gauge.AddPart   ( 0, EEGDoc->GetNumMarkers () );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    const MarkersList&  oldmarkers      = EEGDoc->GetMarkersList ();
    MarkersList         newmarkers;

    bool                markersdirty    = false;
    char                newname     [ 10 * MarkerNameMaxLength ];

                                            // merge accounts only for user markers types only, the other ones are copied as is, even if they mix with a series of identical markers
    for ( int firsti = 0; firsti < oldmarkers.Num (); firsti++ ) {

        Gauge.SetValue ( 0, firsti );

        const TMarker*  firstmarker = oldmarkers[ firsti ];

                                            // not of proper type?
        if ( ! IsFlag ( firstmarker->Type, MarkerTypeUserCoded ) ) {

            newmarkers.Append ( new TMarker ( *firstmarker ) );

            continue;
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        int     countmerged     = 0;
        int     countskipped    = 0;
                                            // browse through following possible identical markers
        for ( int nexti = firsti + 1; nexti < oldmarkers.Num (); nexti++ ) {

            const TMarker*  nextmarker  = oldmarkers[ nexti ];

                                            // not of proper type?
            if ( ! IsFlag ( nextmarker->Type, MarkerTypeUserCoded ) ) {
                                            // might not be inserted at the right position, fix this with a final Sort
                newmarkers.Append ( new TMarker ( *nextmarker ) );

                countskipped++;             // we need to keep track of the irrelevant ones, too

                continue;
                }


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                            // same position and same type (by safety)?
            if ( nextmarker->From == firstmarker->From 
              && nextmarker->To   == firstmarker->To  
              && nextmarker->Type == firstmarker->Type  ) {
                                            // first merge? init with first name
                if ( countmerged == 0 )
                    StringCopy  ( newname, firstmarker->Name );

                                            // append next name
                if ( StringIsNot ( nextmarker->Name, firstmarker->Name ) )
                    StringAppend    ( newname, " ", nextmarker->Name );

                                            // counting number of successive identical markers
                countmerged++;
                } // identical
            else

                break; // not identical - stop the loop

            } // for nexti


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                            // nothing merged?
        if ( countmerged == 0 ) {
                                            // simply copy
            newmarkers.Append ( new TMarker ( *firstmarker ) );
            }

        else {                              // some merged

            if ( StringLength ( newname ) >= MarkerNameMaxLength )
                StringShrink ( newname, newname, MarkerNameMaxLength - 1 );

            newmarkers.Append ( new TMarker ( firstmarker->From, firstmarker->To, firstmarker->Code, newname, firstmarker->Type ) );
                                            // global flag that we actually did something
            markersdirty   = true;
            }

                                            // skip the (possible) merged and the (possible) irrelevant markers
        firsti         += countmerged + countskipped;

        } // for firsti


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( markersdirty ) {

        EEGDoc->SetMarkers      ( newmarkers ); // will sort & clean markers

        EEGDoc->CommitMarkers   ();

                                            // assuming user wants to see the results..
        ShowTags    = true;

        if ( ! IsFlag ( DisplayMarkerType, MarkerTypeMarker ) )
            CmSetMarkerDisplay ( CM_EEGMRKMARKER );

        ButtonGadgetSetState ( IDB_SHOWMARKERS, ShowTags );

        EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );
        }


    Gauge.Finished ();
    }


if ( allsessions )
    EEGDoc->GoToSession ( currsession );
}


//----------------------------------------------------------------------------
void    TTracksView::CmMergeContiguousMarkers ()
{
constexpr char*     title           = "Merging Contiguous Markers";

if ( EEGDoc->GetNumMarkers ( MarkerTypeUserCoded ) == 0 ) {
    ShowMessage ( "There are no markers for you here!", title, ShowMessageWarning );
    return;
    }

if ( ! GetAnswerFromUser ( "Are you sure you want to merge identical," NewLine "contiguous markers (no undo) ?", title ) )
    return;


bool                allsessions     = EEGDoc->HasMultipleSessions () && GetAnswerFromUser ( "Applying to all sessions?", title, this );
int                 currsession     = EEGDoc->GetCurrentSession ();
int                 fromsession     = EEGDoc->HasMultipleSessions () ? ( allsessions ? 1                         : currsession ) : 0;
int                 tosession       = EEGDoc->HasMultipleSessions () ? ( allsessions ? EEGDoc->GetNumSessions () : currsession ) : 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int sessioni = fromsession; sessioni <= tosession; sessioni++ ) {

    EEGDoc->GoToSession ( sessioni );

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compact identical markers, resulting markers could be of any length
    EEGDoc->CompactConsecutiveMarkers ( true );


    if ( EEGDoc->AreMarkersDirty () ) {

        EEGDoc->CommitMarkers ();

                                        // assume we want to see the results!
        ShowTags    = true;

        if ( ! IsFlag ( DisplayMarkerType, MarkerTypeMarker ) )
            CmSetMarkerDisplay ( CM_EEGMRKMARKER );

        ButtonGadgetSetState ( IDB_SHOWMARKERS, ShowTags );

        EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );
        }
    }


if ( allsessions )
    EEGDoc->GoToSession ( currsession );
}


//----------------------------------------------------------------------------
void    TTracksView::CmSearchMarker ( owlwparam w )
{
constexpr char*     title           = "Searching Marker";


if ( w == CM_EEGMRKSEARCH || StringIsEmpty ( MarkerSearch ) ) {
                                        // if filtering the display, provide a hint to the user when current search is empty
    if ( (bool) GrepMarkerFilter && StringIsEmpty ( MarkerSearch ) )
        StringCopy ( MarkerSearch, MarkerFilter );

    do {
        if ( ! GetInputFromUser ( "Give a text expression to be searched (Perl regular expression):", title, MarkerSearch, MarkerSearch, this ) )
            return;

//        if ( StringEndsWith ( MarkerSearch, "/i" ) ) {
//            StringReplace ( MarkerSearch, "/i", "" );
//            StringPrepend ( MarkerSearch, "(?i)" );
//            }

        GrepMarkerSearch.Set ( MarkerSearch, GrepOptionDefaultMarkers );

        } while ( ! ( StringIsEmpty ( MarkerSearch ) || (bool) GrepMarkerSearch ) );

    }

                                        // still nothing to search?
if ( StringIsEmpty ( MarkerSearch ) )
    return;

                                        // get cursor position
long                tfmin;
long                tfmax;
TFCursor.GetPos ( tfmin, tfmax );


const MarkersList&  markers         = EEGDoc->GetMarkersList ();


if ( w == CM_EEGMRKSEARCH || w == CM_EEGMRKSEARCHNEXT ) {

                                        // scan all triggers
    for ( int i = 0; i < markers.Num (); i++ ) {

        if ( ! ( markers[ i ]->From >  tfmin
              || markers[ i ]->From == tfmin && markers[ i ]->To > tfmax ) )
            continue;

                                        // restrict search to current display filtering
        if ( (bool) GrepMarkerFilter && ! GrepMarkerFilter.Matched ( markers[ i ]->Name )
          || (bool) GrepMarkerSearch && ! GrepMarkerSearch.Matched ( markers[ i ]->Name ) )
            continue;


        GoToMarker ( markers[ i ], true );

        return;
        } // for marker

    ShowMessage ( (bool) GrepMarkerFilter ? "No next marker!\n\nNote that your search is restricted by the Marker filtering display" : "No next marker!", title );
    }

else if ( w == CM_EEGMRKSEARCHPREVIOUS ) {

                                        // backward
    for ( int i = markers.Num () - 1; i >= 0; i-- ) {

        if ( ! ( markers[ i ]->To <  tfmax
              || markers[ i ]->To >= tfmax && markers[ i ]->From < tfmin ) )
            continue;

                                        // restrict search to current display filtering
        if ( (bool) GrepMarkerFilter && ! GrepMarkerFilter.Matched ( markers[ i ]->Name )
          || (bool) GrepMarkerSearch && ! GrepMarkerSearch.Matched ( markers[ i ]->Name ) )
            continue;


        GoToMarker ( markers[ i ], true );

        return;
        } // for marker

    ShowMessage ( (bool) GrepMarkerFilter ? "No previous marker!\n\nNote that your search is restricted by the Marker filtering display" : "No previous marker!", title );
    }

}


//----------------------------------------------------------------------------
void    TTracksView::CmSetMarkerFilter ()
{
                                        // if currently searching for markers, provide a hint to the user for filtering the display
if ( StringIsNotEmpty ( MarkerSearch ) && StringIsEmpty ( MarkerFilter ) )
    StringCopy ( MarkerFilter, MarkerSearch );


do {
    if ( ! GetInputFromUser ( "Give a text expression to filter current markers (Perl regular expression),\nor empty to reset filter:", 
                              "Filtering Marker Display", MarkerFilter, MarkerFilter, this ) )
        return;

    StringCleanup        ( MarkerFilter );  // leading/ending spaces are unlikely desirable

    GrepMarkerFilter.Set ( MarkerFilter, GrepOptionDefaultMarkers );

    } while ( ! ( StringIsEmpty ( MarkerFilter ) || (bool) GrepMarkerFilter ) );


                                        // assume we always want to see the results!
ShowTags                = true; // (bool) GrepMarkerFilter;

ButtonGadgetSetState ( IDB_SHOWMARKERS, ShowTags );

EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );
}


//----------------------------------------------------------------------------
void    TTracksView::AddFileMarkers ( char *file )
{
if ( StringIsEmpty ( file ) )
    return;


EEGDoc->InsertMarkers ( file );

EEGDoc->CommitMarkers ();


ShowTags    = true;

ButtonGadgetSetState ( IDB_SHOWMARKERS, ShowTags );

EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );
}


//----------------------------------------------------------------------------
void    TTracksView::CmAddFileMarkers ()
{
static GetFileFromUser  getfile ( "Open Markers", AllMarkersFilesFilter, 1, GetFileRead );


if ( ! getfile.Execute () )
    return;


AddFileMarkers ( getfile );
}


//----------------------------------------------------------------------------
void    TTracksView::CmDeleteMarker ()
{
if ( ! GetAnswerFromUser ( "Are you sure you want to delete the markers within cursor range?" NewLine "(can not be undone)", "Deleting Markers by Range" ) )
    return;


EEGDoc->RemoveMarkers ( TFCursor.GetPosMin(), TFCursor.GetPosMax(), MarkerTypeMarker );

EEGDoc->CommitMarkers ();


ShowTags    = true;

ButtonGadgetSetState ( IDB_SHOWMARKERS, ShowTags );

EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );
}


//----------------------------------------------------------------------------
void    TTracksView::CmDeleteMarkersByName ()
{
constexpr char*     title           = "Deleting Markers by Name";

if ( EEGDoc->GetNumMarkers ( MarkerTypeUserCoded ) == 0 ) {
    ShowMessage ( "There are no markers for you here!", title, ShowMessageWarning );
    return;
    }


static char         regexp[ MarkerNameMaxLength ];

if ( ! GetInputFromUser ( "Give a text expression of the marker names to delete (Perl regular expression)" NewLine "(like 'marker', 'marker(1|2)', 'ma?rk(er)?'):", title, regexp, regexp, this ) )
    return;

if ( StringIsEmpty ( regexp ) )
    return;


bool                allsessions     = EEGDoc->HasMultipleSessions () && GetAnswerFromUser ( "Applying to all sessions?", title, this );
int                 currsession     = EEGDoc->GetCurrentSession ();
int                 fromsession     = EEGDoc->HasMultipleSessions () ? ( allsessions ? 1                         : currsession ) : 0;
int                 tosession       = EEGDoc->HasMultipleSessions () ? ( allsessions ? EEGDoc->GetNumSessions () : currsession ) : 0;


if ( ! GetAnswerFromUser ( "Are you sure you want to delete these markers?" NewLine "(can not be undone)", title ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int sessioni = fromsession; sessioni <= tosession; sessioni++ ) {

    EEGDoc->GoToSession     ( sessioni );

    EEGDoc->RemoveMarkers   ( regexp, MarkerTypeUserCoded );

    EEGDoc->CommitMarkers   ();
    }


if ( allsessions )
    EEGDoc->GoToSession ( currsession );


//ShowTags    = false;
//
//ButtonGadgetSetState ( IDB_SHOWMARKERS, ShowTags );

EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );
}


//----------------------------------------------------------------------------
void    TTracksView::CmSetMarkerFile ()
{
static GetFileFromUser  getfile ( "Open Markers", AllMarkersFilesFilter, 1, GetFileRead );


if ( ! getfile.Execute () )
    return;


EEGDoc->CommitMarkers   ( false, Interactive );

EEGDoc->InitMarkers     ( getfile );

EEGDoc->CommitMarkers   ();


ShowTags    = true;

ButtonGadgetSetState ( IDB_SHOWMARKERS, ShowTags );

EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );
}


//----------------------------------------------------------------------------
void    TTracksView::CmClearMarkers ()
{
constexpr char*     title           = "Deleting All Markers";

if ( ! GetAnswerFromUser ( "Are you sure you want to delete ALL markers?" NewLine "(can not be undone)", title ) )
    return;


bool                allsessions     = EEGDoc->HasMultipleSessions () && GetAnswerFromUser ( "Applying to all sessions?", title, this );
int                 currsession     = EEGDoc->GetCurrentSession ();
int                 fromsession     = EEGDoc->HasMultipleSessions () ? ( allsessions ? 1                         : currsession ) : 0;
int                 tosession       = EEGDoc->HasMultipleSessions () ? ( allsessions ? EEGDoc->GetNumSessions () : currsession ) : 0;


for ( int sessioni = fromsession; sessioni <= tosession; sessioni++ ) {

    EEGDoc->GoToSession     ( sessioni );

    EEGDoc->RemoveMarkers   ( MarkerTypeMarker );

    EEGDoc->CommitMarkers   ();
    }


if ( allsessions )
    EEGDoc->GoToSession ( currsession );


ShowTags    = false;

ButtonGadgetSetState ( IDB_SHOWMARKERS, ShowTags );

EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );
}


//----------------------------------------------------------------------------
void    TTracksView::CmSetMarkerDisplay ( owlwparam w )
{
switch ( w ) {

    case    CM_EEGMRKNONE:                    DisplayMarkerType = MarkerTypeUnknown;    break;
    case    CM_EEGMRKTRIGGER:   XorFlags    ( DisplayMarkerType,  MarkerTypeTrigger );  break;
    case    CM_EEGMRKMARKER:    XorFlags    ( DisplayMarkerType,  MarkerTypeMarker  );  break;
    case    CM_EEGMRKEVENT:     XorFlags    ( DisplayMarkerType,  MarkerTypeEvent   );  break;
    case    CM_EEGMRKALL:
    default:                                  DisplayMarkerType = AllMarkerTypes;       break;
    }

ShowTags    = (bool) DisplayMarkerType;

ButtonGadgetSetState ( IDB_SHOWMARKERS, ShowTags );

Invalidate ( false );
//ShowNow ();
}


void    TTracksView::CmSetMarkerDisplayTEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( IsFlag ( DisplayMarkerType, MarkerTypeTrigger ) );
}


void    TTracksView::CmSetMarkerDisplayEEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( IsFlag ( DisplayMarkerType, MarkerTypeEvent ) );
}


void    TTracksView::CmSetMarkerDisplayMEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( IsFlag ( DisplayMarkerType, MarkerTypeMarker ) );
}


//----------------------------------------------------------------------------
void    TTracksView::CmScanForBadEpochs ()
{
char                answer[ 256 ];

if ( ! GetOptionsFromUser (     "How precise would you like the search to be:"          NewLine 
                                NewLine 
                                Tab "- (L)iberal " Tab Tab Tab Tab          "(more flagged as bad)"  NewLine 
                                Tab "- (R)egular " Tab Tab Tab Tab          "(best compromise)"      NewLine 
                                Tab "- (S)trict "  Tab Tab Tab Tab          "(less flagged as bad)"  NewLine 
                                Tab "- Specific tolerance, in [0..100]" Tab "(0: super-liberal; 100: super-strict)", 
                                BadEpochsTitle, "LRS+-.0123456789", "R", answer ) )
    return;


double              tolerance;          // typically in [0..1] - the amount of standard deviation of the Z-Score of each criterion

if      ( *answer == 'L' )  tolerance   = BadEpochsToleranceLiberal;
else if ( *answer == 'R' )  tolerance   = BadEpochsToleranceRegular;
else if ( *answer == 'S' )  tolerance   = BadEpochsToleranceStrict;
else                        tolerance   = AtLeast ( 0.0, StringToDouble ( answer ) / 100 ); // allow user to manually bypass the tolerance factor


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Control parameters
TFileName           markerprefix;

if ( ! GetInputFromUser ( "Prefix for the generated markers:", BadEpochsTitle, markerprefix, "Bad", this ) )
    return;

if ( StringIsEmpty ( markerprefix ) )
    StringCopy ( markerprefix, "Bad" );

                                        // force deleting existing markers
//char                regexp[ MarkerNameMaxLength ];
//StringCopy ( regexp, "(Bad|bad)" );
//EEGDoc->RemoveMarkers ( MarkerTypeUserCoded, regexp );
//EEGDoc->CommitMarkers ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                allsessions     = EEGDoc->HasMultipleSessions () && GetAnswerFromUser ( "Applying to all sessions?", BadEpochsTitle, this );
int                 currsession     = EEGDoc->GetCurrentSession ();
int                 fromsession     = EEGDoc->HasMultipleSessions () ? ( allsessions ? 1                         : currsession ) : 0;
int                 tosession       = EEGDoc->HasMultipleSessions () ? ( allsessions ? EEGDoc->GetNumSessions () : currsession ) : 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int sessioni = fromsession; sessioni <= tosession; sessioni++ ) {

    EEGDoc->GoToSession ( sessioni );

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    TMarkers            badepochs;
    TFileName           filename ( EEGDoc->GetDocPath () );

                                            // Search for bad epochs
    badepochs.BadEpochsToMarkers    (   0,      
                                        filename,           sessioni,
                                        tolerance, 
                                        markerprefix, 
                                        (bool) Highlighted  ? &Highlighted 
                                      : (bool) BadTracks    ? &BadTracks 
                                      :                       0
                                    );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                            // Insert to current document
    EEGDoc->InsertMarkers ( badepochs );
                                            // push the new markers to disk now
    EEGDoc->CommitMarkers ();

                                            // long story to show the results
    SetMarkerType ( MarkerTypeMarker );

    ShowTags            = EEGDoc->GetNumMarkers ( DisplayMarkerType );

    ButtonGadgetSetState ( IDB_SHOWMARKERS, ShowTags );

    EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );

    Invalidate ( false );
    }


if ( allsessions )
    EEGDoc->GoToSession ( currsession );
}


//----------------------------------------------------------------------------
void    TTracksView::CmGenerateMarkers ()
{
constexpr char*     GenerateAutoEpochsTitle     = "Generating Auto Markers";


char                buff[ 256 ];

if ( ! GetInputFromUser ( "Interval between consecutive markers, in whatever units you want (TF, ms, s, HH:mm:ss):", GenerateAutoEpochsTitle, buff, "1s", this ) )
    return;

double              intervalintf    = StringToTimeFrame ( buff, EEGDoc->GetSamplingFrequency () );

                                        // could also be an absolute origin, user should be able to specify that
if ( ! GetInputFromUser ( "Offset of the first marker, relative to the beginning of session (TF, ms, s, HH:mm:ss):", GenerateAutoEpochsTitle, buff, "0", this ) )
    return;

double              offsetintf      = StringToTimeFrame ( buff, EEGDoc->GetSamplingFrequency () );


char                name[ 256 ];

if ( ! GetInputFromUser ( "Name of the generated markers:", GenerateAutoEpochsTitle, name, "auto", this ) )
    return;



bool                allsessions     = EEGDoc->HasMultipleSessions () && GetAnswerFromUser ( "Applying to all sessions?", GenerateAutoEpochsTitle, this );
int                 currsession     = EEGDoc->GetCurrentSession ();
int                 fromsession     = EEGDoc->HasMultipleSessions () ? ( allsessions ? 1                         : currsession ) : 0;
int                 tosession       = EEGDoc->HasMultipleSessions () ? ( allsessions ? EEGDoc->GetNumSessions () : currsession ) : 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int sessioni = fromsession; sessioni <= tosession; sessioni++ ) {

    EEGDoc->GoToSession ( sessioni );

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    int                 timemin         = 0;                                // scan whole time range
    int                 timemax         = EEGDoc->GetNumTimeFrames () - 1;  // 
    double              tf              = timemin;
    TMarker             marker ( 0, 0, 100, name, MarkerTypeMarker );


    for ( int i = 0; tf <= timemax; i++ ) {
                                            // generate positions as multiples of the given interval, so that no rounding errors occur
        tf      = Truncate ( offsetintf + i * intervalintf );

        marker.Set ( tf );

        EEGDoc->AppendMarker ( marker );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( EEGDoc->AreMarkersDirty () ) {

        EEGDoc->SortAndCleanMarkers ();

        EEGDoc->CommitMarkers ();

                                            // assume we want to see the results!
        SetMarkerType ( MarkerTypeMarker );

        ShowTags    = true;

        ButtonGadgetSetState ( IDB_SHOWMARKERS, ShowTags );

        EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );

        Invalidate ( false );
        }
    }


if ( allsessions )
    EEGDoc->GoToSession ( currsession );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    TTracksView::CmFilter ()
{
                                        // is filtering allowed?
if ( ! EEGDoc->CanFilter () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                dialogok        = false;

if ( IsCommandSender () ) {
                                        // Optionally provide a .xyz or .spi file name if it is already available from a link many file
    bool            senderris       = dynamic_cast<TRisDoc*> ( CartoolApplication->LastActiveBaseView->BaseDoc );

                                        // pick the one coordinate file relevant to current file type
    if      (   senderris && GODoc && GODoc->GetNumSpDoc  () )  dialogok    = EEGDoc->SetFilters   ( 0, GODoc->GetSpDoc  ( CurrSp  )->GetDocPath () );
    else if ( ! senderris && GODoc && GODoc->GetNumXyzDoc () )  dialogok    = EEGDoc->SetFilters   ( 0, GODoc->GetXyzDoc ( CurrXyz )->GetDocPath () );
    else                                                        dialogok    = EEGDoc->SetFilters   ( 0 );
    }

else { // IsCommandReceiver ()          // commands cloning AND a receiving cloned view -> forward the parameters set by caller view
                                                                                   // view should be TTracksView
    TTracksDoc*         sendereeg   = dynamic_cast<TTracksDoc*> ( CartoolApplication->LastActiveBaseView->BaseDoc );

    if ( sendereeg ) {

        bool            senderris       = dynamic_cast<TRisDoc*> ( CartoolApplication->LastActiveBaseView->BaseDoc );
        bool            receiverris     = dynamic_cast<TRisDoc*> ( EEGDoc );

                                        // do not mix filters between EEG and RIS
        if ( senderris ^ receiverris )   return;

                                        // we can already forward some  TTracksFilters*
        dialogok    = EEGDoc->SetFilters   ( sendereeg->GetFilters (), 0 );
        }
    }

                                        // user cancelled dialog?
if ( ! dialogok )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // activate filters
EEGDoc->ActivateFilters ();

//ButtonGadgetSetState ( IDB_FILTER, EEGDoc->AreFiltersActivated () );
                                        // does change the display too much?
ResetScaleTracks    ();

ResetScalingLevel   ();

SetColorTable       ( EEGDoc->GetAtomType ( AtomTypeUseCurrent ) );
}


void    TTracksView::CmFilterEnable ( TCommandEnabler &tce )
{
tce.Enable ( EEGDoc->CanFilter () );
}


//----------------------------------------------------------------------------
void    TTracksView::CmShowSD ()
{
ShowSD      = NextState ( ShowSD, NumSDMode, ShiftKey );

                                        // refresh buffer, just in case
//if ( ShowSD )
if ( HasStandardDeviation () )
    EEGDoc->GetStandardDeviation ( CDPt.GetMin(), CDPt.GetMax(), SdBuff, 0, IsRoiMode () && Rois && AverageRois ? Rois : 0 );

ButtonGadgetSetState ( IDB_SHOWSD, ShowSD );

Invalidate ( false );

RefreshLinkedWindow () ;
}


void    TTracksView::CmShowSDEnable ( TCommandEnabler &tce )
{
tce.Enable ( EEGDoc->HasStandardDeviation () );
}


//----------------------------------------------------------------------------
void    TTracksView::RenderingModeToCurrent3DSet ()
{
                                        // choose 3D set here
if      ( RenderingMode == LayoutOneTrackOneBoxXyzFlat )    CurrentDisplaySpace     = DisplaySpaceProjected;
else if ( RenderingMode == LayoutOneTrackOneBoxXyz3D   )    CurrentDisplaySpace     = DisplaySpace3D;
else                                                        CurrentDisplaySpace     = DisplaySpaceNone;
}


void    TTracksView::CmSetRenderingMode ()
{
int         numrois         = IsRoiMode () ? Rois->NumSet () : 0;
int         numtracks       = IsRoiMode () ? numrois         : EEGDoc->GetNumSelectedRegular ( SelTracks );
int         m               = numtracks > MaxWindowSlots ? NumLayoutsTooMuchTracks : XYZDoc ? NumLayouts : NumLayoutsNoXyz;


RenderingMode   = NextState ( RenderingMode, m, ShiftKey );

RenderingModeToCurrent3DSet ();

InitXyz ();

Orientation         = DefaultPotentialOrientation - ( ShiftKey ? -1 : 1 );
CmOrient ();


//GLSize();                             // refresh the GL context, the 3D mode seems incompatible with the 2D...

SetPartWindows ( SelTracks );

RefreshLinkedWindow ( true ) ;
}


void    TTracksView::Set3DMode ( int current3dset )
{
                                        // choose 3D set here
if      ( current3dset == DisplaySpaceProjected
       || current3dset == DisplaySpaceHeight    )   RenderingMode = LayoutOneTrackOneBoxXyzFlat - ( ShiftKey ? -1 : 1 );
else if ( current3dset == DisplaySpace3D        )   RenderingMode = LayoutOneTrackOneBoxXyz3D   - ( ShiftKey ? -1 : 1 );
//else                                              RenderingMode = LayoutOnePage;
else return;


CmSetRenderingMode ();
}


//----------------------------------------------------------------------------
void    TTracksView::CmBaseline ()
{
ShowBaseline= ! ShowBaseline;

Invalidate ( false );

RefreshLinkedWindow () ;
}


//----------------------------------------------------------------------------
/*
void    TTracksView::CmResetFilling ()
{
bool    all = Highlighted.NumSet() == 0;

for ( int e=0; e < EEGDoc->GetTotalElectrodes (); e++ ) {
    if ( ! Highlighted[e] && ! all )
        continue;

    if ( Filling[e] ) {
        delete Filling[e];
        Filling[e] = 0;
        }
    }

IsFilling   = FillingNone;
for ( int e=0; e < EEGDoc->GetTotalElectrodes (); e++ )
    IsFilling |= ( Filling[e] != 0 );

Highlighted.Reset();
Invalidate ( false );
}
*/

//----------------------------------------------------------------------------
void    TTracksView::CmVoidFilling ()
{
if ( ! HasFilling )
    return;

IsFilling   = NextState ( IsFilling, NumFillingMode, ShiftKey );

Invalidate ( false );
}


void    TTracksView::CmFillingEnable ( TCommandEnabler &tce )
{
tce.Enable ( HasFilling );
}


//----------------------------------------------------------------------------
void    TTracksView::CmPenSize ()
{

if ( ! GetValueFromUser ( "Give the new line width, or 0 for Smart Width:", "Line Width", LineWidth, "0", this ) )
    return;

                                        // round-up & check limits
if ( LineWidth ) {
    LineWidth   = RoundTo ( LineWidth, GLLineWidthStep );

    Clipped ( LineWidth, GLLineWidthMin, GLLineWidthMax );
    }


Invalidate ( false );

RefreshLinkedWindow () ;
}


//----------------------------------------------------------------------------
void    TTracksView::CmSelection ( owlwparam w )
{
char                buff[ 4 * KiloByte ];
double              thr;
int                 e;
int                 tf;
TSelection          sel;


ClearString ( buff );


switch ( w ) {

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // General operations
  case  CM_KEEPSEL :
    SelTracks       = Highlighted;

    Highlighted.Reset ();
    SetPartWindows ( SelTracks );
    UpdateCaption ();
    break;

  case  CM_REMOVESEL :
    SelTracks      -= Highlighted;

    SetPartWindows ( SelTracks );
    Highlighted.Reset ();
    UpdateCaption ();
    break;

  case  CM_CLEARSEL :
    if ( (bool) Highlighted ) {
        Highlighted.Reset ();
        ShowNow ();
        }
    break;

  case  CM_SELECTALL :
    EEGDoc->SetRegular ( Highlighted );
    ShowNow ();
    break;

  case  CM_SELECTALLVISIBLE :
    Highlighted     = SelTracks;
    EEGDoc->ClearPseudo ( Highlighted );
    ShowNow ();
    break;

  case  CM_INVERTSEL :
    Highlighted.Invert ( EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () );
//    EEGDoc->ClearPseudo ( Highlighted );
    ShowNow ();
    break;

  case  CM_ABOVESEL :
    if ( ! GetValueFromUser ( "Absolute threshold above which tracks are selected:", "Selection", thr, "", this ) )
        return;

    thr = fabs ( thr );

    if ( thr <= 0 )
        break;

    long                tf1,    tf2;

    if ( TFCursor.IsSplitted () ) {
        TFCursor.GetPos ( tf1, tf2 );
        tf1 -= CDPt.GetMin();
        tf2 -= CDPt.GetMin();
        }
    else {
        tf1 = 0;
        tf2 = CDPt.GetLength();
        }

    for ( e = 0; e < EEGDoc->GetNumElectrodes (); e++ ) {
        if ( BadTracks[ e ] || AuxTracks[ e ] ) continue;

        for ( tf = tf1; tf < tf2 && !Highlighted[ e ]; tf++ )
            if ( fabs ( EegBuff[ e ][ tf ] ) > thr )
                Highlighted.Set ( e );
        }

    ShowNow ();
    break;

  case  CM_INPUTSEL :
    if ( ! GetInputFromUser ( "Tracks to be added to current selection (like: 1 2 30-40):", "Selection", buff, "", this ) )
        return;

    Highlighted.Set ( buff, GetElectrodesNames () );

    ShowNow ();
    break;


  case  CM_SHOWSELNAMES :
  case  CM_SHOWSELINDEXES :
                                        // use a local selection

    if      ( (bool) Highlighted ) {    // accept this selection as is
        sel                     = Highlighted;
        sel.ToText ( buff, w == CM_SHOWSELNAMES ? GetElectrodesNames () : 0 /*, "*"*/ );
        }

    else if ( (bool) SelTracks ) {      // do some checking
        sel                     = SelTracks;
                                            // if current selection mixes regular and pseudo, clear pseudos
        if ( EEGDoc->GetNumSelectedRegular ( sel ) && EEGDoc->GetNumSelectedPseudo  ( sel ) )
            EEGDoc->ClearPseudo ( sel );

        sel.ToText ( buff, w == CM_SHOWSELNAMES ? GetElectrodesNames () : 0 /*, "*"*/ );
        }

    else                                
        StringCopy ( buff, "" );


    GetInputFromUser ( "Your current selection:", w == CM_SHOWSELNAMES ? "Selected Tracks Names" : "Selected Tracks Indexes", buff, buff, this );
    break;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Bad tracks
  case  CM_BADSET :
    BadTracks      += Highlighted;
    EEGDoc->ClearPseudo ( BadTracks );

    ResetScaleTracks ();

    Highlighted.Reset ();
    EEGDoc->SetBadTracks ( &BadTracks );

//  CmSelection ( CM_BADEXCLUDE );      // remove from display, too
    break;

  case  CM_BADNONE :
    BadTracks.Reset ();

    EEGDoc->SetBadTracks ( &BadTracks );
    ResetScaleTracks ();

    EEGDoc->SetRegular ( SelTracks );   // refresh screen with all tracks
    SetPartWindows ( SelTracks );
    UpdateCaption ();
    break;

  case  CM_BADRESET :
    BadTracks      -= Highlighted;
    EEGDoc->ClearPseudo ( BadTracks );

    ResetScaleTracks ();

    Highlighted.Reset();
    EEGDoc->SetBadTracks ( &BadTracks );
    break;

  case  CM_BADKEEP :
    if ( (bool) Highlighted )
        Highlighted    -= ~ BadTracks;
    else if ( EEGDoc->GetNumSelectedRegular ( SelTracks ) ) {
        Highlighted     = SelTracks;
        Highlighted    -= ~ BadTracks;
        }
    else                                // a bit tricky: no displayed tracks -> use all bads
        Highlighted     = BadTracks;

                                        // don't touch to pseudos
    Highlighted.Copy ( EEGDoc->GetFirstPseudoIndex (), EEGDoc->GetLastPseudoIndex (), &SelTracks );

    CmSelection ( CM_KEEPSEL );
    break;

  case  CM_BADEXCLUDE :
    if ( (bool) Highlighted ) {
        Highlighted    &= BadTracks;
        CmSelection ( CM_REMOVESEL );
        }
    else if ( EEGDoc->GetNumSelectedRegular ( SelTracks ) ) {
        Highlighted     = SelTracks;
        Highlighted    &= BadTracks;
        CmSelection ( CM_REMOVESEL );
        }
    else {                              // a bit tricky: no displayed tracks -> restore all non-bads
        Highlighted     = ~ BadTracks;
        EEGDoc->ClearPseudo ( BadTracks );
        CmSelection ( CM_KEEPSEL );
        }

    break;

  case  CM_BADSELECT :
    if ( (bool) Highlighted )
        Highlighted    &= BadTracks;
    else if ( EEGDoc->GetNumSelectedRegular ( SelTracks ) ) {
        Highlighted     = SelTracks;
        Highlighted    &= BadTracks;
        }
    else                                // a bit tricky: no displayed tracks -> use all bads
        Highlighted  = BadTracks;

    ShowNow ();
    break;

  case  CM_BADDESELECT :
    if ( (bool) Highlighted ) {
        Highlighted    &= ~ BadTracks;
        EEGDoc->ClearPseudo ( Highlighted );
        }

    ShowNow ();
    break;

  case  CM_BADSELECTNON :
    if ( (bool) Highlighted )
        Highlighted    &= ~ BadTracks;
    else if ( EEGDoc->GetNumSelectedRegular ( SelTracks ) ) {
        Highlighted     = SelTracks;
        Highlighted    &= ~ BadTracks;
        }
    else                                // a bit tricky: no displayed tracks -> use all bads
        Highlighted     = ~ BadTracks;

    EEGDoc->ClearPseudo ( Highlighted );

    ShowNow ();
    break;

  case  CM_BADAUXSELECTNON :
    if ( (bool) Highlighted ) {
        Highlighted    &= ~ BadTracks;
        Highlighted    &= ~ AuxTracks;
        }
    else if ( EEGDoc->GetNumSelectedRegular ( SelTracks ) ) {
        Highlighted     = SelTracks;
        Highlighted    &= ~ BadTracks;
        Highlighted    &= ~ AuxTracks;
        }
    else {                              // a bit tricky: no displayed tracks -> use all bads/auxs
        Highlighted     = ~ BadTracks;
        Highlighted    &= ~ AuxTracks;
        }

    EEGDoc->ClearPseudo ( Highlighted );

    ShowNow ();
    break;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Auxiliary tracks
  case  CM_AUXSET :
    AuxTracks      += Highlighted;
    EEGDoc->ClearPseudo ( AuxTracks );

    ResetScaleTracks ();

    Highlighted.Reset ();
    EEGDoc->SetAuxTracks ( &AuxTracks );
    break;

  case  CM_AUXNONE :
    AuxTracks.Reset ();

    EEGDoc->SetAuxTracks ( &AuxTracks );
    ResetScaleTracks ();

    EEGDoc->SetRegular ( SelTracks );   // refresh screen with all tracks
    SetPartWindows ( SelTracks );
    UpdateCaption ();
    break;

  case  CM_AUXRESET :
    AuxTracks      -= Highlighted;
    EEGDoc->ClearPseudo ( AuxTracks );

    ResetScaleTracks ();

    Highlighted.Reset();
    EEGDoc->SetAuxTracks ( &AuxTracks );
    break;

  case  CM_AUXTODEFAULT :
    EEGDoc->ResetAuxTracks ();

    ResetScaleTracks ();

    EEGDoc->SetRegular ( SelTracks );   // refresh screen with all tracks
    SetPartWindows ( SelTracks );
    UpdateCaption ();
    break;

  case  CM_AUXKEEP :
    if ( (bool) Highlighted )
        Highlighted    -= ~ AuxTracks;
    else if ( EEGDoc->GetNumSelectedRegular ( SelTracks ) ) {
        Highlighted     = SelTracks;
        Highlighted    -= ~ AuxTracks;
        }
    else                                // a bit tricky: no displayed tracks -> use all bads
        Highlighted     = AuxTracks;

                                        // don't touch to pseudos
    Highlighted.Copy ( EEGDoc->GetFirstPseudoIndex (), EEGDoc->GetLastPseudoIndex (), &SelTracks );

    CmSelection ( CM_KEEPSEL );
    break;

  case  CM_AUXEXCLUDE :
    if ( (bool) Highlighted ) {
        Highlighted    &= AuxTracks;
        CmSelection ( CM_REMOVESEL );
        }
    else if ( EEGDoc->GetNumSelectedRegular ( SelTracks ) ) {
        Highlighted     = SelTracks;
        Highlighted    &= AuxTracks;
        CmSelection ( CM_REMOVESEL );
        }
    else {                              // a bit tricky: no displayed tracks -> restore all non-bads
        Highlighted     = ~ AuxTracks;
        EEGDoc->ClearPseudo ( AuxTracks );
        CmSelection ( CM_KEEPSEL );
        }
    break;

  case  CM_AUXSELECT :
    if ( (bool) Highlighted )
        Highlighted    &= AuxTracks;
    else if ( EEGDoc->GetNumSelectedRegular ( SelTracks ) ) {
        Highlighted     = SelTracks;
        Highlighted    &= AuxTracks;
        }
    else                                // a bit tricky: no displayed tracks -> use all bads
        Highlighted  = AuxTracks;

    ShowNow ();
    break;

  case  CM_AUXDESELECT :
    if ( (bool) Highlighted ) {
        Highlighted    &= ~ AuxTracks;
        EEGDoc->ClearPseudo ( Highlighted );
        }

    ShowNow ();
    break;

  case  CM_AUXSELECTNON :
    if ( (bool) Highlighted )
        Highlighted    &= ~ AuxTracks;
    else if ( EEGDoc->GetNumSelectedRegular ( SelTracks ) ) {
        Highlighted     = SelTracks;
        Highlighted    &= ~ AuxTracks;
        }
    else                                // a bit tricky: no displayed tracks -> use all bads
        Highlighted     = ~ AuxTracks;

    EEGDoc->ClearPseudo ( Highlighted );

    ShowNow ();
    break;

    }

                                        // send a message
//EEGDoc->NotifyFriendViews ( LinkedViewId, vnNewHighlighted, (TParam2) Highlighted, this );
EEGDoc->NotifyDocViews ( vnNewHighlighted, (TParam2) &Highlighted, this );

RefreshLinkedWindow ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    TTracksView::CmExportTracks ()
{

if ( (bool) Montage ) {
    ShowMessage ( "Can not export tracks while in montage mode.", ExportTracksTitle, ShowMessageWarning );
    return;
    }

                                        // use a common transfer buffer
TExportTracksStructEx&  transfer       = ExportTracksTransfer;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // use a local selection
TSelection      sel;


if      ( (bool) Highlighted ) {  // accept this selection as is
    sel                     = Highlighted;
    sel.ToText ( transfer.Tracks, GetElectrodesNames (), AuxiliaryTracksNames );
    transfer.ChannelsSel    = &sel;
    }

else if ( (bool) SelTracks ) {          // do some checking
    sel                     = SelTracks;
                                        // if current selection mixes regular and pseudo, clear pseudos
    if ( EEGDoc->GetNumSelectedRegular ( sel ) && EEGDoc->GetNumSelectedPseudo  ( sel ) )
        EEGDoc->ClearPseudo ( sel );

    sel.ToText ( transfer.Tracks, GetElectrodesNames(), AuxiliaryTracksNames );
    transfer.ChannelsSel    = &sel;
    }

else {                                  // no selection
    StringCopy  ( transfer.Tracks, "*" );
    transfer.ChannelsSel    = 0;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // time
if ( TFCursor.IsSplitted () ) {

    IntegerToString ( transfer.TimeMin, TFCursor.GetPosMin () );
    IntegerToString ( transfer.TimeMax, TFCursor.GetPosMax () );
    transfer.EndOfFile  = BoolToCheck ( false );
    }
else {

    IntegerToString ( transfer.TimeMin, 0 );
    IntegerToString ( transfer.TimeMax, EEGDoc->GetNumTimeFrames() - 1 );
    transfer.EndOfFile  = BoolToCheck ( true  );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // electrodes
if ( XYZDoc ) {

    StringCopy  ( transfer.XyzDocFile, XYZDoc->GetDocPath () );
    transfer.UseXyzDoc  = BoolToCheck ( true  );
    transfer.XyzLinked  = true;         // to prevent switching to another XYZ
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // switching between Tracks and ROIs
if ( IsRoiMode () && GODoc ) {

    transfer.ExportTracks   = BoolToCheck ( false );
    transfer.ExportRois     = BoolToCheck ( true  );
    StringCopy  ( transfer.RoisDocFile, (char *) GODoc->GetRoisDoc ( CurrRois )->GetDocPath () );
    transfer.RoiLinked      = true;         // to prevent switching to another ROI
    }
else if ( StringIsEmpty ( transfer.RoisDocFile ) ) {

    transfer.ExportTracks   = BoolToCheck ( true  );
    transfer.ExportRois     = BoolToCheck ( false );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // run the dialog, which will do everything by itself
TExportTracksDialog ( CartoolMdiClient, IDD_EXPORTTRACKS, EEGDoc ).Execute ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // clean-up before surrendering transfer buffer
transfer.ChannelsSel    = 0;
transfer.XyzLinked      = false;
transfer.RoiLinked      = false;
}


//----------------------------------------------------------------------------
void    TTracksView::CmFreqAnalysis ()
{

if ( (bool) Montage ) {
    ShowMessage ( "Can not analyse tracks while in montage mode.", "Frequency Analysis", ShowMessageWarning );
    return;
    }

                                        // use a common transfer buffer
TFrequencyAnalysisStructEx  &transfer   = FrequencyAnalysisTransfer;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // use a local selection
TSelection      sel;


if      ( (bool) Highlighted ) {  // accept this selection as is
    sel                     = Highlighted;
    sel.ToText ( transfer.Channels, GetElectrodesNames (), AuxiliaryTracksNames );
    transfer.ChannelsSel    = &sel;
    }

else if ( (bool) SelTracks ) {          // do some checking
    sel                     = SelTracks;
                                        // if current selection mixes regular and pseudo, clear pseudos
    if ( EEGDoc->GetNumSelectedRegular ( sel ) && EEGDoc->GetNumSelectedPseudo  ( sel ) )
        EEGDoc->ClearPseudo ( sel );

    sel.ToText ( transfer.Channels, GetElectrodesNames(), AuxiliaryTracksNames );
    transfer.ChannelsSel    = &sel;
    }

else {                                  // no selection
    StringCopy ( transfer.Channels, "*" );
    transfer.ChannelsSel    = 0;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // time
if ( TFCursor.IsSplitted() ) {
    sprintf ( transfer.TimeMin, "%0d", TFCursor.GetPosMin () );
    sprintf ( transfer.TimeMax, "%0d", TFCursor.GetPosMax () );
    transfer.EndOfFile  = BoolToCheck ( false );
    }
else {
    sprintf ( transfer.TimeMin, "%0d", 0 );
    sprintf ( transfer.TimeMax, "%0d", EEGDoc->GetNumTimeFrames() - 1 );
//  transfer.EndOfFile  = BoolToCheck ( true  );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( XYZDoc ) {                         // provide the coordinates for very first init
    StringCopy ( transfer.XyzDocFile, XYZDoc->GetDocPath () );
    transfer.UseXyzDoc  = BoolToCheck ( true  );
    transfer.XyzLinked  = true;         // to prevent switching to another XYZ
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // run the dialog, which will do everything by itself
TFrequencyAnalysisDialog ( CartoolMdiClient, IDD_FREQANALYSIS, EEGDoc ).Execute();


                                        // clean up
transfer.ChannelsSel    = 0;
transfer.XyzLinked      = false;
}


//----------------------------------------------------------------------------
void    TTracksView::CmInterpolateTracks ()
{

if ( (bool) Montage ) {
    ShowMessage ( "Can not interpolate tracks while in montage mode.", InterpolationTitle, ShowMessageWarning );
    return;
    }

                                        // use a common transfer buffer
TInterpolateTracksStructEx&    transfer   = InterpolateTracksTransfer;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if      ( (bool) Highlighted ) {        // take this whole selection as is

    transfer.BadElectrodesSelection     = Highlighted;

    transfer.BadElectrodesSelection.ToText ( transfer.FromBadElectrodes, GetElectrodesNames (), AuxiliaryTracksNames );
    }

else if ( (bool) BadTracks ) {          // perform some checks on this selection, though

    transfer.BadElectrodesSelection     = BadTracks;
                                        // if current selection mixes regular and pseudo, clear pseudos
    if (   EEGDoc->GetNumSelectedRegular ( transfer.BadElectrodesSelection ) 
        && EEGDoc->GetNumSelectedPseudo  ( transfer.BadElectrodesSelection ) )

        EEGDoc->ClearPseudo ( transfer.BadElectrodesSelection );

    transfer.BadElectrodesSelection.ToText ( transfer.FromBadElectrodes, GetElectrodesNames(), AuxiliaryTracksNames );
    }

//else {                                  // no selection - keep what the user might have typed in a previous call
//    ClearString ( transfer.FromBadElectrodes );
//    transfer.BadElectrodesSelection.Initialize ();
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( XYZDoc ) {                         // provide the coordinates for very first init

    StringCopy ( transfer.FromXyz, XYZDoc->GetDocPath () );
                                        // preventing switching to another XYZ
    transfer.FromXyzLinked  = true;
                                        // it is assumed user wants to just interpolate bad channels
    transfer.SameXyz        = BoolToCheck ( true  );
    transfer.AnotherXyz     = BoolToCheck ( false );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // run the dialog, which will do everything by itself
TInterpolateTracksDialog ( CartoolMdiClient, IDD_INTERPOLATE, EEGDoc ).Execute ();


                                        // clean up
transfer.BadElectrodesSelection.Initialize ();
transfer.FromXyzLinked  = false;
}


//----------------------------------------------------------------------------
void    TTracksView::CmRisToVolume ()
{

if ( typeid ( *EEGDoc ) != typeid ( TRisDoc ) ) {
    ShowMessage ( "Current file doesn't seem to be a proper Results of Inverse Solution one.\n\nEither convert it to .ris type, or make sure the file has the right extension...", RisToVolumeTitle, ShowMessageWarning );
    return;
    }


if ( (bool) Montage ) {
    ShowMessage ( "Can not convert to volume while in montage mode.", RisToVolumeTitle, ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // time
#define     DefaultNumberOfFiles        10

if ( TFCursor.IsSplitted () ) {
                                        // setting the actual time range to the cursor's
    IntegerToString ( RisToVolumeTransfer.TimeMin, TFCursor.GetPosMin () );
    IntegerToString ( RisToVolumeTransfer.TimeMax, TFCursor.GetPosMax () );
    IntegerToString ( RisToVolumeTransfer.StepTF,  TDownsampling ( TFCursor.GetLength (), DefaultNumberOfFiles ).Step );
                                        // switching to saving time interval
    RisToVolumeTransfer.TimeAll         = BoolToCheck ( false );
    RisToVolumeTransfer.TimeInterval    = BoolToCheck ( true  );
    }
else {
                                        // setting the time range correctly
    IntegerToString ( RisToVolumeTransfer.TimeMin, 0 );
    IntegerToString ( RisToVolumeTransfer.TimeMax, EEGDoc->GetNumTimeFrames() - 1 );
    IntegerToString ( RisToVolumeTransfer.StepTF,  TDownsampling ( EEGDoc->GetNumTimeFrames(), DefaultNumberOfFiles ).Step );
                                        // enabling the whole data set? currently doing nothing, i.e. keeping the current user's choice 
//  RisToVolumeTransfer.TimeAll         = BoolToCheck ( true  );
//  RisToVolumeTransfer.TimeInterval    = BoolToCheck ( false );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // run the dialog, which will do everything by itself
TRisToVolumeDialog ( CartoolMdiClient, IDD_RISTOVOLUME, EEGDoc ).Execute();

}


//----------------------------------------------------------------------------
void    TTracksView::RisToCloudVectorsUI ()
{
TRisDoc*            risdoc          = dynamic_cast< TRisDoc* > ( EEGDoc );


if ( ! ( risdoc && risdoc->IsVector ( AtomTypeUseOriginal ) ) ) {
    ShowMessage ( "Current file doesn't seem to be a proper vectorial RIS file!", RisToPointsTitle, ShowMessageWarning );
    return;
    }


if ( (bool) Montage ) {
    ShowMessage ( "Can not convert to volume while in montage mode.", RisToPointsTitle, ShowMessageWarning );
    return;
    }


bool                spontaneous     = GetAnswerFromUser ( "Spontaneous data?", RisToPointsTitle, this );
bool                normalize       = GetAnswerFromUser ( "Normalizing points (spherical vs. real cloud)?", RisToPointsTitle, this );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // use a local selection
TSelection          sel;


if      ( (bool) Highlighted )          // accept this selection as is

    sel                     = Highlighted;

else if ( (bool) SelTracks ) {          // do some checking

    sel                     = SelTracks;
                                        // if current selection mixes regular and pseudo, clear pseudos
    if ( risdoc->GetNumSelectedRegular ( sel ) && risdoc->GetNumSelectedPseudo  ( sel ) )
        risdoc->ClearPseudo ( sel );
    }

else                                    // no current selection, use all SPs

    risdoc->SetRegular ( sel );


if ( sel.NumSet () == risdoc->GetNumElectrodes () 
    && ! GetAnswerFromUser ( "Are you sure you want to output all solution points?", RisToPointsTitle, this ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // time
int                 tfmin           = TFCursor.IsSplitted () ? TFCursor.GetPosMin () : 0;
int                 tfmax           = TFCursor.IsSplitted () ? TFCursor.GetPosMax () : risdoc->GetNumTimeFrames() - 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Do it!
TGoF                outgof;

RisToCloudVectors   (   
                    risdoc->GetDocPath (),
                    tfmin,                  tfmax,
                    &sel,                   0,
                    spontaneous,            normalize,
                    outgof,
                    true
                    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( (int) outgof <= 10 )
    outgof.OpenFiles ();
}


//----------------------------------------------------------------------------
void    TTracksView::CmScanTriggers ()
{

if ( (bool) Montage ) {
    ShowMessage ( "Can not scan tracks while in montage mode.", ExportTracksTitle, ShowMessageWarning );
    return;
    }

                                        // use a common transfer buffer
TScanTriggersStruct&    transfer       = ScanTriggersTransfer;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // use a local selection
TSelection          sel;


if      ( (bool) Highlighted ) {

    Highlighted.ToText ( transfer.Channels, GetElectrodesNames (), AuxiliaryTracksNames );
    }

else if ( (bool) SelTracks ) {

    TSelection  sel ( SelTracks );

    if ( EEGDoc->GetNumSelectedRegular ( sel ) )
        EEGDoc->ClearPseudo ( sel );

    sel.ToText ( transfer.Channels, GetElectrodesNames (), AuxiliaryTracksNames );
    }

else {                                  // no selection
    StringCopy  ( transfer.Channels, "*" );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // time
if ( TFCursor.IsSplitted () ) {

    IntegerToString ( transfer.TimeMin, TFCursor.GetPosMin () );
    IntegerToString ( transfer.TimeMax, TFCursor.GetPosMax () );
    transfer.EndOfFile  = BoolToCheck ( false );
    }
else {

    IntegerToString ( transfer.TimeMin, 0 );
    IntegerToString ( transfer.TimeMax, EEGDoc->GetNumTimeFrames() - 1 );
    transfer.EndOfFile  = BoolToCheck ( true  );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // run the dialog, which will do everything by itself
TScanTriggersDialog ( this, IDD_SCANMARKERS, EEGDoc ).Execute ();

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    TTracksView::CmTimeRef ( owlwparam w )
{

if ( w == CM_EEGTIMERELATIVETO ) {

    char                message [ 256 ];
    char                current [ 256 ];
    char                buff    [ 256 ];

    StringCopy  (   message, 
                    "New time origin, with any of these syntaxes (strings in bracket are optional):" NewLine 
                    Tab "XXXX [ TF ]"   NewLine 
                    Tab "[ HH: ]MM:SS"  NewLine 
                    Tab "[ HH hour ] [ MM min ] [ SS sec ] [ mm msec ]" );


    FloatToString   ( current, TimeFrameToMilliseconds ( TFCursor.GetPosMin (), EEGDoc->GetSamplingFrequency () ) );
    StringAppend    ( current, " msec" );


    if ( ! GetInputFromUser ( message, "Set Relative Time", buff, current, this ) )
        return;

    double              timeorigin      = TimeFrameToMicroseconds ( StringToTimeFrame ( buff, EEGDoc->GetSamplingFrequency () ), EEGDoc->GetSamplingFrequency () );


    EEGDoc->DateTime.RelativeTime       = true;
    EEGDoc->DateTime.RelativeTimeOffset = - timeorigin;
    }
else if ( w == CM_EEGTIMERELATIVE ) {

    EEGDoc->DateTime.RelativeTime       = true;
    EEGDoc->DateTime.RelativeTimeOffset = 0;
    }
else
    EEGDoc->DateTime.RelativeTime       = false;

                                        // refresh owned titles
EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_CAPTION );

                                        // refresh linked views?
//EEGDoc->NotifyFriendViews ( LinkedViewId, vnReloadData, EV_VN_RELOADDATA_CAPTION, this );

                                        // ref to time also influences the cursor value
EEGDoc->NotifyFriendViews ( LinkedViewId, vnNewTFCursor, (TParam2) &TFCursor, this );


Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TTracksView::CmGotoTimeOrigin ()
{
TFCursor.SetPos ( TFCursor.AbsoluteMicrosecondsToRelativeTF ( 0 ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( TFCursor.GetPosMin() < CDPt.GetMin () || TFCursor.GetPosMin() > CDPt.GetMax () )
    CDPt.SetMin ( TFCursor.GetPosMin() - CDPt.GetLength() * 0.1, true );

if ( SyncCDPtTFCursor )
    CheckCDPtAndTFCursor ( true );
else
    ReloadBuffers ();
                                        // send the new cursor to friend views
EEGDoc->NotifyFriendViews ( LinkedViewId, vnNewTFCursor, (TParam2) &TFCursor, this );

Invalidate ( false );
}


void    TTracksView::CmGotoTime ()
{
long                gototf          = 0;


if ( IsCommandSender () ) {

    static char         answer  [ 256 ];
    char                message [ 256 ];
    bool                answerintf;

                                            // build a contextully dependent input message
    StringCopy  (   message, 
                    "Go to ", EEGDoc->DateTime.RelativeTime ? "relative" : "absolute", " time position, with any of these syntaxes (strings in bracket are optional):" NewLine
                );

    StringAppend(   message,
                    NewLine
                    Tab "XXXX [TF]"                             NewLine 
                    Tab "[HH:]MM:SS"                            NewLine 
                    Tab "[HH hour] [MM min] [SS sec] [mm msec]" NewLine
                    Tab "marker name"
                );


    if ( ! GetInputFromUser ( message, "GoTo Time", answer, answer, this ) )
        return;

                                        // convert user input
    gototf      = Truncate ( StringToTimeFrame ( answer, EEGDoc->GetSamplingFrequency (), &answerintf ) );

                                        // new alternative to Find Marker: allowing goto <marker_name>
    if ( gototf == 0 && EEGDoc->GetMarker ( answer ) ) {

        gototf  = EEGDoc->GetMarker ( answer )->Center ();
        } // marker syntax
    
    else { // time syntax
                                        // update with a shift only if NOT TF answer
        if ( ! answerintf ) {

            if ( EEGDoc->DateTime.RelativeTime ) {
                                        // arbitrary relative origin
                gototf     -= Truncate ( MicrosecondsToTimeFrame ( EEGDoc->DateTime.RelativeTimeOffset,        EEGDoc->GetSamplingFrequency () ) );
                }
    
            else {                      // account for an absolute residual offset in ms
                gototf     -= Truncate ( MicrosecondsToTimeFrame ( EEGDoc->DateTime.GetAbsoluteTimeOffset (),  EEGDoc->GetSamplingFrequency () ) );

                if ( gototf < 0 )
                                        // absolute time can wrap around midnight - add a (single) full day
                    gototf += DaysToTimeFrame ( 1, EEGDoc->GetSamplingFrequency () );
                }
            }
        } // time syntax
    }
else {                                  // commands cloning AND a receiving cloned view -> forward the parameters set by caller view
                      
    TTracksView*            orgview     = dynamic_cast<TTracksView*> ( CartoolApplication->LastActiveBaseView );

    if ( orgview )
                                        // retrieve current position
        gototf  = orgview->TFCursor.GetPosMin ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFCursor.SetPos ( gototf );


if ( TFCursor.GetPosMin () < CDPt.GetMin ()
  || TFCursor.GetPosMin () > CDPt.GetMax () )

    CDPt.SetMin ( TFCursor.GetPosMin() - CDPt.GetLength() * 0.1, true );


if ( SyncCDPtTFCursor )
    CheckCDPtAndTFCursor ( true );
else
    ReloadBuffers ();
                                        // send the new cursor to friend views
                                        // forwarding message is a bit tricky, as temporal messages are processed separately - let's try this
if ( GetCommandsCloning () || HasOtherFriendship () )
    EEGDoc->NotifyFriendViews ( LinkedViewId, vnNewTFCursor, (TParam2) &TFCursor, this );


Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TTracksView::CmSetSamplingFrequency ()
{
                                        // Sampling frequency is irrelevant?
if ( ! EEGDoc->CanFilter () )
    return;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char                buff[ 256 ];
double              samplingfrequency;


StringCopy ( buff, FloatToString ( EEGDoc->GetSamplingFrequency (), 2 ) );


if ( ! GetValueFromUser ( "New Sampling Frequency, in [Hz] (0 to reset):", "Sampling Frequency", samplingfrequency, buff, this ) )
    return;


if ( samplingfrequency == EEGDoc->GetSamplingFrequency () )
    return;

                                        // if <= 0, this will reset SF to 0
                                        // Note that this will sets the DirtySamplingFrequency flag if needed
EEGDoc->SetSamplingFrequency ( samplingfrequency );

                                        // not really mandatory, but better update filters right now
EEGDoc->GetFilters ()->SamplingFrequency    = EEGDoc->GetSamplingFrequency ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need to update this
EEGDoc->DateTime.MicrosecondPrecision   = IsMicrosecondPrecision ( EEGDoc->GetSamplingFrequency () );

                                        // changing the SF has an impact only on temporal filters, like f.ex. low-pass
if ( EEGDoc->AreFiltersActivated () && EEGDoc->GetFilters ()->HasTemporalFilter () ) {

    EEGDoc->NotifyDocViews  ( vnReloadData, EV_VN_RELOADDATA_EEG     );

    ResetScaleTracks ();
    }


Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TTracksView::CmShowDate ()
{
ShowDate    = ! ShowDate;
UpdateCaption ();
}


void    TTracksView::CmShowTime ()
{
ShowTime    = ! ShowTime;
UpdateCaption ();
}


void    TTracksView::CmShowColorScale ()
{
ShowColorScale  = ! ShowColorScale;

SetPartWindows ( SelTracks );
}


void    TTracksView::CmShowAll ( owlwparam w )
{
TBaseView::CmShowAll ( w );

SetPartWindows ( SelTracks );
}


bool    TTracksView::HasColorScale ()
{
return  ShowColorScale && NotSmallWindow () && IsIntensityModes ();
}


void    TTracksView::CmShowTimeEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( ShowTime );
tce.Enable   ( EEGDoc->GetSamplingFrequency () );
}


void    TTracksView::CmShowDateEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( ShowDate );
tce.Enable   ( EEGDoc->DateTime.IsOriginDateAvailable () );
}


void    TTracksView::CmTimeRefAbsEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( ! EEGDoc->DateTime.RelativeTime );
tce.Enable   ( EEGDoc->GetSamplingFrequency () && EEGDoc->DateTime.IsOriginTimeAvailable () );
}


void    TTracksView::CmTimeRefRelEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( EEGDoc->DateTime.RelativeTime && ! EEGDoc->DateTime.RelativeTimeOffset );
tce.Enable   ( EEGDoc->GetSamplingFrequency () );
}


void    TTracksView::CmTimeRefRelToEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( EEGDoc->DateTime.RelativeTime &&   EEGDoc->DateTime.RelativeTimeOffset );
tce.Enable   ( EEGDoc->GetSamplingFrequency () );
}


//----------------------------------------------------------------------------
void    TTracksView::CmSetTimeWindow ()
{
long                tr              = 0;

                                        // build a contextully dependent input message
if ( IsCommandSender () ) {

    char                message [ 256 ];
    char                buff    [ 256 ];

    StringCopy  (   message, 
                    "New time range per page, with any of these syntaxes (strings in bracket are optional):" NewLine 
                    Tab "XXXX [ TF ]"   NewLine 
                    Tab "[ HH: ]MM:SS"  NewLine 
                    Tab "[ HH hour ] [ MM min ] [ SS sec ] [ mm msec ]" );

    if ( ! GetInputFromUser ( message, "Set Time Window", buff, "", this ) )
        return;

                                        // convert user input
    tr      = Round ( StringToTimeFrame ( buff, EEGDoc->GetSamplingFrequency () ) );
    }
else {                                  // commands cloning AND a receiving cloned view -> forward the parameters set by caller view
                                                                                           // view should be TTracksView
    TTracksView*            orgview     = dynamic_cast<TTracksView*> ( CartoolApplication->LastActiveBaseView );

    if ( orgview )
                                        // retrieve current length
        tr      = orgview->CDPt.GetLength ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do some checking
long                mintf           = CDPt.GetLength () < DisplayMinTF ? 2 : DisplayMinTF;

long                oddtf           = 0; // mintf <= 1 || CDPt.GetLength () <= 11 ? 0 : 1;

long                reqlen          = Clip ( tr, mintf, (long) CDPt.GetLimitLength () )
                                    | oddtf;

long                oldtfmin        = CDPt.GetMin ();
long                oldtfmax        = CDPt.GetMax ();

                                        // modify CDPt
CDPt.SetLength ( reqlen, TFCursor.GetPosMiddle (), true );
                                        // contaminate TFCursor
CheckCDPtAndTFCursor ( false );
                                        // to be consistent with further user interaction
ComputeSTHfromCDPt ();
                                        // all refreshing business
RefreshLinkedWindow ();


if ( CDPt.GetLength () != ( tr | oddtf ) 
  && IsCommandSender ()
    ) {
    char                buff[ 256 ];

    StringCopy  ( buff, "Requested time range is out of bounds," NewLine "actual time range will be set to:" NewLine NewLine Tab );

    if ( EEGDoc->GetSamplingFrequency () ) {
        
        TDateTime           dt;
        
        dt.GetStringTime ( TimeFrameToMicroseconds ( CDPt.GetLength (), EEGDoc->GetSamplingFrequency () ), StringEnd ( buff ), true );
        }
    else {

        TFToString ( (long) CDPt.GetLength (), StringEnd ( buff ), 0, ShowHorizScaleLegal, 0, true );

        StringAppend( buff, "[TF]" );
        }

    ShowMessage ( buff, "Set Time Window", ShowMessageWarning );
    }


if ( oldtfmin == CDPt.GetMin () && oldtfmax == CDPt.GetMax () )
    return;


UpdateBuffers ( oldtfmin, oldtfmax, CDPt.GetMin (), CDPt.GetMax () );

Invalidate ();

ScrollBar->UpdateIt ();
}


//----------------------------------------------------------------------------
void    TTracksView::CmSelectAllTime ()
{
                                        // take as much data as possible, starting from beginning
CDPt.SetMinLength ( 0, CDPt.GetLimitLength (), true );
                                        // reload everything
UpdateBuffers ( -1, -1, CDPt.GetMin(), CDPt.GetMax() );

                                        // set TFCursor
//CheckCDPtAndTFCursor ( false );

TFCursor.StopExtending ();

TFCursor.SetPos ( CDPt.GetMin (), CDPt.GetMax () );

ButtonGadgetSetState ( IDB_RANGECURSOR, TFCursor.IsExtending() );

                                        // to be consistent with further user interaction
ComputeSTHfromCDPt ();

                                        // all refreshing business
RefreshLinkedWindow ();

EEGDoc->NotifyFriendViews ( LinkedViewId, vnNewTFCursor, (TParam2) &TFCursor, this );

ShowNow ();

ScrollBar->UpdateIt ();
}


//----------------------------------------------------------------------------
void    TTracksView::SetColoring ( int which, int from, ColoringEnum how, TGLColor<GLfloat> cmin, TGLColor<GLfloat> cmax, double minv, double maxv )
{
//if ( ! minv && ! maxv ) {      // limits not specified?
//
//    minv =
//    maxv = EegBuff[ from ][ 0 ];
//                                        // find limits (at least in the range available)
//    for ( int t=0; t < BuffSize; t++ ) {
//        Mined ( minv, (double) EegBuff[ from ][ t ] );
//        Maxed ( maxv, (double) EegBuff[ from ][ t ] );
//        }
//    }

if ( Filling[ which ] ) 
    delete  Filling[ which ];

Filling[ which ]    = new TGLColoring ( from, how, minv, maxv, cmin, cmax );
}


void    TTracksView::CmNextSession ()
{
                                        // switching sessions "live" is now possible, even with linked views
if ( ! EEGDoc->HasMultipleSessions () )
    return;


int                 currentsession  = EEGDoc->GetCurrentSession ();
static int          direction       = 1;
int                 suggestedsession= ( ( currentsession - 1 ) + EEGDoc->GetNumSessions () + direction ) % EEGDoc->GetNumSessions () + 1;
int                 newsession;
char                buff[ 256 ];

StringCopy ( buff, "Choose a session/sequence number, between 1 and ", IntegerToString ( EEGDoc->GetNumSessions () ), ":" );

if ( ! GetValueFromUser ( buff, "Session", newsession, IntegerToString ( suggestedsession ), this ) )
    return;


if ( IsInsideLimits ( newsession, 1, EEGDoc->GetNumSessions () )
  && newsession != currentsession ) {
                                        // remember the step used for next call
    direction   = newsession - currentsession;

    EEGDoc->GoToSession ( newsession );
    }
}


void    TTracksView::CmNextSessionEnable ( TCommandEnabler &tce )
{
tce.Enable ( EEGDoc->HasMultipleSessions () );
}


void    TTracksView::CmOrientEnable ( TCommandEnabler &tce )
{
tce.Enable ( CurrentDisplaySpace != DisplaySpaceNone );
}


void    TTracksView::CmOrient ()
{
/*
TBaseView::CmOrient ();
                                        // add before the standardizing orientation
if ( XYZDoc ) {
    ModelRotMatrix *= XYZDoc->GetStandardOrientation ( LocalToPIR );
    Invalidate ( false );
    }
*/

if ( CurrentDisplaySpace == DisplaySpaceNone || ! XYZDoc )
    return;


if ( CurrentDisplaySpace != DisplaySpace3D ) {
                                        // projection resets orientation -Y / X, so we can overlap different setups
    Orientation         = OrientMinusYX;

    ModelRotMatrix.SetOrientation ( Orientation );

//    SetOrient ( XYZDoc ); // to do: use Current3DSpace for this purpose
    }
else {
    TBaseView::CmOrient ();

    SetOrient ( XYZDoc );
    }

Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TTracksView::CmOpenSeg ( owlwparam w )
{
                                        // only legal files (.error.data and .seg) have an existing menu that allows to land here
//if ( ! StringEndsWith ( (char *) EEGDoc->GetDocPath (), FILEEXT_ERRORDATA ) )
//    return;


TFileName           dir      ;
TFileName           fileseg  ;
TFileName           filetempl;
TGoF                filescsv;
TFileName           search  ;
TFileName           grepbase;
TFileName           grepexts;
int                 seg1            = TFCursor.GetPosMin () + EEGDoc->GetStartingTimeFrame ();
int                 seg2            = TFCursor.GetPosMax () + EEGDoc->GetStartingTimeFrame ();
TGoF                gof;


StringCopy          ( search, EEGDoc->GetDocPath () );
RemoveExtension     ( search, 2 );

StringCopy          ( dir,    EEGDoc->GetDocPath () );
RemoveFilename      ( dir );

StringCopy          ( grepbase, ToFileName ( EEGDoc->GetDocPath () ) );
RemoveExtension     ( grepbase, 2 );
StringGrepNeutral   ( grepbase );


for ( int i = seg1; i <= seg2; i++ ) {
                                        // give enough of a template, and try our chance on it!

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // called from .error.data file
    if ( w == CM_OPENTEMPLATES ) {
                                        // get all possible answers
        ExtensionsToGrep ( grepexts, FILEEXT_EEGEP " " FILEEXT_RIS );
                                        // templates from segmentation or from a merge
        StringCopy      ( filetempl, "(", grepbase, "|", PostfixResampled, ")" );
        StringAppend    ( filetempl, "\\.0*", IntegerToString ( i ), "\\.\\([0-9]+\\).*" );
        StringAppend    ( filetempl, grepexts );

        gof.GrepFiles   ( dir, filetempl, GrepOptionDefaultFiles );
                                        // we don't want .Original (no avg ref) or .NoZScore answers
                                        // keeping either plain file name or .ZScore infix only
        gof.GrepRemove  ( PostfixNoStandardizationGrep, GrepOptionDefaultFiles );
        gof.GrepRemove  ( PostfixOriginalDataGrep,      GrepOptionDefaultFiles );

        if ( (bool) gof )
                                        // open all (eeg and esi)
            gof.OpenFiles ();
//        else
//            return;                     // nothing found
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    else if ( w == CM_OPENSEG ) {

        sprintf ( fileseg, "%s.%02d*.%s", (char*) search, i, FILEEXT_SEG );

        if ( GetFirstFile ( fileseg ) )
            CartoolDocManager->OpenDoc ( fileseg, dtOpenOptions );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    else if ( w == CM_OPENSEGVERBOSE ) {

        sprintf ( fileseg, "%s.%02d*.%s", (char*) search, i, FILEEXT_VRB );

        if ( GetFirstFile ( fileseg ) )
            CartoolDocManager->OpenDoc ( fileseg, dtOpenOptions );
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // called from .error.data file
    else if ( w == CM_OPENFITTINGTEMPLATES1 ) {
                                        // open .seg for confirmation
        sprintf ( fileseg, "%s.%02d*.%s", (char*) search, i, FILEEXT_SEG );

        if ( GetFirstFile ( fileseg ) )
            CartoolDocManager->OpenDoc ( fileseg, dtOpenOptions );


                                        // get all possible answers
        ExtensionsToGrep ( grepexts, FILEEXT_EEGEP " " FILEEXT_RIS );

        StringCopy      ( filetempl, "(", grepbase, "|", PostfixResampled, ")" );
        StringAppend    ( filetempl, "\\.0*", IntegerToString ( i ), "\\.\\([0-9]+\\).*" );
        StringAppend    ( filetempl, grepexts );

        gof.GrepFiles   ( dir, filetempl, GrepOptionDefaultFiles );
                                        // we don't want .Original (no avg ref) or .NoZScore answers
                                        // keeping either plain file name or .ZScore infix only
        gof.GrepRemove  ( PostfixNoStandardizationGrep, GrepOptionDefaultFiles );
        gof.GrepRemove  ( PostfixOriginalDataGrep,      GrepOptionDefaultFiles );

        if ( (bool) gof )
            StringCopy ( filetempl, gof[ 0 ] );
        else
//          return;                     // nothing found
            continue;                   // nothing found

                                        // call dialog only for 1 template file (the first one)
        break;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // called from .seg file
    else if ( w == CM_OPENFITTINGTEMPLATES2 ) {
                                        // .seg file is this
        StringCopy          ( fileseg, EEGDoc->GetDocPath () );

                                        // get all possible answers
        StringCopy          ( filetempl, fileseg );
        GetFilename         ( filetempl );
        StringGrepNeutral   ( filetempl );

        ExtensionsToGrep    ( grepexts, FILEEXT_EEGEP " " FILEEXT_RIS );
        StringAppend        ( filetempl, ".*" );
        StringAppend        ( filetempl, grepexts );


        gof.GrepFiles       ( dir, filetempl, GrepOptionDefaultFiles );
                                        // we don't want .Original (no avg ref) or .NoZScore answers
                                        // keeping either plain file name or .ZScore infix only
        gof.GrepRemove      ( PostfixNoStandardizationGrep, GrepOptionDefaultFiles );
        gof.GrepRemove      ( PostfixOriginalDataGrep,      GrepOptionDefaultFiles );

        if ( (bool) gof )
            StringCopy ( filetempl, gof[ 0 ] );
        else
//          return;                     // nothing found
            continue;                   // nothing found

                                        // call dialog only for 1 template file (the first one)
        break;
        }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // called from fitting .seg file
    else if ( w == CM_OPENSTATSFITTING ) {
                                        // .seg file is this
//      StringCopy          ( fileseg, EEGDoc->GetDocPath () );

                                        // get all possible answers
//      StringCopy          ( filetempl, fileseg );
//      GetFilename         ( filetempl );
//      StringGrepNeutral   ( filetempl );
//      sprintf             ( StringEnd ( filetempl ), ".*\\.(%s|%s)\\.%s", InfixCartoolWorksheet, InfixRWorksheet, FILEEXT_CSV );  // not reading both versions for the moment
//      sprintf             ( StringEnd ( filetempl ), ".*\\.%s\\.%s", InfixCartoolWorksheet, FILEEXT_CSV );

        StringCopy          ( filetempl, InfixCartoolWorksheet, "\\." FILEEXT_CSV, "$" );    // simpler grep, these .csv are only for stats here

        filescsv.GrepFiles  ( dir, filetempl, GrepOptionDefaultFiles );

//      filescsv.Show ();

        if ( ! (bool) filescsv )
//          return;                     // nothing found
            continue;                   // nothing found

                                        // call statistics only once
        break;
        }

    } // for seg


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we got the files, run the dialog
if ( ( w == CM_OPENFITTINGTEMPLATES1
    || w == CM_OPENFITTINGTEMPLATES2 )
      && StringIsNotEmpty ( filetempl )  ) {


//  TTracksDoc*         TemplDoc    = dynamic_cast< TTracksDoc * > ( CartoolDocManager->OpenDoc ( filetempl, dtOpenOptions ) );

                                        // open the Fitting dialog conveniently filled with the current template file
    TFileName           basefilename;

    StringCopy ( FitTransfer.TemplateFileName, filetempl, EditSizeText - 1 );
                                        // cook the base file name
    FitTransfer.ComposeBaseFilename ( basefilename );
                                        // then copy it to the dialog
    StringCopy ( FitTransfer.BaseFileName, basefilename, EditSizeText - 1 );


    if      ( FitTransfer.LastDialogId == IDD_FITTING1 )    TMicroStatesFitFilesDialog  ( this, IDD_FITTING1 ).Execute ();
    else                                                    TMicroStatesFitParamsDialog ( this, IDD_FITTING2 ).Execute ();

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( w == CM_OPENSTATSFITTING
      && (bool) filescsv ) {

//    if      ( StatTransfer.LastDialogId == IDD_STATISTICS1 )    TStatisticsFilesDialog  ( this, IDD_STATISTICS1 ).Execute ();
//    else                                                        TStatisticsParamsDialog ( this, IDD_STATISTICS2 ).Execute ();

                                        // force dialog files part?
    TStatisticsFilesDialog*      statfiles       = new TStatisticsFilesDialog ( this, IDD_STATISTICS1, CM_STATISTICSTRACKS );
                                        // run it modeless
    statfiles->Create ();
                                            
                                        // transfer our group of files
    statfiles->CmClearGroups ();

    for ( int i = 0; i < (int) filescsv; i++ )
        statfiles->ReadSpreadSheet ( filescsv[ i ] );

                                        // switch to modal, give it to user - does not seem to really work...
    statfiles->BeginModal ();
    }
}


//----------------------------------------------------------------------------
void    TTracksView::CmSegmentEnable ( TCommandEnabler &tce )
{
tce.Enable ( SelFilling );
}


void    TTracksView::CmSelectSegments ( owlwparam w )
{
if ( ! SelFilling )
    return;


if ( w == CM_SELECTSEGMENTSFROMLIST ) {
                                        // get user's wishes
    char                buff[ 256 ];

    if ( ! GetInputFromUser ( "Give the list of segments to show, or 0 to reset:", "Select Segments", buff, "", this ) )
        return;


    SelFilling->Reset ();
    SelFilling->Set   ( buff, false );  // set selection
    SelFilling->Reset ( 0 );            // starts from 1

    } // CM_SELECTSEGMENTSFROMLIST

else { // CM_SELECTSEGMENTSFROMCURSOR
                                        // retrieve the index of segments
    TSelection          segsel ( EEGDoc->GetNumElectrodes (), OrderSorted );

    for ( int e = 0; e < EEGDoc->GetNumElectrodes (); e++ )
        if ( StringStartsWith ( GetElectrodeName ( e ), TrackNameSeg ) )
            segsel.Set ( e );

    if ( segsel.IsNoneSet () )
        return;

                                        // get data
    int                 readfromtf      = TFCursor.GetPosMin ();
    int                 readtotf        = TFCursor.GetPosMax ();
    int                 readnumtf       = readtotf - readfromtf + 1;

    TTracks<float>      EegBuff;

    EEGDoc->GetTracks   (   readfromtf,     readtotf, 
                            EegBuff,        0, 
                            AtomTypeScalar, 
                            NoPseudoTracks,
                            ReferenceUsingCurrent 
                        );


    SelFilling->Reset ();

    for ( int tf0 = 0; tf0 < readnumtf; tf0++ )
    for ( TIteratorSelectedForward segi ( segsel ); (bool) segi; ++segi )

        SelFilling->Set ( EegBuff ( segi(), tf0 ) );

    } // CM_SELECTSEGMENTSFROMCURSOR




if ( ! (bool) (*SelFilling) ) {         // nothing selected == reset everything
    SelFilling->Set   ();
    SelFilling->Reset ( 0 );
    }


Invalidate ( false );
}


void    TTracksView::CmNextSegment ( owlwparam w )
{
if ( ! SelFilling )
    return;


if ( SelFilling->NumSet () > 0 && SelFilling->NumSet () <= SelFilling->MaxValue () - 1 ) {

    if ( w == CM_NEXTSEGMENT && SelFilling->IsSelected ( SelFilling->MaxValue () )
      || w == CM_PREVSEGMENT && SelFilling->IsSelected ( 1                       ) ) {
        SelFilling->Set   ();
        SelFilling->Reset ( 0 );
        }
    else
        SelFilling->Shift ( 1, SelFilling->MaxValue (), 1, (TSelectionShiftFlags) ( ( w == CM_NEXTSEGMENT ? TSelectionShiftDown : TSelectionShiftUp ) | TSelectionShiftReportStop ) );
    }
else {
    SelFilling->Reset ();
    SelFilling->Set   (  w == CM_NEXTSEGMENT ? 1 : SelFilling->MaxValue () );
    }


Invalidate ( false );
}


//----------------------------------------------------------------------------
void    TTracksView::CmEasyPublish ()
{
char                buff[ 256 ];

switch ( rand () % 10 ) {
    case 0: StringCopy ( buff, "Please install Service Pack 23 and reboot." ); break;
    case 1: StringCopy ( buff, "This will be ready just in time for Windows 57." ); break;
    case 2: StringCopy ( buff, "So... can you remind me what you are paid for, exactly?" ); break;
    case 3: StringCopy ( buff, "Ha ha, that was a good one." ); break;
    case 4: StringCopy ( buff, "Unimplemented feature, but working on it..." ); break;
    case 5: StringCopy ( buff, "The 'easy publish' wizard is drunk, come back later please." ); break;
    case 6: StringCopy ( buff, "Follow these steps:\n\n 1) Psdkfj wd lk alknn qsd\n 2) Rrlkn kj ioiq nk\n 3) Otrl lkn s jnkja" ); break;
    case 7: StringCopy ( buff, "I thought this menu was well hidden, so I will bury it even deeper!" ); break;
    case 8: StringCopy ( buff, "Hey, YOU got the brain, not ME, I'm a stupid computer after all!" ); break;
    case 9: StringCopy ( buff, "Nice try..." ); break;
    }

ShowMessage ( buff, "Easy Publish Wizard", ShowMessageWarning );
}


//----------------------------------------------------------------------------
                                        // Can produce either "horizontal", per track, or "vertical", per time frame histograms
                                        // Will skip strict zero values
void    TTracksView::CmHistogram ( owlwparam w )
{
bool                pertrack        = IsInsideLimits ( w, (owlwparam) CM_HISTOTIMERAW,   (owlwparam) CM_HISTOTIMECDF   );
bool                pertf           = IsInsideLimits ( w, (owlwparam) CM_HISTOTRACKSRAW, (owlwparam) CM_HISTOTRACKSCDF );
bool                alldata         = IsInsideLimits ( w, (owlwparam) CM_HISTOALLRAW,    (owlwparam) CM_HISTOALLCDF    );

if ( ! ( pertrack || pertf || alldata ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // More options here:
//char                answer          = GetOptionFromUser ( "Output (A)ll tracks together, or (O)ne file per track:", 
//                                                          "Tracks Histogram", "A O", "A", this );
//
//if ( answer == EOS )   return;
//
//bool                allvarstogether     = alldata || answer == 'A';

bool                allvarstogether     = alldata || true;


//answer  = GetOptionFromUser ( "De-skew the data: (N)o, (A)uto, (R)ight, (L)eft?", 
//                              "Tracks Histogram", "N A R L", (int) elsel > 20 ? "N" : "A", this );
//
//if ( answer == EOS )   return;
//
//DeSkewType          deskewdata          = answer == 'A' ? DeSkewAuto
//                                        : answer == 'R' ? DeSkewRight
//                                        : answer == 'L' ? DeSkewLeft
//                                        : answer == 'N' ? DeSkewNone
//                                        :                 DeSkewNone;

DeSkewType          deskewdata          = DeSkewNone;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                cdf             = w == CM_HISTOTIMECDF 
                                   || w == CM_HISTOTRACKSCDF
                                   || w == CM_HISTOALLCDF;
bool                pdf             = ! cdf;


bool                logoutput       = pdf
                                &&  ( w == CM_HISTOTIMELOG 
                                   || w == CM_HISTOTRACKSLOG
                                   || w == CM_HISTOALLLOG    );

                                    // spikey or smoothey?
bool                curvesmooth     = w == CM_HISTOTIMESMOOTH 
                                   || w == CM_HISTOTRACKSSMOOTH
                                   || w == CM_HISTOALLSMOOTH
                                   || cdf
                                   || logoutput;

                                    // makes max = 1, easier on the eyes but not mathematically correct
bool                normalizemax    = false;

                                    // cooking the whole option flags
HistogramOptions    histooptions    = (HistogramOptions)
                                    ( ( cdf             ? HistogramCDF      : HistogramPDF      )
                                    | ( curvesmooth     ? HistogramSmooth   : HistogramRaw      )
                                    | ( normalizemax    ? HistogramNormMax  : HistogramNormArea )
                                    | ( logoutput       ? HistogramLog      : HistogramLinear   ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSelection          elsel ( EEGDoc->GetTotalElectrodes (), OrderSorted );

                                        // as a good starting point, use the highlighted, or the currently displayed tracks
if ( (bool) Highlighted )   elsel   = Highlighted;
else                        elsel   = SelTracks; 


if ( ( pertf || alldata )                       // histogram through tracks needs.. tracks, not pseudos
  || EEGDoc->GetNumSelectedRegular ( elsel ) )  // don't mix regular and pseudos

    EEGDoc->ClearPseudo ( elsel ); 

                                        // across tracks, if not explicitly specified by the user, we don't want outliers
if ( ( pertf || alldata )
  && ! (bool) Highlighted ) {
    EEGDoc->ClearAuxs  ( elsel );
    EEGDoc->ClearBads  ( elsel );
    }

                                        // oops, nothing remains, what do?
if ( elsel.IsNoneSet () ) {
    EEGDoc->SetRegular ( elsel );       // OK, I'll do it for you buddy
    EEGDoc->ClearAuxs  ( elsel );
    EEGDoc->ClearBads  ( elsel );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                autoopen            = allvarstogether || (int) elsel <= 2 * MaxFilesToOpen;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numtotalel      = EEGDoc->GetTotalElectrodes ();
int                 numtotaltf      = EEGDoc->GetNumTimeFrames ();

                                        // number of tracks for statistics
int                 numel           = (int) elsel;

int                 readfromtf      = TFCursor.IsSplitted () || pertf ? TFCursor.GetPosMin ()   // if splitted or per TF, use this time range all the time
                                                                      : 0;                      // otherwise, use the whole time range
int                 readtotf        = TFCursor.IsSplitted () || pertf ? TFCursor.GetPosMax ()   // if splitted or per TF, use this time range all the time
                                                                      : numtotaltf - 1;         // otherwise, use the whole time range
                                        // histogram can become quite long to compute with too much data in, for no real gain, so we might try to downsample too big inputs
TDownsampling       downtf ( readfromtf, readtotf, NumScanHistogram );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TGoEasyStats        stats;

if      ( pertrack )    stats.Resize ( numel,               downtf.NumDownData          );
else if ( pertf    )    stats.Resize ( downtf.NumDownData,  numel                       );
else if ( alldata  )    stats.Resize ( 1,                   numel * downtf.NumDownData  );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge     Gauge;

Gauge.Set       ( pertrack ? "Per Tracks Histogram" : pertf ? "Per Time Histogram" : "All Data Histogram" );

Gauge.AddPart   ( 0,    2,                          05 );
Gauge.AddPart   ( 1,    downtf.NumDownData,         30 );
Gauge.AddPart   ( 2,    2 * stats.GetNumStats (),   65 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Next ( 0 );

TTracks<float>      EegBuff  ( numtotalel, 1 );


for ( int tf = downtf.From, tf0 = 0; tf <= downtf.To; tf += downtf.Step, tf0++ ) {

    Gauge.Next ( 1 );

                                        // DON'T read all data, especially if we're going to only step through...
    EEGDoc->GetTracks   (   tf,         tf, 
                            EegBuff,    0, 
                            AtomTypeScalar, 
                            ComputePseudoTracks, 
                            ReferenceUsingCurrent 
                        );

                                        // Feed these hungry stats
    if      ( pertrack ) {

        for ( TIteratorSelectedForward eli ( elsel ); (bool) eli; ++eli )
            stats ( eli.GetIndex () ).Add ( EegBuff ( eli(), 0 ), ThreadSafetyIgnore );

        } // pertrack

    else if ( pertf ) {

        for ( TIteratorSelectedForward eli ( elsel ); (bool) eli; ++eli )
            stats ( tf0 ).Add ( EegBuff ( eli(), 0 ), ThreadSafetyIgnore );

        } // pertf

    else /*if ( alldata )*/ {

        for ( TIteratorSelectedForward eli ( elsel ); (bool) eli; ++eli ) 
            stats ( 0 ).Add ( EegBuff ( eli(), 0 ), ThreadSafetyIgnore );

        } // alldata
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // De-Skew all data
Gauge.Next ( 0 );

TArray1<int>        skewness ( stats.DeSkew ( deskewdata ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Retrieve common measures for all tracks
double              kerneldensity;
double              allmin;
double              allmax;
double              marginfactor        = 0; // curvesmooth ? 3 : 0;    // smoothing -> adding some margin before and after - !something seems off with a margin!
double              kernelsubsampling   = curvesmooth ? stats.GetNumStats () == 1 ? 30 : 3  // showing more "resolution", for better visual effect
                                                      : 1;                                  // without smoothing, having more resolution will create "gaps" or aliasing effects between buckets
double              curvesize           = 0;
double              curvemin;
double              curveratio;
THistogram          curve_raw;

char                buff [ 256 ];
char                buff2[ 256 ];


TExportTracks       expfile;
expfile.SetAtomType ( AtomTypeScalar );
int                 numtfdigits         = NumIntegerDigits ( readtotf );
char                infix[ 32 ];

if      ( pdf )     StringCopy      ( infix, ".", InfixHistogramShort );
else if ( cdf )     StringCopy      ( infix, ".", InfixCDF            );
else                ClearString     ( infix );

if ( logoutput )    StringAppend    ( infix, "Log" );


if ( allvarstogether ) {

                                        // Retrieve common measures for all tracks
    kerneldensity       =   stats.GaussianKernelDensity ();                 // first, get a common to all tracks Kernel Density
    allmin              = EEGDoc->IsPositive ( AtomTypeUseCurrent ) ? 0 : stats.Min (); // get the biggest encompassing interval on all tracks
    allmax              =   stats.Max ();
//  allmin              =   stats.Quantile ( 0.001 );                       // get a more restrictive range across all tracks
//  allmax              =   stats.Quantile ( 0.999 );
//  allmin              = ( stats.Quantile ( 0.001 ) + stats.Min () ) / 2;  // get a little less restrictive range
//  allmax              = ( stats.Quantile ( 0.999 ) + stats.Max () ) / 2;
    curvesize           = 0;

                                        // Evaluate curvesize, curvemin, curveratio - Kernel Density being given
    curve_raw.ComputeHistogramSize  (   kerneldensity,  allmin,     allmax, 
                                        marginfactor,   kernelsubsampling,
                                        curvesize,      curvemin,   curveratio 
                                    );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    TFileName           elselnames;

    elsel.ToText ( elselnames, GetElectrodesNames (), AuxiliaryTracksNames );


    StringCopy          ( expfile.Filename, EEGDoc->GetDocPath () );
    RemoveExtension     ( expfile.Filename );
    StringAppend        ( expfile.Filename, infix );

                                        // non-exclusive conditions, so alldata will run through both tests
    if ( pertrack || alldata ) {
                                        // Testing for too long selection
        if ( elselnames.Length () > 32 )    StringAppend    ( expfile.Filename, " ", "Tracks"   );  // fallback
        else                                StringAppend    ( expfile.Filename, " ", elselnames );
        }

    if ( pertf || alldata ) {

        if ( downtf.NumDownData == 1 )      StringAppend    ( expfile.Filename, " TF ", IntegerToString ( buff, readfromtf, numtfdigits ) );
        else                                StringAppend    ( expfile.Filename, " TF ", IntegerToString ( buff, readfromtf, numtfdigits ), "-", IntegerToString ( buff2, readtotf, numtfdigits ) );
        }

    AddExtension        ( expfile.Filename, FILEEXT_EEGSEF );

    CheckNoOverwrite    ( expfile.Filename );


    expfile.NumTracks           = stats.GetNumStats ();
    expfile.NumTime             = curvesize;
    expfile.SamplingFrequency   = curveratio;                                           // will "rescale" to actual variable values
    expfile.DateTime            = TDateTime ( 0, 0, 0, 0, 0, 0, curvemin * 1000, 0 );   // will set the offset correctly
    

    expfile.ElectrodesNames.Set ( expfile.NumTracks, 256 );

    if      ( pertrack ) {

        for ( TIteratorSelectedForward eli ( elsel ); (bool) eli; ++eli )
            StringCopy      ( expfile.ElectrodesNames[ eli.GetIndex() ], SkewnessToString ( skewness ( eli.GetIndex () ) ), GetElectrodeName ( eli() ) );
        }

    else if ( pertf ) {

        for ( long tf0 = 0, tf = readfromtf; tf0 < downtf.NumDownData;  tf0++, tf += downtf.Step )
            StringCopy      ( expfile.ElectrodesNames[ tf0 ], SkewnessToString ( skewness ( tf0 ) ), "TF", IntegerToString ( buff, tf, numtfdigits ) );
        }

    else if ( alldata ) {

        elsel.ToText ( expfile.ElectrodesNames[ 0 ], GetElectrodesNames () );
        }

    
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    expfile.Begin ();


    for ( int si = 0; si < stats.GetNumStats (); si++ ) {

        Gauge.Next ( 2 );
                                        

        kerneldensity   = 0;            // !force re-computing an optimal Kernel Density for each track (while keeping the boundaries and curve size constant)!

                                        // try choosing between the global and local Kernel Density, if the latter is too different from the former
//        double      localkerneldensity  =   stats ( si ).GaussianKernelDensity ();
//        if ( VkQuery () ) DBGV4 ( si + 1, localkerneldensity, kerneldensity, RelativeDifference ( localkerneldensity, kerneldensity ), "si#, localkerneldensity, globalkerneldensity, RelativeDifference" );

                                        
        curve_raw.ComputeHistogram  (   stats ( si ),
                                        kerneldensity,  allmin,             allmax, 
                                        marginfactor,   kernelsubsampling,
                                        histooptions,
                                       &curvesize 
                                    );

        Gauge.Next ( 2 );

        for ( int hi = 0; hi < expfile.NumTime; hi++ )

            expfile.Write ( curve_raw ( hi ), hi , si );
        }


    expfile.End ();


    if ( autoopen )
        expfile.Filename.Open ();

    } // if allvarstogether


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else { // ! allvarstogether - alldata NOT a valid case here

    for ( int si = 0; si < stats.GetNumStats (); si++ ) {

                                        // Retrieve current track measures
        kerneldensity       =   stats ( si ).GaussianKernelDensity ();                          // first, get a current track Kernel Density
        allmin              = EEGDoc->IsPositive ( AtomTypeUseCurrent ) ? 0 : stats ( si ).Min ();  // get the biggest encompassing interval
        allmax              =   stats ( si ).Max ();
//      allmin              =   stats ( si ).Quantile ( 0.001 );                                // get a more restrictive range
//      allmax              =   stats ( si ).Quantile ( 0.999 );
//      allmin              = ( stats ( si ).Quantile ( 0.001 ) + stats ( e0 ).Min () ) / 2;    // get a little less restrictive range
//      allmax              = ( stats ( si ).Quantile ( 0.999 ) + stats ( e0 ).Max () ) / 2;
        curvesize           = 0;

                                        // Evaluate the resulting curve size, as the Kernel Density and range are here a given
        curve_raw.ComputeHistogramSize  (   kerneldensity,  allmin,     allmax, 
                                            marginfactor,   kernelsubsampling,
                                            curvesize,      curvemin,   curveratio 
                                        );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        StringCopy          ( expfile.Filename, EEGDoc->GetDocPath () );
        RemoveExtension     ( expfile.Filename );
        StringAppend        ( expfile.Filename, infix );

        if ( pertrack ) StringAppend    ( expfile.Filename, " ", SkewnessToString ( skewness ( si ) ), GetElectrodeName ( elsel.GetValue ( si ) ) );
        else            StringAppend    ( expfile.Filename, " TF ", IntegerToString ( buff, readfromtf + si, numtfdigits ) );

        AddExtension        ( expfile.Filename, FILEEXT_EEGSEF );

        CheckNoOverwrite    ( expfile.Filename );


        expfile.NumTracks           = 1;
        expfile.NumTime             = curvesize;
        expfile.SamplingFrequency   = curveratio;                                           // will "rescale" to actual variable values
        expfile.DateTime            = TDateTime ( 0, 0, 0, 0, 0, 0, curvemin * 1000, 0 );   // will set the offset correctly


        expfile.ElectrodesNames.Set ( expfile.NumTracks, 256 );

        if ( pertrack ) StringCopy      ( expfile.ElectrodesNames[ 0 ], SkewnessToString ( skewness ( si ) ), GetElectrodeName ( elsel.GetValue ( si ) ) );
        else            StringCopy      ( expfile.ElectrodesNames[ 0 ], SkewnessToString ( skewness ( si ) ), "TF", IntegerToString ( buff, readfromtf + si, numtfdigits ) );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        expfile.Begin ();


        Gauge.Next ( 2 );
                                        // Compute current histogram
        curve_raw.ComputeHistogram  (   stats ( si ),
                                        kerneldensity,  allmin,             allmax, 
                                        marginfactor,   kernelsubsampling,
                                        histooptions,
                                        &curvesize 
                                    );

        Gauge.Next ( 2 );

        for ( int hi = 0; hi < expfile.NumTime; hi++ )

            expfile.Write ( curve_raw ( hi ), hi , 0 );


        expfile.End ();


        if ( autoopen )
            expfile.Filename.Open ();
        } // for elsel

    } // ! allvarstogether


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Finished ();
}


//----------------------------------------------------------------------------
void    TTracksView::CmAnalyzeTracks ( owlwparam w )
{
if ( EEGDoc->GetNumRegularElectrodes () <= 1 ) {
    ShowMessage ( "Not enough real tracks to perform this analysis!", "Analyzing Tracks", ShowMessageWarning );
    return;
    }


if ( EEGDoc->GetNumTimeFrames () <= 100 ) {
    ShowMessage ( "Not enough time frames perform this analysis!", "Analyzing Tracks", ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) Get some subsampling of the data, avoiding beginning and ending

int                 numtotalel      = EEGDoc->GetTotalElectrodes ();
long                fromtf          = EEGDoc->GetNumTimeFrames () * 0.10;
long                totf            = EEGDoc->GetNumTimeFrames () * 0.90;
//int                 step            = max ( Round ( ( totf - fromtf + 1 ) / GetValue ( "In number of steps (>100):", "Analyze Tracks" ) ), 1 );

TDownsampling       downtf ( fromtf, totf, /*150*/ 1000 );
int                 numtf           = downtf.NumDownData;

TTracks<float>      EegBuff ( numtotalel, 1 );
//int                 numtf           = 3 * ( 1 + ( totf - fromtf ) / step );     // store triplets of data, for Laplacian & Derivatives
//TTracks<float>      EegBuff ( numtotalel, 3 );
TArray2<float>      data    ( numtotalel, numtf );
TSelection          goodtf  ( numtf, OrderSorted );


TSuperGauge         Gauge ( "Analyzing Tracks", downtf.NumDownData );

                                        // scan TFs
for ( long tfi = 0, tf = downtf.From; tfi < numtf;  tfi++, tf += downtf.Step ) {

    Gauge.Next ();

//  EEGDoc->ReadRawTracks ( tf, tf, data, tfi );
                                        // get tracks with current filters and no ref
    EEGDoc->GetTracks   (   tf,         tf, 
                            EegBuff,    0, 
                            AtomTypeScalar, 
                            NoPseudoTracks,     // will do the pseudos myself
                            /*ReferenceUsingCurrent*/ ReferenceAsInFile
                        );

    for ( int e = 0; e < numtotalel; e++ )

        data ( e, tfi )     = EegBuff ( e, 0 );

/*                                      // reading triplets
    EEGDoc->GetTracks ( tf - 1, tf + 1, EegBuff, 0, &gtparams );
                                        // transfer to buffer
    for ( int e = 0; e < numtotalel; e++ ) {
        data ( e, 3 * tfi     ) = EegBuff ( e, 0 );
        data ( e, 3 * tfi + 1 ) = EegBuff ( e, 1 );
        data ( e, 3 * tfi + 2 ) = EegBuff ( e, 2 );
        }*/
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) Analyze tracks

Highlighted.Reset ();
                                        // original good TFs to scan
UpdateTimeRange ( data, goodtf );


if ( w == CM_ANALYZESELFLAT || w == CM_ANALYZESELARTEFACTS ) {
                                        // simple and robust
    ScanTracksEnergy ( data, goodtf );
                                        // update good TFs to scan
    UpdateTimeRange ( data, goodtf );

//  ShowMessage ( "Flat tracks", "Analyzing Tracks" );
    }


if ( w == CM_ANALYZESELHIGH || w == CM_ANALYZESELARTEFACTS ) {
                                        // very robust
    ScanTracksTooHigh ( data, goodtf );
                                        // update good TFs to scan
    UpdateTimeRange ( data, goodtf );

//  ShowMessage ( "Saturated tracks", "Analyzing Tracks" );
    }


//ScanTracksTooMuchVariability ( data, goodtf );

//ShowMessage ( "Before Noisy", "Analyzing Tracks" );
//ScanTracksNoisy ( data, goodtf, false );
//ShowMessage ( "Noisy high", "Analyzing Tracks" );
//ScanTracksNoisy ( data, goodtf, true );
//ShowMessage ( "Noisy low", "Analyzing Tracks" );


if ( w == CM_ANALYZESELGFP || w == CM_ANALYZESELARTEFACTS ) {
                                        // "weird" tracks - needs to skip blinks, too?
    ScanTracksGfp ( data, goodtf );
                                        // update good TFs to scan
    UpdateTimeRange ( data, goodtf );

//  ShowMessage ( "GFP outliers", "Analyzing Tracks" );
    }


/*if ( w == CM_ANALYZESELNOISY ) {
                                        // tracks with low content
    ScanTracksSD ( data, goodtf );
                                        // update good TFs to scan
    UpdateTimeRange ( data, goodtf );

//  ShowMessage ( "Noisy tracks", "Analyzing Tracks" );
    }*/


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Invalidate ( false );

EEGDoc->NotifyDocViews ( vnNewHighlighted, (TParam2) &Highlighted );


if ( ! (bool) Highlighted ) {

//  Gauge.Finished ();
    Gauge.Reset ();

    char            buff[ 256 ];

    if      ( w == CM_ANALYZESELFLAT )      StringCopy ( buff, "No flat tracks were found..." );
    else if ( w == CM_ANALYZESELHIGH )      StringCopy ( buff, "No broken / jerky tracks were found..." );
    else if ( w == CM_ANALYZESELGFP )       StringCopy ( buff, "No outlier / noisy tracks were found..." );
    else if ( w == CM_ANALYZESELNOISY )     StringCopy ( buff, "No noisy tracks were found..." );
    else if ( w == CM_ANALYZESELARTEFACTS ) StringCopy ( buff, "No artefacted tracks were found..." );

    ShowMessage ( buff, "Analyzing Tracks", ShowMessageWarning );
    }
else {
    Gauge.HappyEnd ();
    }

}


//----------------------------------------------------------------------------
void    TTracksView::UpdateTimeRange ( TArray2<float> &data, TSelection &goodtf )
{
                                        // Scan TFs
int                 numel           = EEGDoc->GetNumElectrodes ();
int                 numtf           = data.GetDim2 ();
int                 gfpi            = EEGDoc->GetGfpIndex ();
TEasyStats          elstats;
TEasyStats          gfpstats ( numtf );


                                        // for all TFs, compute current GFP/SD
for ( int tf = 0; tf < numtf; tf++ ) {

    elstats.Reset ();

    for ( int e = 0; e < numel; e++ )
                                        // don't use the excluded tracks!
        if ( ! ( BadTracks[ e ] || AuxTracks[ e ] || Highlighted[ e ] ) )
            elstats.Add ( data ( e, tf ) );

    data ( gfpi, tf )   = elstats.SD ();
    } // for tf


                                        // stats of GFPs
gfpstats.Reset ();

for ( int tf = 0; tf < numtf; ++tf )
    gfpstats.Add ( data ( gfpi, tf ) );

                                        // exclude GFP peaks (blinks, bad time period...)
double              gfpmax          = gfpstats.Median ( false );
                    gfpmax         += gfpstats.MAD ( CanAlterData );

for ( int tf = 0; tf < numtf; ++tf ) {

    goodtf.Set ( tf, data ( gfpi, tf ) < gfpmax );

//    long                fromtf          = EEGDoc->GetNumTimeFrames () * 0.10;
//    long                totf            = EEGDoc->GetNumTimeFrames () * 0.90;
//    int                 step            = max ( Round ( ( totf - fromtf + 1 ) / 150.0 ), 1 );
//    EEGDoc->AppendMarker ( TMarker ( fromtf + tf / 3 * step + ( tf % 3 - 1 ), fromtf + tf / 3 * step + ( tf % 3 - 1 ), 100, goodtf[ tf ] ? "Analyze OK" : "Analyze BAD", MarkerTypeMarker ) );
//    EEGDoc->SortAndCleanMarkers ();
//    EEGDoc->CommitMarkers ();
    }

//EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_TRG );
}


//----------------------------------------------------------------------------
                                        // Scan the SD of the signal, looking for flat and high variations lines (not recorded & noisy electrodes)
                                        // Use unfiltered raw data
void    TTracksView::ScanTracksEnergy ( TArray2<float> &data, TSelection &goodtf )
{
int                 numel           = EEGDoc->GetNumElectrodes ();
TArray1<double>     measure ( numel );
TEasyStats          stats;
//TEasyStats          metastats ( numel );


//    EEGDoc->ReadRawTracks ( tf, tf, data, tfi );


                                        // check out plain flat tracks
for ( int e = 0; e < numel; e++ ) {

    if ( BadTracks[ e ] || AuxTracks[ e ] || Highlighted[ e ] /* || e == numel - 1 || StringContains ( GetElectrodeName ( e ), "ref" )*/ )
        continue;

    stats.Reset ();

    for ( TIteratorSelectedForward tfi ( goodtf ); (bool) tfi; ++tfi )
        stats.Add ( data ( e, tfi() ) );

    measure[ e ]    = stats.SD ();
//    DBGV2 ( e + 1, measure[ e ], "el#  sd" );

    if ( measure[ e ] == 0 )
        Highlighted.Set ( e );
//    else
//        metastats.Add ( measure[ e ] );
    }


Invalidate ( false );
//ShowMessage ( "Energy - Low", "Tracks Scanning" );


/*                                      // can select tracks with eye blinks...
double              maxsd           = metastats.Average () + metastats.SD ();

for ( int e = 0; e < numel; e++ ) {

    if ( BadTracks[ e ] || AuxTracks[ e ] || Highlighted[ e ] )
        continue;

    if ( measure[ e ] > maxsd )
        Highlighted.Set ( e );
    }
*/

/*
do {
    metastats.Reset ();

    for ( int e = 0; e < numel; e++ )
        if ( ! ( BadTracks[ e ] || AuxTracks[ e ] || Highlighted[ e ] ) )
            metastats.Add ( measure[ e ] );


    if ( metastats.Average () == 0 || metastats.SD () == 0 )
        break;
                                        // we can compute these criterion
//  bool                maxcov          = metastats.CoV () > 0.90;
    bool                maxonavg        = metastats.Max / metastats.Average () > 7.50;  // 7.80 OK
    bool                maxonmedian     = metastats.Max / metastats.Median  () > 8.00;  // 8.15 OK
    bool                maxonsd         = metastats.Max / metastats.SD      () > 10.0;

    int                 maxeli;
    double              maxavg;

//    DBGV4 ( maxonavg, maxonmedian, maxonsd, maxonavg + maxonmedian + maxonsd, "3 criterion" );

                                        // vote for a majority of criterion
//    if ( maxonavg + maxonmedian + maxonsd >= 2 ) {
    if ( ! VkEscape () ) {
                                        // find worse track
        maxavg      = 0;
        maxeli      = 0;

        for ( int e = 0; e < numel; e++ )
            if ( ! ( BadTracks[ e ] || AuxTracks[ e ] || Highlighted[ e ] )
              && measure[ e ] > maxavg ) {
                maxavg  = measure[ e ];
                maxeli  = e;
                }
                                        // remove it
        Highlighted.Set ( maxeli );

        ShowNow ();
//        DBGV3 ( metastats.Average (), metastats.SD (), metastats.CoV (), "Absolute Average: Avg SD CoV" );
//        DBGV3 ( metastats.Median (), metastats.MAD (), metastats.Max, "Absolute Average: Median MAD Max" );
        DBGV3 ( metastats.Max / metastats.Average (), metastats.Max / metastats.Median (), metastats.Max / metastats.SD (), "Max/Average  Max/Median  Max/SD" );
//        DBGV3 ( maxeli + 1, maxavg, metastats.CoV (), "el#  avgabs  CoV" );
        }
    else {
//      ShowMessage ( "Absolute average criterion does not apply...", "Tracks Scanning" );
        break;
        }

    } while ( (int) Highlighted < numel );


Invalidate ( false );
ShowMessage ( "Energy - High", "Tracks Scanning" );
*/
}


//----------------------------------------------------------------------------
                                        // Scan the absolute values of the signal, looking for average outliers ("broken" electrodes)
void    TTracksView::ScanTracksTooHigh ( TArray2<float> &data, TSelection &/*goodtf*/ )
{
int                 numel           = EEGDoc->GetNumElectrodes ();
int                 numtf           = data.GetDim2 ();
TEasyStats          stats;
TEasyStats          metastats   ( numel );
TArray1<double>     measure     ( numel );

//double              limit;
//GetValueFromUser ( "High level (1.00):", "Analyze Tracks", limit, "0.7", this );


                                        // extract our measure
for ( int e = 0; e < numel; e++ ) {

    stats.Reset ();

    for ( int tf = 0; tf < numtf; tf++ )
                                        // look at the average of absolute values, including (especially) the not-good TFs!
        stats.Add ( fabs ( data ( e, tf ) ) );

//  measure[ e ]    = stats.Average ();
//  measure[ e ]    = stats.SD ();
    measure[ e ]    = stats.Average () * stats.SD ();   // high values + lots of variations
    }



do {
    metastats.Reset ();

    for ( int e = 0; e < numel; e++ )
        if ( ! ( BadTracks[ e ] || AuxTracks[ e ] || Highlighted[ e ] ) )
            metastats.Add ( measure[ e ] );


    if ( metastats.Average () == 0 
      || metastats.SD      () == 0 )
        break;
                                        // we can compute these criterion
//  bool                maxcov          = metastats.CoV () > 0.90;
//    bool                maxonavg        = metastats.Max () / metastats.Average () > 4.0;     // avg
//    bool                maxonmedian     = metastats.Max () / metastats.Median  () > 3.80;
//    bool                maxonsd         = metastats.Max () / metastats.SD      () > 7.50;

//    bool                maxonavg        = metastats.Max () / metastats.Average () > 30.00;   // avg * sd
//    bool                maxonmedian     = metastats.Max () / metastats.Median  () > 90.00;
//    bool                maxonsd         = metastats.Max () / metastats.SD      () >  9.00;
//    bool                maxcov          = metastats.CoV ()                     >  3.00;
//    double              compound        = metastats.Max () / metastats.Average () * metastats.Max () / metastats.Median () * metastats.Max () / metastats.SD () / 1e4;
//    bool                maxcompound     = compound > 2.00;

    double              maxonavg        = metastats.Max () / metastats.Average () /  50.00;
    double              maxonmedian     = metastats.Max () / metastats.Median  () / 100.00;
    double              maxonsd         = metastats.Max () / metastats.SD      () /  11.00;
    double              maxcov          = metastats.CoV ()                        /   4.00;
    double              compound        = Power ( maxonavg * maxonmedian * maxonsd * maxcov, 1/4.0 );

    int                 maxeli;
    double              maxavg;

//    DBGV4 ( maxonavg, maxonmedian, maxonsd, maxonavg + maxonmedian + maxonsd, "3 criterion" );

                                        // vote for a majority of criterion
//    if ( maxonavg + maxonmedian + maxonsd >= 2 ) {
//    if ( compound > limit ) {
    if ( compound > 0.30 ) {
//    if ( ! VkEscape () ) {
                                        // find worse track
        maxavg      = 0;
        maxeli      = 0;

        for ( int e = 0; e < numel; e++ )

            if ( ! ( BadTracks[ e ] || AuxTracks[ e ] || Highlighted[ e ] )
              && measure[ e ] > maxavg ) {

                maxavg  = measure[ e ];
                maxeli  = e;
                }
                                        // remove it
        Highlighted.Set ( maxeli );

//        ShowNow ();
//        DBGV3 ( metastats.Average (), metastats.SD (), metastats.CoV (), "Absolute Average: Avg SD CoV" );
//      DBGV3 ( metastats.Median (), metastats.MAD (), metastats.Max, "Absolute Average: Median MAD Max" );
//        DBGV5 ( maxeli + 1, metastats.Max / metastats.Average (), metastats.Max / metastats.Median (), metastats.Max / metastats.SD (), metastats.CoV (), "el#  Max/Average  Max/Median  Max/SD  CoV" );
//        DBGV2 ( maxeli + 1, criterion, "el#  criterion" );
//        DBGV2 ( maxeli + 1, maxonavg + maxonmedian + maxonsd + maxcov + maxcompound, "el#  #criterion" );
//        DBGV2 ( maxeli + 1, compound, "el#  compound" );
//        DBGV3 ( maxeli + 1, maxavg, metastats.CoV (), "el#  avgabs  CoV" );
        }
    else {
//      ShowMessage ( "Absolute average criterion does not apply...", "Tracks Scanning" );
        break;
        }

    } while ( (int) Highlighted < numel );


Invalidate ( false );
}

                                        // not that great, results vary according to current filters, like 50Hz
/*void    TTracksView::ScanTracksTooMuchVariability ( TArray2<float> &data, TSelection &goodtf )
{
int                 numel           = EEGDoc->GetNumElectrodes ();
int                 numtotalel      = EEGDoc->GetTotalElectrodes ();
//TTracks<float>      EegBuff ( numtotalel, 1 );
TTracks<float>      EegBuff ( numtotalel, 3 );
TArray1<double>     EegAvg ( numtotalel );
long                fromtf          = EEGDoc->GetNumTimeFrames () * 0.10;
long                totf            = EEGDoc->GetNumTimeFrames () * 0.90;
int                 step            = max ( Round ( ( totf - fromtf + 1 ) / 100.0 ), 1 );
TEasyStats         *stats           = new TEasyStats[ numtotalel ];
TEasyStats          metastats ( numel );
TArray1<double>     measure ( numel );

TTracksParams       gtparams ( AtomTypeUseCurrent, NoPseudoTracks, ReferenceUsingCurrent );


                                        // scan TFs
for ( long tf = fromtf; tf <= totf - 2; tf += step ) {
                                        // get tracks
    EEGDoc->GetTracks ( tf, tf + 2, EegBuff, 0, &gtparams );

    for ( int e = 0; e < numtotalel; e++ )
                                        // look at the average of absolute values
        stats[ e ].Add ( fabs ( EegBuff[ e ][ 1 ] - EegBuff[ e ][ 0 ] ) );
    }

                                        // extract our measure
for ( int e = 0; e < numel; e++ )
//    measure[ e ]    = stats[ e ].Average ();
    measure[ e ]    = stats[ e ].SD ();



do {
    metastats.Reset ();

    for ( int e = 0; e < numel; e++ )
        if ( ! ( BadTracks[ e ] || AuxTracks[ e ] || Highlighted[ e ] ) )
            metastats.Add ( measure[ e ] );


//    DBGV3 ( metastats.Average (), metastats.SD (), metastats.CoV (), "Absolute Average: Avg SD CoV" );
//    DBGV3 ( metastats.Median (), metastats.MAD (), metastats.Max, "Absolute Average: Median MAD Max" );
//    DBGV3 ( metastats.Max / metastats.Average (), metastats.Max / metastats.Median (), metastats.Max / metastats.SD (), "Max/Average  Max/Median  Max/SD" );

    if ( metastats.Average () == 0 || metastats.Median () == 0 || metastats.SD () == 0 )
        break;

                                        // we can compute these criterion
//  bool                maxcov          = metastats.CoV () > 0.64;
//  bool                max             = metastats.Max > 7.00;
    bool                maxonavg        = metastats.Max / metastats.Average () > 5.35;
    bool                maxonmedian     = metastats.Max / metastats.Median  () > 6.80;
    bool                maxonsd         = metastats.Max / metastats.SD      () > 8.50;

    int                 maxeli;
    double              maxavg;

//    DBGV4 ( maxonavg, maxonmedian, maxonsd, maxonavg + maxonmedian + maxonsd, "3 criterion" );

                                        // vote for a majority of criterion
    if ( maxonavg + maxonmedian + maxonsd >= 2 ) {
                                        // find worse track
        maxavg      = 0;
        maxeli      = 0;

        for ( int e = 0; e < numel; e++ )
            if ( ! ( BadTracks[ e ] || AuxTracks[ e ] || Highlighted[ e ] )
              && measure[ e ] > maxavg ) {
                maxavg  = measure[ e ];
                maxeli  = e;
                }
                                        // remove it
        Highlighted.Set ( maxeli );
//        ShowNow ();
//        DBGV3 ( maxeli + 1, maxavg, metastats.CoV (), "el#  avgabs  CoV" );
        }
    else {
//      ShowMessage ( "Absolute average criterion does not apply...", "Tracks Scanning" );
        break;
        }

    } while ( (int) Highlighted < numel );



delete[] stats;

Invalidate ( false );
}
*/

/*
void    TTracksView::ScanTracksNoisy (  TArray2<float> &data, TSelection &goodtf, bool lownoise )
{
int                 numel           = EEGDoc->GetNumElectrodes ();
int                 numtotalel      = EEGDoc->GetTotalElectrodes ();
//TTracks<float>      EegBuff ( numtotalel, 1 );
TTracks<float>      EegBuff ( numtotalel, 3 );
TArray1<double>     EegAvg ( numtotalel );
long                fromtf          = EEGDoc->GetNumTimeFrames () * 0.10;
long                totf            = EEGDoc->GetNumTimeFrames () * 0.90;
int                 step            = max ( Round ( ( totf - fromtf + 1 ) / 100.0 ), 1 );
TEasyStats         *stats           = new TEasyStats[ numtotalel ];
TEasyStats          metastats ( numel );
TArray1<double>     measure ( numel );

TTracksParams       gtparams ( AtomTypeUseCurrent, NoPseudoTracks, ReferenceUsingCurrent );


                                        // scan TFs
for ( long tf = fromtf; tf <= totf - 2; tf += step ) {
                                        // get tracks
    EEGDoc->GetTracks ( tf, tf + 2, EegBuff, 0, &gtparams );

    for ( int e = 0; e < numtotalel; e++ )
                                        // look at the absolute Laplacian
//        stats[ e ].Add ( fabs ( EegBuff[ e ][ 1 ] - EegBuff[ e ][ 0 ] ) );  // <- code was tuned with that, just abs difference!
        stats[ e ].Add ( fabs ( EegBuff[ e ][ 2 ] + EegBuff[ e ][ 0 ] - 2 * EegBuff[ e ][ 1 ] ) );
    }

                                        // extract our measure
for ( int e = 0; e < numel; e++ )
//  measure[ e ]    = stats[ e ].Average ();
    measure[ e ]    = stats[ e ].SD ();



do {
    metastats.Reset ();

    for ( int e = 0; e < numel; e++ )
        if ( ! ( BadTracks[ e ] || AuxTracks[ e ] || Highlighted[ e ] ) )
            metastats.Add ( measure[ e ] );


    if ( metastats.Average () == 0 || metastats.Median () == 0 || metastats.SD () == 0 )
        break;

                                        // we can compute these criterion
//    bool                maxonavg        = metastats.Max / metastats.Average () > ( lownoise ? 2.10 /*2.50* / : 4.10 ); // for Difference case
//    bool                maxonmedian     = metastats.Max / metastats.Median  () > ( lownoise ? 2.20 /*3.00* / : 4.85 );
//    bool                maxonsd         = metastats.Max / metastats.SD      () > ( lownoise ? 4.50 /*5.00* / : 6.60 );

    bool                cov             = metastats.CoV () > 0.66;
    bool                maxonavg        = metastats.Max / metastats.Average () > 5.70;
    bool                maxonmedian     = metastats.Max / metastats.Median  () > 6;
    bool                maxonsd         = metastats.Max / metastats.SD      () > 8.5;
    bool                diffavgmed      = RelativeDifference ( metastats.Average (), metastats.Median () ) > 0.20;

    int                 maxeli;
    double              maxavg;

//    DBGV3 ( metastats.Average (), metastats.Median (), RelativeDifference ( metastats.Average (), metastats.Median () ), "Avg Median RelDiff" );
//    DBGV5 ( cov, maxonavg, maxonmedian, maxonsd, diffavgmed, "5 criterion" );

                                        // vote for a majority of criterion
    if ( cov + maxonavg + maxonmedian + maxonsd + diffavgmed >= 2 ) {
//    if ( ! VkEscape () ) {
                                        // find worse track
        maxavg      = 0;
        maxeli      = 0;

        for ( int e = 0; e < numel; e++ )
            if ( ! ( BadTracks[ e ] || AuxTracks[ e ] || Highlighted[ e ] )
              && measure[ e ] > maxavg ) {
                maxavg  = measure[ e ];
                maxeli  = e;
                }
                                        // remove it
        Highlighted.Set ( maxeli );

//        ShowNow ();

//      DBGV3 ( metastats.Average (), metastats.SD (), metastats.CoV (), "Absolute Average: Avg SD CoV" );
//      DBGV3 ( metastats.Median (), metastats.MAD (), metastats.Max, "Absolute Average: Median MAD Max" );
//        DBGV3 ( metastats.Max / metastats.Average (), metastats.Max / metastats.Median (), metastats.Max / metastats.SD (), "Max/Average  Max/Median  Max/SD" );
//        DBGV3 ( maxeli + 1, maxavg, metastats.CoV (), "el#  avgabs  CoV" );
        }
    else {
//      ShowMessage ( "Absolute average criterion does not apply...", "Tracks Scanning" );
        break;
        }

    } while ( (int) Highlighted < numel );



delete[] stats;

Invalidate ( false );
}
*/


//----------------------------------------------------------------------------
                                        // Scan the electrodes which, once removed, affect the most the GFP
                                        // This gives some artifacted electrodes
void    TTracksView::ScanTracksGfp ( TArray2<float> &data, TSelection &goodtf )
{
int                 numel           = EEGDoc->GetNumElectrodes ();
int                 numtf           = data.GetDim2 ();
TArray1<double>     gfp     ( numtf );
TArray1<double>     gfpenot ( numtf );
TEasyStats          stats;
TEasyStats          metastats ( numtf );
double              sumdiff;
double              maxdiff;
int                 maxel;
//double              reldiff;
//double              absdiff;
//double              sumgfp;
//double              powerdiff;
//double              sumpower;


do {
                                        // compute current gfp
    for ( TIteratorSelectedForward tfi ( goodtf ); (bool) tfi; ++tfi ) {

        stats.Reset ();

        for ( int e = 0; e < numel; e++ )
            if ( ! ( BadTracks[ e ] || AuxTracks[ e ] || Highlighted[ e ] ) )
                stats.Add ( data ( e, tfi() ) );

        gfp ( tfi() )   = stats.SD ();
        } // for tf

                                        // look for most contributing electrode
    maxdiff     = -1;
    maxel       = -1;

                                        // test by removing each electrode one at a time
    for ( int enot = 0; enot < numel; enot++ ) {

        metastats.Reset ();
//        reldiff     = 0;
//        absdiff     = 0;
//        sumgfp      = 0;
//        powerdiff   = 0;
//        sumpower    = 0;

        for ( TIteratorSelectedForward tfi ( goodtf ); (bool) tfi; ++tfi ) {
                                        // SD without current electrode
            stats.Reset ();
            for ( int e = 0; e < numel; e++ )
                if ( ! ( e == enot || BadTracks[ e ] || AuxTracks[ e ] || Highlighted[ e ] ) )
                    stats.Add ( data ( e, tfi() ) );
            gfpenot ( tfi() )   = stats.SD ();


//            reldiff    += RelativeDifference ( gfpenot ( tf ), gfp ( tf ) );
//            absdiff    += fabs ( gfpenot ( tf ) - gfp ( tf ) );
//            sumgfp     += ( gfpenot ( tf ) + gfp ( tf ) ) / 2;
//            powerdiff  += Power ( gfpenot ( tf ) - gfp ( tf ), 2 );
//            sumpower   += Power ( ( gfpenot ( tf ) + gfp ( tf ) ) / 2, 2 );

            metastats.Add ( RelativeDifference ( gfpenot ( tfi() ), gfp ( tfi() ) ) );
            } // for tf

//        reldiff    /= (int) goodtf;
//        absdiff    /= sumgfp   / 1000;
//        powerdiff  /= sumpower / 1000;

//        sumdiff     = reldiff;
//        sumdiff     = absdiff;
//        sumdiff     = metastats.Average ();
//        sumdiff     = metastats.Median ();  // more solid?
//        sumdiff     = metastats.SD ();  // also good
//        sumdiff     = metastats.Average () * metastats.Median () * metastats.SD () * 1e6;   // compound 3
        sumdiff     = metastats.Average () * metastats.SD () * 1e3; // compound 2 - pretty solid

                                        // keep electrode which removal has the most impact
        if ( sumdiff > maxdiff ) {
            maxdiff     = sumdiff;
            maxel       = enot;
            }
        } // for enot


//    if ( reldiff > 0.018 ) {   // RelativeDifference / #TF

//    if ( absdiff > 1.00 ) {   // fabs  difference / avg gfp
//    if ( maxdiff > 1.00 ) { // 0.10  // Power difference / avg gfp
//    if ( powerdiff > 0.007 ) {   // Power difference / Power avg gfp

//    if ( maxdiff > 10.00 /*5.00*/ ) {   // RelDiff compound 3
    if ( maxdiff > 1.00 ) {   // RelDiff compound 2
//    if ( ! VkEscape () ) {
                                        // remove it
        Highlighted.Set ( maxel );

//        ShowNow ();
//        DBGV2 ( maxel + 1, maxdiff, "el#  maxdiff" );
        }
    else {
//      ShowMessage ( "criterion does not apply...", "Tracks Scanning" );
        break;
        }

    } while ( (int) Highlighted < numel );


Invalidate ( false );
}
                                        // Scan how each track deviates from the average track, in average
                                        // then remove tracks too far from the average
                                        // Works pretty well, but have to skip blinks periods f.ex., as it pollutes the extremum
/*void    TTracksView::ScanTracksSD ( TArray2<float> &data, TSelection &goodtf )
{
int                 numel           = EEGDoc->GetNumElectrodes ();
int                 numtf           = data.GetDim2 ();
TArray1<double>     avg     ( numtf );
TArray1<double>     sd      ( numtf );
TEasyStats          stats   ( max ( numtf, numel ) );
double              sumdiff;
double              maxdiff;
int                 maxel;
//double              limit           = GetValue ( "Noise level (3.5):", "Analyze Tracks" );


do {
                                        // ?
    UpdateTimeRange ( data, goodtf );

                                        // compute current avg & sd
    for ( TIteratorSelectedForward tfi ( goodtf ); (bool) tfi; ++tfi ) {

        stats.Reset ();

        for ( int e = 0; e < numel; e++ )
            if ( ! ( BadTracks[ e ] || AuxTracks[ e ] || Highlighted[ e ] ) )
                stats.Add ( data ( e, tfi() ) );

//        avg ( tfi() )      = stats.Average ();
//        sd  ( tfi() )      = stats.SD () ? stats.SD () : 1;
        avg ( tfi() )      = stats.Median ();
        sd  ( tfi() )      = stats.MAD () ? stats.MAD () : 1;
        } // for tfi


/*    avg ( numtf )   = 0;
    sd  ( numtf )   = 0;
    for ( TIteratorSelectedForward tfi ( goodtf ); (bool) tfi; ++tfi ) {
        avg ( numtf ) += fabs ( avg ( tfi() ) );
        sd  ( numtf ) += sd  ( tfi() );
        }
    avg ( numtf )  /= (int) goodtf;
    sd  ( numtf )  /= (int) goodtf;* /

                                        //
    maxdiff     = -DBL_MAX;
    maxel       = -1;


    for ( int e = 0; e < numel; e++ ) {

        stats.Reset ();

        if ( BadTracks[ e ] || AuxTracks[ e ] || Highlighted[ e ] )
            continue;

        for ( TIteratorSelectedForward tfi ( goodtf ); (bool) tfi; ++tfi )
            stats.Add ( fabs ( data ( e, tfi() ) - avg ( tfi() ) ) / sd ( tfi() ) );

//        sumdiff     = stats.Average ();
//        sumdiff     = stats.Max;    // help a bit?

//        sumdiff     = stats.SD ();    // not bad
//        sumdiff     = stats.Average () * stats.SD (); // compound 2 - better
//        sumdiff     = stats.Average () * stats.Variance () / 1e3; // compound 2 - better
//        sumdiff     = stats.Average () * stats.SD () * stats.Max / 1e3; // compound 3?

//        sumdiff     = stats.Median () * stats.MAD ();   // interesting, picks the noisy tracks (not the most saturated though)
        sumdiff     = stats.Median ();  // -> good: threshold @ 3.75
//        sumdiff     = stats.MAD ();

                                        // keep electrode
        if ( sumdiff > maxdiff ) {
            maxdiff     = sumdiff;
            maxel       = e;
            }
        }

//    if ( maxdiff > 250 ) {
//    if ( maxdiff > limit ) {
//    if ( maxdiff > 3.0 ) {  // 2 or 2.5 for stronger effect
    if ( ! VkEscape () ) {
                                        // remove it
        Highlighted.Set ( maxel );

        ShowNow ();
        DBGV2 ( maxel + 1, maxdiff, "el#  maxdiff" );
        }
    else {
//      ShowMessage ( "criterion does not apply...", "Tracks Scanning" );
        break;
        }

    } while ( (int) Highlighted < numel );


Invalidate ( false );
}
*/

//----------------------------------------------------------------------------
void    TTracksView::CmSetIntensityMode ()
{
if ( ShiftKey ) {
    if      ( IsTracksMode ()          )     SetIntensitySmoothMode ();
    else if ( IsBarsMode ()            )     SetTracksMode ();
    else if ( IsIntensitySquareMode () )     SetBarsMode ();
    else if ( IsIntensitySmoothMode () )     SetIntensitySquareMode ();
    }
else {
    if      ( IsTracksMode ()          )     SetBarsMode ();
    else if ( IsBarsMode ()            )     SetIntensitySquareMode ();
    else if ( IsIntensitySquareMode () )     SetIntensitySmoothMode ();
    else if ( IsIntensitySmoothMode () )     SetTracksMode ();
    }


SetPartWindows ( SelTracks, false );

ShowNow ();

RefreshLinkedWindow ( true ) ;
}


void    TTracksView::CmSetContrast ( owlwparam w )
{
double              osc         = ScalingContrast;

if ( EEGDoc->IsAngular ( AtomTypeUseCurrent ) )
                                        ScalingContrast     = ColorTableToScalingContrast ( 1 );
else
    if      ( w == IDB_ISINCCONTRAST )  ScalingContrast     = AtLeast ( 0.01, ScalingContrast ) * 1.50;
    else if ( w == IDB_ISDECCONTRAST )  ScalingContrast    /= 1.50;


SetScalingContrast ( ScalingContrast );

RefreshLinkedWindow ();

if ( osc == ScalingContrast )
    return;

//Invalidate ( false );
ShowNow ();
}


void    TTracksView::CmSetScalingAdapt ()
{
ScalingAuto     = NextState ( ScalingAuto, EEGDoc->IsPositive ( AtomTypeUseCurrent ) ? NumScalingAutoPositive : NumScalingAuto, ShiftKey );

Invalidate ( false );

ButtonGadgetSetState ( IDB_FIXEDSCALE, ScalingAuto != ScalingAutoOff );
}


void    TTracksView::CmNextColorTable ()
{
ColorTable.NextColorTable ( ShiftKey );

ColorTableIndex[ EEGDoc->GetAtomType ( AtomTypeUseCurrent ) ]   = ColorTable.GetTableType ();

Invalidate ( false );
}


void    TTracksView::CmNextRois ()
{
int     nextrois    = TBaseView::NextRois ( IsRoiMode () ? CurrRois : -1, EEGDoc->GetNumElectrodes () );


if ( nextrois == -1 ) {
    ClearRoiMode ();

    CurrRois    = 0;
    if ( Rois ) delete Rois, Rois = 0;
    }
else {
    SetRoiMode ();

    CurrRois    = nextrois;
    if ( Rois ) delete Rois, Rois = 0;
                                        // create a local copy of next rois
    Rois        = new TRois ( (char *) GODoc->GetRoisDoc ( CurrRois )->GetDocPath () );
    }


if ( IsRoiMode () ) {
    Rois->Set ();                       // set all rois
                                        // sum up all rois into a single selection
    Rois->CumulateInto ( SelTracks, EEGDoc->GetFirstRegularIndex (), EEGDoc->GetLastRegularIndex () );
    }
else {
    EEGDoc->SetRegular ( SelTracks );
    }


if ( AverageRois ) {                    // cancel average rois when leaving rois
    UpdateBuffers ( -1, -1, CDPt.GetMin(), CDPt.GetMax() );

    EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_EEG );
    }


ButtonGadgetSetState ( IDB_NEXTROI, IsRoiMode () );

SetPartWindows ( SelTracks, false );
                                        // ROIs have changed
SetTextMargin ();
                                        // refresh now that we have the next text margin
ShowNow();

RefreshLinkedWindow ( true ) ;
}


void    TTracksView::CmAverageRois ()
{
if ( ! ( GODoc && GODoc->GetNumRoiDoc () && IsRoiMode ()
     && ! ( GODoc->GetNumMriDoc () && GODoc->GetNumSpDoc () && GODoc->GetNumIsDoc () ) ) )
    return;


AverageRois     = ! AverageRois;

UpdateBuffers ( -1, -1, CDPt.GetMin(), CDPt.GetMax() );

EEGDoc->NotifyViews ( vnReloadData, EV_VN_RELOADDATA_EEG );

SetPartWindows ( SelTracks );           // used to set only one track visible in case of averaging
//Invalidate ( false );

ButtonGadgetSetState ( IDB_AVERAGEROIS, AverageRois );
}


void    TTracksView::CmSetIntensityEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsIntensityModes () );
}


void    TTracksView::CmSetBrightnessEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsIntensityModes () && ScalingAuto == ScalingAutoOff );
}


void    TTracksView::CmSetContrastEnable ( TCommandEnabler &tce )
{
tce.Enable ( IsIntensityModes () && ! EEGDoc->IsAngular ( AtomTypeUseCurrent ) );
}


void    TTracksView::CmTracksEnable ( TCommandEnabler &tce )
{
tce.Enable ( ! IsIntensityModes () );
}


void    TTracksView::CmNextRoisEnable ( TCommandEnabler &tce )
{
tce.Enable ( GODoc && GODoc->GetNumRoiDoc () );
}


void    TTracksView::CmAverageRoisEnable ( TCommandEnabler &tce )
{
tce.Enable ( GODoc && GODoc->GetNumRoiDoc () && IsRoiMode ()
          && ! ( GODoc->GetNumMriDoc () && GODoc->GetNumSpDoc () && GODoc->GetNumIsDoc () ) );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
