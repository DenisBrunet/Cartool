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

#include    <stdio.h>

#include    "Files.Extensions.h"

#include    "Strings.Utils.h"
#include    "Files.Utils.h"
#include    "Math.Utils.h"

#include    "TBaseDialog.h"
#include    "TMicroStates.h"                // TGoMaps

#include    "TRois.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TRoi::TRoi ()
{                                       // actually useful, so that internal component can init correctly
}


        TRoi::TRoi ( const TRoi& op )
{
Selection       = op.Selection;
Color           = op.Color;
}


TRoi&   TRoi::operator = ( const TRoi& op2 )
{
if ( &op2 == this )
    return  *this;

Selection       = op2.Selection;
Color           = op2.Color;

return  *this;
}


//----------------------------------------------------------------------------
                                        // Omp / parallel computation is called at the higher level, f.ex. from TRois
void    TRoi::Average ( TArray2<float>& datainout, long tf1, long tf2, FilterTypes how, float* dataout )   const
{
TEasyStats          stat ( how == FilterTypeMedian ? Selection.NumSet () : 0 );
double              v;


for ( int tf = tf1; tf <= tf2; tf++ ) {

    stat.Reset ();

    for ( TIteratorSelectedForward seli ( Selection ); (bool) seli; ++seli )
        stat.Add ( datainout ( seli(), tf ), ThreadSafetyIgnore );

                                        // opted for a Median, so the size of a ROI doesn't introduce a smoothing factor
    v   = how == FilterTypeMedian ? stat.Median () : stat.Mean ();


    if ( dataout )                      // output to given buffer
        dataout[ tf ]  = v;
    else                                // overwrite on data (ok if no overlap in all selections)
        for ( TIteratorSelectedForward seli ( Selection ); (bool) seli; ++seli )
            datainout ( seli(), tf )  = v;

    } // for tf
}


//----------------------------------------------------------------------------
void    TRoi::Average ( TArray1<float>& datainout, FilterTypes how )  const
{
TEasyStats          stat ( how == FilterTypeMedian ? Selection.NumSet () : 0 );
double              v;


for ( TIteratorSelectedForward seli ( Selection ); (bool) seli; ++seli )
    stat.Add ( datainout[ seli() ], ThreadSafetyIgnore );


v   = how == FilterTypeMedian ? stat.Median () : stat.Mean ();

                                        // overwrite on data (ok if no overlap in all selections)
for ( TIteratorSelectedForward seli ( Selection ); (bool) seli; ++seli )
    datainout[ seli() ] = v;
}


//----------------------------------------------------------------------------
void    TRoi::Average ( TArray1<TVector3Float>& datainout, FilterTypes how )  const
{
TGoEasyStats        stat ( 3, how == FilterTypeMedian ? Selection.NumSet () : 0 );
TVector3Float       v;


for ( TIteratorSelectedForward seli ( Selection ); (bool) seli; ++seli ) {

    stat ( 0 ).Add ( datainout[ seli() ][ 0 ], ThreadSafetyIgnore );
    stat ( 1 ).Add ( datainout[ seli() ][ 1 ], ThreadSafetyIgnore );
    stat ( 2 ).Add ( datainout[ seli() ][ 2 ], ThreadSafetyIgnore );
    }

                                        // opted for a Median, so the size of a ROI doesn't introduce a smoothing factor
v[ 0 ]  = how == FilterTypeMedian ? stat[ 0 ].Median () : stat[ 0 ].Mean ();
v[ 1 ]  = how == FilterTypeMedian ? stat[ 1 ].Median () : stat[ 1 ].Mean ();
v[ 2 ]  = how == FilterTypeMedian ? stat[ 2 ].Median () : stat[ 2 ].Mean ();

                                        // overwrite on data (ok if no overlap in all selections)
for ( TIteratorSelectedForward seli ( Selection ); (bool) seli; ++seli )

    datainout[ seli() ] = v;
}


