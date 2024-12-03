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

#include    "MemUtil.h"

#include    "Files.Extensions.h"

#include    "Strings.Utils.h"
#include    "Files.Stream.h"

#include    "TSetArray2.h"
#include    "TMaps.h"

#include    "TEegCartoolSefDoc.h"
#include    "TEegBiosemiBdfDoc.h"
#include    "TRisDoc.h"
#include    "TVolumeAnalyzeDoc.h"
#include    "TFreqDoc.h"
#include    "TFreqCartoolDoc.h"         // TFreqHeader

#include    "TExportTracks.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------

const ExportTracksTypeSpec  ExportTracksTypes[ NumExportTracksType ] = 
                {
                {   ExportTracksUnknown,    "",                 false,      false   },
                {   ExportTracksEp,         FILEEXT_EEGEP,      false,      false   },
                {   ExportTracksEph,        FILEEXT_EEGEPH,     false,      false   },
                {   ExportTracksData,       FILEEXT_DATA,       false,      false   },
                {   ExportTracksSeg,        FILEEXT_SEG,        false,      false   },
                {   ExportTracksSef,        FILEEXT_EEGSEF,     true,       false   },
                {   ExportTracksBv,         FILEEXT_EEGBV,      true,       true    },
                {   ExportTracksEdf,        FILEEXT_EEGEDF,     true,       true    },
                {   ExportTracksRis,        FILEEXT_RIS,        true,       false   },
                {   ExportTracksFreq,       FILEEXT_FREQ,       true,       false   },
                };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TExportTracks::TExportTracks ()
{
of                  = 0;

Reset ();
}


        TExportTracks::~TExportTracks ()
{
End ();
}


void    TExportTracks::Reset ()
{
End ();


TDataFormat::Reset ();


Filename.Reset ();
Type                = ExportTracksUnknown;

NumTracks           = 0;
NumTime             = 0;
SamplingFrequency   = 0;

NumFiles            = 1;
VariableNames.Reset ();
NumClusters         = 0;

NumAuxTracks        = 0;
DateTime            = TDateTime ();
//SelTracks           = 0;
//AuxTracks           = 0;
ElectrodesNames.Reset ();

MaxValue            = 0;
OutputTriggers      = NoTriggersNoMarkers;
Markers.ResetMarkers ();
NumTags             = 0;
TimeMin             = 0;
TimeMax             = 0;

NumFrequencies      = 0;
ClearString ( FreqTypeName, MaxCharFreqType );
BlockFrequency      = 0;
FrequencyNames.Reset ();


CurrentPositionTrack= 0;
CurrentPositionTime = 0;
DoneBegin           = false;
EndOfHeader         = 0;
}


//----------------------------------------------------------------------------
                                        // copy constructor
        TExportTracks::TExportTracks ( const TExportTracks &op )
      : TDataFormat ( op )
{
of                  = op.of;            // or 0?

Filename            = op.Filename;
Type                = op.Type;

NumTracks           = op.NumTracks;
NumTime             = op.NumTime;
SamplingFrequency   = op.SamplingFrequency;

NumFiles            = op.NumFiles;
VariableNames       = op.VariableNames;
NumClusters         = op.NumClusters;

NumAuxTracks        = op.NumAuxTracks;
DateTime            = op.DateTime;
SelTracks           = op.SelTracks;
AuxTracks           = op.AuxTracks;
ElectrodesNames     = op.ElectrodesNames;

MaxValue            = op.MaxValue;
OutputTriggers      = op.OutputTriggers;
Markers             = op.Markers;
NumTags             = op.NumTags;
TimeMin             = op.TimeMin;
TimeMax             = op.TimeMax;

NumFrequencies      = op.NumFrequencies;
StringCopy ( FreqTypeName, op.FreqTypeName, MaxCharFreqType - 1 );
BlockFrequency      = op.BlockFrequency;
FrequencyNames      = op.FrequencyNames;

CurrentPositionTrack= op.CurrentPositionTrack;
CurrentPositionTime = op.CurrentPositionTime;
DoneBegin           = op.DoneBegin;
EndOfHeader         = op.EndOfHeader;
}

                                        // assignation operator
TExportTracks&  TExportTracks::operator= ( const TExportTracks &op2 )
{
if ( &op2 == this )
    return  *this;


TDataFormat::operator= ( op2 );


of                  = op2.of;            // or 0?

Filename            = op2.Filename;
Type                = op2.Type;

NumTracks           = op2.NumTracks;
NumTime             = op2.NumTime;
SamplingFrequency   = op2.SamplingFrequency;

NumFiles            = op2.NumFiles;
VariableNames       = op2.VariableNames;
NumClusters         = op2.NumClusters;

NumAuxTracks        = op2.NumAuxTracks;
DateTime            = op2.DateTime;
SelTracks           = op2.SelTracks;
AuxTracks           = op2.AuxTracks;
ElectrodesNames     = op2.ElectrodesNames;

MaxValue            = op2.MaxValue;
OutputTriggers      = op2.OutputTriggers;
Markers             = op2.Markers;
NumTags             = op2.NumTags;
TimeMin             = op2.TimeMin;
TimeMax             = op2.TimeMax;

NumFrequencies      = op2.NumFrequencies;
StringCopy ( FreqTypeName, op2.FreqTypeName, MaxCharFreqType - 1 );
BlockFrequency      = op2.BlockFrequency;
FrequencyNames      = op2.FrequencyNames;

CurrentPositionTrack= op2.CurrentPositionTrack;
CurrentPositionTime = op2.CurrentPositionTime;
DoneBegin           = op2.DoneBegin;
EndOfHeader         = op2.EndOfHeader;


return  *this;
}


//----------------------------------------------------------------------------
                                        // set as many parameters as possible
                                        // given an optional extension, and 1 or 2 files
void    TExportTracks::CloneParameters ( const char* ext, const char* file1, const char* file2 )
{
Reset ();


if ( StringIsNotEmpty ( ext ) ) {

    Type   = GetExportType ( ext, true );
                                        // file extension not recognized
    if ( Type == ExportTracksUnknown )
                                        // provide a default at that point
        Type   = ExportTracksDefault;
    }
else
                                        // or let an auto choice
    Type   = ExportTracksUnknown;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TOpenDoc< TTracksDoc >  eegdoc  ( file1, OpenDocHidden );
TRisDoc*                risdoc      =         dynamic_cast< TRisDoc*  > ( (TTracksDoc*) eegdoc );
TFreqDoc*               freqdoc     =         dynamic_cast< TFreqDoc* > ( (TTracksDoc*) eegdoc );

                                        // second file is optional
TOpenDoc< TTracksDoc >  eegdoc2 ( file2 ? file2 : 0, OpenDocHidden );
TRisDoc*                risdoc2     = file2 ? dynamic_cast< TRisDoc*  > ( (TTracksDoc*) eegdoc2 ) : 0;
TFreqDoc*               freqdoc2    = file2 ? dynamic_cast< TFreqDoc* > ( (TTracksDoc*) eegdoc2 ) : 0;


if ( eegdoc == 0 )
    return;

                                        // set info - could also check on both files...
NumTracks           =  eegdoc->GetNumElectrodes     ();
NumAuxTracks        =  eegdoc->GetNumAuxElectrodes  ();
NumTime             =  eegdoc->GetNumTimeFrames     ();
SamplingFrequency   =  eegdoc->GetSamplingFrequency ();
DateTime            =  eegdoc->DateTime;
AuxTracks           =  eegdoc->GetAuxTracks ();
ElectrodesNames     = *eegdoc->GetElectrodesNames ();

TimeMin             =  0;               // select the whole time line
TimeMax             =  NumTime - 1;
                                        // for EDF
MaxValue            =  eegdoc->GetAbsMaxValue ();

                                        // fill Freq data, if any
if ( freqdoc ) {
    NumFrequencies      = freqdoc->GetNumFrequencies ();
    BlockFrequency      = freqdoc->GetSamplingFrequency ();
    SamplingFrequency   = freqdoc->GetOriginalSamplingFrequency ();
    StringCopy ( FreqTypeName, FrequencyAnalysisNames[ freqdoc->GetFreqType () ] );
    FrequencyNames      = *freqdoc->GetFrequenciesNames ();
    }

                                        // if not set, deduce a logical file type
if      ( Type == ExportTracksUnknown )
                                        // both files should be ris to elicit ris
    if      ( risdoc  && ( ! file2 || risdoc2  ) )      Type    = ExportTracksRis;
    else if ( freqdoc && ( ! file2 || freqdoc2 ) )      Type    = ExportTracksFreq;
    else                                                Type    = ExportTracksSef;

                                        // if output not a ris, or if not all cast to ris, or if not all are vectorial ris
if      ( Type == ExportTracksRis )

    if ( risdoc && risdoc->IsVector ( AtomTypeUseOriginal ) && ( ! risdoc2 || risdoc2->IsVector ( AtomTypeUseOriginal ) ) )
        SetAtomType ( AtomTypeVector );
    else
        SetAtomType ( AtomTypeScalar );

else if ( Type == ExportTracksFreq )

    if ( freqdoc && freqdoc->IsComplex ( AtomTypeUseOriginal ) && ( ! freqdoc2 || freqdoc2->IsComplex ( AtomTypeUseOriginal ) ) )
        SetAtomType ( AtomTypeComplex );
    else
        SetAtomType ( AtomTypeScalar );

else
        SetAtomType ( AtomTypeScalar );


eegdoc .Close ();
eegdoc2.Close ();
}


//----------------------------------------------------------------------------
                                        // pre-fill the (big) file for better performance
