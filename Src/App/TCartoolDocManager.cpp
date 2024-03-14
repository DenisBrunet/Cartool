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

#include    <owl/picklist.h>

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "TCartoolDocManager.h"
#include    "TCartoolMdiChild.h"
#include    "Dialogs.Input.h"

#include    "TBaseDoc.h"
#include    "TEegBIDMC128Doc.h"
#include    "TEegBiosemiBdfDoc.h"
#include    "TEegBioLogicDoc.h"
#include    "TEegBrainVisionDoc.h"
#include    "TEegMIDDoc.h"
#include    "TEegCartoolEpDoc.h"
#include    "TEegEgiRawDoc.h"
#include    "TEegEgiNsrDoc.h"
#include    "TEegEgiMffDoc.h"
#include    "TEegNeuroscanCntDoc.h"
#include    "TEegNeuroscanAvgDoc.h"
#include    "TEegERPSSRdfDoc.h"
#include    "TEegCartoolSefDoc.h"
#include    "TEegMicromedTrcDoc.h"

#include    "TRisDoc.h"
#include    "TSegDoc.h"
#include    "TXyzDoc.h"
#include    "TElsDoc.h"
#include    "TSpiDoc.h"
#include    "TSxyzDoc.h"
#include    "TLocDoc.h"
#include    "TMatrixIsDoc.h"
#include    "TMatrixSpinvDoc.h"
#include    "TFreqCartoolDoc.h"
#include    "TVolumeAvsDoc.h"
#include    "TVolumeAnalyzeDoc.h"
#include    "TVolumeVmrDoc.h"
#include    "TVolumeNiftiDoc.h"
#include    "TRoisDoc.h"
#include    "TLinkManyDoc.h"

#include    "TTracksView.h"
#include    "TElectrodesView.h"
#include    "TPotentialsView.h"
#include    "TSolutionPointsView.h"
#include    "TInverseMatrixView.h"
#include    "TInverseView.h"
#include    "TFrequenciesView.h"
#include    "TVolumeView.h"
#include    "TRoisView.h"
#include    "TLinkManyView.h"

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

TCartoolObjects     CartoolObjects;

                                        // Just allocate and reset these guys - Main application will set them at creation time (and only once)
TCartoolApp*        TCartoolObjects::CartoolApplication     = 0;
TCartoolDocManager* TCartoolObjects::CartoolDocManager      = 0;
TDecoratedMDIFrame* TCartoolObjects::CartoolMainWindow      = 0;
TCartoolMdiClient*  TCartoolObjects::CartoolMdiClient       = 0;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// To add a new type of EEG file:
//
//  create 2 new files in ./src, with names CrTlE*.h and .cpp
//  add the cpp file in the source pool AllCppAndH
//  add to it the dependencies of .h,
//  add to TCartoolDocManager.cpp and TCartoolMdiClient.cpp the dependency to the new .h
//  add in Files.Extensions.h the file extensions
//  add in TCartoolApp.cpp the file registration infos in the predefined arrays
//  copy/create the code for the new file
//  add in TCartoolApp.cpp the appropriate templates
//  check for ReadFromHeader
//  run a grep on all .cpp for _EEGEPH , then add the line with the new extension
//  idem with                  TEegBIDMC128Doc (f.ex.)
//  ( and maybe a grep for     TEegCartoolEpDoc )


//----------------------------------------------------------------------------
                                        // Define Doc-View pairing templates

DEFINE_DOC_TEMPLATE_CLASS ( TXyzDoc,            TElectrodesView,    TemplXyzView                );

DEFINE_DOC_TEMPLATE_CLASS ( TElsDoc,            TElectrodesView,    TemplElsView                );


DEFINE_DOC_TEMPLATE_CLASS ( TSpiDoc,            TSolutionPointsView,TemplSpiView                );

DEFINE_DOC_TEMPLATE_CLASS ( TLocDoc,            TSolutionPointsView,TemplLocView                );

DEFINE_DOC_TEMPLATE_CLASS ( TSxyzDoc,           TSolutionPointsView,TemplSxyzView               );


DEFINE_DOC_TEMPLATE_CLASS ( TMatrixIsDoc,       TInverseMatrixView, TemplIsView                 );

DEFINE_DOC_TEMPLATE_CLASS ( TMatrixSpinvDoc,    TInverseMatrixView, TemplSpinvView              );


DEFINE_DOC_TEMPLATE_CLASS ( TVolumeAvsDoc,      TVolumeView,        TemplMriAvsVolumeView       );
DEFINE_DOC_TEMPLATE_CLASS ( TVolumeAnalyzeDoc,  TVolumeView,        TemplMriAnalyzeVolumeView   );
DEFINE_DOC_TEMPLATE_CLASS ( TVolumeVmrDoc,      TVolumeView,        TemplMriVmrVolumeView       );
DEFINE_DOC_TEMPLATE_CLASS ( TVolumeNiftiDoc,    TVolumeView,        TemplMriNiftiVolumeView     );


DEFINE_DOC_TEMPLATE_CLASS ( TEegCartoolEpDoc,   TTracksView,        TemplEegEpTracksView        );
DEFINE_DOC_TEMPLATE_CLASS ( TEegCartoolEpDoc,   TPotentialsView,    TemplEegEpPotentialsView    );
DEFINE_DOC_TEMPLATE_CLASS ( TEegCartoolEpDoc,   TInverseView,       TemplEegEpInverseView       );

DEFINE_DOC_TEMPLATE_CLASS ( TEegCartoolSefDoc,  TTracksView,        TemplEegSefTracksView       );
DEFINE_DOC_TEMPLATE_CLASS ( TEegCartoolSefDoc,  TPotentialsView,    TemplEegSefPotentialsView   );
DEFINE_DOC_TEMPLATE_CLASS ( TEegCartoolSefDoc,  TInverseView,       TemplEegSefInverseView      );

DEFINE_DOC_TEMPLATE_CLASS ( TEegEgiNsrDoc,      TTracksView,        TemplEegNsrTracksView       );
DEFINE_DOC_TEMPLATE_CLASS ( TEegEgiNsrDoc,      TPotentialsView,    TemplEegNsrPotentialsView   );
DEFINE_DOC_TEMPLATE_CLASS ( TEegEgiNsrDoc,      TInverseView,       TemplEegNsrInverseView      );

DEFINE_DOC_TEMPLATE_CLASS ( TEegEgiMffDoc,      TTracksView,        TemplEegMffTracksView       );
DEFINE_DOC_TEMPLATE_CLASS ( TEegEgiMffDoc,      TPotentialsView,    TemplEegMffPotentialsView   );
DEFINE_DOC_TEMPLATE_CLASS ( TEegEgiMffDoc,      TInverseView,       TemplEegMffInverseView      );

DEFINE_DOC_TEMPLATE_CLASS ( TEegBIDMC128Doc,    TTracksView,        TemplEeg128TracksView       );
DEFINE_DOC_TEMPLATE_CLASS ( TEegBIDMC128Doc,    TPotentialsView,    TemplEeg128PotentialsView   );
DEFINE_DOC_TEMPLATE_CLASS ( TEegBIDMC128Doc,    TInverseView,       TemplEeg128InverseView      );

