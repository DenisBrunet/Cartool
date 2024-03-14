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

#include    "System.h"
                                        // These structs / classes need to be byte-aligned for proper read/write to file
BeginBytePacking


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// (c) Copyright, 1986-1995
// Biomedical Imaging Resource
// Mayo Foundation

                                        // Acceptable values for hdr.dime.datatype

#define DT_UNKNOWN              0
#define DT_BINARY               1
#define DT_UNSIGNED_CHAR        2
#define DT_SIGNED_SHORT         4
#define DT_SIGNED_INT           8
#define DT_FLOAT                16
#define DT_COMPLEX              32
#define DT_DOUBLE               64

                                        // these 2 don't exist, but are used by AIR...
                                        // from 0..65535
#define DT_UNSIGNED_SHORT       5
                                        // from -32767..32767
#define DT_SIGNED_SHORT_POS     6

                                        // possible values from hdr.hist.orient
#define DT_TRANSVERSE_UNFLIPPED 0
#define DT_CORONAL_UNFLIPPED    1
#define DT_SAGITTAL_UNLFIPPED   2
#define DT_TRANSVERSE_FLIPPED   3
#define DT_CORONAL_FLIPPED      4
#define DT_SAGITTAL_FLIPPED     5


struct header_key               /* header_key        */
 {                                   /* off + size*/
     int        sizeof_hdr;          /* 0 + 4     */
     char       data_type[10];       /* 4 + 10    */
     char       db_name[18];         /* 14 + 18   */
     int        extents;             /* 32 + 4    */
     short int  session_error;       /* 36 + 2    */
     char       regular;             /* 38 + 1    */
     char       hkey_un0;            /* 39 + 1    */
 };                                  /* total=40  */


struct image_dimension          /* image_dimension   */
 {                                   /* off + size*/
     short int  dim[8];              /* 0 + 16    */
     char       vox_units[4];        /* 16 + 4    */
     char       cal_units[8];        /* 20 + 4    */
     short int  unused1;             /* 24 + 2    */
     short int  datatype;            /* 30 + 2    */
     short int  bitpix;              /* 32 + 2    */
     short int  dim_un0;             /* 34 + 2    */
     float      pixdim[8];           /* 36 + 32   */
		     /*
			     pixdim[] specifies the voxel dimensions:
			     pixdim[1] - voxel width
			     pixdim[2] - voxel height
			     pixdim[3] - interslice distance
				     ..etc
		     */
     float      vox_offset;          /* 68 + 4    */
     float      funused1;            /* 72 + 4    */
     float      funused2;            /* 76 + 4    */
     float      funused3;            /* 80 + 4    */
     float      cal_max;             /* 84 + 4    */
     float      cal_min;             /* 88 + 4    */
     int        compressed;          /* 92 + 4    */
     int        verified;            /* 96 + 4    */
     int        glmax, glmin;        /* 100 + 8   */
 };


struct data_history             /* data_history       */
 {                                   /* off + size*/
     char       descrip[80];         /* 0 + 80    */
     char       aux_file[24];        /* 80 + 24   */
     char       orient;              /* 104 + 1   */
     char       originator[10];      /* 105 + 10  */
     char       generated[10];       /* 115 + 10  */
     char       scannum[10];         /* 125 + 10  */
     char       patient_id[10];      /* 135 + 10  */
     char       exp_date[10];        /* 145 + 10  */
     char       exp_time[10];        /* 155 + 10  */
     char       hist_un0[3];         /* 165 + 3   */
     int        views;               /* 168 + 4   */
     int        vols_added;          /* 172 + 4   */
     int        start_field;         /* 176 + 4   */
     int        field_skip;          /* 180 + 4   */
     int        omax, omin;          /* 184 + 8   */
     int        smax, smin;          /* 192 + 8   */
 };                                  /* total=200 */


