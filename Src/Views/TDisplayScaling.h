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
//----------------------------------------------------------------------------

constexpr double    ScalingLevelInit            = 0.5;
constexpr double    ScalingLevelSmallStep       = 1.1;
constexpr double    ScalingLevelStep            = 1.1;
constexpr double    ScalingLevelBigStep         = 1.3;

constexpr double    ScalingMinContrast          = 0.0;
constexpr double    ScalingMaxContrast          = 1.0;


enum    ScalingAutoType
        {  
        ScalingAutoOff,
        ScalingAutoSymmetric,           // both negative and positive scales are the same
        ScalingAutoAsymmetric,          // negative and positive have their own scales

        NumScalingAuto,
        NumScalingAutoPositive  = ScalingAutoSymmetric + 1,
        };


//----------------------------------------------------------------------------
                                        // Class to handle data scaling for display purposes
class   TDisplayScaling
{
public:
    inline          TDisplayScaling ();


    double          ScalingLevel;
    double          ScalingLimitMin;    // ABSOLUTE limits for ScalingLevel
    double          ScalingLimitMax;
    double          ScalingContrast;    // 1 max contrast, 0 low contrast (linear)
    double          ScalingPMax;        // current positive intensity Max
    double          ScalingNMax;        // current negative intensity Max
    ScalingAutoType ScalingAuto;        // adaptative scaling


    virtual inline void     SetScalingLimits    ( double absminv, double absmaxv );
    virtual inline void     SetScaling          ( double scaling );
    virtual inline void     SetScaling          ( double negv, double posv, bool forcesymetric = true );
    virtual inline double   SetScalingContrast  ( double contrast );


protected:

    virtual inline void     UpdateScaling ();
};


//----------------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------------
        TDisplayScaling::TDisplayScaling ()
{
ScalingLimitMin     = SingleFloatEpsilon;
ScalingLimitMax     = 1;
ScalingLevel        = ScalingLimitMin + ( ScalingLimitMax - ScalingLimitMin ) * ScalingLevelInit;
ScalingContrast     = ScalingMinContrast;
ScalingPMax         = ScalingLevel;
ScalingNMax         = - ScalingPMax;
ScalingAuto         = ScalingAutoOff;
}


//----------------------------------------------------------------------------
                                        // Absolute values
void    TDisplayScaling::SetScalingLimits ( double absminv, double absmaxv )
{
absminv         = fabs ( absminv );
absmaxv         = fabs ( absmaxv );

CheckOrder ( absminv, absmaxv );

ScalingLimitMin = absminv;
ScalingLimitMax = absmaxv;
}


//----------------------------------------------------------------------------
                                        // Absolute value
void    TDisplayScaling::SetScaling ( double scaling )
{
ScalingLevel    = Clip ( fabs ( scaling ), ScalingLimitMin, ScalingLimitMax );

UpdateScaling ();
}


//----------------------------------------------------------------------------
                                        // Negative and Positive values
void    TDisplayScaling::SetScaling ( double negv, double posv, bool forcesymetric )
{
                                        // force positive
ScalingPMax         =   fabs ( posv );

                                        // force negative
ScalingNMax         = - fabs ( negv );

                                        // keep biggest scaling (most significant if one is actually 0) + clipping to safe values
ScalingLevel        = Clip ( max ( ScalingPMax, -ScalingNMax ), ScalingLimitMin, ScalingLimitMax );

                                        // make the 2 scaling equal
if ( forcesymetric )
    UpdateScaling ();
}


//----------------------------------------------------------------------------

double  TDisplayScaling::SetScalingContrast ( double contrast )
{
double              osc             = ScalingContrast;

ScalingContrast     = Clip ( contrast, ScalingMinContrast, ScalingMaxContrast );

UpdateScaling ();

return  osc;
}


//----------------------------------------------------------------------------

void    TDisplayScaling::UpdateScaling ()
{
                                        // update these variables - ScalingLevel is always positive (a member with a good attitude)
ScalingPMax =   ScalingLevel;
ScalingNMax = - ScalingLevel;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
