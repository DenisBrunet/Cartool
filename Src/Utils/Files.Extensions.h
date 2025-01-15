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


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Files extensions used in Cartool

#define     FILEEXT_TXT             "txt"

#define     FILEEXT_CI              "ci"
#define     FILEEXT_CSV             "csv"
#define     FILEEXT_DATA            "data"
#define     FILEEXT_BIN             "bin"
#define     FILEEXT_BIOEEG          "eeg"
#define     FILEEXT_BIOMRK          "mrk"
#define     FILEEXT_BVDAT           "dat"
#define     FILEEXT_BVEEG           "eeg"
#define     FILEEXT_BVHDR           "vhdr"
#define     FILEEXT_BVMRK           "vmrk"
#define     FILEEXT_EEG128          "128"
//#define     FILEEXT_EEGASC          "asc"
#define     FILEEXT_EEGBDF          "bdf"
#define     FILEEXT_EEGBIO          FILEEXT_BIOEEG
#define     FILEEXT_EEGBV           FILEEXT_BVEEG
#define     FILEEXT_EEGBVDAT        FILEEXT_BVDAT
#define     FILEEXT_EEGD            "d"
#define     FILEEXT_EEGEDF          "edf"
#define     FILEEXT_EEGEP           "ep"
#define     FILEEXT_EEGEPH          "eph"
#define     FILEEXT_EEGEPSD         "epsd"
#define     FILEEXT_EEGEPSE         "epse"
#define     FILEEXT_EEGMFFDIR       "mff"
#define     FILEEXT_EEGMFF          "bin"
#define     FILEEXT_EEGNSAVG        "avg"
#define     FILEEXT_EEGNSCNT        "cnt"
#define     FILEEXT_EEGNSR          "nsr"
#define     FILEEXT_EEGNSRRAW       "raw"
#define     FILEEXT_EEGRDF          "rdf"
#define     FILEEXT_EEGSEF          "sef"
#define     FILEEXT_EEGTRC          "trc"
#define     FILEEXT_ELS             "els"
#define     FILEEXT_FREQ            "freq"
#define     FILEEXT_IS              "is"
#define     FILEEXT_IS_ASCII        FILEEXT_TXT
#define     FILEEXT_LF              "lf"
#define     FILEEXT_LFT             "lft"
#define     FILEEXT_LM              "lm"
#define     FILEEXT_LOC             "loc"
#define     FILEEXT_MATLABMAT       "mat"
#define     FILEEXT_MRIAVS          "fld"
#define     FILEEXT_MRIAVW_HDR      "hdr"
#define     FILEEXT_MRIAVW_IMG      "img"
#define     FILEEXT_MRIVMR          "vmr"
#define     FILEEXT_MRINII          "nii"
#define     FILEEXT_MRK             "mrk"
#define     FILEEXT_MTG             "mtg"
#define     FILEEXT_NSRRAWGAIN      "gain"
#define     FILEEXT_NSRRAWZERO      "zero"
#define     FILEEXT_RIS             "ris"
#define     FILEEXT_RIS_ASCII       FILEEXT_TXT
#define     FILEEXT_ROIS            "rois"
#define     FILEEXT_SEG             "seg"
#define     FILEEXT_SPINV           "spinv"
#define     FILEEXT_SPIRR           "spi"
#define     FILEEXT_SXYZ            "sxyz"
#define     FILEEXT_TVA             "tva"
#define     FILEEXT_VRB             "vrb"
#define     FILEEXT_XYZ             "xyz"


                                        // Files filters are made from the files extensions

#define     FILEFILTER_TXT          "*." FILEEXT_TXT

