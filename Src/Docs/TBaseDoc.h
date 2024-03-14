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

#include    <owl/filedoc.h>             // TFileDocument

#include    "CartoolTypes.h"
#include    "TList.h"
#include    "Geometry.TOrientation.h"
#include    "Geometry.TGeometryTransform.h"
#include    "Geometry.TDisplaySpaces.h"

#include    "TCartoolApp.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // ALL Cartool documents will derive from TBaseDoc, which itself derives from TFileDocument.
                                        // (this basically implies that all Cartool documents come from files)
                                        // Class upgrades are the following:
                                        //  - defining data type, 3D orientation (if relevant), different display space (2D and 3D f.ex.)
                                        //  - providing easy access to application, doc manager, MDI window etc..
                                        //  - referencing / linking / syncing between documents
                                        //  - passing message to (docs + views), or friend views, instead of only views from a given doc
                                        //  - preventing closing document when doing critical user interactions
class       TLinkManyDoc;
class       TTracksView;
class       TRisDoc;
class       TRois;
class       GeometryTransform;


class	TBaseDoc :      public          owl::TFileDocument, // all documents come from some files
                        public          TCartoolObjects,    // centralized access to application, doc manager, main window etc...
                        public          TProductInfo,       // company name and stuff
                        public virtual  TDataFormat,        // centralized the atomic data type handling - !could be inherited by different sub-children, hence virtual!
                        public          TOrientation        // many data need some sort of orientation
{
public:
                        TBaseDoc ( owl::TDocument* parent = 0 );
    virtual            ~TBaseDoc ();        // for proper destructor calls, when using TBaseDoc*


    TLinkManyDoc*       LastGroupDoc;       // Group of the last active window of this doc


    bool                CanClose            ();                 // TDocument virtual method
    bool                CanClose            ( bool silent );    // Cartool CanClose
    bool                IsDirty             ()                              { return TFileDocument::IsDirty (); }
    bool                IsUsedBy            ()  const                       { return (bool) UsedBy;             }
    void                AddLink             ( const owl::TDocument* doc )   { UsedBy.Append ( doc );            }
    void                RemoveLink          ( const owl::TDocument* doc )   { UsedBy.Remove ( doc );            }
    void                PreventClosing      ()                              { DoNotClose    = true;             }
    void                AllowClosing        ()                              { DoNotClose    = false;            }

    virtual bool        CanSync             ( owl::TWindow* towin ) const;

    virtual const char* GetDefaultExtension ()                              { return GetTemplate ()->GetDefaultExt (); }
    virtual bool        ExtensionIs         ( const char* ext )     const   { return IsExtension ( GetDocPath (), ext ); /*StringIs ( GetDefaultExtension (), ext );*/  }

    virtual char*       GetBaseFileName     ( char* basefilename )  const;  // the base name to be used for any processing ouput (removing extension, caring for mff directory...)

                                            // Extending NotifyViews functionalities:
    virtual bool        NotifyDocViews      ( int event, owl::TParam2 item = 0, owl::TView* viewexclude = 0, owl::TDocument* docexclude = 0 );
    virtual bool        NotifyFriendViews   ( owl::uint friendshipid, int event, owl::TParam2 item = 0, owl::TView* viewexclude = 0);

                                            // Specializing TDocument GetViewList and NextView
    TBaseView*          GetViewList         ()  const;
    TBaseView*          GetViewList         ( TLinkManyDoc* group, bool equal = true );
    TBaseView*          NextView            ( const owl::TView* view );
    TBaseView*          NextView            ( TBaseView* bview, TLinkManyDoc* group, bool equal=true );

    void                SetFocus            ( owl::uint viewid );
    void                MinimizeViews       ();


    const TDisplaySpaces&       GetDisplaySpaces     () const               { return DisplaySpaces;     }
          TDisplaySpaces&       GetDisplaySpaces     ()                     { return DisplaySpaces;     }

    const TGeometryTransform*   GetGeometryTransform () const               { return GeometryTransform; }
          TGeometryTransform*   GetGeometryTransform ()                     { return GeometryTransform; }


protected:

    bool                DoNotClose;         // some processing can put a lock on a given active document to prevent it from closing
    crtl::TList<owl::TDocument> UsedBy;

    TDisplaySpaces      DisplaySpaces;      // 3D data can be seen in different spaces / projection
    TGeometryTransform* GeometryTransform;

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