void    TExportTracks::PreFillFile ()
{
if ( ! DoneBegin )
    return;                             // this should called right after the header has been written


if ( IsFileTextual () )
    return;                             // this is only for binary files


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

of->seekp ( EndOfHeader, ios::beg );

                                        // compute the correct size to be filled after the header
size_t              sizetoreset;

if      ( Type == ExportTracksRis   )   sizetoreset     = NumTracks * NumTime * ( IsVector ( AtomTypeUseOriginal ) ? 3 : 1 ) * sizeof ( float );
else if ( Type == ExportTracksSef   )   sizetoreset     = NumTracks * NumTime * sizeof ( float );
else if ( Type == ExportTracksBv    )   sizetoreset     = NumTracks * NumTime * sizeof ( float );   // BVTypeFloat32
                                    //  sizetoreset     = NumTracks * NumTime * sizeof ( short );   // OK only for equal sampling frequencies across channels
else if ( Type == ExportTracksEdf   )   sizetoreset     = ( ( NumTime + EdfTrailingTF ) / EdfTfPerRec ) * EdfBlockSize;
else if ( Type == ExportTracksFreq  )   sizetoreset     = NumTracks * NumTime * NumFrequencies * ( IsComplex ( AtomTypeUseOriginal ) ? sizeof ( complex<float> ) : sizeof ( float ) );
else                                    sizetoreset     = 0;


AllocateFileSpace   ( of, sizetoreset, FILE_CURRENT );


//of->flush ();
                                        // go back to data origin
of->seekp ( EndOfHeader, ios::beg );
}


//----------------------------------------------------------------------------
                                        // Compute the correct position for EDF files
LONGLONG    TExportTracks::EDFseekp ( long tf, long e )
{
return      EdfDataOrg 
        + ( tf / EdfTfPerRec ) * EdfBlockSize                               // block position
        + ( e  * EdfTfPerRec + ( tf % EdfTfPerRec ) ) * sizeof ( short );   // position within block
}


//----------------------------------------------------------------------------
void    TExportTracks::End ()
{
                                        // Some last-minute processing needed for EDF: trailing space, triggers
if ( IsOpen () && Type == ExportTracksEdf ) {
                                        // 1) pad the last block with 0
    if ( EdfTrailingTF ) {

        TIteratorSelectedForward    seli ( SelTracks );

        for ( int i = SelTracks.IsNotAllocated () ? 0 : seli(), j = 0; i >= 0 && ( SelTracks.IsAllocated () || i < NumTracks ); i = SelTracks.IsNotAllocated () ? i + 1 : ++seli, j++ ) {

            for ( long tfi = NumTime; tfi < NumTime + EdfTrailingTF; tfi++ ) {
                                        // 0, after conversion
                EdfValue    = (short) ( EdfDigitalMin - EdfPhysicalMin * EdfRatio );

                of->seekp ( EDFseekp ( tfi, j ) );

                of->write ( (char *) &EdfValue, sizeof ( EdfValue ) );
                }
            }
                                        // and again the status
        for ( long tfi = NumTime; tfi < NumTime + EdfTrailingTF; tfi++ ) {
                                        // resetting trigger line
            EdfValue    = (short) 0;

            of->seekp ( EDFseekp ( tfi, NumTracks ) );

            of->write ( (char *) &EdfValue, sizeof ( EdfValue ) );
            }
        } // EdfTrailingTF

                                        // 2) write the triggers
    WriteTriggers ();
    } // if EDF


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

CloseStream ();

DoneBegin           = false;
}


//----------------------------------------------------------------------------
                                        // Actually opening the file
bool    TExportTracks::OpenStream ( bool reopen )
{
if ( IsOpen () )
    return  true;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // be careful with the opening mode
ios::openmode       mode        = (ios::openmode )  (                       ios::out            // always output...
//                                                  | ( reopen          ?   ios::app    : 0 )   // !not working for streams!
                                                    | ( IsFileBinary () ?   ios::binary : 0 )   // some files are binary
                                                    );

                                        // massage that file name - avoiding update, though
TFileName           filename ( Filename, (TFilenameFlags) ( TFilenameExtendedPath | TFilenameSibling /*| TFilenameNoOverwrite*/ ) );

of          = new ofstream ( filename, mode );

bool                isok        = of->good ();

if ( ! isok )
    CloseStream ();

return  isok;
}


//----------------------------------------------------------------------------
                                        // Actually closing the file
