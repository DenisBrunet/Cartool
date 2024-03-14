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

#if defined(CHECKASSERT)
#include    <assert.h>
#endif

#include    "Electrodes.ExtractElectrodesFromKrios.h"

#include    "Geometry.TPoints.h"
#include    "Strings.TStrings.h"
#include    "Files.SpreadSheet.h"

#include    "Electrodes.Utils.h"

#include    "TCartoolMdiClient.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

namespace crtl {

OptimizeOff

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

bool        ExtractElectrodesFromKrios  (   const char*     filein,
                                            const char*     namestoskip,
                                            const char*     xyzreffile,
                                            char*           fileout 
                                        )
{
if ( ! CanOpenFile ( filein ) )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSpreadSheet        krioscsv;


if ( ! krioscsv.ReadFile ( filein ) )
    return  false;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Read file into first list
TPoints             electrodes;
TPoints             fiducials;
TPoints             others;
TStrings            electrodesnames;
TStrings            fiducialsnames;
TStrings            othersnames;
TSplitStrings       splitnamestoskip ( namestoskip, UniqueStrings, SplitStringsSeparators );

char                scannertype[ 256 ];
char                sensortype [ 256 ];
char                name       [ 256 ];
TPointFloat         e;
double              scaling;

                                        // we wish to have the output set in [mm]
if      ( StringContains ( (const char*) krioscsv.GetAttribute ( KriosX ), "(cm)" ) )   scaling     = 10;
else if ( StringContains ( (const char*) krioscsv.GetAttribute ( KriosX ), "(mm)" ) )   scaling     =  1;
else																					scaling     =  1;
        
                                        // read, rescale, and store
for ( int i = 0; i < krioscsv.GetNumRecords (); i++ ) {

                                        // retrieve current point infos
    krioscsv.GetRecord  ( i, KriosType,          scannertype );
    krioscsv.GetRecord  ( i, KriosSpecialSensor, sensortype );
    krioscsv.GetRecord  ( i, KriosName,          name );
    krioscsv.GetRecord  ( i, KriosX,             e.X );
    krioscsv.GetRecord  ( i, KriosY,             e.Y );
    krioscsv.GetRecord  ( i, KriosZ,             e.Z );

    e  *= scaling;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Enumerate a few rules, which are both non-exhaustive and possible contradictive
    bool            acceptel    = StringIsNot ( scannertype, KriosTypeProbed       )    // could be SCANNED, ESTIMATED, or any other string
                               || StringIs    ( sensortype,  KriosSpecialSensorEeg );   // generally we want the EEG sensors

    bool            acceptfid   = StringIs    ( scannertype, KriosTypeProbed       );   // fiducial

    bool            reject      = splitnamestoskip.Contains ( name )                    // in excluded list
                               || StringIs    ( scannertype, KriosTypeProbed       ) && StringIs ( sensortype,  KriosSpecialSensorEeg );    // weird case - bug from scanner?
//                             || StringIs    ( scannertype, KriosTypeEstimated    );    // what to do with this type?
//                             || StringIs    ( scannertype, KriosTypeScanned      ) && StringIs ( sensortype,  KriosSpecialSensorEeg ) && StringIsEmpty ( name ); // !not compatible with some old files!


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // update missing name here
    if ( StringIsEmpty ( name ) )
        StringCopy  ( name, sensortype /*"MissingName"*/ ); // this will nicely give some name to fiducial - the others, well, just generic "EEG" name I guess


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Keep current point only if both conditions are fulfilled
    if      ( reject ) {
                                        // put all unwanted points there - note that we don't make use of these points for the moment, but we could f.ex. with nasion landmark
        others.Add ( e );

        othersnames.Add ( name );
        }

    else if ( acceptel ) {

        electrodes.Add ( e );

        electrodesnames.Add ( name );
        }

    else if ( acceptfid ) {

        fiducials.Add ( e );

        fiducialsnames.Add ( name );
        }

    } // for all records


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // in case we don't have exactly the same amount of names, reset the whole list(s)
if ( (int) electrodesnames != (int) electrodes )
    electrodesnames.Reset ();

if ( (int) fiducialsnames != (int) fiducials )
    fiducialsnames.Reset ();

if ( (int) othersnames != (int) others )
    othersnames.Reset ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Writing the 3 categories of (raw) points
//StringCopy          ( fileout, filein );
//RemoveExtension     ( fileout );
//StringAppend        ( fileout, ".Scanner.Electrodes" );
//AddExtension        ( fileout, FILEEXT_XYZ );
//CheckNoOverwrite    ( fileout );
//electrodes.WriteFile( fileout, &electrodesnames );
//
//StringReplace       ( fileout, ".Electrodes", ".Fiducials" );
//fiducials.WriteFile  ( fileout, &fiducialsnames );
//
//StringReplace       ( fileout, ".Fiducials", ".Others" );
//others.WriteFile  ( fileout, &othersnames );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Check for near-duplicates electrodes
double              eldist          = electrodes.GetMedianDistance ();
double              localdist       = eldist * 0.25 /*0.40*/;


TPoints             pointsout;
TPoints             onecluster;
TSelection          scanned ( (int) electrodes, OrderSorted );  // tracking electrodes that have been done, or duplicates

                                        // for each input electrode
for ( int i = 0; i < (int) electrodes; i++ ) {

    if ( scanned[ i ] )  continue;

    onecluster.Reset ();

    onecluster.Add ( electrodes[ i ] );

                                        // look at all suspiciously too close neighbors
    for ( int j = i + 1; j < (int) electrodes; j++ ) {
        
        if ( scanned[ j ] )  continue;

        if ( ( electrodes[ i ] - electrodes[ j ] ).Norm () < localdist ) {
                                        // exclude duplicate for next scans
            scanned.Set ( j );
                                        // add duplicate to cluster
            onecluster.Add ( electrodes[ j ] );
            }
        }
                                        // this point is done
    scanned.Set ( i );
                                        // use the centroid of all closed electrodes
    pointsout.Add ( onecluster.GetCenter () );
    }

                                        // final number of points, after merging redundant ones
int                 NumElectrodes   = (int) pointsout;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Writing filtered (raw) points
//StringCopy          ( fileout, filein );
//RemoveExtension     ( fileout );
//StringAppend        ( fileout, ".", IntegerToString ( NumElectrodes ) );
//StringAppend        ( fileout, "." "Raw" );
//AddExtension        ( fileout, FILEEXT_XYZ );
//CheckNoOverwrite    ( fileout );
//pointsout.WriteFile ( fileout, &electrodesnames );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Normalizing these points
if ( ! NormalizeXyz ( pointsout, electrodesnames, fiducials, xyzreffile ) ) {

    ShowMessage (   "Aligning electrodes to reference failed!"          NewLine
                    "Either the retrieved positions were erroneous,"    NewLine
                    "or the number of electrodes did not match with the reference file.",
                    KriosTitle, ShowMessageWarning );

    return  false;
    }

                                        // normalization could kick some electrodes out, per the reference xyz
NumElectrodes   = (int) pointsout;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Complete any missing electrode name
                                        // However, don't provide default names if the whole list was totally empty
if ( electrodesnames.IsNotEmpty () )

    for ( int i = (int) electrodesnames; i < NumElectrodes; i++ )

//      electrodesnames.Add ( i + 1 );
        electrodesnames.Add ( "MissingName" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Cumulating electrodes and fiducials, or separately within an .els format(?)
//pointsout          += fiducials;
//electrodesnames.Add ( fiducialsnames );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Writing to file
StringCopy          ( fileout, filein );
RemoveExtension     ( fileout );
StringAppend        ( fileout, ".", IntegerToString ( NumElectrodes ) );
AddExtension        ( fileout, FILEEXT_XYZ );

CheckNoOverwrite    ( fileout );

pointsout.WriteFile ( fileout, &electrodesnames );

                                        // Writing fiducials to separate file(?)
//if ( ! fiducials.IsEmpty () ) {
//    TFileName           filefiducials       = fileout;
//    PostfixFilename     ( filefiducials, ".Fiducials" );
//    fiducials.WriteFile ( filefiducials, &fiducialsnames );
//    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

return  true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OptimizeOn

}
