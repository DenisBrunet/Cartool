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

#include    <owl/pch.h>

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "TCartoolMdiClient.h"
#include    "TCartoolMdiChild.h"
#include    "TBaseView.h"

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1 (TCartoolMdiChild, TMDIChild)
    EV_WM_SYSCOMMAND,
END_RESPONSE_TABLE;


        TCartoolMdiChild::TCartoolMdiChild ( TMDIClient& parent, const char* title, TWindow* clientWnd, bool shrinkToClient, TModule* module )
      : TMDIChild ( parent, title, clientWnd, shrinkToClient, module )
{
Attr.W  = crtl::RescaleSizeDpi ( this, DefaultWindowSize );
Attr.H  = crtl::RescaleSizeDpi ( this, DefaultWindowSize );
}


//----------------------------------------------------------------------------
void    TCartoolMdiChild::EvSysCommand ( uint cmdType, const TPoint& )
{
TDocument*          doc             = CartoolDocManager->GetCurrentDoc();
TBaseView*          view            = CartoolDocManager->GetCurrentView ();
TLinkManyDoc*       mygod           = view ? view->GODoc : 0;


switch ( cmdType & 0xFFF0 ) {
                                        // allow a special behavior for a group window
                                        // -> spread command to all windows of the group
    case SC_MINIMIZE:

        if ( mygod != 0 && (const void*) doc == (const void*) mygod )
            CartoolMdiClient->CmGroupWinAction ( CM_GTV_MIN );
        else
            DefaultProcessing();
        break;

    case SC_MAXIMIZE:

        if ( mygod != 0 && (const void*) doc == (const void*) mygod )
            CartoolMdiClient->CmGroupWinAction ( IDB_GTVFIT );
        else
            DefaultProcessing();
        break;

    case SC_RESTORE:

        if ( mygod != 0 && (const void*) doc == (const void*) mygod )
            CartoolMdiClient->CmGroupWinAction ( CM_GTV_RESTORE );
        else
            DefaultProcessing();
        break;

    default:
        DefaultProcessing();
    }

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}







