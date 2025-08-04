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

#include    "PCA.h"
#include    "ICA.h"

#include    "Math.Utils.h"
#include    "Math.Stats.h"
#include    "TArray1.h"
#include    "TArray2.h"
#include    "TVector.h"

#include    "TExportTracks.h"
#include    "TMaps.h"

#include    "Files.WriteInverseMatrix.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace arma;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const char      PcaIcaTypeString[ NumPcaIcaProcessing ][ 32 ] =
                {
                "Unknown PCA/ICA",
                "Computing PCA",
                "Computing ICA",
                };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // PCA / ICA on a single file
                                        // Does some common pre-processing then call PCA / ICA
void    PcaIcaFile (    char*               filename,
                        AtomType            datatype,
                        ReferenceType       dataref,
                        bool                datanormalized,
                        bool                covrobust,
                        int                 /*cliptf*/,
                        PcaIcaType          processing,
                        bool                savematrix,     bool            savetopo,           bool            saveeigenvalues,     
                        bool                savepcatracks,  bool            savepcapoints,     
                        const char*         prefix,
                        TSuperGauge*        gauge 
                 )
{
if ( StringIsEmpty ( filename ) )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( gauge )
    gauge->Next ( gaugepcamain );


TStrings            xyznames;

TMaps               maps ( filename, 0, datatype, dataref, &xyznames );

//int                 numel           = maps.GetDimension ();
//int                 numtf           = maps.GetNumMaps   ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( gauge )
    gauge->Next ( gaugepcamain );


if ( datanormalized )

    maps.Normalize ( datatype );


/*                                      // optionally saving templatized data
TFileName           TemplatizedFileName;
TArray2<double>     eegtempl ( numel, numtf );

for ( int i = 0; i < eegb.GetLinearDim (); i++ )
    eegtempl.GetValue ( i )  = eegb.GetValue ( i );

StringCopy          ( TemplatizedFileName, EEGDoc->GetDocPath () );
RemoveExtension     ( TemplatizedFileName );
StringAppend        ( TemplatizedFileName, ".Templatized" );
AddExtension        ( TemplatizedFileName, FILEEXT_EEGSEF );

eegtempl.WriteFile ( TemplatizedFileName );
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*{                                        // Save original data as clouds of points
TFileName           CloudFileName;
TPoints             points;
TPoints             allpoints;

                                        // Save all dimensions 3 by 3
for ( int i = 0; i < numel - 2; i += 3 ) {

    points.Reset ();

    for ( int tf = 0; tf < numtf; tf++ )
        points.Add ( eegb ( i , tf ), eegb ( i + 1 , tf ), eegb ( i + 2 , tf ) );

    allpoints  += points;


    StringCopy          ( CloudFileName, EEGDoc->GetDocPath () );
    RemoveExtension     ( CloudFileName );
    sprintf             ( StringEnd ( CloudFileName ), ".Dim %0d %0d %0d", i + 1, i + 2, i + 3 );
    AddExtension        ( CloudFileName, FILEEXT_SPIRR );

    points.WriteFile ( CloudFileName );
    }


StringCopy          ( CloudFileName, EEGDoc->GetDocPath () );
RemoveExtension     ( CloudFileName );
StringAppend        ( CloudFileName, ".All Dims" );
AddExtension        ( CloudFileName, FILEEXT_SPIRR );

allpoints.WriteFile ( CloudFileName );
}
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( gauge )
    gauge->Next ( gaugepcamain );


PcaResultsType      pcaresults      = PcaWhitened;
TMaps               eigenvectors;
TVector<float>      eigenvalues;
AMatrix             topca;              // might not be a squared matrix
AMatrix             towhite;
TMaps               pcamaps;
bool                removelasteigen = false;


if ( gauge )
    gauge->CurrentPart   = gaugepca;

                                        // returns the Eigenvectors, Eigenvalues, and the transformed data in original buffer
if       ( processing == PcaProcessing )

    PCA (   maps, 
            covrobust,
            removelasteigen,
            pcaresults,
            eigenvectors,   eigenvalues,
            topca,          towhite,
            pcamaps,
            gauge 
        );

else if ( processing == IcaProcessing )

    ICA (   maps, 
            covrobust,
            removelasteigen,
            eigenvectors,
            pcamaps,
            gauge
        );


                                        // done with the original data
maps.DeallocateMemory ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // results might have different size (#el)
//int                 pcanumel        = pcamaps     .GetDimension ();     // it could be that we are back to the original data space
int                 pcanumcomp      = eigenvectors.GetNumMaps ();
int                 pcanumtf        = pcamaps     .GetNumMaps ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           PCAInfix;

StringCopy          ( PCAInfix,     ".", processing == PcaProcessing ? "PCA" : "ICA" );

if ( datanormalized )
    StringAppend    ( PCAInfix,     " Norm" );

                                        // NOT an option anymore
//if ( ignorepolarity )
//    StringAppend    ( PCAInfix,     " NoPol" );

                                        // always ON
//if ( timecentering )                  
//    StringAppend    ( PCAInfix,     " CentTime" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // we gonna need some names
TStrings            compnames;
char                buff1[ 256 ];
char                buff2[ 256 ];

for ( int i = 0; i < pcanumcomp; i++ ) {
    StringCopy      ( buff1, "PC", IntegerToString ( buff2, i + 1 /*, NumIntegerDigits ( pcanumcomp + 1 )*/ ) );
    compnames.Add   ( buff1 );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Save eigenvalues as a matrix file .is
                                        // !To use the matrix the data should be first templatized, then centered in time!
