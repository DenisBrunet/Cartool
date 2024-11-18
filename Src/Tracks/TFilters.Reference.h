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

#include    "TMaps.h"
#include    "CartoolTypes.h"

namespace crtl {

//----------------------------------------------------------------------------
                                        // Non-temporal filter
//----------------------------------------------------------------------------

template <class TypeD>
class   TFilterReference    : public TFilter<TypeD>
{
public:
                    TFilterReference        ();
                    TFilterReference        ( ReferenceType reference, const TSelection& referencetracks );


    void            Reset                   ();
    void            Set                     ( ReferenceType reference, const TSelection& referencetracks );

    inline void     Apply                   ( TypeD* data, int dim, ReferenceType reference = ReferenceUsingCurrent, const TSelection* referencetracks = 0, const TSelection* validtracks = 0, const TSelection* auxtracks = 0 );
    inline void     Apply                   ( TVector<TypeD>& v,    ReferenceType reference = ReferenceUsingCurrent, const TSelection* referencetracks = 0, const TSelection* validtracks = 0, const TSelection* auxtracks = 0 );


                        TFilterReference    ( const TFilterReference& op  );
    TFilterReference&   operator    =       ( const TFilterReference& op2 );


protected:

    ReferenceType   Reference;
    TSelection      ReferenceTracks;

};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------

template <class TypeD>
        TFilterReference<TypeD>::TFilterReference ()
      : TFilter<TypeD> ()
{
Reset ();
}


template <class TypeD>
        TFilterReference<TypeD>::TFilterReference ( ReferenceType reference, const TSelection& referencetracks )
{
Set ( reference, referencetracks );
}


template <class TypeD>
void    TFilterReference<TypeD>::Reset ()
{
Reference           = ReferenceNone;
ReferenceTracks.Reset ();   // just reset, keeping the allocation
}


template <class TypeD>
            TFilterReference<TypeD>::TFilterReference ( const TFilterReference& op )
{
Reference           = op.Reference;
ReferenceTracks     = op.ReferenceTracks;
}


template <class TypeD>
TFilterReference<TypeD>& TFilterReference<TypeD>::operator= ( const TFilterReference& op2 )
{
if ( &op2 == this )
    return  *this;


Reference           = op2.Reference;
ReferenceTracks     = op2.ReferenceTracks;


return  *this;
}


template <class TypeD>
void    TFilterReference<TypeD>::Set ( ReferenceType reference, const TSelection& referencetracks )
{
Reset ();

Reference           = reference;
ReferenceTracks     = referencetracks;
}


//----------------------------------------------------------------------------
                                        // validtracks and auxtracks must have sizes >= dim; referencetracks could be whatever size, though
template <class TypeD>
void    TFilterReference<TypeD>::Apply ( TypeD* data, int dim, ReferenceType reference, const TSelection* referencetracks, const TSelection* validtracks, const TSelection* auxtracks )
{
if ( data == 0 || dim == 0 )

    return;

                                        // resolving this case
if ( reference == ReferenceUsingCurrent ) {

    reference       =  Reference;       // using our current state
    referencetracks = &ReferenceTracks;
    }


if ( reference == UnknownReference          // what?
  || reference == ReferenceAsInFile         // actually nothing to do
  || reference == ReferenceAverage         && (   validtracks     && validtracks    ->IsNoneSet () ) // validtracks can be missing
  || reference == ReferenceArbitraryTracks && ( ! referencetracks || referencetracks->IsNoneSet () ) // referencetracks should be present
  || reference == ReferenceUsingCurrent     // not an option here
   )

    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double              refvalue        = 0;


if      ( reference == ReferenceAverage ) {
    
    if      ( validtracks ) {
                                        // validtracks is not empty
        for ( int i = 0; i < dim; i++ )
            if ( (*validtracks)[ i ] )  // using only non-bad, non-aux and non-pseudos channels
                refvalue   += data[ i ];

        refvalue   /= validtracks->NumSet ();
        }

    else if ( auxtracks ) {
                                        // validtracks is empty, but we have auxtracks
        for ( int i = 0; i < dim; i++ )
            if ( ! (*auxtracks)[ i ] )  // using only non-aux channels
                refvalue   += data[ i ];

        refvalue   /= dim - auxtracks->NumSet ();
        }

    else {
                                        // using all channels
        for ( int i = 0; i < dim; i++ )
            refvalue   += data[ i ];

        refvalue   /= dim;
        }
    } // ReferenceAverage


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( reference == ReferenceArbitraryTracks ) {
                                        // referencetracks must be provided
    int             numset          = referencetracks->NumSet ();

                                        // Single reference - skip the loop
    if ( numset == 1 ) {

        refvalue    = data[ referencetracks->GetValue ( 0 ) ];
        }

    else { // NumSet > 1
    
        for ( int seli = 0; seli < numset; seli++ )
            refvalue   += data[ referencetracks->GetValue ( seli ) ];

        refvalue   /= numset;
        }        

    } // ReferenceArbitraryTracks


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // finally subtract the reference
if ( auxtracks ) {
    
    for ( int i = 0; i < dim; i++ )
        if ( ! (*auxtracks)[ i ] )      // ref applied to good and bad channels, but not on auxiliary channels
            data[ i ] -= refvalue;
    }
else {
    
    for ( int i = 0; i < dim; i++ )
        data[ i ] -= refvalue;
    }
}


//----------------------------------------------------------------------------
template <class TypeD>
void    TFilterReference<TypeD>::Apply ( TVector<TypeD>& v, ReferenceType reference, const TSelection* referencetracks, const TSelection* validtracks, const TSelection* auxtracks )
{
Apply ( (TypeD*) v, v.GetDim (), reference, referencetracks, validtracks, auxtracks );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
