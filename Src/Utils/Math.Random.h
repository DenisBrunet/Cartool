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


#include    <random>
#include    <limits.h>                  // INT_MAX, UCHAR_MAX...
#include    <minwindef.h>               // basic Windows types

#include    "Math.Utils.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Using C++11 random generator
                                        // Max value INCLUDED
constexpr UINT      RandomMaxIncl      = UINT_MAX;
                                        // Max value EXCLUDED by adding 1 to RandomMaxIncl (!value is therefor greater than a single UINT32!)
constexpr UINT64    RandomMaxExcl      = 0x100000000Ui64;

                                        // Just wrapping the preferred random generator, which is then re-used by all derived classes
class   TRand
{
public:
                    TRand           ( UINT seed = 0 );

    void            Reload          ( UINT seed = 0 );

protected:

    std::mt19937    Mersene32;          // 32 bits version seems good enough, and is definitely faster

    UINT            GetRandomDevice ( UINT additionalseed = 0 );

    UINT            Random          ()              {  return  Mersene32 ();    }   // the core function, results in [0..0xFFFFFFFF]

};


//----------------------------------------------------------------------------
                                        // Derived distributions, which all need the uniform generator
class   TRandUniform    : public TRand
{
public:
                    TRandUniform    ( UINT seed = 0 )                           : TRand ( seed )    {}

                                    // integer results in [0  ..max) - !not optimal for values greater than UINT32!
    UINT            operator()      ( UINT       max )                  {  return (UINT) ( ( (double) TRand::Random () / RandomMaxExcl ) * max );  }
                                    // integer results in [min..max] - !not optimal for values greater than UINT32!
    UINT            operator()      ( UINT       min, UINT       max )  {  return (UINT) ( ( (double) TRand::Random () / RandomMaxExcl ) * ( max - min + 1 ) + min );    }
                                    // float   results in [0  ..max]
    double          operator()      ( double     max )                  {  return          ( (double) TRand::Random () / RandomMaxIncl ) * max; }
                                    // float   results in [min..max]
    double          operator()      ( double     min, double     max )  {  return          ( (double) TRand::Random () / RandomMaxIncl ) * ( max - min ) + min; }

};


//----------------------------------------------------------------------------

class   TRandCoin       : public TRand
{
public:
                    TRandCoin       ( UINT seed = 0 )                           : TRand ( seed )    {}

    bool            operator()      ()      {  return  IsOdd ( TRand::Random () ); }   // Half chance to be true

};


//----------------------------------------------------------------------------

class   TRandNormal     : public TRand
{
public:
                    TRandNormal     ( UINT seed = 0 )                           : TRand ( seed )    {}
                    TRandNormal     ( double mean, double sd, UINT seed = 0 )   : TRand ( seed )    { Set ( mean, sd ); }

    void            Set             ( double mean, double sd )  { distr = std::normal_distribution<double> ( mean, AtLeast ( SingleFloatEpsilon, sd ) ); }

    double          operator()      ()      {  return distr ( Mersene32 ); }

private:

    std::normal_distribution<double>  distr;

};


//----------------------------------------------------------------------------

class   TRandChiSquare  : public TRand
{
public:
                    TRandChiSquare  ()                                          : TRand ()    {}
//                  TRandChiSquare  ( UINT seed = 0 )                           : TRand ( seed )    {}                  // !caller must take care between UINT and int parameter!
                    TRandChiSquare  ( int degree, UINT seed = 0 )               : TRand ( seed )    { Set ( degree ); }

    void            Set             ( int degree )  { distr = std::chi_squared_distribution<double> ( degree ); }

    double          operator()      ()      {  return distr ( Mersene32 ); }

private:

    std::chi_squared_distribution<double>  distr;

};


//----------------------------------------------------------------------------

class   TRandSpherical  : protected TRandUniform
{
public:
                    TRandSpherical  ( UINT seed = 0 )                           : TRandUniform ( seed )    {}

                                        // Random uniform position ON THE SPHERE
    TVector3Float   operator()      ()              {   TVector3Float   p;
//                                                      double  theta   =         TRandUniform::operator() (     TwoPi );           // in [0..2Pi], but should be in [0..2Pi)
                                                        double  theta   = ( (double) TRand::Random () / RandomMaxExcl ) * TwoPi;    // in [0..2Pi)
                                                        p.Z             = (float) TRandUniform::operator() ( -1.0, 1.0 );
                                                        p.X             = (float) ( sin ( theta ) * sqrt ( 1 - Square ( p.Z ) ) );
                                                        p.Y             = (float) ( cos ( theta ) * sqrt ( 1 - Square ( p.Z ) ) );
                                                        return  p;
                                                    }

                                        // Random uniform position WITHIN THE SPHERE of radius r
    TVector3Float   operator()      ( double r )    {   return  operator() () * ( r * CubicRoot ( TRandUniform::operator() ( 1.0 ) ) ); }

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}