void    TExportTracks::CloseStream ()
{
if ( ! IsOpen () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

of->close ();

delete  of;

of  = 0;
}


//----------------------------------------------------------------------------
                                        // Deduce output format from provided file name
ExportTracksType    GetExportType ( const char* file, bool isext )
{
if ( StringIsEmpty ( file ) )
    return  ExportTracksUnknown;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char            ext[ 256 ];

if ( isext )    StringCopy      ( ext, file );
else            GetExtension    ( ext, file );

                                        // Resolve special cases right here:
if      ( IsStringAmong ( ext, FILEEXT_EEGEPSD " " FILEEXT_EEGEPSE ) )
                                        // EPSD and EPSE == EPH
    StringCopy ( ext, FILEEXT_EEGEPH );

else if ( IsStringAmong ( ext, FILEEXT_TXT ) )
                                        // txt == ep
    StringCopy ( ext, FILEEXT_EEGEP );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Loop through our known output formats
for ( int i = ExportTracksUnknown + 1; i < NumExportTracksType; i++ )

    if ( StringIs ( ExportTracksTypes[ i ].Ext, ext ) ) {

//      DBGV ( i, ExportTracksTypes[ i ].Ext );

        return  ExportTracksTypes[ i ].Code;
        }

                                        // We don't make any choice at that point
return  ExportTracksUnknown;
}


//----------------------------------------------------------------------------
void    TExportTracks::Begin ( bool dummyheader )
{
End ();                                 // close any open file


if ( Filename.IsEmpty () ) {            // no filename is kind of a problem...
    Reset ();
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // no Type already provided / set? get it from the filename
if ( Type == ExportTracksUnknown ) {

    Type   = GetExportType ( Filename, false );
                                        // file extension not recognized
    if ( Type == ExportTracksUnknown )
                                        // provide a default at that point
        Type   = ExportTracksDefault;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( ! OpenStream () ) {

    ShowMessage (   "There seems to be a problem while trying to write this file." NewLine 
                    "Maybe it is write-protected, or the disk is full?", 
                    ToFileName ( Filename ), ShowMessageWarning );
    Reset ();
    return;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

WriteHeader ();


if ( dummyheader ) {                    // enough to overwrite this case

    if ( Type == ExportTracksEph ) {

        of->seekp ( 0, ios::beg );
                                        // writing as much spaces as the real header will need later on
        *of << string ( ' ', WidthInt32 + WidthInt32 + WidthFloat64 ) << fastendl;
        }
                                        // not implemented
//  else if ( Type == ExportTracksData
//         || Type == ExportTracksSeg  ) {}
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

CurrentPositionTrack    = 0;
CurrentPositionTime     = 0;
DoneBegin               = true;


if ( ! dummyheader )                    // a dummy header means unknown size, so pre-allocation is not feasable

    PreFillFile ();
}


//----------------------------------------------------------------------------
const char*     TExportTracks::GetElectrodeName ( int i, char *name, int maxlen )
{
if ( ElectrodesNames.IsNotEmpty () && i < ElectrodesNames.NumStrings () )
                                        // get the provided name
    StringCopy      ( name, ElectrodesNames[ i ] );

else {                                  // generate a standard name
    StringCopy      ( name, Type == ExportTracksRis ? "sp" : "e" );
    IntegerToString ( StringEnd ( name ), i + 1 /*, NumIntegerDigits ( NumTracks )*/ );
    }

                                        // finally, check length
StringShrink        ( name, name, maxlen );


return  name;
}


//----------------------------------------------------------------------------
const char*     TExportTracks::GetFrequencyName ( int i, char *name, int maxlen )
{
if ( FrequencyNames.IsNotEmpty () && i < FrequencyNames.NumStrings () )
                                        // get the provided name
    StringCopy      ( name, FrequencyNames[ i ] );

else {                                  // generate a standard name
    StringCopy      ( name, "Freq" );
    IntegerToString ( StringEnd ( name ), i + 1, NumIntegerDigits ( NumFrequencies ) );
    }

                                        // finally, check length
StringShrink        ( name, name, maxlen );


return  name;
}


//----------------------------------------------------------------------------
                                        // Possible only on a few types of output files
                                        // If not possible, fallback will be to output triggers as markers (maybe add a controlling option on that?)
void    TExportTracks::WriteTriggers ()
{
if ( Markers.IsEmpty () 
  || OutputTriggers == NoTriggersNoMarkers 
  || ! HasNativeTriggers () )

    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

MarkerType          markertype      = MarkerTypeUnknown;

                                        // Common case
if ( IsFlag ( OutputTriggers, TriggersAsTriggers) )    // Common case

    SetFlags    ( markertype, MarkerTypeTrigger );


if ( IsFlag ( OutputTriggers, MarkersAsTriggers ) )    // Writing markers as triggers

    SetFlags    ( markertype, MarkerTypeMarker );


if ( markertype == MarkerTypeUnknown )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Done at the end just before closing file
if      ( Type == ExportTracksEdf ) {
                                        // check if file is still open
    bool                isopen          = IsOpen ();
    TFileStream         ofs;
  
    if ( ! isopen )
                                        // then locally re-open file
        if ( ! ofs.Open    ( Filename, FileStreamUpdate ) )
            return;                     // hu-hu...

                                        // Here we have the EEG file open
    const TMarker*      tomarker;

    for ( int ti = 0; ti < Markers.GetNumMarkers (); ti++ ) {

        tomarker   = Markers[ ti ];

        if ( ! IsFlag ( tomarker->Type, markertype ) )
            continue;


        if ( tomarker->From >= NumTime )
            break;

                                        // keep only those completely inside
        if ( ! IsInsideLimits ( tomarker->From, tomarker->To, (long) 0, (long) NumTime - 1 ) ) 
            continue;

                                        // try to convert the name to integer
        EdfValue    = (short) StringToInteger ( tomarker->Name );
                                        // if not, try the associated code
        if ( ! EdfValue )
            EdfValue    = (short) tomarker->Code;
                                        // still not, put a default
        if ( ! EdfValue )
            EdfValue    = (short) 1;

                                        // write as long as needed
        for ( long tf = tomarker->From; tf <= tomarker->To; tf++ ) {

            if ( ofs.IsOpen () )

                ofs.Write ( EDFseekp ( tf - TimeMin, NumTracks ), EdfValue );

            else {
                of->seekp ( EDFseekp ( tf - TimeMin, NumTracks ) );

                of->write ( (char *) &EdfValue, sizeof ( EdfValue ) );
                }
            }
        } // for marker
    } // EDF


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // This one can be done at any time, it doesn't need the main file to be open
else if ( Type == ExportTracksBv ) {

    TFileName           fileoutmrk;
    TFileName           filenameout;
    char                buff[ 1 * KiloByte ];


    fileoutmrk  = Filename;
    ReplaceExtension    ( fileoutmrk, FILEEXT_BVMRK );

    StringCopy          ( filenameout,    ToFileName ( Filename   ) );


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // write / over-write the trigger file

                                        // force overwriting, by safety
    WriteBrainVisionMarkerFile ( fileoutmrk, filenameout );

                                        // re-open in appending mode
    ofstream            ofmrk ( TFileName ( fileoutmrk, TFilenameExtendedPath ), ios::app );


    const TMarker*      tomarker;

    for ( int ti = 0, trigout = 2; ti < Markers.GetNumMarkers (); ti++ ) {

        tomarker   = Markers[ ti ];

        if ( ! IsFlag ( tomarker->Type, markertype ) )
            continue;


        if ( tomarker->From >= NumTime )
            break;

                                        // keep only those completely inside
        if ( ! IsInsideLimits ( tomarker->From, tomarker->To, (long) 0, NumTime - 1 ) )
            continue;

                                        // !Quoting the name as it could contain a comma!
        sprintf ( buff, "Mk%0d=Trigger,\"%s\",%ld,%ld,0,0", trigout, tomarker->Name, tomarker->From - TimeMin + 1, tomarker->To - tomarker->From + 1 );

        ofmrk << buff << fastendl;

        trigout++;
        } // for marker


    ofmrk << fastendl;

    ofmrk.close ();
    } // BrainVision

}


//----------------------------------------------------------------------------
void    TExportTracks::WriteMarkers ()
{
if ( Markers.IsEmpty () || OutputTriggers == NoTriggersNoMarkers )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

MarkerType          markertype         = MarkerTypeUnknown;


if ( IsFlag ( OutputTriggers, MarkersAsMarkers  )                              // Common case
  || IsFlag ( OutputTriggers, MarkersAsTriggers ) && ! HasNativeTriggers () )  // Trying to write the markers as triggers, but file type does not allow it, so back to markers

    SetFlags    ( markertype, MarkerTypeMarker );

                                        // in case caller wanted triggers, but file type doesn't allow, put them as markers
if ( IsFlag ( OutputTriggers, TriggersAsMarkers )                              // Triggers as markers
  || IsFlag ( OutputTriggers, TriggersAsTriggers) && ! HasNativeTriggers () )  // Common case: trying to write the triggers as triggers, but file type does not allow it, so back to markers 

    SetFlags    ( markertype, MarkerTypeTrigger );

                                        // !does not delete any existing file, as this is not sure what the caller actually wanted!
if ( markertype == MarkerTypeUnknown )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           filemrk ( Filename, TFilenameExtendedPath );

AddExtension        ( filemrk, FILEEXT_MRK );
                                        // !this however, if given trigger list ends up to be empty, will delete any empty marker file!
Markers.WriteFile  ( filemrk, markertype, 0,  NumTime - 1 );
}


//----------------------------------------------------------------------------
                                        // for binary files, needs to know NumElectrodes and NumFrequencies (if relevant)
                                        // to write the electrode names and frequency names
void    TExportTracks::WriteBrainVisionMarkerFile ( const char* fileoutmrk, const char* filenameout )
{
if ( ! ( StringIsNotEmpty ( fileoutmrk ) && StringIsNotEmpty ( filenameout ) ) )
    return;


ofstream            ofmrk ( TFileName ( fileoutmrk, TFilenameExtendedPath ) );

                                        // Standard pamphlet
ofmrk << "Brain Vision Data Exchange Marker File, Version 1.0" << fastendl;
ofmrk << ";" << ExportedByCartool << fastendl;
ofmrk << fastendl;

ofmrk << "[Common Infos]" << fastendl;
ofmrk << "Codepage=UTF-8" << fastendl;
ofmrk << "DataFile=" << filenameout << fastendl;
ofmrk << fastendl;

ofmrk << "[Marker Infos]" << fastendl;
ofmrk << "; Each entry: Mk<Marker number>=<Type>,<Description>,<Position In data points>," << fastendl;
ofmrk << "; <Size In data points>, <Channel number (0 = Marker Is related To All Channels)>" << fastendl; 
ofmrk << "; <Date (YYYYMMDDhhmmssuuuuuu)>" << fastendl;
ofmrk << "; Fields are delimited by commas, some fields might be omitted (Empty)." << fastendl;
ofmrk << "; Commas in type or description text are coded as '\\1'." << fastendl;

                                        // date & segment
char            buff[ KiloByte ];

                                        // force using positive values
sprintf ( buff, "%04d%02d%02d%02d%02d%02d%03d%03d", abs ( DateTime.GetYear        () ), 
                                                    abs ( DateTime.GetMonth       () ), 
                                                    abs ( DateTime.GetDay         () ), 
                                                    abs ( DateTime.GetHour        () ), 
                                                    abs ( DateTime.GetMinute      () ), 
                                                    abs ( DateTime.GetSecond      () ), 
                                                    abs ( DateTime.GetMillisecond () ),
                                                    abs ( DateTime.GetMicrosecond () )  );
                                        // then see if offset is negative
double              us          = DateTime.RelUsToAbsUs ( 0 );
                                        // -> prepend string with "-", although NOT STANDARD
if ( us < 0 )
    StringPrepend ( buff, "-" );

//DateTime.Show ( buff );

ofmrk << "Mk1=New Segment,,1,1,0," << buff << fastendl;


ofmrk.close ();
}


//----------------------------------------------------------------------------
                                        // for binary files, needs to know NumElectrodes and NumFrequencies (if relevant)
                                        // to write the electrode names and frequency names
void    TExportTracks::WriteHeader ( bool overwrite )
{
char            buff[ KiloByte ];

                                        // go back to beginning, if needed
of->seekp ( 0, ios::beg );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Text files

                                        // now, put the appropriate header
if      ( Type == ExportTracksEp ) {

    SetAtomType ( AtomTypeString );
    NumFiles            = 1;
                                        // this will format nicely, with the risk of losing some digits in very low values like 0.000123456 -> 0.0001235
    *of << StreamFormatFixed;
    }

else if ( Type == ExportTracksEph ) {

    SetAtomType ( AtomTypeString );
    NumFiles            = 1;

    *of << StreamFormatGeneral;
    *of << StreamFormatInt32   << NumTracks         << Tab;
    *of << StreamFormatInt32   << NumTime           << Tab;
    *of << StreamFormatFloat64 << SamplingFrequency;

    if ( ! overwrite )
        *of << fastendl;

    *of << StreamFormatFixed;
    }

else if ( Type == ExportTracksSeg
       || Type == ExportTracksData ) {

    SetAtomType ( AtomTypeString );

    *of << StreamFormatGeneral;

    if ( Type == ExportTracksSeg )
        *of << StreamFormatInt32   << NumClusters << Tab;

    *of << StreamFormatInt32   << NumFiles  << Tab;
    *of << StreamFormatInt32   << NumTime;
    *of << fastendl;

    VariableNames.Concatenate ( buff, Tab );   // using tab separators
    *of << StreamFormatInt32   << NumTracks << Tab;
    *of << StreamFormatText    << buff;

    if ( ! overwrite )
        *of << fastendl;

    *of << StreamFormatFixed;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Binary files

else if ( Type == ExportTracksRis ) {

    if ( ! ( IsScalar ( AtomTypeUseOriginal ) || IsVector ( AtomTypeUseOriginal ) ) )
        SetAtomType ( AtomTypeScalar );

    TRisHeader      risheader;

    *((int *) risheader.Magic)  = RISBIN_MAGICNUMBER1;
    risheader.NumSolutionPoints = NumTracks;
    risheader.NumTimeFrames     = NumTime;
    risheader.SamplingFrequency = SamplingFrequency;
    risheader.IsInverseScalar   = IsScalar ( AtomTypeUseOriginal );

                                        // write header
    of->write ( (char *) (&risheader), sizeof ( risheader ) );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else if ( Type == ExportTracksSef ) {

    SetAtomType ( AtomTypeScalar );
                                        // not defined, or under-defined?
//  if ( SelTracks.Size () < NumTracks ) {
//      SelTracks   = TSelection ( NumTracks, OrderArbitrary );
//      SelTracks.Set ();
//      }


    TSefHeader      sefheader;

    sefheader.Version           = SEFBIN_MAGICNUMBER1;

    sefheader.NumElectrodes     = NumTracks;
    sefheader.NumAuxElectrodes  = NumAuxTracks;
    sefheader.SamplingFrequency = SamplingFrequency;
    sefheader.NumTimeFrames     = NumTime;

    sefheader.Year              = (short) DateTime.GetYear  ();
    sefheader.Month             = (short) DateTime.GetMonth ();
    sefheader.Day               = (short) DateTime.GetDay   ();
    sefheader.Hour              = (short) DateTime.GetHour  ();
    sefheader.Minute            = (short) DateTime.GetMinute();
    sefheader.Second            = (short) DateTime.GetSecond();
    sefheader.Millisecond       = (short) DateTime.GetMillisecond ();

                                        // write header
    of->write ( (char *) (&sefheader), sizeof ( sefheader ) );

                                        // write electrode names
//  TSefChannelName     elname;
    char                elname[ 256 ];

                                        // loops to handle both present or missing SelTracks
    TIteratorSelectedForward    seli ( SelTracks );
    for ( int i = SelTracks.IsNotAllocated () ? 0 : seli(); i >= 0 && ( SelTracks.IsAllocated () || i < NumTracks ); i = SelTracks.IsNotAllocated () ? i + 1 : ++seli ) {

        ClearString      ( elname, sizeof ( TSefChannelName ) );

        GetElectrodeName ( i, elname, sizeof ( TSefChannelName ) );

                                        // is an aux, but the name is not recognized as such?
        if (   AuxTracks.IsAllocated () 
          && ( i >= AuxTracks.Size () || AuxTracks[ i ] ) 
          && ! IsElectrodeNameAux ( elname ) ) {

            StringPrepend ( elname, "aux" );
            elname[ sizeof ( TSefChannelName ) - 1 ]    = EOS;
            }

        of->write ( elname, sizeof ( TSefChannelName ) );
        }

    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else if ( Type == ExportTracksBv ) {
                                        // write the header file
    TFileName           fileouthdr;
    TFileName           fileoutmrk;
    TFileName           filenameout;
    TFileName           filenameoutmrk;

    fileouthdr  = Filename;
    ReplaceExtension    ( fileouthdr, FILEEXT_BVHDR );

    fileoutmrk  = Filename;
    ReplaceExtension    ( fileoutmrk, FILEEXT_BVMRK );

    StringCopy          ( filenameout,    ToFileName ( Filename   ) );
    StringCopy          ( filenameoutmrk, ToFileName ( fileoutmrk ) );


    ofstream            ofhdr ( TFileName ( fileouthdr, TFilenameExtendedPath ) );


    ofhdr << "Brain Vision Data Exchange Header File Version 1.0" << fastendl;
    ofhdr << ";" << ExportedByCartool << fastendl;
    ofhdr << fastendl;


    ofhdr << "[Common Infos]" << fastendl;
    ofhdr << "Codepage=UTF-8" << fastendl;
    ofhdr << "DataFile=" << filenameout << fastendl;
    ofhdr << "MarkerFile=" << filenameoutmrk << fastendl;
    ofhdr << "DataFormat=BINARY" << fastendl;
    ofhdr << "; Data orientation: VECTORIZED=ch1,pt1, ch1,pt2..., MULTIPLEXED=ch1,pt1, ch2,pt1 ..." << fastendl;
    ofhdr << "DataOrientation=MULTIPLEXED" << fastendl;
    ofhdr << "DataType=TIMEDOMAIN" << fastendl;
    ofhdr << "NumberOfChannels=" << NumTracks << fastendl;
    ofhdr << "DataPoints=" << NumTime << fastendl;
    ofhdr << "; Sampling interval in microseconds if time domain (convert to Hertz:" << fastendl << "; 1000000 / SamplingInterval) or in Hertz if frequency domain:" << fastendl;
    ofhdr << "SamplingInterval="  << StreamFormatFloat64 << StreamFormatLeft << ( SamplingFrequency ? TimeFrameToMicroseconds ( 1, SamplingFrequency ) : 0 ) << fastendl;
    ofhdr << fastendl;

                                        // if binary
    if ( IsFileBinary () ) {
        ofhdr << "[Binary Infos]" << fastendl;
        ofhdr << "BinaryFormat=IEEE_FLOAT_32" << fastendl;  // only this type for the moment, also possible: INT_16 and UINT_16
        ofhdr << fastendl;
        }
    else {                              // if text !!not tested!!
        ofhdr << "[ASCII Infos]" << fastendl;
        ofhdr << "; Decimal symbol for floating point numbers: the header file always uses a dot (.)," << fastendl << "; however the data file might use a different one" << fastendl;
        ofhdr << "DecimalSymbol=." << fastendl;
        ofhdr << "; SkipLines, SkipColumns: leading lines and columns with additional informations." << fastendl;
        ofhdr << "SkipLines=1" << fastendl;
        ofhdr << "SkipColumns=0" << fastendl;
        ofhdr << fastendl;
        }

                                        // electrodes name
    ofhdr << "[Channel Infos]" << fastendl;
    ofhdr << "; Each entry: Ch<Channel number>=<Name>,<Reference channel name>," << fastendl;
    ofhdr << "; <Resolution in ""Unit"">,<Unit>, Future extensions..." << fastendl;
    ofhdr << "; Fields are delimited by commas, some fields might be omitted (empty)." << fastendl;
    ofhdr << "; Commas in channel names are coded as '\\1'." << fastendl;

    char                elname[ 256 ];

                                        // loops to handle both present or missing SelTracks
    TIteratorSelectedForward    seli ( SelTracks );
    for ( int i = SelTracks.IsNotAllocated () ? 0 : seli(), j = 0; i >= 0 && ( SelTracks.IsAllocated () || i < NumTracks ); i = SelTracks.IsNotAllocated () ? i + 1 : ++seli, j++ ) {

        GetElectrodeName ( i, elname, 255 );

                                        // is an aux, but the name is not recognized as such?
//      if ( AuxTracks.IsAllocated () && AuxTracks[ i ] && ! IsElectrodeNameAux ( elname ) ) {
//          StringPrepend ( elname, "aux" );
//          elname[ sizeof ( TBVChannelName ) - 1 ]     = EOS;
//          }

        sprintf ( buff, "Ch%0d=%s,,,%s", j + 1, elname, "µV" );
        ofhdr << buff << fastendl;
        }

    ofhdr << fastendl;

/*                                        // electrodes coordinates
    ofhdr << "[Coordinates]" << fastendl;
    ofhdr << "; Each entry: Ch<Channel number>=<Radius>,<Theta>,<Phi>" << fastendl;

                                        // loops to handle both present or missing SelTracks
    seli.FirstSelected ( SelTracks );
    for ( int i = SelTracks.IsNotAllocated () ? 0 : seli(), j = 0; i >= 0 && ( SelTracks.IsAllocated () || i < NumTracks ); i = SelTracks.IsNotAllocated () ? i + 1 : ++seli, j++ ) {

        GetElectrodeCoordinate ( i, elname, 255 );

        sprintf ( buff, "Ch%0d=%0d,%0d,%0d", j + 1,  );
        ofhdr << buff << fastendl;
        }

    ofhdr << fastendl;
*/

//  ofhdr << "[Comment]" << fastendl;
                                        // can put anything here
//  ofhdr << fastendl;

    ofhdr.close ();

                                        // Write the default trigger file, used for the date(!)
    WriteBrainVisionMarkerFile ( fileoutmrk, filenameout );

                                        // Then the actual triggers, although it could possibly over-write the one we just wrote
    WriteTriggers ();


    SetAtomType ( AtomTypeScalar );
/*                                        // not, or sub-defined?
    if ( SelTracks.Size () < NumTracks ) {
        SelTracks   = TSelection ( NumTracks, OrderArbitrary );
        SelTracks.Set ();
        }
*/
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else if ( Type == ExportTracksEdf ) {

    SetAtomType ( AtomTypeScalar );
                                        // Write fixed size header
                                        // type
    SetString ( buff, ' ', 8 );
    buff[ 0 ] = '0';
    of->write ( buff, 8 );
                                        // subject id
    SetString ( buff, ' ', 80 );
    of->write ( buff, 80 );
                                        // recording id
    of->write ( buff, 80 );
                                        // write date - !only last 2 digits for year!
    sprintf ( buff, "%02d.%02d.%02d", DateTime.GetDay(), DateTime.GetMonth(), ( DateTime.GetYear() % 100 ) );
    of->write ( buff, 8 );
                                        // !there are NO milliseconds, and much less microseconds in this format! - caller is responsible to truncate / round before calling
    sprintf ( buff, "%02d.%02d.%02d", DateTime.GetHour(), DateTime.GetMinute(), DateTime.GetSecond() );
    of->write ( buff, 8 );

                                        // header size
    int     numelinfile     = NumTracks + 1;    // add the status (triggers) line
    EdfDataOrg              = ( numelinfile + 1 ) * 256;

    SetString ( buff, ' ', 8 );
    sprintf ( buff, "%0lld", EdfDataOrg );
    *StringEnd ( buff ) = ' ';
    of->write ( buff, 8 );

                                        // reserved
    SetString ( buff, ' ', 44 );
    of->write ( buff, 44 );

                                        // sets block size
    if ( SamplingFrequency > 0 )
        EdfTfPerRec         = Clip ( Round ( SamplingFrequency ), 1, EdfMaxBlockSize ); // 1 second of data, clipped within 1 to 61440 limit
    else
        EdfTfPerRec         = Clip ( Round ( NumTime ),           1, EdfMaxBlockSize ); // using time as block size


    int     blockduration   = SamplingFrequency > 0 ? Round ( EdfTfPerRec / SamplingFrequency ) // also rounding the result, if below 1 Hz
                                                : 0;                                        // unknown - we count on the reading part to handle that...


    EdfNumRecords       = ( NumTime + EdfTfPerRec - 1 ) / EdfTfPerRec;          // round the number of records, but have to handle trailing data
//  EdfNumRecords       = NumTime / EdfTfPerRec;                                // truncated number of records, the easiest way

                                        // used later
    EdfBlockSize        = numelinfile * EdfTfPerRec * sizeof ( EdfValue );

                                        // the remaining part will be padded with 0
    EdfTrailingTF       = EdfNumRecords * EdfTfPerRec - NumTime;

                                        // removed this, because if we want to export 100, don't do 500!
/*
                                        // use available data to extend the write
    if ( processinterval ) {            // only if in interval
                                        // enough room?
        if ( timemax + EdfTrailingTF < EEGDoc->GetNumTimeFrames() ) {
            timemax         += EdfTrailingTF;
            timenum         += EdfTrailingTF;
            EdfTrailingTF   = 0;
            }
        else {                          // otherwise, take what is available
            long        add     = EEGDoc->GetNumTimeFrames() - 1 - timemax;
            timemax         += add;
            timenum         += add;
            EdfTrailingTF   -= add;
            }
        }
*/
                                        // number of records
    SetString ( buff, ' ', 8 );
    sprintf ( buff, "%0ld", EdfNumRecords );
    *StringEnd ( buff ) = ' ';
    of->write ( buff, 8 );
                                        // duration of 1 record, in [s] - could be 0 if sampling frequency is unknown
    SetString ( buff, ' ', 8 );
    sprintf ( buff, "%0d", blockduration );
    *StringEnd ( buff ) = ' ';
    of->write ( buff, 8 );
                                        // number of electrodes + status line
    SetString ( buff, ' ', 4 );
    sprintf ( buff, "%0d", numelinfile );
    if ( StringLength ( buff ) < 4 )          // this will allow up to 9999 electrodes, in case we deal with solution points!
        *StringEnd ( buff ) = ' ';
    of->write ( buff, 4 );

                                        // Header, variable part
                                        // Electrodes names
    TIteratorSelectedForward    seli ( SelTracks );
    for ( int i = SelTracks.IsNotAllocated () ? 0 : seli(); i >= 0 && ( SelTracks.IsAllocated () || i < NumTracks ); i = SelTracks.IsNotAllocated () ? i + 1 : ++seli ) {

        SetString ( buff, ' ', 16 );

        GetElectrodeName ( i, buff, 16 );

        if ( StringLength ( buff ) < 16 )
            *StringEnd ( buff ) = ' ';
        of->write ( buff, 16 );
        }
                                        // ...and one more
    CopyVirtualMemory ( buff, "Status          ", 16 );
    of->write ( buff, 16 );

                                        // Electrodes types
    seli.FirstSelected ( SelTracks );
    for ( int i = SelTracks.IsNotAllocated () ? 0 : seli(); i >= 0 && ( SelTracks.IsAllocated () || i < NumTracks ); i = SelTracks.IsNotAllocated () ? i + 1 : ++seli ) {

        SetString ( buff, ' ', 80 );
        if      ( AuxTracks.IsAllocated () && AuxTracks[ i ] )                      CopyVirtualMemory ( buff, "Auxiliary channel", 17 );
//      else if ( i >= EEGDoc->GetNumElectrodes() )                                 CopyVirtualMemory ( buff, "Computed channel",  16 );
        else if ( ElectrodesNames.IsNotEmpty () 
               && IsStringAmong ( ElectrodesNames[ i ], ComputedTracksNames ) )     CopyVirtualMemory ( buff, "Computed channel",  16 );
        else                                                                        CopyVirtualMemory ( buff, "Regular channel",   15 );

        of->write ( buff, 80 );
        }
                                        // ...and one more
    SetString ( buff, ' ', 80 );
    CopyVirtualMemory ( buff, "Status", 6 );
    of->write ( buff, 80 );

                                        // units
    seli.FirstSelected ( SelTracks );
    for ( int i = SelTracks.IsNotAllocated () ? 0 : seli(); i >= 0 && ( SelTracks.IsAllocated () || i < NumTracks ); i = SelTracks.IsNotAllocated () ? i + 1 : ++seli ) {

//      if ( i == EEGDoc->GetDisIndex () )
        if ( ElectrodesNames.IsNotEmpty () && StringIs ( ElectrodesNames[ i ], "DIS" ) )
            CopyVirtualMemory ( buff, "None    ", 8 );
        else
            CopyVirtualMemory ( buff, "uV      ", 8 );
        of->write ( buff, 8 );
        }
                                        // ...and one more
    CopyVirtualMemory ( buff, "None    ", 8 );
    of->write ( buff, 8 );

                                        // tracks rescaling
    double          physicalmax     =   EdfPhysicalMaxMargin * MaxValue;// !boost the actual expected value so to avoid clipping as much as possible!
    double          physicalmin     = - physicalmax;
    int             digitalmax      =   EdfDigitalMax;                  // what is actually written in file, as if coming from an ADC converter - !also added some margin in case of real-time output!
    int             digitalmin      = - digitalmax;

                                        // convert values to strings
    char            physicalmaxtxt  [ 16 ];
    char            physicalmintxt  [ 16 ];
    char            digitalmaxtxt   [ 16 ];
    char            digitalmintxt   [ 16 ];

    auto    FloatToFixedWidthString = [] ( char* string, double value, int totalwidth )     { sprintf ( string, "%*lg", totalwidth, value ); };

    SetString ( physicalmaxtxt, ' ', 8 );
    SetString ( physicalmintxt, ' ', 8 );
    SetString ( digitalmaxtxt,  ' ', 8 );
    SetString ( digitalmintxt,  ' ', 8 );

    FloatToFixedWidthString ( physicalmaxtxt, physicalmax, 8 );
    FloatToFixedWidthString ( physicalmintxt, physicalmin, 8 );
    IntegerToString         ( digitalmaxtxt,  digitalmax );
    IntegerToString         ( digitalmintxt,  digitalmin );


    physicalmax     = StringToDouble ( physicalmaxtxt );    // converting float to string has introduced some rounding error, so we have to recompute the new physical min / max
    physicalmin     = StringToDouble ( physicalmintxt );

                                        // computing the factors of the linear transform  digital <-> physical  values
    EdfPhysicalMin  = physicalmin;
    EdfDigitalMin   = digitalmin;
    EdfRatio        = (double) ( digitalmax - digitalmin ) / ( physicalmax - physicalmin );


    if ( StringLength ( physicalmaxtxt ) < 8 )
        *StringEnd ( physicalmaxtxt ) = ' ';

    if ( StringLength ( physicalmintxt ) < 8 )
        *StringEnd ( physicalmintxt ) = ' ';


    if ( StringLength ( digitalmaxtxt ) < 8 )
        *StringEnd ( digitalmaxtxt ) = ' ';

    if ( StringLength ( digitalmintxt ) < 8 )
        *StringEnd ( digitalmintxt ) = ' ';

                                        // physical min
    for ( int i = 0; i < numelinfile; i++ )
        of->write ( physicalmintxt, 8 );

                                        // physical max
    for ( int i = 0; i < numelinfile; i++ ) 
        of->write ( physicalmaxtxt, 8 );

                                        // digital min
    for ( int i = 0; i < numelinfile; i++ )
        of->write ( digitalmintxt, 8 );

                                        // digital max
    for ( int i = 0; i < numelinfile; i++ )
        of->write ( digitalmaxtxt, 8 );

                                        // prefiltering
    for ( int i = 0; i < numelinfile; i++ ) {

        SetString ( buff, ' ', 80 );
        of->write ( buff, 80 );
        }

                                        // number of TF in 1 record
    for ( int i=0; i < numelinfile; i++ ) {

        SetString ( buff, ' ', 8 );

        sprintf ( buff, "%0ld", EdfTfPerRec );

        if ( StringLength ( buff ) < 8 )
            *StringEnd ( buff ) = ' ';
        of->write ( buff, 8 );
        }

                                        // reserved
    for ( int i = 0; i < numelinfile; i++ ) {

        SetString ( buff, ' ', 32 );
        of->write ( buff, 32 );
        }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
else if ( Type == ExportTracksFreq ) {

    if ( ! ( IsScalar ( AtomTypeUseOriginal ) || IsComplex ( AtomTypeUseOriginal ) ) )
        SetAtomType ( AtomTypeScalar );

/*                                        // not, or sub-defined?
    if ( SelTracks.Size () < NumTracks ) {
        SelTracks   = TSelection ( NumTracks, OrderArbitrary );
        SelTracks.Set ();
        }
*/

                                        // setup header
    TFreqHeader     freqheader;

    freqheader.Version          = FREQBIN_MAGICNUMBER2;

                                        // clear string
    ClearString ( freqheader.Type, MaxCharFreqType );
                                        // choose the analysis type and result type
    StringCopy  ( freqheader.Type, FreqTypeName );

    freqheader.NumChannels      = NumTracks;
    freqheader.NumFrequencies   = NumFrequencies;
    freqheader.NumBlocks        = NumTime;
    freqheader.SamplingFrequency= SamplingFrequency;
    freqheader.BlockFrequency   = BlockFrequency;

    freqheader.Year             = (short) DateTime.GetYear();
    freqheader.Month            = (short) DateTime.GetMonth();
    freqheader.Day              = (short) DateTime.GetDay();
    freqheader.Hour             = (short) DateTime.GetHour ();
    freqheader.Minute           = (short) DateTime.GetMinute ();
    freqheader.Second           = (short) DateTime.GetSecond ();
    freqheader.Millisecond      = (short) DateTime.GetMillisecond ();

                                        // write header
    of->write ( (char *)(&freqheader), sizeof ( freqheader ) );

                                        // write electrode names
    char                elname[ 256 ];

                                        // loops to handle both present or missing SelTracks
    TIteratorSelectedForward    seli ( SelTracks );
    for ( int i = SelTracks.IsNotAllocated () ? 0 : seli(); i >= 0 && ( SelTracks.IsAllocated () || i < NumTracks ); i = SelTracks.IsNotAllocated () ? i + 1 : ++seli ) {

        ClearString ( elname, sizeof ( TFreqChannel ) );

        GetElectrodeName ( i, elname, sizeof ( TFreqChannel ) );

                                        // is an aux, but the name is not recognized as such?
        if ( AuxTracks.IsAllocated () && AuxTracks[ i ] && ! IsElectrodeNameAux ( elname ) ) {
            StringPrepend ( elname, "aux" );
            elname[ sizeof ( TFreqChannel ) - 1 ]   = 0;
            }

        of->write ( elname, sizeof ( TFreqChannel ) );
        }

                                        // FREQBIN_MAGICNUMBER2
    char                freqname[ 256 ];

    for ( int fi = 0; fi < NumFrequencies; fi++ ) {

        ClearString ( freqname, sizeof ( TFreqFrequencyName ) );

        GetFrequencyName ( fi, freqname, sizeof ( TFreqFrequencyName ) );

        of->write ( freqname, sizeof ( TFreqFrequencyName ) );
        }

    }

                                            // remember where data starts, mostly for binary files
EndOfHeader     = of->tellp ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Write markers if requested
WriteMarkers ();
}


//----------------------------------------------------------------------------
                                        // All the various writing methods
//----------------------------------------------------------------------------
                                        // The versatile work-horse, not optimized but safe
void    TExportTracks::Write ( float value )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


if      ( Type == ExportTracksEp
       || Type == ExportTracksEph
       || Type == ExportTracksSeg
       || Type == ExportTracksData ) {


    *of << StreamFormatFloat32 << value << Tab;


    CurrentPositionTrack = ++CurrentPositionTrack % ( NumTracks * NumFiles );
    if ( ! CurrentPositionTrack )
        CurrentPositionTime++;

    if ( ! CurrentPositionTrack )
        *of << fastendl;
    }


else if ( Type == ExportTracksSef
       || Type == ExportTracksBv  ) {

    of->write ( (char *) &value, sizeof ( float ) );


    CurrentPositionTrack = ++CurrentPositionTrack % NumTracks;
    if ( ! CurrentPositionTrack )
        CurrentPositionTime++;
    }


else if ( Type == ExportTracksRis ) {

    of->write ( (char *) &value, sizeof ( float ) );

    if ( IsVector ( AtomTypeUseOriginal ) ) {   // ouput vectorial, input scalar: complement with null Y / Z component
        value   = 0;
        of->write ( (char *) &value, sizeof ( float ) );
        of->write ( (char *) &value, sizeof ( float ) );
        }


    CurrentPositionTrack = ++CurrentPositionTrack % NumTracks;
    if ( ! CurrentPositionTrack )
        CurrentPositionTime++;
    }


else if ( Type == ExportTracksEdf ) {
                                        // convert value to 16 bits
    value       = EdfDigitalMin + ( value - EdfPhysicalMin ) * EdfRatio;
                                        // do a nice rounding + final safety clipping
    EdfValue    = (short) Clip ( Round ( value ), SHRT_MIN, SHRT_MAX );

    of->seekp ( EDFseekp ( CurrentPositionTime, CurrentPositionTrack ) );

    of->write ( (char *) &EdfValue, sizeof ( EdfValue ) );

                                        // handle only once the status line
    if ( CurrentPositionTrack == NumTracks - 1 ) {
                                        // reset trigger line to 0
        EdfValue    = (short) 0;

        of->seekp ( EDFseekp ( CurrentPositionTime, NumTracks ) );

        of->write ( (char *) &EdfValue, sizeof ( EdfValue ) );
        }


    CurrentPositionTrack = ++CurrentPositionTrack % NumTracks;
    if ( ! CurrentPositionTrack )
        CurrentPositionTime++;
    }


if ( CurrentPositionTime >= NumTime )   // this should be the end!
    End ();
}


//----------------------------------------------------------------------------
void    TExportTracks::Write ( long value )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


if      ( Type == ExportTracksEp
       || Type == ExportTracksEph
       || Type == ExportTracksSeg
       || Type == ExportTracksData ) {


    *of << StreamFormatInt32 << value << Tab;


    CurrentPositionTrack = ++CurrentPositionTrack % ( NumTracks * NumFiles );
    if ( ! CurrentPositionTrack )
        CurrentPositionTime++;

    if ( ! CurrentPositionTrack )
        *of << fastendl;


    if ( CurrentPositionTime >= NumTime )   // this should be the end!
        End ();
    }

else
    Write ( (float) value );            // all other cases don't use double, but only float
}


//----------------------------------------------------------------------------
void    TExportTracks::Write ( double value )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


if      ( Type == ExportTracksEp
       || Type == ExportTracksEph
       || Type == ExportTracksSeg
       || Type == ExportTracksData ) {


    *of << StreamFormatFloat64 << value << Tab;


    CurrentPositionTrack = ++CurrentPositionTrack % ( NumTracks * NumFiles );
    if ( ! CurrentPositionTrack )
        CurrentPositionTime++;

    if ( ! CurrentPositionTrack )
        *of << fastendl;


    if ( CurrentPositionTime >= NumTime )   // this should be the end!
        End ();
    }

else
    Write ( (float) value );            // all other cases don't use double, but only float
}


//----------------------------------------------------------------------------
// !Does not update CurrentPositionTrack nor CurrentPositionTime -> caller handles the loops & indexes!

void    TExportTracks::Write ( float value, long t, int e )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


if ( ! ( IsInsideLimits ( t, (long) 0, NumTime   - 1 )
      && IsInsideLimits ( e, 0,        NumTracks - 1 ) ) )
    return;


if      ( Type == ExportTracksEp
       || Type == ExportTracksEph
       || Type == ExportTracksSeg
       || Type == ExportTracksData ) {
                                        // quite complicated because text format!
    }


else if ( Type == ExportTracksSef
       || Type == ExportTracksBv
       || Type == ExportTracksRis ) {


    int     atomsize    = IsVector ( AtomTypeUseOriginal ) ? 3 * sizeof ( float ) : sizeof ( float );

    of->seekp ( EndOfHeader + ( t * NumTracks + e ) * atomsize, ios::beg );


    if ( IsScalar ( AtomTypeUseOriginal ) )

        of->write ( (char *) &value, sizeof ( float ) );

    else if ( IsVector ( AtomTypeUseOriginal ) ) {  // ouput vectorial, input scalar: complement with null Y / Z component

        of->write ( (char *) &value, sizeof ( float ) );
        value   = 0;
        of->write ( (char *) &value, sizeof ( float ) );
        of->write ( (char *) &value, sizeof ( float ) );
        }

    }


else if ( Type == ExportTracksEdf ) {
                                        // quite complicated, too!
    }

}


//----------------------------------------------------------------------------
// !Does not update CurrentPositionTrack nor CurrentPositionTime -> caller handles the loops & indexes!

void    TExportTracks::Write ( float value, long t, int e, int f )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


if ( ! ( IsInsideLimits ( t, (long) 0, NumTime   - 1      )
      && IsInsideLimits ( e, 0,        NumTracks - 1      )
      && IsInsideLimits ( f, 0,        NumFrequencies - 1 ) ) )
    return;


if      ( Type == ExportTracksFreq ) {

    int     atomsize    = IsComplex ( AtomTypeUseOriginal ) ? sizeof ( complex<float> ) : sizeof ( float );

    of->seekp ( EndOfHeader + ( ( t * NumTracks + e ) * NumFrequencies + f ) * atomsize, ios::beg );


    if ( IsScalar ( AtomTypeUseOriginal ) )

        of->write ( (char *) &value, sizeof ( float ) );
    else {                              // convert to complex
                                        // write real part
        of->write ( (char *) &value, sizeof ( float ) );
                                        // force imaginary to 0
        value   = 0;
        of->write ( (char *) &value, sizeof ( float ) );
        }

    }

else                                    // case not handled
    return;
}


//----------------------------------------------------------------------------
void    TExportTracks::Write ( complex<float> comp, long t, int e, int f )
{
Write ( comp.real (), comp.imag (), t, e, f );
}


//----------------------------------------------------------------------------
                                        // saves complex numbers as consecutive, single float values
                                        // it used to be using the complex structure, which is long double, which makes files too big
void    TExportTracks::Write ( float realpart, float imagpart, long t, int e, int f )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


if ( ! ( IsInsideLimits ( t, (long) 0, NumTime   - 1      )
      && IsInsideLimits ( e, 0,        NumTracks - 1      )
      && IsInsideLimits ( f, 0,        NumFrequencies - 1 ) ) )
    return;


if      ( Type == ExportTracksFreq ) {

    int     atomsize    = IsComplex ( AtomTypeUseOriginal ) ? sizeof ( complex<float> ) : sizeof ( float );

    of->seekp ( EndOfHeader + ( ( t * NumTracks + e ) * NumFrequencies + f ) * atomsize, ios::beg );


    if ( IsScalar ( AtomTypeUseOriginal ) ) {   // convert to scalar
        float       value   = sqrt ( Square ( realpart ) + Square ( imagpart ) );
//      float       value   = realpart;
        of->write ( (char *) &value,    sizeof ( float ) );
        }
    else {
        of->write ( (char *) &realpart, sizeof ( float ) );
        of->write ( (char *) &imagpart, sizeof ( float ) );
        }

    }

else                                    // case not handled
    return;
}


//----------------------------------------------------------------------------
// !Does not update CurrentPositionTrack nor CurrentPositionTime -> caller handles the loops & indexes!

void    TExportTracks::Write ( const TVector<float> v, long t, int f )
{
for ( int el = 0; el < v.GetDim (); el++ ) {

    UpdateApplication;

    Write ( v[ el ], t, el, f );
    }

//of->flush ();
}


//----------------------------------------------------------------------------
void    TExportTracks::Write ( const TVector3Float& vector )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


if ( IsVector ( AtomTypeUseOriginal ) && Type == ExportTracksRis ) {

                                        // output vectorial, input vectorial: write the vector
    of->write ( (char *) & vector.X, vector.MemorySize () );

//  of->flush ();

    CurrentPositionTrack = ++CurrentPositionTrack % NumTracks;
    if ( ! CurrentPositionTrack )
        CurrentPositionTime++;


    if ( CurrentPositionTime >= NumTime )   // this should be the end!
        End ();
    }

else
    Write ( (float) vector.Norm () );   // all other cases: use the norm
}


//----------------------------------------------------------------------------
void    TExportTracks::Write ( const TVector3Float& vector, long t, int e )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


if ( ! ( IsInsideLimits ( t, (long) 0, NumTime   - 1 )
      && IsInsideLimits ( e, 0,        NumTracks - 1 ) ) )
    return;


if      ( Type == ExportTracksEp
       || Type == ExportTracksEph
       || Type == ExportTracksSeg
       || Type == ExportTracksData ) {
                                        // quite complicated because text format!
    }


else if ( Type == ExportTracksSef
       || Type == ExportTracksBv
       || Type == ExportTracksRis ) {


    int     atomsize    = 3 * sizeof ( float );

    of->seekp ( EndOfHeader + ( t * NumTracks + e ) * atomsize, ios::beg );

    of->write ( (char *) &vector.X, sizeof ( float ) );
    of->write ( (char *) &vector.Y, sizeof ( float ) );
    of->write ( (char *) &vector.Z, sizeof ( float ) );
    }


else if ( Type == ExportTracksEdf ) {
                                        // quite complicated, too!
    }

}


//----------------------------------------------------------------------------
void    TExportTracks::Write ( const TArray1<float>& values )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();

                                        // optimized write
if      ( Type == ExportTracksSef
       || Type == ExportTracksBv
       || Type == ExportTracksRis /*&& ! IsVector ( AtomTypeUseOriginal )*/   // caller's responsibility to send a multiplexed array
        ) {

    UpdateApplication;
                                        // send the whole array at once
    of->write ( (char *) values.GetMemoryAddress (), values.GetMemorySize () );

//  of->flush ();
    }

else

    for ( long tf = 0; tf < values.GetDim1 (); tf++ ) {

        UpdateApplication;

        Write ( values ( tf ) );
        }
}


//----------------------------------------------------------------------------
void    TExportTracks::Write ( const TArray1<double>& values )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


for ( long tf = 0; tf < values.GetDim1 (); tf++ ) {

    UpdateApplication;

    Write ( values ( tf ) );
    }
}