#define     FILEFILTER_CI           "*." FILEEXT_CI
#define     FILEFILTER_CSV          "*." FILEEXT_CSV
#define     FILEFILTER_DATA         "*." FILEEXT_DATA
#define     FILEFILTER_BIN          "*." FILEEXT_BIN
#define     FILEFILTER_BIOEEG       "*." FILEEXT_BIOEEG
#define     FILEFILTER_BIOMRK       "*." FILEEXT_BIOMRK
#define     FILEFILTER_BVDAT        "*." FILEEXT_BVDAT
#define     FILEFILTER_BVEEG        "*." FILEEXT_BVEEG
#define     FILEFILTER_BVHDR        "*." FILEEXT_BVHDR
#define     FILEFILTER_BVMRK        "*." FILEEXT_BVMRK
#define     FILEFILTER_EEG128       "*." FILEEXT_EEG128
//#define   FILEFILTER_EEGASC       "*." FILEEXT_EEGASC
#define     FILEFILTER_EEGBDF       "*." FILEEXT_EEGBDF
#define     FILEFILTER_EEGBIO       FILEFILTER_BIOEEG
#define     FILEFILTER_EEGBV        FILEFILTER_BVEEG
#define     FILEFILTER_EEGD         "*." FILEEXT_EEGD
#define     FILEFILTER_EEGEDF       "*." FILEEXT_EEGEDF
#define     FILEFILTER_EEGEP        "*." FILEEXT_EEGEP
#define     FILEFILTER_EEGEPH       "*." FILEEXT_EEGEPH
#define     FILEFILTER_EEGEPSD      "*." FILEEXT_EEGEPSD
#define     FILEFILTER_EEGEPSE      "*." FILEEXT_EEGEPSE
#define     FILEFILTER_EEGMFF       "*." FILEEXT_EEGMFF
#define     FILEFILTER_EEGNSAVG     "*." FILEEXT_EEGNSAVG
#define     FILEFILTER_EEGNSCNT     "*." FILEEXT_EEGNSCNT
#define     FILEFILTER_EEGNSR       "*." FILEEXT_EEGNSR
#define     FILEFILTER_EEGNSRRAW    "*." FILEEXT_EEGNSRRAW
#define     FILEFILTER_EEGRDF       "*." FILEEXT_EEGRDF
#define     FILEFILTER_EEGSEF       "*." FILEEXT_EEGSEF
#define     FILEFILTER_EEGTRC       "*." FILEEXT_EEGTRC
#define     FILEFILTER_ELS          "*." FILEEXT_ELS
#define     FILEFILTER_FREQ         "*." FILEEXT_FREQ
#define     FILEFILTER_IS           "*." FILEEXT_IS
#define     FILEFILTER_IS_ASCII     "*." FILEEXT_IS_ASCII
#define     FILEFILTER_LF           "*." FILEEXT_LF
#define     FILEFILTER_LFT          "*." FILEEXT_LFT
#define     FILEFILTER_LM           "*." FILEEXT_LM
#define     FILEFILTER_LOC          "*." FILEEXT_LOC
#define     FILEFILTER_MATLABMAT    "*." FILEEXT_MATLABMAT
#define     FILEFILTER_MRIAVS       "*." FILEEXT_MRIAVS
#define     FILEFILTER_MRIAVW_HDR   "*." FILEEXT_MRIAVW_HDR
#define     FILEFILTER_MRIAVW_IMG   "*." FILEEXT_MRIAVW_IMG
#define     FILEFILTER_MRIVMR       "*." FILEEXT_MRIVMR
#define     FILEFILTER_MRINII       "*." FILEEXT_MRINII
#define     FILEFILTER_MRK          "*." FILEEXT_MRK
#define     FILEFILTER_MTG          "*." FILEEXT_MTG
#define     FILEFILTER_NSRRAWGAIN   "*." FILEEXT_NSRRAWGAIN
#define     FILEFILTER_NSRRAWZERO   "*." FILEEXT_NSRRAWZERO
#define     FILEFILTER_RIS          "*." FILEEXT_RIS
#define     FILEFILTER_RIS_ASCII    "*." FILEEXT_RIS_ASCII
#define     FILEFILTER_ROIS         "*." FILEEXT_ROIS
#define     FILEFILTER_SEG          "*." FILEEXT_SEG
#define     FILEFILTER_SPINV        "*." FILEEXT_SPINV
#define     FILEFILTER_SPIRR        "*." FILEEXT_SPIRR
#define     FILEFILTER_SXYZ         "*." FILEEXT_SXYZ
#define     FILEFILTER_TVA          "*." FILEEXT_TVA
#define     FILEFILTER_VRB          "*." FILEEXT_VRB
#define     FILEFILTER_XYZ          "*." FILEEXT_XYZ

#define     FILEFILTER_ALL          "*.*"


