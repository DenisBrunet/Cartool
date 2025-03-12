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

#include    "Time.Utils.h"

#include    "TTracksDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr short         EEGBIODOC_VERSIONOK         = 668;
constexpr int           EEGBIODOC_MAXTRACKS         = 256;

constexpr USHORT        EEGBIODOC_FILETYPEEEG       = 0;
constexpr USHORT        EEGBIODOC_FILETYPEMRK       = 1;

constexpr MarkerCode    EEGBIODOC_MARKERCODE_TAG    = 88;
constexpr char*         EEGBIODOC_MARKERTEXT_TAG    = "Tagged";
constexpr MarkerCode    EEGBIODOC_MARKERCODE_PUSHB  = 89;
constexpr char*         EEGBIODOC_MARKERTEXT_PUSHB  = "Button";


//----------------------------------------------------------------------------
                                        // Markers defines

enum {
    MARKER_FILE_ID          = 0,
    MARKER_HEADER,
    MARKER_PAT_INFO_OLD,				// obsolete
    MARKER_AD_PARMS_OLD,				// NetLink obsolete
    MARKER_AMP_PARMS, 				    // 32 channels sized
    MARKER_COLLECT_MONTAGE_I,		    // 32 channels sized, 1992 design
    MARKER_DISPLAY_MONTAGE_S, 		    // 32 channels sized, 1992 design
    MARKER_IMPEDANCE, 				    // 32 channels sized
    MARKER_CALIBRATION,				    // 32 channels sized
    MARKER_SW_ADRES, 					// 32 channels sized
    MARKER_SW_SCALE,
    MARKER_DATA_START,
    MARKER_DATA_END,
    MARKER_TIME_STAMP,				    // obsolete
    MARKER_TAG,
    MARKER_COMMENT,
    MARKER_PHOTIC,
    MARKER_VCR_SYNC,
    MARKER_PUSH_BUTTON,
    MARKER_VCR_PAL_NTSC,
    MARKER_SEGMENT,
    MARKER_PAT_INFO_NEW,				// replaces MARKER_PAT_INFO_OLD
    MARKER_BPOS,
    MARKER_XL_AMP_PARMS, 			    // 128 channels sized
    MARKER_XL_DISPLAY_MONTAGE_S, 	    // 128 channels sized, 1992 design
    MARKER_XL_COLLECT_MONTAGE_I, 	    // 128 channels sized, 1992 design
    MARKER_XL_IMPEDANCE,				// 128 channels sized
    MARKER_XL_CALIBRATION,			    // 128 channels sized
    MARKER_XL_SW_ADRES, 				// 128 channels sized
    MARKER_XL_SW_SCALE,				    // 128 channels sized
    MARKER_FILE_SEQ,					// new July 2, 1999
    MARKER_FILE_SEQ_LINK,				// for Persyst SpikeDetection
    MARKER_AD_PARMS_NEW,			    // for NetLink, V 6.63 and beyond
    MARKER_GROUP_CHANNELS,			    // Montage Editor
    MARKER_DISPLAY_MONTAGE,			    // 32 channel sized, version 6.68 and beyond
    MARKER_XL_DISPLAY_MONTAGE,		    // 128 channel sized, version 6.68 and beyond
    MARKER_COLLECT_MONTAGE,			    // 32 channel sized, version 6.68 and beyond
    MARKER_XL_COLLECT_MONTAGE,		    // 128 channels sized, version 6.68 and beyond
    MARKER_NUM_TYPES,
    MARKER_TERMINATOR       = 0xFFFFFFFF
    };


enum {
    MARKER_ANALYZE_COMMENT,				// user entered comment during data playback
    MARKER_COLLECT_COMMENT,				// user entered comment during data recording
    MARKER_SYSTEM_COMMENT,				// program generated comment
    MARKER_SPIKE_COMMENT,				// not written to marker file, used internal to CeeGraph
    MARKER_SENSITIVITY_COMMENT,			// not written to marker file, used internal to CeeGraph
    MARKER_EVENT_COMMENT, 				// patient button push detected
    MARKER_SEGMENT_COMMENT,				// not written to marker file, used internal to CeeGraph
    MARKER_BPOS_COMMENT,				// user selected body position comment
    MARKER_CPAP_TX_COMMENT,				// user entered CPAP level, stored as comment
    MARKER_COMMENT_MAXTYPE
    };