//----------------------------------------------------------------------------
void    TExportTracks::Write ( const TArray1<long>& values )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


for ( long tf = 0; tf < values.GetDim1 (); tf++ ) {

    UpdateApplication;

    Write ( (double) values ( tf ) );
    }
}


//----------------------------------------------------------------------------
void    TExportTracks::Write ( const TArray1<TVector3Float>& vec3 )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


for ( long tf = 0; tf < vec3.GetDim1 (); tf++ ) {

    UpdateApplication;

    Write ( vec3 ( tf ) );
    }
}


//----------------------------------------------------------------------------
                                        // !NOT for vectorial!
void    TExportTracks::Write ( const TMap& map )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();

                                        // optimized write
if      ( Type == ExportTracksSef
       || Type == ExportTracksBv
       || Type == ExportTracksRis /*&& ! IsVector ( AtomTypeUseOriginal )*/   // caller's responsibility to send a multiplexed array
//     && typeid ( TMapAtomType ) == typeid ( float ) 
        ) {

    UpdateApplication;
                                        // send the whole array at once
    of->write ( (char *) map.GetMemoryAddress (), map.GetMemorySize () );

//  of->flush ();
    }

else

    for ( int i = 0; i < map.GetDim (); i++ ) {

        UpdateApplication;

        Write ( map ( i ) );
        }
}


