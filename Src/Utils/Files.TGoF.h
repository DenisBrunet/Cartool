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

#include    <owl\wsyscls.h>

#include    "Files.Utils.h"
#include    "Strings.Utils.h"
#include    "Strings.TStrings.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

enum                CompatibilityFlags
                    {
                    CompatibilityIrrelevant     = -2,   // variable does not exist / can not be retrieved
                    CompatibilityNotConsistent  = -1,   // variable WAS retrieved, but not consistent across all files
                    CompatibilityConsistent     =  0,   // variable exists and is consistent across all files - this is actually the minimum value for a variable
                 // values >= 0                         // are considered retrieved and consistent across group
                    };


enum                CopyToFlags
                    {
                    CopyAndDeleteOriginals      = 0x001,
                    CopyAndKeepOriginals        = 0x002,

                    CopyAndUpdateFileNames      = 0x010,
                    CopyAndPreserveFileNames    = 0x020,

                    CopyAllKnownBuddies         = 0x100,
                    };


enum                SplitFreqFlags
                    {
                    SplitFreqByFrequency    = 1,
                    SplitFreqByElectrode,
                    SplitFreqByTime,        // aka SplitFreqBySpectrum
                    };


//----------------------------------------------------------------------------
                                        // Useful classes to group all the polled dimensions
class   TracksCompatibleClass
{
public:
                    TracksCompatibleClass ()        { Reset (); }

    int             NumTracks;
    int             NumAuxTracks;
    int             NumSolPoints;
    int             NumTF;
    int             NumFreqs;
    double          SamplingFrequency;

    void            Reset ();
    void            Show            ( const char* title = 0 )   const;
};


class   FreqsCompatibleClass
{
public:
                    FreqsCompatibleClass ()         { Reset (); }

    int             FreqType;
    int             NumTracks;
    int             NumTF;
    int             NumFreqs;
    double          SamplingFrequency;
    double          OriginalSamplingFrequency;

    void            Reset ();
};


class   ElectrodesCompatibleClass
{
public:
                    ElectrodesCompatibleClass ()    { Reset (); }

    int             NumElectrodes;

    void            Reset ();
};


class   InverseCompatibleClass
{
public:
                    InverseCompatibleClass ()       { Reset (); }

    int             NumElectrodes;
    int             NumSolPoints;

    void            Reset ();
};


class   RoisCompatibleClass
{
public:
                    RoisCompatibleClass ()       { Reset (); }

    int             NumDimensions;
    int             NumElectrodes;  // = NumDimensions
    int             NumSolPoints;   // = NumDimensions
    int             NumRois;

    void            Reset ();
};


//----------------------------------------------------------------------------

class   TracksGroupClass
{
public:
                    TracksGroupClass ()             { Reset (); }

    bool            alleeg;
    bool            allfreq;
    bool            allris;
    bool            allrisv;
//  bool            allsegdata;
    bool            alldata;
    bool            noneofthese;

    void            Reset ();
};


//----------------------------------------------------------------------------
                                        // Group of Files (GoF) is a strings list at its core, but with methods to interact with real files
enum    ResamplingType;
class   TGoGoF;


class   TGoF        :   public TStrings
{
public:
                    TGoF ()                                     : TStrings   ()                             {}
                    TGoF ( int numstrings )                     : TStrings   ( numstrings, MaxPathShort )   {}  // expected max size of file names
                    TGoF ( const char*              string  )   : TStrings   ( string  )                    {}
                    TGoF ( const TStrings*          strings )   : TStrings   ( strings )                    {}
                    TGoF ( const TStrings&          strings )   : TStrings   ( strings )                    {}
                    TGoF ( const TList<char>&       strings )   : TStrings   ( strings )                    {}
                    TGoF ( const owl::TStringArray& strings )   : TStrings   ( strings )                    {}
                    TGoF ( const owl::TDropInfo&      drop, const char* filesext = 0, TPointInt* where = 0, bool doesnotbelong = false );   // from drop info + extension filter + position
                    TGoF ( const TGoGoF& gogof, int gofi1 = -1, int gofi2 = -1 );   // Very handy trick to "flatten" a group of group of files
                    TGoF ( const TGoGoF& gogof, const TGoF& gof );                  // Very handy trick to "flatten" a group of group of files + another group


    int             NumFiles        ()                          const;   
    int             NumFiles        ( int filei1, int filei2 )  const;          // optional range that will be tested against current range

                                        // virtual functions
    void            Add             ( const char* string );
    void            Add             ( const char* string, long length );
    void            Add             ( const TGoF& gof );


