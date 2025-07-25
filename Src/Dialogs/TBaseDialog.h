/************************************************************************\
� 2024-2025 Denis Brunet, University of Geneva, Switzerland.

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
                                        // Bunch of OwlNext controls:
#include    <owl/window.h>              // TWindow

#include    <owl/inputdia.h>            // TInputDialog
#include    <owl/checkbox.h>
#include    <owl/radiobut.h>
#include    <owl/gauge.h>
#include    <owl/combobox.h>
#include    <owl/static.h>
#include    <owl/dc.h>
#include    <owl/menu.h>
#include    <owl/edit.h>
#include    <owl/groupbox.h>
#include    <owl/updown.h>
#include    <owl/validate.h>            // TFilterValidator

#include    "resource.h"

#include    "Strings.TFixedString.h"
#include    "Files.TGoF.h"
#include    "WindowingUtils.h"

#include    "TCartoolDocManager.h"      // TCartoolObjects

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

                                        // edit field for values (int and float)
constexpr int       EditSizeValue               = StringValueLength;
                                        // edit field for strings, including file names
constexpr int       EditSizeText                =  4 * KiloByte;
                                        // edit field for very long strings, especially enumeration of entities
constexpr int       EditSizeTextLong            = 64 * KiloByte;
                                        // Maximum output files to open in batch mode, to avoid cluttering the display / crashing the app
constexpr int       MaxFilesToOpen              = 10;


//----------------------------------------------------------------------------
                                        // OwlNext dialog classes utilities
                                        // Note: the old macros should be replaced by the newer functions

                                        // usually after et Set..., to forcefully restore the cursor to the rightmost position
#define             ResetCaret                  SetSelection ( 10000, 10000 )

                                        // Utilities for easier access to dialog controls, without going through a transfer buffer
inline  bool        IsEmpty     ( const owl::TEdit*         edit                )   { return edit->GetLineLength ( 0 ) == 0;    }
inline  bool        IsNotEmpty  ( const owl::TEdit*         edit                )   { return ! IsEmpty ( edit );                }
inline  bool        IsSpace     ( const owl::TEdit*         edit                )   { TFixedString<256> s; ; edit->GetText ( s, s.Size () ); return s.IsSpace (); }
inline  bool        IsNotSpace  ( const owl::TEdit*         edit                )   { return ! IsSpace ( edit );                }
inline  void        ClearText   (       owl::TEdit*         edit                )   { edit->Clear ();                           }
inline  void        SetText     (       owl::TEdit*         edit,   const char* s ) { edit->SetText ( s );                      }
inline  void        CopyText    ( owl::TEdit*   to,     const owl::TEdit*  from )   { TFixedString<EditSizeText> s; from->GetText ( s, s.Size () ); to->SetText ( s ); }
inline  int         GetInteger  ( const owl::TEdit*         edit                )   { TStringValue  s; edit->GetText ( s, s.Size () ); return StringToInteger ( s ); }
inline  double      GetDouble   ( const owl::TEdit*         edit                )   { TStringValue  s; edit->GetText ( s, s.Size () ); return StringToDouble  ( s ); }
inline  void        SetInteger  (       owl::TEdit*         edit,   int    v    )   { edit->SetText ( IntegerToString ( v ) ); }
inline  void        SetDouble   (       owl::TEdit*         edit,   double v    )   { edit->SetText ( FloatToString   ( v ) ); }


inline  bool        IsChecked   ( const owl::TCheckBox*     chk                 )   { return CheckToBool ( chk->GetCheck () );  }
inline  bool        IsNotChecked( const owl::TCheckBox*     chk                 )   { return ! IsChecked ( chk );               }
inline  void        SetCheck    (       owl::TCheckBox*     chk,    bool   s    )   { chk->SetCheck ( BoolToCheck ( s ) );      }
inline  void        SetCheck    (       owl::TCheckBox*     chk                 )   { SetCheck ( chk, true  );                  }
inline  void        ResetCheck  (       owl::TCheckBox*     chk                 )   { SetCheck ( chk, false );                  }
inline  void        ToggleCheck (       owl::TCheckBox*     chk                 )   { SetCheck ( chk, ! IsChecked ( chk ) );    }

                                            // !be careful when forcing states of a radio button to also update all the other radio buttons that are parts of the same group!
inline  bool        IsChecked   ( const owl::TRadioButton*  rdb                 )   { return CheckToBool ( rdb->GetCheck () );  }
inline  bool        IsNotChecked( const owl::TRadioButton*  rdb                 )   { return ! IsChecked ( rdb );               }
inline  void        SetCheck    (       owl::TRadioButton*  rdb,    bool   s    )   { rdb->SetCheck ( BoolToCheck ( s ) );      }
inline  void        SetCheck    (       owl::TRadioButton*  rdb                 )   { SetCheck ( rdb, true  );                  }
inline  void        ResetCheck  (       owl::TRadioButton*  rdb                 )   { SetCheck ( rdb, false );                  }
inline  void        ToggleCheck (       owl::TRadioButton*  rdb                 )   { SetCheck ( rdb, ! IsChecked ( rdb ) );    }


inline  void        SetIndex    (       owl::TComboBox*     cmbb,   int    i    )   { cmbb->SetSelIndex ( i );                  }
inline  void        SetIndex    (       owl::TComboBoxData& cmbbd,  int    i    )   { cmbbd.Select      ( i );                  }   // yes, this one has a different syntax
inline  int         GetIndex    ( const owl::TComboBox*     cmbb                )   { return cmbb->GetSelIndex ();              }
inline  int         GetIndex    (       owl::TComboBoxData& cmbbd               )   { return cmbbd.GetSelIndex ();              }   // does not accept a const TComboBoxData&
inline  bool        IsIndex     ( const owl::TComboBox*     cmbb,   int    i    )   { return GetIndex ( cmbb  ) == i;           }
inline  bool        IsIndex     (       owl::TComboBoxData& cmbbd,  int    i    )   { return GetIndex ( cmbbd ) == i;           }   // does not accept a const TComboBoxData&


int                 DialogStringLengthToPixels  ( int stringlength );
void                UpdateHorizontalScroller    ( const owl::TListBox* listbox, int stringlength ); //  { listbox->SendMessage ( LB_SETHORIZONTALEXTENT, DialogStringLengthToPixels ( stringlength ) ); }


//----------------------------------------------------------------------------
                                        // TEdit pre-defined validator strings that will forbid the entry of any non-conforming characters
constexpr char*     ValidatorPositiveInteger    = "0-9";
constexpr char*     ValidatorSignedInteger      = "-+0-9";

constexpr char*     ValidatorPositiveFloat      = ".0-9";
constexpr char*     ValidatorPositiveFloats     = ".0-9 ";
constexpr char*     ValidatorPositiveFloatList  = ".0-9,; \t-";
constexpr char*     ValidatorSignedFloat        = "-+.0-9";

constexpr char*     ValidatorOrientation        = "APLRIS";


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Handy aliases for transfer buffer:
using               TCheckBoxData               = UINT16;
using               TRadioButtonData            = UINT16;
using               TEditData                   = char;
using               TStaticData                 = char;
using               TListBoxData                = owl::TListBoxData;
using               TComboBoxData               = owl::TComboBoxData;


/*                                      // For the record, this is the recommended initialization sequence for transfer buffers:
void    TDerivedDialog::SetupWindow ()
{
                                        // !called first!
TParentDialog::SetupWindow ();
                                        // !called right after!
SetTransferBuffer ( &DerivedDialogTransfer );
}
*/

