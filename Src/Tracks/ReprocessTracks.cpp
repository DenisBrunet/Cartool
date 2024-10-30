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
                        SavingEegFileTypes  filetype,
                        const char*         infixfilename,
                        const char*         freqinfix,              // for frequency processing purpose
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


if ( tracksoptions == ProcessRois && roisdoc == 0 )
    return  false;

                                        // Off, as this will be checked later on
//if ( timeoptions == ExportTimeInterval && ( timemin < 0 || timemax < 0 || timemin > timemax ) )
//    return  false;

                                        // or do we allow to export empty files?
if ( timeoptions == ExportTimeTriggers && StringIsEmpty ( triggerstokeeplist ) )
    return  false;


if ( filtersoptions == UsingOtherFilters && altfilters == 0 )
    return  false;

                                        // reset flag if there are indeed no filters set / activated in altfilters
if ( filtersoptions == UsingOtherFilters && altfilters->HasNoFilters () )
    filtersoptions = NotUsingFilters;

                                        // if EEGDoc has existing BUT deactivated filters, reset use of filters - Note that this preferably be addressed by caller beforehand...
if ( filtersoptions == UsingCurrentFilters && ! EEGDoc->AreFiltersActivated () )
    filtersoptions = NotUsingFilters;


if ( ( ref == ReferenceSingleTrack || ref == ReferenceMultipleTracks ) && StringIsEmpty ( reflist ) )
    return  false;


if ( baselinecorr && baselinecorrpre > baselinecorrpost )
    return  false;


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


int                 numregel            = EEGDoc->GetNumElectrodes   ();
int                 numtotalel          = EEGDoc->GetTotalElectrodes ();

TSplitStrings       addedchannels ( addingnullchannels, NonUniqueStrings );
int                 numaddedchannels    = addedchannels.GetNumTokens ();

long                numtimeframes       = EEGDoc->GetNumTimeFrames   ();
long                lasttimeframe       = numtimeframes - 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const TRois*        ROIs                = tracksoptions == ProcessRois && roisdoc ? roisdoc->ROIs : 0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Clone electrodes names, but keep the pseudo from the EEG (case of RMS instead of GFP)
TStrings            ElectrodesNames;


ElectrodesNames.Set ( EEGDoc->GetTotalElectrodes () + numaddedchannels, ElectrodeNameSize );

                                        // copy regular electrode names, preferably from the XYZ
for ( int ei = EEGDoc->GetFirstRegularIndex (); ei <= EEGDoc->GetLastRegularIndex (); ei++ )

    StringCopy ( ElectrodesNames[ ei ], xyzdoc ? xyzdoc->GetElectrodeName ( ei ) : EEGDoc->GetElectrodeName ( ei ) );

                                        // copy pseudo names, always from EEG
for ( int ei = EEGDoc->GetFirstPseudoIndex (); ei <= EEGDoc->GetLastPseudoIndex (); ei++ )

    StringCopy ( ElectrodesNames[ ei ], EEGDoc->GetElectrodeName ( ei ) );

                                        // finally, add the optional null tracks
for ( int ni = 0, ei = EEGDoc->GetLastPseudoIndex () + 1; ni < numaddedchannels; ni++, ei++ )

    StringCopy ( ElectrodesNames[ ei ], addedchannels[ ni ] );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // list of channels -> TSelection
TSelection          elsel  ( numtotalel + numaddedchannels, OrderArbitrary );
TSelection          refsel ( numtotalel, OrderSorted    );
char                buff[ EditSizeTextLong ];

ClearString ( buff );
StringCopy  ( buff, tracks );

elsel.Reset ();

                                        // force to all regular electrodes, only
if ( tracksoptions == ProcessRois || StringIsSpace ( buff ) )

    EEGDoc->SetRegular ( elsel );

else if ( StringContains ( (const char*) buff, "*" ) ) {
                                        // bypass the '*' of TSelection with only our regular electrodes
    EEGDoc->SetRegular ( elsel );

                                        // but also look for a remaining selection
    ReplaceChars ( buff, "*", " " );

    TSelection          elselother ( numtotalel, OrderArbitrary );

    elselother.Set ( buff, &ElectrodesNames, ! silent );

    elsel      += elselother;
    }

else                                    // get electrodes list
    elsel.Set ( buff, &ElectrodesNames, ! silent );

                                        // ?does it work with rois?
