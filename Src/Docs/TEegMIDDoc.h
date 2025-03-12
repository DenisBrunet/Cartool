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

#include    "TTracksDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


struct      TDFileHeader
{
    char        sign[15];               /* program signature         */
    char        ftype;                  /* data file type            */
    UCHAR       nchan;                  /* number of(all)channels    */
    UCHAR       naux;                   /* number of aux channels    */
    UINT16      fsamp;                  /* sampling frequency        */
    UINT32      nsamp;                  /* number of samples         */
    UCHAR       d_val;                  /* data validation mark      */
    UCHAR       unit;                   /* æV/bin                    */
    short       zero;                   /* value ÷ 0 uV              */
    UINT16      data_org;               /* data sect offset (para)   */
    UINT16      xhdr_org;               /* extheader offset (para)   */
};                                      /*               Total:  32  */


struct      TDFileExtHeaderRecord
{
    UINT16      mnemo;                  /* record name         */
    UINT16      size;                   /* record length       */
};


struct      TDFileExtHeaderCI
{
    short           ncal;
    short           ampl;
//  float*          zero;
//  float*          range;
    TArray1<float>  zero;
    TArray1<float>  range;
} ;


struct      TDFileExtHeaderTT
{
    UINT16      def_len;
    UINT16      list_len;
    ULONG       def_off;
    ULONG       list_off;
} ;


struct      TDFileTagDef
{
    char        abrv[2];
    UINT16      count;
    UINT16      txtlen;
    UINT16      txtoff;
} ;


EndBytePacking

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // M&I (Czech Republic) / BrainScope (outside) company
class   TEegMIDDoc  :   public  TTracksDoc
{
public:
                    TEegMIDDoc      ( owl::TDocument *parent = 0 );


    bool            CanClose        ()                                  final;
    bool            Close           ()                                  final;
    bool            IsOpen          ()                                  final       { return InputStream != 0; }
    bool            Open            ( int mode, const char *path = 0 )  final;


    static bool     ReadFromHeader  ( const char* file, ReadFromHeaderType what, void* answer );
    void            ReadRawTracks   ( long tf1, long tf2, TArray2<float> &buff, int tfoffset = 0 )  final;

    bool            IsDataCalibrated()  const           { return DataCalibrated; }
    bool            IsXHCalibration ()  const           { return XHCalibration; }
    void            UseCalibration  ( TDFileExtHeaderCI *ci = 0 ); // used in ReadRawTracks only
    const char*     GetDataInfo     ()  const           { return DataInfo; }


protected:

    owl::TInStream*     InputStream;
    int                 BuffTFSize;

    bool                DataCalibrated;
    bool                Polarity;
    bool                BlockStructure;
    bool                PackedDatas;
    double              Zero;
    double              Unit;
    long                ExtHeaderOrg;
    bool                XHCalibration;

    TDFileExtHeaderCI   XHCI;
    TDFileExtHeaderCI*  forceCI;
    TArray1<char>       DataInfo;

    TArray1<float>      Tracks;
    TArray1<short>      FileBuff;


    bool            SetArrays           ()  final;
    void            ReadNativeMarkers   ()  final;

    void            FileDataToMicroVolts ( const TDFileExtHeaderCI* ci = 0 );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