//----------------------------------------------------------------------------
                                        // various file names part

#define     DefaultFileName         "File"
#define     DefaultMriExt           FILEEXT_MRINII

                                        // Averaging
#define     InfixTrigger            "Trigger"
#define     InfixEpoch              "Epoch"
#define     InfixEpochs             "Epochs"
#define     InfixSum                "Sum"
#define     InfixSumSqr             "SumSqr"
#define     InfixSplit              "Split"
#define     InfixExcl               "Excl"
#define     MarkerOrigin            "Origin"

                                        // Segmentation / Fitting
#define     InfixKMean              "KMeans"
#define     InfixAAHC               "AAHC"
#define     InfixTAAHC              "TAAHC"
#define     InfixError              "error"
#define     FILEEXT_ERRORDATA       InfixError "." FILEEXT_DATA
#define     InfixCartoolWorksheet   "LinesAsSamples"
#define     InfixRWorksheet         "ColumnsAsFactors"
#define     PostfixSpatialFilter    "SpatialFilter"
#define     PostfixGfpMax           "GfpMax"
#define     PostfixSkipBadEpochs    "SkipBads"
#define     PostfixConcat           "Concat"
#define     InfixBestClustering     "BestClustering"
#define     PostfixResampled        "Resampled"
#define     PostfixResampling       "Resampling"
#define     InfixSubsampled         "Subsampled"
#define     InfixSubject            "Subj"
                                        // search for exact infix:
//#define   PostfixSpatialGrep      "\\." PostfixSpatialFilter
                                        // a bit more liberal, in case files have been renamed:
#define     PostfixSpatialGrep      "\\.Spatial"
#define     PostfixEnvelope         "Env"
#define     DirMore                 "More"
#define     DirPreProcData          "PreprocData"
#define     DirDataClusters         "Data Clusters"

                                        // Standardization: File postfixes
#define     PostfixOriginalData                         "Original"
#define     PostfixStandardizationNone                  "NoStand"
#define     PostfixStandardizationZScore                "ZScore"
                                        // Standardization: File factors
#define     InfixStandardizationZScoreFactors           "ZScoreFactors"
#define     InfixStandardizationZScoreFactorsCenter     "ModeLeft"
#define     InfixStandardizationZScoreFactorsSpread     "MADLeft"
                                        // Standardization: Grepping these guys - note that no ending "." is tested, as Z-Score infixes are always completed with some additional letters code
#define     PostfixOriginalDataGrep                     "\\." PostfixOriginalData
#define     PostfixNoStandardizationGrep                "\\." PostfixStandardizationNone
#define     PostfixStandardizationGrep                  "\\." "(" PostfixStandardizationZScore ")" 
#define     InfixStandardizationFactorsGrep             "\\." "(" InfixStandardizationZScoreFactors ")"

                                        // Frequency Analysis
#define     InfixFft                "Fft"
#define     InfixPowerMaps          "PowerMaps"
#define     InfixFftApprox          "FftApprox"
#define     InfixSTransform         "STransform"
#define     InfixBand               "Band"
#define     InfixBands              "Bands"
#define     InfixSeq                "Seq"
#define     InfixAvg                "Avg"
#define     InfixSpectrum           "Spectrum"
#define     InfixApproxEeg          "ApproxEeg"
#define     InfixSplitFreqs         "SplitFrequencies"
#define     InfixSplitElecs         "SplitElectrodes"
#define     InfixWindow             "Window"
#define     InfixReal               "Real"
#define     InfixImag               "Imag"

                                        // Statistics
#define     InfixGroup              "Group"
#define     InfixSubjects           "Subjects"
#define     InfixDelta              "Delta"
#define     InfixMean               "Mean"
#define     InfixMedian             "Median"
#define     InfixJoint              "Joint"
#define     InfixVar                "Variance"
#define     InfixSD                 "SD"
#define     InfixSE                 "SE"
#define     InfixSNR                "SNR"
#define     InfixMad                "MAD"
#define     InfixT                  "t"
#define     InfixP                  "p"
#define     Infix1MinusP            "1-p"
#define     InfixLogP               "-logp"
#define     InfixUnpaired           "Unpaired"
#define     InfixPaired             "Paired"
#define     InfixTTest              "tTest"
#define     InfixRand               "Rand"
#define     InfixTAnova             "TAnova"
#define     InfixDis                "Diss"
#define     InfixSampledDis         "SampledDiss"

                                        // Interpolation
