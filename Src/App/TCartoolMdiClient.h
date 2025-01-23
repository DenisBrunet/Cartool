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

#include    "owl/mdi.h"

#include    "resource.h"

#include    "TCartoolApp.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum        FrequencyAnalysisType; 
class       TGoF;


class   TCartoolMdiClient   :   public  owl::TMDIClient,
                                public  TCartoolObjects

{
public:

    using           owl::TMDIClient::TMDIClient;
    virtual        ~TCartoolMdiClient       ();


    bool            CanClose                ();

    void            RefreshWindows          ();


    void            CmWinAction             ( owlwparam w );
    void            CmDocWinAction          ( owlwparam w );
    void            CmAllWinAction          ( owlwparam w );
    void            CmGroupWinAction        ( owlwparam w );
    void            CmCloseChildren         ();
    void            ScreenToClient          ( owl::TRect& r );


protected:

    void            SetupWindow             ();
    void            BeforeClosing           ();

    void            CmNewEmptyLM            ();
    void            CmNewLMFromExisting     ();

    void            CmDocWinActionEnable    ( owl::TCommandEnabler &tce );
    void            CmAllWinActionEnable    ( owl::TCommandEnabler &tce );
    void            CmGroupWinActionEnable  ( owl::TCommandEnabler &tce );

    void            CmToolsHelp             ();

    void            EvDropFiles             ( owl::TDropInfo );

                                        // Menu interface handlers:

                                        // Runs actual specialized dialogs
    void            CmComputeRis            ();
    void            CmCoregistration        ();
    void            CmCreateInverseSolutions();
    void            CmCreateRois            ();
    void            CmExportTracks          ();
    void            CmFileCalculator        ();
    void            CmFreqAnalysis          ();
    void            CmInteractiveAveraging  ();
    void            CmInterpolate           ();
    void            CmPreprocessMris        ();
    void            CmRisToVolume           ();
    void            CmStatistics            ( owlwparam w );

    void            CmSegmentEeg            ();
    void            CmFitting               ();

                                        // Does not run actual dialogs but simplified UI (simply asking user some questions)
    void            GenerateDataUI          ( owlwparam w );
    void            GenerateOscillatingDataUI ();
    void            GenerateRandomDataUI    ();
    void            AnalyzeGeneratedDataUI  ( owlwparam w );

    void            BuildTemplateElectrodesUI ();
    void            DownsamplingElectrodesUI();
    void            ExtractElectrodesFromKriosUI ();

    void            BrainToSolutionPointsUI ();

    void            BatchProcessMrisUI      ( owlwparam w );
    void            MergingMriMasksUI       ();
    void            CoregistrationMrisUI    ();
    void            ComputingTemplateMriUI  ();

    void            BatchAveragingFilesUI   ( owlwparam w );
    void            ComputeCentroidFilesUI  ( owlwparam w );
    void            CorrelateFilesUI        ();
    void            PCA_ICA_UI              ( owlwparam w );

    void            SplitFreqFilesUI        ( owlwparam w );
    void            MergeTracksToFreqFilesUI();
//  void            SplitMatFilesUI         ( owlwparam w );
    void            FilesConversionVrbToTvaUI ();
    void            RisToCloudVectorsUI     ();


    DECLARE_RESPONSE_TABLE(TCartoolMdiClient);
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
