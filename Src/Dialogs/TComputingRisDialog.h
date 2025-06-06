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

#include    "Files.TGoF.h"

#include    "TMicroStates.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr char*     ComputingRisTitle           = "Computing RIS";


enum        ComputingRisPresetsEnum
            {
            ComputingRisPresetErpGroupMeans,
            ComputingRisPresetErpIndivMeans,
            ComputingRisPresetErpIndivEpochs,
            ComputingRisPresetErpSegClusters,
            ComputingRisPresetErpFitClusters,

            ComputingRisPresetSeparator1,
            ComputingRisPresetIndIndivEpochs,

            ComputingRisPresetSeparator2,
            ComputingRisPresetSpont,
            ComputingRisPresetSpontClusters,

            ComputingRisPresetSeparator3,
            ComputingRisPresetFreq,

            NumComputingRisPresets,

            ComputingRisPresetDefault   = ComputingRisPresetErpIndivMeans,
            };


enum        CRISPresetFlags
            {
            CRISPresetNone                  = 0x0000,

            CRISPresetERP                   = 0x0001,
            CRISPresetInd                   = 0x0002,
            CRISPresetSpont                 = 0x0004,
            CRISPresetFreq                  = 0x0008,

            CRISPresetClusters              = 0x0010,
            CRISPresetEpochs                = 0x0020,
            CRISPresetCentroids             = 0x0040,

            CRISPresetNorm                  = 0x0100,
            CRISPresetVect                  = 0x0200,
            CRISPresetDataTypeMask          = CRISPresetNorm | CRISPresetVect,
            };


class       CRISPresetSpec
{
public:

    const ComputingRisPresetsEnum   Code;
    const char                      Text [ 64 ];
    const CRISPresetFlags           Flags;          // Allowed values
          CRISPresetFlags           Current;        // Init state                       - could also be used to save current user's choices
          bool                      GroupCond;      // Init state + user current states - field could be updated


    bool                IsErp                   ()  const   {   return  IsFlag ( Flags,     CRISPresetERP       ); }
    bool                IsInduced               ()  const   {   return  IsFlag ( Flags,     CRISPresetInd       ); }
    bool                IsSpontaneous           ()  const   {   return  IsFlag ( Flags,     CRISPresetSpont     ); }
    bool                IsFrequency             ()  const   {   return  IsFlag ( Flags,     CRISPresetFreq      ); }

    bool                IsClusters              ()  const   {   return  IsFlag ( Flags,     CRISPresetClusters  ); }
    bool                IsEpochs                ()  const   {   return  IsFlag ( Flags,     CRISPresetEpochs    ); }
    bool                IsCentroids             ()  const   {   return  IsFlag ( Flags,     CRISPresetCentroids ); }

    bool                CanNorm                 ()  const   {   return  IsFlag ( Flags,     CRISPresetNorm      ); }
    bool                CanVect                 ()  const   {   return  IsFlag ( Flags,     CRISPresetVect      ); }
    bool                CmDataTypeEnable        ()  const   {   return  IsFlag ( Flags,     CRISPresetDataTypeMask, CRISPresetDataTypeMask );   }   // enabled if more than 1 data type is available
    bool                IsNorm                  ()  const   {   return  IsFlag ( Current,   CRISPresetNorm      ); }
    bool                IsVect                  ()  const   {   return  IsFlag ( Current,   CRISPresetVect      ); }

    AtomType            GetAtomTypeEpochs       ()  const   {   return  IsErp       ()  ?   AtomTypeVector              // unrelated vectors should cancel each others
                                                                      : IsInduced   ()  ?   AtomTypePositive            // targetting power
                                                                      : IsFrequency ()  ?   AtomTypePositive            // if epochs case existed, should be power already
                                                                      :                     AtomTypeVector;         }
    AtomType            GetAtomTypeProcessing   ( AtomType datatypeepochs, AtomType  datatypefinal )  const   {   
                                                                                                    // Actual processing data type + Z-Score + Envelope
                                                            return  IsEpochs    ()  ?   datatypeepochs  : datatypefinal;  
                                                            }