//----------------------------------------------------------------------------
void    TExportTracks::Write ( const TArray2<float>& values, const TMarkers* keeplist )
{
                                        // compute / update number of time frames - !Note that it could be less than that due to TimeMin/TimeMax test!
NumTime     = keeplist->GetMarkersTotalLength ();


if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


const TMarker*      tomarker;
                                        // n intervals
                                        // Overlapping is not tested here, so it technically can save non-ordered and/or overlapping epochs
for ( int ki = 0; ki < (int) (*keeplist); ki++ ) {

    UpdateApplication;

    tomarker   = (*keeplist)[ ki ];

                                        // skip markers not inside limit
    if ( ! tomarker->IsInsideLimits ( TimeMin, TimeMax ) )
        continue;

    for ( long tf = tomarker->From; tf <= tomarker->To; tf++ ) {

        for ( int el = 0; el < NumTracks; el++)
            Write ( values ( el, tf ) );
        }

    } // for keeplist


End ();
}


//----------------------------------------------------------------------------
void    TExportTracks::Write ( const TArray2<float>& values, ExportTracksTransposed transpose )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


if ( transpose )    // the "\n" is not handled correctly, user must swap NumTime and NumTracks values to have it work correctly

    for ( long tf = 0; tf < NumTime; tf++ ) {

        UpdateApplication;

        for ( int el = 0; el < NumTracks * NumFiles; el++ )
            Write ( values ( el, tf ) );
        }