enum {
    MARKER_HEADBOX_DISCON_SYSCOM,		// headbox disconnect detected
    MARKER_HEADBOX_RECONN_SYSCOM,		// headbox reconnect detected
    MARKER_RESET_START_SYSCOM,			// start user commanded amplifier reset
    MARKER_RESET_STOP_SYSCOM,			// user commanded amplifier reset completed
    MARKER_HV_START_SYSCOM,				// patient started hyper-ventilating
    MARKER_HV_ELAPSED_SYSCOM,			// patient hyper-ventilation time tick
    MARKER_HV_STOP_SYSCOM,				// patient stopped hyper-ventilation
    MARKER_HV_ELAPSED_POST_SYSCOM,		// patient hyper-ventilation recovery time tick
    MARKER_HV_START_POST_SYSCOM,		// patient started hyper-ventilation recovery
    MARKER_CAL_START_SYSCOM,			// start user commanded amplifier calibration
    MARKER_CAL_STOP_SYSCOM,				// end of user commanded amplifier calibration
    MARKER_IMPEDANCE_TEST_SYSCOM  		// user commanded an impedance test
    };


//----------------------------------------------------------------------------
                                        // General marker

                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


struct  TEBMarker {
   unsigned long int	marker_curr_offset;		// Absolute offset of this marker in the file
   unsigned long int	marker_next_offset;		// Absolute offset of the next marker record
   unsigned long int	marker_prev_offset;		// Absolute offset of the previous marker record
   unsigned long int	marker_unused;
   unsigned long int	marker_point_num;		// The associated data packet number
   unsigned long int	marker_time_of_day;	    // Seconds since 00:00:00 01/01/1970 GMT
   unsigned long int	marker_elapsed_time;	// Seconds of data in this file
   unsigned short       marker_data_type;		// One of MARKER_TYPE: what type of data is here?
   unsigned short       marker_data_size;		// Size of the marker data area
   unsigned char        marker_deleted;		    // TRUE if marker record is deleted.
   unsigned char        unused_byte;
   };


//----------------------------------------------------------------------------
                                        // Data associated with a marker

struct  TEBFileID {
   char			        mrk_id_ascii[5];		// Always 'BLSC\0'
   unsigned short	    mrk_id_file_type;		// 0 = EEG file, 1 = MRK file
   unsigned short	    mrk_id_file;			// connects marker and data files
   char 			    mrk_id_filename[13];	// EEG filename, 8.3 format
   };


struct  TEBPatInfo {
   char			        mrk_patient_last_name[26];
   char			        mrk_patient_first_name[21];
   char			        mrk_patient_id_number[16];
   char			        mrk_patient_blsc_id[21];    // assures integrity between DBF, MRK, EEG files
   unsigned short	    mrk_patient_change_count;   // ensures consistency between DBF,MRK,EEG
   };


struct  TEBHeader {
   unsigned short	    mrk_header_application;	// Creating application
   unsigned short	    mrk_header_version;		// Program version # x100
   unsigned short	    mrk_header_model;		// Program model number
   unsigned long int	mrk_header_hardware_id;	// Hardware identification
   unsigned short	    mrk_header_ad_type;		// Collection hardware type
   unsigned short	    mrk_header_amp_type;	// Amplifier hardware type
   unsigned short	    mrk_header_stim_type;	// Stimulator hardware type
   unsigned short	    mrk_header_file_version;// File header design version
   unsigned short	    mrk_header_unused;
   };


struct  TEBCMontMap {
   unsigned short       cmont_out; 			    // Electrode # or channel # depending on use
   char   			    cmont_label[7];			// BLSC standard name for channel
   char			        cmont_user_label[7];	// user assigned name for channel
   unsigned short	    cmont_ref;			    // channel reference identification code, defined in cmont.h
   char			        ref_label[7];			// BLSC standard name for channel reference
   char			        user_ref_label[7];		// future expansion, user assigned name for reference
   };


struct  TEBCollectMontage {
   short			    mrk_cmont_num_chan;		// number of collected channels
   TEBCMontMap          mrk_cmont[EEGBIODOC_MAXTRACKS]; // describes individual channel
   };


struct  TEBADParms {
   unsigned char		mrk_ad_signal_source;	// 0=all internal, 0xFF=all external
   short			    mrk_ad_resolution;		// bits of A/D resolution
   short			    mrk_ad_start_channel;	// 1..AD_MAX_CHANNEL
   short			    mrk_ad_end_channel;		// 1..AD_MAX_CHANNEL
   short			    mrk_ad_artifact_level;	// percent of full scale
   short			    mrk_ad_packet_points;	// number of points returned at a time
   short			    mrk_ad_pre_post_points;	// number of points pre or post stimulus
   short			    mrk_ad_blocked_points;	// number of points to ignore artifacts
   unsigned long int	mrk_ad_artifact_enable;	// 1 bit set on channels detecting artifacts
   short			    mrk_ad_packet_rate;		// (Hz) packets / 10 sec
   long int			    mrk_ad_point_rate;		// (Hz) points per second
   float			    mrk_ad_point_float_rate;// (Hz) points per second (256.03)
   unsigned long int	mrk_ad_sync_sec;		// Number of seconds which mrk_ad_point_float_rate
											    // calculated from it
   };


struct  TEBSWADRes {
   long int		        adres_num[EEGBIODOC_MAXTRACKS]; // numerator
   long int		        adres_den[EEGBIODOC_MAXTRACKS]; // denominator
   };


