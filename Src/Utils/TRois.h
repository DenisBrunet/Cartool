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

#include    "OpenGL.Colors.h"           // TGLColor

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class   TMaps;
class   TGoMaps;


class   TRoi
{
public:
                    TRoi ();

    TSelection          Selection;
    TGLColor<GLfloat>   Color;

                                        // Currently implemented: FilterTypeMedian, FilterTypeMean
    void            Average ( TArray2<float>&         datainout, long tf1, long tf2, FilterTypes how, float* dataout = 0 ) const;
    void            Average ( TArray1<float>&         datainout, FilterTypes how )              const;
    void            Average ( TArray1<TVector3Float>& datainout, FilterTypes how )              const;
    void            Average ( const TMaps& datain, TMaps& dataout, int roii, FilterTypes how )  const;


                    TRoi            ( const TRoi& op );
    TRoi&           operator    =   ( const TRoi& op2 );
};


//----------------------------------------------------------------------------

constexpr int   RoiNameLength           = 256;

class           TStrings;

enum            RoiType
                {
                UnknownRoiType,
                RoiIndex,
                //RoiCoordinates    // not currently used
                };


class   TRois //:   public virtual  TDataFormat   // to handle Type and more if needed
{
public:
                    TRois ();
                    TRois ( int numrois, int dimension );
                    TRois ( const char* filepath );
                   ~TRois ();


    bool            IsAllocated     ()      const           { return    Rois    != 0; }
    bool            IsNotAllocated  ()      const           { return    Rois    == 0; }
    bool            IsEmpty         ()      const           { return    NumRois == 0; }
    bool            IsNotEmpty      ()      const           { return    NumRois != 0; }


    RoiType         GetType         ()      const           { return    Type; }
    const char*     GetName         ()      const           { return    Name; }
    int             GetDimension    ()      const           { return    Dimension; }
    int             GetNumRois      ()      const           { return    NumRois; }
    int             GetTotalSelected()      const           { return    TotalSelected; }

    const TStrings* GetRoiNames ()          const           { return   &RoiNames; }
    const char*     GetRoiName  ( int r )   const           { return    RoiNames[ r ]; }
    char*           RoiNamesToText ( char* text )   const;


    const TRoi*     GetRoi      ( int r = 0 )   const       { return   &Rois[ r ]; }
    const TRoi*     IndexToRoi  ( int index )   const;

    const TSelection*   GetRoisSelected     ()  const       { return   &RoisSelected; }
    const TSelection*   GetAtomsNotSelected ()  const       { return   &AtomsNotSelected; }

                                            // RoisSelected
    void            Set         ();
    void            Set         ( int roi );

    void            Reset       ();
    void            Reset       ( int roi );

    void            ShiftUp     ( int num = 1 );
    void            ShiftDown   ( int num = 1 );

    int             NumSet      ()          const           { return    RoisSelected.NumSet (); }

                                        // cumulate rois selection into one selection
    void            CumulateInto ( TSelection &sel, int fromvalue = -1, int tovalue = -1 );

                                        // Currently implemented: FilterTypeMedian, FilterTypeMean
    void            Average     ( TArray2<float>&           datainout, long tf1, long tf2, FilterTypes how, TArray2<float>* dataout = 0 )   const;
    void            Average     ( TArray1<float>&           datainout, FilterTypes how )                    const;
    void            Average     ( TArray1<TVector3Float>&   datainout, FilterTypes how )                    const;
    void            Average     ( const TMaps&              datain, TMaps&      dataout, FilterTypes how )  const;
    void            Average     ( const TGoMaps&            datain, TGoMaps&    dataout, FilterTypes how )  const;


    bool            AddRoi      ( TSelection& roisel, const char* roiname, bool doallocate, bool checkoverlap );
    void            RemoveRoi   ();
    void            WriteFile   ( const char* file )    const;
    void            ReadFile    ( const char* file );


    const TRoi&     operator    []      ( int i )   const   { return    Rois[ i ]; }
          TRoi&     operator    []      ( int i )           { return    Rois[ i ]; }
                    operator    TSelection& ()              { return    RoisSelected; }

protected:

    RoiType         Type;                   // index or voxels
    int             Dimension;              // of the original data
    char            Name[ RoiNameLength ];  // of the roi itself

    TRoi*           Rois;                   // C++ array of single roi's (could be using some std::vector instead)
    int             NumRois;
    int             TotalSelected;          // the count of all selections in all ROIs
    TSelection      RoisSelected;           // handy to know which Rois are active (working like Tracks)
    TStrings        RoiNames;               // names of rois
    TSelection      AtomsNotSelected;       // keep track of elements that are not part of any ROI


    void            ResetClass ();
    void            Allocate ( int numrois, int dimension );
    void            AddRoiFinalize ();

    void            UpdateToRoisSelected   ();
    void            UpdateFromRoisSelected ();

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
