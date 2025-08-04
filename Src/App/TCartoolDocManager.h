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

#pragma once

#include    <owl/decmdifr.h>
#include    <owl/docmanag.h>

#include    "Files.Utils.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Forward declarations
class   TCartoolApp;
class   TCartoolDocManager;
class   TCartoolMdiClient;
class   TLinkManyDoc;
class   TBaseDoc;
class   TBaseView;
class   TSuperGauge;


//----------------------------------------------------------------------------
                                        // Handy class which gives access to application, doc manager, MDI client etc.. to any classe that will need them, even if not part of OwlNext framework
class   TCartoolObjects
{
public:
                                        // Direct pointers that will be set at creation time
    static  TCartoolApp*                CartoolApplication;
    static  TCartoolDocManager*         CartoolDocManager;
    static  owl::TDecoratedMDIFrame*    CartoolMainWindow;
    static  TCartoolMdiClient*          CartoolMdiClient;

                                        // Called so often
    bool                                IsInteractive    () const;
    bool                                IsNotInteractive () const;
};

                                        // Creating a global object for those functions with a lack of class
extern  TCartoolObjects     CartoolObjects;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // default opening flags
constexpr UINT  dtOpenOptions           = owl::dtAutoDelete | owl::dtUpdateDir | owl::dtOverwritePrompt;
                                        // hidden from the templates, as to avoid confusing duplicates when creating new views
constexpr UINT  dtOpenOptionsHidden     = dtOpenOptions | owl::dtHidden;
                                        // when views are not needed, this is faster
constexpr UINT  dtOpenOptionsNoView     = dtOpenOptions | owl::dtNoAutoView;

constexpr int   AnimationMaxDocOpen     = 50;
constexpr int   AnimationMaxDocDropped  = 5;


//----------------------------------------------------------------------------

class       TCartoolObjects;
class       TGoF;


class   TCartoolDocManager :    public  owl::TDocManager,
                                public  TCartoolObjects
{
public:
                    TCartoolDocManager  ( int mode, owl::TApplication* app, owl::TDocTemplate*& templateHead = ::DocTemplateStaticHead );


    void            CmFileOpen          ();
    void            CmViewCreate        ();
    void            CmFileRevert        ();
    void            CmFileSave          ();
    void            CmFileSaveAs        ();


    int             NumDocOpen          ();
    owl::TDocument* CreateAnyDoc        ( const char *path, long flags=0 );
    TBaseDoc*       CreateDoc           ( owl::TDocTemplate* tpl, /*const*/ char *path, owl::TDocument *parent=0, long flags=0 );    // !OwlNext complaining!
    int             GetNewTemplates     ( owl::TDocTemplate** tplList, int size, bool newDoc );

    TBaseDoc*       IsOpen              ( const char *filename );
    void            GetDocs             ( TGoF& gof, const char* extensions = 0 );
    TBaseDoc*       OpenDoc             ( const char* path, long flags );
    void            CloseDoc            ( TBaseDoc *doc, bool refresh = true );
    void            CloseDoc            ( const char *filename, bool refresh = true );
    void            CloseView           ( owl::TView* view );
    void            CloseViews          ( owl::TDocument* doc, owl::TView* notthisview = 0, TLinkManyDoc* god = 0 );

    owl::TDocTemplate*  MatchTemplate   ( const char *path );     // virtual, although the Help says it is not
    TBaseDoc*       DocListNext         ( owl::TDocument *doc );
//  char*           GetOpenCommand      ( char *extension, char *command );
    void            OpenUnknownFile     ( const char *path );

    TBaseView*      GetView             ( UINT viewid );
    TBaseDoc*       GetCurrentBaseDoc   ();
    TBaseView*      GetCurrentView      ();


protected:
    int             SelectViewType ( owl::TDocTemplate** tpllist, int tplcount );


    DECLARE_RESPONSE_TABLE(TCartoolDocManager);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