if ( savematrix ) {

    TFileName           MatrixFileName;

    StringCopy          ( MatrixFileName, filename );
    if ( StringIsNotEmpty ( prefix ) )  PrefixFilename ( MatrixFileName, prefix, "." );
    RemoveExtension     ( MatrixFileName );
    StringAppend        ( MatrixFileName, PCAInfix, ".ToPCA" );
    AddExtension        ( MatrixFileName, FILEEXT_IS );
    CheckNoOverwrite    ( MatrixFileName );


    WriteInverseMatrixFile  (   topca,      true, 
                                xyznames,   compnames,  0, 
                                0,          0,          0,          0, 
                                MatrixFileName 
                            );


    StringReplace       ( MatrixFileName, ".ToPCA", ".ToWhite" );
    CheckNoOverwrite    ( MatrixFileName );

    WriteInverseMatrixFile (   towhite,    true, 
                                xyznames,   compnames,  0, 
                                0,          0,          0,          0, 
                                MatrixFileName 
                            );

    } // savematrix


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // testing .is file
TInverseMatrixDoc*  isdoc           = dynamic_cast<TInverseMatrixDoc*> ( CartoolDocManager->OpenDoc ( EigenvectorsFileName, dtOpenOptions ) );
int                 reg             = 0;
TArray2<double>     eegpca ( pcanumel, pcanumtf );
TArray1< float >    dat ( numel );
TArray1< float >    inv ( pcanumel );

                                        // IF we need eegb later, subtract time average
SubtractAverage ( eegtempl, 1, numel, numtf );


for ( int tf = 0; tf < numtf; tf++ ) {

    for ( int i  = 0; i  < numel; i++  )
        dat[ i ]    = eegtempl ( i, tf );

    isdoc->MultiplyMatrix ( reg, &dat, inv );

    for ( int i  = 0; i  < pcanumel; i++  )
        eegpca ( i, tf ) = inv[ i ];
    }


TFileName           MyMultFileName;

StringCopy          ( MyMultFileName, EEGDoc->GetDocPath () );
if ( StringIsNotEmpty ( prefix ) )  PrefixFilename ( MyMultFileName, prefix, "." );
RemoveExtension     ( MyMultFileName );
StringAppend        ( MyMultFileName, ".Templatized.PCA" );
AddExtension        ( MyMultFileName, FILEEXT_EEGSEF );
CheckNoOverwrite    ( MyMultFileName );

eegpca.WriteFile ( MyMultFileName );
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( gauge )
    gauge->Next ( gaugepcamain );

                                        // Save Topographies (matrix V)
if ( savetopo ) {

    TFileName           TopoFileName;

    StringCopy          ( TopoFileName, filename );
    if ( StringIsNotEmpty ( prefix ) )  PrefixFilename ( TopoFileName, prefix, "." );
    RemoveExtension     ( TopoFileName );
    StringAppend        ( TopoFileName, PCAInfix, ".Topo" );
    AddExtension        ( TopoFileName, FILEEXT_EEGSEF );
    CheckNoOverwrite    ( TopoFileName );


    eigenvectors.WriteFile ( TopoFileName, false, 0, &xyznames );


    Cartool.CartoolDocManager->OpenDoc ( TopoFileName, dtOpenOptions );
    } // savetopo


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( gauge )
    gauge->Next ( gaugepcamain );

                                        // Save Eigenvalues to file