#define     InfixXyzSphere          "Sphere"
#define     InfixFiducial           "Fiducial"
#define     InfixXyzFrom            "From"
#define     InfixXyzTo              "Dest"

                                        // Inverse Solutions
#define     InfixOriginal               "Original"
#define     InfixReoriented             "Reoriented"
#define     InfixSpherized              "Spherized"
#define     InfixCoregistered           "Coregistered"
#define     InfixInverseMN              "MN"
#define     InfixInverseWMN             "WMN"
#define     InfixInverseLoreta          "LORETA"
#define     InfixInverseLaura           "LAURA"
#define     InfixInverseEpifocus        "Epifocus"
#define     InfixInverseSLoreta         "sLORETA"
#define     InfixInverseELoreta         "eLORETA"
#define     InfixInverseDale            "sDale"
#define     InfixResMatrix              "ResolutionMatrix"
#define     InfixResMatrixPointSpread   "PointSpread"
#define     InfixResMatrixKernel        "Kernel"
#define     InfixDefaultEsi             "Esi"
#define     InfixHead                   "Head"
#define     InfixBrain                  "Brain"
#define     InfixCsf                    "CSF"
#define     InfixGrey                   "Grey"
#define     InfixWhite                  "White"
#define     InfixTissues                "Tissues"
#define     InfixSkull                  "Skull"
#define     InfixBiasFieldCorrection    "BFC"
#define     InfixMRI                    "MRI"
#define     InfixSagittal               "Sag"
#define     InfixTransverse             "Tra"
#define     InfixEquatorial             "Equ"
#define     InfixElectrodes             "Electrodes"
#define     InfixHeadModel              "Head Model"
#define     InfixSurface                "Top Head"
#define     InfixDisplay                "For Display"
#define     InfixLeadField              "Lead Field"
#define     InfixTransform              "Transform"
#define     InfixNeighborhood           "Neighbors"
#define     InfixBoundary               "Boundary"
#define     InfixScalarShort            "S"
#define     InfixScalar                 "Scalar"
#define     InfixVectorialShort         "V"
#define     InfixVectorial              "Vectorial"
#define     InfixRegularizationShort    "R"
#define     InfixRegularization         "Regularization"

                                        // Retrieving clusters from fitting, f.ex. file ending as  .Cluster01.sef  or  _class1.eeg
                                        // Note that it allows for heterogenous file naming (mix of _ and .) which we ignore for the moment
                                        // Using a named group to isolate the actual cluster number
#define     InfixClusterGrep            "(\\.|_)(Cluster|class)(?'number'[0-9]+)" AllEegClusterFilesGrep
                                        // Retrieving a specific cluster, two possible versions - not tested
//#define   InfixClusterNGrep(N)        "(\\.|_)(Cluster|class)0*", IntegerToString ( N ), AllEegClusterFilesGrep

                                        // These are quite important Grep strings, to check if either some file names or marker names are epochs deliminters
                                        //  Separators are now optionals, as some cases of concatenation can make them disappear
                                        //  Separators can be either "." or "_":
                                        //      "." being Cartool's preferred way
                                        //      "_" being generated by Windows copy/paste, which produces something like ".Epoch 123_2.sef" from ".Epoch 123.sef"
#define     InfixEpochGrep              "(\\.|_)?" InfixEpoch "s? *[0-9]*" "(\\.|_)?"
#define     InfixEpochsGrep             "(\\.|_)?" InfixEpochs "(\\.|_)?"
#define     InfixConcatGrep             "(\\.|_)?Concat(|enate)s?(\\.|_)?"
                                        // "Epoch" or "Concat" or "Concatenate" strings could be used exchangeably
#define     InfixEpochConcatGrep        "(\\.|_)?" "(Epoch|Concat|Concatenate)s? *[0-9]*" "(\\.|_)?"

                                        // Known extensions don't include locale chars
#define     InfixAnyExtensionGrep       "\\.([A-Za-z]+)$"

                                        // Histograms
