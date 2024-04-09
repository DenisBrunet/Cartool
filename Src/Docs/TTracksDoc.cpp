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
Reference           = ReferenceAsInFile;
ExtraContentType    = TracksContentUnknown;
CopyVirtualMemory ( ExtraContentTypeNames, TracksContentNames, NumTracksContentTypes * ContentTypeMaxChars );

NumTimeFrames       = 0;
StartingTimeFrame   = 0;
Dim2Type            = DimensionTypeUnknown;

NumElectrodes       = 0;
NumAuxElectrodes    = 0;
NumMinElectrodes    = 0;
TotalElectrodes     = 0;

SamplingFrequency   = 0;

NumSequences        = 0;
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
    if ( dynamic_cast< TFreqDoc* > (this ) )
        BuffDiss.Resize ( dynamic_cast< TFreqDoc* > (this )->GetNumFrequencies (), TotalElectrodes, 1 );
    else
        BuffDiss.Resize ( 1, TotalElectrodes, 1 );


    if ( BadTracks.IsNotAllocated () ) {
        BadTracks       = TSelection ( TotalElectrodes, OrderSorted );
        BadTracks.Reset ();
        }

    if ( AuxTracks.IsNotAllocated () ) {
        AuxTracks       = TSelection ( TotalElectrodes, OrderSorted );
        AuxTracks.Reset ();
        }

    if ( ReferenceTracks.IsNotAllocated () ) {
        ReferenceTracks = TSelection ( TotalElectrodes, OrderSorted );
        ReferenceTracks.Reset ();
        }

    
//  DBGM ( "InitMarkers", "TTracksDoc::InitDoc" );
    InitMarkers     ();

//  DBGM ( "InitLimits", "TTracksDoc::InitDoc" );
    InitLimits      ( true );

//  DBGM ( "InitAtomType", "TTracksDoc::InitDoc" );
//  InitAtomType    ();

//  DBGM ( "InitContentType", "TTracksDoc::InitDoc" );
    InitContentType ();

//  DBGM ( "InitReference", "TTracksDoc::InitDoc" );
    InitReference   ();

//  DBGM ( "InitFilters", "TTracksDoc::InitDoc" );
    InitFilters     ();

//  DBGM ( "InitDateTime", "TTracksDoc::InitDoc" );
    InitDateTime    ();

                                        // safety: check offsets are not null
    if ( HasPseudoElectrodes () && OffGfp == 0 ) {
        OffGfp              = NumElectrodes + PseudoTrackOffsetGfp;
        OffDis              = NumElectrodes + PseudoTrackOffsetDis;
        OffAvg              = NumElectrodes + PseudoTrackOffsetAvg;
        }

                                        // No more, we always use centered SD even for positive data
//                                      // check on the late, and replace GFP & Dis by RMS and Dis+
//  if ( IsAbsolute () && HasPseudoElectrodes () ) {
//      if ( StringIs ( ElectrodesNames[ OffGfp ], TrackNameGFP ) )
//          StringCopy ( ElectrodesNames[ OffGfp ], TrackNameRMS );
//
//      if ( StringIs ( ElectrodesNames[ OffDis ], TrackNameDIS ) )
//          StringCopy ( ElectrodesNames[ OffDis ], TrackNameDISPlus );
//      }

    } // ! IsOpen


/*                                      // generating random data
TRandUniform        randunif;

for ( tfi = tf1, tf = tfoffset; tfi <= tf2; tfi++, tf++ )
    for ( el = 0; el < NumElectrodes; el++ )
        buff[ el ][ tf ] = randunif ( -1.0, 1.0 );
return;
*/

/*                                        // Transpose and write to file
TExportTracks       expfile;
StringCopy      ( expfile.Filename, GetDocPath () );
PostfixFilename ( expfile.Filename, ".Transpose" );

expfile.SetAtomType ( AtomTypeScalar );

expfile.NumTracks           =  GetNumTimeFrames ();
expfile.NumTime             =  GetNumElectrodes ();

TTracks<float>      eegbuff ( GetNumElectrodes (), GetNumTimeFrames () );
                                        // get whole data set
ReadRawTracks ( 0, GetNumTimeFrames () - 1, eegbuff );

//expfile.Write ( eegbuff, false );

for ( int tf = 0; tf < expfile.NumTime; tf++ )
for ( int e  = 0; e  < expfile.NumTracks; e++  )
    expfile.Write ( eegbuff[ tf ][ e ] );
*/


return  true;
}


//----------------------------------------------------------------------------
                                        // Centralizing the whole logic for modified tracks