DEFINE_DOC_TEMPLATE_CLASS ( TEegMIDDoc,         TTracksView,        TemplEegDTracksView         );
DEFINE_DOC_TEMPLATE_CLASS ( TEegMIDDoc,         TPotentialsView,    TemplEegDPotentialsView     );
DEFINE_DOC_TEMPLATE_CLASS ( TEegMIDDoc,         TInverseView,       TemplEegDInverseView        );

DEFINE_DOC_TEMPLATE_CLASS ( TEegBioLogicDoc,    TTracksView,        TemplEegBioTracksView       );
DEFINE_DOC_TEMPLATE_CLASS ( TEegBioLogicDoc,    TPotentialsView,    TemplEegBioPotentialsView   );
DEFINE_DOC_TEMPLATE_CLASS ( TEegBioLogicDoc,    TInverseView,       TemplEegBioInverseView      );

DEFINE_DOC_TEMPLATE_CLASS ( TEegEgiRawDoc,      TTracksView,        TemplEegNsrRawTracksView    );
DEFINE_DOC_TEMPLATE_CLASS ( TEegEgiRawDoc,      TPotentialsView,    TemplEegNsrRawPotentialsView );
DEFINE_DOC_TEMPLATE_CLASS ( TEegEgiRawDoc,      TInverseView,       TemplEegNsrRawInverseView   );

DEFINE_DOC_TEMPLATE_CLASS ( TEegNeuroscanCntDoc,TTracksView,        TemplEegNsCntTracksView     );
DEFINE_DOC_TEMPLATE_CLASS ( TEegNeuroscanCntDoc,TPotentialsView,    TemplEegNsCntPotentialsView );
DEFINE_DOC_TEMPLATE_CLASS ( TEegNeuroscanCntDoc,TInverseView,       TemplEegNsCntInverseView    );

DEFINE_DOC_TEMPLATE_CLASS ( TEegNeuroscanAvgDoc,TTracksView,        TemplEegNsAvgTracksView     );
DEFINE_DOC_TEMPLATE_CLASS ( TEegNeuroscanAvgDoc,TPotentialsView,    TemplEegNsAvgPotentialsView );
DEFINE_DOC_TEMPLATE_CLASS ( TEegNeuroscanAvgDoc,TInverseView,       TemplEegNsAvgInverseView    );

DEFINE_DOC_TEMPLATE_CLASS ( TEegMicromedTrcDoc, TTracksView,        TemplEegTrcTracksView       );
DEFINE_DOC_TEMPLATE_CLASS ( TEegMicromedTrcDoc, TPotentialsView,    TemplEegTrcPotentialsView   );
DEFINE_DOC_TEMPLATE_CLASS ( TEegMicromedTrcDoc, TInverseView,       TemplEegTrcInverseView      );

DEFINE_DOC_TEMPLATE_CLASS ( TEegBiosemiBdfDoc,  TTracksView,        TemplEegBdfTracksView       );
DEFINE_DOC_TEMPLATE_CLASS ( TEegBiosemiBdfDoc,  TPotentialsView,    TemplEegBdfPotentialsView   );
DEFINE_DOC_TEMPLATE_CLASS ( TEegBiosemiBdfDoc,  TInverseView,       TemplEegBdfInverseView      );

DEFINE_DOC_TEMPLATE_CLASS ( TEegERPSSRdfDoc,    TTracksView,        TemplEegRdfTracksView       );
DEFINE_DOC_TEMPLATE_CLASS ( TEegERPSSRdfDoc,    TPotentialsView,    TemplEegRdfPotentialsView   );
DEFINE_DOC_TEMPLATE_CLASS ( TEegERPSSRdfDoc,    TInverseView,       TemplEegRdfInverseView      );

DEFINE_DOC_TEMPLATE_CLASS ( TEegBrainVisionDoc, TTracksView,        TemplEegBvTracksView        );
DEFINE_DOC_TEMPLATE_CLASS ( TEegBrainVisionDoc, TPotentialsView,    TemplEegBvPotentialsView    );
DEFINE_DOC_TEMPLATE_CLASS ( TEegBrainVisionDoc, TInverseView,       TemplEegBvInverseView       );


DEFINE_DOC_TEMPLATE_CLASS ( TFreqCartoolDoc,    TFrequenciesView,   TemplFreqTracksView         );
DEFINE_DOC_TEMPLATE_CLASS ( TFreqCartoolDoc,    TPotentialsView,    TemplFreqPotentialsView     );
DEFINE_DOC_TEMPLATE_CLASS ( TFreqCartoolDoc,    TInverseView,       TemplFreqInverseView        );


DEFINE_DOC_TEMPLATE_CLASS ( TRisDoc,            TTracksView,        TemplRisTracksView          );
DEFINE_DOC_TEMPLATE_CLASS ( TRisDoc,            TInverseView,       TemplRisInverseView         );


DEFINE_DOC_TEMPLATE_CLASS ( TSegDoc,            TTracksView,        TemplSegTracksView          );


DEFINE_DOC_TEMPLATE_CLASS ( TLinkManyDoc,       TLinkManyView,      TemplLmView                 );


DEFINE_DOC_TEMPLATE_CLASS ( TRoisDoc,           TRoisView,          TemplRoisView               );


//----------------------------------------------------------------------------
                                        // Use the Doc-View templates


#define     TEMPLDESC_POTMAP    "Potential Map"
#define     TEMPLDESC_INVMRI    "Inverse in MRI"

                                                                 // Drop-down visible text (NOT fixed-width, hence != lengths)  Associated file filter                    & extension                           Opening options
TemplLmView                     templLmView                     (   FILEEXT_LM"\t Linking Many Files ",                         FILEFILTER_LM,                          0,  FILEEXT_LM,                         dtOpenOptions       );
                                                                                                                                                                                                                
                                                                                                                                                                                                                
TemplEegEpTracksView            templEegEphTracksView           (   FILEEXT_EEGEPH"\t Evoked Potentials + Header ",             FILEFILTER_EEGEPH,                      0,  FILEEXT_EEGEPH,                     dtOpenOptions       );
TemplEegEpPotentialsView        templEegEphPotentialsView       (   TEMPLDESC_POTMAP,                                           FILEFILTER_EEGEPH,                      0,  FILEEXT_EEGEPH,                     dtOpenOptionsHidden );
TemplEegEpInverseView           templEegEphInverseView          (   TEMPLDESC_INVMRI,                                           FILEFILTER_EEGEPH,                      0,  FILEEXT_EEGEPH,                     dtOpenOptionsHidden );
                                                                                                                                                                                                                