else {
                                        // optimized write - only for float case
    if      ( Type == ExportTracksSef
           || Type == ExportTracksBv
           || Type == ExportTracksRis /*&& ! IsVector ( AtomTypeUseOriginal )*/   // caller's responsibility to send a multiplexed array
            ) {

        UpdateApplication;
                                        // send the whole array at once
        of->write ( (char *) values.GetMemoryAddress (), values.GetMemorySize () );

//      of->flush ();
        }

    else

        for ( long tf = 0; tf < NumTime; tf++ ) {

            UpdateApplication;

            for ( int el = 0; el < NumTracks * NumFiles; el++ )
                Write ( values ( tf, el ) );
            }
    }


End ();
}


//----------------------------------------------------------------------------
void    TExportTracks::Write ( const TSetArray2<float>& values )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


for ( long tf   = 0; tf   < NumTime;         tf++   )
for ( int  e    = 0; e    < NumTracks;       e++    ) {

    UpdateApplication;

    for ( int freq = 0; freq < NumFrequencies ; freq++ )

        Write ( values ( freq, e, tf ), tf, e, freq );
    }


End ();
}


//----------------------------------------------------------------------------
                                        // Array dimensions, related to frequency display:
                                        //   Dim 1: Electrodes, downward
                                        //   Dim 2: Time,       rightward
                                        //   Dim 3: Frequency,  upward