bool    TTracksDoc::IsDirty	()
{
return  FiltersActivated
    ||  DirtyReference ()
    ||  DirtyBadTracks ()
    ||  DirtyAuxTracks ()
    ||  DirtySamplingFrequency
    ||  TBaseDoc::IsDirty ();
}


//----------------------------------------------------------------------------
bool    TTracksDoc::Revert	( bool clear )
{
if ( ! TFileDocument::Revert ( clear ) )
    return false;

//DBGM3 ( GetDocPath (), GetAtomTypeName ( AtomTypeUseOriginal ), GetAtomTypeName ( AtomTypeUseCurrent ), "TTracksDoc::Revert" );

                                        // Reloading data
Close   ();                             // correctly closing streams

//InitDoc ();                           // should do all needed setup, but crashes later on upon closing document
Open    ( ofRead, 0 );                  // this seems to be doing the trick

                                        // kind of manual resetting, to avoid calling multiple times InitLimits and stuff
BadTracks.Reset ();

AuxTracks.Reset ();

Reference       = ReferenceAsInFile;
ReferenceTracks.Reset ();

                                        // will call InitLimits + NotifyViews + NotifyDocViews
TTracksFiltersDialog    (   CartoolMainWindow,      IDD_TRACKSFILTERS, 
                            Filters.FiltersParam,   SamplingFrequency );
Filters.FiltersParam.Reset ();
Filters.SetFromStruct ( SamplingFrequency );
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


if ( ! IsExtensionAmong ( GetDocPath (), AllCommitTracksExt ) ) {
    ShowMessage (   "Can not save with this file extension!" NewLine 
                    "Try one of these extensions " AllCommitTracksExt, 
                    "Saving Tracks", ShowMessageWarning );
    return  false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Special case for RIS data
TRisDoc*            RISDoc          = dynamic_cast< TRisDoc* > ( this );

                                        // We need to disambiguate how to write vectorial data: either as vectors or as norm
if ( RISDoc && ExtensionIs ( FILEEXT_RIS ) && IsVector ( AtomTypeUseOriginal ) )
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
expfile.NumAuxTracks        = NumAuxElectrodes;
expfile.DateTime            = DateTime;
expfile.ElectrodesNames     = *GetElectrodesNames ();
expfile.MaxValue            = GetMaxValue ();

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

                                        // works for both scalar and 3D vectorial data (x0,y0,z0,x1,y1,z1...)
expfile.Write ( EegBuff, Transposed );


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
void    TTracksDoc::InitLimits ( bool precise )
{
ResetLimits ();


long                fromtf          = 0;
long                totf            = NoMore ( (long) EegMaxPointsDisplay, NumTimeFrames ) - 1;
TDownsampling       downtf ( fromtf, totf, precise ? 100 : 7 );


TTracks<float>      EegBuff ( TotalElectrodes, 1 );

                                        // scanning for the actual data: current filters
AtomType            atomtype        = IsUnknownType ( AtomTypeUseCurrent )  ? AtomTypeScalar        : GetAtomType ( AtomTypeUseCurrent );   // at initialization, we still don't know the type, switch to scalar
PseudoTracksType    pseudotracks    = HasPseudoElectrodes ()                ? ComputePseudoTracks   : NoPseudoTracks;
ReferenceType       reference       = IsUnknownType ( AtomTypeUseCurrent )  ? ReferenceAsInFile     : ReferenceUsingCurrent;                // first time, use no reference - other times, use actual reference

TEasyStats          stat;
double              v;
int                 gfpindex        = OffGfp ? OffGfp : NumElectrodes - 1;  // a bit of a trick: if no pseudo-tracks, compute on the last track


AbsMaxValue     = 0;                    // always positive
MaxGfp          = 0;                    // always positive

                                        // scan TFs
for ( long tf = downtf.From; tf <= downtf.To; tf += downtf.Step ) {
                                        // get tracks, but allows filtering
                                        // !Reading vectorial RIS data will return the norm, which is >= 0!
    GetTracks   (   tf,         tf,
                    EegBuff,    0, 
                    atomtype,
                    pseudotracks,
                    reference
                );


    for ( int e = 0; e < NumElectrodes; e++ ) {
                                        // skip bads for min / max
        if ( BadTracks[ e ] /*|| AuxTracks[ e ]*/ )
            continue;

        v   = EegBuff ( e, 0 );

        stat.Add ( v );

        if ( fabs ( v ) > AbsMaxValue ) { 
            AbsMaxValue = fabs ( v ); 
            AbsMaxTF    = tf; 
            }
        } // for electrode


    v   = EegBuff ( gfpindex, 0 );

    if ( v > MaxGfp ) {
        MaxGfp      = v; 
        MaxGfpTF    = tf; 
        }
    }


MinValue    = stat.Min (); // NoMore  ( 0.0, stat.Min ); // was: always negative
MaxValue    = stat.Max (); // AtLeast ( 0.0, stat.Max ); // was: always positive

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

StringCopy      ( filename, GetTitle () );
RemoveExtension ( filename );
GetExtension    ( ext2, filename );

//DBGM2 ( filename, ext2, GetTitle () );


bool                lookforcutpaste         = false;    // file can be the result of "Cut & Paste", like exporting Max of GFP
bool                lookfortemplates        = false;    // file can hold templates


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Test all classes derived from TTracksDoc first

if      ( dynamic_cast< TFreqDoc* > ( this ) ) {

    TFreqDoc*       freqdoc     = dynamic_cast< TFreqDoc * > ( this );

    ContentType         = ContentTypeFreq;

    if ( freqdoc->GetOriginalSamplingFrequency () )
        Dim2Type            = freqdoc->GetSamplingFrequency () == freqdoc->GetOriginalSamplingFrequency () ? DimensionTypeTime : DimensionTypeWindow;
    else {
        Dim2Type            = freqdoc->GetSamplingFrequency () >= 250 ? DimensionTypeTime : DimensionTypeWindow;
        }

                                        // don't offset if there is a time origin set
    if ( Dim2Type == DimensionTypeWindow && ! DateTime.IsOriginTimeAvailable ()  )
        StartingTimeFrame   = 1;        // start from window "1"

//    DBGV3 ( freqdoc->GetSamplingFrequency (), freqdoc->GetOriginalSamplingFrequency (), Dim2Type == DimensionTypeWindow, "SF,  Orig SF,  IsWindow" );
    }


else if ( dynamic_cast< TRisDoc * > ( this ) ) {

    ContentType         = ContentTypeRis;
    Dim2Type            = DimensionTypeTime;

    lookforcutpaste     = true;
    lookfortemplates    = true;
    }


else if ( dynamic_cast< TSegDoc * > ( this ) ) {

    if      ( IsExtension ( GetTitle (), FILEEXT_SEG  ) ) {
        ContentType         = ContentTypeSeg;
        Dim2Type            = DimensionTypeTime;
        }
    else if ( IsExtension ( GetTitle (), FILEEXT_DATA ) ) {
        ContentType         = ContentTypeData;

        Dim2Type            = StringIs ( ext2, InfixError ) ? DimensionTypeSegmentation : DimensionTypeTime;
        }
    }


else if ( dynamic_cast< TTracksDoc * > ( this ) ) {
                                        // all derived class have been tested above
    ContentType         = ContentTypeEeg;
    Dim2Type            = DimensionTypeTime;

                                        // refine the type of EEG
    if      ( dynamic_cast< TEegCartoolEpDoc  * > ( this ) ) {
        ExtraContentType    = TracksContentERP;

        lookforcutpaste     = true;
        lookfortemplates    = true;
        }
                                        // Sef, Edf files are not quite sure...
                                        // See definition of AllCommitTracksExt for files that can be exported
    else if (  dynamic_cast< TEegCartoolSefDoc * > ( this )
            || dynamic_cast< TEegBrainVisionDoc  * > ( this )
            || IsExtension ( GetTitle (), FILEEXT_EEGEDF ) ) {
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
if ( IsExtensionAmong ( filename, SpecialFilesInfix ) ) {

    ExtraContentType    = TracksContentSpecialInfix;

    lookforcutpaste     = false;
    lookfortemplates    = false;
    }

                                        // Spectrum - overriding SpecialFilesInfix
if ( ( StringIs ( ext2, InfixSpectrum ) || StringContains ( (const char*) filename, (const char*) "." InfixSpectrum ) ) 
  && IsPositive ( AtomTypeUseOriginal ) ) {

    ExtraContentType    = TracksContentSpectrum;
    Dim2Type            = DimensionTypeFrequency;

    if ( ! DateTime.IsOriginTimeAvailable () )
        StartingTimeFrame   = 1;
    else
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

    if ( IsExtension ( GetTitle (), FILEEXT_DATA ) )
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

        if ( AuxTracks[ e ] || BadTracks[ e ] )
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
        if ( ! AuxTracks[ e ] && ! BadTracks[ e ] && TrackNonNull[ e ] )
            stats.Add ( EegAvg[ e ] );

                                        // very high difference between channels? (normal max: 1800, no DC min: 38'000)
    needshighpass   = stats.Range () > 10000;

//  stats.Show ( "Avg tracks" );
    }


                                        // 3) Biosemi always need a high pass filter
//if ( ! needshighpass ) {
//    needshighpass   = ExtensionIs ( FILEEXT_EEGBDF );


//DBGV5 ( stats.Average (), stats.SD (), stats.CoV (), stats.Range (), needshighpass, "Avg SD CoV Range -> needshighpass" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( needshighpass ) {
                                    // set a high pass filter
    TTracksFiltersDialog    (   CartoolMainWindow,      IDD_TRACKSFILTERS, 
                                Filters.FiltersParam,   SamplingFrequency );

    Filters.FiltersParam.Baseline           = BoolToCheck ( true );

    Filters.FiltersParam.ButterworthHigh    = BoolToCheck ( true );
    FloatToString   ( Filters.FiltersParam.ValueButterworthHigh, EegFilterDefaultMinFrequency );

    Filters.SetFromStruct ( SamplingFrequency );

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
char   *TTracksDoc::GetBaseFileName ( char *basefilename )  const
{
TBaseDoc::GetBaseFileName ( basefilename );


if ( basefilename == 0 )
    return  0;


if ( GetNumSessions () > 1 )
    sprintf ( StringEnd ( basefilename ), ".Session %0d", GetCurrentSession () );


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
int     TTracksDoc::GetCurrentSession ()    const
{
return NumSequences <= 1 ? 0 : CurrSequence + 1;
}


void    TTracksDoc::GoToSession ( int newsession )
{
                                        // not allowed to switch if used in a link, or no sessions within eeg
if ( NumSequences <= 1 || ! CanClose () )
    return;


int                 oldcs           = CurrSequence;
int                 newcs;


if ( IsInsideLimits ( newsession, 1, NumSequences ) )

    newcs   = newsession - 1;

else {
    char            buffdlg[ 256 ];

    sprintf ( buffdlg, "Choose a session/sequence number, between %0d and %0d:", 1, NumSequences );

    if ( ! GetValueFromUser ( buffdlg, "Session", newcs, IntegerToString ( oldcs + 1 ), CartoolMainWindow ) )
        return;

    if ( ! IsInsideLimits ( newcs, 1, NumSequences ) )
        return;

    newcs--;
    }

                                        // no change in session?
if ( newcs == oldcs )
    return;

                                        // apply changes, specific to each doc
if ( ! UpdateSession ( newcs ) )        // some sessions can be fishy
    return;

                                        // OK, this is the new session!
CurrSequence    = newcs;

                                        // do some postprocessing to update state variables
InitDateTime ();


//CommitMarkers ();
                                        // this will delete all existing markers, then load the new ones
InitMarkers ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // save some of the display states, if view exists
                                                                     // should search better for the last/current view of this doc
TTracksView*        tracksview      = dynamic_cast< TTracksView * > ( GetViewList () );


if ( tracksview ) {

    TTracksViewState    tracksviewstate;

    tracksviewstate.SaveState ( tracksview );

                                        // destroy everything
    CartoolDocManager->CloseViews ( this );
     
                                        // and create a new view while it's hot enough
    tracksview                      = new TTracksView ( *this );

    CartoolDocManager->PostEvent ( dnCreate, *tracksview );
                                        // force application to process event - now
    UpdateApplication;

                                        // restore some of the display states
    tracksviewstate.RestoreState ( tracksview );
    }


UpdateTitle ();
}

                                        // Default update with current sequence #, if any
void    TTracksDoc::UpdateTitle ()
{
if ( NumSequences <= 1 )    return;

char                buff[ 4 * KiloByte ];
char*               oldtitle        = (char *) GetTitle ();
char*               toc             = StringContains ( oldtitle, ':', StringContainsBackward );

if ( toc )  ClearString ( toc );

sprintf ( buff, "%s:%0d", oldtitle, GetCurrentSession () );

SetTitle ( buff );
}


//----------------------------------------------------------------------------
void    TTracksDoc::SetReferenceType ( ReferenceType ref, const char* tracks, const TStrings* elnames, bool verbose )
{
                                        // do we forbid re-referencing a single track?
//if ( NumMinElectrodes <= 1 && ref != ReferenceAsInFile )
//    return;


Reference   = ref;

                                        // Allow any sort of re-referencing
//CheckReference ( ReferenceType, OriginalAtomType );

                                        // init?
if ( ReferenceTracks.IsNotAllocated () ) {
    ReferenceTracks = TSelection ( TotalElectrodes, OrderSorted );
    ReferenceTracks.Reset();
    }


switch ( Reference ) {

  case ReferenceAsInFile:

    ReferenceTracks.Reset ();

    break;


  case ReferenceSingleTrack:
  case ReferenceMultipleTracks:
                                        // use the selection to get the tracks #
    ReferenceTracks.Reset ();

    if ( elnames )                      // use the names provided
        ReferenceTracks.Set ( tracks, elnames, verbose );
    else
        ReferenceTracks.Set ( tracks, GetElectrodesNames(), verbose );

                                        // remove aux tracks
//  ReferenceTracks -= AuxTracks;

    if      ( (int) ReferenceTracks == 0 ) {// cancel if no tracks
        SetReferenceType ( ReferenceAsInFile );
        return;
        }
    else if ( (int) ReferenceTracks == 1 )  // modify if only 1 track
        Reference = ReferenceSingleTrack;
    else
        Reference = ReferenceMultipleTracks;

    break;


  case ReferenceAverage:
                                        // use the selection to get the tracks #
    ReferenceTracks.Reset ();
                                        // use all real tracks
    SetRegular ( ReferenceTracks );
                                        // but remove auxs and bads
    ReferenceTracks -= AuxTracks;
    ReferenceTracks -= BadTracks;

    break;
    };


InitLimits ();
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

//    DBGM2 ( regname, auxnames, "regname, auxnames" );

    AuxTracks.Reset ();                 // force to set ONLY what is found in the registry

    if ( StringIsNot ( auxnames, TracksPreferenceNoAuxs ) )   // this string tells that no auxiliaries are to be done
        AuxTracks.Set ( auxnames, GetElectrodesNames(), false );
    }

else {                                  // scan all electrodes names
                                        // !ADD to CURRENT auxs!
    for ( int e = 0; e < NumElectrodes; e++ )
        AuxTracks.Set ( e, IsElectrodeNameAux ( ElectrodesNames[ e ] ) );
    }

                                        // security check
ClearPseudo ( AuxTracks );

                                        // set consistent new values
NumAuxElectrodes = (int) AuxTracks;
NumMinElectrodes = NumElectrodes - NumAuxElectrodes;
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

    InitLimits ();

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

    Filters.SetFilters ( SamplingFrequency );
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
    Filters.SetFromStruct ( SamplingFrequency, false );

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

    InitLimits ();

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


void    TTracksDoc::GetTracks   (   long                tf1,            long            tf2,
                                    TArray2<float>&     buff,           int             tfoffset, 
                                    AtomType            atomtype,
                                    PseudoTracksType    pseudotracks,
                                    ReferenceType       reference,      TSelection*     referencesel,
                                    TRois*              rois      
                                )
{
                                        // Checking parameters consistency

                                        // should resolve to a meaningful type
atomtype    = GetAtomType ( atomtype );


if ( pseudotracks == ComputePseudoTracks && ! ( HasPseudoElectrodes () && OffGfp != 0 ) )

    pseudotracks    = NoPseudoTracks;

                                        // falling back to current doc type
if (         reference == ReferenceUsingCurrent
     || (    reference == ReferenceSingleTrack 
          || reference == ReferenceMultipleTracks ) && referencesel == 0 ) {

    reference       =  Reference;   // current type can be anything, including single/multiple track(s)
    referencesel    = &ReferenceTracks;
    }

//                                        // To prevent average reference in case of positive data - Off now, always use the average ref
//bool                pseudonoavgref  = IsAbsolute ( atomtype );
//
//                                        // assume the caller knows what he wants, even an average ref of Positive, Vector etc...
//if ( pseudonoavgref && IsVector ( atomtype ) ) {
//    reference     = ReferenceAsInFile;
//    referencesel  = 0;
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // temporal filtering variables
int                 numtf           = tf2 - tf1 + 1;
long                mirrorl         = 0;    // how many TFs to mirror on the left
long                mirrorr         = 0;    // how many TFs to mirror on the right
long                lastvalidtf;
int                 savedtf1        = tf1;
//int               savedtf2        = tf2;
int                 savednumtf      = numtf;
int                 savedtfoffset   = tfoffset;
bool                dontfilterauxs  = ! ( FiltersActivated && CheckToBool ( Filters.FiltersParam.FilterAuxs ) );


bool                needsmargin     = FiltersActivated && Filters.HasTemporalFilter ();
                                        // can be 0 if only non-temporal filters - we have to rely on BuffDiss in that case
                                        // should be at least 1 in case of temporal, the only problematic case being Baseline alone so the previous TF is baseline-d correctly
int                 filtermargin    = needsmargin ? AtLeast ( 1, Filters.GetSafeMargin () ) : 0;

                                        // check if we need to load a separate 1 TF buffer for the first dissimilarity
bool                loadbuffdiss    = pseudotracks          // only if we compute the pseudos
                                   && filtermargin == 0     // if the buffer has not been extended due to filtering
                                   && tf1 > 0;              // previous absolute TF must exist


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // set new limits if temporal filtering
                                        // note that we can still non-temporal filter, but it doesn't need any of the margin gymnastic
if ( needsmargin ) {

    numtf          += 2 * filtermargin; // add some TFs to the borders


    tfoffset        = 0;                // place all stuff at the beginning of buffer, to have all space for filtering


    tf1            -= filtermargin;     // shift left side

    if ( tf1 < 0 ) {                    // but check its limit
        mirrorl     = -tf1;             // if out, ask for mirroring what is known
        tf1         = 0;
        tfoffset    = mirrorl;          // temporarily make some room for beginning mirror
        }


    tf2            += filtermargin;     // shift right side

    if ( tf2 >= NumTimeFrames ) {       // but check its limit
        mirrorr     = tf2 - ( NumTimeFrames - 1 );  // mirroring
        tf2         = NumTimeFrames - 1;            // new limit
        }

    lastvalidtf     = numtf - 1 - mirrorr;  // last position with real data, before the right mirroring part
    } // needsmargin


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Caller doesn't have to worry about the necessary margin, it will be adjusted here!
                                        // Note that the previous content will be lost, though this usually not a problem as when data is filtered, buffer is usually evaluated as a whole, and not a sub-part of it
                                        // buff can only grow, not shrink
buff.Resize (   max ( pseudotracks ? NumElectrodes + NumPseudoTracks 
                                   : NumElectrodes, 
                      buff.GetDim1 () ), 
                max ( numtf, 
                      buff.GetDim2 () ) );

                                        // BuffDiss dimensions could also be asserted here...

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // load buffer for the Dissimilarity of first time frame
if ( loadbuffdiss ) 
    ReadRawTracks ( tf1 - 1, tf1 - 1, BuffDiss );   // this is not optimal as it launches a separate read for 1 TF - also tf1 > 0 here
else
    BuffDiss.ResetMemory ();                        // just to be clean...


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // read raw Eeg tracks from file
                                        // tfoffset & numtf might have been modified if filtering
ReadRawTracks ( tf1, tf2, buff, tfoffset );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // apply pre-reference filters
if ( FiltersActivated ) {

                                        // temporal filtering, needs filling the mirror left and/or right parts?
    if ( needsmargin && ( mirrorl > 0 || mirrorr > 0 ) ) {

        for ( int el = 0; el < NumElectrodes; el++ ) {
                                            // skip auxs?
            if ( dontfilterauxs && AuxTracks[ el ] )
                continue;


            if ( mirrorl > 0 )              // mirror missing part, and complete with 0 if too little data

                for ( int tfl = mirrorl - 1, tfr = mirrorl + 1; tfl >= 0; tfl--, tfr++ )
                                            // don't mirror from the right mirror (if any)
                    buff ( el, tfl )   = tfr <= lastvalidtf ? buff ( el, tfr ) : 0;   // symetrical filling
    //              buff ( el, tfl )   = buff ( el, mirrorl );                        // cst filling
    //              buff ( el, tfl )   = 0;                                            // null filling


            if ( mirrorr > 0 )              // mirror missing part, and complete with 0 if too little data

                for ( int tf = 0, tfr = lastvalidtf + 1, tfl = lastvalidtf - 1; tf < mirrorr; tf++, tfr++, tfl-- )
                                            // don't mirror from the left mirror (if any)
                    buff ( el, tfr )   = tfl >= mirrorl ? buff ( el, tfl ) : 0;
            } // for el


        if ( mirrorl )
            tfoffset    = 0;            // after the left mirroring, there is no offset anymore

        } // filling mirror parts

                                        // do all filters
    Filters.ApplyFilters    (   buff,   NumElectrodes,  dontfilterauxs ? &AuxTracks : 0, 
                                numtf,  tfoffset, 
                                (FilterPrecedence) ( FilterBeforeRef | FilterTemporal | FilterTopographic ) 
                            );

                                        // don't forget case filtermargin == 0, we need to non-temporally filter this guy
    if ( loadbuffdiss )
        Filters.ApplyFilters    (   BuffDiss,   NumElectrodes,  dontfilterauxs ? &AuxTracks : 0, 
                                    1,          0, 
                                    (FilterPrecedence) ( FilterBeforeRef | FilterTopographic ) 
                                );

    } // if filtering


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // count only non-aux and non-bad electrodes
double              numt            = NonNull ( GetNumValidElectrodes () );

                                        // change any reference
                                        // tfoffset & numtf might have been modified if filtering
if ( reference != ReferenceAsInFile ) {

    double                      refvalue    = 0;
    TIteratorSelectedForward*   toseli      = reference == ReferenceSingleTrack || reference == ReferenceMultipleTracks ? new TIteratorSelectedForward ( *referencesel ) : 0;


    for ( int tf = 0, tfo = tfoffset; tf < numtf; tf++, tfo++ ) {

                                        // choose the right reference
        if      ( reference == ReferenceAverage ) {
                                        // don't use ReferenceTracks, as we can be in an overriding case
            refvalue    = 0;

            for ( int el = 0; el < NumElectrodes; el++ ) 
                if ( ! ( BadTracks[ el ] || AuxTracks[ el ] ) )
                    refvalue   += buff ( el, tfo );
                
            refvalue   /= numt;
            }

        else if ( reference == ReferenceSingleTrack ) {

            refvalue    = buff ( (*toseli)(), tfo );
            }

        else if ( reference == ReferenceMultipleTracks ) {

            refvalue    = 0;
                                        // !re-launch FirstSelected every time!
            for ( toseli->FirstSelected ( *referencesel ); (bool) (*toseli); ++(*toseli) )

                refvalue   += buff ( (*toseli)(), tfo );

            refvalue   /= NonNull ( referencesel->NumSet () );
            }

                                        // subtract reference
        for ( int el = 0; el < NumElectrodes; el++ ) 
            if ( ! AuxTracks[ el ] )   
                buff ( el, tfo ) -= refvalue;
            
        } // for tf
    

    if ( toseli != 0 )
        delete  toseli;
    } // ! ReferenceAsInFile


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // post-reference filtering, like Envelope / Thresholding, should be done at the end, after referencing
if ( FiltersActivated ) {

                                        // filter both temporal & non-temporal
    Filters.ApplyFilters    (   buff,   NumElectrodes,  dontfilterauxs ? &AuxTracks : 0, 
                                numtf,  tfoffset, 
                                (FilterPrecedence) ( FilterAfterRef | FilterTemporal | FilterTopographic ) 
                            );

                                        // don't forget case filtermargin == 0, we need to non-temporal filter this guy
    if ( loadbuffdiss )
        Filters.ApplyFilters    (   BuffDiss,   NumElectrodes,  dontfilterauxs ? &AuxTracks : 0, 
                                    1,          0, 
                                    (FilterPrecedence) ( FilterAfterRef | FilterTopographic ) 
                                );

                                        // save the precious values @ [TF - 1], if we have some margin
    if ( pseudotracks && filtermargin > 0 )
        for ( int el = 0; el < NumElectrodes; el++ )
            BuffDiss ( el, 0 ) = buff ( el, filtermargin - 1 );

                                        // shift the data back to their original place
//  for ( int el = 0; el < NumElectrodes; el++ ) 
//      MoveVirtualMemory ( buff[ el ] + savedtfoffset, buff[ el ] + filtermargin, savednumtf * buff.AtomSize () );

                                        // temporal filters -> unroll the margin
    if      ( filtermargin > 0 )
                                        // memory is contiguous, do it in 1 move - take some care about the amount of data to move to avoid out of memory access
        MoveVirtualMemory ( buff.GetArray () + savedtfoffset,
                            buff.GetArray () + filtermargin,
                            buff.MemorySize () - max ( filtermargin, savedtfoffset ) * buff.AtomSize () );

                                        // non-temporal filter or no filter -> unroll different offsets - shouldn't occur, everything has been done on-site
//  else if ( savedtfoffset != tfoffset )
//                                      // memory is contiguous, do it in 1 move - take some care about the amount of data to move to avoid out of memory access
//      MoveVirtualMemory ( buff.GetArray () + savedtfoffset,
//                          buff.GetArray () + tfoffset,
//                          buff.MemorySize () - max ( filtermargin, savedtfoffset ) * buff.AtomSize () );

                                        // restore original parameters
    if ( needsmargin ) {
        tf1         = savedtf1;
        numtf       = savednumtf;
        tfoffset    = savedtfoffset;
        }
    } // post filtering


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute pseudos
                                        // tfoffset & numtf conform to the parameters
if ( pseudotracks ) {

    double              sum;
    double              sumsqr;
    double              avg;
    double              gfp;
    double              avg2;
    double              gfp2;
    double              rescalevector   = IsVector ( atomtype ) ? 3 : 1;


    for ( int tf = 0, tfo = tfoffset, tfo1 = tfo - 1; tf < numtf; tf++, tfo++, tfo1++ ) {

                                        // don't use ReferenceTracks, as we can be in an overriding case
        sum     = 0;

        for ( int el = 0; el < NumElectrodes; el++ ) 
            if ( ! ( BadTracks[ el ] || AuxTracks[ el ] ) )
                sum     += buff ( el, tfo );
                
        buff ( OffAvg, tfo )   = avg   = sum / numt;


//      if ( pseudonoavgref )
//          avg   = 0;                  // don't center on the average

                                        // compute GFP with average ref
        sumsqr  = 0;

        for ( int el = 0; el < NumElectrodes; el++ ) 
            if ( ! ( BadTracks[ el ] || AuxTracks[ el ] ) )
                sumsqr += Square ( buff ( el, tfo ) - avg );
            
        buff ( OffGfp, tfo )   = gfp   = sqrt ( sumsqr / numt * rescalevector );

                                        // real GFP has been saved already, now avoid some 0 division
        if ( gfp == 0 )     gfp  = 1;

                                        // compute DIS
        if ( tf == 0 ) {                // first occurence of the loop, i.e. no previous data?

            if ( tf1 == 0 )             // TF 0?

                buff ( OffDis, tfo )   = 0;    // previous value doesn't exist
                
            else {                      // use local 1 TF buffer, compute again AVG, GFP, then the DIS

                sum     = 0;

                for ( int el = 0; el < NumElectrodes; el++ ) 
                    if ( ! ( BadTracks[ el ] || AuxTracks[ el ] ) )
                        sum    += BuffDiss ( el, 0 );
                
//              avg2  = pseudonoavgref ? 0 : sum / numt;
                avg2  = sum / numt;


                sumsqr  = 0;

                for ( int el = 0; el < NumElectrodes; el++ ) 
                    if ( ! ( BadTracks[ el ] || AuxTracks[ el ] ) )
                        sumsqr += Square ( BuffDiss ( el, 0 ) - avg2 );

                gfp2 = sqrt ( sumsqr / numt * rescalevector );


                if ( ! gfp2 )   gfp2 = 1;


                sumsqr  = 0;

                for ( int el = 0; el < NumElectrodes; el++ ) 
                    if ( ! ( BadTracks[ el ] || AuxTracks[ el ] ) )
                        sumsqr += Square ( ( buff     ( el, tfo  ) - avg  ) / gfp
                                         - ( BuffDiss ( el, 0    ) - avg2 ) / gfp2 );

                buff ( OffDis, tfo )   = sqrt ( sumsqr / numt * rescalevector );

                } // else tf1 != 0
            } // tf == 0

        else {                          // tf != 0 - NOT the first occurence in the loop

            sumsqr  = 0;
                                        // avg2 and gfp2 exist!
            for ( int el = 0; el < NumElectrodes; el++ ) 
                if ( ! ( BadTracks[ el ] || AuxTracks[ el ] ) )
                    sumsqr += Square ( ( buff ( el, tfo  ) - avg  ) / gfp
                                     - ( buff ( el, tfo1 ) - avg2 ) / gfp2 );

            buff ( OffDis, tfo )   = sqrt ( sumsqr / numt * rescalevector );
            }


        gfp2    = gfp;                  // store previous values for the next tf
        avg2    = avg;
        } // for tf

    } // if pseudotracks


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // optional ROIing
if ( rois )
    rois->Average ( buff, tfoffset, tfoffset + numtf - 1, FilterTypeMean );
}


//----------------------------------------------------------------------------
void    TTracksDoc::SetBadTracks ( TSelection *bad, bool notify )
{
if ( *bad != BadTracks ) {
                                        // copy new bad electrodes
    BadTracks = *bad;
                                        // update in case of average reference
    if ( Reference == ReferenceAverage )
        SetReferenceType ( Reference );
                                        // tell the good news to others
    if ( notify )
        NotifyViews ( vnNewBadSelection, (TParam2) &BadTracks ); // send only to track, not xyz
//      NotifyDocViews ( vnNewBadSelection, (TParam2) &BadTracks ); // send to xyz, which will send to all linked eegs...
    }
}


void    TTracksDoc::SetAuxTracks ( TSelection *aux, bool notify )
{
if ( *aux != AuxTracks ) {
                                        // copy new bad electrodes
    AuxTracks = *aux;
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

NotifyViews ( vnNewAuxSelection, (TParam2) &AuxTracks );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
