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


#include    "Math.Armadillo.h"
#include    "CartoolTypes.h"
#include    "Files.Utils.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // forward declarations
enum        RegularizationType;

class       TInverseMatrixDoc;
class       TMarkers;
class       TGoMaps;
class       TLeadField;
class       TLabeling;
class       TStrings;
class       TGoF;
class       TGoGoF;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Basically scalar product with polarity checks - doesn't care for data normalized or not
double      Project                     ( const TMap&   map1,   const TMap&   map2,     PolarityType    polarity );  // single map    vs single map
double      Project                     ( const TMaps&  maps1,  const TMaps&  maps2,    PolarityType    polarity );  // group of maps vs group of maps


constexpr int       MedoidNumSamples            = 1033;
constexpr int       LabelingNumSamples          =  599;


TMap        ComputeCentroid             (   const TArray1<TMap*>&   allmaps,
                                            CentroidType        centroid,
                                            AtomType            datatype,           PolarityType        polarity,
                                            int                 maxsamples  = MedoidNumSamples,
                                            TLabeling*          labels      = 0,    int         l   = UndefinedLabel,   const TMap*         ref         = 0
                                        );

TMap        ComputeMeanCentroid         (   const TArray1<TMap*>&    allmaps,
                                            AtomType            datatype,           PolarityType        polarity,
                                            TLabeling*          labels      = 0,    int         l   = UndefinedLabel
                                        );

TMap        ComputeWeightedMeanCentroid (   const TArray1<TMap*>&    allmaps,
                                            AtomType            datatype,           PolarityType        polarity,
                                            TLabeling*          labels      = 0,    int         l   = UndefinedLabel,   const TMap*         ref         = 0
                                        );

TMap        ComputeMaxCentroid          (   const TArray1<TMap*>&    allmaps,
                                            AtomType            datatype,           PolarityType        polarity,
                                            TLabeling*          labels      = 0,    int         l   = UndefinedLabel
                                        );

TMap        ComputeMedianCentroid       (   const TArray1<TMap*>&    allmaps,
                                            AtomType            datatype,           PolarityType        polarity,
                                            TLabeling*          labels      = 0,    int         l   = UndefinedLabel
                                        );

TMap        ComputeMedoidCentroid       (   const TArray1<TMap*>&    allmaps,
                                            AtomType            datatype,           PolarityType        polarity,
                                            int                 maxsamples,
                                            TLabeling*          labels      = 0,    int         l   = UndefinedLabel
                                        );

TMap        ComputeEigenvectorCentroid  (   const TArray1<TMap*>&    allmaps,
                                            AtomType            datatype,           PolarityType        polarity,
                                            int                 maxsamples,
                                            TLabeling*          labels      = 0,    int         l   = UndefinedLabel
                                        );

TMap        ComputeCloudsFolding        (   const TArray1<TMap*>&    allmaps,
                                            int                 maxsamples,
                                            TLabeling*          labels,             int         l   = UndefinedLabel
                                        );

                                        // 11 samplings do the job
constexpr int       ReferenceNumSamplesMap      =   11;
                                        // there is much more variability at the SP level than on the map level
constexpr int       ReferenceNumSamplesSP       =  200;
constexpr int       ReferenceNumSamplesSP2      = 1000;

TMap        ComputeReferenceMap         (   const TArray1<TMap*>&   allmaps,
                                            AtomType            datatype,
                                            TLabeling*          labels      = 0,    int         l   = UndefinedLabel
                                        );

TVector3Float       ComputeCloudFolding     (   const TPoints&      points,
                                                int                 numcandidates,
                                                int                 numtests
                                            );


//----------------------------------------------------------------------------
                                        // Maps == 1 file

                                        // Optimized:       Pro: all maps are allocated in a contiguous block of memory - Con: can not directly assign to a map from a TMaps
                                        // Not optimized:   Con: maps are fragmented in memory, each map is allocated somewhere - Pro: can assign directly; maybe can fit more into memory due to fragmentation?
//#define             TMapsOptimized

                                        // For Z-Score, priority to the number of resampling
constexpr int       TMapsNumResampling          = 21;
constexpr double    TMapsResamplingCoverage     = 0.95;
constexpr int       TMapsMinSampleSize          = 1000;
//constexpr double  TMapsMaxSampleSizeRatio     = 0.95;
constexpr double    TMapsMaxSampleSizeRatio     = 0.50;

                                        // not given a name here, as we want to be able to either pass on of these flags, or a valid, positive, fixed index
