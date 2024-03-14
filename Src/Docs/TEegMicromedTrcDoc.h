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
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


/*******************************************************************
******************** CONSTANTS FOR ARRAY DIMENSION *****************
*******************************************************************/

#define MAX_LAB_3			256

#define LEN_MONTAGE_3		3072

#define MAX_NOTE_2			100
#define LEN_MONTAGE_2		1600


#define MAX_CAN				256		//	Maximum Number of Channels to be Acquired
#define MAX_DERIV			128		//	Maximum Number of Derivations to be Displayed
#define MAX_NOTE			200
#define MAX_SAMPLE			128
#define MAX_HISTORY			30
#define MAX_LAB				640		//	Maximum Number of Labels
#define MAX_MONT			30
#define MAX_FLAG			100
#define MAX_SEGM			100
#define LEN_MONTAGE			4096
#define MAX_SEC				40
#define	MAX_EVENT			100
#define	MAX_VIDEO_FILE		1024
#define	MAX_TRIGGER			8192



typedef struct
{
	unsigned short int	NonInv;		//	This is for montages of Type 4
	unsigned short int	Inv;
}
Micromed_Double_Word;



/*******************************************************************
******************* STRUCTURE NECESSARY FOR TYPE 3 & 4 *************
*******************************************************************/


typedef unsigned short int Micromed_New_Code;


typedef struct
{
	char			Surname[22];
	char			Name[20];
	unsigned char	Month;
	unsigned char	Day;
	unsigned char	Year;
	unsigned char	Reserved[19];
}
Micromed_New_Patient_Data;



typedef struct
{
	unsigned char	Day;
	unsigned char	Month;
	unsigned char	Year;
}
Micromed_New_Date_Type;



typedef struct
{
	unsigned char	Hour;
	unsigned char	Min;
	unsigned char	Sec;
}
Micromed_New_Time_Type;



typedef struct
{
	char				Name[8];
	unsigned long int	Start_Offset;
	unsigned long int	Length;
}
Micromed_New_Descriptor;



typedef struct
{
	unsigned char		Status;
	unsigned char		Type;
	char				Positive_Input_Label[6];
	char				Negative_Input_Label[6];
	unsigned long int	Logic_Minimum;
	unsigned long int	Logic_Maximum;
	unsigned long int	Logic_Ground;
	long int			Physic_Minimum;
	long int			Physic_Maximum;
	unsigned short int	Measurement_Unit;
	unsigned short int	Prefiltering_HiPass_Limit;
	unsigned short int	Prefiltering_HiPass_Type;
	unsigned short int	Prefiltering_LowPass_Limit;
	unsigned short int	Prefiltering_LowPass_Type;
	unsigned short int	Rate_Coefficient;
	unsigned short int	Position;
	float				Latitude;
	float				Longitude;
	unsigned char		Maps;
	unsigned char		Average_Ref;
	char				Description[32];
	unsigned char		Reserved_2[38];
}
Micromed_New_Electrode;



typedef struct
{
	unsigned long int	Sample;
	char				Comment[40];
}
Micromed_New_Annotation;



typedef struct
{
	unsigned long int	Begin;
	unsigned long int	End;
}
Micromed_New_Marker_Pair;



typedef struct
{
	unsigned long int	Time;
	unsigned long int	Sample;
}
Micromed_New_Segment;



typedef struct
{
	unsigned char	Positive;
	unsigned char	Negative;
}
Micromed_New_Impedance;



typedef struct
{
	unsigned short int		Lines;
	unsigned short int		Sectors;
	unsigned short int		Base_Time;
	unsigned char			Notch;
	unsigned char			ViewReference;
	unsigned char			Colour[MAX_DERIV];
	unsigned char			Selection[MAX_DERIV];
	char					Description[64];
	Micromed_Double_Word	Inputs[MAX_DERIV];
	unsigned long int		HiPass_Filter[MAX_DERIV];
	unsigned long int		LowPass_Filter[MAX_DERIV];
	unsigned long int		Reference[MAX_DERIV];

	unsigned char			Free[1720];
}
Micromed_New_Montage;