TemplEegEpTracksView            templEegEpTracksView            (   FILEEXT_EEGEP"\t Evoked Potentials ",                       FILEFILTER_EEGEP,                       0,  FILEEXT_EEGEP,                      dtOpenOptions       );
TemplEegEpPotentialsView        templEegEpPotentialsView        (   TEMPLDESC_POTMAP,                                           FILEFILTER_EEGEP,                       0,  FILEEXT_EEGEP,                      dtOpenOptionsHidden );
TemplEegEpInverseView           templEegEpInverseView           (   TEMPLDESC_INVMRI,                                           FILEFILTER_EEGEP,                       0,  FILEEXT_EEGEP,                      dtOpenOptionsHidden );
                                                                                                                                                                                                                
TemplEegSefTracksView           templEegSefTracksView           (   FILEEXT_EEGSEF"\t Simple Eeg Format ",                      FILEFILTER_EEGSEF,                      0,  FILEEXT_EEGSEF,                     dtOpenOptions       );
TemplEegSefPotentialsView       templEegSefPotentialsView       (   TEMPLDESC_POTMAP,                                           FILEFILTER_EEGSEF,                      0,  FILEEXT_EEGSEF,                     dtOpenOptionsHidden );
TemplEegSefInverseView          templEegSefInverseView          (   TEMPLDESC_INVMRI,                                           FILEFILTER_EEGSEF,                      0,  FILEEXT_EEGSEF,                     dtOpenOptionsHidden );
                                                                                                                                                                                                                
TemplEegNsrTracksView           templEegNsrTracksView           (   FILEEXT_EEGNSR "\t NetStation ",                            FILEFILTER_EEGNSR,                      0,  FILEEXT_EEGNSR,                     dtOpenOptions       );
TemplEegNsrPotentialsView       templEegNsrPotentialsView       (   TEMPLDESC_POTMAP,                                           FILEFILTER_EEGNSR,                      0,  FILEEXT_EEGNSR,                     dtOpenOptionsHidden );
TemplEegNsrInverseView          templEegNsrInverseView          (   TEMPLDESC_INVMRI,                                           FILEFILTER_EEGNSR,                      0,  FILEEXT_EEGNSR,                     dtOpenOptionsHidden );
                                                                                                                                                                                                                
TemplEegMffTracksView           templEegMffTracksView           (   FILEEXT_EEGMFF "\t NetStation ",                            FILEFILTER_EEGMFF,                      0,  FILEEXT_EEGMFF,                     dtOpenOptions       );
TemplEegMffPotentialsView       templEegMffPotentialsView       (   TEMPLDESC_POTMAP,                                           FILEFILTER_EEGMFF,                      0,  FILEEXT_EEGMFF,                     dtOpenOptionsHidden );
TemplEegMffInverseView          templEegMffInverseView          (   TEMPLDESC_INVMRI,                                           FILEFILTER_EEGMFF,                      0,  FILEEXT_EEGMFF,                     dtOpenOptionsHidden );
                                                                                                                                                                                                                
TemplEegTrcTracksView           templEegTrcTracksView           (   FILEEXT_EEGTRC"\t micromed ",                               FILEFILTER_EEGTRC,                      0,  FILEEXT_EEGTRC,                     dtOpenOptions       );
TemplEegTrcPotentialsView       templEegTrcPotentialsView       (   TEMPLDESC_POTMAP,                                           FILEFILTER_EEGTRC,                      0,  FILEEXT_EEGTRC,                     dtOpenOptionsHidden );
TemplEegTrcInverseView          templEegTrcInverseView          (   TEMPLDESC_INVMRI,                                           FILEFILTER_EEGTRC,                      0,  FILEEXT_EEGTRC,                     dtOpenOptionsHidden );
                                                                                                                                                                                                                
TemplEegBdfTracksView           templEegBdfTracksView           (   FILEEXT_EEGBDF"\t BioSemi ",                                FILEFILTER_EEGBDF,                      0,  FILEEXT_EEGBDF,                     dtOpenOptions       );
TemplEegBdfPotentialsView       templEegBdfPotentialsView       (   TEMPLDESC_POTMAP,                                           FILEFILTER_EEGBDF,                      0,  FILEEXT_EEGBDF,                     dtOpenOptionsHidden );
TemplEegBdfInverseView          templEegBdfInverseView          (   TEMPLDESC_INVMRI,                                           FILEFILTER_EEGBDF,                      0,  FILEEXT_EEGBDF,                     dtOpenOptionsHidden );
                                                                                                                                                                                                                
TemplEegBdfTracksView           templEegEdfTracksView           (   FILEEXT_EEGEDF"\t European Data Format ",                   FILEFILTER_EEGEDF,                      0,  FILEEXT_EEGEDF,                     dtOpenOptions       );
TemplEegBdfPotentialsView       templEegEdfPotentialsView       (   TEMPLDESC_POTMAP,                                           FILEFILTER_EEGEDF,                      0,  FILEEXT_EEGEDF,                     dtOpenOptionsHidden );
TemplEegBdfInverseView          templEegEdfInverseView          (   TEMPLDESC_INVMRI,                                           FILEFILTER_EEGEDF,                      0,  FILEEXT_EEGEDF,                     dtOpenOptionsHidden );
                                                                                                                                                                                                                
TemplEegRdfTracksView           templEegRdfTracksView           (   FILEEXT_EEGRDF"\t ERPSS ",                                  FILEFILTER_EEGRDF,                      0,  FILEEXT_EEGRDF,                     dtOpenOptions       );
TemplEegRdfPotentialsView       templEegRdfPotentialsView       (   TEMPLDESC_POTMAP,                                           FILEFILTER_EEGRDF,                      0,  FILEEXT_EEGRDF,                     dtOpenOptionsHidden );
TemplEegRdfInverseView          templEegRdfInverseView          (   TEMPLDESC_INVMRI,                                           FILEFILTER_EEGRDF,                      0,  FILEEXT_EEGRDF,                     dtOpenOptionsHidden );
                                                                                                                                                                                                                
TemplEeg128TracksView           templEeg128TracksView           (   FILEEXT_EEG128"\t Depth 128 ",                              FILEFILTER_EEG128,                      0,  FILEEXT_EEG128,                     dtOpenOptions       );
TemplEeg128PotentialsView       templEeg128PotentialsView       (   TEMPLDESC_POTMAP,                                           FILEFILTER_EEG128,                      0,  FILEEXT_EEG128,                     dtOpenOptionsHidden );
TemplEeg128InverseView          templEeg128InverseView          (   TEMPLDESC_INVMRI,                                           FILEFILTER_EEG128,                      0,  FILEEXT_EEG128,                     dtOpenOptionsHidden );
                                                                                                                                                                                                                
TemplEegBioTracksView           templEegBioTracksView           (   FILEEXT_BIOEEG"\t Biologic ",                               FILEFILTER_EEGBIO,                      0,  FILEEXT_EEGBIO,                     dtOpenOptions       );
TemplEegBioPotentialsView       templEegBioPotentialsView       (   TEMPLDESC_POTMAP,                                           FILEFILTER_EEGBIO,                      0,  FILEEXT_EEGBIO,                     dtOpenOptionsHidden );
TemplEegBioInverseView          templEegBioInverseView          (   TEMPLDESC_INVMRI,                                           FILEFILTER_EEGBIO,                      0,  FILEEXT_EEGBIO,                     dtOpenOptionsHidden );

