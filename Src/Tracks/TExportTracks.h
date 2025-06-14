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

#pragma once

#include    "Strings.TStrings.h"
#include    "Files.Utils.h"
#include    "Time.TDateTime.h"
#include    "TMarkers.h"

#include    "TFreqDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template <class TypeD> class        TArray1;
template <class TypeD> class        TArray2;
template <class TypeD> class        TSetArray2;
template <class TypeD> class        TVector;


//----------------------------------------------------------------------------

class           TTracksDoc;

                                        // Actual output file types - Note that some file extensions can resolve into one of these
enum            ExportTracksType
                {
                ExportTracksUnknown,
                ExportTracksEp,
                ExportTracksEph,
                ExportTracksData,
                ExportTracksSeg,
                ExportTracksSef,
                ExportTracksBv,
                ExportTracksEdf,
                ExportTracksRis,
                ExportTracksFreq,

                NumExportTracksType,
                ExportTracksDefault     = ExportTracksBv
                };


class           ExportTracksTypeSpec
{
public:
    ExportTracksType        Code;                   // redundant but included for clarity
    const char              Ext [ 16 ];             // extension
    bool                    Binary;                 // binary file
    bool                    NativeTriggers;         // format integrates triggers natively
};

extern  const ExportTracksTypeSpec  ExportTracksTypes[ NumExportTracksType ];

ExportTracksType    GetExportType ( const char* file, bool isext );


//----------------------------------------------------------------------------

enum            ExportTriggers
                {
                NoTriggersNoMarkers = 0x00,

                NoTriggers          = 0x00,
                TriggersAsTriggers  = 0x01,
                TriggersAsMarkers   = 0x02,
                TriggersMask        = TriggersAsTriggers | TriggersAsMarkers,

                NoMarkers           = 0x00,
                MarkersAsMarkers    = 0x10,
                MarkersAsTriggers   = 0x20,
                MarkersMask         = MarkersAsMarkers | MarkersAsTriggers,

                AnyTriggers         = TriggersAsTriggers | MarkersAsTriggers,
                AnyMarkers          = TriggersAsMarkers  | MarkersAsMarkers,
                TriggersMarkers     = TriggersAsTriggers | MarkersAsMarkers,
                };


enum            ExportTracksTransposed
                {
                NotTransposed,
                Transposed,
                };


enum            AnimationEnum
                {
                NoAnimation,
                ShowAnimation,

                Silently    = NoAnimation,
                Verbosy     = ShowAnimation,
                };


//----------------------------------------------------------------------------
                                        // EDF file format needs some extra care
constexpr double    EdfPhysicalMaxDefault   = 100;      // Default physical max
constexpr double    EdfPhysicalMaxMargin    = 5;        // Boosting the max value with this factor, as the given max might be approximate
constexpr int       EdfDigitalMax           = 0x77FF;   // Digital max value, a little less than 0x7FFF / SHRT_MAX


//----------------------------------------------------------------------------
                                        // Centralized class to save tracks to file

class   TExportTracks :   public  TDataFormat
{
public:
                    TExportTracks ();
                   ~TExportTracks ();

                                        // most fields have to be filled
    TFileName           Filename;
    ExportTracksType    Type;

    int             NumTracks;          // mandatory for EP / EPH / RIS
    long            NumTime;
    double          SamplingFrequency;

    int             NumFiles;           // mandatory for DATA
    TStrings        VariableNames;
    int             NumClusters;        // mandatory for SEG

    int             NumAuxTracks;       // mandatory for SEF
    TDateTime       DateTime;
    TSelection      SelTracks;          // ignored if not redefined
    TSelection      AuxTracks;          // ignored if not redefined
    TStrings        ElectrodesNames;    // can be missing

//  TPoints         ElectrodesCoordinates;  // could be needed for Brain Vision .eeg

    double          MaxValue;           // mandatory for EDF

    ExportTriggers  OutputTriggers;     // Controls how the markers will be outputted as Triggers or markers
    TMarkers        Markers;            // local list of triggers / markers to output
    int             NumTags;
    long            TimeMin;
    long            TimeMax;

    int             NumFrequencies;     // for FREQ
    char            FreqTypeName[ MaxCharFreqType ];
    double          BlockFrequency;
    TStrings        FrequencyNames;

                                        // Either create a new object each time: calling Write will do the all the job
                                        // Or use 1 object multiple times, then call in sequence: Reset, Begin, Write, End
    void            Reset               ();
    bool            IsOpen              ()              const   { return  of != 0; }
    bool            IsFileBinary        ()              const   { return    ExportTracksTypes[ Type ].Binary; }
    bool            IsFileTextual       ()              const   { return  ! ExportTracksTypes[ Type ].Binary; }
    bool            HasNativeTriggers   ()              const   { return  ExportTracksTypes[ Type ].NativeTriggers; }
    const char*     GetExtension        ()              const   { return  ExportTracksTypes[ Type ].Ext; }

    void            CloneParameters ( const char* ext, const char* file1, const char* file2 = 0 );
                                        // Overriding from TDataFormat - force assignation all the time, as the object could be re-used multiple times
                                        // RIS: AtomTypeScalar or AtomTypeVector; FREQ: AtomTypeScalar or AtomTypeComplex
    void            SetAtomType     ( AtomType at )     final   { OriginalAtomType = CurrentAtomType = at; }


