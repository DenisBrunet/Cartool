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

#include    "TMicroStates.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Existing labeling will be clipped out
                                        // very close to CentroidsToLabeling, but without the loop through all maps
int     TMicroStates::RejectLowCorrelation  (   long            tfmin,      long            tfmax,
                                                TMaps&          maps,       TLabeling&      labels,
                                                double          limitcorr 
                                            )
{
//if ( maps.IsNotAllocated () )     return;


OmpParallelFor

for ( long tf = tfmin; tf <= tfmax; tf++ ) {

    if ( labels.IsUndefined ( tf ) )
        continue;
    
                                         // maps are already centered and normalized
    double          corr        = Project ( maps[ labels[ tf ] ], Data[ tf ], labels.GetPolarity ( tf ) );
                                        // !strict test, so no labeling get reset with a limit of -1!
    if ( corr < limitcorr )
        labels.Reset ( tf );

    } // tf


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // pack and get exact number of final maps
return  labels.PackLabels ( maps );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