if ( saveeigenvalues ) {

    TFileName           EigenvaluesFileName;

    StringCopy          ( EigenvaluesFileName, filename );
    if ( StringIsNotEmpty ( prefix ) )  PrefixFilename ( EigenvaluesFileName, prefix, "." );
    RemoveExtension     ( EigenvaluesFileName );
    StringAppend        ( EigenvaluesFileName, PCAInfix, ".Eigenvalues" );
    AddExtension        ( EigenvaluesFileName, FILEEXT_EEGSEF );
    CheckNoOverwrite    ( EigenvaluesFileName );


    eigenvalues.WriteFile ( EigenvaluesFileName, "Eigenval" );

    } // saveeigenvalues


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( gauge )
    gauge->Next ( gaugepcamain );

                                        // Save PCA data as tracks
if ( savepcatracks ) {

    TFileName           PcaDataFileName;

    StringCopy          ( PcaDataFileName, filename );
    if ( StringIsNotEmpty ( prefix ) )  PrefixFilename ( PcaDataFileName, prefix, "." );
    RemoveExtension     ( PcaDataFileName );
    StringAppend        ( PcaDataFileName, PCAInfix );
    AddExtension        ( PcaDataFileName, FILEEXT_EEGSEF );
    CheckNoOverwrite    ( PcaDataFileName );


    pcamaps.WriteFile ( PcaDataFileName, false, pcamaps.GetSamplingFrequency (), &compnames );


    Cartool.CartoolDocManager->OpenDoc ( PcaDataFileName, dtOpenOptions );

    } // savepcatracks


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Save PCA data as clouds of points
if ( savepcapoints ) {

    TFileName           CloudFileName;
    TPoints             points;

                                        // Save all dimensions 3 by 3
    for ( int i = 0; i < AtMost ( 12, pcanumcomp - 2 ); i += 3 ) {

        points.Reset ();

        for ( int tf = 0; tf < pcanumtf; tf++ )
            points.Add ( pcamaps ( tf, i ), pcamaps ( tf, i + 1 ), pcamaps ( tf, i + 2 ) );



        StringCopy          ( CloudFileName, filename );
        if ( StringIsNotEmpty ( prefix ) )  PrefixFilename ( CloudFileName, prefix, "." );
        RemoveExtension     ( CloudFileName );
        StringAppend        ( CloudFileName, PCAInfix );
        StringAppend        ( CloudFileName, ".Dim ", IntegerToString ( i + 1 ), " ", IntegerToString ( i + 2 ), " ", IntegerToString ( i + 3 ) );
        AddExtension        ( CloudFileName, FILEEXT_SPIRR );
        CheckNoOverwrite    ( CloudFileName );

        points.WriteFile    ( CloudFileName );

        Cartool.CartoolDocManager->OpenDoc ( CloudFileName, dtOpenOptions );
        }

    } // savepcapoints

    
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Subtract average of each column or row
                                        // axis is the axis to be averaged
/*void        SubtractAverage (   TArray2<float>     &data,           int                 axis,
                                int                 maxdim,         int                 maxsamples )
{
if ( data.IsNotAllocated () )
    return;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // pick the variable and the samples axis
int                 numdim          = axis == 1 ? data.GetDim1 () : data.GetDim2 ();
int                 numsamples      = axis == 1 ? data.GetDim2 () : data.GetDim1 ();

                                        // caller can optionally restrict some of the dimensions
if ( maxdim     > 0 )   Clipped ( numdim,     1, maxdim     );
if ( maxsamples > 0 )   Clipped ( numsamples, 1, maxsamples );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // center each variable through all samples
double              avg;


if ( axis == 1 )
    for ( int i  = 0; i  < numdim; i++  ) {

        avg     = 0;

        for ( int tf = 0; tf < numsamples; tf++ )
            avg    += data ( i, tf );

        avg    /= numsamples;


        for ( int tf = 0; tf < numsamples; tf++ )
            data ( i, tf ) -= avg;
        }
else
    for ( int i  = 0; i  < numdim; i++  ) {

        avg     = 0;

        for ( int tf = 0; tf < numsamples; tf++ )
            avg    += data ( tf, i );

        avg    /= numsamples;


        for ( int tf = 0; tf < numsamples; tf++ )
            data ( tf, i ) -= avg;
        }

}
*/

//----------------------------------------------------------------------------
                                        // Do a PCA on the already loaded buffer
                                        // eigenvectors and eigenvalues can be less than the input dimension