void    TExportTracks::Write ( const TArray3<float>& values, ExportTracksTransposed transpose )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


if ( transpose ) {
                                        // no optimization possible
    for ( long tf   = 0; tf   < NumTime;         tf++   )
    for ( int  e    = 0; e    < NumTracks;       e++    ) {

        UpdateApplication;

        for ( int freq = 0; freq < NumFrequencies ; freq++ )

            Write ( values ( e, tf, freq ), tf, e, freq );
        }
    }
else {
                                        // optimized write - no checks on dimensions, so we can write complex data f.ex.
    if      ( Type == ExportTracksFreq
            ) {

        UpdateApplication;

        of->seekp ( EndOfHeader, ios::beg );
                                        // send the whole array at once - caller is trusted to send the right amount of data here
        of->write ( (char *) values.GetMemoryAddress (), values.GetMemorySize () );

//      of->flush ();
        }

    else {

        for ( long tf   = 0; tf   < NumTime;         tf++   )
        for ( int  e    = 0; e    < NumTracks;       e++    ) {

            UpdateApplication;

            for ( int freq = 0; freq < NumFrequencies ; freq++ )

                Write ( values ( tf, e, freq ), tf, e, freq );
            }
        }
    }


End ();
}


//----------------------------------------------------------------------------
void    TExportTracks::Write ( const TArray3<double>& values )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


for ( long tf   = 0; tf   < NumTime;         tf++   )
for ( int  e    = 0; e    < NumTracks;       e++    ) {

    UpdateApplication;

    for ( int freq = 0; freq < NumFrequencies ; freq++ )

        Write ( values ( e, tf, freq ), tf, e, freq );
    }


End ();
}


//----------------------------------------------------------------------------
void    TExportTracks::Write ( const TArray2<double>& values, ExportTracksTransposed transpose )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


for ( long tf = 0; tf < NumTime; tf++ ) {

    UpdateApplication;

    for ( int el = 0; el < NumTracks * NumFiles; el++ )

        if ( transpose )    Write ( values ( el, tf ) );
        else                Write ( values ( tf, el ) );
    }


End ();
}


//----------------------------------------------------------------------------
void    TExportTracks::Write ( const TArray2<bool>& values, ExportTracksTransposed transpose )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


for ( long tf = 0; tf < NumTime; tf++ ) {

    UpdateApplication;

    for ( int el = 0; el < NumTracks * NumFiles; el++ )

        if ( transpose )    Write ( (long) values ( el, tf ) );
        else                Write ( (long) values ( tf, el ) );
    }


End ();
}


//----------------------------------------------------------------------------
void    TExportTracks::Write ( const TArray2<long>& values, ExportTracksTransposed transpose )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();


