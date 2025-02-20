/************************************************************************\
� 2024-2025 Denis Brunet, University of Geneva, Switzerland.

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

#include    <iostream>

#include    "TCartoolApp.h"
crtl::TCartoolApp   app ( crtl::CartoolTitle, 0, 0, owl::Module, 0 ); // We need a (minimal) Cartool app object properly initialized


void    ExampleTEasyStats   ();
void    ExampleTTracks      ();
void    ExampleTMaps        ();
void    ExampleTTracksDoc   ();
void    ExampleSegmentation ();
void    ExampleBackFitting  ();


int     main ()
{
ExampleTEasyStats   ();
ExampleTTracks      ();
ExampleTMaps        ();
ExampleTTracksDoc   ();
ExampleSegmentation ();
ExampleBackFitting  ();
}