TemplEegBvTracksView            templEegBvTracksView            (   FILEEXT_EEGBV " " FILEEXT_EEGBVDAT "\t Brain Products ",    FILEFILTER_EEGBV ";" FILEFILTER_BVDAT,  0,  FILEEXT_EEGBV ";" FILEEXT_EEGBVDAT, dtOpenOptions       );
TemplEegBvPotentialsView        templEegBvPotentialsView        (   TEMPLDESC_POTMAP,                                           FILEFILTER_EEGBV ";" FILEFILTER_BVDAT,  0,  FILEEXT_EEGBV ";" FILEEXT_EEGBVDAT, dtOpenOptionsHidden );
TemplEegBvInverseView           templEegBvInverseView           (   TEMPLDESC_INVMRI,                                           FILEFILTER_EEGBV ";" FILEFILTER_BVDAT,  0,  FILEEXT_EEGBV ";" FILEEXT_EEGBVDAT, dtOpenOptionsHidden );

TemplEegDTracksView             templEegDTracksView             (   FILEEXT_EEGD"\t EaSys ",                                    FILEFILTER_EEGD,                        0,  FILEEXT_EEGD,                       dtOpenOptions       );
TemplEegDPotentialsView         templEegDPotentialsView         (   TEMPLDESC_POTMAP,                                           FILEFILTER_EEGD,                        0,  FILEEXT_EEGD,                       dtOpenOptionsHidden );
TemplEegDInverseView            templEegDInverseView            (   TEMPLDESC_INVMRI,                                           FILEFILTER_EEGD,                        0,  FILEEXT_EEGD,                       dtOpenOptionsHidden );

TemplEegNsrRawTracksView        templEegNsrRawTracksView        (   FILEEXT_EEGNSRRAW"\t NetStation ",                          FILEFILTER_EEGNSRRAW,                   0,  FILEEXT_EEGNSRRAW,                  dtOpenOptions       );
TemplEegNsrRawPotentialsView    templEegNsrRawPotentialsView    (   TEMPLDESC_POTMAP,                                           FILEFILTER_EEGNSRRAW,                   0,  FILEEXT_EEGNSRRAW,                  dtOpenOptionsHidden );
TemplEegNsrRawInverseView       templEegNsrRawInverseView       (   TEMPLDESC_INVMRI,                                           FILEFILTER_EEGNSRRAW,                   0,  FILEEXT_EEGNSRRAW,                  dtOpenOptionsHidden );

TemplEegNsCntTracksView         templEegNsCntTracksView         (   FILEEXT_EEGNSCNT"\t NeuroScan ",                            FILEFILTER_EEGNSCNT,                    0,  FILEEXT_EEGNSCNT,                   dtOpenOptions       );
TemplEegNsCntPotentialsView     templEegNsCntPotentialsView     (   TEMPLDESC_POTMAP,                                           FILEFILTER_EEGNSCNT,                    0,  FILEEXT_EEGNSCNT,                   dtOpenOptionsHidden );
TemplEegNsCntInverseView        templEegNsCntInverseView        (   TEMPLDESC_INVMRI,                                           FILEFILTER_EEGNSCNT,                    0,  FILEEXT_EEGNSCNT,                   dtOpenOptionsHidden );

TemplEegNsAvgTracksView         templEegNsAvgTracksView         (   FILEEXT_EEGNSAVG"\t NeuroScan ",                            FILEFILTER_EEGNSAVG,                    0,  FILEEXT_EEGNSAVG,                   dtOpenOptions       );
TemplEegNsAvgPotentialsView     templEegNsAvgPotentialsView     (   TEMPLDESC_POTMAP,                                           FILEFILTER_EEGNSAVG,                    0,  FILEEXT_EEGNSAVG,                   dtOpenOptionsHidden );
TemplEegNsAvgInverseView        templEegNsAvgInverseView        (   TEMPLDESC_INVMRI,                                           FILEFILTER_EEGNSAVG,                    0,  FILEEXT_EEGNSAVG,                   dtOpenOptionsHidden );


TemplFreqTracksView             templFreqTracksView             (   FILEEXT_FREQ"\t Frequency Analysis ",                       FILEFILTER_FREQ,                        0,  FILEEXT_FREQ,                       dtOpenOptions       );
TemplFreqPotentialsView         templFreqPotentialsView         (   TEMPLDESC_POTMAP,                                           FILEFILTER_FREQ,                        0,  FILEEXT_FREQ,                       dtOpenOptionsHidden );
TemplFreqInverseView            templFreqInverseView            (   TEMPLDESC_INVMRI,                                           FILEFILTER_FREQ,                        0,  FILEEXT_FREQ,                       dtOpenOptionsHidden );


TemplEegEpTracksView            templEegSDTracksView            (   FILEEXT_EEGEPSD"\t Standard Deviation of EP ",              FILEFILTER_EEGEPSD,                     0,  FILEEXT_EEGEPSD,                    dtOpenOptions       );

TemplEegEpTracksView            templEegSETracksView            (   FILEEXT_EEGEPSE"\t Standard Error of EP ",                  FILEFILTER_EEGEPSE,                     0,  FILEEXT_EEGEPSE,                    dtOpenOptions       );


TemplRoisView                   templRoisView                   (   FILEEXT_ROIS"\t Regions of Interest ",                      FILEFILTER_ROIS,                        0,  FILEEXT_ROIS,                        dtOpenOptions       );


TemplXyzView                    templXyzView                    (   FILEEXT_XYZ"\t Head Coordinates ",                          FILEFILTER_XYZ,                         0,  FILEEXT_XYZ,                        dtOpenOptions       );

TemplElsView                    templElsView                    (   FILEEXT_ELS"\t Electrodes Setup ",                          FILEFILTER_ELS,                         0,  FILEEXT_ELS,                        dtOpenOptions       );


TemplSpiView                    templSpiView                    (   FILEEXT_SPIRR "\t Solution Points ",                        FILEFILTER_SPIRR,                       0,  FILEEXT_SPIRR,                      dtOpenOptions       );
TemplLocView                    templLocView                    (   FILEEXT_LOC"\t Solution Points ",                           FILEFILTER_LOC,                         0,  FILEEXT_LOC,                        dtOpenOptions       );
TemplSxyzView                   templSxyzView                   (   FILEEXT_SXYZ"\t Loreta Coordinates ",                       FILEFILTER_SXYZ,                        0,  FILEEXT_SXYZ,                       dtOpenOptions       );


TemplIsView                     templIsView                     (   FILEEXT_IS"\t Inverse Solution ",                           FILEFILTER_IS,                          0,  FILEEXT_IS,                         dtOpenOptions       );
TemplSpinvView                  templSpinvView                  (   FILEEXT_SPINV"\t Inverse Solution ",                        FILEFILTER_SPINV,                       0,  FILEEXT_SPINV,                      dtOpenOptions       );