//----------------------------------------------------------------------------
                                        // List of available file types for saving EEG in Dialogs
enum                SavingEegFileTypes
                    {
                    PresetFileTypeTxt,
                    PresetFileTypeEp,
                    PresetFileTypeEph,
                    PresetFileTypeSef,
                    PresetFileTypeBV,
                    PresetFileTypeEdf,
                    PresetFileTypeRis,

                    NumSavingEegFileTypes,
                    PresetFileTypeDefaultEEG    = PresetFileTypeBV,
                    PresetFileTypeDefaultRIS    = PresetFileTypeRis,
                    };

extern const char   SavingEegFileTypePreset[ NumSavingEegFileTypes ][ 64 ];
extern const char   SavingEegFileExtPreset [ NumSavingEegFileTypes ][  8 ];

inline bool         IsFileTypeTextual               ( SavingEegFileTypes filetype ) { return  filetype == PresetFileTypeTxt || filetype == PresetFileTypeEp || filetype == PresetFileTypeEph; }
inline bool         IsFileTypeBinary                ( SavingEegFileTypes filetype ) { return  filetype == PresetFileTypeSef || filetype == PresetFileTypeBV || filetype == PresetFileTypeEdf || filetype == PresetFileTypeRis; }
SavingEegFileTypes  ExtensionToSavingEegFileTypes   ( const char* ext );


constexpr char*     ExportedByCartool       = "Generated by Cartool";


//----------------------------------------------------------------------------
                                        // Font height used in all dialogs
constexpr int       DialogFontHeight    = 8;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Basic class for all dialogs, used mainly to abstract the  ProcessCurrent  vs  BatchProcess  of most dialogs
                                        // as well as looping through frequencies, when relevant


constexpr char*     BatchNotAvailMessage    = "Batch 'Drag & Drop' is not currently available," NewLine 
                                              "only the current file can be processed."         NewLine 
                                              NewLine 
                                              "Close the current file to enable the Batch processing mode...";

constexpr char*     BatchErrorMessage       = "Batch 'Drag & Drop' is not currently available,"     NewLine 
                                              "because your parameters don't seem to be correct!"   NewLine 
                                              NewLine 
                                              "Please check your parameters, then try again...";

constexpr char*     IrrelevantErrorMessage  = "Skipping non-relevant file:";