struct  TEBAmpFiltChannel {
   unsigned long		low_pass_filter; 		// x10
   unsigned long		high_pass_filter; 		// x100
   unsigned char		notch_filter;			// 0 = FALSE, 1 = TRUE
   unsigned char		unused_byte;
   };


struct  TEBDispMontFiltCntrl {
   TEBAmpFiltChannel    signal [EEGBIODOC_MAXTRACKS];
   };


struct  TEBChanOp {
    short               a_index;                // a_index op b_index
    short               b_index;
    short               op;                     // only - (code number 1) supported
    };


struct  TEBXLChanOpCntrl {                      // defines master chanops operation
   short			    disp_chans;	            // number of display channels
   short				ref_type;		        // type of display reference used
   short				ref_chans;		        // number of amp chans used for average ref
   short				ref_parts[EEGBIODOC_MAXTRACKS]; // average ref amp channel index array
   short				a1_index;		        // standard reference channel amplifier
   short				a2_index;		        // channel array indexes
   TEBChanOp            channel[40];	        // individual channels
   };


struct  TEBDispMont {
   char					name[21];				// what user calls it, for ex. "Double banana"
   char					label[40][20];			// wave trace label strings
   unsigned short		default_label_flag[40];	// 0 = user label, 1 = auto
   unsigned short		enabled[40];			// 0 = not enabled, 1 = enabled
   unsigned short		slaved[40];			    // signal slaved to master signal
   short				scale[40];				// display scale set by montage
   short				color[40];				// signal color
   TEBXLChanOpCntrl     chanops;                // describes individual signals
   TEBDispMontFiltCntrl filters;     			// display waves are filtered
   };


struct  TEBFileSeq {
   unsigned int		    file_type;				// what type of file this is
   unsigned int		    video_type;			    // what type of video this is
   unsigned short	    seq_number;			    // file/tape number 1,2,3...
   char			        blscid[38+1];			// assigned by P&TI
   char			        test_id[7+1];			// assigned by user
   unsigned char		spare[16];				// future expansion
   };


struct  TEBDataStart {
   short                mrk_compression_mode;	// algorithm used to compress data
   short                mrk_points_per_epoch;	// number of samples in epoch of compressed data
    };


struct  TEBComment {
   unsigned long		marker_comment_time;	// time comment was started, not written to disk
   unsigned short	    marker_comment_code;	// identifies which system comment it is
   unsigned char		marker_comment_type;	// identifies who created comment
   char			        marker_comment_text[31];// the actual comment ASCIIZ text
   };


//----------------------------------------------------------------------------
                                        // EEG record defines

class   TEBEegData
{
public:

    TEBEegData ( int n )                    {
                                            numvalues   = n <= 0 ? 0 : n;
                                            sizeofpn    = sizeof ( point_num );
                                            sizeofv     = numvalues * sizeof ( *value );
                                            sizeofthis  = sizeofpn + sizeofv;
                                            }

    int                 SizeOf ()   const   { return sizeofthis; }
    friend  std::istream&   operator >> ( std::istream& is, TEBEegData &eegd );


    unsigned long       point_num;      // TF number, starting from 1
    short               value[ EEGBIODOC_MAXTRACKS ];   // allow up to 256 channels in file (128 currently)

protected:

    int                 numvalues;      // in the array
    int                 sizeofthis;     // total size of record on disk
    int                 sizeofpn;       // first part of record (for optimization)
    int                 sizeofv;        // second part
};


//----------------------------------------------------------------------------
                                        // Cartool own structure to store sequences infos
class   TEBSeq
{
public:
    LONGLONG        DataOrg;
    ULONG           NumTimeFrames;
    ULONG           StartingPacketNumber;
    ULONG           EndingPacketNumber;
    TDateTime       DateTime;
    int             CompressionMode;
};


EndBytePacking

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class   TEegBioLogicDoc :   public  TTracksDoc
{
public:
                    TEegBioLogicDoc ( owl::TDocument *parent = 0 );


    bool            CanClose        ();
    bool            Close           ();
    bool            IsOpen          ()  final           { return InputStream != 0; }
    bool            Open            ( int mode, const char *path = 0 );


    static bool     ReadFromHeader  ( const char* file, ReadFromHeaderType what, void* answer );
    void            ReadRawTracks   ( long tf1, long tf2, TArray2<float> &buff, int tfoffset = 0 )  final;
    bool            UpdateSession   ( int newsession )  final;


protected:

    owl::TInStream* InputStream;

    TArray1<float>  Tracks;
    TArray1<double> Gains;
    long            StartingPacketNumber;
    TEBEegData*     FileBuff;
    int             BuffSize;
    TArray1<TEBSeq> Sequences;
    int             CompressionMode;


    void            ReadNativeMarkers   ()  final;
    bool            SetArrays           ()  final;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