TemplMriAvsVolumeView           templMriAvsVolumeView           (   FILEEXT_MRIAVS"\t MRI AVS",                                 FILEFILTER_MRIAVS,                      0,  FILEEXT_MRIAVS,                     dtOpenOptions       );
TemplMriAnalyzeVolumeView       templMriAnalyzeVolumeView       (   FILEEXT_MRIAVW_HDR "\t MRI Analyze",                        FILEFILTER_MRIAVW_HDR,                  0,  FILEEXT_MRIAVW_HDR,                 dtOpenOptions       );
TemplMriAnalyzeVolumeView       templMriAnalyzeVolumeView2      (   FILEEXT_MRIAVW_IMG"\t MRI Analyze",                         FILEFILTER_MRIAVW_IMG,                  0,  FILEEXT_MRIAVW_IMG,                 dtOpenOptions       );
TemplMriVmrVolumeView           templMriVmrVolumeView           (   FILEEXT_MRIVMR "\t MRI BrainVoyager",                       FILEFILTER_MRIVMR,                      0,  FILEEXT_MRIVMR,                     dtOpenOptions       );
TemplMriNiftiVolumeView         templMriNiftiVolumeView         (   FILEEXT_MRINII "\t MRI Nifti",                              FILEFILTER_MRINII,                      0,  FILEEXT_MRINII,                     dtOpenOptions       );


TemplRisTracksView              templRisTracksView              (   FILEEXT_RIS"\t Result of Inverse Solution ",                FILEFILTER_RIS,                         0,  FILEEXT_RIS,                        dtOpenOptions       );
TemplRisInverseView             templRisInverseView             (   TEMPLDESC_INVMRI,                                           FILEFILTER_RIS,                         0,  FILEEXT_RIS,                        dtOpenOptionsHidden );


TemplSegTracksView              templSegTracksView              (   FILEEXT_SEG"\t Segments ",                                  FILEFILTER_SEG,                         0,  FILEEXT_SEG,                        dtOpenOptions       );
TemplSegTracksView              templDataTracksView             (   FILEEXT_DATA"\t Data ",                                     FILEFILTER_DATA,                        0,  FILEEXT_DATA,                       dtOpenOptions       );


//----------------------------------------------------------------------------
DEFINE_RESPONSE_TABLE1( TCartoolDocManager, TDocManager )

//  EV_WM_CANCLOSE,
//  EV_WM_PREPROCMENU,  // used to pre-process menus before window is displayed
//  EV_WM_WAKEUP,

//  EV_COMMAND(CM_FILENEW,           CmFileNew      ),
    EV_COMMAND(CM_FILEOPEN,          CmFileOpen     ),
    EV_COMMAND(CM_FILESAVE,          CmFileSave     ),
    EV_COMMAND(CM_FILESAVEAS,        CmFileSaveAs   ),
    EV_COMMAND(CM_FILEREVERT,        CmFileRevert   ),
//  EV_COMMAND(CM_FILECLOSE,         CmFileClose    ),
    EV_COMMAND(CM_VIEWCREATE,        CmViewCreate   ),

END_RESPONSE_TABLE;


//----------------------------------------------------------------------------
                                        // Checklist for functions related to file opening:
                                        //
                                        //      CmFileNew,  CmFileOpen
                                        //      CreateDoc,  CreateAnyDoc
                                        //      OpenDoc
                                        //      ProcessCmdLine, AddFiles, CmFileSelected


        TCartoolDocManager::TCartoolDocManager ( int mode, TApplication *app, TDocTemplate *&templateHead )
      : TDocManager ( mode, app, templateHead )
{
}


//----------------------------------------------------------------------------
int     TCartoolDocManager::NumDocOpen ()
{
int                 numdoc          = 0;

for ( auto doc = DocList.Next ( 0 ); doc != 0; doc = DocList.Next ( doc ) )
    numdoc++;

return  numdoc;
}


//----------------------------------------------------------------------------
TBaseDoc    *TCartoolDocManager::IsOpen ( const char *filename )
{
if ( StringIsEmpty ( filename ) )
    return  false;

return  dynamic_cast<TBaseDoc*> ( FindDocument ( TFileName ( filename, TFilenameExtendedPath ) ) );

/*
TBaseDoc           *doc;

for ( doc = dynamic_cast< TBaseDoc * > ( DocList.Next ( 0 ) ); doc != 0; doc = dynamic_cast< TBaseDoc * > ( DocList.Next ( doc ) ) )
    if ( StringIsNotEmpty ( doc->GetDocPath () )
      && StringIs ( doc->GetDocPath (), filename ) )
        return doc;

return 0;
*/
}


//----------------------------------------------------------------------------
                                        // Retrieve all opened docs, optionally filtered by file extensions
void    TCartoolDocManager::GetDocs ( TGoF& gof, const char* extensions )
{
gof.Reset ();

for ( auto doc = DocList.Next ( 0 ); doc != 0; doc = DocList.Next ( doc ) )

    if ( StringIsEmpty ( extensions ) || IsExtensionAmong ( doc->GetDocPath (), extensions ) )

        gof.Add ( doc->GetDocPath () );
}


//----------------------------------------------------------------------------
TDocument   *TCartoolDocManager::CreateAnyDoc ( const char *path, long flags )
{
                                        // process file name copy
TFileName           pathok ( path, (TFilenameFlags) ( TFilenameMsDosToWindows | TFilenameExtendedPath | TFilenameSibling ) );
                                        // path is coming as const, we shouldn't modify it!
//StringCopy ( (char*) path, pathok );

                                        // overload flag to avoid opening view if not interactive
if ( CartoolApplication->IsNotInteractive () )
    
    SetFlags ( flags, (long) owl::dtNoAutoView );


bool                oldav           = CartoolApplication->AnimateViews;
CartoolApplication->AnimateViews    =    CartoolApplication->AnimateViews       // might have been reset elsewhere
                                      && CartoolApplication->IsInteractive ()   // must be interactive
                                      && NumDocOpen () < AnimationMaxDocOpen    // and not already crowded
                                      && ! IsExtension ( path, FILEEXT_LM);     // and not dropping a lm file either


TDocument*          todoc           = TDocManager::CreateAnyDoc ( /*path*/ pathok, flags );


CartoolApplication->AnimateViews    = oldav;


//UpdateApplication;

return  todoc;
}


//----------------------------------------------------------------------------
                                        // Returns the list of templates which can be Newed
                                        // Could be easier if we could introduce our own flags, like dtNew?