#define     InfixHistogramLong      "Histogram"
#define     InfixHistogramShort     "Histo"
#define     InfixCDF                "CDF"
#define     InfixHistogram          InfixHistogramShort " " InfixHistogramLong
                                        // Typical cases: .Histogram .LogHisto .HistogramLog .MaxModeHistogram .CDF
#define     InfixHistogramGrep      "[ _\\.]+(Log|.*Mode.*)? *(Histo|Histogram|CDF) *(Log)?"

#define     InfixToCoregistered     ".To."

#define     InfixTemplate           "Template"


#define     SpecialFilesInfix       InfixT " " InfixP " " Infix1MinusP " " InfixLogP " " InfixMean " " InfixSD " " InfixSE " " InfixDelta " " InfixSpectrum " " InfixApproxEeg
#define     EmptyLmFilename         "Empty." FILEEXT_LM
#define     AllOpenedLmFilename     "All Opened." FILEEXT_LM


//----------------------------------------------------------------------------
                                        // Groups of file extensions

                                        // EEG files per origin
#define     AllRawEegFilesExt       FILEEXT_EEGBV " " FILEEXT_EEGBVDAT " " FILEEXT_EEGTRC " " FILEEXT_EEGBIO " " FILEEXT_EEGNSCNT " " FILEEXT_EEGNSAVG " " FILEEXT_EEGNSR " " FILEEXT_EEGNSRRAW " " FILEEXT_EEGMFF " " FILEEXT_EEGRDF " " FILEEXT_EEGBDF " " FILEEXT_EEGEDF " " FILEEXT_EEG128 " " FILEEXT_EEGD
#define     AllCartoolEegFilesExt   FILEEXT_EEGSEF " " FILEEXT_EEGEPH " " FILEEXT_EEGEP
#define     AllSdExt                FILEEXT_EEGEPSD " " FILEEXT_EEGEPSE
#define     AllEegFilesExt          AllRawEegFilesExt " " AllCartoolEegFilesExt " " AllSdExt
                                        // EEG files per type
#define     AllEegErpFilesExt       FILEEXT_EEGEP " " FILEEXT_EEGEPH " " FILEEXT_EEGNSAVG
#define     AllEegSpontFilesExt     AllRawEegFilesExt " " FILEEXT_EEGSEF


#define     AllFreqFilesExt         FILEEXT_FREQ
#define     AllRisFilesExt          FILEEXT_RIS
#define     AllRisFilesGrep         ".+\\." FILEEXT_RIS "$"
#define     AllDataExt              FILEEXT_DATA
#define     AllErrorDataGrep        ".+\\." InfixError "\\." FILEEXT_DATA "$"
#define     AllSegDataExt           FILEEXT_SEG " " FILEEXT_DATA

                                        // Über-groups of EEGs / tracks
#define     AllEegRisFilesExt       AllEegFilesExt " " AllRisFilesExt
#define     AllEegFreqFilesExt      AllEegFilesExt " " AllFreqFilesExt
#define     AllEegFreqRisFilesExt   AllEegFilesExt " " AllFreqFilesExt " " AllRisFilesExt
#define     AllTracksFilesExt       AllEegFreqRisFilesExt
#define     AllTracksFilesGrep      ".+\\.(" FILEEXT_FREQ "|" FILEEXT_RIS "|" FILEEXT_EEGBV "|" FILEEXT_EEGBVDAT "|" FILEEXT_EEGTRC "|" FILEEXT_EEGBIO "|" FILEEXT_EEGNSCNT "|" FILEEXT_EEGNSAVG "|" FILEEXT_EEGNSR "|" FILEEXT_EEGNSRRAW "|" FILEEXT_EEGMFF "|" FILEEXT_EEGRDF "|" FILEEXT_EEGBDF "|" FILEEXT_EEGEDF "|" FILEEXT_EEG128 "|" FILEEXT_EEGD "|" FILEEXT_EEGSEF "|" FILEEXT_EEGEPH "|" FILEEXT_EEGEP ")$"
#define     AllEegClusterFilesGrep    "\\.(" FILEEXT_EEGBV "|" FILEEXT_EEGSEF "|" FILEEXT_EEGEP "|" FILEEXT_EEGEPH ")$"

                                        // Other useful groups
