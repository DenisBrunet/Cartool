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

constexpr char*     CentroidTitle           = "Centroids Computation";


enum        AtomType;
enum        ReferenceType;
enum        CentroidType;
enum        PolarityType;
enum        SpatialFilterType;
class       TMaps;
class       TGoF;


enum        ComputeCentroidEnum {
            OneFileOneCentroid,     // typical case: each file is basically 1 condition & 1 subject
            AllFilesOneCentroid,    // typical case: files are epochs for 1 condition & 1 subject
            };


void    ProcessResults  (   TMaps&      data,           AtomType        datatype,       ReferenceType   reference,
                            bool        ranking, 
                            bool        thresholding,   double          threshold, 
                            bool        normalize 
                        );

void    ProcessResults  (   TGoF&       gof,            AtomType        datatype,       ReferenceType   reference,
                            bool        ranking, 
                            bool        thresholding,   double          threshold, 
                            bool        normalize 
                        );


void    ComputeCentroidFiles    (   const TGoF&         gof,                ComputeCentroidEnum     layout,
                                    CentroidType        centroidflag,
                                    AtomType            datatype,
                                    PolarityType        polarity,
                                    ReferenceType       processingref,
//                                  bool                toabszscore,
                                    bool                ranking,
                                    bool                thresholding,       double                  threshold,
                                    bool                normalize,
                                    SpatialFilterType   spatialfilter,      const char*             xyzfile,
                                    TMaps&              mapscentroid,
                                    bool                showprogress    = false
                                );


void    ComputeCentroidFiles    (   const TGoF&         gof,                ComputeCentroidEnum     layout,
                                    CentroidType        centroidflag,
                                    AtomType            datatype,
                                    PolarityType        polarity,
                                    ReferenceType       processingref,
//                                  bool                toabszscore,
                                    bool                ranking,
                                    bool                thresholding,       double                  threshold,
                                    bool                normalize,
                                    SpatialFilterType   spatialfilter,      const char*             xyzfile,
                                    char*               centrfile,
                                    bool                showprogress    = false
                                );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
