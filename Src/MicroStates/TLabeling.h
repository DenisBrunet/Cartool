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

#include    "CartoolTypes.h"
#include    "TArray1.h"
#include    "Math.Utils.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Setting some limits on the range of correlation rejection
constexpr double    MinCorrelationThreshold     = -1.00;
constexpr double    MaxCorrelationThreshold     =  0.99;    // NOT exactly 1.0, so as to at least let some data in, otherwise it makes no sense

                                        // Default correlation threshold when no limit on correlation has been set
constexpr double    IgnoreCorrelationThreshold  = -1.00;


class       TMaps;


//----------------------------------------------------------------------------
                                        // !Make sure to call UpdatePolarities after using these functions!
bool    KeepBestLabelingDirect  (   
                                const TMap&     map1,
                                const TMap&     map2,
                                double&         corrmax,
                                double          limitcorr   = IgnoreCorrelationThreshold
                                );

bool    KeepBestLabelingEvaluate(   
                                const TMap&     map1,
                                const TMap&     map2,
                                double&         corrmax,
                                double          limitcorr   = IgnoreCorrelationThreshold
                                );


//----------------------------------------------------------------------------
                                        // Labeling class, for concatenated data
class   TLabeling
{
public:
                    TLabeling           ();
                    TLabeling           ( int numdata );
                   ~TLabeling           ();


    bool            IsNotAllocated      ()                          const   { return Labels.IsNotAllocated  (); }


    void            DeallocateMemory    ();
    void            Resize              ( int numdata );
    void            Reset               ();
    void            Reset               ( long tf );
    void            Reset               ( long tfmin, long tfmax );


    void            SetLabel            ( long tf, LabelType label                        );    // !Make sure to call UpdatePolarities after using this function!
    void            SetLabel            ( long tf, LabelType label, PolarityType polarity );    // PolarityEvaluate NOT allowed
    void            SetPolarity         ( long tf,                  PolarityType polarity );


    int             GetNumData          ()                          const   { return  Labels.GetDim1 (); }
    int             CountLabels         ( int maxclusters )         const;
    LabelType       MaxLabel            ()                          const;
    int             GetSizeOfClusters   ( int cluster )             const;
    int             GetSizeOfClusters   ( int mincluster, int maxcluster, int step = 1 )    const;
    int             PackLabels          ( TMaps& maps );
    void            ReorderLabels       ( TMaps& maps, TArray2<int>& ordering );

    void            KeepBestLabeling    ( const TMap& map1, const TMap& map2, long tf, LabelType templatenumber, PolarityType polarity, double& corrmax, double limitcorr = IgnoreCorrelationThreshold );
    void            UpdatePolarities    ( const TMaps& data, long tfmin, long tfmax, const TMaps& maps, PolarityType polarity );


    bool            IsDefined           ( long tf )                 const   { return  Labels   ( tf ) != UndefinedLabel; }
    bool            IsUndefined         ( long tf )                 const   { return  Labels   ( tf ) == UndefinedLabel; }
    bool            IsInverted          ( long tf )                 const   { return  Polarity ( tf ) == PolarityInvert; }
    bool            IsNotInverted       ( long tf )                 const   { return  Polarity ( tf ) != PolarityInvert; }
    bool            IsDifferent         ( TLabeling& lab, long tf ) const   { return  ! ( Labels ( tf ) == lab.Labels ( tf ) && Polarity ( tf ) == lab.Polarity ( tf ) ); }
    int             GetSign             ( long tf )                 const   { return  TrueToMinus ( IsInverted ( tf ) ); }  // Inverted -> -1, used for summation
    PolarityType    GetPolarity         ( long tf )                 const   { return  (PolarityType) Polarity ( tf ); }


    void            ReadFile            ( const char*  filename );  // !All files will be internally allocated the same size!


                    TLabeling           ( const TLabeling& op  );
    TLabeling&      operator    =       ( const TLabeling& op2 );


    const LabelType&    operator[]      ( int index )               const   { return Labels ( index ); }
          LabelType&    operator[]      ( int index )                       { return Labels ( index ); }
    const LabelType&    operator()      ( int index )               const   { return Labels ( index ); }
          LabelType&    operator()      ( int index )                       { return Labels ( index ); }


protected:
    
    TArray1<LabelType>  Labels;         // labels for all data, first map has index 0
    TArray1<UCHAR>      Polarity;       // PolarityType type

};



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
