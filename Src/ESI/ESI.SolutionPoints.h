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

enum                    GreyMatterFlags;
enum                    NeighborhoodType;
class                   TVolumeDoc;
class                   TPoints;
class                   TStrings;
class                   TSelection;
template <class> class  TArray1;


bool            SegmentGreyMatter       ( const TVolumeDoc* mridoc, GreyMatterFlags greyflags, const char* greyfile );

void            ScanForOutsidePoints    ( const TPoints& referencepoints, const TPoints& checkedpoints, TSelection& spsrejected );

int             RejectSingleNeighbors   ( const TPoints& pointsin, NeighborhoodType neighborhood, TSelection& spsrejected );

void            RejectPointsFromList    ( TPoints& points, TStrings&    names, const TSelection& spsrejected );

void            GetNumGreyNeighbors     ( const TPoints& points, double neighborradius, TArray1<int>& NumNeighbors, const TSelection* spsrejected = 0 );

bool            DownsampleSolutionPoints( const TVolumeDoc* mribraindoc, const TPoints& solpointsoriginal, double neighborradius, int numsolpointswished, TSelection& rejectsel );


//----------------------------------------------------------------------------
                                        // Wrapping everything up to compute the solution points
bool            ComputeSolutionPoints   (   
                            const TVolumeDoc*   mribraindoc,        const TVolumeDoc*   mrigreydoc, 
                            int                 numsolpointswished, double              resolutionwished,
                            GreyMatterFlags     spflags,
                            NeighborhoodType    loretaneighborhood, NeighborhoodType    lauraneighborhood, 
                            TPoints&            solpoints,          TStrings&           spnames,
                            const char*         filesp
                            );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