if ( tracksoptions != ProcessRois )

    for ( int ni = 0, ei = EEGDoc->GetLastPseudoIndex () + 1; ni < numaddedchannels; ni++, ei++ )

        elsel.Set ( ei );

                                        // aborting now?
if ( tracksoptions == ProcessTracks && elsel.NumSet () == 0 )
    return  false;


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

    Clipped ( baselinecorrpre,  baselinecorrpost,   (long) 0, lasttimeframe );

    baselinecorrnum     = baselinecorrpost - baselinecorrpre + 1;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool                haspseudos          = EEGDoc->HasPseudoElectrodes ();

bool                dopseudos           = haspseudos 
                                       && tracksoptions == ProcessTracks 
                                       && ( EEGDoc->GetNumSelectedPseudo ( elsel ) > 0 );   // can we / do we need to handle the pseudos?

int                 gfpoff              = EEGDoc->GetGfpIndex ();
int                 disoff              = EEGDoc->GetDisIndex ();
int                 avgoff              = EEGDoc->GetAvgIndex ();

                                        // triggers only allowed to mimick the original file, i.e. time intervals or excluded markers
bool                outputtriggers      = outputmarkers
                                       && timeoptions != ExportTimeTriggers 
                                       && sequenceoptions == SequenceProcessing;

bool                createfile          = concatenateoptions == NoConcatenateTime                       // single file
                                       || concatenateoptions == ConcatenateTime && IsBatchFirstCall;    // concatenation and first file

bool                closefile           = concatenateoptions == NoConcatenateTime                       // single file
                                       || concatenateoptions == ConcatenateTime && IsBatchLastCall;     // concatenation and last file

TTracksFilters<float>   oldfilters;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TArray1<int>        reassignedtf;
TSplitStrings       triggerlist;

TMarker             marker;

TMarkers            remaintags;
TMarkers            timechunks;         // this list will hold all valid time intervals to process, even for the sequential case
long                maxtimechunk    = 0;
long                chunklength;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//TExportTracks       expfile;

if ( createfile )

    expfile.Markers.ResetMarkers ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Setting data type, reference, and reading parameters
AtomType         	datatype        = EEGDoc->GetAtomType ( AtomTypeUseOriginal );

                                        // force reset reference?
//if (    IsPositive ( datatype )
//     || IsAngular  ( datatype ) )   ref = ReferenceAsInFile;


