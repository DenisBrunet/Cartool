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

#include    <owl/pch.h>

#include    "Files.Utils.h"
#include    "TArray3.h"
#include    "Strings.Utils.h"
#include    "Strings.Grep.h"
#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"

#include    "TExportTracks.h"

#include    "TTracksDoc.h" 
#include    "TFreqCartoolDoc.h"         // TFreqFrequencyName

#include    "TMicroStates.h"
#include    "GenerateData.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void    TCartoolMdiClient::AnalyzeGeneratedDataUI ( owlwparam w )
{
GenerateTypeFlag    what            = w == CM_ANALYZEGENERATEDATAEEG ? GenerateEeg
                                    : w == CM_ANALYZEGENERATEDATARIS ? GenerateRis
                                    :                                  GenerateUnknown;

if ( what == GenerateUnknown )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           path;


GetFileFromUser     getbasefile ( "Directory with synthetic data:", AllFilesFilter, 1, GetFileDirectory );

if ( ! getbasefile.Execute () || (int) getbasefile < 1 )
    return;
else 
    StringCopy ( path, getbasefile[ 0 ] );

                                        // move 1 directory up
RemoveFilename      ( path );

                                        // force 1 common output directory
//StringCopy ( basefilename, "E:\\Data\\Segmentation\\SynthData\\SynthData" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           grepmaps;
TGoF                gofmaps;

if ( what == GenerateEeg )  StringCopy ( grepmaps,  "\\.Maps [0-9]+\\." );
else                        StringCopy ( grepmaps,  "\\.Ris [0-9]+\\." );

                                        // !The advantage right here is that it will gather all directories with '.Maps XX' in them
                                        // so in case of parallel computation with slightly different base file names, like base1 base2 base3 etc...
                                        // all these baseX directories will be merged together for the stats!
if ( ! gofmaps.GrepFiles    ( path, grepmaps, GrepOptionDefaultFiles, false ) )  
    return;

//gofmaps.Show ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 map;
int                 corr;
int                 noise;
int                 noiseoffset     = 1000;
TSelection          mapsel   ( 1000, OrderSorted );
TSelection          corrsel  ( 1000, OrderSorted );
TSelection          noisesel ( 2000, OrderSorted );

                                        // browse all file names to retrieve max sizes
for ( int diri = 0; diri < (int) gofmaps; diri++ ) {
                                        // retrieve number of maps + levels of correlation / noise
    if ( what == GenerateEeg )  
        sscanf ( StringContains ( (const char*) gofmaps[ diri ], "Maps ", StringContainsBackward ), "Maps %ld.Corr %ld.Noisedb %ld", &map, &corr, &noise );
    else
        sscanf ( StringContains ( (const char*) gofmaps[ diri ], "Ris ",  StringContainsBackward ), "Ris %ld.Corr %ld.Noisedb %ld",  &map, &corr, &noise );

    mapsel  .Set ( map                 );
    corrsel .Set ( corr                );
    noisesel.Set ( noise + noiseoffset );   // noise can be negative, so offset values

//    DBGV4 ( diri, map, corr, noisedb, ToFileName ( gofmaps[ diri ] ) );
    } // for diri (map x corr x noise)


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Allocate storage
int                 nummap          = mapsel  .LastSelected ();
int                 numcorr         = corrsel .NumSet ();
int                 numnoise        = noisesel.NumSet ();

                                        // !segnumvar is too big, we are actually with .error.data file, but that's OK!
TArray3<double>*    anmean          = new TArray3<double> [ segnumvar ];
TArray3<double>*    anmax           = new TArray3<double> [ segnumvar ];


for ( int ci = 0; ci < segnumvar; ci++ ) {
                                        // map 1 -> index 0
    anmean[ ci ].Resize ( numcorr, nummap, numnoise );
    anmax [ ci ].Resize ( numcorr, nummap, numnoise );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TGoF                goferrordata;


TOpenDoc<TTracksDoc>        eegdoc;
TTracks<float>              EegBuff;
TGoEasyStats                statsRankMCN( segnumvar                     /*, 100*/ );    // actually ( numcrit, (int) goferrordata )
TGoEasyStats                statsRankMC ( segnumvar * nummap  * numcorr /*, 100*/ );    // Map x Corr,   cumulated in file & noise
TGoEasyStats                statsRankMN ( segnumvar * nummap  * numnoise/*, 100*/ );    // Map X Noise,  cumulated in file & corr
TGoEasyStats                statsRankCN ( segnumvar * numcorr * numnoise/*, 100*/ );    // Corr x Noise, cumulated in file & map
TGoEasyStats                statsRankM  ( segnumvar * nummap            /*, 100*/ );    // Map,   cumulated in file & noise & corr
TGoEasyStats                statsRankC  ( segnumvar * numcorr           /*, 100*/ );    // Corr,  cumulated in file & noise & map
TGoEasyStats                statsRankN  ( segnumvar * numnoise          /*, 100*/ );    // Noise, cumulated in file & map & corr
TGoEasyStats                statsRankAll( segnumvar                     /*, 100*/ );    //        cumulated in file & noise & corr & map


TGoEasyStats                statsMaxMCN ( segnumvar                     /*, 100*/ );    // actually ( numcrit, (int) goferrordata )
TGoEasyStats                statsMaxMC  ( segnumvar * nummap  * numcorr /*, 100*/ );    // Map x Corr,   cumulated in file & noise
TGoEasyStats                statsMaxMN  ( segnumvar * nummap  * numnoise/*, 100*/ );    // Map X Noise,  cumulated in file & corr
TGoEasyStats                statsMaxCN  ( segnumvar * numcorr * numnoise/*, 100*/ );    // Corr x Noise, cumulated in file & map
TGoEasyStats                statsMaxM   ( segnumvar * nummap            /*, 100*/ );    // Map,   cumulated in file & noise & corr
TGoEasyStats                statsMaxC   ( segnumvar * numcorr           /*, 100*/ );    // Corr,  cumulated in file & noise & map
TGoEasyStats                statsMaxN   ( segnumvar * numnoise          /*, 100*/ );    // Noise, cumulated in file & map & corr
TGoEasyStats                statsMaxAll ( segnumvar                     /*, 100*/ );    //        cumulated in file & noise & corr & map


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;
Gauge.Set           ( "Data Analysis" );
Gauge.AddPart       ( 0, (int) gofmaps );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numcriterrordata    = 0;
int                 mincriterrordatai;
int                 maxcriterrordatai;
int                 numcriterrordatai;
int                 numclusterrordata;
int                 mapi;
int                 corri;
int                 corrinvi;
int                 noisei;
int                 noiseinvi;
int                 metacriti;
double              rank;
double              maxcritpos;
TStrings            critnames;

                                        // browse again to extract all .error.data
for ( int diri = 0; diri < (int) gofmaps; diri++ ) {

    Gauge.Next ( 0 );

    CartoolApplication->SetMainTitle ( "Data Analysis", gofmaps[ diri ], Gauge );

                                        // retrieve number of maps + levels of correlation / noise
    if ( what == GenerateEeg )  
        sscanf ( StringContains ( (const char*) gofmaps[ diri ], "Maps ", StringContainsBackward ), "Maps %ld.Corr %ld.Noisedb %ld", &map, &corr, &noise );
    else
        sscanf ( StringContains ( (const char*) gofmaps[ diri ], "Ris ",  StringContainsBackward ), "Ris %ld.Corr %ld.Noisedb %ld",  &map, &corr, &noise );


    mapi        = map - 1;                                      // !we force scan all maps!
    corri       = corrsel .GetIndex ( corr                );    // value to index
    corrinvi    = numcorr  - 1 - corri;                         // inverted index
    noisei      = noisesel.GetIndex ( noise + noiseoffset );    // value to index
    noiseinvi   = numnoise - 1 - noisei;                        // inverted index


    if ( ! goferrordata.GrepFiles   ( gofmaps[ diri ], AllErrorDataGrep, GrepOptionDefaultFiles ) )
        continue;

                                        // for each map/corr/noise
    statsRankMCN.Reset ();
    statsMaxMCN .Reset ();

                                        // now browse through all error.data for the particular directory
    for ( int filei = 0; filei < (int) goferrordata; filei++ ) {
                                        // this is not optimal, we load all the data while we only need header informations...
        if ( ! eegdoc.Open ( goferrordata[ filei ], OpenDocHidden ) )
            continue;

                                        // only now we now - done once actually
        if ( numcriterrordata == 0 ) {
            numcriterrordata    = eegdoc->GetNumElectrodes ();
            numclusterrordata   = eegdoc->GetNumTimeFrames ();

            mincriterrordatai   = 3;                                // hardwired: skip first 3 lines - could scan the names, too
//          maxcriterrordatai   = numcriterrordata - 2;             // hardwired: don't save meta-criterion
            maxcriterrordatai   = numcriterrordata - 1;             // include all, including meta-criterion
            numcriterrordatai   = maxcriterrordatai - mincriterrordatai + 1;
            metacriti           = numcriterrordata - 1;

            critnames           = *eegdoc->GetElectrodesNames ();
            }

                                        // get tracks with file reference, filtering off
        eegdoc->GetTracks ( 0, numclusterrordata - 1, EegBuff );

                                        // we look here only for the ranking value computed at the exact number of clusters
        for ( int criti  = mincriterrordatai; criti  <= maxcriterrordatai;  criti++  ) {

                                        // criterion is a linear rank in range [0..1]
            rank    = EegBuff ( criti, mapi );

                                        // Meta-Criterion is a geometrical rank (1 1/2 1/3 1/4), so convert it to linear range [0..1]
            if ( criti == metacriti && rank > 0 )
                rank    = ( numclusterrordata + 1 - 1.0 / rank ) / numclusterrordata;

                                        // no stats on null data (shouldn't happen, but still), as a ranking is non-null
                                        // give an exception for metacriterion, which is now a single blip, so value 0 is valid
//          if ( rank > 0 || criti == metacriti ) {
                                        // error.data first "TF" is for cluster 1, but we have a 0-based index
                statsRankMCN[   criti                                         ].Add ( rank );
                                        // cumulate in 2D
                statsRankMC [ ( criti * numcorr  + corri  ) * nummap  + mapi  ].Add ( rank );
                statsRankMN [ ( criti * numnoise + noisei ) * nummap  + mapi  ].Add ( rank );
                statsRankCN [ ( criti * numnoise + noisei ) * numcorr + corri ].Add ( rank );
                                        // cumulate in 1D
                statsRankM  [   criti * nummap   + mapi                       ].Add ( rank );
                statsRankC  [   criti * numcorr  + corri                      ].Add ( rank );
                statsRankN  [   criti * numnoise + noisei                     ].Add ( rank );
                                        // cumulate in 0D (ending up with a single value for each criterion)
                statsRankAll[   criti                                         ].Add ( rank );
//              }


                                        // Other way round: look at the distance from the max criterion to actual number of maps
            for ( maxcritpos = 0; maxcritpos < numclusterrordata; maxcritpos++ )
                if ( EegBuff ( criti, maxcritpos ) > 0.999 )    // == 1.0, but be safe about floating points
                    break;

                                        // check we really have a max
            if ( maxcritpos < numclusterrordata ) {
                                        // invert & normalize difference, to have 1 for best match, 0 for worse match
                                        // both 0-base indexes; skipping 1 & 2 clustering

                                        // relative distance, normalized and inverted
                maxcritpos      = 1 - fabs ( maxcritpos - mapi ) / ( numclusterrordata - 3 );


                statsMaxMCN[   criti                                         ].Add ( maxcritpos );
                statsMaxMC [ ( criti * numcorr  + corri  ) * nummap  + mapi  ].Add ( maxcritpos );
                statsMaxMN [ ( criti * numnoise + noisei ) * nummap  + mapi  ].Add ( maxcritpos );
                statsMaxCN [ ( criti * numnoise + noisei ) * numcorr + corri ].Add ( maxcritpos );
                                        // cumulate in 1D
                statsMaxM  [   criti * nummap   + mapi                       ].Add ( maxcritpos );
                statsMaxC  [   criti * numcorr  + corri                      ].Add ( maxcritpos );
                statsMaxN  [   criti * numnoise + noisei                     ].Add ( maxcritpos );
                                        // cumulate in 0D (ending up with a single value for each criterion)
                statsMaxAll[   criti                                         ].Add ( maxcritpos );
                }
            }


        eegdoc.Close ();
        } // for filei

                                        // store at the right place
    for ( int criti  = mincriterrordatai; criti  <= maxcriterrordatai;  criti++  ) {
                                        // invert correlations: low values at bottom
                                        // invert noise: lowest values on top (maximum noise in dB)
        anmean[ criti ] ( corrinvi, mapi, noiseinvi ) = statsRankMCN[ criti ].Mean ();
        anmax [ criti ] ( corrinvi, mapi, noiseinvi ) = statsMaxMCN [ criti ].Mean ();
        }

    } // for diri (map x corr x noise)


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here we need a big bunch of different outputs, about all combinations
                                        // of dimensions and merges...
char                buff[ 256 ];
TFileName           basefilename;

StringCopy  ( basefilename,     path,   "\\SynthAnalysis.Rank" );


TExportTracks       expfile3;
expfile3.SetAtomType ( AtomTypeScalar );
expfile3.NumTracks          = numcorr;
expfile3.NumTime            = nummap;
expfile3.NumFrequencies     = numnoise;

expfile3.ElectrodesNames.Set ( expfile3.NumTracks, ElectrodeNameSize );
for ( int corri  = 0, corrinvi  = numcorr  - 1 - corri;  corri  < numcorr;  corri++,  corrinvi--  )
                                        // invert correlations: lowest values at the bottom
    sprintf ( expfile3.ElectrodesNames[ corri ], "Corr%02d", corrsel.GetValue ( corrinvi ) );

expfile3.FrequencyNames.Set ( expfile3.NumFrequencies, sizeof ( TFreqFrequencyName ) );
for ( int noisei = 0, noiseinvi = numnoise - 1 - noisei; noisei < numnoise; noisei++, noiseinvi-- )
                                        // Frequency names are inverted, first values at the bottom
    sprintf ( expfile3.FrequencyNames[ noisei ], "Noise%02d", noisesel.GetValue ( noiseinvi ) - noiseoffset );



TExportTracks       expfile2MC;

StringCopy      ( expfile2MC.Filename, basefilename );
StringAppend    ( expfile2MC.Filename, ".MeanNoise.MapCorr" );
AddExtension    ( expfile2MC.Filename, FILEEXT_FREQ );

expfile2MC.SetAtomType ( AtomTypeScalar );
expfile2MC.NumTracks        = numcorr;
expfile2MC.NumTime          = nummap;
expfile2MC.NumFrequencies   = numcriterrordatai;

expfile2MC.ElectrodesNames.Set ( expfile2MC.NumTracks, ElectrodeNameSize );
for ( int corri  = 0, corrinvi  = numcorr  - 1 - corri;  corri  < numcorr;  corri++,  corrinvi--  )
                                        // invert correlations: lowest values at the bottom
    sprintf ( expfile2MC.ElectrodesNames[ corri ], "Corr%02d", corrsel.GetValue ( corrinvi ) );

expfile2MC.FrequencyNames.Set ( expfile2MC.NumFrequencies, sizeof ( TFreqFrequencyName ) );
for ( int criti = mincriterrordatai, crit0invi = numcriterrordatai - 1; criti <= maxcriterrordatai; criti++, crit0invi-- )
                                        // Frequency names are inverted, first values at the bottom
    StringCopy ( expfile2MC.FrequencyNames[ crit0invi ], critnames[ criti ] );



TExportTracks       expfile2MN;

StringCopy      ( expfile2MN.Filename, basefilename );
StringAppend    ( expfile2MN.Filename, ".MeanCorr.MapNoise" );
AddExtension    ( expfile2MN.Filename, FILEEXT_FREQ );

expfile2MN.SetAtomType ( AtomTypeScalar );
expfile2MN.NumTracks        = numnoise;
expfile2MN.NumTime          = nummap;
expfile2MN.NumFrequencies   = numcriterrordatai;

expfile2MN.ElectrodesNames.Set ( expfile2MN.NumTracks, ElectrodeNameSize );
for ( int noisei = 0; noisei < numnoise; noisei++ )
                                        // noise names are inverted twice: we want the highest values at the bottom, tracks are inverted
    sprintf ( expfile2MN.ElectrodesNames[ noisei ], "Noise%02d", noisesel.GetValue ( noisei ) - noiseoffset );

expfile2MN.FrequencyNames.Set ( expfile2MN.NumFrequencies, sizeof ( TFreqFrequencyName ) );
for ( int criti = mincriterrordatai, crit0invi = numcriterrordatai - 1; criti <= maxcriterrordatai; criti++, crit0invi-- )
                                        // Frequency names are inverted, first values at the bottom
    StringCopy ( expfile2MN.FrequencyNames[ crit0invi ], critnames[ criti ] );



TExportTracks       expfile2CN;

StringCopy      ( expfile2CN.Filename, basefilename );
StringAppend    ( expfile2CN.Filename, ".MeanMap.CorrNoise" );
AddExtension    ( expfile2CN.Filename, FILEEXT_FREQ );

expfile2CN.SetAtomType ( AtomTypeScalar );
expfile2CN.NumTracks        = numnoise;
expfile2CN.NumTime          = numcorr;
expfile2CN.NumFrequencies   = numcriterrordatai;

expfile2CN.ElectrodesNames.Set ( expfile2CN.NumTracks, ElectrodeNameSize );
for ( int noisei = 0; noisei < numnoise; noisei++ )
                                        // noise names are inverted twice: we want the highest values at the bottom, tracks are inverted
    sprintf ( expfile2CN.ElectrodesNames[ noisei ], "Noise%02d", noisesel.GetValue ( noisei ) - noiseoffset );

expfile2CN.FrequencyNames.Set ( expfile2CN.NumFrequencies, sizeof ( TFreqFrequencyName ) );
for ( int criti = mincriterrordatai, crit0invi = numcriterrordatai - 1; criti <= maxcriterrordatai; criti++, crit0invi-- )
                                        // Frequency names are inverted, first values at the bottom
    StringCopy ( expfile2CN.FrequencyNames[ crit0invi ], critnames[ criti ] );



TExportTracks       expfile1M;
StringCopy      ( expfile1M.Filename, basefilename );
StringAppend    ( expfile1M.Filename, ".MeanCorrNoise.Map" );
AddExtension    ( expfile1M.Filename, FILEEXT_EEGSEF );
expfile1M.SetAtomType ( AtomTypeScalar );
expfile1M.NumTracks        = numcriterrordatai;
expfile1M.NumTime          = nummap;

expfile1M.ElectrodesNames.Set ( expfile1M.NumTracks, ElectrodeNameSize );
for ( int criti = mincriterrordatai, criti0 = 0; criti <= maxcriterrordatai; criti++, criti0++ )
    StringCopy ( expfile1M.ElectrodesNames[ criti0 ], critnames[ criti ] );




TExportTracks       expfile1C;
StringCopy      ( expfile1C.Filename, basefilename );
StringAppend    ( expfile1C.Filename, ".MeanMapNoise.Corr" );
AddExtension    ( expfile1C.Filename, FILEEXT_EEGSEF );
expfile1C.SetAtomType ( AtomTypeScalar );
expfile1C.NumTracks        = numcriterrordatai;
expfile1C.NumTime          = numcorr;

expfile1C.ElectrodesNames.Set ( expfile1C.NumTracks, ElectrodeNameSize );
for ( int criti = mincriterrordatai, criti0 = 0; criti <= maxcriterrordatai; criti++, criti0++ )
    StringCopy ( expfile1C.ElectrodesNames[ criti0 ], critnames[ criti ] );



TExportTracks       expfile1N;
StringCopy      ( expfile1N.Filename, basefilename );
StringAppend    ( expfile1N.Filename, ".MeanMapCorr.Noise" );
AddExtension    ( expfile1N.Filename, FILEEXT_EEGSEF );
expfile1N.SetAtomType ( AtomTypeScalar );
expfile1N.NumTracks        = numcriterrordatai;
expfile1N.NumTime          = numnoise;

expfile1N.ElectrodesNames.Set ( expfile1N.NumTracks, ElectrodeNameSize );
for ( int criti = mincriterrordatai, criti0 = 0; criti <= maxcriterrordatai; criti++, criti0++ )
    StringCopy ( expfile1N.ElectrodesNames[ criti0 ], critnames[ criti ] );



TExportTracks       expfile0;
StringCopy      ( expfile0.Filename, basefilename );
StringAppend    ( expfile0.Filename, ".MeanMapCorrNoise" );
AddExtension    ( expfile0.Filename, FILEEXT_EEGSEF );
expfile0.SetAtomType ( AtomTypeScalar );
expfile0.NumTracks        = numcriterrordatai;
expfile0.NumTime          = 1;

expfile0.ElectrodesNames.Set ( expfile0.NumTracks, ElectrodeNameSize );
for ( int criti = mincriterrordatai, criti0 = 0; criti <= maxcriterrordatai; criti++, criti0++ )
    StringCopy ( expfile0.ElectrodesNames[ criti0 ], critnames[ criti ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int criti = mincriterrordatai, criti0 = 0, crit0invi = numcriterrordatai - 1; criti <= maxcriterrordatai; criti++, criti0++, crit0invi-- ) {

    StringCopy      ( expfile3.Filename, basefilename );
    StringAppend    ( expfile3.Filename, ".Crit ", IntegerToString ( buff, criti0 + 1, 2 ) );
    StringAppend    ( expfile3.Filename, ".", critnames[ criti ] );
    AddExtension    ( expfile3.Filename, FILEEXT_FREQ );

    expfile3.Write  ( anmean[ criti ] );
    expfile3.End    ();
    } // for criti


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int criti = mincriterrordatai, criti0 = 0, crit0invi = numcriterrordatai - 1; criti <= maxcriterrordatai; criti++, criti0++, crit0invi-- ) {


    for ( int mapi  = 0; mapi  < nummap;  mapi ++ )
    for ( int corri  = 0, corrinvi  = numcorr  - 1 - corri;  corri  < numcorr;  corri++,  corrinvi--  )
                                        // invert correlation: lowest correlation as lower tracks
        expfile2MC.Write ( statsRankMC[ ( criti * numcorr  + corri  ) * nummap  + mapi  ].Mean (), mapi, corrinvi, crit0invi );


    for ( int mapi   = 0; mapi   < nummap;   mapi  ++ )
    for ( int noisei = 0, noiseinvi = numnoise - 1 - noisei; noisei < numnoise; noisei++, noiseinvi-- )
                                        // invert twice the noise: lowest noise value on top == maximum noise on top
        expfile2MN.Write ( statsRankMN[ ( criti * numnoise + noisei ) * nummap  + mapi  ].Mean (), mapi, noisei,   crit0invi );


    for ( int corri  = 0, corrinvi  = numcorr  - 1 - corri;  corri  < numcorr;  corri++,  corrinvi--  )
    for ( int noisei = 0, noiseinvi = numnoise - 1 - noisei; noisei < numnoise; noisei++, noiseinvi-- )
                                        // invert twice the noise: lowest noise value on top == maximum noise on top
        expfile2CN.Write ( statsRankCN[ ( criti * numnoise + noisei ) * numcorr + corri ].Mean (), corri, noisei,  crit0invi );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    for ( int mapi   = 0; mapi   < nummap;   mapi  ++ )
        expfile1M.Write ( statsRankM[   criti * nummap   + mapi                       ].Mean (), mapi,      criti0 );

                                                      
    for ( int corri  = 0;  corri  < numcorr;  corri++ )
        expfile1C.Write ( statsRankC[   criti * numcorr  + corri                      ].Mean (), corri,     criti0 );

                                        // invert noise, highest values on the left
    for ( int noisei = 0, noiseinvi = numnoise - 1 - noisei; noisei < numnoise; noisei++, noiseinvi-- )
        expfile1N.Write ( statsRankN[   criti * numnoise + noisei                     ].Mean (), noiseinvi, criti0 );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    expfile0.Write ( statsRankAll[   criti                                            ].Mean (), 0,         criti0 );

    } // for criti


expfile2MC.End ();
expfile2MN.End ();
expfile2CN.End ();
expfile1M .End ();
expfile1C .End ();
expfile1N .End ();
expfile0  .End ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Change base file name for the other stats
StringCopy  ( basefilename,     path,   "\\SynthAnalysis.ArgMax" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

for ( int criti = mincriterrordatai, criti0 = 0, crit0invi = numcriterrordatai - 1; criti <= maxcriterrordatai; criti++, criti0++, crit0invi-- ) {

    StringCopy      ( expfile3.Filename, basefilename );
    StringAppend    ( expfile3.Filename, ".Crit ", IntegerToString ( buff, criti0 + 1, 2 ) );
    StringAppend    ( expfile3.Filename, ".", critnames[ criti ] );
    AddExtension    ( expfile3.Filename, FILEEXT_FREQ );

    expfile3.Write  ( anmax[ criti ] );
    expfile3.End    ();
    } // for criti


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

StringCopy      ( expfile2MC.Filename, basefilename );
StringAppend    ( expfile2MC.Filename, ".MeanNoise.MapCorr" );
AddExtension    ( expfile2MC.Filename, FILEEXT_FREQ );

StringCopy      ( expfile2MN.Filename, basefilename );
StringAppend    ( expfile2MN.Filename, ".MeanCorr.MapNoise" );
AddExtension    ( expfile2MN.Filename, FILEEXT_FREQ );

StringCopy      ( expfile2CN.Filename, basefilename );
StringAppend    ( expfile2CN.Filename, ".MeanMap.CorrNoise" );
AddExtension    ( expfile2CN.Filename, FILEEXT_FREQ );

StringCopy      ( expfile1M.Filename, basefilename );
StringAppend    ( expfile1M.Filename, ".MeanCorrNoise.Map" );
AddExtension    ( expfile1M.Filename, FILEEXT_EEGSEF );

StringCopy      ( expfile1C.Filename, basefilename );
StringAppend    ( expfile1C.Filename, ".MeanMapNoise.Corr" );
AddExtension    ( expfile1C.Filename, FILEEXT_EEGSEF );

StringCopy      ( expfile1N.Filename, basefilename );
StringAppend    ( expfile1N.Filename, ".MeanMapCorr.Noise" );
AddExtension    ( expfile1N.Filename, FILEEXT_EEGSEF );

StringCopy      ( expfile0.Filename, basefilename );
StringAppend    ( expfile0.Filename, ".MeanMapCorrNoise" );
AddExtension    ( expfile0.Filename, FILEEXT_EEGSEF );


for ( int criti = mincriterrordatai, criti0 = 0, crit0invi = numcriterrordatai - 1; criti <= maxcriterrordatai; criti++, criti0++, crit0invi-- ) {


    for ( int mapi  = 0; mapi  < nummap;  mapi ++ )
    for ( int corri  = 0, corrinvi  = numcorr  - 1 - corri;  corri  < numcorr;  corri++,  corrinvi--  )
                                        // invert correlation: lowest correlation as lower tracks
        expfile2MC.Write ( statsMaxMC[ ( criti * numcorr  + corri  ) * nummap  + mapi  ].Mean (), mapi, corrinvi, crit0invi );


    for ( int mapi   = 0; mapi   < nummap;   mapi  ++ )
    for ( int noisei = 0, noiseinvi = numnoise - 1 - noisei; noisei < numnoise; noisei++, noiseinvi-- )
                                        // invert twice the noise: lowest noise value on top == maximum noise on top
        expfile2MN.Write ( statsMaxMN[ ( criti * numnoise + noisei ) * nummap  + mapi  ].Mean (), mapi, noisei,   crit0invi );


    for ( int corri  = 0, corrinvi  = numcorr  - 1 - corri;  corri  < numcorr;  corri++,  corrinvi--  )
    for ( int noisei = 0, noiseinvi = numnoise - 1 - noisei; noisei < numnoise; noisei++, noiseinvi-- )
                                        // invert twice the noise: lowest noise value on top == maximum noise on top
        expfile2CN.Write ( statsMaxCN[ ( criti * numnoise + noisei ) * numcorr + corri ].Mean (), corri, noisei,  crit0invi );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    for ( int mapi   = 0; mapi   < nummap;   mapi  ++ )
        expfile1M.Write ( statsMaxM[   criti * nummap   + mapi                       ].Mean (), mapi,      criti0 );

                                                      
    for ( int corri  = 0;  corri  < numcorr;  corri++ )
        expfile1C.Write ( statsMaxC[   criti * numcorr  + corri                      ].Mean (), corri,     criti0 );

                                        // invert noise, highest values on the left
    for ( int noisei = 0, noiseinvi = numnoise - 1 - noisei; noisei < numnoise; noisei++, noiseinvi-- )
        expfile1N.Write ( statsMaxN[   criti * numnoise + noisei                     ].Mean (), noiseinvi, criti0 );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    expfile0.Write ( statsMaxAll[   criti                                            ].Mean (), 0,         criti0 );

    } // for criti


expfile2MC.End ();
expfile2MN.End ();
expfile2CN.End ();
expfile1M .End ();
expfile1C .End ();
expfile1N .End ();
expfile0  .End ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

delete[]    anmean;
delete[]    anmax;

WindowMaximize ( CartoolMainWindow );

CartoolApplication->SetMainTitle ( "Data Analysis", path, Gauge );

Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