enum                {
                    ReadGoMapsToDimension   = -1,   // tells which file dimension goes to map Dimension
                    ReadGoMapsToNumMaps     = -2,   // tells which file dimension goes to number of maps
                    ReadGoMapsIgnore        = -3,   // 
                    };

                                        // Use the given order to reorder the maps & labels
enum                MapOrderingVariables {
                    scanindex       = 0,    // always first
                    scanbackindex,          // always second
                    scanmintf,
                    scanmeantf,
                    scannumtf,
//                  scanweight,             // to weight by GFP
                    scanfromtemplates,
                    scanorient,

                    numscanvar
                    };


class   TMaps
{
public:
                    TMaps                       ();
                    TMaps                       ( int nummaps, int dim );
                    TMaps                       ( const char* filename, AtomType datatype, ReferenceType reference, TStrings* tracksnames = 0 );
                    TMaps                       ( const TGoMaps* gogomaps );
                    TMaps                       ( const TGoF& gof, AtomType datatype, ReferenceType reference, TStrings*    tracksnames = 0 );
                    TMaps                       ( const TMaps &op, int downsampling, const TSelection* ignoretracks = 0 );
                   ~TMaps                       ();


    bool            IsAllocated                 ()                      const   { return NumMaps != 0; }
    bool            IsNotAllocated              ()                      const   { return NumMaps == 0; }
    bool            SomeNullMaps                ( int nummaps = -1 )    const;
    int             CheckNumMaps                ( int& nummaps )        const   { nummaps = nummaps < 0 ? NumMaps : crtl::NoMore ( NumMaps, (const int) nummaps ); return nummaps; }  // passing -1 for all maps, otherwise doing a safe range clipping


    void            DeallocateMemory            ();                         // delete
    void            Resize                      ( int nummaps, int dim );
    void            Reset                       ( int nummaps = -1 );       // clear


    int             GetNumMaps                  ()                      const   { return  NumMaps; }
    int             GetDimension                ()                      const   { return  Dimension; }
    int             GetLinearDim                ()                      const   { return  NumMaps * Dimension; }
    void            GetIndexes                  ( TArray1<TMap *> &indexes, const TSelection* tfok = 0 )    const;  // returns a linear index structure so we can iterate through all data at once
    TArray1<TMap *> GetIndexes                  ()                              const;
    double          GetSamplingFrequency        ()                      const   { return SamplingFrequency; }
    void            SetSamplingFrequency        ( double sf )                   { SamplingFrequency = crtl::AtLeast ( 0.0, sf ); }


    void            Add                         ( const TMap&    map      );
    void            Set                         ( const TGoMaps* gogomaps );

    void            CopyFrom                    ( TMaps& othermaps, int nummaps = -1 );   // !this will allocate space if needed!


