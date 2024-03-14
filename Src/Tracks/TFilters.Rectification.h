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
                                        // Non-temporal filter
//----------------------------------------------------------------------------

enum            FilterRectificationdEnum
                {
                FilterRectifyAbs,
                FilterRectifySquare,
                };


template <class TypeD>
class   TFilterRectification    : public TFilter<TypeD>
{
public:
                    TFilterRectification ();
                    TFilterRectification ( FilterRectificationdEnum how );


    void            Reset   ();
    void            Set     ( FilterRectificationdEnum how );

    void            Apply                   ( TypeD* data, int numpts );


                            TFilterRectification    ( const TFilterRectification& op  );
    TFilterRectification&   operator    =           ( const TFilterRectification& op2 );


protected:

    FilterRectificationdEnum    How;

};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------

template <class TypeD>
        TFilterRectification<TypeD>::TFilterRectification ()
      : TFilter<TypeD> ()
{
Reset ();
}


template <class TypeD>
        TFilterRectification<TypeD>::TFilterRectification ( FilterRectificationdEnum how )
{
Set ( how );
}


template <class TypeD>
void    TFilterRectification<TypeD>::Reset ()
{
How                 = FilterRectifyAbs;
}


template <class TypeD>
            TFilterRectification<TypeD>::TFilterRectification ( const TFilterRectification& op )
{
How                 = op.How;
}


template <class TypeD>
TFilterRectification<TypeD>& TFilterRectification<TypeD>::operator= ( const TFilterRectification& op2 )
{
if ( &op2 == this )
    return  *this;

How                 = op2.How;

return  *this;
}


template <class TypeD>
void    TFilterRectification<TypeD>::Set ( FilterRectificationdEnum how )
{
Reset ();

How                 = how;
}


template <class TypeD>
void    TFilterRectification<TypeD>::Apply ( TypeD* data, int numpts )
{
for ( int tf = 0; tf < numpts; tf++ )
                                        // filter action is removal
    if      ( How == FilterRectifyAbs       )   data[ tf ]  = fabs   ( data[ tf ] );
    else/*if( How == FilterRectifySquare    )*/ data[ tf ]  = Square ( data[ tf ] );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
