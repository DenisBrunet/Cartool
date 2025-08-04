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

#include    "Files.PreProcessFiles.h"

#include    "CartoolTypes.h"            // GfpPeaksDetectType BackgroundNormalization EpochsType SkippingEpochsType ZScoreType
#include    "Strings.Utils.h"
#include    "Files.Stream.h"
#include    "Math.Resampling.h"
#include    "TMarkers.h"
#include    "TFilters.h"
#include    "BadEpochs.h"
#include    "TRois.h"
#include    "ESI.TissuesConductivities.h"
#include    "TExportTracks.h"

#include    "TTracksDoc.h"
#include    "TInverseMatrixDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------

char*       GetInverseInfix (   const TInverseMatrixDoc*    ISDoc,  RegularizationType  regularization, AtomType            datatype,
                                char*                       infix 
                            )
{
if ( infix == 0 )
    return  "";

ClearString ( infix );

if ( ISDoc == 0 )
    return  infix;
                                        // preprocessing inverse name
                                        // tells it's an inverse, either the inverse name or the generic "ESI" term
ISDoc->GetInverseName   ( infix );

                                        // specifying the number of SPs
StringAppend            ( infix, " ", IntegerToString ( ISDoc->GetNumSolPoints () ) );

                                        // Regularization
StringAppend            ( infix, " ", InfixRegularizationShort );
RegularizationToString  ( regularization, StringEnd ( infix ), true );

                                        // then Vectorial or Scalar
StringAppend            ( infix, IsVector ( datatype ) ? InfixVectorialShort : InfixScalarShort );


return  infix;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Progress bar's number of steps
int         PreProcessFilesGaugeSize ( int numconditions )
{
return  AtLeast ( 1, numconditions ) * 10;
}


//----------------------------------------------------------------------------
                                        // Take a Group of Files, and preprocess/Convert them to another group of files (on disk)
                                        // Output:
                                        //   - .sef or .ris group(s) of file(s)
                                        //   - ZScore factors, for each group (same values, though)
                                        //   - returns the new files names + the new base file names

                                        // Time cropping, epochs pipe-line:
                                        //   - 1 epoch whole time interval / periodic epochs / list of epochs
                                        //   - keep current epoch / Gfp Peaks
                                        //   - keep current markers / exclude bads
                                        //   - keep current markers

                                        // Note: this function has some kind of overlap with the  ReprocessTracks  one, but is not designed for the same use

void        PreProcessFiles (   const TGoF&             gofin,              AtomType                datatypeout,
                                DualDataType            dualdata,           const TGoF*             dualgofin,
                                SpatialFilterType       spatialfilter,      const char*             xyzfile,
                                bool                    computeesi,         TInverseMatrixDoc*      ISDoc,          RegularizationType  regularization,     RegularizationType* usedregularization,
                                bool                    mergecomplex,
                                bool                    gfpnormalize,
                                BackgroundNormalization backnorm,           ZScoreType              zscoremethod,   const char*         zscorefile,
                                bool                    ranking,
                                bool                    thresholding,       double                  threshold,
                                FilterTypes             envelope,           double                  envelopeduration,
                                const TRois*            rois,               FilterTypes             roimethod,
                                EpochsType              epochs,             const TStrings*         epochfrom,      const TStrings*     epochto,
                                GfpPeaksDetectType      gfppeaks,           const char*             listgfppeaks,
                                SkippingEpochsType      badepochs,          const char*             listbadepochs,  double              badepochstolerance,
                                const char*             outputdir,
                                const char*             fileprefix,
                                int                     clipfromname,       int                     cliptoname,
                                bool                    createtempdir,      char*                   temppath,
                                bool                    savemainfiles,      TGoGoF&                 gogofout,       TGoGoF*             dualgogofout,       TGoF&               gofoutdir,      bool&       newfiles,
                                bool                    savezscore,         TGoF*                   zscoregof,
                                TSuperGauge*            gauge
                            )

{
if ( usedregularization )   *usedregularization = regularization;

gogofout.Reset ();

if ( dualgogofout )  dualgogofout->Reset ();

gofoutdir.Reset ();

newfiles    = false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Parameters no-nonsense check
if ( spatialfilter != SpatialFilterNone && StringIsEmpty ( xyzfile ) )

    spatialfilter   = SpatialFilterNone;


if ( computeesi && ISDoc == 0 ) {
    if ( Cartool.IsInteractive () )
        ShowMessage ( "There seems to be a problem with your Inverse Matrix!\nCheck your input file and come back...", "Data Preprocessing", ShowMessageWarning );
    return;
    }

                                        // for safety
if ( ! computeesi )

    regularization  = RegularizationNone;

                                        // it is totally useless to gfp normalize before Z-Scoring tracks - maybe show some warning, too?
if ( gfpnormalize && backnorm != BackgroundNormalizationNone )

    gfpnormalize    = false;


if ( backnorm == BackgroundNormalizationLoadingZScoreFile && ! CanOpenFile ( zscorefile ) )

    backnorm    = BackgroundNormalizationComputingZScore;


if ( backnorm != BackgroundNormalizationNone && IsZScore ( zscoremethod ) ) {

    ClearZScoreOptions ( zscoremethod );
                                        // upgrade with some options of mine
    zscoremethod    = SetZScore ( zscoremethod, 
                                  (ZScoreType) ( ZScoreAllData | ( mergecomplex ? ZScoreDimension6 : ZScoreDimension3 ) ) );
    }


if ( savezscore && backnorm == BackgroundNormalizationNone )
                                        // no z-score to be saved
    savezscore  = false;


if ( thresholding && threshold == 0 )
    thresholding    = false;


CheckFilterTypeEnvelope ( envelope );

if ( envelope != FilterTypeNone && envelopeduration <= 0 )

    envelope    = FilterTypeNone;


if ( rois && ! ( roimethod == FilterTypeMean || roimethod == FilterTypeMedian ) )

//  rois        = 0;                // reset ROIs?
    roimethod   = FilterTypeMean;   // or correct caller?


if ( gfppeaks == GfpPeaksDetectionList && StringIsEmpty ( listgfppeaks ) )

    gfppeaks    = GfpPeaksDetectionAuto;


if ( badepochs == SkippingBadEpochsList && StringIsEmpty ( listbadepochs ) ) {

    badepochs           = SkippingBadEpochsAuto;
    badepochstolerance  = AtLeast ( BadEpochsToleranceDefault, badepochstolerance );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // per file computation - not needed if not saved
bool                computefiles        =   savemainfiles;

                                        // processings that need to scan all data beforehand:
bool                subsamplesallfiles  =   gfpnormalize        
                                        ||  regularization  == RegularizationAutoGlobal 
                                        ||  regularization  == RegularizationAutoLocal
                                        ||  backnorm        == BackgroundNormalizationComputingZScore;

bool                timelinedisrupted   =   gfppeaks        != NoGfpPeaksDetection;

                                        // processings that alter the timeline:
bool                timecropping        =   epochs          == EpochsFromList 
                                        ||  epochs          == EpochsPeriodic
                                        ||  gfppeaks        != NoGfpPeaksDetection 
                                        ||  badepochs       != NoSkippingBadEpochs;

                                        // all processings:
bool                ispreprocessing     =   spatialfilter   != SpatialFilterNone
                                        ||  computeesi      
                                        ||  gfpnormalize        
                                        ||  backnorm        == BackgroundNormalizationComputingZScore
                                        ||  ranking
                                        ||  thresholding
                                        ||  envelope        != FilterTypeNone
                                        ||  rois        
                                        ||  savezscore
                                        ||  timecropping;

                                        // going through all files
int                 numprocfiles        =   gofin.NumFiles ();
                                        // extra files needed for subsampling - !must be handled before the regular files!
int                 numextrafiles       = ! subsamplesallfiles  ? 0
                                        :   mergecomplex        ? 2
                                                                : 1;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Expand current range for preprocessing and Z-Scoring
                                        // Drawback is that it can seems the progress bar is going "backward" a little bit
if ( gauge )    gauge->SetRange ( SuperGaugeDefaultPart, gauge->GetRange ( SuperGaugeDefaultPart ) 
                                                        + ( subsamplesallfiles  ? gofin.NumFiles () : 0 )
                                                        + ( numextrafiles       ? numextrafiles * 8 : 0 ) );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Files check

                                        // returned to caller if new files were created
newfiles    = ispreprocessing;

                                        // these 2 go together
if ( dualdata && ! dualgofin )
    dualdata    = NoDualData;

                                        // force reset dualgogofout if not used - this enforces dualdata + dualgofin if dualgogofout
if ( ! dualdata && dualgogofout )
    dualgogofout    = 0;


if ( ! ispreprocessing ) {
                                        // copy input, no change is done
    gogofout    .Add ( &gofin, true, MaxPathShort );

    if ( dualgogofout )
        dualgogofout->Add ( dualgofin, true, MaxPathShort );

    gofoutdir   .Add ( outputdir,   MaxPathShort );

    return;
    }

                                        // copy input group of files
TGoF                gof ( gofin );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TMaps               Data;
TMaps               ESI;
TMaps               ESIf;
TMaps               ROIs;
TMaps*              ToData;             // pointer to data to actually process: either EEG or Inverse
//TMaps             DataNoZScore;
TTracks<float>      ZScoreFactors;
double              gfpnorm;


int                 numeegelec          = gof.GetNumElectrodes ();
                                        // if not found, returns 0, which will use the duration as time frames
double              samplingfrequency   = gof.GetSamplingFrequency ();

//int                 maxnumtf            = gof.GetMaxNumTF ( true );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Inverse Matrix preprocessing
TFileName           isfilename;
char                buff                [ 256 ];
int                 numiselec           = 0;
int                 numsp               = 0;
//                  regularization      = computeesi && ISDoc.IsOpen () ? Clip ( regularization, 0, ISDoc->GetNumRegularizations () - 1 ) : 0;  // trust caller


if ( computeesi && ISDoc ) {

    numiselec           = ISDoc->GetNumElectrodes ();
    numsp               = ISDoc->GetNumSolPoints  ();

    GetInverseInfix ( ISDoc, regularization, datatypeout, isfilename );
    }
else
    regularization      = RegularizationNone;   // otherwise, trust caller, as it can be values with special meaning


if ( usedregularization )   *usedregularization = regularization;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // is ROIs dimension compatible with current dimensions (EEG or ESI)?
if ( rois ) {

    if      (   computeesi && rois->GetDimension () != numsp ) {
        if ( Cartool.IsInteractive () )
            ShowMessage ( "Your ROIs dimension does not fit to your Inverse Matrix's Solution Points.\nPlease check your ROIs file and come back...", "Data Preprocessing", ShowMessageWarning );
        return;
        }

    else if ( ! computeesi && rois->GetDimension () != numeegelec ) {
        if ( Cartool.IsInteractive () )
            ShowMessage ( "Your ROIs dimension does not fit to your EEG number of electrodes.\nPlease check your ROIs file and come back...", "Data Preprocessing", ShowMessageWarning );
        return;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           infix;
TFileName           infixbase;
TFileName           filenamein;
TFileName           filenameoutprefix;
TFileName           filenameout;
TFileName           filenameoutalt;
TFileName           filemrk;
TFileName           tempdir;
TFileName           fileext;
TFileName           concatfile;
TFileName           concatfilef;


bool                allris              = gof.AllExtensionsAre ( AllRisFilesExt );

                                            // /*prevent extension to be ris if ROIs are applied, which would lose the ROIs names*/
StringCopy ( fileext, ( computeesi || allris ) /*&& ! rois*/ ? FILEEXT_RIS : FILEEXT_EEGSEF );


AtomType            saveddatatypeout    = datatypeout;

                                            // just to be sure
CreatePath      ( outputdir, false );


if ( createtempdir ) {

    if ( StringIsEmpty ( temppath ) )   GetTempFileName ( tempdir );                                // generate a temp dir
    else                                StringCopy      ( tempdir, ToLastDirectory ( temppath ) );  // use a given temp dir
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Do a sub-sampling of all the data: 1 GoF -> 1 file
if ( subsamplesallfiles ) {

                                        // we might not need the same amount of resampling
                                        // regularization might downsample again if the amount is still too much, f.ex. from Z-Score
    int                 DownsamplingTargetSize  = max ( gfpnormalize                                        ? DownsamplingTargetSizeGfp     : 0,
                                                        regularization == RegularizationAutoGlobal          ? DownsamplingTargetSizeReg     : 0,
                                                        backnorm == BackgroundNormalizationComputingZScore  ? DownsamplingTargetSizeZScore  : 0 );


                              // For frequencies, subsampling is done in 2 files, one Real and one Imaginary
    TDownsampling       downtf ( gof.GetSumNumTF () / ( mergecomplex ? 2 : 1 ), DownsamplingTargetSize );


    TMaps               submaps  (                downtf.NumDownData,                    numeegelec     );
    TMaps               submapsf ( mergecomplex ? downtf.NumDownData : 0, mergecomplex ? numeegelec : 0 );
    int                 numsubmaps      = 0;
    int                 numsubmapsf     = 0;


    for ( int fi = 0; fi < gof.NumFiles (); fi++ ) {

        if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );


        Data.ReadFile ( gof[ fi ], 0, AtomTypeScalar /*datatype*/, ReferenceAsInFile );

                                        // !split in 2 here, as each loop has to test its own overflowing limit!
        if ( ! mergecomplex || mergecomplex && IsEven ( fi ) )
                                        // the total number of submaps was computed on the concatenation, be careful on the per-file scan to not overflow
            for ( int mi = 0; mi < Data.GetNumMaps () && numsubmaps  < downtf.NumDownData; mi += downtf.Step )

                submaps [ numsubmaps ++ ]   = Data[ mi ];

        else // mergecomplex && IsOdd

            for ( int mi = 0; mi < Data.GetNumMaps () && numsubmapsf < downtf.NumDownData; mi += downtf.Step )

                submapsf[ numsubmapsf++ ]   = Data[ mi ];
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // write the sub-sampled data to a single file
    StringCopy              ( concatfile, gof[ 0 ] );
    RemoveExtension         ( concatfile );
    PrefixFilename          ( concatfile, fileprefix );
    StringAppend            ( concatfile, "." InfixSubsampled ".", StringRandom ( buff, 6 ) );
    if ( mergecomplex )
        StringAppend        ( concatfile, "." InfixReal );

    AddExtension            ( concatfile, FILEEXT_EEGSEF /*fileext*/ );

    ReplaceDir              ( concatfile, outputdir );  // currently not using optional tempdir

    submaps.WriteFile       ( concatfile, false, 0 );


    if ( mergecomplex ) {
        StringCopy          ( concatfilef, concatfile );
        StringReplace       ( concatfilef, "." InfixReal, "." InfixImag );
        submapsf.WriteFile  ( concatfilef, false, 0 );
        }

    } // subsamplesallfiles


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FctParams           p;
bool                istempfile;
bool                isfirstfile;

                                        // Loop through all files + optionally adding a first one for the Normalization / Standardization
for ( int fi = - numextrafiles; fi < numprocfiles; fi++ ) {

                                        // is that this very special subsampled file we want to process first?
    istempfile      = subsamplesallfiles && fi < 0;

                                        // is that the first regular file?
    isfirstfile     =    ! mergecomplex && fi == 0 
                      ||   mergecomplex && fi <= 1;

                                        // Free any existing data
    Data.DeallocateMemory ();
    ESI .DeallocateMemory ();
    ROIs.DeallocateMemory ();
//  ToData  = 0;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // load a single file at once, WITHOUT reference, and WITHOUT positive flag
    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );

                                        // Either load 1 or 2 temp concatenated file(s), or the the next input file
    StringCopy      ( filenamein, istempfile ? ( fi == -1 ? (char*) concatfile : (char*) concatfilef )
                                             : gof[ fi ]                                                );

                                        // reading data only if really needed
    if ( computefiles || istempfile )

        Data.ReadFile   ( filenamein, 0, AtomTypeScalar /*datatype*/, ReferenceAsInFile );

    numeegelec  = Data.GetDimension ();
                                        // this might be changed by the preprocessing, so just make sure we start each file anew
    datatypeout = saveddatatypeout;

    infix.Clear ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Output directory / file name cooking

    StringCopy      ( filenameoutprefix, filenamein );

    RemoveExtension ( filenameoutprefix );

    if ( mergecomplex ) {
        StringReplace ( filenameoutprefix, "." InfixReal, "" );
        StringReplace ( filenameoutprefix, "." InfixImag, "" );
        }

    if ( clipfromname >= 0 && cliptoname >= 0 )
        filenameoutprefix.ClipFileName ( clipfromname, cliptoname );

                                        // optional prefix, unclipped
    PrefixFilename  ( filenameoutprefix, fileprefix );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Read only the markers, and not working on multisessions - OK for the moment
                                        // If triggers are needed, then the ReadNativeMarkers functions should be called somehow
    TMarkers            markers;

    StringCopy          ( filemrk, filenamein  );
    AddExtension        ( filemrk, FILEEXT_MRK );

    markers.ReadFile    ( filemrk );

//  filemrk.Show ( "markers: input markers list" );
//  markers.Show ( "markers: input markers list" );

                                        // read per file, or keep the group value?
//  ReadFromHeader ( filenamein, ReadSamplingFrequency,    &samplingfrequency );
//  Maxed ( samplingfrequency, 0.0 );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );


    if ( spatialfilter != SpatialFilterNone ) {

        infix  += "." PostfixSpatialFilter;

        Data.FilterSpatial ( SpatialFilterDefault, xyzfile );
        } // spatialfilter


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );


    if ( computeesi )
                                        // do this inconditionnally
        infix  += "." + isfilename;

                                        // !skip any of this if data was not read!
    if ( computeesi && Data.IsAllocated () ) {
                                        // compatible EEG & IS? (should be tested before)
        if ( numeegelec != numiselec ) {
            if ( Cartool.IsInteractive () )
                ShowMessage ( "Your Inverse Matrix does not fit your EEG data.\nCheck the dimensions of your files and come back...", "Data Preprocessing", ShowMessageWarning );
            return;
            }

                                        // doing the real job here - also note that the ESI variable will be allocated there
        RegularizationType  usedreg     = Data.ComputeESI ( ISDoc, regularization, IsVector ( datatypeout ), ESI );

                                        // !RegularizationAutoLocal will not update regularization!
        if ( regularization == RegularizationAutoGlobal ) {
                                        // overriding should be done only once - even for frequencies
            regularization  = usedreg;
                                        // update returned parameter
            if ( usedregularization )   *usedregularization = usedreg;
            }

                                        // trust caller, and merge files 2 by 2 (real and imaginary parts are in alternating files)
        if ( mergecomplex ) {
                                        // first part (Real) of frequency?
            if ( IsEven ( fi ) ) {      
                                        // save to temp variable
                ESIf    = ESI;          
                                        // proceed to the imaginary part
                continue;
                }
                                        // Here we have already done the Real part, and Imaginary is the current results


            for ( int mi  = 0; mi  < ESI.GetNumMaps ();    mi++   )    // conditions could have different lengths, but real & imaginary must absolutely be equal
            for ( int spi = 0; spi < ESI.GetDimension ();  spi++  )
                                        // merge the 2 consecutive files into the first one: end up with 1 norm, computed from 6 dimensions
                ESI[ mi ][ spi ]    = sqrt ( Square ( ESIf[ mi ][ spi ] ) + Square ( ESI[ mi ][ spi ] ) );


            ESIf.DeallocateMemory ();

                                        // ?remove the imaginary parts from the maps, so we are clean for any further normalization and stuff?
//            gof.Remove ( gof[ fi ] );
//            fi--; // !to isfirstfile flag in that case!

            } // mergecomplex

                                        // free Data memory now, EEG is no longer needed
        Data.DeallocateMemory ();

                                        // we will proceed with the results of the inverse, though technically not and EEG anymore...
        ToData      = &ESI;
        } // computeesi
    else                                // will use the EEG data
        ToData      = &Data;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // From here, processings only make use of ToData
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );

                                        // Should be exclusive with BackgroundNormalizationComputingZScore BTW
    if ( gfpnormalize ) {

        infix  += "." "GfpNorm";


        if ( istempfile )
                                        // compute factor on the subsampled temp file, using the BACKGROUND activity / Max Mode of distribution
            gfpnorm     = ToData->ComputeGfpNormalization ( datatypeout );
        else
                                        // not the first file, apply factor we computed on the concatenated file
            ToData->ApplyGfpNormalization   ( gfpnorm );

        } // gfpnormalize


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Z-Score is done on the whole group (all conditions within subject) at once

    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );


    if ( backnorm != BackgroundNormalizationNone ) {

                                        // allocate and save data before Z-Score, for dual data (could be EEG or ESI)
    //  DataNoZScore  = *ToData;

                                        // computing factors from subsampled file?
        if      ( backnorm == BackgroundNormalizationComputingZScore   && istempfile )

            ToData->ComputeZScore ( zscoremethod, ZScoreFactors );
                                        // reloading the Z-Score factors?
        else if ( backnorm == BackgroundNormalizationLoadingZScoreFile && ZScoreFactors.IsNotAllocated () )
                                        // at this point, this is the responsibility of caller to make sure this file exists and is the proper one!
            ZScoreFactors.ReadFile ( zscorefile );

                                        // only on real data, not the temp ones
        if ( ! istempfile )

            ToData->ApplyZScore   ( zscoremethod, ZScoreFactors );


                                        // !in these cases, results are now signed!
        if ( GetZScoreProcessing  ( zscoremethod ) == ZScorePositive_CenterScale
          || GetZScoreProcessing  ( zscoremethod ) == ZScoreSigned_CenterScale   )
            datatypeout     = AtomTypeScalar;


        infix  += "." + (TFileName) ZScoreEnumToInfix ( zscoremethod );

        } // BackgroundNormalizationNone


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we're done with the subsampled temp file, we now have the factors we needed, continue to the first real input file
    if ( istempfile ) {
                                        // Clean-up concatenated file(s)
        DeleteFileExtended ( concatfile  );
        DeleteFileExtended ( concatfilef );

        continue; // to next file
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Ranking
    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );


    if ( ranking ) {
                                        // used for merging ESI templates with different distributions, which impacts the min-max
                                        // handles both vectorial and scalar data
                                        // MUST account for all data
        ToData->ToRank  ( datatypeout, RankingOptions ( RankingAccountNulls | RankingCountIdenticals ) );


        infix  += "." "Rank";
        } // BackgroundNormalizationNone


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Thresholding
    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );


    if ( thresholding ) {

        ToData->Thresholding ( threshold, datatypeout );


        infix  += "." "Clip" + FloatToString ( threshold, 2 );
        } // BackgroundNormalizationNone


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // For EEG, Analytic is best only if not rectified data, followed by PeakToPeak, which handles all cases
                                        // For ESI, PeakToPeak is best on Norm. Vectorial case is not currently handled. Analytic doens't work at all.
                                        // Also for ESI + Standardization, results will of course no longer have a Normal distribution
    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );


    if ( envelope != FilterTypeNone ) {
                                        // in milliseconds
        p ( FilterParamEnvelopeWidth )  = envelopeduration;


        ToData->FilterTime ( envelope, p, false );

                                        // we have positive data now...
        datatypeout     = AtomTypePositive;


        infix  += "." PostfixEnvelope + IntegerToString ( envelopeduration );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );

                                        // !ROIs don't compute on vectorial data - caller should enforce that!
    if ( rois ) {
                                        // does the dirty job - also ROIs will be allocated there
        rois->Average ( *ToData, ROIs, roimethod );

                                        // !could either point to GoData or GoESI!
        ToData->DeallocateMemory ();

                                        // we will proceed with the results of the ROIs
        ToData      = &ROIs;

                                        // re-apply these 2 options to always return ranked and/or thresholded data(?)
        if ( ranking )

            ToData->ToRank  ( datatypeout, RankingOptions ( RankingAccountNulls | RankingCountIdenticals ) );

        if ( thresholding )

            ToData->Thresholding ( threshold, datatypeout );


        infix  += ".ROIS" + IntegerToString ( rois->GetNumRois () );
      //infix  += "." + (TFileName) rois->GetName () + "." + IntegerToString ( rois->GetNumRois () );
        } // rois


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( gauge )    gauge->Next ( -1, SuperGaugeUpdateTitle );


    TMarkers            epochlist;

                                        // handles all cases: whole time range / periodic epochs / list of epochs
    epochlist.EpochsToMarkers   (   epochs, 
                                    epochfrom,  epochto, 
                                    0,          ToData->GetNumMaps () /*maxnumtf*/ - 1, 
                                    (long)      0 /*periodicsize*/ 
                                );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute whole GFP at once
    TVector<double>     gfp;

    if ( gfppeaks == GfpPeaksDetectionAuto )

        ToData->ComputeGFP ( gfp, IsAbsolute ( datatypeout ) ? ReferenceNone : ReferenceAverage, datatypeout );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Generate a clipping list of intervals, from parts to be excluded
    TMarkers            badepochslist;


    if      ( badepochs == SkippingBadEpochsAuto ) {
                                        // gracefully extract the bad epochs
        badepochslist.BadEpochsToMarkers    (   ToData,     0,      0, 
                                                badepochstolerance, 
                                                MarkerNameAutoBadEpoch,
                                                0
                                            );
        }

    else if ( badepochs == SkippingBadEpochsList ) {
                                        // use list given by user
                                        // better than  InsertMarkers  as it will consolidate overlapping epochs
        badepochslist.MarkersToTimeChunks ( markers,                listbadepochs,          KeepingMarkers, 
                                            0,                      ToData->GetNumMaps () /*maxnumtf*/ - 1, 
                                            MarkerNameAutoBadEpoch 
                                            );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    TMarkers            goodepochslist;
    TMarkers            writingepochslist;


    int                 fromtf;
    int                 totf;
//  TFileName           gfppeaksmrk;

                                        // epochlist can be the whole time range in the simplest scenario
                                        // !force entering loop even if empty, to actually update directory & gogofout!
    for ( int epoch = 0; epoch < AtLeast ( 1, (int) epochlist ); epoch++ ) {

        if ( gauge )    gauge->Actualize ();


        fromtf      = epoch < (int) epochlist ? epochlist[ epoch ]->From : 0;
        totf        = epoch < (int) epochlist ? epochlist[ epoch ]->To   : 0;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        goodepochslist.ResetMarkers ();


        if      ( gfppeaks == NoGfpPeaksDetection )
                                        // generate a single epoch
            goodepochslist.AppendMarker ( TMarker ( fromtf, totf, 0, MarkerNameBlock, MarkerTypeTemp ) );


        else if ( gfppeaks == GfpPeaksDetectionAuto )
                                        // gracefully extract the current epoch for the local maxes, restricted to the current epoch
                                        // use previously processed file (filters, ESI...)
            goodepochslist.MaxTrackToMarkers ( gfp, fromtf, totf, true, MarkerNameAutoMaxGfp );


        else if ( gfppeaks == GfpPeaksDetectionList ) {
                                        // use list given by user
            goodepochslist.InsertMarkers ( markers, listgfppeaks );

            goodepochslist.KeepMarkers   ( fromtf, totf );

            } // GfpPeaksDetectionList

                                        // Here goodepochslist contains the list of Gfp Peaks, clipped to current epoch time window

                                        // In case there is no more markers, still proceed by outputting an empty file
                                        // This is a utility function, it is better to output something even empty and rather have the caller check that
//      if ( goodepochslist.IsEmpty () ) {
//          if ( Cartool.IsInteractive () )
//              ShowMessage ( "No markers were found, resulting in an empty extracted file.\nPlease check your files and markers and come back...", "Data Preprocessing", ShowMessageWarning );
//          return;
//          } 
            

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // excluding some time chunks?
        if ( badepochs != NoSkippingBadEpochs ) {
                                        // does nothing if list is empty...
            if ( gfppeaks == NoGfpPeaksDetection )  goodepochslist.ClipMarkers   ( badepochslist, AllMarkerTypes );   
            else                                    goodepochslist.RemoveMarkers ( badepochslist, AllMarkerTypes ); // we could simply use ClipMarkers here, too
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // let's be consistent and writing in the same order as the input
        goodepochslist.SortAndCleanMarkers ();

                                        // these are the markers to be written
        writingepochslist   = goodepochslist;


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Same remark as above: save an empty file, get the caller do the testing
//      if ( writingepochslist.IsEmpty () ) {
//          if ( Cartool.IsInteractive () )
//              ShowMessage ( "There appear to be no remaining time points...\nConsider checking this:\n\t-No GFP Peaks marker were found due to erroneous marker names\n\t-Too much BAD epochs\n\t-Too small epochs\n\nPlease check your parameters and come back...", "Data Preprocessing", ShowMessageWarning );
//          return;
//          }

//      writingepochslist.InsertMarkers ( writingepochslist );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // File name cooking
        if ( gauge )    gauge->Actualize ();

                                        // adding processing dependent infix
        filenameout = filenameoutprefix + infix;

                                        // adding optional temporal postfix
        if ( epochs     == EpochsFromList 
          || epochs     == EpochsPeriodic       )   filenameout += "." + IntegerToString ( fromtf, 0 ) + "_" + IntegerToString ( totf, 0 ); // !separator "_" is used to retrieve epoch (for verbose purpose only) in segmentation!
        if ( gfppeaks   != NoGfpPeaksDetection  )   filenameout += "." + (TFileName) PostfixGfpMax;
        if ( badepochs  != NoSkippingBadEpochs  )   filenameout += "." + (TFileName) PostfixSkipBadEpochs;

        filenameout.AddExtension ( fileext );

        filenameout.ReplaceDir   ( outputdir );

        if ( createtempdir ) {

            AppendDir       ( filenameout, true, tempdir );

            if ( isfirstfile )
                CreatePath  ( filenameout, true );

            if ( temppath && StringIsEmpty ( temppath ) ) {
                StringCopy      ( temppath, filenameout );
                RemoveFilename  ( temppath );
                }
            }
                                        // thank you for avoiding overwriting!
        filenameout.CheckNoOverwrite ();


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Actually writing file(s)
        if ( savemainfiles ) {

            if ( timelinedisrupted )
                ToData->SetSamplingFrequency ( 0 );

            ToData->WriteFileEpochs (   filenameout, 
                                        IsVector ( datatypeout ), 
                                        timelinedisrupted ? 0 : samplingfrequency, 
                                        writingepochslist, 
                                        rois ? rois->GetRoiNames () : 0 
                                    );

                                        // saving alternative version of data?
//          if ( IsZScore ( zscoremethod ) ) {
//
//              StringCopy              ( buff,   filenameout );
//              StringReplace           ( buff,   "." PostfixStandardizationZScore,     "." PostfixStandardizationNone );
//              DataNoZScore.WriteFile  ( buff, IsVector ( datatypeout ), timelinedisrupted ? 0 : samplingfrequency, writingepochslist );
//              }


            if ( dualdata ) {
                                        // read with the general case
                TMaps               altesi ( (*dualgofin)[ fi ], 0, DualDataPresets[ dualdata ].DataType, ReferenceAsInFile );

                if ( timelinedisrupted )
                    altesi.SetSamplingFrequency ( 0 );


                StringCopy          ( filenameoutalt, filenameout );
                ReplaceExtension    ( filenameoutalt, DualDataPresets[ dualdata ].SavingExtension );


                CheckNoOverwrite    ( filenameoutalt );


                altesi.WriteFileEpochs  (   filenameoutalt, 
                                            IsVector ( DualDataPresets[ dualdata ].DataType ), 
                                            timelinedisrupted ? 0 : samplingfrequency, 
                                            writingepochslist, 
                                            rois ? rois->GetRoiNames () : 0 
                                        );

                // could also copy the markers from original file?

                } // dualdata


                                        // We can duplicate the markers IF the time line has not been altered (no cropping and the likes...)
            if ( ! timelinedisrupted ) {

                StringCopy      ( filemrk,  filenameout );
                AddExtension    ( filemrk,  FILEEXT_MRK );
                                        // currently, this contains only markers, and no triggers
                markers.WriteFile ( filemrk );

                                        // complimentary copying input markers to alt data(?)
                if ( dualdata ) {
                    StringCopy      ( filemrk,  filenameoutalt );
                    AddExtension    ( filemrk,  FILEEXT_MRK );

                    markers.WriteFile ( filemrk );
                    }

                } // ! timelinedisrupted

            } // savemainfiles


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( isfirstfile ) {
                                        // build an infix according to EpochsFromList and/or gfp peaks
            infixbase.Clear ();

            if ( epochs     == EpochsFromList 
              || epochs     == EpochsPeriodic       )   StringAppend    ( infixbase, ".", IntegerToString ( fromtf ), "_", IntegerToString ( totf ) );  // !separator "_" is used to retrieve epoch (for verbose purpose only) in segmentation!
//          if ( gfppeaks   != NoGfpPeaksDetection  )   StringAppend    ( infixbase, "." PostfixGfpMax );
//          if ( badepochs  != NoSkippingBadEpochs  )   StringAppend    ( infixbase, "." PostfixSkipBadEpochs );


                                        // update current base file name with infix
            TFileName       tempbasefilename    = outputdir + infixbase;
                                        // add to new list
            gofoutdir  .Add ( tempbasefilename, MaxPathShort );

                                        // create as many GoF as epochs, but only once
            gogofout   .Add ( new TGoF );

            if ( dualgogofout )
                dualgogofout->Add ( new TGoF );
            }

                                        // add current exported file to current epoch
//      if ( savemainfiles )            // !always update the output file names structures, even if the files are not actually written!
            gogofout[ epoch ].Add ( filenameout );

                                        // also add to optional alternative gogof
        if ( dualgogofout )
            (*dualgogofout)[ epoch ].Add ( filenameoutalt );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // save GFP Peaks marker to a separate file
        if ( gfppeaks != NoGfpPeaksDetection ) {
                                        // use new base file name - File name generation is OK, except the directoy doesn't exist yet at that point...    
//          StringCopy      ( gfppeaksmrk,  gofoutdir[ epoch ] );
//
//          StringCopy      ( buff,         filenamein );
//          RemoveDir       ( buff );
//          RemoveExtension ( buff );
//          StringAppend    ( gfppeaksmrk, "\\", buff );
//
//          StringAppend    ( gfppeaksmrk, ".GFP Peaks" );

                                        // file just before GFP peaks extraction
            StringCopy      ( gfppeaksmrk,  filenamein );
            RemoveExtension ( gfppeaksmrk );
            StringAppend    ( gfppeaksmrk, infix, ".To", postfixfileepoch );
            AddExtension    ( gfppeaksmrk, FILEEXT_MRK );

            goodepochslist.WriteFile ( gfppeaksmrk );
            }
*/

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        } // for epoch


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    } // for file


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Save Z-Score factors to file - even if was reloaded from a file

if ( savezscore 
  && backnorm != BackgroundNormalizationNone
  && ZScoreFactors.IsAllocated () ) {

                                        // complimentary saving the ZScore factors that have been computed, for each new time-line
    for ( int absg2 = 0; absg2 < gogofout.NumGroups (); absg2++ ) {

                                        // directory does not exist yet
        CreatePath      ( gofoutdir[ absg2 ], false );

                                        // output directory, constant file name
//      StringCopy      ( filenameout,  gofoutdir[ absg2 ], "\\", ZScoreEnumToFactorFileInfix ( zscoremethod ) );
//      AddExtension    ( filenameout,  /*fileext*/ FILEEXT_EEGSEF );

                                        // output directory, file name from first input file
        StringCopy      ( filenameout, gof[ 0 ] );

        RemoveExtension ( filenameout );

        if ( mergecomplex ) {
            StringReplace ( filenameout, "." InfixReal, "" );
            StringReplace ( filenameout, "." InfixImag, "" );
            }
                                        // optional prefix, unclipped
        PrefixFilename  ( filenameout, fileprefix );

        StringAppend    ( filenameout, infix, ".", ZScoreEnumToFactorFileInfix ( zscoremethod ) );

        AddExtension    ( filenameout, /*fileext*/ FILEEXT_EEGSEF );

        ReplaceDir      ( filenameout, outputdir );

        if ( createtempdir )
            AppendDir   ( filenameout, true, tempdir ); // or not(?)

        CheckNoOverwrite( filenameout );

                                        // transposed: values are shown in the X axis
        TStrings            zscorenames;

        ZScoreEnumToFactorNames ( zscoremethod, zscorenames );


        ZScoreFactors.WriteFile ( filenameout, &zscorenames );
                                        // returning a GoF for the Z-Score
        if ( zscoregof )
            zscoregof->Add ( filenameout );


                                        // we can be interested in a ris version that we can visualize in 3D
        ZScoreFactors.Transpose ();

        ReplaceExtension    ( filenameout, FILEEXT_RIS );

        CheckNoOverwrite    ( filenameout );

        ZScoreFactors.WriteFile ( filenameout );

                                                        // returning a GoF for the Z-Score
        if ( zscoregof )
            zscoregof->Add ( filenameout );
        }

    } // savezscore

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