typedef struct
{
	unsigned char	Undefined[10];
}
Micromed_New_Compression;



#define AVERAGE_FREE	82
typedef struct
{
	unsigned long int		Mean_Trace;					//	Number of Averaged Traces (Triggers)
	unsigned long int		Mean_File;					//	Number of Averaged Files
	unsigned long int		Mean_Prestim;				//	Pre Stim (msec.)
	unsigned long int		Mean_PostStim;				//	Post Stim (msec.)
	unsigned long int		Mean_Type;					//	0 = Not Weighted, 1 = Weighted
	unsigned short int		Correct_Answers;			//	Number of Sweep with Correct Answers
	unsigned short int      Wrong_Answers;				//	Number of Sweep with Wrong Answers
	unsigned short int		No_Answers;					//	Number of Sweep with No Answer
	float					Corr_Mean_Time;				//
	float					Corr_SD;					//
	float					Corr_Max_Time;				//
	float					Corr_Min_Time;				//
	float					Corr_Ratio;					//
	unsigned char			Free[AVERAGE_FREE];			//
}
Micromed_New_Average;



typedef struct
{
	unsigned char	Undefined[10];
}
Micromed_New_Free;



typedef unsigned long int Micromed_New_Sample;



typedef struct
{
	char						Description[64];
	Micromed_New_Marker_Pair	Selection[MAX_EVENT];
}
Micromed_New_Event;



typedef struct
{
	long int					Sample;
	short int					Type;
}
Micromed_New_Trigger;


/*******************************************************************
******************* STRUCTURE OF TYPE "4" FILES ********************
*******************************************************************/

typedef struct
{
	char			   			Title[32];
	char						Laboratory[32];
	Micromed_New_Patient_Data	Patient_Data;
	Micromed_New_Date_Type		Date;
	Micromed_New_Time_Type		Time;
	unsigned short int			Acquisition_Unit;
	unsigned short int    		Filetype;
	unsigned long int			Data_Start_Offset;
	unsigned short int			Num_Chan;
	unsigned short int	   		Multiplexer;
	unsigned short int	   		Rate_Min;
	unsigned short int			Bytes;
	unsigned short int			Compression;
	unsigned short int	   		Montages;
	unsigned long int			Dvideo_Begin;
	unsigned short int			MPEG_Difference;
	unsigned char			    Reserved_1[15];

	unsigned char			    Header_Type;

	Micromed_New_Descriptor		Code_Area;
	Micromed_New_Descriptor		Electrode_Area;
	Micromed_New_Descriptor		Note_Area;
	Micromed_New_Descriptor		Flag_Area;
	Micromed_New_Descriptor		Segment_Area;
	Micromed_New_Descriptor		B_Impedance_Area;
	Micromed_New_Descriptor		E_Impedance_Area;
	Micromed_New_Descriptor		Montage_Area;
	Micromed_New_Descriptor		Compression_Area;
	Micromed_New_Descriptor		Average_Area;
	Micromed_New_Descriptor		History_Area;
	Micromed_New_Descriptor		Digital_Video_Area;
	Micromed_New_Descriptor		EventA_Area;
	Micromed_New_Descriptor		EventB_Area;
	Micromed_New_Descriptor		Trigger_Area;

	unsigned char			    Reserved_2[224];
}
Micromed_Header_Type_4;



//*******************************************************************
//************** SET DESCRIPTORS FOR HEADER OF TYPE "4" *************
//*******************************************************************

