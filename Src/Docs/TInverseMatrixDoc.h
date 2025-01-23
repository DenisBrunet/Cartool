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

#include    "Math.Armadillo.h"

#include    "Strings.TStrings.h"
#include    "TArray1.h"
#include    "TArray2.h"

#include    "TBaseDoc.h"
#include    "TInverseResults.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Base class for any inverse matrix

class   TInverseMatrixDoc   :   public  TBaseDoc,
                                public  TInverseResults // can produce Inverse Solution results
{
public:
                    TInverseMatrixDoc ( owl::TDocument *parent = 0 );


    bool            Close	();
    bool            Commit	( bool force = false );
    bool            Revert	( bool force = false );
    bool            IsOpen	()                  const   { return  NumSolPoints > 0; }
    bool            InitDoc ();


    size_t          AtomSize ()                 const   { return sizeof ( AReal ); }
    size_t          SingleMatrixMemorySize()    const   { return GetNumLines () * NumElectrodes * AtomSize (); }

    int             GetNumElectrodes      ()    const   { return    NumElectrodes; }
    int             GetNumSolPoints       ()    const   { return    NumSolPoints; }
    int             GetNumLines           ()    const   { return    IsVector ( AtomTypeUseOriginal ) ? 3 * NumSolPoints : NumSolPoints; }

    bool            IsStackMatrices       ()    const   { return    (bool) NumRegularizations; }
    int             GetNumRegularizations ()    const   { return    NumRegularizations; }
    int             GetMaxRegularization  ()    const   { return    IsStackMatrices () ? NumRegularizations : 1; }  // returns 1 for 0 or 1 regularization, NumRegularizations otherwise
    int             GetRegularizationIndex ( const char *regstring )    const;
    const TStrings* GetRegularizationsNames ()  const   { return   &RegularizationsNames; }

    int             GetBestRegularization       ( const TMap* map, TTracksView* eegview, long tf )  const;                                  // either map or eegview + tf
    int             GetGlobalRegularization     ( const TMaps* maps, TTracksView* eegview, long fromtf, long totf, long steptf )    const;  // either from maps or range of tf from eegview


    void                    SetAveragingBefore ( TAveragingPrecedence ap )  { AveragingPrecedence = ap; }
    TAveragingPrecedence    GetAveragingBefore ()                           { return  AveragingPrecedence; }


    const char*     GetInverseTitle ()                  const   { return GetTitle(); }
    const char*     GetInverseName  ( char* isname )    const;   // short version within a list of known names

                                        // does all the job to fetch the EEG and returns the resulting multiplication
    void            GetInvSol      ( int reg, long tf1, long tf2, TArray1< float >         &inv, TTracksView *eegview, TRois *rois = 0 )    const;
    void            GetInvSol      ( int reg, long tf1, long tf2, TArray1< TVector3Float > &inv, TTracksView *eegview, TRois *rois = 0 )    const;

                                        // provide also these functions, if the caller provides an EEG buffer
    void            MultiplyMatrix ( int reg, const TMap&                   map,            TArray1<float>&            inv )    const;
    void            MultiplyMatrix ( int reg, const TMap&                   map,            TArray1<TVector3Float>&    inv )    const;
//  void            MultiplyMatrix ( int reg, const AVector&                map,            AVector&                   inv )    const;
    void            MultiplyMatrix ( int reg, const TArray2<float>&         eeg,    int tf, TArray1<float>&            inv )    const; 
    void            MultiplyMatrix ( int reg, const TArray2<float>&         eeg,    int tf, TArray1<TVector3Float>&    inv )    const; 
    void            MultiplyMatrix ( int reg, const AMatrix&                eeg,    int tf, TArray1<TVector3Float>&    inv )    const; 


protected:
                                        // Set of (at least 1) matrices, with increasing regularization if more than 1
//  std::vector<AMatrix>            M;  // using an Armadillo matrix - problem is it is column-major ordering (FORTRAN / Matlab style) and it would need all MultiplyMatrix to rewritten
    std::vector<TArray2<AReal>>     M;  // using a simpler 2D array, which is row-major as expected by current methods

    int             NumElectrodes;
    int             NumSolPoints;
    int             NumRegularizations;
    TAveragingPrecedence    AveragingPrecedence;

    TStrings        ElectrodesNames;
    TStrings        SolutionPointsNames;
    TArray1<double> RegularizationsValues;
    TStrings        RegularizationsNames;


    void            SetDefaultVariables ();

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
