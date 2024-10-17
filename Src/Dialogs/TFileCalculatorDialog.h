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

#include    "System.h"
#include    "TBaseDialog.h"
#include    "TCartoolApp.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


class   TFileCalculatorStruct
{
public:
                        TFileCalculatorStruct ();


    TEditData           NumGroups   [ EditSizeValue ];
    TListBoxData        GroupsSummary;

    TComboBoxData       Expression;

    TEditData           BaseDir       [ EditSizeText ];
    TRadioButtonData    CompoundFilenames;
    TRadioButtonData    GenericFilenames;
    TComboBoxData       FileTypes;

    TEditData           Regularization[ EditSizeText ];
    };


EndBytePacking

//----------------------------------------------------------------------------

class   TGoF;


class   TFileCalculatorDialog   :   public  TBaseDialog
{
public:
                        TFileCalculatorDialog ( owl::TWindow* parent, owl::TResId resId );
                       ~TFileCalculatorDialog ();

protected:
    owl::TEdit          *NumGroups;
    owl::TListBox       *GroupsSummary;

    owl::TComboBox      *Expression;

    owl::TEdit          *BaseDir;
    owl::TRadioButton   *CompoundFilenames;
    owl::TRadioButton   *GenericFilenames;
    owl::TComboBox      *FileTypes;

    owl::TEdit          *Regularization;


    static TGoGoF       GoGoF;


    bool                CheckGroups             ( const TGoF& gof );
    void                AddFileToGroup          ( const char* filename, bool first );
    void                GuessOutputFileExtension();
    void                AddGroupSummary         ( int gofi );
    void                SetBaseFilename         ();

    void                CmOk                    ();
    void                CmOkEnable              ( owl::TCommandEnabler &tce );

    void                CmUpOneDirectory        ();
    void                CmBrowseBaseFileName    ();
    void                CmAddGroup              ( owlwparam w );
    void                CmRemoveGroup           ();
    void                CmClearGroups           ();
    void                CmSortGroups            ();
    void                EvDropFiles             ( owl::TDropInfo drop );
    void                CmReadParams            ();
    void                ReadParams              ( char *filename = 0 );
    void                CmWriteParams           ();


    DECLARE_RESPONSE_TABLE ( TFileCalculatorDialog );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