    bool            AllExtensionsAre            ( const char* exts, int atomtype = 0 /*UnknownAtomType*/ )    const;  // all files have extensions belonging to the list provided
    bool            AllExtensionsIdentical      ( const char* errormessage = 0 )                              const;
    bool            SomeExtensionsAre           ( const char* exts /*, int atomtype = UnknownAtomType*/ )     const;
    bool            AnyTracksGroup              ( TracksGroupClass& tg )                                const;
    bool            AllFreqsGroup               ()                                                      const;
    void            AllTracksAreCompatible      ( TracksCompatibleClass&     tc )                       const;  // aims at testing Tracks files compatibility
    void            AllFreqsAreCompatible       ( FreqsCompatibleClass&      fc )                       const;  // aims at testing Frequency files compatibility
    void            AllElectrodesAreCompatible  ( ElectrodesCompatibleClass& ec )                       const;  // aims at testing inverse files compatibility
    void            AllInverseAreCompatible     ( InverseCompatibleClass&    ic )                       const;  // aims at testing inverse files compatibility
    void            AllRoisAreCompatible        ( RoisCompatibleClass&       rc )                       const;  // aims at rois files compatibility

    int             GetSumNumTF                 ( bool verbose = false )    const;
    int             GetMaxNumTF                 ( bool verbose = false )    const;
    int             GetNumElectrodes            ()                          const;
    double          GetSamplingFrequency        ()                          const;
    int             GetMeanFileSize             ()                          const;


    bool            CanOpenFiles    ( CanOpenFlags flags = CanOpenFileDefault )                 const;  // test all files can open
    void            CopyFilesTo     ( const char*  newdir,  CopyToFlags flags, const char* buddyexts = 0 ); // copy the files
    void            CopyFilesTo     ( const TGoF&  newlist, CopyToFlags flags, const char* buddyexts = 0 )  const;
    void            DeleteFiles     ( const char* buddyexts = 0 )                               const;
    void            NukeDirectories ( bool confirm = false )                                    const;
    void            OpenFiles       ()                                                          const;


    void            RemoveDir       ();
    void            RemoveFilename  ();
    void            RemoveExtension ();
    TGoF            GetPaths        ()  const;


    void            SetTempFileNames( int numfiles, const char* ext );  // generates only the file names

    int             FindFiles       ( const char* templ, bool searchfiles = true );
    int             GrepFiles       ( const char* path, const char* regexp, GrepOption options, bool searchfiles = true );
    void            GrepGoF         ( const TGoF& gof, const char* prefilename, const char* postfilename, const char* newexts, bool allresults );
    void            RevertGrepGoF   ( const TGoF& gof, const char* prefilename, const char* postfilename, const char* newexts );
    void            KeepLatestFile  ();
    void            ReplaceExtension( const char* newext );
    void            GetCommonParts  ( char* commondir, char* commonstart, char* commonend, TStrings*    diffs ) const;
    bool            GetCommonString ( char* base, bool includedir = true, bool includedifference = false )      const;
    void            GetFilenamesSubRange    ( int& fromchars, int& tochars )                                    const;

    void            Sort            (); // overriding TStrings::Sort

    void            SplitFreqFiles  ( SplitFreqFlags how, TGoGoF *gogofout = 0, bool showgauge = true );
    void            Resample        ( ResamplingType resampling, int numresamples, int resamplingsize, const TGoF* gofalt, TGoGoF& resgogof, TGoGoF* resgogofalt );  // resampling into anoth group of files
    int             SplitByNames    ( const char* splitwith, TGoGoF& gogofout, TStrings*    groupnames = 0 )        const;
    void            SplitByEpochs   ( const char* greppedwith, int maxepochs, const char* dirprefix, TGoF& gofout ) const;


//  char*           operator    []              ( int index )           { return TStrings::operator[] ( index ); }

    TGoF                                        ( const TGoF &op );
    TGoF&           operator    =               ( const TGoF &op2 );
};

                                        // templatized function to retrieve a value, either integer or double, from a group of files
enum                ReadFromHeaderType;

template <class TypeD>  
void                CheckParameterCompatibility ( const TGoF& gof, TypeD& value, ReadFromHeaderType parameter );


//----------------------------------------------------------------------------
                                        // Group of Group of Files
class   TGoGoF
{
public:
                    TGoGoF ();
                    TGoGoF ( int numgofs );
                    TGoGoF ( const TGoF& gof );
    virtual        ~TGoGoF ();          // class is derived


    int             NumGroups       ()      const       { return    Group.Num (); }
    int             NumFiles        ()      const;   
    int             NumFiles        ( int gofi1, int gofi2, int filei1 = -1, int filei2 = -1 )  const;     // optional range that will be tested against the range of each group
    bool            IsEmpty         ()      const       { return    Group.IsEmpty    (); }
    bool            IsNotEmpty      ()      const       { return    Group.IsNotEmpty (); }


