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

#pragma once

#include    "Time.TDateTime.h"
#include    "TLimits.h"
#include    "TSelection.h"
#include    "Strings.TSplitStrings.h"
#include    "TArray2.h"
#include    "TSetArray2.h"
#include    "Files.Stream.h"

#include    "TRois.h"
#include    "TTracksFilters.h"

#include    "TMarkers.h"
#include    "TBaseDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr int       ElectrodeNameSize                   = 32;

                                        // Names of additional tracks
#define             TrackNameGFP                        "GFP"
#define             TrackNameRMS                        "RMS"
#define             TrackNameDIS                        "Dis"
#define             TrackNameDISPlus                    "Dis+"
#define             TrackNameAVG                        "Avg"

                                        // Computed and added to EEG tracks (Dis+ and RMS not used anymore)
#define             ComputedTracksNames                 TrackNameGFP " " TrackNameDIS " " TrackNameAVG " " TrackNameDISPlus " " TrackNameRMS
                                        // Recognized auxiliary channels SUBSTRINGS - a lot from sleep recording setups (removed: TEMP)
constexpr char*     AuxiliaryTracksNames                = ComputedTracksNames" ECG EKG EOG EMG SLI VERT HOR AUX STATUS EYE RESP HYPNO AIRFLOW FLOW CHEST ABDOMEN THORAX CALIB EX EXG GSR ERG RES RESP PLET VBAT ANA PUL MAX EAR PNG DC PULS BEAT MKR PCPAP SAO2 PART ZEIT TIME SNORE SCHNARCHEN";
                                        // Channels always NON-auxiliaries
constexpr char*     NonAuxiliaryTracksNames             = "VREF REF CZ tempo";

constexpr char*     TracksPreferenceNoAuxs              = "No Auxiliaries";

                                        // Tracks have currently 3 additional tracks
enum                PseudoTracksIndex
                    {
                    PseudoTrackOffsetGfp,
                    PseudoTrackOffsetDis,
                    PseudoTrackOffsetAvg,

                    NumPseudoTracks,
                    };


//----------------------------------------------------------------------------
                                        // max number of points to be displayed
constexpr int       EegMaxPointsDisplay                 = 15 * 60 * 1000;   // 15 minutes recording @ 1000Hz - memory can sustain much more, but file access delays are more of a concern

constexpr int       EegNumPointsWideDisplay             = 3000;             // how big a file must be until considered a recording / raw file


//----------------------------------------------------------------------------
                                        // Specializing content types
enum                TracksContentType
                    {
                    TracksContentUnknown,
                    TracksContentEEGRecording,      // raw EEG
                    TracksContentERP,               // averaged ERP
                    TracksContentFrequency,         // 
                    TracksContentTemplates,         // set of template maps
                    TracksContentCollection,        // Cut&Paste into a file
                    TracksContentPValues,           // Statistical results
                    TracksContentSpectrum,          // FFT results
                    TracksContentSpecialInfix,      // SpecialFilesInfix (.t .p .sd etc...)

                    NumTracksContentTypes
                    };

extern const char   TracksContentNames[ NumTracksContentTypes ][ ContentTypeMaxChars ];
                                        // All possible dimensions
                                        // Used f.ex. to refine the "time" display
enum                DimensionType
                    {
                    DimensionTypeUnknown,
                    DimensionTypeTime,          // continuous time
                    DimensionTypeWindow,        // continuous windows from FFT
                    DimensionTypeTemplate,      // templates from segmentation, no continuous time
                    DimensionTypeValue,         // generic, continuous values
                    DimensionTypeCollection,    // stuff put there, no continuous time
                    DimensionTypeSegmentation,  // segmentation #
                    DimensionTypeFrequency,     //

//                  Electrodes          // for dimensions 1 and 2
//                  SolPoints
//                  Tracks      .seg  .error.data
//                  Variable    .fit.p.data
//                  File        .seg

                    NumDimensionTypes
                    };

extern const char   DimensionNames[ NumDimensionTypes ][ ContentTypeMaxChars ];


//----------------------------------------------------------------------------

static  TSplitStrings   AllAuxNames    ( AuxiliaryTracksNames,    UniqueStrings, " " );
static  TSplitStrings   AllNonAuxNames ( NonAuxiliaryTracksNames, UniqueStrings, " " );

                                        // !Case insensitive, and partial matches for aux!
