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

#include    <typeinfo>

#include    <owl/pch.h>
#include    <owl/buttonga.h>
#include    <owl/statusba.h>
#include    <owl/timegadg.h>
#include    <owl/decframe.h>
#include    <owl/doctpl.h>
#include    <owl/decmdifr.h>
#include    <owl/gdiobjec.h>

#include    <io.h>
#include    <htmlhelp.h>

#include    <owl/cmdline.h>

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "TCartoolApp.h"
#include    "TCartoolMdiChild.h"
#include    "TCartoolVersionInfo.h"
#include    "TCartoolAboutDialog.h"

#include    "Volumes.AnalyzeNifti.h"
#include    "Volumes.TTalairachOracle.h"
#include    "TExportTracks.h"
#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"

#include    "TLinkManyDoc.h"

#include    "TBaseView.h"

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr char*     CmdLineRegister         = "register";
constexpr char*     CmdLineUnregister       = "unregister";
constexpr char*     CmdLineNoRegister       = "noregister";
constexpr char*     CmdLineResetRegister    = "resetregister";
constexpr char*     CmdLineMainWindowWidth  = "mainwidth";
constexpr char*     CmdLineMainWindowHeight = "mainheight";
constexpr char*     CmdLineMainWindowTop    = "maintop";
constexpr char*     CmdLineMainWindowLeft   = "mainleft";
constexpr char*     CmdLineMainWindowMax    = "mainmaximized";
constexpr char*     CmdLineMainWindowMin    = "mainminimized";
constexpr char*     CmdLineMainWindowRest   = "mainnormal";


enum        RegistrationOpeningType
            {
            NoOpening           =   0x00,

            OpenCartool         =   0x01,
            OpenNotePad         =   0x02,
            OpenWordPad         =   0x04,

            OpenForceRegistry   =   0x10,         // Some files (Biologic, Deltamed and BrainVision f.ex.) share some extensions, so we need to force overwriting the registry for both files
            };


class       FileRegistrationInfo
{
public:
    const char      Extension   [ 16 ];
    const char      DisplayText [ 64 ];
    int             IconResource;
    int             OpeningType;

    int             IconIndex   ()          {   return  IconResource - IDI_MDIAPPLICATION; }    // Icon index starts from application icon, with a relative index starting from 0
};


constexpr int       NumExtensionRegistrations   = 43;

FileRegistrationInfo    ExtReg[ NumExtensionRegistrations ] =
            {
            { FILEEXT_EEGBVDAT,     "BrainVision",                      IDI_EEGBINARY,      OpenCartool                 | OpenWordPad   | OpenForceRegistry },
            { FILEEXT_EEGBV,        "BrainVision/Biologic",             IDI_EEGBINARY,      OpenCartool                 | OpenWordPad   | OpenForceRegistry },
            { FILEEXT_EEGEP,        "EP",                               IDI_EEGTEXT,        OpenCartool                 | OpenWordPad                       },
            { FILEEXT_EEGEPH,       "EP + Header",                      IDI_EEGTEXT,        OpenCartool                 | OpenWordPad                       },
            { FILEEXT_EEGSEF,       "Simple Eeg Format",                IDI_EEGBINARY,      OpenCartool                 | OpenWordPad                       },
            { FILEEXT_EEGD,         "EasRec D",                         IDI_EEGBINARY,      OpenCartool                 | OpenWordPad                       },
            { FILEEXT_EEG128,       "Depth 128",                        IDI_EEGBINARY,      OpenCartool                 | OpenWordPad                       },
            { FILEEXT_EEGNSR,       "EGI NetStation",                   IDI_EEGBINARY,      OpenCartool                 | OpenWordPad                       },
            { FILEEXT_EEGMFF,       "EGI MFF",                          IDI_EEGBINARY,      OpenCartool                 | OpenWordPad                       },
            { FILEEXT_EEGNSRRAW,    "EGI RAW",                          IDI_EEGBINARY,      OpenCartool                 | OpenWordPad                       },
            { FILEEXT_EEGNSCNT,     "NeuroScan CNT",                    IDI_EEGBINARY,      OpenCartool                 | OpenWordPad                       },
            { FILEEXT_EEGNSAVG,     "NeuroScan AVG",                    IDI_EEGTEXT,        OpenCartool                 | OpenWordPad                       },
            { FILEEXT_EEGBDF,       "Biosemi BDF",                      IDI_EEGBINARY,      OpenCartool                 | OpenWordPad                       },
            { FILEEXT_EEGEDF,       "EDF",                              IDI_EEGBINARY,      OpenCartool                 | OpenWordPad                       },
            { FILEEXT_EEGRDF,       "ERPSS RDF",                        IDI_EEGBINARY,      OpenCartool                 | OpenWordPad                       },
            { FILEEXT_EEGTRC,       "micromed TRC",                     IDI_EEGBINARY,      OpenCartool                 | OpenWordPad                       },

//          { FILEEXT_NSRRAWGAIN,   "NetStation Calibration",           IDI_CALIBRATION,                  OpenNotePad                                       },
//          { FILEEXT_NSRRAWZERO,   "NetStation Calibration",           IDI_CALIBRATION,                  OpenNotePad                                       },

            { FILEEXT_EEGEPSD,      "EP Stand Dev",                     IDI_EEGSD,          OpenCartool                 | OpenWordPad                       },
            { FILEEXT_EEGEPSE,      "EP Stand Err",                     IDI_EEGSD,          OpenCartool                 | OpenWordPad                       },
            { FILEEXT_TVA,          "Triggers Validation",              IDI_TVA,                          OpenNotePad                                       },
            { FILEEXT_MRK,          "Markers",                          IDI_MRK,                          OpenNotePad                                       },
            { FILEEXT_MTG,          "Montage",                          IDI_MTG,                          OpenNotePad                                       },

            { FILEEXT_SEG,          "Segments",                         IDI_SEG,            OpenCartool                 | OpenWordPad                       },
            { FILEEXT_DATA,         "Data",                             IDI_DATA,           OpenCartool                 | OpenWordPad                       },
            { FILEEXT_FREQ,         "Frequency Analysis",               IDI_FREQ,           OpenCartool                 | OpenWordPad                       },

            { FILEEXT_XYZ,          "Head Coordinates",                 IDI_XYZCOORD,       OpenCartool | OpenNotePad                                       },
            { FILEEXT_ELS,          "Electrodes Setup",                 IDI_XYZCOORD,       OpenCartool | OpenNotePad                                       },

            { FILEEXT_SPIRR,        "Sol. Points Irregular",            IDI_SPIRR,          OpenCartool                 | OpenWordPad                       },
            { FILEEXT_LOC,          "Sol. Points BESA",                 IDI_SPIRR,          OpenCartool                 | OpenWordPad                       },
            { FILEEXT_SXYZ,         "Loreta Coordinates",               IDI_SPIRR,          OpenCartool                 | OpenWordPad                       },
            { FILEEXT_LF,           "Lead Field Matrix",                IDI_LEADFIELD,      NoOpening                                                       },
            { FILEEXT_LFT,          "Lead Field Matrix",                IDI_LEADFIELD,      NoOpening                                                       },
            { FILEEXT_IS,           "Inverse Solution Matrix",          IDI_INVERSEMATRIX,  OpenCartool                 | OpenWordPad                       },
            { FILEEXT_SPINV,        "Inverse Solution, Loreta",         IDI_INVERSEMATRIX,  OpenCartool                 | OpenWordPad                       },
            { FILEEXT_RIS,          "Result of Inverse Solution",       IDI_INVERSERESULTS, OpenCartool                 | OpenWordPad                       },

            { FILEEXT_MRIAVS,       "MRI AVS",                          IDI_MRI,            OpenCartool                 | OpenWordPad                       },
            { FILEEXT_MRIAVW_HDR,   "MRI Analyze/Nifti" ,               IDI_MRI,            OpenCartool                 | OpenWordPad                       },
            { FILEEXT_MRIAVW_IMG,   "MRI Analyze/Nifti",                IDI_MRI,            OpenCartool                 | OpenWordPad                       },
            { FILEEXT_MRINII,       "MRI Nifti",                        IDI_MRI,            OpenCartool                 | OpenWordPad                       },
            { FILEEXT_MRIVMR,       "MRI BrainVoyager",                 IDI_MRI,            OpenCartool                 | OpenWordPad                       },

            { FILEEXT_ROIS,         "Regions of Interest",              IDI_ROIS,           OpenCartool | OpenNotePad                                       },
                                 // additional space is intended, and will make it appear above all the others
            { FILEEXT_LM,           " " "Cartool Link Many",            IDI_LM,             OpenCartool | OpenNotePad                                       },
            { FILEEXT_VRB,          "Verbose",                          IDI_VERBOSE,                      OpenNotePad                                       },
            };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