/*void Set_Descriptors(Micromed_Header_Type_4* Head)
{

	Head->Code_Area.Name[0]='O';
	Head->Code_Area.Name[1]='R';
	Head->Code_Area.Name[2]='D';
	Head->Code_Area.Name[3]='E';
	Head->Code_Area.Name[4]='R';
	Head->Code_Area.Name[5]=' ';
	Head->Code_Area.Name[6]=' ';
	Head->Code_Area.Name[7]=' ';
	Head->Code_Area.Start_Offset=sizeof(Micromed_Header_Type_4);
	Head->Code_Area.Length=(unsigned long int)MAX_CAN*sizeof(Micromed_New_Code);

	Head->Electrode_Area.Name[0]='L';
	Head->Electrode_Area.Name[1]='A';
	Head->Electrode_Area.Name[2]='B';
	Head->Electrode_Area.Name[3]='C';
	Head->Electrode_Area.Name[4]='O';
	Head->Electrode_Area.Name[5]='D';
	Head->Electrode_Area.Name[6]=' ';
	Head->Electrode_Area.Name[7]=' ';
	Head->Electrode_Area.Start_Offset=Head->Code_Area.Start_Offset+Head->Code_Area.Length;
	Head->Electrode_Area.Length=(unsigned long int)MAX_LAB*sizeof(Micromed_New_Electrode);

	Head->Note_Area.Name[0]='N';
	Head->Note_Area.Name[1]='O';
	Head->Note_Area.Name[2]='T';
	Head->Note_Area.Name[3]='E';
	Head->Note_Area.Name[4]=' ';
	Head->Note_Area.Name[5]=' ';
	Head->Note_Area.Name[6]=' ';
	Head->Note_Area.Name[7]=' ';
	Head->Note_Area.Start_Offset=Head->Electrode_Area.Start_Offset+Head->Electrode_Area.Length;
	Head->Note_Area.Length=(unsigned long int)MAX_NOTE*sizeof(Micromed_New_Annotation);

	Head->Flag_Area.Name[0]='F';
	Head->Flag_Area.Name[1]='L';
	Head->Flag_Area.Name[2]='A';
	Head->Flag_Area.Name[3]='G';
	Head->Flag_Area.Name[4]='S';
	Head->Flag_Area.Name[5]=' ';
	Head->Flag_Area.Name[6]=' ';
	Head->Flag_Area.Name[7]=' ';
	Head->Flag_Area.Start_Offset=Head->Note_Area.Start_Offset+Head->Note_Area.Length;
	Head->Flag_Area.Length=(unsigned long int)MAX_FLAG*sizeof(Micromed_New_Marker_Pair);

	Head->Segment_Area.Name[0]='T';
	Head->Segment_Area.Name[1]='R';
	Head->Segment_Area.Name[2]='O';
	Head->Segment_Area.Name[3]='N';
	Head->Segment_Area.Name[4]='C';
	Head->Segment_Area.Name[5]='A';
	Head->Segment_Area.Name[6]=' ';
	Head->Segment_Area.Name[7]=' ';
	Head->Segment_Area.Start_Offset=Head->Flag_Area.Start_Offset+Head->Flag_Area.Length;
	Head->Segment_Area.Length=(unsigned long int)MAX_SEGM*sizeof(Micromed_New_Segment);

	Head->B_Impedance_Area.Name[0]='I';
	Head->B_Impedance_Area.Name[1]='M';
	Head->B_Impedance_Area.Name[2]='P';
	Head->B_Impedance_Area.Name[3]='E';
	Head->B_Impedance_Area.Name[4]='D';
	Head->B_Impedance_Area.Name[5]='_';
	Head->B_Impedance_Area.Name[6]='B';
	Head->B_Impedance_Area.Name[7]=' ';
	Head->B_Impedance_Area.Start_Offset=Head->Segment_Area.Start_Offset+Head->Segment_Area.Length;
	Head->B_Impedance_Area.Length=(unsigned long int)MAX_CAN*sizeof(Micromed_New_Impedance);

	Head->E_Impedance_Area.Name[0]='I';
	Head->E_Impedance_Area.Name[1]='M';
	Head->E_Impedance_Area.Name[2]='P';
	Head->E_Impedance_Area.Name[3]='E';
	Head->E_Impedance_Area.Name[4]='D';
	Head->E_Impedance_Area.Name[5]='_';
	Head->E_Impedance_Area.Name[6]='E';
	Head->E_Impedance_Area.Name[7]=' ';
	Head->E_Impedance_Area.Start_Offset=Head->B_Impedance_Area.Start_Offset+Head->B_Impedance_Area.Length;
	Head->E_Impedance_Area.Length=(unsigned long int)MAX_CAN*sizeof(Micromed_New_Impedance);

	Head->Montage_Area.Name[0]='M';
	Head->Montage_Area.Name[1]='O';
	Head->Montage_Area.Name[2]='N';
	Head->Montage_Area.Name[3]='T';
	Head->Montage_Area.Name[4]='A';
	Head->Montage_Area.Name[5]='G';
	Head->Montage_Area.Name[6]='E';
	Head->Montage_Area.Name[7]=' ';
	Head->Montage_Area.Start_Offset=Head->E_Impedance_Area.Start_Offset+Head->E_Impedance_Area.Length;
	Head->Montage_Area.Length=(unsigned long int)MAX_MONT*sizeof(Micromed_New_Montage);

	Head->Compression_Area.Name[0]='C';
	Head->Compression_Area.Name[1]='O';
	Head->Compression_Area.Name[2]='M';
	Head->Compression_Area.Name[3]='P';
	Head->Compression_Area.Name[4]='R';
	Head->Compression_Area.Name[5]='E';
	Head->Compression_Area.Name[6]='S';
	Head->Compression_Area.Name[7]='S';
	Head->Compression_Area.Start_Offset=Head->Montage_Area.Start_Offset+Head->Montage_Area.Length;
	Head->Compression_Area.Length=(unsigned long int)sizeof(Micromed_New_Compression);

	Head->Average_Area.Name[0]='A';
	Head->Average_Area.Name[1]='V';
	Head->Average_Area.Name[2]='E';
	Head->Average_Area.Name[3]='R';
	Head->Average_Area.Name[4]='A';
	Head->Average_Area.Name[5]='G';
	Head->Average_Area.Name[6]='E';
	Head->Average_Area.Name[7]=' ';
	Head->Average_Area.Start_Offset=Head->Compression_Area.Start_Offset+Head->Compression_Area.Length;
	Head->Average_Area.Length=sizeof(Micromed_New_Average);

	Head->History_Area.Name[0]='H';
	Head->History_Area.Name[1]='I';
	Head->History_Area.Name[2]='S';
	Head->History_Area.Name[3]='T';
	Head->History_Area.Name[4]='O';
	Head->History_Area.Name[5]='R';
	Head->History_Area.Name[6]='Y';
	Head->History_Area.Name[7]=' ';
	Head->History_Area.Start_Offset=Head->Average_Area.Start_Offset+Head->Average_Area.Length;
	Head->History_Area.Length=(unsigned long int)MAX_SAMPLE*sizeof(Micromed_New_Sample)+(unsigned long int)MAX_HISTORY*sizeof(Micromed_New_Montage);

	Head->Digital_Video_Area.Name[0]='D';
	Head->Digital_Video_Area.Name[1]='V';
	Head->Digital_Video_Area.Name[2]='I';
	Head->Digital_Video_Area.Name[3]='D';
	Head->Digital_Video_Area.Name[4]='E';
	Head->Digital_Video_Area.Name[5]='O';
	Head->Digital_Video_Area.Name[6]=' ';
	Head->Digital_Video_Area.Name[7]=' ';
	Head->Digital_Video_Area.Start_Offset=Head->History_Area.Start_Offset+Head->History_Area.Length;
	Head->Digital_Video_Area.Length=(unsigned long int)MAX_VIDEO_FILE*sizeof(Micromed_New_Sample);

	Head->EventA_Area.Name[0]='E';
	Head->EventA_Area.Name[1]='V';
	Head->EventA_Area.Name[2]='E';
	Head->EventA_Area.Name[3]='N';
	Head->EventA_Area.Name[4]='T';
	Head->EventA_Area.Name[5]=' ';
	Head->EventA_Area.Name[6]='A';
	Head->EventA_Area.Name[7]=' ';
	Head->EventA_Area.Start_Offset=Head->Digital_Video_Area.Start_Offset+Head->Digital_Video_Area.Length;
	Head->EventA_Area.Length=(unsigned long int)sizeof(Micromed_New_Event);

	Head->EventB_Area.Name[0]='E';
	Head->EventB_Area.Name[1]='V';
	Head->EventB_Area.Name[2]='E';
	Head->EventB_Area.Name[3]='N';
	Head->EventB_Area.Name[4]='T';
	Head->EventB_Area.Name[5]=' ';
	Head->EventB_Area.Name[6]='B';
	Head->EventB_Area.Name[7]=' ';
	Head->EventB_Area.Start_Offset=Head->EventA_Area.Start_Offset+Head->EventA_Area.Length;
	Head->EventB_Area.Length=(unsigned long int)sizeof(Micromed_New_Event);

	Head->Trigger_Area.Name[0]='T';
	Head->Trigger_Area.Name[1]='R';
	Head->Trigger_Area.Name[2]='I';
	Head->Trigger_Area.Name[3]='G';
	Head->Trigger_Area.Name[4]='G';
	Head->Trigger_Area.Name[5]='E';
	Head->Trigger_Area.Name[6]='R';
	Head->Trigger_Area.Name[7]=' ';
	Head->Trigger_Area.Start_Offset=Head->EventB_Area.Start_Offset+Head->EventB_Area.Length;
	Head->Trigger_Area.Length=(unsigned long int)MAX_TRIGGER*sizeof(Micromed_New_Trigger);

	return;
}
*/