    void            Absolute                    ();
    void            ApplyGfpNormalization       ( double  gfpnorm );
    void            ApplyZScore                 ( ZScoreType how, const TArray2<float>& zscorevalues );
    void            ApplyZScoreSigned           ( ZScoreType how, const TArray2<float>& zscorevalues );
    void            ApplyZScorePositive         ( ZScoreType how, const TArray2<float>& zscorevalues );
    void            ApplyZScoreVectorial        ( ZScoreType how, const TArray2<float>& zscorevalues );
    void            AtLeast                     ( TMapAtomType minv );
    void            AverageReference            ( AtomType datatype );
    void            Clipped                     ( TMapAtomType minv, TMapAtomType maxv );
    void            ComputeDissimilarity        ( TArray1<double> &dis, PolarityType polarity, ReferenceType reference ) const;
    RegularizationType  ComputeESI              ( const TInverseMatrixDoc* ISDoc, RegularizationType regularization, bool vectorial, TMaps& ESI )    const;
    void            ComputeGFP                  ( TArray1<double> &gfp,  ReferenceType reference, AtomType datatype ) const;
    double          ComputeGfpNormalization     ( AtomType datatype, double& gfpnorm );
    void            ComputeNorm                 ( TArray1<double> &norm, ReferenceType reference );
    void            ComputeZScore               ( ZScoreType how, TArray2<float>& zscorevalues )    const;
    void            ComputeZScoreSigned         ( ZScoreType how, TArray2<float>& zscorevalues )    const;
    void            ComputeZScorePositive       ( ZScoreType how, TArray2<float>& zscorevalues )    const;
    void            ComputeZScoreVectorial      ( ZScoreType how, TArray2<float>& zscorevalues )    const;
    void            Correlate                   ( TMaps& maps1, TMaps& maps2, CorrelateType how, PolarityType polarity, ReferenceType reference, int numrand = 0, TMaps* pvalues = 0, char* infix = 0 );
    bool            Covariance3DVectorial       ( AMatrix33& Cov, const TSelection* tfok = 0 )    const;
    bool            FilterSpatial               ( SpatialFilterType filtertype, const char *xyzfile );
    void            FilterTime                  ( FilterTypes filtertype, FctParams& params, bool showprogress = false );
    TMapAtomType    GetMinValue                 ()                                          const;
    TMapAtomType    GetMaxValue                 ()                                          const;
    TMapAtomType    GetAbsMaxValue              ()                                          const;
    void            GetNullElectrodes           ( TSelection& sel )                         const;
    bool            IsZScorePositiveShifted     ()                                          const;
    void            Mean                        ( int frommap, int tomap, TMap& avgmap )    const;
    void            Median                      ( int frommap, int tomap, TMap& avgmap )    const;
    void            Multiply                    ( const AMatrix& T, TMaps& results )        const;
    void            NoMore                      ( TMapAtomType maxv );
    void            Normalize                   ( AtomType datatype, int nummaps = -1, bool centeraverage = false );
    void            NormalizeSolutionPoints     ( AtomType datatype, int nummaps = -1 );
    void            Orthogonalize               ( int nummaps = -1 );
    void            OrthogonalizeRanks          ( RankingOptions options, int nummaps = -1 );
    void            Random                      ( double minv, double maxv );
    void            SD                          ( int frommap, int tomap, TMap& avgmap )    const;
    void            SetReference                ( ReferenceType ref, AtomType datatype );
    void            ThresholdAbove              ( TMapAtomType t );
    void            Thresholding                ( double threshold, AtomType datatype, int nummaps = -1 );
    void            TimeCentering               ( bool robust = false );
    void            ToRank                      ( AtomType datatype, RankingOptions options, int nummaps = -1 );
    void            ZPositiveToZSigned          ();
    void            ZPositiveToZSignedAuto      ();
    void            ZPositiveAuto               ();
    void            ZScore                      ( ZScoreType how, TArray2<float>* tozscorevalues = 0 );

                                        // Functions used during segmentation / fitting
    void            CentroidsToLabeling         ( const TMaps& data, long tfmin, long tfmax, int nclusters, const TSelection *mapsel, TLabeling& labels, PolarityType polarity, double limitcorr )   const;
    void            LabelingToCentroids         ( const TMaps& data, const TArray1<TMap *>* todata, int nclusters, TLabeling& labels, PolarityType polarity, CentroidType centroid, bool ranking, bool updatepolarity = true );

    TMap            ComputeCentroid             ( CentroidType centroid, AtomType datatype, PolarityType polarity, int maxsamples = MedoidNumSamples, TLabeling* labels = 0, int l = UndefinedLabel, const TMap* ref = 0 )    const;

    double          GetClosestPair              ( int nclusters, PolarityType polarity, LabelType &index1, LabelType &index2 );
    

    void            AlignSuccessivePolarities   ();   // for spontaneous data, successive maps are flipped for a better visual result


    bool            MapsFromLeadField           ( int nummaps, double correlationmin, double correlationmax, bool ignorepolarity, TLeadField& leadfield, TTracks<float>& K, int numsources, TMaps* sourcemaps = 0 );
    bool            RisFromSolutionPoints       ( int nummaps, double correlationmin, double correlationmax, TPoints solp, int numsources, double spreadmax, double axisvariability );
    double          EstimateSigmaData           ();                                             // SD of all dimensions
    void            AddGaussianNoise            ( double sigmadata, double percentsnr );
    void            AddGaussianNoise            ( double sigmadata, double percentsnr1, double percentsnr2 );


