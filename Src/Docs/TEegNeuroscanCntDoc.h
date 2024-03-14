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

#include    "TTracksDoc.h"

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

constexpr char*     EEGNS_REV                   = "Version 3.0";
constexpr char      EEGNS_EEG                   = 0;
constexpr char      EEGNS_AVG                   = 1;

constexpr char      ContinousType2ExtraChannels = 0;
constexpr char      ContinousTypeEventTable1    = 1;
constexpr char      ContinousTypeEventTable2    = 3;

                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


typedef struct{
    char            rev[20];            /* Revision string                         */
    char            type;               /* File type AVG=1, EEG=0                  */
    char            id[20];             /* Patient ID                              */
    char            oper[20];           /* Operator ID                             */
    char            doctor[20];         /* Doctor ID                               */
    char            referral[20];       /* Referral ID                             */
    char            hospital[20];       /* Hospital ID                             */
    char            patient[20];        /* Patient name                            */
    short  int      age;                /* Patient Age                             */
    char            sex;                /* Patient Sex Male='M', Female='F'        */
    char            hand;               /* Handedness Mixed='M',Rt='R', lft='L'    */
    char            med[20];            /* Medications                             */
    char            classif[20];        /* Classification                          */
    char            state[20];          /* Patient wakefulness                     */
    char            label[20];          /* Session label                           */
    char            date[10];           /* Session date string                     */
    char            time[12];           /* Session time string                     */
    float           mean_age;           /* Mean age (Group files only)             */
    float           stdev;              /* Std dev of age (Group files only)       */
    short int       n;                  /* Number in group file                    */
    char            compfile[38];       /* Path and name of comparison file        */
    float           SpectWinComp;       /* Spectral window compensation factor     */
    float           MeanAccuracy;       /* Average respose accuracy                */
    float           MeanLatency;        /* Average response latency                */
    char            sortfile[46];       /* Path and name of sort file              */
    long            NumEvents;          /* Number of events in eventable           */
    char            compoper;           /* Operation used in comparison            */
    char            avgmode;            /* Set during online averaging             */
    char            review;             /* Set during review of EEG data           */
    short unsigned  nsweeps;            /* Number of expected sweeps               */
    short unsigned  compsweeps;         /* Number of actual sweeps                 */
    short unsigned  acceptcnt;          /* Number of accepted sweeps               */
    short unsigned  rejectcnt;          /* Number of rejected sweeps               */
    short unsigned  pnts;               /* Number of points per waveform           */
    short unsigned  nchannels;          /* Number of active channels               */
    short unsigned  avgupdate;          /* Frequency of average update             */
    char            domain;             /* Acquisition domain TIME=0, FREQ=1       */
    char            variance;           /* Variance data included flag             */
    unsigned short  rate;               /* D-to-A rate                             */
    double          scale;              /* scale factor for calibration            */
    char            veogcorrect;        /* VEOG corrected flag                     */
    char            heogcorrect;        /* HEOG corrected flag                     */
    char            aux1correct;        /* AUX1 corrected flag                     */
    char            aux2correct;        /* AUX2 corrected flag                     */
    float           veogtrig;           /* VEOG trigger percentage                 */
    float           heogtrig;           /* HEOG trigger percentage                 */
    float           aux1trig;           /* AUX1 trigger percentage                 */
    float           aux2trig;           /* AUX2 trigger percentage                 */
    short int       heogchnl;           /* HEOG channel number                     */
    short int       veogchnl;           /* VEOG channel number                     */
    short int       aux1chnl;           /* AUX1 channel number                     */
    short int       aux2chnl;           /* AUX2 channel number                     */
    char            veogdir;            /* VEOG trigger direction flag             */
    char            heogdir;            /* HEOG trigger direction flag             */
    char            aux1dir;            /* AUX1 trigger direction flag             */
    char            aux2dir;            /* AUX2 trigger direction flag             */
    short int       veog_n;             /* Number of points per VEOG waveform      */
    short int       heog_n;             /* Number of points per HEOG waveform      */
    short int       aux1_n;             /* Number of points per AUX1 waveform      */
    short int       aux2_n;             /* Number of points per AUX2 waveform      */
    short int       veogmaxcnt;         /* Number of observations per point - VEOG */
    short int       heogmaxcnt;         /* Number of observations per point - HEOG */
    short int       aux1maxcnt;         /* Number of observations per point - AUX1 */
    short int       aux2maxcnt;         /* Number of observations per point - AUX2 */
    char            veogmethod;         /* Method used to correct VEOG             */
    char            heogmethod;         /* Method used to correct HEOG             */
    char            aux1method;         /* Method used to correct AUX1             */
    char            aux2method;         /* Method used to correct AUX2             */
    float           AmpSensitivity;     /* External Amplifier gain                 */
    char            LowPass;            /* Toggle for Amp Low pass filter          */
    char            HighPass;           /* Toggle for Amp High pass filter         */
    char            Notch;              /* Toggle for Amp Notch state              */
    char            AutoClipAdd;        /* AutoAdd on clip                         */
    char            baseline;           /* Baseline correct flag                   */
    float           offstart;           /* Start point for baseline correction     */
    float           offstop;            /* Stop point for baseline correction      */
    char            reject;             /* Auto reject flag                        */
    float           rejstart;           /* Auto reject start point                 */
    float           rejstop;            /* Auto reject stop point                  */
    float           rejmin;             /* Auto reject minimum value               */
    float           rejmax;             /* Auto reject maximum value               */
    char            trigtype;           /* Trigger type                            */
    float           trigval;            /* Trigger value                           */
    char            trigchnl;           /* Trigger channel                         */
    short int       trigmask;           /* Wait value for LPT port                 */
    float           trigisi;            /* Interstimulus interval (INT trigger)    */
    float           trigmin;            /* Min trigger out voltage (start of pulse)*/
    float           trigmax;            /* Max trigger out voltage (during pulse)  */
    char            trigdir;            /* Duration of trigger out pulse           */
    char            Autoscale;          /* Autoscale on average                    */
    short int       n2;                 /* Number in group 2 (MANOVA)              */
    char            dir;                /* Negative display up or down             */
    float           dispmin;            /* Display minimum (Yaxis)                 */
    float           dispmax;            /* Display maximum (Yaxis)                 */
    float           xmin;               /* X axis minimum (epoch start in sec)     */
    float           xmax;               /* X axis maximum (epoch stop in sec)      */
    float           AutoMin;            /* Autoscale minimum                       */
    float           AutoMax;            /* Autoscale maximum                       */
    float           zmin;               /* Z axis minimum - Not currently used     */
    float           zmax;               /* Z axis maximum - Not currently used     */
    float           lowcut;             /* Archival value - low cut on external amp*/
    float           highcut;            /* Archival value - Hi cut on external amp */
    char            common;             /* Common mode rejection flag              */
    char            savemode;           /* Save mode EEG AVG or BOTH               */
    char            manmode;            /* Manual rejection of incomming data      */
    char            ref[10];            /* Label for reference electode            */
    char            Rectify;            /* Rectification on external channel       */
    float           DisplayXmin;        /* Minimun for X-axis display              */
    float           DisplayXmax;        /* Maximum for X-axis display              */
    char            phase;              /* flag for phase computation              */
    char            screen[16];         /* Screen overlay path name                */
    short int       CalMode;            /* Calibration mode                        */
    short int       CalMethod;          /* Calibration method                      */
    short int       CalUpdate;          /* Calibration update rate                 */
    short int       CalBaseline;        /* Baseline correction during cal          */
    short int       CalSweeps;          /* Number of calibration sweeps            */
    float           CalAttenuator;      /* Attenuator value for calibration        */
    float           CalPulseVolt;       /* Voltage for calibration pulse           */
    float           CalPulseStart;      /* Start time for pulse                    */
    float           CalPulseStop;       /* Stop time for pulse                     */
    float           CalFreq;            /* Sweep frequency                         */
    char            taskfile[34];       /* Task file name                          */
    char            seqfile[34];        /* Sequence file path name                 */
    char            SpectMethod;        /* Spectral method                         */
    char            SpectScaling;       /* Scaling employed                        */
    char            SpectWindow;        /* Window employed                         */
    float           SpectWinLength;     /* Length of window %                      */
    char            SpectOrder;         /* Order of Filter for Max Entropy method  */
    char            NotchFilter;        /* Notch Filter in or out                  */
    char            unused[11];         /* Free space                              */
    short           FspStopMethod;      /* FSP - Stoping mode                      */
    short           FspStopMode;        /* FSP - Stoping mode                      */
    float           FspFValue;          /* FSP - F value to stop terminate         */
    short int       FspPoint;           /* FSP - Single point location             */
    short int       FspBlockSize;       /* FSP - block size for averaging          */
    unsigned short  FspP1;              /* FSP - Start of window                   */
    unsigned short  FspP2;              /* FSP - Stop  of window                   */
    float           FspAlpha;           /* FSP - Alpha value                       */
    float           FspNoise;           /* FSP - Signal to ratio value             */
    short int       FspV1;              /* FSP - degrees of freedom                */
    char            montage[40];        /* Montage file path name                  */
    char            EventFile[40];      /* Event file path name                    */
    float           fratio;             /* Correction factor for spectral array    */
    char            minor_rev;          /* Current minor revision                  */
    short int       eegupdate;          /* How often incomming eeg is refreshed    */
    char            compressed;         /* Data compression flag                   */
    float           xscale;             /* X position for scale box - Not used     */
    float           yscale;             /* Y position for scale box - Not used     */
    float           xsize;              /* Waveform size X direction               */
    float           ysize;              /* Waveform size Y direction               */
    char            ACmode;             /* Set SYNAP into AC mode                  */
    unsigned char   CommonChnl;         /* Channel for common waveform             */
    char            Xtics;              /* Scale tool- 'tic' flag in X direction   */
    char            Xrange;             /* Scale tool- range (ms,sec,Hz) flag X dir*/
    char            Ytics;              /* Scale tool- 'tic' flag in Y direction   */
    char            Yrange;             /* Scale tool- range (uV, V) flag Y dir    */
    float           XScaleValue;        /* Scale tool- value for X dir             */
    float           XScaleInterval;     /* Scale tool- interval between tics X dir */
    float           YScaleValue;        /* Scale tool- value for Y dir             */
    float           YScaleInterval;     /* Scale tool- interval between tics Y dir */
    float           ScaleToolX1;        /* Scale tool- upper left hand screen pos  */
    float           ScaleToolY1;        /* Scale tool- upper left hand screen pos  */
    float           ScaleToolX2;        /* Scale tool- lower right hand screen pos */
    float           ScaleToolY2;        /* Scale tool- lower right hand screen pos */
    short int       port;               /* Port address for external triggering    */
    long            NumSamples;         /* Number of samples in continous file     */
    char            FilterFlag;         /* Indicates that file has been filtered   */
    float           LowCutoff;          /* Low frequency cutoff                    */
    short int       LowPoles;           /* Number of poles                         */
    float           HighCutoff;         /* High frequency cutoff                   */
    short int       HighPoles;          /* High cutoff number of poles             */
    char            FilterType;         /* Bandpass=0 Notch=1 Highpass=2 Lowpass=3 */
    char            FilterDomain;       /* Frequency=0 Time=1                      */
    char            SnrFlag;            /* SNR computation flag                    */
    char            CoherenceFlag;      /* Coherence has been  computed            */
    char            ContinousType;      /* Method used to capture events in *.cnt  */
    long            EventTablePos;      /* Position of event table                 */
    float           ContinousSeconds;   /* Number of seconds to displayed per page */
    long            ChannelOffset;      /* Block size of one channel in SYNAMPS    */
    char            AutoCorrectFlag;    /* Autocorrect of DC values                */
    unsigned char   DCThreshold;        /* Auto correct of DC level                */
    }   TNsSetup;