#define     AllCoordinatesFilesExt  FILEEXT_ELS " " FILEEXT_XYZ
#define     AllCoordinatesFilesGrep ".+\\.(" FILEEXT_ELS "|" FILEEXT_XYZ ")$"
#define     AllSolPointsFilesExt    FILEEXT_SPIRR " " FILEEXT_SXYZ " " FILEEXT_LOC
#define     AllSolPointsFilesGrep   ".+\\.(" FILEEXT_SPIRR "|" FILEEXT_SXYZ "|" FILEEXT_LOC ")$"
#define     AllPointsFilesExt       AllCoordinatesFilesExt " " AllSolPointsFilesExt
#define     AllPointsFilesGrep      ".+\\.(" FILEEXT_ELS "|" FILEEXT_XYZ "|" FILEEXT_SPIRR "|" FILEEXT_SXYZ "|" FILEEXT_LOC ")$"

#define     AllMriFilesExt          FILEEXT_MRINII " " FILEEXT_MRIAVW_HDR " " FILEEXT_MRIAVS " " FILEEXT_MRIVMR
#define     AllMriFilesGrep         ".+\\.(" FILEEXT_MRINII "|" FILEEXT_MRIAVW_HDR "|" FILEEXT_MRIAVS "|" FILEEXT_MRIVMR ")$"
#define     AllMriHeadFilesGrep     ".*" InfixHead  ".*\\.(" FILEEXT_MRINII "|" FILEEXT_MRIAVW_HDR "|" FILEEXT_MRIAVS "|" FILEEXT_MRIVMR ")$"
#define     AllMriBrainFilesGrep    ".*" InfixBrain ".*\\.(" FILEEXT_MRINII "|" FILEEXT_MRIAVW_HDR "|" FILEEXT_MRIAVS "|" FILEEXT_MRIVMR ")$"

#define     AllLeadFieldFilesExt    FILEEXT_LF " " FILEEXT_RIS " " FILEEXT_LFT " " FILEEXT_CSV
#define     AllInverseFilesExt      FILEEXT_IS " " FILEEXT_SPINV
#define     AllZScoreFilesExt       FILEEXT_EEGSEF /*" "FILEEXT_RIS*/
#define     AllLmFilesExt           FILEEXT_LM
#define     AllMarkersFilesExt      FILEEXT_MRK
#define     AllRoisFilesExt         FILEEXT_ROIS
#define     AllSegFilesExt          FILEEXT_SEG
#define     AllTextFilesExt         FILEEXT_TXT


#define     AllCartoolNewFileExt    FILEEXT_LM " " FILEEXT_IS
#define     AllCartoolSaveFileExt   FILEEXT_LM " " AllSolPointsFilesExt " " AllCoordinatesFilesExt " " AllMriFilesExt " " AllInverseFilesExt
#define     AllCommitTracksExt      FILEEXT_EEGEP " " FILEEXT_EEGEPH " " FILEEXT_EEGSEF " " FILEEXT_EEGBV " " FILEEXT_EEGEDF " " FILEEXT_RIS

                                        // File extensions that are associated with others, like pairs of files
#define     TracksBuddyExt          FILEEXT_BVEEG " " FILEEXT_BVDAT " " FILEEXT_BVHDR " " FILEEXT_BVMRK " " FILEEXT_MRK
#define     VolumesBuddyExt         FILEEXT_MRIAVW_HDR " " FILEEXT_MRIAVW_IMG
#define     AllKnownBuddyExt        TracksBuddyExt " " VolumesBuddyExt " " FILEEXT_MRK " " FILEEXT_VRB


//----------------------------------------------------------------------------
                                        // file filters, conveniently grouped for dialogs

#define     AllFilesFilter          "All files|" FILEFILTER_ALL



#define     AllLmFilesFilter        "Link Many files|" FILEFILTER_LM



#define     AllCoordinatesFilesFilter   "Head Coordinates files|" FILEFILTER_ELS ";" FILEFILTER_XYZ

#define     AllSolPointsFilesFilter     "Solution Points files|" FILEFILTER_SPIRR ";" FILEFILTER_SXYZ ";" FILEFILTER_LOC
#define     AllSolPointsFilesTxtFilter  AllSolPointsFilesFilter "|Text files|" FILEFILTER_TXT