bool        PCA (   TMaps&              data,           
                    bool                covrobust,
                    bool                removelasteigen,
                    PcaResultsType      pcaresults,
                    TMaps&              eigenvectors,       TVector<float>&         eigenvalues,
                    AMatrix&            topca,              AMatrix&                towhite,
                    TMaps&              pcadata,        
                    TSuperGauge*        gauge
                )
{
if ( data.IsNotAllocated () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // if variablesindim1, then first dimension holds the variables (like an EEG buffer)
int                 numdim          = data.GetDimension ();
int                 numsamples      = data.GetNumMaps ();

                                        // not enough data?
//if ( dim2 < dim1 )
//    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSelection          skiptracks ( numdim, OrderSorted );


data.GetNullElectrodes ( skiptracks );

                                        // center each dimension through all samples
                                        // ?With average referenced data, it seems we have better results (less components) when skipping the time centering?
data.TimeCentering ();


//skiptracks.Show ( "skipping tracks" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( gauge )
    gauge->Next ();


ASymmetricMatrix    covariance;
double              mahamaxsd       = 3;    // data past this SD will be ignored


if ( covrobust ) {

//  if ( ! RobustCovarianceSingleDim    (   data,
//                                          mahamaxsd,
//                                          covariance  ) )

    if ( ! RobustCovarianceMahalanobis  (   data, 
                                            skiptracks, 
                                            mahamaxsd, 
                                            covariance ) )
        return  false;
    }
else {

    if ( ! Covariance                   (   data,
                                            covariance ) )
        return  false;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( gauge )
    gauge->Next ();


AVector             D;
AMatrix             V;

                                        // ascending signed values order
AEigenvaluesEigenvectorsArma ( covariance, D, V );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Permutate Eigenvalues & Eigenvectors, to be in decreasing order
if ( gauge )
    gauge->Next ();


for ( int i  = 0; i  < numdim;     i++  )
for ( int j  = 0; j  < numdim / 2; j++  )
                                        // permutate the columns
    Permutate ( V ( i, j ), V ( i, numdim - 1 - j ) );


for ( int i  = 0; i  < numdim / 2; i++  )

    Permutate ( D ( i ), D ( numdim - 1 - i ) );


//for ( int eigeni  = 0; eigeni  < numdim; eigeni++  )
//    DBGV2 ( eigeni + 1, D ( eigeni ), "# Eigenvalue" );

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // clipping out low Eigenvalues?

                                        // Find the max meaningful Eigenvalues
int                 maxeigen;
//double              tolerance       = 0;
                                        // tolerance formula, as from Matlab pinv
double              tolerance       = GetMachineEpsilon<AReal> () * numdim * D ( 0 );


for ( maxeigen = 0; maxeigen  < numdim; maxeigen++  )

    if ( D ( maxeigen ) <= tolerance )

        break;

//DBGV3 ( tolerance, numdim, maxeigen, "tolerance, numdim -> maxeigen" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // copy back the (clipped) eigentrucs, sorted from highest to lowest
eigenvectors.   Resize ( maxeigen, numdim );
eigenvalues.    Resize ( maxeigen );


for ( int eigeni  = 0; eigeni  < maxeigen; eigeni++  ) {

    eigenvalues[ eigeni ]    = D ( eigeni );

    for ( int j = 0; j  < numdim;     j++  )
        eigenvectors ( eigeni, j )   = V ( j, eigeni );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // compute the 2 transforms
if ( gauge )
    gauge->Next ();

                                        // invert Eigenvectors leads to PCA space
topca       = V.t ();


ADiagonalMatrix     InvSqrtD ( numdim, numdim );
                                        // compute the inverse sqrt of D
for ( int i  = 0; i  < numdim; i++  )

    InvSqrtD ( i, i )   = D ( i ) > tolerance ? 1 / sqrt ( D ( i ) ) : 0;

                                    // apply - doesn't change the dimension, i.e. can have less rows than columns
towhite     = InvSqrtD * topca;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( gauge )
    gauge->Next ();

                                        // set data transform T according to requested output
AMatrix             T;

                                        // results can have less rows than columns
if      ( pcaresults ==            Vt_X )       T   =             topca.head_rows ( maxeigen );
                                        // results can have less rows than columns
else if ( pcaresults ==   InvSqrtD_Vt_X )       T   =     towhite.head_rows ( maxeigen );
                                        // results are back to the original number of rows
else if ( pcaresults == V_InvSqrtD_Vt_X )       T   = V * towhite;


if ( maxeigen < numdim ) {
                                        // !clipping only when all computations have been done!
    topca       = topca  .head_rows ( maxeigen );
    towhite     = towhite.head_rows ( maxeigen );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if ( gauge )
    gauge->Next ();


data.Multiply ( T, pcadata );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
if ( removelasteigen ) {
                                        // more clipping? looking at the actual SD of whitened data
    TMap                sdmap;
                                        // !!!!! whitened !!!!!              
                                        //                                    /
                                        //                                   /
                                        // Results is something  -\/\--\/\--/
                                        // the last part has too high SD in the data
    pcadata.SD ( 0, numsamples - 1, sdmap );


    FctParams           p;

                                        // remove bumps, 0's
    p ( FilterParamDiameter )     = 5;
    sdmap.Filter ( FilterTypeMedian, p );
    sdmap.Filter ( FilterTypeMedian, p );

                                        // one more smoothing
    p ( FilterParamDiameter )     = 3;
    sdmap.Filter ( FilterTypeGaussian, p );


    //sdmap.WriteFile ( "E:\\Data\\PCA White.SD.sef", "WhiteSD" );


                                        // get stats from SD across components
    TEasyStats          stat;

    stat.Set ( sdmap, true );

    //stat.Show ( "SD stats" );

                                        // 3 SD is quite conservative actually
    double              lastthreshold   = stat.Median () 
                                        + 3 * ( stat.Sn ( 5000 ) + stat.Qn ( 5000 ) + stat.InterQuartileRange () + stat.MAD () ) / 4;

    //DBGV ( lastthreshold, "lastthreshold" );

                                        // scan from last to first until we reach the threshold
    int                 lasteigen;

                                        // first try to climb, it can happen
    for ( lasteigen = maxeigen - 1; lasteigen > 0; lasteigen-- )

        if ( sdmap[ lasteigen - 1 ] < sdmap[ lasteigen ] )

            break;

                                        // then proceed to descend
    for ( ; lasteigen >= 0; lasteigen-- )

        if ( sdmap[ lasteigen ] < lastthreshold )

            break;

                                        // make some consistency check
    if ( lasteigen < 0 )
        lasteigen   = maxeigen;
    else
        lasteigen++;                    // make it a count, not an index


    if ( lasteigen != maxeigen ) {
        DBGV ( maxeigen - lasteigen, "Removing last #" );
//        maxeigen    = lasteigen;
        }

    } // if removelasteigen
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  true;
}


//----------------------------------------------------------------------------
                                        // Classical Covariance
                                        // Data already centered
bool        Covariance (    TMaps&              data,
                            ASymmetricMatrix&   Cov 
                        )
{
if ( data.IsNotAllocated () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // if variablesindim1, then first dimension holds the variables (like an EEG buffer)
int                 numdim          = data.GetDimension ();
int                 numsamples      = data.GetNumMaps ();

                                        // not enough data?
//if ( dim2 < dim1 )
//    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Cov.AResizeZero ( numdim, numdim );


//#define     debugcovariancecloud

#ifdef debugcovariancecloud
                                        // not sure we really need to go through all the data...
int                 step            = AtLeast ( 1, Round ( (double) numsamples / 10000 ) );

TFileName           _file;
char                buff1[ 256 ];
char                buff2[ 256 ];
TPoints             cloud;
#endif

                                        // fill covariance matrix
OmpParallelFor

for ( int i  = 0; i  < numdim; i++  )
for ( int j  = 0; j  <= i;     j++  ) {


#ifdef debugcovariancecloud
    StringCopy      ( _file, "E:\\Data\\CovarianceCloud.", IntegerToString ( buff1, i + 1, 3 ), ".", IntegerToString ( buff2, j + 1, 3 ) );
    AddExtension    ( _file, FILEEXT_SPIRR );

    cloud.Reset ();

    for ( int tf = 0; tf < numsamples; tf += step )
        cloud.Add ( data ( tf, i ), data ( tf, j ), 0 );

    cloud.WriteFile ( _file );
#endif


    for ( int tf = 0; tf < numsamples; tf++ )

        Cov ( i, j )   += data ( tf, i ) * data ( tf, j );
    }

                                        // mirror to upper triangular part
ASymmetrize ( Cov, true );


Cov    /= NonNull ( numsamples - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  true;
}


//----------------------------------------------------------------------------
/*                                        // Added after Cov has been computed:
                                          // Ledoit shrinkage
ASymmetricMatrix    F               = AMatrixIdentity ( numdim );       // !subtract 0.5 to the eigenvalues! - looks the best(?)
//ASymmetricMatrix    F               = AMatrixOnes ( numdim, numdim );   // no need to subtract

double              delta           = 0.5;

Cov     = ( 1 - delta ) * Cov + delta * F;

                                        // ...then later, after eigenvalues extraction...

                                        // For Ledoit + 0.5 * Id
if ( shrinkage ) 
    D  -= 0.5 * AMatrixOnes ( numdim, numdim );
*/

//----------------------------------------------------------------------------
                                        // Robust Covariance
                                        // Data already centered
                                        // Any electrode out of a Z-Score value will make the corresponding
                                        // TF null for all the other electrodes.
                                        // maxsd: 2 for regular data, 3 for normalized data
                                        // ?Does it make really sense: ERP data has high values on the biggest components, which would be removed?
bool        RobustCovarianceSingleDim ( TMaps&              data,           
                                        double              maxsd,
                                        ASymmetricMatrix&   Cov 
                                      )
{
if ( data.IsNotAllocated () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // if variablesindim1, then first dimension holds the variables (like an EEG buffer)
int                 numdim          = data.GetDimension ();
int                 numsamples      = data.GetNumMaps ();

                                        // not enough data?
//if ( dim2 < dim1 )
//    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Get some stats on each track
TGoEasyStats        stats ( numdim );
                                        // not sure we really need to go through all the data...
int                 step            = AtLeast ( 1, Round ( (double) numsamples / 10000 ) );


OmpParallelFor

for ( int i  = 0; i  < numdim;     i++        )
for ( int tf = 0; tf < numsamples; tf += step )

    stats[ i ].Add ( data ( tf, i ), ThreadSafetyIgnore );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*                                        // Exploring: computing Z values and good TFs
TMaps               dataz ( data.GetNumMaps (), data.GetDimension () );
TVector<bool>       goodtf ( numsamples );

goodtf  = true;


OmpParallelFor

for ( int i  = 0; i  < numdim;     i++        )
for ( int tf = 0; tf < numsamples; tf += step ) {

    dataz ( tf, i ) = fabs ( stats[ i ].ZScore ( data ( tf, i ) ) );

    if ( dataz ( tf, i ) > maxsd )

        goodtf[ tf ]    = false;
    }


//dataz .WriteFile ( "E:\\Data\\RobustCovariance.DataZ.sef" );
//goodtf.WriteFile ( "E:\\Data\\RobustCovariance.GoodTF.sef", "GoodTF" );
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // exclude TF all the time?
TVector<bool>       goodtf ( numsamples );

goodtf  = true;

                                        // scan ALL TF
for ( int tf = 0; tf < numsamples; tf++ )
                                        // look at each dimension
    for ( int i  = 0; i  < numdim;     i++        )
                                        // if one dimension is bad, then bad for all the others (?)
        if ( fabs ( stats[ i ].ZScore ( data ( tf, i ) ) ) > maxsd ) {

            goodtf[                           tf       ]    = false;
                                        // also spread to neighbors
            goodtf[ AtLeast ( 0,              tf - 1 ) ]    = false;
            goodtf[ NoMore  ( numsamples -1,  tf + 1 ) ]    = false;

            break;
            }

                                        // remove single good TFs
for ( int tf = 0; tf < numsamples; tf++ )

    if ( goodtf[ tf ]
         && ( tf == 0              || ! goodtf[ tf - 1 ] )
         && ( tf == numsamples - 1 || ! goodtf[ tf + 1 ] ) )

        goodtf[ tf ]    = false;


//goodtf.WriteFile ( "E:\\Data\\Covariance.GoodTF.sef", "GoodTF" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


Cov.AResizeZero ( numdim, numdim );


//#define     debugcovariancecloud

#ifdef debugcovariancecloud
TFileName           _file;
char                buff1[ 256 ];
char                buff2[ 256 ];
TPoints             cloud;
#endif

                                        // fill covariance matrix
OmpParallelFor

for ( int i  = 0; i  < numdim; i++  )
for ( int j  = 0; j  <= i;     j++  ) {


#ifdef debugcovariancecloud
    StringCopy      ( _file, "E:\\Data\\CovarianceCloud.", IntegerToString ( buff1, i + 1, 3 ), ".", IntegerToString ( buff2, j + 1, 3 ) );
    AddExtension    ( _file, FILEEXT_SPIRR );

    cloud.Reset ();

    for ( int tf = 0; tf < numsamples; tf += step )

        if ( goodtf[ tf ] )

            cloud.Add ( data ( tf, i ), data ( tf, j ), 0 );

    cloud.WriteFile ( _file );
#endif


    for ( int tf = 0; tf < numsamples; tf++ )

        if ( goodtf[ tf ] )

            Cov ( i, j )   += data ( tf, i ) * data ( tf, j );
    }

                                        // mirror to upper triangular part
ASymmetrize ( Cov, true );

int             numgoodsamples      = goodtf.GetSumValues ();

Cov    /= NonNull ( numgoodsamples - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  true;
}


//----------------------------------------------------------------------------
                                        // Robust Covariance
                                        // Data already centered
                                        // Using Mahalanobis distance between pairs of electrodes
                                        // ?Using different number of TF per pair of tracks might not be a good idea for covariance?
                                        // ?Same question as for RobustCovarianceSingleDim: strong Maha is for ERP components, so ditching interesting data?
bool        RobustCovarianceMahalanobis (   TMaps&              data,
                                            TSelection&         skiptracks,
                                            double              maxsd,
                                            ASymmetricMatrix&   Cov 
                                        )
{
if ( data.IsNotAllocated () )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 numdim          = data.GetDimension ();
int                 numsamples      = data.GetNumMaps ();

                                        // not enough data?
//if ( dim2 < dim1 )
//    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ASymmetricMatrix    covall ( numdim, numdim );

covall.AReset ();


//#define     debugcovariancecloud

#ifdef debugcovariancecloud
int                 step            = AtLeast ( 1, Round ( (double) numsamples / 10000 ) );
int                 modulout        = 33;
int                 maxout          = 50;
int                 numout          = 0;

TFileName           _file;
char                buff1[ 256 ];
char                buff2[ 256 ];
TPoints             cloud;
#endif

                                        // fill covariance matrix
OmpParallelFor

for ( int i  = 0; i  < numdim; i++  ) {

    if ( skiptracks[ i ] )
        continue;

    for ( int j  = 0; j  <= i;     j++  ) {

        if ( skiptracks[ j ] )
            continue;

    #ifdef debugcovariancecloud
        if ( ( i + j + 1 ) % modulout == 0 && numout++ < maxout ) {

            StringCopy      ( _file, "E:\\Data\\CovarianceCloud.", IntegerToString ( buff1, i + 1, 3 ), ".", IntegerToString ( buff2, j + 1, 3 ) );
            AddExtension    ( _file, FILEEXT_SPIRR );

            cloud.Reset ();

            for ( int tf = 0; tf < numsamples; tf += step )

                cloud.Add ( data ( tf, i ), data ( tf, j ), 0 );

            cloud.WriteFile ( _file );
            }
    #endif

                                            // first pass is to blindly use all data
        for ( int tf = 0; tf < numsamples; tf++ )

            covall ( i, j )    += data ( tf, i ) * data ( tf, j );
        }
    }

                                            // mirror to upper triangular part - not really needed here, but better be safe
ASymmetrize ( covall, true );


covall    /= NonNull ( numsamples - 1 );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Second pass is to use the Mahalanobis distance for each
                                        // pair of electrodes (so we deal with 2 dimensions, but still)
                                        // This way, we reject the points outside the ellipsoid, for
                                        // each pair and each TF
TArray2<double>     covcount ( numdim, numdim );
//TMaps               datam    ( numsamples, numdim * numdim );


Cov.AResizeZero ( numdim, numdim );

covcount    = 0;

#ifdef debugcovariancecloud
numout          = 0;
#endif


                                        // First do WITHOUT the diagonal
OmpParallelFor

for ( int i  = 0; i  < numdim; i++  ) {

    if ( skiptracks[ i ] )
         continue;

    TTracks<double>     S ( 2, 2 );

                                        // NO diagonal
    for ( int j  = 0; j  <  i;     j++  ) {

                                        // avoid unnecessary work for the diagonal, the covariance is identity
        if ( skiptracks[ j ]  )
             continue;

        S ( 0, 0 )  = covall ( i, i );
        S ( 1, 1 )  = covall ( j, j );
        S ( 0, 1 )  =
        S ( 1, 0 )  = covall ( i, j );


        double      det     = S ( 0, 0 ) * S ( 1, 1 ) - S ( 0, 1 ) * S ( 1, 0 );

        if ( det == 0 ) {           // this shouldn't happen...
            S ( 0, 0 )  = 1;
            S ( 1, 1 )  = 1;
            S ( 0, 1 )  = 0;
            S ( 1, 0 )  = 0;
            }
        else {
                                    // invert matrix - special symmetrical 2x2 case
            Permutate ( S ( 0, 0 ), S ( 1, 1 ) );

            S ( 0, 1 )  = -S ( 0, 1 );
            S ( 1, 0 )  = -S ( 1, 0 );
            S          /= det;
            }


#ifdef debugcovariancecloud
        if ( ( i + j + 1 ) % modulout == 0 && numout++ < maxout ) {

            StringCopy      ( _file, "E:\\Data\\CovarianceCloud.Maha.", IntegerToString ( buff1, i + 1, 3 ), ".", IntegerToString ( buff2, j + 1, 3 ) );
            AddExtension    ( _file, FILEEXT_SPIRR );

            cloud.Reset ();

            for ( int tf = 0; tf < numsamples; tf += step ) {

                dmaha   = sqrt (       S ( 0, 0 ) * data ( tf, i ) * data ( tf, i )
                                 +     S ( 1, 1 ) * data ( tf, j ) * data ( tf, j )
                                 + 2 * S ( 0, 1 ) * data ( tf, i ) * data ( tf, j ) );

                if ( dmaha <= maxsd )

                    cloud.Add ( data ( tf, i ), data ( tf, j ), 0 );
                }

            cloud.WriteFile ( _file );
            }
#endif


        for ( int tf = 0; tf < numsamples; tf++ ) {
                                        // taking the symmetry of S into account
                                        // case i==j is simply data divided by sd
            double  dmaha   = sqrt (       S ( 0, 0 ) * data ( tf, i ) * data ( tf, i )
                                     +     S ( 1, 1 ) * data ( tf, j ) * data ( tf, j )
                                     + 2 * S ( 0, 1 ) * data ( tf, i ) * data ( tf, j ) );

//            datam ( tf, i * numdim + j )  = datam ( tf, j * numdim + i )  = dmaha;

            if ( dmaha <= maxsd ) {

                Cov     ( i, j )   += data ( tf, i ) * data ( tf, j );
                covcount( i, j )   ++;
                covcount( j, i )   ++;
                } // dmaha OK
            } // for tf
        } // for j
    } // for i


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Second, ONLY the diagonal, with additional processing
                                        // Basically, we want to have about the same number of samples
                                        // in each diagonal bin as there are in the corresponding row/column.
                                        // Having more or less samples produces weird results.
                                        // Also, we want to adjust the Mahalanobis threshold for the case
                                        // of a single factor, instead of 4 for the covariance case.

                                        // approximately  sqrt ( Xi4 (1) / Xi1 (1) )
double              xi1to4          = sqrt ( 0.24 / 0.15 );

TGoEasyStats        stat ( numdim, numdim );


OmpParallelFor

for ( int i  = 0; i  < numdim; i++  ) {

    if ( skiptracks[ i ] )
         continue;


    for ( int j  = 0; j  < numdim; j++  )

        if ( i != j 
          && ! skiptracks[ j ] 
          && covcount ( i, j ) > 0 )

            stat[ i ].Add ( covcount ( i, j ), ThreadSafetyIgnore );
                                        // limit the amount of data into the variance
    int         maxcount    = Round ( stat[ i ].Median () );


                                        // diagonal case simplifies with a single dimension (variance)
    double      S00         = covall ( i, i ) ? 1 / covall ( i, i ) : 0;


#ifdef debugcovariancecloud
        if ( ( i + j + 1 ) % modulout == 0 && numout++ < maxout ) {

            StringCopy      ( _file, "E:\\Data\\CovarianceCloud.Maha.", IntegerToString ( buff1, i + 1, 3 ), ".", IntegerToString ( buff2, j + 1, 3 ) );
            AddExtension    ( _file, FILEEXT_SPIRR );

            cloud.Reset ();

            for ( int tf = 0; tf < numsamples; tf += step ) {

                dmaha   = sqrt (       S ( 0, 0 ) * data ( tf, i ) * data ( tf, i )
                                 +     S ( 1, 1 ) * data ( tf, j ) * data ( tf, j )
                                 + 2 * S ( 0, 1 ) * data ( tf, i ) * data ( tf, j ) );

                if ( dmaha <= maxsd )

                    cloud.Add ( data ( tf, i ), data ( tf, j ), 0 );
                }

            cloud.WriteFile ( _file );
            }
#endif


    for ( int tf = 0; tf < numsamples; tf++ ) {
                                    // taking the symmetry of S into account
                                    // case i==j is simply data divided by sd
        double      dmaha   = sqrt ( S00 * data ( tf, i ) * data ( tf, i ) );
                                        // Chi square scaling factor, so that comparing a sum 4 squares can match a single square
        dmaha  *= xi1to4;

//        datam ( tf, i * numdim + i )  = dmaha;

        if ( dmaha <= maxsd ) {

            Cov     ( i, i )   += data ( tf, i ) * data ( tf, i );
            covcount( i, i )   ++;

            if ( covcount ( i, i ) >= maxcount )
                break;
            } // dmaha OK
        } // for tf
    } // for i


	//datam .WriteFile ( "E:\\Data\\MahaCovariance.DataM.sef" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // # of samples vary for each pair i,j
for ( int i  = 0; i  < numdim; i++  )
for ( int j  = 0; j  <= i;     j++  )

    if ( covcount ( i , j ) > 1 )
        Cov ( i, j )   /= covcount ( i , j )  - 1;
    else
        Cov ( i, j )    = i == j ? 1 : 0;

                                            // finally, don't forget to mirror to upper triangular part
ASymmetrize ( Cov, true );


//ofstream        ocovall ( "E:\\Data\\Covariance.Regular.ep" );
//ocovall << covall;
//ofstream        ocov ( "E:\\Data\\Covariance.Mahalanobis.ep" );
//ocov << Cov;
//covcount.WriteFile ( "E:\\Data\\Covariance.Count Final.sef" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