int     TCartoolDocManager::GetNewTemplates ( TDocTemplate** tplList, int size, bool newDoc )
{
                                        // No templates?
if ( ! GetTemplateList () )

    return  TDocManager::GetNewTemplates ( tplList, size, newDoc );

                                        // Walk through all templates, looking for the visible ones, and if a new doc, non-dtReadOnly ones. (from Borland)
int                 tplCount        = 0;

for ( TDocTemplate* tpl = GetTemplateList (); tpl; tpl = tpl->GetNextTemplate () ) {

    if (     tpl->IsVisible ()
      && ! ( tpl->IsFlagSet ( dtReadOnly ) && newDoc )
      && IsStringAmong ( tpl->GetDefaultExt (), AllCartoolNewFileExt ) ) {

        if ( tplList ) {
            CHECK(tplCount < size);
            tplList[ tplCount ] = tpl;
            }

        tplCount++;
        }
    }


return tplCount;
}


//----------------------------------------------------------------------------
void    TCartoolDocManager::CmFileOpen ()
{
                                        // use modern interface
static GetFileFromUser  getfiles ( "", FileOpenFilesFilter, 1, GetFileMulti );


if ( ! getfiles.Execute () )
    return;

                                            // disabling animations?
bool                oldav           = CartoolApplication->AnimateViews;
CartoolApplication->AnimateViews    =    CartoolApplication->AnimateViews           // might have been reset elsewhere
                                      && (int) getfiles <= AnimationMaxDocDropped;  // only few files dropped


for ( int i = 0; i < (int) getfiles; i++ )
    OpenDoc ( getfiles[ i ], dtOpenOptions );


//UpdateApplication;

CartoolApplication->AnimateViews    = oldav;
}


//----------------------------------------------------------------------------
void    TCartoolDocManager::CmFileRevert ()
{
TDocManager::CmFileRevert ();

CartoolMdiClient->RefreshWindows ();
}


//----------------------------------------------------------------------------
void    TCartoolDocManager::CmFileSave ()
{
TDocument*          doc             = GetCurrentDoc ();

if ( ! doc )
    return;

                                        // Some files are allowed without much check
if ( IsExtensionAmong ( doc->GetDocPath (), AllCartoolSaveFileExt ) )

    TDocManager::CmFileSave ();

else
    //ShowMessage ( "You can not directly save this type of file," NewLine "though you can try some processing from the Tools menu...", "Save File", ShowMessageWarning );
                                        // For tracks-like, we are more cautious and ask for confirmation
    if ( IsExtensionAmong ( doc->GetDocPath (), AllTracksFilesExt ) )
                                        // Explicitly warn / ask user about saving, we don't want to overwrite original files...
        CmFileSaveAs ();
}


