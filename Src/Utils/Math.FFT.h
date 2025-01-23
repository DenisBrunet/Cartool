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

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Handy function that returns the max legal frequency
                                        // It handles the S-Transform, which has a lower max frequency than a regular FFT
                                        // This formula works for both Hz or relative integer frequency bins
                                        // (Note that this is the formula I end up to with, the article is not very precise on that part...)
inline  double  GetNyquist              ( double samplingfrequency, bool /*stransform*/ )
{
return  samplingfrequency / 2;

                  // truncation is to make it less weird for these big windows
//return  stransform ? TruncateTo ( samplingfrequency / 2 * ( M_PI / ( M_PI + 1 ) ), 0.5 )
//                   :              samplingfrequency / 2;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}







