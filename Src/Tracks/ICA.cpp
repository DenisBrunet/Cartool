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

#include    "ICA.h"
#include    "PCA.h"

#include    "Math.Utils.h"
#include    "Math.Random.h"
#include    "Math.Armadillo.h"
#include    "TVector.h"

#include    "TMaps.h"
#include    "TExportTracks.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;

namespace crtl {

//----------------------------------------------------------------------------
                                        // a in [1..2]
double      G1 ( double x, double a )
{
return  log ( cosh ( a * x ) ) / a;
}


double      G1Prime ( double x, double a )
{
return  tanh ( a * x );
}


double      G2 ( double x )
{
return  - exp ( - Square ( x ) / 2 );
}


double      G2Prime ( double x )
{
return  x * exp ( - Square ( x ) / 2 );
}


//----------------------------------------------------------------------------
                                        // Do an ICA on the already loaded buffer
                                        // V and eigenvalues can be less than the input dimension
bool        ICA (   TMaps&              data,           
                    bool                robust,
                    bool                removelasteigen,
                    TMaps&              icavectors,
                    TMaps&              icadata,
                    TSuperGauge*        gauge )
{
if ( data.IsNotAllocated () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // if variablesindim1, then first dimension holds the variables (like an EEG buffer)
//int                 numdim          = data.GetDimension ();
//int                 numsamples      = data.GetNumMaps ();

                                        // not enough data?
//if ( dim2 < dim1 )
//    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we need some temp variables to call PCA

PcaResultsType      pcaresults      = PcaWhitened;
TMaps               eigenvectors;
TVector<float>      eigenvalues;
AMatrix             topca;              // might not be a squared matrix
AMatrix             towhite;
AMatrix             Etopca;              // might not be a squared matrix
AMatrix             Etowhite;
AMatrix             fromwhite;
AMatrix             Efromwhite;
TMaps               datawhite;


PCA (   data, 
        robust,
        removelasteigen,
        pcaresults,
        eigenvectors,   eigenvalues,
        Etopca,          Etowhite,
        datawhite,
        gauge
    );


//datawhite.WriteFile ( "E:\\Data\\DataWhite.sef" );


fromwhite   = towhite.t ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 whitenumel      = datawhite.GetDimension ();
int                 whitenumtf      = datawhite.GetNumMaps   ();


TVector<double>     wt   ( whitenumel );
TVector<double>     wtold( whitenumel );
TVector<double>     wtX  ( whitenumtf );
TVector<double>     gwtX ( whitenumtf );


long double         gprimewtX;
long double         XgwtX;
TMap*               map;

                                        // initial vector
//wt.Random    ( 0.0, 1.0 );
wt.Random    ( -1.0, 1.0 );
wt.Normalize ();
//wt.Normalize ( true );


int                 iter        = 0;

TFileName           _file;
char                buff[ 256 ];
StringCopy      ( _file, "E:\\Data\\convmap.", IntegerToString ( buff, iter ), ".sef" );
wt.WriteFile    ( _file, "wt" );


do {

    wtold   = wt;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    for ( int tf = 0; tf < whitenumtf; tf++ ) {

        map         = &datawhite[ tf ];

                                            // wtX: project data on w
        wtX[ tf ]   = 0;

        for ( int i  = 0; i  < whitenumel; i++  )

            wtX[ tf ]  += wt[ i ] * (*map)[ i ];
        } // for tf


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                            // first part of convergence
    for ( int tf = 0; tf < whitenumtf; tf++ )

        gwtX[ tf ]  = G2 ( wtX[ tf ] );


    XgwtX   = 0;

    for ( int i  = 0; i  < whitenumel; i++  ) {

        for ( int tf = 0; tf < whitenumtf; tf++ )

            XgwtX  += gwtX[ tf ] * datawhite ( tf, i );
        }

    XgwtX  /= whitenumel;
//    XgwtX  /= whitenumel * whitenumtf; // ??????


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                            // second part of convergence
    gprimewtX   = 0;

    for ( int tf = 0; tf < whitenumtf; tf++ )

        gprimewtX  += G2Prime ( wtX[ tf ] );

    gprimewtX  /= whitenumtf;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // new wt

    for ( int i  = 0; i  < whitenumel; i++  )

        wt[ i ]     = XgwtX - gprimewtX * wt[ i ];
//        wt[ i ]     = ( XgwtX - gprimewtX ) * wt[ i ];


    wt.Normalize ();
    //wt.Normalize ( true );


    StringCopy      ( _file, "E:\\Data\\convmap.", IntegerToString ( buff, iter + 1 ), ".sef" );
    wt.WriteFile    ( _file, "wt" );

//    if ( VkQuery () ) DBGV3 ( iter, XgwtX, - gprimewtX, "iter, XgwtX, - gprimewtX" );
//    DBGV ( wt.ScalarProduct ( wtold ) * 1000, "convergence" );

//    } while ( fabs ( wt.ScalarProduct ( wtold ) ) < 0.9999999 );
    } while ( ++iter < 10 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

AMatrix             T;
AMatrix             ET;

                                        // copy single topography
TMaps               wicavectors;

wicavectors.Resize      ( 1, whitenumel );
T          .AResizeFast ( 1, whitenumel );


for ( int i  = 0; i  < whitenumel; i++  ) {

    wicavectors ( 0, i )    = wt[ i ];

    T ( 0, i )              = wt[ i ];
    }


                                        // apply to whitened data to have the time-course of wt
datawhite.Multiply ( ET, icadata );

                                        // transform the white icavectors back to original space
wicavectors.Multiply ( Efromwhite, icavectors );
//icavectors = wicavectors;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