//----------------------------------------------------------------------------
void    TCartoolDocManager::CmFileSaveAs ()
{
TDocument*          doc             = GetCurrentDoc ();

if ( ! doc )
    return;

                                        // could also preset the file filter according to current file type...
static GetFileFromUser  getfile ( "File Save As", FileOpenFilesFilter, 1, GetFileWrite );

                                        // Asking for new file path
if ( ! getfile.Execute ( doc->GetDocPath () ) )
    return;


TFileName           filename ( (const char *) getfile );

                                        // Not all file types can be saved
if ( ! IsExtensionAmong ( filename, AllCartoolSaveFileExt " " AllCommitTracksExt ) ) {

    ShowMessage (   "Cartool doesn't know how to save this type of file!" NewLine 
                    "Either check the file extension, or use some proper processing from the Tools menu...", "Save File", ShowMessageWarning );
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//DBGM ( filename, "TheApp::Save As" );

                                        // Setting new file name
if ( ! doc->SetDocPath ( filename ) )
    return;

                                        // Notifying that doc has indeed been renamed
PostEvent ( dnRename, *doc );           


//                                        // Updating template, in case file type has changed
//TDocTemplate       *tpl             = MatchTemplate ( filename );
//
////DBGM2 ( doc->GetTemplate ()->GetDescription (), tpl ? tpl->GetDescription () : "No Template", "TheApp::Save As  /  Templates old & new" );
//
//if ( tpl && tpl != doc->GetTemplate () )
//
//    doc->SetTemplate ( tpl );

                                        // Force writing to new path (from CmFileSaveAs), using actual new file extension
if ( doc->Commit ( true ) ) {
                                        // retrieving current window position
    TWindowAttr         Attr        = doc->GetViewList ()->GetWindow ()->GetParentO ()->Attr;

                                        // Force closing new file: doc* can be of one type, new file from another one!
    CloseDoc    ( dynamic_cast< TBaseDoc*> ( doc ) );

                                        // Re-opening new file, from scratch, so that everything is clean (pointer, template, internal state)
    TBaseDoc*           newdoc      = OpenDoc     ( filename, dtOpenOptions );

                                        // Restauring old window position
    if ( newdoc && newdoc->GetViewList () )
        newdoc->GetViewList ()->WindowSetPosition ( Attr.X, Attr.Y, Attr.W, Attr.H );
    }
}


//----------------------------------------------------------------------------
TBaseView      *TCartoolDocManager::GetCurrentView ()
{
HWND                hWnd            = CartoolMainWindow->GetCommandTarget ();
TDocument*          doc             = 0;
TWindow*            towin;

if ( hWnd )

    if ( ::IsWindow ( hWnd ) ) {

        while ( (doc = DocList.Next(doc)) != 0 ) {

            TDocument*          childDoc    = doc->DocWithFocus ( hWnd );

            if ( childDoc ) {

                towin = CartoolApplication->GetWindowPtr ( hWnd );
                                        // switch to child/parent if needed, to have a TBaseView
                if ( dynamic_cast<TCartoolMdiChild *> ( towin ) )
                    towin = towin->GetFirstChild();

                if ( dynamic_cast<TTracksViewScrollbar *> ( towin ) )
                    towin = towin->GetParentO();

                return dynamic_cast<TBaseView *> ( towin );
                }
            }
        }

return 0;
}


//----------------------------------------------------------------------------
int     TCartoolDocManager::SelectViewType ( TDocTemplate** tpllist, int tplcount )
{
TDocument*          doc             = GetCurrentDoc ();
TBaseView*          view            = GetCurrentView ();
int                 index[ 32 ];        // keep tracks of the original location

// hide the templates if doc is an eeg, and either is not belonging to a group
// or the group does not contain all the kinds of docs for all the views.

if ( doc && view ) {
                                        // a BaseEEG derived class?
    if ( dynamic_cast<TTracksDoc*> ( doc ) ) {
                                        // no group -> no other views available
        if ( view->GODoc == 0 ) {
            return 1;                   // always eeg view as first view type
            }
        else {                          // build a context sensitive list
            TPickListPopup pickl ( CartoolMainWindow, IDS_VIEWLIST );

            for ( int i=0, j=0 ; i < tplcount; i++, tpllist++ ) {
                CHECK(*tpllist);
                                        // add first view, always
                if ( i == 0 ) {
                    index[j++]  = i;
                    pickl.AddString ( (*tpllist)->GetViewName() );
                    }

                if ( StringIs ( (*tpllist)->GetDescription (), TEMPLDESC_POTMAP ) )
                    if ( view->GODoc->GetNumEegDoc() && view->GODoc->GetNumXyzDoc() ) {
                        index[j++]  = i;
                        pickl.AddString ( (*tpllist)->GetViewName() );
                        }

                if ( StringIs ( (*tpllist)->GetDescription (), TEMPLDESC_INVMRI ) )
                    if ( view->GODoc->GetNumEegDoc() && view->GODoc->GetNumSpDoc() && view->GODoc->GetNumIsDoc() && view->GODoc->GetNumMriDoc()
                      || view->GODoc->GetNumRisDoc() && view->GODoc->GetNumSpDoc() && view->GODoc->GetNumMriDoc() ) {
                        index[j++]  = i;
                        pickl.AddString ( (*tpllist)->GetViewName() );
                        }
                } // for

            pickl.Execute();
            if ( pickl.GetResult() < 0 )
                return  pickl.GetResult();
            else
                return  index[ pickl.GetResult() ] + 1;
            }
        }
    }

return  TDocManager::SelectViewType ( tpllist, tplcount );
}


//----------------------------------------------------------------------------
void    TCartoolDocManager::CmViewCreate ()
{
//TDocManager::CmViewCreate ();

TDocument          *doc             = GetCurrentDoc ();
if ( ! doc )        return;

TBaseView          *view            = (TBaseView*) CreateAnyView ( *doc );
if ( ! view )       return;

//TBaseView *view   = GetCurrentView ();

                                        // if belongs to a group, create the same view to the other eeg
if ( view->GODoc ) {
    TLinkManyDoc*       god             = view->GODoc;
    TBaseDoc*           doc             = GetCurrentBaseDoc ();
    TTracksDoc*         doceeg;
    TRisDoc*            docris;
    TBaseView*          v;


    if ( typeid ( *view ) == typeid ( TTracksView ) ) {
        for ( int i=0; i < god->GetNumEegDoc(); i++ ) {
            doceeg  = god->GetEegDoc (i);
            if ( doc == doceeg )    continue;   // already created

            PostEvent ( dnCreate, *(new TTracksView (*doceeg, 0, god ) ) );
            }
        }

    else if ( typeid ( *view ) == typeid ( TPotentialsView ) ) {
        for ( int i=0; i < god->GetNumEegDoc(); i++ ) {
            doceeg  = god->GetEegDoc (i);
            if ( doc == doceeg )    continue;   // already created

            god->LastEegViewId  = 0;
            for ( v=doceeg->GetViewList(god); v != 0; v=doceeg->NextView (v, god) )
                if ( typeid (*v) == typeid ( TTracksView ) )
                    god->LastEegViewId  = v->GetViewId();

            PostEvent ( dnCreate, *(new TPotentialsView (*doceeg, 0, god ) ) );
            }
        }

    else if ( typeid ( *view ) == typeid ( TInverseView ) ) {

        for ( int i=0; i < god->GetNumEegDoc(); i++ ) {
            doceeg  = god->GetEegDoc (i);
            if ( doc == doceeg )    continue;

            god->LastEegViewId  = 0;
            for ( v=doceeg->GetViewList(god); v != 0; v=doceeg->NextView (v, god) )
                if ( typeid (*v) == typeid ( TTracksView ) )
                    god->LastEegViewId  = v->GetViewId();

            PostEvent ( dnCreate, *(new TInverseView (*doceeg, 0, god) ) );
            }

        for ( int i=0; i < god->GetNumRisDoc(); i++ ) {
            docris  = god->GetRisDoc (i);
            if ( doc == docris )    continue;

            god->LastEegViewId  = 0;
            for ( v=docris->GetViewList(god); v != 0; v=docris->NextView (v, god) )
                if ( typeid (*v) == typeid ( TTracksView ) )
                    god->LastEegViewId  = v->GetViewId();

            PostEvent ( dnCreate, *(new TInverseView (*docris, 0, god) ) );
            }
        }
                                        // in any case, do a nice insertion
    god->GroupTileViews ( CombineFlags ( GroupTilingViews_Resize, GroupTilingViews_Insert ) );
    }

                                        // Forcing the focus to activate buttons and stuff
view->SetFocus();
}


//----------------------------------------------------------------------------
                                        // retrieve existing opening command for an extension
/*char        *TCartoolDocManager::GetOpenCommand ( char *extension, char *command )
{
char                buff2 [ 4096 ];

StringCopy      ( buff2, extension, "file\\Shell\\Open\\command" );

QueryDefValue   ( TRegKey::GetClassesRoot (), buff2, command );

//DBGM ( command, extension );

return  command;
}
*/

//----------------------------------------------------------------------------
TBaseDoc*   TCartoolDocManager::OpenDoc ( const char* path, long flags )
{
                                        // process file name copy
TFileName           pathok ( path, (TFilenameFlags) ( TFilenameMsDosToWindows | TFilenameExtendedPath | TFilenameSibling ) );

//StringCopy ( path, pathok );          // not allowed anymore - do we really need to propagate the potentially updated path back?


TDocTemplate       *tpl             = MatchTemplate ( pathok );

if ( tpl == 0 ) {
                                        // unknown file type, try this and get out
    OpenUnknownFile ( pathok );

    return  0;
    }

                                        // here is a legal file extension

TBaseDoc           *doc             = IsOpen        ( pathok ); // if already open, return a pointer to it


bool                oldav           = CartoolApplication->AnimateViews;
CartoolApplication->AnimateViews    =    CartoolApplication->AnimateViews       // might have been reset elsewhere
                                      && CartoolApplication->IsInteractive ()   // must be interactive
                                      && NumDocOpen () < AnimationMaxDocOpen    // and not already crowded
                                      && ! IsExtension ( path, FILEEXT_LM);     // and not dropping a lm file either


if ( doc == 0 )                         // if not already open, open it

    doc     = CreateDoc ( tpl, pathok, 0, flags );


CartoolApplication->AnimateViews    = oldav;


if ( doc == 0 ) {                       // problem if still 0
    StringPrepend   ( pathok, "Unable to open document:" NewLine NewLine );
    ShowMessage     ( pathok, "Opening Error", ShowMessageWarning );
    return  0;
    }


return  doc;
}


//----------------------------------------------------------------------------
                                        // Optionally offering to open an unknown file with its default viewer
void    TCartoolDocManager::OpenUnknownFile ( const char *path )
{
//if ( IsDirectory ( path ) )
//    return;


if ( IsExtensionAmong ( path, FILEEXT_VRB )
  || GetAnswerFromUser ( "I can't seem to be able to open this file." NewLine "Should I try the default Windows open instead?", path ) )

    ShellExecute ( NULL, "open", path, NULL, NULL, SW_SHOWNORMAL);
}


//----------------------------------------------------------------------------
                                        // Disambiguating files with identical extensions
TDocTemplate*   TCartoolDocManager::MatchTemplate ( const char *path )
{
if ( IsDirectory ( path ) )
    return  0;

                                        // get first template available
TDocTemplate*       tpl             = TDocManager::MatchTemplate ( path );


if ( tpl == 0 )                         // no template -> abort
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // disambiguate .eeg files
                                        // see also ReadFromHeader
if      ( StringIs ( tpl->GetDefaultExt (), FILEEXT_BIOEEG ) ) {

    if      ( DisambiguateExtEEG ( path, Biologic ) )
        ;                               // this is the first template, no action needed


    else if ( DisambiguateExtEEG ( path, BrainVision ) ) {
                                        // step to second next non-hidden template
//      int     count = 0;

        for ( tpl = tpl->GetNextTemplate (); tpl != 0; tpl = tpl->GetNextTemplate () )

            if ( ! tpl->IsFlagSet ( dtHidden ) ) {
                                        // step to next non-hidden template
                break;
//                                      // step to second-to-next non-hidden template
//              count++;
//
//              if ( count == 2 )
//                  break;
                }
        } // if brainvision

    else {                              // Houston, we have a problem
        ShowMessage ( "I did my best, but I can not find" NewLine "or can not open this file...", "Opening Error", ShowMessageWarning );
        return  0;
        }

    } // disambiguate FILEEXT_BIOEEG


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Disambiguate between Analyze7.5 and Nifit1.0
else if ( StringIs ( tpl->GetDefaultExt (), FILEEXT_MRIAVW_HDR ) ) {

    AnalyzeFamilyType   type            = GetAnalyzeType ( path );


    for ( ; tpl != 0; tpl = tpl->GetNextTemplate () ) {

        if ( tpl->IsFlagSet ( dtHidden ) )  continue;

        if      (    IsAnalyze75 ( type ) 
                  && dynamic_cast< TemplMriAnalyzeVolumeView* > ( tpl ) )
            return  tpl;

        else if (    IsNifti ( type )       // silly case with files badly renamed .nii.hdr - gracefully redirect them anyway
                /*&& ( type & DualFiles )*/ // either single or dual, we really cool
                  && dynamic_cast< TemplMriNiftiVolumeView* > ( tpl ) )
            return  tpl;
        }

    ShowMessage ( "I did my best, but I can not find" NewLine "or can not open this file...", "Opening Error", ShowMessageWarning );
    return  0;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


return  tpl;
}


//----------------------------------------------------------------------------
TBaseDoc*   TCartoolDocManager::CreateDoc ( TDocTemplate *tpl, char *path, TDocument *parent, long flags )
{
if ( tpl == 0 ) {                       // unknown file type?

//  OpenUnknownFile ( path );

    return  0;
    }

                                        // !not working!
//CartoolApplication->DestroySplashScreen ();

                                        // process file name copy
TFileName           pathok ( path, (TFilenameFlags) ( TFilenameMsDosToWindows | TFilenameExtendedPath | TFilenameSibling ) );
                                        // a priori we own the path
StringCopy ( path, pathok );


if ( ! CanOpenFile ( pathok ) 
  && ! ( StringEndsWith ( pathok, EmptyLmFilename     ) 
      || StringEndsWith ( pathok, AllOpenedLmFilename ) ) )

    return 0;


TBaseDoc*           doc             = IsOpen ( pathok );

                                        // already open, avoid duplicate. Also check for existing views
if ( doc != 0 ) {

//  ShowMessage ( string(*CartoolApplication, IDS_DUPLICATEDOC).c_str(), doc->GetTitle() );

    if ( doc->GetViewList () )
        doc->GetViewList ()->GetParentO ()->SetFocus ();

    return doc;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // overload flag to avoid opening view if not interactive
if ( CartoolApplication->IsNotInteractive () )
    
    SetFlags ( flags, (long) owl::dtNoAutoView );


bool                oldav           = CartoolApplication->AnimateViews;
CartoolApplication->AnimateViews    =    CartoolApplication->AnimateViews       // might have been reset elsewhere
                                      && CartoolApplication->IsInteractive ()   // must be interactive
                                      && NumDocOpen () < AnimationMaxDocOpen    // and not already crowded
                                      && ! IsExtension ( path, FILEEXT_LM);     // and not dropping a lm file either


doc     = dynamic_cast<TBaseDoc*> ( TDocManager::CreateDoc ( tpl, pathok, parent, flags ) );


CartoolApplication->AnimateViews    = oldav;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//UpdateApplication;

return doc;
}


//----------------------------------------------------------------------------
void    TCartoolDocManager::CloseDoc ( TBaseDoc *doc, bool /*refresh*/ )
{
if ( doc == 0 )
    return;


if ( doc && doc->CanClose () )  // normally calls back to FlushDoc()

    if ( ! doc->Close () )
        PostDocError ( *doc, IDS_UNABLECLOSE );
    else
        delete doc;


//if ( refresh )
//    UpdateApplication;
}


void    TCartoolDocManager::CloseDoc ( const char *filename, bool refresh )
{
if ( StringIsEmpty ( filename ) )
    return;


TBaseDoc           *doc             = IsOpen ( filename );

if ( doc != 0 )

    CloseDoc ( doc, refresh );
}


//----------------------------------------------------------------------------
                                        // Moved from header because compiler is whining it doesn't really know TBaseDoc afterall
TBaseDoc*       TCartoolDocManager::DocListNext         ( owl::TDocument *doc ) { return dynamic_cast<TBaseDoc*> ( TDocManager::DocList.Next ( doc ) ); }
TBaseDoc*       TCartoolDocManager::GetCurrentBaseDoc   ()                      { return dynamic_cast<TBaseDoc*> ( TDocManager::GetCurrentDoc () );     }


//----------------------------------------------------------------------------
void    TCartoolDocManager::CloseView ( TView *view )
{
if ( view == 0 )
    return;

PostEvent ( dnClose, *view );

//UpdateApplication;
}


TBaseView*  TCartoolDocManager::GetView ( UINT viewid )
{
                                        // Search is done on basic TDocument / TView objects, not our own TBaseDoc / TBaseView
                                        // so that the loops will not stop if one view is not derived from TBaseView
for ( TDocument* doc = DocList.Next ( 0 ); doc != 0; doc = DocList.Next ( doc ) )
for ( TView* v = doc->GetViewList (); v != 0; v = doc->NextView ( v ) )

    if ( v->GetViewId () == viewid )

        return  dynamic_cast<TBaseView*> ( v );

return  0;
}


void    TCartoolDocManager::CloseViews ( TDocument *doc, TLinkManyDoc* god )
{
//DBGM ( "", "Close Views" );

if ( doc == 0 || doc->GetViewList () == 0 )
    return;


TView*          view;
TView*          nextview;
TBaseView*      baseview;

                                        // be careful with the linked list of pointers!
for ( view = doc->GetViewList(), nextview = doc->NextView ( view ); view != 0; view = nextview, nextview = doc->NextView ( view ) ) {

    baseview = dynamic_cast<TBaseView *> ( view );
                                        // if optional group provided, skip if this view is not belonging to it
    if ( baseview && god && baseview->GODoc != god )
        continue;

    if ( baseview )
        baseview->Destroy ();           // better
    else
        delete view;                    // default
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
