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

#include    <fstream>

#include    <owl/pch.h>

#include    "TInverseMatrixDoc.h"

#include    "Math.Utils.h"
#include    "Math.Stats.h"
#include    "TVector.h"
#include    "TMaps.h"
#include    "Strings.Utils.h"
#include    "Files.Utils.h"
#include    "TTracks.h"

#include    "Files.WriteInverseMatrix.h"

#include    "TTracksView.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;
using namespace arma;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TInverseMatrixDoc::TInverseMatrixDoc ( TDocument *parent )
        : TBaseDoc ( parent )
{
ContentType         = ContentTypeMatrix;

NumElectrodes       = 0;
NumSolPoints        = 0;
NumRegularizations  = 0;
AveragingPrecedence = AverageDefault;
}


bool    TInverseMatrixDoc::Commit ( bool force )
{
if ( ! ( IsDirty () || force ) )
    return true;


TFileName           safepath ( GetDocPath (), TFilenameExtendedPath );

                                        // ISBIN_MAGICNUMBER3 type
WriteInverseMatrixHeader    (   NumElectrodes,          NumSolPoints,               NumRegularizations,             ! IsVector ( AtomTypeUseOriginal ),
                                ElectrodesNames,        SolutionPointsNames,       &RegularizationsValues[ 0 ],    &RegularizationsNames, 
                                safepath 
                            );

                                        // append matrix body, while converting to float type
ofstream            ofs ( safepath, ios::binary | ios::app );

                                        // row-major
for ( int reg = 0; reg < GetMaxRegularization (); reg++ )

    ofs.write ( (char *) M[ reg ].GetArray (), SingleMatrixMemorySize () );

                                        // column-major
//for ( int reg = 0; reg < GetMaxRegularization (); reg++ ) {
//                                        // transform to row major
//    AMatrix     Mt      = M[ reg ].t ();
//
//    ofs.write ( (char *) Mt.memptr (), SingleMatrixMemorySize () );
//    }


SetDirty ( false );

return  true;
}


bool    TInverseMatrixDoc::Revert ( bool clear )
{
if ( ! TFileDocument::Revert ( clear ) )
    return  false;

if ( ! clear )
    Open ( GetOpenMode (), GetDocPath () );
//  Open ( ofRead, 0 );

return true;
}


bool    TInverseMatrixDoc::InitDoc ()
{
if ( ! IsOpen () ) {
    if ( ! Open ( ofRead, 0 ) )
        return  false;
    }

return  true;
}


bool    TInverseMatrixDoc::Close ()
{
return  TFileDocument::Close ();
}


//----------------------------------------------------------------------------
int     TInverseMatrixDoc::GetRegularizationIndex ( const char *regstring )  const
{
if ( NumRegularizations <= 1 )
    return  0;

                                        // !works because list is sorted!
for ( int reg = 0; reg < NumRegularizations; reg++ )
    if ( StringContains ( RegularizationsNames[ reg ], regstring ) )
        return  reg;                    // found it!

                                        // not found, try another approach:
                                        // scan requested string & maximum reg for numbers
                                        // then compute a ratio of the percentages
double              regf            = GetFirstNumber ( regstring );
double              regmax          = GetFirstNumber ( RegularizationsNames[ NumRegularizations - 1 ] );
int                 reg             = Clip ( Round ( regf / regmax * ( NumRegularizations - 1 ) ), 0, NumRegularizations - 1 );

return  reg;
}


//----------------------------------------------------------------------------
                                        // search through a range of time frames
int     TInverseMatrixDoc::GetGlobalRegularization  (   const TMaps*        maps,
                                                        TTracksView*        eegview,    
                                                        long                fromtf,     long        totf,   long        steptf 
                                                    )   const
{
if ( ! ( GetNumRegularizations () >= 3 
         && ( maps || eegview )        ) )
    return  0;


TEasyStats          stat; // ( ( totf - fromtf ) / steptf + 1 );


for ( long tf = fromtf; tf <= totf; tf += steptf )

    if ( maps )     stat.Add ( GetBestRegularization ( &(*maps)[ tf ],  0,          tf ) );
    else            stat.Add ( GetBestRegularization ( 0,               eegview,    tf ) );


return  Clip ( Round ( stat.Mean () ), 0, GetNumRegularizations () - 1 );
}


