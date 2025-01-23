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

#include    "Math.Armadillo.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Math functions

enum            PcaIcaType
                {
                UnknownPcaProcessing,
                PcaProcessing,
                IcaProcessing,

                NumPcaIcaProcessing,
                };

extern const char   PcaIcaTypeString[ NumPcaIcaProcessing ][ 32 ];


enum            {
                gaugepcamain,
                gaugepca,
                };

                    
//----------------------------------------------------------------------------

enum            PcaResultsType
                {
                           Vt_X,
                  InvSqrtD_Vt_X,
                V_InvSqrtD_Vt_X,

                PcaOnly             =            Vt_X,  // just applying the transform into the orthogonal space (through Vt)
                PcaWhitened         =   InvSqrtD_Vt_X,  // same as above + rescaling components by variance
//              PcaWhitenedBack     = V_InvSqrtD_Vt_X,  // same as above + final transform back to original space
                };


constexpr int   PCANumGauge         = 6;


enum                AtomType;
enum                ReferenceType;
                    class   TMaps;
                    class   TSuperGauge;
                    class   TSelection;
template <class>    class   TVector;


bool            PCA (   TMaps&              data,           
                        bool                covrobust,
                        bool                removelasteigen,
                        PcaResultsType      pcaresults,
                        TMaps&              eigenvectors,       TVector<float>&         eigenvalues,
                        AMatrix&            topca,              AMatrix&                towhite,
                        TMaps&              pcadata,        
                        TSuperGauge*        gauge
                    );


//----------------------------------------------------------------------------
                                        // All the different covariances implementations
bool            Covariance                  (   TMaps&              data,           
                                                ASymmetricMatrix&   Cov
                                            );

bool            RobustCovarianceSingleDim   (   TMaps&              data,           
                                                double              maxsd,
                                                ASymmetricMatrix&   Cov 
                                            );

bool            RobustCovarianceMahalanobis (   TMaps&              data,           
                                                TSelection&         skiptracks,
                                                double              maxsd,  // the max Mahalanobis distance for Covariance
                                                ASymmetricMatrix&   Cov
                                            );


//void            SubtractAverage         ( TArray2<float> &data, int axis, int maxdim, int maxsamples );


//----------------------------------------------------------------------------

#define         PCAFileNumGauge     6

                                        // Whole PCA procedure on a single file - No UI
void    PcaIcaFile (    char*               filename,
                        AtomType            datatype,
                        ReferenceType       dataref,
                        bool                datanormalized,
                        bool                covrobust,
                        int                 cliptf,
                        PcaIcaType          processing,
                        bool                savematrix,     bool            savetopo,           bool            saveeigenvalues,     
                        bool                savepcatracks,  bool            savepcapoints,     
                        const char*         prefix,
                        TSuperGauge*        gauge       = 0
                 );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