enum    FrequencyAnalysisType;
class   TTracksDoc;
class   TGoF;
class   TGoGoF;

                                        // Let's break up how these methods articulate among themselves:
// 
//                                      First pipe-line, when current / batch is relevant:
// 
// BatchProcessing      Is a flag set by derived class at creation time, something like  "BatchProcessing = currentdoc == 0;"
//                      It will condition which "Process" buttons will be enabled, as well as assessing if a current EEG document is available or not
// 
// CmBatchProcess       Handles message IDC_PROCESSCURRENT sent by "Process Current" button -> Calls    BatchProcess    with single file
// CmProcessCurrent     Handles message IDC_PROCESSBATCH   sent by "Batch Process"   button -> Calls    BatchProcess    with multiple files
//
// BatchProcess         Loops through the given files, the loops through all EEG sessions   -> Calls    BatchProcessCurrent
// 
// BatchProcessCurrent  Default is "empty" ans simply..                                        Calls    ProcessCurrent
//                      Derived class can override it, though, like for frequency loops
// 
// ProcessCurrent       Has to be overridden by all derived class - This is were dialogs actually run, and expect EEGDoc to be set for them
// 
//                                      Second pipe-line, when current / batch is NOT relevant:
// 
// BatchProcessGroups   Repeats processing for n groups of groups of files                  -> Calls    ProcessGroups
//                      Optionally also looping through frequencies when relevant
//
// ProcessGroups        Has to be overridden by all derived class - This is were dialogs actually run, and expect EEGDoc to be set for them


class	TBaseDialog :   public  owl::TDialog,
                        public  TCartoolObjects
{
public:
                    TBaseDialog ( owl::TWindow* parent, owl::TResId resId, TTracksDoc* doc = 0 );


    TTracksDoc*     EEGDoc;                 // current doc to process - could also be a TFreqDoc seen as a TTracksDoc


    bool            BatchProcessing;        // tells which of "batch processing" or "process current file" mode is available
    char            BatchFilesExt[ 256 ];

    bool            HasBatchFiles           ()  const   { return  (bool) BatchFileNames; }
    int             NumBatchFiles           ()  const   { return  BatchFileNames.NumFiles (); }
    bool            IsBatchFirstCall        ()  const   { return  HasBatchFiles () && BatchFileIndex == 0; }
    bool            IsBatchLastCall         ()  const   { return  HasBatchFiles () && BatchFileIndex == BatchFileNames.NumFiles () - 1; }
    int             GetBatchFileIndex       ()  const   { return  BatchFileIndex;}

                                        // For looping into frequencies:
    int             NumFreqs;
    int             FreqIndex;              // current frequency index
    char            FreqName[ 64 ];         // current frequency name
    FrequencyAnalysisType   FreqType;

    void            ResetFreq               ();
    bool            IsFreqLoop              ()  const   { return FreqIndex != -1;           }
    bool            IsFirstFreq             ()  const   { return FreqIndex ==  0;           }
    bool            IsLastFreq              ()  const   { return FreqIndex == NumFreqs - 1; }


    void            BeginModal              (); // A Modal window can not lose the focus, contrary to a Modeless window which allows another one to grab it for their own purpose
    void            EndModal                ();

    void            SetControlText          ( int resid, const char* text ) const   { SetDlgItemText ( resid, text ); }


    virtual void    CmBatchProcess          ();                             // calls BatchProcess with multiple files
    void            CmBatchProcess          ( const char* filesfilter );    
    virtual void    CmProcessCurrent        ();                             // calls BatchProcess with a single file
    void            CmProcessCurrent        ( const char* file );

    void            BatchProcessDropped     ( TGoF& gof               );    // calls BatchProcess with a Group of Files
    void            BatchProcessLmFile      ( char* lmfile            );    // calls BatchProcess with files from a lm file
    void            BatchProcessSplitFile   ( char* splitfile         );    // calls BatchProcess with files from a csv file
    virtual void    BatchProcess            ();                             // loop in the files and calls ProcessCurrent
    virtual void    BatchProcessCurrent     ();                             // a call to actual ProcessCurrent - here for overriding purposes
    virtual void    ProcessCurrent          ( void* usetransfer = 0, const char* moreinfix = 0 )    {}  // this is where the ACTUAL processing should be put

    void            BatchProcessGroups      ( TGoGoF *gogof, int bygroupsof, int testinggroups, void *usetransfer, bool endadvertised = true ); // loop within groups, including a frequency loop if relevant
    virtual void    ProcessGroups           ( TGoGoF *gogof, int gofi1, int gofi2, void *usetransfer )  {}  // this is where the ACTUAL processing should be put


    void            CmHelp                  ();


protected:

    TGoF            BatchFileNames;     // used to store & transfer batch files across fcts
    int             BatchFileIndex;     // index of current file in batch processing

    DECLARE_RESPONSE_TABLE ( TBaseDialog );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