    void            ReadFile                    ( const char* filename, AtomType datatype, ReferenceType reference, TStrings*    tracksnames = 0, TStrings*    freqsnames = 0, int dim1goes = ReadGoMapsToDimension, int dim2goes = ReadGoMapsToNumMaps, int dim3goes = ReadGoMapsIgnore, int dimmargin = 0 );
    void            ReadFile                    ( const char* filename, AtomType datatype, ReferenceType reference, const TMarkers& keeplist, TStrings*    tracksnames = 0 );
    void            ReadFiles                   ( const TGoF& gof,      AtomType datatype, ReferenceType reference, TStrings*    tracksnames = 0 );
    void            WriteFile                   ( const char* filename, bool vectorial = false, double samplingfrequency = 0, const TStrings*    tracksnames = 0 )  const;
    void            WriteFileEpochs             ( const char* filename, bool vectorial,         double samplingfrequency,     const TMarkers& keeplist, const TStrings*    tracksnames = 0 )  const; 
    void            WriteFileScalar             ( const char* filename,                         double samplingfrequency = 0, const TStrings*    tracksnames = 0 )  const;
    void            WriteFileReordered          ( const char* filename, bool vectorial,         double samplingfrequency,     const TStrings*    tracksnames, const TStrings*    freqsnames = 0, int dim1gets = ReadGoMapsToDimension, int dim2gets = ReadGoMapsToNumMaps, int dim3gets = ReadGoMapsIgnore, int dimsize = 0, bool updatingcall = false )    const;


                    TMaps                       ( const TMaps &op  );
    TMaps&          operator    =               ( const TMaps &op2 );


    TMap&           operator    ()              ( int index )                   { return Maps[ index ]; }
    TMap&           operator    ()              ( int index )           const   { return Maps[ index ]; }
    TMapAtomType&   operator    ()              ( int index, int dimi )         { return Maps[ index ][ dimi ]; }
    TMapAtomType&   operator    ()              ( int index, int dimi ) const   { return Maps[ index ][ dimi ]; }
    TMap&           operator    []              ( int index )                   { return Maps[ index ]; }
    TMap&           operator    []              ( int index )           const   { return Maps[ index ]; }
    
                    operator    int             ()                      const   { return NumMaps; }
                    operator    bool            ()                      const   { return (bool) NumMaps; }


    TMaps&          operator    +=              ( const TMaps& op2 );
    TMaps&          operator    *=              ( const TMaps& op2 );
    TMaps&          operator    +=              ( double op2 );
    TMaps&          operator    -=              ( double op2 );
    TMaps&          operator    *=              ( double op2 );
    TMaps&          operator    /=              ( double op2 );
    TMaps&          operator    =               ( double op2 );


protected:
    TMap*           Maps;               // maps storage - can be optimized to be consecutive data in memory, see below
    int             NumMaps;
    int             Dimension;
    double          SamplingFrequency;  // in Hertz


private:

#if defined(TMapsOptimized)
    TMapAtomType*   ToMapsArray;        // Optimized allocation - big block of data content
    int             DataMemorySize              ( int nummaps )         const   { return  nummaps * ( Dimension * sizeof ( TMapAtomType ) ); }
#endif

};


//----------------------------------------------------------------------------
                                        // Group Of Maps == Group of files (TGoF)
class   TGoMaps
{
public:
                    TGoMaps         ();
                    TGoMaps         ( int numgroups, int nummaps, int dim );
                    TGoMaps         ( const TGoF* gof, AtomType datatype, ReferenceType reference, TStrings*    tracksnames = 0 );
                   ~TGoMaps         ();


    bool            IsNotAllocated  ()  const                       { return   Group.IsEmpty (); }


    void            DeallocateMemory(); // delete
    void            Resize          ( int numgroups, int nummaps, int dim );
    void            Reset           (); // clear


    int             NumGroups       ()  const                       { return    Group.Num (); }
    int             GetNumMaps      ()  const                       { return    IsNotAllocated () ? 0 : Group[ 0 ]->GetNumMaps   (); }
    int             GetMaxNumMaps   ()  const;
    int             GetTotalNumMaps ()  const;
    int             GetDimension    ()  const                       { return    IsNotAllocated () ? 0 : Group[ 0 ]->GetDimension (); }
    void            GetIndexes      ( TArray1<TMap *> &indexes )    const;  // returns a linear index structure so we can iterate through all data at once
    TArray1<TMap *> GetIndexes      ()                              const;
    void            GetColumnIndexes( TArray1<TMap *> &indexes, int colindex )  const;
    double          GetSamplingFrequency        ()                  const;


    void            Add             ( const TMaps* maps, bool copy = false );
    void            Remove          ( const TMaps* maps );          // from the list, does not destroy the TMaps object