if ( ref == ReferenceSingleTrack || ref == ReferenceMultipleTracks ) {

    refsel.Reset ();

    refsel.Set    ( reflist, &ElectrodesNames, ! silent );

                                        // that should not be...
    if ( refsel.NumSet () == 0 )
        ref     = ReferenceAsInFile;          // either setting to no reference, as is currently done in  TTracksDoc::SetReferenceType
//      return  false;                  // or exiting?

                                        // allow us to override single / multiple electrode(s) ref
    if      ( refsel.NumSet () == 1 && ref == ReferenceMultipleTracks )     ref     = ReferenceSingleTrack;
    else if ( refsel.NumSet () >  1 && ref == ReferenceSingleTrack    )     ref     = ReferenceMultipleTracks;
    }

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


        if ( StringIsNotEmpty ( infixfilename ) )
            StringAppend ( filename, ".", infixfilename );

        if ( StringIsNotEmpty ( freqinfix ) )
            StringAppend ( filename, StringIsEmpty ( infixfilename ) ? "." : " ", freqinfix );
        }

                                        // append extension if not already there?
    if ( ! IsExtension ( filename, SavingEegFileExtPreset[ filetype ] ) )

        AddExtension ( filename, SavingEegFileExtPreset[ filetype ] );

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
    timechunks.AppendMarker ( TMarker ( intimemin, intimemax, 1, "Block", MarkerTypeTemp ), false );


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
        if ( intimemin > 0              )   remaintags.ClipMarkers ( 0,             intimemin - 1,     AllMarkerTypes, false );
        if ( intimemax < lasttimeframe  )   remaintags.ClipMarkers ( intimemax + 1, lasttimeframe,     AllMarkerTypes, false );


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
        remaintags.ClipMarkers ( cliptags, AllMarkerTypes, false );


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

        remaintags.RemoveMarkers   ( triggerstoexcludelist, AllMarkerTypes, false );
                                        // punching all time periods from excludetags - overlapping markers OK
        remaintags.ClipMarkers ( excludetags, AllMarkerTypes, false );


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need this to remap the old TFs to the new output ones
        reassignedtf.Resize ( numtimeframes );
        reassignedtf   = -1;

        long                tf0             = 0;
                                        // this is identical to remaintags, except "flatten" so no overlapping markers anymore
        for ( int ti = 0; ti < timechunks.GetNumMarkers (); ti++ ) {
                                        // adding splice markers (only) in-between 2 deleted blocks
            if ( ti > 0 )
                remaintags.InsertMarker ( TMarker ( timechunks[ ti ]->From, "Splice", MarkerTypeMarker ), false );

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
                                              MarkerTypeMarker ), false );


    expfile.Markers.AppendMarker ( TMarker (  *concatoutputtime, 
                                              EEGDoc->GetTitle (), 
                                              MarkerTypeMarker ), false );
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
        expfile.Markers.AppendMarker ( marker, false );
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
    if ( ! GetValueFromUser ( "Missing sampling frequency, please provide one:", (char *) EEGDoc->GetTitle (), defaultsamplingfrequency, "1000" ) ) {
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
if      ( filtersoptions == UsingCurrentFilters )

    oldfilters  = *EEGDoc->GetFilters ();

else if ( filtersoptions == UsingOtherFilters ) {

    oldfilters  = *EEGDoc->GetFilters ();

    EEGDoc->SetFilters ( altfilters, 0, true );
    }

                                        // if we modify the filters in an already opened file, we need to restore them to this state once we are done
bool                oldfiltersactivated = filtersoptions == NotUsingFilters ? EEGDoc->DeactivateFilters ( true ) 
                                                                            : EEGDoc->ActivateFilters   ( true );


if ( downsample )
    downsamplfreq   = samplfreq / downsampleratio;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Buffers allocation

if ( baselinecorr )                     // also check for baseline length!
    maxtimechunk    = max ( (long) baselinecorrnum, maxtimechunk );

                                        // output # of tracks - elsel already accounted for numaddedchannels
int                 outnumtracks    = tracksoptions == ProcessRois && ROIs ? ROIs->GetNumRois () + numaddedchannels: (int) elsel;

                                        // allocate a max chunk buffer
TTracks<float>      eegb ( numtotalel + numaddedchannels, maxtimechunk );

                                        // some more buffers, if needed
TVector<double>     eegbavg;
TArray2<float>      eegbrois;


if ( sequenceoptions == AverageProcessing )
    eegbavg .Resize ( numregel );

if ( tracksoptions == ProcessRois )
    eegbrois.Resize ( outnumtracks, maxtimechunk );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge;
                                        // Frequency is annoying, concatenation is fast
if ( ! silent ) {

    Gauge.Set           ( ExportTracksTitle );
    Gauge.AddPart       ( 0, outtimenum );

    CartoolObjects.CartoolApplication->SetMainTitle ( "Exporting", EEGDoc->GetDocPath (), Gauge );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute baseline correction values - this is not part of our time chunks
TVector<float>      baseline;


if ( baselinecorr ) {
                                        // create the buffer, only on real electrodes (no pseudos)
    baseline.Resize ( numregel );

                                        // get the baseline chunk only, no need for pseudos
                                        // vectorial RIS converted to norm
    EEGDoc->GetTracks   (   baselinecorrpre,    baselinecorrpost, 
                            eegb,               0, 
                            AtomTypeUseCurrent, 
                            NoPseudoTracks /*dopseudos*/, 
                            ref,                &refsel
                        );


    for ( int e = 0; e < numregel; e++ )
    for ( long tfi = 0; tfi < baselinecorrnum; tfi++ )

        baseline[ e ]  += eegb ( e , tfi );

    baseline   /= baselinecorrnum;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute rescaling factor
if ( rescalingoptions == GfpRescaling ) {
                                        // compute average Gfp across selected TFs, using current ref & filters
    rescalingfactor = 0;

    TMap            map ( numregel );

                                        // have to loop through all our valid time chunks
    for ( int i = 0; i < timechunks.GetNumMarkers (); i++ ) {
                                        // read chunk
        EEGDoc->GetTracks   (   timechunks[ i ]->From,  timechunks[ i ]->To, 
                                eegb,                   0,
                                AtomTypeUseCurrent, 
                                NoPseudoTracks /*dopseudos*/, 
                                ref,                    &refsel
                            );

        chunklength     = timechunks[ i ]->Length ();


        for ( long tfi = 0; tfi < chunklength; tfi++ ) {

            map.GetColumn ( eegb, tfi );
                                        // at that point, we might have a baseline correction to apply
            if ( baselinecorr )
                map    -= baseline;
                                        // get GFP
            rescalingfactor    += map.GlobalFieldPower ( ! IsAbsolute ( datatype ), IsVector ( datatype ) );
            }

        } // timechunks

                                        // here is the average Gfp factor
    rescalingfactor = rescalingfactor ? intimenum / rescalingfactor : 0;
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

        expfile.NumAuxTracks    = ( elsel & EEGDoc->GetAuxTracks () ).NumSet ()
                                  + EEGDoc->GetNumSelectedPseudo ( elsel );

        expfile.SelTracks       = elsel;
                                        // !currently, adding null tracks interferes with TExportTracks aux detection!
        if ( numaddedchannels == 0 )
            expfile.AuxTracks       = EEGDoc->GetAuxTracks ();

        expfile.ElectrodesNames = ElectrodesNames;
        }
    else if ( tracksoptions == ProcessRois ) {

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
    expfile.MaxValue            = EEGDoc->GetAbsMaxValue () * ( rescalingfactor ? abs ( rescalingfactor ) : 2 );
    expfile.NumTags             = EEGDoc->GetNumMarkers ();
    expfile.TimeMin             = 0;              // sequenceoptions == AverageProcessing ? ( intimemin + intimenum / 2 ) : outtimemin;  // !we re-process the output triggers/markers position, now starting from 0!
    expfile.TimeMax             = outtimenum - 1; // sequenceoptions == AverageProcessing ? ( intimemin + intimenum / 2 ) : outtimemax;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // until the end, prevent any trigger and any marker to be written
    expfile.OutputTriggers      = NoTriggersNoMarkers;

    if ( concatenateoptions == ConcatenateTime ) {  // don't know the full time yet
        expfile.NumTime         = Highest ( expfile.NumTime );
        expfile.MaxValue       *= 2;    // EDF may clip while running through all files, so raise the limit somehow
        }

    } // if createfile
//else
    // expfile is still opened


if ( concatenateoptions == ConcatenateTime && IsBatchFirstCall )

    expfile.Begin ( true );             // write some dummy header, now


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numtimechuncks  = timechunks.GetNumMarkers ();
bool                lastchunk;

                                        // loop through all our valid time chunks
for ( int i = 0; i < timechunks.GetNumMarkers (); i++ ) {

                                        // read chunk, store to relative origin TF 0
    EEGDoc->GetTracks   (   timechunks[ i ]->From, timechunks[ i ]->To, 
                            eegb,                   0,
                            AtomTypeUseCurrent, 
                            NoPseudoTracks /*dopseudos*/, 
                            ref,                    &refsel
                        );


    chunklength     = timechunks[ i ]->Length ();

    lastchunk       = i == numtimechuncks - 1;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Apply previously computed baseline correction on compacted data
    if ( baselinecorr ) {
                                        // subtract baseline correction - only on regular electrodes
        for ( int   e   = 0; e   < numregel;    e++   )
        for ( long tfi = 0; tfi < chunklength; tfi++ )

            eegb ( e , tfi )   -= baseline[ e ];

        } // if baselinecorr


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // apply rescaling, if any
    if ( rescalingoptions != NotRescaled ) {

        for ( int   e   = 0; e   < numregel;    e++   )
        for ( long tfi = 0; tfi < chunklength; tfi++ )

            eegb ( e , tfi )   *= rescalingfactor;

        } // if NotRescaled


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Time cumulation - either averaging or downsampling
    if ( sequenceoptions == AverageProcessing ) {

        Gauge.Next ( 0, SuperGaugeUpdateTitle );


        for ( int   e   = 0; e   < numregel;    e++   )
        for ( long tfi = 0; tfi < chunklength; tfi++ )

            eegbavg[ e ]   += eegb ( e , tfi );     // cumulate values


        if ( lastchunk ) {
                                        // finalize the averaging
            eegbavg    /= intimenum;
                                        // transfer to eegb
            for ( int e = 0; e < numregel; e++ )

                eegb ( e , 0 )  = eegbavg[ e ];
                                        // update our new local chunk length
            chunklength     = 1;
            }
        else {

            continue;                   // the chunk loop
            }

        } // if AverageProcessing

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
    else if ( /*sequenceoptions == SequenceProcessing &&*/ downsample ) {

                                        // cumulate within same buffer (can)
        for ( int   e   = 0; e   < numregel;    e++   )
        for ( long tfi = 0; tfi < chunklength; tfi++ )

            if ( ( tfi % downsampleratio ) == 0 )   eegb[ e ][ tfi / downsampleratio ]  = eegb ( e , tfi ); // set @ beginning of bucket
            else                                    eegb[ e ][ tfi / downsampleratio ] += eegb ( e , tfi ); // cumulate the other values


                                        // update our new local chunk length
        chunklength     = chunklength / downsampleratio;


        for ( int   e   = 0; e   < numregel;    e++   )
        for ( long tfi = 0; tfi < chunklength; tfi++ )

            eegb ( e , tfi )   /= downsampleratio;

                                        // followed by a light high-pass FIR
        double              eegbdowntfm1;
        double              eegbdowntf0;
        double              eegbdowntfp1;

        for ( int e = 0; e < numregel; e++ ) {
  
            eegbdowntf0     = eegb ( e , 0 );

            for ( long tfi = 0; tfi < chunklength; tfi++ ) {

                eegbdowntfm1        = eegbdowntf0;
                eegbdowntf0         = eegb ( e , tfi );
                eegbdowntfp1        = eegb ( e , tfi < chunklength - 1 ? tfi + 1 : tfi );
                eegb ( e , tfi )    = ( 18 * eegbdowntf0 - ( eegbdowntfm1 + eegbdowntfp1 ) ) / 16;
                }

                                        // if data is KNWOWN to be positive, better avoiding slightly negative results!
            if ( EEGDoc->IsAbsolute ( AtomTypeUseOriginal ) /*EEGDoc->IsPositive ()*/ )
                for ( long tfi = 0; tfi < chunklength; tfi++ )
                    Maxed ( eegb ( e , tfi ), (float) 0 );
            } // for e

        } // if downsample


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // here, chunklength has been updated to the output size
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Final pseudos (we haven't needed any pseudo before this point..)
    if ( dopseudos ) {

        bool                dogfp               = dopseudos && elsel[ gfpoff ]; // we can be selective to skip useless & costly computations
        bool                dodis               = dopseudos && elsel[ disoff ];
        bool                doavg               = dopseudos && elsel[ avgoff ];
        TVector<float>      map1 ( numregel );
        TVector<float>      map2 ( numregel );

                                        // here, we don't have access to previous TF
        map1.GetColumn ( eegb, 0 );
                                        // here, we have access to data @ (intimemin - 1), so we can compute the actual Dis
//          map1.GetColumn ( eegb, intimemin ? intimemin - 1 : 0 ); // for first TF, will do dissimilarity with itself = 0
//          for ( int e = 0; e < numregel; e++ )
//              map1[ e ]  -= baseline[ e ];


        for ( long tfi = 0; tfi < chunklength; tfi++ ) {

            map2    = map1;

            map1.GetColumn ( eegb, tfi );

            if ( dogfp )    eegb ( gfpoff , tfi )   = map1.GlobalFieldPower (       ! IsAbsolute ( datatype ), IsVector ( datatype ) );
            if ( dodis )    eegb ( disoff , tfi )   = map1.Dissimilarity    ( map2, ! IsAbsolute ( datatype ), IsVector ( datatype ) );
            if ( doavg )    eegb ( avgoff , tfi )   = map1.Average          ();
            }

        } // dopseudos


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // ROIs
    if ( tracksoptions == ProcessRois && ROIs )

        ROIs->Average ( eegb, 0, chunklength - 1, FilterTypeMean, &eegbrois );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Output loop
    for ( long tfi = 0; tfi < chunklength; tfi++ ) {

        Gauge.Next ( 0, SuperGaugeUpdateTitle );


        if ( tracksoptions == ProcessTracks )
                                        // this can include the optional null tracks, which are not computed, hence 0's
            for ( TIteratorSelectedForward seli ( elsel ); (bool) seli; ++seli )

                expfile.Write ( eegb    [ seli() ][ tfi ] );

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
        verbose.Put ( "Input   File         :", EEGDoc->GetDocPath() );

    verbose.Put ( "Output  File         :", filename );
    verbose.Put ( "Verbose File (this)  :", verbosefilename );

    verbose.NextLine ();
    verbose.Put ( "Using electrode names from file:", xyzdoc  ? xyzdoc ->GetDocPath() : "No" );
    verbose.Put ( "Using ROIs            from file:", roisdoc ? roisdoc->GetDocPath() : "No" );
    if ( concatenateoptions == ConcatenateTime )
        verbose.Put ( "Concatenate into 1 file:", concatenateoptions == ConcatenateTime );
    }


    verbose.NextTopic ( "Exporting:" );
    {
    if ( EEGDoc->GetNumSessions () > 1 ) {
        verbose.Put ( "Session exported:", EEGDoc->GetCurrentSession () );
        verbose.Put ( "Total number of sessions:", EEGDoc->GetNumSessions () );
        verbose.NextLine ();
        }

    verbose.Put ( "Type of export:", tracksoptions == ProcessTracks ? "Tracks" : "ROIs" );

    if      ( tracksoptions == ProcessTracks ) {
    //  verbose.Put ( "Writing tracks:", transfer->Tracks );
        verbose.Put ( "Number of tracks:", outnumtracks );
        verbose.Put ( "Track names:",      elsel.ToText ( buff, &ElectrodesNames, AuxiliaryTracksNames ) );
        }
    else if ( tracksoptions == ProcessRois && ROIs ) {
        verbose.Put ( "Number of ROIs:", outnumtracks );
        verbose.Put ( "ROIs names:",     ROIs->RoiNamesToText ( buff ) );
        }
    }


    if ( isfrequency ) {
        verbose.NextTopic ( "Frequency:" );
        verbose.Put ( "Frequency:", freqinfix );
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
    if      ( datatype == AtomTypeScalar )
        verbose.Put ( "Data type:", "Signed Data" );
    else if ( datatype == AtomTypePositive )
        verbose.Put ( "Data type:", "Positive Data" );


    verbose.NextLine ();
    if      ( filtersoptions == UsingCurrentFilters )   verbose.Put ( "Current filters:",   oldfilters. ParametersToText ( buff ) );
    else if ( filtersoptions == UsingOtherFilters   )   verbose.Put ( "Filters:",           altfilters->ParametersToText ( buff ) );
    else                                                verbose.Put ( "Filters:",           false );


    verbose.NextLine ();
    if      ( datatype == AtomTypePositive
           || ref == ReferenceAsInFile          )       verbose.Put ( "Reference:", "No reference, as in file" );
    else if ( ref == ReferenceAverage           )       verbose.Put ( "Reference:", "Average reference" );
    else if ( ref == ReferenceSingleTrack       )       verbose.Put ( "Reference:", reflist );
    else if ( ref == ReferenceMultipleTracks    )       verbose.Put ( "Reference:", reflist );
    else if ( ref == ReferenceUsingCurrent      ) {

        if      ( EEGDoc->GetReferenceType () == ReferenceAverage  )    verbose.Put ( "Current reference:", "Average reference" );
        else if ( EEGDoc->GetReferenceType () == ReferenceAsInFile )    verbose.Put ( "Current reference:", "No reference, as in file" );
        else {
            EEGDoc->GetReferenceTracks ().ToText ( buff, &ElectrodesNames, AuxiliaryTracksNames );
                                                                        verbose.Put ( "Current reference:", buff );
            }
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


    verbose.NextTopic ( "Processing Summary:" );
    (ofstream&) verbose << "Data are processed according to this sequence (& depending on user's choice):" << fastendl;
    (ofstream&) verbose                                                         << fastendl;
    (ofstream&) verbose << "    * Reading data from file"                       << fastendl;
    (ofstream&) verbose << "    * Filtering"                                    << fastendl;
    (ofstream&) verbose << "    * Applying new reference"                       << fastendl;
    (ofstream&) verbose << "    * Applying baseline correction"                 << fastendl;
    (ofstream&) verbose << "    * Rescaling data levels"                        << fastendl;
    (ofstream&) verbose << "    * Time downsampling, or computing time average" << fastendl;
    (ofstream&) verbose << "    * Computing ROIs"                               << fastendl;


    verbose.NextLine ();
    verbose.NextLine ();
    } // verbose file


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Silently restoring previous filtering state - User shouldn't see anything
if ( filtersoptions == UsingOtherFilters )

    EEGDoc->SetFilters ( &oldfilters, 0, true );


EEGDoc->SetFiltersActivated ( oldfiltersactivated, true );


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