extern  TTalairachOracle    Taloracle;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE2( TCartoolApp, TRecentFiles, TApplication )

    EV_COMMAND          ( CM_HELPABOUT,                 CmHelpAbout ),
    EV_COMMAND_AND_ID   ( CM_HELPCONTENTS,              CmHelpContents ),
    EV_COMMAND_AND_ID   ( CM_HELPCARTOOLCOMMUNITY,      CmHelpContents ),
    EV_COMMAND_AND_ID   ( CM_HELPCARTOOLGITHUB,         CmHelpContents ),
//  EV_COMMAND_AND_ID   ( CM_HELPWEBUSERSGUIDE,         CmHelpContents ),
    EV_COMMAND_AND_ID   ( CM_HELPWEBREFERENCEGUIDE,     CmHelpContents ),
    EV_COMMAND_AND_ID   ( CM_HELPWEBRELEASES,           CmHelpContents ),
    EV_COMMAND_AND_ID   ( CM_HELPWEBFBMLAB,             CmHelpContents ),
    EV_COMMAND_AND_ID   ( CM_HELPWEBCIBM,               CmHelpContents ),
    EV_COMMAND_AND_ID   ( CM_HELPAUTOUPDATE,            CmHelpContents ),
//  EV_COMMAND          ( CM_HELPUSING,                 CmHelpUsing),

    EV_WM_DROPFILES,
    EV_REGISTERED       ( MruFileMessage,               CmFileSelected ),

    EV_OWLDOCUMENT      ( dnCreate,                     EvOwlDocument ),    // documents
    EV_OWLDOCUMENT      ( dnRename,                     EvOwlDocument ),
    EV_OWLDOCUMENT      ( dnClose,                      EvOwlDocument ),

    EV_OWLVIEW          ( dnCreate,                     EvNewView ),        // views
    EV_OWLVIEW          ( dnClose,                      EvCloseView ),

    EV_COMMAND_AND_ID   ( CM_RESETREGISTRY,             CmPrefsRegistry ),
    EV_COMMAND_AND_ID   ( CM_CLEARREGISTRY,             CmPrefsRegistry ),
    EV_COMMAND_AND_ID   ( CM_GRAPHICACCELAUTO,          CmPrefsGraphic ),
    EV_COMMAND_AND_ID   ( CM_GRAPHICACCELON,            CmPrefsGraphic ),
    EV_COMMAND_AND_ID   ( CM_GRAPHICACCELOFF,           CmPrefsGraphic ),
    EV_COMMAND_AND_ID   ( CM_GRAPHIC3DTEXTURESAUTO,     CmPrefsGraphic ),
    EV_COMMAND_AND_ID   ( CM_GRAPHIC3DTEXTURESON,       CmPrefsGraphic ),
    EV_COMMAND_AND_ID   ( CM_GRAPHIC3DTEXTURESOFF,      CmPrefsGraphic ),
    EV_COMMAND_ENABLE   ( CM_GRAPHICACCELAUTO,          CmPrefsGraphicAccelAutoEnable ),
    EV_COMMAND_ENABLE   ( CM_GRAPHICACCELON,            CmPrefsGraphicAccelOnEnable ),
    EV_COMMAND_ENABLE   ( CM_GRAPHICACCELOFF,           CmPrefsGraphicAccelOffEnable ),
    EV_COMMAND_ENABLE   ( CM_GRAPHIC3DTEXTURESAUTO,     CmPrefsGraphic3DTexturesAutoEnable ),
    EV_COMMAND_ENABLE   ( CM_GRAPHIC3DTEXTURESON,       CmPrefsGraphic3DTexturesOnEnable ),
    EV_COMMAND_ENABLE   ( CM_GRAPHIC3DTEXTURESOFF,      CmPrefsGraphic3DTexturesOffEnable ),

END_RESPONSE_TABLE;


//----------------------------------------------------------------------------
                                        // tomodule is 0 for non-interactive application, f.ex. when running tests
                                        // Expected files at root directory: HelpShortFileName, TalairachOracleFileName
                                        // 'name' will be overridden with actual exe file name + revision + optional debug/console flag
        TCartoolApp::TCartoolApp ( LPCTSTR name, TModule*& tomodule, TAppDictionary* appdir )
      : TApplication ( name, tomodule, appdir ),
        TRecentFiles ( ".\\Cartool.ini", 9 ),
        TPreferences ( TRegKey::GetCurrentUser (), CartoolRegistryUserHome )
{
CartoolApplication  = this;

CartoolDocManager   = new TCartoolDocManager ( dmMDI, this );

SetDocManager ( CartoolDocManager );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ApxHarbor           = 0;
ControlBar          = 0;

LastActiveBaseView  = 0;

HelpState           = false;
ContextHelp         = false;
HelpCursor          = 0;

Splash              = 0;

Bitmapping          = false;
Closing             = false;
AnimateViews        = true;

ApplicationFullPath .Reset ();
ApplicationDir      .Reset ();
ClearString ( ApplicationFileName, ShortStringLength );
HelpFullPath        .Reset ();

ClearString ( ProdName,     ShortStringLength );
ClearString ( ProdVersion,  ShortStringLength );
ClearString ( ProdRevision, ShortStringLength );
ClearString ( DefaultTitle, MaxAppTitleLength );

PrefGraphic3DTextures   = FormatDontUse;
PrefGraphicAccel        = FormatDontUse;

ClearString ( Title,       MaxAppTitleLength );
ClearString ( TitlePrefix, MaxAppTitleLength );

nCmdShow            = SW_SHOWMAXIMIZED; // default, which can be overriden with options


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // not interactive mode (console, stand-alone app, unit tests)
//bool                isinteractrive  = *tomodule != 0;
                                        // we can actually retrieve caller module this way, though:
//HMODULE             moduleh         = GetModuleHandle ( 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

RetrievePreferences ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // retrieve and save full path to executable
::GetModuleFileName ( 0, ApplicationFullPath, ApplicationFullPath.Size () );

                                        // extract executable file name
StringCopy          ( ApplicationFileName, ApplicationFullPath );
GetFilename         ( ApplicationFileName );

                                        // extract only the directory
StringCopy          ( ApplicationDir, ApplicationFullPath );
RemoveFilename      ( ApplicationDir );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // expect the help file to be in the same directory as exe
StringCopy          ( HelpFullPath,     ApplicationDir,     "\\",   HelpShortFileName );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // expect the Talairach file to be in the same directory as executable
TFileName           TaloracleFile;

StringCopy          ( TaloracleFile,    ApplicationDir,     "\\",   TalairachOracleFileName );

Taloracle.Read      ( TaloracleFile );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read these global variables
LPSTR               tostr = 0;
TCartoolVersionInfo applVersion ( this );

applVersion.GetProductName     ( tostr );
StringCopy      ( ProdName,      tostr );

applVersion.GetProductVersion  ( tostr );
StringCopy      ( ProdVersion,   tostr );

applVersion.GetProductRevision ( tostr );
StringCopy      ( ProdRevision,  tostr );

                                        // Compose default title: real file name + version + revision
StringCopy      ( DefaultTitle, ApplicationFileName );

if ( StringIsNotEmpty ( ProdVersion ) )
    StringAppend    ( DefaultTitle, "  ", ProdVersion );

if ( StringIsNotEmpty ( ProdRevision ) )
    StringAppend    ( DefaultTitle, "  (", ProdRevision, ")" );


#if defined (_CONSOLE)
StringAppend    ( DefaultTitle, "  " "CONSOLE" );
#elif defined (_DEBUG)
StringAppend    ( DefaultTitle, "  " "DEBUG" );
#endif

SetName         ( DefaultTitle );
}


        TCartoolApp::~TCartoolApp ()
{
}


