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

#include    "System.OpenMP.h"

#include    "FrequencyAnalysis.h"

#include    "TArray2.h"
#include    "TArray3.h"
#include    "TVector.h"
#include    "Math.Utils.h"
#include    "Math.Stats.h"
#include    "Math.FFT.MKL.h"
#include    "Math.FFT.h"
#include    "Files.TVerboseFile.h"
#include    "Files.TOpenDoc.h"
#include    "Files.Conversions.h"
#include    "Dialogs.TSuperGauge.h"

#include    "TExportTracks.h"

#include    "TTracksDoc.h"
#include    "TFreqCartoolDoc.h"             // TFreqFrequencyName

#include    "TCartoolDocManager.h"

#include    "TFrequencyAnalysisDialog.h"    // ComputeTimeMax, ComputeStep

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const char  FFTRescalingString[ NumFFTRescalingType ][ 64 ] =
            {
            "No Rescaling",
            "Dividing by Square Root of Window Size",
            "Dividing by Window Size (Parseval)",
            };


const char  FreqOutputAtomString[ NumFreqOutputAtomType ][ 32 ] =
            {
            "Real part",
            "Norm",
            "Power (Squared Norm)",
            "Complex",
            "Phase",
            };


const char  FreqWindowingString[ NumFreqWindowingType ][ 64 ] =
            {
            "No Windowing / Flat Top",
            "Hanning Windowing, done on each block",
            "Hanning Windowing, done only on the edges",
            };


//----------------------------------------------------------------------------
                                        // Returns an error message if types don't match, and 0 if they are compatible
const char* AnalysisAtomtypeCompatibility ( FreqAnalysisType    analysis,   
                                            bool                savingbands,  
                                            bool                averagingblocks, 
                                            FreqOutputAtomType  outputatomtype  )
{
switch ( outputatomtype ) {

    case    OutputAtomReal:
    
        if      ( analysis != FreqAnalysisFFTApproximation )

            return  "Can not save Real components for the FFT, PowerMaps and S-Transform analysis!";

        else if ( analysis == FreqAnalysisSTransform && averagingblocks )   // case shouldn't happen 

            return  "Can not average blocks for S-Transform analysis!";

        break;


    case    OutputAtomNorm:
    case    OutputAtomNorm2:

        if      ( analysis == FreqAnalysisFFTApproximation )

            return  "Can not save Norm/Power values for the FFT Approximation analysis!";

        else if ( analysis == FreqAnalysisSTransform && averagingblocks )   // case shouldn't happen

            return  "Can not average blocks for S-Transform analysis!";

        break;


    case    OutputAtomComplex:

        if      ( savingbands )     // for ALL types of analysis

            return  "Can not save Complex values for Frequency Bands output!";

        else if ( averagingblocks ) // for ALL types of analysis

            return  "Can not average Complex values!";

        else if ( analysis == FreqAnalysisFFTApproximation )

            return  "Can not save Complex values for the FFT Approximation analysis!";

        break;


    case    OutputAtomPhase:

        if      ( savingbands )     // for ALL types of analysis

            return  "Can not save Phase values for Frequency Bands output!";

        else if ( averagingblocks ) // for ALL types of analysis

            return  "Can not average Phase values!";
        
        break;
    }

return  0;
}


const char* AnalysisAtomtypeCompatibility ( FreqAnalysisCases   flags,   
                                            bool                savingbands,  
                                            bool                averagingblocks, 
                                            FreqOutputAtomType  outputatomtype  )
{
switch ( outputatomtype ) {

    case    OutputAtomReal:
    
        if      ( ! IsFFTApproxMethod ( flags ) )

            return  "Saving the Real components is only possible with the FFT Approximation analysis!";

        else if ( IsSTMethod ( flags ) && averagingblocks )   // case shouldn't happen 

            return  "Can not average blocks with the S-Transform analysis!";

        break;


    case    OutputAtomNorm:
    case    OutputAtomNorm2:

        if      ( IsFFTApproxMethod ( flags ) )

            return  "Saving the Norm/Power values is not possible with the FFT Approximation analysis!";

        else if ( IsSTMethod ( flags ) && averagingblocks )   // case shouldn't happen

            return  "Can not average blocks for S-Transform analysis!";

        break;


    case    OutputAtomComplex:

        if      ( savingbands )     // for ALL types of analysis

            return  "Saving Complex values is not possible with Frequency Bands output!";

        else if ( averagingblocks ) // for ALL types of analysis

            return  "Can not average Complex values!";

        else if ( IsFFTApproxMethod ( flags ) )

            return  "Saving Complex values is not possible with the FFT Approximation analysis!";

        break;


    case    OutputAtomPhase:

        if      ( savingbands )     // for ALL types of analysis

            return  "Saving Phase values is not possible with Frequency Bands output!";

        else if ( averagingblocks ) // for ALL types of analysis

            return  "Can not average Phase values!";
        
        break;
    }

return  0;
}


//----------------------------------------------------------------------------
                                        // Used internally to keep tracks of real / truncated / index of frequencies
class   TOneFrequencyBand
{
public:
                    TOneFrequencyBand ();


    int             SaveFreqMin_i;      // from freq index (included)
    double          SaveFreqMin_hz;     //  "     "  in [Hz]

    int             SaveFreqMax_i;      // to   freq index (included)
    double          SaveFreqMax_hz;     //  "     "  in [Hz]

    int             SaveFreqStep_i;     // saving by steps of indexes
    double          SaveFreqStep_hz;    //  "     "  in [Hz]

    int             SaveNumFreqs;       // resulting number of freqs