    TMaps          *GetFirst ()                                     { return   Group.GetFirst (); }
    TMaps          *GetLast  ()                                     { return   Group.GetLast  (); }


    void            Normalize                   ( AtomType datatype, bool centeraverage = false  );
    void            SetReference                ( ReferenceType ref, AtomType datatype );
    bool            FilterSpatial               ( SpatialFilterType filtertype, const char *xyzfile );
    void            FilterTime                  ( FilterTypes filtertype, FctParams& params, bool showprogress = false );
    double          ComputeGfpNormalization     ( AtomType datatype, double& gfpnorm );
    void            ApplyGfpNormalization       ( double gfpnorm );
    TMaps           ComputeCentroids            ( CentroidType centroid, AtomType datatype, PolarityType polarity )    const;


    void            ReadFiles       ( const TGoF& gof, AtomType datatype, ReferenceType reference, TStrings* tracksnames = 0 );
    void            WriteFiles      ( const TGoF& gof,  bool vectorial = false, double samplingfrequency = 0, const TStrings*    tracksnames = 0 )  const;
    void            WriteFile       ( const char* file, bool vectorial = false, double samplingfrequency = 0, const TStrings*    tracksnames = 0 )  const;


                    TGoMaps         ( const TGoMaps &op  );
    TGoMaps&        operator    =   ( const TGoMaps &op2 );     


          TMaps&    operator    ()              ( int index )                               { return *Group[ index ]; }
    const TMaps&    operator    ()              ( int index )                       const   { return *Group[ index ]; }
          TMap&     operator    ()              ( int groupi, int index )                   { return (*Group[ groupi ])[ index ]; }
    const TMap&     operator    ()              ( int groupi, int index )           const   { return (*Group[ groupi ])[ index ]; }
          TMapAtomType& operator()              ( int groupi, int index, int dimi )         { return (*Group[ groupi ])[ index ][ dimi ]; }
    const TMapAtomType& operator()              ( int groupi, int index, int dimi ) const   { return (*Group[ groupi ])[ index ][ dimi ]; }
          TMaps&    operator    []              ( int index )                               { return *Group[ index ]; }
    const TMaps&    operator    []              ( int index )                       const   { return *Group[ index ]; }

                    operator    int             ()                                  const   { return (int)  Group; }
                    operator    bool            ()                                  const   { return (bool) Group; }
                    operator    TList<TMaps>&   ()                                          { return Group; }

    TGoMaps&        operator    /=      ( double op2 );


protected:
    crtl::TList<TMaps>  Group;
};


//----------------------------------------------------------------------------
                                        // Multiple Groups Of Maps == Group of Groups of files (TGoGoF)
class   TGoGoMaps
{
public:
                    TGoGoMaps       ();
                    TGoGoMaps       ( int numgroups1, int numgroups2, int nummaps, int dim );
                   ~TGoGoMaps       ();


    bool            IsNotAllocated  ()  const                       { return   Group.IsEmpty (); }


    void            DeallocateMemory(); // delete
    void            Reset           (); // clear


    int             NumGroups       ()  const                       { return    Group.Num (); }
//  int             GetMaxNumMaps   ()  const;
    int             GetTotalNumMaps ()  const;
    int             GetDimension    ()  const                       { return    IsNotAllocated () ? 0 : Group[ 0 ]->GetDimension (); }
//  void            GetIndexes      ( TArray1<TMap *> &indexes );   // returns a linear index structure so we can iterate through all data at once


    void            Add             ( const TGoMaps* gomaps, bool copy = false );
    void            Remove          ( const TGoMaps *gomaps );      // from the list, does not destroy the TGoMaps object


    void            Normalize       ( AtomType datatype, bool centeraverage = false  );
    void            SetReference    ( ReferenceType ref, AtomType datatype );


    void            ReadFiles       ( const TGoGoF* gogof, AtomType datatype, ReferenceType reference );


                    TGoGoMaps       ( const TGoGoMaps &op  );
    TGoGoMaps&      operator    =   ( const TGoGoMaps &op2 );     


          TGoMaps&  operator    []              ( int index )       { return *Group[ index ]; }
    const TGoMaps&  operator    []              ( int index ) const { return *Group[ index ]; }

                    operator    int             ()  const           { return (int)  Group; }
                    operator    bool            ()  const           { return (bool) Group; }
                    operator    TList<TGoMaps>& ()                  { return Group; }


protected:
    crtl::TList<TGoMaps>    Group;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
