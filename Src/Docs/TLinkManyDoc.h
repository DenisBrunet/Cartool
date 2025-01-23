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

#include    "TBaseDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Flags used to resize all views from a given group
enum                GroupTilingViewsFlags
                    {
                    GroupTilingViews_Move           = 0x01,
                    GroupTilingViews_BestFitSize    = 0x02,     // Best fit to current MDI Client size
                    GroupTilingViews_StandSize      = 0x04,     // Set standard sizes
                    GroupTilingViews_SizeMask       = GroupTilingViews_Move | GroupTilingViews_BestFitSize | GroupTilingViews_StandSize,

                    GroupTilingViews_RightSide      = 0x10,
                    GroupTilingViews_Insert         = 0x20,
                    GroupTilingViews_InsertMask     = GroupTilingViews_RightSide | GroupTilingViews_Insert,
                    };

constexpr int       GroupTilingSpaceBetweenGroups   = 20;

constexpr char*     LinkingFilesTitle               = "Linking Files";


//----------------------------------------------------------------------------
                                        // Link-Many (files) document is a class used to group related files together,
                                        // f.ex. Electrodes file + n EEG files
                                        // Linking files unlocks additional functionalities, like displaying potentials
                                        // from previous example, or inverse solution display, or synchronizing views.
class   TTracksDoc;
class   TElectrodesDoc;
class   TSolutionPointsDoc;
class   TInverseMatrixDoc;
class   TRisDoc;
class   TVolumeDoc;
class   TRoisDoc;
class   TGoF;


class	TLinkManyDoc    :  public  TBaseDoc
{
public:
                        TLinkManyDoc ( owl::TDocument *parent = 0 );
                       ~TLinkManyDoc ();
                     

    UINT                LastEegViewId;      // in case of multiple eeg views
    bool                CommandsCloning;

                                            // owl::TDocument
    bool                CanClose        ();
    bool                Close	        ();
    bool                Commit	        ( bool force = false );
    bool                InitDoc         ();
    bool                IsOpen	        ()  const;
    bool                Open 	        ( int mode, const char* path = 0 );
    bool                Revert	        ( bool force = false );

                                            // TBaseDoc
    bool                NotifyDocViews  ( int event, owl::TParam2 item = 0, owl::TView *vexclude = 0, owl::TDocument *dexclude = 0 );


    int                 GetNumEegDoc   ()       const   { return (int) ListEegDoc;  }
    int                 GetNumXyzDoc   ()       const   { return (int) ListXyzDoc;  }
    int                 GetNumSpDoc    ()       const   { return (int) ListSpDoc;   }
    int                 GetNumIsDoc    ()       const   { return (int) ListIsDoc;   }
    int                 GetNumRisDoc   ()       const   { return (int) ListRisDoc;  }
    int                 GetNumMriDoc   ()       const   { return (int) ListMriDoc;  }
    int                 GetNumRoiDoc   ()       const   { return (int) ListRoisDoc; }


    TTracksDoc*         GetEegDoc ( int index )         { return ListEegDoc [ index ]; }
    TElectrodesDoc*     GetXyzDoc ( int index )         { return ListXyzDoc [ index ]; }
    TSolutionPointsDoc* GetSpDoc  ( int index )         { return ListSpDoc  [ index ]; }
    TInverseMatrixDoc*  GetIsDoc  ( int index )         { return ListIsDoc  [ index ]; }
    TRisDoc*            GetRisDoc ( int index )         { return ListRisDoc [ index ]; }
    TVolumeDoc*         GetMriDoc ( int index )         { return ListMriDoc [ index ]; }
    TRoisDoc*           GetRoisDoc( int index )         { return ListRoisDoc[ index ]; }
                                        // guess the head, brain and grey, and always return non-null values (best guess)
    void                GuessHeadBrainGreyMris ( TVolumeDoc*& head, TVolumeDoc*& brain, TVolumeDoc*& grey, int *toheadi = 0, int *tobraini = 0, int *togreyi = 0 )   const;


    void                GroupTileViews  ( GroupTilingViewsFlags flags, UINT viewidabove = 0 );
    bool                AddToGroup      ( TBaseDoc *doc, bool refresh = true );
    void                SyncUtility     ( owlwparam w );
    void                RefreshWindows  ( bool retile = true );

    void                Segment         ();


protected:

    crtl::TList<TTracksDoc>         ListEegDoc;
    crtl::TList<TElectrodesDoc>     ListXyzDoc;
    crtl::TList<TSolutionPointsDoc> ListSpDoc;
    crtl::TList<TInverseMatrixDoc>  ListIsDoc;
    crtl::TList<TRisDoc>            ListRisDoc;
    crtl::TList<TVolumeDoc>         ListMriDoc;
    crtl::TList<TRoisDoc>           ListRoisDoc;

    bool                UnspecifiedDocPath;

    bool                Compatibility   ( const TGoF& leeg, const TGoF& lrois, const TGoF& lxyz, const TGoF& lsp, const TGoF& lis, const TGoF& lris )  const;

};


//----------------------------------------------------------------------------
                                        // Dynamically linking already opened documents
class	TDynamicLinkManyDoc :   public  TLinkManyDoc
{
public:
                    TDynamicLinkManyDoc ( TTracksDoc *doceeg, TElectrodesDoc* docxyz, owl::TDocument *parent = 0 );
                   ~TDynamicLinkManyDoc ();


    bool            Open 	    ( int mode, const char* path = 0 )  { return true; }
    bool            Commit	    ( bool force = false )              { SetDirty ( false ); return true; }
    bool            Revert	    ( bool force = false )              { SetDirty ( false ); return true; }
    bool            Close       ();
//  bool            CanClose    ()                                  { return false; }   // closing only by destroying the object
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
