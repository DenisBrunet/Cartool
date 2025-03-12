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
#include    "TInverseMatrixDoc.h"

#include    "TBaseView.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum    {
        ISGLVIEW_CBG_NUM    = NumBaseViewButtons
        };


class   TInverseMatrixView  :   public  TBaseView
{
public:
                        TInverseMatrixView ( TInverseMatrixDoc &doc, owl::TWindow *parent = 0, TLinkManyDoc *group=0 );


    static const char*  StaticName              ()          { return "Inverse &Matrix"; }
    const char*         GetViewName             ()  final   { return StaticName(); }

    void                CreateGadgets           ()  final;


    void                GLPaint                 ( int how, int renderingmode, TGLClipPlane *otherclipplane )    final;


protected:

    TInverseMatrixDoc*  ISDoc;


    bool                ValidView               ()  final   { return ISDoc->GetNumElectrodes(); }

    void                Paint                   ( owl::TDC& dc, bool erase, owl::TRect& rect )  final;


    void                CmSetAveragingBefore    ( owlwparam w );
    void                CmPrecedenceBeforeEnable( owl::TCommandEnabler& tce );
    void                CmPrecedenceAfterEnable ( owl::TCommandEnabler& tce );

    DECLARE_RESPONSE_TABLE (TInverseMatrixView);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