//----------------------------------------------------------------------------
void    TRoi::Average ( const TMaps& datain, TMaps& dataout, int roii, FilterTypes how )    const
{
TEasyStats          stat ( how == FilterTypeMedian ? Selection.NumSet () : 0 );
double              v;


for ( int tf = 0; tf < datain.GetNumMaps (); tf++ ) {

    stat.Reset ();

    for ( TIteratorSelectedForward seli ( Selection ); (bool) seli; ++seli )
        stat.Add ( datain ( tf, seli() ), ThreadSafetyIgnore );


    v   = how == FilterTypeMedian ? stat.Median () : stat.Mean ();


    dataout ( tf, roii )    = v;
    } // for tf
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

        TRois::TRois ()
      : Rois ( 0 )
{
ResetClass ();
}


        TRois::TRois ( int numrois, int dimension )
      : Rois ( 0 )
{
ResetClass ();

Dimension           = dimension;

Allocate ( numrois, dimension );        // and create variables
}


        TRois::TRois ( const char* filepath )
      : Rois ( 0 )
{
ResetClass ();

ReadFile ( filepath );
}


        TRois::~TRois ()
{
ResetClass ();
}


void    TRois::ResetClass ()
{
Type                = UnknownRoiType;
Dimension           = 0;
ClearString ( Name );

if ( Rois )
    delete[] Rois;
Rois                = 0;

NumRois             = 0;
TotalSelected       = 0;

RoisSelected    .Reset ();
RoiNames        .Reset ();
AtomsNotSelected.Reset ();
}


//----------------------------------------------------------------------------
                                        // create the variables
void    TRois::Allocate ( int numrois, int dimension )
{
                                        // init variables
                                        // cluster setup
Rois            = new TRoi [ numrois ];
                                        // set it sorted, but indeed, could be interesting to have it arbitrary!
RoisSelected    = TSelection ( numrois, OrderSorted );

RoiNames.Set ( numrois, RoiNameLength );

AtomsNotSelected= TSelection ( dimension, OrderSorted );
AtomsNotSelected.Set ();
}


//----------------------------------------------------------------------------
bool    TRois::AddRoi ( TSelection& roisel, const char* roiname, bool doallocate, bool checkoverlap )
{
                                        // first call? init these variables
if ( Type == UnknownRoiType ) {

    Type        = RoiIndex;
    NumRois     = 0;
    Dimension   = roisel.Size ();

    StringCopy ( Name, "TempRois", RoiNameLength - 1 );
    }


if ( doallocate ) {                     // if requested, we allocate the space for the new roi
                                        // save old & pertinent contents
    TRoi*           oldrois     = Rois;
    TStrings        oldroinames = RoiNames;


    Allocate ( NumRois + 1, Dimension );// make enough room for the new roi

                                        // transfer the old to the new
    for ( int r = 0; r < NumRois; r++ ) {

        Rois[ r ]   = oldrois[ r ];
        StringCopy ( RoiNames[ r ], oldroinames[ r ] );
        }


    delete[]    oldrois;                // now we can get rid of the old data
    }


if ( checkoverlap )                     // purge the would-be new selection from existing rois
    for ( int r = 0; r < NumRois; r++ )
        roisel  -= Rois[ r ].Selection;


if ( ! (bool) roisel )                  // is the new roi finally empty?
    return  false;                      // so just leave the party, even if the variables were set with an extra roi


                                        // add the new roi
Rois[ NumRois ].Selection = TSelection ( Dimension, OrderTemplate, &roisel );
Rois[ NumRois ].Selection.Set ();       // set to the whole template

                                        // set name
StringCopy      ( RoiNames[ NumRois ], roiname, RoiNameLength - 1 );
StringCleanup   ( RoiNames[ NumRois ] );

                                        // check roi name not empty
if ( StringIsEmpty ( RoiNames[ NumRois ] ) )
    sprintf ( RoiNames[ NumRois ], "roi%0d", NumRois + 1 );

// check roi names are all different?


NumRois++;                              // now, we do have a new roi


if ( doallocate )                       // we assume it's the end
    AddRoiFinalize ();


return  true;
}

                                        // final scan to get the colors, # of selected items etc...
void    TRois::AddRoiFinalize ()
{
TGLColoring coloring ( 0, Discrete, 0, NumRois - 1, TGLColor<GLfloat> ( 0.00, 0.00, 0.00, 1.0 ),
                                                    TGLColor<GLfloat> ( 0.50, 0.50, 0.50, 1.0 )  );

TSelection  selcount ( Dimension, OrderSorted );
selcount.Reset ();

                                        // rescan and update
for ( int r = 0; r < NumRois; r++ ) {
                                        // cumulate all non-redundant selections
    selcount            += Rois[ r ].Selection;
                                        // filter out elements actually in use
    AtomsNotSelected    -= Rois[ r ].Selection;

    coloring.GetColor ( r, Rois[ r ].Color );
    }


TotalSelected   = (int) selcount;

UpdateToRoisSelected ();
}


void    TRois::RemoveRoi ()
{
if ( NumRois == 0 )
    return;

NumRois--;
}


//----------------------------------------------------------------------------
                                        // scan all Rois selection, and flag which are selected
void    TRois::UpdateToRoisSelected ()
{
for ( int r = 0; r < NumRois; r++ )
    RoisSelected.Set ( r, (bool) Rois[ r ].Selection );
}

                                        // scan RoisSelected, and set / clear the Rois
void    TRois::UpdateFromRoisSelected ()
{
for ( int r = 0; r < NumRois; r++ )
    if ( RoisSelected[ r ] )
        Set ( r );
    else
        Reset ( r );
}


void    TRois::CumulateInto ( TSelection& sel, int fromvalue, int tovalue )
{
if ( fromvalue >= 0 && tovalue >= 0 )
    sel.Reset ( fromvalue, tovalue );
else
    sel.Reset ();

for ( int r = 0; r < NumRois; r++ )
    sel += Rois[ r ].Selection;
}


//----------------------------------------------------------------------------
const TRoi* TRois::IndexToRoi ( int index )     const
{
for ( int r = 0; r < NumRois; r++ )
    if ( Rois[ r ].Selection[ index ] )
        return &Rois[ r ];

return  0;
}


//----------------------------------------------------------------------------
void    TRois::ReadFile ( const char* file )
{
ifstream*           is              = new ifstream ( TFileName ( file, TFilenameExtendedPath ) );

if ( ! is || is->fail () )
    return;

char                buff[ EditSizeTextLong ];

                                        // read magic number
GetToken ( is, buff );

if ( ! StringStartsWith ( buff, ROITXT_MAGICNUMBER1 ) ) {
    delete is;
    return;
    }
                                        // ROITXT_MAGICNUMBER1 is a list of indexes
Type    = RoiIndex;


StringCopy    ( Name, file, RoiNameLength - 1 );
GetFilename   ( Name );
StringCleanup ( Name );


Dimension   = StringToInteger ( GetToken ( is, buff ) );

if ( Dimension <= 0 ) {
    delete is;
    return;
    }


NumRois     = StringToInteger ( GetToken ( is, buff ) );

if ( NumRois <= 0 ) {
    delete is;
    return;
    }


//TSuperGauge         Gauge ( "Reading ROIs", NumRois, SuperGaugeLevelInter );

                                        // init variables
Allocate ( NumRois, Dimension );


                                        // using the incremental update
int                 numrois         = NumRois;
NumRois = 0;

char                roiname[ MaxPathShort ];
TSelection          seltempl ( Dimension, OrderArbitrary );


for ( int r = 0; r < numrois; r++ ) {

//  Gauge.Next ();

                                        // Roi name
    is->getline ( roiname, RoiNameLength );
    roiname[ RoiNameLength - 1 ] = 0;
    StringCleanup ( roiname );

                                        // Get indexes (buffer is real big, should be enough for a long time)
    is->getline ( buff, EditSizeTextLong );

                                        // check for letters that krept into the indexes
    if ( Type == RoiIndex && IsStringAmong ( buff, "A B C D E F G H I J K L M N O P Q R S T U V W X Y Z a b c d e f g h i j k l m n o p q r s t u v w x y z" ) )
        ShowMessage ( "There seems to be a problem with some of the indexes." NewLine "Please check your file!", "Error in ROIs file", ShowMessageWarning );


    seltempl.Reset ();
    seltempl.Set ( buff, true );


    if ( ! AddRoi ( seltempl, roiname, false, false ) )
        ShowMessage ( "There seems to be some overlapping or missing indexes." NewLine "Please check your file!", "Error in ROIs file", ShowMessageWarning );
    }


AddRoiFinalize ();                      // just at the end


delete is;
}


//----------------------------------------------------------------------------
void    TRois::WriteFile ( const char* file )   const
{
if ( NumRois == 0 || Dimension == 0 || StringIsEmpty ( file ) )
    return;


ofstream            os ( TFileName ( file, TFilenameExtendedPath ) );

if ( os.fail () )
    return;


os << ( Type == RoiIndex ? ROITXT_MAGICNUMBER1 : ROITXT_MAGICNUMBER1 ) << fastendl
   << Dimension    << fastendl
   << NumRois      << fastendl;


for ( int r = 0; r < NumRois; r++ ) {

    os << RoiNames[ r ] << fastendl;

    for ( TIteratorSelectedForward seli ( Rois[ r ].Selection ); (bool) seli; ++seli )
        os << ( seli() + 1 ) << " ";

    os << fastendl;
    }
}


//----------------------------------------------------------------------------
void    TRois::Average ( TArray2<float>& datainout, long tf1, long tf2, FilterTypes how, TArray2<float> *dataout )  const
{
OmpParallelFor

for ( int r = 0; r < NumRois; r++ )    // dataout dimension is the number of rois
    Rois[ r ].Average ( datainout, tf1, tf2, how, dataout ? (*dataout)[ r ] : 0 );

                                        // set to 0 the atoms not in any ROI
for ( TIteratorSelectedForward seli ( AtomsNotSelected ); (bool) seli; ++seli )
                                        // clear up memory directly
    ClearVirtualMemory ( datainout[ seli() ] + tf1, ( tf2 - tf1 + 1 ) * datainout.AtomSize () );
}


//----------------------------------------------------------------------------
void    TRois::Average ( TArray1<float> &datainout, FilterTypes how )   const
{
OmpParallelFor

for ( int r = 0; r < NumRois; r++ )
    Rois[ r ].Average ( datainout, how );

                                        // set to 0 the atoms not in any ROI
for ( TIteratorSelectedForward seli ( AtomsNotSelected ); (bool) seli; ++seli )

    datainout[ seli() ] = 0;
}


//----------------------------------------------------------------------------
void    TRois::Average ( TArray1<TVector3Float> &datainout, FilterTypes how )   const
{
OmpParallelFor

for ( int r = 0; r < NumRois; r++ )
    Rois[ r ].Average ( datainout, how );

                                        // set to 0 the atoms not in any ROI
for ( TIteratorSelectedForward seli ( AtomsNotSelected ); (bool) seli; ++seli )

    datainout[ seli() ].Reset ();
}


//----------------------------------------------------------------------------
                                        // will resize dataout if needed
void    TRois::Average ( const TMaps& datain, TMaps& dataout, FilterTypes how )   const
{
dataout.Resize ( datain.GetNumMaps (), NumRois );

OmpParallelFor

for ( int r = 0; r < NumRois; r++ )
    Rois[ r ].Average ( datain, dataout, r, how );
}


//----------------------------------------------------------------------------
                                        // will create and resize dataout if needed
void    TRois::Average ( const TGoMaps& datain, TGoMaps& dataout, FilterTypes how )   const
{
dataout.DeallocateMemory ();

                                        // allocate all group of maps
for ( int gofi = 0; gofi < datain.NumGroups (); gofi++ )

    dataout.Add ( new TMaps ( datain[ gofi ].GetNumMaps (), NumRois ), false );


for ( int gofi = 0; gofi < datain.NumGroups (); gofi++ )

    Average ( datain[ gofi ], dataout[ gofi ], how );
}


//----------------------------------------------------------------------------
void    TRois::Set ()
{
for ( int r = 0; r < NumRois; r++ )
    Set ( r );
}


void    TRois::Set ( int roi )
{
if ( ! Clipped ( roi, 0, GetNumRois () - 1 ) )
    return;

RoisSelected.Set ( roi );
Rois[ roi ].Selection.Set ();
}


//----------------------------------------------------------------------------
void    TRois::Reset ()
{
for ( int r = 0; r < NumRois; r++ )
    Reset ( r );
}


void    TRois::Reset ( int roi )
{
if ( ! Clipped ( roi, 0, GetNumRois () - 1 ) )
    return;

RoisSelected.Reset ( roi );
Rois[ roi ].Selection.Reset ();
}


//----------------------------------------------------------------------------
void    TRois::ShiftUp ( int num )
{
RoisSelected.ShiftUp ( 0, GetNumRois () - 1, num );

UpdateFromRoisSelected ();
}


void    TRois::ShiftDown ( int num )
{
RoisSelected.ShiftDown ( 0, GetNumRois () - 1, num );

UpdateFromRoisSelected ();
}


//----------------------------------------------------------------------------
char*   TRois::RoiNamesToText ( char* names )   const
{
if ( names == 0 )
    return  0;

ClearString ( names );

for ( int r = 0; r < NumRois; r++ )
    sprintf ( StringEnd ( names ), "%s\"%s\"", r ? ", " : "", RoiNames[ r ] );

return  names;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
