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

#include    "Files.Extensions.h"
#include    "Files.Utils.h"
#include    "Files.ReadFromHeader.h"

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
#include    "TFreqCartoolDoc.h"
#include    "TSpiDoc.h"
#include    "TSxyzDoc.h"
#include    "TLocDoc.h"
#include    "TXyzDoc.h"
#include    "TElsDoc.h"
#include    "TMatrixIsDoc.h"
#include    "TMatrixSpinvDoc.h"
#include    "TRoisDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Centralizing and dispatching ReadFromHeader for all documents, calling the right one according to the file type
                                        // Each specialized version aims at being the fastest as possible, f.ex. by NOT loading the actual data, but just the header (if it exists)
bool    ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
if ( ! CanOpenFile ( file ) )

    return false;

                                        // systematically testing all extensions one by one, then calling corresponding static ReadFromHeader methods
if      ( IsExtension ( file, FILEEXT_XYZ       ) )     return TXyzDoc              ::ReadFromHeader ( file, what, answer );
else if ( IsExtension ( file, FILEEXT_ELS       ) )     return TElsDoc              ::ReadFromHeader ( file, what, answer );

else if ( IsExtension ( file, FILEEXT_EEGEPH    )
       || IsExtension ( file, FILEEXT_EEGEPSD   )
       || IsExtension ( file, FILEEXT_EEGEPSE   )
       || IsExtension ( file, FILEEXT_EEGEP     ) )     return TEegCartoolEpDoc     ::ReadFromHeader ( file, what, answer );
else if ( IsExtension ( file, FILEEXT_EEGD      ) )     return TEegMIDDoc           ::ReadFromHeader ( file, what, answer );
else if ( IsExtension ( file, FILEEXT_EEG128    ) )     return TEegBIDMC128Doc      ::ReadFromHeader ( file, what, answer );
else if ( IsExtension ( file, FILEEXT_EEGBVDAT  ) )     return TEegBrainVisionDoc   ::ReadFromHeader ( file, what, answer );
else if ( IsExtension ( file, FILEEXT_EEGNSRRAW ) )     return TEegEgiRawDoc        ::ReadFromHeader ( file, what, answer );
else if ( IsExtension ( file, FILEEXT_EEGNSR    ) )     return TEegEgiNsrDoc        ::ReadFromHeader ( file, what, answer );    // TTracksDoc::ReadFromHeader
else if ( IsExtension ( file, FILEEXT_EEGMFF    ) )     return TEegEgiMffDoc        ::ReadFromHeader ( file, what, answer );    // TTracksDoc::ReadFromHeader
else if ( IsExtension ( file, FILEEXT_EEGNSCNT  ) )     return TEegNeuroscanCntDoc  ::ReadFromHeader ( file, what, answer );
else if ( IsExtension ( file, FILEEXT_EEGNSAVG  ) )     return TEegNeuroscanAvgDoc  ::ReadFromHeader ( file, what, answer );
else if ( IsExtension ( file, FILEEXT_EEGSEF    ) )     return TEegCartoolSefDoc    ::ReadFromHeader ( file, what, answer );
else if ( IsExtension ( file, FILEEXT_EEGBDF    )
       || IsExtension ( file, FILEEXT_EEGEDF    ) )     return TEegBiosemiBdfDoc    ::ReadFromHeader ( file, what, answer );
else if ( IsExtension ( file, FILEEXT_EEGTRC    ) )     return TEegMicromedTrcDoc   ::ReadFromHeader ( file, what, answer );
else if ( IsExtension ( file, FILEEXT_EEGRDF    ) )     return TEegERPSSRdfDoc      ::ReadFromHeader ( file, what, answer );
else if ( IsExtension ( file, FILEEXT_DATA      )
       || IsExtension ( file, FILEEXT_SEG       ) )     return TSegDoc              ::ReadFromHeader ( file, what, answer );
else if ( IsExtension  ( file, FILEEXT_EEGBIO ) ) { // == FILEEXT_EEGBV
    if      ( DisambiguateExtEEG ( file, Biologic   ) ) return TEegBioLogicDoc      ::ReadFromHeader ( file, what, answer );    // see also: TCartoolDocManager::MatchTemplate
    else if ( DisambiguateExtEEG ( file, BrainVision) ) return TEegBrainVisionDoc   ::ReadFromHeader ( file, what, answer );
    else                                                return false;
    }

else if ( IsExtension ( file, FILEEXT_IS        ) )     return TMatrixIsDoc         ::ReadFromHeader ( file, what, answer );
else if ( IsExtension ( file, FILEEXT_RIS       ) )     return TRisDoc              ::ReadFromHeader ( file, what, answer );

else if ( IsExtension ( file, FILEEXT_SPINV     ) )     return TMatrixSpinvDoc      ::ReadFromHeader ( file, what, answer );
else if ( IsExtension ( file, FILEEXT_SPIRR     ) )     return TSpiDoc              ::ReadFromHeader ( file, what, answer );
else if ( IsExtension ( file, FILEEXT_LOC       ) )     return TLocDoc              ::ReadFromHeader ( file, what, answer );
else if ( IsExtension ( file, FILEEXT_SXYZ      ) )     return TSxyzDoc             ::ReadFromHeader ( file, what, answer );

else if ( IsExtension ( file, FILEEXT_FREQ )    )       return TFreqCartoolDoc      ::ReadFromHeader ( file, what, answer );

else if ( IsExtension ( file, FILEEXT_ROIS      ) )     return TRoisDoc             ::ReadFromHeader ( file, what, answer );

else if ( IsExtension  ( file, FILEEXT_MRK ) ) {

    ifstream        is ( file, ios::binary );

    switch ( what ) {
        case ReadMagicNumber :
            is.read ( (char*) answer, sizeof ( int ) );
            return  IsMagicNumber ( (const char*) answer, TLBIN_MAGICNUMBER2 );
        }
    }


#if defined (_DEBUG)
ShowMessage ( "File type not recognized!", ToFileName ( file ), ShowMessageWarning );
#endif

return  false;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