struct dsr                      /* dsr               */
 {                                   /* off + size*/
     struct     header_key      hk;  /* 0 + 40    */
     struct     image_dimension dime;/* 40 + 108  */
     struct     data_history    hist;/* 148 + 200 */
 };                                  /* total=348 */


/*
 See thorough explanations here:    http://www.grahamwideman.com/gw/brain/analyze/formatdoc.htm
                                    http://eeg.sourceforge.net/mri_orientation_notes.html
                                    http://eeg.sourceforge.net/MJenkinson_coordtransforms.pdf

 Comments:
	struct header_key
		int sizeof_header   /* must indicate size of header file * /
		int extants;        /* should be 16384 * /
		char regular;       /* 'r' * /


	struct image_dimension struct decribes the organization and
	side of images. These elements enable IO routines to reference
	images by volume and slice number.

		short int dim[]  /* array of image dimensions * /
			dim[0]        /* number of dimensions; usually 4 * /
			dim[1]        /* image width * /
			dim[2]        /* image height * /
			dim[3]        /* volume depth * /
			dim[4]        /* volumes in file * /

		char vox_units[4] /* labels voxerl spatial unit * /
		char cal_units[4] /* labels voxel calibration unit * /
		short int datatype /* Acceptable values are * /

#define DT_NONE				0
#define DT_UNKNOWN			0
#define DT_BINARY			1
#define DT_UNSIGNED_CHAR	2
#define DT_SIGNED_SHORT		4
#define DT_SIGNED_INT		8
#define DT_FLOAT			16
#define DT_COMPLEX			32
#define DT_DOUBLE			64
#define DT_RGB				128
#define DT_ALL				255

		short int bitpix     /* bits per pixel * /
		float pixdim[]  /* parallel array to dim giving voxel dimensions
		                   in each dimension * /
		         pixdim[1]  /* voxel width * /
		         pixdim[2]  /* voxel height * /
		         pixdim[3]  /* voxel depth or slice thickness * /

		float vox_offset; /* byte offset in the .img file at which
		                     voxels start. If value is negative
		                     specifies that the absolute value
		                     is applied for every image in the file. * /

		float cal_max, cal_min /* specify range of calibration values * /
		int glmax, glmin    /* the max and min values for entire data set * /


The data_history substructure is not required, but the 'orient' element
is used to indicate individual slice orientation and determines whether
the ANALYZE 'Movie' program will attempt to flip the images before
displaying a movie sequence.
	orient:
			0 - transverse unflipped
			1 - coronal unflipped
			2 - sagittal unflipped
			3 - transverse flipped
			4 - coronal flipped
			5 - sagittal flipped
*/

/*
        SPM Modifications
        =================

        originator

SPM uses one of the Analyze header fields in an unorthodox way. This is the Originator field.
In the basic format, this is meant to be 10 bytes of text. In SPM, this space is used to contain
three short (two byte) integers. These numbers describe the current centre or 'Origin' of the image.
Specifically, they give the coordinate of the central voxel, in voxels, in X, then Y then Z.

        scalefactor

The field in the header that SPM uses for the scale factor is image_dimension.funused1

        .mat file

This simply contains a 4x4 affine transformation matrix in a variable M.
These files are normally generated by the realignment' and coregistration' modules.
What these matrixes contain is a mapping from the voxel coordinates (x0,y0,z0)
(where the first voxel is at coordinate (1,1,1)), to coordinates in millimeters (x1,y1,z1).
By default, the the new coordinate system is derived from the origin and vox fields of the image header.

        x1 = M(1,1)*x0 + M(1,2)*y0 + M(1,3)*z0 + M(1,4)
        y1 = M(2,1)*x0 + M(2,2)*y0 + M(2,3)*z0 + M(2,4)
        z1 = M(3,1)*x0 + M(3,2)*y0 + M(3,3)*z0 + M(3,4) 
*/


//----------------------------------------------------------------------------
                                        // End of Cartool wrapping
//----------------------------------------------------------------------------


EndBytePacking