typedef struct {                        /* Electrode structure  ------------------- */
    char            lab[10];            /* Electrode label - last bye contains NULL */
    char            reference;          /* Reference electrode number               */
    char            skip;               /* Skip electrode flag ON=1 OFF=0           */
    char            reject;             /* Artifact reject flag                     */
    char            display;            /* Display flag for 'STACK' display         */
    char            bad;                /* Bad electrode flag                       */
    unsigned short int n;               /* Number of observations                   */
    char            avg_reference;      /* Average reference status                 */
    char            ClipAdd;            /* Automatically add to clipboard           */
    float           x_coord;            /* X screen coord. for 'TOP' display        */
    float           y_coord;            /* Y screen coord. for 'TOP' display        */
    float           veog_wt;            /* VEOG correction weight                   */
    float           veog_std;           /* VEOG std dev. for weight                 */
    float           snr;                /* signal-to-noise statistic                */
    float           heog_wt;            /* HEOG Correction weight                   */
    float           heog_std;           /* HEOG Std dev. for weight                 */
    short int       baseline;           /* Baseline correction value in raw ad units*/
    char            Filtered;           /* Toggel indicating file has be filtered   */
    char            Fsp;                /* Extra data                               */
    float           aux1_wt;            /* AUX1 Correction weight                   */
    float           aux1_std;           /* AUX1 Std dev. for weight                 */
    float           sensitivity;        /* electrode sensitivity                    */
    char            Gain;               /* Amplifier gain                           */
    char            HiPass;             /* Hi Pass value                            */
    char            LoPass;             /* Lo Pass value                            */
    unsigned char   Page;               /* Display page                             */
    unsigned char   Size;               /* Electrode window display size            */
    unsigned char   Impedance;          /* Impedance test                           */
    unsigned char   PhysicalChnl;       /* Physical channel used                    */
    char            Rectify;            /* Free space                               */
    float           calib;              /* Calibration factor                       */
    }   TNsElectLoc;