    bool            CheckGroupIndexes   ( int& gofi1, int& gofi2 )          const;  // test the range of indexes, used anywhere we have range parameters
    int             GetMaxFiles         ( int gofi1 = -1, int gofi2 = -1 )  const;
    bool            AreGroupsEqual      ( int gofi1 = -1, int gofi2 = -1 )  const;
    bool            HasNoDuplicates     ()                                  const   { return TGoF ( *this ).HasNoDuplicates (); }
    int             GetMaxGroupsWithinSubjects  ()                          const;


    bool            AllExtensionsAre        ( const char* exts, int atomtype = 0 /*UnknownAtomType*/, int gofi1 = -1, int gofi2 = -1 )    const;
    bool            SomeExtensionsAre       ( const char* exts /*, int atomtype = 0*/, int gofi1 = -1, int gofi2 = -1 )                   const;
    bool            AnyTracksGroup          ( TracksGroupClass& tg, int gofi1 = -1, int gofi2 = -1 )                                const;
    bool            AllFreqsGroup           ( int gofi1 = -1, int gofi2 = -1 )                                                      const;
    void            AllTracksAreCompatible  ( TracksCompatibleClass& tc, int gofi1 = -1, int gofi2 = -1 )                           const;  // aims at testing Tracks files compatibility
    void            AllFreqsAreCompatible   ( FreqsCompatibleClass&  fc, int gofi1 = -1, int gofi2 = -1 )                           const;  // aims at testing Frequency files compatibility
    bool            AllSplitGroupsAreCompatible ()                                                                                  const;  // test that splitting into sub-groups is consistent

    int             GetSumNumTF             ( int gofi1 = -1, int gofi2 = -1, bool verbose = false )    const;
    int             GetMaxNumTF             ( int gofi1 = -1, int gofi2 = -1, bool verbose = false )    const;
    double          GetSamplingFrequency    ()                                                          const;


    bool            AllStringsGrep          ( const char* regexp, GrepOption options, int gofi1 = -1, int gofi2 = -1 )  const;  // true if all files of all groups Grep to true
    bool            SimplifyFilenames       ( int gofi1, int gofi2, TGoF& gof )         const;
    void            GetFilenamesSubRange    ( int& fromchars, int& tochars )            const;


    void            Reset           ();
    void            Set             ( int numgofs );
    void            Set             ( const TGoF& gof );

    void            Add             ( const TGoF   *gof,   bool copy = false, long length = MaxPathShort );
    void            Add             ( const TGoGoF& gogof, long length = MaxPathShort );

    bool            RemoveLastGroup ();

    void            RevertOrder             ();
    void            ConditionsToSubjects    ( int gofi1, int gofi2, TGoGoF& outgogof )  const;
    void            SubjectsToConditions    ( TGoGoF& outgogof )                        const;


    void            Show            ( int gofi1, int gofi2, const char* title = 0 )     const;
    void            Show            ( const char* title = 0 )                           const   { Show ( 0, NumGroups () - 1, title ); }


    const TGoF*     GetFirst        ()  const                           { return   Group.GetFirst (); }
    const TGoF*     GetLast         ()  const                           { return   Group.GetLast  (); }
          TGoF*     GetFirst        ()                                  { return   Group.GetFirst (); }
          TGoF*     GetLast         ()                                  { return   Group.GetLast  (); }


    void            CopyFilesTo     ( const char* newdir, CopyToFlags flags, const char* buddyexts = 0 ); // copy the files
    void            DeleteFiles     ( const char* buddyexts = 0 )                       const;
    void            NukeDirectories ( bool confirm = false )                            const;


    void            GrepGoF         ( const TGoGoF& gogof, const char* prefilename, const char* postfilename, const char* newexts, bool allresults );
    void            RevertGrepGoF   ( const TGoGoF& gogof, const char* prefilename, const char* postfilename, const char* newexts );


    TGoGoF                                      ( const TGoGoF& op  );
    TGoGoF&         operator    =               ( const TGoGoF& op2 );
    TGoGoF&         operator    =               ( const TGoF&   op2 );


    TGoF&           operator    []              ( int index )           { return *Group[ index ]; }
    const TGoF&     operator    []              ( int index )   const   { return *Group[ index ]; }

                    operator    int             ()              const   { return (int)  Group; }
                    operator    bool            ()              const   { return (bool) Group; }
                    operator    TList<TGoF>&    ()                      { return Group; }

protected:

    TList<TGoF>     Group;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
