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

enum    ComputingRisPresetsEnum;
enum    GroupsLayoutEnum;
enum    RegularizationType;
enum    BackgroundNormalization;
enum    AtomType;
enum    CentroidType;
enum    SpatialFilterType;
enum    FilterTypes;
class   TGoGoF;

bool    ComputingRis    (   ComputingRisPresetsEnum esicase,
                            const TGoGoF&       gogof,                  
                            GroupsLayoutEnum    grouplayout,            int                 numsubjects,        int             numconditions,
                            
                            const char*         InverseFile,            RegularizationType  regularization,     BackgroundNormalization     backnorm,
                            AtomType            datatypeepochs,         AtomType            datatypefinal,
                            CentroidType        centroidsmethod,

                            SpatialFilterType   spatialfilter,          const char*         xyzfile,
                            bool                ranking,
                            bool                thresholding,           double              keepingtopdata,
                            bool                envelope,               FilterTypes         envelopetype,       double          envelopelowfreq,    double          envelopeduration,
                            bool                roiing,                 const char*         roifile,            FilterTypes     roimethod,

                            bool                savingindividualfiles,  bool                savingepochfiles,   bool            savingzscorefactors,
                            bool                computegroupsaverages,  bool                computegroupscentroids,
                            const char*         basedir,                const char*         basefilename
                        );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