inline  bool    IsElectrodeNameAux ( const char* elname )
{
if      ( AllNonAuxNames.Contains           ( elname ) )    return false;   // 'white list' of non-aux names has overriding priority
else if ( AllAuxNames   .PartiallyContains  ( elname ) )    return true;    // 'black list' of PARTIAL aux names. F.ex. "EOG1" or "LEOG" electrodes will both match to aux list "EOG"
else                                                        return false;   // not in either list, default is 'not an aux'

                                        // problem with this is creating some TSplitStrings at each call (at least for each electrode f.ex.!)
//if      ( IsStringAmong ( elname, NonAuxiliaryTracksNames ) )   return false;
//else if ( IsStringAmong ( elname, AuxiliaryTracksNames    ) )   return true;
//else                                                            return false;   // don't know
}


//----------------------------------------------------------------------------
                                        // TTracksDoc defines the functionalities of all tracks (EEG, RIS, results...) documents
                                        // The minimum number of fields has been included here

class   TTracksDoc  :   public  TBaseDoc,
                        public  TMarkers,
                        public  TLimits
{
public:
                    TTracksDoc ( owl::TDocument *parent = 0 );


    TDateTime       DateTime;

                                        // owl::TDocument
    bool            InitDoc         ();
    bool            IsDirty         ();
    bool            Revert          ( bool force = false );
    bool            Commit          ( bool force = false );
    bool            CanClose        ()                          { return  TBaseDoc::CanClose ( true   ); }  // force it to be silent, so that filtered (or any other edit) EEG will not request from user
    bool            CanClose        ( bool silent )             { return  TBaseDoc::CanClose ( silent ); }

                                        // TLimits
    void            InitLimits      ( bool precise = false );
    void            InitContentType ();
//  void            InitAtomType    ();
    void            InitReference   ();
    void            InitFilters     ();
    virtual void    InitDateTime    ();
    void            InitAuxiliaries ();


    bool            IsSpontaneous   ()                  const   { return IsExtraContentType ( TracksContentEEGRecording ); }
    bool            IsErp           ()                  const   { return IsExtraContentType ( TracksContentERP          ); }
    bool            IsTemplates     ()                  const   { return IsExtraContentType ( TracksContentTemplates    ); }
    bool            IsCollection    ()                  const   { return IsExtraContentType ( TracksContentCollection   ); }
    bool            IsP             ()                  const   { return IsExtraContentType ( TracksContentPValues      ); }
    bool            IsSpectrum      ()                  const   { return IsExtraContentType ( TracksContentSpectrum     ); }
    bool            IsSpecialInfix  ()                  const   { return IsExtraContentType ( TracksContentSpecialInfix ); }

                                        // Reference
    virtual ReferenceType       GetReferenceType    ()  const   { return Reference; }
    virtual const TSelection&   GetReferenceTracks  ()  const   { return ReferenceTracks; }
    virtual void                SetReferenceType    ( ReferenceType ref, const char* tracks = 0, const TStrings* elnames = 0, bool verbose = true );

                                        // Time and frequency
    virtual double  GetSamplingFrequency    ()          const   { return SamplingFrequency; }
    virtual void    SetSamplingFrequency    ( double sf )       { Maxed ( sf, 0.0 ); if ( sf != SamplingFrequency ) { SamplingFrequency = sf; DirtySamplingFrequency = true; } }  // !caller is accountable for correct check & call!
    virtual long    GetNumTimeFrames        ()          const   { return NumTimeFrames;          }
    virtual long    GetStartingTimeFrame    ()          const   { return StartingTimeFrame;      }
    virtual long    AbsToRelTime            ( long tf ) const   { return tf - StartingTimeFrame; }
    virtual long    RelToAbsTime            ( long tf ) const   { return tf + StartingTimeFrame; }
    int             GetDim2Type             ()          const   { return Dim2Type; }
    const char*     GetDim2TypeName         ()          const;

                                        // Channels / electrodes precise desciption
    virtual int     GetTotalElectrodes      ()          const   { return TotalElectrodes;                       }   // Grand total of all channels          Total       = Electrodes + Pseudos      = Valids + Bads + Auxs + Pseudos
    virtual int     GetNumElectrodes        ()          const   { return NumElectrodes;                         }   // Number of channels in file           Electrodes  = Regulars + Auxs           = Valids + Bads + Auxs
    virtual int     GetNumRegularElectrodes ()          const   { return NumElectrodes - AuxTracks.NumSet ();   }   // Number of channels without the auxs  Regulars    = Valids   + Bads
    virtual int     GetNumValidElectrodes   ()          const   { return ValidTracks.NumSet ();                 }   // Number of valid channels             Valids      = Electrodes - Bads - Auxs
    virtual int     GetNumBadElectrodes     ()          const   { return BadTracks  .NumSet ();                 }   // Number of bad channels
    virtual int     GetNumAuxElectrodes     ()          const   { return AuxTracks  .NumSet ();                 }   // Number of auxiliary channels
    bool            HasPseudoElectrodes     ()          const   { return TotalElectrodes > NumElectrodes;       }   // Not all files have the pseudo tracks
    virtual int     GetNumPseudoElectrodes  ()          const   { return TotalElectrodes - NumElectrodes;       }   // Number of tracks added by Cartool
                                        // Wrapping all sorts of handy channel indexes functions
    virtual int     GetFirstRegularIndex    ()          const   { return 0;                 }
    virtual int     GetLastRegularIndex     ()          const   { return NumElectrodes - 1; }
    virtual int     GetFirstPseudoIndex     ()          const   { return HasPseudoElectrodes () ? NumElectrodes       : -1; }
    virtual int     GetLastPseudoIndex      ()          const   { return HasPseudoElectrodes () ? TotalElectrodes - 1 : -1; }
    virtual int     GetGfpIndex             ()          const   { return OffGfp; }
    virtual int     GetDisIndex             ()          const   { return OffDis; }
    virtual int     GetAvgIndex             ()          const   { return OffAvg; }

    int             GetNumSelectedRegular ( const TSelection& sel ) const   { return sel.NumSet ( GetFirstRegularIndex(), GetLastRegularIndex() ); }
    int             GetNumSelectedPseudo  ( const TSelection& sel ) const   { return HasPseudoElectrodes () ? sel.NumSet ( GetFirstPseudoIndex(),  GetLastPseudoIndex()  ) : 0; }

    void            SetRegular      ( TSelection& sel ) const   { sel.Set   ( GetFirstRegularIndex(), GetLastRegularIndex() ); }
    void            ClearRegular    ( TSelection& sel ) const   { sel.Reset ( GetFirstRegularIndex(), GetLastRegularIndex() ); }
    void            SetPseudo       ( TSelection& sel ) const   { if ( HasPseudoElectrodes () ) sel.Set   ( GetFirstPseudoIndex(),  GetLastPseudoIndex()  ); }
    void            ClearPseudo     ( TSelection& sel ) const   { if ( HasPseudoElectrodes () ) sel.Reset ( GetFirstPseudoIndex(),  GetLastPseudoIndex()  ); }
    void            SetAuxs         ( TSelection& sel ) const   { sel  += AuxTracks; }
    void            ClearAuxs       ( TSelection& sel ) const   { sel  -= AuxTracks; }
    void            SetBads         ( TSelection& sel ) const   { sel  += BadTracks; }
    void            ClearBads       ( TSelection& sel ) const   { sel  -= BadTracks; }

    virtual const TSelection&   GetBadTracks    ()      const   { return BadTracks; }
    virtual void                SetBadTracks    ( TSelection *bad, bool notify = true );
    virtual const TSelection&   GetAuxTracks    ()      const   { return AuxTracks; }
    virtual void                SetAuxTracks    ( TSelection *aux, bool notify = true );
    virtual void                ResetAuxTracks  ();
    virtual const TSelection&   GetValidTracks  ()      const   { return ValidTracks; }
    virtual void                SetValidTracks  ();      // Update ValidTracks from BadTracks and AuxTracks


    virtual const char*         GetElectrodeName    ( int e = 0 )   const   { return  ElectrodesNames[ e ]; }
    virtual const TStrings*     GetElectrodesNames  ()              const   { return &ElectrodesNames;      }

                                        // Recordings can store multiple blocks / sessions within the same file(!)
    int             GetNumSessions      ()              const   { return NumSequences; }
    virtual int     GetCurrentSession   ()              const;  // 0 if only one session, otherwise 1..3 for 3 sessions
    virtual void    GoToSession         ( int newsession = 0 );
    virtual bool    UpdateSession       ( int newsession )      { return true; }
    virtual void    UpdateTitle         ();
    char*           GetBaseFileName     ( char* basefilename )  const;  // the base name to be used for any processing ouput (removing extension, caring for mff directory...)

                                        // Reading the RAW TRACKS directly from the file
    virtual void    ReadRawTracks       ( long tf1, long tf2, TArray2<float> &buff, int tfoffset = 0 )    = 0;

                                        // Retrieving tracks with optional filter / re-reference / pseudo-tracks / ROIs computation
    virtual void    GetTracks           ( long tf1, long tf2, TArray2<float> &buff, int tfoffset = 0, AtomType atomtype = AtomTypeUseCurrent, PseudoTracksType pseudotracks = NoPseudoTracks, ReferenceType reference = ReferenceAsInFile, TSelection* referencetracks = 0, TRois *rois = 0 );


    virtual bool    IsStandDevAvail     ()                      { return false; }
    virtual void    GetStandDev         ( long tf1, long tf2, TArray2<float> &buff, int tfoffset = 0, TRois *rois = 0 )     {}


    virtual bool    CanFilter           ()              const   { return true; }                                                // can / will be overriden by files where filtering is not allowed
    virtual bool    SetFilters          ( const TTracksFilters<float>* filters, const char* xyzfile = 0, bool silent = false ); // 1) set the parameters of the filtering first
    virtual bool    AreFiltersActivated ()              const   { return FiltersActivated; }                                    // true if currently filtering
    virtual bool    SetFiltersActivated ( bool activate, bool silent = false );                                                 // 2) then activate / deactivate the actual filtering - it also returns the previous state
            bool    ActivateFilters     ( bool silent = false ) { return SetFiltersActivated ( true,  silent ); }               // just handy aliases
            bool    DeactivateFilters   ( bool silent = false ) { return SetFiltersActivated ( false, silent ); }
    const   TTracksFilters<float>*  GetFilters ()       const   { return &Filters; }
            TTracksFilters<float>*  GetFilters ()               { return &Filters; }


    static bool     ReadFromHeader      ( const char *file, ReadFromHeaderType what, void* answer );    // fall-back for derived classes


protected:

    TFileStream     FileStream;         // Wrapper to some low-level file access (currently used only in TEegCartoolSefDoc for faster R/W)

                                        // Typology of tracks
    ReferenceType&  Reference           = Filters.Reference;        // now stored in Filters - we use some "aliases" to (temporarily) keep the code as is
    TSelection&     ReferenceTracks     = Filters.ReferenceTracks;

    TSelection      BadTracks;          // Broken channels    : channels that dit not pick up signal correctly during recording
    TSelection      AuxTracks;          // Auxiliary channels : channels with non-eeg data - Note that BadTracks and AuxTracks should not have any common elements
    TSelection      ValidTracks;        // All good channels  : NOT Bad, NOT Auxiliaries and NOT Pseudo-Tracks

                                        // Tracks parameters
    long            NumTimeFrames;      // Time Frames will be in the [0 .. NumTimeFrames - 1] range
    long            StartingTimeFrame;  // first "TF" value: 0 for actual EEG, or some Template index for segmentation f.ex.
    int             Dim2Type;           // specifies first dimension

    int             NumElectrodes;      // = Valids + Bads + Auxs
    int             TotalElectrodes;    // = NumElectrodes + optional Pseudos (Gfp, Dis, Avg)

    double          SamplingFrequency;  // in Hertz

    int             NumSequences;       // number of sequences / blocks / epochs / sessions
    int             CurrSequence;       // and the current one, in [0..NumSequences-1]

    TStrings        ElectrodesNames;    // tracks names


    int             OffGfp;             // constant offset in buffers to access these pseudo-tracks
    int             OffDis;
    int             OffAvg;

    TSetArray2<float>   BuffDiss;       // dissimilarity computation needs a 1 TF buffer

    TTracksFilters<float>   Filters;            // encapsulates all legal tracks filters
    bool                    FiltersActivated;   // applying or not said filters

    int             NumInverseSolutions;

                                                       // ignore No Reference and Average Reference, which is quite common
    bool            DirtyReference ()       { return  ! ( Reference == ReferenceAsInFile || Reference == ReferenceAverage ); }
//  bool            DirtyBadTracks ()       { return  BadTracks.NumSet () > 0; }    // it rather needs a bool flag to track any change
//  bool            DirtyAuxTracks ()       { return  AuxTracks.NumSet () > 0; }
    bool            DirtySamplingFrequency;

                                        
    virtual bool    SetArrays               () = 0;     // Initializing arrays at opening time
    virtual void    ElectrodesNamesCleanUp  ();
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
