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

#include    "TBaseDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template <class TypeD> class        TArray1;
class                               TMaps;

                                        // define the common interface to retrieve inverses from either an IS matrix or a RIS file
constexpr int       InverseMaxElectrodeName         = 32;
constexpr int       InverseMaxSolutionPointName     = 16;
constexpr int       InverseMaxRegularizationName    = 32;


//----------------------------------------------------------------------------
                                        // Fraction of the biggest Eigenvalue taken at each regularization step
                                        // Each inverse can have a different factor, and factors are calibrated
                                        // so that their "smoothing" effect on the data are comparable among inverses
                                        // Based on the MN, WMN, LORETA and LAURA, we have approximately these decays:
                                        //      Reg0 -> Reg1    -25%
                                        //      Reg1 -> Reg2    -15%
                                        //      Reg2 -> Reg3     -8%
                                        //      Reg3 -> Reg4     -6%
                                        //      Reg0 -> Reg12   -57%
constexpr double    EigenvalueToRegularizationFactorMN          = 13000;
constexpr double    EigenvalueToRegularizationFactorWMN         = 15000;
constexpr double    EigenvalueToRegularizationFactorLORETA      = 20000;
constexpr double    EigenvalueToRegularizationFactorLAURA       = 20000;
constexpr double    EigenvalueToRegularizationFactorSLORETA     = 50000;
constexpr double    EigenvalueToRegularizationFactorELORETA     =   100;
constexpr double    EigenvalueToRegularizationFactorDale        = 15000;


//----------------------------------------------------------------------------

enum                TAveragingPrecedence
                    {
                    AverageBeforeInverse,
                    AverageAfterInverse,
                    AverageDefault          = AverageAfterInverse,
                    };


//----------------------------------------------------------------------------
                                        // Common class to both  TInverseMatrixDoc  (inverse matrices) and  TRisDoc  (results of computation)
                                        // It allows for a single interface to retrieve the inverse reults from these 2 different sources, f.ex. for display purpose

class   TInverseResults :   public virtual  TDataFormat   // !Could be inherited by different sub-children, hence virtual!
{
public:

    virtual const char*             GetInverseTitle         ()  const   = 0;

    virtual int                     GetNumRegularizations   ()                                                                                  const   { return    0; }    // if there are any
    virtual int                     GetMaxRegularization    ()                                                                                  const   { return    1; }    // # stored matrices
    virtual const TStrings*         GetRegularizationsNames ()                                                                                  const   { return    0; }
    virtual int                     GetRegularizationIndex  ( const char *regstring )                                                           const   { return    0; }
    virtual int                     GetGlobalRegularization ( const TMaps* maps, TTracksView* eegview, long fromtf, long totf, long steptf )    const   { return    Regularization0; }

    virtual void                    SetAveragingBefore ( TAveragingPrecedence ap )  {}
    virtual TAveragingPrecedence    GetAveragingBefore ()                           { return  AverageAfterInverse; }


    virtual void    GetInvSol ( int reg, long tf1, long tf2, TArray1< float >         &inv, TTracksView *eegview, TRois *rois = 0 )     const = 0;
    virtual void    GetInvSol ( int reg, long tf1, long tf2, TArray1< TVector3Float > &inv, TTracksView *eegview, TRois *rois = 0 )     const = 0;

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

RegularizationType  StringToRegularization ( const char* regstr )
{
if ( StringIsEmpty ( regstr ) )
    return  RegularizationNone;

if      ( StringStartsWith ( regstr, RegularizationNoneStringS       ) )    return  RegularizationNone;         // "N" or "None"
else if ( StringStartsWith ( regstr, RegularizationAutoGlobalStringS ) )    return  RegularizationAutoGlobal;   // "G" or "Global"
else if ( StringStartsWith ( regstr, RegularizationAutoLocalStringS  ) )    return  RegularizationAutoLocal;    // "L" or "Local"
else {

    int                 regi            = StringToInteger ( regstr );
                                        // negative regularization is definitely not legal
    if ( regi < 0 )     return  RegularizationNone;

    return  Clip ( (RegularizationType) regi, FirstRegularization, LastRegularization );
    }
}


char*               RegularizationToString ( RegularizationType reg, char* regstr, bool shortversion )
{
if      ( reg == RegularizationNone       )     StringCopy      ( regstr, shortversion ? RegularizationNoneStringS       : "No Regularization"             );
else if ( reg == RegularizationAutoGlobal )     StringCopy      ( regstr, shortversion ? RegularizationAutoGlobalStringS : "Global Optimal Regularization" );
else if ( reg == RegularizationAutoLocal  )     StringCopy      ( regstr, shortversion ? RegularizationAutoLocalStringS  : "Local Optimal Regularization"  );
else                                            IntegerToString ( regstr, reg - Regularization0 );  // Regularization0 to Regularization12

return  regstr;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
