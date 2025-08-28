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

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "TTracksDoc.h"

#include    "MemUtil.h"

#include    "Math.Resampling.h"
#include    "Math.Stats.h"
#include    "TArray2.h"
#include    "TFilters.h"
#include    "TList.h"
#include    "TArray1.h"
#include    "Dialogs.Input.h"

#include    "TExportTracks.h"

#include    "TEegBIDMC128Doc.h"
#include    "TEegBiosemiBdfDoc.h"
#include    "TEegBioLogicDoc.h"
#include    "TEegBrainVisionDoc.h"
#include    "TEegMIDDoc.h"
#include    "TEegCartoolEpDoc.h"
#include    "TEegEgiRawDoc.h"
#include    "TEegNeuroscanCntDoc.h"
#include    "TEegNeuroscanAvgDoc.h"
#include    "TEegERPSSRdfDoc.h"
#include    "TEegCartoolSefDoc.h"
#include    "TEegMicromedTrcDoc.h"
#include    "TRisDoc.h"
#include    "TSegDoc.h"
#include    "TRoisDoc.h"
#include    "TSolutionPointsDoc.h"
#include    "TFreqDoc.h"

#include    "TTracksView.h"
#include    "TPotentialsView.h"
#include    "TInverseView.h"

using namespace std;
using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const char          TracksContentNames[ NumTracksContentTypes ][ ContentTypeMaxChars ] = 
                    {
                    "",
                    "Raw",
                    "ERP of",
                    "Frequencies of",
                    "Templates of",
                    "Collection of",
                    "P values of",
                    "Spectrum",
                    "Special Infix from",
                    };


const char          DimensionNames[ NumDimensionTypes ][ ContentTypeMaxChars ] = 
                    {
                    "Dimension Type not specified",
                    "TF",
                    "Window",
                    "Template",
                    "X",
                    "TF",                       // be silent to the user
                    "Segmentation",
                    "Frequency",
                    };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TTracksDoc::TTracksDoc ( TDocument *parent )
      : TBaseDoc ( parent ), TMarkers ( this )
{
DataOrg             = 0;

Reference           = ReferenceAsInFile;

ExtraContentType    = TracksContentUnknown;
CopyVirtualMemory ( ExtraContentTypeNames, TracksContentNames, NumTracksContentTypes * ContentTypeMaxChars );

NumTimeFrames       = 0;
StartingTimeFrame   = 0;
Dim2Type            = DimensionTypeUnknown;

NumElectrodes       = 0;
TotalElectrodes     = 0;

SamplingFrequency   = 0;

NumSequences        = 1;                // default is to have at least 1 sequence
CurrSequence        = 0;

OffGfp              = 0;
OffDis              = 0;
OffAvg              = 0;
NumInverseSolutions = 0;

FiltersActivated        = false;
DirtySamplingFrequency  = false;

DateTime            = TDateTime ();
}


//----------------------------------------------------------------------------
bool    TTracksDoc::InitDoc ()
{

if ( ! IsOpen () ) {

    if ( ! Open ( ofRead, 0 ) )
        return  true; // false;         // should be false, but here to avoid the annoying message

                                        // here, we can define this buffer, needed in InitLimits
    if ( dynamic_cast<TFreqDoc*> (this ) )
        BuffDiss.Resize ( dynamic_cast<TFreqDoc*> (this )->GetNumFrequencies (), TotalElectrodes, 1 );
    else
        BuffDiss.Resize ( 1, TotalElectrodes, 1 );


    if ( BadTracks.IsNotAllocated () )
        BadTracks       = TSelection ( TotalElectrodes, OrderSorted );

    if ( AuxTracks.IsNotAllocated () )
        AuxTracks       = TSelection ( TotalElectrodes, OrderSorted );
                                        // we take care about valid tracks
    ValidTracks     = TSelection ( TotalElectrodes, OrderSorted );
    SetValidTracks ();

    if ( ReferenceTracks.IsNotAllocated () ) {
        ReferenceTracks = TSelection ( TotalElectrodes, OrderSorted );
        ReferenceTracks.Reset ();
        }
    

    InitMarkers     ();
                                        // rewrite file if it had some inconsistencies
    CommitMarkers   ();

    InitLimits      ( InitFast );       // sets both min/max limits and AtomType

//  InitAtomType    ();                 // currently done in InitLimits

    InitContentType ();                 // needs AtomType

    InitReference   ();                 // needs AtomType

    InitFilters     ();

    InitDateTime    ();

    InitSD          ();                 // optional Standard Deviation / Error data

                                        // not for all files for the moment
//  CheckElectrodesNamesDuplicates ();
                                        // safety: check offsets are not null
    if ( HasPseudoElectrodes () && OffGfp == 0 ) {
        OffGfp              = NumElectrodes + PseudoTrackOffsetGfp;
        OffDis              = NumElectrodes + PseudoTrackOffsetDis;
        OffAvg              = NumElectrodes + PseudoTrackOffsetAvg;
        }

    } // ! IsOpen


return  true;
}


//----------------------------------------------------------------------------
                                        // Centralizing the whole logic for modified tracks
bool    TTracksDoc::IsDirty	()
{
return  FiltersActivated
    ||  DirtyReference ()
//  ||  DirtyBadTracks ()
//  ||  DirtyAuxTracks ()
    ||  DirtySamplingFrequency
    ||  TBaseDoc::IsDirty ();
}


//----------------------------------------------------------------------------
bool    TTracksDoc::Revert	( bool clear )
{
if ( ! TFileDocument::Revert ( clear ) )
    return false;

//InitDoc ();                           // should do all needed setup, but crashes later on upon closing document

                                        // Kind of manual resetting, to avoid calling multiple times InitLimits and stuff

Close   ();                             // first, close any opened stream

BadTracks.Reset ();                     // reset these guys
AuxTracks.Reset ();

Open    ( ofRead, 0 );                  // this seems to be doing the trick

SetValidTracks ();                      // update the remaining valid tracks

Reference       = ReferenceAsInFile;
ReferenceTracks.Reset ();

                                        // will call InitLimits + NotifyViews + NotifyDocViews
TTracksFiltersDialog    (   CartoolMainWindow,      IDD_TRACKSFILTERS, 
                            Filters.FiltersParam,   SamplingFrequency );
Filters.FiltersParam.Reset ();
Filters.SetFromStruct ( SamplingFrequency, Silent );
DeactivateFilters ();

                                        // finally updating the views
NotifyViews     ( vnNewBadSelection, (TParam2) &BadTracks ); // send only to track, not xyz
NotifyViews     ( vnNewAuxSelection, (TParam2) &AuxTracks ); // send only to track, not xyz
NotifyDocViews  ( vnReloadData, EV_VN_RELOADDATA_REF     );


FiltersActivated        = false;
DirtySamplingFrequency  = false;


return true;
}


//----------------------------------------------------------------------------
                                        // Common Commit for most types of Tracks