    int             AvgFreqStep_i;      // averaging by steps of indexes
    double          AvgFreqStep_hz;     //     "     "    "   of [Hz]
    int             AvgNumFreqs;        // resulting number of freqs averaged, per saved frequency
};


    TOneFrequencyBand::TOneFrequencyBand ()
{
SaveFreqMin_i   = 0;
SaveFreqMin_hz  = 0;
SaveFreqMax_i   = 0;
SaveFreqMax_hz  = 0;
SaveFreqStep_i  = 0;
SaveFreqStep_hz = 0;
SaveNumFreqs    = 0;
AvgFreqStep_i   = 0;
AvgFreqStep_hz  = 0;
AvgNumFreqs     = 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Tracing the "constellation", i.e. the electrodes in the Complex plane
//#define     OutputConstellation
#if defined (OutputConstellation)
#define             maxconst    1000
ofstream            ooo[ maxconst ];
ofstream           *toooo;
double              maxws[ maxconst ];
double              maxw;
double              orgx,   orgy,   orgz;
#endif


auto    ApproximateFrequency    = []    (   std::complex<float>     cstl[], 
                                            int                     numel   
                                        )
{
#if defined (OutputConstellation)
                                        // write original constellation
for ( int eli = 0; eli < numel; eli++ )
    *toooo << StreamFormatFloat32 << ( orgx + cstl[ eli ].real () * maxw ) << Tab
           << StreamFormatFloat32 << ( orgy + cstl[ eli ].imag () * maxw ) << Tab
           << StreamFormatFloat32 << orgz << NewLine;
#endif


double              sumx2           = 0;
double              sumy2           = 0;
double              sumxy           = 0;
                                        // compute phi
OmpParallelForSum ( sumx2, sumy2, sumxy )

for ( int eli = 0; eli < numel; eli++ ) {
    double          x       = cstl[ eli ].real ();
    double          y       = cstl[ eli ].imag ();

    sumx2  += x * x;
    sumy2  += y * y;
    sumxy  += x * y;
    }


double              phi1            = 0.5 * atan2 ( 2 * sumxy, sumx2 - sumy2 );
double              phi2            = phi1 + HalfPi;

double              phisd           = cos ( 2 * phi1 ) * ( sumx2 - sumy2 ) + 2 * sin ( 2 * phi1 ) * sumxy;

double              cosphi          = phisd > 0 ? cos ( phi1 ) : cos ( phi2 );
double              sinphi          = phisd > 0 ? sin ( phi1 ) : sin ( phi2 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we have the axis direction, project all electrodes
OmpParallelFor

for ( int eli = 0; eli < numel; eli++ )
                                        // project on directional, unity vector
                                        // and store in the complex as a real
    cstl[ eli ] = cosphi * cstl[ eli ].real () + sinphi * cstl[ eli ].imag ();



#if defined (OutputConstellation)
double              ps;
                                        // write projected constellation
for ( int eli = 0; eli < numel; eli++ ) {
    ps = cstl[ eli ].real ();

    *toooo << StreamFormatFloat32 << ( orgx + cosphi * ps * maxw ) << Tab
           << StreamFormatFloat32 << ( orgy + sinphi * ps * maxw ) << Tab
           << StreamFormatFloat32 << ( orgz + 1 ) << NewLine;
    }
#endif
};


//----------------------------------------------------------------------------
                                        // invert a whole set of electrodes values
                                        // could use some TMap...
auto    InvertFreqMap           = []    (   complex<float>*     v,
                                            int                 numel   
                                        )
{
for ( int eli = 0; eli < numel; eli++ )
    v[ eli ] = - v[ eli ];
};

                                        // just convert data format
auto    IsNegativeCorrelation   = []    (   complex<float>*     vc1, 
                                            complex<float>*     vc2, 
                                            int                 numel   
                                        )
{
TVector<float>      v1 ( numel );
TVector<float>      v2 ( numel );

for ( int eli = 0; eli < numel; eli++ ) {
    v1[ eli ]   = vc1[ eli ].real ();
    v2[ eli ]   = vc2[ eli ].real ();
    }

return  v1.Correlation ( v2 ) < 0;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

bool    FrequencyAnalysis   (   TTracksDoc*         eegdoc,             // not const, because we activate/deactivate filters
                                const char*         xyzfile,
                                FreqAnalysisType    analysis,
                                const char*         channels,           // could be empty or "*" to select all regular tracks
                                ReferenceType       ref,                const char*         reflist,
                                long                timemin,            long                timemax,
                                SkippingEpochsType  badepochs,          const char*         listbadepochs,
                                double              samplingfrequency,
                                int                 blocksize,          double              blocksoverlap,
                                FFTRescalingType    fftnorm,
                                FreqOutputBands     outputbands,
                                FreqOutputAtomType  outputatomtype,
                                bool                outputmarkers,      MarkerType          outputmarkerstype,
                                const char*         outputbandslist,
                                double              outputfreqmin,      double              outputfreqmax,
                                double              outputfreqstep,     
                                int                 outputdecadestep,
                                bool                outputsequential,   // averaged otherwise
                                FreqWindowingType   windowing,
                                bool                optimaldownsampling,
                                const char*         infixfilename,      bool                createsubdir,
                                char*               fileoutfreq,        // pointers act as flags and returned strings
                                char*               fileoutsplitelec,
                                char*               fileoutsplitfreq,
                                char*               fileoutspectrum,
                                char*               fileoutapprfreqs,
                                VerboseType         verbosey
                            )
{
if ( ! eegdoc )
    return  false;

                                        // FFT analysis requires these parameters
if ( analysis != FreqAnalysisSTransform
 && (    blocksize     <= 0
      || blocksoverlap <  0             // values are currently: 0% (100% jump); 75% (25% jump); ((blocksize-1)/blocksize)% (1TF jump)
      || blocksoverlap >  1 ) )
    return  false;


if ( analysis == FreqAnalysisSTransform )
    outputsequential    = true;

                                        // !It is assumed caller has already checked the consistency between analysis and outputatomtype!
//if ( ! AnalysisAtomtypeCompatibility    (   analysis, 
//                                            outputbands == OutputBands,
//                                            ! outputsequential, 
//                                            outputatomtype      ) )
//    return  false;


if ( analysis == FreqAnalysisSTransform && fftnorm != FFTRescalingNone )
    fftnorm         = FFTRescalingNone;

                                        // recover output files flags
bool                savefreq            = fileoutfreq      != 0;
bool                splitelectrode      = fileoutsplitelec != 0;
bool                splitfrequency      = fileoutsplitfreq != 0;
bool                splitspectrum       = fileoutspectrum  != 0;
bool                savefftapprox       = fileoutapprfreqs != 0;

                                        // force silent if not in interactive mode
if ( verbosey == Interactive && CartoolObjects.CartoolApplication->IsNotInteractive () )
    verbosey = Silent;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check if file is already open, which also works for linked XYZ
bool                isxyzdocalreadyopen = CartoolObjects.CartoolDocManager->IsOpen ( xyzfile );

TOpenDoc<TElectrodesDoc>    xyzdoc ( xyzfile, OpenDocHidden );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // list of channels -> TSelection
TSelection          elsave ( eegdoc->GetTotalElectrodes (), OrderArbitrary );  // electrodes saved
char                buff[ 64 * KiloByte /*EditSizeTextLong*/ ];


elsave.Reset ();

StringCopy      ( buff, channels );
StringCleanup   ( buff );
                                        
if ( StringIsEmpty ( buff ) 
  || StringIs ( buff, "*" )             // bypassing the '*' of TSelection
//|| analysis == FreqAnalysisPowerMaps  // force?
//|| analysis == FreqAnalysisFFTApproximation 
   ) {

    eegdoc->SetRegular ( elsave );

    elsave   -= eegdoc->GetAuxTracks ();
    elsave   -= eegdoc->GetBadTracks ();
    }
else                                    // get the exact list from user - !including for power maps!
    elsave.Set ( buff, xyzdoc.IsOpen () ? xyzdoc->GetElectrodesNames() : eegdoc->GetElectrodesNames() );


int                 numelsave       = (int) elsave;
                                        // no tracks to analyze?
if ( numelsave == 0 )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // resolve possible EOF
Clipped ( timemin, timemax, (long) 0, eegdoc->GetNumTimeFrames () - 1 );

long                timenum             = timemax - timemin + 1;


if ( analysis == FreqAnalysisSTransform ) {
                                        // override / provide these parameters
    blocksize       = timenum;
    blocksoverlap   = 0;
    }

                                        // heuristic to retrieve 1TF step
bool                blockstep1tf        = blocksoverlap > 0.75;  // blocksoverlap = ( blocksize - 1 ) / blocksize

int                 blockstep           = ComputeStep       ( analysis == FreqAnalysisSTransform, blocksize, blocksoverlap );

int                 numblocks           = ComputeNumBlocks  ( analysis == FreqAnalysisSTransform, blockstep1tf, timenum, blocksize, blocksoverlap );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TMarkers            rejectmarkers;

if ( badepochs == SkippingBadEpochsList )
                                        // transfer & filter at the same time
    rejectmarkers.InsertMarkers ( *eegdoc, listbadepochs );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // because we have real input signal, it can be processed faster and with less memory with some tricks
int                 freqsize            = blocksize / 2 + 1;


double              datafreqstep_hz     = samplingfrequency / (double) blocksize;
int                 limitfreqmax_i      = freqsize - 1;                             // Nyquist frequency; = blocksize / 2 - was:  GetNyquist ( blocksize, analysis == FreqAnalysisSTransform )
double              limitfreqmax_hz     = limitfreqmax_i * datafreqstep_hz;
double              limitfreqmin_i      = 1;                                        // first non-null frequency
double              limitfreqmin_hz     = limitfreqmin_i * datafreqstep_hz;

                                        // these are the cases where we DON'T want to average sub-bands frequencies
bool                nofreqavg           = outputatomtype == OutputAtomComplex               // don't average complex values
                                       || outputatomtype == OutputAtomPhase                 // don't average complex values, then computing the phase
                                       || outputbands    == OutputLogInterval               // this seems visually heavy when showing all frequencies
                                       || outputbands    == OutputLinearInterval
                                       || analysis       == FreqAnalysisSTransform;         // never for S-Transform, smooth enough all the time(?)
//                                     || analysis       == FreqAnalysisFFTApproximation;   // never for FFT Approximation


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

enum                {
                    gaugefreqglobal     = 0,
                    gaugefreqloop,
                    };


TSuperGauge         Gauge;
                                        // Frequency is annoying, concatenation is fast
if ( verbosey == Interactive ) {

    Gauge.Set           ( FrequencyAnalysisTitle, SuperGaugeLevelInter );

    Gauge.AddPart       ( gaugefreqglobal,  8,                                05 );
                                        // S-Transform has only 1 block, so let's animate on the electrodes
    Gauge.AddPart       ( gaugefreqloop,    analysis == FreqAnalysisSTransform ? numelsave : numblocks, 95 );

    CartoolObjects.CartoolApplication->SetMainTitle ( "Frequency Analysis of", eegdoc->GetDocPath (), Gauge );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numfreqbands            = 0;
vector<TOneFrequencyBand>  freqbands;
double              savedfreqmax            = 0;


if      ( outputbands == OutputBands ) {

    double          fminhz;
    double          fmaxhz;


    TSplitStrings   bandlist ( outputbandslist, UniqueStrings );

    numfreqbands        = (int) bandlist;

    if ( numfreqbands == 0 )
        return  false;

                                        // trust the parameters (until better checking..), and allocate all the bands
    freqbands.resize ( numfreqbands );


    TOneFrequencyBand*          fb          = freqbands.data ();

    for ( int fbi = 0; fbi < numfreqbands; fbi++, fb++ ) {

        sscanf ( bandlist[ fbi ], "%lf-%lf", &fminhz, &fmaxhz );

                                        // this is a safe over-estimate, as the resulting band will have a mean frequency lower than that
        Maxed ( savedfreqmax, fmaxhz );


        fb->SaveFreqMin_i   = Clip ( Truncate ( fminhz / datafreqstep_hz ), 0, limitfreqmax_i );
        fb->SaveFreqMax_i   = Clip ( Truncate ( fmaxhz / datafreqstep_hz ), 0, limitfreqmax_i );


        CheckOrder ( fb->SaveFreqMin_i, fb->SaveFreqMax_i );


        fb->SaveFreqMin_hz  = fb->SaveFreqMin_i * datafreqstep_hz;
        fb->SaveFreqMax_hz  = fb->SaveFreqMax_i * datafreqstep_hz;


        fb->SaveNumFreqs    = 1;
        fb->SaveFreqStep_i  = fb->SaveFreqMax_i - fb->SaveFreqMin_i + 1; // actually, a jump beyond the max
        fb->SaveFreqStep_hz = fb->SaveFreqStep_i * datafreqstep_hz;

                                        // do NOT average complex values from a given sub-band - phases will be all messed-up
                                        // setup values so that AvgNumFreqs is 1
        if ( nofreqavg ) {
            fb->AvgFreqStep_i   = fb->SaveFreqStep_i;
            fb->AvgFreqStep_hz  = fb->AvgFreqStep_i * datafreqstep_hz;
            }
        else {
                                        // get a reasonable averaging step, an eventual over-sampling of the min frequency step (especially for STransform)
            fb->AvgFreqStep_hz  = AtLeast ( AvgMinStep_hz, datafreqstep_hz );
            fb->AvgFreqStep_i   = fb->AvgFreqStep_hz / datafreqstep_hz;             // always >= 1
            fb->AvgFreqStep_i   = NoMore ( fb->SaveFreqStep_i, fb->AvgFreqStep_i ); // avg step can not be greater than saved step
            }

                                        // band includes both min and max
        fb->AvgNumFreqs     = ( fb->SaveFreqMax_i - fb->SaveFreqMin_i ) / fb->AvgFreqStep_i + 1;

                                        // limit the STransform number of averaging
        if ( analysis == FreqAnalysisSTransform && fb->AvgNumFreqs > STranformMaxAvg ) {

            fb->AvgFreqStep_i   = ( fb->SaveFreqMax_i - fb->SaveFreqMin_i ) / ( STranformMaxAvg - 1 );
            fb->AvgNumFreqs     = ( fb->SaveFreqMax_i - fb->SaveFreqMin_i ) / fb->AvgFreqStep_i + 1;
            }


        fb->AvgFreqStep_hz  = fb->AvgFreqStep_i * datafreqstep_hz;
        } // for freqband

    } // OutputBands


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need to create a set of bands, as for OutputBands
else if ( outputbands == OutputLogInterval ) {

                                        // ?maybe use  AtLeast ( FrequencyResolution, datafreqstep_hz )?

                                        // due to log things, we don't want no stinking 0Hz in here
    double          fminhz          = outputfreqmin;
    double          fmaxhz          = outputfreqmax;

    Clipped ( fminhz, fmaxhz, limitfreqmin_hz, limitfreqmax_hz );

                                        // explicit number of division within a decade ( log1 to log10, log10 to log100 etc...)
    int             stepsperdecade  = AtLeast ( 1, outputdecadestep );

    double          decaderes       = AtLeast ( Log10 ( AtLeast ( FreqMinLog10Value, datafreqstep_hz ) ), 1 / (double) stepsperdecade );

                                        // or extrapolate the given linear step to a decade resolution
//  double          decaderes       = AtLeast    ( Log10 ( AtLeast ( FreqMinLog10Value, datafreqstep_hz ) ), outputfreqstep / ( 10 - 1 ) );


    double          logfminhz       = TruncateTo ( Log10 ( AtLeast ( FreqMinLog10Value, fminhz ) ), decaderes );
    double          logfmaxhz       = TruncateTo ( Log10 ( AtLeast ( FreqMinLog10Value, fmaxhz ) ), decaderes );
                                        // double check the truncation in log didn't introduce some inconsistencies
    Clipped ( logfminhz, logfmaxhz, Log10 ( AtLeast ( FreqMinLog10Value, limitfreqmin_hz ) ), Log10 ( AtLeast ( FreqMinLog10Value, limitfreqmax_hz ) ) );

                                                                         // when not averaging (i.e. just single freqs), we can have the upper freq included
    numfreqbands        = ( logfmaxhz - logfminhz + 1e-10 ) / decaderes + ( nofreqavg ? 1 : 0 );


    if ( numfreqbands == 0 )
        return  false;


    savedfreqmax    = Power10 ( logfmaxhz );

                                        // trust the parameters (until better checking..), and allocate all the bands
    freqbands.resize ( numfreqbands );
    int             duplicatefreqs      = 0;


    TOneFrequencyBand*          fb          = freqbands.data ();

    for ( int fbi = 0; fbi < numfreqbands; fbi++, fb++ ) {
                                        // convert from log to regular Hz, rounded to the current resolution
        fb->SaveFreqMin_hz  = TruncateTo ( Power10 ( logfminhz +   fbi                           * decaderes ), datafreqstep_hz );
                                                                         // forbid doing bands in case of no averaging
        fb->SaveFreqMax_hz  = TruncateTo ( Power10 ( logfminhz + ( fbi + ( nofreqavg ? 0 : 1 ) ) * decaderes ), datafreqstep_hz );

                                        // clipping done in indexes
        fb->SaveFreqMin_i   = Clip ( Truncate ( fb->SaveFreqMin_hz / datafreqstep_hz ), 0, limitfreqmax_i );
        fb->SaveFreqMax_i   = Clip ( Truncate ( fb->SaveFreqMax_hz / datafreqstep_hz ), 0, limitfreqmax_i );

                                        // don't include the upper boundary, to avoid overlap between successive bands
        if ( fb->SaveFreqMax_i > fb->SaveFreqMin_i )
            fb->SaveFreqMax_i --;

                                        // indexes back to frequencies
        fb->SaveFreqMin_hz  = fb->SaveFreqMin_i * datafreqstep_hz;
        fb->SaveFreqMax_hz  = fb->SaveFreqMax_i * datafreqstep_hz;

                                        // do we have the same frequencies as previous entry?
        if ( fbi > 0 && fb->SaveFreqMin_i == (fb-1)->SaveFreqMin_i
                     && fb->SaveFreqMax_i == (fb-1)->SaveFreqMax_i ) {

            fb--;                       // skip this by overwriting this duplicate
            duplicatefreqs++;
            continue;
            }


        fb->SaveNumFreqs    = 1;
        fb->SaveFreqStep_i  = fb->SaveFreqMax_i - fb->SaveFreqMin_i + 1; // actually, a jump beyond the max
        fb->SaveFreqStep_hz = fb->SaveFreqStep_i * datafreqstep_hz;


        if ( nofreqavg ) {
            fb->AvgFreqStep_i   = fb->SaveFreqStep_i;
            fb->AvgFreqStep_hz  = fb->AvgFreqStep_i * datafreqstep_hz;
            }
        else {
                                        // get a reasonable averaging step, an eventual over-sampling of the min frequency step (especially for STransform)
            fb->AvgFreqStep_hz  = AtLeast ( AvgMinStep_hz, datafreqstep_hz );
            fb->AvgFreqStep_i   = fb->AvgFreqStep_hz / datafreqstep_hz;             // always >= 1
            fb->AvgFreqStep_i   = NoMore ( fb->SaveFreqStep_i, fb->AvgFreqStep_i ); // avg step can not be greater than saved step
            }

                                        // band includes both min and max
        fb->AvgNumFreqs     = ( fb->SaveFreqMax_i - fb->SaveFreqMin_i ) / fb->AvgFreqStep_i + 1;
        }


    if ( duplicatefreqs > 0 ) {
                                        // just getting rid of last entries
        freqbands.erase ( freqbands.end() - duplicatefreqs, freqbands.end () );
                                        // don't forget to update the final number of frequencies
        numfreqbands   -= duplicatefreqs;

        if ( numfreqbands == 0 )
            return  false;
        }

    } // OutputLogInterval


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( outputbands == OutputLinearInterval ) {
                                        // we have only one band, which will be enumerated
    numfreqbands        = 1;
    freqbands.resize ( numfreqbands  );
    TOneFrequencyBand*          fb          = freqbands.data ();

    
    savedfreqmax        = outputfreqmax;


    fb->SaveFreqStep_hz = outputfreqstep;
    fb->SaveFreqStep_i  = AtLeast ( 1.0, fb->SaveFreqStep_hz / datafreqstep_hz );   // always >= 1, and the smallest int step
    fb->SaveFreqStep_hz = fb->SaveFreqStep_i * datafreqstep_hz;

                                        // compute the min frequency
    fb->SaveFreqMin_i   = Clip ( Truncate ( outputfreqmin / datafreqstep_hz ), 0, limitfreqmax_i );
                                        // first, get the max frequency according to sampling frequency
    fb->SaveFreqMax_i   =        Truncate ( outputfreqmax / datafreqstep_hz );
                                        // second, round it according to the saving steps
    fb->SaveFreqMax_i   = fb->SaveFreqMin_i + RoundTo ( fb->SaveFreqMax_i - fb->SaveFreqMin_i, fb->SaveFreqStep_i );

                                        // test limits
    if      ( fb->SaveFreqMax_i < 0 )                                           fb->SaveFreqMax_i = 0;
    else if ( fb->SaveFreqMax_i > limitfreqmax_i - fb->SaveFreqStep_i + 1 )     fb->SaveFreqMax_i = fb->SaveFreqMin_i
                                                                                                  + TruncateTo ( limitfreqmax_i - fb->SaveFreqStep_i + 1 - fb->SaveFreqMin_i, fb->SaveFreqStep_i );

                                        // permutate?
    CheckOrder ( fb->SaveFreqMin_i, fb->SaveFreqMax_i );


    fb->SaveFreqMin_hz  = fb->SaveFreqMin_i * datafreqstep_hz;
    fb->SaveFreqMax_hz  = fb->SaveFreqMax_i * datafreqstep_hz;

                                        // num frequencies, from the min to the max (both included), with the stepped intermediates
    fb->SaveNumFreqs    = ( fb->SaveFreqMax_i - fb->SaveFreqMin_i ) / fb->SaveFreqStep_i + 1;

                                        // do NOT average complex values from a given sub-band - phases will be all messed-up
                                        // setup values so that AvgNumFreqs is 1
    if ( nofreqavg ) {
        fb->AvgFreqStep_i   = fb->SaveFreqStep_i;
        fb->AvgFreqStep_hz  = fb->AvgFreqStep_i * datafreqstep_hz;
        }
    else {
                                        // get a reasonable averaging step, an eventual over-sampling of the min frequency step (especially for STransform)
        fb->AvgFreqStep_hz  = AtLeast ( AvgMinStep_hz, datafreqstep_hz );
        fb->AvgFreqStep_i   = fb->AvgFreqStep_hz / datafreqstep_hz;             // always >= 1
        fb->AvgFreqStep_i   = NoMore ( fb->SaveFreqStep_i, fb->AvgFreqStep_i ); // avg step can not be greater than saved step
        }

                                        // don't include the next saved step
    fb->AvgNumFreqs     = ( fb->SaveFreqStep_i - 1 ) / fb->AvgFreqStep_i + 1;

                                        // limit the STransform number of averaging
    if ( analysis == FreqAnalysisSTransform && fb->AvgNumFreqs > STranformMaxAvg ) {

        fb->AvgFreqStep_i   = ( fb->SaveFreqStep_i - 1 ) / ( STranformMaxAvg - 1 );
        fb->AvgNumFreqs     = ( fb->SaveFreqStep_i - 1 ) / fb->AvgFreqStep_i + 1;
        }


    fb->AvgFreqStep_hz  = fb->AvgFreqStep_i * datafreqstep_hz;
    } // OutputLinearInterval


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // finally, get the total number of saved frequencies in file
int                 numsavedfreqs   = 0;
int                 maxfftafreqs    = 0;

for_each (  freqbands.cbegin (), freqbands.cend (),
            [ &numsavedfreqs, &maxfftafreqs ] ( const TOneFrequencyBand& fb ) { numsavedfreqs      += fb.SaveNumFreqs; 
                                                                                Maxed ( maxfftafreqs, fb.AvgNumFreqs * fb.SaveNumFreqs ); } 
         );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( verbosey == Interactive ) {
    Gauge.Next ( gaugefreqglobal );
    CartoolObjects.CartoolApplication->SetMainTitle ( Gauge );
    }

AtomType            datatypein          = eegdoc->GetAtomType ( AtomTypeUseOriginal );
AtomType            datatypeout         = outputatomtype == OutputAtomComplex ? AtomTypeComplex : AtomTypeScalar;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Currently applies only to S-Transform
                    optimaldownsampling = optimaldownsampling
                                       && analysis == FreqAnalysisSTransform                                // S-Transform is very smooth, and we don't need the extra time resolution (?)
                                       && savedfreqmax > 0;
                                        // default is to not downsample results
int                 downsamplingfactor  = 1;

                                        // S-Transform is very smooth, and we don't need the extra time resolution (?)
if ( optimaldownsampling ) {
                                        // giving the Nyquist seems OK
    downsamplingfactor  = AtLeast ( 1, Truncate ( samplingfrequency / ( 2 * savedfreqmax ) ) );

//  Maxed ( blockstep, downsamplingfactor );
                                        // well, turns out we cannot downsample, better turn off the option
    if ( downsamplingfactor == 1 )
        optimaldownsampling = false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // something went wrong in the parameters?
if (   numblocks <= 0 
    || blocksize <= 0 ) {

    if ( verbosey == Interactive ) {

        if ( numblocks <= 0 || blocksize <= 0 )
            ShowMessage ( "Number of windows or window size seem to be incorrect...", FrequencyAnalysisTitle, ShowMessageWarning );
        }

    return  false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( verbosey == Interactive ) {
    Gauge.Next ( gaugefreqglobal );
    CartoolObjects.CartoolApplication->SetMainTitle ( Gauge );
    }

                                        // base directory & file names
TFileName           BaseDir;
TFileName           BaseDirSpectrum;
TFileName           BaseDirApprox;
TFileName           BaseDirSplitFreqs;
TFileName           BaseDirSplitElecs;

TFileName           BaseFileName;
TFileName           BaseFileNameSpectrum;
TFileName           BaseFileNameApprox;
TFileName           BaseFileNameSplitFreqs;
TFileName           BaseFileNameSplitElecs;
TFileName           fileoutprefix;


                                        // Get full path - extension and weird stuff
eegdoc->GetBaseFileName ( buff );

                                        // Generate BaseDir
if ( StringIsNotEmpty ( infixfilename ) )

    StringCopy ( BaseDir, buff, ".", infixfilename );

else {
                                        // append method name + parameters
    StringCopy ( BaseDir, buff, ".", analysis == FreqAnalysisFFT ?              InfixFft
                                   : analysis == FreqAnalysisPowerMaps ?        InfixPowerMaps
                                   : analysis == FreqAnalysisFFTApproximation ? InfixFftApprox
                                   : analysis == FreqAnalysisSTransform ?       InfixSTransform
                                   :                                            "Other" );

    if      ( outputbands == OutputLinearInterval 
           || outputbands == OutputLogInterval    )

        sprintf ( StringEnd ( BaseDir ), " %.2f-%.2f Hz", freqbands[ 0 ].SaveFreqMin_hz, freqbands[ numfreqbands - 1 ].SaveFreqMax_hz );

    else // OutputBands
        sprintf ( StringEnd ( BaseDir ), " %0d %s", numfreqbands, numfreqbands > 1 ? InfixBands : InfixBand );

                                        // add if average or sequence
    if ( analysis != FreqAnalysisSTransform )
        if ( outputsequential ) StringAppend ( BaseDir, "." InfixSeq );
        else                    StringAppend ( BaseDir, "." InfixAvg );
    }


                                        // extract string "prefix"
StringCopy      ( fileoutprefix,                 ToFileName ( BaseDir ) );


if ( createsubdir ) {

    StringCopy      ( BaseDirSpectrum,  BaseDir,     "\\",      InfixSpectrum   );
    StringCopy      ( BaseDirApprox,    BaseDir,     "\\",      InfixApproxEeg  );
    StringCopy      ( BaseDirSplitFreqs,BaseDir,     "\\",      InfixSplitFreqs );
    StringCopy      ( BaseDirSplitElecs,BaseDir,     "\\",      InfixSplitElecs );
                                        // can create subdirectories now
    CreatePath      ( BaseDir,     false );

    if ( splitspectrum                  ) CreatePath      ( BaseDirSpectrum,      false );
    if ( splitspectrum && savefftapprox ) CreatePath      ( BaseDirApprox,        false );
    if ( splitfrequency                 ) CreatePath      ( BaseDirSplitFreqs,    false );
    if ( splitelectrode                 ) CreatePath      ( BaseDirSplitElecs,    false );
    }

else {

    RemoveFilename  ( BaseDir );

    StringCopy      ( BaseDirSpectrum,      BaseDir                                 );
    StringCopy      ( BaseDirApprox,        BaseDir                                 );
    StringCopy      ( BaseDirSplitFreqs,    BaseDir                                 );
    StringCopy      ( BaseDirSplitElecs,    BaseDir                                 );
                                        // no subdirectorie to be created here
    }


                                        // compose path access and main prefix "full path\prefix\prefix"
StringCopy ( BaseFileName,              BaseDir,            "\\",   fileoutprefix,  "." );
StringCopy ( BaseFileNameSpectrum,      BaseDirSpectrum,    "\\",   fileoutprefix,  "." );
StringCopy ( BaseFileNameApprox,        BaseDirApprox,      "\\",   fileoutprefix,  "." );
StringCopy ( BaseFileNameSplitFreqs,    BaseDirSplitFreqs,  "\\",   fileoutprefix,  "." );
StringCopy ( BaseFileNameSplitElecs,    BaseDirSplitElecs,  "\\",   fileoutprefix,  "." );


if ( StringLength ( BaseFileName ) > MaxPathShort - 32 ) {
    if ( verbosey == Interactive )
        ShowMessage ( "File name is too long to generate a correct output...", eegdoc->GetDocPath(), ShowMessageWarning );
    return  false;
    }

                                        // delete any previous files, with my prefix only!
StringCopy  ( buff,  BaseFileName,     "*" );
DeleteFiles ( buff );

if ( createsubdir ) {

    StringCopy  ( buff,  BaseFileNameSpectrum,      "*" );
    DeleteFiles ( buff );

    StringCopy  ( buff,  BaseFileNameApprox,        "*" );
    DeleteFiles ( buff );

    StringCopy  ( buff,  BaseFileNameSplitFreqs,    "*" );
    DeleteFiles ( buff );

    StringCopy  ( buff,  BaseFileNameSplitElecs,    "*" );
    DeleteFiles ( buff );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Generate file names
TFileName           fileoutvrb;
TFileName           fileoutmrk;


StringCopy      ( fileoutvrb,           BaseFileName,     FILEEXT_VRB  );


if ( savefreq       )   StringCopy      ( fileoutfreq,          BaseFileName,     FILEEXT_FREQ );
if ( savefreq       )   StringCopy      ( fileoutmrk,           fileoutfreq,  "." FILEEXT_MRK  );
if ( splitelectrode )   StringCopy      ( fileoutsplitelec,     BaseFileNameSplitElecs, "*.",                                                       FILEEXT_EEGSEF /*FILEEXT_EEGEP*/ );
if ( splitfrequency )   StringCopy      ( fileoutsplitfreq,     BaseFileNameSplitFreqs, "*.",                                                       FILEEXT_EEGSEF /*FILEEXT_EEGEP*/ );
if ( splitspectrum  )   StringCopy      ( fileoutspectrum,      BaseFileNameSpectrum, outputsequential ? InfixWindow" *." : "", InfixSpectrum".",   FILEEXT_EEGSEF /*FILEEXT_EEGEP*/ );
if ( savefftapprox  )   StringCopy      ( fileoutapprfreqs,     BaseFileNameApprox,   outputsequential ? InfixWindow" *." : "", InfixApproxEeg".",  FILEEXT_EEGSEF /*FILEEXT_EEGEP*/ );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // choose the right reference
TSelection          refsel ( eegdoc->GetTotalElectrodes (), OrderSorted );     // electrodes for specified reference

if ( ref == ReferenceArbitraryTracks ) {

    refsel.Reset ();

    refsel.Set    ( reflist, xyzdoc.IsOpen () ? xyzdoc->GetElectrodesNames() : eegdoc->GetElectrodesNames(), verbosey == Interactive );

                                        // that should not be...
    if ( refsel.NumSet () == 0 ) {

        ref     = ReferenceAsInFile;    // either setting to no reference, as is currently done in  TTracksDoc::SetReferenceType
        //return  false;
        }
    }


CheckReference ( ref, datatypein );

                                        // rare case, user asked for average reference on 1 track...
if ( ref == ReferenceAverage && eegdoc->GetNumElectrodes () == 1 )

    ref     = ReferenceAsInFile;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // verbose file init
TVerboseFile        verbose ( fileoutvrb, VerboseFileDefaultWidth );

verbose.PutTitle ( "Frequency Analysis" );


verbose.NextTopic ( "Input files:" );
{
verbose.Put ( "Input file:", (char *) eegdoc->GetDocPath() );
verbose.Put ( "Electrodes coordinates file:", xyzdoc.IsOpen () ? (char *) xyzdoc->GetDocPath() : "None" );
}


verbose.NextTopic ( "Output files:" );
{
if ( savefreq )
    verbose.Put ( "All results in single file:", fileoutfreq );

if ( splitelectrode )
    verbose.Put ( "Results split by Electrode:", fileoutsplitelec );

if ( splitfrequency )
    verbose.Put ( "Results split by Frequency:", fileoutsplitfreq );

if ( splitspectrum )
    verbose.Put ( "Results split by Spectrum:", fileoutspectrum );

if ( splitspectrum && savefftapprox )
    verbose.Put ( "FFT Approximation EEG file(s):", fileoutapprfreqs );

verbose.Put ( "Creating sub-directory:", createsubdir );

verbose.Put ( "Verbose file (this):", fileoutvrb );
}


verbose.NextTopic ( "Tracks to analyse:" );
{
//verbose.Put ( "Writing tracks:", channels );
verbose.Put ( "Number of tracks:", (int) elsave );
verbose.Put ( "Writing tracks:", elsave.ToText ( buff, xyzdoc.IsOpen () ? xyzdoc->GetElectrodesNames() : eegdoc->GetElectrodesNames(), AuxiliaryTracksNames ) );
}


verbose.NextTopic ( "Time windows:" );
{
verbose.Put ( "From      Time Frame:",  (int) timemin );
verbose.Put ( "To        Time Frame:",  (int) timemax );
verbose.Put ( "Number of Time Frames:", (int) timenum );

if ( analysis != FreqAnalysisSTransform ) {

    verbose.NextLine ();
    verbose.Put ( "Windows size:", blocksize, 0, " [TF]" );
    verbose.Put ( "Windows step:", blocksoverlap == 0.00 ? "100% Window"    // transform overlap back to jump
                                 : blocksoverlap == 0.75 ? "25% Window" 
                                 :                         "1 TF"       );  // blocksoverlap = ( blocksize - 1 ) / blocksize
    verbose.Put ( "Windows step duration:", TimeFrameToMilliseconds ( blockstep, samplingfrequency ), 2, " [ms]" );
    verbose.Put ( "Number of windows:", numblocks );
    }

verbose.NextLine ();
verbose.Put ( "Excluding bad epochs:", SkippingEpochsNames[ badepochs ] );
if ( badepochs == SkippingBadEpochsList ) {
    verbose.Put ( "Skipping markers:", listbadepochs );
    verbose.Put ( "Number of excluding markers:", (int) rejectmarkers );
    }
}


verbose.NextTopic ( "Frequencies:" );
{
verbose.Put ( "Sampling frequency   [Hz]:", samplingfrequency );
verbose.Put ( "Frequency min        [Hz]:", limitfreqmin_hz );
verbose.Put ( "Frequency max        [Hz]:", limitfreqmax_hz );
verbose.Put ( "Frequency resolution [Hz]:", datafreqstep_hz );

verbose.NextLine ();
if          ( outputbands == OutputLinearInterval ) verbose.Put ( "Saving:", "Interval" );
else if     ( outputbands == OutputLogInterval    ) verbose.Put ( "Saving:", "Log Interval" );
else if     ( outputbands == OutputBands          ) verbose.Put ( "Saving:", "Frequency Bands" );
else                                                verbose.Put ( "Saving:", "Unknown" );

if          ( outputbands == OutputLinearInterval ) {

    verbose.Put ( "From frequency [Hz]:",           freqbands[ 0 ].SaveFreqMin_hz );
    verbose.Put ( "To   frequency [Hz]:",           freqbands[ 0 ].SaveFreqMax_hz );
    verbose.Put ( "By steps of    [Hz]:",           freqbands[ 0 ].SaveFreqStep_hz );

    verbose.NextLine ();
    verbose.Put ( "Number of frequencies saved:",   freqbands[ 0 ].SaveNumFreqs );
                                        // be explicit, give all freqs

    const TOneFrequencyBand*    fb          = freqbands.data ();

    for ( int fi = fb->SaveFreqMin_i, fi0 = 0; fi <= fb->SaveFreqMax_i; fi += fb->SaveFreqStep_i, fi0++ ) {

        if ( fb->AvgNumFreqs > 1 ) {

            verbose.Put ( fi0 ? "" : "Merging frequencies [Hz]:" );

            for ( int downf = 0, fi2 = fi; downf < fb->AvgNumFreqs; downf++, fi2 += fb->AvgFreqStep_i )
                (ofstream&) verbose << ( downf ? "," Tab : "" ) << fi2 * datafreqstep_hz;

            verbose.NextLine ();
            }
        else
            verbose.Put ( fi0 ? "" : "Frequencies [Hz]:", fi * datafreqstep_hz );
        }
    }

else if     ( outputbands == OutputBands 
           || outputbands == OutputLogInterval ) {

    verbose.Put ( "Number of frequency bands:", numfreqbands );

    const TOneFrequencyBand*    fb          = freqbands.data ();

    for ( int fbi = 0; fbi < numfreqbands; fbi++, fb++ ) {

        verbose.NextLine ();
        verbose.Put ( "Frequency band #:",          fbi + 1 );

        if ( fb->AvgNumFreqs > 1 ) {

            verbose.Put ( "From frequency [Hz]:",            fb->SaveFreqMin_hz );
            verbose.Put ( "To   frequency [Hz]:",            fb->SaveFreqMax_hz );
            verbose.Put ( "Number of frequencies averaged:", fb->AvgNumFreqs );

            for ( int fi = fb->SaveFreqMin_i, fi0 = 0; fi <= fb->SaveFreqMax_i; fi += fb->SaveFreqStep_i, fi0++ ) {

                if ( fb->AvgNumFreqs > 1 ) {
                    verbose.Put ( fi0 ? "" : "Merging frequencies [Hz]:" );

                    for ( int downf = 0, fi2 = fi; downf < fb->AvgNumFreqs; downf++, fi2 += fb->AvgFreqStep_i )
                        (ofstream&) verbose << ( downf ? "," Tab : "" ) << fi2 * datafreqstep_hz;

                    verbose.NextLine ();
                    }
                else
                    verbose.Put ( fi0 ? "" : "Frequencies [Hz]:", fi * datafreqstep_hz );
                }
            }
        else {
            verbose.Put ( "Frequency [Hz]:",    fb->SaveFreqMin_hz );
            }
        }
    }
}


verbose.NextTopic ( "Analysis:" );
{
if          ( analysis == FreqAnalysisFFT )                 verbose.Put ( "Type of frequency analysis:", "FFT" );
else if     ( analysis == FreqAnalysisPowerMaps )           verbose.Put ( "Type of frequency analysis:", "FFT (Power Maps)" );
else if     ( analysis == FreqAnalysisFFTApproximation )    verbose.Put ( "Type of frequency analysis:", "FFT Approximation" );
else if     ( analysis == FreqAnalysisSTransform )          verbose.Put ( "Type of frequency analysis:", "Wavelet (S-Transform)" );
else                                                        verbose.Put ( "Type of frequency analysis:", "Unknown" );

verbose.Put ( "Windowing function:", FreqWindowingString[ windowing ] );

verbose.NextLine ();

if ( analysis != FreqAnalysisSTransform ) {

    verbose.Put ( "FFT Time Windows rescaling:", FFTRescalingString[ fftnorm ] );
    verbose.Put ( "Time Windows results are:", outputsequential ? "Written sequentially" : "Averaged" );

    if ( ! outputsequential )
        if ( analysis == FreqAnalysisFFTApproximation )
            verbose.Put ( "Averaging is done:", "After polarity check" );
        else
            verbose.Put ( "Averaging is done:", "After taking norm" );
    }

verbose.Put ( "Output values:", FreqOutputAtomString[ outputatomtype ] );
}


verbose.NextTopic ( "Reference of data:" );
{
if      ( ref == ReferenceAsInFile          )       verbose.Put ( "Reference:", ReferenceNames[ ref ] );
else if ( ref == ReferenceAverage           )       verbose.Put ( "Reference:", ReferenceNames[ ref ] );
else if ( ref == ReferenceArbitraryTracks   )       verbose.Put ( "Reference:", reflist );
else if ( ref == ReferenceUsingCurrent      ) {

    ReferenceType   currref     = eegdoc->GetReferenceType ();

    if      ( currref == ReferenceAsInFile  )       verbose.Put ( "Current reference:", ReferenceNames[ currref ] );
    else if ( currref == ReferenceAverage   )       verbose.Put ( "Current reference:", ReferenceNames[ currref ] );
    else {
        eegdoc->GetReferenceTracks ().ToText ( buff, xyzdoc.IsOpen () ? xyzdoc->GetElectrodesNames() : eegdoc->GetElectrodesNames(), AuxiliaryTracksNames );
                                                                    
                                                    verbose.Put ( "Current reference:", buff );
        }
    }
else                                                verbose.Put ( "Reference:", ReferenceNames[ ref ] );
}


verbose.NextTopic ( "Options:" );
{
verbose.Put ( "Saving all freqs into a single file:",   savefreq );
verbose.Put ( "Splitting results per electrode:",       splitelectrode );
verbose.Put ( "Splitting results per frequency:",       splitfrequency );
verbose.Put ( "Splitting results per spectrum :",       splitspectrum );
//verbose.Put ( "Creating sub-directory for results:", createsubdir );

verbose.NextLine ();
verbose.Put ( "Optimally downsampling the file:", optimaldownsampling );
if ( optimaldownsampling )
    verbose.Put ( "Downsampling factor:", downsamplingfactor );
}


verbose.Flush ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // reset filters
bool                oldfiltersactivated = eegdoc->DeactivateFilters ( true );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( verbosey == Interactive ) {
    Gauge.Next ( gaugefreqglobal );
    CartoolObjects.CartoolApplication->SetMainTitle ( Gauge );
    }

                                        // create file - don't ask for any overwriting
if ( ! CanOpenFile ( fileoutfreq, CanOpenFileWrite ) )
    return  false;


TExportTracks       expfile;


expfile.SetAtomType ( datatypeout );

StringCopy ( expfile.Filename, fileoutfreq );

expfile.Type    = ExportTracksFreq;

                                        // clear string
ClearVirtualMemory ( expfile.FreqTypeName, sizeof ( expfile.FreqTypeName ) /*MaxCharFreqType*/ );
                                        // choose the analysis type and result type
if          ( analysis == FreqAnalysisFFTApproximation )    StringCopy ( expfile.FreqTypeName, FrequencyAnalysisNames[ FreqFFTApproximation     ] );

else if     ( analysis == FreqAnalysisSTransform ) {

    if      ( outputatomtype == OutputAtomNorm    )         StringCopy ( expfile.FreqTypeName, FrequencyAnalysisNames[ FreqSTransformNorm       ] );
    else if ( outputatomtype == OutputAtomNorm2   )         StringCopy ( expfile.FreqTypeName, FrequencyAnalysisNames[ FreqSTransformNorm2      ] );
    else if ( outputatomtype == OutputAtomComplex )         StringCopy ( expfile.FreqTypeName, FrequencyAnalysisNames[ FreqSTransformComplex    ] );
    else if ( outputatomtype == OutputAtomPhase   )         StringCopy ( expfile.FreqTypeName, FrequencyAnalysisNames[ FreqSTransformPhase      ] );
    }

else if     ( analysis == FreqAnalysisPowerMaps
           || analysis == FreqAnalysisFFT ) {

    if      ( outputatomtype == OutputAtomNorm    )         StringCopy ( expfile.FreqTypeName, FrequencyAnalysisNames[ FreqFFTNorm              ] );
    else if ( outputatomtype == OutputAtomNorm2   )         StringCopy ( expfile.FreqTypeName, FrequencyAnalysisNames[ FreqFFTNorm2             ] );
    else if ( outputatomtype == OutputAtomComplex )         StringCopy ( expfile.FreqTypeName, FrequencyAnalysisNames[ FreqFFTComplex           ] );
    else if ( outputatomtype == OutputAtomPhase   )         StringCopy ( expfile.FreqTypeName, FrequencyAnalysisNames[ FreqFFTPhase             ] );
    }


expfile.NumTracks           = numelsave;
expfile.NumFrequencies      = numsavedfreqs;
expfile.SamplingFrequency   = samplingfrequency;

                                        // technically, we need to shift time to half block forward to be centered
//long                epochtimeoffset     = analysis == FreqAnalysisSTransform ? 0 : blocksize / 2;
                                        // but not applying any shift looks actually more intuitive. It also means we see the time at the beginning of each block
long                epochtimeoffset     = 0;
long                usoffset;

if     ( analysis == FreqAnalysisSTransform ) {
    expfile.NumTime             = RoundAbove ( blocksize         / (double) downsamplingfactor );
    expfile.BlockFrequency      =              samplingfrequency /          downsamplingfactor;
    usoffset                    = TimeFrameToMicroseconds ( timemin, samplingfrequency ); // + 0.25; // ?
    }
else {
    expfile.NumTime             = outputsequential ? numblocks : 1;
    expfile.BlockFrequency      = samplingfrequency / blockstep;
    usoffset                    = TimeFrameToMicroseconds ( timemin + epochtimeoffset, samplingfrequency );
    }

                                        // get the shift from starting point in ms
//long    msoffset = (double) timemin * 1000 / samplingfrequency + 0.5;

                                        // create a new date, correctly shifted of msoffset
expfile.DateTime            = TDateTime (   eegdoc->DateTime.GetYear (),
                                            eegdoc->DateTime.GetMonth (),  
                                            eegdoc->DateTime.GetDay (),
                                            eegdoc->DateTime.GetHour (), 
                                            eegdoc->DateTime.GetMinute (), 
                                            eegdoc->DateTime.GetSecond (), 
                                            eegdoc->DateTime.GetMillisecond (), 
                                            eegdoc->DateTime.GetMicrosecond () + usoffset 
                                        );

expfile.SelTracks           = elsave;
expfile.ElectrodesNames     = xyzdoc.IsOpen () ? *xyzdoc->GetElectrodesNames() : *eegdoc->GetElectrodesNames();

                                        // FREQBIN_MAGICNUMBER2
expfile.FrequencyNames.Set ( numsavedfreqs, sizeof ( TFreqFrequencyName ) );


bool                nofractions;
int                 intwidth;


if      ( outputbands == OutputLinearInterval ) {

    const TOneFrequencyBand*    fb          = freqbands.data ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // check if the output freqs have fractions, if not, we remove the trailing zeroes after the dot
    nofractions = true;

    for ( int fi = fb->SaveFreqMin_i; fi <= fb->SaveFreqMax_i; fi += fb->SaveFreqStep_i ) {

        double      f;

        f   = RoundTo ( fi * datafreqstep_hz, FrequencyResolution );
        if ( Fraction ( f ) != 0 )   nofractions = false;

        f   = RoundTo ( ( fi + ( fb->AvgNumFreqs - 1 ) * fb->AvgFreqStep_i ) * datafreqstep_hz, FrequencyResolution );
        if ( Fraction ( f ) != 0 )   nofractions = false;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we can compute the width of the integer part
    intwidth    = NumIntegerDigits ( RoundTo ( fb->SaveFreqMax_i * datafreqstep_hz, FrequencyResolution ) );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    for ( int fi = fb->SaveFreqMin_i, fi0 = 0; fi <= fb->SaveFreqMax_i; fi += fb->SaveFreqStep_i, fi0++ ) {

        if ( fb->AvgNumFreqs > 1 ) {
                                        // loop down the precision, if needed by lack of space
            for ( int precision = nofractions ? 0 : 2; precision >= 0; precision-- ) {
                                                     // int width + optional fraction width
                sprintf ( buff, "%0*.*lf-%0*.*lf Hz",   intwidth + precision + ( precision ? 1 : 0 ), precision, RoundTo (   fi * datafreqstep_hz, FrequencyResolution ),
                                                        intwidth + precision + ( precision ? 1 : 0 ), precision, RoundTo ( ( fi + ( fb->AvgNumFreqs - 1 ) * fb->AvgFreqStep_i ) * datafreqstep_hz, FrequencyResolution ) );


                if ( StringLength ( buff ) <= sizeof ( TFreqFrequencyName ) - 1 )
                    break;
                }

//            DBGM ( buff, "Interval Freq name" );
            }
        else {
                                        // loop down the precision, if needed by lack of space
            for ( int precision = nofractions ? 0 : 2; precision >= 0; precision-- ) {
                                           // int width + optional fraction width
                sprintf ( buff, "%0*.*lf Hz",           intwidth + precision + ( precision ? 1 : 0 ), precision, RoundTo ( fi * datafreqstep_hz, FrequencyResolution ) );

                if ( StringLength ( buff ) <= sizeof ( TFreqFrequencyName ) - 1 )
                    break;
                }

//            DBGM ( buff, "Single Freq name" );
            }


        ClearVirtualMemory ( expfile.FrequencyNames[ fi0 ],       sizeof ( TFreqFrequencyName ) );
        StringCopy         ( expfile.FrequencyNames[ fi0 ], buff, sizeof ( TFreqFrequencyName ) - 1 );
//        DBGM ( expfile.FrequencyNames[ fi0 ], "Written freq name" );
        }

    }

else if ( outputbands == OutputBands
       || outputbands == OutputLogInterval ) {

                                        // check if the output freqs have fractions, if not, we remove the trailing zeroes after the dot
    nofractions = true;
    intwidth    = 1;
    bool        avgnumfreqs     = false;


    const TOneFrequencyBand*    fb          = freqbands.data ();

    for ( int fbi = 0; fbi < numfreqbands; fbi++, fb++ ) {

        if ( Fraction ( RoundTo ( fb->SaveFreqMin_hz, FrequencyResolution ) ) != 0 )   nofractions = false;
        if ( Fraction ( RoundTo ( fb->SaveFreqMax_hz, FrequencyResolution ) ) != 0 )   nofractions = false;

                                        // compute the biggest width of the integer part
        Maxed ( intwidth, NumIntegerDigits ( AtLeast ( FreqMinLog10Value, RoundTo ( fb->SaveFreqMax_hz, FrequencyResolution ) ) ) );
                                        // test all bands
        avgnumfreqs = avgnumfreqs || ( fb->AvgNumFreqs > 1 );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    fb          = freqbands.data ();

    for ( int fbi = 0; fbi < numfreqbands; fbi++, fb++ ) {

        if ( avgnumfreqs ) {
                                        // loop down the precision, if needed by lack of space
            for ( int precision = nofractions ? 0 : 2; precision >= 0; precision-- ) {
                                                     // int width + optional fraction width
                sprintf ( buff, "%0*.*lf-%0*.*lf Hz",   intwidth + precision + ( precision ? 1 : 0 ), precision, fb->SaveFreqMin_hz, 
                                                        intwidth + precision + ( precision ? 1 : 0 ), precision, fb->SaveFreqMax_hz );

                if ( StringLength ( buff ) <= sizeof ( TFreqFrequencyName ) - 1 )
                    break;
                }
//          DBGM ( buff, "Band Freq name" );
            }
        else {
                                        // loop down the precision, if needed by lack of space
            for ( int precision = nofractions ? 0 : 2; precision >= 0; precision-- ) {
                                           // int width + optional fraction width
                sprintf ( buff, "%0*.*lf Hz",           intwidth + precision + ( precision ? 1 : 0 ), precision, fb->SaveFreqMin_hz );

                if ( StringLength ( buff ) <= sizeof ( TFreqFrequencyName ) - 1 )
                    break;
                }

//            DBGM ( buff, "Single Freq name" );
            }


        ClearVirtualMemory ( expfile.FrequencyNames[ fbi ],       sizeof ( TFreqFrequencyName ) );
        StringCopy         ( expfile.FrequencyNames[ fbi ], buff, sizeof ( TFreqFrequencyName ) - 1 );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( verbosey == Interactive ) {
    Gauge.Next ( gaugefreqglobal );
    CartoolObjects.CartoolApplication->SetMainTitle ( Gauge );
    }

                                        // we read blocks of EEG data
TArray2<float>      blockeeg ( eegdoc->GetTotalElectrodes (), blocksize );  // + EegFilterSideSize ); no filters here!
TArray3<float>      results  (  expfile.NumTime,                                                        // !already transposed for direct file output!
                                expfile.NumTracks, 
                                expfile.NumFrequencies 
                            * ( expfile.GetAtomType ( AtomTypeUseCurrent ) == AtomTypeComplex ? 2 : 1 ) // multiplex the real / imaginary parts manually
                             );

                                        // FFT Approximation variables 
                                        // need a few arrays, with number of output electrodes only
TTracks<AComplex>   blockfft;           // each frequency to be saved, all the electrodes
TTracks<AComplex>   sumfft;             // declare, but don't allocate if not needed
TArray3<AComplex>   fftappr;            // all the intermediate frequencies needed, for all bands
TVector<double>     windowingtable;
int                 numgoodblocks       = 0;

                                        // do create summation buffer
if ( analysis != FreqAnalysisSTransform )       
    blockfft.Resize ( numsavedfreqs, numelsave );

                                        // do create summation buffer
if ( ! outputsequential )                       
    sumfft.Resize ( numsavedfreqs, numelsave );

                                        // do create intermediate buffer
if ( analysis == FreqAnalysisFFTApproximation ) 
    fftappr.Resize ( numfreqbands, maxfftafreqs, numelsave );

                                        // create windowing buffer
if ( windowing == FreqWindowingHanning ) {  

    windowingtable.Resize ( blocksize );

    for ( int i = 0; i < blocksize; i++ )   
                                        // beginning & ending with non-zero values (making the window appear as with +2 bins)
//      windowingtable[ i ] = HanningRescaling * Hanning ( (double) ( i + 1 ) / ( blocksize + 1 ) );
                                        // beginning & ending with zero values
        windowingtable[ i ] = HanningRescaling * Hanning ( (double) i / ( blocksize - 1 ) );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if defined (OutputConstellation)
if ( analysis == FreqAnalysisFFTApproximation ) {
    for ( int fi0 = 0; fi0 <  numsavedfreqs; fi0++ ) {
        sprintf ( buff, "E:\\Data\\Constellation.%02d.spi", fi0 );
        ooo[ fi0 ].open ( buff );
        //ooo << ( numsavefreq * elsave.NumSet () ) << "  " << 1.0 << NewLine;
        ooo[ fi0 ] << StreamFormatScientific;
        }

    for ( int fi0 = 0; fi0 <  numsavedfreqs; fi0++ )
        maxws[ fi0 ]   = 0;
    }
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // MKL FFT processing objects
mkl::TMklFft        fft;
mkl::TMklFft        ffti;

if ( analysis == FreqAnalysisSTransform ) {
                                        // There is no available FFT rescaling option here, as we do the full (forward -> backward) transform
    fft .Set ( mkl::FromReal,      FFTRescalingForward,     blocksize );
    ffti.Set ( mkl::BackToComplex, FFTRescalingBackward,    blocksize );
    }
else { // FFT, Power Maps (also FFT), FFT Approximation
                                        // Recommended FFT rescaling is FFTRescalingParseval
    fft .Set ( mkl::FromReal,      fftnorm,                 blocksize );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // For a REAL INPUT SIGNAL, the optimized FFT will return only ~HALF OF THE RESULTS, namely the "positive" frequencies
                                        // But we still need to ACCOUNT for the corresponding "negative" frequencies, either for the norm or energy, for the Parseval equality to be correct
                                        // But there is a twist, however, as both the 0 and Nyquist frequencies have NO CONJUGATE COUNTERPARTS, so they should NOT be doubled up
                                        // This does not qualify to the full FFT -> FFTI transform, as in that case, we just need to get back to original data (scaling)
//auto    NegFreqFactor   = [ limitfreqmax_i ] ( int fi ) { return (AReal) ( fi == 0 || fi == limitfreqmax_i ? 1 : 2 ); };

                                        // Use if for FFT and FFT Approximation (not nedded for ST, and Phase output):
// NegFreqFactor ( fi ) * F ( fi )  or  NegFreqFactor ( fi ) * abs ( F ( fi ) )  or  NegFreqFactor ( fi ) * norm ( F ( fi ) )

                                        // Finally chose not to make use of it, because it will make a visual / numerical discrepancy between the 0 / Nyquist frequencies and the other doubled frequencies...


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( verbosey == Interactive ) {
    Gauge.Next ( gaugefreqglobal );
    CartoolObjects.CartoolApplication->SetMainTitle ( Gauge );
    }

                                        // scan all blocks
for ( int blocki0 = 0, firsttf = timemin; blocki0 < numblocks; blocki0++, firsttf += blockstep ) {

    if ( verbosey == Interactive && analysis != FreqAnalysisSTransform ) {
        Gauge.Next ( gaugefreqloop );
        CartoolObjects.CartoolApplication->SetMainTitle ( Gauge );
        }


    int             fromtf          = firsttf;
    int             totf            = firsttf + blocksize - 1;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    bool            goodblock       = true;


    if ( badepochs == SkippingBadEpochsList 
      && analysis  != FreqAnalysisSTransform ) {    // we need the data anyway

        for ( int i = 0; i < (int) rejectmarkers; i++ )

            if ( rejectmarkers[ i ]->IsOverlappingInterval ( fromtf, totf ) ) {

                goodblock   = false;
                break;
                }
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( goodblock ) {

        numgoodblocks++;
                                        // read the block of time frames, with all electrodes, without filters and pseudos
        eegdoc->GetTracks   (   fromtf,         totf, 
                                blockeeg,       0, 
                                datatypein, 
                                ComputePseudoTracks, 
                                ref,            &refsel 
                            );
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Settinp up our variables
    if ( analysis == FreqAnalysisFFTApproximation )

        fftappr.ResetMemory ();


    OmpParallelBegin
                                        // Allocating on each new block seems OK compared to the following computation load - Allocating outside the parallel loop complexifies the code, and gains are not assured

    TVector<AReal>      X;              // current block of input, real data

    TVector<AComplex>   F;              // frequency results, complex

                                        // Only for S-Transform:
    TVector<AReal>      GaussianKernel;
    TVector<AComplex>   FG;             // frequencies multiplied by Gaussian Kernel
    TVector<AComplex>   ST;             // S-Transform results
    TVector<AComplex>   SumST;


    if ( analysis == FreqAnalysisSTransform ) {

        X               .Resize ( fft .GetDirectDomainSize ()   );

        F               .Resize ( fft .GetDirectDomainSize ()   );  // !allocate more than the real FFT transform would do, as we need all frequencies for the Analytic Signal!

        GaussianKernel  .Resize ( fft .GetDirectDomainSize ()   );  // same reason
        FG              .Resize ( fft .GetDirectDomainSize ()   );  // same reason
        ST              .Resize ( ffti.GetDirectDomainSize ()   );
        SumST           .Resize ( ffti.GetDirectDomainSize ()   );
        }
    else if ( analysis == FreqAnalysisFFTApproximation ) {

        X               .Resize ( fft.GetDirectDomainSize ()    );
        F               .Resize ( fft.GetFrequencyDomainSize () );  // FFT optimization of real signal allows to allocate half data points
        }
    else { // all other FFT analysis

        X               .Resize ( fft.GetDirectDomainSize ()    );
        F               .Resize ( fft.GetFrequencyDomainSize () );  // FFT optimization of real signal allows to allocate half data points
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    OmpFor
                                        // process only the selected electrodes
                                        // also saving results, except for FFT Approximation where this is done on second part
    for ( int eli = 0; eli < numelsave; eli++ ) {

        int         el      = elsave.GetValue ( eli );

        if ( verbosey == Interactive && analysis == FreqAnalysisSTransform ) {
            Gauge.Next ( gaugefreqloop );
            CartoolObjects.CartoolApplication->SetMainTitle ( Gauge );
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // transfer data + optional windowing
        if ( goodblock ) {

            if ( windowing == FreqWindowingHanning ) 
        
                for ( int i = 0; i < blocksize; i++ )   X ( i ) = blockeeg ( el, i ) * windowingtable ( i );

            else
    //          for ( int i = 0; i < blocksize; i++ )   X ( i ) = blockeeg ( el, i );

                X.CopyMemoryFrom ( &blockeeg ( el, 0 ) );
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( analysis == FreqAnalysisSTransform ) {

                                        // real FFT only - will not touch anything past freqsize in the F vector, which will remain 0
            fft ( X, F );


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Analytic signal (signal + i * Hilbert)
                                        // 1) double the frequencies - !but NOT for f=0 nor f=dim/2!
            for ( int i = 1;        i < freqsize - 1; i++ )     F ( i ) *= 2;
                                        // 2) then clear-up the negative frequencies
//          for ( int i = freqsize; i < blocksize;    i++ )     F ( i )  = 0;   // !buffer was not touched by the real FFT, so it is still 0!


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // loop through the output bands
            const TOneFrequencyBand*    fb          = freqbands.data ();

            for ( int fbi = 0, savedfi0 = 0; fbi < numfreqbands; fbi++, fb++ ) {

                                        // process all the freqs within the current band
                for ( int fi = fb->SaveFreqMin_i; fi <= fb->SaveFreqMax_i; fi += fb->SaveFreqStep_i, savedfi0++ ) {

                                        // clear sum buffer
                    SumST   = (AComplex) 0;


                    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                    for ( int downf = 0, fi2 = fi; downf < fb->AvgNumFreqs; downf++, fi2 += fb->AvgFreqStep_i ) {

                        if ( fi2 == 0 ) {   // 0 Hz - manually computing the mean of signal

                            double      mean        = 0;
                                        // we should have value of 0 in case of Hanning, but actually, we can still compute the average value
//                          if ( windowing == FreqWindowingHanningBorder ) {

                                for ( int i = 0; i < freqsize; i++ )    // data is 0 above freqsize
                                    mean   +=  F[ i ].real ();

                                mean   /= blocksize;
//                              }
                                        // assigning single real value
                            if      ( outputatomtype == OutputAtomComplex )     { SumST = (AComplex)          mean;    }    // we only have a real part
                            if      ( outputatomtype == OutputAtomPhase   )     { SumST = (AComplex)          0;       }    // no imaginary -> phase is 0
                            else if ( outputatomtype == OutputAtomNorm    )     { SumST = (AComplex) abs    ( mean );  }
                            else if ( outputatomtype == OutputAtomNorm2   )     { SumST = (AComplex) Square ( mean );  }
                            else if ( outputatomtype == OutputAtomReal    )     { SumST = (AComplex)          mean;    }    // case shouldn't happen - provide a default formula
                            }

                        else {          // freqs > 0 [Hz]

                            if ( outputatomtype != OutputAtomPhase ) {
                                        // pre-compute Gaussian for current frequency
                                GaussianKernel ( 0 )    = 1.0;

                                for ( int i = 1; i <= blocksize / 2; i++ )
                                        // !Gaussian is centered on 0 / blocksize (not on mid-part)!
                                    GaussianKernel ( i )             = 
                                    GaussianKernel ( blocksize - i ) = expl ( -2.0 * Square ( Pi * i / (double) fi2 ) );

                                        // 3) shift frequencies F to the "left", to be centered on current frequency fi2 (== multiplying by cosine of freq in time series)
                                        // 4) !Actual S-Transform part here, en plus of the Analytic signal: multiply by Gaussian to get smooth results!
                                for ( int i = 0, k = fi2; i < blocksize; i++, k = ++k % blocksize )

                                    FG ( i ) = F ( k ) * GaussianKernel ( i );

                                } // Not Phase
                            else { // Phase
                                        // 3) !no weighted sum, i.e. No S-Transform, just Hilbert phase here!
                                        // reason is that we have "rotating" data, averaging them will not give a meaningful final angle
                                for ( int i = 0, k = fi2; i < blocksize; i++, k = ++k % blocksize )

                                    FG ( i ) = F ( k );

                                }


                            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // End of analytic signal:
                                        // 5) Invert FFT, using full inverse scaling
                            ffti ( FG, ST );    // actually, we could use some in-place transform here, getting rid od FG


                            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here clipping any bad epochs FROM the single data epoch
                            if ( badepochs == SkippingBadEpochsList ) {

                                for ( int mi = 0; mi < (int) rejectmarkers; mi++ ) {

                                    if ( rejectmarkers[ mi ]->IsOverlappingInterval ( fromtf, totf ) ) {

                                            // !markers could actually go beyond buffer!
                                        int         mfrom       = Clip ( rejectmarkers[ mi ]->From, timemin, timemax ) - timemin;
                                        int         mto         = Clip ( rejectmarkers[ mi ]->To,   timemin, timemax ) - timemin;

                                            // zero-ing results at markers intervals
                                        for ( int i = mfrom; i <= mto; i++ )
                                            ST ( i )  = (AComplex) 0;
                                        }
                                    } // for rejectmarkers
                                } // if SkippingBadEpochsList


                            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // apply windowing at the end, to get rid of lousy borders
                                        // this is also known as Cone Of Influence (COI)
                            if ( windowing      == FreqWindowingHanningBorder 
                              && outputatomtype != OutputAtomPhase ) {
                                        // the trick here is to have the width varying with the current frequency
                                        // also paying attention to avoid scaling twice the middle point of the lowest frequency

                                        // Formula that does a good job at cleaning the borders, using 2 (4/8) full cycles on each side (instead of 1)
                                        // It has to be reminded that with Hanning, weighting will be 50% at the middle of that window
//                              constexpr double    numcyclesinhanning  = ( 2 * ( SqrtTwo * 6 / TwoPi ) ); // = 2.70 - from litterature
                                constexpr double    numcyclesinhanning  = 2;

                                double      sthwd       = Clip ( blocksize / (double) fi2 * numcyclesinhanning, 0.0, blocksize / 2.0 );    
                                int         sthwi       = Truncate ( sthwd );

                                        // weighting has a limited support (not applied on all data)
                                for ( int i = 0; i < sthwi; i++ ){
                                        // !using a floating point length, NOT an integer one, to avoid weird clipping artifacts!
                                        // we also use only the first half of the Hanning, the one going 0->1
                                    double      h       = Hanning ( (double) i / sthwd / 2 );

                                        // do 1 half Hanning 0->1 on the left, and the other half on the right 1->0
                                    ST (                 i )   *= h;
                                    ST ( blocksize - 1 - i )   *= h;

                                        // hard steps, useful for debugging / visualization
//                                  h   = TruncateTo ( (double) i / sthwd, 1.0 / numcyclesinhanning );
//                                  ST (                 i )    = h;
//                                  ST ( blocksize - 1 - i )    = h;
                                    }
                                } // post-windowing


                            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // ST is of complex type, some care should be taken for the averaging
                                        // converting to needed type
                            for ( int i = 0; i < blocksize; i++ ) {

                                if      ( outputatomtype == OutputAtomComplex )     { SumST ( i ) +=              ST ( i );         }
                                else if ( outputatomtype == OutputAtomPhase   )     { SumST ( i ) += ArcTangent ( ST ( i ) );       }
                                else if ( outputatomtype == OutputAtomNorm    )     { SumST ( i ) += abs        ( ST ( i ) );       }
                                else if ( outputatomtype == OutputAtomNorm2   )     { SumST ( i ) += norm       ( ST ( i ) );       }
                                else if ( outputatomtype == OutputAtomReal    )     { SumST ( i ) +=              ST ( i ).real (); } // saving ST ( i ) is the original data + filter; abs ( ST ( i ) ) is the envelope
                                } // for i

                            } // freqs > 0 [Hz]

                        } // for downfreq

                    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // average of multiple frequencies
                    SumST  /= fb->AvgNumFreqs;


                    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // in case of downsampling, we don't fear any aliasing effect as a Gaussian has already been applied before
                    for ( int i = 0, i0 = 0; i < blocksize; i += downsamplingfactor, i0++ ) // do all TFs

                        if      ( datatypeout == AtomTypeComplex )  {   results ( i0, eli, 2 * savedfi0     )   = SumST ( i ).real ();
                                                                        results ( i0, eli, 2 * savedfi0 + 1 )   = SumST ( i ).imag ();     }
                        else                                            results ( i0, eli,     savedfi0     )   = SumST ( i ).real ();

                    } // for savefreqs

                } // for freqband

            } // if S-Transform

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Note: FFT Approximation and bands can somehow produce some weird results (summing discontinuities)
                                        // so bands and sub-bands averaging should be avoided
        else if ( analysis == FreqAnalysisFFTApproximation ) {

            if ( ! goodblock )
                continue; // eli

                                        // real FFT only
            fft ( X, F );

                                        // loop through the output bands, saving ALL data of each band
            const TOneFrequencyBand*    fb          = freqbands.data ();

            for ( int fbi = 0;                             fbi   <  numfreqbands;       fbi++, fb++ )
                                        // frequencies by steps
            for ( int fi = fb->SaveFreqMin_i, fftafi0 = 0; fi    <= fb->SaveFreqMax_i;  fi += fb->SaveFreqStep_i )
                                        // we need to store the whole band - BUT only 1 band at a time
            for ( int downf = 0, fi2 = fi;                 downf <  fb->AvgNumFreqs;    downf++, fi2 += fb->AvgFreqStep_i, fftafi0++ )
                                        // simply store in temp
                fftappr ( fbi, fftafi0, eli ) = F ( fi2 );

            } // FFT Approximation


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        else { // all other FFT analysis

            if ( ! goodblock ) {
                                        // do nothing except filling results with 0's
                if ( outputsequential )

                    for ( int fi0 = 0; fi0 < numsavedfreqs; fi0++ )

                        if      ( datatypeout == AtomTypeComplex )  {   results ( blocki0, eli, 2 * fi0     )   = 0;    
                                                                        results ( blocki0, eli, 2 * fi0 + 1 )   = 0; }
                        else                                            results ( blocki0, eli,     fi0     )   = 0;

                                        // simply skipping without summing 0's for sumfft

                continue; // eli
                }


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // real FFT only
            fft ( X, F );


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // loop through the output bands
            const TOneFrequencyBand*    fb          = freqbands.data ();

            for ( int fbi = 0, savedfi0 = 0; fbi < numfreqbands; fbi++, fb++ ) {

                                        // convert & average frequencies by steps
                for ( int fi = fb->SaveFreqMin_i; fi <= fb->SaveFreqMax_i; fi += fb->SaveFreqStep_i, savedfi0++ ) {

                    if      ( outputatomtype == OutputAtomComplex )             { blockfft ( savedfi0, eli )    =              F ( fi );        }   // OK when dealing with norm of complex, but not norm^2
                    else if ( outputatomtype == OutputAtomPhase   )             { blockfft ( savedfi0, eli )    = ArcTangent ( F ( fi ) );      }
                    else if ( outputatomtype == OutputAtomNorm    )             { blockfft ( savedfi0, eli )    = abs        ( F ( fi ) );      }
                    else if ( outputatomtype == OutputAtomNorm2   )             { blockfft ( savedfi0, eli )    = norm       ( F ( fi ) );      }
                    else if ( outputatomtype == OutputAtomReal    )             { blockfft ( savedfi0, eli )    =              F ( fi ).real ();}   // case shouldn't happen - provide a default formula


                                        // allow frequency averaging only if not complex - AvgNumFreqs must be set to 1
                    bool        avgfreqs    = fb->AvgNumFreqs > 1 && ! nofreqavg;

                    if ( avgfreqs ) {   // sum following frequencies

                        for ( int downf = 1, fi2 = fi + fb->AvgFreqStep_i ; downf < fb->AvgNumFreqs; downf++, fi2 += fb->AvgFreqStep_i ) {

                                        // !OutputAtomComplex and OutputAtomPhase types are NOT allowed for averaging!
                            if      ( outputatomtype == OutputAtomNorm    )     { blockfft ( savedfi0, eli )   += abs        ( F ( fi2 ) );         }
                            else if ( outputatomtype == OutputAtomNorm2   )     { blockfft ( savedfi0, eli )   += norm       ( F ( fi2 ) );         }
                            else if ( outputatomtype == OutputAtomReal    )     { blockfft ( savedfi0, eli )   +=              F ( fi2 ).real ();   }   // case shouldn't happen - provide a default formula
                            }

                                        // then average
                        blockfft ( savedfi0, eli ) /= fb->AvgNumFreqs;

                        } // if avgfreqs

                    } // for sub-band

                } // for freqband


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

            for ( int fi0 = 0; fi0 < numsavedfreqs; fi0++ ) {

                if ( outputsequential ) {
                    if      ( datatypeout == AtomTypeComplex )  {   results ( blocki0, eli, 2 * fi0     )   = blockfft ( fi0, eli ).real ();    
                                                                    results ( blocki0, eli, 2 * fi0 + 1 )   = blockfft ( fi0, eli ).imag ();}
                    else                                            results ( blocki0, eli,     fi0     )   = blockfft ( fi0, eli ).real ();
                    } // if outputsequential

                else {
                    if      ( datatypeout == AtomTypeComplex )  {   sumfft  ( fi0, eli )                   += blockfft ( fi0, eli );        }   // case shouldn't happen - not meaningful to average complex numbers!
                    else                                        {   sumfft  ( fi0, eli )                   += blockfft ( fi0, eli ).real ();}   // norm/norm2 has already been computed, and is stored in the Real component
                    } // if average freqs

                } // for savedfreqband

            } // other FFT analysis

        } // for eli


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // stopping before FFT Approximation - ApproximateFrequency is currently the part that has been parallelized
    OmpParallelEnd

                                        // FFT Approximation - filling completed, time to actually process results
                                        // loop is NOT parallelized
    if ( analysis == FreqAnalysisFFTApproximation ) {

        if ( ! goodblock ) {
                                        // do nothing except filling results with 0's
            if ( outputsequential ) {

                const TOneFrequencyBand*    fb          = freqbands.data ();

                for ( int fbi = 0, savedfi0 = 0; fbi  < numfreqbands;     fbi++, fb++ )
                for ( int fi0 = 0;               fi0  < fb->SaveNumFreqs; fi0++, savedfi0++ )
                for ( int eli2 = 0;              eli2 < numelsave;        eli2++ )

                    if      ( datatypeout == AtomTypeComplex )  {   results ( blocki0, eli2, 2 * savedfi0       )   = 0;  
                                                                    results ( blocki0, eli2, 2 * savedfi0 + 1   )   = 0;   }
                    else                                            results ( blocki0, eli2,     savedfi0       )   = 0;
                }
                                        // simply skipping without summing 0's for sumfft

            continue; // blocki0
            }

                                        // loop through the output bands
        const TOneFrequencyBand*    fb          = freqbands.data ();

        for ( int fbi = 0, savedfi0 = 0; fbi < numfreqbands; fbi++, fb++ ) {

                                        // here all electrodes have been done, now proceed to approximation
            blockfft.ResetMemory ();    // we are going to average things

                                        // re-do again the frequencies scanning
            for ( int fi = fb->SaveFreqMin_i, fi0 = 0, fftafi0 = 0; fi <= fb->SaveFreqMax_i; fi += fb->SaveFreqStep_i, fi0++ ) {

#if defined (OutputConstellation)
                toooo   = &ooo[ fi0 ];
                                        // compute max norm electrode for each frequency, on the first block
                if ( maxws[ fi0 ] == 0 ) {
                    maxw    = 0;
                    for ( int eli2 = 0; eli2 < numelsave; eli2++ )
                        Maxed ( maxw, (double) abs ( fftappr ( fbi, fftafi0, eli2 ) ) );
                    if ( maxw == 0 )
                        maxw    = 1;
                    maxws[ fi0 ]    = 100 / maxw;
                    }
                                        // get a per-frequency scaling
                maxw    = maxws[ fi0 ];
                                        // time is in Z
                orgz    = 2 * blocki0;
#endif
                                        // the actual approximation is here
                ApproximateFrequency ( &fftappr ( fbi, fftafi0, 0 ), numelsave );

                                        // transfer results
                for ( int eli2 = 0; eli2 < numelsave; eli2++ )
                    blockfft ( fi0, eli2 )  = fftappr ( fbi, fftafi0, eli2 );


                                        // allow frequency averaging only if not complex - AvgNumFreqs must be set to 1
                bool        avgfreqs    = fb->AvgNumFreqs > 1 && ! nofreqavg;

                if ( avgfreqs ) {   // sum following frequencies

                    fftafi0++;

                    for ( int downf = 1, fi2 = fi + fb->AvgFreqStep_i ; downf < fb->AvgNumFreqs; downf++, fi2 += fb->AvgFreqStep_i, fftafi0++  ) {


                        ApproximateFrequency ( &fftappr ( fbi, fftafi0, 0 ), numelsave );

                                    // check for negative correlation with first freq
                        if ( IsNegativeCorrelation ( &fftappr ( fbi, fftafi0, 0 ), &fftappr ( fbi, fftafi0 - downf, 0 ), numelsave ) )

                            InvertFreqMap ( &fftappr ( fbi, fftafi0, 0 ), numelsave );

                                    // now can sum, all polarities are coherents
                        for ( int eli2 = 0; eli2 < numelsave; eli2++ )
                            blockfft ( fi0, eli2 ) += fftappr ( fbi, fftafi0, eli2 );
                        } // for downf

                                    // then rescale
                    for ( int eli2 = 0; eli2 < numelsave; eli2++ )
                        blockfft ( fi0, eli2 ) /= fb->AvgNumFreqs;
                    } // avgfreqs

                else // don't forget to step

                    fftafi0     += fb->AvgNumFreqs;
                } // for saved frequencies


            //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                    // Writing to file
            for ( int fi0 = 0; fi0 <  fb->SaveNumFreqs; fi0++, savedfi0++ ) {
                                    // we have to explicitly run an electrodes loop here, as we disrupted the outer electrode loop
                for ( int eli2 = 0; eli2 < numelsave; eli2++ ) {

                    if ( outputsequential ) {

                        if      ( datatypeout == AtomTypeComplex )  {   results ( blocki0, eli2, 2 * savedfi0       )   = blockfft ( fi0, eli2 ).real ();  
                                                                        results ( blocki0, eli2, 2 * savedfi0 + 1   )   = blockfft ( fi0, eli2 ).imag ();   }
                        else                                            results ( blocki0, eli2,     savedfi0       )   = blockfft ( fi0, eli2 ).real ();
                        } // if outputsequential

                    else {                      // one final step is to do a temporal averaging
                                                // again, be careful to polarity if time averaging FFTA
                        if ( blocki0 > 0 && IsNegativeCorrelation ( blockfft[ fi0 ], sumfft[ savedfi0 ], numelsave ) )

                            InvertFreqMap ( blockfft[ fi0 ], numelsave );

                        if      ( datatypeout == AtomTypeComplex )  {   sumfft ( savedfi0, eli2 )                      += blockfft ( fi0, eli2 );           }   // case shouldn't happen - not meaningful to average in C!
                        else                                        {   sumfft ( savedfi0, eli2 )                      += blockfft ( fi0, eli2 ).real ();   }   // norm, norm2 have already been computed, in the Real part
                        }
                    }
                } // for freq

            } // for freqband

        } // FFT Approximation

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    } // for blocki0

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( verbosey == Interactive ) {
    Gauge.Next ( gaugefreqglobal );
    CartoolObjects.CartoolApplication->SetMainTitle ( Gauge );
    }


#if defined (OutputConstellation)
if ( analysis == FreqAnalysisFFTApproximation ) {
    for ( int fi0 = 0; fi0 <  numsavedfreqs; fi0++ ) {
        ooo[fi0].close ();
        }
    }
#endif

                                        // write all data in one shot
if ( outputsequential 
  || analysis == FreqAnalysisSTransform )

    expfile.Write ( results, NotTransposed );

else {                                  // writing temporal mean
                                        // we averaged only the good blocks
    sumfft     /= NonNull ( numgoodblocks );

                                        // go through the selected electrodes
    for ( int eli = 0; eli < numelsave; eli++ ) {
                                        // process all saved frequencies
        for ( int fi0 = 0; fi0 < numsavedfreqs; fi0++ )
                                        // just test between complex case versus all the others
            if      ( datatypeout == AtomTypeComplex )  expfile.Write ( sumfft ( fi0, eli ),         0, eli, fi0 );
            else                                        expfile.Write ( sumfft ( fi0, eli ).real (), 0, eli, fi0 );
        } // for electrode


    verbose.NextBlock ();
    verbose.Put ( "Number of blocks:", numblocks );
    verbose.Put ( "Number of good blocks:", numgoodblocks );
    verbose.Put ( "Percentage of good blocks:", 100 * numgoodblocks / (double) numblocks, 2, "%" );
    verbose.Flush ();
    }

expfile.End ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // re-order results
if ( analysis == FreqAnalysisFFTApproximation )
    SortFFTApproximation ( fileoutfreq );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // restore old filters
eegdoc->SetFiltersActivated ( oldfiltersactivated, true );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // write a marker file
if ( verbosey == Interactive ) {
    Gauge.Next ( gaugefreqglobal );
    CartoolObjects.CartoolApplication->SetMainTitle ( Gauge );
    }


if ( outputmarkers ) {

    ofstream        ofmrk ( TFileName ( fileoutmrk, TFilenameExtendedPath ) );

    WriteMarkerHeader ( ofmrk );


    double          mrkdownsampling = analysis == FreqAnalysisSTransform ? downsamplingfactor : blockstep;
    TMarker         marker;
    int             nummarkers      = 0;

    marker.Set ( -1 );                  // start from first trigger

                                        // read triggers as currently selected
    for ( ; eegdoc->GetNextMarker ( marker, true, outputmarkerstype ) && marker.From <= timemax; ) {
                                        // keep markers that overlap
        if ( marker.IsOverlappingInterval ( timemin, timemax ) ) {
                                        // but take care to clip them!
            WriteMarker (   ofmrk, 
                            Clip ( Round ( ( marker.From - ( timemin + epochtimeoffset ) ) / mrkdownsampling ), 0, (int) expfile.NumTime - 1 ),
                            Clip ( Round ( ( marker.To   - ( timemin + epochtimeoffset ) ) / mrkdownsampling ), 0, (int) expfile.NumTime - 1 ),
                            marker.Name 
                        );

            nummarkers++;
            }
        } // for marker


    ofmrk.close ();

                                        // erase a file with 0 triggers, and prevent duplication
    if ( nummarkers == 0 ) {

        DeleteFileExtended ( fileoutmrk );
        outputmarkers   = false;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // ouput to different files
if ( verbosey == Interactive ) {
    Gauge.Next ( gaugefreqglobal );
    CartoolObjects.CartoolApplication->SetMainTitle ( Gauge );
    }


 if ( splitelectrode || splitfrequency || splitspectrum ) {

    TGoF                goffreqdoc;
    TGoGoF              gofsplit;
                                        // add our single freq file
    goffreqdoc.Add ( fileoutfreq );


    if ( splitfrequency ) {

        goffreqdoc.SplitFreqFiles ( SplitFreqByFrequency, &gofsplit, false );

        gofsplit.CopyFilesTo      ( BaseDirSplitFreqs, (CopyToFlags) ( CopyAndDeleteOriginals | CopyAndUpdateFileNames | CopyAllKnownBuddies ) );
        }


    if ( splitelectrode ) {

        goffreqdoc.SplitFreqFiles ( SplitFreqByElectrode, &gofsplit, false );

        gofsplit.CopyFilesTo      ( BaseDirSplitElecs, (CopyToFlags) ( CopyAndDeleteOriginals | CopyAndUpdateFileNames | CopyAllKnownBuddies ) );
        }


    if ( splitspectrum ) {

        goffreqdoc.SplitFreqFiles ( SplitFreqByTime, &gofsplit, false );

        gofsplit.CopyFilesTo      ( BaseDirSpectrum, (CopyToFlags) ( CopyAndDeleteOriginals | CopyAndUpdateFileNames | CopyAllKnownBuddies ) );
        }

    }

                                        // now, we're finished with these files
if ( ! savefreq ) {
    DeleteFileExtended ( fileoutfreq );
    DeleteFileExtended ( fileoutmrk  );
    }


verbose.NextLine ( 2 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // closing xyzdoc?
if ( xyzdoc.IsOpen () && xyzdoc->CanClose ( true ) )

    xyzdoc.Close ( isxyzdocalreadyopen /*|| BatchProcessing*/ ? CloseDocLetOpen : CloseDocRestoreAsBefore );


if ( verbosey == Interactive ) {

    Gauge.FinishParts ();

    CartoolObjects.CartoolApplication->SetMainTitle ( FrequencyAnalysisTitle, "", Gauge );

    UpdateApplication;
    }


return  true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