    void            Begin           ( bool dummyheader = false );               // checks, create stream, call write header - Called automatically
    void            End             ();                                         // close stream & stuff - Called automatically
    void            WriteHeader     ( bool overwrite   = false );               // separate the function, in case we need to call it more than once (overwrite/update)
    void            WriteTriggers   ();                                         // writing native triggers
    void            WriteMarkers    ();                                         // writing markers (mrk file)

                                        // Tracks files
    void            Write ( float                   value );                    // write 1 value to file, handling all cases (text/binary, EOL, etc..)
    void            Write ( double                  value );                    // same as above, just handling the text output with more precision
    void            Write ( long                    value );
    void            Write ( float                   value, long t, int e );     // !Does not update CurrentPositionTrack nor CurrentPositionTime -> caller handles the loops & indexes!
    void            Write ( const TVector3Float&    vector );                   // mostly to write vectorial ris files
    void            Write ( const TVector3Float&    vector, long t, int e );    // mostly to write vectorial ris files

                                        // Writing full 1D array at once
    void            Write ( const TArray1<float>&   values );                   // write full array at once, in sequence, without caring for output dimension (1xn or nx1 f.ex.)
    void            Write ( const TArray1<double>&  values );                   // write full array at once, in sequence, without caring for output dimension (1xn or nx1 f.ex.)
    void            Write ( const TArray1<long>&    values );                   // write full array at once, in sequence, without caring for output dimension (1xn or nx1 f.ex.)
    void            Write ( const TArray1<TVector3Float>&   vec3 );             // write full array at once, in sequence, without caring for output dimension (1xn or nx1 f.ex.)
    void            Write ( const TMap&             map    );                   // write whole map at once

                                        // Writing full 2D array at once
    void            Write ( const TArray2<float>&   values, const TMarkers* keeplist,         AnimationEnum animation = NoAnimation );
    void            Write ( const TArray2<float>&   values, ExportTracksTransposed transpose, AnimationEnum animation = NoAnimation );
    void            Write ( const TArray2<double>&  values, ExportTracksTransposed transpose, AnimationEnum animation = NoAnimation );
    void            Write ( const TArray2<bool>&    values, ExportTracksTransposed transpose, AnimationEnum animation = NoAnimation );
    void            Write ( const TArray2<long>&    values, ExportTracksTransposed transpose, AnimationEnum animation = NoAnimation );
    void            Write ( const TMaps& maps,                                                AnimationEnum animation = NoAnimation );

                                        // Writing full EEG / RIS Doc at once
    void            Write (       TTracksDoc*       EEGDoc,     const TMarkers* keeplist = 0 );             // write & crop an EEG Doc
    void            Write ( const TRisDoc*          RISDoc,     int reg,    const TMarkers* keeplist = 0 ); // write & crop an RIS Doc
    void            Write ( const char*             file,       const TMarkers* keeplist = 0 );             // wrapper to function above

                                        // !Frequency files only!
    void            Write ( float value,                    long t, int e, int f ); // FREQ only! Does not update CurrentPositionTrack nor CurrentPositionTime -> caller handles the loops & indexes!
    void            Write ( std::complex<float> comp,       long t, int e, int f );
    void            Write ( float realpart, float imagpart, long t, int e, int f );
    void            Write ( const TVector<float>        v,  long t,        int f );
    void            Write ( const TSetArray2<float>&    values,                                   AnimationEnum animation = NoAnimation );  // FREQ only! write a full array at once
    void            Write ( const TArray3<float>&       values, ExportTracksTransposed transpose, AnimationEnum animation = NoAnimation );
    void            Write ( const TArray3<double>&      values,                                   AnimationEnum animation = NoAnimation );

                                                            // Copying all variables, as well as the ofstream* which is still debatable
    TExportTracks                           ( const TExportTracks &op  );   // copy constructor
    TExportTracks&  operator    =           ( const TExportTracks &op2 );   // assignation operator

                    operator    std::ofstream&   ()     { return  *of; }
    
protected:

    std::ofstream*  of;
    int             CurrentPositionTrack;
    long            CurrentPositionTime;
    bool            DoneBegin;
    LONGLONG        EndOfHeader;

                                        // Used for convenient Edf generation
    LONGLONG        EdfDataOrg;
    long            EdfBlockSize;
    long            EdfTrailingTF;
    long            EdfNumRecords;
    long            EdfTfPerRec;
    double          EdfPhysicalMin;
    double          EdfDigitalMin;
    double          EdfRatio;
    short           EdfValue;


    bool            OpenStream  ( bool reopen = false );        // open stream - Called automatically
    void            CloseStream ();                             // close stream - Called automatically
    void            PreFillFile ();

    const char*     GetElectrodeName ( int i, char *name, int maxlen );
    const char*     GetFrequencyName ( int i, char *name, int maxlen );

    void            WriteBrainVisionMarkerFile ( const char* fileoutmrk, const char* filenameout );
    inline LONGLONG EDFseekp ( long tf, long e );

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
