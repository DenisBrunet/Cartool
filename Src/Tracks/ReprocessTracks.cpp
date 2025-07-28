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

#include    "ReprocessTracks.h"
#include    "TExportTracksDialog.h"

#include    "Strings.Utils.h"
#include    "Files.TVerboseFile.h"
#include    "Dialogs.Input.h"
#include    "Dialogs.TSuperGauge.h"
#include    "TExportTracks.h"

#include    "TTracksFilters.h"

#include    "TRoisDoc.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Tracks document EEGDoc should be already open
                                        // tracks to export could be empty, meaning all tracks
                                        // ExportTimeInterval: timemin and timemax must be totally resolved at that point, and consistent
                                        // ExportTimeTriggers: list will be reordered
                                        // Single / Multiple tracks reference will skip unknown tracks names. If none are to be found, it will switch to ReferenceAsInFile
                                        // Downsampling will occur only if > 1, otherwise it will be ignored
                                        // Concatenation: concatinputtime, concatoutputtime, batchfilenames and batchfileindex must be provided
bool    ReprocessTracks (
                        TTracksDoc*         EEGDoc,
                        TracksOptions       tracksoptions,          const char*         tracks,
                        const TElectrodesDoc*   xyzdoc,             // either 0 if not used, or already opened - function does not handle the opening or closing
                        const TRoisDoc*         roisdoc,
                        TimeOptions         timeoptions,
                        long                timemin,                long                timemax,            // could be set to max for EOF
                        const char*         triggerstokeeplist,     const char*         triggerstoexcludelist,
                        const char*         addingnullchannels,
                        FiltersOptions      filtersoptions,         const TTracksFilters<float>*    altfilters,     double&     defaultsamplingfrequency,
                        ReferenceType       ref,                    const char*         reflist,
                        bool                baselinecorr,
                        long                baselinecorrpre,        long                baselinecorrpost,   // could be set to max for EOF
                        RescalingOptions    rescalingoptions,       double              rescalingfactor,
                        SequenceOptions     sequenceoptions,
                        int                 downsampleratio,        // <= 1 if no downsampling, > 1 if downsampling
                        const char*         outputdir,
                        const char*         infix,
                        const char*         freqinfix,              // for frequency processing purpose
                        SavingEegFileTypes  filetype,
                        bool                outputmarkers,
                        ConcatenateOptions  concatenateoptions,     long*               concatinputtime,        long*               concatoutputtime,
                        TExportTracks&      expfile,                // Needed for concatenating across multiple calls/files
                        const TGoF*         batchfilenames,         // For the moment, transmit this group of files
                        int                 batchfileindex,         // index of current file in batch processing
                        bool                silent
                        )
{
if (   EEGDoc           == 0 
    || tracksoptions    == ProcessUnknown
    || timeoptions      == UnknownTimeProcessing 
    || sequenceoptions  == UnknownSequenceProcessing 
    )

    return  false;

                                        // Fail if ROIs are not correctly set - fallback is not easy to make
if ( tracksoptions == ProcessRois && ( roisdoc == 0 || roisdoc->ROIs == 0 ) )
    return  false;

                                        // Off, as this will be checked later on
//if ( timeoptions == ExportTimeInterval && ( timemin < 0 || timemax < 0 || timemin > timemax ) )
//    return  false;

                                        // or do we allow to export empty files?
if ( timeoptions == ExportTimeTriggers && StringIsEmpty ( triggerstokeeplist ) )
    return  false;

                                        // Off, as this will be checked later on
//if ( filtersoptions == UsingOtherFilters && altfilters == 0 )
//    return  false;
// 
//if ( filtersoptions == UsingOtherFilters && altfilters->HasNoFilters () )
//    filtersoptions = NotUsingFilters;
// 
//if ( filtersoptions == UsingCurrentFilters && ! EEGDoc->AreFiltersActivated () )
//    filtersoptions = NotUsingFilters;

                                        // Off, as this will be checked later on
//if ( ref == ReferenceArbitraryTracks && StringIsEmpty ( reflist ) )
//    return  false;

                                        // Off, as this will be checked later on
//if ( baselinecorr && baselinecorrpre > baselinecorrpost )
//    return  false;


if ( rescalingoptions == GfpRescaling && ! EEGDoc->HasPseudoElectrodes () )
    rescalingoptions    = NotRescaled;

//if ( rescalingoptions == ConstantRescaling && rescalingfactor == 0 )
//    return  false;


if ( concatenateoptions == ConcatenateTime && ( concatinputtime == 0 || concatoutputtime == 0 ) )
    return  false;

                                        // force silent if not in interactive mode
if ( ! silent && CartoolObjects.CartoolApplication->IsNotInteractive () )
    silent  = true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // What is the current batching status?
bool                HasBatchFiles       = batchfilenames && batchfilenames->IsNotEmpty ();
int                 NumBatchFiles       = HasBatchFiles ? batchfilenames->NumFiles () : 0;
bool                IsBatchFirstCall    = HasBatchFiles && batchfileindex == 0;
bool                IsBatchLastCall     = HasBatchFiles && batchfileindex == NumBatchFiles - 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                isfrequency         = IsExtensionAmong ( EEGDoc->GetDocPath (), AllFreqFilesExt )
                                       || StringIsNotEmpty ( freqinfix );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We already asserted that ROIs will be != 0 for ProcessRois option...
const TRois*        ROIs                = tracksoptions == ProcessRois && roisdoc ? roisdoc->ROIs : 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Electrodes / Data layout will be:
                                        //      Original Regular Electrodes
                                        //   -> Optional Null Tracks <-
                                        //      Pseudo Tracks

                                        // number of electrodes in file
int                 numorigels          = EEGDoc->GetNumElectrodes   ();
int                 firstorig           = 0;
int                 lastorig            = numorigels - 1;

                                        // number of added null channels - mechanism could be upgraded to adding other types of channels, though
                                        // we currently disallow adding channels AND ROIs, because consistency starts to be difficult to assert
TSplitStrings       addedchannels ( addingnullchannels, NonUniqueStrings );
int                 numaddedels         = tracksoptions == ProcessRois ? 0 : addedchannels.GetNumTokens ();
int                 firstadded          = numaddedels > 0 ? numorigels                   : -1;
int                 lastadded           = numaddedels > 0 ? numorigels + numaddedels - 1 : -1;

                                        // our new number of electrodes for computation and saving, but not reading
int                 numels              = numorigels + numaddedels;

                                        // pseudo-tracks indexes
int                 gfpoff              = numels + 0;
int                 disoff              = numels + 1;
int                 avgoff              = numels + 2;
int                 firstpseudo         = numels;
int                 lastpseudo          = numels + NumPseudoTracks - 1;

                                        // grand total of tracks, for buffers allocation
int                 numtotalels         = numels + NumPseudoTracks;


long                numtimeframes       = EEGDoc->GetNumTimeFrames   ();
long                lasttimeframe       = numtimeframes - 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Clone electrodes names, but keep the pseudo from the EEG (case of RMS instead of GFP)
TStrings            ElectrodesNames;

                                        // copy the regular electrodes names, preferably from the XYZ
for ( int ei = 0; ei < numorigels; ei++ )

    ElectrodesNames.Add ( xyzdoc ? xyzdoc->GetElectrodeName ( ei ) : EEGDoc->GetElectrodeName ( ei ) );

                                        // add the optional null tracks
for ( int ni = 0; ni < numaddedels; ni++ )

    ElectrodesNames.Add ( addedchannels[ ni ] );

                                        // add pseudo names, taking extra-care for positive data
ElectrodesNames.Add ( EEGDoc->IsAbsolute ( AtomTypeUseOriginal ) ? TrackNameRMS     : TrackNameGFP );
ElectrodesNames.Add ( EEGDoc->IsAbsolute ( AtomTypeUseOriginal ) ? TrackNameDISPlus : TrackNameDIS );
ElectrodesNames.Add ( TrackNameAVG );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Set the selected channels
TSelection          elsel  ( numtotalels, OrderArbitrary );     // electrodes selection includes ALL tracks
char                buff[ EditSizeTextLong ];

ClearString ( buff );
StringCopy  ( buff, tracks );

elsel.Reset ();


if      ( tracksoptions == ProcessRois ) {
                                        // only the regular electrodes for ROIs
    elsel.Set ( firstorig, lastorig );
    }
                                        // ProcessTracks cases:
else if ( StringIsSpace ( buff ) ) {
                                        // the regular electrodes
    elsel.Set ( firstorig, lastorig );
                                        // ..plus the nulls
    if ( numaddedels > 0 )
        elsel.Set ( firstadded, lastadded );
    }

else if ( StringContains ( (const char*) buff, "*" ) ) {
                                        // caller requested all electrodes, but it may also be mixed with others: pseudos and/or nulls, and at any position

    TSplitStrings       tokens ( buff, UniqueStrings );

    bool        nonulls     = numaddedels > 0 && ! tokens.Intersect ( addedchannels );

    for ( int toki = 0; toki < (int) tokens; toki++ ) {
                                        // insertion will follow the input sequence
        if      ( StringIs ( tokens[ toki ], "*" ) ) {
                                        // bypass the '*' of TSelection with only our regular electrodes
            elsel.Set ( firstorig, lastorig );
                                        // if no nulls specified, inserted them now
            if ( nonulls )
                elsel.Set ( firstadded, lastadded );
            }
        else // just insert whatever comes next

            elsel.Set ( tokens[ toki ], &ElectrodesNames, ! silent );
        }
    } // "*"

else if ( numaddedels > 0 ) {
                                        // inserting the nulls at the right place - not that straightforward as you can see...

    TSelection      usersel ( numtotalels, OrderArbitrary );

    usersel.Set ( buff, &ElectrodesNames, ! silent );

    bool            hasregular  = usersel.NumSet ( firstorig,   lastorig   );
    bool            haspseudos  = usersel.NumSet ( firstpseudo, lastpseudo );
    bool            hasnulls    = usersel.NumSet ( firstadded,  lastadded  );
                                            

    if      (    hasnulls &&   haspseudos       // user explicitly specified nulls AND pseudos
              || hasnulls && ! haspseudos   ) { // user specified (some) nulls, fair enough
            
        elsel   = usersel;
        }
    else if ( hasregular && ! hasnulls && haspseudos ){
                                                // user asked for some regular tracks and some pseudos, but forgot about the nulls
                                                // so insert the null tracks before the pseudos - order might change in the process...

                                    // split selection in 2: regular and pseudos
        TSelection      userselreg    ( usersel );  userselreg   .Reset ( lastorig + 1, numtotalels - 1 );
        TSelection      userselpseudo ( usersel );  userselpseudo.Reset ( firstorig,    firstpseudo - 1 );

        elsel  += userselreg;                   // user regulars
        elsel.Set ( firstadded, lastadded );    // introduce the added tracks here
        elsel  += userselpseudo;                // user pseudos
        }

    else if ( hasregular && ! hasnulls && ! haspseudos ) {
                                                // user asked for some regular tracks and no pseudos, but forgot about the nulls
        elsel   = usersel; 
        elsel.Set ( firstadded, lastadded ); 
        }

    else {                                      // only some pseudos, no regular and no nulls - don't add the nulls!
        elsel   = usersel; 
        }

    } // numaddedels > 0

else { // numaddedels == 0
                                        // no added tracks, just set from user
    elsel.Set ( buff, &ElectrodesNames, ! silent );
    }

                                        // aborting now?
if ( tracksoptions == ProcessTracks && elsel.NumSet () == 0 )

    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Just for verbose output
TSelection          badsel ( numels,     OrderSorted    );
badsel     += EEGDoc->GetBadTracks ();

                                        // Auxiliaries are limited to the original input for the moment, adding aux is not allowed
TSelection          auxsel ( numels,     OrderSorted    );
auxsel     += EEGDoc->GetAuxTracks ();

                                        // We need to upgrade the valid tracks to the new null tracks
TSelection          validsel ( numels,     OrderSorted    );
validsel   += EEGDoc->GetValidTracks ();

if ( numaddedels > 0 )

    validsel.Set ( firstadded, lastadded );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

long                intimemin           = 0;    // requested from the input parameters
long                intimemax           = 0;
long                intimenum           = 0;
long                outtimemin;                 // actually used for the output
long                outtimemax;
long                outtimenum;

int                 downsampleshiftorg  = 0; // ( downsampleratio + 1 ) / 2 - 1;    // ?do we shift the downsampled bucket to the center of the old group of TF? currently not, using truncation
bool                downsample          = downsampleratio > 1 
                                       && timeoptions     == ExportTimeInterval 
                                       && sequenceoptions == SequenceProcessing;

long                downtimemin;
long                downtimemax;
long                downtimenum;
double              downsamplfreq;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Some more consistency checks
long                baselinecorrnum     = 0;

                                        // check over current limits
if ( baselinecorr ) {
                                        // this is not beyond limit, this is totally wrong parameters
    if ( baselinecorrpre < 0             && baselinecorrpost < 0
      || baselinecorrpre > lasttimeframe && baselinecorrpost > lasttimeframe )

        baselinecorr    = false;

    else {
        Clipped ( baselinecorrpre,  baselinecorrpost,   (long) 0, lasttimeframe );

        baselinecorrnum     = baselinecorrpost - baselinecorrpre + 1;
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                haspseudos          = EEGDoc->HasPseudoElectrodes ();

bool                dopseudos           = haspseudos                                    // we could totally bypass this, as we are computing the pseudo-tracks locally
                                       && tracksoptions == ProcessTracks 
                                       && elsel.NumSet ( firstpseudo, lastpseudo );     // can we / do we need to handle the pseudos?

                                        // triggers only allowed to mimick the original file, i.e. time intervals or excluded markers
bool                outputtriggers      = outputmarkers
                                       && timeoptions != ExportTimeTriggers 
                                       && sequenceoptions == SequenceProcessing;

bool                createfile          = concatenateoptions == NoConcatenateTime                       // single file
                                       || concatenateoptions == ConcatenateTime && IsBatchFirstCall;    // concatenation and first file

bool                closefile           = concatenateoptions == NoConcatenateTime                       // single file
                                       || concatenateoptions == ConcatenateTime && IsBatchLastCall;     // concatenation and last file


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TArray1<int>        reassignedtf;
TSplitStrings       triggerlist;

TMarker             marker;

TMarkers            remaintags;
TMarkers            timechunks;         // this list will hold all valid time intervals to process, even for the sequential case
long                maxtimechunk    = 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//TExportTracks       expfile;

if ( createfile )

    expfile.Markers.ResetMarkers ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setting data type, reference, and reading parameters
AtomType         	datatype        = EEGDoc->GetAtomType ( AtomTypeUseOriginal );  // AtomTypeUseCurrent(?)

                                        // force reset reference? not for the moment
//if (    IsPositive ( datatype )
//     || IsAngular  ( datatype ) )   
//    ref     = ReferenceAsInFile;


TSelection          refsel ( numels,      OrderSorted    );     // reference selection does NOT include pseudo tracks

                                        // resolve this before we use it
if      ( ref == ReferenceUsingCurrent ) {

    ref     = EEGDoc->GetReferenceType   ();
    refsel  = EEGDoc->GetReferenceTracks ();    // TSelection size will be from the original EEG, but code is OK with that
    }

else if ( ref == ReferenceArbitraryTracks ) {

    refsel.Reset ();
    refsel.Set   ( reflist, &ElectrodesNames, ! silent );   // could make use of the null tracks
    }

                                        // safety check
if ( ref == ReferenceArbitraryTracks && refsel.NumSet () == 0 )

        ref     = ReferenceAsInFile;    // either setting to no reference, as is currently done in  TTracksDoc::SetReferenceType
//      return  false;                  // or exiting?

                                        // Allow any sort of re-referencing
//CheckReference ( ref, datatype );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Output file type
if ( isfrequency )
    filetype    = PresetFileTypeSef;    // these are temp files actually, force using binary files

                                        // output file can handle sampling frequency?
bool                samplingfreqout     = filetype != PresetFileTypeEp
                                       && filetype != PresetFileTypeTxt;

TFileName           filename;

                                        // setting output file name
if ( createfile ) {
                                        // output file name could have been set by caller
    if ( concatenateoptions == ConcatenateTime && expfile.Filename.IsNotEmpty () ) {

        StringCopy ( filename, expfile.Filename );
        }

    else {
                                        // all other cases: compose an output file name from the input one
        EEGDoc->GetBaseFileName ( filename );


        if ( StringIsNotEmpty ( outputdir ) )
            ReplaceDir   ( filename, outputdir );

        if ( StringIsNotEmpty ( infix ) )
            StringAppend ( filename, ".", infix );

        if ( StringIsNotEmpty ( freqinfix ) )
            StringAppend ( filename, StringIsEmpty ( infix ) ? "." : " ", freqinfix );
        }

                                        // append extension if not already there?
    if ( ! IsExtension ( filename, SavingEegFileExtPreset[ filetype ] ) )

        AddExtension ( filename, SavingEegFileExtPreset[ filetype ] );


    CreatePath  ( filename, true );

                                        // maybe not a good idea, if we expect an exact output file name(?)
//  CheckNoOverwrite ( filename );

                                        // test file creation - don't ask before overwriting
    if ( ! CanOpenFile ( filename, CanOpenFileWrite ) )
        return  false;
    }


if ( concatenateoptions == ConcatenateTime ) {
                                        // update caller with new file
    if ( IsBatchFirstCall )
                                        // file name is updated only once, at first call
        StringCopy ( expfile.Filename, filename );
    else                                // just retrieve file name on successive iterations
        StringCopy ( filename, expfile.Filename );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set the time limits
if      ( timeoptions == ExportTimeInterval ) {

    intimemin       = timemin;
    intimemax       = timemax;

    Clipped ( intimemin, intimemax, (long) 0, lasttimeframe );

    intimenum       = intimemax - intimemin + 1;


    if ( downsample ) {                 // force the time interval to be an exact multiple of the downsampling ratio

                                        // try first by adding more points
        intimenum   = RoundToAbove ( intimenum, downsampleratio );

        intimemax   = intimemin + intimenum - 1;

                                        // maybe we requested beyond file limit?
        if ( intimemax > lasttimeframe ) {
                                        // hu-hu, use less points then?
            intimenum   = AtLeast ( (long) 0, intimenum - downsampleratio );

            intimemax   = intimemin + intimenum - 1;

            Clipped ( intimemin, intimemax, (long) 0, lasttimeframe );
                                        // !here it is possible to end up with intimenum = 0, an error which will be dealt with just below!
            }
        }

                                        // easy case: we have a single time chunk
    maxtimechunk    = intimenum;

                                        // here, we have a single interval in our list
    timechunks.AppendMarker ( TMarker ( intimemin, intimemax, 1, "Block", MarkerTypeTemp ) );


    if ( downsample ) {
        downtimenum     = intimenum / downsampleratio;    // we have an exact multiple here
        downtimemin     = 0;
        downtimemax     = downtimenum - 1;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( sequenceoptions == SequenceProcessing && intimenum > 0 ) {
                                        // Copy ALL markers from EEGDoc
        remaintags  = *EEGDoc;

                                        // then manually clipping outside timechunks
        if ( intimemin > 0              )   remaintags.ClipMarkers ( 0,             intimemin - 1,     AllMarkerTypes );
        if ( intimemax < lasttimeframe  )   remaintags.ClipMarkers ( intimemax + 1, lasttimeframe,     AllMarkerTypes );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need this to remap the old TFs to the new output ones
        reassignedtf.Resize ( numtimeframes );
        reassignedtf   = -1;

        long                tf0             = 0;
                                        // here, single block
        for ( int ti = 0; ti < timechunks.GetNumMarkers (); ti++ )

            for ( long tfi = timechunks[ ti ]->From; tfi <= timechunks[ ti ]->To; tfi++, tf0++ )

                reassignedtf[ tfi ]    = downsample ? tf0 / downsampleratio : tf0;
        } // if SequenceProcessing

    } // if ExportTimeInterval

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( timeoptions == ExportTimeTriggers ) {
                                        // chunks of time that overlaps of all markers
    timechunks.MarkersToTimeChunks ( *EEGDoc, triggerstokeeplist, KeepingMarkers, 
                                     0, lasttimeframe );

    intimemin       = 0;
    intimenum       = timechunks.GetMarkersTotalLength ();
    intimemax       = intimemin + intimenum - 1;
    maxtimechunk    = timechunks.GetLongestMarker ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // split triggers list into atomic elements
    triggerlist.Set ( triggerstokeeplist, UniqueStrings );
                                        // retrieve marker names
    TStrings            markernames;
    EEGDoc->GetMarkerNames ( markernames, AllMarkerTypes );

    triggerlist.FilterWith ( markernames, silent );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( sequenceoptions == SequenceProcessing ) {
                                        // requested markers - overlapping markers OK
                                        // showing all markers that intersected the given list, clipped to the list of markers - that may not be what the user wants to see
//      remaintags  = *EEGDoc;
                                        // output will show only the required markers - more what the user wants
        remaintags.InsertMarkers  ( *EEGDoc, triggerstokeeplist );


        TMarkers            cliptags;
                                        // inverting timechunks
        cliptags.MarkersToTimeChunks   ( *EEGDoc, triggerstokeeplist, ExcludingMarkers, 
                                          0, lasttimeframe );
                                        // punching all time periods from excludetags - overlapping markers OK
        remaintags.ClipMarkers ( cliptags, AllMarkerTypes );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need this to remap the old TFs to the new output ones
        reassignedtf.Resize ( numtimeframes );
        reassignedtf   = -1;

        long                tf0             = 0;
                                        // this is identical to remaintags, except "flatten" so no overlapping markers anymore
        for ( int ti = 0; ti < timechunks.GetNumMarkers (); ti++ )

            for ( long tfi = timechunks[ ti ]->From; tfi <= timechunks[ ti ]->To; tfi++, tf0++ )

                reassignedtf[ tfi ]    = tf0;
        } // if SequenceProcessing


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // !case that does not appear yet, for the future!
    if ( downsample ) {
        downtimenum     = intimenum / downsampleratio;    // we have an exact multiple here
        downtimemin     = 0;
        downtimemax     = downtimenum - 1;
        }

    } // if ExportTimeTriggers

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

else if ( timeoptions == ExcludeTimeTriggers ) {
                                        // computing the time chunks outside of excluded markers - overlapping markers are OK
    timechunks.MarkersToTimeChunks ( *EEGDoc, triggerstoexcludelist, ExcludingMarkers, 
                                     0, lasttimeframe );

    intimemin       = 0;
    intimenum       = timechunks.GetMarkersTotalLength ();
    intimemax       = intimemin + intimenum - 1;
    maxtimechunk    = timechunks.GetLongestMarker ();


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // setting triggerlist - no need to filter out non-existing markers, though
    triggerlist.Set ( triggerstoexcludelist, UniqueStrings );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( sequenceoptions == SequenceProcessing ) {
                                        // excluded markers exclusively - overlapping markers OK - both triggers AND markers, but usually markers
        TMarkers            excludetags;

        excludetags.InsertMarkers  ( *EEGDoc, triggerstoexcludelist );

                                        // all the other markers - overlapping markers OK
        remaintags  = *EEGDoc;

        remaintags.RemoveMarkers   ( triggerstoexcludelist, AllMarkerTypes );
                                        // punching all time periods from excludetags - overlapping markers OK
        remaintags.ClipMarkers ( excludetags, AllMarkerTypes );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need this to remap the old TFs to the new output ones
        reassignedtf.Resize ( numtimeframes );
        reassignedtf   = -1;

        long                tf0             = 0;
                                        // this is identical to remaintags, except "flatten" so no overlapping markers anymore
        for ( int ti = 0; ti < timechunks.GetNumMarkers (); ti++ ) {
                                        // adding splice markers (only) in-between 2 deleted blocks
            if ( ti > 0 )
                remaintags.InsertMarker ( TMarker ( timechunks[ ti ]->From, "Splice", MarkerTypeMarker ) );

            for ( long tfi = timechunks[ ti ]->From; tfi <= timechunks[ ti ]->To; tfi++, tf0++ )

                reassignedtf[ tfi ]    = tf0;
            }
        } // if SequenceProcessing


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // !case that does not appear yet, for the future!
    if ( downsample ) {
        downtimenum     = intimenum / downsampleratio;    // we have an exact multiple here
        downtimemin     = 0;
        downtimemax     = downtimenum - 1;
        }

    } // if ExcludeTimeTriggers

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( intimenum == 0 ) {

    if ( ! silent ) {
        if      ( timeoptions == ExportTimeTriggers  )     ShowMessage ( "No triggers were found that match your input list,"  NewLine "skipping current file..",   EEGDoc->GetTitle (), ShowMessageWarning );
        else if ( timeoptions == ExcludeTimeTriggers )     ShowMessage ( "Your list of triggers excluded the whole file,"      NewLine "skipping current file..",   EEGDoc->GetTitle (), ShowMessageWarning );
        else if ( timeoptions == ExportTimeInterval  )     ShowMessage ( "The interval duration appears to be less than 1 TF," NewLine "skipping current file..",   EEGDoc->GetTitle (), ShowMessageWarning );
        }

    return  false;
    }

                                        // now we can set our current file output time
if ( sequenceoptions == AverageProcessing ) {
    outtimemin      = 0;                // single time frame
    outtimemax      = 0;
    outtimenum      = intimenum == 0 ? 0 : 1;   // !we can not output non-existing average!
    }
else {
    outtimemin      = downsample ? downtimemin : intimemin;
    outtimemax      = downsample ? downtimemax : intimemax;
    outtimenum      = downsample ? downtimenum : intimenum;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Transfer to Markers
if ( concatenateoptions == ConcatenateTime && intimenum != 0 ) {      
                                        // add markers at each file's beginning - currently 2 version, index and file name
                                        // time position is already correct for output
    expfile.Markers.AppendMarker ( TMarker (  *concatoutputtime, 
                                              StringCopy  ( buff, "Concat", IntegerToString ( batchfileindex + 1, /*3*/ NumIntegerDigits ( NumBatchFiles ) ) ), 
                                              MarkerTypeMarker ) );


    expfile.Markers.AppendMarker ( TMarker (  *concatoutputtime, 
                                              EEGDoc->GetTitle (), 
                                              MarkerTypeMarker ) );
    }

                                        // Averaging is not concerned with saving these markers
if ( sequenceoptions == SequenceProcessing ) {
                                        // Copy & reassign time positions to global Markers
    for ( int ti = 0; ti < remaintags.GetNumMarkers (); ti++ ) {
                                        // copy new marker
        marker          = *remaintags[ ti ];

        marker.From     = ( concatoutputtime ? *concatoutputtime : 0 ) + reassignedtf[ marker.From ];
        marker.To       = ( concatoutputtime ? *concatoutputtime : 0 ) + reassignedtf[ marker.To   ];

                                        // !we keep the original marker type!
                                        // !we assume remaintags to be sorted!
        expfile.Markers.AppendMarker ( marker );
        }

                                        // finito
    reassignedtf.DeallocateMemory ();
    } // if SequenceProcessing


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( concatenateoptions == ConcatenateTime ) {

    if ( IsBatchFirstCall )
        *concatinputtime    = *concatoutputtime     = 0;

                                        // now we can update for next file
    *concatinputtime       += intimenum;
    *concatoutputtime      += outtimenum;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Retrieving sampling frequency, with many fall-backs strategies

                                        // 1) using file Sampling Frequency - the usual case
double              samplfreq       = EEGDoc->GetSamplingFrequency ();
bool                missingorgsf    = samplfreq <= 0;

                                        // 2) then using altfilters optional field?
if ( samplfreq <= 0 /*&& filtersoptions == UsingOtherFilters*/ && altfilters )
                                        // filters most certainly have a sampling frequency set
    samplfreq = StringToDouble ( altfilters->FiltersParam.ValueSamplingFrequency );


                                        // 3) caller might have provided a saved value, either from previous calls or "as is"?
if ( samplfreq <= 0 && defaultsamplingfrequency > 0 )

    samplfreq   = defaultsamplingfrequency;

                                        // 4) at that point, we have to stop and ask user
if ( samplfreq <= 0 && samplingfreqout && ! silent ) {
                                        // Not thread-safe but still OK-ish, we are not supposed to run this in concurrent threads, or are we?
    if ( ! GetValueFromUser ( "Missing sampling frequency, please provide one:", EEGDoc->GetTitle (), defaultsamplingfrequency, "1000" ) ) {
//      return  false;
        }

    defaultsamplingfrequency    = AtLeast ( 0.0, defaultsamplingfrequency );
                                        // one more chance, but user might still have cancelled or set bad value..
    samplfreq   = defaultsamplingfrequency;
    }

                                        // in the end, do we need a sampling frequency but have none?
if ( samplfreq <= 0 && samplingfreqout )

//  return  false;                      // error?
    samplfreq   = 0;                    // or continue without sampling frequency?

                                        // if we finally have some sampling frequency, we might update these guys:
if ( samplfreq > 0 ) {

    if ( defaultsamplingfrequency <= 0 )
                                        // useful for any next batch calls
        defaultsamplingfrequency    = samplfreq;

    if ( missingorgsf )
                                        // updating EEG
        EEGDoc->SetSamplingFrequency ( samplfreq );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Silently setting the filtering state

                                        // Consistency checks that should have been addressed by the caller beforehand...
if ( filtersoptions == UsingOtherFilters   && ( altfilters == 0 || altfilters->HasNoFilters () )    // no filters set / activated in altfilters
  || filtersoptions == UsingCurrentFilters && ! EEGDoc->AreFiltersActivated () )                    // EEGDoc has filters BUT they are deactivated
                                        
    filtersoptions = NotUsingFilters;

                                        // New approach is to have a local filter object, so we know for sure it will NOT interfere with EEGDoc state
TTracksFilters<float>   filters;

if      ( filtersoptions == UsingCurrentFilters )   filters     = *EEGDoc->GetFilters ();
else if ( filtersoptions == UsingOtherFilters   )   filters     = *altfilters;
//else if ( filtersoptions == NotUsingFilters   )   filters.Reset ();

                                        // We can check here if the filter we got is really compatible with our current data
bool                spatialfiltererror  = filters.HasSpatialFilter () && filters.GetSpatialFilterDim () != numels;

if ( spatialfiltererror ) {
                                        // disable Spatial Filter
    filters.SpatialFiltering    = false;

    if ( ! silent ) {

        StringCopy (    buff,
                        "Spatial Filter has been disabled due to dimensions mismatch:"                          NewLine
                        NewLine
                        Tab "Spatial Filter Dimension = ", IntegerToString ( filters.GetSpatialFilterDim () ),  NewLine
                        Tab "Actual Number of Tracks  = ", IntegerToString ( numels ),                          NewLine
                        NewLine
                        "Try using the  'Other Filters'  option with an appropriate XYZ file."
                    );

        ShowMessage ( buff, EEGDoc->GetTitle (), ShowMessageWarning );
        }
    }


if ( downsample )
    downsamplfreq   = samplfreq / downsampleratio;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Buffers allocation

if ( baselinecorr )                     // also check for baseline length!
    Maxed ( maxtimechunk, baselinecorrnum );

                                        // output # of tracks - elsel already accounted for numaddedels
int                 outnumtracks    = tracksoptions == ProcessRois ? ROIs->GetNumRois () : (int) elsel;

                                        // allocate a max chunk buffer
TTracks<float>      eegb        ( numtotalels, maxtimechunk );
                                        // some more optional buffers
TVector<double>     eegbavg     ( sequenceoptions == AverageProcessing ? numels : 0 );

TArray2<float>      eegbrois    ( tracksoptions == ProcessRois ? outnumtracks : 0, 
                                  tracksoptions == ProcessRois ? maxtimechunk : 0  );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;
                                        // Frequency is annoying, concatenation is fast
if ( ! silent ) {

    Gauge.Set           ( ExportTracksTitle );

    Gauge.AddPart       ( 0, outtimenum );

    CartoolObjects.CartoolApplication->SetMainTitle ( "Exporting", EEGDoc->GetDocPath (), Gauge );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Simplified version of TTracksDoc::GetTracks
                                        // It allows for EEG buffer to have MORE TRACKS than the original input EEG, allowing us to include any added null channels in the processing
auto    GetTracks   =   [   &EEGDoc, 
                            &eegb,          &numels,    &numtimeframes,
                            &filters,       &ref,       &refsel,        &validsel,      &auxsel
                        ] ( long tf1, long tf2, long numtf, int tfoffset )
{
                                        // atom type, reference and filters have been resolved and set
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                doanyfilter         = filters.HasAnyFilter ();
bool                dotemporalfilters   = doanyfilter && filters.HasTemporalFilter ();
bool                dofilterauxs        = doanyfilter && CheckToBool ( filters.FiltersParam.FilterAuxs );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TTracksFiltersLimits<float>  timelimits;

if ( dotemporalfilters )
                                        // Adjust the time parameters and set the time margins for temporal filtering
    timelimits.UpdateTimeRange  (   tf1,        tf2,        numtf,      tfoffset,   // !update values with new limits!
                                    AtLeast ( 1, filters.GetSafeMargin () ),        // !make sure it has at least 1 TF so we don't need BuffDiss!
                                    numtimeframes
                                );

                                        // Adjust buffer to either original or expanded limits
                                        // Note that the previous content could be lost, though this usually is not a problem, as for filtered data, the buffer is usually evaluated as a whole
eegb.Resize (             eegb.GetDim1 (),
                AtLeast ( eegb.GetDim2 (), (int) numtf )  );

                                        // making sure any added channels will be 0
eegb.ResetMemory ();


EEGDoc->ReadRawTracks ( tf1, tf2, eegb, tfoffset );


if ( doanyfilter
  || IsEffectiveReference ( ref ) ) {   // !filters semantic do not include the reference for the moment!


    if ( dotemporalfilters )

        timelimits.MirrorData       (   eegb,   numels,
                                        numtf,  tfoffset    );

                                            // do ALL filters at once, including reference, with the correct sequence
    filters.ApplyFilters    (   eegb,       numels,  
                                numtf,      tfoffset,
                                ref,        &refsel,    &validsel,  dofilterauxs ? 0 : &auxsel,
                                doanyfilter ? AllFilters : NoFilter
                            );


    if ( dotemporalfilters )

        timelimits.RestoreTimeRange (   eegb,       numels,  
                                        tf1,        tf2,        numtf,      tfoffset,       // !restore original time limits!
                                        0
                                    );
    } // filter or reference
};


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute baseline correction values - this is not part of our time chunks
TVector<float>      baseline ( baselinecorr ? numels : 0 );

if ( baselinecorr ) {
                                        // get the baseline chunk only
    GetTracks   ( baselinecorrpre, baselinecorrpost, baselinecorrnum, 0 );


    for ( int  e   = 0; e   < numels;          e++   )
    for ( long tfi = 0; tfi < baselinecorrnum; tfi++ )

        baseline[ e ]  += eegb ( e , tfi );

    baseline   /= baselinecorrnum;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute rescaling factor and/or global max value
TMapAtomType        maxvalue        = EEGDoc->GetAbsMaxValue ();    // this is only an approximate value, as the EEG might not have been fully scanned


if ( rescalingoptions == GfpRescaling
  || createfile && IsExtension ( filename, FILEEXT_EEGEDF ) ) {
                                        // compute average Gfp across selected TFs, using current ref & filters
    rescalingfactor = 0;

    TMap            map ( numels );

                                        // have to loop through all our valid time chunks
    for ( int i = 0; i < timechunks.GetNumMarkers (); i++ ) {
                                        // read chunk
        long            numtf           = timechunks[ i ]->Length ();

        GetTracks   ( timechunks[ i ]->From, timechunks[ i ]->To, numtf, 0 );


        for ( long tfi = 0; tfi < numtf; tfi++ ) {

            map.GetColumn ( eegb, tfi );
                                        // at that point, we might have a baseline correction to apply, in case we don't use the average reference formula
            if ( baselinecorr )
                map    -= baseline;

                                        // get GFP
            if ( rescalingoptions == GfpRescaling )
                rescalingfactor    += map.GlobalFieldPower ( ! IsAbsolute ( datatype ), IsVector ( datatype ) );

                                        // compute exact maxvalue, including baseline
            Maxed ( maxvalue, map.GetAbsMaxValue () );
            }

        } // timechunks

                                        // here is the inverse average Gfp factor
    rescalingfactor = rescalingfactor == 0 ? 0 : intimenum / rescalingfactor;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Output file setup
if ( createfile ) {

    StringCopy ( expfile.Filename, filename );
    
                                        // vectorial RIS saved as norm - to save as vectorial would need more options from the dialog
    expfile.SetAtomType ( AtomTypeScalar );

                                        // common variables
    expfile.NumTracks           = outnumtracks;
    expfile.NumTime             = outtimenum;
    expfile.SamplingFrequency   = downsample ? downsamplfreq : samplfreq;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // specific variables
    if ( tracksoptions == ProcessTracks ) {

        expfile.SelTracks       = elsel;

        expfile.AuxTracks       = auxsel;
        expfile.NumAuxTracks    = ( elsel & auxsel ).NumSet ()                  // original auxiliaries which are to be saved
                                  + elsel.NumSet ( firstpseudo, lastpseudo );   // pseudo-tracks that will be saved among the other channels

        expfile.ElectrodesNames = ElectrodesNames;
        }
    else if ( tracksoptions == ProcessRois ) {

        expfile.SelTracks.Initialize ();

        expfile.AuxTracks.Initialize ();
        expfile.NumAuxTracks    = 0;

        expfile.ElectrodesNames = *ROIs->GetRoiNames ();
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // get the shift from starting point in ms
    long            usoffset        = TimeFrameToMicroseconds   (   intimemin                                                       // starting time point
                                                                  + downsampleshiftorg                                              // optional target bucket centering - currently 0
                                                                  + ( sequenceoptions == AverageProcessing ? intimenum / 2.0 : 0 ), // averaging -> shift to average position(?)
                                                                  samplfreq );
//                                  + ( filetype == PresetFileTypeEdf ? 500000.0 : 0.0 );                           // EDF can not write milliseconds, only seconds - to round to the closest second, add 500000 (microseconds)

    if ( timeoptions == ExportTimeInterval )     // create a new date, correctly shifted with msoffset

        expfile.DateTime        = TDateTime (   EEGDoc->DateTime.GetYear(),           
                                                EEGDoc->DateTime.GetMonth(),                    
                                                EEGDoc->DateTime.GetDay(),
                                                EEGDoc->DateTime.GetHour (),          
                                                EEGDoc->DateTime.GetMinute (),                  
                                                EEGDoc->DateTime.GetSecond (), 
                                                EEGDoc->DateTime.GetMillisecond (),   
                                                EEGDoc->DateTime.GetMicrosecond () + usoffset   // additional offset, which could nicely propagate to higher fields if needed
                                            );
    else // if ( timeoptions == ExportTimeTriggers || timeoptions == ExcludeTimeTriggers )

        expfile.DateTime        = TDateTime ( EEGDoc->DateTime.GetYear(),  EEGDoc->DateTime.GetMonth(),   EEGDoc->DateTime.GetDay(), 0, 0, 0, 0, 0 );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // for EDF, BV

                                        // file is only 0's, try to see if caller has set MaxValue before calling?
    if ( maxvalue <= 0 )    maxvalue    = expfile.MaxValue;
                                        // if still not OK, set a fall-back value - we really don't want a null value
    if ( maxvalue <= 0 )    maxvalue    = EdfPhysicalMaxDefault;

    expfile.MaxValue            = rescalingoptions == NotRescaled ? maxvalue
                                                                  : maxvalue * NonNull ( abs ( rescalingfactor ) );

    expfile.NumTags             = EEGDoc->GetNumMarkers ();
    expfile.TimeMin             = 0;              // sequenceoptions == AverageProcessing ? ( intimemin + intimenum / 2 ) : outtimemin;  // !we re-process the output triggers/markers position, now starting from 0!
    expfile.TimeMax             = outtimenum - 1; // sequenceoptions == AverageProcessing ? ( intimemin + intimenum / 2 ) : outtimemax;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // until the end, prevent any trigger and any marker to be written
    expfile.OutputTriggers      = NoTriggersNoMarkers;

    if ( concatenateoptions == ConcatenateTime ) {      // don't know the full time yet
        expfile.NumTime         = Highest ( expfile.NumTime );
        expfile.MaxValue       *= EdfPhysicalMaxMargin; // EDF might clip while running through all files, so raise the limit somehow by an educated guess scaling factor
        }

    } // if createfile
//else
    // expfile is still opened


if ( concatenateoptions == ConcatenateTime && IsBatchFirstCall )

    expfile.Begin ( true );             // write some dummy header, now


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // loop through all our valid time chunks


for ( int i = 0; i < timechunks.GetNumMarkers (); i++ ) {

    bool            lastchunk       = i == timechunks.GetNumMarkers () - 1;

                                        // read chunk, store to relative origin TF 0
    long            numtf           = timechunks[ i ]->Length ();

    GetTracks   ( timechunks[ i ]->From, timechunks[ i ]->To, numtf, 0 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Apply previously computed baseline correction on compacted data
    if ( baselinecorr ) {
                                        // subtract baseline correction - only on regular electrodes
        for ( int  e   = 0; e   < numels;   e++ )
        for ( long tfi = 0; tfi < numtf;    tfi++ )

            eegb ( e , tfi )   -= baseline[ e ];

        } // if baselinecorr


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // apply rescaling, if any
    if ( rescalingoptions != NotRescaled ) {

        for ( int  e   = 0; e   < numels;   e++ )
        for ( long tfi = 0; tfi < numtf;    tfi++ )

            eegb ( e , tfi )   *= rescalingfactor;

        } // if NotRescaled


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Time cumulation - either averaging or downsampling
    if ( sequenceoptions == AverageProcessing ) {

        Gauge.Next ( 0, SuperGaugeUpdateTitle );


        eegbavg.ResetMemory ();

        for ( int  e   = 0; e   < numels;   e++ )
        for ( long tfi = 0; tfi < numtf;    tfi++ )

            eegbavg[ e ]   += eegb ( e , tfi );     // cumulate values


        if ( lastchunk ) {
                                        // finalize the averaging
            eegbavg    /= intimenum;
                                        // transfer to eegb
            for ( int e = 0; e < numels; e++ )

                eegb ( e , 0 )  = eegbavg[ e ];
                                        // update our new local chunk length
            numtf   = 1;
            }
        else {

            continue;                   // the chunk loop
            }

        } // if AverageProcessing

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
    else if ( /*sequenceoptions == SequenceProcessing &&*/ downsample ) {

                                        // cumulate within same buffer (can)
        for ( int  e   = 0; e   < numels;   e++ )
        for ( long tfi = 0; tfi < numtf;    tfi++ )

            if ( ( tfi % downsampleratio ) == 0 )   eegb[ e ][ tfi / downsampleratio ]  = eegb ( e , tfi ); // set @ beginning of bucket
            else                                    eegb[ e ][ tfi / downsampleratio ] += eegb ( e , tfi ); // cumulate the other values


                                        // update our new local chunk length
        numtf   = numtf / downsampleratio;


        for ( int  e   = 0; e   < numels;   e++ )
        for ( long tfi = 0; tfi < numtf;    tfi++ )

            eegb ( e , tfi )   /= downsampleratio;

                                        // followed by a light high-pass FIR
        double              eegbdowntfm1;
        double              eegbdowntf0;
        double              eegbdowntfp1;

        for ( int e = 0; e < numels; e++ ) {
  
            eegbdowntf0     = eegb ( e , 0 );

            for ( long tfi = 0; tfi < numtf; tfi++ ) {

                eegbdowntfm1        = eegbdowntf0;
                eegbdowntf0         = eegb ( e , tfi );
                eegbdowntfp1        = eegb ( e , tfi < numtf - 1 ? tfi + 1 : tfi );
                eegb ( e , tfi )    = ( 18 * eegbdowntf0 - ( eegbdowntfm1 + eegbdowntfp1 ) ) / 16;
                }

                                        // if data is KNWOWN to be positive, better avoiding slightly negative results!
            if ( EEGDoc->IsAbsolute ( AtomTypeUseOriginal ) /*EEGDoc->IsPositive ()*/ )
                for ( long tfi = 0; tfi < numtf; tfi++ )
                    Maxed ( eegb ( e , tfi ), (float) 0 );
            } // for e

        } // if downsample


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here, numtf has been updated to the actual output size
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Finally, compute our pseudos (we haven't needed any pseudo before this point..)
    if ( dopseudos ) {

        TVector<float>      map1 ( numels );
        TVector<float>      map2 ( numels );

                                        // here, we don't have access to previous TF
        map1.GetColumn ( eegb, 0 );
                                        // here, we have access to data @ (intimemin - 1), so we can compute the actual Dis
//      map1.GetColumn ( eegb, intimemin ? intimemin - 1 : 0 ); // for first TF, will do dissimilarity with itself = 0
//      for ( int e = 0; e < numels; e++ )
//          map1[ e ]  -= baseline[ e ];


        for ( long tfi = 0; tfi < numtf; tfi++ ) {

            map2    = map1;

            map1.GetColumn ( eegb, tfi );

            if ( elsel[ gfpoff ] )  eegb ( gfpoff, tfi )    = map1.GlobalFieldPower (       validsel, ! IsAbsolute ( datatype ), IsVector ( datatype ) );
            if ( elsel[ disoff ] )  eegb ( disoff, tfi )    = map1.Dissimilarity    ( map2, validsel, ! IsAbsolute ( datatype ), IsVector ( datatype ) );
            if ( elsel[ avgoff ] )  eegb ( avgoff, tfi )    = map1.Average          (       validsel );
            }

        } // dopseudos


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if ( tracksoptions == ProcessRois )
                                        // It can not see any of our additional null tracks, as roidoc was opened before
        ROIs->Average ( eegb, 0, numtf - 1, FilterTypeMean, &eegbrois );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Output loop
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    for ( long tfi = 0; tfi < numtf; tfi++ ) {

        Gauge.Next ( 0, SuperGaugeUpdateTitle );


        if ( tracksoptions == ProcessTracks )
                                        // this can include the optional null tracks, which are not computed, hence 0's
            for ( TIteratorSelectedForward seli ( elsel ); (bool) seli; ++seli )

                expfile.Write ( eegb ( seli(), tfi ) );

        else // tracksoptions == ProcessRois
                                        // !NOT tested for optional null tracks!
            for ( int e = 0; e < outnumtracks; e++ )

                expfile.Write ( eegbrois ( e , tfi ) );

        }

    } // for timechunks


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // verbose file filling, one per new file, or only one in case of concatenation
                                        // wait for last batch call to have full time extends
if ( closefile ) {

    string          auxsandnulls    = AuxiliaryTracksNames + ( numaddedels ? " " + string ( addingnullchannels ) : "" );


    TFileName       verbosefilename ( filename );
    AddExtension    ( verbosefilename, FILEEXT_VRB );

    TVerboseFile    verbose ( verbosefilename, VerboseFileDefaultWidth );

    verbose.PutTitle ( ExportTracksTitle );


    verbose.NextTopic ( "Files:" );
    {
    if ( concatenateoptions == ConcatenateTime ) {
        verbose.Put ( "Number of Input Files:", NumBatchFiles );

        for ( int i = 0; i < NumBatchFiles; i++ )
            verbose.Put ( ! i ? "Input   Files        :" : "", (*batchfilenames)[ i ] );

        verbose.NextLine ();
        }
    else
        verbose.Put ( "Input   File         :", EEGDoc->GetDocPath () );

    if ( StringIsNotEmpty ( outputdir ) )
        verbose.Put ( "Output  Directory    :", outputdir );
    verbose.Put ( "Output  File         :", filename );
    verbose.Put ( "Verbose File (this)  :", verbosefilename );

    verbose.NextLine ();
    verbose.Put ( "Using electrode names from file:", xyzdoc  ? xyzdoc ->GetDocPath () : "No" );
    verbose.Put ( "Using ROIs            from file:", roisdoc ? roisdoc->GetDocPath () : "No" );
    if ( concatenateoptions == ConcatenateTime )
        verbose.Put ( "Concatenate into 1 file:", concatenateoptions == ConcatenateTime );
    }


    verbose.NextTopic ( "Exporting:" );
    {
    if ( EEGDoc->HasMultipleSessions () ) {
        verbose.Put ( "Session exported:",          EEGDoc->GetCurrentSession () );
        verbose.Put ( "Total number of sessions:",  EEGDoc->GetNumSessions () );
        verbose.NextLine ();
        }

    verbose.Put ( "Type of export:", tracksoptions == ProcessTracks ? "Tracks" : "ROIs" );

    if      ( tracksoptions == ProcessTracks ) {
    //  verbose.Put ( "Writing tracks:", transfer->Tracks );
        verbose.Put ( "Number of tracks:", outnumtracks );
                                                                            // always enumerate the added channels
        verbose.Put ( "Track names:",   elsel.ToText ( buff, &ElectrodesNames, auxsandnulls.c_str () ) );
        }
    else if ( tracksoptions == ProcessRois ) {
        verbose.Put ( "Number of ROIs:", outnumtracks );
        verbose.Put ( "ROIs names:",     ROIs->RoiNamesToText ( buff ) );
        }
    }


    if ( isfrequency ) {
        verbose.NextTopic ( "Frequency:" );
        verbose.Put ( "Frequency file name infix:", freqinfix );
        }


    verbose.NextTopic ( "Input Tracks:" );
    {
    TSelection      insel ( numels,     OrderSorted    );    EEGDoc->SetRegular ( insel );

    verbose.Put ( "Input channels:",            (bool) insel    ? insel   .ToText ( buff, &ElectrodesNames, auxsandnulls.c_str () ) : "None" );
    verbose.Put ( "Input auxiliary channels:",  (bool) auxsel   ? auxsel  .ToText ( buff, &ElectrodesNames, auxsandnulls.c_str () ) : "None" );
    verbose.Put ( "Input bad channels:",        (bool) badsel   ? badsel  .ToText ( buff, &ElectrodesNames, auxsandnulls.c_str () ) : "None" );
    verbose.Put ( "Input valid channels:",      (bool) validsel ? validsel.ToText ( buff, &ElectrodesNames, auxsandnulls.c_str () ) : "None" );
    }


    verbose.NextTopic ( "Input Time Period:" );
    {
    verbose.Put ( "Input time range defined by:", timeoptions == ExportTimeInterval ? "Interval" : /*timeoptions == ExportTimeTriggers || timeoptions == ExcludeTimeTriggers*/ "Triggers" );

    if      ( timeoptions == ExportTimeInterval ) {
        verbose.Put ( "From      Time Frame :", (int) intimemin );
        verbose.Put ( "To        Time Frame :", (int) intimemax );
        }
    else if ( timeoptions == ExportTimeTriggers ) {
        verbose.Put ( "Keeping Triggers     :", triggerlist.ToString ( buff, true ) );
        }
    else if ( timeoptions == ExcludeTimeTriggers ) {
        verbose.Put ( "Excluding Triggers   :", triggerlist.ToString ( buff, true ) );
        }

    if ( concatenateoptions == ConcatenateTime )
        verbose.Put ( "Number of Time Frames, for all files:", *concatinputtime );
    else
        verbose.Put ( "Number of Time Frames:", (int) intimenum );
    }


    verbose.NextTopic ( "Processing Parameters:" );
    {
    if      ( datatype == AtomTypeScalar    )   verbose.Put ( "Data type:", "Signed Data" );
    else if ( datatype == AtomTypePositive  )   verbose.Put ( "Data type:", "Positive Data" );


    verbose.NextLine ();
    verbose.Put ( "Adding null tracks:", numaddedels > 0 ? addingnullchannels : "No" );


    verbose.NextLine ();
    if      ( filtersoptions == UsingCurrentFilters )   verbose.Put ( "Current filters:",   filters.ParametersToText ( buff ) );
    else if ( filtersoptions == UsingOtherFilters   )   verbose.Put ( "Other filters:",     filters.ParametersToText ( buff ) );
    else                                                verbose.Put ( "Filters:",           false );
    if      ( filtersoptions != NotUsingFilters && (bool) auxsel )       
                                                        verbose.Put ( "Filtering auxiliary channels:",   CheckToBool ( filters.FiltersParam.FilterAuxs ) );
    if ( spatialfiltererror )                           verbose.Put ( "", "Spatial filter has been disabled due to dimensions mismatch" );


    verbose.NextLine ();
    if      ( datatype == AtomTypePositive
           || ref == ReferenceAsInFile          )       verbose.Put ( "Reference:", "No reference, as in file" );
    else if ( ref == ReferenceAverage           )       verbose.Put ( "Reference:", "Average reference" );
    else if ( ref == ReferenceArbitraryTracks   )       verbose.Put ( "Reference:", refsel.ToText ( buff, &ElectrodesNames, auxsandnulls.c_str () ) );
    else if ( ref == ReferenceUsingCurrent      ) {

        if      ( EEGDoc->GetReferenceType () == ReferenceAverage  )    verbose.Put ( "Current reference:", "Average reference" );
        else if ( EEGDoc->GetReferenceType () == ReferenceAsInFile )    verbose.Put ( "Current reference:", "No reference, as in file" );
        else                                                            verbose.Put ( "Current reference:", EEGDoc->GetReferenceTracks ().ToText ( buff, &ElectrodesNames, auxsandnulls.c_str () ) );
        }
    else                                                verbose.Put ( "Reference:", "Unknown" );


    verbose.NextLine ();
    verbose.Put ( "Baseline correction:", baselinecorr );

    if ( baselinecorr ) {
        verbose.Put ( "Inferior limit (absolute value) in TF:", baselinecorrpre );
        verbose.Put ( "Superior limit (absolute value) in TF:", baselinecorrpost );
        }


    verbose.NextLine ();
    verbose.Put ( "Rescaling:", rescalingoptions != NotRescaled );

    if ( rescalingoptions != NotRescaled ) {

        if      ( rescalingoptions == ConstantRescaling )
            verbose.Put ( "Rescaling method:", "Fixed value" );
        else if ( rescalingoptions == GfpRescaling )
            verbose.Put ( "Rescaling method:", "Mean GFP" );

        if ( rescalingoptions == ConstantRescaling || rescalingoptions == GfpRescaling && concatenateoptions == NoConcatenateTime )
            verbose.Put ( "Rescaling factor:", rescalingfactor );
                                        // concatenation: one factor per file
        }
    }


    verbose.NextTopic ( "Output Tracks:" );
    {
    TSelection      outauxsel   = elsel & auxsel;
    TSelection      outbadsel   = elsel & badsel;
    TSelection      outvalidsel = elsel & validsel;

    verbose.Put ( "Output channels:",           (bool) elsel        ? elsel      .ToText ( buff, &ElectrodesNames, auxsandnulls.c_str () ) : "None" );
    verbose.Put ( "Output auxiliary channels:", (bool) outauxsel    ? outauxsel  .ToText ( buff, &ElectrodesNames, auxsandnulls.c_str () ) : "None" );
    verbose.Put ( "Output bad channels:",       (bool) outbadsel    ? outbadsel  .ToText ( buff, &ElectrodesNames, auxsandnulls.c_str () ) : "None" );
    verbose.Put ( "Output valid channels:",     (bool) outvalidsel  ? outvalidsel.ToText ( buff, &ElectrodesNames, auxsandnulls.c_str () ) : "None" );
    }


    verbose.NextTopic ( "Output Time:" );
    {
    if ( concatenateoptions == ConcatenateTime )
        verbose.Put ( "Output time is:", sequenceoptions == SequenceProcessing ? "Sequential" : "Sequence of each file's average" );
    else
        verbose.Put ( "Output time is:", sequenceoptions == SequenceProcessing ? "Sequential" : "Average" );

    verbose.Put ( "Number of output Time Frames:", (int) ( concatenateoptions == ConcatenateTime ? *concatoutputtime : expfile.NumTime ) );

    if ( sequenceoptions == SequenceProcessing ) {
        verbose.NextLine ();
        verbose.Put ( "Time downsampling:", downsample );

        if ( downsample ) {
            verbose.Put ( "Downsampling ratio:", downsampleratio );
            verbose.Put ( "Downsampled Sampling Frequency [Hz]:", downsamplfreq );
            }
        }
    }


    verbose.NextTopic ( "Options:" );
    {
    verbose.Put ( "File name infix:",   infix );
    verbose.Put ( "Output file type:",  SavingEegFileExtPreset[ filetype ] );
    verbose.Put ( "Saving markers:",    outputmarkers );
    }


    verbose.NextTopic ( "Processing Summary:" );
    (ofstream&) verbose << "Data is processed according to this sequence:" << fastendl;
    (ofstream&) verbose                                                             << fastendl;
    (ofstream&) verbose << "    * Reading Raw Data from file"                       << fastendl;
    (ofstream&) verbose << "    * Adding Null Tracks"                               << fastendl;
    (ofstream&) verbose << "    * Pre-Reference Filtering"                          << fastendl;
    (ofstream&) verbose << "    * Re-referencing"                                   << fastendl;
    (ofstream&) verbose << "    * Post-Reference Filtering"                         << fastendl;
    (ofstream&) verbose << "    * Applying baseline correction"                     << fastendl;
    (ofstream&) verbose << "    * Rescaling"                                        << fastendl;
    (ofstream&) verbose << "    * Temporal Downsampling or Computing Time Average"  << fastendl;
    (ofstream&) verbose << "    * Computing ROIs"                                   << fastendl;


    verbose.NextLine ();
    verbose.NextLine ();
    } // verbose file


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We are done with the last file, so now we have the correct final length
if ( concatenateoptions == ConcatenateTime && IsBatchLastCall ) {
                                        // set real duration
    expfile.NumTime     = *concatoutputtime;
                                        // overwrite the header
    expfile.WriteHeader ( true );       
                                        // !don't close - EDF case!
//  expfile.End ();
    }


if ( closefile ) {

    if ( outputmarkers  ) {
                                        // set appropriate flags, NOW
        expfile.OutputTriggers      = CombineFlags  (   outputmarkers   ? MarkersAsMarkers   : NoMarkers,
                                                        outputtriggers  ? TriggersAsTriggers : NoTriggers );    // TExportTracks will elegantly handle triggers-to-markers conversion, if file type does not support native triggers


        expfile.WriteTriggers ();

        expfile.WriteMarkers  ();
        }


    expfile.End ();

                                        // and just to be totally clean:
    expfile.Markers.ResetMarkers ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! silent ) {

    Gauge.FinishParts ();

    CartoolObjects.CartoolApplication->SetMainTitle ( ExportTracksTitle, expfile.Filename, Gauge );

    UpdateApplication;
    }


return  true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
