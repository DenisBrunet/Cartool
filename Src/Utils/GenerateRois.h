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

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum    CreateRoisTypes;
class   TRois;
class   TBaseDoc;
class   TVolumeDoc;
class   TSolutionPointsDoc;


bool    GenerateRois                    (
                                        CreateRoisTypes             Processing,
                                        const TRois&                Rois,

                                        const TBaseDoc*             BaseDoc,            // mandatory - could be either tracks, electrodes or solution points doc
                                        const TVolumeDoc*           MRIDoc,
                                        const char*                 RoisLabelsFile,     // optional - labels associated with MRIDoc
                                        const char*                 GenerateRoisFile,   // optional

                                        const char*                 BaseFileName
                                        );


bool    GenerateRoisFromSpAndMri        (
                                        CreateRoisTypes             Processing,

                                        const TSolutionPointsDoc*   SPDoc,
                                        const TVolumeDoc*           MRIDoc,
                                        const char*                 RoisLabelsFile,     // labels associated with MRIDoc

                                        const char*                 OptionalRoisFile,   // optional files with specific ROIS - otherwise it will generate all ROIs

                                        const char*                 BaseFileName
                                        );


bool    GenerateRoisFromSpAndTalairach  (
                                        CreateRoisTypes             Processing,

                                        const TSolutionPointsDoc*   SPDoc,
                                        const TVolumeDoc*           MRIDoc,             // optional volume

                                        const char*                 GenerateRoisFile,   // mandatory file with specific ROIS

                                        const char*                 BaseFileName
                                        );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
