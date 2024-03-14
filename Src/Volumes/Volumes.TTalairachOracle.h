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

#pragma once

#include    "Geometry.TPoint.h"
#include    "TVolume.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr char*     TalairachOracleFileName = "Talairach.dlf";

constexpr int       TalairachOracleDim1     = 171;
constexpr int       TalairachOracleDim2     = 172;
constexpr int       TalairachOracleDim3     = 110;

constexpr int       TalairachOracleOriginX  =  85;
constexpr int       TalairachOracleOriginY  = 102;
constexpr int       TalairachOracleOriginZ  =  42;


                                        // Talairach is splitted into 5 groups
constexpr int       NumTalairachGroups      = 5;

extern const char   TalairachGroups [ NumTalairachGroups ][ 32 ];


                                        // Ranges of indexes of the 5 Talairach groups
constexpr UINT      TalairachStructMin      =   1;
constexpr UINT      TalairachStructMax      =   7;
constexpr UINT      TalairachLobesMin       =   8;
constexpr UINT      TalairachLobesMax       =  19;
constexpr UINT      TalairachGyriMin        =  20;
constexpr UINT      TalairachGyriMax        =  78;
constexpr UINT      TalairachMatterMin      =  79;
constexpr UINT      TalairachMatterMax      =  80;
constexpr UINT      TalairachBrodmannMin    =  81;
constexpr UINT      TalairachBrodmannMax    = 151;
constexpr UINT      NumTalairachCodes       = 152;

inline UINT         TalairachEncodeLabel1 ( UINT L )    { return IsInsideLimits ( L, TalairachStructMin, TalairachBrodmannMax ) ? L               : 0; }
inline UINT         TalairachEncodeLabel4 ( UINT L )    { return IsInsideLimits ( L, TalairachStructMin, TalairachBrodmannMax ) ? ( L - 78) <<  4 : 0; }
inline UINT         TalairachEncodeLabel2 ( UINT L )    { return IsInsideLimits ( L, TalairachStructMin, TalairachBrodmannMax ) ? L         <<  8 : 0; }
inline UINT         TalairachEncodeLabel3 ( UINT L )    { return IsInsideLimits ( L, TalairachStructMin, TalairachBrodmannMax ) ? L         << 16 : 0; }
inline UINT         TalairachEncodeLabel5 ( UINT L )    { return IsInsideLimits ( L, TalairachStructMin, TalairachBrodmannMax ) ? L         << 24 : 0; }

inline UINT         TalairachDecodeLabel1 ( UINT L )    { return (     L         & 0x0F ); }
inline UINT         TalairachDecodeLabel4 ( UINT L )    { return ( ( ( L >>  4 ) & 0x0F ) ? ( ( L >> 4  ) & 0x0F ) + 78 : 0 ); }
inline UINT         TalairachDecodeLabel2 ( UINT L )    { return (   ( L >>  8 ) & 0xFF ); }
inline UINT         TalairachDecodeLabel3 ( UINT L )    { return (   ( L >> 16 ) & 0xFF ); }
inline UINT         TalairachDecodeLabel5 ( UINT L )    { return (   ( L >> 24 ) & 0xFF ); }


extern const char   TalairachLabels [ NumTalairachCodes ][ 36 ];


//----------------------------------------------------------------------------

class   TTalairachOracle
{
public:
                        TTalairachOracle ( const char* filepath = 0 );


    bool                IsAllocated    ()   const           { return TalairachVolume.IsAllocated    (); }
    bool                IsNotAllocated ()   const           { return TalairachVolume.IsNotAllocated (); }


    static void         PointsToVolume  ( const char* path );
    bool                Read            ( const char* filepath = 0 );


    const char*         CodeToName      ( UINT  talcode )   const;
    UINT                NameToCode      ( char* talname )   const;

    UINT                PositionToCode  ( const TPointFloat& pos )                                  const;
    bool                PositionToCodes ( const TPointFloat& pos, int codes[ NumTalairachGroups ] ) const;  // returns codes in 5 groups
    bool                PositionHasCode ( const TPointFloat& pos, UINT talcode )                    const;
    const char*         PositionToString( const TPointFloat& pos, char* name, bool groupresult )    const;


    TVolume<UINT>       TalairachVolume;

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
