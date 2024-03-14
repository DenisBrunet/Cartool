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

#include    "MergingMriMasks.h"

#include    "Strings.Utils.h"
#include    "Files.TVerboseFile.h"
#include    "Dialogs.TSuperGauge.h"
#include    "GlobalOptimize.Tracks.h"

#include    "TVolumeDoc.h"

#include    "ESI.TissuesConductivities.h"
#include    "ESI.TissuesThicknesses.h"

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

using namespace std;
using namespace owl;

namespace crtl {

OptimizeOff

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void    MergingMriMasks (
                        const char*     filehead,
                        const char*     fileskull,
                        const char*     fileskullsp,
                        const char*     filecsf,
                        const char*     filegrey,
                        const char*     filewhite,
                        const char*     fileblood,
                        const char*     fileair,
                        const char*     fileeyes,
                        bool            createspongy,       double          compactthickness
                        )
{
                                        // Mandatory volumes
if ( StringIsEmpty ( filehead  )
  || StringIsEmpty ( fileskull )
  || StringIsEmpty ( filecsf   )
  || StringIsEmpty ( filegrey  )
  || StringIsEmpty ( filewhite ) )

    return;


if ( StringIsEmpty ( fileskullsp ) && createspongy && compactthickness <= 0 ) {
    createspongy        = false;
    compactthickness    = 0;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TSuperGauge         Gauge ( MergingMriMasksTitle, 23, SuperGaugeLevelBatch );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TFileName           FileMerged;
TFileName           VerboseFile;


StringCopy          ( FileMerged, filehead      );
PostfixFilename     ( FileMerged, "." "Merged"  );
ReplaceExtension    ( FileMerged, FILEEXT_MRINII );

CheckNoOverwrite    ( FileMerged );

StringCopy          ( VerboseFile,              FileMerged );
ReplaceExtension    ( VerboseFile,              FILEEXT_VRB );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TVerboseFile    verbose ( VerboseFile, VerboseFileDefaultWidth );

verbose.PutTitle ( "MRI Tissues Merging" );


verbose.NextTopic ( "Input Files:" );
{
if ( StringIsNotEmpty ( filehead    ) )     verbose.Put ( "Full head mask:",               filehead    );
if ( StringIsNotEmpty ( fileskull   ) )     verbose.Put ( "Skull (compact+spongy) mask:",  fileskull   );
if ( StringIsNotEmpty ( fileskullsp ) )     verbose.Put ( "Spongy skull sub-mask:",        fileskullsp );
if ( StringIsNotEmpty ( filecsf     ) )     verbose.Put ( "CSF probabilities:",            filecsf     );
if ( StringIsNotEmpty ( filegrey    ) )     verbose.Put ( "Grey Matter probabilities:",    filegrey    );
if ( StringIsNotEmpty ( filewhite   ) )     verbose.Put ( "White Matter probabilities:",   filewhite   );
if ( StringIsNotEmpty ( fileblood   ) )     verbose.Put ( "Venous sinus mask:",            fileblood   );
if ( StringIsNotEmpty ( fileair     ) )     verbose.Put ( "Air sinus mask:",               fileair     );
if ( StringIsNotEmpty ( fileeyes    ) )     verbose.Put ( "Eyes mask:",                    fileeyes    );
}


verbose.NextTopic ( "Parameters:" );
{
verbose.Put ( "Automatic spongy skull:", createspongy );
if ( createspongy ) {
    verbose.Put ( "Keeping a compact table thickness of:", compactthickness, 2, " [mm]" );
//  verbose.Put ( "Ratio of spongy to compact bone:", SkullSpongyPercentage * 100, 2, " %" );
//  verbose.Put ( "Min compact table thickness:", SkullCompactMinThickness, 2, " [mm]" );
//  verbose.Put ( "Max compact table thickness:", SkullCompactMaxThickness, 2, " [mm]" );
    }
}


verbose.NextTopic ( "Output Files:" );
{
verbose.Put ( "Verbose file (this):", VerboseFile );

verbose.NextLine ();
verbose.Put ( "Merged volume:", FileMerged );

for ( int  ti = 0; ti < NumTissuesIndex; ti++ )
    verbose.Put ( ti ? "" : "Tissue labels:", TissuesSpecs[ ti ].Code, NumIntegerDigits ( NumTissuesIndex - 1 ), "\t", TissuesSpecs[ ti ].Text );
}


verbose.Flush ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Opengin all MRIs at once
TOpenDoc<TVolumeDoc>    MriHead;
TOpenDoc<TVolumeDoc>    MriSkull;
TOpenDoc<TVolumeDoc>    MriSkullSpongy;
TOpenDoc<TVolumeDoc>    MriCsf;
TOpenDoc<TVolumeDoc>    MriGrey;
TOpenDoc<TVolumeDoc>    MriWhite;
TOpenDoc<TVolumeDoc>    MriBlood;
TOpenDoc<TVolumeDoc>    MriAir;
TOpenDoc<TVolumeDoc>    MriEyes;

                                        // Mandatory volumes
Gauge.Next ();                                              MriHead         .Open ( filehead,    OpenDocHidden );
Gauge.Next ();                                              MriSkull        .Open ( fileskull,   OpenDocHidden );
Gauge.Next ();                                              MriCsf          .Open ( filecsf,     OpenDocHidden );
Gauge.Next ();                                              MriGrey         .Open ( filegrey,    OpenDocHidden );
Gauge.Next ();                                              MriWhite        .Open ( filewhite,   OpenDocHidden );

                                        // Optional volumes
Gauge.Next ();  if ( StringIsNotEmpty ( fileskullsp  ) )    MriSkullSpongy  .Open ( fileskullsp, OpenDocHidden );
Gauge.Next ();  if ( StringIsNotEmpty ( fileblood    ) )    MriBlood        .Open ( fileblood,   OpenDocHidden );
Gauge.Next ();  if ( StringIsNotEmpty ( fileair      ) )    MriAir          .Open ( fileair,     OpenDocHidden );
Gauge.Next ();  if ( StringIsNotEmpty ( fileeyes     ) )    MriEyes         .Open ( fileeyes,    OpenDocHidden );

// Could do some coherence testing here...

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Volume&             head                =                               *MriHead        ->GetData ();
Volume&             skull               =                               *MriSkull       ->GetData ();
Volume*             skullspongy         = MriSkullSpongy    .IsOpen () ? MriSkullSpongy ->GetData () : 0;
Volume&             csf                 =                               *MriCsf         ->GetData ();
Volume&             grey                =                               *MriGrey        ->GetData ();
Volume&             white               =                               *MriWhite       ->GetData ();
Volume*             eyes                = MriEyes           .IsOpen () ? MriEyes        ->GetData () : 0;
Volume*             blood               = MriBlood          .IsOpen () ? MriBlood       ->GetData () : 0;
Volume*             air                 = MriAir            .IsOpen () ? MriAir         ->GetData () : 0;

                                        // If input is mask, take it as probabilities 0/1
FctParams           p;

if ( MriCsf->IsMask () ) {
                                        // Make sure it is now only 0 or 1
    p ( FilterParamBinarized )     = 1;
    csf.Filter ( FilterTypeBinarize, p );
    }

if ( MriGrey->IsMask () ) {
                                        // Make sure it is now only 0 or 1
    p ( FilterParamBinarized )     = 1;
    grey.Filter ( FilterTypeBinarize, p );
    }

if ( MriWhite->IsMask () ) {
                                        // Make sure it is now only 0 or 1
    p ( FilterParamBinarized )     = 1;
    white.Filter ( FilterTypeBinarize, p );
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int                 dim                 = head.GetLinearDim ();
int                 dim1                = head.GetDim1 ();
int                 dim2                = head.GetDim2 ();
int                 dim3                = head.GetDim3 ();
Volume              vol ( dim1, dim2, dim3 );
TPointDouble        origin              = MriHead->GetOrigin ();
TVector3Double      voxelsize           = MriHead->GetVoxelSize ();
TVector3Double      realsize            = MriHead->GetRealSize ();
//double              background          = MriHead->GetBackgroundValue ();
int                 niftitransform      = AtLeast ( NiftiTransformDefault, MriHead->GetNiftiTransform () );
double              mingrey             = 0.25;     
double              minwhite            = mingrey;          // Low enough & equal probabilities for balanced competition
double              mincsf              = 0.08; // 0.05;    // !Favoring the CSF a bit to inflate its layer around the brain!


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// We could also make sure that head mask contains all the other masks...

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Build a nice internal mask, which opposite would ne very likely the scalp
Gauge.Next ();

                                        // putting all we know that looks internal to the brain & skull
Volume              insideskull         = csf + grey + white + skull;

if ( skullspongy    )   insideskull    += *skullspongy;
if ( blood          )   insideskull    += *blood;
if ( air            )   insideskull    += *air;

                                        // clipping to a big mask
Gauge.Next ();

p ( FilterParamThresholdMin )     = SingleFloatEpsilon;
p ( FilterParamThresholdMax )     = FLT_MAX;
p ( FilterParamThresholdBin )     = 1;
insideskull.Filter ( FilterTypeThresholdBinarize, p );

                                        // closing
Gauge.Next ();

p ( FilterParamDiameter )     = 4;
insideskull.Filter ( FilterTypeClose, p );


//TFileName           _fileinskull;
//StringCopy          ( _fileinskull, filehead );
//PostfixFilename     ( _fileinskull, ".InsideSkull" );
//ReplaceExtension    ( _fileinskull, FILEEXT_MRINII );
//CheckNoOverwrite    ( _fileinskull );
//insideskull.WriteFile   (   _fileinskull, 
//                            &origin, &voxelsize, &realsize, 
//                            NiftiOrientation, niftitransform, NiftiIntentCodeLabels, NiftiIntentNameLabels
//                        );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Next ();
                                        // If spongy skull was provided, output compact/spongy indexes; otherwise output only generic skull index
int                 skullindex      = skullspongy ? SkullCompactIndex : SkullIndex;


OmpParallelBegin

TArray1<MriType>    tissues ( NumTissuesIndex );

OmpFor

for ( int i = 0; i < dim; i++ ) {
                                        // not part of the whole head?
//  if ( head ( i ) < background )  continue;   // does not work well for inner parts below threshold - we would need to compute the head mask beforehand
    if ( head ( i ) == 0 )          continue;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Many volumes are just considered as masks, and conversely, just only a few of them are used as real probabilities
                                        // If some probabilities were expected but a mask was provbided, the value of the mask will be taken instead
    tissues ( ScalpIndex        )   =                 (bool)    head            ( i ) 
                                                 && ! (bool)    insideskull     ( i );      // !not considered as "scalp" if inside the skull!
    tissues ( SkullIndex        )   =                 (bool)    skull           ( i );    
    tissues ( SkullSpongyIndex  )   = skullspongy   ? (bool)  (*skullspongy)    ( i ) : 0;
    tissues ( CsfIndex          )   = csf   ( i ) > mincsf   ?  csf             ( i ) : 0;
    tissues ( GreyIndex         )   = grey  ( i ) > mingrey  ?  grey            ( i ) : 0;
    tissues ( WhiteIndex        )   = white ( i ) > minwhite ?  white           ( i ) : 0;
    tissues ( EyeIndex          )   = eyes          ? (bool)  (*eyes)           ( i ) : 0;
    tissues ( BloodIndex        )   = blood         ? (bool)  (*blood)          ( i ) : 0;
    tissues ( AirIndex          )   = air           ? (bool)  (*air)            ( i ) : 0;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // First, setting all masks, so all voxels have a default value
                                        // !Sequence matters, first ones will override all the others!
    if      ( tissues ( AirIndex            ) )                     vol ( i ) = AirIndex;           // !overwrites bone!
    else if ( tissues ( BloodIndex          ) )                     vol ( i ) = BloodIndex;         // !overwrites bone!
    else if ( tissues ( SkullSpongyIndex    ) )                     vol ( i ) = SkullSpongyIndex;
    else if ( tissues ( SkullIndex          ) )                     vol ( i ) = skullindex;
    else if ( tissues ( EyeIndex            ) )                     vol ( i ) = EyeIndex;
                                        // default OUTSIDE the skull is Scalp tissue (i.e. skin / fat / muscle)
    else if ( tissues ( ScalpIndex  ) )                             vol ( i ) = ScalpIndex;
                                        // default INSIDE the brain is CSF
    else                                                            vol ( i ) = CsfIndex;


    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Second, override with competing Grey / White / CSF tissues, which can therefor "eat" some of the skull parts
                                        // !Strict comparison allows to skip anything below threshold, or null values!
    if (    insideskull ( i ) 
         && ! (     vol ( i ) == AirIndex           // don't overwrite these labels
                ||  vol ( i ) == EyeIndex
                ||  vol ( i ) == BloodIndex ) ) {

        if      ( tissues ( GreyIndex  ) > tissues ( WhiteIndex )
               && tissues ( GreyIndex  ) > tissues ( CsfIndex   ) )     vol ( i ) = GreyIndex;

        else if ( tissues ( WhiteIndex ) > tissues ( GreyIndex  )
               && tissues ( WhiteIndex ) > tissues ( CsfIndex   ) )     vol ( i ) = WhiteIndex;

        else if ( tissues ( CsfIndex   ) > tissues ( GreyIndex  )
               && tissues ( CsfIndex   ) > tissues ( WhiteIndex ) )     vol ( i ) = CsfIndex;
        }
    }

OmpParallelEnd


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // We are going to build a nice internal mask
Gauge.Next ();

Volume              morecsf ( dim1, dim2, dim3 );

                                        // keeping the internal part, within the skull
OmpParallelFor

for ( int i = 0; i < dim; i++ )

    morecsf ( i )  = IsInsideLimits ( (TissuesIndex) (int) vol ( i ), BrainMinIndex, BrainMaxIndex )
                    || vol ( i ) == CsfIndex 
                    || vol ( i ) == BloodIndex;

                                        // getting rid of dusty voxels
Gauge.Next ();

p ( FilterParamDiameter )     = 2;
morecsf.Filter ( FilterTypeErode, p );


Gauge.Next ();

morecsf.Filter ( FilterTypeKeepBiggestRegion, p );


Gauge.Next ();
                                        // inflating back + adding some more room
p ( FilterParamDiameter )     = 2 + 1;
morecsf.Filter ( FilterTypeDilate, p );
//                                        // still inflating, but a tiny bit less
//p ( FilterParamDiameter )     = 7;
//morecsf.Filter ( FilterTypeMax, p );
//                                        // !Back to original size IF not used for expanding CSF!
//p ( FilterParamDiameter )     = 2;
//morecsf.Filter ( FilterTypeDilate, p );


//TFileName           _filemorecsf;
//StringCopy          ( _filemorecsf, filecsf );
//PostfixFilename     ( _filemorecsf, ".MoreCsf" );
//ReplaceExtension    ( _filemorecsf, FILEEXT_MRINII );
//CheckNoOverwrite    ( _filemorecsf );
//morecsf.WriteFile   (   _filemorecsf, 
//                        &origin, &voxelsize, &realsize, 
//                        NiftiOrientation, niftitransform, NiftiIntentCodeLabels, NiftiIntentNameLabels
//                    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Create the spongy part
Volume              autospongy;


if ( createspongy ) {

    Gauge.Next ();

    autospongy  = vol;

                                        // keeping anything that looks like skull
    p ( FilterParamThresholdMin )     = SkullMinIndex;
    p ( FilterParamThresholdMax )     = SkullMaxIndex;
    p ( FilterParamThresholdBin )     = 1;
    autospongy.Filter ( FilterTypeThresholdBinarize, p );

                                        // adding air back to skull
    if ( air )  autospongy |= *air;

                                        // clip again to original skull, to keep only the frontal air sinus
    autospongy  &= skull;

                                        // Erode it to something reasonably close to spongy
    Gauge.Next ();
                                        // let 2 mm of compact bone on each side
    p ( FilterParamDiameter )     = 2 * compactthickness + 1;
    autospongy.Filter ( FilterTypeMin, p );

                                        // removing the air sinuses we added before
    if ( air )  autospongy.BinaryOp ( *air, OperandMask, OperandMask, OperationSub );


//    TFileName           _filespongy;
//    StringCopy          ( _filespongy, fileskull );
//    PostfixFilename     ( _filespongy, ".Spongy" );
//    ReplaceExtension    ( _filespongy, FILEEXT_MRINII );
//    CheckNoOverwrite    ( _filespongy );
//    autospongy.WriteFile    (   _filespongy, 
//                            &origin, &voxelsize, &realsize, 
//                            NiftiOrientation, niftitransform, NiftiIntentCodeLabels, NiftiIntentNameLabels
//                        );
    } // createspongy
else {
    Gauge.Next ();
    Gauge.Next ();
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Refining mask
Gauge.Next ();


OmpParallelFor

for ( int i = 0; i < dim; i++ ) {

    if ( createspongy ) {
                                        
        if      ( autospongy ( i )              )   vol ( i )   = SkullSpongyIndex;     // force replace with our auto spongy
        else if ( vol ( i ) == SkullIndex
               || vol ( i ) == SkullSpongyIndex )   vol ( i )   = SkullCompactIndex;    // otherwise, forcibly converts the other skull indexes into compact skull
        }

                                        // cleaning "external" CSF / Grey / White
    if ( ! morecsf ( i ) || ! insideskull ( i ) ) {

        if (    vol ( i ) == CsfIndex
             || vol ( i ) == GreyIndex
             || vol ( i ) == WhiteIndex       )

            vol ( i ) = ScalpIndex;

        }

                                        // Currently not needed, as we can favor the CSF hence making it thicker enough, while still using the real probabilities
//                                      // expanding a bit CSF (will override code above) - !Needs blood volume open!
//  if ( morecsf ( i ) ) {
//                                      // restore blood that might have been overwritten by skull
//      if      ( blood && (bool) (*blood) ( i ) )
//
//          vol ( i ) = BloodIndex;
//                                      // "grignote" internal part of the skull, pushing the CSF boudaries a little bit
//      else if (    vol ( i ) == SkullIndex
//                || vol ( i ) == SkullCompactIndex
//                || vol ( i ) == SkullSpongyIndex
//                || vol ( i ) == ScalpIndex       )
//
//          vol ( i ) = CsfIndex;
//      }
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Next ();

p ( FilterParamDiameter )     = 1;
morecsf.Filter ( FilterTypeErode, p );


TArray1<MriType>    tissues ( NumTissuesIndex );
tissues.ResetMemory ();
                                        // counting inside brain tissues
for ( int i = 0; i < dim; i++ )
    if ( morecsf ( i ) ) 
        tissues ( (int) vol ( i ) )++;


verbose.NextTopic ( "Statistics:" );
{
double              numbrain        = morecsf.GetNumSet ();

verbose.Put ( "Percentage of tissues within the skull space:", "" );
for ( int  ti = 0; ti < NumTissuesIndex; ti++ )
    verbose.Put ( TissuesSpecs[ ti ].Text, tissues ( ti ) / numbrain * 100, 2, " %" );
}


//TFileName           _filebrain;
//StringCopy          ( _filebrain, filecsf );
//PostfixFilename     ( _filebrain, ".WholeBrain" );
//ReplaceExtension    ( _filebrain, FILEEXT_MRINII );
//CheckNoOverwrite    ( _filebrain );
//morecsf.WriteFile   (   _filebrain, 
//                        &origin, &voxelsize, &realsize, 
//                        NiftiOrientation, niftitransform, NiftiIntentCodeLabels, NiftiIntentNameLabels
//                    );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // A tiny bit of ironing, for small outliers(?)
//Gauge.Next ();
//
//p ( FilterParamDiameter )     = 2;
//vol.Filter ( FilterTypeMedian, p, true );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Gauge.Next ();

MriHead         .Close ();
MriSkull        .Close ();
MriSkullSpongy  .Close ();
MriCsf          .Close ();
MriGrey         .Close ();
MriWhite        .Close ();
MriEyes         .Close ();
MriBlood        .Close ();
MriAir          .Close ();

insideskull .DeallocateMemory ();
morecsf     .DeallocateMemory ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Writing mask volume
Gauge.Next ();


vol.WriteFile   (   FileMerged, 
                    &origin, &voxelsize, &realsize, 
                    NiftiOrientation, niftitransform, NiftiIntentCodeLabels, NiftiIntentNameLabels
                );

FileMerged.Open ();


Gauge.HappyEnd ();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OptimizeOn

}