//----------------------------------------------------------------------------
                                        // search for 1 time point only
                                        // input is either a map or an eegview + tf - code is a bit ugly, but better than duplicating the search logic...
int     TInverseMatrixDoc::GetBestRegularization(   const TMap*     map, 
                                                    TTracksView*    eegview,    long        tf 
                                                )   const
{
if ( GetNumRegularizations () < 3 )
    return  0;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int                 regi;
double              dv1;
double              dv2;

TVector<double>     RegSearch ( GetNumRegularizations () );

RegSearch.ResetMemory ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        
TTracks<float>      EegBuff;            // !will be resized in GetTracks!
TArray1<float>      InvBuffS ( NumSolPoints );
//EVector           Einv ( NumSolPoints );

                                        // scan across all regularizations, for current time cursor
for ( regi = 0; regi < GetNumRegularizations (); regi++ ) {

                                        // get next value
    if ( map )

        MultiplyMatrix      ( regi, *map, InvBuffS );

    else {

        eegview->GetTracks  ( tf, tf,   EegBuff, ReferenceAverage  );

        MultiplyMatrix      ( regi, EegBuff, 0, InvBuffS );
        }


    for ( int sp = 0; sp < NumSolPoints; sp++ )
//      RegSearch [ regi ]  = max ( fabs ( InvBuffS[ sp ] ), regnext );
        RegSearch [ regi ] += fabs ( InvBuffS[ sp ] );

                                        // we need to fill slot 0 and 1 before we can test 3 points
    if ( regi < 2 )
        continue;

                                        // we have enough data for 2 deltas
    dv1     = RegSearch[ regi - 1 ] - RegSearch[ regi - 2 ];
    dv2     = RegSearch[ regi     ] - RegSearch[ regi - 1 ];

//    DBGV5 ( regi, reg0 * 1e4, dv1 * 1e4, dv2 * 1e4, ( dv1 + dv2 ) / 2 / reg0, "regi, reg0, dv1, dv2, K" );

                                        // climbing?
    if ( dv1 >= 0 )     continue;

                                        // local well?
//  if ( dv2 >= 0 )     break;
    if ( dv2 >= 0 )     continue;

                                        // deceleration?
//  if ( ( dv2 - dv1 ) / RegSearch[ regi - 1 ] > 0.1  )         break;
    if ( ( dv1 + dv2 ) / 2 / RegSearch[ regi - 1 ] >= /*-0.045*/ -0.06 )  break;    // lower value -> earlier stop -> lower reg
    }


/*                                        // filling last part for display
for ( int regi2 = regi + 1; regi2 < GetNumRegularizations (); regi2++ ) {

    if ( map )

        MultiplyMatrix      ( regi2, *map, InvBuffS );

    else {
        eegview->GetTracks  ( tf, tf,   EegBuff, ReferenceAverage  );

        MultiplyMatrix      ( regi2, EegBuff, 0, InvBuffS );
        }


    for ( int sp = 0; sp < NumSolPoints; sp++ )
//      RegSearch [ regi2 ]  = max ( fabs ( InvBuffS[ sp ] ), regnext );
        RegSearch [ regi2 ] += fabs ( InvBuffS[ sp ] );
    }

TFileName       _file;
StringCopy  ( _file, "E:\\Data\\RegSearch.", IntegerToString ( tf, 3 ), ".sef" );
RegSearch.WriteFile ( _file, IntegerToString ( tf, 3 ) );
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  Clip ( regi, 0, GetNumRegularizations () - 1 );
}


//----------------------------------------------------------------------------
const char*     TInverseMatrixDoc::GetInverseName ( char* isname )   const
{
TFileName           filename ( GetDocPath (), TFilenameExtendedPath );

                                        // Unfortunately, inverse name is not store within the matrices, we have to rely on the file name...
GetFilename     ( filename );

                                        // let see if we can recognize some predefined inverse names
if      ( StringIsEmpty  (				 filename ) )											StringCopy      ( isname, InfixDefaultEsi       );
else if ( StringContains ( (const char*) filename, (const char*) "." InfixInverseMN         ) ) StringCopy      ( isname, InfixInverseMN        );
else if ( StringContains ( (const char*) filename, (const char*) "." InfixInverseWMN        ) ) StringCopy      ( isname, InfixInverseWMN       );
else if ( StringContains ( (const char*) filename, (const char*) "." InfixInverseLoreta     ) ) StringCopy      ( isname, InfixInverseLoreta    );
else if ( StringContains ( (const char*) filename, (const char*) "." InfixInverseLaura      ) ) StringCopy      ( isname, InfixInverseLaura     );
else if ( StringContains ( (const char*) filename, (const char*) "." InfixInverseEpifocus   ) ) StringCopy      ( isname, InfixInverseEpifocus  );
else if ( StringContains ( (const char*) filename, (const char*) "." InfixInverseSLoreta    ) ) StringCopy      ( isname, InfixInverseSLoreta   );
else if ( StringContains ( (const char*) filename, (const char*) "." InfixInverseELoreta    ) ) StringCopy      ( isname, InfixInverseELoreta   );
else if ( StringContains ( (const char*) filename, (const char*) "." InfixInverseDale       ) ) StringCopy      ( isname, InfixInverseDale      );
else																					      { StringShrink    ( filename, isname, 20 ); StringCamelCase ( isname ); }


return  isname;
}


//----------------------------------------------------------------------------
void    TInverseMatrixDoc::SetDefaultVariables ()
{
ElectrodesNames      .Set ( NumElectrodes, InverseMaxElectrodeName      );
SolutionPointsNames  .Set ( NumSolPoints,  InverseMaxSolutionPointName  );

RegularizationsValues.Resize ( NumRegularizations );
RegularizationsNames .Set    ( NumRegularizations, InverseMaxRegularizationName );

                                        // set default values
                                        // electrodes names
for ( int el = 0; el < NumElectrodes; el++ )
    sprintf ( ElectrodesNames[ el ],       "e%0d", el + 1 );

                                        // sp names
for ( int sp = 0; sp < NumSolPoints; sp++ )
    sprintf ( SolutionPointsNames[ sp ],   "sp%0d", sp + 1 );

                                        // regularization names
for ( int reg = 0; reg < NumRegularizations; reg++ ) {
    RegularizationsValues[ reg ]    = 0;
    StringCopy  ( RegularizationsNames[ reg ], InfixRegularization " ", IntegerToString ( reg ) );
    }
}


//----------------------------------------------------------------------------
void    TInverseMatrixDoc::MultiplyMatrix ( int reg, const TMap& map, TArray1<float>& inv )  const
{
reg     = reg == RegularizationAutoLocal ? GetBestRegularization ( &map, 0, 0 ) 
                                         : Clip ( reg, 0, GetMaxRegularization () - 1 );


if ( IsVector ( AtomTypeUseOriginal ) ) {
                                        // inverse is vectorial, results are scalar so return the norm of vectors
    OmpParallelFor

    for ( int sp = 0; sp < NumSolPoints; sp++ ) {

        const AReal*        toinvfx     = &M[ reg ] ( 3 * sp    , 0 );
        const AReal*        toinvfy     = &M[ reg ] ( 3 * sp + 1, 0 );
        const AReal*        toinvfz     = &M[ reg ] ( 3 * sp + 2, 0 );
        double              sumx        = 0;
        double              sumy        = 0;
        double              sumz        = 0;

        for ( int el = 0; el < NumElectrodes; el++, toinvfx++, toinvfy++, toinvfz++ ) {
            sumx    += *toinvfx * map[ el ];
            sumy    += *toinvfy * map[ el ];
            sumz    += *toinvfz * map[ el ];
            }

        inv[ sp ]   = sqrt ( sumx * sumx + sumy * sumy + sumz * sumz );
        }
    }
else {
                                        // correct case: inverse is scalar, and results too
    OmpParallelFor

    for ( int sp = 0; sp < NumSolPoints; sp++ ) {

        const AReal*        toinvf      = &M[ reg ] ( sp, 0 );
        double              sum         = 0;

        for ( int el = 0; el < NumElectrodes; el++, toinvf++ )
            sum     += *toinvf * map[ el ];

        inv[ sp ]   = sum;
        }
    }
}


void    TInverseMatrixDoc::MultiplyMatrix ( int reg, const TMap& map, TArray1<TVector3Float>& inv )  const
{
reg     = reg == RegularizationAutoLocal ? GetBestRegularization ( &map, 0, 0 ) 
                                         : Clip ( reg, 0, GetMaxRegularization () - 1 );


if ( IsVector ( AtomTypeUseOriginal ) ) {
                                        // correct case: inverse is vectorial, and results too
    OmpParallelFor

    for ( int sp = 0; sp < NumSolPoints; sp++ ) {

        const AReal*        toinvfx     = &M[ reg ] ( 3 * sp    , 0 );
        const AReal*        toinvfy     = &M[ reg ] ( 3 * sp + 1, 0 );
        const AReal*        toinvfz     = &M[ reg ] ( 3 * sp + 2, 0 );
        double              sumx        = 0;
        double              sumy        = 0;
        double              sumz        = 0;

        for ( int el = 0; el < NumElectrodes; el++, toinvfx++, toinvfy++, toinvfz++ ) {
            sumx    += *toinvfx * map[ el ];
            sumy    += *toinvfy * map[ el ];
            sumz    += *toinvfz * map[ el ];
            }

        inv[ sp ].X     = sumx;
        inv[ sp ].Y     = sumy;
        inv[ sp ].Z     = sumz;
        }
    }
else {
                                        // inverse is scalar, results are vectorial so return dummy vectors ( value, 0, 0 )
    OmpParallelFor

    for ( int sp = 0; sp < NumSolPoints; sp++ ) {

        const AReal*        toinvf      = &M[ reg ] ( sp, 0 );
        double              sum         = 0;

        for ( int el = 0; el < NumElectrodes; el++, toinvf++ )
            sum     += *toinvf * map[ el ];

        inv[ sp ].X     = sum;
        inv[ sp ].Y     = 0;
        inv[ sp ].Z     = 0;
        }
    }
}


//----------------------------------------------------------------------------
/*                                        // Size of inv tells if results are vectorial or scalar
void    TInverseMatrixDoc::MultiplyMatrix ( int reg, const AVector& map, AVector& inv )  const
{
#if defined (_DEBUG)
if ( reg == RegularizationAutoLocal ) { ShowMessage ( "Not implemented", "TInverseMatrixDoc::MultiplyMatrix auto local", ShowMessageWarning ); exit ( 1 ); }
#endif

reg     = reg == RegularizationAutoLocal ? 0 // GetBestRegularization ( &map, 0, 0 ) 
                                         : Clip ( reg, 0, GetMaxRegularization () - 1 );


if ( IsVector ( AtomTypeUseOriginal ) ) {

    if ( inv.n_rows == M[ reg ].GetDim1 () )
                                        // best case: inverse vectorial -> results vectorial (multiplexed)
        inv     = M[ reg ] * map;

    else {
                                        // other case: inverse vectorial -> results scalar - return the norm of vectors
        AVector         invv        = M[ reg ] * map;
        
        for ( int sp = 0; sp < NumSolPoints; sp++ )

            inv ( sp )  = sqrt ( Square ( invv ( 3 * sp ) ) + Square ( invv ( 3 * sp + 1 ) ) + Square ( invv ( 3 * sp + 2 ) ) );
        }
    }
else { // Scalar inverse

    if ( inv.n_rows == M[ reg ].GetDim1 () )
                                        // best case: inverse scalar -> results scalar
        inv     = M[ reg ] * map;

    else {
                                        // other case: inverse scalar -> results scalar - X=result Y=Z=0
        AVector         invs        = M[ reg ] * map;

        inv.AReset ();
        
        for ( int sp = 0; sp < NumSolPoints; sp++ )

            inv ( 3 * sp ) = invs ( sp );
        }
    }
}
*/

//----------------------------------------------------------------------------
                                        // At specified TF
void    TInverseMatrixDoc::MultiplyMatrix ( int reg, const TArray2<float>& eeg, int tf, TArray1<float>& inv )  const
{
                                        // TODO: GetBestRegularization  with TArray2
#if defined (_DEBUG)
if ( reg == RegularizationAutoLocal ) { ShowMessage ( "Not implemented", "TInverseMatrixDoc::MultiplyMatrix auto local", ShowMessageWarning ); exit ( 1 ); }
#endif

reg     = reg == RegularizationAutoLocal ? 0 // GetBestRegularization ( &map, 0, 0 ) 
                                         : Clip ( reg, 0, GetMaxRegularization () - 1 );


if ( IsVector ( AtomTypeUseOriginal ) ) {
                                        // inverse is vectorial, results are scalar so return the norm of vectors
    OmpParallelFor

    for ( int sp = 0; sp < NumSolPoints; sp++ ) {

        const AReal*        toinvfx     = &M[ reg ] ( 3 * sp    , 0 );
        const AReal*        toinvfy     = &M[ reg ] ( 3 * sp + 1, 0 );
        const AReal*        toinvfz     = &M[ reg ] ( 3 * sp + 2, 0 );
        double              sumx        = 0;
        double              sumy        = 0;
        double              sumz        = 0;

        for ( int el = 0; el < NumElectrodes; el++, toinvfx++, toinvfy++, toinvfz++ ) {
            sumx    += *toinvfx * eeg ( el, tf );
            sumy    += *toinvfy * eeg ( el, tf );
            sumz    += *toinvfz * eeg ( el, tf );
            }

        inv[ sp ]   = sqrt ( sumx * sumx + sumy * sumy + sumz * sumz );
        }
    }
else {
                                        // correct case: inverse is scalar, and results too
    OmpParallelFor

    for ( int sp = 0; sp < NumSolPoints; sp++ ) {

        const AReal*        toinvf      = &M[ reg ] ( sp, 0 );
        double              sum         = 0;

        for ( int el = 0; el < NumElectrodes; el++, toinvf++ )
            sum     += *toinvf * eeg ( el, tf );

        inv[ sp ]   = sum;
        }
    }
}

                                        // At specified TF
void    TInverseMatrixDoc::MultiplyMatrix ( int reg, const TArray2<float>& eeg, int tf, TArray1<TVector3Float>& inv )  const
{
                                        // TODO: GetBestRegularization  with TArray2
#if defined (_DEBUG)
if ( reg == RegularizationAutoLocal ) { ShowMessage ( "Not implemented", "TInverseMatrixDoc::MultiplyMatrix auto local", ShowMessageWarning ); exit ( 1 ); }
#endif

reg     = reg == RegularizationAutoLocal ? 0 // GetBestRegularization ( &map, 0, 0 ) 
                                         : Clip ( reg, 0, GetMaxRegularization () - 1 );


if ( IsVector ( AtomTypeUseOriginal ) ) {
                                        // correct case: inverse is vectorial, and results too
    OmpParallelFor

    for ( int sp = 0; sp < NumSolPoints; sp++ ) {

        const AReal*        toinvfx     = &M[ reg ] ( 3 * sp    , 0 );
        const AReal*        toinvfy     = &M[ reg ] ( 3 * sp + 1, 0 );
        const AReal*        toinvfz     = &M[ reg ] ( 3 * sp + 2, 0 );
        double              sumx        = 0;
        double              sumy        = 0;
        double              sumz        = 0;

        for ( int el = 0; el < NumElectrodes; el++, toinvfx++, toinvfy++, toinvfz++ ) {
            sumx    += *toinvfx * eeg ( el, tf );
            sumy    += *toinvfy * eeg ( el, tf );
            sumz    += *toinvfz * eeg ( el, tf );
            }

        inv[ sp ].X     = sumx;
        inv[ sp ].Y     = sumy;
        inv[ sp ].Z     = sumz;
        }
    }

else {
                                        // inverse is scalar, results are vectorial so return dummy vectors ( value, 0, 0 )
    OmpParallelFor

    for ( int sp = 0; sp < NumSolPoints; sp++ ) {

        const AReal*        toinvf      = &M[ reg ] ( sp, 0 );
        double              sum         = 0;

        for ( int el = 0; el < NumElectrodes; el++, toinvf++ )
            sum     += *toinvf * eeg ( el, tf );

        inv[ sp ].X     = sum;
        inv[ sp ].Y     = 0;
        inv[ sp ].Z     = 0;
        }
    }
}


//----------------------------------------------------------------------------
void    TInverseMatrixDoc::MultiplyMatrix ( int reg, const AMatrix& eeg, int tf, TArray1<TVector3Float>& inv )  const
{
                                        // TODO: GetBestRegularization  with TArray2
#if defined (_DEBUG)
if ( reg == RegularizationAutoLocal ) { ShowMessage ( "Not implemented", "TInverseMatrixDoc::MultiplyMatrix auto local", ShowMessageWarning ); exit ( 1 ); }
#endif

reg     = reg == RegularizationAutoLocal ? 0 // GetBestRegularization ( &map, 0, 0 ) 
                                         : Clip ( reg, 0, GetMaxRegularization () - 1 );


if ( IsVector ( AtomTypeUseOriginal ) ) {
                                        // correct case: inverse is vectorial, and results too
    OmpParallelFor

    for ( int sp = 0; sp < NumSolPoints; sp++ ) {

        const AReal*        toinvfx     = &M[ reg ] ( 3 * sp    , 0 );
        const AReal*        toinvfy     = &M[ reg ] ( 3 * sp + 1, 0 );
        const AReal*        toinvfz     = &M[ reg ] ( 3 * sp + 2, 0 );
        double              sumx        = 0;
        double              sumy        = 0;
        double              sumz        = 0;

        for ( int el = 0; el < NumElectrodes; el++, toinvfx++, toinvfy++, toinvfz++ ) {
            sumx    += *toinvfx * eeg.at ( el, tf );
            sumy    += *toinvfy * eeg.at ( el, tf );
            sumz    += *toinvfz * eeg.at ( el, tf );
            }

        inv[ sp ].X     = sumx;
        inv[ sp ].Y     = sumy;
        inv[ sp ].Z     = sumz;
        }
    }

else {
                                        // inverse is scalar, results are vectorial so return dummy vectors ( value, 0, 0 )
    OmpParallelFor

    for ( int sp = 0; sp < NumSolPoints; sp++ ) {

        const AReal*        toinvf      = &M[ reg ] ( sp, 0 );
        double              sum         = 0;

        for ( int el = 0; el < NumElectrodes; el++, toinvf++ )
            sum     += *toinvf * eeg.at ( el, tf );

        inv[ sp ].X     = sum;
        inv[ sp ].Y     = 0;
        inv[ sp ].Z     = 0;
        }
    }
}


//----------------------------------------------------------------------------
// The real impact of  AveragingPrecedence  occurs when reading a vectorial inverse to a scalar buffer
// otherwise the sums remain in their native dimensions, or better (scalar in a vector)

void    TInverseMatrixDoc::GetInvSol ( int reg, long tf1, long tf2, TArray1<float>& inv, TTracksView* eegview, TRois* rois )     const
{
Clipped ( reg, 0, GetMaxRegularization () - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TTracks<float>      EegBuff;            // !will be resized in GetTracks!
int                 numtf           = tf2 - tf1 + 1;


if ( numtf == 1 ) {
                                        // always force average reference
    eegview->GetTracks ( tf1, tf1, EegBuff, ReferenceAverage );
                                        // then call inverse solution
    MultiplyMatrix ( reg, EegBuff, 0, inv );
    }

else { // numtf > 1

    if ( AveragingPrecedence == AverageBeforeInverse ) {
                                        // always force average reference
        eegview->GetTracks ( tf1, tf2, EegBuff, ReferenceAverage );


        TMap                EegBuffAvg ( NumElectrodes );

        for ( int el = 0; el < NumElectrodes; el++ )
        for ( int tf0 = 0; tf0 < numtf; tf0++ )
            EegBuffAvg[ el ]   += EegBuff ( el, tf0 );


        EegBuffAvg     /= numtf;

                                        // then call inverse solution
        MultiplyMatrix ( reg, EegBuffAvg, inv );
        } // if AverageBeforeInverse

    else { // AverageAfterInverse

        TArray1<float>      InvBuffS ( NumSolPoints );
        inv.ResetMemory ();


        for ( int tf = tf1; tf <= tf2; tf++ ) {
                                        // always force average reference
            eegview->GetTracks ( tf, tf, EegBuff, ReferenceAverage );

                                        // compute inverse in local buffer
            MultiplyMatrix ( reg, EegBuff, 0, InvBuffS );

                                        // cumulate inverse
            for ( int sp = 0; sp < NumSolPoints; sp++ )
                inv[ sp ]  += InvBuffS[ sp ];
            }

        for ( int sp = 0; sp < NumSolPoints; sp++ )
            inv[ sp ]  /= (double) numtf;
        }

    } // numtf > 1


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // optional ROIing
if ( rois )
    rois->Average ( inv, FilterTypeMean );
}


//----------------------------------------------------------------------------
void    TInverseMatrixDoc::GetInvSol ( int reg, long tf1, long tf2, TArray1<TVector3Float>& inv, TTracksView* eegview, TRois* rois )     const
{
Clipped ( reg, 0, GetMaxRegularization () - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TTracks<float>      EegBuff;            // !will be resized in GetTracks!
int                 numtf           = tf2 - tf1 + 1;


if ( numtf == 1 ) {
                                        // always force average reference
    eegview->GetTracks ( tf1, tf1, EegBuff, ReferenceAverage );
                                        // then call inverse solution
    MultiplyMatrix ( reg, EegBuff, 0, inv );
    }

else { // numtf > 1

    if ( AveragingPrecedence == AverageBeforeInverse ) {
                                        // always force average reference
        eegview->GetTracks ( tf1, tf2, EegBuff, ReferenceAverage );


        TMap                EegBuffAvg ( NumElectrodes );

        for ( int el = 0; el < NumElectrodes; el++ )
        for ( int tf0 = 0; tf0 < numtf; tf0++ )
            EegBuffAvg[ el ]   += EegBuff ( el, tf0 );


        EegBuffAvg     /= numtf;

                                        // then call inverse solution
        MultiplyMatrix ( reg, EegBuffAvg, inv );
        } // if AverageBeforeInverse

    else { // AverageAfterInverse

        TArray1<TVector3Float>  InvBuffV ( NumSolPoints );
        inv.ResetMemory ();


        for ( int tf = tf1; tf <= tf2; tf++ ) {
                                        // always force average reference
            eegview->GetTracks ( tf, tf, EegBuff, ReferenceAverage );

                                        // compute inverse in local buffer
            MultiplyMatrix ( reg, EegBuff, 0, InvBuffV );

                                        // cumulate inverse
            for ( int sp = 0; sp < NumSolPoints; sp++ )
                inv[ sp ]  += InvBuffV[ sp ];
            }

        for ( int sp = 0; sp < NumSolPoints; sp++ )
            inv[ sp ]  /= (double) numtf;
        }
    } // numtf > 1


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // optional ROIing
if ( rois )
    rois->Average ( inv, FilterTypeMean );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
