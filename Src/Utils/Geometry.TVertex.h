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

#include    "MemUtil.h"
#include    "Geometry.TPoint.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Vertex = location + normal
class   TVertex
{
public:

    TPointFloat     Vertex;
    TVector3Float   Normal;


    double          Norm2     ()                                { return Normal.Norm2 (); }
    double          Norm      ()                                { return Normal.Norm (); }
    void            Normalize ()                                { Normal.Normalize (); }

    bool            SamePosition ( const TVertex* vn2 ) const   { return Vertex == vn2->Vertex; }


    void*           operator new    []  ( size_t numbytes )     {   return  GetVirtualMemory ( numbytes );  }   // direct call to GetVirtualMemory, using TMemory would add too many variables for a single vertex - otherwise, do a class like ArrayVertices
    void            operator delete []  ( void *block )         {   FreeVirtualMemory ( block );            }

};

                                        // vertex + normal + index
class   TVertexI :  public TVertex,
                    public TIndex
{
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