#define     AllPointsFilesFilter    "Points files|" FILEFILTER_SPIRR ";" FILEFILTER_SXYZ ";" FILEFILTER_LOC ";" FILEFILTER_ELS ";" FILEFILTER_XYZ


#define     AllLeadFieldFilesFilter "Lead Field files|" FILEFILTER_RIS ";" FILEFILTER_LF ";" FILEFILTER_LFT

#define     AllInverseFilesFilter   "Inverse Solution files|" FILEFILTER_IS ";" FILEFILTER_SPINV

#define     AllInputInverseFilesFilter  "TEXT matrix files|" FILEFILTER_IS_ASCII\
                                        "|BINARY matrix files|*.*"

#define     AllZScoreFilesFilter    "Standardization files|" FILEFILTER_EEGSEF /*";" FILEFILTER_RIS*/

#define     AllMriFilesFilter       "MRI files|" FILEFILTER_MRIAVS ";" FILEFILTER_MRIAVW_HDR ";" FILEFILTER_MRINII ";" FILEFILTER_MRIVMR

#define     AllRoisFilesFilter      "Regions of Interest files|" FILEFILTER_ROIS

#define     AllMriPointsFilesFilter "Transformable files|" FILEFILTER_MRIAVS ";" FILEFILTER_MRIAVW_HDR ";" FILEFILTER_MRINII ";" FILEFILTER_MRIVMR ";" FILEFILTER_SPIRR ";" FILEFILTER_SXYZ ";" FILEFILTER_LOC ";" FILEFILTER_ELS ";" FILEFILTER_XYZ


#define     AllTvaFilesFilter       "Trigger Validation files|" FILEFILTER_TVA

#define     AllMontageFilesFilter   "Montage files|" FILEFILTER_MTG

#define     AllMarkersFilesFilter   "Markers files|" FILEFILTER_MRK

#define     AllTextFilesFilter      "Text files|" FILEFILTER_TXT

#define     AllVerboseFilesFilter   "Verbose files|" FILEFILTER_VRB


#define     AllEegErpFilesFilter    "Evoked Potentials files|" FILEFILTER_EEGEPH ";" FILEFILTER_EEGEP ";" FILEFILTER_EEGNSAVG

#define     AllEegSpontFilesFilter  "Spontaneous EEG files|" FILEFILTER_EEGNSR ";" FILEFILTER_EEGMFF ";" FILEFILTER_EEGBDF ";" FILEFILTER_EEGEDF\
                                    ";" FILEFILTER_EEGTRC ";" FILEFILTER_EEGNSCNT ";" FILEFILTER_EEGBIO ";" FILEFILTER_EEG128\
                                    ";" FILEFILTER_EEGD ";" FILEFILTER_EEGNSRRAW ";" FILEFILTER_EEGRDF ";" FILEFILTER_EEGBV ";" FILEFILTER_BVDAT ";" FILEFILTER_EEGSEF
                                        // all EEG files in 1 slot
#define     AllEegFilesFilter       "All EEG files|" FILEFILTER_EEGEPH ";" FILEFILTER_EEGEP ";" FILEFILTER_EEGNSAVG\
                                    ";" FILEFILTER_EEGNSR ";" FILEFILTER_EEGMFF ";" FILEFILTER_EEGBDF ";" FILEFILTER_EEGEDF\
                                    ";" FILEFILTER_EEGTRC ";" FILEFILTER_EEGNSCNT ";" FILEFILTER_EEGBIO ";" FILEFILTER_EEG128\
                                    ";" FILEFILTER_EEGD ";" FILEFILTER_EEGNSRRAW ";" FILEFILTER_EEGRDF ";" FILEFILTER_EEGBV ";" FILEFILTER_BVDAT ";" FILEFILTER_EEGSEF

#define     AllFreqFilesFilter      "Frequency files|" FILEFILTER_FREQ

#define     AllRisFilesFilter       "Results of Inverse Solution files|" FILEFILTER_RIS

#define     AllSegFilesFilter       "Segmentation files|" FILEFILTER_SEG

#define     AllSdFilesFilter        "Standard Error files|" FILEFILTER_EEGEPSD ";" FILEFILTER_EEGEPSE