bool    TTracksDoc::Commit  ( bool force )
{
if ( ! ( IsDirty () || force ) )
    return  true;


if ( ! IsExtensionAmong ( AllCommitTracksExt ) ) {
    ShowMessage (   "Can not save with this file extension!" NewLine 
                    "Try one of these extensions " AllCommitTracksExt, 
                    "Saving Tracks", ShowMessageWarning );
    return  false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Special case for RIS data
TRisDoc*            RISDoc          = dynamic_cast<TRisDoc*> ( this );

                                        // We need to disambiguate how to write vectorial data: either as vectors or as norm
if ( RISDoc && IsExtension ( FILEEXT_RIS ) && IsVector ( AtomTypeUseOriginal ) )
                                        // we could also suggest Scalar without filtering
    if ( GetOptionFromUser (    "Saving Vectorial RIS file as:" NewLine 
                                NewLine 
                                Tab "(V)ectorial, with the original un-filtered data" NewLine 
                                Tab "(S)calar, with the current filters", 
                                "Saving Tracks", "V S", 0 ) == 'V' )
                                        // vectorial writing is done there
        return  RISDoc->CommitRis ( force );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // can save either scalar or 3D vectors (although not processed here because of how to read the data)
AtomType            atomtype        = GetAtomType ( AtomTypeUseCurrent );
int                 atomdim         = IsVector ( atomtype ) ? 3 : 1;
PseudoTracksType    pseudotracks    = NoPseudoTracks;
ReferenceType       reference       = IsVector ( atomtype ) ? ReferenceAsInFile : ReferenceUsingCurrent;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TExportTracks       expfile;

StringCopy ( expfile.Filename, GetDocPath () );

expfile.SetAtomType ( atomtype );
                                        // basic info
expfile.NumTracks           = NumElectrodes;
expfile.NumTime             = NumTimeFrames;
expfile.SamplingFrequency   = SamplingFrequency;
                                        // more info
expfile.NumAuxTracks        = GetNumAuxElectrodes ();
expfile.DateTime            = DateTime;
expfile.ElectrodesNames     = *GetElectrodesNames ();

expfile.OutputTriggers      = (ExportTriggers) ( TriggersAsTriggers | MarkersAsMarkers );
expfile.Markers             = *this;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numdim          = atomdim * NumElectrodes;
TTracks<float>      EegBuff         ( numdim, NumTimeFrames );

                                        // read whole file, with current filters and reference
GetTracks   (   0,              NumTimeFrames - 1,
                EegBuff,        0, 
                atomtype,
                pseudotracks,
                reference
            );

                                        // here we have the exact value
expfile.MaxValue            = EegBuff.GetAbsMaxValue ();    // GetAbsMaxValue ();

                                        // works for both scalar and 3D vectorial data (x0,y0,z0,x1,y1,z1...)
expfile.Write ( EegBuff, Transposed, Interactive );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We have now a new file, reload and cancel current filters + ref states
                                        // !Actually skipping this, Application will forcibly close this doc, as it might have changed type, and doc* would be erroneous - Also a safe way to open with all flags correctly set!
//Revert ();

                                        // Resetting these flags to appear non-dirty, so that Application can silently close the new file
//BadTracks.Reset ();
//AuxTracks.Reset ();
//ReferenceType           = ReferenceAsInFile;
//Filtering               =
DirtySamplingFrequency  = false;
SetDirty    ( false );


return  true;
}


//----------------------------------------------------------------------------
                                        // Estimate the type of data, like positive or signed (display scaling is each view's business)
                                        // We can not spend much time in here, as it can be called at each filtering / reference change
void    TTracksDoc::InitLimits ( InitType how )
{
ResetLimits ();

if ( NumTimeFrames == 0 )
    return;

                                        // overriding parameter for the moment
how     = InitFast;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        
long                fromtf;             // Range of time frames to scan
long                totf;
int                 numtfsamples;       // how many TF SAMPLES we wish to scan - this is different from the range to scan
int                 blocksize;          // block size to read each time
int                 numblocks;          // number of blocks to read


if ( how == InitExact ) {
                                        // Whole range of data - actually, we might lose a bit on the trailing end...
    fromtf          = 0;
    totf            = NumTimeFrames - 1;

    numtfsamples    = NumTimeFrames;

  //blocksize       = NoMore ( (int) NumTimeFrames, AtLeast ( 1000, Filters.GetSafeMargin () / 2 ) );   // way too many small blocks
    blocksize       = NoMore ( (int) NumTimeFrames, max ( NoMore  ( 50000, (int) NumTimeFrames / 50     ),
                                                          AtLeast (  1000, Filters.GetSafeMargin () / 2 ) ) );

    numblocks       = AtLeast ( 1, numtfsamples / blocksize );
    }
else { // how == InitFast 

    const TTracksView*  tracksview      = dynamic_cast<const TTracksView*> ( GetViewList () );

    if ( tracksview ) {
                                        // use current display range, which is more relevant to what user is currently seeing
        fromtf      = tracksview->GetCDP ()->GetMin ();
        totf        = tracksview->GetCDP ()->GetMax ();
        }
    else {
                                        // For very long recording, just focus on the first 15 minutes
        fromtf      = 0;
        totf        = NoMore ( (int) NumTimeFrames, Truncate ( MinutesToTimeFrame ( 15, SamplingFrequency ) ) ) - 1;
        }

    numtfsamples    = NoMore ( (int) NumTimeFrames, 50000 );
                                        // using a bigger block in case of filtering so to amortize the cost of adding margins
    blocksize       = NoMore ( (int) NumTimeFrames, AtLeast ( 1000, Filters.GetSafeMargin () / 2 ) );
                                        // estimate the number of blocks - but up to a hard limit
    numblocks       = Clip ( numtfsamples / blocksize, 1, NoMore ( 10, (int) NumTimeFrames / blocksize ) );
    }

                                        // even more checks for downsampling limits, as we will read 'blocksize' data from the last TF
int                 fromblock       =                              fromtf                       / blocksize;
int                 toblock         = AtLeast ( fromblock, (int) ( totf   - ( blocksize - 1 ) ) / blocksize );

                                        // finally, get a downsampling, in BLOCKS
TDownsampling       downblock ( fromblock, toblock, numblocks );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TTracks<float>      EegBuff ( TotalElectrodes, blocksize + 2 * Filters.GetSafeMargin () );

                                        // scanning for the actual data: current filters
AtomType            atomtype        = IsUnknownType ( AtomTypeUseCurrent )  ? AtomTypeScalar        : GetAtomType ( AtomTypeUseCurrent );   // at initialization, we still don't know the type, switch to scalar
PseudoTracksType    pseudotracks    = HasPseudoElectrodes ()                ? ComputePseudoTracks   : NoPseudoTracks;
ReferenceType       reference       = IsUnknownType ( AtomTypeUseCurrent )  ? ReferenceAsInFile     : ReferenceUsingCurrent;                // first time, use no reference - other times, use actual reference

TEasyStats          stat;
int                 gfpindex        = OffGfp ? OffGfp : NumElectrodes - 1;  // a bit of a trick: if no pseudo-tracks, compute on the last track


AbsMaxValue     = 0;                    // always positive
MaxGfp          = 0;                    // always positive

                                        // scan blocks of TFs
for ( int block = downblock.From; block <= downblock.To; block += downblock.Step ) {

                                        // reading + filtering tracks -  !Reading vectorial RIS data will return the norm, which is >= 0!
    GetTracks   (   block * blocksize,  ( block + 1 ) * blocksize - 1,
                    EegBuff,    0, 
                    atomtype,
                    pseudotracks,
                    reference
                );


    for ( int tf0 = 0, tf = block * blocksize; tf0 < blocksize; tf0++, tf++ ) {

        for ( int e = 0; e < NumElectrodes; e++ ) {
                                            // skip bads for min / max
            if ( BadTracks[ e ] /* ! ValidTracks[ e ] */ )
                continue;

            double      v   = EegBuff ( e, tf0 );

            stat.Add ( v, ThreadSafetyIgnore );

            if ( abs ( v ) > AbsMaxValue ) { 
                AbsMaxValue = abs ( v ); 
                AbsMaxTF    = tf; 
                }
            } // for electrode


        double      v   = EegBuff ( gfpindex, tf0 );

        if ( v > MaxGfp ) {
            MaxGfp      = v; 
            MaxGfpTF    = tf; 
            }
        } // for tf

    } // for block


MinValue    = stat.Min ();  // could be positive
MaxValue    = stat.Max ();  // could be negative

//stat.Show ( "InitLimits::stat" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Merged InitAtomType here, as we already have the stats at hand...
//void    TTracksDoc::InitAtomType ()

                                        // OriginalAtomType is intrinsic to the type of data and is set only once
                                        // Note that some document could have already set the OriginalAtomType by themselves
if ( IsUnknownType ( AtomTypeUseOriginal ) ) {

    if      ( stat.IsAngular () )                       SetAtomType ( AtomTypeAngular );

    else if ( MinValue >= 0                             // strict formula
//         || fabs ( MinValue ) < MaxValue / 1e2        // min value less than 1/100 of max value - but what if max is already small?
           || MinValue > -1e-6                          // absolute negative epsilon above which it is assumed positive data
                                                    )   SetAtomType ( AtomTypePositive );

    else                                                SetAtomType ( AtomTypeScalar );
    }

else {
                                        // Will NOT modify OriginalAtomType, but only CurrentAtomType
    if      ( stat.IsAngular () )                       SetAtomType ( AtomTypeAngular );

    else if ( MinValue >= 0 )                           SetAtomType ( AtomTypePositive );

    else if ( MinValue <  0 )                           SetAtomType ( AtomTypeScalar );
                                        // by safety
    else if ( IsUnknownType ( AtomTypeUseCurrent ) )    SetAtomType ( OriginalAtomType );
    } // ! IsUnknownType


//DBGM2 ( GetAtomTypeName ( AtomTypeUseOriginal ), GetAtomTypeName ( AtomTypeUseCurrent ), "OriginalAtomType / CurrentAtomType" );
}


//----------------------------------------------------------------------------
void    TTracksDoc::InitContentType ()
{
                                        // get second-to-last "extension"
TFileName           filename;
TFileName           ext2;

StringCopy      ( filename, GetDocPath () );
RemoveExtension ( filename );
GetExtension    ( ext2, filename );


bool                lookforcutpaste         = false;    // file can be the result of "Cut & Paste", like exporting Max of GFP
bool                lookfortemplates        = false;    // file can hold templates


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Test all classes derived from TTracksDoc first

if      ( dynamic_cast<TFreqDoc*> ( this ) ) {

    TFreqDoc*       freqdoc     = dynamic_cast<TFreqDoc*> ( this );

    ContentType         = ContentTypeFreq;

    if ( freqdoc->GetOriginalSamplingFrequency () )
        Dim2Type            = freqdoc->GetSamplingFrequency () == freqdoc->GetOriginalSamplingFrequency () ? DimensionTypeTime : DimensionTypeWindow;
    else {
        Dim2Type            = freqdoc->GetSamplingFrequency () >= 250 ? DimensionTypeTime : DimensionTypeWindow;
        }

                                        // don't offset if there is a time origin set
    if ( Dim2Type == DimensionTypeWindow && ! DateTime.IsOriginTimeAvailable ()  )
        StartingTimeFrame   = 1;        // start from window "1"
    }


else if ( dynamic_cast<TRisDoc*> ( this ) ) {

    ContentType         = ContentTypeRis;
    Dim2Type            = DimensionTypeTime;

    lookforcutpaste     = true;
    lookfortemplates    = true;
    }

                                        // There can be 3 different types of data hosted in a TSegDoc object
else if ( dynamic_cast<TSegDoc*> ( this ) ) {

    if      ( IsExtension ( FILEEXT_SEG  ) ) {
        ContentType         = ContentTypeSeg;
        Dim2Type            = DimensionTypeTime;
        }
    else if ( IsExtension ( FILEEXT_DATA ) ) {
        
        if ( StringIs ( ext2, InfixError ) ) {
            ContentType     = ContentTypeErrorData;
            Dim2Type        = DimensionTypeSegmentation;
            }
        else {
            ContentType     = ContentTypeData;
            Dim2Type        = DimensionTypeTime;
            }
        }
    }


else if ( dynamic_cast<TTracksDoc*> ( this ) ) {
                                        // all derived class have been tested above
    ContentType         = ContentTypeEeg;
    Dim2Type            = DimensionTypeTime;

                                        // refine the type of EEG
    if      ( dynamic_cast<TEegCartoolEpDoc*> ( this ) ) {
        ExtraContentType    = TracksContentERP;

        lookforcutpaste     = true;
        lookfortemplates    = true;
        }
                                        // Sef, Edf files are not quite sure...
                                        // See definition of AllCommitTracksExt for files that can be exported
    else if (  dynamic_cast<TEegCartoolSefDoc*>  ( this )
            || dynamic_cast<TEegBrainVisionDoc*> ( this )
            || IsExtension ( FILEEXT_EEGEDF ) ) {
                                        // big enough should be Spontaneous, else can't assert if it's an ERP, an Epoch...
        ExtraContentType    = NumTimeFrames >= EegNumPointsWideDisplay ? TracksContentEEGRecording : TracksContentUnknown;

        lookforcutpaste     = true;
        lookfortemplates    = true;
        }

    else                                // all others are real raw files
        ExtraContentType    = TracksContentEEGRecording;

    }


else {
    ContentType         = UnknownContentType;
    ExtraContentType    = TracksContentUnknown;
    Dim2Type            = DimensionTypeUnknown;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Consider some special cases

                                        // .t .p .sd etc...
if ( crtl::IsExtensionAmong ( filename, SpecialFilesInfix ) ) {

    ExtraContentType    = TracksContentSpecialInfix;

    lookforcutpaste     = false;
    lookfortemplates    = false;
    }

                                        // Spectrum - overriding SpecialFilesInfix
if ( ( StringIs ( ext2, InfixSpectrum ) || StringContains ( (const char*) filename, (const char*) "." InfixSpectrum ) ) 
  && IsPositive ( AtomTypeUseOriginal ) ) {

    ExtraContentType    = TracksContentSpectrum;
    Dim2Type            = DimensionTypeFrequency;

    //if ( ! DateTime.IsOriginTimeAvailable () )
    //    StartingTimeFrame   = 1;
    //else
        StartingTimeFrame   = 0;        // force resetting

    lookforcutpaste     = false;
    lookfortemplates    = false;
    }

                                        // P values - overriding SpecialFilesInfix
if ( ( StringIs ( ext2, InfixP       )
    || StringIs ( ext2, Infix1MinusP )
    || StringIs ( ext2, InfixLogP    )
    || StringContains ( (const char*) filename, (const char*) "." Infix1MinusP "." ) ) 
  && IsPositive ( AtomTypeUseOriginal ) ) {

    ExtraContentType    = TracksContentPValues;

    if ( IsExtension ( FILEEXT_DATA ) )
        Dim2Type        = DimensionTypeTemplate;

    lookforcutpaste     = false;
    lookfortemplates    = false;
    }

                                        // FFT Approximation - overriding SpecialFilesInfix
if ( StringIs ( ext2, InfixApproxEeg ) ) {

    ExtraContentType    = TracksContentUnknown; // not an ERP!
    Dim2Type            = DimensionTypeWindow;

    if ( Dim2Type == DimensionTypeWindow && ! DateTime.IsOriginTimeAvailable () )
        StartingTimeFrame   = 1;            // start from window "1"

    lookforcutpaste     = false;
    lookfortemplates    = false;
    }

                                        // looks like a RIS into an EEG file
if ( ExtraContentType == TracksContentERP 
  && NumElectrodes > 1024 ) {

    ExtraContentType    = TracksContentUnknown;

    lookforcutpaste     = false;
    }

                                        // recognized epoch file
if ( StringContains ( (const char*) filename, (const char*) "." InfixEpoch " " ) ) {

    ExtraContentType    = TracksContentUnknown;

    lookforcutpaste     = false;
    lookfortemplates    = false;
    }

                                        // Histogram
//if ( StringStartsWith ( ext2, InfixHistogramShort )
//  || StringStartsWith ( ext2, InfixHistogramLong  ) ) {
                                        // look inside the whole filename
if ( StringGrep ( filename, InfixHistogramGrep, GrepOptionDefaultFiles ) ) {

    ContentType         = ContentTypeHistogram;
    ExtraContentType    = TracksContentUnknown; // not an ERP!
    Dim2Type            = DimensionTypeValue;

    lookforcutpaste     = false;
    lookfortemplates    = false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                      // sort out between real ERP and a collection of Cut&Paste maps, like Max Gfp
if ( lookforepoch ) {

                                        // find the optimal scaling for first display
    double          v;
    long            totf        = NoMore ( (long) EegMaxPointsDisplay, NumTimeFrames );
    int             step;
    double          gfpmin      =  DBL_MAX;
    double          gfpmax      = -DBL_MAX;
    double          avggfp      = 0;
    int             numgfp      = 0;

    step        = totf < EegMaxPointsDisplay ? 1 + NumTimeFrames       / 101    // scan all points below 100 TF, then a little less
                                             : 1 + EegMaxPointsDisplay / 50;    // big raw files, no need to be really precise

    TTracks<float>  EegBuff ( TotalElectrodes, 1 );

                                        // scan TFs
    for ( long tf = 0; tf < totf; tf += step ) {
                                        // get raw tracks & pseudos
        GetTracks ( tf, tf, EegBuff, 0, AtomTypeUseCurrent, ComputePseudoTracks, ReferenceAsInFile );

        v           = EegBuff[ OffGfp ][ 0 ];

        gfpmin      = min ( gfpmin, v );
        gfpmax      = max ( gfpmax, v );
        avggfp     += v;
        numgfp++;
        }

    avggfp     /= numgfp ? numgfp : 1;

    double ratio       = ( avggfp - gfpmin ) / ( avggfp ? gfpmax - gfpmin : 1 );
    DBGV4 ( gfpmin, gfpmax, avggfp, ratio, "gfpmin, gfpmax, avggfp, ratio" );

//    if ( istempl )
//        ExtraContentType  = TracksContentEEGRecording;
    }*/


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // sort out between real ERP and a collection of Cut&Paste maps, like Max Gfp
if ( lookforcutpaste ) {
/*
    long            totf        = NoMore ( (long) EegMaxPointsDisplay, NumTimeFrames );
    int             step;
    double          lapl        = 0;
    double          avggfp      = 0;


    step        = totf < EegMaxPointsDisplay ? 1 + NumTimeFrames       / 101    // scan all points below 100 TF, then a little less
                                             : 1 + EegMaxPointsDisplay / 50;    // big raw files, no need to be really precise

    TTracks<float>  EegBuff ( TotalElectrodes, 3 );


    for ( long tf = 0; tf < totf - 3; tf += step ) {
                                        // get raw tracks & pseudos
        GetTracks ( tf, tf + 2, EegBuff, 0, AtomTypeUseCurrent, ComputePseudoTracks, ReferenceAsInFile );

        avggfp     += EegBuff[ OffGfp ][ 0 ];
        lapl       += fabs ( ( 2 * EegBuff[ OffGfp ][ 1 ] - EegBuff[ OffGfp ][ 0 ] - EegBuff[ OffGfp ][ 2 ] ) );
        }
                                        // Sum of Laplacians, weighted by Average GFP (more stable than Max GFP)
    lapl    = lapl / NonNull ( avggfp ) * 100;

//  DBGV2 ( lapl, lapl > 30, "SumLaplacian   IsCutPaste" );

                                        // min for Cut & Paste  = 37
                                        // max for ERP          = 22
    if ( lapl > 30 ) {
*/
                                        // if very few TFs, then a collection
    if ( NumTimeFrames <= 50 ) {
        ExtraContentType    = TracksContentCollection;
        Dim2Type            = DimensionTypeCollection;

        if ( ! DateTime.IsOriginTimeAvailable () )
            StartingTimeFrame   = 1;    // start from element "1"
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
if ( lookfortemplates && NumTimeFrames > 1 ) {

    long            totf        = NoMore ( (long) EegMaxPointsDisplay, NumTimeFrames );
    int             step;

    step        = totf < EegMaxPointsDisplay ? 1 + NumTimeFrames       / 101    // scan all points below 100 TF, then a little less
                                             : 1 + EegMaxPointsDisplay / 50;    // big raw files, no need to be really precise

    TTracks<float>  EegBuff ( TotalElectrodes, 1 );
    TGoEasyStats    stats ( NumPseudoTracks );

                                        // scan TFs
    for ( long tf = 0; tf < totf; tf += step ) {
                                        // get raw tracks & pseudos
        GetTracks   (   tf,                 tf, 
                        EegBuff,            0, 
                        AtomTypeUseCurrent, 
                        ComputePseudoTracks, 
                        ReferenceAsInFile 
                    );

        stats ( PseudoTrackOffsetGfp ).Add ( EegBuff ( OffGfp, 0 ) );
        stats ( PseudoTrackOffsetAvg ).Add ( EegBuff ( OffAvg, 0 ) );
        }


//  stats.Show ();


    if (           stats ( PseudoTrackOffsetGfp ).SNR     ()   > 1e3         // EEG Templates around 1e6, others around 4
         && fabs ( stats ( PseudoTrackOffsetAvg ).Average () ) < 1e-3        // EEG Templates near 0
      ||           stats ( PseudoTrackOffsetAvg ).SNR     ()   > 1e1  ) {    // ESI Templates around 40, others around 4

        ExtraContentType    = TracksContentTemplates;
        Dim2Type            = DimensionTypeTemplate;

        if ( ! DateTime.IsOriginTimeAvailable () )
            StartingTimeFrame   = 1;    // start from template "1"
        }

//  DBGV ( StartingTimeFrame, "Template" );
    } // lookfortemplates



//char    buff[ 256 ];
//DBGM3 ( GetContentTypeName ( buff ), GetDim2TypeName (), GetAtomTypeName ( AtomTypeUseOriginal ), "Content Type,  Dim 1 Type,  Atom Type" );
}


//----------------------------------------------------------------------------
void    TTracksDoc::InitReference ()
{
SetReferenceType (           IsErp () 
                   && ! (    IsAbsolute    ( AtomTypeUseOriginal ) 
                          || IsUnknownType ( AtomTypeUseOriginal ) ) ? ReferenceAverage : ReferenceAsInFile );
}


//----------------------------------------------------------------------------
void    TTracksDoc::InitFilters ()
{
                                        // Guess if we need a high-pass filter to visually compensate for high bias data

if ( IsAbsolute ( AtomTypeUseOriginal ) 
  || SamplingFrequency == 0
  || TimeFrameToSeconds ( NumTimeFrames, SamplingFrequency ) < 10 )     // at least 10s of data!
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TTracks<float>      EegBuff      ( NumElectrodes, 2 );
TVector<double>     EegAvg       ( NumElectrodes );
TArray1<bool>       TrackNonNull ( NumElectrodes );
                                        // pick data not from the beginning, in case of raw data, the filters take some time to setup
long                fromtf          = NumTimeFrames * 0.33;
long                totf            = min ( fromtf + 1000, NumTimeFrames - 2 );
TDownsampling       downtf ( fromtf, totf, 50 /*100*/ );


TEasyStats          stats; // ( numsamples * NumElectrodes );
bool                needshighpass;

                                        // scan TFs
stats.Reset ();

for ( long tf = downtf.From; tf <= downtf.To; tf += downtf.Step ) {

                                        // get raw tracks
    ReadRawTracks ( tf, tf + 1, EegBuff );


    for ( int e = 0; e < NumElectrodes; e++ ) {

        if ( ! ValidTracks[ e ] )
            continue;
                                        // compute the baseline of each tracks
        EegAvg[ e ]        += EegBuff ( e, 0 );
                                        // we are not interested in constant tracks
        TrackNonNull[ e ]  |= EegBuff ( e, 0 ) != EegBuff ( e, 1 );

                                        // do stats of successive relative differences, all tracks being merged at that point
        if ( EegBuff ( e, 0 ) != EegBuff ( e, 1 ) )
            stats.Add ( fabs ( RelativeDifference ( EegBuff ( e, 0 ), EegBuff ( e, 1 ) ) ) );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 1) If gobally the relative differences were low, that means the useful signal was outpaced by a baseline -> needs high pass filter
//stats.Show ( "Stats reldiff" );
                                        // Average: yes max=0.02    no min=0.10
                                        // SD:      yes max=0.10    no min=1.97
needshighpass   =   stats.IsNotEmpty ()         && stats.Average () > 0
               && ( stats.Average ()     < 0.06 && stats.SD ()      < 1.035 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // 2) If all baselines differ, that means each track was offsetted -> needs high pass filter
if ( ! needshighpass ) {
                                        // average in time, for each electrode
    EegAvg         /= downtf.NumDownData;
                                        // stats across electrodes
    stats.Reset ();

//  stats.Add ( EegAvg );

    for ( int e = 0; e < NumElectrodes; e++ )
        if ( ValidTracks[ e ] && TrackNonNull[ e ] )
            stats.Add ( EegAvg[ e ] );

                                        // very high difference between channels? (normal max: 1800, no DC min: 38'000)
    needshighpass   = stats.Range () > 10000;

//  stats.Show ( "Avg tracks" );
    }


                                        // 3) BioSemi always need a high pass filter
//if ( ! needshighpass ) {
//    needshighpass   = IsExtension ( FILEEXT_EEGBDF );


//DBGV5 ( stats.Average (), stats.SD (), stats.CoV (), stats.Range (), needshighpass, "Avg SD CoV Range -> needshighpass" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( needshighpass ) {
                                    // set a high pass filter
    TTracksFiltersDialog    (   CartoolMainWindow,      IDD_TRACKSFILTERS, 
                                Filters.FiltersParam,   SamplingFrequency );

    Filters.FiltersParam.Baseline           = BoolToCheck ( true );

    Filters.FiltersParam.ButterworthHigh    = BoolToCheck ( true );
    FloatToString   ( Filters.FiltersParam.ValueButterworthHigh, EegFilterDefaultMinFrequency );

    Filters.SetFromStruct ( SamplingFrequency, Silent );

    ActivateFilters ();
    }

}


//----------------------------------------------------------------------------
void    TTracksDoc::InitDateTime ()
{
                                        // choose the precision of the time displayed
if ( SamplingFrequency > 0 ) {          // either high SF, or a SF that gives non-integer steps (f.ex. 128 Hz)

    DateTime.MicrosecondPrecision   = IsMicrosecondPrecision ( SamplingFrequency );
//                                 || ( ( (int) ( 1000.0 / SamplingFrequency ) ) * SamplingFrequency ) != 1000.0;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Try setting offset to "origin" marker
    const MarkersList&  markers        = GetMarkersList ();

    for ( int i = 0; i < markers.Num (); i++ ) {

//      if ( ! ( markers[ i ]->Type & AllMarkerTypes ) )
//          continue;

        if ( StringIsNot ( markers[ i ]->Name, MarkerOrigin ) )
            continue;
                                        // auto switch? or only if original time not set?
                                        // set to relative time
        DateTime.RelativeTime       = true;
        DateTime.RelativeTimeOffset = - TimeFrameToMicroseconds ( markers[ i ]->From, SamplingFrequency );


//      DateTime.Show ( "InitDateTime" );
        return;
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // No overriding marker - Try to adjust offset from absolute 0, if it is within range of data (like histogram f.ex.)
                                        // Could also restrict to some types of files only
    double              tfoffsetof0         = MicrosecondsToTimeFrame ( - DateTime.GetAbsoluteTimeOffset (), SamplingFrequency );
                      // !rounding as histogram bins are centered and can have a fractional negative value, which still rounds to 0!
    if ( IsInsideLimits ( Round ( tfoffsetof0 ), 0, (int) NumTimeFrames - 1 ) ) {

        DateTime.RelativeTime       = true;
        DateTime.RelativeTimeOffset = - TimeFrameToMicroseconds ( tfoffsetof0, SamplingFrequency );
//      DateTime.ZeroInRange        = true; // ?define as a flag?
        }


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//  DateTime.Show ( "InitDateTime" );
    }
}


//----------------------------------------------------------------------------
                                        // See if a buddy file corresponding to either SE or SD exists, and load it in memory (previous way was to open it as a document)
                                        // Buddy files can now have 4 different syntaxes (first 2 are new way), instead of 2 previously (last 2 are old way)
void    TTracksDoc::InitSD ()
{
                                        // any recognized SD or SE configuration?
static TStringGrep  grepeegsdse (   "("
                                        "\\.(" InfixSE "|" InfixSD ")( [0-9]+)?" InfixAnyExtensionGrep
                                    "|"
                                        "\\.(" FILEEXT_EEGEPSE "|" FILEEXT_EEGEPSD ")$"
                                    ")", GrepOptionDefaultFiles );

                                        // any recognized Mean configuration ("Average" is not used since long)
static TStringGrep  grepeegmean ( "\\." InfixMean "( [0-9]+)?" InfixAnyExtensionGrep, GrepOptionDefaultFiles );

                                        // no SD on a SD file itself, avoiding recursion
if ( grepeegsdse.Matched ( GetDocPath () ) )
    return;


TFileName           filesd;

for ( int sdi = 0; sdi < 6; sdi++ ) {

    filesd  = GetDocPath ();
                                        // Checking for various buddy files configurations:

    if      ( sdi == 0 ) {              // file.Mean.ext    -> file.SE.ext
        if ( grepeegmean.Matched ( GetDocPath () ) )    StringReplace   ( filesd, InfixMean, InfixSE );
        else                                            continue;
        }
    else if ( sdi == 1 ) {              // file.Mean.ext    -> file.SD.ext
        if ( grepeegmean.Matched ( GetDocPath () ) )    StringReplace   ( filesd, InfixMean, InfixSD );
        else                                            continue;
        }
                                        // file.ext         -> file.SE.ext
    else if ( sdi == 2 )                                PostfixFilename ( filesd, "." InfixSE );

                                        // file.ext         -> file.SD.ext
    else if ( sdi == 3 )                                PostfixFilename ( filesd, "." InfixSD );

                                        // file.ext         -> file.EPSE
    else if ( sdi == 4 )                                ReplaceExtension( filesd, FILEEXT_EEGEPSE );

                                        // file.ext         -> file.EPSD
    else if ( sdi == 5 )                                ReplaceExtension( filesd, FILEEXT_EEGEPSD );

                                        // Trying to open file and see?
    TOpenDoc<TTracksDoc>    SDEDoc ( filesd, OpenDocHidden );

    if ( SDEDoc.IsNotOpen () )
        continue;

                                        // Is candidate compatible with current EEG?
    if (   SDEDoc->GetNumElectrodes     () == NumElectrodes 
        && SDEDoc->GetNumTimeFrames     () == NumTimeFrames 
        && SDEDoc->GetSamplingFrequency () == SamplingFrequency ) {

        SDEDoc->GetTracks   (   0,                  NumTimeFrames - 1, 
                                SDBuff,             0,
                                AtomTypeUseCurrent,
                                NoPseudoTracks,
                                ReferenceAsInFile,  0,
                                0 
                            );

        break;
        }
    } // for sdi
}


//----------------------------------------------------------------------------
                                        // Default function opens the document and retrieves the requested variable
                                        // This is a lengthy but safe approach
                                        // Another benefit is that the Open function usually tries to select the first meaningful session of the file
bool	TTracksDoc::ReadFromHeader ( const char* file, ReadFromHeaderType what, void* answer )
{
*((int *) answer)   = 0;


TOpenDoc<TTracksDoc>    EEGDoc ( file, OpenDocHidden );

if ( EEGDoc.IsNotOpen () )
    return  false;


switch ( what ) {

    case ReadNumElectrodes :
        *((int *) answer)       = EEGDoc->GetNumElectrodes ();
        return  true;

    case ReadNumAuxElectrodes :
        *((int *) answer)       = EEGDoc->GetNumAuxElectrodes ();
        return  true;

    case ReadNumTimeFrames :
        *((int *) answer)       = EEGDoc->GetNumTimeFrames ();
        return  true;

    case ReadSamplingFrequency :
        *((double *) answer)    = EEGDoc->GetSamplingFrequency ();
        return  true;
    }

return  true;
}


//----------------------------------------------------------------------------
char*   TTracksDoc::GetBaseFileName ( char* basefilename )  const
{
TBaseDoc::GetBaseFileName ( basefilename );


if ( basefilename == 0 )
    return  0;


if ( HasMultipleSessions () )
    StringAppend ( basefilename, ".Session ", IntegerToString ( GetCurrentSession () ) );


return  basefilename;
}


//----------------------------------------------------------------------------
const char* TTracksDoc::GetDim2TypeName ()  const
{
if ( Dim2Type < 0 || Dim2Type >= NumDimensionTypes )
    return  "";

return  DimensionNames[ Dim2Type ];
}


//----------------------------------------------------------------------------
                                        // parameter should be in [1..NumSequences]
void    TTracksDoc::GoToSession ( int newsession )
{
                                        // switching sessions "live" is now possible, even with linked views
if ( ! HasMultipleSessions ()                           // no multisession?
  || ! IsInsideLimits ( newsession, 1, NumSequences )   // not in proper range, or is 0?
  || newsession == CurrSequence + 1 )                   // no actual session change?

    return;

                                        // internally, we store in the range [0..NumSequences)
newsession--;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // do some postprocessing to update state variables
auto                olddatetime     = DateTime;

                                        // apply changes, specific to each doc
if ( ! UpdateSession ( newsession ) )   // some sessions can be fishy
    return;

                                        // OK, this is the new session!
CurrSequence    = newsession;


InitDateTime    ();
                                        // restore old state
DateTime.RelativeTime           = olddatetime.RelativeTime;
DateTime.RelativeTimeOffset     = olddatetime.RelativeTimeOffset;
DateTime.MicrosecondPrecision   = olddatetime.MicrosecondPrecision;

                                        // markers file should already be up-to-date
//CommitMarkers ();

                                        // this will delete all existing markers, then load the new ones
InitMarkers     ();
                                        // rewrite file if it had some inconsistencies
CommitMarkers   ();


UpdateTitle ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

NotifyDocViews  ( vnSessionUpdated );
}


//----------------------------------------------------------------------------
                                        // Default update with current sequence #, if any
void    TTracksDoc::UpdateTitle ()
{
if ( ! HasMultipleSessions () )
    return;

char                buff[ 4 * KiloByte ];
char*               oldtitle        = (char *) GetTitle ();
char*               toc             = StringContains ( oldtitle, ':', StringContainsBackward );

if ( toc )  ClearString ( toc );

StringCopy  ( buff, oldtitle, ":", IntegerToString ( GetCurrentSession () ) );

SetTitle    ( buff );
}


//----------------------------------------------------------------------------
void    TTracksDoc::SetReferenceType ( ReferenceType ref, const char* tracks, const TStrings* elnames, bool verbose )
{
                                        // do we forbid re-referencing a single track?
//if ( GetNumRegularElectrodes () <= 1 && ref != ReferenceAsInFile )
//    return;


Reference   = ref;

                                        // Allow any sort of re-referencing
//CheckReference ( ReferenceType, OriginalAtomType );


if ( ReferenceTracks.IsNotAllocated () )
    ReferenceTracks     = TSelection ( TotalElectrodes, OrderSorted );


switch ( Reference ) {

  case ReferenceAsInFile:

    ReferenceTracks.Reset ();

    break;


  case ReferenceArbitraryTracks:
                                        // use the selection to get the tracks #
    ReferenceTracks.Reset ();

    ReferenceTracks.Set ( tracks, elnames ? elnames : GetElectrodesNames (), verbose );

    if ( (int) ReferenceTracks == 0 ) {

        SetReferenceType ( ReferenceAsInFile );
        return;
        }

    break;


  case ReferenceAverage:
                                        // use the selection to get the tracks #
    ReferenceTracks.Reset ();
                                        // use all real tracks
    SetRegular ( ReferenceTracks );
                                        // but remove auxs and bads
    ReferenceTracks &= ValidTracks;

    break;
    };


InitLimits ( InitFast );
}


//----------------------------------------------------------------------------
                                        // check for spaces in names
void    TTracksDoc::ElectrodesNamesCleanUp ()
{
for ( int e = 0; e < NumElectrodes; e++ ) {
                                        // first, remove trailing spaces
    StringCleanup ( ElectrodesNames[ e ] );

                                        // second, replace inner spaces with _
//  ReplaceChars ( ElectrodesNames[ e ], " ", "_" );
    }
}


//----------------------------------------------------------------------------
                                        // Some badly defined files can have duplicate names
void    TTracksDoc::CheckElectrodesNamesDuplicates ()
{
if ( NumElectrodes <= 0 || ElectrodesNames.NumStrings () < NumElectrodes )
    return;


TSelection          duplicates ( NumElectrodes, OrderArbitrary );
TSelection          remaining  ( NumElectrodes, OrderArbitrary );
char                base[ 256 ];

remaining.Set ();

for ( int i = 0; i < NumElectrodes; i++ ) {

    if ( remaining.IsNotSelected ( i ) )
        continue;
        

    duplicates.Reset ();
        
    for ( TIteratorSelectedForward selj ( remaining ); (bool) selj; ++selj )

        if ( StringIs ( ElectrodesNames[ i ], ElectrodesNames[ selj() ] ) )

            duplicates.Set ( selj() );

                                        // more than 1 with identical name?
    if ( (int) duplicates > 1 ) {

        StringCopy  ( base, ElectrodesNames[ i ] );

        if ( StringSize ( base ) + NumIntegerDigits ( (int) duplicates ) > ElectrodeNameSize )
            StringClip  ( base, AtLeast ( 0, ( ElectrodeNameSize - 1 ) - NumIntegerDigits ( (int) duplicates ) ) );
        

        for ( TIteratorSelectedForward selj ( duplicates ); (bool) selj; ++selj )
                                        // append relative index to base
            StringCopy ( ElectrodesNames[ selj() ], base, IntegerToString ( selj.GetIndex () + 1 ) );
        } // duplicates exist


    remaining  -= duplicates;
    }
}


//----------------------------------------------------------------------------
                                        // If there are some electrodes that are aux., force them
void    TTracksDoc::InitAuxiliaries ()
{
                                        // put it here, so everybody should be checked
ElectrodesNamesCleanUp ();


if ( AuxTracks.IsNotAllocated () )
    AuxTracks   = TSelection ( TotalElectrodes, OrderSorted );


                                        // check if any registry override
char                regname [ RegistryMaxKeyLength  ];
char                auxnames[ RegistryMaxDataLength ];


if ( GetRegistryName ( UptoSubversion, regname )
  && *CartoolApplication->GetPreference ( regname, PrefAuxiliaries, auxnames ) ) {

    AuxTracks.Reset ();                 // force to set ONLY what is found in the registry

    if ( StringIsNot ( auxnames, TracksPreferenceNoAuxs ) )   // this string tells that no auxiliaries are to be done
        AuxTracks.Set ( auxnames, GetElectrodesNames(), false );
    }

else {                                  // scan all electrodes names
                                        // !ADD to CURRENT auxs!
    for ( int e = 0; e < NumElectrodes; e++ )
        AuxTracks.Set ( e, IsElectrodeNameAux ( ElectrodesNames[ e ] ) );
    }

                                        // safety check
ClearPseudo ( AuxTracks );
                                        // update the remaining valid tracks
SetValidTracks ();
}


//----------------------------------------------------------------------------
bool    TTracksDoc::SetFiltersActivated ( bool activate, bool silent )
{
bool                oldfiltersactivated = FiltersActivated;

                                        // filtering according to wishes AND at least some filters!
activate        = activate && Filters.HasAnyFilter ();

                                        // did filtering state actually change?
if ( oldfiltersactivated == activate )

    return  FiltersActivated;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set new state
FiltersActivated    = activate;


if ( ! silent ) {                       // something has changed, notify

    InitLimits      ( InitFast );

    NotifyViews     ( vnReloadData, EV_VN_RELOADDATA_FILTERS );
    NotifyDocViews  ( vnReloadData, EV_VN_RELOADDATA_EEG     );
    }


return  oldfiltersactivated;
}


//----------------------------------------------------------------------------
bool    TTracksDoc::SetFilters ( const TTracksFilters<float>* filters, const char* xyzfile, bool silent )
{
if ( filters ) {
                                        // stupid trick to set the init flag to false, and prevent overwriting the textual params
    TTracksFiltersDialog    (   CartoolMainWindow,      IDD_TRACKSFILTERS, 
                                Filters.FiltersParam,   SamplingFrequency );

    Filters = *filters;                 // just copy, assuming the filling is correct

    Filters.SetFilters ( SamplingFrequency, Silent );
    }
else {                                  // run dialog to ask the user

    if ( StringIsNotEmpty ( xyzfile ) )
        StringCopy ( Filters.FiltersParam.XyzFile, xyzfile );

                                        // Ask user
    if ( TTracksFiltersDialog   (   CartoolMainWindow,      IDD_TRACKSFILTERS, 
                                    Filters.FiltersParam,   SamplingFrequency 
                                ).Execute() == IDCANCEL )
        return false;

                                        // chew the results in a proper way
    Filters.SetFromStruct ( SamplingFrequency, Interactive );

                                        // update in case of average reference
//  if ( Filters.EnvelopeWidth && ReferenceType != ReferenceAsInFile ) {
//
//      SetReferenceType ( ReferenceAsInFile );   // reset any reference
//
//      NotifyViews ( vnReloadData, EV_VN_RELOADDATA_REF ); // update views
//      }
    }

                                        // rare case where the data have no sampling frequency, and the user set one in the dialog -> update doc
if ( SamplingFrequency <= 0 && Filters.SamplingFrequency > 0 )

    SetSamplingFrequency ( Filters.SamplingFrequency );

                                        // a priori, turn on filtering
//bool                oldfiltersactivated = ActivateFilters ();
                                        // SetFiltersActivated does not notify in this case
if ( /*oldfiltersactivated &&*/ FiltersActivated && ! silent ) {

    InitLimits      ( InitFast );

    NotifyViews     ( vnReloadData, EV_VN_RELOADDATA_FILTERS );
    NotifyDocViews  ( vnReloadData, EV_VN_RELOADDATA_EEG     );
    }

return true;
}


//----------------------------------------------------------------------------
//          ATTENTION:
// If filters are used, buff could be reallocated to allow some margins in time (especially for High Pass)
// It is NO MORE MANDATORY TO ALLOCATE SPACE IN ADVANCE, hence easing the caller life, and the code could not crash anymore due to memory access
//
// Sequence of operations:
//
// * Reads and fills ALL eeg tracks, from tf1 to tf2 (included),
//
// * Data stored in buff, starting at tfoffset if no filters,
//   or at offset 0 for filters.
//
//   Filtering operates in 2 passes: before and after new reference
//
// * Pre-filtering: Butterworth, DC, Notches...
//   Some data are added on each side of the requested TFs to avoid filter side-effect
//   These borders are normally filled with real eeg data, if available,
//   otherwise mirroring is used, and finally filling with 0 (for very short data file)
//   (in the filtering case, buffer below tfoffset and above tfoffset+tf2-tf1+1 can be overwritten)
//
// * New reference
//
// * Post-filtering: Envelope, Threshold


void    TTracksDoc::GetTracks   (   long                tf1,            long                tf2,
                                    TArray2<float>&     buff,           int                 tfoffset, 
                                    AtomType            atomtype,
                                    PseudoTracksType    pseudotracks,
                                    ReferenceType       reference,      const TSelection*   referencetracks,
                                    const TRois*        rois      
                                )
{
                                        // Checking parameters consistency

                                        // should resolve to a meaningful type
atomtype    = GetAtomType ( atomtype );


if ( pseudotracks == ComputePseudoTracks && ! ( HasPseudoElectrodes () && OffGfp != 0 ) )

    pseudotracks    = NoPseudoTracks;

                                        // resolving current reference, & checking consistency
if (    reference == ReferenceUsingCurrent
     || reference == ReferenceArbitraryTracks && referencetracks == 0 ) {

    reference       =  Reference;       // current type can be anything, including single/multiple track(s)
    referencetracks = &ReferenceTracks;
    }

                                        // When data is positive, compute the RMS instead of the GFP by skipping the re-centering
bool                gfpnotcentered      = IsAbsolute ( atomtype );

                                        // assume the caller knows what he wants, even an average ref of Positive, Vector etc...
if ( gfpnotcentered && IsVector ( atomtype ) ) {

    reference       = ReferenceAsInFile;
    referencetracks = 0;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // !Filters can exist but user could have deactivated them!
bool                doanyfilter         = FiltersActivated && Filters.HasAnyFilter ();
bool                dotemporalfilters   = doanyfilter && Filters.HasTemporalFilter ();
bool                dofilterauxs        = doanyfilter && CheckToBool ( Filters.FiltersParam.FilterAuxs );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // wrapping up all the handling of time limits & data mirroring for temporal filters
TTracksFiltersLimits<float>  timelimits;

long                numtf               = tf2 - tf1 + 1;


if ( dotemporalfilters )
                                        // Adjust the time parameters and set the time margins for temporal filtering
    timelimits.UpdateTimeRange  (   tf1,        tf2,        numtf,      tfoffset,   // !update values with new limits!
                                    AtLeast ( 1, Filters.GetSafeMargin () ),        // !make sure it has at least 1 TF so we don't need BuffDiss!
                                    NumTimeFrames
                                );

                                        // Adjust buffer to either original or expanded limits
                                        // Note that the previous content could be lost, though this usually is not a problem, as for filtered data, the buffer is usually evaluated as a whole
                                        // If this is still a problem, make sure the buffer is allocated large enough beforehand
buff.Resize (   AtLeast ( buff.GetDim1 (), NumElectrodes + ( pseudotracks ? NumPseudoTracks : 0 ) ),
                AtLeast ( buff.GetDim2 (), (int) ( tfoffset + numtf )                             )  );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We have to care for an optional 1 TF buffer used for dissimilarity computation
if ( pseudotracks ) {
    
    if      ( tf1 == 0 )                        // original or modified tf1 - Dissimilarity can not be computed 

        BuffDiss.ResetMemory ();                // by safety

    else if ( ! dotemporalfilters && tf1 > 0 )  // unmodified tf1 > 0 - read an additional time point @ tf1-1 as there is addition margin from any filter here

        ReadRawTracks ( tf1 - 1, tf1 - 1, BuffDiss );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read raw Eeg tracks from file
                                        // tfoffset & numtf might have been modified if filtering
ReadRawTracks ( tf1, tf2, buff, tfoffset );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Filters & reference
if ( doanyfilter
  || IsEffectiveReference ( reference ) ) { // !filters semantic do not include the reference for the moment!


    if ( dotemporalfilters )

        timelimits.MirrorData       (   buff,   NumElectrodes,  
                                        numtf,  tfoffset        );

                                            // do ALL filters at once, including reference, with the correct sequence
    Filters.ApplyFilters    (   buff,       NumElectrodes,  
                                numtf,      tfoffset,
                                reference,  referencetracks,    &ValidTracks,   dofilterauxs ? 0 : &AuxTracks,
                                doanyfilter ? AllFilters : NoFilter
                            );


    if ( dotemporalfilters )

        timelimits.RestoreTimeRange (   buff,       NumElectrodes,  
                                        tf1,        tf2,        numtf,      tfoffset,       // !restore original time limits!
                                        pseudotracks && dotemporalfilters ? &BuffDiss : 0   // recover data @ tf1-1
                                    );


    if ( pseudotracks && ! dotemporalfilters && tf1 > 0 )

        Filters.ApplyFilters    (   BuffDiss,   NumElectrodes,
                                    1,          0,
                                    reference,  referencetracks,    &ValidTracks,   dofilterauxs ? 0 : &AuxTracks,
                                    doanyfilter ? FilterNonTemporal : NoFilter              // just to make sure we skip the temporal filters
                                );
    } // filter or reference


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Compute pseudo-tracks Avg, Gfp and Dis
if ( pseudotracks ) {
                                        // count only non-aux and non-bad electrodes
    double              numt            = NonNull ( GetNumValidElectrodes () );
    double              rescalevector   = IsVector ( atomtype ) ? 3 : 1;
    double              avgbefore       = 0;
    double              gfpbefore       = 1;


    auto    ComputeAvg  = [ this, &numt ] ( TArray2<float>& buff, int tfo ) {

        double      sum     = 0;
                                        // don't use ReferenceTracks, as we can be in an overriding case
        for ( int el = 0; el < NumElectrodes; el++ ) 
            if ( ValidTracks[ el ] )
                sum    += buff ( el, tfo );
                
        return  sum / numt;
        };

    auto    ComputeGfp  = [ this, &numt, &rescalevector ] ( TArray2<float>& buff, int tfo, double avg ) {

        double      sumsqr  = 0;

        for ( int el = 0; el < NumElectrodes; el++ ) 
            if ( ValidTracks[ el ] )
                sumsqr += Square ( buff ( el, tfo ) - avg );
            
        return  sqrt ( sumsqr / numt * rescalevector );
        };

    auto    ComputeDis  = [ this, &numt, &rescalevector ] ( TArray2<float>& buff1, int tfo1, double avg1, double gfp1, TArray2<float>& buff2, int tfo2, double avg2, double gfp2 ) {

        double      sumsqr  = 0;

        for ( int el = 0; el < NumElectrodes; el++ ) 
            if ( ValidTracks[ el ] )
                sumsqr += Square (   ( buff1 ( el, tfo1 ) - avg1 ) / gfp1
                                   - ( buff2 ( el, tfo2 ) - avg2 ) / gfp2 );
            
        return  sqrt ( sumsqr / numt * rescalevector );
        };


    for ( int tf = 0, tfo = tfoffset; tf < numtf; tf++, tfo++ ) {

        double          avg     = ComputeAvg ( buff, tfo );

        buff ( OffAvg, tfo )    = avg;  // saving real average


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if ( gfpnotcentered )
            avg     = 0;                // resetting avg? GFP becomes RMS

        double          gfp     = ComputeGfp ( buff, tfo, avg );

        buff ( OffGfp, tfo )    = gfp;  // saving real GFP / RMS


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        NonNulled ( gfp );              // just to prevent division by zero


        if ( tf == 0 ) {                // first occurence in the loop, which means we have to handle some missing data

            if ( tf1 == 0 ) {           // first actual tf: dissimilarity can not be computed, as there is no previous data to compare from

                buff ( OffDis, tfo )   = 0;
                }
            else {                      // tf1 > 0 - previous data exist and is to be found in our local 1 TF buffer

                double      avgbuffdiss     = gfpnotcentered ? 0 : ComputeAvg ( BuffDiss, 0 );

                double      gfpbuffdiss     = NonNull ( ComputeGfp ( BuffDiss, 0, avgbuffdiss ) );

                buff ( OffDis, tfo )        = ComputeDis    (   buff,       tfo,    avg,            gfp,
                                                                BuffDiss,   0,      avgbuffdiss,    gfpbuffdiss );
                } // else tf1 > 0

            } // tf == 0

        else {                          // tf > 0 - NOT the first occurence in the loop, and avgbefore and gfpbefore exist

            buff ( OffDis, tfo )    = ComputeDis    (   buff,   tfo,        avg,        gfp,
                                                        buff,   tfo - 1,    avgbefore,  gfpbefore );
            }


        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // store current values for the next tf
        avgbefore   = avg;
        gfpbefore   = gfp;
        } // for tf

    } // if pseudotracks


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // optional ROI-ing
if ( rois )

    rois->Average ( buff, tfoffset, tfoffset + numtf - 1, FilterTypeMean );
}


//----------------------------------------------------------------------------
void    TTracksDoc::GetStandardDeviation ( long tf1, long tf2, TArray2<float>& buff, int tfoffset, const TRois* rois )   const
{
if ( ! HasStandardDeviation () ) {
    buff.ResetMemory ();
    return;
    }


long                numtf           = tf2 - tf1 + 1;

                                        // Mimick GetTracks by also adjusting the buffer size if needed
buff.Resize (   AtLeast ( buff.GetDim1 (), NumElectrodes              ),
                AtLeast ( buff.GetDim2 (), (int) ( tfoffset + numtf ) )  );

                                        // All SD data is already loaded into our own buffer, just copy the requested bits
for ( int el = 0; el < NumElectrodes; el++ )

    CopyVirtualMemory ( buff[ el ] + tfoffset, SDBuff[ el ] + tf1, numtf * SDBuff.AtomSize () );

                                        // Mimick GetTracks by also computing an optional ROIs
if ( rois )

    rois->Average ( buff, tfoffset, tfoffset + numtf - 1, FilterTypeMean );
}


//----------------------------------------------------------------------------
void    TTracksDoc::SetBadTracks ( TSelection *bad, bool notify )
{
if ( *bad != BadTracks ) {
                                        // copy new bad electrodes
    BadTracks = *bad;
                                        // update the remaining valid tracks
    SetValidTracks ();
                                        // update in case of average reference
    if ( Reference == ReferenceAverage )
        SetReferenceType ( Reference );
                                        // tell the good news to others
    if ( notify )
        NotifyViews    ( vnNewBadSelection, (TParam2) &BadTracks ); // send only to track, not xyz
//      NotifyDocViews ( vnNewBadSelection, (TParam2) &BadTracks ); // send to xyz, which will send to all linked eegs...
    }
}


//----------------------------------------------------------------------------
void    TTracksDoc::SetAuxTracks ( TSelection *aux, bool notify )
{
if ( *aux != AuxTracks ) {
                                        // copy new bad electrodes
    AuxTracks = *aux;
                                        // update the remaining valid tracks
    SetValidTracks ();
                                        // update in case of average reference
    if ( Reference == ReferenceAverage )
        SetReferenceType ( Reference );
                                        // tell the good news to others
    if ( notify )
        NotifyViews ( vnNewAuxSelection, (TParam2) &AuxTracks ); // send only to track, not xyz

                                        // override is save as preference
    char                regname[ RegistryMaxKeyLength ];

    if ( GetRegistryName ( UptoSubversion, regname ) ) {

        if ( ! GetAnswerFromUser ( "Do you want to permanently\noverride the default auxiliaries?", "Setting auxiliaries" ) )
            return;

        char                auxnames[ RegistryMaxDataLength ];

        AuxTracks.ToText ( auxnames, GetElectrodesNames (), AuxiliaryTracksNames );

        if ( StringIsEmpty ( auxnames ) )
            StringCopy ( auxnames, TracksPreferenceNoAuxs );    // set a special string, to avoid being empty

        CartoolApplication->SetPreference ( regname, PrefAuxiliaries, auxnames );

        ShowMessage ( "Close and restart the application to apply the changes.", "Setting auxiliaries", ShowMessageWarning );
        }
    }
}


void    TTracksDoc::ResetAuxTracks ()
{
                                        // override is save as preference
char                regname [ RegistryMaxKeyLength  ];
char                auxnames[ RegistryMaxDataLength ];


if ( GetRegistryName ( UptoSubversion, regname )
  && *CartoolApplication->GetPreference ( regname, PrefAuxiliaries, auxnames ) ) {

    if ( ! GetAnswerFromUser ( "Are you sure you want to reset to the default auxiliaries?", "Resetting auxiliaries" ) )
        return;

    CartoolApplication->DeletePreference ( regname, PrefAuxiliaries );

    ShowMessage ( "Close and restart the application to apply the changes", "Resetting auxiliaries", ShowMessageWarning );
    }

else
    AuxTracks.Reset ();


InitAuxiliaries ();
                                        // update the remaining valid tracks
SetValidTracks ();

NotifyViews ( vnNewAuxSelection, (TParam2) &AuxTracks );
}


//----------------------------------------------------------------------------
                                        // Valid tracks are non-bads, non-auxiliaries regular tracks
void    TTracksDoc::SetValidTracks ()
{
ValidTracks.Set (); 

ClearBads   ( ValidTracks ); 
ClearAuxs   ( ValidTracks ); 
ClearPseudo ( ValidTracks ); 
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