//************************************************************************
//******************** READS HEADER OF TYPE "4" **************************
//************************************************************************

/*void Read_Header_4(FILE* f,
			Micromed_Header_Type_4*     Head,
			Micromed_New_Code			Code[],
			Micromed_New_Electrode      Electrode[],
			Micromed_New_Annotation     Note[],
			Micromed_New_Marker_Pair    Flag[],
			Micromed_New_Segment		Segment[],
			Micromed_New_Impedance		B_Impedance[],
			Micromed_New_Impedance		E_Impedance[],
			Micromed_New_Montage		Montage[],
			Micromed_New_Compression*   Compression,
			Micromed_New_Average*		Average,
			Micromed_New_Sample			History_Sample[],
			Micromed_New_Montage		History[],
			Micromed_New_Sample			Digital_Video[],
			Micromed_New_Event*			EventA,
			Micromed_New_Event*			EventB,
			Micromed_New_Trigger		Trigger[])
{
	unsigned short int	Amount,m;
	void* Address;

//************** Set "Head" Structure ******************************************************
	fseek(f,0,SEEK_SET);														//	reads
	fread(Head,sizeof(Micromed_Header_Type_4),1,f);								//	Head


//************** Set "Code" Structure ******************************************************
	fseek(f,Head->Code_Area.Start_Offset,SEEK_SET);								//	reads
	fread(Code,(unsigned short)Head->Code_Area.Length,1,f);						//	Code


//************** Set "Electrode" Structure *************************************************
	Amount=(unsigned short int)(Head->Electrode_Area.Length/(unsigned long)2);	//
	Address = &(Electrode[0]);													//
	fseek(f,Head->Electrode_Area.Start_Offset,SEEK_SET);						//	reads
	fread(Address,Amount,1,f);													//	Electrode
	Address = &(Electrode[MAX_LAB/2]);											//
	fread(Address,Amount,1,f);													//


//************** Set "Note" Structure ******************************************************
	fseek(f,Head->Note_Area.Start_Offset,SEEK_SET);								//	reads
	fread(Note,(unsigned short)Head->Note_Area.Length,1,f);						//	Note


//************** Set "Flag" Structure ******************************************************
	fseek(f,Head->Flag_Area.Start_Offset,SEEK_SET);								//	reads
	fread(Flag,(unsigned short)Head->Flag_Area.Length,1,f);						//	Flag


//************** Set "Segment" Structure ***************************************************
	fseek(f,Head->Segment_Area.Start_Offset,SEEK_SET);							//	reads
	fread(Segment,(unsigned short)Head->Segment_Area.Length,1,f);				//	Segment


//************** Set "B_Impedance" Structure ***********************************************
	fseek(f,Head->B_Impedance_Area.Start_Offset,SEEK_SET);						//	reads
	fread(B_Impedance,(unsigned short)Head->B_Impedance_Area.Length,1,f);		//	B_Impedance


//************** Set "E_Impedance" Structure ***********************************************
	fseek(f,Head->E_Impedance_Area.Start_Offset,SEEK_SET);						//	reads
	fread(E_Impedance,(unsigned short)Head->E_Impedance_Area.Length,1,f);		//	E_Impedance


//************** Set "Montage" Structure ***************************************************
	fseek(f,Head->Montage_Area.Start_Offset,SEEK_SET);							//
	for (m=0; m<Head->Montages; m++)											//	reads
	{																			//	Montage
		Address = &(Montage[m]);												//
		Amount=(unsigned short int)sizeof(Micromed_New_Montage);				//
		fread(Address,Amount,1,f);												//
	}																			//


//************** Set "Compression" Structure ***********************************************
	fseek(f,Head->Compression_Area.Start_Offset,SEEK_SET);						//	reads
	fread(Compression,(unsigned short)Head->Compression_Area.Length,1,f);		//	Compression


//************** Set "Average" Structure ***************************************************
	fseek(f,Head->Average_Area.Start_Offset,SEEK_SET);							//  reads
	fread(Average,(unsigned short)Head->Average_Area.Length,1,f);				//	Average


//************** Set "History" Structure ***************************************************
	fseek(f,Head->History_Area.Start_Offset,SEEK_SET);							//	reads
	fread(History_Sample,MAX_SAMPLE*sizeof(Micromed_New_Sample),1,f);			//	History_Sample

	Amount = (unsigned short int)sizeof(Micromed_New_Montage);					//
	for (m=0; History_Sample[m]!= (ULONG) -1; m++)										//	reads
	{																			//	History
		Address = &(History[m]);												//
		fread(Address,Amount,1,f);												//
	}																			//

//************** Set "Digital_Video" Structure *********************************************
	fseek(f,Head->Digital_Video_Area.Start_Offset,SEEK_SET);					//	reads
	fread(Digital_Video,(unsigned short)Head->Digital_Video_Area.Length,1,f);	//	Digital_Video


//************** Set "Event_A" Structure ***************************************************
	fseek(f,Head->EventA_Area.Start_Offset,SEEK_SET);							//	reads
	fread(EventA,(unsigned short)Head->EventA_Area.Length,1,f);					//	EventA


//************** Set "Event_B" Structure ***************************************************
	fseek(f,Head->EventB_Area.Start_Offset,SEEK_SET);							//	reads
	fread(EventB,(unsigned short)Head->EventB_Area.Length,1,f);					//	EventB


//************** Set "Trigger" Structure ***************************************************
	fseek(f,Head->Trigger_Area.Start_Offset,SEEK_SET);							//	reads
	fread(Trigger,(unsigned short)Head->Trigger_Area.Length,1,f);				//	Trigger

	return;
}
*/