//----------------------------------------------------------------------------
bool    TCartoolApp::CanClose ()
{
Closing     = true;

bool                result          = TApplication::CanClose();

//Closing     = false;

                                        // Close the help engine if we used it.
//if ( result && HelpState )
//    CartoolMainWindow->WinHelp ( HelpFullPath, HELP_QUIT, 0 );


return result;
}


//----------------------------------------------------------------------------
void    TCartoolApp::InsertGadgets ( TGadget** listgadgets, int numgadgets )
{
TTooltip*           tooltip         = ControlBar->GetTooltip ();


for ( int g = 0; g < numgadgets; g++ ) {

    ControlBar->Insert ( *listgadgets[ g ] );


//  if ( listgadgets[g]->IsVisible () || ! listgadgets[g]->GetId() )
//      ControlBar->Insert ( *listgadgets[g] );


    if ( tooltip ) {
                                        // setting hint from resource string
        TToolInfo	toolInfo	(	ControlBar->GetHandle (),
									listgadgets[ g ]->GetBounds (),
									listgadgets[ g ]->GetId (), 
									LoadString ( listgadgets[ g ]->GetId () )
								);

        tooltip->UpdateTipText ( toolInfo );
        }
    }

                                        // update & show
ControlBar->LayoutSession ();

ControlBar->Invalidate ();

//UpdateApplication;    // ?interferes with Set/Kill focus from child windows?
}


void    TCartoolApp::RemoveGadgets ( int afterID )
{
if ( Closing )  
    return;


TGadget*            tog1;
TGadget*            tog2;

tog1    = ControlBar->GadgetWithId ( afterID );

if ( ! tog1 )
    return;


tog1    = ControlBar->NextGadget ( *tog1 );

while ( tog1 ) {
    tog2    = ControlBar->NextGadget ( *tog1 );
    ControlBar->Remove ( *tog1 );
    tog1    = tog2;
    }

                                        // update & show
ControlBar->LayoutSession ();

ControlBar->Invalidate ();

//UpdateApplication;    // ?interferes with Set/Kill focus from child windows?
}


void    TCartoolApp::CreateGadgets ( bool server )
{
                                        // Many buttons have been removed from standard display..
if ( ! server ) {

//  ControlBar->Insert ( *new TButtonGadget     ( CM_MDIFILENEW,    CM_MDIFILENEW       ) );
    ControlBar->Insert ( *new TButtonGadget     ( CM_FILENEWEMPTYLM,CM_FILENEWEMPTYLM   ) );
//  ControlBar->Insert ( *new TButtonGadget     ( CM_MDIFILEOPEN,   CM_MDIFILEOPEN      ) );
//  ControlBar->Insert ( *new TButtonGadget     ( CM_FILESAVE,      CM_FILESAVE         ) );
//  ControlBar->Insert ( *new TSeparatorGadget  ( ButtonSeparatorWidth                  ) );
    }


//ControlBar->Insert ( *new TButtonGadget       ( CM_EDITCUT,       CM_EDITCUT          ) );
//ControlBar->Insert ( *new TButtonGadget       ( CM_EDITCOPY,      CM_EDITCOPY         ) );
//ControlBar->Insert ( *new TButtonGadget       ( CM_EDITPASTE,     CM_EDITPASTE        ) );
//ControlBar->Insert ( *new TSeparatorGadget    ( ButtonSeparatorWidth                  ) );
ControlBar  ->Insert ( *new TButtonGadget       ( CM_EDITUNDO,      CM_EDITUNDO         ) );
//ControlBar->Insert ( *new TSeparatorGadget    ( ButtonSeparatorWidth                  ) );
//ControlBar->Insert ( *new TButtonGadget       ( CM_EDITFIND,      CM_EDITFIND         ) );
//ControlBar->Insert ( *new TButtonGadget       ( CM_EDITFINDNEXT,  CM_EDITFINDNEXT     ) );

//ControlBar->Insert ( *new TSeparatorGadget    ( ButtonSeparatorWidth                  ) );
//ControlBar->Insert ( *new TButtonGadget       ( CM_HELPCONTENTS,  CM_HELPCONTENTS     ) );


ControlBar->Insert ( *new TSeparatorGadget      ( ButtonSeparatorWidth                  ) );
ControlBar->Insert ( *new TButtonGadget         ( IDB_ADDVIEW, CM_VIEWCREATE            ) );

                                        // Add caption and fly-over help hints.
ControlBar->SetCaption ( "Toolbar" );

ControlBar->SetHintMode ( TGadgetWindow::EnterHints );
}


void    TCartoolApp::SetupSpeedBar ( TDecoratedMDIFrame* frame )
{
ApxHarbor   = new THarbor ( *frame );

                                        // Create default toolbar New and associate toolbar buttons with commands.
ControlBar  = new TDockableControlBar ( frame );

                                        // Create bare minimum gadgets for application
CreateGadgets ();

                                        // Setup the toolbar ID used by OLE 2 for toolbar negotiation.
ControlBar->Attr.Id = IDW_TOOLBAR;

ApxHarbor->Insert ( *ControlBar, alTop );
}


//----------------------------------------------------------------------------
void    TCartoolApp::CreateSplashScreen ()
{
                                        // Splash screen will delete itlsef when timer runs off - no need to keep track of it
Splash  = new TCartoolSplashWindow  (   *unique_ptr<TDib> ( new TDib ( 0, TResId ( IDB_SPLASH ) ) ),  // get resource bitmap
                                        800, 640, TSplashWindow::None,
                                        2.5 * 1000,
                                        0 /*DefaultTitle*/, // putting a title will mess up with the whole size...
                                        this 
                                    );
Splash->Create ();
                                        // force to top
Splash->SetWindowPos ( HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE );
                                        // force redraw now (when busy opening file & starting) (?)
//Splash->RedrawWindow ( 0, 0, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN );
}

                                        // !Something is not correct here!
void    TCartoolApp::DestroySplashScreen ()
{
if ( Splash != 0 ) {

    if ( Splash->IsActive () )  delete  Splash;

    Splash  = 0;
    }
}


