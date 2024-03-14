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

#include    <owl/edit.h>

#include    "Math.Utils.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Class to handle downsampling of some collection / array
class   TDownsampling
{
public:
                    TDownsampling ();
                    TDownsampling ( int numdata, int numwishedata );
                    TDownsampling ( int from, int to, int numwishedata );


    int             From;               // optional from
    int             To;                 // and optional to
    int             NumData;            // actual number of data points
    int             NumDownData;        // the number after caller wish..
    int             Step;               // ..and the corresponding step


    void            Reset   ();
    void            Set     ( int numdata, int numwishedata );
    void            Set     ( int from, int to, int numwishedata );


protected:

    void            SetInterval ( int from, int to );

};


//----------------------------------------------------------------------------
                                        // Class to handle resampling of some collection / array
constexpr double    ResamplingMinCoverage       = 0.0001;
constexpr double    ResamplingMaxCoverage       = 0.9999;


class   TResampling
{
public:
                    TResampling ();


    int             NumData;            // actual number of data points
    int             SampleSize;         // size of sample
    int             NumResampling;      // and number of samples
    double          Coverage;           // probability to pick any given sample, the higher the better but it depends on the parameters from the caller


    void            Reset           ();
    void            SetNumData      ( int           numdata    );
    void            SetSampleSize   ( int           samplesize );
    void            SetSampleSize   ( const char*   samplesize );
    void            SetSampleSize   ( owl::TEdit*	samplesize );
    void            SetNumResampling( int           numresampling );
    void            SetNumResampling( const char*   numresampling );
    void            SetNumResampling( owl::TEdit*	numresampling );


    double          SampleSizeToNumResampling ( double coverage );
    double          NumResamplingToSampleSize ( double coverage );

    int             GetNumResampling( double coverage, int minnumsamples = 0, int maxnumsamples = 0 );
    int             GetSampleSize   ( double coverage, int minsamplesize = 0, int maxsamplesize = 0 );
    double          GetCoverage     ();
    double          GetRelativeSampleSize   ();


    void            Show        ( char *title = 0 );

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}