//#define   AllOtherTracksFilter    "Other tracks files|" FILEFILTER_SEG ";" FILEFILTER_DATA   // not processing .seg files
#define     AllOtherTracksFilter    "Error.Data files|" FILEFILTER_DATA

#define     AllMatlabFilesFilter    "MATLAB mat files|" FILEFILTER_MATLABMAT



#define     AllErpEegFilesFilter        AllEegFilesFilter "|" AllEegErpFilesFilter "|" AllEegSpontFilesFilter

#define     AllErpRisFilesFilter        AllEegErpFilesFilter "|" AllRisFilesFilter

#define     AllErpEegRisFilesFilter     AllEegFilesFilter "|" AllEegErpFilesFilter "|" AllEegSpontFilesFilter "|" AllRisFilesFilter

#define     AllErpEegFreqRisFilesFilter AllEegFilesFilter "|" AllEegErpFilesFilter "|" AllEegSpontFilesFilter "|" AllFreqFilesFilter "|" AllRisFilesFilter

#define     AllTracksRoisFilesFilter    AllEegFilesFilter "|" AllEegErpFilesFilter "|" AllEegSpontFilesFilter "|" AllRisFilesFilter "|" AllCoordinatesFilesFilter "|" AllSolPointsFilesFilter

#define     AllUsualFilesFilter     "Files for Maps display|" FILEFILTER_EEGEPH ";" FILEFILTER_EEGEP ";" FILEFILTER_EEGNSAVG ";" FILEFILTER_XYZ ";" FILEFILTER_ELS\
                                    "|Files for Inverse display|" FILEFILTER_EEGEP ";" FILEFILTER_EEGEPH ";" FILEFILTER_XYZ ";" FILEFILTER_ELS ";" FILEFILTER_IS ";" FILEFILTER_SPINV ";" FILEFILTER_SPIRR ";" FILEFILTER_SXYZ ";" FILEFILTER_LOC ";" FILEFILTER_MRIAVS ";" FILEFILTER_MRIAVW_HDR ";" FILEFILTER_MRINII ";" FILEFILTER_MRIVMR\
                                    "|Files for RIS display|" FILEFILTER_RIS ";" FILEFILTER_SPIRR ";" FILEFILTER_SXYZ ";" FILEFILTER_LOC ";" FILEFILTER_MRIAVS ";" FILEFILTER_MRIAVW_HDR ";" FILEFILTER_MRINII ";" FILEFILTER_MRIVMR\
                                    "|" AllEegErpFilesFilter "|" AllEegSpontFilesFilter



#define     FileOpenFilesFilter     AllFilesFilter "|" AllLmFilesFilter "|" AllTracksRoisFilesFilter "|" AllFreqFilesFilter "|" AllSegFilesFilter\
                                    "|" AllMriFilesFilter "|" AllInverseFilesFilter "|" AllRoisFilesFilter



//----------------------------------------------------------------------------
                                        // Files magic numbers used in Cartool
                                        // Use IsMagicNumber function for testing
                                        // 'ABCD' format is an int32 and needs to be swapped before writing or testing
                                        // "ABCD" format is a string and does not to be swapped, it is stored as it appears here
#define     RISBIN_MAGICNUMBER1     SwapBytes ( 'RI01' )

#define     ISBIN_MAGICNUMBER1      SwapBytes ( 'IS01' )
#define     ISBIN_MAGICNUMBER2      SwapBytes ( 'IS02' )
#define     ISBIN_MAGICNUMBER3      SwapBytes ( 'IS03' )

#define     TLTXT_MAGICNUMBER2      "TL02"
#define     TLBIN_MAGICNUMBER2      SwapBytes ( 'TL02' )

#define     TVATXT_MAGICNUMBER1     "TV01"

#define     MTGTXT_MAGICNUMBER1     "MT01"

#define     SEFBIN_MAGICNUMBER1     SwapBytes ( 'SE01' )

#define     ELSTXT_MAGICNUMBER1     "ES01"

#define     FREQBIN_MAGICNUMBER1    SwapBytes ( 'FR01' )
#define     FREQBIN_MAGICNUMBER2    SwapBytes ( 'FR02' )

#define     ROITXT_MAGICNUMBER1     "RO01"


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
