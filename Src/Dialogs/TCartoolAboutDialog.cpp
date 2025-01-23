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

#include    <stdio.h>

#include    <owl/pch.h>
#include    <owl/version.h>
#include    <owl/window.h>
#include    <owl/module.h>
#include    <owl/dialog.h>
#include    <owl/static.h>

#pragma     hdrstop
//-=-=-=-=-=-=-=-=-

#include    "TCartoolAboutDialog.h"
#include    "..\..\Setup\GitWCRev.h"

#include    "Strings.Utils.h"
#include    "TGlobalOpenGL.h"
#include    "TBaseView.h"

#include    "TCartoolVersionInfo.h"
#include    <GL\gl.h>
#include    "mkl_types.h"

using namespace owl;

namespace crtl {

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        TCartoolAboutDialog::TCartoolAboutDialog ( TWindow* parent, TResId resId, TModule* module )
      : TDialog ( parent, resId, module )
{
}


//----------------------------------------------------------------------------
void    TCartoolAboutDialog::SetupWindow ()
{
                                        // Creating the actual window
TStatic*            nameCtrl        = new TStatic ( this, IDC_APPNAME,              256 );
TStatic*            branchCtrl      = new TStatic ( this, IDC_BRANCH,               256 );
TStatic*            versionCtrl     = new TStatic ( this, IDC_VERSION,              256 );
TStatic*            buildCtrl       = new TStatic ( this, IDC_BUILD,                256 );
TStatic*            builddateCtrl   = new TStatic ( this, IDC_BUILDDATE,            256 );
TStatic*            optimizationCtrl= new TStatic ( this, IDC_OPTIMIZATIONS,        256 );

TStatic*            owlnextversion  = new TStatic ( this, IDC_OWLNEXTVERSION,       256 );
TStatic*            owlnextbuild    = new TStatic ( this, IDC_OWLNEXTBUILD,         256 );
TStatic*            owlnextdate     = new TStatic ( this, IDC_OWLNEXTBUILDDATE,     256 );
TStatic*            owlnextlicence  = new TStatic ( this, IDC_OWLNEXTCOMPANYNAME,   256 );

TStatic*            oglversion      = new TStatic ( this, IDC_OGLVERSION,           256 );
TStatic*            glslversion     = new TStatic ( this, IDC_GLSLVERSION,          256 );
TStatic*            oglvendor       = new TStatic ( this, IDC_OGLVENDOR,            256 );
TStatic*            oglrenderer     = new TStatic ( this, IDC_OGLRENDERER,          256 );
TStatic*            oglprop1        = new TStatic ( this, IDC_OGLPROP1,             256 );
TStatic*            oglprop2        = new TStatic ( this, IDC_OGLPROP2,             256 );
//TStatic*          oglextensions   = new TStatic ( this, IDC_OGLEXTENSIONS,       1024 );


TDialog::SetupWindow ();


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Retrieve VERSIONINFO
TCartoolVersionInfo applVersion ( GetModule () );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char                name [ KiloByte ];
LPCTSTR             prodName        = 0;
LPCTSTR             arch            = 0;
LPCTSTR             build           = 0;
//LPCTSTR           runtime         = 0;


if ( applVersion.GetProductName ( prodName ) )
    StringCopy      ( name, prodName );
else                                    // Oops, provide a default title
    StringCopy      ( name, CartoolTitle );

                                        // Adding the current architecture to the name
if ( applVersion.GetArchitecture ( arch ) )
    StringAppend    ( name, " ", arch );

                                        // Adding build info (release/debug)
if ( applVersion.GetBuild ( build ) )
    StringAppend    ( name, " ", build );

//                                        // Adding run time library info
//if ( applVersion.GetRunTimeLibrary ( runtime ) ) {
//    AppendSeparator ( name, ", " );
//    StringAppend    ( name, runtime );
//    }
                                        // Finally showing the concatenated name
nameCtrl   ->SetText ( name );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char                optimization[ KiloByte ];
LPCTSTR             openmp          = 0;
LPCTSTR             instrset        = 0;
LPCTSTR             mkl             = 0;
LPCTSTR             dpiawareness    = 0;


ClearString ( optimization );

                                        // OpenMP info
if ( applVersion.GetOpenMP ( openmp ) )
    StringAppend    ( optimization, openmp );

                                        // Adding instruction set info
if ( applVersion.GetInstructionSet ( instrset ) ) {
    AppendSeparator ( optimization, ", " );
    StringAppend    ( optimization, instrset );
    }

                                        // Adding MKL library info
if ( applVersion.GetMKL ( mkl ) ) {
    AppendSeparator ( optimization, ", " );
    StringAppend    ( optimization, mkl );
    }

                                        // Adding DPI awareness info
if ( applVersion.GetDPIAwareness ( dpiawareness ) ) {
    AppendSeparator ( optimization, ", " );
    StringAppend    ( optimization, dpiawareness );
    }


if ( StringIsEmpty ( optimization ) )
    StringCopy  ( optimization, "<none>" );


optimizationCtrl->SetText ( optimization );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LPCTSTR             branch      = 0;
                                        // Get the branch name string
if ( applVersion.GetBranchName ( branch ) )
    branchCtrl->SetText ( branch );
else
    branchCtrl->SetText ( "" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LPCTSTR             prodVersion     = 0;
                                        // Get the product version string
if ( applVersion.GetProductVersion ( prodVersion ) )
    versionCtrl->SetText ( prodVersion );
else
    versionCtrl->SetText ( "" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // Git infos
LPCTSTR             prodRevision    = 0;
LPCTSTR             prodDate        = 0;


if ( applVersion.GetProductRevision ( prodRevision ) )
    buildCtrl->SetText ( prodRevision );
else
    buildCtrl->SetText ( "" );


if ( applVersion.GetProductDate ( prodDate ) )
    builddateCtrl->SetText ( prodDate );
else
    builddateCtrl->SetText ( "" );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // OwlNext infos
owlnextversion  ->SetText ( OWL_VERSION(OWL_FORMAT_VERSION_MAJOR_MINOR_RELEASE_STRING) );

owlnextbuild    ->SetText ( IntegerToString ( OWL_BUILD_REVISION ) );

owlnextdate     ->SetText ( OWL_BUILD_REVISION_DATE );

owlnextlicence  ->SetText ( OWL_COMPANYNAME );


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                                        // OpenGL infos
                                        // using the same window as in TGlobalOpenGL, just by safety
UseThisDC           pdc  ( CartoolObjects.CartoolMdiClient->GetHandle () );

TGlobalOpenGL       globopengl;         // although we create an object here, it is initialized only once

//globopengl.GLrc.GLize ( pdc );                // static
globopengl.GLrc.Set ( pdc, globopengl.GLpfd );  // non-static


char                buff [ KiloByte ];
char                buff2[ KiloByte ];


StringCopy      ( buff, (char*) glGetString ( GL_VERSION  ) );
oglversion->SetText ( buff );

StringCopy      ( buff, (char*) glGetString ( GL_SHADING_LANGUAGE_VERSION ) );
glslversion->SetText ( buff );

StringCopy      ( buff, (char*) glGetString ( GL_VENDOR   ) );
oglvendor->SetText ( buff );

StringCopy      ( buff, (char*) glGetString ( GL_RENDERER ) );
oglrenderer->SetText ( buff );


StringCopy      ( buff, globopengl.GLpfd.IsAccelerated  () ? "Accelerated"   : "Not accelerated",
                        ",  ",
                        globopengl.GLpfd.IsDoubleBuffer () ? "Double buffer" : "Single buffer" );
oglprop1->SetText ( buff );


StringCopy      ( buff, IntegerToString ( buff2, globopengl.GLpfd.GetColorBits ()     ), " b/pixel, "     );
StringAppend    ( buff, IntegerToString ( buff2, globopengl.GLpfd.GetDepthBits ()     ), " b/depth, "     );
StringAppend    ( buff, IntegerToString ( buff2, globopengl.GLpfd.GetStencilBits ()   ), " b/stencil, "   );
StringAppend    ( buff, IntegerToString ( buff2, globopengl.GLpfd.GetAccumBits ()     ), " accum.buff."   );
oglprop2->SetText ( buff );


//StringCopy      ( buff, "Extensions:\t", (char*) glGetString ( GL_EXTENSIONS ) );
//oglextensions->SetText ( buff );


//globopengl.GLrc.unGLize ();   // static
globopengl.GLrc.Reset ();       // non-static
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

}