    AtomType            GetAtomTypeFinal        ( AtomType datatypeproc, AtomType datatypefinal )   const   {   
                                                                                                    // don't output vectorial results for frequencies (complex case)
                                                            if (   IsVector ( datatypefinal ) && Code == ComputingRisPresetFreq )
                                                                datatypefinal   = AtomTypePositive;
                                                                                                    // Can not save to Vector if processing is Norm
                                                            if ( ! IsVector ( datatypeproc  ) && IsVector ( datatypefinal ) )
                                                                datatypefinal   = datatypeproc;

                                                            return  datatypefinal;
                                                            }


    bool                CmGroupsAveragingEnable ()  const   {   return  Code == ComputingRisPresetErpIndivMeans
                                                                     || IsEpochs ();                                }   // still needs to check all groups of files are compatibles somewhere
};

extern CRISPresetSpec   CRISPresets[ NumComputingRisPresets ];


//----------------------------------------------------------------------------
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


class  TComputingRisStruct
{
public:
                        TComputingRisStruct ();

        
    TComboBoxData       EegPresets;

    TRadioButtonData    GroupCond;
    TRadioButtonData    GroupSubj;
    TListBoxData        GroupsSummary;
    TEditData           NumSubjectsEdit     [ EditSizeValue ];
    TEditData           NumConditionsEdit   [ EditSizeValue ];
    TEditData           NumInversesEdit     [ EditSizeValue ];

    TCheckBoxData       SpatialFilter;
    TEditData           XyzFile             [ EditSizeText ];

    TEditData           InverseFile         [ EditSizeText ];
    TRadioButtonData    PositiveData;
    TRadioButtonData    VectorData;
    TRadioButtonData    RegAuto;
    TRadioButtonData    RegFixed;
    TEditData           RegFixedEdit        [ EditSizeValue ];

    TCheckBoxData       TimeZScore;
    TRadioButtonData    ComputeZScore;
    TRadioButtonData    LoadZScoreFile;

    TCheckBoxData       Rank;

    TCheckBoxData       Envelope;
    TEditData           EnvelopeLowFreq     [ EditSizeValue ];

    TCheckBoxData       ApplyRois;
    TEditData           RoisFile            [ EditSizeText ];

    TEditData           BaseFileName        [ EditSizeText ];

    TCheckBoxData       SavingIndividualFiles;
    TCheckBoxData       SavingEpochFiles;
    TCheckBoxData       ComputeGroupsAverages;
    TCheckBoxData       ComputeGroupsCentroids;
    TCheckBoxData       SavingZScoreFactors;
};


class  TComputingRisStructEx:   public  TComputingRisStruct
{
public:
                    TComputingRisStructEx ();

                                        // Useful to check and save consistencies across runs
    InverseCompatibleClass      CompatInverse;
    TracksCompatibleClass       CompatEegs;
    ElectrodesCompatibleClass   CompatElectrodes;
    InverseCompatibleClass      CompatSp;
    RoisCompatibleClass         CompatRois;

    bool            IsInverseOK;
    bool            IsEegOK;
    bool            IsXyzOK;
    bool            IsRoisOK;
    bool            IsInverseAndEegOK;
    bool            IsInverseAndXyzOK;
    bool            IsInverseAndRoisOK;
    bool            IsXyzAndEegOK;


    void            CheckInverses       ();
    void            CheckInverseAndEeg  ();
    void            CheckInverseAndXyz  ();
    void            CheckInverseAndRois ();

    void            CheckXyz            ();
    void            CheckXyzAndEeg      ();

    void            CheckRois           ();
};


EndBytePacking

//----------------------------------------------------------------------------

class   TComputingRisDialog :   public  TBaseDialog
{
public:
                        TComputingRisDialog ( owl::TWindow* parent, owl::TResId resId );
                       ~TComputingRisDialog ();
                       
protected:
    owl::TComboBox      *EegPresets;