typedef struct {
    char            Teeg;               /* Either 1 or 2                           */
    long            Size;               /* Total length of all the events          */
    long            Offset;             /* Hopefully always 0                      */
    }   TNsTEeg;


typedef struct {
    unsigned short  StimType;           /* range 0-65535                           */
    unsigned char   KeyBoard;           /* range 0-11 corresponding to fcn keys +1 */
    char            KeyPad_Accept;      /* 0->3 range 0-15 bit coded response pad  */
                                        /* 4->7 values 0xd=Accept 0xc=Reject       */
    long            Offset;             /* file offset of event                    */
    }   TNsEvent1;


typedef struct{
    unsigned short  StimType;           /* range 0-65535                           */
    unsigned char   KeyBoard;           /* range 0-11 corresponding to fcn keys +1 */
    char            KeyPad_Accept;      /* 0->3 range 0-15 bit coded response pad  */
                                        /* 4->7 values 0xd=Accept 0xc=Reject       */
    long            Offset;             /* file offset of event                    */
    short           Type;
    short           Code;
    float           Latency;
    char            EpochEvent;
    char            Accept2;
    char            Accuracy;
    }   TNsEvent2;

/*
// Taken from  Robert Oostenveld,  see:  http://m50-042.azn.nl/mbfys/people/roberto/neuroscan/

#define ADINDEX(CHNL,PNT) ((float)(ad_buff[((CHNL)+(int)erp.nchannels*(PNT))]-erp.elect_tab[(CHNL)].baseline))

// Conversion for microvolts without channel-specific calibration
#define GETADVAL(CHNL,PNT) ((ADINDEX(CHNL,PNT)/204.8)*erp.elect_tab[CHNL].sensitivity)

// Complete conversion to microvolts
#define GETADuV(CHNL,PNT)  (GETADVAL(CHNL,PNT)*erp.elect_tab[(CHNL)].calib)

// Conversion from microvolts back to AD raw values
#define PUTADVAL(CHNL,PNT) ((elect[CHNL]->v[PNT]*204.8)/erp.elect_tab[CHNL].sensitivity)

// Conversion for scaled ad value  with calibration
#define GETADRAW(CHNL,PNT) ((ADINDEX(CHNL,PNT))*erp.elect_tab[(CHNL)].calib)

// Conversion for scaled int value  with baseline but no calibration
#define GETADINT(CHNL,PNT) ((ADINDEX(CHNL,PNT))/204.8)

#define GETVAL(CHNL,PNT) ((elect[CHNL]->v[PNT]/(float)erp.elect_tab[CHNL].n)*erp.elect_tab[CHNL].calib)

#define GETRVAL(CHNL,PNT) ((result[CHNL]->v[PNT]/(float)rslterp.elect_tab[CHNL].n)*rslterp.elect_tab[CHNL].calib)

#define GETCVAL(CHNL,PNT) ((compelect[CHNL]->v[PNT]/(float)comperp.elect_tab[CHNL].n)*comperp.elect_tab[CHNL].calib)
*/


EndBytePacking

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

class   TEegNeuroscanCntDoc :   public  TTracksDoc
{
public:
                    TEegNeuroscanCntDoc ( owl::TDocument *parent = 0 );


    bool            CanClose        ();
    bool            Close           ();
    bool            IsOpen          ()  const           { return NumElectrodes > 0; }
    bool            Open            ( int mode, const char *path = 0 );
    static bool     ReadFromHeader  ( const char* file, ReadFromHeaderType what, void* answer );

                                            // overriding virtual functions
    bool            SetArrays       ();
    void            ReadRawTracks   ( long tf1, long tf2, TArray2<float> &buff, int tfoffset = 0 );


protected:

    owl::TInStream* InputStream;

    long            DataOrg;

    int             NumElectrodesInFile;
    TArray1<float>  Tracks;
    TArray1<short>  FileBuff;
    int             BuffSize;
    int             NumEvents;
    TArray1<double> Gains;
    TArray1<double> Zeros;
                                        // virtual TMarkers
    void            ReadNativeMarkers ();
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