EndBytePacking

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
                                        // Internal structure to browse the sequences
class   TEegMicromedSession
{
public:
    ULONG           DataOrg;
    ULONG           NumTimeFrames;
    ULONG           OffsetTimeFrames;
    TDateTime       DateTime;
};


//----------------------------------------------------------------------------

class   TEegMicromedTrcDoc	:	public  TTracksDoc
{
public:
                    TEegMicromedTrcDoc (owl::TDocument *parent = 0);


    bool            CanClose		();
    bool            Close			();
    bool            IsOpen			()	const		{ return NumElectrodes > 0; }
    bool            Open			( int mode, const char *path = 0 );
    static bool     ReadFromHeader	( const char* file, ReadFromHeaderType what, void* answer );

                                            // overriding virtual functions
    bool            SetArrays		();
    void            ReadRawTracks	( long tf1, long tf2, TArray2<float> &buff, int tfoffset = 0 );
    bool            UpdateSession	( int newsession );


protected:

    owl::TInStream* InputStream;

    TArray1<char>   Tracks;
    TArray1<double> Offset;
    TArray1<double> Gain;
    int             BuffSize;
    long            DataOrg;
    int             DataType;

    TArray1<TEegMicromedSession>	Sequences;

                                        // virtual TMarkers
    void            ReadNativeMarkers ();
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