for ( long tf = 0; tf < NumTime; tf++ ) {

    UpdateApplication;

    for ( int el = 0; el < NumTracks * NumFiles; el++ )

        if ( transpose )    Write ( values ( el, tf ) );
        else                Write ( values ( tf, el ) );
    }


End ();
}


//----------------------------------------------------------------------------
                                        // This version not fully tested
void    TExportTracks::Write ( const TMaps& maps )
{
if ( ! DoneBegin )                      // remove the hassle from the caller
    Begin ();

                                        // check we are really good to save vectorial data
bool                savevector      = IsVector ( AtomTypeUseOriginal ) && NumTracks == maps.GetDimension () / 3;

                                        // assume type and output dimensions are being set OK...
//if ( savevector ) {
//    SetAtomType ( AtomTypeVector );
//    NumTracks           = maps.GetDimension () / 3;    // actual number of tracks
//    }
//else {
//    SetAtomType ( AtomTypeScalar );
//    NumTracks           = maps.GetDimension ();
//    }


for ( long tf = 0; tf < NumTime; tf++) {

    UpdateApplication;


    if ( savevector )

        for ( int el = 0, el3 = 0; el < NumTracks; el++, el3+=3 )

            Write ( TVector3Float ( maps ( tf, el3     ),
                                    maps ( tf, el3 + 1 ),
                                    maps ( tf, el3 + 2 ) ) );
    else // Scalar

        Write ( maps[ tf ] );

//      for ( int el = 0; el < NumTracks; el++)
//
//          Write ( maps ( tf, el ) );
    }


End ();
}


//----------------------------------------------------------------------------
                                        // Input parameters:
                                        // EEGDoc*, TimeMin, TimeMax, Type (option), Filename (option)
                                        // Processing is either interval or markers mode
void    TExportTracks::Write ( TTracksDoc* EEGDoc, const TMarkers* keeplist )
{
if ( EEGDoc == 0 )
    return;


long                numtimeframes   = EEGDoc->GetNumTimeFrames ();

                                        // doesn't check that the interval is big enough to contain all markers
                                        // however, each marker will be tested for inclusion
long                timemin         = TimeMin;   
long                timemax         = TimeMax;
                                        // make sure in case of triggerlist that the buffer will load all data
//long                timemin         = keeplist ? min ( (long) TimeMin, keeplist->GetMinPosition () ) : TimeMin;     // GetMinPosition could return MAXLONG
//long                timemax         = keeplist ? max ( (long) TimeMax, keeplist->GetMaxPosition () ) : TimeMax;     // GetMaxPosition could return -MAXLONG

                                        // check time range - also, don't modify input parameters, in case the same object is used to process many files...
Clipped ( timemin, timemax, (long) 0, numtimeframes - 1 );

                                        // actual number of time frames
long                deltatime       = timemax - timemin + 1;

                                        // compute / update number of time frames
NumTime     = keeplist ? keeplist->GetMarkersTotalLength () : deltatime;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // no filename provided? create one
if ( Filename.IsEmpty () ) {
                                        // choose extension myself
//  bool                saveeph         = EEGDoc->GetSamplingFrequency () > 0;

    if ( Type == ExportTracksUnknown )
        Type    = ExportTracksDefault;  // nope, opt for .sef all the time
//      if ( saveeph )  Type = ExportTracksEph;
//      else            Type = ExportTracksEp;


                                        // build an infix according to crop and/or gfp peaks
    TFileName           infix;
    TFileName           oldname;


    ClearString ( infix );
                    StringCopy      ( infix, ".", IntegerToString ( timemin, NumIntegerDigits ( timemax ) ), "_", IntegerToString ( timemax, NumIntegerDigits ( timemax ) ) );
    if ( keeplist ) StringAppend    ( infix, "." "Markers" );


    StringCopy      ( oldname, EEGDoc->GetDocPath () );
    RemoveExtension ( oldname );

                                        // Filename can then be retrieved upon completion
    StringCopy  ( Filename, oldname, infix, ".", GetExtension () );
    }
//else                                    // assume filename is correct, including file type
//    saveeph = saveeph && IsExtension ( Filename, FILEEXT_EEGEPH );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Read the whole interval at once, so to make sure all filters are applied at once
                                        // Also to make it on-par with  MaxTrackToMarkers  function, that reads the interval block
                                        // !Use Maps instead, writing would be faster!
TTracks<float>      EegBuff ( EEGDoc->GetTotalElectrodes (), deltatime );

                                        // load whole interval at once
EEGDoc->GetTracks   (   timemin,            timemax, 
                        EegBuff,            0, 
                        AtomTypeUseCurrent, 
                        NoPseudoTracks,
                        ReferenceUsingCurrent
                    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( keeplist ) {

    const TMarker*      tomarker;
                                        // n intervals
                                        // Overlapping is not tested here, so it technically can save non-ordered and/or overlapping epochs
    for ( int ki = 0; ki < (int) (*keeplist); ki++ ) {

        tomarker   = (*keeplist)[ ki ];

                                        // skip markers not inside limit
        if ( ! tomarker->IsInsideLimits ( TimeMin, TimeMax ) )
            continue;


        for ( long tf = tomarker->From, tf0 = tf - timemin; tf <= tomarker->To; tf++, tf0++ ) {

            UpdateApplication;

            for ( int el = 0; el < NumTracks; el++)
                Write ( EegBuff[ el ][ tf0 ] );
            }

        } // for keeplist

    } // keeplist

else {

    for ( long tf0 = 0; tf0 < deltatime; tf0++ ) {

        UpdateApplication;

        for ( int el = 0; el < NumTracks; el++)
            Write ( EegBuff[ el ][ tf0 ] );
        }
    }


End ();
}


//----------------------------------------------------------------------------
                                        // Input parameters:
                                        // EEGDoc*, TimeMin, TimeMax, Type (option), Filename (option)
                                        // Processing is either interval or markers mode
void    TExportTracks::Write ( TRisDoc* RISDoc, int reg, const TMarkers* keeplist )
{
if ( RISDoc == 0 )
    return;


long                numtimeframes   = RISDoc->GetNumTimeFrames ();

                                        // doesn't check that the interval is big enough to contain all markers
                                        // however, each marker will be tested for inclusion
long                timemin         = TimeMin;   
long                timemax         = TimeMax;
                                        // make sure in case of triggerlist that the buffer will load all data
//long                timemin         = keeplist ? min ( (long) TimeMin, keeplist->GetMinPosition () ) : TimeMin;     // GetMinPosition could return MAXLONG
//long                timemax         = keeplist ? max ( (long) TimeMax, keeplist->GetMaxPosition () ) : TimeMax;     // GetMaxPosition could return -MAXLONG

                                        // check time range - also, don't modify input parameters, in case the same object is used to process many files...
Clipped ( timemin, timemax, (long) 0, numtimeframes - 1 );

                                        // actual number of time frames
long                deltatime       = timemax - timemin + 1;

                                        // compute / update number of time frames
NumTime     = keeplist ? keeplist->GetMarkersTotalLength () : deltatime;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // no filename provided? create one
if ( Filename.IsEmpty () ) {

    if ( Type == ExportTracksUnknown )
        Type    = ExportTracksRis;

                                        // build an infix according to crop and/or gfp peaks
    TFileName           infix;
    TFileName           oldname;


    ClearString ( infix );
                    StringCopy      ( infix, ".", IntegerToString ( timemin, NumIntegerDigits ( timemax ) ), "_", IntegerToString ( timemax, NumIntegerDigits ( timemax ) ) );
    if ( keeplist ) StringAppend    ( infix, ".Markers" );


    StringCopy      ( oldname, RISDoc->GetDocPath () );
    RemoveExtension ( oldname );

                                        // Filename can then be retrieved upon completion
    StringCopy  ( Filename, oldname, infix, ".", GetExtension () );
    }
//else                                    // assume filename is correct, including file type
//    saveeph = saveeph && IsExtension ( Filename, FILEEXT_EEGEPH );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TArray1<TVector3Float>  InvBuff ( RISDoc->GetNumElectrodes () );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( keeplist ) {

    const TMarker*      tomarker;
                                        // n intervals
                                        // Overlapping is not tested here, so it technically can save non-ordered and/or overlapping epochs
    for ( int ki = 0; ki < (int) (*keeplist); ki++ ) {

        tomarker   = (*keeplist)[ ki ];

                                        // skip markers not inside limit
        if ( ! tomarker->IsInsideLimits ( TimeMin, TimeMax ) )
            continue;


        for ( long tf = tomarker->From; tf <= tomarker->To; tf++ ) {

            UpdateApplication;

            RISDoc->GetInvSol ( reg, tf, tf, InvBuff, 0, 0 );

            for ( int el = 0; el < NumTracks; el++)
                                        // will either write a TVector3Float or a float, according to file type
                Write ( InvBuff[ el ] );
            }

        } // for keeplist

    } // keeplist

else {

    for ( long tf0 = 0; tf0 < deltatime; tf0++ ) {

        UpdateApplication;

        RISDoc->GetInvSol ( reg, tf0, tf0, InvBuff, 0, 0 );

        for ( int el = 0; el < NumTracks; el++)
                                        // will either write a TVector3Float or a float, according to file type
            Write ( InvBuff[ el ] );
        }
    }


End ();
}


//----------------------------------------------------------------------------
                                        // input parameters: file, TimeMin, TimeMax, Type (option), Filename (option)
void    TExportTracks::Write ( const char* file, const TMarkers* keeplist )
{
TOpenDoc< TTracksDoc >  EEGDoc ( file, OpenDocHidden );

Write ( EEGDoc, keeplist );

EEGDoc.Close ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