    owl::TRadioButton   *GroupCond;
    owl::TRadioButton   *GroupSubj;
    owl::TListBox       *GroupsSummary;
    owl::TEdit          *NumSubjectsEdit;
    owl::TEdit          *NumConditionsEdit;
    owl::TEdit          *NumInversesEdit;

    owl::TCheckBox      *SpatialFilter;
    owl::TEdit          *XyzFile;

    owl::TEdit          *InverseFile;
    owl::TRadioButton   *PositiveData;
    owl::TRadioButton   *VectorData;
    owl::TRadioButton   *RegAuto;
    owl::TRadioButton   *RegFixed;
    owl::TEdit          *RegFixedEdit;

    owl::TCheckBox      *TimeZScore;
    owl::TRadioButton   *ComputeZScore;
    owl::TRadioButton   *LoadZScoreFile;

    owl::TCheckBox      *Rank;

    owl::TCheckBox      *Envelope;
    owl::TEdit          *EnvelopeLowFreq;

    owl::TCheckBox      *ApplyRois;
    owl::TEdit          *RoisFile;

    owl::TEdit          *BaseFileName;

    owl::TCheckBox      *SavingIndividualFiles;
    owl::TCheckBox      *SavingEpochFiles;
    owl::TCheckBox      *ComputeGroupsAverages;
    owl::TCheckBox      *ComputeGroupsCentroids;
    owl::TCheckBox      *SavingZScoreFactors;


                                        // Use to test files consistencies, with all the asynchronous D&D
    void            CheckEeg                (); // for the moment, method remains in the dialog instead of transfer buffer struct, as it does update the dialog a lot


    void            SetupWindow             ()  final;
    void            CmOk                    ();
    void            CmOkEnable              ( owl::TCommandEnabler &tce );

    void            EvEegPresetsChange      ();
    void            EvGroupPresetsChange    ();
    void            EvDropFiles             ( owl::TDropInfo drop );
    void            CmGroupPresetsEnable    ( owl::TCommandEnabler &tce );
    void            CmDataTypeEnable        ( owl::TCommandEnabler &tce );

    void            CmBrowseISFile          ();
    void            AddISGroup              ( const TGoF& gofis );
    void            CmNotFrequencyEnable    ( owl::TCommandEnabler &tce );
    void            CmRoisChange            ();

    void            CmAddEegGroup           ();
    void            AddEegGroups            ( const TGoF& gofeeg );
    void            AddEegGroup             ( const TGoF& gofeeg );
    void            CmRemoveEegGroup        ();
    void            CmClearEegGroups        ();
    void            CmReadParams            ();
    void            ReadParams              ( const char* filename = 0 );
    void            CmWriteParams           ();

    void            CmBrowseXyzFile         ();
    void            SetXyzFile              ( const char* file );
    void            CmXyzEnable             ( owl::TCommandEnabler &tce );

    void            CmRegularizationEnable  ( owl::TCommandEnabler &tce );

    void            CmEnvelopeEnable        ( owl::TCommandEnabler &tce );
    void            CmEnvelopeLowFreqEnable ( owl::TCommandEnabler &tce );

    void            CmBrowseRoiDoc          ();
//  void            CmRoiChange             ();
    void            SetRoisFile             ( const char* file );
    void            CmRoisEnable            ( owl::TCommandEnabler &tce );

    void            CmGroupsAveragingEnable ( owl::TCommandEnabler &tce );
    void            CmGroupsCentroidsEnable ( owl::TCommandEnabler &tce );
    void            CmSaveEpochsEnable      ( owl::TCommandEnabler &tce );

    void            CmZScoreChange          ();
    void            CmZScoreFactorsEnable   ( owl::TCommandEnabler &tce );
    void            CmThresholdEnable       ( owl::TCommandEnabler &tce );

    void            UpdateSubjectsConditions( bool updategroups = true );
    void            UpdateGroupSummary      ();


    DECLARE_RESPONSE_TABLE ( TComputingRisDialog );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
