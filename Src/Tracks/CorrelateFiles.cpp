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

#include    "CorrelateFiles.h"

#include    "Files.TGoF.h"
#include    "Files.ReadFromHeader.h"

#include    "TFreqDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void    CorrelateFiles (    TGoF&           filenames1,     TGoF&           filenames2, 
                            CorrelateType   correlate, 
                            bool            ignorepolarity,
                            bool            spatialfilter1, bool            spatialfilter2,     char*           xyzfile,
                            int             cliptf,
                            int             numrand,
                            const char*     corrtitle,      TSuperGauge*    gauge 
                         )
{
if ( filenames1.IsEmpty () || filenames2.IsEmpty () )
    return;

if ( correlate == CorrelateTypeNone )
    return;

if ( ( spatialfilter1 || spatialfilter2 ) && StringIsEmpty ( xyzfile ) )
    spatialfilter1  = spatialfilter2    = false;

Maxed ( cliptf, 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we don't really need to rescan the whole groups, we could instead just read 1 file to get the dimensions & types...
TracksCompatibleClass   tc1;
FreqsCompatibleClass    fc1;
TracksGroupClass        tg1;


filenames1.AllTracksAreCompatible ( tc1 );

filenames1.AnyTracksGroup ( tg1 );

if ( tg1.allfreq )
    filenames1.AllFreqsAreCompatible ( fc1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // store some of the extracted variables
bool                alleeg1         = tg1.alleeg;
bool                allfreq1        = tg1.allfreq;
bool                allris1         = tg1.allris;
bool                allrisv1        = tg1.allrisv;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TracksCompatibleClass   tc2;
FreqsCompatibleClass    fc2;
TracksGroupClass        tg2;


filenames2.AllTracksAreCompatible ( tc2 );

filenames2.AnyTracksGroup ( tg2 );

if ( tg2.allfreq )
    filenames2.AllFreqsAreCompatible ( fc2 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // store some of the extracted variables
bool                alleeg2         = tg2.alleeg;
bool                allfreq2        = tg2.allfreq;
bool                allris2         = tg2.allris;
bool                allrisv2        = tg2.allrisv;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // !vectorial case not tested!
AtomType           	datatype1       = allrisv1 ? AtomTypeVector : AtomTypeScalar;
AtomType           	datatype2       = allrisv2 ? AtomTypeVector : AtomTypeScalar;
                                                                                   
                                        // only for eeg & not Positive, not Vectorial
PolarityType        polarity        = ( alleeg1 || alleeg2 ) 
                                    && correlate == CorrelateTypeSpatialCorrelation
                                    && ! ( IsAbsolute ( datatype1 ) || IsAbsolute ( datatype2 ) )
                                    && ignorepolarity ? PolarityEvaluate 
                                                      : PolarityDirect;
                                        // Reference:
                                        // - Spatial Correlation     : regular way - Note that the ref could be updated for each file...
                                        // - Time-course Correlation : always average reference IN TIME, once the data have been read and transposed
                                        // - Phase-Intensity coupling: always average reference IN TIME, once the data have been read and transposed
ReferenceType       processingref   = correlate == CorrelateTypeSpatialCorrelation  ? SetProcessingRef ( allris1 && allris2 ? ProcessingReferenceESI : ProcessingReferenceEEG ) 
                                    : correlate == CorrelateTypeTimeCorrelation     ? ReferenceAverage
                                    : correlate == CorrelateTypePhaseIntCoupling    ? ReferenceAverage
                                                                                    : ReferenceAverage;

ReferenceType       processingref1;
ReferenceType       processingref2;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Initialize data spreading
int                 dim1goes1,  dim2goes1,  dim3goes1;
int                 dim1goes2,  dim2goes2,  dim3goes2;
int                 dim1gets,   dim2gets,   dim3gets;

                                        // Targetting these "virtual" axis:
                                        // Data[   el ][ time ][ freq ]
                                        //       dim1    dim2    dim3
if      ( correlate == CorrelateTypeSpatialCorrelation ) {

    dim1goes1       = ReadGoMapsToDimension;
    dim2goes1       = ReadGoMapsToNumMaps;
    dim3goes1       = ReadGoMapsIgnore;
                                        // equivalent for both files
    dim1goes2       = dim1goes1;
    dim2goes2       = dim2goes1;
    dim3goes2       = dim3goes1;
                                        // writing by reverting the read order
    dim1gets        = dim1goes1;
    dim2gets        = dim2goes1;
    dim3gets        = dim3goes1;
    }
else if ( correlate == CorrelateTypeTimeCorrelation ) {

    dim1goes1       = ReadGoMapsToNumMaps;
    dim2goes1       = ReadGoMapsToDimension;
    dim3goes1       = ReadGoMapsIgnore; // will be replaced with actual frequency index
                                        // equivalent for both files
    dim1goes2       = dim1goes1;
    dim2goes2       = dim2goes1;
    dim3goes2       = dim3goes1;
                                        // writing by reverting the read order
    dim1gets        = dim1goes1;
    dim2gets        = dim2goes1;
    dim3gets        = dim3goes1;
    }
else if ( correlate == CorrelateTypePhaseIntCoupling ) {
                                        // !in 2D we expect El==Freqs TF==Time
    dim1goes1       = allfreq1 ? ReadGoMapsIgnore       : ReadGoMapsToNumMaps;
    dim2goes1       = allfreq1 ? ReadGoMapsToDimension  : ReadGoMapsToDimension;
    dim3goes1       = allfreq1 ? ReadGoMapsToNumMaps    : ReadGoMapsIgnore;
                                        // equivalent for both files
    dim1goes2       = dim1goes1;
    dim2goes2       = dim2goes1;
    dim3goes2       = dim3goes1;
                                        // 
    dim1gets        = dim1goes1; // ReadGoMapsIgnore;
    dim2gets        = dim2goes1; // ReadGoMapsToDimension;
    dim3gets        = dim3goes1; // ReadGoMapsToNumMaps;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TStrings            tracksnamesi;
TStrings            tracksnamesj;
TStrings            mapsnames;
TStrings            freqsnamesi;
char                buff     [ 256 ];


if      ( correlate == CorrelateTypeSpatialCorrelation ) {

                                        // create names, as the samples (time frames) don't have any - it still gives a hint of spatial correlation
    mapsnames.Set ( tc2.NumTF, 64 );

    for ( int i = 0; i < tc2.NumTF; i++ )
        StringCopy ( mapsnames[ i ], "Map", IntegerToString ( buff, i + 1 /*, NumIntegerDigits ( tc2.numtf )*/ ) );

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do some checking + retrieve some info
TMaps               mapsi;
TMaps               mapsj;
TMaps               mapsr;
TMaps               mapsrp;

TFileName           pathi    ;
TFileName           filenamei;
TFileName           filenamej;
TFileName           filenamer;
char                corrinfix[  32 ];

int                 numfreq1;
//int                 numfreq2;

bool                doingrand       = numrand > 0 && correlate == CorrelateTypePhaseIntCoupling;


                                        // repeat the whole thing twice, one with and one without randomization
                                        // saving the correlation on the first pass, the probabilities on the second one
for ( int somerand = 0; somerand < ( doingrand ? 2 : 1 ); somerand++ )

for ( int i = 0; i < (int) filenames1; i++ ) {

    StringCopy      ( pathi,     filenames1[ i ] );
    RemoveFilename  ( pathi );
    StringCopy      ( filenamei, filenames1[ i ] );
    GetFilename     ( filenamei );
//    StringShrink    ( filenamei, filenamei, 32 );


    if ( allfreq1 ) ReadFromHeader ( filenames1[ i ], ReadNumFrequencies, &numfreq1 );
    else            numfreq1        = 1;    // to run once the freq loop


    for ( int j = 0; j < (int) filenames2; j++ ) {

        StringCopy      ( filenamej, filenames2[ j ] );
        GetFilename     ( filenamej );
//        StringShrink    ( filenamej, filenamej, 32 );


//      if ( allfreq2 ) ReadFromHeader ( filenames2[ j ], ReadNumFrequencies, &numfreq2 );
//      else            numfreq2        = 1;    // to run once the freq loop


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Here we have to consider each type of processing with its own loop(s)
        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if      ( correlate == CorrelateTypeSpatialCorrelation ) {
                                        // do this only once, it remains constant for all mapsj
            if ( j == 0 ) {

                if ( gauge ) {
                    gauge->Next ( gaugecorrfile1 );
                    CartoolObjects.CartoolApplication->SetMainTitle ( corrtitle, filenames1[ i ], *gauge );
                    }

                                        // Read & preprocess data
                mapsi.ReadFile ( filenames1[ i ], datatype1, ReferenceNone, 
                                 0, 0,
                                 dim1goes1, dim2goes1, dim3goes1 );


                processingref1  = SetProcessingRef ( allris1 ? ProcessingReferenceESI : ProcessingReferenceEEG );

                                        // !works only on non-transposed data - we can add a parameter if needed!
                if ( spatialfilter1 )   mapsi.FilterSpatial ( SpatialFilterDefault, xyzfile );


                mapsi.SetReference ( processingref1, AtomTypeScalar );
                }



            if ( gauge ) {
                gauge->Next ( gaugecorrfile2 );
                CartoolObjects.CartoolApplication->SetMainTitle ( corrtitle, filenames2[ j ], *gauge );
                }

                                        // Read & preprocess data
            mapsj.ReadFile ( filenames2[ j ], datatype2, ReferenceNone,
                             0, 0,
                             dim1goes2, dim2goes2, dim3goes2 );


            processingref2  = SetProcessingRef ( allris2 ? ProcessingReferenceESI : ProcessingReferenceEEG );

                                        // !works only on non-transposed data - we can add a parameter if needed!
            if ( spatialfilter2 )   mapsj.FilterSpatial ( SpatialFilterDefault, xyzfile );


            mapsj.SetReference ( processingref2, AtomTypeScalar );



            mapsr.Correlate (   mapsi, mapsj, /*CorrelateTypeAuto*/ CorrelateTypeLinearLinear, 
                                polarity, ReferenceNone /*processingref*/, 
                                0, 0, 
                                corrinfix 
                            );


                                        // Use original file name, appended with some info
            StringCopy          ( filenamer, pathi );
            StringAppend        ( filenamer, "\\", filenamei );
            StringAppend        ( filenamer, ".CorrSpatial" );
            StringAppend        ( filenamer, ".", filenamej );
            AddExtension        ( filenamer, FILEEXT_EEGSEF );


            mapsr.WriteFileReordered    (   filenamer, 
                                            allrisv1 && allrisv2, 
                                            0,
                                            &mapsnames, 0,
                                            dim1gets, dim2gets, dim3gets 
                                        );

            } // CorrelateTypeSpatialCorrelation


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        else if ( correlate == CorrelateTypeTimeCorrelation ) {
                                        // frequency loop, synchronous on both files
            for ( int freqi = 0; freqi < numfreq1; freqi++ ) {
                                        // we can either be in EEG or in FREQ data
                dim3goes1   = allfreq1 ? freqi : ReadGoMapsIgnore;
                dim3goes2   = dim3goes1;
                dim3gets    = dim3goes1;

                                        // do this only once, if it remains constant for all mapsj
                if ( dim3goes1 >= 0 
                  || dim3goes1 == ReadGoMapsIgnore && j == 0 ) {

                    if ( gauge ) {
                        gauge->Next ( gaugecorrfile1 );
                        CartoolObjects.CartoolApplication->SetMainTitle ( corrtitle, filenames1[ i ], *gauge );
                        }

                                        // Read & preprocess data
                    mapsi.ReadFile  (   filenames1[ i ], datatype1, ReferenceNone, 
                                        &tracksnamesi, &freqsnamesi,
                                        dim1goes1, dim2goes1, dim3goes1,
                                        cliptf 
                                    );
                    }


                if ( gauge ) {
                    gauge->Next ( gaugecorrfile2 );
                    CartoolObjects.CartoolApplication->SetMainTitle ( corrtitle, filenames2[ j ], *gauge );
                    }

                                        // Read & preprocess data
                mapsj.ReadFile ( filenames2[ j ], datatype2, ReferenceNone,
                                 0, 0,
                                 dim1goes2, dim2goes2, dim3goes2,
                                 cliptf );


                mapsr.Correlate (   mapsi, mapsj, CorrelateTypeAuto, 
                                    polarity, processingref, 
                                    0, 0, 
                                    corrinfix 
                                );

                                        // Use original file name, appended with some info
                StringCopy          ( filenamer, pathi );
                StringAppend        ( filenamer, "\\", filenamei );
                StringAppend        ( filenamer, ".CorrTime", corrinfix );
                StringAppend        ( filenamer, ".", filenamej );
                AddExtension        ( filenamer, allfreq1 ? FILEEXT_FREQ : FILEEXT_EEGSEF );


                mapsr.WriteFileReordered    (   filenamer, 
                                                allrisv1 && allrisv2, 
                                                0, 
                                                &tracksnamesi, &freqsnamesi,
                                                dim1gets, dim2gets, dim3gets, numfreq1, (bool) freqi
                                            );

                } // for freqi

            } // CorrelateTypeTimeCorrelation


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        else if ( correlate == CorrelateTypePhaseIntCoupling ) {
                                        // first electrode loop
            for ( int eli1 = 0; eli1 < tc1.NumTracks; eli1++ ) {
                                        // we can either be in EEG or in FREQ data
                if ( allfreq1 )
                    dim1goes1   = eli1;

                if ( gauge ) {
                    gauge->Next ( gaugecorrfile1 );
                    CartoolObjects.CartoolApplication->SetMainTitle ( corrtitle, filenames1[ i ], *gauge );
                    }

                                        // Read & preprocess data
                mapsi.ReadFile  (   filenames1[ i ], datatype1, ReferenceNone, 
                                    &tracksnamesi, &freqsnamesi,
                                    dim1goes1, dim2goes1, dim3goes1,
                                    cliptf 
                                );


                for ( int eli2 = 0; eli2 < tc2.NumTracks; eli2++ ) {
                                        // we can either be in EEG or in FREQ data
                    if ( allfreq2 ) {
                        dim1goes2   = eli2;
                        dim1gets    = eli2;
                        }


                    if ( gauge ) {
                        gauge->Next ( gaugecorrfile2 );
                        CartoolObjects.CartoolApplication->SetMainTitle ( corrtitle, filenames2[ j ], *gauge );
                        }

                                        // Read & preprocess data
                    mapsj.ReadFile  (   filenames2[ j ], datatype2, ReferenceNone,
                                        &tracksnamesj, 0,
                                        dim1goes2, dim2goes2, dim3goes2,
                                        cliptf 
                                    );

                    mapsr.Correlate (   mapsi, mapsj, CorrelateTypeAuto, 
                                        polarity, processingref, 
                                        somerand ? numrand : 0, somerand ? &mapsrp : 0, 
                                        corrinfix 
                                    );

                                        // Use original file name, appended with some info
                    StringCopy          ( filenamer, pathi );
                    StringAppend        ( filenamer, "\\", filenamei );
                    if ( allfreq1 ) StringAppend    ( filenamer, ".", tracksnamesi[ eli1 ] );
                    StringAppend        ( filenamer, "." "Coupling", corrinfix );
                    StringAppend        ( filenamer, ".", filenamej );
                    AddExtension        ( filenamer, allfreq1 ? FILEEXT_FREQ : FILEEXT_EEGSEF );


                    if ( ! somerand )
                                        // save only once in a file, on the first run
                        mapsr.WriteFileReordered    (   filenamer, 
                                                        allrisv1 && allrisv2, 
                                                        0, 
                                                        allfreq1 ? &tracksnamesj : &tracksnamesi,     // frequencies: sorted by tracks names of file #2 / split per tracks: frequencies == tracks
                                                        allfreq1 ? &freqsnamesi  : 0,                 // frequencies: frequencies taken from file #1
                                                        dim1gets, dim2gets, dim3gets, tc2.NumTracks, (bool) eli2 
                                                    );


                    if ( doingrand && somerand && mapsrp.IsAllocated () ) {
                                        // save only on the optional second run
                        PostfixFilename ( filenamer, "." Infix1MinusP );

                        mapsrp.WriteFileReordered   (   filenamer, 
                                                        allrisv1 && allrisv2, 
                                                        0, 
                                                        allfreq1 ? &tracksnamesj : &tracksnamesi,     // frequencies: sorted by tracks names of file #2 / split per tracks: frequencies == tracks
                                                        allfreq1 ? &freqsnamesi  : 0,                 // frequencies: frequencies taken from file #1
                                                        dim1gets, dim2gets, dim3gets, tc2.NumTracks, (bool) eli2 
                                                    );

                                        // want to see the correlation x probability of not bullshit?
                        mapsrp     *= mapsr;

                        PostfixFilename ( filenamer, " x Corr" );

                        mapsrp.WriteFileReordered   (   filenamer, 
                                                        allrisv1 && allrisv2, 
                                                        0, 
                                                        allfreq1 ? &tracksnamesj : &tracksnamesi,     // frequencies: sorted by tracks names of file #2 / split per tracks: frequencies == tracks
                                                        allfreq1 ? &freqsnamesi  : 0,                 // frequencies: frequencies taken from file #1
                                                        dim1gets, dim2gets, dim3gets, tc2.NumTracks, (bool) eli2 
                                                    );

                        }


                    if ( ! allfreq2 )   // there is no looping in 2D file
                        break;
                    } // for eil2


                if ( ! allfreq1 )       // there is no looping in 2D file
                    break;
                } // for eli1

            } // CorrelateTypePhaseIntCoupling


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        } // for j

    } // for i

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
