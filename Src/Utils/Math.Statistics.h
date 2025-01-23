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

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Used internally to store various intermediate tests results
enum        TestVariablesEnum
            {      
            TestNumSamples,
            TestDoF,
            TestMean,
            TestMeanSquares,
            TestVariance,
            TestStandardDeviation,
            TestStandardError,
            Test_t_value,
            Test_p_value,
            Test_Dissimilarity,

            NumTestVariables    
            };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Statistics utilities
enum        OutputPType;
enum        CorrectionType;
enum        StatTimeType;
enum        PairedType;
enum        ReferenceType;
enum        PolarityType;
template <class TypeD> class        TArray2;
template <class TypeD> class        TArray3;


int         GetTimeAverageIndex         (   const TArray3<float>&   data    );  // Where the optional time average value is being stored


long double Student_TwoTails_t_to_p     ( long double t, int dof );



double      Format_p_value              (   double                  p,
                                            OutputPType             how,                          
                                            bool                    clippingpvalue,     double              pvaluemin 
                                        );


void        Correct_p_values            (   TArray3<float>&         Results,            int                 numvars,        int                 maxnumtf,
                                            CorrectionType          how,
                                            int                     bonferronicorrectionvalue,
                                            double                  fdrcorrectionvalue
                                        );


void        CheckDuration_p_values      (   TArray3<float>&         Results,            int                 numvars,        int                 maxnumtf,
                                            bool                    clippingpvalue,     double              pvaluemin,      int                 pvalueduration
                                        );


void        ComputeSingleGroupMeasures  (   const TArray3<float>&   data,               StatTimeType    stattime,
                                            int                     numtf,              int             numvars,        int             numsamples,
                                            bool                    checkmissingvalues, double          missingvalue,
                                            TArray3<float>&         results
                                        );


void        Run_t_test                  (   const TArray3<float>&   data1,              const TArray3<float>&   data2,
                                            StatTimeType            stattime1,          StatTimeType            stattime2,
                                            int                     numtf1,             int                     numtf2,             int                     jointnumtf,
                                            int                     numsamples1,        int                     numsamples2,        int                     jointnumsamples,
                                            bool                    checkmissingvalues, double                  missingvalue,
                                            int                     numvars,            
                                            PairedType              paired,
                                            TArray3<float>&         results1,           TArray3<float>&         results2,           TArray3<float>&         jointresults
                                        );


void        Run_Randomization_test      (   const TArray3<float>&   data1,              const TArray3<float>&   data2,
                                            StatTimeType            stattime1,          StatTimeType            stattime2,
                                            int                     numtf1,             int                     numtf2,             int                     jointnumtf,
                                            int                     numsamples1,        int                     numsamples2,        int                     jointnumsamples,
                                            bool                    checkmissingvalues, double                  missingvalue,
                                            int                     numvars,            
                                            PairedType              paired,
                                            int                     numrand,
                                            TArray3<float>&         results1,           TArray3<float>&         results2,           TArray3<float>&         jointresults
                                        );


void        Run_TAnova                  (   const TArray3<float>&   data1,              const TArray3<float>&   data2,
                                            StatTimeType            stattime1,          StatTimeType            stattime2,
                                            int                     numtf,
                                            int                     numsamples1,        int                     numsamples2,        int                     jointnumsamples,
                                            int                     numvars,            
                                            PairedType              paired,
                                            int                     numrand,            
                                            ReferenceType           processingref,      PolarityType            polarity, 
                                            TArray3<float>&         Results,            TArray2<float>*         sampleddiss
                                        );


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
