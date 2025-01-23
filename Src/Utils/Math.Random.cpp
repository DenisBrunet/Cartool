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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "System.h"
#include    "Time.Utils.h"
#include    "Math.Random.h"

using namespace std;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace crtl {

//----------------------------------------------------------------------------
                                        // Mersenne Twister 32 bits
        TRand::TRand ( UINT seed )
{
Reload ( seed );
}
                                        // When seed is provided, it allows to generate exactly the same pseudo-random numbers
                                        // Otherwise, it uses some random device as seed
void    TRand::Reload ( UINT seed )
{
Mersene32.seed ( seed ? seed : GetRandomDevice () );
}
                                        // We build our our own "random device":
                                        //  - uses current thread Id, so multiple threads will have different seeds
                                        //  - uses current time at the micro-second resolution
                                        //  - uses current object's address, again useful for multi-thread
UINT    TRand::GetRandomDevice ( UINT additionalseed )
{
//                                        // Real random device from std
//random_device       RandomDevice;
//return  RandomDevice ();                // random_device is opaque and can go through a file, so it is über-slow

return  (UINT)  (   GetThreadId () * 7919                // thread count times a prime number big enough that a series of threads will have significantly different values
                  + GetLowTicks ()                       // only the lowest part of the tick count is really useful for us
                  + LOULONG ( this ) + HIULONG ( this )  // hashing current object address
                  + additionalseed                       // whatever caller thinks is useful - usually ignored except for real Monte-Carlo types of processing where it can add an extra "kick"
                );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
