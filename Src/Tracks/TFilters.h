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
                                        // TFilter and TFilters definition
#include    "TFilters.Base.h"
                                        // Temporal filters
#include    "TFilters.Butterworth.h"
#include    "TFilters.Baseline.h"
#include    "TFilters.Envelope.h"
                                        // Non-temporal filters
#include    "TFilters.Threshold.h"
#include    "TFilters.Rectification.h"
#include    "TFilters.Ranking.h"
#include    "TFilters.Spatial.h"