//----------------------------------------------------------------------------
void    TCartoolApp::InitMainWindow ()
{
                                        // nCmdShow could be overriden in InitInstance
if ( (   nCmdShow == SW_SHOWMAXIMIZED 
      || nCmdShow == SW_SHOWNORMAL 
      || nCmdShow == SW_SHOWDEFAULT   )
  && IsInteractive () )

    CreateSplashScreen ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

CartoolMdiClient    = new TCartoolMdiClient ( this );

CartoolMainWindow   = new TDecoratedMDIFrame ( Name, IDM_MDI, std::unique_ptr<TMDIClient> ( CartoolMdiClient ), true, this );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

CartoolMainWindow->Attr.Style |= WS_BORDER | WS_CAPTION | WS_CLIPCHILDREN | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME | WS_VISIBLE;
CartoolMainWindow->Attr.Style &= ~(WS_CHILD);

                                        // Enable application to accept Drag & Drop'ped files - Individual windows then also needs some more flags...
CartoolMainWindow->Attr.ExStyle |= WS_EX_ACCEPTFILES;

                                        // Full HD outer window size - Note it is not aware of any screen rescaling, which didn't exist back in the day
int             screenwidth     = GetSystemMetrics ( SM_CXSCREEN );
int             screenheight    = GetSystemMetrics ( SM_CYSCREEN );

CartoolMainWindow->Attr.W       = (int) ( screenwidth  * 0.50 );
CartoolMainWindow->Attr.H       = (int) ( screenheight * 0.75 );
CartoolMainWindow->Attr.X       = AtLeast ( 0, ( screenwidth  - GetWindowWidth  ( CartoolMainWindow ) ) / 2 );
CartoolMainWindow->Attr.Y       = AtLeast ( 0, ( screenheight - GetWindowHeight ( CartoolMainWindow ) ) / 2 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Assign ICON w/ this application.
CartoolMainWindow->SetIcon   ( this, IDI_MDIAPPLICATION );
CartoolMainWindow->SetIconSm ( this, IDI_MDIAPPLICATION );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Associate with the accelerator table.
CartoolMainWindow->Attr.AccelTable = IDM_MDI;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // All toolbars
SetupSpeedBar ( CartoolMainWindow );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SetMainWindow ( CartoolMainWindow );


CartoolMainWindow->SetMenuDescr ( TMenuDescr ( IDM_MDI ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Cursors
CursorSyncViews     = make_unique<TCursor> ( CartoolMainWindow->GetModule()->GetHandle(), TResId ( IDC_SYNCVIEWS  ) );
CursorAddToGroup    = make_unique<TCursor> ( CartoolMainWindow->GetModule()->GetHandle(), TResId ( IDC_ADDTOGROUP ) );
CursorLinkView      = make_unique<TCursor> ( CartoolMainWindow->GetModule()->GetHandle(), TResId ( IDC_LINKVIEW   ) );
CursorMagnify       = make_unique<TCursor> ( CartoolMainWindow->GetModule()->GetHandle(), TResId ( IDC_MAGNIFY    ) );
CursorOperation     = make_unique<TCursor> ( CartoolMainWindow->GetModule()->GetHandle(), TResId ( IDC_OPERATION  ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Message queue threading
EnableMultiThreading ( true );
}


//----------------------------------------------------------------------------
                                        // Called first
void    TCartoolApp::InitInstance ()
{
ProcessCommandLine ( GetCmdLine ().c_str (), true  );   // process early options, some might directly exit the app

TApplication::InitInstance ();                          // then calls InitWindow

ProcessCommandLine ( GetCmdLine ().c_str (), false );   // process files to open
}


//----------------------------------------------------------------------------
void    TCartoolApp::ProcessCommandLine ( const char*   CmdLine,    bool    beforeinit )
{
TCmdLine            cmd ( CmdLine );
char                buff[ MaxPathShort ];

                                        // get next value token and return it converted to integer
auto    GetNextValue    = [] ( TCmdLine& cmd )
{
cmd.NextToken ();

if ( cmd.GetTokenKind () != TCmdLine::Value )
    return  -1;
                
return  StringToInteger ( cmd.GetToken ().c_str () );
};


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // early options, before main window has been created
if ( beforeinit ) {

    for ( cmd.NextToken (); cmd.GetTokenKind () != TCmdLine::Done; cmd.NextToken () ) {

        if ( cmd.GetTokenKind () != TCmdLine::Option )
            continue;


        if      ( StringIs ( cmd.GetToken ().c_str (), CmdLineRegister ) ) {

            RegisterInfo ();

            ::exit ( 0 );
            return;
            }

        else if ( StringIs ( cmd.GetToken ().c_str (), CmdLineUnregister ) ) {

            UnRegisterInfo ();

            ::exit ( 0 );
            return;
            }

        else if ( StringIs ( cmd.GetToken ().c_str (), CmdLineResetRegister ) ) {

            ResetRegisterInfo ();

            ::exit ( 0 );
            return;
            }
                                                                                     
        else if ( StringIs ( cmd.GetToken ().c_str (), CmdLineMainWindowMax     ) )     nCmdShow    = SW_SHOWMAXIMIZED; // overriding main window state before creation
        else if ( StringIs ( cmd.GetToken ().c_str (), CmdLineMainWindowMin     ) )     nCmdShow    = SW_SHOWMINIMIZED;
        else if ( StringIs ( cmd.GetToken ().c_str (), CmdLineMainWindowRest    ) )     nCmdShow    = SW_SHOWNORMAL;

        else if ( StringIs ( cmd.GetToken ().c_str (), CmdLineMainWindowWidth   )
               || StringIs ( cmd.GetToken ().c_str (), CmdLineMainWindowHeight  )
               || StringIs ( cmd.GetToken ().c_str (), CmdLineMainWindowTop     )
               || StringIs ( cmd.GetToken ().c_str (), CmdLineMainWindowLeft    ) )     nCmdShow    = SW_SHOWNORMAL;    // using normal window if these were specified
        }

    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // regular options processing, main window has been created
else { // ! beforeinit

    int             mainwidth       = -1;
    int             mainheight      = -1;
    int             maintop         = -1;
    int             mainleft        = -1;
     

    for ( cmd.NextToken (); cmd.GetTokenKind () != TCmdLine::Done; cmd.NextToken () ) {

        if      ( cmd.GetTokenKind () == TCmdLine::Option ) {

            if      ( StringIs ( cmd.GetToken ().c_str (), CmdLineNoRegister        ) )     continue;   // actually doing nothing
            else if ( StringIs ( cmd.GetToken ().c_str (), CmdLineMainWindowWidth   ) )     mainwidth       = GetNextValue ( cmd );
            else if ( StringIs ( cmd.GetToken ().c_str (), CmdLineMainWindowHeight  ) )     mainheight      = GetNextValue ( cmd );
            else if ( StringIs ( cmd.GetToken ().c_str (), CmdLineMainWindowTop     ) )     maintop         = GetNextValue ( cmd );
            else if ( StringIs ( cmd.GetToken ().c_str (), CmdLineMainWindowLeft    ) )     mainleft        = GetNextValue ( cmd );
            else if ( StringIs ( cmd.GetToken ().c_str (), CmdLineMainWindowMax     ) )     continue;   // already parsed
            else if ( StringIs ( cmd.GetToken ().c_str (), CmdLineMainWindowMin     ) )     continue;   // already parsed
            else if ( StringIs ( cmd.GetToken ().c_str (), CmdLineMainWindowRest    ) )     continue;   // already parsed

            else {

                if ( IsInteractive () )
                    ShowMessage ( cmd.GetToken ().c_str (), "Unknown option:", ShowMessageWarning );
                }

            } // if Option

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we have a "floating" value, certainly due to a prior bad option
        else if ( cmd.GetTokenKind () == TCmdLine::Value ) {

//          if ( IsInteractive () )
//              ShowMessage ( cmd.GetToken ().c_str (), "Unknown option:", ShowMessageWarning );

            } // if Value

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // file name -> open it
        else if ( cmd.GetTokenKind () == TCmdLine::Name ) {

            StringCopy  ( buff, cmd.GetToken ().c_str () );

                                        // special handling for full quotes - shouldn't happen anymore(?)
            if ( buff[ 0 ] == '\"' ) {

                const char*     tob     = StringContains ( cmd.GetLine (), '\"', StringContainsForward  ) + 1;
                const char*     toe     = StringContains ( cmd.GetLine (), '\"', StringContainsBackward ) - 1;
                int             len     = toe - tob + 1;

                StringCopy ( buff, tob, len );

                do cmd.NextToken (); while ( cmd.GetTokenKind () != TCmdLine::Done );
                }


            CartoolDocManager->OpenDoc ( buff, dtOpenOptions );
            } // if Name

        } // for cmd


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // some main window size / origin has been set?
    if ( mainwidth > 0 || mainheight > 0 || maintop > 0 || mainleft > 0 ) {
                                        // some dimension is missing?
        if ( mainwidth  <= 0 )  mainwidth   = CartoolMainWindow->Attr.W;
        if ( mainheight <= 0 )  mainheight  = CartoolMainWindow->Attr.H;
                                        // some origin is missing?
        if ( maintop    <= 0 )  maintop     = CartoolMainWindow->Attr.Y;
        if ( mainleft   <= 0 )  mainleft    = CartoolMainWindow->Attr.X;

                                        // remember current window state
        bool    wasmin  = IsWindowMinimized ( CartoolMainWindow );
        bool    wasmax  = IsWindowMaximized ( CartoolMainWindow );

                                        // this applies to the normal window state
        WindowRestore   ();

        WindowSetPosition ( mainleft, maintop, mainwidth, mainheight );

                                        // restore window state
        if      ( wasmin )  WindowMinimize ();
        else if ( wasmax )  WindowMaximize ();
        }
    } // regular parsing

}


//----------------------------------------------------------------------------
void    TCartoolApp::SetMainTitle ( const char* title, const char* path )
{
if ( ! IsMainThread () || IsNotInteractive () )
    return;


StringCopy          ( Title, title );


const char*         tof             = ToFileName ( path );

if ( StringIsNotEmpty ( tof ) )
    StringAppend    ( Title, " ", tof );// last part of path (filename or directory)

                                        // also updates Title member from TWindow, which is reallocated at each call!
//CartoolMainWindow->SetCaption ( Title ); 
                                        // this one doesn't update Title member, but simply does the job of showing the caption
CartoolMainWindow->SetWindowText ( Title ); 
}

                                        // kind if first call, composing the Title
void    TCartoolApp::SetMainTitle ( const char* prestring, const char* path, TSuperGauge& gauge )
{
if ( ! IsMainThread () || IsNotInteractive () )
    return;


StringCopy          ( TitlePrefix, prestring, 255 );


const char*         tof             = ToFileName ( path );

if ( StringIsNotEmpty ( tof ) )
    StringAppend    ( TitlePrefix, " ", tof );// last part of path (filename or directory)


SetMainTitle    ( gauge );
}

                                        // refreshing a title previously set with a gauge current state
void    TCartoolApp::SetMainTitle ( TSuperGauge& gauge )
{
if ( ! IsMainThread () || IsNotInteractive () )
    return;


StringCopy          ( Title, TitlePrefix );


if ( ! gauge.IsAlive () )

    ;

else if ( gauge.IsDone () )

//  StringAppend    ( Title,    " - Done" );
    StringPrepend   ( Title,    "Done - " );    // showing progress first, so to be seen even with a minimized title

else {
    char                buffgauge    [ 32 ];
    char                buffval      [ 32 ];

//  StringAppend    ( Title,    " - ",
//                              IntegerToString ( buffval, gauge.GetValue () ), 
//                              gauge.IsStylePercentage () ? "%" : "" );

                                        // showing progress first, so to be seen even with a minimized title
    StringCopy      ( buffgauge, IntegerToString ( buffval, gauge.GetValue () ), gauge.IsStylePercentage () ? "%" : "", " - " );
    StringPrepend   ( Title,    buffgauge );
    } // gauge alive && ! Done

                                        // !SetCaption has no actual limit, but will reallocate its local string on each call!
StringClip      ( Title, 255 );

SetMainTitle    ( Title );
}


void    TCartoolApp::ResetMainTitle  ()                  
{
if ( ! IsMainThread () || IsNotInteractive () )
    return;

//CartoolMainWindow->SetCaption ( DefaultTitle );

CartoolMainWindow->SetWindowText ( DefaultTitle ); 
}


//----------------------------------------------------------------------------
                                        // Register application info.
void    TCartoolApp::RegisterInfo ()
{
#if !defined (_DEBUG)

uint32              type            = REG_SZ;
long                sizelong        = MaxPathShort;
uint32              sizeuint        = MaxPathShort;
char                buffnp[ MaxPathShort ];
char                buffwp[ MaxPathShort ];
char                buff  [ MaxPathShort ];
char                b1    [ 2 * MaxPathShort ];
char                b2    [ 2 * MaxPathShort ];
//char              b3[512];
//bool              touchedreg  = false;
bool                opennotcartool;
bool                writeext;


ClearString ( buffnp );
ClearString ( buffwp );
ClearString ( buff   );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                       // find wordpad location
QueryDefValue   ( TRegKey::GetClassesRoot (), "Wordpad.Document.1\\shell\\open\\command", buffwp );

                                        // find notepad location
QueryValue  ( TRegKey::GetLocalMachine (), "SOFTWARE\\Microsoft\\Windows NT", "CurrentVersion", "SystemRoot", buffnp );

if ( StringIsNotEmpty ( buffnp ) )
    StringAppend ( buffnp, "\\notepad.exe %1" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Force showing extensions

                                        // for current user, it's here:
TPreferences        regexplorer ( TRegKey::GetCurrentUser (), "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer" );
UINT32              value;

value       = 0;
regexplorer.SetPreference ( "Advanced", "HideFileExt", value );

//regexplorer.GetPreference ( "Advanced", "HideFileExt", value );
//DBGV ( value, "CurrentUser - Explorer default hide extension" );


                                        // for local machine, it's here:
//TPreferences        regexplorer ( TRegKey::GetLocalMachine (), "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\\Folder" );
//DWORD               value;
//regexplorer.GetPreference ( "HideFileExt", "DefaultValue", value );
//DBGV ( value, "LocalMachine - Explorer default hide extension" );
//char                buff[ RegistryMaxDataLength ];
//regexplorer.GetPreference ( "HideFileExt", "RegPath", buff );
//DBGM ( buff, "LocalMachine - Explorer RegPath" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // register application
QueryDefValue   ( TRegKey::GetClassesRoot (), "Cartool.Application\\DefaultIcon", buff );

//touchedreg  |= StringIsEmpty ( buff );    // key already exist?

if ( ! SetDefValue ( TRegKey::GetClassesRoot (), "Cartool.Application\\DefaultIcon", ApplicationFullPath ) ) {
                                        // it seems we do not have the proper rights, just get out now
    ShowMessage ( "Can not register Cartool!" NewLine NewLine "Try again by running Cartool with Admin rights.", "Registering Cartool", ShowMessageWarning );

    return;
    } 


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // register all data files types
for ( int i=0; i < NumExtensionRegistrations; i++ ) {

                                        // Extension
    StringCopy  ( b1, ".", ExtReg[ i ].Extension );         // ".xyz"
    StringCopy  ( b2,      ExtReg[ i ].Extension, "file" ); // "xyzfile"

    QueryDefValue   ( TRegKey::GetClassesRoot (), b1, buff );


//  touchedreg  |= StringIsEmpty ( buff );  // key already exist?

    writeext    = StringIsEmpty ( buff ) || ( ExtReg[ i ].OpeningType & OpenForceRegistry );

    if ( writeext )
        SetDefValue ( TRegKey::GetClassesRoot (), b1, b2 ); 


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Text + main key
    QueryDefValue   ( TRegKey::GetClassesRoot (), b2, buff );

//  touchedreg  |= StringIsEmpty ( buff );  // key already exist?

    writeext    = StringIsEmpty ( buff ) || ( ExtReg[ i ].OpeningType & OpenForceRegistry );

    if ( writeext )
        SetDefValue ( TRegKey::GetClassesRoot (), b2, ExtReg[ i ].DisplayText ); 


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Test if this extension belongs to another program
    StringCopy  ( b2, ExtReg[ i ].Extension, "file\\Shell\\Open\\command" );

    QueryDefValue   ( TRegKey::GetClassesRoot (), b2, buff );

    opennotcartool  = StringIsNotEmpty ( buff ) && ! StringContains ( (const char *) buff, (const char *) "Cartool" );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Icon
    StringCopy  ( b1, (char *) ApplicationFullPath, ",", IntegerToString ( ExtReg[ i ].IconIndex () ) );
    StringCopy  ( b2, ExtReg[ i ].Extension, "file\\DefaultIcon" );

//    DBGM2 ( b1, b2, "icon" );

    if ( writeext )
        SetDefValue ( TRegKey::GetClassesRoot (), b2, b1 ); 


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Open with Cartool
    if ( ExtReg[ i ].OpeningType & OpenCartool ) {           // open cartool

        StringCopy  ( b1, (char *) ApplicationFullPath, " \"%1\"" );

        if ( opennotcartool )
            StringCopy  ( b2, ExtReg[ i ].Extension, "file\\Shell\\Cartool\\command" );
        else
            StringCopy  ( b2, ExtReg[ i ].Extension, "file\\Shell\\Open\\command" );

        SetDefValue ( TRegKey::GetClassesRoot (), b2, b1 ); 
        }

                                        // Open with Notepad
    if ( ExtReg[ i ].OpeningType & OpenNotePad && StringIsNotEmpty ( buffnp ) ) {

        if ( ExtReg[ i ].OpeningType & OpenCartool )
            StringCopy  ( b2, ExtReg[ i ].Extension, "file\\Shell\\Notepad\\command" );
        else
            StringCopy  ( b2, ExtReg[ i ].Extension, "file\\Shell\\Open\\command" );

        SetDefValue ( TRegKey::GetClassesRoot (), b2, buffnp ); 
        }

                                        // Open with Wordpad
    if ( ExtReg[ i ].OpeningType & OpenWordPad && StringIsNotEmpty ( buffwp ) ) {

        if ( ExtReg[ i ].OpeningType & ( OpenCartool | OpenNotePad ) )
            StringCopy  ( b2, ExtReg[ i ].Extension, "file\\Shell\\Wordpad\\command" );
        else
            StringCopy  ( b2, ExtReg[ i ].Extension, "file\\Shell\\Open\\command" );

        SetDefValue ( TRegKey::GetClassesRoot (), b2, buffwp ); 
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // allow File->new .lm
StringCopy  ( b1, ".", FILEEXT_LM, "\\", FILEEXT_LM, "file\\ShellNew" );
ClearString ( b2 );

SetValue    ( TRegKey::GetClassesRoot (), b1, 0, "NullFile", b2 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*
if ( touchedreg ) {
    if ( GetAnswerFromUser ( "Windows has been updated and should be restarted.\n\nDo you want to restart now?", CartoolMainWindow->Title ) )
        if ( ExitWindowsEx ( EWX_REBOOT | EWX_FORCE, 0 ) )
            if ( CanClose() )
                EndModal ( 0 );         // not very useful, but in any case
    }
*/

#endif
}


//----------------------------------------------------------------------------
                                        // Unregister application info.
void    TCartoolApp::UnRegisterInfo ()
{
#if !defined (_DEBUG)

                                        // recursively destroy all these keys
if ( ! NukeKey ( TRegKey::GetClassesRoot (), "Cartool.Application" ) ) {
                                        // it seems we do not have the proper rights, just get out now
    ShowMessage ( "Can not un-register Cartool!" NewLine NewLine "Try again by running Cartool with Admin rights.", "Un-registering Cartool", ShowMessageWarning );

    return;
    }


//uint32            type;
//uint32            size;
char                b1[ MaxPathShort ];
//char              b2[ MaxPathShort ];
//char              buninst[ MaxPathShort ]  = "";


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // look for uninstall
StringCopy ( b2, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\uninstall\\", ProdName, ProdVersion );
QueryValue  ( TRegKey::GetLocalMachine (), b2, "UninstallString2", b1 );

if ( *b1 ) {                            // second string exist -> perform the 2 uninstall
    StringCopy ( buninst, b1 );
    }
*/
                                        // remove from the uninstall
StringCopy ( b1, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\uninstall\\", ProdName, ProdVersion );

NukeKey ( TRegKey::GetLocalMachine (), b1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // remove all file extensions
for ( int i=0; i < NumExtensionRegistrations; i++ ) {
                                        // extension
    StringCopy ( b1, ".", ExtReg[ i ].Extension );
    NukeKey ( TRegKey::GetClassesRoot (), b1 );

    StringCopy ( b1, ExtReg[ i ].Extension, "file" );
    NukeKey ( TRegKey::GetClassesRoot (), b1 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
if ( *buninst )                         // only now, run the uninstallhield
    system ( buninst );
*/

#endif
}


//----------------------------------------------------------------------------
                                        // Reset registry
void    TCartoolApp::ResetRegisterInfo ()
{
#if !defined (_DEBUG)

UnRegisterInfo ();
RegisterInfo   ();

#endif
}


void    TCartoolApp::CmPrefsRegistry ( owlwparam w )
{
#if !defined (_DEBUG)

if      ( w == CM_RESETREGISTRY ) {

    if ( ! GetAnswerFromUser (  "Do you want to associate all known files (.xyz .hdr etc...) with Cartool?"   NewLine
                                NewLine
                                "This will re-register Cartool, so you need the program to run with administrator rights...",
                                "Files Preferences" ) )
        return;

    ResetRegisterInfo ();
    }

else if ( w == CM_CLEARREGISTRY ) {

    if ( ! GetAnswerFromUser (  "Do you want to remove all known files associations of Cartool?"   NewLine
                                NewLine
                                "This will un-register Cartool, so you need the program to run with administrator rights...",
                                "Files Preferences" ) )

    UnRegisterInfo ();
    }

ShowMessage ( "Please restart your machine to apply these changes!", "Files Preferences", ShowMessageWarning );

#endif
}


//----------------------------------------------------------------------------
void    TCartoolApp::CmPrefsGraphic ( owlwparam w )
{
char                option[ RegistryMaxVarLength  ];
char                value [ RegistryMaxDataLength ];


if      ( w == CM_GRAPHICACCELAUTO ) {

    if ( ! GetAnswerFromUser ( "Detect and use automatically the hardware acceleration?", "Graphic Preferences" ) )
        return;

    StringCopy ( option, PrefAcceleration );
    StringCopy ( value,  PrefAuto );
    }
else if ( w == CM_GRAPHICACCELON ) {

    if ( ! GetAnswerFromUser ( "Force the use of the hardware acceleration?", "Graphic Preferences" ) )
        return;

    StringCopy ( option, PrefAcceleration );
    StringCopy ( value,  PrefOn );
    }
else if ( w == CM_GRAPHICACCELOFF ) {

    if ( ! GetAnswerFromUser ( "Force to never use the hardware acceleration?", "Graphic Preferences" ) )
        return;

    StringCopy ( option, PrefAcceleration );
    StringCopy ( value,  PrefOff );
    }

else if ( w == CM_GRAPHIC3DTEXTURESAUTO ) {

    if ( ! GetAnswerFromUser ( "Detect and use automatically the 3D textures (MRI slices)?", "Graphic Preferences" ) )
        return;

    StringCopy ( option, Pref3DTextures );
    StringCopy ( value,  PrefAuto );
    }
else if ( w == CM_GRAPHIC3DTEXTURESON ) {

    if ( ! GetAnswerFromUser ( "Force the use of the 3D textures (MRI slices)?", "Graphic Preferences" ) )
        return;

    StringCopy ( option, Pref3DTextures );
    StringCopy ( value,  PrefOn );
    }
else if ( w == CM_GRAPHIC3DTEXTURESOFF ) {

    if ( ! GetAnswerFromUser ( "Force to never use the 3D textures (MRI slices)?", "Graphic Preferences" ) )
        return;

    StringCopy ( option, Pref3DTextures );
    StringCopy ( value,  PrefOff );
    }


SetPreference ( PrefGraphic, option, value );

                                        // checking!
//ClearString ( value );
//if ( GetPreference ( PrefGraphic, option, value ) )
//    DBGM ( value, "retrieved" );

                                        // update application
RetrievePreferences ();


ShowMessage ( "You may have to restart Cartool to apply these changes!\n(or maybe not if you are lucky)", "Graphic Preferences", ShowMessageWarning );
}


void    TCartoolApp::RetrievePreferences ()
{
                                        // retrieve and store options
char                value[ RegistryMaxDataLength ];


if ( GetPreference ( PrefGraphic, PrefAcceleration, value ) )

    PrefGraphicAccel        = StringIs ( value, PrefOn   ) ? FormatUse
                            : StringIs ( value, PrefOff  ) ? FormatDontUse
                            : StringIs ( value, PrefAuto ) ? FormatUseBest
                                                           : FormatUseBest;
else
    PrefGraphicAccel        = FormatUseBest;

//DBGV ( PrefGraphicAccel, value );


if ( GetPreference ( PrefGraphic, Pref3DTextures, value ) )

    PrefGraphic3DTextures   = StringIs ( value, PrefOn   ) ? FormatUse
                            : StringIs ( value, PrefOff  ) ? FormatDontUse
                            : StringIs ( value, PrefAuto ) ? FormatUseBest
                                                           : FormatUseBest;
else
    PrefGraphic3DTextures   = FormatUseBest;

//DBGV ( PrefGraphic3DTextures, value );
}


void    TCartoolApp::CmPrefsGraphicAccelAutoEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( PrefGraphicAccel == FormatUseBest );
}


void    TCartoolApp::CmPrefsGraphicAccelOnEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( PrefGraphicAccel == FormatUse );
}


void    TCartoolApp::CmPrefsGraphicAccelOffEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( PrefGraphicAccel == FormatDontUse );
}


void    TCartoolApp::CmPrefsGraphic3DTexturesAutoEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( PrefGraphic3DTextures == FormatUseBest );
}


void    TCartoolApp::CmPrefsGraphic3DTexturesOnEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( PrefGraphic3DTextures == FormatUse );
}


void    TCartoolApp::CmPrefsGraphic3DTexturesOffEnable ( TCommandEnabler &tce )
{
tce.SetCheck ( PrefGraphic3DTextures == FormatDontUse );
}


//----------------------------------------------------------------------------
                                        // Response Table handlers:
void    TCartoolApp::EvNewView ( TView& view )
{
if ( CartoolMdiClient ) {

    TCartoolMdiChild*   child           = new TCartoolMdiChild ( *CartoolMdiClient, 0, view.GetWindow (), true );

    if ( child == 0 )   return;

    child->SetIcon   ( this, IDI_DOC );
    child->SetIconSm ( this, IDI_DOC );

    if ( view.GetViewMenu () )
        child->SetMenuDescr ( *view.GetViewMenu () );

    child->Create ();
                                        // !forcing focus, useful when directly calling dnCreate!
    child->GetClientWindow ()->SetFocus ();

//  CartoolMdiClient->AdjustScroller ();
    }

//UpdateApplication;
}


void    TCartoolApp::EvCloseView ( TView& view )
{
if ( LastActiveBaseView )

    if ( LastActiveBaseView->GetViewId () == view.GetViewId () )

        LastActiveBaseView  = 0;


//if ( CartoolMdiClient )
//  CartoolMdiClient->AdjustScroller();

//UpdateApplication;
}

                                        // Saving to the list of recent files
void    TCartoolApp::EvOwlDocument ( TDocument& doc )
{
if ( doc.GetDocPath () )
    SaveMenuChoice ( TFileName ( doc.GetDocPath (), TFilenameExtendedPath ) );

//UpdateApplication;
}


//----------------------------------------------------------------------------
                                        // Menu Help Contents command
void    TCartoolApp::CmHelpContents ( owlwparam w )
{
                                        // Show the help table of contents.
//HelpState = CartoolMainWindow->WinHelp ( HelpFullPath, HELP_CONTENTS, 0 ); // old help system


if      ( w == CM_HELPCONTENTS          )   HtmlHelp        ( CartoolMainWindow->GetHandle (), HelpFullPath, HH_DISPLAY_TOPIC, 0 );
//else if ( w == CM_HELPWEBUSERSGUIDE   )   ShellExecute    ( NULL, "open", "https://cartoolcommunity.unige.ch/user-s-guide",                               NULL, NULL, SW_SHOWNORMAL); // does not exist anymore, and not yet replaced either...
else if ( w == CM_HELPCARTOOLGITHUB     )   ShellExecute    ( NULL, "open", "https://github.com/DenisBrunet/Cartool",                                       NULL, NULL, SW_SHOWNORMAL);
else if ( w == CM_HELPCARTOOLCOMMUNITY  )   ShellExecute    ( NULL, "open", "https://cartoolcommunity.unige.ch",                                            NULL, NULL, SW_SHOWNORMAL);
else if ( w == CM_HELPAUTOUPDATE        )   ShellExecute    ( NULL, "open", "https://sites.google.com/site/cartoolcommunity/downloads",                     NULL, NULL, SW_SHOWNORMAL);
else if ( w == CM_HELPWEBRELEASES       )   ShellExecute    ( NULL, "open", "https://sites.google.com/site/cartoolcommunity/downloads/cartool-releases",    NULL, NULL, SW_SHOWNORMAL);
else if ( w == CM_HELPWEBCIBM           )   ShellExecute    ( NULL, "open", "https://cibm.ch/",                                                             NULL, NULL, SW_SHOWNORMAL);
else if ( w == CM_HELPWEBFBMLAB         )   ShellExecute    ( NULL, "open", "https://www.unige.ch/medecine/neuf/en/research/grecherche/christoph-michel/",  NULL, NULL, SW_SHOWNORMAL);
}

/*                                        // Menu Help Using Help command
void    TCartoolApp::CmHelpUsing ()
{
                                        // Display the contents of the Windows help file.
HelpState = CartoolMainWindow->WinHelp(HelpFullPath, HELP_HELPONHELP, 0);
}
*/
                                        // Menu Help About Cartool command
void    TCartoolApp::CmHelpAbout ()
{
                                        // Show the modal dialog.
TCartoolAboutDialog ( CartoolMainWindow ).Execute ();
}


//----------------------------------------------------------------------------
                                        // does all the job to extract the potential base view dropping area
TBaseView*  TCartoolApp::GetViewFromDrop ( TDropInfo &drop )
{
                                        // see if it is dropped into a lm window
TPoint              where;
drop.DragQueryPoint ( where );

                                        // where -> window
HWND                child           = ChildWindowFromPoint ( CartoolMdiClient->GetHandle (), where );
TWindow            *towin           = child ? GetWindowPtr ( child ) : CartoolMdiClient;

                                        // edge (MDI child) window?
if ( typeid ( *towin ) == typeid ( TCartoolMdiChild ) )
    towin           = towin->GetFirstChild ();

                                        // try converting into a TBaseView
return  dynamic_cast<TBaseView *> ( towin );
}


//----------------------------------------------------------------------------
void    TCartoolApp::EvDropFiles ( TDropInfo drop )
{
                                        // convert to a sorted list of file names, also expanding into sibling files if needed
                                        // list is also lexico-numerically sorted (epochs and stuff)
TGoF                files ( drop );
                                        // process files
OpenDroppedFiles ( files, GetViewFromDrop ( drop ) );

drop.DragFinish ();
}

                                        // optionally supplied with a group doc
void    TCartoolApp::OpenDroppedFiles ( TGoF &files, TBaseView *view )
{
                                        // dropped into a LM doc?
                                        // !This should actually be in TLinkManyView::EvDropFiles, like for dropping in Tracks Display!
TLinkManyDoc*       intolmdoc       = view ? dynamic_cast<TLinkManyDoc*> ( view->BaseDoc ) : 0;

                                        // disabling animations?
bool                oldav           = AnimateViews;
AnimateViews        =    AnimateViews                                               // might have been reset elsewhere
                      && IsInteractive ()                                           // must be interactive
                      && (int) files                      <= AnimationMaxDocDropped // only few files dropped
                      && CartoolDocManager->NumDocOpen () <  AnimationMaxDocOpen    // and not already crowded
                      && ! intolmdoc                                                    // and not dropping into lm view
                      && ! files.SomeExtensionsAre ( AllLmFilesExt );               // and not dropping a lm file either


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

std::list<TBaseDoc*>    docstosync;


for ( int fi = 0; fi < (int) files; fi++ ) {

    TDocTemplate*   tpl     = CartoolDocManager->MatchTemplate ( files[ fi ] );

    if ( tpl ) {

        TDocument*      doc         = CartoolDocManager->IsOpen ( files[ fi ] );

        if ( doc == 0 || doc->GetViewList () == 0 )

            doc     = CartoolDocManager->CreateDoc ( tpl, files[ fi ] );


        TBaseDoc*       basedoc     = dynamic_cast<TBaseDoc*> ( doc );

        if ( intolmdoc && basedoc ) {
                                        // don't update the added windows intermediate steps
            intolmdoc->AddToGroup ( basedoc, false );
                                        // instead, update the tiny .lm window
            intolmdoc->GetViewList()->Invalidate ( false );


            if ( IsExtensionAmong ( basedoc->GetDocPath (),     AllCoordinatesFilesExt 
                                                            " " AllSolPointsFilesExt 
                                                            " " AllMriFilesExt 
                                                            " " AllInverseFilesExt 
                                                            " " AllRoisFilesExt         ) )
                                        // we can hide these guys
                basedoc->MinimizeViews ();


            if ( IsExtensionAmong ( basedoc->GetDocPath (),     AllTracksFilesExt ) )
                                        // for later use
                docstosync.push_back ( basedoc );

                                        // dropping a lm into a lm -> close the dropped one
            if ( IsExtensionAmong ( basedoc->GetDocPath (), AllLmFilesExt ) )

                CartoolDocManager->CloseDoc ( basedoc );
            }
        } // doc template exists
    else                                // unknown file type?

        CartoolDocManager->OpenUnknownFile ( files[ fi ] );

    } // for file


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Dropped files into a .lm file
if ( intolmdoc ) {
                                        // force sync everything - a bit indiscriminate
//  intolmdoc->SyncUtility ( CM_SYNCALL /*CM_SYNCBETWEENEEG*/ );
// 
                                        // Only sync the dropped EEG / RIS together - but NOT with other existing ones
    TBaseView*      firstview   = 0;

    for ( auto doctosync = docstosync.cbegin (); doctosync != docstosync.cend (); doctosync++ ) {
                                        // loop through all views
        for ( TBaseView* view = (*doctosync)->GetViewList ( intolmdoc ); view != 0; view = (*doctosync)->NextView (view, intolmdoc) ) {

            if ( firstview == 0 ) {
                firstview = view;
                continue;
                }

            if ( view->GODoc == firstview->GODoc ) {
                view->SetFriendView ( firstview );
                view->GetParentO ()->SetFocus ();
                }
            }
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( intolmdoc )                            // force retile any .lm windows
    intolmdoc->RefreshWindows ( true );


AnimateViews        = oldav;
}


//----------------------------------------------------------------------------
LRESULT TCartoolApp::CmFileSelected ( WPARAM wp, LPARAM )
{
//TAPointer<char>     text            = new char [ MaxPathShort ];
char                text[ MaxPathShort ];

GetMenuText ( wp, text, MaxPathShort );

TDocTemplate*       tpl             = CartoolDocManager->MatchTemplate ( text );

if ( tpl )
    CartoolDocManager->CreateDoc ( tpl, text );

return 0;
}


//----------------------------------------------------------------------------
                                        // Process application messages to provide context sensitive help
bool    TCartoolApp::ProcessAppMsg ( MSG& msg )
{
/*
if ( msg.message == WM_COMMAND ) {
    if (ContextHelp || ::GetKeyState(VK_F1) < 0) {
        ContextHelp = false;
        CartoolMainWindow->WinHelp(HelpFullPath, HELP_CONTEXT, msg.wParam);
        return true;
        }
    }
else */
    switch ( msg.message ) {

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case WM_KEYDOWN:

            if ( msg.wParam == VK_F1 ) {
                                        // If the Shift/F1 then set the help cursor and turn on the modal help state.
                if ( ::GetKeyState ( VK_SHIFT ) < 0 ) {

                    ContextHelp = true;
                    HelpCursor  = new TCursor ( CartoolMainWindow->GetModule()->GetHandle(), TResId ( IDC_HELPCURSOR ) );
                    ::SetCursor ( *HelpCursor );

                    return true;        // Gobble up the message.
                    }
                else {
                                        // If F1 w/o the Shift key then bring up help's main index.
//                  CartoolMainWindow->WinHelp(HelpFullPath, HELP_INDEX, 0); // old help system
                    HtmlHelp ( CartoolMainWindow->GetHandle (), HelpFullPath, HH_DISPLAY_TOPIC, 0 );

                    return true;        // Gobble up the message.
                    }
                }
            else {

                if ( ContextHelp && msg.wParam == VK_ESCAPE ) {

                    if ( HelpCursor )
                        delete HelpCursor;
                    HelpCursor  = 0;

                    ContextHelp = false;
                    CartoolMainWindow->SetCursor ( 0, IDC_ARROW );

                    return true;    // Gobble up the message.
                    }
                }
            break;

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case WM_MOUSEMOVE:
        case WM_NCMOUSEMOVE:

            if ( ContextHelp ) {

                ::SetCursor ( *HelpCursor );
                return true;        // Gobble up the message.
                }
            break;

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case WM_INITMENU:

            if ( ContextHelp ) {
                ::SetCursor ( *HelpCursor );

                return true;    // Gobble up the message.
                }
            break;

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        case WM_ENTERIDLE:

            if ( msg.wParam == MSGF_MENU )
                if ( ::GetKeyState ( VK_F1 ) < 0 ) {

                    ContextHelp = true;
                    CartoolMainWindow->PostMessage ( WM_KEYDOWN, VK_RETURN, 0 );

                    return true;     // Gobble up the message.
                    }
            break;

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        default:

            return  TApplication::ProcessAppMsg ( msg );

    } // End of message switch

                                        // Continue normal processing.
return  TApplication::ProcessAppMsg ( msg );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

} // crtl namespace

//----------------------------------------------------------------------------
                                        // This one has to be in the GLOBAL namespace
int     OwlMain ( int /*argc*/, char** /*argv*/ )
{
crtl::TCartoolApp   app ( crtl::CartoolTitle );

return  app.Run ();
}


//----------------------------------------------------------------------------
